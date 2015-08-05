/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/19 14:48:08 $
 * $Id: userstat.c,v 1.2 2011/09/19 14:48:08 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#include <perf_mon.h>
#include <prod_status.h>
#include <orpg.h>
#include <infr.h>

/* Type Definitions and Macros. */
#define MAX_LINES		50
#define MAX_HISTORY		US_STATUS_SIZE

typedef struct Utilization_data {

   guint64 total_in;		/* Total bytes received since monitoring began. */

   guint64 total_out;		/* Total bytes sent since monitoring began. */

   unsigned int elev_total_in;	/* Total bytes received this elevation. */

   unsigned int elev_total_out;	/* Total bytes sent this elevation. */

   double elev_rate_in;		/* Receive rate (bytes/sec). */

   double elev_rate_out;	/* Send rate (bytes/sec) */

} Utilization_data_t;

typedef struct User_utilization {

   Utilization_data_t data;	/* Utilization data (see above). */

   /* Note: The following 2 items are starting values ... 
      values at start of monitoring. */
   guint64 start_bytes_in;	/* Bytes Received starting value. */

   guint64 start_bytes_out;	/* Bytes Sent starting value. */

   int init_flag;		/* Set when the above 2 items initialized. */

} User_util_t;

/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern struct utsname *Uname_info;
extern int Verbose;

/* For dynamic loading of library liborpg.so. */
extern void *ORPG_library;
extern int Library_link_failed;
extern int (*ORPGDA_read_fp)();

/* Static Global Variables. */
static User_util_t User_util[MAX_LINES];
static Prod_user_status Line_usage[MAX_LINES];
static Prod_user_status Previous_line_usage[MAX_LINES];

static int User_fd[MAX_LINES];
static char Buffer[256];

/* Used for computing volume statistics. */
static unsigned int Volume_in[MAX_LINES];
static unsigned int Volume_out[MAX_LINES];
static unsigned int Volume_time[MAX_LINES];

/* Function Prototypes. */
static void Compute_utilization( int line_ind );
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int ind, int terminate );


/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes User statistics.  User statistics are on a line-by-line
      basis.  

      Checks each stats file for existence and if exist, removes the file.

      Posts event to increase the cm_tcp monitoring rate to 2 seconds.

//////////////////////////////////////////////////////////////////////////\*/
int US_init_user_usage( ){

   char name[256], file[256];
   char ext[8];

   int line_ind;

   /* Set the User usage area to all zeros. */
   memset( &Line_usage[0], 0, MAX_HISTORY*sizeof( Prod_user_status ) );
   memset( &Previous_line_usage[0], 0, MAX_HISTORY*sizeof( Prod_user_status ) );
   memset( &Volume_in[0], 0, MAX_LINES*sizeof( unsigned int ) );
   memset( &Volume_out[0], 0, MAX_LINES*sizeof( unsigned int ) );
   memset( &Volume_time[0], 0, MAX_LINES*sizeof( unsigned int ) );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );
      return(-1);

   }

   for( line_ind = 1; line_ind < MAX_LINES; line_ind++ ){

      /* Initialize all the user fds. */
      User_fd[line_ind] = -1;

      /* Initialize the utilization data. */
      User_util[line_ind].init_flag = 0;
      User_util[line_ind].start_bytes_in = 0;
      User_util[line_ind].start_bytes_out = 0;

      /* Put file to store gnuplot data in the work directory. */
      memset( name, 0, 256 );
      if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

         fprintf( stderr, "MISC_get_work_dir failed\n" );
         return(-1);

      }

      /* Construct the file name for the user stats. */
      sprintf( file, "/%s.user.stats", Uname_info->nodename );
      strcat( name, file );
      sprintf( ext, ".%d", line_ind );
      strcat( name, (char *) &ext[0] );

      /* Open the file.  If User_fd is not negative, the file exists.
         Close the file and remove it. */
      if( (User_fd[line_ind] = open( name, O_RDWR, 0666 )) >= 0 ){

         close( User_fd[line_ind] );
         unlink( name );
         User_fd[line_ind] = -1;

      }

   }
   
   /* Post event to increase the reporting rate of user statistics.  The
      reporting period should be 2 seconds. */
   ext[0] = '2';
   ext[1] = '\0';
   EN_post_msgevent( ORPGEVT_STATISTICS_PERIOD, ext, 2 );
   
   return 0;

