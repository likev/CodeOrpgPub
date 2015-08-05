/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:40 $
 * $Id: prcpacum_test_n_interp.c,v 1.3 2014/03/18 14:12:40 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_test_n_interp.c
    Author: Kelley Miles
    Created: 14 DEC 2004

    Description
    ===========
    This function examines the eight surrounding neighbors of the current bin
    to determine whether or not a neighbor exeeds the hourly outlier
    threshold. If there is a neighboring bin that exceeds the threshold,
    then interpolation is not performed. If there are no neighbors that
    exceed the threshold, then these neighbors are summed and the result
    divided by the total numbers of neighbors (8). This interpolated value
    then replaces the hourly accumulation value at the current bin/radial
    location.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    04/23/90      0001      DAVID M. LYNCH       SPR # 90697
    02/22/91      0002      PAUL JENDROWSKI      SPR # 91254
    02/15/91      0002      JOHN DEPHILIP        SPR # 91762
    12/03/91      0003      STEVE ANDERSON       SPR # 92740
    12/10/91      0004      ED NICHLAS           SPR 92637 PDL Removal
    04/24/92      0005      Toolset              SPR 91895
    03/25/93      0006      Toolset              SPR NA93-06801
    01/28/94      0007      Toolset              SPR NA94-01101
    03/03/94      0008      Toolset              SPR NA94-05501
    04/11/96      0009      Toolset              CCR NA95-11802
    12/23/96      0010      Toolset              CCR NA95-11807
    03/16/99      0011      Toolset              CCR NA98-23803
    11/27/00      0012      C. Stephenson        CCR NA00-33301
    12/14/04      0013      K. Miles, C. Pham    CCR NA05-01303
    10/26/05      0014      C. Pham              CCR NA05-21401

Included variable descriptions
    MAX_AZMTHS     Constant. Maximum number of azimuths in a scan (index into
                   output buffer of adjusted values).
    THRSH_HRLY_OUTLI A value used to check hourly accumulations. If any hourly
                     value exceeds this, it is replaced by the average of its
                     eight neighbors. Default value is 400mm.
    DECR           Constant. A value used for decrementing.
    INCR           Constant. A value used for incrementing by one.
    MAX_ACUBINS    Constant. Total number of range bins for accumulation.
    NULL0          Constant. A value used for initalization and testing.
    acumhrly       OFFSET INTO OUTPUT BUFFER OF 115 X 360 I*2 HOURLY 
                   ACCUMULATION TOTALS.
    rn             CURRENT RADIAL NUMBER IN LOOP FOR PROCESSING ALL RADIALS(360)
    bn             CURRENT RANGE BIN IN LOOP FOR PROCESSING ALL RANGE BINS (115)
    interped       Flag indicating that interpolation has been performed.

Internal Tables/Work Area
    bm1            BIN NUMBER ONE CLOSER TO THE CENTER OF THE RADAR THAN 
                   CURRENT BIN.
    bp1            BIN NUMBER ONE FURTHER AWAY FROM THE RADAR THAN THE CURRENT
                   BIN.
    MAX_AZMTHS     LOCAL VALUE FOR MAXIMUM NUMBER OF RADIALS (360)
    MAX_ACUBINS    LOCAL VALUE FOR MAXIMUM NUMBER OF BINS (115)
    MAX_NGHBOR     TOTAL NUMBER OF NEIGHBORS (8) SURROUNDING THE CURRENT BIN.
    max_ni         VALUE SET TO TOTAL NEIGHBORS (8) USED TO CONTROL A MOCK DO
                   LOOP.
    min_azm        MINIMUM RADIAL NUMBER (1)
    min_bin        MINIMUM BIN NUMBER (1)
    neighbors      TOTAL NUMBER NEIGHBORS (8) SURROUNDING THE CURRENT BIN.
    ni             VARIABLE THAT CONTAINS THE NEIGHBOR BIN (1 - 8) FOR THE 
                   CURRENT BIN.
    none_over      LOGICAL VARIABLE USED TO CONTROL THE LOOP THAT DETERMINES  
                   THE NUMERATOR FOR INTERPOLATION.
    numer          NUMERATOR FOR THE INTERPOLATION COMPUTATION. CONTAINS SUM OF 
                   ALL EIGHT NEIGHBORS.
    rm1            RADIAL ONE CLOSER TO THE RADAR THAN THE CURRENT BIN.
    rp1            RADIAL ONE FURTHER AWAY FROM THE RADAR THAN THE CURRENT 
                   RADIAL.
