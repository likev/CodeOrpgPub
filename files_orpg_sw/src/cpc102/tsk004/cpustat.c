/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:17 $
 * $Id: cpustat.c,v 1.1 2011/05/22 16:48:17 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#include <perf_mon.h>

/* Type Definitions. */
typedef struct Cpu_stats {

   /* Note: All times should be scale by "frequency" to get time in seconds. */

   guint64 total;             /* Total elapsed time.  Sum of User, Nice, Sys, Idle,
                                 IOwait, IRQ and SoftIRQ time.  This is also the 
                                 total over all CPUs. */

   guint64 user;              /* Elapsed CPU time while executing at the user level. */

   guint64 nice;              /* Elapsed CPU time while executing at the user level with 
                                 nice priority. */

   guint64 system;            /* Elapsed CPU time while executing system level (kernel) code. */

   guint64 iowait;            /* Elapsed time the CPU is idle waiting on I/O request. */

   guint64 idle;              /* Elapsed time the CPU was idle having nothing to do. */

   guint64 irq;               /* Elapsed time the CPU is servicing interrupts. */

   guint64 softirq;           /* Elapsed time the CPU is servicing soft interrupts. */

   guint64 busy;              /* Total of user, nice, system, iowait, irq, and softirq. */

   guint64 frequency;         /* CPU frequency (e.g., 100 --> 1/100 sec ). */

   /* The following information is per CPU information.  For definitions of fields,
      see above. */
   guint64 x_total[GLIBTOP_NCPU];

   guint64 x_user[GLIBTOP_NCPU];

   guint64 x_nice[GLIBTOP_NCPU];

   guint64 x_system[GLIBTOP_NCPU];

   guint64 x_idle[GLIBTOP_NCPU];

   guint64 x_iowait[GLIBTOP_NCPU];

   guint64 x_irq[GLIBTOP_NCPU];

   guint64 x_softirq[GLIBTOP_NCPU];

   guint64 x_busy[GLIBTOP_NCPU];

} Cpu_stats_t;


typedef struct CPU_utilization {

   double total;              /* Total CPU time. */

   double idle;               /* Percentage of idle time. */

   double busy;               /* Percentage of busy time. */

   double nice;               /* Percentage of busy time. */

   double iowait;             /* Percentage of I/O Wait time. */

   double system;             /* Percentage of system time.. */

   double irq;                /* Percentage of irq time.. */

   double softirq;            /* Percentage of softirq time.. */

   double x_idle[GLIBTOP_NCPU];

   double x_busy[GLIBTOP_NCPU];

} CPU_utilization_t;

/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern time_t Delta_clock;
extern time_t Start_time;
extern struct utsname *Uname_info; 
extern int Verbose;

/* Static Global Variables. */
static Cpu_stats_t Cpu_usage;
static Cpu_stats_t Previous_cpu_usage;
static CPU_utilization_t CPU_utilization;

/* These are for computing average CPU utilization during the performance
    monitoring period. */
static double Total_cpu_time = 0;
static double Total_idle_time = 0;
static double Total_busy_time = 0;
static double Total_iowait_time = 0;
static double Total_system_time = 0;

/* These are for computing average CPU utilization for a volume scan. */
static unsigned int Volume_time = 0;
static double Volume_cpu_time = 0;
static double Volume_idle_time = 0;
static double Volume_busy_time = 0;
static double Volume_iowait_time = 0;
static double Volume_system_time = 0;
static double Est_proc = 0;

static double X_total_busy_time[GLIBTOP_NCPU];
static double X_total_idle_time[GLIBTOP_NCPU];

static int Cpu_fd = -1;
static char Buffer[256];

glibtop_cpu Cpu;

/* Function Prototypes. */
static void Compute_utilization( );
static void End_of_volume( int year, int mon, int day, int hour, 
                           int min, int sec, int terminate );


/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes CPU statistics and opens the CPU stats file.  The
      file will be removed if the file already exists.  It will then
      create the file hostname.cpu.stats in the RPG work directory 
      (typically the "tmp" directory under $HOME). 