/* End of US_init_user_usage() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Reads ORPGDAT_PROD_STATUS buffer to obtain User statistics.  If
      line is connected, creates the stats file if the file does not
      exist.   

      Writes stats to the line-specific stats file.

   Inputs:
      scan_type - VOLUME_EVENT, ELEVATION_EVENT or NO_EVENT
      first_time - first time through flag.   If true, do not write
                   stats to stats file.

//////////////////////////////////////////////////////////////////////////\*/
void US_get_user_usage( int scan_type, int first_time ){

   int line_ind, year, mon, day, hour, min, sec;

   static char name[256], file[256], ext[8];

   /* Check if library link failed.  Return on failure. */
   if( Library_link_failed )
      return;

   /* Read the Product Status LB. */
   US_read_stats();

   if( VOLUME_EVENT == scan_type ){

      unix_time( &Scan_info.scan_time, &year, &mon, &day, &hour, &min, &sec );
      if( year >= 2000 )
         year -= 2000;

      else
         year -= 1900;

   }

   for( line_ind = 1; line_ind < MAX_LINES; line_ind++ ){

      /* If the link is not enabled or connected, ignore this link. */
      if( (Line_usage[line_ind].enable == 0) || (Line_usage[line_ind].link != US_CONNECTED ))
         continue;

      /* Check if the user's fd is defined.   If not, construct stats file name
         and open the file. */
      if( User_fd[line_ind] == -1 ){

         /* Put file to store gnuplot data in the work directory. */
         memset( name, 0, 256 );
         if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

            fprintf( stderr, "MISC_get_work_dir failed\n" );
            return;

         }

         /* Construct the file name for the user stats. */
         sprintf( file, "/%s.user.stats", Uname_info->nodename );
         strcat( name, file );
         sprintf( ext, ".%d", line_ind );
         strcat( name, ext );

         /* Create the "User.stats.##" file. */
         if( (User_fd[line_ind] = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

            fprintf( stderr, "Open Failed For FILE %s\n", name );
            User_fd[line_ind] = -1;

         }

      }

      Compute_utilization( line_ind );

      /* If the gnuplot file is open, write User stats to file. */
      if( User_fd[line_ind] >= 0 ){

         int size;

         /* If this is the first time through, add some comments to stats file. */
         if( VOLUME_EVENT == scan_type ){

            /* Only do this the first time. */
            if( first_time ){

               memset( Buffer, 0, sizeof(Buffer) );
               sprintf( Buffer, "#======================================================\
=============================================================\n" );

               size = strlen( Buffer );
               if( write( User_fd[line_ind], Buffer, size ) != size )
                  fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

               memset( Buffer, 0, sizeof(Buffer) );
               sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
                        mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

               size = strlen( Buffer );
               if( write( User_fd[line_ind], Buffer, size ) != size )
                  fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

               memset( Buffer, 0, sizeof(Buffer) );
               sprintf( Buffer,
                 "# Cut    Time (s)    Total (s)   T-Bytes (I)   T-Bytes (O)   E-Bytes (I)   E-Bytes (O)   I-Rate (B/s)   O-Rate (B/s)\n" );

               size = strlen( Buffer );
               if( write( User_fd[line_ind], Buffer, size ) != size )
                  fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

            }

         }

         /* The first time through this function will be at start of volume.  We don't want
            to write data to file at this point because we want the data to reflect the
            information between the start of the 2nd cut and the start of volume. */
         if( !first_time ){

            memset( Buffer, 0, sizeof(Buffer) );
            sprintf( Buffer, " %3d  %12u    %4d     %10lld    %10lld      %8u      %8u      %8.1f       %8.1f\n",
                     Prev_scan_info.rda_elev_num, 
                     (unsigned int) Scan_info.scan_time, 
                     (int) Delta_time,
                     User_util[line_ind].data.total_in, 
                     User_util[line_ind].data.total_out, 
                     User_util[line_ind].data.elev_total_in, 
                     User_util[line_ind].data.elev_total_out, 
                     User_util[line_ind].data.elev_rate_in,
                     User_util[line_ind].data.elev_rate_out );

            size = strlen( Buffer );
            if( write( User_fd[line_ind], Buffer, size ) != size )
               fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

            /* Increment the volume totals. */
            Volume_in[line_ind] += User_util[line_ind].data.elev_total_in;
            Volume_out[line_ind] += User_util[line_ind].data.elev_total_out;
            Volume_time[line_ind] += Delta_time;

            /* Print out the volume statistics. */
            if( VOLUME_EVENT == scan_type )
               End_of_volume( year, mon, day, hour, min, sec, line_ind, 0 );

         }

      }

   }

/* End of US_get_user_usage(). */
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
      line_ind - line index
      terminate - 1 - yes, 0 - no

//////////////////////////////////////////////////////////////////////////\*/
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int line_ind, int terminate ){

   int size;
   double rate_in = 0, rate_out = 0;

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#------------------------------------------------------\
-------------------------------------------------------------\n" );

   size = strlen( Buffer );
   if( write( User_fd[line_ind], Buffer, size ) != size )
      fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

   /* In the event Volume_time[line_ind] is 0. */
   if( Volume_time[line_ind] > 0 ){

      rate_in = (double) Volume_in[line_ind] / (double) Volume_time[line_ind];
      rate_out = (double) Volume_out[line_ind] / (double) Volume_time[line_ind];

   }

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Stats:              %4d                               \
    %8u      %8u      %8.1f       %8.1f\n",
            Volume_time[line_ind], Volume_in[line_ind], Volume_out[line_ind],
            rate_in, rate_out );

   size = strlen( Buffer );
   if( write( User_fd[line_ind], Buffer, size ) != size )
      fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#======================================================\
