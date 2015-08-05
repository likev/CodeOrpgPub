/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/16 15:07:53 $
 * $Id: orpgbdr.h,v 1.12 2011/03/16 15:07:53 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#include <stdlib.h>
#include <basedata.h>
#include <orpg.h>
#include <rdacnt.h>

#ifndef ORPGBDR_H
#define ORPGBDR_H

#define ALL_SUB_TYPES           0xffffffff

typedef struct replay_elev_t{

   int num_radials;			/* Number of radials this elevation. */
   int rpg_elev_ind;			/* RPG elevation index for this elevation. */
   int sub_type;			/* Type of data this elevation.  This further
                                           identifies the type of data.  (e.g., sub-type
                                           might be REFLDATA_TYPE, COMBBASE_TYPE, ... ). */
   LB_id_t radial_msgid[MAX_SR_RADIALS];	
					/* LB message ID for each radial in elevation. */

} Replay_elev_t;

typedef struct replay_volume_t{

   int vol_seq_num;		/* Volume sequence number */
   int vol_start_time;		/* Volume scan start time (seconds 
				   since midnight) */
   int vol_start_date;		/* Volume scan start date. (Modified
				   Julian) */
   int vol_cov_pat;		/* Volume coverage pattern number. */
   int num_elev_cuts;		/* Number of RPG elevation scans. */
   int num_completed_cuts;	/* Number of complete elevation scans.
				   This is used to determine whether
				   accounting data is complete for this
				   volume scan. */
   struct replay_elev_t elevs[MAX_ELEVS];
				/* Elevation scan data. */

} Replay_volume_t;

typedef struct radial_replay_t{

   int data_id;			/* Radial replay data ID */
   int class_id;		/* Data class ID. */
   int LB_id_data;		/* LB data ID where radial replay data is 
				   to be stored. */
   int LB_id_acct_data;		/* LB data ID of accounting file. */
   int lbfd;			/* Used for LB_direct function calls. */
   Replay_volume_t *vol_data;	/* Pointer to accounting data for this
				   volume scan. */

} Radial_replay_t;

/* Macro definitions. */
#define ORPGBDR_INVALID_DATATYPE   -1
#define ORPGBDR_ERROR              -2
#define ORPGBDR_WRITE_FAILED       -3
#define ORPGBDR_DATA_NOT_FOUND     -4

#define MAXN_TYPES                  2
#define ORPGBDR_MAX_ACCT_VOLS       2


/* Function prototypes. */
int ORPGBDR_reg_radial_replay_type( int data_type, int data_lb_id,
                                    int acct_data_lb_id );
int ORPGBDR_write_radial( int data_type, int sub_type, void *ptr, int len );
LB_id_t ORPGBDR_get_start_of_elevation_msgid( int data_type, int sub_type, int vol_seq_num,
                                              int elev_ind );
LB_id_t ORPGBDR_get_end_of_elevation_msgid( int data_type, int sub_type, int vol_seq_num,
                                            int elev_ind );
LB_id_t ORPGBDR_get_start_of_volume_msgid( int data_type, int sub_type, int vol_seq_num );
LB_id_t ORPGBDR_get_end_of_volume_msgid( int data_type, int sub_type, int vol_seq_num );
int ORPGBDR_get_num_completed_cuts( int data_type, int vol_seq_num ); 
int ORPGBDR_get_num_elevation_cuts( int data_type, int vol_seq_num ); 
int ORPGBDR_get_start_date_and_time( int data_type, int vol_seq_num,
                                     int *start_date, time_t *start_time ); 
LB_id_t ORPGBDR_check_complete_volume( int data_type, int sub_type, int vol_seq_num );
LB_id_t ORPGBDR_check_complete_elevation( int data_type, int sub_type, int vol_seq_num,
                                          int elev_ind );
int ORPGBDR_set_read_pointer( int data_type, LB_id_t msg_id,int position );
int ORPGBDR_read_radial( int data_type, void *buf, int buflen,
                         LB_id_t msg_id );
int ORPGBDR_read_radial_direct( int data_type, void *buf, LB_id_t msg_id );

#endif
