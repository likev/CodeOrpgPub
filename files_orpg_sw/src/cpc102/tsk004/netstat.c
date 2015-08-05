/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:18 $
 * $Id: netstat.c,v 1.1 2011/05/22 16:48:18 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#include <perf_mon.h>
#include <glibtop/netlist.h>
#include <glibtop/netload.h>

/* Type Definitions and Macros. */
#define MAX_INTERFACES		4

/* These are all the interface information tracked. */  
typedef struct Utilization_data {

   guint64 bytes_in;		/* Bytes received. */

   guint64 bytes_out;		/* Bytes sent. */

   guint64 bytes_total;		/* Total bytes. */

   guint64 errors_in;		/* Total errors received. */

   guint64 errors_out;		/* Total errors sent. */

   guint64 errors_total;	/* Total errors. */

   guint64 collisions;		/* Total collisions. */

   unsigned int elev_bytes_in;	/* Total bytes received this elevation. */

   unsigned int elev_bytes_out;	/* Total bytes sent this elevation. */

   unsigned int elev_bytes;	/* Total bytes this elevation. */

   double elev_byte_rate_in;	/* Receive rate this elevation (bytes/sec). */

   double elev_byte_rate_out;	/* Send rate this elevation (bytes/sec) */

   double elev_byte_rate;	/* Total rate this elevation (bytes/sec) */

   unsigned int elev_errors_in;	/* Total errors received this elevation. */

   unsigned int elev_errors_out;/* Total errors sent this elevation. */

   unsigned int elev_errors;	/* Total errors this elevation. */

   double elev_error_rate_in;	/* Receive rate (bytes/sec). */

   double elev_error_rate_out;	/* Send rate (bytes/sec) */

   double elev_error_rate;	/* Total rate (bytes/sec) */

   unsigned int elev_collisions;/* Total collisions this elevation. */

   double elev_collision_rate;  /* Collision rate this elevation. */

} Utilization_data_t;


/* Contains the interface information necessary to compute rates. */
typedef struct Interface_utilization {

   Utilization_data_t data;	/* Utilization data (see above). */

   /* Note: The following 2 items are starting values ... 
      values at start of monitoring. */
   guint64 start_bytes_in;	/* Bytes in starting value. */

   guint64 start_bytes_out;	/* Bytes out starting value. */

   guint64 start_bytes;		/* Total bytes starting value. */

   guint64 start_errors_in;	/* Errors in starting value. */

   guint64 start_errors_out;	/* Errors out starting value. */

   guint64 start_errors;	/* Total errors starting value. */

   guint64 start_collisions;	/* Number of collisionss starting value. */

   int init_flag;		/* Set when the above starting values initialized. */

   char name[32];		/* Interface name. */

} Interface_util_t;

/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern struct utsname *Uname_info;
extern int Verbose;

/* Static Global Variables. */
static Interface_util_t Interface_util[MAX_INTERFACES];
static glibtop_netload Net_usage[MAX_INTERFACES];
static glibtop_netload Previous_net_usage[MAX_INTERFACES];

static int Num_interface = 0;
static char Interface[MAX_INTERFACES][64];

static int Interface_fd[MAX_INTERFACES];
static char Buffer[256];
static time_t Curr_time = 0; 
static time_t Start_time = 0;

/* For computing volume scan statistics. */
static unsigned int Volume_bytes_in[MAX_INTERFACES];
static unsigned int Volume_bytes_out[MAX_INTERFACES];
static unsigned int Volume_time[MAX_INTERFACES];

/* Function Prototypes. */
static void Compute_utilization( int line_ind );
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int ind, int terminate );


/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes interface statistics.  Interface statistics are on an 
      interface by interface basis.  We ignore the loopback interface lo.

      Checks each stats file for existence and if exist, removes the file.

