/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:02:43 $
 * $Id: memstat.c,v 1.2 2014/03/18 18:02:43 jeffs Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#include <perf_mon.h>
#include <glibtop/mem.h>

/* Type Definitions. */
/* Note: All memory values are in bytes.   The output of free and the 
         information in /proc/meminfo are in kB.   1 kB = 1024 bytes. */
typedef struct Mem_stats {

   guint64 total;             /* Total usable RAM (excludes kernel binary code) */

   guint64 free;              /* The sum of LowFree + HighFree (see /proc/meminfo) */

   guint64 used;              /* Difference total - free */

   guint64 buffers;           /* Relatively temporary storage for raw disk blocks. */

   guint64 cached;            /* In-memory cache for files read from the disk. */

   guint64 user;              /* used - buffers - cached (see "free" command output) */

   guint64 available;         /* free + buffers + cached (see "free" command output) */

} Mem_stats_t;


typedef struct Memory_utilization {

   guint64 total;             /* Total Memory available, in bytes. */

   double used;               /* Percentage of memory used. */

   double free;               /* Percentage of memory free. */

} Memory_utilization_t;

/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern time_t Start_time;
extern struct utsname *Uname_info;
extern int Verbose;

/* Static Global Variables. */
static Mem_stats_t Mem_usage;
static Memory_utilization_t Memory_utilization;

/* These are for computing average Memory utilization during the performance
   monitoring period. */
static double Total_used = 0.0;
static double Total_free = 0.0;
static int Total_reports = 0;

/* These are for computing volume averages. */
static double Volume_free = 0;
static double Volume_used = 0;
static unsigned int Volume_time = 0;
static int Elev_cnt = 0;

static glibtop_mem Memory;
static int Mem_fd = -1;
static char Buffer[256];

/* Function Prototypes. */
static void Compute_utilization( );
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec, 
                           int terminate );

/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes Memory statistics and opens the Memory stats file.  The
      file will be removed if the file already exists.  It will then
      create the file hostname.mem.stats in the RPG work directory 
      (typically the "tmp" directory under $HOME). 


