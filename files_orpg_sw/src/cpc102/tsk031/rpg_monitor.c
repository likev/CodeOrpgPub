/* 
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/10/10 18:49:23 $
 * $Id: rpg_monitor.c,v 1.1 2007/10/10 18:49:23 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

#include <mrpg.h>

static int Iostat_monitoring = 0;	/* iostat performance monitoring mode */
static int Free_monitoring = 0;		/* free performance monitoring mode */
static int Vmstat_monitoring = 0;	/* vmstat performance monitoring mode */
static int Monitoring_period = 0;	/* Monitoring period */
static int End_elevation = 0;		/* RPG elevation scan ended */
static int New_volume = 0;		/* RPG new volume started */

static int Monitor_pid = 0;		/* PID of monitoring process. */

static Orpgevt_scan_info_t Elev_info;
static Orpgevt_scan_info_t Vol_info;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Perf_monitor ();
static void An_callback (EN_id_t evtcd, char *msg, int msglen, void *arg);
static void Print_volume_info ();
static void Print_elevation_info ();
static int Terminate( int signal, int sigtype );

#ifdef LITTLE_ENDIAN_MACHINE
char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len );
#endif

/******************************************************************

    Description: The main function.

******************************************************************/
int main (int argc, char **argv) {

   /* read options */
   if (Read_options (argc, argv) != 0)
      exit (0);

   /* Register termination handler. */
   ORPGTASK_reg_term_handler( Terminate );

   /* Do performance monitoring. */
   Perf_monitor ();

   return 0;

/* End of main(). */
}

/******************************************************************

    Reporting memory information (triggered by the volume 
    start/elevation start event).
	
******************************************************************/
static void Perf_monitor (){

   int ret;
   char cmd[256];

   New_volume = 0;
   ret = EN_register (ORPGEVT_SCAN_INFO, An_callback);
   if( ret < 0 ){

      fprintf( stderr, "EN_register scan info event failed (ret %d)\n", 
               ret );
      exit (1);

    }

#ifdef LITTLE_ENDIAN_MACHINE
   /* Set byte swapping function. */
   EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_msg);
#endif

   if( Iostat_monitoring )
      sprintf( cmd, "iostat -t -x %d &", Monitoring_period );

   else if( Vmstat_monitoring )
      sprintf( cmd, "vmstat %d &", Monitoring_period );

   else if( Free_monitoring )
      sprintf( cmd, "free -k -s %d &", Monitoring_period );

   /* Start monitoring .... */
   Monitor_pid = MISC_system_to_buffer( cmd, 0, 0, 0 );
   if( Monitor_pid <= 0 ){

      fprintf( stderr, "MISC_system_to_buffer( %s, 0, 0, 0 ) Failed: %d\n", 
               cmd, Monitor_pid );
      exit(1);

   }

   while (1){

      if( !End_elevation && !New_volume ){

         msleep (10000);
	 continue;

      }

      if( End_elevation ){

         Print_elevation_info ();
	 End_elevation = 0;

      }
      else if( New_volume ){

         Print_volume_info ();
         New_volume = 0;

      }

   }

}

/******************************************************************

    The RPG event callback function.

    Input:	See LB man page.
	
******************************************************************/
static void An_callback (EN_id_t evtcd, char *msg, int msglen, void *arg) {

   Orpgevt_scan_info_t *scan_info;

   if (evtcd == ORPGEVT_SCAN_INFO){

      scan_info = (orpgevt_scan_info_t *) msg;
      if( (scan_info->key == ORPGEVT_END_ELEV) 
                          || 
          (scan_info->key == ORPGEVT_END_VOL) ){

         End_elevation = 1;
         memcpy( &Elev_info, msg, sizeof(Orpgevt_scan_info_t) );

      }
      else if (scan_info->key == ORPGEVT_BEGIN_VOL){

         New_volume = 1;
         memcpy( &Vol_info, msg, sizeof(orpgevt_scan_info_t) );

      }

   }
        
}

