/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:02:43 $
 * $Id: perf_mon.h,v 1.3 2014/03/18 18:02:43 jeffs Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
#ifndef PERF_MON_H
#define PERF_MON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orpg.h>
#include <infr.h>
#include <math.h>
#include <sys/utsname.h>
#include <glibtop/sysinfo.h> 

/* Macros ... */
#define PM_VOLUME_DATA		1
#define PM_ELEVATION_DATA	2

#define NO_EVENT		  0
#define ELEVATION_EVENT		100
#define VOLUME_EVENT		200

/* Static Global Variables. */
const glibtop_sysinfo *Sys_info;
guint64 Num_CPUs;
guint64 Total_memory;
guint64 Total_swap;

/* Type definitions. */
typedef struct {

   int scan_type;		/* Either ELEVATION_EVENT, VOLUME_EVENT or NO_EVENT. */

   time_t scan_time;		/* UTC time. */

   short rda_elev_num;

   short vcp;

} Scan_info_t;

/* Function Prototypes. */

/* The following files defined in iostat.c. */
void DS_get_disk_usage( int scan_type, int first_time );
int DS_init_disk_usage();
void DS_cleanup();

/* The following functions defined in memstat.c. */
void MS_get_memory_usage( int scan_type, int first_time );
int MS_init_mem_usage();
void MS_cleanup();

/* The following functions defined in swapstat.c. */
void SS_get_swap_usage( int scan_type, int first_time );
int SS_init_swap_usage();
void SS_read_stats();
void SS_cleanup();

/* The following functions defined in cpustat.c. */
int CS_init_cpu_usage();
void CS_get_cpu_usage( int scan_type, int first_time );
void CS_read_stats();
void CS_cleanup();

/* The following functions defined in ldmstat.c. */
int LS_init_LDM_usage();
void LS_get_LDM_usage( int scan_type, int first_time );
int LS_read_stats();
void LS_cleanup();

/* The following functions defined in userstat.c. */
int US_init_user_usage();
void US_get_user_usage( int scan_type, int first_time );
int US_read_stats();
void US_cleanup();

/* The following functions defined in netstat.c. */
int IS_init_interface_usage( );
void IS_get_interface_usage( int scan_type, int first_time );
int IS_read_stats();
void IS_cleanup();

/* The following functions defined in signals.c. */
int SIG_reg_term_handler( int (*term_handler)(int, int) );

/* The following functions defined in rpg_perf_mon.c */
int RPG_register_events();

/* The following functions defined in rda_perf_mon.c */
int RDA_register_events();

#endif
