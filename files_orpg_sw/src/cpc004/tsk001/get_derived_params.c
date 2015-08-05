/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:54 $
 * $Id: get_derived_params.c,v 1.2 2003/07/17 15:07:54 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************

   get_derived_params.c

   PURPOSE:

   This routine calculates derived adaptable parameters.

   CALLED FROM:

   apply_mpda1

   INPUTS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   CALLS:

   None.

   HISTORY:

   R. May,  3/02      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_adapt_params.h"
#include "mpda_structs.h"

void 
get_derived_params()
{
  short max_nyq;

  seed_unf_prf1 = seed_unf*save.prf1_nyq;
  seed_unf_prf2 = seed_unf*save.prf2_nyq;
  seed_unf_prf3 = seed_unf*save.prf3_nyq;

/* Calculate the amount of radials that overlap a complete 360 degree sweep */
  
  save.rad_offset = save.tot_prf1_rads - save.ps_tot_prf1_rads;

/* Find which velocity cut has the highest nyquist velocity */

  if(save.prf1_nyq > save.prf2_nyq)
    {
    max_nyq = save.prf1_nyq;
    vel_max_nyq = 1;
    }
  else
    {
    max_nyq = save.prf2_nyq;
    vel_max_nyq = 2;
    }

  if(save.prf3_nyq > max_nyq)
    vel_max_nyq = 3;

  return;
}
