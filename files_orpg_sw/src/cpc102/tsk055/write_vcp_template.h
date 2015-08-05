/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/23 21:23:46 $
 * $Id: write_vcp_template.h,v 1.4 2007/02/23 21:23:46 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/


#ifndef WRITE_VCP_TEMPLATE_H
#define WRITE_VCP_TEMPLATE_H

#include <stdio.h>
#include <string.h>
#include <orpgvcp.h>
#include <pulse_counts.h>

#define NO_ACTION            0
#define START_ELEV_ATTR      1
#define END_ELEV_ATTR        2
#define MAX_PRF_SECTORS      3

extern int Verbose_mode;

/* Structure templates. */

typedef struct {

   float edge_angle;
   int   dop_prf;

} Prf_sector_t;

typedef struct {

   int   pattern_num;
   int   wx_mode;
   int   num_elev_cuts;
   int   where_defined;
   int   pulse_width;
   int   cluttermap_group;
   float vel_reso;

} Vol_info_t;

typedef struct {

   int   cut_num; 			/* Elevation cut number( RDA_cut_number-1). */
   float elev_ang_deg;			/* Elevation angle, in degrees. */
   float scan_rate_deg;			/* Azimuth Scan Rate, in degrees/second. */
   int   phase; 
   int   waveform; 
   int   def_surv_prf; 
   int   def_surv_p_cnt;
   int   def_dop_prf; 
   int   def_dop_p_cnt[ MAX_DOP_PRI+1 ];
   Prf_sector_t prf_sector[MAX_PRF_SECTORS];
   float surv_snr; 
   float vel_snr; 
   float spw_snr;

} Cut_info_t;

/* Function prototypes. */
FILE* Get_filename( int pattern_num );
int Init_VCP_attr( Vol_info_t *vol_info );
int Init_elevation_attr( Cut_info_t *cut_info );
int Write_VCP_attr( Vol_info_t *vol_info );
int Write_elevation_attr( int flag, Cut_info_t *cut_info,
                          int num_alwb_prfs, int alwb_prfs[] );
int Write_elevation_info( Cut_info_t *cut_info, int num_alwb_prfs, 
                          int alwb_prfs[] );
void Close_file();

#endif
