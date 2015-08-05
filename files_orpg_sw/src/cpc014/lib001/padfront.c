/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:25:03 $
 * $Id: padfront.c,v 1.1 2004/01/21 20:25:03 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/************************************************************************
Module:         padfront.c                 

Description:    This module contains a translation of the FORTRAN module
                A3CM27_PADFRONT which is used to run length encode            
                missing radial data bins to zero. This routine is called      
                by run_length_encode.c and is followed by padback.c.
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000

************************************************************************/

#include "padfront.h"

/* -----------------------------------------------------------------------
   padfront.c implements FORTRAN subroutine A3CM27_PADFRONT in ANSI C 

   Parameter Translation Table
   FORTRAN Name   ANSI C Name    Type     Description
   --------------------------------------------------------------------
   BSTEP          buff_step      int      (In) Bin Increment (stuff every
                                          bin, every other bin, etc)
   OIDX           outbuf_index   int      (In) Output Buffer Index
   OUTBUF         outbuf         short*   (In/Out) Output Buffer
   PADCNT         pad_cnt        int      (Out) Number of "padded runs"
                                          (2-byte words stuffed into outbuf)
                                          function return value
   STARTIX        start_index    int      (In) Index to beginning of the
                                          output buffer
   STRTDECR       sub_from_start int*     (In/Out) Number of bins to subtract
                                          from the start bin after padding
                                          with runs of 0


   Local Variable Translation Table (initialized values in ()'s )
   FORTRAN Name   ANSI C Name    Type     Description
   -----------------------------------------------------------------------
   F0_BINS        F0_bins        int      Number of bins to fill divided by
                                          the increment step (buff_step)
   F0_RUNS        F0_runs        int      Even number of 2-byte F0F0 words
                                          stuffed into outbuf
   F0F0           F0F0           short    Pad value for missing data
                                          (30 bins = 0xF0F0)
   I              i              int      loop counter
   ODD_BINS       odd_bins       int      Number of odd bins between 0
                                          and 30
   PADD_RUN       pad_run        int      Holds the hex value of the number
                                          of missing bins above a multiple
                                          of 30 bins
   TODD_RUN[]     odd_run_table  short    Odd run table - contains hex values
                                          to be stored in pad_run in the event
                                          that the number of missing bins is
                                          not an even multiple of 30 (F0F0) bins

------------------------------------------------------------------------*/

/************************************************************************
Description:    pad missing radial data bins with 0's (if needed) at
                the beginning of a radial
Input:          see the translation table above for input parameters
Output:         see the translation table above for output parameters
Returns:        the number of bytes encoded as 0       
Globals:        none
Notes:          the program is still being validated
************************************************************************/

