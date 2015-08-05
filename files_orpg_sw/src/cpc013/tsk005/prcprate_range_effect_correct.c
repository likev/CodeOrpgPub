/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:20:01 $
 * $Id: prcprate_range_effect_correct.c,v 1.2 2006/02/09 18:20:01 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_range_effect_correc.c

   Description
   ===========
       This function performs range-effect correction on rate scan bins beyond 
   a cutoff range.
 
   Change History
   =============
   08/29/88      0000      Greg Umstead         spr # 80390
   04/05/90      0001      Dave Hozlock         spr # 90697
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
   10/26/05      0013      Cham Pham            ccr NA05-21401
****************************************************************************/
/* Global include files */
#include <rpgc.h>
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

#define CNTR_STEP 1	       /* Parameter value (1) used for initialization.*/
#define LOG_BASE 10.	       /* Parameterized value (10.0) used for 
                                  converting back to linear units.            */

void range_effect_correc( )
{
int   i,j,			/* Array index use in for loop                */
      cutoff_bin;		/* Bin index corresponding to cutoff range    */
double r_e_corr,		/* Range-effect correction function result    */
       rate_dbr;		/* Rate bin value converted from mm/hr to DBR */

   if ( DEBUG ) 
   {
     fprintf(stderr,"***** Begin range_effect_correc(), ad_cutoff_rng=%f\n",
                  a313hadp.ad_cutoff_rng);
   }

/* Determine bin corresponding to cutoff range*/
   cutoff_bin = (a313hadp.ad_cutoff_rng + CNTR_STEP)/r_bin_size + CNTR_STEP;

   if (DEBUG) {fprintf(stderr,"CUTOFF_BIN = %d\n",cutoff_bin);}

/* Do for each azimuth...*/
   for ( j=0; j<MAX_AZMTHS; j++ ) 
   {

/* Do for each rate scan bin beyond cutoff range...*/
     for ( i=cutoff_bin-CNTR_STEP; i<MAX_RABINS; i++ ) 
     {

        if ( RateScan[j][i] > a313hadp.ad_minrat ) 
        {
/* If precip rate greater than zero, compute range-effect correction 
   ...now compute rate in dbr...
 */
          rate_dbr = LOG_BASE * log10(RateScan[j][i] / RATE_SCALING);

/* ...then compute the range effect correction...*/
          r_e_corr = a313hadp.ad_c01 + a313hadp.ad_c02 * rate_dbr +
                     a313hadp.ad_c03 * log10(a313hlfm.bin_range[i]);

/* and convert back to linear units, rounding the integer result...*/
/* Note: Changed for LINUX - Used RPGC_NINT library function instead
         of adding 0.5 for rounding to the nearest integer.
 */

          RateScan[j][i] = RPGC_NINT(pow(LOG_BASE, (r_e_corr / LOG_BASE)) *
                                     RATE_SCALING);

/* Check to see if corrected value is above maximum rate...*/
          if (RateScan[j][i] > a313hadp.ad_maxrat) 
          {
            RateScan[j][i] = a313hadp.ad_maxrat;
          }

        }

     }/* End for loop MAX_RABINS */

   }/* End for loop MAX_AZMTHS */

   if ( DEBUG ) {fprintf(stderr," ***** End of range_effect_correc()\n");}
}