****************************************************************************/
/** Global include files */
#include <rpgc.h>
#include <a313h.h>
#include <a313hbuf.h>

/** Local include file */
#include "prcprtac_Constants.h"

#define MAX_NGHBOR 8
#define TRUE       1
#define FALSE      0

void test_n_interp( short acumhrly[][MAX_ACUBINS],
                    int bn, int rn, int *interped )
{

int ni, rm1, rp1, bm1, bp1, 
    max_ni, numer, none_over,
    neighbors[MAX_NGHBOR];
int min_azm=0;
int min_bin=0;

   if ( DEBUG ) {fprintf(stderr," A3135O__TEST_N_INTERP\n");}

/* Determine indices of surrounding sample bins used in test. */

   rm1 = rn + DECR;
   if ( rm1 < min_azm )
   {
      rm1 = MAX_AZMTHS-1;
   }

   rp1 = rn + INCR;
   if ( rp1 > (MAX_AZMTHS-1) )
   {
      rp1 = min_azm;
   }

   bm1 = bn + DECR;
   bp1 = bn + INCR;
   ni = NULL0;

   if (DEBUG)
     {fprintf(stderr,"rm1: %d  rp1: %d  bm1 : %d  bp1:%d\n",rm1,rp1,bm1,bp1);}

/* Store surrounding bins (5 or 8) in temporary neighbors array.
   Row of bins one closer to center of radar than center bin. 
 */
   if ( bm1 >= min_bin )
   {
      ni += INCR;
      neighbors[ni] = acumhrly[rm1][bm1];
      ni += INCR;
      neighbors[ni] = acumhrly[rn][bm1];
      ni += INCR;
      neighbors[ni] = acumhrly[rp1][bm1];
   }

   if (DEBUG) 
     {fprintf(stderr,"111: neighbors(%d) = %d\n",ni,neighbors[ni]);}

/* row of bins at same slant range as center bin */

   ni += INCR;
   neighbors[ni] = acumhrly[rm1][bn];
   ni += INCR;
   neighbors[ni] = acumhrly[rp1][bn];

   if (DEBUG) 
     {fprintf(stderr,"222: neighbors(%d) = %d\n",ni,neighbors[ni]);}

/* row of bins one farther from radar than center bin */

   if (bp1 < (MAX_ACUBINS-1) )
   {
      ni += INCR;
      neighbors[ni] = acumhrly[rm1][bp1];
      ni += INCR;
      neighbors[ni] = acumhrly[rn][bp1];
      ni += INCR;
      neighbors[ni] = acumhrly[rp1][bp1];
   }

   if (DEBUG) 
     {fprintf(stderr,"333: neighbors(%d) = %d\n",ni,neighbors[ni]);}

/* Test for neighbors above threshold while adding them if they are not. */

   none_over = TRUE;
   max_ni = ni;
   ni = INCR;
   numer = NULL0;

   if (DEBUG) 
     {fprintf(stderr,"===>  MAX_NI= %d\n",max_ni);}

/* Do for all neighbors while 'none_over' is '1'. */
   for (; ;)
   {
      if ( none_over ) 
      {
         if ( neighbors[ni] > blka.thrsh_hrly_outli )
         {
            none_over = FALSE;
         }
         else
         {
            numer = numer + neighbors[ni];
         }
         ni += INCR;
         if ( ni <= max_ni ) continue;
      }
      break;
   }

/* If none of the neighbors are over the threshold, perform interpolation
   of neighbors, replace center bin value with interpolated value and return
   flag to calling routine indicating interpolation performed. 
 */
   if (DEBUG) 
     {fprintf(stderr,"BEFORE: NUMER= %d  MAX_NI=%d\n",numer,max_ni);}

   *interped = FALSE;

   if ( none_over )
   { 
      if (DEBUG) 
        {fprintf(stderr,"NUMER= %d  MAX_NI=%d\n",numer,max_ni);}

/* Note: Changed for LINUX - Used RPGC_NINT library function instead of adding
         0.5 for rounding to the nearest integer.
 */

      acumhrly[rn][bn] = RPGC_NINT( (double)numer / max_ni );
      *interped = TRUE;
   }

}
