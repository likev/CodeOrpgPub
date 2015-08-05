/**************************************************************************

   Module:  rms_interface.c

   Description:  This is the main module for the rms interface.


   Assumptions:

   **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 17:34:29 $
 * $Id: rms_interface.c,v 1.43 2014/10/03 17:34:29 steves Exp $
 * $Revision: 1.43 $
 * $State: Exp $
 */


/* System Include Files/Local Include Files */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

#include <orpg.h>
#include <infr.h>
#include <rms_util.h>
#include <rms_ptmgr.h>
#include <rms_message.h>
#include <misc.h>
#include <orpgtask.h>
#include <orpgedlock.h>
#include <orpgerr.h>


/* Constant Definitions/Macro Definitions/Type Definitions */

#define INITIALIZE                0
#define RESET                     1
#define CLOSE                     2
#define RMS_CONNECTION_TIMED_OUT  3
#define RMS_CONNECTION_LOST       4
#define RMS_SEND_STATUS_VALUE     60   /* Send status message every minute */
#define RMS_SEND_STATE_VALUE      60   /* Send RPG state message every minute */
#define RMS_SEND_ALARM_VALUE      60   /* Send alarm message every minute */
#define RMS_RESEND_VALUE          20   /* Check the resend buffer every 20 seconds*/
#define RMS_UP_VALUE              10   /* Send RMS up message every 10 seconds */
#define UTIL_MAX                  10   /* Utilization change that will genrate a status message */


/* Static Globals */

extern int rms_connection_down;
extern int  rms_shutdown_flag;
extern int  Ev_exit;     /* Set by the port manager to ensure shutdown message is sent */
/* extern char ttyport[80]; */

static int Ev_message_received;
static int Ev_rda_alarms_update;
static int Ev_rpg_status_change;
static int Ev_rda_status_change;
static int Ev_user_status_change;
static int Ev_scan_info_change;
static int Ev_load_shed_change;
static int Ev_inhibit_message;
static int Ev_text_message;
static int Ev_events;
static int Ev_status_message;
static int Ev_check_resend;
static int Ev_rms_up;
static int inhibit_time;
static int reset_flag;


/* Static Function Prototypes */

static int  check_utilization_change ();
static int  exit_interface (int signal, int sig_type);
static int  set_rms_events (int state);
static void En_callback (en_t evtcd, void *msg, size_t msglen);


/**************************************************************************

   Module:  Main

   Description:  This is the main module for the rms interface.

 **************************************************************************/

