/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:02:44 $
 * $Id: swapstat.c,v 1.2 2014/03/18 18:02:44 jeffs Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#include <perf_mon.h>
#include <glibtop/swap.h>

/* Type Definitions. */
/* Note: All swap values are in bytes.   The output of swap and the information
         stored in /proc/meminfo are in kB.   1 kB = 1024 bytes. */
typedef struct Swap_stats {

   guint64 total;             /* Total usable swap space. */

   guint64 free;              /* The amount of free swap space. */

   guint64 used;              /* The amount of used swap space. */

   guint64 pagein;            /* The number of page in to memory. */

   guint64 pageout;           /* The number of page out to swap. */

} Swap_stats_t;


typedef struct Swap_utilization {

   guint64 total;             /* Total Swap space available. */

   double used;               /* Percentage of swap used. */

   double free;               /* Percentage of swap free. */

} Swap_utilization_t;

/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern time_t Start_time;
extern struct utsname *Uname_info;
extern int Verbose;

/* Static Global Variables. */
static Swap_stats_t Previous_swap_usage;
static Swap_stats_t Swap_usage;
static Swap_utilization_t Swap_utilization;

/* These are for computing average Swap utilization during the performance
   monitoring period. */
static double Total_used = 0.0;
static double Total_free = 0.0;
static guint64 Total_pagein = 0;
static guint64 Total_pageout = 0;
static int Total_reports = 0;

static glibtop_swap Swap;
static int Swap_fd = -1;
static char Buffer[256];

/* Used for volume statistics. */
static double Volume_used = 0;
static double Volume_free = 0;
static guint64 Volume_pagein = 0;
static guint64 Volume_pageout = 0;
static unsigned int Volume_time = 0;
static int Elev_cnt = 0;

/* Function Prototypes. */
static void Compute_utilization( );
static void End_of_volume( int year, int mon, int day, int hour,
                           int min, int sec, int terminate );

/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes Swap statistics. 

