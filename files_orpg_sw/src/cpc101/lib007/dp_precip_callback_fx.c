/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/04/21 22:05:04 $
 * $Id: dp_precip_callback_fx.c,v 1.10 2015/04/21 22:05:04 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: dp_precip_callback_fx.c

   Description:
   ============
   This function gets the Dual-Pol adaptation data values.

   Inputs: void* struct_address - where to put the data

   Outputs: status - always 0

   Change History
   ==============
   DATE          VERSION   PROGRAMMERS  NOTES
   -----------   -------   -----------  ---------------------------
   15 May 2007   0000      Cham Pham    Initial implementation
   14 Aug 2007   0001      Jihong Liu   Changed based on QPE V2
   23 Jan 2008   0002      James Ward   Put all 3 files together
   20 Aug 2008   0003      James Ward   Added Min/Max/Default check,
                                        changed aborting into replacing
                                        with default value
   10 Mar 2010   0004      James Ward   Removed RhoHV_min_Kdp_rate which
                                        has been replaced with
                                        corr_thresh.
   28 Sep 2010   0005      James Ward   Added Use_PBB1, Max_Rate
   12 Aug 2014   0006      Murnan       Rewrite for easier maintability
                                        and follow methodology demonstrated
                                        in other callback function that 
                                        reference data is the DEA files.
*****************************************************************************/

#include <alg_adapt.h>       /* RPGC_log_msg      */
#include <dp_precip_adapt.h> /* dp_precip_adapt_t */

#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define dp_precip_callback_fx dp_precip_callback_fx_
#endif

#ifdef LINUX
#define dp_precip_callback_fx dp_precip_callback_fx__
#endif

#endif


/* #define DEBUG */

/******************************************************************************
   Filename: dp_precip_callback_fx.c

   Description:
   ============
   dp_precip_callback_fx() gets the Dual-Pol adaptation data values.

   Inputs: void* common_block_address - where to put the data

   Outputs: status - always 0

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS  NOTES
   -----------   -------    ------------ ---------------------------
   15 May 2007   0000       Cham Pham    Initial implementation
   14 Aug 2007   0001       Jihong Liu   Changed based on QPE V2
   23 Jan 2008   0002       James Ward   Put all 3 files together
   20 Aug 2008   0003       James Ward   Added Min/Max/Default check,
                                         changed aborting into replacing
                                         with default value
   21 Nov 2008   0004       James Ward   Added 3 new adaptable params
   02 Feb 2011   0005       James Ward   At request of Mark Fresch,
                                         don't write replacement to log.
   31 Oct 2011   0006       James Ward   For CCR NA11-00372:
                                         Removed USE_PBB1
                                         Removed Z_MAX_BEAM_BLK
                                         Added   MIN_BLOCKAGE
                                         Added   KDP_MIN_BEAM_BLK
                                         Removed RHOHV_MIN_RATE
   13 Aug 2014   0007       Murnan       Reworked to streamline code for
                                         maintainability.  Removed orignal
                                         max/min checks referencing dp_Const.h
                                         which could be different from the 
                                         dp_precip.alg DEA file which has defined
                                         valid ranges.  Potential mismatch if
                                         max/min check was left in this file.
*****************************************************************************/

