/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:52:33 $
 * $Id: perf_mon.c,v 1.2 2014/03/13 19:52:33 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#define PERF_MON_MAIN
#include <perf_mon.h>

/* Macro Definitions. */
#define DEFAULT_RATE		20
#define NO_MONITOR		 0
#define RDA_MONITOR		 1
#define RPG_MONITOR		 2

/* Global Variables. */
Scan_info_t Scan_info;
Scan_info_t Prev_scan_info;
struct utsname *Uname_info = NULL;
int Volume_num = 0;
time_t Delta_time = 0;
time_t Delta_clock = 0;
time_t Start_time = 0;
int Library_link_failed = 1;
int Verbose = 0;

/* Global Static Variables. */
static int Terminate = 0;
static int RPG_monitor = -1;
static int Poll_rate = 0;
static int First_time = 1;
static char Buffer[512];

/* Function Prototypes. */
static void Poll_monitor();
static void Event_monitor();
static int Signal_handler( int signal, int status );
static int Cleanup_fxn( );
static int Read_options( int argc, char **argv );
static void Print_volume_elevation_info( int type );
static void Write_system_info();

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Main function for performance monitoring on the RPG and RDA.

/////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int ret;
   char *str = NULL;

   /* Read command line options. */
   if( (ret = Read_options( argc, argv )) < 0 )
      return 0;

   /* Register termination handler. */
   SIG_reg_term_handler( Signal_handler ); 

   /* Get system information. */
   Uname_info = malloc( sizeof( struct utsname ) );
   if( Uname_info != NULL ){

      /* Uname provides the system information. */
      if( uname( Uname_info ) == -1 ){

         fprintf( stderr, "Unable to get UNAME information.\n" );
         free( Uname_info );
         Uname_info = NULL;

      }

   }
   else
      fprintf( stderr, "malloc failed for %d bytes\n", sizeof( struct utsname ) );

   /* Get the number of CPUs .... */
   Sys_info = (const glibtop_sysinfo*) glibtop_get_sysinfo( );
   if( Sys_info == NULL ){

      fprintf( stderr, "Unable to Read System Information. \n" );
      exit(1);

   }

   Num_CPUs = Sys_info->ncpu;

   /* According to glibtop documentation, if single-processor,
      Sys_info->ncpu has a value of 0.  If this is the case
      we set Num_CPUs to 1 to avoid divide by zero. */
   if( Num_CPUs == 0 )
      Num_CPUs = 1;

   /* See whether this is RPG or RDA monitoring based on nodename
      if not already set by command line. */
   if( (Poll_rate == 0) && (RPG_monitor == NO_MONITOR) ){

      if( ((str = strstr( Uname_info->nodename, "rcpg" )) != NULL)
                                  ||
          ((str = strstr( Uname_info->nodename, "rvpg" )) != NULL) )
         RPG_monitor = RDA_MONITOR;

      else if( ((str = strstr( Uname_info->nodename, "rpg" )) != NULL) 
                                  ||
               ((str = strstr( Uname_info->nodename, "mscf" )) != NULL) )
         RPG_monitor = RPG_MONITOR;

   }

   /* Could not determine system to monitor.   Default to polling. */
   if( (Poll_rate == 0) && (RPG_monitor == NO_MONITOR) ){

      RPG_monitor = NO_MONITOR;
      Poll_rate = 20;

   }

   /* Register events. */
   if( RPG_monitor == RPG_MONITOR ){

      if( (ret = RPG_register_events()) < 0 )
         exit(0);

   }
   else if( RPG_monitor == RDA_MONITOR ){

      /* This is a safe default in the event the user did not 
         specify RPG on the command line and the node name does
         not follow standard naming convention.   That states
         recorded for RDA should be available for any Linux box. */
      if( (ret = RDA_register_events()) < 0 )
         exit(0);

   }

   /* Initialize the statistics. */

   /* Disk .... */
   DS_init_disk_usage();
   DS_get_disk_usage( Scan_info.scan_type, First_time );

   /* CPU .... */
   CS_init_cpu_usage( );

   /* Memory ... */
   MS_init_mem_usage( );

   /* Swap ... */
   SS_init_swap_usage( );

   /* Network ... */
   IS_init_interface_usage();

   /* The following are only appropriate if monitoring the RPG. */
   if( RPG_monitor == RPG_MONITOR ){

      /* LDM ... */
      LS_init_LDM_usage( );

      /* User ... */
      US_init_user_usage( );

   }

   /* Write the UNAME information and number of CPUs to file. */
   Write_system_info();

   /* Event monitoring or Poll monitoring? */
   if( Poll_rate > 0 )
      Poll_monitor();

   else{

      /* Code added to handle large time gaps in the data. */
      while(1){ 

         Event_monitor();

         /* If the First_time flag not reset, then break because
            no time gap detected. */
         if( First_time == 0 )
            break;

      }

   }

   return 0;

