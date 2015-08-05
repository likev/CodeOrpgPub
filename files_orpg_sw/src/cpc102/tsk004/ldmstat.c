/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/19 14:48:08 $
 * $Id: ldmstat.c,v 1.2 2011/09/19 14:48:08 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#include <perf_mon.h>
#include <archive_II.h>

/* Type Definitions. */
typedef struct LDM_utilization {

   guint64 total_u;      	/* Total LDM bytes, uncompressed. */

   guint64 total_c;      	/* Total LDM bytes, compressed. */

   double total_ratio;          /* Compression ratio (total_u/total_c). */

   guint64 total_elev_u;	/* Total LDM bytes this elevation, uncompressed. */

   double elev_rate;		/* Total LDM bytes/per second. */

   guint64 total_elev_c;	/* Total LDM bytes this elevation, compressed. */

   double elev_ratio;		/* Compression ratio (total_elev_u/total_elev_c). */

} LDM_util_t;

/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern time_t Start_time;
extern struct utsname *Uname_info;
extern int Verbose;

/* Static Global Variables. */
static ArchII_perfmon_t Previous_LDM_usage;
static ArchII_perfmon_t LDM_usage;
static LDM_util_t LDM_util;
static int LDM_fd = -1;
static char Buffer[256];

/* Used for computing volume statistics. */
static guint64 Volume_total_c = 0;
static guint64 Volume_total_u = 0;
static unsigned int Volume_time = 0;

/* Function Prototypes. */
static void Compute_utilization( );
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int terminate );

/* For dynamic loading of library liborpg.so. */
extern void *ORPG_library;
extern int Library_link_failed;
extern int (*ORPGDA_read_fp)();


/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes LDM statistics and opens the LDM stats file.  The
      file will be removed if the file already exists.  It will then
      create the file hostname.LDM.stats in the RPG work directory 
      (typically the "tmp" directory under $HOME). 