int main (int argc, char *argv[]) {

   int  sum, ret;
   char process_name[1024];
   char cmd_string[128];

      /* Terminate all previous instance of this process */

   sprintf (cmd_string, "%s%ld%s",
            "prm -keep-", (long) getpid(), " rms_interface\0");

   if ((ret = system (cmd_string)) < 0)
        LE_send_msg (GL_ERROR, 
           "Error terminating previous instances of rms_interface (errno: %d)", ret);


   /* Create the log error LB. */
   if ((ret = LE_create_lb (argv[0], 1000, 0, -1)) < 0) {
      fprintf (stderr, "Error creating LE log (err: %d)", ret);
               ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

   /* preserve the process name */
   strcpy (process_name, argv[0]);

   /* Process command line options */
/*   while (argc > 1){
      if (argv [1][0] == '-'){
         switch (argv [1][1]) {
            case 'p':
               sscanf (&argv [2][0], "%s", ttyport);
               argc--;
               argv++;
            break;

            default:
               argc--;
               argv++;
            break;
         }
      }
      else {
         argc--;
         argv++;
      }
   }
*/
   /* copy process name to global buffer */

   /* Register parent for socket manager task failure */

   if (( ret = EN_register 
         (ORPGEVT_RMS_SOCKET_MGR_FAILED, (void *) En_callback )) < 0) {
      LE_send_msg(GL_ERROR, 
        "Failed to register event ORPGEVT_RMS_SOCKET_MGR_FAILED (ret: %d)",
        ret );
   }

   /* Initialize the Log Error buffer */
   if ((ret = ORPGMISC_init (argc, argv, 1000, 0, -1, -1)) < 0) {
      LE_send_msg (RMS_LE_ERROR, "ORPGMISC_init failed: %d\n", ret);
      ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

   LE_send_msg (RMS_LE_LOG_MSG, "ORPGMISC_init %s registered", process_name);


   /* This function must be called (and called early) to prevent conflicts 
      with a library - this app opens the LB as read only but the library 
      opens the LB with write permissions. */
   ret = ORPGDA_write_permission (ORPGDAT_GSM_DATA);

   if (ret < 0) {
      LE_send_msg (GL_ERROR,
         "ORPGDA_write_permission (ORPGDAT_GSM_DATA) failed (err: %d)", ret);
   }


   /* Initialize the interface */
   if (init_rms_interface(argc, argv) != 1){
      LE_send_msg  (GL_ERROR | GL_CRIT_BIT, "RMS interface failed.");
      ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

   /* Register for events */
   if (set_rms_events(INITIALIZE) != 0){
      printf ("RMS: RMMS events failed.");
      ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   } /*End if */

   /* Register a termination callback */
   if (ORPGTASK_reg_term_handler(exit_interface) != 0){
      LE_send_msg( RMS_LE_LOG_MSG, "Unable to register termination handler.\n");
      ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   } /*End if */

   /* Set up to recieve shutdown command */
   LB_NTF_control(LB_NTF_SIGNAL, SIGUSR2);

   /* Main interface loop */
   while (1) {

      if(Ev_message_received){
         /*Clear message received flag */
         Ev_message_received = 0;
         /* Message handling callback */
         rms_handle_msg();
      } /*End if */

      if(Ev_rda_alarms_update){
         /*Clear alarms update flag */
         Ev_rda_alarms_update = 0;
         /* Alarm update callback */
         rms_send_alarm();
      } /*End if */

      if(Ev_rpg_status_change){
         LE_send_msg(RMS_LE_LOG_MSG,"Status sent (RPG Status)");
         /*Clear status change flag */
         Ev_rpg_status_change = 0;
         /* Status change callback */
         rms_send_rpg_state();
         /* Status change callback */
         send_status_msg();
      } /*End if */

      if(Ev_rda_status_change){
         LE_send_msg(RMS_LE_LOG_MSG,"Status sent (RDA Status)");
         /*Clear status change flag */
         Ev_rda_status_change = 0;
         /* Status change callback */
         send_status_msg();
      } /*End if */

      if(Ev_user_status_change){
         LE_send_msg(RMS_LE_LOG_MSG,"Status sent (Prod User)");
         /*Clear status change flag */
         Ev_user_status_change = 0;
         /* Status change callback */
         send_status_msg();
      } /*End if */

      if(Ev_scan_info_change){
         LE_send_msg(RMS_LE_LOG_MSG,"Status sent (Scan Info)");
         /*Clear status change flag */
         Ev_scan_info_change = 0;
         /* Status change callback */
         send_status_msg();
      } /*End if */

      if(Ev_load_shed_change){
         /*Clear status change flag */
         Ev_load_shed_change = 0;
         /* Check for utilization change */
         ret = check_utilization_change ();
         /* If a utilization value has changed send message */
         if ( ret == 1){
            LE_send_msg(RMS_LE_LOG_MSG,"Status sent (Load Shed)");
            /* Status change callback */
            send_status_msg();
         }/* End if */
      } /*End if */

      if(Ev_inhibit_message){
         /*Clear inhibit message flag */
         Ev_inhibit_message = 0;
         /* Inhibit callback */
         rms_inhibit(inhibit_time);
      } /*End if */

      if(Ev_text_message){
         /*Clear text message flag */
         Ev_text_message = 0;
         /* Free text callback */
         rms_send_free_text_msg();
      } /*End if */

      if(Ev_status_message){
         LE_send_msg(RMS_LE_LOG_MSG,"Status sent (Scheduled Update)");
         /*Clear status message flag */
         Ev_status_message = 0;
         /* Send status callback */
         send_status_msg();
      } /*End if */
         
      if(Ev_check_resend){
         /*Clear resend flag */
         Ev_check_resend = 0;
         /* Status change callback */check_resend();
      } /*End if */
      
      if(Ev_rms_up){
         /*Clear rms up flag */
         Ev_rms_up = 0;
         /* Status change callback */send_rms_up_msg();
      } /*End if */
         
      if(Ev_events){
         /*Clear set events flag */
         Ev_events = 0;
         /* Set events callback */
         ret = set_rms_events(reset_flag);
         if (ret >= 0){
            if(reset_flag == RESET){
               reset_rms_interface();
            }/* End reset if*/
         }/*End ret if*/
      }/*End Ev_events if*/
         
      if(Ev_exit || rms_shutdown_flag){
         /*Clear exit flag */
         Ev_exit = 0;

         /* Deregister all events */
         set_rms_events (CLOSE);

         /* Remove all LBs and close port manager */
         close_rms_interface();

         /* Send message that interface is shutting down */
         LE_send_msg(RMS_LE_LOG_MSG, "Shutting down the RMS interface");

         /* Exit program callback */
         exit (GL_EXIT_SUCCESS);
      } /*End if */

      /* Block the control loop */
      LB_NTF_control (LB_NTF_BLOCK);

      /* Summation of all flags */
      sum = Ev_message_received  + Ev_check_resend      +
            Ev_rda_alarms_update + Ev_rpg_status_change +
            Ev_inhibit_message   + Ev_text_message      +
            Ev_events            + Ev_status_message;
         
      /* If all flags have been processed wait 1 second before starting loop again */
      if( sum == 0){
         LB_NTF_control(LB_NTF_WAIT, 1000);
      } /* End if */
      else{
         /* If all flags haven't been processed unblock the control loop and begin processing */
         LB_NTF_control (LB_NTF_UNBLOCK);
      } /* End else */

         /* Check the health of the child process every 5 seconds */

      if ((time(NULL) % 5) == 0) {

         if (waitpid ((pid_t) RMS_get_child_pid(), NULL, WNOHANG) != 0) {
             LE_send_msg (GL_ERROR, "Child process abnormally terminated (child pid: %u",
                         (pid_t) RMS_get_child_pid());
             rms_shutdown_flag = 1; 
         }
      }
         
   } /* End while loop */   

   exit (0);   
         
}/* End main */

/**************************************************************************
   Description:  This function closes the interface before exiting ther program.

   Input: Signal and signal type.

   Output: Sets the exit flag.

   Returns: None

   Notes:

   **************************************************************************/
static int exit_interface (int signal, int sigtype){

   /* Set the shutdown flag to start the shutdown process*/
   rms_shutdown_flag = 1;

   /* Send an RPG state message to the RMMS */
   rms_send_rpg_state();

   return (1);

}/* End exit_interface */

/**************************************************************************
   Description:  This function controls the registering and deregistering
                        of events, timers and  alarms.

   Input: State of the interface

   Output: None

   Returns: 0 = successful edit.

   Notes:

   **************************************************************************/
static int set_rms_events (int state){

   int    ret = 0;
   int    lbfd, ret_val;
   time_t current_time ;
   
   Ev_message_received = 0;
   Ev_rda_alarms_update = 0;
   Ev_rpg_status_change = 0;
   Ev_inhibit_message = 0;
   Ev_text_message = 0;
   inhibit_time = 0;

   /* These only need to be done when initializing the interface */
   if(state == INITIALIZE){
      /* Register for event ORPGEVT_RMS_MSG_RECEIVED */
      if( (ret = EN_register( ORPGEVT_RMS_MSG_RECEIVED, (void *) En_callback )) < 0 ){
              LE_send_msg(RMS_LE_ERROR, 
                  "Failed to Register ORPGEVT_RMS_MSG_RECEIVED (%d).\n",ret );
      } /* End if */
      else {
        LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RMS_MSG_RECEIVED registered.\n");
      } /* End else */
         
      /* Register for event ORPGEVT_RESET_RMS */
      if( (ret = EN_register( ORPGEVT_RESET_RMS, (void *) En_callback )) < 0 ){
         LE_send_msg(RMS_LE_ERROR, "Failed to Register ORPGEVT_RESET_RMS (%d).\n", ret );
      } /* End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RESET_RMS registered.\n");
      } /* End else */

      /* Write to ORPGDAT_SYSLOG */
      ret = ORPGDA_write (ORPGDAT_SYSLOG, NULL, -1, 0);

      /* Get lb id for ORPGDAT_SYSLOG */
      lbfd = ORPGDA_lbfd(ORPGDAT_SYSLOG);

      /* Register for LB_notify for ORPGDAT_SYSLOG */
      if( (ret = LB_UN_register(lbfd, LB_ANY, rms_send_record_log_msg )) != LB_SUCCESS ){
         LE_send_msg( RMS_LE_LOG_MSG, "Failed to Register ORPGDAT_SYSLOG (%d).\n", ret );
      } /* End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGDAT_SYSLOG registered.\n");
      } /* End else */
   }/* End if */

   /* These only need to be done when reseting the interface */
   if (state == RESET){

      /* Deregister rms up message timer */
      if ((ret_val = MALRM_deregister( MALRM_SEND_RMS_UP)) < 0) {
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister MALRM_SEND_RMS_UP (%d).\n", ret_val);
         return (ret_val);
      } /* End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_RMS_UP deregistered (interface up).\n");
      } /* End else */
   } /*End if */

   /* These need to be done when reseting and initializing the interface */
   if (state == INITIALIZE || state == RESET){
      /* Register for event ORPGEVT_RDA_ALARMS_UPDATE */
      if( (ret = EN_register( ORPGEVT_RDA_ALARMS_UPDATE, (void *) En_callback )) < 0 ){
               LE_send_msg(RMS_LE_ERROR, 
                   "Failed to Register ORPGEVT_RDA_ALARMS_UPDATE (%d).\n",ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RDA_ALARMS_UPDATE registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_RPG_ALARM */
   if( (ret = EN_register( ORPGEVT_RPG_ALARM, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to Register ORPGEVT_RPG_ALARM (%d).\n",ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RPG_ALARM registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_RPG_STATUS_CHANGE ( Check later to see if good) */
   if( (ret = EN_register( ORPGEVT_RPG_STATUS_CHANGE, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, 
          "Failed to Register rpg ORPGEVT_RPG_STATUS_CHANGE (%d).\n",ret );
   } /*End if */
   else {
      LE_send_msg( RMS_LE_LOG_MSG, "ORPGEVT_RPG_STATUS_CHANGE registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_RDA_STATUS_CHANGE */
   if( (ret = EN_register( ORPGEVT_RDA_STATUS_CHANGE, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, 
          "Failed to Register rpg ORPGEVT_RDA_STATUS_CHANGE (%d).\n",ret );
   } /*End if */
   else {
      LE_send_msg( RMS_LE_LOG_MSG, "ORPGEVT_RDA_STATUS_CHANGE registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_PROD_USER_STATUS */
   if( (ret = EN_register( ORPGEVT_PROD_USER_STATUS, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, 
          "Failed to Register rpg ORPGEVT_PROD_USER_STATUS (%d).\n",ret );
   } /*End if */
   else {
      LE_send_msg( RMS_LE_LOG_MSG, "ORPGEVT_PROD_USER_STATUS registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_SCAN_INFO */
   if( (ret = EN_register( ORPGEVT_SCAN_INFO, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to Register rpg ORPGEVT_SCAN_INFO (%d).\n",ret );
   } /*End if */
   else {
      LE_send_msg( RMS_LE_LOG_MSG, "ORPGEVT_SCAN_INFO registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_LOAD_SHED_CAT */
   if( (ret = EN_register( ORPGEVT_LOAD_SHED_CAT, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, 
          "Failed to Register rpg ORPGEVT_LOAD_SHED_CAT (%d).\n",ret );
   } /*End if */
   else {
      LE_send_msg( RMS_LE_LOG_MSG, "ORPGEVT_LOAD_SHED_CAT registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_RMS_INHIBIT_MSG */
   if( (ret = EN_register( ORPGEVT_RMS_INHIBIT_MSG, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to Register ORPGEVT_RMS_INHIBIT_MSG (%d).\n", ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RMS_INHIBIT_MSG registered.\n");
   } /*End else */

   /* Register for event ORPGEVT_RMS_TEXT_MSG */
   if( (ret = EN_register( ORPGEVT_RMS_TEXT_MSG, (void *) En_callback )) < 0 ){
      LE_send_msg(RMS_LE_ERROR, "Failed to Register ORPGEVT_RMS_TEXT_MSG (%d).\n", ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RMS_TEXT_MSG registered.\n");
   } /*End else */

   /* Get current time to set timers for messages */
   current_time = time((time_t *) NULL) ;

   /* Register state message timer.*/
   if( (ret = MALRM_register( MALRM_SEND_STATE_MSG, (void *) En_callback)) < 0 ){
      if(ret != -204)
         LE_send_msg(RMS_LE_ERROR, "Unable To Register MALARM_SEND_STATE_MSG (%d).\n", ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_STATE_MSG registered.\n");
   } /*End else */

   /* Set the timer for 60 seconds. */
   ret = MALRM_set( MALRM_SEND_STATE_MSG, MALRM_START_TIME_NOW, RMS_SEND_STATE_VALUE );

   /* Register status message timer.*/
   if( (ret = MALRM_register( MALRM_SEND_STATUS_MSG, (void *) En_callback)) < 0 ) {
      if(ret != -204)
          LE_send_msg(RMS_LE_ERROR, "Unable To Register MALARM_SEND_STATUS_MSG (%d).\n", ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_STATUS_MSG registered.\n");
   } /*End else */

   /* Set the timer for 60 seconds.*/
   ret = MALRM_set( MALRM_SEND_STATUS_MSG, (current_time + 2), RMS_SEND_STATUS_VALUE );

   /* Register alarm message timer.*/
   if( (ret = MALRM_register( MALRM_SEND_ALARM_MSG, (void *) En_callback)) < 0 ){
      if(ret != -204)
         LE_send_msg(RMS_LE_ERROR, "Unable To Register MALARM_SEND_ALARM_MSG (%d).\n", ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_ALARM_MSG registered.\n");
   } /*End else */

   /* Set the timer for 60 seconds. Start 2 second after alarm to avoid conflict*/
   ret = MALRM_set( MALRM_SEND_ALARM_MSG, (current_time + 4), RMS_SEND_ALARM_VALUE );

   /* Register check the resend buffer timer.*/
   if( (ret = MALRM_register( MALRM_CHECK_RESEND, (void *) En_callback)) < 0 ){
      if(ret != -204)
         LE_send_msg(RMS_LE_ERROR, "Unable To Register MALARM_CHECK_RESEND (%d).\n", ret );
   } /*End if */
   else {
      LE_send_msg(RMS_LE_LOG_MSG, "MALRM_CHECK_RESEND registered.\n");
   } /*End else */

   /* Set the timer for 20 seconds.*/
   ret = MALRM_set( MALRM_CHECK_RESEND, MALRM_START_TIME_NOW, RMS_RESEND_VALUE );

   if (ret < 0) 
      LE_send_msg(RMS_LE_ERROR, "MALRM_set( MALRM_CHECK_RESEND,... failed (ret: %d)", ret);
   }/* End if */

   if((state == RMS_CONNECTION_TIMED_OUT) || (state == RMS_CONNECTION_LOST)) {

         /* Register rms up message timer.*/

      if( (ret_val = MALRM_register( MALRM_SEND_RMS_UP, (void *) En_callback)) < 0 ){
         if(ret != -204)
             LE_send_msg(RMS_LE_ERROR, "Unable To Register MALARM_SEND_RMS_UP (%d).\n", ret_val );
      }else 
         LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_RMS_UP registered.\n");

         /*Set the timer for 10 seconds.*/

      ret_val = MALRM_set( MALRM_SEND_RMS_UP, MALRM_START_TIME_NOW, RMS_UP_VALUE );

      rms_connection_down = 1;

        /* Post event to alert HCI the interface is down */

      EN_post (ORPGEVT_RMS_CHANGE, &rms_connection_down, sizeof(rms_connection_down), 0);

   } /* End if */

   if(state == CLOSE || state == RMS_CONNECTION_TIMED_OUT || state == RMS_CONNECTION_LOST) { 
      /* Deregister RPG status change */
      if ((ret = EN_deregister( ORPGEVT_RPG_STATUS_CHANGE, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister rpg ORPGEVT_RPG_STATUS_CHANGE(%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RPG_STATUS_CHANGE deregistered.\n");
      } /*End else */

      /* Deregister RDA status change */
      if ((ret = EN_deregister( ORPGEVT_RDA_STATUS_CHANGE, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister rpg ORPGEVT_RDA_STATUS_CHANGE(%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RDA_STATUS_CHANGE deregistered.\n");
      } /*End else */

      /* Deregister PROD_USER_STATUS */
      if ((ret = EN_deregister( ORPGEVT_PROD_USER_STATUS, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister rpg ORPGEVT_PROD_USER_STATUS(%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_PROD_USER_STATUS deregistered.\n");
      } /*End else */

      /* Deregister SCAN_INFO */
      if ((ret = EN_deregister( ORPGEVT_SCAN_INFO, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister rpg ORPGEVT_SCAN_INFO(%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_SCAN_INFO deregistered.\n");
      } /*End else */

      /* Deregister LOAD_SHED_CAT */
      if ((ret = EN_deregister( ORPGEVT_LOAD_SHED_CAT, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister rpg ORPGEVT_LOAD_SHED_CAT(%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_LOAD_SHED_CAT deregistered.\n");
      } /*End else */

      /* Deregister RDA alarms update */
      if( (ret = EN_deregister( ORPGEVT_RDA_ALARMS_UPDATE, En_callback  )) < 0 ){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister ORPGEVT_RDA_ALARMS_UPDATE (%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RDA_ALARMS_UPDATE deregistered.\n");
      } /*End else */

      /* Deregister ORPG alarms update */
      if( (ret = EN_deregister( ORPGEVT_RPG_ALARM, En_callback  )) < 0 ){
          LE_send_msg(RMS_LE_ERROR, 
              "Failed to deregister ORPGEVT_RPG_ALARM (%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RPG_ALARM deregistered.\n");
      } /*End else */

      /* Deregister inhibit message */
      if ((ret = EN_deregister( ORPGEVT_RMS_INHIBIT_MSG, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister ORPGEVT_RMS_INHIBIT_MSG (%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RMS_INHIBIT_MSG deregistered.\n");
      } /*End else */

      /* Deregister free text message */
      if ((ret = EN_deregister( ORPGEVT_RMS_TEXT_MSG, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister ORPGEVT_RMS_TEXT_MSG (%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RMS_TEXT_MSG deregistered.\n");
      } /*End else */

      /* Deregister send status timer */
      if ((ret_val = MALRM_deregister( MALRM_SEND_STATUS_MSG)) < 0) {
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister MALRM_SEND_STATUS_MSG (%d).\n", ret_val );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_STATUS_MSG deregistered.\n");
      } /*End else */

      /* Deregister send state timer */
      if ((ret_val = MALRM_deregister( MALRM_SEND_STATE_MSG)) < 0) {
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister MALRM_SEND_STATE_MSG (%d).\n", ret_val );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_STATE_MSG deregistered.\n");
      } /*End else */

      /* Deregister send alarms timer */
      if ((ret_val = MALRM_deregister( MALRM_SEND_ALARM_MSG)) < 0) {
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister MALRM_SEND_ALARM_MSG (%d).\n", ret_val );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "MALRM_SEND_ALARM_MSG deregistered.\n");
      } /*End else */

      /* Deregister resend message timer */
      if ((ret_val = MALRM_deregister(MALRM_CHECK_RESEND)) < 0) {
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister MALRM_CHECK_RESEND (%d).\n", ret_val );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "MALRM_CHECK_RESEND deregistered.\n");
      } /*End else */
   }

   if(state == CLOSE){
      /* Deregister message received event */
      if ((ret = EN_deregister( ORPGEVT_RMS_MSG_RECEIVED, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister ORPGEVT_RMS_MSG_RECEIVED(%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RMS_MSG_RECEIVED deregistered.\n");
      } /*End else */

      /* Deregister RMS reset event */
      if ((ret = EN_deregister( ORPGEVT_RESET_RMS, En_callback )) < 0){
         LE_send_msg(RMS_LE_ERROR, 
             "Failed to deregister ORPGEVT_RESET_RMS (%d).\n", ret );
      } /*End if */
      else {
         LE_send_msg(RMS_LE_LOG_MSG, "ORPGEVT_RESET_RMS deregistered.\n");
      } /*End else */
   } /*End if */

   return (0);

}/* End set events*/

/**************************************************************************
   Description:  This function sets the processing flags based on incoming events
                      timers, and alarms.

   Input: Event, message (if any), message size (if any).

   Output: Sets the processing flags for the main loop.

   Returns: None

   Notes:

   **************************************************************************/

void En_callback (en_t evtcd, void *msg, size_t msglen){

   /* This switch accepts the callbacks and sets the appropriate flag
      based on the event, timer or alarm that needs to be rpocessed */
   switch(evtcd){

      case ORPGEVT_RMS_MSG_RECEIVED:
         /* ORPG has received a message from the FAA/RMMS.
            If the flag is already set, keep trying until the flag
            is cleared and can be set again to accept the new message */
         if(Ev_message_received){
            LB_NTF_control(LB_NTF_REDELIVER);
            break;
         }
         /* Set the message received flag */
         Ev_message_received = 1;
      break;

      case ORPGEVT_RESET_RMS:
         /* Set the type of reset flag */
         reset_flag = *(int*)msg;
          /* Set the events flag */
         Ev_events = 1;
      break;

      case ORPGEVT_RMS_SOCKET_MGR_FAILED:
            /* the server process failed */
         Ev_exit = 1;
      break;

      case ORPGEVT_RDA_ALARMS_UPDATE:
         /* Set RDA alarms flag */
         Ev_rda_alarms_update = 1;
      break;
         
      case ORPGEVT_RPG_ALARM:
         /* Set RDA alarms flag */
         Ev_rda_alarms_update = 1;
      break;
         
      case ORPGEVT_RPG_STATUS_CHANGE:
         /* Set ORPG status change flag */
         Ev_rpg_status_change = 1;
      break;

      case ORPGEVT_RDA_STATUS_CHANGE:
         /* Set rda status message flag */
         Ev_rda_status_change = 1;
      break;

      case ORPGEVT_PROD_USER_STATUS:
         /* Set prod user status message flag */
         Ev_user_status_change = 1;
      break;

      case ORPGEVT_SCAN_INFO:
         /* Set scan info status message flag */
         Ev_scan_info_change = 1;
      break;

      case ORPGEVT_LOAD_SHED_CAT:
         /* Set load shed status message flag */
         Ev_load_shed_change = 1;
      break;

      case ORPGEVT_RMS_INHIBIT_MSG:
         /* Set inhibit message flag */
         Ev_inhibit_message = 1;
         /* Set inhibit time */
         inhibit_time = *(int*)msg;
      break;
         
      case ORPGEVT_RMS_TEXT_MSG:
         /* Set text message flag */
         Ev_text_message = 1;
      break;

      case MALRM_SEND_ALARM_MSG:
         /* Set alarm flag (every 60 seconds) */
         Ev_rda_alarms_update = 1;
      break;

      case MALRM_SEND_STATE_MSG:
         /* Set ORPG status change flag (every 60 seconds) */
         Ev_rpg_status_change = 1;
      break;

      case MALRM_SEND_STATUS_MSG:
         /* Set ORPG status flag (every 60 seconds) */
         Ev_status_message = 1;
      break;
         
      case MALRM_CHECK_RESEND:
         /* Set resend flag (every 20 seconds) */
         Ev_check_resend = 1;
      break;
         
      case MALRM_SEND_RMS_UP:
         /* Set rms up flag (every 10 seconds
            when interface connection is down) */
         Ev_rms_up = 1;
      break;
         
   }/* End switch */

}/* End En_callback */

/**************************************************************************
   Description:  

   Input: 

   Output: 

   Returns: 

   Notes:

   **************************************************************************/

static int check_utilization_change (){

   int new_storage_util;
   int new_input_util;
   int ret;

   static int old_storage_util;
   static int old_input_util;

   /* Get current ORPG on-line storage utilization level*/
   ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_PROD_STORAGE, LOAD_SHED_CURRENT_VALUE,
                           &new_storage_util);

   if (ret == ORPGLOAD_DATA_NOT_FOUND)
       return (0);

   /* Get the current ORPG input buffer utilization level*/
   ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_INPUT_BUF, LOAD_SHED_CURRENT_VALUE,
                           &new_input_util);
 
   if (ret == ORPGLOAD_DATA_NOT_FOUND)
       return (0);
 
   if (new_storage_util <= (old_storage_util - UTIL_MAX) || new_storage_util >= (old_storage_util + UTIL_MAX) ){
        /* At least one value has change so replace the old with the new and set the flag*/
        old_storage_util = new_storage_util;
        old_input_util = new_input_util;
        return (1);
   }/* End else if*/
   else if (new_input_util <= (old_input_util - UTIL_MAX) || new_input_util >= (old_input_util + UTIL_MAX) ){
        /* At least one value has change so replace the old with the new and set the flag*/
        old_storage_util = new_storage_util;
        old_input_util = new_input_util;
        return (1);
   }/* End else if */
   else {
      return (0);
   }/* End else */
 
}/* End check utilization change */
