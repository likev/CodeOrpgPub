/*************************************************************************

      Module:  mnttsk_prod_gen.c

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:48 $
 * $Id: mnttsk_prod_gen.c,v 1.10 2005/12/27 16:41:48 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>


#include "mnttsk_pg_def.h"


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Function Prototypes
 */
static int Get_command_line_options( int argc, char *argv[], int *startup_action );

/**************************************************************************
    Description:
       The maintenance task to handle data stores:
  
          ORPGDAT_RT_REQUESTS

   Inputs:
      argc - number of command line arguments
      argv - command line arguments

   Outputs:

   Returns:
      Exists with 0 exit code on success, non-zero exit code on failure.

   Notes:

**************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;

    retval = Get_command_line_options(argc, argv, &startup_action) ;
    if (retval < 0)
        exit(1) ;

    (void) ORPGMISC_init(argc, argv, 100, 0, -1, 0) ;

    /*
      Call maintenance routine ... the routine decides what
      to do ... the routine generates the usual informative messages ...
    */

    retval = MNTTSK_PG_RT_REQUEST_maint( startup_action ) ;
    if (retval < 0){

        LE_send_msg( GL_INFO,
                    "Data ID %d: MNTTSK_PG_RT_REQUEST_maint() failed: %d",
                    ORPGDAT_RT_REQUEST, retval) ;
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
      Exits with non-zero exit code on error, or returns 0 on success.

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
      printf ("\t\t-t Startup Mode/Action (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
