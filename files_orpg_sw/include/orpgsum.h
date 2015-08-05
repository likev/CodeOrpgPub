/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/02/06 22:45:34 $
 * $Id: orpgsum.h,v 1.8 2014/02/06 22:45:34 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#ifndef ORPGSUM_H
#define ORPGSUM_H

#include <orpg.h>
#include <gen_stat_msg.h>

#define MAX_SCAN_SUM_VOLS	(MAX_VSCANS-1)
#define SCAN_SUMMARY_ID		1
#define ORPGSUM_UNDEFINED_CUT	0xff

typedef struct {

   int volume_start_date;	/* Modified Julian start date */

   int volume_start_time;	/* From midnight, in seconds */

   int weather_mode;		/* 2 = convective, 1 = clear air */

   int vcp_number;		/* Pattern number:
                     		   Maintenance/Test: > 255
                     		   Operational: <= 255
                     		   Constant Elevation Types: 1 - 99 */

   short rpg_elev_cuts;		/* Number of RPG elevation cuts in VCP */

   short rda_elev_cuts;		/* Number of RDA elevation cuts in VCP */

   int spot_blank_status;	/* Bitmap indicating whether elevation
				   cut has spot blanking enabled */

   unsigned char super_res[ECUT_UNIQ_MAX];	
				/* Bitmap indicating whether elevation
				   cut is Super Resolution.   Each byte 
                                   corresponds to an RPG elevation cut. 
				   Bits are defined in vcp.h */

   unsigned char last_rda_cut;	/* Cut number of last RDA elevation number in VCP.  
                                   This could be different from VCP definition if
                                   AVSET is active.  A value of 0xff indicates 
                                   undefined. */

   unsigned char last_rpg_cut;	/* Cut number of last RPG elevation number in VCP.
                                   This could be different from VCP definition if
                                   AVSET is active.  A value of 0xff indicates
                                   undefined. */

   unsigned char avset_status;  /* Set to 2 if AVSET enabled, 4 if disabled. */

   unsigned char n_sails_cuts;  /* Number of SAILS cuts. */

   unsigned short avset_term_ang; /* AVSET termination angle (deg*10) if AVSET active. 
                                     Otherwise, 0. */

   unsigned short spare;	/* Spare. */

} Scan_Summary;

/* Note:  Scan Summary Data is defined as 81 volume scans.  Index 0
   (volume scan number 0 is reserved for the initial volume scan. */
typedef struct {

   Scan_Summary scan_summary[MAX_VSCANS];

} Summary_Data;

Scan_Summary* ORPGSUM_get_scan_summary(int vol_num );
Summary_Data* ORPGSUM_get_summary_data( );
int ORPGSUM_set_summary_data( Summary_Data* summary_data );
int ORPGSUM_read_summary_data( void *buf );

#endif /* #ifndef ORPGSUM_H */