/* End of main(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Main function for performance monitoring using polling.  The polling
      rate is set by Poll_rate, a command line option.

/////////////////////////////////////////////////////////////////////////\*/
static void Poll_monitor(){

   static unsigned int ticker = 0;

   /* Initialize the Scan_info_t structure. */
   Scan_info.scan_type = VOLUME_EVENT;
   Scan_info.scan_time = time(NULL);
   Scan_info.rda_elev_num = 0;
   Scan_info.vcp = 0;
   
   memcpy( &Prev_scan_info, &Scan_info, sizeof(Scan_info_t) );

   /* Do Forever ..... */
   while(1){

      /* Save the previous scan information. */ 
      memcpy( &Prev_scan_info, &Scan_info, sizeof(Scan_info_t) );

      /* Update the scan information data. */
      if( (ticker % 10) == 0 )
         Scan_info.scan_type = VOLUME_EVENT;

      else
         Scan_info.scan_type = ELEVATION_EVENT;

      Scan_info.scan_time = time(NULL);
      
      /* Set the start time if not already set. */
      if( Start_time == 0 )
         Start_time = Scan_info.scan_time;

      /* Compute the delta time. */
      Delta_time = Scan_info.scan_time - Prev_scan_info.scan_time;

      /* Compute the wall clock delta time. */
      Delta_clock = Delta_time;

      /* Get CPU stats. */
      CS_get_cpu_usage( Scan_info.scan_type, First_time );

      /* Get Memory stats. */
      MS_get_memory_usage( Scan_info.scan_type, First_time );

      /* Get Swap stats. */
      SS_get_swap_usage( Scan_info.scan_type, First_time );

      /* Get Disk stats. */
      DS_get_disk_usage( Scan_info.scan_type, First_time );

      /* Get Interface stats. */
      IS_get_interface_usage( Scan_info.scan_type, First_time );

      /* If Terminate flag is set, cleanup and terminate. */
      if( Terminate == 1 ){

         Cleanup_fxn();
         exit(0);

      }

      First_time = 0;
      sleep(Poll_rate);

      ticker++;

   }

   return;

/* End of Poll_monitor(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Main function for performance monitoring using events.

/////////////////////////////////////////////////////////////////////////\*/
static void Event_monitor( ){

   time_t ctime = 0;
   static time_t ptime = 0;

   /* Do Until a Volume Event is encountered. */
   while(1){

      if( First_time && (Scan_info.scan_type != VOLUME_EVENT) ){

         /* If Terminate flag is set greater than 1, cleanup and terminate. */
         if( Terminate > 1 ){

            Cleanup_fxn();
            exit(0);

         }

         /* Read the CPU stats. */
         CS_read_stats();

         /* Read the Swap stats. */
         SS_read_stats();

         /* Read the Interface stats. */
         IS_read_stats();

         /* The following are only appropriate if monitoring the RPG. */
         if( RPG_monitor == RPG_MONITOR ){

            /* Read the LDM stats. */
            LS_read_stats();

            /* Read the User stats. */
            US_read_stats();

         }

         sleep(5); 
         continue;

      }

      /* Must have encountered a VOLUME_EVENT .... */
      break;

   }

   /* Do Forever ..... */
   while(1){

      /* Did we encounter a beginning of elevation of volume? */
      if( (Scan_info.scan_type == ELEVATION_EVENT) 
                               || 
          (Scan_info.scan_type == VOLUME_EVENT) ){

         /* At beginning of volume, print volume start date/time. */
         if( Scan_info.scan_type == VOLUME_EVENT ){

            Volume_num++;

            if( First_time )
               Print_volume_elevation_info( PM_VOLUME_DATA );

         }
           
         /* Compute the wall clock delta time. */
         ctime = time(NULL);
         Delta_clock = ctime - ptime;
         ptime = ctime;

         /* Code added to handle large time gaps in the data. */
         if( Delta_time > 100 ){

            fprintf( stderr, "Resetting First Time flag Owing to Data Gap\n" );
            First_time = 1;
            
            memset( &Prev_scan_info, 0, sizeof(Scan_info_t) );
            Delta_time = 0;

            return;

         }

         /* Get CPU stats. */
         CS_get_cpu_usage( Scan_info.scan_type, First_time );

         /* Get Memory stats. */
         MS_get_memory_usage( Scan_info.scan_type, First_time );

         /* Get Swap stats. */
         SS_get_swap_usage( Scan_info.scan_type, First_time );

         /* Get Disk stats. */
         DS_get_disk_usage( Scan_info.scan_type, First_time );

         /* Get Interface stats. */
         IS_get_interface_usage( Scan_info.scan_type, First_time );

         /* The following are only appropriate if monitoring the RPG. */
         if( RPG_monitor == RPG_MONITOR ){

            /* Get LDM stats. */
            LS_get_LDM_usage( Scan_info.scan_type, First_time );

            /* Get User stats. */
            US_get_user_usage( Scan_info.scan_type, First_time );

         }

         /* If Terminate flag is set, cleanup and terminate. */
         if( (Scan_info.scan_type == VOLUME_EVENT) 
                            && 
                     (Terminate == 1) ){

            Cleanup_fxn();
            exit(0);

         }

         /* This needs to be printed out after the utilization for the previous
            cut is printed. */
         if( (!First_time) && (Scan_info.scan_type == VOLUME_EVENT) )
            Print_volume_elevation_info( PM_VOLUME_DATA );

         else if( Scan_info.scan_type == ELEVATION_EVENT )
            Print_volume_elevation_info( PM_ELEVATION_DATA );

         /* Reset the scan type. */
         Scan_info.scan_type = NO_EVENT;

         /* Clear the first time flag. */
         First_time = 0;

      }

      /* If Terminate flag is set greater than 1, cleanup and terminate. */
      if( Terminate > 1 ){

         Cleanup_fxn();
         exit(0);

      }

      sleep(5);

   }

   return;

/* End of Event_monitor(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Signal handler for perf_mon.   Sets flag that signal received. 

   Inputs:
      see ORPGTASK man page.

   Returns:
      Always returns 0. 

/////////////////////////////////////////////////////////////////////////\*/
static int Signal_handler( int signal, int status ){

   /* Set flag so that we terminate at next start of volume.  If the poll
      rate is greater than 0, terminate immediately. */
   Terminate++;

   if( ((Terminate > 1) && (First_time)) || (Poll_rate > 0) ){

      fprintf( stderr, "perf_mon Terminating .....\n" );
      return 0;

   }

   return 1;

/* End of Signal_handler(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup and terminate. 

/////////////////////////////////////////////////////////////////////////\*/
static int Cleanup_fxn( int signal, int status ){

   /* Call all cleanup functions. */
   CS_cleanup();
   MS_cleanup();
   SS_cleanup();
   DS_cleanup();
   IS_cleanup();

   /* The following are only appropriate for monitoring the RPG. */
   if( RPG_monitor == RPG_MONITOR ){

      LS_cleanup();
      US_cleanup();

   }

   exit(0);

/* End of Cleanup_fxn() */
}

#define MAX_STR_LEN		8

/*\/////////////////////////////////////////////////////////////////////////

   Description:  
      This function reads command line arguments.

   Input:        
      argc - Number of command line arguments.
      argv - Command line arguments.

   Output:       
      Usage message

   Returns:      
      0 on success or -1 on failure

///////////////////////////////////////////////////////////////////////\*/
static int Read_options (int argc, char **argv){

    extern char *optarg;    
    extern int optind;
    int c;                  
    int err;                
    char system[MAX_STR_LEN+1], *str_p = NULL;

    RPG_monitor = NO_MONITOR;
    Poll_rate = 0;
    err = 0;
    while ((c = getopt (argc, argv, "vr:s:h?")) != EOF) {
        switch (c) {

            /* If set, poll instead of event based. */
            case 'r':
               Poll_rate = atoi( optarg );
               if( Poll_rate < 0 )
                  Poll_rate = DEFAULT_RATE;

               break;

            /* Check if this is RPG or RDA monitoring. */
            case 's':
               strncpy( system, optarg, MAX_STR_LEN );
               system[MAX_STR_LEN -1] = '\0';
               if( (str_p = strstr( &system[0], "RDA" )) != NULL )
                  RPG_monitor = RDA_MONITOR;

               else if( (str_p = strstr( &system[0], "RPG" )) != NULL )
                  RPG_monitor = RPG_MONITOR;

               break;

            /* Verbose mode. */
            case 'v':
               Verbose = 1;
               break;

            /* Print out the usage information. */
            case 'h':
            case '?':
                err = 1;
                break;
        }
    }

    if( (err == 1)
            ||
        ((RPG_monitor != NO_MONITOR) && (Poll_rate > 0)) ){                     /* Print usage message */

        printf ("Usage: %s [options]\n", argv[0]);
        printf ("       Options:\n");
        printf ("       s - \"RDA\" or \"RPG\" monitoring (Default: Unknown)\n" );
        printf ("       r - Polling rate (seconds)\n" );
        printf ("       v - Verbose mode\n\n" );
        printf ("       Note 1:  The -s and -r options are mutually exclusive.\n\n" );
        printf ("       Note 2:  If -r and -s options are not specified, perf_mon will attempt\n" );
        printf ("                to determine system based on host name.   If host name does\n" );
        printf ("                not include \"rpg\" or \"rda\" in its name, then polling\n" );
        printf ("                will be used with a default polling rate of 20 s.\n" );

        exit(0);

    }

    return (0);

/* End of Read_options() */
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Prints volume and elevation info to the terminal.
        
//////////////////////////////////////////////////////////////////////\*/
static void Print_volume_elevation_info( int type ){

   int yy, mon, dd, hh, min, ss;

   /* If not Verbose mode, just return. */
   if( !Verbose )
      return;

   /* Get year, month, day, hours, minutes and seconds. */
   unix_time (&Scan_info.scan_time, &yy, &mon, &dd, &hh, &min, &ss);

   if( type == PM_VOLUME_DATA ){

      fprintf( stderr, "===============================================================\
================================\n" );
      fprintf( stderr, "VCP: %d, Date:  %.2d/%.2d/%.2d, Time: %.2d:%.2d:%.2d\n",
               Scan_info.vcp, mon, dd, (yy - 1900) % 100, hh, min, ss );

   }
   else if( type == PM_ELEVATION_DATA ){

      fprintf( stderr, "---------------------------------------------------------------\
--------------------------------\n" );
      fprintf( stderr, "Elevation: # %d, Date:  %.2d/%.2d/%.2d, Time: %.2d:%.2d:%.2d\n",
               (int) Scan_info.rda_elev_num, mon, dd, (yy - 1900) % 100,
               hh, min, ss );

   }
   return;

/* End of Print_volume_info() */
}


/*\//////////////////////////////////////////////////////////////////////

   Description:
      Writes system info to node.uname.info file.
        
//////////////////////////////////////////////////////////////////////\*/
static void Write_system_info(){

   int size, uname_fd = -1;

   /* Is the UNAME info available?. */
   if( Uname_info != NULL ){

      char name[256], file[256];

      if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

         fprintf( stderr, "MISC_get_work_dir failed\n" );

         /* Files will be created in the current working directory. */
         name[0] = '\0';

      }

      sprintf( file, "/%s.uname.info", Uname_info->nodename );
      strcat( name, file );

      /* Open the file.  If uname_fd is not negative, the file exists.
         Close the file and remove it. */
      if( (uname_fd = open( name, O_RDWR, 0666 )) >= 0 ){

         close( uname_fd );
         unlink( name );
         uname_fd = -1;

      }

      /* Create the "uname.info" filei with read/write priviledges. */
      if( (uname_fd = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

         fprintf( stderr, "Open Failed For FILE %s\n", name );
         uname_fd = -1;

      }

      /* Write the UNAME and other machine information to file. */
      if( uname_fd >= 0 ){

         /* UNAME information. */
         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "System Name: %s\nNode Name: %s\nRelease: %s\nVersion: %s\nMachine: %s\n",
                  Uname_info->sysname, Uname_info->nodename, Uname_info->release, Uname_info->version,
                  Uname_info->machine );
         size = strlen( Buffer );
         if( write( uname_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to uname_fd Failed\n" );

         /* Write the number of CPUs to file. */
         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "\nNumber of CPUs: %lld\n", Num_CPUs );
         size = strlen( Buffer );
         if( write( uname_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to uname_fd Failed\n" );

         /* Write the total memory size to file. */
         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "Total Memory: %lld Bytes\n", Total_memory );
         size = strlen( Buffer );
         if( write( uname_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to uname_fd Failed\n" );

         /* Write the total memory size to file. */
         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, "Total Swap: %lld Bytes\n", Total_swap );
         size = strlen( Buffer );
         if( write( uname_fd, Buffer, size ) != size )
            fprintf( stderr, "Write to uname_fd Failed\n" );

         /* Close the uname.info file .... it is no longer needed. */
         close(uname_fd);

      }
   
   }

/* End of Write_system_info(). */
}
