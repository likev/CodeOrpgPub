/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2014/04/25 17:23:55 $ 
 * $Id: rpgcs.h,v 1.17 2014/04/25 17:23:55 steves Exp $ 
 * $Revision: 1.17 $ 
 * $State: Exp $ 
 */ 
#ifndef RPGCS_H
#define RPGCS_H

#include <rpgc_globals.h>

#include <rpgcs_data_conversion.h>
#include <rpgcs_time_funcs.h>
#include <rpgcs_miscellaneous.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define RPGCS_ERROR           0x8000000
/* The following functions are declared in rpgcs_vcp_info.c */
int RPGCS_get_target_elev_ang( int vcp_num, int elev_ind );
int RPGCS_get_last_elev_index( int vcp_num );
Vcp_struct* RPGCS_get_vcp_data( int vcp_num );
int RPGCS_get_wxmode_for_vcp( int vcp_num );
int RPGCS_get_target_elev_ang( int vcp_num, int elev_ind );
short* RPGCS_get_elev_index_table( int vcp_num );
unsigned short* RPGCS_get_suppl_flags_table( int vcp_num );
int RPGCS_get_num_elevations( int vcp_num );
int RPGCS_get_rpg_elevation_num( int vcp_num, int elev_ind );
float RPGCS_get_elevation_angle( int vcp_num, int elev_ind );
int RPGCS_get_elev_waveform( int vcp_num, int elev_ind );
float RPGCS_get_azimuth_rate( int vcp_num, int elev_ind );
int RPGCS_get_super_res( int vcp_num, int elev_ind );
int RPGCS_remap_rpg_elev_index( int vcp_num, int elev_ind );

/* The following functions are declared in rpgcs_misc_prod_funcs.c */
void RPGCS_window_extraction( float radius_center, float azimuth_center,
                              float length, float *max_rad, float *min_rad,
                              float *max_theta, float *min_theta );

/* The following functions are declared in rpgcs_rda_status.c */
int RPGCS_get_CMD_status();
int RPGCS_is_CMD_applied_this_elev( int vcp_num, int rpg_elev_ind );

#ifdef __cplusplus
}
#endif

#endif