int pad_front(int start_index, int buff_step, short* outbuf,
   int outbuf_index, int* sub_from_start) {
   /* Pad the start of the runs with F0F0 if there are missing data
   bins before the start of good data */

   int pad_cnt=0;    /* number of bytes encoded (return value)          */
   int F0_bins;      /* number of bins to fill                          */ 
   int F0_runs;      /* number of 2-byte F0F0 words stuffed in outbuf   */
   int odd_bins;     /* number of odd bins between 0 and 30             */
   short pad_run;    /* holds value from the odd_run_table              */
   int i;            /* loop iteration variable                         */
   short F0F0=(short)0xF0F0; /* holds hex 0xF0F0 (30 bins worth of 0's) */
   short odd_run_table[]={
      (short)0x0000, (short)0x1000,(short)0x1010,(short)0x2010,
      (short)0x3010, (short)0x4010,(short)0x5010,(short)0x6010,
      (short)0x7010, (short)0x8010,(short)0x9010,(short)0xA010,
      (short)0xB010, (short)0xC010,(short)0xD010,(short)0xE010,
      (short)0xF010, (short)0xF020,(short)0xF030,(short)0xF040,
      (short)0xF050, (short)0xF060,(short)0xF070,(short)0xF080,
      (short)0xF090, (short)0xF0A0,(short)0xF0B0,(short)0xF0C0,
      (short)0xF0D0, (short)0xF0E0};
   int TEST=FALSE; /* boolean test flag                                 */

   *sub_from_start=0;  /* initialize value to subtract from the start   */

   /* display input parameters before processing                        */
   if(TEST) {
      fprintf(stderr,"Padfront Input Parameter Diagnostics:\n");
      fprintf(stderr,"  start_index:           %i\n",start_index);
      fprintf(stderr,"  buff_step:             %i\n",buff_step);
      fprintf(stderr,"  outbuf_index:          %i\n",outbuf_index);
      }

   /* calculate number of runs of 30 and the odd bins of missing data   */

   /* note that work is done in the function ONLY if missing data is    */
   /* located at the beginning of a radial. if the start_index (which   */
   /* points to the first bin with GOOD data is not coincident with an  */
   /* index of 1, then "missing" data exists                            */
   if(start_index!=1) {
      /* set the number of missing bins equal to F0_bins. Subtract 1 from
      the count to not include the first good bin as missing            */
      F0_bins=start_index-1;
      if(TEST) 
         fprintf(stderr,"  Total number of missing bins: F0_bins=%i\n",F0_bins);

      /* calculate the number of groups depending upon buff_step. buff_step
      is the storage increment value. Hence, to store every bin, then
      buff_step would have a value of 1, etc. The 'fmod' library call is
      equivalent to the FORTRAN MOD function to calculate the modulus of
      two values. The return is the whole number remainder of the division
      between the two parameters. Hence mod(31,7)=3 or (31 - int(31/7))=7 */
      *sub_from_start=fmod(F0_bins,buff_step);
      if(TEST) fprintf(stderr,"  mod(F0_bins(%i),buff_step(%i))=%i\n",F0_bins,
         buff_step,*sub_from_start);

      /* next, calculate how many bins would be left given the input 
         buff_step increment value                                      */
      F0_bins=F0_bins/buff_step;
      if(TEST) fprintf(stderr,"  F0_bins/buff_step=%i\n",F0_bins);

      /* Once the increment value has been taken care of, determine how 
         many multiples of 30 bins remain. Store this value in F0_runs  */
      F0_runs=F0_bins/30;
      if(TEST) fprintf(stderr,"  F0_bins/30=%i\n",F0_runs);

      /* if there was not an even multiple of bins, then copy the 
         remaining number of bins (in excess of a multiple of 30) into 
         odd_bins                                                       */
      odd_bins=fmod(F0_bins,30);
      if(TEST) fprintf(stderr,"  mod(F0_bins,30)=%i\n",odd_bins);

      /* initialize pad_run to 0 before stuffing outbuf with data. this
         variable will eventually hold the total number of stuffed 2-byte
         words within outbuf */
      pad_run=0;

      /* if the number of "odd_bins" is greater than 0 (but less than 30),
      then we need to handle this condition for stuffing a word other than
      F0F0 into outbuf */
      if(odd_bins>0) {
         /* Not sure why the upcoming decrementation takes place if odd_bins=1 */
         if(TEST) fprintf(stderr,"  enter odd_bins processing\n");
         if(odd_bins==1) {
            /* set flag for caller to decrement starting bin by 1
            to make an even number of runs of 30 of missing data */
            *sub_from_start=*sub_from_start + (odd_bins * buff_step);
            if(TEST) fprintf(stderr,"  odd_bins=1 sub_from_start=%i\n",*sub_from_start);
            }
          else {
            /* if odd_bins >1 and <30 then access the correct hex value for
            stuffing into outbuf by using the odd_run_table. Each value within
            this table is a hex/rle representation of the same number
            (i.e. 0x1010=2 bins, 0xF010=16 bins and 0xF0E0=29 bins   */
            pad_run=odd_run_table[odd_bins];
            if(TEST) fprintf(stderr,"  pad_run value=%02x\n",(unsigned short)pad_run);
            } /* end if(odd_bins==1) */
         } /* end if(odd_bins>0) */

      /* F0_runs contains the number of 2-byte words of F0F0 to stuff into
      the output buffer. stuff the groups of 30 missing bins now */
      for(i=1;i<=F0_runs;i++) {
         outbuf[outbuf_index+i-1]=F0F0;
         } /* end for i loop */

      /* pad_count will now contain the number of 2-bytes that were stuffed
      into outbuf and the amount that the buffer index will have to be
      incremented back in the calling procedure */
      pad_cnt=F0_runs;

      /* finally, we need to add the odd hex value that was stored in pad_run
      if there was not an even multiple of 30 bins designated as missing */
      if(pad_run!=0) {
         /* store odd runs and increment count */
         outbuf[outbuf_index+F0_runs]=pad_run;
         /* final incrementing of pad_count. it now contains the total number
         of 'padded' words in the output buffer */
         pad_cnt=pad_cnt+1;
         } /* end of if(pad_run) */
      if(TEST) fprintf(stderr,"  final pad_cnt value=%i\n",pad_cnt);
      } /* end of if start_index */

   return(pad_cnt);
   }  /* end of pad_front */

