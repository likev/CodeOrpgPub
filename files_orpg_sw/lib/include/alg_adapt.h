/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/02/25 22:34:36 $
 * $Id: alg_adapt.h,v 1.7 2009/02/25 22:34:36 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef ALG_ADAPT_H
#define ALG_ADAPT_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef RPGC_LIBRARY

#define RPG_ade_get_values RPGC_ade_get_values
#define RPG_ade_get_string_values RPGC_ade_get_string_values
#define RPG_abort_task RPGC_abort_task
#include <rpgc.h>

#else

#include <rpg.h>

#endif

/*** Function Prototypes ***/
int cell_prod_callback_fx( void * );
int hail_callback_fx( void * );
int hydromet_acc_callback_fx( void * );
int hydromet_adj_callback_fx( void * );
int hydromet_prep_callback_fx( void * );
int hydromet_rate_callback_fx( void * );
int layer_reflectivity_callback_fx( void * );
int mda_callback_fx( void * );
int Hca_callback_fx( void * );
int mode_select_callback_fx( void * );
int mpda_callback_fx( void * );
int precip_detect_callback_fx( void * );
int radazvd_callback_fx( void * );
int recclalg_callback_fx( void * );
int saa_callback_fx( void * );
int storm_cell_component_callback_fx( void * );
int storm_cell_seg_callback_fx( void * );
int storm_cell_track_callback_fx( void * );
int superob_callback_fx( void * );
int tda_callback_fx( void * );
int vad_callback_fx( void * );
int vil_echo_tops_callback_fx( void * );
int vad_rcm_heights_callback_fx( void * );

#endif
