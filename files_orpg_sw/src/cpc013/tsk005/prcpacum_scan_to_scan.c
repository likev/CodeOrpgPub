/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:23 $
 * $Id: prcpacum_scan_to_scan.c,v 1.1 2005/03/09 15:43:23 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_scan_to_scan.c
    Author: Kelley Miles
    Created: 09 DEC 2004

    Description
    ===========
    This function is called whenever the method for calculating period
    accumulations is extrapolation. The contents of the first period scan
    (reference) are added to the contents of the third period (current)
    and stored in the scan to scan portion of the ourput buffer. This process
    is performed for the entire scan which is 115 x 360 I*2 words long.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    02/22/91      0001      BAYARD JOHNSTON      SPR # 91254
    02/15/91      0001      JOHN DEPHILIP        SPR # 91762
    12/03/91      0002      STEVE ANDERSON       SPR # 92740
    12/10/91      0003      ED NICHLAS           SPR 92637 PDL Removal
    04/24/92      0004      Toolset              SPR 91895
    03/25/93      0005      Toolset              SPR NA93-06801
    01/28/94      0006      Toolset              SPR NA94-01101
    03/03/94      0007      Toolset              SPR NA94-05501
    04/11/96      0008      Toolset              CCR NA95-11802
    12/23/96      0009      Toolset              CCR NA95-11807
    03/16/99      0010      Toolset              CCR NA98-23803
    12/31/02      0011      D. Miller            CCR NA00-28601
    12/09/04      0012      K. Miles             CCR NA05-01303
    
    MAX_AZMTHS - Maximum number of azimuths in a scan
    MAX_ACUBINS - Total number of range bins for accumulation
    FIRST_BIN - Starting bin number for 115 bins
    FIRST_RADIAL - Starting bin number for 360 radials
    acumscan - 2-D array that contains ( 360 x 115 ) short integer
               period scan accumulation data for the third period
               when method of computing period totals is extrapolation
    fprdscan - 2-D array that contains ( 360 x 115 ) short integer
               period scan accumulation data for the first period
               when method for computin period totals is extrapolation
    bn - Current Range bin in loop for processing all range bins (115)
    rn - Current Radial number in loop for processing all radialss (360)

****************************************************************************/
/** Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/** Local include files */
#include "prcprtac_Constants.h"

void scan_to_scan( short fprdscan[][MAX_ACUBINS],
                   short acumscan[][MAX_ACUBINS] )
{

   int rn, bn;

   if ( DEBUG ) fprintf(stderr," A3135I__SCAN_TO_SCAN\n");

   for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
   {
      for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
      {
         acumscan[rn][bn] += fprdscan[rn][bn];
      }
   }
}
