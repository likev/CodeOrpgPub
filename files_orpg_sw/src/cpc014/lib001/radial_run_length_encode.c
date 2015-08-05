/*
 * RCS info
 * $Author: dzittel $
 * $Locker:  $
 * $Date: 2005/02/17 16:03:38 $
 * $Id: radial_run_length_encode.c,v 1.2 2005/02/17 16:03:38 dzittel Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */



/************************************************************************
Module:         radial_run_length_encode.c

Description:    This module contains a translation of the FORTRAN module
                A3CM01.FTN which is used to compress data by a run length
                encoding scheme. With the use of additional routines such
                as padfront.c, padback.c and short_isbyte.c, a radial of
                16-level data can be compressed and returned in a buffer
                for stuffing into an ICD data layer.
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
Adapted by:	Dave Zittel, Meteorologist, ROC/Applications Branch
			walter.d.zittel@noaa.gov
		Version 1.0  October 2003

History:
	11/04/2004	SW CCR NA04-30810	Build8 changes

************************************************************************/

#include "saaprods_main.h"
#include "radial_run_length_encode.h"
#include "build_saa_color_tables.h"

/* -----------------------------------------------------------------------
   Radial Run Length Encoding Function
   Translated from FORTRAN Module A3CM01.FTN
   
   Parameter Translation Table
   FORTRAN Name   ANSI C Name    Type     Description
   -----------------------------------------------------------------------
   BUFFIND        buf_index      int      (In) Product buffer index
   BUFFSTEP       buff_step      int      (In) Bin Increment
   CLTABIND       ctable_index   int      (In) Color Table Index
   DELTA          angle_delta    int      (In) Angle Delta value
   ENDIX          end_index      int      (In) Index of the end of good
                                               data within inbuf   
   INBUFF         inbuf          short*   (In) Pointer to input buffer
                                               containing radial data
   NRLEB          num_rle_bytes  int      (Out) Total number of RLE'd
                                               bytes (function return)
   NUMBUFEL       num_data_bins  int      (In) Number of bins contained 
                                               within inbuf
   OUTBUFF        outbuf         short*   (Out) Output buffer
   START          start_angle    int      (In) Start Angle Assignment
   STARTIX        start_index    int      (In) Index of beginning of good
                                               data within inbuf
                                                  
   Local Variable Translation Table (initialized values in ()'s )
   FORTRAN Name   ANSI C Name    Type     Description
   -----------------------------------------------------------------------
   BYTEFLAG       byteflag       int      Byte flag
   FIRSTPIX       first_pix      int      boolean first pixel flag
   IBUFFIND       inbuf_index    int      Input buffer index
   LEFTBYTE       left_byte      int      Left Byte pad value (0)
   NEWINDEX       new_index      int      Index of next word in buffer
   NEWPIX         new_pixel      int      New Pixel
   NRLEW          num_rle_words  int      Number of RLE'd words 
   NSTARTIX       new_start_index int     New start index in inbuf
   OBUFFINC       obuf_increment int      Output Buffer Increment (1)   
   OLDPIX         old_pixel      int      Old Pixel   
   PADCNT         pad_cnt        int      Number of padded runs
   PBUFFIND       outbuf_index   int      Output buffer index
   RGTBYTE        rgt_byte       int      Right Byte pad value (1)
   RUN            run_index      int      Index of run_table
   RUNCOL         run_column     short    Run Column
   RUNINC         num_runs       int      Number of runs (1)   
   RUNLIM         run_limit      int      Run Limit (15)
   RUNTAB(RUNLIM) run_table      int[]    Array containing factors of 16
   SBUFFIND       saved_buf_index int     Saved Product buffer index
   STARTRUN       start_run      int      Start Run Position (1)
   STRTDECR       sub_from_start int      Number of bins to subtract from
                                          start bin after padding with 
                                          runs of zero
   ZEROPNT        zero_value     int      Used to Init values to zero (0)

   Variables Declared but not used within the FORTRAN Source Code
   FORTRAN Name                           Description
   -----------------------------------------------------------------------     
   NRLEBINC                               Not found beyond declaration
   NRLEWINC                               Not found beyond declaration
   PAD                                    Not found beyond declaration
   RHBYTES                                Not found beyond declaration
   BYTESPWD                               Not found beyond declaration
   LP10SP                                 Not found beyond declaration   
-----------------------------------------------------------------------*/

