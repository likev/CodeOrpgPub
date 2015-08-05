/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:25:59 $
 * $Id: mon_cpu_use.c,v 1.3 2014/12/09 22:25:59 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <mon_cpu_use.h>

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Main program for monitoring CPU use on a Linux machine.

   Notes:
      This program reads the /proc/<pid>/stat file.   If the format
      changes, this program will also need to change.

/////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv [] ){

   int err, ret, first_time = 1;
   int elev_num, total_time = 0, num_vols = 0;

   double delta_user_cpu_time, delta_cpu_time, utilization;
   double est_util, cum_time = 0, cum_cpu_time = 0;

   int yy, mon, dd, hh, min, ss;

   /* Get command line arguments. */
   if( (err = Read_options( argc, argv )) < 0 )
      return 0;
   
   /* The following are needed in order to monitor CPU according
      to volume scan events. */

   /* Initialize log-error services. */
   ORPGMISC_init( argc, argv, 5000, 0, -1, 0 );

   /* Register for Scan Info Event. */
   ret = EN_register( ORPGEVT_SCAN_INFO, An_callback );
   if( ret < 0 ){

      LE_send_msg( GL_INFO, "EN_register failed (ret %d)\n", ret);
      exit (1);

   }

   /* Register a termination handler. */
   ORPGTASK_reg_term_handler( Signal_handler );

#ifdef LITTLE_ENDIAN_MACHINE
   /* Set byte swapping function for event message. */
   EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_msg);
#endif

   /* Initialize this program. */
   fprintf( stderr, "\n    >>>>  mon_cpu_use:  CPU Use Monitoring Tool for RPG Applications <<<<\n\n" );
   Num_CPUs = CU_initialize( Pid );
   if( (ret = CU_cpu_use( Pid, &Curr_stats )) < 0 )
      exit(0);

   /* To avoid compiler warning .... */
   elev_num = -1;

   /* Initialize the accumulators. */
   cum_time = cum_cpu_time = 0;

   /* If the processor capability was specified, calculate the utilization
      scale factor. */ 
   Scale_utilization = 1.0; 
   if( Proc_capability != 0 ){

      Scale_utilization = (double) Proc_capability / (double) DEFAULT_RPGA_CAPABILITY;
      Scale_utilization *= (double) Num_CPUs / (double) DEFAULT_NUM_CPUS;

   }

   /* Write the Scale Utilization value. */
   fprintf( stderr, "\nUTILIZATION SCALE FACTOR: %7.3f\n", Scale_utilization );

   /* Wait until a new volume scan is encountered. */
   while(1){

      if( Scan_info.scan_type != VOLUME_EVENT ){

         /* If Terminate flag is set greater than 1, cleanup and terminate. */
         if( Terminate > 1 ){ 

            Cleanup_fxn();
            exit(0);

         }

         /* Save off previous stats. */
         memcpy( &Prev_stats, &Curr_stats, sizeof(Cpu_stats_t) );

         /* Get the current CPU usage. */
         if( (ret = CU_cpu_use( Pid, &Curr_stats )) < 0 ){

            fprintf( stderr, "Some Error Occurred.  Exiting .....\n" ); 
            exit(0);

         }

         sleep(5);
         continue;

      }

      /* Must have encountered a VOLUME_EVENT .... */
      break;

   }


   /* Do Forever ..... */
   while(1){

      /* If terminate flag set twice (or more), break out of loop. */
      if( Terminate > 1 ){

         Cleanup_fxn();
         break;

      }

      /* Did we encounter a beginning of elevation of volume? */
      if( (Scan_info.scan_type != ELEVATION_EVENT)
                               &&
          (Scan_info.scan_type != VOLUME_EVENT) ){

         sleep(30);
         continue;

      }

      /* Save off previous stats. */
      memcpy( &Prev_stats, &Curr_stats, sizeof(Cpu_stats_t) );

      /* Code added to handle large time gaps in the data. */
      if( Delta_time > 100 ){

         fprintf( stderr, "Large Time Gap Encountered In Input Data\n" );
         Delta_time = 0;
         memset( &Prev_scan_info, 0, sizeof(Scan_info_t) );
         first_time = 1;

      }

      /* Get the current CPU usage. */
      if( (ret = CU_cpu_use( Pid, &Curr_stats )) < 0 ){

         fprintf( stderr, "Some Error Occurred.  Exiting .....\n" );
         exit(0);

      }

      /* We don't want to start print stats until the start of a new volume
         scan.  Also, we want to print the first elevation of stats based
         on the difference between start of elevation and start of volume. 
         Therefore we initially delay printing. */
      if( !first_time ){

         /* Compute the stats. */
         delta_cpu_time = (double) (Curr_stats.total_time - Prev_stats.total_time);
         delta_user_cpu_time = (double) ((Curr_stats.user_cpu_time + Curr_stats.sys_cpu_time)
                               - (Prev_stats.user_cpu_time + Prev_stats.sys_cpu_time));

         /*  We want total delta CPU time (total idle + total user + total system + 
             total wait + etc), not delta clock time. */ 
         utilization = (delta_user_cpu_time / ((double) delta_cpu_time)) * 100.0;
         est_util = utilization * Scale_utilization;

         /* We want the start of scan time, not the time from the most current
            scan event. */
         unix_time( &Prev_scan_info.scan_time, &yy, &mon, &dd, &hh, &min, &ss );

         /* Increment the accumulators.  These are for volume totals. */
         cum_time += delta_cpu_time;
         cum_cpu_time += delta_user_cpu_time;
         total_time += Delta_time;

         /* Print out elevation start date/time, elevation number, elevation time (measured
            from start of elevation to start of elevation), CPU utilization and estimated
            CPU utilization. */
         fprintf( stderr, "%.2d/%.2d/%.2d  %.2d:%.2d:%.2d   %2d         %3d          %10.2f       %10.2f          %6.2f       %6.2f\n",
                  mon, dd, (yy - 1900) % 100, hh, min, ss, (int) Prev_scan_info.rda_elev_num, 
                  (int) Delta_time, cum_time/Curr_stats.frequency, cum_cpu_time/Curr_stats.frequency, utilization, est_util );

         /* Compute max values. */
         if( utilization > Max_elev_util ){

            Max_elev_vol_num = Prev_scan_info.volnum;
            Max_elev_num = Prev_scan_info.rda_elev_num;
            Max_elev_vcp = Prev_scan_info.vcp;
            Max_elev_util = utilization;
            Max_elev_est_util = est_util;

         }
          
      }

      /* If at the beginning of volume, increment the number
         of volumes. */
      if( Scan_info.scan_type == VOLUME_EVENT ){

         /* Print out accumulations and averages. */
         if( !first_time ){

            double util = (cum_cpu_time / cum_time )  * 100.0;
            double est_util = util * Scale_utilization; 

            fprintf( stderr, "-----------------------------------------------------------------------------------------------------\n" );
            fprintf( stderr, "Totals:              %2d         %3d          %10.2f       %10.2f          %6.2f       %6.2f\n\n",
                     Prev_scan_info.rda_elev_num, total_time, cum_time/Curr_stats.frequency, cum_cpu_time/Curr_stats.frequency, util, est_util );

            /* Compute max values. */
            if( util > Max_vol_util ){

               Max_vol_num = Prev_scan_info.volnum;
               Max_vol_vcp = Prev_scan_info.vcp;
               Max_vol_util = util;
               Max_vol_est_util = est_util;

            }

            cum_time = 0;
            cum_cpu_time = 0;
            total_time = 0;

         }

         /* If all the volume scans processed or CTRl-C depressed, break out of loop. */
         if( (num_vols >= Volume_scans)
                      ||
              (Terminate >= 1) ){

            Cleanup_fxn();
            break;

         }

         /* Increment the number of volumes processed. */
         num_vols++;

         /* Print header. */
         unix_time (&Scan_info.scan_time, &yy, &mon, &dd, &hh, &min, &ss);

         fprintf( stderr, "=====================================================================================================\n" );
         fprintf( stderr, "Volume: # %d, VCP: %d, Date:  %.2d/%.2d/%.2d, Time: %.2d:%.2d:%.2d\n",
                  (int) Scan_info.volnum, Scan_info.vcp, mon, dd, (yy - 1900) % 100, hh, min, ss );
         fprintf( stderr, "=====================================================================================================\n" );
         fprintf( stderr, "    Date/Time       El #   Elapsed Time (s)   CPU Time (s)   Proc CPU Time (s)   Util (%%)      Est (%%)\n" );

         /* Reset some flags. */
         first_time = 0;

      }

      Scan_info.scan_type = NO_EVENT;

   }

   /* Wait for next event. */
   sleep(30);

   return 0;

