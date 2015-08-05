/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:46:50 $
 * $Id: dp_lt_accum_CopySupl.c,v 1.4 2009/10/27 22:46:50 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

/***********************************************************************
   Filename: dp_lt_accum_CopySupl.c

   Description
   ===========
      copy_supplemental() copies the supplemental data from S2S_Accum_Supl_t
   to LT_Accum_Supl_t.

   Input:   S2S_Accum_Supl_t *s2s_supl - Supplemental scan-to-scan accumulation 
                                         data structure.

   Output:  LT_Accum_Supl_t *lt_supl   - Supplemental long term accumulation 
                                         data structure.

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Called by: LT main()

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----------  -------    ----------         ----------------------
   10/25/2007  0000       Cham               Initial implementation
   11/19/2007  0001       Ward               Deleted unused fields
************************************************************************/

int copy_supplemental(S2S_Accum_Supl_t *s2s_supl, LT_Accum_Supl_t *lt_supl)
{
  static unsigned int lt_supl_size = sizeof(LT_Accum_Supl_t);

  if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning copy_supplemental()\n");

  /* Check for NULL pointers */

  if(pointer_is_NULL(s2s_supl, "copy_supplemental", "s2s_supl"))
      return(NULL_POINTER);

  if(pointer_is_NULL(lt_supl, "copy_supplemental", "lt_supl"))
      return(NULL_POINTER);

  /* Initialize LT_Accum_Supl_t structure */

  memset(lt_supl, 0, lt_supl_size);

  /* Copy Supplemental data from Accum_Supl_t structure */

  lt_supl->last_time_prcp     = s2s_supl->last_time_prcp;
  lt_supl->ST_active_flg      = s2s_supl->ST_active_flg;
  lt_supl->prcp_begin_flg     = s2s_supl->prcp_begin_flg;
  lt_supl->prcp_detected_flg  = s2s_supl->prcp_detected_flg;
  lt_supl->pct_hybrate_filled = s2s_supl->pct_hybrate_filled;
  lt_supl->highest_elev_used  = s2s_supl->highest_elev_used;
  lt_supl->sum_area           = s2s_supl->sum_area;
  lt_supl->vol_sb             = s2s_supl->vol_sb;

  return(FUNCTION_SUCCEEDED);

}/* copy_supplemental() ==================================================== */