//////////////////////////////////////////////////////////////////////////\*/
int LS_init_LDM_usage( ){

   char name[256];
   char file[256];

   /* Set the LDM usage area to all zeros. */
   memset( &LDM_usage, 0, sizeof( ArchII_perfmon_t ) );
   memset( &Previous_LDM_usage, 0, sizeof( ArchII_perfmon_t ) );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );

      /* Create and open file in current working directory. */
      name[0] = '\0';

   }

   sprintf( file, "/%s.LDM.stats", Uname_info->nodename );
   strcat( name, file );

   /* Open the file.  If LDM_fd is not negative, the file exists.
      Close the file, remove it, then re-open the file with create. */
   if( (LDM_fd = open( name, O_RDWR, 0666 )) >= 0 ){

      close( LDM_fd );
      unlink( name );
      LDM_fd = -1;

   }
   
   /* Create the "LDM.stats" file. */
   if( (LDM_fd = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

      fprintf( stderr, "Open Failed For FILE %s\n", name );
      LDM_fd = -1;

   }

   return LDM_fd;

/* End of LS_init_LDM_usage() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Reads ORPGDAT_ARCHIVE_II_INFO buffer to obtain ARCHIVE II statistics.

   Inputs:
      scan_type - VOLUME_EVENT, ELEVATION_EVENT or NO_EVENT
      first_time - first time through flag.   If true, do not write
                   stats to stats file.

//////////////////////////////////////////////////////////////////////////\*/
void LS_get_LDM_usage( int scan_type, int first_time ){

   int ret, size, year, mon, day, hour, min, sec;

   /* Check if library link failed.  Return on failure. */
   if( Library_link_failed )
      return;

   /* If the gnuplot file is open, write LDM stats to file. */
   if( LDM_fd < 0 )
      return;

   /* Read the LDM statistics LB. */
   if( (ret = LS_read_stats()) < 0 ){

      /* On errors other than LB_NOT_FOUND, log error and return. */
      if( ret != LB_NOT_FOUND ){

         fprintf( stderr, "ORPGDA_read( ORPGDAT_ARCHIVE_II_INFO, ... ) Failed: %d\n", ret );
         return;

      }
      else{

         /* Assume no data written to LDM. */
         LDM_usage.total_bytes_u = 0;
         LDM_usage.total_bytes_c = 0;

      }

   }

   /* Compute utilization. */
   Compute_utilization( );

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
         sprintf( Buffer, "#==================================================================\
========================\n" );

         size = strlen( Buffer );
         if( write( LDM_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to LDM_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
                  mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

         size = strlen( Buffer );
         if( write( LDM_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to LDM_fd Failed\n" );

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, 
           "# Cut    Time (s)    Total (s)      Total (B)      Elev (B)    Rate (B/s)    Ratio\n" );

         size = strlen( Buffer );
         if( write( LDM_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to LDM_fd Failed\n" );

      }

   }

   /* The first time through this function will be at start of volume.  We don't want
      to write data to file at this point because we want the data to reflect the
      information between the start of the 2nd cut and the start of volume. */
   if( !first_time ){

      memset( Buffer, 0, sizeof(Buffer) );
      sprintf( Buffer, " %3d  %12u    %4d       %10lld      %8lld      %8.1f   %8.1f\n", 
               Prev_scan_info.rda_elev_num, (unsigned int) Scan_info.scan_time, 
               (int) Delta_time, LDM_util.total_c,  LDM_util.total_elev_c, 
               LDM_util.elev_rate, LDM_util.elev_ratio );

      size = strlen( Buffer );
      if( write( LDM_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to LDM_fd Failed\n" );

      /* Increment the volume totals. */
      Volume_total_u += LDM_util.total_elev_u;
      Volume_total_c += LDM_util.total_elev_c;
      Volume_time += Delta_time;

      /* Print out the volume statistics. */
      if( VOLUME_EVENT == scan_type )
         End_of_volume( year, mon, day, hour, min, sec, 0 );

      /* This information goes to the main terminal. */
      if( Verbose ){

         double util = 0;

         if( LDM_util.total_elev_c > 0 )
            util = (double) LDM_util.total_elev_u / (double) LDM_util.total_elev_c;

         fprintf( stderr, "LDM--> Total: %10lld (B),  Elev: %10lld (B),  Ratio: %6.1f\n",
                  LDM_util.total_c, LDM_util.total_elev_c, util );

      }

   }

/* End of LS_get_LDM_usage(). */
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

   double rate = 0, ratio = 0;
   int size;

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#------------------------------------------------------------------\
------------------------\n" );

   size = strlen( Buffer );
   if( write( LDM_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to LDM_fd Failed\n" );
   memset( Buffer, 0, sizeof(Buffer) );

   /* In the event Volume_time is 0. */
   if( Volume_time > 0 )
      rate = ((double) Volume_total_c / (double) Volume_time );

   /* In the event Volume_total_c is 0. */
   if( Volume_total_c )
      ratio = ((double) Volume_total_u / (double) Volume_total_c);

   sprintf( Buffer, "# Stats:              %4d                       %8lld      %8.1f \
    %6.1f\n",
            Volume_time, Volume_total_c, rate, ratio );

   size = strlen( Buffer );
   if( write( LDM_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to LDM_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#==================================================================\
========================\n" );

   size = strlen( Buffer );
   if( write( LDM_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to LDM_fd Failed\n" );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

   /* Reset the volume totals. */
   Volume_total_u = 0;
   Volume_total_c = 0;
   Volume_time = 0;

   /* Write the LDM stats header for the next volume scan. */
   sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
            mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

   size = strlen( Buffer );
   if( write( LDM_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to LDM_fd Failed\n" );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer,
     "# Cut    Time (s)    Total (s)      Total (B)      Elev (B)    Rate (B/s)    Ratio\n" );

   size = strlen( Buffer );
   if( write( LDM_fd, Buffer, size ) != size )
      fprintf( stderr, "Write to LDM_fd Failed\n" );

/* End of End_of_volume(). */
}


/*\////////////////////////////////////////////////////////////////////////////////

   Description:
      Reads the LDM statistics from LDM.

   Returns:
      Negative number of error, 0 otherwise.

////////////////////////////////////////////////////////////////////////////////\*/
int LS_read_stats(){

   int ret;

   /* Check if library link failed.  Return on failure. */
   if( Library_link_failed )
      return -1;

   /* Save previous LDM statistics. */
   memcpy( &Previous_LDM_usage, &LDM_usage, sizeof(ArchII_perfmon_t) );

   /* This function reads data from ORPGDAT_ARCHIVE_II_INFO. */
   if( (ret = ORPGDA_read_fp( ORPGDAT_ARCHIVE_II_INFO, (char *) &LDM_usage,
                              sizeof(ArchII_perfmon_t), ARCHIVE_II_PERFMON_ID )) < 0 )
      return ret;

   return 0;

/* End of LS_read_stats(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Computes LDM utilization. 

//////////////////////////////////////////////////////////////////////////\*/
static void Compute_utilization( ){

   memset( &LDM_util, 0, sizeof( LDM_util_t ) );

   LDM_util.total_u = LDM_usage.total_bytes_u;
   LDM_util.total_c = LDM_usage.total_bytes_c;

   if( LDM_util.total_c != 0 )
      LDM_util.total_ratio = 
               ((double) LDM_util.total_u / (double) LDM_util.total_c);

   LDM_util.total_elev_u = LDM_usage.total_bytes_u - 
                           Previous_LDM_usage.total_bytes_u;
   LDM_util.total_elev_c = LDM_usage.total_bytes_c - 
                           Previous_LDM_usage.total_bytes_c;

   if( LDM_util.total_elev_c != 0 )
      LDM_util.elev_ratio = 
         ((double) LDM_util.total_elev_u / (double) LDM_util.total_elev_c);

   if( Delta_time != 0 )
      LDM_util.elev_rate = 
         ((double) LDM_util.total_elev_c / (double) Delta_time );

/* End of Compute_utilization() */
} 

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.

///////////////////////////////////////////////////////////////////////////\*/
void LS_cleanup(){
 
   /* Close LDM stats file if file is open. */
   if( LDM_fd >= 0 ){

      int size;
      double ave_rate = 0, ave_ratio = 0;
      time_t ctime = Scan_info.scan_time - Start_time;

      /* Do end of volume processing. */
      End_of_volume( 0, 0, 0, 0, 0, 0, 1 );

      /* Write out the average LDM utilization. */
      memset( Buffer, 0, sizeof(Buffer) );

      /* In the event Delta_time is 0. */
      if( Delta_time > 0 )
         ave_rate = (double) ((double) LDM_util.total_c / (double) ctime );

      /* In the event LDM_util.total_c is 0. */
      if( LDM_util.total_c > 0 )
         ave_ratio = LDM_util.total_u / LDM_util.total_c;

      sprintf( Buffer, "# Avgs:               %4d                       %8lld      %8.1f \
    %6.1f\n",
               (int) ctime, LDM_util.total_c, ave_rate, ave_ratio );

      size = strlen( Buffer );
      if( write( LDM_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to LDM_fd Failed\n" );

      close( LDM_fd );

   }
   
/* End of LS_cleanup(). */
}