/* End of main. */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      This function reads command line arguments.

   Inputs:     
      argc - number of command arguments
      argv - the list of command arguments

   Returns:     
      It returns 0 on success or -1 on failure.

///////////////////////////////////////////////////////////////////////\*/
static int Read_options( int argc, char *argv[] ){

   extern char *optarg;    
   extern int optind;
   int c;                 
   int err;              

   Pid = -1;
   Volume_scans = 1000000;
   Proc_capability = 0;
   err = 0;
   while( (c = getopt (argc, argv, "V:C:h?")) != EOF ){

      switch (c) {

         case 'V':
            Volume_scans = atoi( optarg );
            break;

         case 'C':
            Proc_capability = atoi( optarg );
            break;

         case 'h':
         case '?':
            Print_usage (argv);
            err = 1;
            break;

      }

   }

   /* On error, just return. */
   if( err )
      return -1;

   /* Get the PID of the process. */
   if( optind == argc - 1 )       
      sscanf( argv[optind], "%d", &Pid );

   /* Pid not specified or is not valid. */
   if( Pid < 0 ){

       Print_usage (argv);
       fprintf( stderr, "\nPid Not Specified or Incorrect\n");
       err = -1;
   }

   if( Proc_capability == 0 ){

      /* Try to open CPU Capability file. If file exists, read the capability. */
      char fname[256], buf[128];
      int ret;
      FILE *fd = NULL;

      ret = MISC_get_cfg_dir (fname, 156 - 32);
      if( ret >= 0 ){

         strcat( fname, "/.CPU_capability" );
         if( (fd = fopen( fname, "r" )) != NULL ){

            if( (fgets( buf, 128, fd ) == NULL) 
                             || 
                (sscanf( buf, "%d", &Proc_capability ) != 1)
                             ||
                (Proc_capability <= 0) )
               Proc_capability = 0;

            fclose(fd);

         }

      }

   }

   return (err);

/* End of Read_options(). */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      This function prints the usage info.

////////////////////////////////////////////////////////////////////////\*/
static void Print_usage( char *argv[] ){

   fprintf( stderr, "Usage: %s (options) pid\n", argv[0] );
   fprintf( stderr, "       Options:\n" );
   fprintf( stderr, "       -V <Volume Scans> (Default: 1 volume)\n" );
   fprintf( stderr, "       -C <CPU Capability> (From use_resource -t)\n" );
   fprintf( stderr, "       -h Help\n" );
   exit (0);

/* End of Print_usage(). */
}

#ifdef LITTLE_ENDIAN_MACHINE
/*\////////////////////////////////////////////////////////////////////////

   Description:
      Byte-swapping function for scan info event.

   Note:
      This is required on LITTLE_ENDIAN_MACHINE since the message is
      passed Big Endian format.

////////////////////////////////////////////////////////////////////////\*/
static char* Process_event_msg( int where, EN_id_t event, char *msg, 
                                int msg_len ){

   static char swapped_msg[ ORPGEVT_SCAN_INFO_DATA_LEN ];
   int num_ints = msg_len / sizeof( int );

   if( (event == ORPGEVT_SCAN_INFO) && (msg != NULL) ){

      MISC_bswap( sizeof(int), msg, num_ints, swapped_msg );
      memcpy( msg, swapped_msg, msg_len );
      return( msg );

   }

   return( msg );

/* End of Process_event_msg() */
}

#endif

/*\////////////////////////////////////////////////////////////////////////

   Description:
      The RPG event callback function.  Checks if the event is a
      Scan Info event.  If it is, checks for Start of Elevation or
      Start of Volume.

   Input:      
      See LB man page.
        
////////////////////////////////////////////////////////////////////////\*/
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg ){

    Orpgevt_scan_info_t *scan_info;

    /* Verify the event ..... looking for ORPGEVT_SCAN_INFO. */
    if( evtcd == ORPGEVT_SCAN_INFO ){

        scan_info = (orpgevt_scan_info_t *) msg;

      /* If end of volume or end of elevation, just return. */
      if( (scan_info->key == ORPGEVT_END_ELEV)
                          ||
          (scan_info->key == ORPGEVT_END_VOL) )
         return;

      /* Copy the previous scan information data. */
      memcpy( &Prev_scan_info, &Scan_info, sizeof(Scan_info_t) );

      /* Start of elevation .... */
      if( scan_info->key == ORPGEVT_BEGIN_ELEV )
         Scan_info.scan_type = ELEVATION_EVENT;

      /* Start of volume .... */
      else if (scan_info->key == ORPGEVT_BEGIN_VOL)
         Scan_info.scan_type = VOLUME_EVENT;

      Scan_info.scan_time = scan_info->data.time/1000 + (scan_info->data.date-1)*86400;
      Scan_info.rda_elev_num = scan_info->data.elev_cut_number;
      Scan_info.vcp = scan_info->data.vcp_number;
      Scan_info.volnum = scan_info->data.vol_scan_number;

      /* Set the delta time (time between current and previous scan). */
      Delta_time = Scan_info.scan_time - Prev_scan_info.scan_time;
      if( Prev_scan_info.scan_time == 0 )
         Delta_time = 0;

      /* Set the start time if not already set. */
      if( Start_time == 0 )
         Start_time = Scan_info.scan_time;

    }

/* End of An_callback() */
}

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Signal handler for mon_cpu_use.   Sets flag that signal received. 

   Inputs:
      see ORPGTASK man page.

   Returns:
      Always returns 0. 

/////////////////////////////////////////////////////////////////////////\*/
static int Signal_handler( int signal, int status ){

   /* Set flag so that we terminate. */
   Terminate++;

   return 1;

/* End of Signal_handler(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Write maximum values, then terminate. 

/////////////////////////////////////////////////////////////////////////\*/
static int Cleanup_fxn(){

   /* Elevation and volume max values. Max values are based on utilization, not CPU time. */
   fprintf( stderr, "\n\n*******************************MAXIMUM VALUES*******************************\n" );
   fprintf( stderr, "Elev Max:  Vol: %3d  VCP: %3d  Elev #: %2d   Util: %6.2f   Est: %6.2f\n",
            Max_elev_vol_num, Max_elev_vcp, Max_elev_num, Max_elev_util, Max_elev_est_util );  
   fprintf( stderr, "Vol Max:   Vol: %3d  VCP: %3d               Util: %6.2f   Est: %6.2f\n",
            Max_vol_num, Max_vol_vcp, Max_vol_util, Max_vol_est_util );  
   fprintf( stderr, "****************************************************************************\n" );
   exit(0);

/* End of Cleanup_fxn() */
}
