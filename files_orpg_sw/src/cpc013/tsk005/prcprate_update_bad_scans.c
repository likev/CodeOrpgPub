/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:44:08 $
 * $Id: prcprate_update_bad_scans.c,v 1.1 2005/03/09 15:44:08 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_update_bad_scans.c

   Description
   ===========
      This function updates the bad scan time-stamp array if necessary, and
   count the number of bad scans in the last hour.

   Change History
   =============
   08/29/88      0000      Greg Umstead         spr # 80390
   04/04/90      0001      Dave Hozlock         spr # 90697
   02/22/91      0002      Paul Jendrowski      spr # 91254
   02/15/91      0002      John Dephilip        spr # 91762
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      Toolset              spr 91895 
   03/25/93      0006      Toolset              spr na93-06801
   01/28/94      0007      Toolset              spr na94-01101
   03/03/94      0008      Toolset              spr na94-05501
   04/11/96      0009      Toolset              ccr na95-11802
   12/23/96      0010      Toolset              ccr na95-11807
   03/16/99      0011      Toolset              ccr na98-23803
   12/20/04      0012      Cham Pham            ccr NA05-01303
****************************************************************************/
/** Global include files */
#include <a313hbuf.h>
#include <a313h.h>

/** Local include files */ 
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

#define ZERO 0			/* Constant representing the value 0          */

void update_bad_scans( int *iostatus )
{

int  n,			/* Array index used in for loop                       */
     scn_timdif,	/* Time difference between current scan               */
     c1, c2, c3;	/* Logical variable used to test loop                 */
int cntr_step = 1;	/* Parameterized value (1) used to initialize         */
int cntr_init = 1;	/* Parameterized value (1) used to initialize         */
int readrec = 0;        /* Read operation parameter for function call         */
int writerec = 1;       /* Write operation parameter for function call        */

   if ( DEBUG ) {fprintf(stderr,"***** Begin update_bad_scans()\n");}

/* Initialize test loop flag to FALSE */
   c1=c2=c3=FALSE;

/* Read the bad scan array from disk...*/
   if ( (a313hgen.nbr_badscns != ZERO)||(a313hgen.flag_bad == FLAG_SET) ) 
   {
/* Debug writes...*/
     if (DEBUG) {fprintf(stderr,"reading bad scan record...\n");}

/* If neccesary, read bad scan record... */
     *iostatus = Badscan_IO( readrec );

/* More debug writes...*/
     if (DEBUG) {fprintf(stderr,"    after read call - %d\n",*iostatus);}
   }

/* If i/o status was ok, or if the read was not performed...*/
   if ( *iostatus == IO_OK ) 
   {
/* Initialize bad scan counter to zero...*/
     a313hgen.nbr_badscns = ZERO;

     if ( a313hgen.flag_bad == FLAG_SET ) 
     {
/* If bad scan, add scan date and time to bad time-stamps...
   adjust bad scan pointer... 
 */
       a313hgen.ptr_badscn = a313hgen.ptr_badscn + cntr_step;

       if ( a313hgen.ptr_badscn > MAX_TSTAMPS ) 
       {   
         a313hgen.ptr_badscn = cntr_init;
       }

/* Add current scan to array...*/
       a313hbsc[a313hgen.ptr_badscn-1].date = a313hgen.scan_date;
       a313hbsc[a313hgen.ptr_badscn-1].time = a313hgen.scan_time;
     }

/* If the array is not in its initial state, then calculate the
   number of bad scans (last hour)... 
 */
     if ( a313hgen.ptr_badscn != ZERO ) 
     {

/* Set index to current position before beginning loop... */
       n = a313hgen.ptr_badscn;

/* Top of infinite loop...*/
       for (; ;) 
       {
/* Calculate time difference...*/
         scn_timdif = (a313hgen.scan_date - a313hbsc[n-1].date) * 
                 SEC_P_DAY - a313hbsc[n-1].time + a313hgen.scan_time;

/* Evaluate logical conditions to determine loop continuation...*/
         if ( a313hbsc[n-1].date != INIT_VALUE ) c1 = TRUE;
         if ( scn_timdif <= SEC_P_HR ) c2 = TRUE;
         if ( a313hgen.nbr_badscns < MAX_TSTAMPS ) c3 = TRUE;

/* Now test conditions that would terminate loop...*/
         if ( c1 && c2 && c3 ) 
         {
/* If all conditions are true, execute block of loop
   (i.e. if one 'while' condition is false, jump out of loop...)*/
           n = n - cntr_step;
           if ( n < cntr_init ) 
           {
             n = MAX_TSTAMPS;
           }
/* Increment number of bad scans...*/
           a313hgen.nbr_badscns = a313hgen.nbr_badscns + cntr_step;

/* Jump to top of infinite loop, goto used to minic a infinite loop ...*/
           c1=c2=c3=FALSE;
           continue;
         }
         break;
       }/* End finite for loop */

/* At least one condition failed, out of do...while loop,
   now write the updated array back to disk, if necessary...
 */
       if ( a313hgen.flag_bad == FLAG_SET ) 
       {
         *iostatus = Badscan_IO( writerec );

/* More debug writes... */
         if (DEBUG) {fprintf(stderr,"after write call - %d\n",*iostatus);}
       }

     }/* End if block (ptr_badscn != ZERO) */

/* Debug writes...*/
     if ( DEBUG ) 
     {
      fprintf(stderr,"**** a3134b: number of bad scans - %d\n",
              a313hgen.nbr_badscns);
      fprintf(stderr,"                bad scan pointer - %d",
              a313hgen.ptr_badscn);
      fprintf(stderr,"***** End of update_bad_scans()\n");
     }

   }/* End if block iostatus equals to IO_OK */
}
