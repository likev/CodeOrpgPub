/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:50 $
 * $Id: prcpacum_init_acum_adapt.c,v 1.2 2006/02/09 18:19:50 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_init_acum_adapt.c

   Description
   ===========
      This function is responsible for converting adaptation data from real 
   to integer and to the most useful format needed by the precipitation
   accumulation algorithm.  The data to be converted is contained in both
   the rate and accumulation offsets into the adaptation data within the input
   buffer as passed by the precipitation rate algorithm.
   
   Change History
   ==============
   date          version   programmer           notes
   ----------    -------   ----------------     --------------------
   02/21/89      0000      P. Pisani            spr # 90067
   04/23/90      0001      David M. Lynch       spr # 90697
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
   12/31/02      0012      D. Miller            ccr na00-28601
   01/05/05      0013      C. Pham              ccr NA05-01303
   10/26/05      0014      C. Pham              ccr NA05-21401
****************************************************************************/
/* Global include files */
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

void init_acum_adapt( )
{
    
int   max_p01mm_sto = 32000;   /* Max precip rate or accum stored in hundredths
                                  of mm */
double cent_factor = 100.0;     /* factor (100.) for conversion to hundredths of
                                  mm    */ 

  if ( DEBUG ) 
    {fprintf(stderr,"A31352_init_acum_adapt\n");}

/* Get max precip rate and convert to integer hundredths of mm/sec*/

  blka.max_prcp_rate = rate_adpt.max_prate * cent_factor;

  if ( blka.max_prcp_rate > max_p01mm_sto ) 
  {
    blka.max_prcp_rate = max_p01mm_sto;
  }

  if ( DEBUG ) 
    {fprintf(stderr,"blka.max_prcp_rate = %d\n",blka.max_prcp_rate);}

/* Get restart time threshold and convert to integer seconds*/

  blka.thrsh_restart = acum_adpt.tim_restrt * SEC_P_MIN;

  if (DEBUG) 
    {fprintf(stderr,"blka.thrsh_restart = %d\n",blka.thrsh_restart);}

/* Get maximum time for interpolation and convert to integer seconds.*/

  blka.max_interp_tim = acum_adpt.max_timint * SEC_P_MIN;

  if (DEBUG) 
    {fprintf(stderr,"blka.max_interp_tim = %d\n",blka.max_interp_tim);}

/* Get maximum time for hour totals and convert to integer seconds.*/

  blka.min_tim_hrly = acum_adpt.min_timprd * SEC_P_MIN;

  if (DEBUG) 
    {fprintf(stderr,"blka.min_tim_hrly = %d\n",blka.min_tim_hrly);}

/* Get threshold outlier value and convert to integer hundredths of mm.*/

  blka.thrsh_hrly_outli = acum_adpt.thr_hlyout * cent_factor;

  if ( blka.thrsh_hrly_outli > max_p01mm_sto ) 
  {
    blka.thrsh_hrly_outli = max_p01mm_sto;
  }

  if ( DEBUG )
    {fprintf(stderr,"blka.thrsh_hrly_outli = %d\n",blka.thrsh_hrly_outli);}

/* Convert end gage accumulation time to integer leaving in minutes.*/

  blka.gage_accum_tim = acum_adpt.end_timgag;

  if (DEBUG) 
    {fprintf(stderr,"blka.gage_accum_tim = %d\n",blka.gage_accum_tim);}

/* Get maximum hourly threshold and convert to integer hundredths of mm.*/

  blka.max_acum_hrly = acum_adpt.max_hlyval * cent_factor;

  if ( blka.max_acum_hrly > max_p01mm_sto )
  {
    blka.max_acum_hrly = max_p01mm_sto;
  }

  if (DEBUG) 
    {fprintf(stderr,"blka.max_acum_hrly = %d\n", blka.max_acum_hrly);}
}