//////////////////////////////////////////////////////////////////////////\*/
int IS_init_interface_usage( ){

   char name[256], file[256];

   int ind;
   glibtop_netlist netlist;
   char **interface = NULL;

   /* Set the User usage area to all zeros. */
   memset( &Net_usage[0], 0, sizeof( glibtop_netload)*MAX_INTERFACES );
   memset( &Previous_net_usage[0], 0, sizeof( glibtop_netload)*MAX_INTERFACES );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );
      return(-1);

   }

   for( ind = 0; ind < MAX_INTERFACES; ind++ ){

      /* Initialize all the user fds. */
      Interface_fd[ind] = -1;

      /* Initialize the utilization data. */
      Interface_util[ind].init_flag = 0;
      Interface_util[ind].start_bytes_in = 0;
      Interface_util[ind].start_bytes_out = 0;
      Interface_util[ind].name[0] = '\0';

   }

   /* Get the number of interfaces and the interface names. */
   interface = glibtop_get_netlist( &netlist );

   for( ind = 0; ind < netlist.number; ind++ ){

      glibtop_netload netload;
      glibtop_get_netload( &netload, interface[ind]);

      if( netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK) )
         continue;

      /* Save the interface name. */
      strcpy( &Interface[Num_interface][0], interface[ind] );

      /* Put file to store gnuplot data in the work directory. */
      memset( name, 0, 256 );
      if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

         fprintf( stderr, "MISC_get_work_dir failed\n" );
         return(-1);

      }

      /* Construct the file name for the user stats. */
      sprintf( file, "/%s.%s.stats", Uname_info->nodename, Interface[Num_interface] );
      strcat( name, file );

      /* Open the file.  If User_fd is not negative, the file exists.
         Close the file and remove it. */
      if( (Interface_fd[Num_interface] = open( name, O_RDWR, 0666 )) >= 0 ){

         close( Interface_fd[Num_interface] );
         unlink( name );
         Interface_fd[Num_interface] = -1;

      }

      /* Create the "interface.stats" file. */
      if( (Interface_fd[Num_interface]  = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

         fprintf( stderr, "Open Failed For File %s\n", name );
         Interface_fd[Num_interface] = -1;

         /* Skip this interface. */
         continue;

      }

      /* Increment the number of interfaces. */
      Num_interface++;

   }

   return 0;

