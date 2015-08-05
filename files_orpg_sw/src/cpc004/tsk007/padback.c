/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 21:49:25 $
 * $Id: padback.c,v 1.2 2002/11/26 21:49:25 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/************************************************************************
Module:         padback.c                 

Description:    This module contains a translation of the FORTRAN module
                A3CM28_PADBACK which is used to run length encode            
                missing radial data bins to zero. This routine is called      
                by run_length_encode.c for processing missing bins at
                the end of the radial.
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000
$Id: padback.c,v 1.2 2002/11/26 21:49:25 nolitam Exp $
************************************************************************/

/* ----------------------------------------------------------------------
   padback.c implements FORTRAN subroutine A3CM28_PADBACK in ANSI C 

   Parameter Translation Table
   FORTRAN Name   ANSI C Name    Type     Description
   --------------------------------------------------------------------
   BSTEP          buff_step      int      (In) Bin Increment
   BYTEFLG        byte_flag      int      (In) Padding char byte flag
   FINALIDX       final_index    int*     (In/Out) Index to next avbl word
   IENDIX         end_index      int      (In) Index to end of input buffer
   NUMBINS        num_bins       int      (In) Number of data bins
   OUTBUF         outbuf         short*   (In/Out) Output Buffer
   PBUFFIND       begin_index    int      (In) Index to beginning of buffer
   
   Local Variable Translation Table (initialized values in ()'s )
   FORTRAN Name   ANSI C Name    Type     Description
   --------------------------------------------------------------------
   F0             F0             short    Represents Hex F0
   F0F0           F0F0           short    Represents Hex F0F0
   F0_RUNS        F0_runs        int      Number of odd runs padded
   I              i              int      loop counter
   MAXRUN         max_run        int      Maximum number of bins in 1 byte
   ODD_RUNS       odd_runs       int      Number of bins above a multiple of 30
   PAD            pad            short    Padding character
   RGTBYTE        right_byte     int      Right byte value
   TODD_RUN[]     odd_run_table  short    Odd run table
   TPAD[]         tpad           short    table of Padding characters
   TRALN_BINS     trailing_bins  int      Number of missing bins at the end of
                                          the buffer that need to be padded

---------------------------------------------------------------------*/

#include "padback.h"

