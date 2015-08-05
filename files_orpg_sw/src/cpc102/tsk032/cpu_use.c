/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:25:59 $
 * $Id: cpu_use.c,v 1.2 2014/12/09 22:25:59 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#define CPU_USE
#include <cpu_use.h>
#include <glibtop/uptime.h>
#include <glibtop/cpu.h>
#include <glibtop/sysinfo.h>
#include <glibtop/procargs.h>
#include <glibtop/proctime.h>
#include <math.h>

/* Static Variables. */
static int Initialized = 0;
static char Command_line[256];
static char Process_name[256];
static int Pid = -1;

/* Function prototypes(). */
static void Get_Hz(void);
static void Get_proc_cpu_nr( void );
static int Get_stat( unsigned long long *total_cpu_time );
static int Get_pid_stat( int proc_pid, unsigned long *user_time, 
                         unsigned long *sys_time );
static void Get_proc_args( );

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Initialization module for CPU use.

   Inputs:
      pid - Process ID.

   Returns:
      The number of CPUs.

///////////////////////////////////////////////////////////////////////////\*/
int CU_initialize( int pid ){

   /* Initialize this module. */
   if( Initialized )
      return Num_CPUs;

   /* Save the PID. */
   Pid = pid;

   /* Get the frequency ... used to convert jiffies to time. */
   Get_Hz();

   /* Get Number of Processors. */
   Get_proc_cpu_nr();

   /* Get the command line arguments for the processs being monitored. */
   Get_proc_args();

   /* Write some useful information. */
   fprintf( stderr, "CMD LINE: %s\n", &Command_line[0] );
   fprintf( stderr, "# CPUs: %d\n", Num_CPUs );

   /* Done with initialization. */
   Initialized = 1;

   return Num_CPUs;

/* End of CU_initialize(). */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Main function for returning CPU use on a per process basis.

   Inputs:
      proc_pid - process ID (pid) of the process to monitor.

   Outputs:
      stats - Cpu_stats_t structure that holds CPU usage information for
              Process ID pid.

   Returns:
      -1 on error, 0 on success. 

   Notes:
      Some of the code was taken from SYSSTAT.

///////////////////////////////////////////////////////////////////////////\*/
int CU_cpu_use( int proc_pid, Cpu_stats_t *stats ){

   int ret, curr;

   /* Get processor stats. */
   curr = Get_stat( &stats->total_time );

   /* Get process stats. */
   stats->sys_cpu_time = stats->user_cpu_time = 0;
   if( (ret = Get_pid_stat( proc_pid, &stats->user_cpu_time, 
                            &stats->sys_cpu_time )) < 0 )
      return -1;

   /* Save the PID. */
   stats->pid = proc_pid;

   /* Save the process name. */
   strcpy( &stats->proc_name[0], &Process_name[0] );

   /* Used for scaling CPU times. */
   stats->num_cpus = Num_CPUs;
   stats->frequency = Hz;

   /* Return to caller. */
   return 0;

/* End of CU_cpu_use(). */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Get the frequency.   This number is used to convert the usage numbers 
      (in jiffies) into seconds.

///////////////////////////////////////////////////////////////////////////\*/
static void Get_Hz( void ){

   glibtop_cpu gltop_cpu;

   /* Get CPU time from glibtop. */
   glibtop_get_cpu( &gltop_cpu ); 

   Hz = (unsigned int) gltop_cpu.frequency;

/* End of Get_HZ(). */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Get number of CPUs on this machine.

///////////////////////////////////////////////////////////////////////////\*/
static void Get_proc_cpu_nr( void ){

   const glibtop_sysinfo *gltop_sysinfo = NULL;

   /* Get System Information. */
   gltop_sysinfo = glibtop_get_sysinfo(); 
   
   /* On failure, log error and set number of CPUs to 1. */
   if( gltop_sysinfo == NULL ){

      fprintf( stderr, "Error Obtaining Number of CPUs\n" );
      Num_CPUs = 1;

   }

   /* Set the number of CPUs from the system information. */
   Num_CPUs = gltop_sysinfo->ncpu;

/* End of Get_proc_cpu_nr(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Get stats for CPU.

   Outputs:
      Returns the total CPU time (includes idle, user, system, iowait,
      etc).

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////////\*/
static int Get_stat( unsigned long long *total_cpu_time ){

   static glibtop_cpu gltop_cpu;

   /* Get CPU time from glibtop. */
   glibtop_get_cpu( &gltop_cpu ); 

#ifdef DEBUG
   fprintf( stderr, "Total: %llu, User: %llu, Sys: %llu, Idle: %llu\n",
            gltop_cpu.total, gltop_cpu.user, gltop_cpu.sys, gltop_cpu.idle );
   fprintf( stderr, "-->Frequency: %llu\n", gltop_cpu.frequency );
#endif

   *total_cpu_time = gltop_cpu.total;

   return(0);
   
/* End of Get_stat() */
}

/*\//////////////////////////////////////////////////////////////////

   Description:
      Get the user and system time for the input pid.

   Input:
      prod_pid - process ID (pid)

   Output:
      user_time - user CPU time, in jiffies.
      sys_time - system CPU time, in jiffies.
      idle_time - Idle CPU time, in jiffies

   Returns:
      -1 on error, 0 on success.

//////////////////////////////////////////////////////////////////\*/
static int Get_pid_stat( int proc_pid, unsigned long *user_time, 
                         unsigned long *sys_time ){

   static glibtop_proc_time gltop_proc_time;

   /* Initialize the process time structure. */
   memset( &gltop_proc_time, 0, sizeof(glibtop_proc_time) );

   /* Get process information. */
   glibtop_get_proc_time( &gltop_proc_time, proc_pid );

   /* Set return values. */
   *user_time = gltop_proc_time.utime;
   *sys_time = gltop_proc_time.stime;

   /* Return to caller. */
   return 0;

/* End of Get_pid_stat() */
}


/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Creates the command line string of the process we are displaying 
      CPU  utilization for.

///////////////////////////////////////////////////////////////////////////\*/
static void Get_proc_args( ){

   char *temp_args = NULL;
   char **args_ptr = NULL;
   int i, j, len, size = 0;

   glibtop_proc_args gltop_proc_args;

   /* Get the process arguments. */
   args_ptr = glibtop_get_proc_argv( &gltop_proc_args, Pid, 255 );

   /* If error reading, return. */
   if( args_ptr == NULL )
      return;

   /* Create the command line string. */
   size = gltop_proc_args.size;

   i = 0;
   j = 0;
   len = 0;

   while( j < size ){

      temp_args = args_ptr[i]; 
      len = strlen( args_ptr[i] );

      /* Copy argument and replace trailing NULL with space. */
      strcpy( &Command_line[j], temp_args );
      strcpy( &Process_name[0], temp_args );
      j += len;
      Command_line[j] = ' ';

      /* Increment indices for next argument. */
      i++;
      j++;

   }

   /* Free the command line argument buffer. */
   g_strfreev( args_ptr );

/* End of Get_proc_args() */
}