=============================================================\n" );

   size = strlen( Buffer );
   if( write( User_fd[line_ind], Buffer, size ) != size )
      fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

   /* Reset values for next volume. */
   Volume_in[line_ind] = 0;
   Volume_out[line_ind] = 0;
   Volume_time[line_ind] = 0;

   /* Write the User stat header for next volume scan. */
   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
            mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

   size = strlen( Buffer );
   if( write( User_fd[line_ind], Buffer, size ) != size )
      fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer,
            "# Cut    Time (s)    Total (s)   T-Bytes (I)   T-Bytes (O)   E-Bytes (I)   \
E-Bytes (O)   I-Rate (B/s)   O-Rate (B/s)\n" );

   size = strlen( Buffer );
   if( write( User_fd[line_ind], Buffer, size ) != size )
      fprintf( stderr, "Write to User_fd[%d] Failed\n", line_ind );

/* End of End_of_volume(). */
}

/*\////////////////////////////////////////////////////////////////////////////////

   Description:
      Reads the User Line statistics on a line-by-line basis.  The stats buffer
      contains a history of MAX_HISTORY Prod_user_status structures.   The
      first structure in the list are the latest available statistics.

////////////////////////////////////////////////////////////////////////////////\*/
int US_read_stats(){

   int line_ind, size;
   Prod_user_status *user = NULL;

   /* Save previous User status. */
   memcpy( &Previous_line_usage[0], &Line_usage[0], MAX_LINES*sizeof(Prod_user_status) );

   /* Do For All possible narrowband lines.  Note: Line Index 0 is the wideband. */
   for( line_ind = 1; line_ind < MAX_LINES; line_ind++ ){

      /* This function reads data from ORPGDAT_PROD_USER_STATUS. */
      if( (size = ORPGDA_read_fp( ORPGDAT_PROD_USER_STATUS, (char **) &user,
                           LB_ALLOC_BUF, line_ind )) < 0 ){

         /* No more line information LB_NOT_FOUND returned. */
         if( size == LB_NOT_FOUND )
            continue;

      }

      /* Check for correct size ... should be MAX_HISTORY*sizeof(Prod_user_status). */
      if( size != MAX_HISTORY*sizeof(Prod_user_status) ){

         fprintf( stderr, "Bad Product User Status Size: %d\n", size );
         free( user );
         continue;

      }

      /* Only save the latest user status. */
      memcpy( (void *) &Line_usage[line_ind], (void *) &user[0], 
              sizeof(Prod_user_status) );
   
      free( user );

   }

   return 0;

/* End of US_read_stats(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Computes the user statistics. 

//////////////////////////////////////////////////////////////////////////\*/
static void Compute_utilization( int line_ind ){

   /* Initialize the User utilization data. */
   memset( &User_util[line_ind].data, 0, sizeof(Utilization_data_t) );

   /* Initialize the user utilization data. */
   if( User_util[line_ind].init_flag == 0 ){

      User_util[line_ind].init_flag = 1;
      User_util[line_ind].start_bytes_in = 
                          (guint64) Line_usage[line_ind].tnb_received;
      User_util[line_ind].start_bytes_out = (
                          guint64) Line_usage[line_ind].tnb_sent;

   }

   /* Total bytes in/out. */
   User_util[line_ind].data.total_in = (guint64) Line_usage[line_ind].tnb_received -
                                                  User_util[line_ind].start_bytes_in;
   User_util[line_ind].data.total_out = (guint64) Line_usage[line_ind].tnb_sent - 
                                                  User_util[line_ind].start_bytes_out;
   User_util[line_ind].data.elev_total_in = (guint64) (Line_usage[line_ind].tnb_received -
                                                  Previous_line_usage[line_ind].tnb_received);
   User_util[line_ind].data.elev_total_out = (guint64) (Line_usage[line_ind].tnb_sent -
                                                  Previous_line_usage[line_ind].tnb_sent);

   /* Line rates since last report. */
   User_util[line_ind].data.elev_rate_in = 
                 ((double) User_util[line_ind].data.elev_total_in / (double) Delta_time);
   User_util[line_ind].data.elev_rate_out = 
                 ((double) User_util[line_ind].data.elev_total_out / (double) Delta_time);

/* End of Compute_utilization() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.

///////////////////////////////////////////////////////////////////////////\*/
void US_cleanup(){
   
   int line_ind;
   char ext[8];

   /* Do for each line .... */
   for( line_ind = 1; line_ind < MAX_LINES; line_ind++ ){

      /* Close User stats file if file is open. */
      if( User_fd[line_ind] >= 0 ){
   
         /* Do end of volume processing. */
         End_of_volume( 0, 0, 0, 0, 0, 0, line_ind, 1 );

         close( User_fd[line_ind] );

      }

   }

   /* Post event to return the comm manager statistics reporting back to 
      normal.   This can be accomplished by setting the reporting period
      to a very large number. */
   sprintf( ext, "100" );
   EN_post_msgevent( ORPGEVT_STATISTICS_PERIOD, ext, strlen(ext) + 1 );

/* End of US_cleanup(). */
}

