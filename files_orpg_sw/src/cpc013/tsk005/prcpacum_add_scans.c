/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:45 $
 * $Id: prcpacum_add_scans.c,v 1.2 2006/02/09 18:19:45 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_add_scans.c
    Author: Kelley Miles
    Created: 17 DEC 2004

    Description
    ===========
    THIS FUNCTION ADDS THE CONTENTS OF A PERIOD ACCUMULATION SCAN TO THE 
    CONTENTS OF THE OUTPUT BUFFER FOR THE ENTIRE 360 X 115 I*2 ARRAYS 
    OF DATA. THERE ARE SIX SPECIAL CASES FOR PERFORMING THIS ADDITION 
    FUNCTION LISTED BELOW:

       1) THERE IS NOTHING IN THE OUTPUT BUFFER CURRENTLY AND THE PERCENT
          OF THE PERIOD WITHIN THE HOUR IS ONE THOUSAND. IN THIS CASE 
          EACH ELEMENT OF THE PERIOD SCAN ARRAY WILL BE COPIED TO THE 
          OUTPUT BUFFER. THIS CASE REPRESENTATION. AFTER EXECUTION
          A FLAG WILL BE CLEARED (FRST_IN_OUTBUF) WHICH
          INDICATES THAT THE OUTPUT BUFFER CONTAINS DATA.

       2) THERE IS NOTHING IN THE OUTPUT BUFFER CURRENTLY AND THE PERCENT 
          OF THE PERIOD WITHIN THE HOUR IS LESS THAN ONE THOUSAND. THIS CASE 
          IS HANDLED LIKE THE ONE SPECIFIED IN 1) ABOVE WITH THE FOLLOWING
          EXCEPTION. PRIOR TO COPYING EACH PERIOD ACCUMULATION
          VALUE TO THE OUTPUT BUFFER IT IS MULTIPLIED BY
          FRACTION OF THE PERIOD THAT IS WITHIN THE HOUR.

       3) IF THERE IS CURRENTLY DATA IN THE BUFFER AND NO OVERFLOW OF THE 
          MAXIMUM HOURLY VALUE THRESHOLD IS POSIBLE AND THE PERCENT OF THE 
          PERIOD WITHIN THE HOUR IS ONE THOUSAND, THEN THE CONTENTS OF THE
          PERIOD ACCUMULATION SCAN ARE ADDED TO THE CONTENTS OF THE HOURLY SCAN.

       4) IF THERE IS CURRENTLY DATA IN THE OUTPUT BUFFER AND NO OVERFLOW OF 
          THE MAXIMUM HOURLY VALUE THRESHOLD IS POSSIBLE AND THE PERCENT OF THE 
          PERIOD WITHIN THE HOUR IS LESS THAN ONE THOUSAND, THEN EACH PERIOD
          ACCUMULATION SCAN VALUE IS MULTIPLIED BY A FRACTIONAL OF THE PERIOD 
          WITHIN THE HOUR PRIOR TO ADDING IT TO THE HOURLY ACCUMULATION.

       5) IF THERE IS CURRENTLY DATA IN THE OUTPUT BUFFER AND OVERFLOW OF THE 
          MAXIMUM HOURLY VALUE THRESHOLD IS POSSIBLE AND THE PERCENT OF THE 
          PERIOD WITHIN THE HOUR IS ONE THOUSAND, THEN LIKE IN CASE 3) ABOVE, 
          PERIOD ACCUMULATION SCAN DATA IS ADDED TO THE HOURLY ACCUMULATION
          SCAN BUT IN ADDITION THE RESULTS OF EACH COMPUTATION IS COMPARED WITH 
          THE MAXIMUM VALUE ALLOWED FOR THE HOUR AND IF EXCEEDED THE MAXIMUM 
          VALUE REPLACES THE COMPUTED VALUE.

       6) IF THERE IS CURRENTLY DATA IN THE OUTPUT BUFFER AND OVERFLOW OF THE 
          MAXIMUM HOURLY VALUE THRESHOLD IS POSSIBLE AND THE PERCENT OF THE 
          PERIOD WITHIN THE HOUR IS LESS THAN ONE THOUSAND, THEN THE PERIOD 
          ACCUMULATION SCAN IS MULTIPLIED BY THE FRACTIONAL PART OF THE PERIOD
          WITHIN THE HOUR AND THIS RESULT ADDED TO THE HOURLY TOTAL.IN ADDITION,
          THE COMPUTED VALUE IS COMPARED AGAINST THE MAXIMUM VALUE FOR THE HOUR 
          AND IF EXCEEDED, THE MAXIMUM VALUE REPLACES THE COMPUTED VALUE.

    AFTER ONE OF THE SIX CASES IS EXECUTED THE MAXIMUM POSSIBLE HOUR VALUE IS 
    UPDATED. THE REASON FOR DIFFERENTIATING THESE SIX CASES IS TO PRECLUDE 
    UNNECESSARY COMPUTATIONS IN THE LARGE 360 X 115 LOOPS.

    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    02/21/89      0000       P. PISANI          SPR # 90067
    02/22/91      0001       BAYARD JOHNSTON    SPR # 91254
    02/15/91      0002       JOHN DEPHILIP      SPR # 91762
    12/03/91      0003       STEVE ANDERSON     SPR # 92740
    12/10/91      0004       ED NICHLAS         SPR 92637 PDL Removal
    04/24/92      0005       Toolset            SPR 91895
    03/25/93      0006       Toolset            SPR NA93-06801
    01/28/94      0007       Toolset            SPR NA94-01101
    03/03/94      0008       Toolset            SPR NA94-05501
    04/11/96      0009       Toolset            CCR NA95-11802
    12/23/96      0010       Toolset            CCR NA95-11807
    03/16/99      0011       Toolset            CCR NA98-23803
    11/27/00      0012       C. Stephenson      CCR NA00-33301
    12/31/02      0013       D. Miller          CCR NA00-28601
    12/17/04      0014       K. Miles, C. Pham  CCR NA05-01303
    10/26/05      0015       C. Pham            CCR NA05-21401

