/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:40 $
 * $Id: prcprate_avg_hybscn_pairs.c,v 1.1 2005/03/09 15:43:40 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_avg_hybscn_pairs.c

   Description
   ===========
      This function averages hybrid scan bin pairs along each radial to form 
   the rate scan, rounding results up 50 of time rounding is invoked.

   Change History
   =============
   08/29/88      0000      Greg Umstead         spr # 80390
   04/13/90      0001      Dave Hozlock         spr # 90697
   02/22/91      0002      Paul Jendrowski      spr # 91254
   02/15/91      0002      John Dephilip        spr # 91762
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      Toolset              spr 91895 
   03/25/93      0006      Toolset              spr na93-06801
   01/28/94      0007      Toolset              spr na94-01101
   03/03/94      0008      Toolset              spr na94-05501
   04/01/95      0009      Toolset              ccr na95-11802
   08/25/95      0010      Dennis Miller        ccr na94-08459
   12/23/96      0011      Toolset              ccr na95-11807
   03/16/99      0012      Toolset              ccr na98-23803
   01/13/05      0013      D. Miller; J. Liu    CCR NA04-33201 
   02/20/05      0014      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

#define RDMSNG 256

void avg_hybscn_pairs( )
{
int  i,j,n,ip1;
int  cntr_step = 1;
int  ihalf_denom = 2;
int  isum_init = -1;

  if ( DEBUG ) 
    {fprintf(stderr," ***** begin a3134v_average_pairs to form RateScan\n");}

/* Do for each azimuth...*/
  for ( j=0; j<MAX_AZMTHS; j++ ) 
  {
    n = isum_init;

/* Do for each hybrid scan bin pair along radial...*/
    for ( i=0; i<MAX_HYBINS; i=i+r_bin_size ) 
    {

    /* Compute rate scan array position...*/
      n = n + cntr_step;
      ip1 = i + cntr_step;

      if ( HybrScan[j][i] != RDMSNG ) 
      {
      /* Good bins, compute simple average of pair. if 2nd of pair is even, 
         round result down,if it is odd, round result up...
       */
        if ( HybrScan[j][ip1] != RDMSNG )
        {
          RateScan[j][n] = a3134ca.rate_table[HybrScan[j][i]]/ihalf_denom +
            (a3134ca.rate_table[HybrScan[j][ip1]]+cntr_step)/ihalf_denom;
        }
        else
        {
      /* Second of pair bad or missing, take first*/
          RateScan[j][n] = a3134ca.rate_table[HybrScan[j][i]];
        }
      }
      else 
      {
      /* First of pair bad or missing, check second*/
        if ( HybrScan[j][ip1] != RDMSNG )
        {
        /* Second of pair good, therefore take second*/
          RateScan[j][n] = a3134ca.rate_table[HybrScan[j][ip1]];
        }
        else
        {
        /* Both bins bad or missing, use flag value...*/
          RateScan[j][n] = FLG_MISSNG;
        }
      }

/* Check for values above maximum rate threshold, and correct if above */
      if ( RateScan[j][n] > a313hadp.ad_maxrat ) 
      {
        RateScan[j][n] = a313hadp.ad_maxrat;
      }

    }/* End loop MAX_HYBINS */

  }/* End loop MAX_AZMTHS */

/* More debug writes...*/
  if ( DEBUG ) 
    {fprintf(stderr,"end of avg_hybscn_pairs(), RateScan array formed\n");}

}