/************************************************************************
Description:    pad missing radial data bins with 0's (if needed) at
                the END of a radial
Input:          see the translation table above for input parameters
Output:         see the translation table above for output parameters
Returns:        the number of bytes encoded as 0       
Globals:        none
Notes:          the program is still being validated
************************************************************************/
int pad_back(int byte_flag, short* outbuf, int buff_step, 
   int begin_index, int end_index, int num_bins) {
   /* pack runs of zero level for bins of missing data at the end of
      a radial                                                          */

   int final_index=0;        /* Index to the next avbl word (return val)*/
   int i;                    /* loop iteration variable                 */
   short F0=(short)0x00F0;   /* pack 15 bins w/0's (diff from fortran)  */
   short F0F0=(short)0xF0F0; /* pack 30 bins w/0's (diff from fortran)  */
   int max_run=15;           /* maximun number runs of 0's per byte     */
   int right_byte=1;         /* right byte value                        */
   int trailing_bins;        /* number of missing bins @ end of buffer  */
                             /* that need to be padded                  */
   short pad=(short)0;       /* short integer pad (diff from fortran)   */
   int F0_runs=0;            /* number of odd runs padded               */
   int odd_runs=0;           /* number of bins above a multiple of 30   */
   short odd_run_table[30]={
      (short)0x0000, (short)0x1000,(short)0x1010,(short)0x2010,
      (short)0x3010, (short)0x4010,(short)0x5010,(short)0x6010,
      (short)0x7010, (short)0x8010,(short)0x9010,(short)0xA010,
      (short)0xB010, (short)0xC010,(short)0xD010,(short)0xE010,
      (short)0xF010, (short)0xF020,(short)0xF030,(short)0xF040,
      (short)0xF050, (short)0xF060,(short)0xF070,(short)0xF080,
      (short)0xF090, (short)0xF0A0,(short)0xF0B0,(short)0xF0C0,
      (short)0xF0D0, (short)0xF0E0};
   /* values in tpad are used to stuff ONE byte (different from fortran)*/
   short tpad[]={(short)0x00,  /* <- added for use in C */
      (short)0x10,(short)0x20,(short)0x30,(short)0x40,(short)0x50,
      (short)0x60,(short)0x70,(short)0x80,(short)0x90,(short)0xA0,
      (short)0xB0,(short)0xC0,(short)0xD0,(short)0xE0,(short)0xF0};
   int  TEST=FALSE;          /* test flag: set to TRUE for diagnostics  */
   
   /* calculate the number of missing data bins on end. The value
      (num_bins-end_index) is the total number of bins remaining from 
      the end of the good data to the end of the buffer. Division by 
      buff_step reduces the increment value                             */
   trailing_bins=(num_bins-end_index)/buff_step;
   if(TEST) {
      fprintf(stderr,"\nPadback Output\n");
      fprintf(stderr,"  Total number of bins in the buffer=%i\n",num_bins);
      fprintf(stderr,"  Index at the end of the good data =%i\n",end_index);
      fprintf(stderr,"  Buffer increment (buff_step)      =%i\n",buff_step);
      fprintf(stderr,"  #trailing bins after buff_step    =%i\n",trailing_bins);
      }
   
   /* set the 'current' outbuf pointer to final_index                   */
   final_index=begin_index;
   
   /* ADDED TO COVER THE CASE WHERE THE INDEX GOES FROM 0 TO 299 BUT THERE
   ARE 230 BINS. 
   if(num_bins-end_index==1) return(final_index); */  

   /* first, if there is the need to only pad ONE BYTE after the
   end of the good data...then process here. right_byte=1               */
   if(byte_flag==right_byte) {
      if(TEST) fprintf(stderr,"  begin first half/ONE byte processing \n");
      pad=0;   /* pad will take on the stuff value if missing bins occur*/
      /* if there are any missing bins after the end of good data->process*/
      if(trailing_bins>0) {
         /* if the number of missing bins are > 15 (max_run) then stuff
         the right byte with 0x00F0. Any remaining missing bins will be
         taken care of in the next section */
         if(trailing_bins>=max_run) {
            pad=F0;   /* pad now = 0x00F0 */
            trailing_bins=trailing_bins-15; /* decrement missing bin counter */
            if(TEST) {
               fprintf(stderr,"  trailing bins>=15: pad value=%2x\n",(unsigned short)pad);
               fprintf(stderr,"  remaining missing bins = %i\n",trailing_bins);
               }
            }
          else { /* if number of missing bins are < 15 then pick the stuff
            value out of tpad */
            pad=tpad[trailing_bins]; /* get 1 byte pad value */
            trailing_bins=0;     /* reset number of missing bins to 0 */
            if(TEST) {
               fprintf(stderr,"  trailing bins <15: pad value=%2x\n",(unsigned short)pad);
               fprintf(stderr,"  remaining missing bins = %i\n",trailing_bins);
               }
            } /* end of if(trailing_bins>=max_run) */
   
         } /* end of if(trailing_bins>0) */

      /* now that we have the proper value within 'pad'...we need to stuff it
      into the 2-byte value in outbuf by calling short_isbyte */
      short_isbyte(pad,&outbuf[final_index],right_byte);
      if(TEST) fprintf(stderr,"  *new value in outbuf[%i]=%02x\n",final_index,
         (unsigned short)outbuf[final_index]);
      final_index=final_index + 1; /* increment the current index by one */
      if(TEST) fprintf(stderr,"  new final_index value=%i\n",final_index);
      } /* end of if(byte_flag) */
   
   /* now store the remaining bins of missing data */
   if(trailing_bins!=0) {
      if(TEST) fprintf(stderr,"  Begin second half processing\n");
      /* calculate runs of 30 and odd bins */
      F0_runs = trailing_bins/30;
      if(TEST) fprintf(stderr,"  number of remaining bins %i/30=F0_runs(%i)\n",
         trailing_bins,F0_runs);
      odd_runs=fmod(trailing_bins,30);
      if(TEST) fprintf(stderr,"  number of odd bins mod(%i/30)=%i\n",trailing_bins,
         odd_runs);
      
      /* store all complete runs of 30 */
      for(i=0;i<F0_runs;i++) {
         outbuf[final_index+i-1]=F0F0;
         } /* end of for i loop */

      /* update buffer indices */
      final_index=final_index+F0_runs;
      if(TEST) fprintf(stderr,"  after storage F0_runs=%i...new final_index=%i\n",
         F0_runs,final_index);

      /* table look-up for any odd_runs */
      if(odd_runs>0) {
         if(TEST) fprintf(stderr,"  processing for odd_runs now. value=%02x\n",
            (unsigned short)odd_run_table[odd_runs]);
         outbuf[final_index]=odd_run_table[odd_runs];
         /* update index */
         final_index=final_index+1;
         if(TEST) fprintf(stderr,"  final index to be returned=%i\n",final_index);
         } /* end of if(odd_runs>0) */      
      
      } /* end of if(trailing_bins!=0) */

   return(final_index);
   }  /* end of pad_back */
 