/* End of IS_init_interface_usage() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Reads network interface statistics and writes out interface specific
      utilization values. 

   Inputs:
      scan_type - VOLUME_EVENT, ELEVATION_EVENT or NO_EVENT
      first_time - first time through flag.   If true, do not write
                   stats to stats file.

//////////////////////////////////////////////////////////////////////////\*/
void IS_get_interface_usage( int scan_type, int first_time ){

   int ind, year, mon, day, hour, min, sec;

   Curr_time = Scan_info.scan_time;

   /* Inititalize the start time.   This is the time stats first collected. */
   if( Start_time == 0 )
      Start_time = Curr_time;

   /* Read the Product Status LB. */
   IS_read_stats();

   /* If start of volume scan, extract time. */
   if( VOLUME_EVENT == scan_type ){

      unix_time( &Scan_info.scan_time, &year, &mon, &day, &hour, &min, &sec );
      if( year >= 2000 )
         year -= 2000;

      else
         year -= 1900;

   }

   /* Do For All Interfaces ... */
   for( ind = 0; ind < Num_interface; ind++ ){

      Compute_utilization( ind );

      /* Write Interface stats to file. */
      if( Interface_fd[ind] >= 0 ){

         int size;

         /* If this is the first time through, add some comments to stats file. */
         if( (VOLUME_EVENT == scan_type) && (first_time) ){

            memset( Buffer, 0, sizeof(Buffer) );
            sprintf( Buffer, "#============================================================\
=======================================================\n" );

            size = strlen( Buffer );
            if( write( Interface_fd[ind], Buffer, size ) != size )
               fprintf( stderr, "Write to Interface_fd[%d] Failed\n", ind );

            memset( Buffer, 0, sizeof(Buffer) );
            sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
                     mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

            size = strlen( Buffer );
            if( write( Interface_fd[ind], Buffer, size ) != size )
               fprintf( stderr, "Write to Interface_fd[%s] Failed\n", &Interface_util[ind].name[0] );

            memset( Buffer, 0, sizeof(Buffer) );
            sprintf( Buffer,
                 "# Cut    Time (s)    Total(s)    T-Bytes (I)     T-Bytes (O)    E-Bytes (I)    \
E-Bytes (O)      Rate (I)     Rate (O)\n" );

            size = strlen( Buffer );
            if( write( Interface_fd[ind], Buffer, size ) != size )
               fprintf( stderr, "Write to Interface_fd[%s] Failed\n", &Interface_util[ind].name[0] );

         }

         /* The first time through this function will be at start of volume.  We don't want
            to write data to file at this point because we want the data to reflect the
            information between the start of the 2nd cut and the start of volume. */
         if( !first_time ){

            memset( Buffer, 0, sizeof(Buffer) );
            sprintf( Buffer, " %3d  %12u    %4d      %10lld      %10lld       %8u       \
%8u       %8.1f     %8.1f\n",
                     Prev_scan_info.rda_elev_num, (unsigned int) Scan_info.scan_time, 
                     (int) Delta_time, Interface_util[ind].data.bytes_in, 
                     Interface_util[ind].data.bytes_out, 
                     Interface_util[ind].data.elev_bytes_in, 
                     Interface_util[ind].data.elev_bytes_out, 
                     Interface_util[ind].data.elev_byte_rate_in, 
                     Interface_util[ind].data.elev_byte_rate_out );

            size = strlen( Buffer );
            if( write( Interface_fd[ind], Buffer, size ) != size )
               fprintf( stderr, "Write to Interface_fd[%s] Failed\n", &Interface_util[ind].name[0] );

            /* Increment the volume totals. */
            Volume_bytes_in[ind] += Interface_util[ind].data.elev_bytes_in;
            Volume_bytes_out[ind] += Interface_util[ind].data.elev_bytes_out;
            Volume_time[ind] += Delta_time;

            /* Print out the volume statistics. */
            if( VOLUME_EVENT == scan_type )
               End_of_volume( year, mon, day, hour, min, sec, 
                              ind, 0 );

         }

      }

   }

/* End of IS_get_interface_usage(). */
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
      ind - interface index
      terminate - 1 - yes, 0 - no

//////////////////////////////////////////////////////////////////////////\*/
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int ind, int terminate ){

   int size;
   double ibytes = 0, obytes = 0;

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#------------------------------------------------------------\
-------------------------------------------------------\n" );

   size = strlen( Buffer );
   if( write( Interface_fd[ind], Buffer, size ) != size )
      fprintf( stderr, "Write to Interface_fd[%d] Failed\n", ind );

   /* In the event Volume_time[ind] is 0. */
   if( Volume_time[ind] > 0 ){

      ibytes = (double) Volume_bytes_in[ind] / (double) Volume_time[ind];
      obytes = (double) Volume_bytes_out[ind] / (double) Volume_time[ind];

   }

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# Stats:              %4d                                       \
%8u       %8u       %8.1f     %8.1f\n",
            Volume_time[ind], Volume_bytes_in[ind], Volume_bytes_out[ind],
            ibytes, obytes );

   size = strlen( Buffer );
   if( write( Interface_fd[ind], Buffer, size ) != size )
      fprintf( stderr, "Write to Interface_fd[%d] Failed\n", ind );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "#============================================================\
=======================================================\n" );

   size = strlen( Buffer );
   if( write( Interface_fd[ind], Buffer, size ) != size )
      fprintf( stderr, "Write to Interface_fd[%d] Failed\n", ind );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

   /* Reset values for next volume. */
   Volume_bytes_in[ind] = 0;
   Volume_bytes_out[ind] = 0;
   Volume_time[ind] = 0;

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
            mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

   size = strlen( Buffer );
   if( write( Interface_fd[ind], Buffer, size ) != size )
      fprintf( stderr, "Write to Interface_fd[%s] Failed\n", &Interface_util[ind].name[0] );

   memset( Buffer, 0, sizeof(Buffer) );
   sprintf( Buffer,
     "# Cut    Time (s)    Total(s)    T-Bytes (I)     T-Bytes (O)    E-Bytes (I)    E-Bytes (O)      Rate (I)     Rate (O)\n" );

   size = strlen( Buffer );
   if( write( Interface_fd[ind], Buffer, size ) != size )
      fprintf( stderr, "Write to Interface_fd[%s] Failed\n", &Interface_util[ind].name[0] );

/* End of End_of_volume(). */
}