int dp_precip_callback_fx(void* common_block_address)
{
  int i;
  char name[20];                /* exclusion zone field name */
  int ret = -1;			/* return status */
  double get_value;		/* temp value */

  dp_precip_adapt_t* dp_adapt = (dp_precip_adapt_t*) common_block_address;

  /* Get dp_elev_prod data elements */
  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Mode_filter_len", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Mode_filter_len = (short) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Mode_filter_len unavailable, abort task\n" );
    RPG_abort_task();
  }

  /* Get qperate data elements */

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Kdp_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Kdp_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Kdp_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Kdp_power", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Kdp_power = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Kdp_power unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Z_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Z_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Z_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Z_power", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Z_power = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Z_power unavailable, abort task\n" );
    RPG_abort_task();
  }


  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Zdr_z_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Zdr_z_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Zdr_z_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Zdr_z_power", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Zdr_z_power = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Zdr_z_power unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Zdr_zdr_power", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Zdr_zdr_power = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Zdr_zdr_power unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Gr_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Gr_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Gr_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Rh_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Rh_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Rh_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Ds_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Ds_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Ds_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Ds_BelowMLTop_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Ds_BelowMLTop_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Ds_BelowMLTop_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Ic_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Ic_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Ic_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Ws_mult", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Ws_mult = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Ws_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Grid_is_full", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Grid_is_full = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Grid_is_full unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Min_blockage", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Min_blockage = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Min_blockage unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Kdp_min_beam_blk", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Kdp_min_beam_blk = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Kdp_min_beam_blk unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Hr_HighZThresh", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Hr_HighZThresh = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Hr_HighZThresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Kdp_max_beam_blk", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Kdp_max_beam_blk = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Kdp_max_beam_blk unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Kdp_min_usage_rate", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Kdp_min_usage_rate = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Kdp_min_usage_rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Refl_min", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Refl_min = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Refl_min unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Refl_max", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Refl_max = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Refl_max unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Paif_area", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Paif_area = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Paif_area unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Paif_rate", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Paif_rate = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Paif_rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Max_vols_per_hour", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Max_vols_per_hour = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Max_vols_per_hour unavailable, abort task\n");
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Min_early_term_ang", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Min_early_term_ang = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Min_early_term_ang unavailable, abort task\n");
    RPG_abort_task();
  }

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Max_precip_rate", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Max_precip_rate = (float) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Max_precip_rate unavailable, abort task\n");
    RPG_abort_task();
  }


  /* Get exclusion zones data elements */

  ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, "Num_zones", &get_value );
  if( ret == 0 )
  {
    dp_adapt -> Num_zones = (int) get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "DP_PRECIP: Num_zones unavailable, abort task\n" );
    RPG_abort_task();
  }

  /* loop over all exclusion zones */

  for(i=0; i<dp_adapt->Num_zones; i++)
  {
     sprintf(name, "Beg_azm%d", i+1);

     ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, name, &get_value );
     if( ret == 0 )
     {
        dp_adapt -> Beg_azm[i] = (float) get_value;
     }
     else
     {
        LE_send_msg( GL_ERROR, "DP_PRECIP: %s unavailable, abort task\n", name );
        RPG_abort_task();
     }

     sprintf(name, "End_azm%d", i+1);

     ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, name, &get_value );
     if( ret == 0 )
     {
        dp_adapt -> End_azm[i] = (float) get_value;
     }
     else
     {
        LE_send_msg( GL_ERROR, "DP_PRECIP: %s unavailable, abort task\n", name );
        RPG_abort_task();
     }

     sprintf(name, "Beg_rng%d", i+1);

     ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, name, &get_value );
     if( ret == 0 )
     {
        dp_adapt -> Beg_rng[i] = (int) get_value;
     }
     else
     {
        LE_send_msg( GL_ERROR, "DP_PRECIP: %s unavailable, abort task\n", name);
        RPG_abort_task();
     }

     sprintf(name, "End_rng%d", i+1);

     ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, name, &get_value );
     if( ret == 0 )
     {
        dp_adapt -> End_rng[i] = (int) get_value;
     }
     else
     {
        LE_send_msg( GL_ERROR, "DP_PRECIP: %s unavailable, abort task\n", name);
        RPG_abort_task();
     }

     sprintf(name, "Elev_ang%d", i+1);

     ret = RPG_ade_get_values(DP_PRECIP_ADAPT_DEA_NAME, name, &get_value );
     if( ret == 0 )
     {
        dp_adapt -> Elev_ang[i] = (float) get_value;
     }
     else
     {
        LE_send_msg( GL_ERROR, "DP_PRECIP: %s unavailable, abort task\n", name);
        RPG_abort_task();
     }

  } /* end loop over all exclusion zones */

