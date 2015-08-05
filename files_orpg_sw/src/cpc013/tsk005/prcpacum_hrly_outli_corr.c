/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:39 $
 * $Id: prcpacum_hrly_outli_corr.c,v 1.3 2014/03/18 14:12:39 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_hrly_outli_corr.c
    Author: Kelley Miles
    Created: 14 DEC 2004

    Description
    ===========
    This function controls the process of determining whether there are any
    outliers in the hourly scan array and if there are, calls the function 
    test_n_interp() to interpolate the surrounding eight neighbors,
    if applicable. This process is performed for the entire 360 x 115 I*2
    output buffer.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    04/23/90      0001      DAVID M. LYNCH       SPR # 90697
    02/22/91      0002      BAYARD JOHNSTON      SPR # 91254
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
    12/14/04      0012      K. Miles             CCR NA05-01303

Included variable descriptions
   HYZ_SUPL               Constant. Size of Supplemental Data array in 
                          Hybrid Scan output buffer.
                          SSIZ_PRE + SSIZ_RATE + SSIZ_ACUM +
                          SSIZ_ADJU (= (13+14+16+5) = 48)
   MAX_AZMTHS             Constant. Maximum number of azimuths in a scan 
                          (index into output buffer of adjusted values).
   NUM_INTOUT             Constant. Offset into supplemental data array 
                          within the output buffer for number of 
                          interpolated outliers.
   MAX_HRLY_POSS          Maximum value possible for hourly totals.
   THRSH_HRLY_OUTLI       A value used to check hourly accumulations. If 
                          any hourly value exceeds this, it is replaced by the
                          average of its eight neighbors. Default value is 400mm
   FIRST_BIN              Constant. Starting bin number for 115 bins.
   FIRST_RADIAL           Constant. Starting radial number for 360 radials.
   INCR                   Constant. A value used for incrementing by one.
   MAX_ACUBINS            Constant. Total number of range bins for accumulation.
   NULL0                  Constant. A value used for initalization and testing.


****************************************************************************/
/***  Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/***  Local include file */
#include "prcprtac_Constants.h"

/*** Declare function prototype */
void test_n_interp( short[MAX_AZMTHS][MAX_RABINS], int, int, int* );

void hrly_outli_corr( short acumhrly[][MAX_ACUBINS] )

{
/*  acumhrly[][] output buffer of 360 x 115 I*2 hourly accumulation totals */

   int num_tested_outli;
   int num_interp_outli;
   int bn,rn;
   int interped=0;

   if (DEBUG) {fprintf(stderr,"A3135N__HRLY_OUTLI_CORR\n");}

/* No testing performed if maximum possible value of hourly accumulation
   cannot exceed hourly outlier threshold 
 */
   num_interp_outli = NULL0;
   num_tested_outli = NULL0;

   if (DEBUG) 
   {
     fprintf(stderr," ......blka.max_hrly_poss: %d\nblka.thrsh_hrly_outli:%d\n",
                  blka.max_hrly_poss,blka.thrsh_hrly_outli);
   }

   if ( blka.max_hrly_poss > blka.thrsh_hrly_outli )
   {

/* Test hourly accumulation value against hourly outlier threshold for all
   radials and bins. If accumulation value is greater than threshold call
   subroutine to replace this high value with the average of its five or
   eight neighbors. 
 */
      for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
      {
         for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
         {
            if (acumhrly[rn][bn] > blka.thrsh_hrly_outli )
            {
               num_tested_outli += INCR;

             /* Interpret new value if possible */
               test_n_interp( acumhrly, bn, rn, &interped );

               if ( interped == 1 )
               {
                  num_interp_outli += INCR;
               }
            }
         }
      }
   }

/* Save number of interpolated outlier values in supplementary output
   data array. 
 */
   AcumSupl.num_intout = num_interp_outli;
}
