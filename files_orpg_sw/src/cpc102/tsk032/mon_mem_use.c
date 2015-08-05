/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/03 20:15:58 $
 * $Id: mon_mem_use.c,v 1.1 2009/03/03 20:15:58 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <mem_use.h>
#include <orpg.h>

/* Static Global Variables. */

/* Process ID. */
static int Pid = -1;

/* Summary or extended smaps Listing. */
static int Long_listing = 0;

/* Monitoring Period (0 - disabled). */
static int Sleep_time = 0;

/* Monitoring Duration (0 - disabled). */
static int Monitor_time = 0;

/* Termination Flag. */
static int Terminate = 0;

/* Function Prototypes. */
static int Read_options( int argc, char *argv[] );
static void Print_usage( char *argv[] );
static int Signal_handler( int signal, int status );
static int Cleanup_fxn();

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Main program for monitoring memory use on a Linux machine.

   Notes:
      This program reads the /proc/<pid>/smaps file.   If the format
      changes, this program will also need to change.

/////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv [] ){

   int err, ret;
   unsigned int elapsed_time = 0;

   /* Get command line arguments. */
   if( (err = Read_options( argc, argv )) < 0 )
      return 0;
   
   /* Register a termination handler. */
   ORPGTASK_reg_term_handler( Signal_handler );

   /* Initialize this program. */
   MU_initialize( Pid, Long_listing );

   while(1){

      /* If terminate flag is set, write out max values. */
      if( Terminate )
         Cleanup_fxn();

      /* Display formatted smaps output. */
      if( (ret = MU_display_smaps()) < 0 ){

         fprintf( stderr, "Some Error Occurred.  Exiting .....\n" ); 
         exit(0);

      }

      /* If periodic monitoring not enabled, break out of loop. */
      if( (Sleep_time == 0) || (Monitor_time == 0) )
         break;

      /* If elapsed_time is greater than Monitor time, break out of loop. */
      if( elapsed_time >= Monitor_time )
         break;

      sleep(Sleep_time);

      /* Increment elapsed time by the Sleep time. */
      elapsed_time += Sleep_time;

   }

   Cleanup_fxn();

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
   Long_listing = 0;
   Sleep_time = 0;
   Monitor_time = 0;
   err = 0;
   while( (c = getopt (argc, argv, "r:m:l?")) != EOF ){

      switch (c) {

         case 'm':
            Monitor_time = atoi( optarg );
            break;

         case 'r':
            Sleep_time = atoi( optarg );
            break;

         case 'l':
            Long_listing = 1;
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

       fprintf( stderr, "Pid Not Specified or Incorrect\n");
       err = -1;
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
   fprintf( stderr, "       -l (Long Listing. Default: Summary)\n" );
   fprintf( stderr, "       -m Monitor Period (Default: 0 seconds)\n" );
   fprintf( stderr, "       -r Monitor Rate (Default: 0 seconds)\n" );
   fprintf( stderr, "\nNote:  Command line options -m and -r are intended\n" );
   fprintf( stderr, "       to be used together.\n" );
   exit (0);

/* End of Print_usage(). */
}

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Signal handler for mon_mem_use.   Sets flag that signal received. 

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
      Cleanup and terminate. 

/////////////////////////////////////////////////////////////////////////\*/
static int Cleanup_fxn( ){

   MU_print_max_values();
   exit(0);

/* End of Cleanup_fxn() */
}