/************************************************************************
Description:    run length encode data within the radial structure and
                return RLE shorts within the output buffer.
Input:          see the translation table above for input parameters
Output:         see the translation table above for output parameters
Returns:        the number of bytes (from the input radial) that was     
                run length encoded.           
Globals:        none
Notes:          the program is still being validated
************************************************************************/

int radial_run_length_encode(int start_angle, int angle_delta, 
   short *inbuf, int start_index, int end_index, int num_data_bins, 
   int buff_step, int ctable_index, int buf_index, short *outbuf,
   int prod_id) {

   int rgt_byte=1;          /* Right Byte pad value (1)                 */
   int left_byte=0;         /* Left Byte pad value (0)                  */
   int start_run=1;         /* Start run position (1)                   */
   int run_limit=15;        /* Limits each run to a max of 15           */   
   int num_runs=1;          /* Number of runs (1)                       */
   short obuf_increment=1;  /* Output buffer increment (1)              */
   int zero_value=0;        /* Alias for the value 0                    */
   int saved_buf_index;     /* Saved Product Buffer index               */
   int new_index;           /* Index of the next word in the buffer     */
   int pad_cnt;             /* Number of padded runs                    */
   int sub_from_start;      /* num of bins to subtract from start       */
   int new_start_index;     /* New start index within the input buffer  */
   int new_pixel;           /* New pixel value                          */
   int old_pixel;           /* Old pixel value                          */
   int run_index;           /* Index used to access the run table       */
   short run_column;        /* Run column                               */
   int inbuf_index;         /* Index of the input buffer                */
   int byteflag;            /* Byte flag                                */
   int num_rle_words;       /* Number of run length encoded shorts      */
   int outbuf_index;        /* Index of the output buffer               */
   short first_pix;         /* Flag to indicate the 1st pixel in a run  */
   int num_rle_bytes;       /* Number of RLE bytes (RETURN VALUE)       */
   int TEST=FALSE;          /* Diagnostic Flag: set to TRUE for output  */
   int i;                   /* Loop variable                            */
   int color_indx;          /* index into color table from product array */
   int max_indx = 1023;     /* Maximum value of color table changed from
   				511 to 1023 for Build8		         */
                            /* Run table used for bit shifting          */
   int run_table[]={0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,
      240};
  
   /* test routines to test procedure input                             */
   if(TEST) {
      fprintf(stderr,"\nBegin Radial Run Length Encoding\n");
      fprintf(stderr,"  Start Angle:      %i or %4.1f degrees\n",start_angle,
         (float)start_angle/10.0);
      fprintf(stderr,"  Angle Delta:      %i or %4.1f degrees\n",angle_delta,
         (float)angle_delta/10.0);
      fprintf(stderr,"  Start Index:      %i\n",start_index);
      fprintf(stderr,"  End Index:        %i\n",end_index);
      fprintf(stderr,"  Num of Data Bins: %i\n",num_data_bins);
      fprintf(stderr,"  Buffer Increment: %i\n",buff_step);
      fprintf(stderr,"  Colortable Index: %i\n",ctable_index);
      fprintf(stderr,"  Buffer Index:     %i\n",buf_index);
      for(i=0;i<10;i++)
         fprintf(stderr,"%03u ",(unsigned short)inbuf[i]);
      fprintf(stderr,"\n");
      }

   /* ----------------------------------------------------------------- */
   /* We must first make accomodations for the header portion of the    */
   /* Run Length Encoded output buffer.                                 */
   /*                                                                   */
   /* Position 1: number of RLE halfwords in the radial                 */
   /* Position 2: Radial Start Angle                                    */
   /* Position 3: Radial Angle Delta                                    */
   /* Position 4: Begins RLE encoding to end of buffer                  */
   /*                                                                   */
   /* Each position is a short word (2-bytes). Hence, we'll need        */
   /* to cast some of the input into the proper size.                   */
   /*                                                                   */
   /* Make start angle and angle-delta assignments (first, save the     */
   /* product buffer index for future assignment in the product buffer  */
   /* of the number of run-length encoded words in this input buffer)   */
   
   num_rle_bytes=0;           /* Initialize #rle bytes accumulator      */
                              /* this is the future return value        */
   outbuf_index=buf_index;    /* align input buffer index w/output      */
   saved_buf_index=outbuf_index; /* save the buffer index to stuff #    */
                              /* of the RLE halfwords value later       */
   outbuf_index++;            /* increment the output buf index         */

   /* load the radial start angle and angle delta portion of the        */
   /* radial header                                                     */
   outbuf[outbuf_index++]=(short)start_angle; /* radial start angle     */
   outbuf[outbuf_index++]=(short)angle_delta; /* radial angle delta     */
   if(TEST) {
      fprintf(stderr,"  Word 1: start angle = %i\n",outbuf[saved_buf_index  ]);
      fprintf(stderr,"  Word 2: start angle = %i\n",outbuf[saved_buf_index+1]);
      fprintf(stderr,"  Word 3: angle delta = %i\n",outbuf[saved_buf_index+2]);
      }
  
   /* Pad the start of the runs with F0F0 if there are MISSING data     */
   /* bins before the start of good data. Function returns the number   */
   /* of padded words                                                   */
   pad_cnt=pad_front(start_index,buff_step,outbuf,outbuf_index,
      &sub_from_start);

   outbuf_index+=pad_cnt;  /* move output pointer to account for any    */
                           /* pad values                                */
   new_start_index=start_index - sub_from_start; /* move product pointer*/
                           /* to the beginning of the good data         */

   if(TEST) {
      fprintf(stderr,"  Output after padfront. Number of padded groups=%i\n",
         pad_cnt);
      fprintf(stderr,"  Updated index=%i\n",new_start_index);
      }
 
   /* ------- initialize data for doing rle of the GOOD data ---------- */

   byteflag=left_byte;
   first_pix=TRUE;
   run_index=zero_value;
   old_pixel=zero_value;
   new_pixel=zero_value;
   num_rle_words=zero_value;

   /* process all of the data designated as good data in the radial     */
   for(inbuf_index = new_start_index;inbuf_index<=end_index;
      inbuf_index+=buff_step) {
      color_indx = (inbuf[inbuf_index] < max_indx) ? inbuf[inbuf_index] : max_indx;
      
      /* perform color table lookup for this pixel                      */
      switch (prod_id)
        {
        case OSWACCUM: /* OSWACCUM */
          {
          new_pixel = lweqv_clr[color_indx];
          break;
          }
        case OSDACCUM: /* OSDACCUM */
          {
          new_pixel = ldpth_clr[color_indx];
          break;
          }
        case SSWACCUM:  /* SSWACCUM */
          {
          /* hi_sf_flg added for Build8  */
          if(hi_sf_flg)
             new_pixel = shweqv_clr[color_indx];
          else
             new_pixel = hweqv_clr[color_indx];
          break;
          }
        case SSDACCUM:  /* SSDACCUM */
          {
          /* hi_sf_flg added for Build8  */
          if(hi_sf_flg)
	     new_pixel = shdpth_clr[color_indx];
          else
             new_pixel = hdpth_clr[color_indx];
          break;
          }
        case USWACCUM:  /* USWACCUM */
          {
          if(hi_sf_flg)
             new_pixel = hweqv_clr[color_indx];
          else
             new_pixel = lweqv_clr[color_indx];
          break;
          }
        case USDACCUM:  /* USDACCUM */
          {
          if(hi_sf_flg)
             new_pixel = hdpth_clr[color_indx];
          else
             new_pixel = ldpth_clr[color_indx];
          break;
          }
        default:
          return (0);
        }
         
      if(TEST) fprintf(stderr,"Color Table Lookup for inbuf[%i]=%u is %u\n",
         inbuf_index,inbuf[inbuf_index],ldpth_clr[color_indx]);

      if(TEST) fprintf(stderr," index=%03i pixel value=%02x\n",inbuf_index,
         (unsigned short)new_pixel);

      /* first pixel in the buffer, or first pixel after a run of 15    */
      if(first_pix==TRUE) {
            /* process first pixel                                      */
            if(TEST) 
               fprintf(stderr,"-> this is the first element of a new run\n");
            run_index=start_run;    /* initialize to 1                  */
            old_pixel=new_pixel;    /* copy new pix code to holding vrbl*/
            first_pix=FALSE;        /* reset boolean                    */
            } /* end of if(first_pix) */
        else if(new_pixel==old_pixel) {
            /* process intermediate pixel                               */
            run_index+=num_runs;    /* increment runs of the same value */
            if(TEST) 
               fprintf(stderr,"-> the run continues. up to %i\n",run_index);
            } /* end of else if(newpix) */
        else if(byteflag==left_byte) {
            /* work on LEFT byte                                        */
            run_column=(short)(run_table[run_index]+ old_pixel);
            if(TEST) fprintf(stderr,"-> work on left byte. run_column=%04x\n",
               (unsigned short) run_column);
            short_isbyte(run_column,&outbuf[outbuf_index],left_byte);
            byteflag=rgt_byte;
            run_index=start_run;
            old_pixel=new_pixel;
            } /* end of if(byteflag) */ 
         else {
            /* work on RIGHT byte                                       */
            run_column=(short)(run_table[run_index]+ old_pixel);
            if(TEST) fprintf(stderr,"-> work on right byte. run_column=%04x\n",
               (unsigned short) run_column);
            short_isbyte(run_column,&outbuf[outbuf_index],rgt_byte);
            byteflag=left_byte;
            outbuf_index+=obuf_increment;
            run_index=start_run;
            old_pixel=new_pixel;
            } /* end of final else */

         /* run of 15 pixels is detected                                */
         if(run_index==run_limit) {
            if(TEST) fprintf(stderr,"-> Run of 15 Detected!\n");
            run_column=(short)(run_table[run_index] + old_pixel);
            if(byteflag==left_byte) {
                  short_isbyte(run_column,&outbuf[outbuf_index],left_byte);
                  byteflag=rgt_byte;
                  } /* end of if(byteflag) */
              else {
                  short_isbyte(run_column,&outbuf[outbuf_index],rgt_byte);
                  outbuf_index+=obuf_increment;                  
                  byteflag=left_byte;
                  }  /* end of else */
            
            /* reset for next pass                                      */
            run_index=zero_value;
            first_pix=TRUE;

            } /* end of if(run_index) */
      
      } /* end of for inbuf_index loop */

   /* end of input buffer processing */
   
   /* set up byteflags for padback routine.                             */ 
   /* flags and pointers point to the next available storage location   */
   
   if(first_pix==FALSE) {
      run_column=(short)(run_table[run_index]+old_pixel);
      if(byteflag==left_byte) {
            short_isbyte(run_column,&outbuf[outbuf_index],left_byte);
            byteflag=rgt_byte;
            }
         else {
            short_isbyte(run_column,&outbuf[outbuf_index],rgt_byte);
            byteflag=left_byte;
            outbuf_index+=obuf_increment; 
            }
      } /* end if(first_pix) */

   /* now do the end processing -- pack any needed runs of zero level   */
   /* to account for bins of missing data. The index passed is the next */   
   /* available one to use                                              */

   new_index=pad_back(byteflag,outbuf,buff_step,outbuf_index,end_index,
      num_data_bins);
      
   /* now calculate the number of rle bytes and #of rle words           */
   num_rle_words=new_index-saved_buf_index-3;
   
   /* note that the num_rle_words does not include the 3 words before   */
   /* the rle starts, but these words are included in the byte count    */
   
   num_rle_bytes=(new_index-saved_buf_index)*2;
   
   /* assign the number of run length encoded words to the              */
   /* appropriate position in the radial header                         */
   outbuf[saved_buf_index]=(short)num_rle_words;

   /* completed run length encoding processing for this buffer          */
   if (TEST) {
       fprintf(stderr,"returning %i bytes from radial_run_length_encode\n",
      num_rle_bytes);
      }
   return(num_rle_bytes);
} /* end of radial_run_length_encode */

