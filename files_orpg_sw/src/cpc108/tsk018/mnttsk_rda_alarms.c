/************************************************************************
 *	Module: mnttsk_rda_alarms.c					*
 *	Description: This module initializes the RDA alarms LB.  This	*
 *		     LB contains alarm information extracted from RDA	*
 *		     status messages.					*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:49 $
 * $Id: mnttsk_rda_alarms.c,v 1.5 2005/12/27 16:41:49 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*	System include file definitions.				*/

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>

/*	Local include file definitions.					*/

#include "mnttsk_rda_alarms.h"

/*	Static Function Prototypes	*/
 
static int Get_command_line_options( int argc, char *argv[], int *startup_type );

/**************************************************************************
 Description: This is the main function for the RDA alarms initialization
	      maintenance task.
       Input: argc - number of commandline arguments
	      argv - pointer to commandline argument data
      Output: NONE
     Returns: exit code
       Notes:
 **************************************************************************/

int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;

/*  initialize log error services.	*/

    (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

/*  parse commandline arguments.	*/

    retval = Get_command_line_options(argc, argv, &startup_action) ;
    if (retval < 0) 
        exit(1) ;

/*  if command to clear, remove all messages from RDA alarms LB	*/

    if( startup_action == CLEAR ){

       retval = ORPGDA_clear( ORPGDAT_RDA_ALARMS, LB_ALL );
       if( retval < 0 ){

          LE_send_msg( GL_ERROR, "ORPGDA_clear Failed (%d)\n",
                       retval );
          exit(2);

       }
       else
          LE_send_msg( GL_INFO, "ORPGDAT_RDA_ALARMS Cleared\n" );

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
      startup_action - start up action (STARTUP or RESTART)

   Returns:
      exits on error, or returns 0 on success.

*****************************************************************************/
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