Included variable descriptions
   FLAG_CLEAR            Constant. Parameter for a cleared flag
   FLAG_SET              Constant. Parameter for a set flag.
   MAX_AZMTHS            Constant. Maximum number of azimuths in a scan 
                         (index into output buffer of adjusted values).
   FRST_IN_OUTBUF        If set indicates no values in the buffer, therefore 
                         1st operation is a copy; otherwise the 1st operation
                         is add.
   MAX_ACUM_HRLY         Maximum value allowed for an hourly accumulation. 
                         Multiplied by 10 to give tenths of mm (max value is 
                         1600mm).
   MAX_HRLY_POSS         Maximum value possible for hourly totals.
   FIRST_BIN             Constant. Starting bin number for 115 bins.
   FIRST_RADIAL          Constant. Starting radial number for 360 radials.
   MAX_ACUBINS           Constant. Total number of range bins for accumulation.
   MAX_PER_CENT          Constant. Maximum amount a period can be within the 
                         hour (Indicates 100 percent).

Internal Tables/Work Area
   oflowtest             Test condition for overflow.
   BN                    Current range bin in loop for processing all range 
                         bins (115)
   DEC_FRAC              Fractional part of period that is within the hour 
                         expressed
                         as a decimal fraction.
   ITEMP                 Temporary repository for accumulation totals (mmx100)
   RN                    Current radial number in loop for processing all 
                         radials (360)
****************************************************************************/
/***  Global include files */
#include <rpgc.h>
#include <a313h.h>
#include <a313hbuf.h>

/***  Local include file */
#include "prcprtac_Constants.h"

#define MIN0(x1,x2) ((x1) > (x2))? (x2):(x1) /* Find smallest value         */