//////////////////////////////////////////////////////////////////////////\*/
int MS_init_mem_usage( ){

   char name[256];
   char file[256];

   /* Set the Memory usage area to all zeros. */
   memset( &Mem_usage, 0, sizeof( Mem_stats_t ) );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );

      /* Files will be created in the current working directory. */
      name[0] = '\0';

   }

   sprintf( file, "/%s.mem.stats", Uname_info->nodename );
   strcat( name, file );

   /* Open the file.  If Mem_fd is not negative, the file exists.
      Close the file, remove it, then re-open the file with create. */
   if( (Mem_fd = open( name, O_RDWR, 0666 )) >= 0 ){
   
      close( Mem_fd );
      unlink( name );
      Mem_fd = -1;

   }

   /* Create the "mem.stats" file. */
   if( (Mem_fd = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

      fprintf( stderr, "Open Failed For FILE %s\n", name );
      Mem_fd = -1;

   }

   /* This function reads data from /proc/meminfo. */
   glibtop_get_mem( &Memory );
   Total_memory = Memory.total;

   return Mem_fd;

/* End of MS_init_memory_usage() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Calls glibtop_get_mem() to obtain information from /proc/meminfo.
      Information returned by call is packed into Mem_stat_t buffer.
      The information is essentially the same as what is returned by the 
      "free" command.

   Inputs:
      scan_type - VOLUME_EVENT, ELEVATION_EVENT or NO_EVENT.
      first_time - first time through flag.   If true, do not write
                   stats to stats file.

//////////////////////////////////////////////////////////////////////////\*/
void MS_get_memory_usage( int scan_type, int first_time ){

   int size, year, mon, day, hour, min, sec;

   /* This function reads data from /proc/meminfo. */
   glibtop_get_mem( &Memory );

   /* If the gnuplot file is open, write Memory stats to file. */
   if( Mem_fd < 0 )
      return;

   /* Fill the local data structure with data. */
   Mem_usage.total = Memory.total;	/* MemTotal: */
   Mem_usage.free = Memory.free;	/* MemFree: */
   Mem_usage.used = Memory.used;	/* MemTotal: - MemFree: */
   Mem_usage.buffers = Memory.buffer; 	/* Buffers: */
   Mem_usage.cached = Memory.cached;	/* Cached: */

   /* The next two items should agree with the -/+ buffers/cache line
      of the "free" command. */
   Mem_usage.user = Memory.user;	/* MemTotal: - MemFree: - Cached: - Buffers: */
   Mem_usage.available = Mem_usage.free + Mem_usage.buffers + Mem_usage.cached;	
					/* MemFree: + Cached: + Buffers: */
   /* Compute memory utilization. */
   Compute_utilization();

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
         sprintf( Buffer, "#================================================================\
==================\n" );

         size = strlen( Buffer );
         if( write( Mem_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Mem_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
                  mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

         size = strlen( Buffer );
         if( write( Mem_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Mem_fd Failed\n" );

         memset( Buffer, 0 , sizeof(Buffer) );
         sprintf( Buffer, "# Cut    Time (s)    Total(s)      Total (B)      Free (%%)     Used (%%)\n" );

         size = strlen( Buffer );
         if( write( Mem_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Mem_fd Failed\n" );

      }

   }

   /* The first time through this function will be at start of volume.  We don't want
      to write data to file at this point because we want the data to reflect the
      information between the start of the 2nd cut and the start of volume. */
   if( !first_time ){

      memset( Buffer, 0, sizeof(Buffer) );
      sprintf( Buffer, " %3d  %12u    %4d       %lld       %5.1f        %5.1f\n", 
               Prev_scan_info.rda_elev_num, (unsigned int) Prev_scan_info.scan_time, 
               (int) Delta_time, Memory_utilization.total, Memory_utilization.free, 
               Memory_utilization.used );

      size = strlen( Buffer );
      if( write( Mem_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Mem_fd Failed\n" );

      /* Increment the volume totals. */
      Volume_free += Memory_utilization.free * (double) Delta_time;
      Volume_used += Memory_utilization.used * (double) Delta_time;
      Volume_time += Delta_time;
      Elev_cnt++;

      /* Print out the volume totals. */
      if( VOLUME_EVENT == scan_type )
         End_of_volume( year, mon, day, hour, min, sec, 0 );

      /* This information goes to the main terminal. */
      if( Verbose )
         fprintf( stderr, "MEMORY--> Total: %lld (B),  Used: %5.1f (%%),  Free: %5.1f (%%)\n",
                  Memory_utilization.total, Memory_utilization.used, 
                  Memory_utilization.free );

   }

/* End of MS_get_memory_usage(). */
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
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec, 
                           int terminate ){

   int size; 
   double pfree = 0, pused = 0;

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#----------------------------------------------------------------\
------------------\n" );

   size = strlen( Buffer );
   if( write( Mem_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Mem_fd Failed\n" );

   /* In the event Volume_time or Elev_cnt are 0. */
   if( (Elev_cnt > 0) && (Volume_time > 0) ){

       pfree = Volume_free / Volume_time;
       pused = Volume_used / Volume_time;

   }

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Stats:              %4d       %lld       %5.1f        %5.1f\n",
            (int) Volume_time, Memory_utilization.total, pfree, pused );

   size = strlen( Buffer );
   if( write( Mem_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Mem_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#================================================================\
==================\n" );

   size = strlen( Buffer );
   if( write( Mem_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Mem_fd Failed\n" );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

   /* Reset values for next volume. */
   Volume_free = 0;
   Volume_used = 0;
   Volume_time = 0;
   Elev_cnt = 0;

   /* Write out the Mem stats header for next volume scan. */
   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
            mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

   size = strlen( Buffer );
   if( write( Mem_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Mem_fd Failed\n" );

   memset( Buffer, 0 , sizeof(Buffer) );
   sprintf( Buffer, "# Cut    Time (s)    Total(s)     Total (B)      Free (%%)     Used (%%)\n" );

   size = strlen( Buffer );
   if( write( Mem_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Mem_fd Failed\n" );

/* End of End_of_volume(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Computes the memory free and used, in percentages. 

//////////////////////////////////////////////////////////////////////////\*/
static void Compute_utilization( ){

   Memory_utilization.total = Mem_usage.total;
   Memory_utilization.used = (((double) Mem_usage.user) / ((double) Mem_usage.total)) * 100;
   Memory_utilization.free = (((double) Mem_usage.available) / ((double) Mem_usage.total)) * 100;

   /* Add values for running total. */
   Total_used += Memory_utilization.used;
   Total_free += Memory_utilization.free;
   Total_reports++;

/* End of Compute_utilization() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.

///////////////////////////////////////////////////////////////////////////\*/
void MS_cleanup(){

   /* Close memory stats file if file is open. */
   if( Mem_fd >= 0 ){

      int size;
      double ave_used = 0, ave_free = 0;
      time_t ctime = Scan_info.scan_time - Start_time;

      /* Do end of volume processing. */
      End_of_volume( 0, 0, 0, 0, 0, 0, 1 );

      /* Write out the average memory utilization. */
      memset( Buffer, 0, sizeof(Buffer) );

      /* In the event the Total_reports is 0. */
      if( Total_reports > 0 ){

         ave_used = (double) ((double) Total_used / (double) Total_reports);
         ave_free = (double) ((double) Total_free / (double) Total_reports);

      }

      sprintf( Buffer, "# Avgs:               %4d       %lld       %5.1f        %5.1f\n",
               (int) ctime, Memory_utilization.total, ave_free, ave_used );

      size = strlen( Buffer );
      if( write( Mem_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Mem_fd Failed\n" );

      close( Mem_fd );

   }

/* End of MS_cleanup(). */
}

