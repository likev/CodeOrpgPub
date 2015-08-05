/*************************************************************************

      Module:  mnttsk_loadshed.c

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:40 $
 * $Id: mnttsk_loadshed.c,v 1.9 2005/12/27 16:41:40 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>


#include "mnttsk_ls_def.h"

/*
 * Static Function Prototypes
 */
static int Get_command_line_options( int argc, char **argv, int *startup_action );

/**************************************************************************
   Description:
      Initialization task for loadshed table.

   Input:
      argc - number of command line arguments
      argv - command line arguments

   Output:

   Returns:
      Exits with non-zero exit code on error, 0 exit code on success.

   Notes:

*****************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;
    int no_system_log_messages = 1;

    /* Initialize log-error services. */
    (void) ORPGMISC_init(argc, argv, 200, 0, -1, no_system_log_messages ) ;

    /* Get command line arguments. */
    retval = Get_command_line_options(argc, argv, &startup_action) ;
    if (retval < 0)
        exit(1) ;

    /* Initialize loadshed information. */
    retval = MNTTSK_LS_LEVELS_maint(startup_action) ;
    if (retval < 0) {

        LE_send_msg( GL_INFO,
                     "Data ID %d: MNTTSK_LS_LEVELS_maint(%d) failed: %d",
                     ORPGDAT_LOAD_SHED_CAT, startup_action, retval) ;
        exit(1) ;
    }

    exit(0) ;

/*END of main()*/
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

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
      printf ("\t\t-t Startup Mode/Action (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