void add_scans( int frac, int del_time,
                short prdscan[][MAX_ACUBINS],
                short acumhrly[][MAX_ACUBINS])
{

/*
     prdscan    array contains 360 x 115 I*2 period accumulation scan for one
                of the previous period scans.
     acumhrly   offset into output buffer of 360 x 115 I*2 hourly
                accumulation totals.
     del_time   Time difference between current and previous volume scans (sec).
     frac       Fraction of period within the hour as a whole number scaled
                by 1000.
*/

int    rn, bn, itemp, oflowtest;
double dec_frac;

   if ( DEBUG ) {fprintf(stderr,"A3135J__ADD_SCANS\n");}

/* Get fraction value passed as a real decimal */

   dec_frac = (double)frac / (double)MAX_PER_CENT;

/* If there is nothing in the output buffer, the add becomes a copy. Either the
   entire period or a fraction of the period is copied. 
 */

   if ( blka.frst_in_outbuf == FLAG_SET )
   {
      if ( frac == MAX_PER_CENT )
      {
         memcpy(acumhrly,prdscan,sizeof(short)*MAX_AZMTHS*MAX_ACUBINS);
         if ( DEBUG ) {fprintf(stderr,"11111111111111\n");}
      }
      else
/* Do for all polar bins */
      {
         if (DEBUG) {fprintf(stderr,"22222222222222222\n");}
         for ( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
         {
            for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
            {

             /* Take fractional part of period scan */
             /* Note: Changed for LINUX - Used RPGC_NINT library function 
                   instead of adding 0.5 for rounding to the nearest integer*/

               acumhrly[rn][bn] = RPGC_NINT(prdscan[rn][bn] * dec_frac);
            }
         }
      }

/* Clear flag */

      blka.frst_in_outbuf = FLAG_CLEAR;
   }
   else
   {

/* Check to determine of there is a possibility of overflow for maximum hourly
   accumulation. If there is not, then the period accumulation scan is added to
   the hourly total. If there is a possibility of overflow, then the overflow
   condition is checked and hte hourly accumulation replaced with the maximum
   allowed for the hour if applicable. 
 */
/* Note: Changed for LINUX - Used RPGC_NINT library function instead of 
         adding 0.5 for rounding to the nearest integer.*/

      oflowtest = RPGC_NINT(blka.max_prcp_rate * del_time/(double)SEC_P_HR);

      if ( (blka.max_hrly_poss + oflowtest) <= blka.max_acum_hrly )
      {
         if ( DEBUG ) 
         {
           fprintf(stderr,"### blka.max_hrly_poss: %d\n",blka.max_hrly_poss);
           fprintf(stderr,"### oflowtest:          %d\n",oflowtest);
           fprintf(stderr,"### blka.max_acum_hrly: %d\n",blka.max_acum_hrly);
         }

         if ( frac == MAX_PER_CENT )
         {
            if (DEBUG) {fprintf(stderr,"33333333333333333\n");}
            for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
            {
               for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
               {
                /* Add in whole period scan */
                  acumhrly[rn][bn] += prdscan[rn][bn];
               }
            }
         }
         else
         {

/* Fractional case */

            if (DEBUG) {fprintf(stderr,"44444444444444444\n");}
            for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
            {
               for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
               {

               /* Add in fraction of period scan */
               /* Note: Changed for LINUX - Used RPGC_NINT library function
                      instead of adding 0.5 for rounding to the nearest integer.
                */

                 acumhrly[rn][bn] = RPGC_NINT(acumhrly[rn][bn] +
                                     prdscan[rn][bn] * dec_frac);
               }
            }
         }
      }
      else
      {
/* The following code takes care of the overflow condidion when adding period
   scan accumulations (whole or part) to the hour. 
 */
         if ( frac == MAX_PER_CENT )
         {
            if (DEBUG) {fprintf(stderr,"55555555555555555\n");}

            for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
            {
               for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
               {

                /* Add in whole period scan and check for exceeding max. */
                  itemp = acumhrly[rn][bn] + prdscan[rn][bn];
                  acumhrly[rn][bn] = MIN0( itemp, blka.max_acum_hrly );

                  if ( DEBUG ) {
                    /*if ( (rn+1)%180 == 1) {
                      fprintf(stderr,
                              "bn, rn = (%d, %d)  prdscan= %d  acumhrly=  %d\n",
                               bn+1,rn+1,prdscan[rn][bn],acumhrly[rn][bn]); 
                    } */
                  }
               }
            }
         }
         else
         {

/* Fractional case */

            if (DEBUG) {fprintf(stderr,"66666666666666666\n");}
            for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
            {
               for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
               {

                /* Add in a fraction and check for exceeding max. */
                /* Note: Changed for LINUX - Used RPGC_NINT library function
                   instead of adding 0.5 for rounding to the nearest integer.
                 */
                  itemp = RPGC_NINT(acumhrly[rn][bn] + prdscan[rn][bn] * dec_frac);
                  acumhrly[rn][bn] = MIN0( itemp, blka.max_acum_hrly );

                  if ( DEBUG ) {
                    /*if ( (rn+1)%180 == 1) {
                      fprintf(stderr,
                              "bn, rn = (%d, %d)  prdscan= %d  acumhrly=  %d\n",
                                   bn+1,rn+1,prdscan[rn][bn],acumhrly[rn][bn]); 
                    }*/
                  }
               }
            }
         }
      }
   }

/* Update maximum possible hourly value by maximum period value multiplied by
   fraction of the period included in the hour. 
   Note: Changed for LINUX - Used RPGC_NINT library function instead of
         adding 0.5 for rounding to the nearest integer.
 */
   blka.max_hrly_poss += RPGC_NINT(blka.max_prcp_rate * del_time / (double)SEC_P_HR);

   if ( DEBUG ) 
     {fprintf(stderr,"!!!!! MAX_HRLY_POSS = %d\n\n",blka.max_hrly_poss);}

   blka.hbuf_empty = FLAG_CLEAR;
}
