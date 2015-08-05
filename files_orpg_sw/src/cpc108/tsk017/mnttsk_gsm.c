/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/08/22 16:11:37 $
 * $Id: mnttsk_gsm.c,v 1.10 2012/08/22 16:11:37 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */

#include <orpg.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

#define	STARTUP		1
#define	RESTART		2
#define	CLEAR		3

/*
 * Static Global Variables
 */

/*
 * Function Prototypes
 */
int MNTTSK_PD_GSM_maint( int startup_action );

/*
 * Static Function Prototypes
 */
static int Get_command_line_options( int argc, char *argv[], int *startup_type );


/**************************************************************************
   Description:
      Main module for the General Status Message Information Initialization.

   Input:
      argc - number of command line arguments
      argv - command line arguments

   Output:

   Returns:
      Non-zero exit code on error, zero exit code on success.

   Notes:

**************************************************************************/
int main(int argc, char *argv[]){

    int startup_type ;
    int retval ;


    /* Initialize log-error services. */
    (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

    if( (retval = Get_command_line_options(argc, argv, &startup_type )) < 0 )
       exit(1) ;

    /*
     * We call each of the maintenance routines in turn ... they are
     * responsible for deciding what to do (and generating the usual
     * informative messages) ...
     */

    if( (retval = MNTTSK_PD_GSM_maint(startup_type)) < 0 ){ 

       LE_send_msg( GL_INFO,
                   "Data ID %d: MNTTSK_PD_GSM_maint() failed: %d",
                   ORPGDAT_GSM_DATA, retval) ;
       exit(2) ;

    }

    exit(0) ;

/*END of main()*/
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP, RESTART, or CLEAR)

   Returns:
      exits on error, or returns 0 on success.

****************************************************************************/
static int Get_command_line_options( int argc, char *argv[], int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (argc, argv, "ht:")) != EOF) {

      switch (c) {

         case 't':
            if( strlen( optarg ) < 255 ){

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" ) ;
                  err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else
                     *startup_action = RESTART;

               }

            }
            else
               err = 1;

            break;
         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if (err == 1) {              /* Print usage message */
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-l Startup Action (optional - default: RESTART)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