#ifdef DEBUG
   LE_send_msg( GL_INFO, "DP_PRECIP: Mode_filter_len \t\t= %d\n",  dp_adapt-> Mode_filter_len );
   LE_send_msg( GL_INFO, "DP_PRECIP: Kdp_mult \t\t= %5.3f\n",  dp_adapt-> Kdp_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Kdp_power \t\t= %5.3f\n",  dp_adapt-> Kdp_power );
   LE_send_msg( GL_INFO, "DP_PRECIP: Z_mult \t\t= %5.3f\n",  dp_adapt-> Z_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Z_power \t\t= %5.3f\n",  dp_adapt-> Z_power );
   LE_send_msg( GL_INFO, "DP_PRECIP: Zdr_z_mult \t\t= %5.3f\n",  dp_adapt-> Zdr_z_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Zdr_z_power \t\t= %5.3f\n",  dp_adapt-> Zdr_z_power );
   LE_send_msg( GL_INFO, "DP_PRECIP: Zdr_zdr_power \t\t= %5.3f\n",  dp_adapt-> Zdr_zdr_power );
   LE_send_msg( GL_INFO, "DP_PRECIP: Gr_mult \t\t= %4.1f\n",  dp_adapt-> Gr_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Rh_mult \t\t= %4.1f\n",  dp_adapt-> Rh_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Ds_mult \t\t= %4.1f\n",  dp_adapt-> Ds_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Ds_BelowMLTop_mult \t\t= %4.1f\n",  dp_adapt-> Ds_BelowMLTop_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Ic_mult \t\t= %4.1f\n",  dp_adapt-> Ic_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Ws_mult \t\t= %4.1f\n",  dp_adapt-> Ws_mult );
   LE_send_msg( GL_INFO, "DP_PRECIP: Grid_is_full \t\t= %4.1f\n",  dp_adapt-> Grid_is_full );
   LE_send_msg( GL_INFO, "DP_PRECIP: Min_blockage \t\t= %d\n",  dp_adapt-> Min_blockage );
   LE_send_msg( GL_INFO, "DP_PRECIP: Kdp_min_beam_blk \t\t= %d\n",  dp_adapt-> Kdp_min_beam_blk );
   LE_send_msg( GL_INFO, "DP_PRECIP: Hr_HighZThresh \t\t= %5.1f\n",  dp_adapt-> Hr_HighZThresh );
   LE_send_msg( GL_INFO, "DP_PRECIP: Kdp_max_beam_blk \t\t= %d\n",  dp_adapt-> Kdp_max_beam_blk );
   LE_send_msg( GL_INFO, "DP_PRECIP: Kdp_min_usage_rate \t\t= %5.1f\n",  dp_adapt-> Kdp_min_usage_rate );
   LE_send_msg( GL_INFO, "DP_PRECIP: Refl_min \t\t= %4.1f\n",  dp_adapt-> Refl_min );
   LE_send_msg( GL_INFO, "DP_PRECIP: Refl_max \t\t= %4.1f\n",  dp_adapt-> Refl_max );
   LE_send_msg( GL_INFO, "DP_PRECIP: Paif_area \t\t= %d\n",  dp_adapt-> Paif_area );
   LE_send_msg( GL_INFO, "DP_PRECIP: Paif_rate \t\t= %5.1f\n",  dp_adapt-> Paif_rate );
   LE_send_msg( GL_INFO, "DP_PRECIP: Max_vols_per_hour \t\t= %d\n",  dp_adapt-> Max_vols_per_hour );
   LE_send_msg( GL_INFO, "DP_PRECIP: Min_early_term_ang \t\t= %d\n",  dp_adapt-> Min_early_term_ang );
   LE_send_msg( GL_INFO, "DP_PRECIP: Max_precip_rate \t\t= %5.1f\n",  dp_adapt-> Max_precip_rate );
   LE_send_msg( GL_INFO, "DP_PRECIP: Num_zones \t\t= %d\n",  dp_adapt-> Num_zones );

   for(i=0; i<dp_adapt->Num_zones; i++)
   {
      sprintf(name, "Beg_azm%d", i+1);
      LE_send_msg( GL_INFO, "DP_PRECIP: %s \t\t= %5.1f\n",  name, dp_adapt -> Beg_azm[i] );
      sprintf(name, "End_azm%d", i+1);
      LE_send_msg( GL_INFO, "DP_PRECIP: %s \t\t= %5.1f\n",  name, dp_adapt -> End_azm[i] );
      sprintf(name, "Beg_rng%d", i+1);
      LE_send_msg( GL_INFO, "DP_PRECIP: %s \t\t= %d\n",  name, dp_adapt -> Beg_rng[i] );
      sprintf(name, "End_rng%d", i+1);
      LE_send_msg( GL_INFO, "DP_PRECIP: %s \t\t= %d\n",  name, dp_adapt -> End_rng[i] );
      sprintf(name, "Elev_ang%d", i+1);
      LE_send_msg( GL_INFO, "DP_PRECIP: %s \t\t= %4.2f\n",  name, dp_adapt -> Elev_ang[i] );
   }

#endif

  return 0; /* return OK */

} /* end dp_precip_callback_fx() ==================================================*/