/*\////////////////////////////////////////////////////////////////////////////////

   Description:
      Reads the network interface statistics on an interface-by-interface basis.  

////////////////////////////////////////////////////////////////////////////////\*/
int IS_read_stats(){

   int ind;

   /* Do For All possible interfaces. */
   for( ind = 0; ind < Num_interface; ind++ ){

      /* Save off the previous network usage data. */
      memcpy( &Previous_net_usage[ind], &Net_usage[ind], sizeof(glibtop_netload) );

      /* This function reads interface statistics. */
      glibtop_get_netload( &Net_usage[ind], Interface[ind]);

   }

   return 0;

/* End of IS_read_stats(). */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Computes the Interface statistics. 

//////////////////////////////////////////////////////////////////////////\*/
static void Compute_utilization( int ind ){

   /* Initialize the Interface utilization data. */
   memset( &Interface_util[ind].data, 0, sizeof(Utilization_data_t) );

   /* Compute utilization. */
   if( Interface_util[ind].init_flag == 0 ){

      Interface_util[ind].init_flag = 1;
      Interface_util[ind].start_bytes_in = (guint64) Net_usage[ind].bytes_in;
      Interface_util[ind].start_bytes_out = (guint64) Net_usage[ind].bytes_out;
      Interface_util[ind].start_bytes = (guint64) Net_usage[ind].bytes_total;
      Interface_util[ind].start_errors_in = (guint64) Net_usage[ind].errors_in;
      Interface_util[ind].start_errors_out = (guint64) Net_usage[ind].errors_out;
      Interface_util[ind].start_errors = (guint64) Net_usage[ind].errors_total;
      Interface_util[ind].start_collisions = (guint64) Net_usage[ind].collisions;

   }

   /* Bytes in/out. */
   Interface_util[ind].data.bytes_in = (guint64) (Net_usage[ind].bytes_in -
                                                  Interface_util[ind].start_bytes_in);
   Interface_util[ind].data.bytes_out = (guint64) (Net_usage[ind].bytes_out - 
                                                   Interface_util[ind].start_bytes_out);
   Interface_util[ind].data.bytes_total = (guint64) (Net_usage[ind].bytes_total - 
                                                     Interface_util[ind].start_bytes);
   Interface_util[ind].data.elev_bytes_in = (guint64) (Net_usage[ind].bytes_in -
                                                       Previous_net_usage[ind].bytes_in);
   Interface_util[ind].data.elev_bytes_out = (guint64) (Net_usage[ind].bytes_out -
                                                        Previous_net_usage[ind].bytes_out);
   Interface_util[ind].data.elev_bytes = (guint64) (Net_usage[ind].bytes_total -
                                                    Previous_net_usage[ind].bytes_total);

   /* Errors in/out. */
   Interface_util[ind].data.errors_in = (guint64) (Net_usage[ind].errors_in -
                                                   Interface_util[ind].start_errors_in);
   Interface_util[ind].data.errors_out = (guint64) (Net_usage[ind].errors_out - 
                                                    Interface_util[ind].start_errors_out);
   Interface_util[ind].data.errors_total = (guint64) (Net_usage[ind].errors_total - 
                                                      Interface_util[ind].start_errors);
   Interface_util[ind].data.elev_errors_in = (guint64) (Net_usage[ind].errors_in -
                                                        Previous_net_usage[ind].errors_in);
   Interface_util[ind].data.elev_errors_out = (guint64) (Net_usage[ind].errors_out -
                                                         Previous_net_usage[ind].errors_out);
   Interface_util[ind].data.elev_errors = (guint64) (Net_usage[ind].errors_total -
                                                     Previous_net_usage[ind].errors_total);

   /* Collisions. */
   Interface_util[ind].data.collisions = (guint64) (Net_usage[ind].collisions -
                                                    Interface_util[ind].start_collisions);
   /* Interface rates since last report. */
   Interface_util[ind].data.elev_byte_rate_in =
                 ((double) Interface_util[ind].data.elev_bytes_in / (double) Delta_time);
   Interface_util[ind].data.elev_byte_rate_out = 
                 ((double) Interface_util[ind].data.elev_bytes_out / (double) Delta_time);
   Interface_util[ind].data.elev_byte_rate = 
                 ((double) Interface_util[ind].data.elev_bytes / (double) Delta_time);
   Interface_util[ind].data.elev_error_rate_in =
                 ((double) Interface_util[ind].data.elev_errors_in / (double) Delta_time);
   Interface_util[ind].data.elev_error_rate_out = 
                 ((double) Interface_util[ind].data.elev_errors_out / (double) Delta_time);
   Interface_util[ind].data.elev_error_rate = 
                 ((double) Interface_util[ind].data.elev_errors / (double) Delta_time);
   Interface_util[ind].data.elev_collision_rate = 
                 ((double) Interface_util[ind].data.elev_collisions / (double) Delta_time);

/* End of Compute_utilization() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.  

///////////////////////////////////////////////////////////////////////////\*/
void IS_cleanup(){
   
   int ind;
   
   /* Do for each interface .... */
   for( ind = 0; ind < MAX_INTERFACES; ind++ ){
   
      /* Close Interface stats file if file is open. */
      if( Interface_fd[ind] >= 0 ){

         /* Do end of volume processing. */
         End_of_volume( 0, 0, 0, 0, 0, 0, ind, 1 );

         /* Write out the average Interface utilization. */
         memset( Buffer, 0, sizeof(Buffer) );

         close( Interface_fd[ind] );

      }

   }

/* End of IS_cleanup(). */
}

