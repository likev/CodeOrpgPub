/*************************************************************************

 Description:
    Maintenance Task: Manage RPG Tasks and Information

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:42 $
 * $Id: mnttsk_mngrpg.c,v 1.13 2005/12/27 16:41:42 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>


#include "mnttsk_mngrpg_def.h"

/* Function Prototypes.  */
static int Get_command_line_options( int argc, char *argv[], int *startup_action );

/**************************************************************************
   Description:
      Maintenance task for initialization of State File Shared data,
      Endian Value, and System Status Log.

   Input:
      argc - number of command line arguments.
      argv - command line arguments.

   Output:

   Returns:
      On success, exits with 0 exit code.  On failure, exits with non-zero
      exit code.

   Notes:

**************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;

    retval = Get_command_line_options(argc, argv, &startup_action) ;
    if (retval < 0)
        exit(1) ;

    (void) ORPGMISC_init(argc, argv, 100, 0, -1, 0) ;

    /* Call maintenance routine ... routine decides what to do */
    retval = MNTTSK_MNGRPG_CRITDS_maint(startup_action) ;
    if (retval < 0){

        LE_send_msg(GL_INFO,
                    "MNTTSK_MNGRPG_CRITDS_main() failed: %d", retval) ;
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
      startup_action - start up action (STARTUP or RESTART)

   Returns:
      Exits with non-zero error code on error, or returns 0 on success.

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

                  else if( strstr( start_up, "clear_syslog" ) != NULL )
                     *startup_action = CLEAR_SYSLOG;

                  else if( strstr( start_up, "clear_statefile" ) != NULL )
                     *startup_action = CLEAR_STATEFILE;

                  else if( strstr( start_up, "clear_all" ) != NULL )
                     *startup_action = CLEAR_ALL;

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
      printf ("\t\t-t Startup Mode (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