//////////////////////////////////////////////////////////////////////////\*/
int CS_init_cpu_usage( ){

   char name[256];
   char file[256];

   /* Set the CPU usage area to all zeros. */
   memset( &Cpu_usage, 0, sizeof( Cpu_stats_t ) );
   memset( &Previous_cpu_usage, 0, sizeof( Cpu_stats_t ) );
   memset( &X_total_busy_time[0], 0, sizeof(double)*GLIBTOP_NCPU );
   memset( &X_total_idle_time[0], 0, sizeof(double)*GLIBTOP_NCPU );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );

      /* Files will be created in the current working directory. */
      name[0] = '\0';

   }

   sprintf( file, "/%s.cpu.stats", Uname_info->nodename );
   strcat( name, file );

   /* Open the file.  If Cpu_fd is not negative, the file exists.
      Close the file, remove it, then re-open the file with create. */
   if( (Cpu_fd = open( name, O_RDWR, 0666 )) >= 0 ){

      close( Cpu_fd );
      unlink( name );
      Cpu_fd = -1;

   }

   /* Create the "cpu.stats" file. */
   if( (Cpu_fd = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

      fprintf( stderr, "Open Failed For FILE %s\n", name );
      Cpu_fd = -1;

   }

   return Cpu_fd;

/* End of CS_init_cpu_usage() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Calls glibtop_get_cpu() (via CS_read_stats) to obtain information i
      from /proc/stat.  Information returned by call is packed into 
      Cpu_stat_t buffer.

      Compute_utilization is called to compute CPU statistics.

      After the statistics are computed, if the "cpu.stats" file is open, 
      this function writes CPU stats to this file.

   Inputs:
      scan_type - VOLUME_EVENT, ELEVATION_EVENT or NO_EVENT
      first_time - first time through flag.   If true, do not write
                   stats to stats file.



//////////////////////////////////////////////////////////////////////////\*/
void CS_get_cpu_usage( int scan_type, int first_time ){

   int i, size, year, mon, day, hour, min, sec;

   /* Read the CPU data. */
   CS_read_stats();

   /* Compute utilization. */
   Compute_utilization( );

   /* If the gnuplot file is open, write CPU stats to file. */
   if( Cpu_fd < 0 )
      return;

   /* Did a start of volume event occur? */
   if( VOLUME_EVENT == scan_type ){

      unix_time( &Scan_info.scan_time, &year, &mon, &day, &hour, &min, &sec ); 
      if( year >= 2000 )
         year -= 2000;

      else 
         year -= 1900;

      /* Only do this the first time through. */
      if( first_time ){

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "#==============================================================\
====================\n" );

         size = strlen( Buffer );
         if( write( Cpu_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Cpu_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME: %4d   VCP: %3d\n",
                  mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

         size = strlen( Buffer );
         if( write( Cpu_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Cpu_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "# Cut    Time (s)    Total (s)    Idle (%%)    Busy (%%)   IOwait (%%)\
  System (%%)\n" );

         size = strlen( Buffer );
         if( write( Cpu_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Cpu_fd Failed\n" );

      }

   }

   /* The first time through this function will be at start of volume.  We don't want
      to write data to file at this point because we want the data to reflect the
      information between the start of the 2nd cut and the start of volume. */
   if( !first_time ){

      memset( Buffer, 0, sizeof(Buffer) );
      sprintf( Buffer, " %3d  %12u    %4d         %5.1f       %5.1f        %5.1f       %5.1f\n", 
               Prev_scan_info.rda_elev_num, (unsigned int) Prev_scan_info.scan_time, 
               (int) Delta_time, CPU_utilization.idle, CPU_utilization.busy,
               CPU_utilization.iowait, CPU_utilization.system );

      size = strlen( Buffer );
      if( write( Cpu_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Cpu_fd Failed\n" );

      /* Increment the volume totals. */
      Volume_time += Delta_time;
      Volume_cpu_time += CPU_utilization.total;
      Volume_idle_time += (CPU_utilization.idle * CPU_utilization.total / 100.0);
      Volume_busy_time += (CPU_utilization.busy * CPU_utilization.total / 100.0);
      Volume_iowait_time += (CPU_utilization.iowait * CPU_utilization.total / 100.0);
      Volume_system_time += (CPU_utilization.system * CPU_utilization.total / 100.0);

      /* Print out the volume statistics at the start of volume event. */
      if( VOLUME_EVENT == scan_type )
         End_of_volume( year, mon, day, hour, min, sec, 0 );

      /* This information goes to the main terminal. */
      if( Verbose ){

         fprintf( stderr, "CPU--> Total: %5d, Idle: %5.1f (%%), Busy: %5.1f (%%), Est # Processors: %5.2f\n",
                  (int) round( CPU_utilization.total ), CPU_utilization.idle, 
                  CPU_utilization.busy, Est_proc );

         for( i = 0; i < Num_CPUs; i++ )
            fprintf( stderr, "------------>CPU%d--> Idle: %5.1f (%%), Busy: %5.1f (%%)\n",
                     i, CPU_utilization.x_idle[i], CPU_utilization.x_busy[i] );

      }

   }

/* End of CS_get_cpu_usage(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Handles end of volume scan processing. 

   Inputs:
      year - year in yyyy format
      mon - month in mm format
      day - day in dd format
      hour - hour in hh format
      min - minute in mm format
      sec - seconds in ss format
      terminate - 1 - yes, 0 - no

//////////////////////////////////////////////////////////////////////////\*/
static void End_of_volume( int year, int mon, int day, int hour, 
                           int min, int sec, int terminate ){

   int size;
   double idle = 0, busy = 0, iowait = 0, system = 0;

   /* Print out the volume statistics. */
   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#--------------------------------------------------------------\
--------------------\n" );
   size = strlen( Buffer );
   if( write( Cpu_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Cpu_fd Failed\n" );

   /* In the event Volume_cpu_time is 0. */
   if( Volume_cpu_time > 0 ){

      idle = (Volume_idle_time / Volume_cpu_time) * 100.0;
      busy = (Volume_busy_time / Volume_cpu_time) * 100.0;
      iowait = (Volume_iowait_time / Volume_cpu_time) * 100.0;
      system = (Volume_system_time / Volume_cpu_time) * 100.0;

   }

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Stats:              %4d         %5.1f       %5.1f        %5.1f       %5.1f\n",
            (int) Volume_time, idle, busy, iowait, system );

   size = strlen( Buffer );
   if( write( Cpu_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Cpu_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#==============================================================\
====================\n" );

   size = strlen( Buffer );
   if( write( Cpu_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Cpu_fd Failed\n" );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

   /* Reset values for next volume. */
   Volume_time = 0;
   Volume_cpu_time = 0;
   Volume_idle_time = 0;
   Volume_busy_time = 0;
   Volume_iowait_time = 0;
   Volume_system_time = 0;

   /* Write the CPU Stats header for next volume scan. */
   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME: %4d   VCP: %3d\n",
            mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

   size = strlen( Buffer );
   if( write( Cpu_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Cpu_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Cut    Time (s)    Total (s)    Idle (%%)    Busy (%%)   IOwait (%%)\
  System (%%)\n" );

   size = strlen( Buffer );
   if( write( Cpu_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Cpu_fd Failed\n" );

/* End of End_of_volume() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Reads the CPU information from /proc.  This is done via glibtop
      library call to glibtop_get_cpu. 

//////////////////////////////////////////////////////////////////////////\*/
void CS_read_stats( ){

   int i;

   double delta_cpu_total;

   /* Save previous CPU statistics. */
   memcpy( &Previous_cpu_usage, &Cpu_usage, sizeof(Cpu_stats_t) );

   /* This function reads data from /proc/stat. */
   glibtop_get_cpu( &Cpu );

   /* Fill the local data structure with data. */
   Cpu_usage.total = Cpu.total;
   Cpu_usage.user = Cpu.user;
   Cpu_usage.nice = Cpu.nice;
   Cpu_usage.iowait = Cpu.iowait;
   Cpu_usage.system = Cpu.sys;
   Cpu_usage.idle = Cpu.idle;
   Cpu_usage.irq = Cpu.irq;
   Cpu_usage.softirq = Cpu.softirq;
   Cpu_usage.frequency = Cpu.frequency;

   Cpu_usage.busy = Cpu_usage.user + Cpu_usage.nice + Cpu_usage.iowait +
                    Cpu_usage.irq + Cpu_usage.softirq + Cpu_usage.system;

   /* The following estimates the processing capabilities.  A value
      near 1 means the CPU delta equals the clock delta. */
   if( Verbose ){

      delta_cpu_total = 
         (double) (Cpu_usage.total - Previous_cpu_usage.total) / Cpu.frequency;
   
      Est_proc = 0;
      if( Delta_clock > 0 )
         Est_proc = ( delta_cpu_total / (double) Delta_clock);

   }

   /* Do for all CPUs. */
   for( i = 0; i < Num_CPUs; i++ ){

      Cpu_usage.x_total[i] = Cpu.xcpu_total[i]; 
      Cpu_usage.x_user[i]= Cpu.xcpu_user[i]; 
      Cpu_usage.x_nice[i] = Cpu.xcpu_nice[i]; 
      Cpu_usage.x_system[i] = Cpu.xcpu_sys[i]; 
      Cpu_usage.x_idle[i] = Cpu.xcpu_idle[i]; 
      Cpu_usage.x_iowait[i] = Cpu.xcpu_iowait[i]; 
      Cpu_usage.x_irq[i] = Cpu.xcpu_irq[i]; 
      Cpu_usage.x_softirq[i] = Cpu.xcpu_softirq[i]; 

      Cpu_usage.x_busy[i] = Cpu_usage.x_user[i] + Cpu_usage.x_nice[i]
                          + Cpu_usage.x_iowait[i] + Cpu_usage.x_irq[i] 
                          + Cpu_usage.x_softirq[i] + Cpu_usage.x_system[i];

   }

/* End of CS_read_stats(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Computes the CPU idle, busy, iowait, system, irq and softirq
      percentage of time. 

//////////////////////////////////////////////////////////////////////////\*/
static void Compute_utilization( ){

   double idle, busy, iowait, system, nice, irq, softirq;
   int i;

   /* Total CPU. */
   CPU_utilization.total = (double) (Cpu_usage.total - Previous_cpu_usage.total);

   /* Idle time, in percent. */
   idle = (double) (Cpu_usage.idle - Previous_cpu_usage.idle); 
   CPU_utilization.idle = (double) ((idle / CPU_utilization.total) * 100.0);

   /* Busy time, in percent. */
   busy = (double) (Cpu_usage.busy - Previous_cpu_usage.busy); 
   CPU_utilization.busy = (double) ((busy / CPU_utilization.total) * 100.0);

   /* I/O wait time, in percent. */
   iowait = (double) (Cpu_usage.iowait - Previous_cpu_usage.iowait); 
   CPU_utilization.iowait = (double) ((iowait / CPU_utilization.total) * 100.0);
   
   /* System call time, in percent. */
   system = (double) (Cpu_usage.system - Previous_cpu_usage.system); 
   CPU_utilization.system = (double) ((system / CPU_utilization.total) * 100.0);

   /* System nice time, in percent. */
   nice = (double) (Cpu_usage.nice - Previous_cpu_usage.nice); 
   CPU_utilization.nice = (double) ((nice / CPU_utilization.total) * 100.0);

   /* System irq time, in percent. */
   irq = (double) (Cpu_usage.irq - Previous_cpu_usage.irq); 
   CPU_utilization.irq = (double) ((irq / CPU_utilization.total) * 100.0);

   /* System irq time, in percent. */
   softirq = (double) (Cpu_usage.softirq - Previous_cpu_usage.softirq); 
   CPU_utilization.softirq = (double) ((softirq / CPU_utilization.total) * 100.0);

   /* Add values to the running totals. */
   Total_cpu_time += CPU_utilization.total;
   Total_idle_time += idle;
   Total_busy_time += busy;
   Total_iowait_time += iowait;
   Total_system_time += system;

   /* Do For Each CPU .... */
   for( i = 0; i < Num_CPUs; i++ ){

      double total;

      /* Total CPU. */
      total = (double) (Cpu_usage.x_total[i] - Previous_cpu_usage.x_total[i]);

      /* Busy time. */
      busy = (double) (Cpu_usage.x_busy[i] - Previous_cpu_usage.x_busy[i]); 
      CPU_utilization.x_busy[i] = (double) ((busy / total) * 100.0);

      /* Idle time. */
      idle = (double) (Cpu_usage.x_idle[i] - Previous_cpu_usage.x_idle[i]); 
      CPU_utilization.x_idle[i] = (double) ((idle / total) * 100.0);

      X_total_idle_time[i] += CPU_utilization.x_idle[i] * total;
      X_total_busy_time[i] += CPU_utilization.x_busy[i] * total;

   }

/* End of Compute_utilization() */
} 

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.

///////////////////////////////////////////////////////////////////////////\*/
void CS_cleanup(){

   /* Close cpu stats file if file is open. */
   if( Cpu_fd >= 0 ){

      int size;
      double ave_idle = 0, ave_busy = 0, ave_iowait = 0, ave_system = 0;
      time_t ctime = Scan_info.scan_time - Start_time;

      /* Do end of volume processing. */
      End_of_volume( 0, 0, 0, 0, 0, 0, 1 );

      /* Write out the average CPU utilization. */
      memset( Buffer, 0, sizeof(Buffer) );

      /* In the event Total_cpu_time is 0. */
      if( Total_cpu_time > 0 ){

         ave_idle = (Total_idle_time / Total_cpu_time) * 100.0;
         ave_busy = (Total_busy_time / Total_cpu_time) * 100.0;
         ave_iowait = (Total_iowait_time / Total_cpu_time) * 100.0;
         ave_system = (Total_system_time / Total_cpu_time) * 100.0;

      }

      sprintf( Buffer, "# Avgs:               %4d         %5.1f       %5.1f        %5.1f       %5.1f\n",
               (int) ctime, ave_idle, ave_busy, ave_iowait, ave_system );

      size = strlen( Buffer );
      if( write( Cpu_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Cpu_fd Failed\n" );
      
      close( Cpu_fd );

   }
   
/* End of CS_cleanup(). */
}
