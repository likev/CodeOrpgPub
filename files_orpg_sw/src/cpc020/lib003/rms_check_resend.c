/**************************************************************************

   Module: rms_check_resend.c

   Description: This module handles resendeing unacknowleded messages sent to
   RMMS.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2007/01/05 18:02:16 $
 * $Id: rms_check_resend.c,v 1.20 2007/01/05 18:02:16 garyg Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define RMS_RESEND      37
#define MAX_RETRIES      5
/* Static Globals */

extern struct resend *resend_array_ptr;
extern int resend_lbfd;
extern int in_lbfd;
extern int rms_connection_down;
/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function checks the resend array and determines if
   a message needs to be resent to RMMS.  The function will attempt to resend
   a message three time.  if unable it will determine the interface down
   stop sending status and alarm messages and initiate an RMS up message to
   reestablish the interface.

   Input: None

   Output: Sends messages that have not received and acknowledgement.

   Returns: None

   Notes:

   **************************************************************************/
void check_resend() {

   time_t tm;

   int i;
   int buf_count, ret_val;
   int rms_down;
   int ret;
   int reset_value = 3;

    /* RMS buffer for resend messages */
   UNSIGNED_BYTE resend_buf[MAX_BUF_SIZE];

   /* Get current time */
   tm = time((time_t*)NULL);

   if(interface_errors > MAX_ERRORS){

      LE_send_msg(RMS_LE_ERROR,
             "RMS: Exceeded interface errors (MAX = %d / total = %d).", MAX_ERRORS, interface_errors);

      /* If there have been more than ten interface errors then
         shutdown the interface */

      resend_cnt = 0;

      /* Set interface flag for HCI and status log messages*/
      rms_connection_down = 1;

       /* Post an event to shut down the RMS interface and start the RMS up message. */
      if ((ret_val = EN_post (ORPGEVT_RESET_RMS, &reset_value, sizeof(reset_value), 0)) < 0) {
             LE_send_msg(RMS_LE_ERROR, "RMS: Failed to Post ORPGEVT_RESET_RMS (%d).\n",
                           ret_val );
      } /* End if statement */

      msleep (1000);

      /* Clear linear buffers for the interface*/

      if ((ret = LB_clear (in_lbfd, LB_ALL)) < 0 )
         LE_send_msg(RMS_LE_ERROR, "RMS: Unable to clear input linear buffer (ret %d).\n", ret );
      else
         LE_send_msg( RMS_LE_LOG_MSG, "RMS: Cleared input linear buffer(interface down).\n");

      if ((ret = LB_clear (resend_lbfd, LB_ALL)) < 0 )
          LE_send_msg(RMS_LE_ERROR, "RMS: Unable to clear resend linear buffer (ret %d).\n", ret );
      else
         LE_send_msg( RMS_LE_LOG_MSG, "RMS: Cleared resend linear buffer (interface down).\n");

      rms_down = 1;

      /* Post and event to alert HCI the interface is down */
/*      EN_post (ORPGEVT_RMS_CHANGE, &rms_down, sizeof(rms_down),0);

      LE_send_msg(GL_STATUS,"RMS: Interface down starting RMS UP message.");
*/
      return;
   } /* End if */

   if (resend_cnt != 0) {

      for (i = 0; i< MAX_RESEND_ARRAY_SIZE; i++) {

         /* Check to see if the message exists and it has been in the resend buffer longer than 20 seconds. */
         if (resend_array_ptr[i].msg_seq_num != 0 &&
            (RPG_TIME_IN_SECONDS(tm)) - resend_array_ptr[i].time_sent >= 20){

            /* Check to see if the message has been sent less than 3 times. */
            if (resend_array_ptr[i].retrys <= MAX_RETRIES){

               /* Read the message from the resend LB */
               buf_count = LB_read (resend_lbfd, (char*)resend_buf, MAX_BUF_SIZE,
                           resend_array_ptr[i].lb_location);

               if (buf_count != MAX_BUF_SIZE) {
                  LE_send_msg (RMS_LE_ERROR,
                     "RMS: LB read failed (ret %d) for check resend", buf_count);
                  break;
               }

               /* Write the message to the output LB */

               ret = send_message (resend_buf, RMS_RESEND, RMS_RETRY);

               if (ret != 1)
                  LE_send_msg (RMS_LE_ERROR, "RMS: Send message resend message failed (ret %d)", ret);

               /* If message had been sent three times and the interface is up, stop resending the message */
               if (resend_array_ptr[i].retrys == MAX_RETRIES && interface_errors < MAX_ERRORS){

                  LE_send_msg (RMS_LE_LOG_MSG,
                     "RMS: Last resend ORPG message %d",resend_array_ptr[i].msg_seq_num);

                  resend_array_ptr[i].msg_seq_num = 0;
                  resend_array_ptr[i].lb_location = 0;
                  resend_array_ptr[i].time_sent = 0;
                  resend_array_ptr[i].retrys = 0;

                  /* Decrement resend counter for removed message*/
                  if (resend_cnt != 0)
                     resend_cnt--;

               } /* End if */
               else {
                  /* Increment retrys */
                  resend_array_ptr[i].retrys += 1;

                  /* Increment error counter until acknowledgement received */
                  interface_errors ++;

                  LE_send_msg (RMS_LE_LOG_MSG,
                     "RMS: Resend ORPG message %d retry number %d.",
                        resend_array_ptr[i].msg_seq_num,resend_array_ptr[i].retrys);

               } /* End else */

               msleep(1000);

            } /* End if */
         }/*End if statement*/
      } /* End for loop */
   } /* End if statement */
   return;
}/* End check resend message */