/******************************************************************

    Prints volume info.
	
******************************************************************/
static void Print_volume_info () {

    time_t vol_time, local_time;
    int yy, mon, dd, hh, min, ss;
    int yy_l, mon_l, dd_l, hh_l, min_l, ss_l;

    vol_time = UNIX_SECONDS_FROM_RPG_DATE_TIME 
				(Vol_info.data.date, Vol_info.data.time);
    unix_time (&vol_time, &yy, &mon, &dd, &hh, &min, &ss);

    local_time = time( NULL );
    unix_time (&local_time, &yy_l, &mon_l, &dd_l, &hh_l, &min_l, &ss_l);

    printf ("\nVolume: # %d  %.2d/%.2d/%.2d %.2d:%.2d:%.2d  vcp %d (Local Time: %.2d/%.2d/%.2d %.2d:%.2d:%.2d)\n\n", 
	(int)Vol_info.data.vol_scan_number, mon, dd, (yy - 1900) % 100, hh, min, ss, 
	Vol_info.data.vcp_number, mon_l, dd_l, (yy_l - 1900) % 100, hh_l, min_l, ss_l);

    return;
}

/******************************************************************

    Prints elevation info.
	
******************************************************************/
static void Print_elevation_info () {

    time_t elev_time, local_time;
    int yy, mon, dd, hh, min, ss;
    int yy_l, mon_l, dd_l, hh_l, min_l, ss_l;

    elev_time = UNIX_SECONDS_FROM_RPG_DATE_TIME 
				(Elev_info.data.date, Elev_info.data.time);
    unix_time (&elev_time, &yy, &mon, &dd, &hh, &min, &ss);

    local_time = time( NULL );
    unix_time (&local_time, &yy_l, &mon_l, &dd_l, &hh_l, &min_l, &ss_l);

    printf ("\nElevation: # %d  %.2d/%.2d/%.2d %.2d:%.2d:%.2d  vcp %d (Local Time: %.2d/%.2d/%.2d %.2d:%.2d:%.2d)\n\n", 
	(int)Elev_info.data.elev_cut_number, mon, dd, (yy - 1900) % 100, hh, min, ss, 
	Elev_info.data.vcp_number, mon_l, dd_l, (yy_l - 1900) % 100, hh_l, min_l, ss_l );

    return;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/
static int Read_options( int argc, char **argv ){

    extern int optind;
    int c;	
    int err;

    Monitoring_period = -1;
    Iostat_monitoring = 0;
    Free_monitoring = 0;
    Vmstat_monitoring = 0;
    err = 0;
    while ((c = getopt (argc, argv, "ifvh?")) != EOF) {
	switch (c) {

	    case 'i':
		Iostat_monitoring = 1;
                if( Free_monitoring || Vmstat_monitoring )
                   Print_usage( argv );
		break;

            case 'f':
		Free_monitoring = 1;
                if( Iostat_monitoring || Vmstat_monitoring )
                   Print_usage( argv );
		break;

            case 'v':
		Vmstat_monitoring = 1;
                if( Free_monitoring || Iostat_monitoring )
                   Print_usage( argv );
		break;

	    case 'h':
	    case '?':
		Print_usage( argv );
		break;
	}
    }

    /* If no monitoring selected, print error message and return. */
    if( !Iostat_monitoring && !Free_monitoring && !Vmstat_monitoring )
       Print_usage( argv );

    /* Get the monitoring period, in seconds. */
    if( optind == argc - 1 )       
        sscanf( argv[optind], "%d", &Monitoring_period );

    if( Monitoring_period < 0 )
       Monitoring_period = 2;

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/
static void Print_usage (char **argv) {

    printf ("Usage: %s (options)\n", argv[0]);
    printf ("Options:\n");
    printf ("     -i (iostat performance monitoring)\n");
    printf ("     -f (free performance monitoring)\n");
    printf ("     -v (vmstat performance monitoring)\n\n");
    printf ("Note:  i, f and v options are mutually exclusive\n");
    exit (0);
}

#ifdef LITTLE_ENDIAN_MACHINE
/**************************************************************************

   Description:
      Byte-swapping function for scan info event.

   Note:
      This is required on LITTLE_ENDIAN_MACHINE since the message is
      passed Big Endian format.

**************************************************************************/
char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len ){

   static char swapped_msg[ ORPGEVT_SCAN_INFO_DATA_LEN ];
   int num_ints = msg_len / sizeof( int );

   if( (event == ORPGEVT_SCAN_INFO) && (msg != NULL) ){

      MISC_bswap( sizeof(int), msg, num_ints, swapped_msg ); 
      memcpy( msg, swapped_msg, msg_len );
      return( msg );

   }

   return( msg );
   
}

#endif

/**************************************************************************

    Description: This function terminates this process.

**************************************************************************/
static int Terminate( int signal, int sigtype ){

    if( Monitor_pid != 0 )
       kill( Monitor_pid, signal );

   return (0);

}