//////////////////////////////////////////////////////////////////////////\*/
int SS_init_swap_usage( ){

   char name[256];
   char file[256];

   /* Set the Swap usage area to all zeros. */
   memset( &Swap_usage, 0, sizeof( Swap_stats_t ) );
   memset( &Previous_swap_usage, 0, sizeof( Swap_stats_t ) );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );

      /* Create and open file in current working directory. */
      name[0] = '\0';

   }

   sprintf( file, "/%s.swap.stats", Uname_info->nodename );
   strcat( name, file );

   /* Open the file.  If Swap_fd is not negative, the file exists.
      Close the file, remove it, then re-opent the file with create. */
   if( (Swap_fd = open( name, O_RDWR, 0666 )) >= 0 ){
   
      close( Swap_fd );
      unlink( name );
      Swap_fd = -1;

   }

   /* Create the "swap.stats" file. */
   if( (Swap_fd = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

      fprintf( stderr, "Open Failed For FILE %s\n", name );
      Swap_fd = -1;

   }

   /* This function reads data from /proc/vmstat. */
   glibtop_get_swap( &Swap );
   Total_swap = Swap.total;

   return Swap_fd;

/* End of SS_init_swap_usage(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Calls glibtop_get_swap() to obtain information from /proc/vmstat and 
      /proc/meminfo.  Information returned by call is packed into Swap_stat_t 
      buffer.  The information is essentially the same as what is returned by 
      the "vmstat" (page-in/page-out) and "free" (total, used and free) 
      commands concerning the swap device.

   Inputs:
      scan_type - VOLUME_EVENT, ELEVATION_EVENT or NO_EVENT
      first_time - first time through flag.   If true, do not write
                   stats to stats file.

//////////////////////////////////////////////////////////////////////////\*/
void SS_get_swap_usage( int scan_type, int first_time ){

   guint64 pagein_delta, pageout_delta;
   int size, year, mon, day, hour, min, sec;

   /* Read the swap information. */
   SS_read_stats();

   /* Compute swap utilization. */
   Compute_utilization();

   /* If the gnuplot file is open, write Swap stats to file. */
   if( Swap_fd < 0 )
      return;


   /* Did a start of volume event occur? */
   if( VOLUME_EVENT == scan_type  ){

      unix_time( &Scan_info.scan_time, &year, &mon, &day, &hour, &min, &sec );
      if( year >= 2000 )
         year -= 2000;

      else
         year -= 1900;

      /* Only do this the first time through. */
      if( first_time ){

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "#========================================================\
=========================================================\n" );

         size = strlen( Buffer );
         if( write( Swap_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Swap_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
                  mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

         size = strlen( Buffer );
         if( write( Swap_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Swap_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "# Cut    Time (s)    Total (s)      Total (B)       Used (%%)     Free (%%)     \
Page-in    Page-out\n" );

         size = strlen( Buffer );
         if( write( Swap_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to Swap_fd Failed\n" );

      }

   }

   /* The first time through this function will be at start of volume.  We don't want
      to write data to file at this point because we want the data to reflect the
      information between the start of the 2nd cut and the start of volume. */
   if( !first_time ){

      memset( Buffer, 0, sizeof(Buffer) );

      pagein_delta = Swap_usage.pagein - Previous_swap_usage.pagein;
      pageout_delta = Swap_usage.pageout - Previous_swap_usage.pageout;

      sprintf( Buffer, " %3d  %12u    %4d         %lld         %4.1f        %4.1f     %8lld    %8lld\n", 
               Prev_scan_info.rda_elev_num, (unsigned int) Scan_info.scan_time, (int) Delta_time, 
               Swap_utilization.total, Swap_utilization.used, Swap_utilization.free, pagein_delta, 
               pageout_delta );

      size = strlen( Buffer );
      if( write( Swap_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Swap_fd Failed\n" );

      /* This information goes to the main terminal. */
      if( Verbose )
         fprintf( stderr, "SWAP--> Total: %lld (B), Used: %5.1f (%%), Free: %5.1f (%%)\n",
                  Swap_utilization.total, Swap_utilization.used, 
                  Swap_utilization.free );

      /* Increment the volume totals. */
      Volume_used += Swap_utilization.used;
      Volume_free += Swap_utilization.free;
      Volume_time += Delta_time;
      Volume_pagein += pagein_delta;
      Volume_pageout += pageout_delta;
      Elev_cnt++;
    
      /* Print out the volume statistics. */
      if( VOLUME_EVENT == scan_type )
         End_of_volume( year, mon, day, hour, min, sec, 0 );

   }

/* End of SS_get_swap_usage(). */
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

   double avg_used = 0, avg_free = 0;
   int size;

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#--------------------------------------------------------\
---------------------------------------------------------\n" );

   size = strlen( Buffer );
   if( write( Swap_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Swap_fd Failed\n" );

   /* In the event Elev_cnt is 0. */
   if( Elev_cnt > 0 ){

      avg_used = round( Volume_used / (double) Elev_cnt );
      avg_free = round( Volume_free / (double) Elev_cnt );

   }

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Stats:              %4d                            %4.1f        \
%4.1f     %8lld    %8lld\n",
            Volume_time, avg_used, avg_free, Volume_pagein, Volume_pageout );

   size = strlen( Buffer );
   if( write( Swap_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Swap_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#========================================================\
=========================================================\n" );

   size = strlen( Buffer );
   if( write( Swap_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Swap_fd Failed\n" );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

   /* Reset the volume totals. */
   Volume_used = 0;
   Volume_free = 0;
   Volume_time = 0;
   Elev_cnt = 0;

   /* Write out the swap header for the next volume scan. */
   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
            mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

   size = strlen( Buffer );
   if( write( Swap_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Swap_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Cut    Time (s)    Total (s)      Total (B)       Used (%%)     Free (%%)     \
Page-in    Page-out\n" );

   size = strlen( Buffer );
   if( write( Swap_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to Swap_fd Failed\n" );

/* End of End_of_volume(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Reads the swap information. 

//////////////////////////////////////////////////////////////////////////\*/
void SS_read_stats(){

   /* Copy swap usage to previous. */
   memcpy( &Previous_swap_usage, &Swap_usage, sizeof(Swap_stats_t) );

   /* This function reads data from /proc/vmstat. */
   glibtop_get_swap( &Swap );

   /* Fill the local data structure with data. */
   Swap_usage.total = Swap.total;
   Swap_usage.free = Swap.free;
   Swap_usage.used = Swap.used;
   Swap_usage.pagein = Swap.pagein;
   Swap_usage.pageout = Swap.pageout;

/* End of SS_read_stats(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Computes the swap free and used, in percentages. 

//////////////////////////////////////////////////////////////////////////\*/
static void Compute_utilization( ){

   Swap_utilization.total = Swap_usage.total;
   Swap_utilization.used = (((double) Swap_usage.used) / ((double) Swap_usage.total)) * 100;
   Swap_utilization.free = (((double) Swap_usage.free) / ((double) Swap_usage.total)) * 100;

   /* Add values for running total. */
   Total_used += Swap_utilization.used;
   Total_free += Swap_utilization.free;
   Total_pagein += (Swap_usage.pagein - Previous_swap_usage.pagein);
   Total_pageout += (Swap_usage.pageout - Previous_swap_usage.pageout);
   Total_reports++;
   
/* End of Swap_compute_utilization() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.

///////////////////////////////////////////////////////////////////////////\*/
void SS_cleanup(){

   /* Close swap stats file if file is open. */
   if( Swap_fd >= 0 ){

      int size;
      double ave_used = 0, ave_free = 0;
      time_t ctime = Scan_info.scan_time - Start_time;

      /* Do end of volume processing. */
      End_of_volume( 0, 0, 0, 0, 0, 0, 1 );

      /* Write out the average swap utilization. */
      memset( Buffer, 0, sizeof(Buffer) );

      /* In the event the Total_reports is 0. */
      if( Total_reports > 0 ){

         ave_used = (double) ((double) Total_used / (double) Total_reports);
         ave_free = (double) ((double) Total_free / (double) Total_reports);

      }

      sprintf( Buffer, "# Avg Util ... Used: %4.1f, Free: %4.1f\n",
               ave_used, ave_free );
   sprintf( Buffer, "# Avgs:               %4d                             %4.1f       \
%4.1f     %8lld    %8lld\n",
            (int) ctime, ave_used, ave_free, Total_pagein, Total_pageout );

      size = strlen( Buffer );
      if( write( Swap_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Swap_fd Failed\n" );

      close( Swap_fd );

   }

/* End of SS_cleanup(). */
}

