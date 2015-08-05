/*************************************************************************

   Module:  init_alert_request.c

   Description:
      Maintenance Task: Alert Request Message Initialization

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:24 $
 * $Id: init_alert_request.c,v 1.3 2005/12/27 16:41:24 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */

#include <init_alert_request.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define WX_ALERT_MSG_SIZE       600
#define MAX_NB_LINES            41


/*
 * Static Global Variables
 */


/*
 * Static Function Prototypes
 */
static int Get_command_line_options( int argc, char *argv[], int *startup_type );
static int Init_wx_alert_request ();


/**************************************************************************
   Description:
      Main module for alert request message initialization.

   Input:
      argc - number of command line arguments
      argv - the command line arguments

   Output:

   Returns:

   Notes:
      The process exits with non-zero exit code on failure, 0 exit code on
      successful completion or non-fatal completion.  

 **************************************************************************/
int main(int argc, char *argv[])
{
    int startup_type;
    int retval;


    /* Initialize log-error services. */
    (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

    /* Parse the command line options.   If parsing detects error, 
       exit with non-zero exit code. */
    if( (retval = Get_command_line_options(argc, argv, &startup_type )) < 0 )
    {
       exit(1);
    }

    /* Perform initialization according to startup mode.  Exit with non-zero
       exit code on failure. */
    if( (retval = Init_alert_request(startup_type)) < 0 )
    {
       LE_send_msg( GL_INFO,
          "init_alert_request main() failed: %d", retval) ;
       exit(3) ;
    }

    exit(0) ;

} /* end main() */


/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP or RESTART)

   Returns:
      exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_options( int argc, char *argv[], int *startup_action )
{
   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (argc, argv, "ht:")) != EOF) 
   {
      switch (c) 
      {
         case 't':
            if( strlen( optarg ) < 255 )
            {
               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) 
               {
                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" ) ;
                  err = 1 ;
               }
               else
               {
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
            {
               err = 1;
            } 

            break;

         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }
   }

   /* Print usage message */
   if (err == 1) 
   {
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Mode (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

} /* End Get_command_line_options() */



/**************************************************************************

   Description: 
      This function is the driver module for the initialization of the 
      alert adaptation data and alert request messages.

   Inputs:
      startup_action - startup mode.  Either STARTUP, RESTART, or CLEAR.

   Outputs:

   Returns:
      It returns 0 if no update performed,  the number of entries
                of the table if updated, or -1 on failure.
**************************************************************************/
int Init_alert_request(int startup_action)
{
   int retval ;

   /* Do the following according to startup mode. */
   if (startup_action == STARTUP) 
   {
      LE_send_msg( GL_INFO,
         "Alerting Maint. INIT FOR STARTUP DUTIES:");
      LE_send_msg(GL_INFO,
         "\t1. Initialize wx alert request messages.");

      /* Initialize the alert request messages. */
      retval = Init_wx_alert_request ();
      if (retval != 0) 
      {
         /* An error occurred!!! */
         LE_send_msg(GL_INFO,
            "Data ID %d: Init_wx_alert_request() failed: %d",
            ORPGDAT_WX_ALERT_REQ_MSG, retval) ;
         return(-1) ;
      }
        
      /* All normal. */
      LE_send_msg(GL_INFO,
         "Data ID %d: has been inititalized",
         ORPGDAT_WX_ALERT_REQ_MSG) ;
   }

   return(0) ;

} /* End Init_alert_request() */


/**************************************************************************

   Description: 
      This function intializes the WX alert request messages.
 
   Inputs:

   Outputs:

   Returns:  
      It returns 0 on success or -1 on failure.
 
   Notes:
      The maximum number of narrowband lines (41) is defined to 
      support legacy alerting.

      The alert request messages should be initialized with 0 length
      messages vice initialization using buffer defined on the stack.
      The stack contents are not guaranteed.  (SS note 04/24/01)

**************************************************************************/
static int Init_wx_alert_request ()
{
   char buf[WX_ALERT_MSG_SIZE];
   int i;
  
   LE_send_msg (GL_INFO,"Initializing wx alert request\n");
 
   for (i = 0 ; i < MAX_NB_LINES; i++) 
   {
      int n, ret;
  
      for (n = 1; n <= 2; n++) 
      {
         if ((ret = ORPGDA_write (ORPGDAT_WX_ALERT_REQ_MSG, buf,
                                  WX_ALERT_MSG_SIZE, i * 100 + n))
                                  != WX_ALERT_MSG_SIZE) 
         {
            LE_send_msg (GL_INFO | 18,
            "ORPGDA_write ORPGDAT_WX_ALERT_REQ_MSG failed (ret %d, size %d)",
                         ret, WX_ALERT_MSG_SIZE);
            return (-1);
         }

         if ((ret = ORPGDA_write (ORPGDAT_WX_ALERT_REQ_MSG, buf,
                                  0, i * 100 + n)) != 0) 
         {
            LE_send_msg (GL_INFO | 19,
               "ORPGDA_write ORPGDAT_WX_ALERT_REQ_MSG failed (ret %d, size 0)",
               ret);
            return (-1);
         }
      }
   }

   return (0); 

} /* End Init_wx_alert_request() */
