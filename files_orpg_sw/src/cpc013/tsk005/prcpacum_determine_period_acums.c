/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:43:42 $
 * $Id: prcpacum_determine_period_acums.c,v 1.3 2008/01/04 20:43:42 aamirn Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_determine_period_acums.c

   Description
   ===========
       This function controls the calculations for period accumulations. If the
   method for computing period totals is interpolation then control is passed 
   to one of two modules. If the flag zero reference is set, indicating zero
   values for the previous rate scan then interp1_acum() is called.  This
   function will compute period totals for only the current rate scan, storing 
   the results at the scan to scan offset in the output buffer. If the flag
   zero reference is clear, then interp2_acum() is called. This function
   will compute period totals for both the current and previous rate scan
   storing the results in the scan to scan offset of the output buffer.  If
   the method for computing period totals is extrapolation, extrap_acum() is
   called twice depending on the state of flag zero reference. If this flag is
   set, then extrap_acum() is called only once to compute period totals for the
   current rate scan; Otherwise, it is called twice to compute totals for both
   the previous and current rate scans. period accumulations for the current 
   rate scan are stored in the scan to scan offset of the output buffer and
   accumulations for the previous rate scan are stored in scratch buffer two.

   Change History
   ==============
   02/21/89      0000      P. Pisani            spr # 90067
   04/23/90      0001      David M. Lynch       spr # 90697
   02/22/91      0002      Bayard Johnston      spr # 91254
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
   12/31/02      0012      D. Miller            ccr na00-28601
   01/07/05      0013      Cham Pham            ccr NA05-01303
   10/26/05      0014      Cham Pham            ccr NA05-21401

   INPUT      	   TYPE        DESCRIPTION
   -----           ----        -----------
   ratescan[][]    short	Array that contais ncurrent rate scan of 115x360
   pratescan[][]   short	Array that contains previous rate scan of 
                                115x360.
   nprdscn[][]     short	Array that contains third period accumulation 
                                scan data when case is extrapolation.
   acumscan[][]    short	Array that contains (115x360) period scan 
                                accumulation data for the third period
                                when method of computing period totals is
                                extrapolation.
  
   OUTPUT          TYPE        DESCRIPTION
   ------          ----        -----------
   acumscan[][]    short	Array that contains (115x360) period scan
                                accumulation data for the third period
                                when method of computing period totals is
                                extrapolation.
   nprdscn[][]     short        Array that contains third period accumulation
                                scan data when case is extrapolation.
****************************************************************************/
/* Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

/* Declare function prototypes */
extern void interp1_acum( short[MAX_AZMTHS][MAX_RABINS], 
                          short[MAX_AZMTHS][MAX_RABINS], double* );
extern void interp2_acum( short[MAX_AZMTHS][MAX_RABINS], 
                          short[MAX_AZMTHS][MAX_RABINS], 
                          short[MAX_AZMTHS][MAX_RABINS], double* );
extern void extrap_acum( short[MAX_AZMTHS][MAX_RABINS], 
                         short[MAX_AZMTHS][MAX_RABINS] );

void determine_period_acums( short ratescan[][MAX_RABINS],
                             short pratescan[][MAX_RABINS],
                             short nprdscn[][MAX_RABINS],
                             short acumscan[][MAX_RABINS] ) 
{
double time_scan_dif;

  if (DEBUG) 
  {
   fprintf(stderr," A3135A_DETERMINE_PRD_ACUMS\n");
  }

/* Determine scan time difference between previous and current
   scans and convert to real number. 
 */
  time_scan_dif = (double)RateSupl.tim_scndif;

  if (DEBUG) 
  {
    fprintf(stderr,"time_scan_dif = %f RateSupl.tim_scndif = %d\n",
                  time_scan_dif,RateSupl.tim_scndif);
  }

/* Determine whether case is interpolation. if so do period acumulations for
   either current and previous rate scan or current only. 
 */
  if ( blka.cases == INTERP ) 
  {

    if (DEBUG) 
      {fprintf(stderr,"CASE = INTERP\n");}

    if ( RateSupl.flg_zerref == FLAG_SET ) 
    {
      if (DEBUG) 
        {fprintf(stderr,"... A3135B__INTERP1_ACUM called\n");}

      interp1_acum( ratescan, acumscan, &time_scan_dif );
    }
    else 
    {
      if (DEBUG) 
        {fprintf(stderr,"... A3135C__INTERP2_ACUM called\n");}

      interp2_acum( pratescan, ratescan, acumscan, &time_scan_dif );
    }

  }   
/* Case is extrapolation so do period accumulations for periods
   one and three or three only. 
 */
  else 
  {

    if (DEBUG) 
      {fprintf(stderr,"CASE = EXTRAP\n");}

/* Do forward extrapolation with data from previous rate scan for the first
   period being converted from rate to accumulation. 
 */ 
    if ( RateSupl.flg_zerref == FLAG_CLEAR ) 
    {
      if (DEBUG) 
        {fprintf(stderr,"... A3135D__EXTRAP_ACUM called... prev scan\n");}

      extrap_acum( pratescan, nprdscn );
    }

/* Do backward extrapolation with data from current rate scan for the third
   period being converted from rate to accumulation with destination being
   accumulation scan to scan buffer.
 */
    if ( DEBUG ) 
      {fprintf(stderr,"... A3135D__EXTRAP_ACUM called... cur scan\n");}

    extrap_acum( ratescan, acumscan );

  }

}
