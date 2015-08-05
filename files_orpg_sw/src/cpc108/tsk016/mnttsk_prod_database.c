/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:45 $
 * $Id: mnttsk_prod_database.c,v 1.9 2005/12/27 16:41:45 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>


#include <prod_status.h>
#include "mnttsk_prod_database.h"


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Function Prototypes
 */
static int Get_command_line_options( int argc, char *argv[], int *startup_type );
static int Create_dir_message ();


/**************************************************************************
   Description:
      Driver module for initializing the product data base.

   Input:
      argc - number of command line arguments
      argv - command line arguments

   Output:

   Returns:
      Exits with non-zero exit code on error, zero exit code on success.

   Notes:

 **************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;

    (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

    retval = Get_command_line_options(argc, argv, &startup_action) ;
    if (retval < 0) 
        exit(1) ;

    /* On clear, remove all messages (products) from product data
       base. */
    if( startup_action == CLEAR ){

       char buf;

       retval = ORPGDA_clear( ORPGDAT_PRODUCTS, LB_ALL );
       if( retval < 0 ){

          LE_send_msg( GL_ERROR, "ORPGDA_clear Failed (%d)\n",
                       retval );
          exit(2);

       }

       LE_send_msg( GL_INFO, "Product Data Base Initialized\n" );

       /* Remove the product status if it exists.  We write a 0 length
          message. */
       retval = ORPGDA_write( ORPGDAT_PROD_STATUS, &buf, 0, PROD_STATUS_MSG );
       if( retval < 0 ){

          LE_send_msg( GL_ERROR, "Unable to Clear Product Status (%d)\n",
                       retval );
          exit(3);

       }

       LE_send_msg( GL_INFO, "Product Status Initialized\n" );

    }

    /* create the directory message in the database LB */
/*
    if (Create_dir_message () < 0)
	exit (1);
*/

    exit(0) ;

/*END of main()*/
}

/****************************************************************************

    Create an empty directory message in the database LB as a place holder.
    Returns 0 on success or a negative error code.

****************************************************************************/

static int Create_dir_message () {
    LB_info info;
    int n_msgs, ret;

    ORPGDA_open (ORPGDAT_PRODUCTS, LB_WRITE);
    if (ORPGDA_msg_info (ORPGDAT_PRODUCTS, 1, &info) >= 0)
	return (0);			/* the message exists */

    n_msgs = 0;
    ret = ORPGDA_write (ORPGDAT_PRODUCTS, (char *)&n_msgs, 
						sizeof (int), LB_ANY);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGDA_write Failed (%d)\n", ret);
	return (-1);
    }
    if ((ret = ORPGDA_previous_msgid (ORPGDAT_PRODUCTS)) != 1) {
	LE_send_msg (GL_ERROR, "Bad product DB dir message id (%d)\n", ret);
	return (-1);
    }
    return (0);
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP, RESTART, CLEAR)

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
      printf ("\t\t-t Startup Mode/Action (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
