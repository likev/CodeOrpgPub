/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:25:59 $
 * $Id: cpu_use.h,v 1.2 2014/12/09 22:25:59 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef CPU_USE_H
#define CPU_USE_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Structure for Monitoring CPU use. */
typedef struct cpu_stats {

   unsigned long long total_time;	/* Total elapsed time, in jiffies. */

   unsigned long user_cpu_time;		/* Total user CPU time for pid, in 
                                           jiffies. */

   unsigned long sys_cpu_time;		/* Total system CPU time for pid, in 
                                           jiffies. */

   unsigned long idle_cpu_time;		/* Total idle CPU time for pid, in 
                                           jiffies. */

   int pid;				/* Process ID. */

   char proc_name[256];			/* Process name for pid. */

   unsigned int num_cpus;

   unsigned int frequency;
   
} Cpu_stats_t;

/* Function Prototypes. */
int CU_initialize( int pid );
int CU_cpu_use( int pid, Cpu_stats_t *stats );

#ifdef CPU_USE
/* Structure for CPU statistics. */
typedef struct stats_cpu {

   unsigned long long cpu_user;
   unsigned long long cpu_nice;
   unsigned long long cpu_sys;
   unsigned long long cpu_idle;
   unsigned long long cpu_iowait;
   unsigned long long cpu_steal;
   unsigned long long cpu_hardirq;
   unsigned long long cpu_softirq;
   unsigned long long cpu_guest;

} Stats_cpu_t;

#define STATS_CPU_SIZE  (sizeof(Stats_cpu_t))

static unsigned int Hz;
static unsigned int Num_CPUs;
#endif

#endif
