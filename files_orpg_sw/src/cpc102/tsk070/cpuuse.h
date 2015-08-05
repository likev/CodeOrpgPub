/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/04/30 13:46:58 $
 * $Id: cpuuse.h,v 1.7 2008/04/30 13:46:58 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
*/


#include <stdio.h>
#include <stdlib.h>            
#include <unistd.h>           
#include <errno.h>
#include <sys/types.h>       
#include <sys/stat.h>       
#include <fcntl.h>         
#include <math.h>
#include <string.h>
#include <assert.h>
#include <orpg.h>

#ifndef CPUUSE_H
#define CPUUSE_H

extern int errno;

/* macro definitions */ 
#define HEADER_REC   			1
#define ELEV_REC     			2
#define VOL_REC     			4
#define TASK_REC     			8
#define INIT_REC			16
#define UNKNOWN_REC  			32

#define MAX_ELEVATIONS		 	25
#define MAX_NUM_TASKS			255
#define STATS_OVERHEAD			2

/* typedefs  ..... */

typedef struct {

   int month;
   int day;
   int year;
   int hour;
   int minute;
   int second;

} Date_time_t;

typedef struct {

   int cut_number;
   Date_time_t dt;
   int vcp_number;

} Elev_rec_t;

typedef struct {

   int vol_number;
   Date_time_t dt;
   int vcp_number;

} Vol_rec_t;

typedef struct {

   Vol_rec_t vol;
   int vcp_duration;

} Vol_hdr_t;

typedef struct {

   char task_name[132];
   int instance; 
   int pid; 
   unsigned int cpu; 
   unsigned int mem; 
   unsigned int life;
   char node_name[8];

} Task_rec_t; 

typedef struct proc_stats_t{

   int pid;
   int cpu;
   struct proc_stats_t *next;

} Proc_stats_t;

typedef struct cpu_stats_t{

   char task_name[132];
   char node_name[8];
   struct proc_stats_t stats;

} Cpu_stats_t;

typedef union {

   int task_id;
   char units[4];

} linelabel_t;

typedef struct {

   char task_name[ORPG_TASKNAME_SIZ];
   linelabel_t label;
   int cpu[MAX_ELEVATIONS];
    
} Vol_stats_t;

typedef struct Vol_stats_list {

   Vol_stats_t *vol_stats[MAX_NUM_TASKS];
   int num_elevs;
   Vol_hdr_t vol_hdr;
   struct Vol_stats_list *next;

} Vol_stats_list_t;

/* Global Variables. */
char Ttcf_fname[256];
char Node_name[8];

/* Function Prototypes. */
int CPUUSE_initialize( );
int CPUUSE_init_vol_stats();
int CPUUSE_save_vol_stats( Vol_hdr_t *vol_hdr );
int CPUUSE_compute_cpu_utilization( char *buf, time_t monitor_time,
                                    int elevation_cut, int len );
int CPUUSE_write_cpu_utilization( Vol_stats_list_t *vol_stats_list );

int CPUUSE_sum_and_avg( Vol_stats_list_t **array, Vol_stats_list_t **avg,
                        int start_vol, int end_vol );
int CPUUSE_build_tat_directory();

#endif
