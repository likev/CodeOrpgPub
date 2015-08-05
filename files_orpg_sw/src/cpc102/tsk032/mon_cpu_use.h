/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/11/19 20:32:13 $
 * $Id: mon_cpu_use.h,v 1.1 2008/11/19 20:32:13 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef MON_CPU_USE_H
#define MON_CPU_USE_H

#include <orpg.h>
#include <infr.h>
#include <cpu_use.h>

/* Macros ... */
#define VOLUME_DATA             	1
#define ELEVATION_DATA          	2

/* Current (as of 10/8/08) CPU capability of the rpga processor measured by
   use_resource. */
#define DEFAULT_RPGA_CAPABILITY		3990
#define DEFAULT_NUM_CPUS		2

#define NO_EVENT                  0
#define ELEVATION_EVENT         100
#define VOLUME_EVENT            200

/* Type definitions. */
typedef struct {

   int scan_type;               /* Either ELEVATION_EVENT, VOLUME_EVENT or NO_EVENT. */

   time_t scan_time;            /* UTC time. */

   int volnum;			/* Volume scan number. */

   short rda_elev_num;

   short vcp;

} Scan_info_t;


/* Static Global Variables. */
static Scan_info_t Scan_info;
static Scan_info_t Prev_scan_info;
static time_t Delta_time = 0;
static time_t Start_time = 0;

/* Process ID. */
static int Pid = -1;

/* Number of volume scans to monitor. */
static int Volume_scans = 1;

/* Used for estimating load on an operational RPG. */
static int Proc_capability = 0;
static double Scale_utilization = 0;
static int Num_CPUs = 0;

/* CPU Statistics. */
static Cpu_stats_t Curr_stats;
static Cpu_stats_t Prev_stats;

/* Termination flag. */
int Terminate = 0;

/* For maximum value tracking. */
double Max_elev_util = 0, Max_vol_util = 0;
double Max_elev_est_util = 0, Max_vol_est_util = 0;
int Max_elev_num = 0, Max_vol_vcp = 0, Max_elev_vcp = 0;
int Max_elev_vol_num = 0, Max_vol_num = 0;

/* Function Prototypes. */
static int Read_options( int argc, char *argv[] );
static void Print_usage( char *argv[] );
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg );
static char* Process_event_msg( int where, EN_id_t event, char *msg,
                                int msg_len );
static int Signal_handler( int signal, int status );
static int Cleanup_fxn( );

#endif
