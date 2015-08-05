/**************************************************************************

   Module:  rms_rec_ack.c

   Description:  This module handles acknowledgement messages from RMMS.


   Assumptions:

   **************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2007/01/05 18:02:28 $
 * $Id: rms_rec_ack.c,v 1.17 2007/01/05 18:02:28 garyg Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */


/* System Include Files/Local Include Files */

#include <rms_message.h>


/* Constant Definitions/Macro Definitions/Type Definitions */


/* Static Globals */


extern struct resend *resend_array_ptr;
extern int resend_cnt;


/* Static Function Prototypes */


/**************************************************************************

   Description:  This function reads the ack message and removes the message
                 from the resend buffer.

         Input: in_buf - Pointer to the message buffer.

        Output:

        Return:

 **************************************************************************/

int receive_ack(UNSIGNED_BYTE *in_buf){

   UNSIGNED_BYTE *buf_ptr;
   int i, ret_val;
   int reset_value = 1;
   int rms_down;

   ushort msg_type;
   ushort seq_num;
   ushort ack_code;

   extern int in_lbfd;

      /* Set pointer to beginning of buffer */

   buf_ptr = in_buf;

      /* Clear variables */

   seq_num = 0;
   msg_type = 0;
   ack_code = 0;

      /* Place pointer past header */

   buf_ptr += MESSAGE_START;

      /* Get message type from RMMS message */

   msg_type = conv_ushrt(buf_ptr);
   buf_ptr += PLUS_SHORT;

      /* Get sequence number */

   seq_num = conv_ushrt(buf_ptr);
   buf_ptr += PLUS_SHORT;

      /* Get acknowledgement code */

   ack_code = conv_ushrt(buf_ptr);
   buf_ptr += PLUS_SHORT;

      /* If the message being acknowledged is an RPG state message
         reset the color of the HCI button. */

/*   if (msg_type == 1) {
         rms_down = 0;
         EN_post (ORPGEVT_RMS_CHANGE, &rms_down, sizeof(rms_down), 0);
   }
*/
      /* If the message being acknowledged is an rms up message
         (39) then the interface has been down.  Stop sending rms
         up messages and clear all buffers and counters to begin
         the normal interface function */

   if (msg_type == 39) {

         /* Clear the input buffer */

      if ((ret_val = LB_clear (in_lbfd, LB_ALL)) < 0 )
         LE_send_msg(RMS_LE_ERROR, 
                     "Unable to clear input linear buffer (ret %d).\n", ret_val );
      else
         LE_send_msg(RMS_LE_LOG_MSG, 
                     "Cleared input linear buffer after restart.\n");

         /* Reset the interface */

      if ((ret_val = EN_post (ORPGEVT_RESET_RMS, &reset_value, sizeof(reset_value), 0)) < 0)
         LE_send_msg(RMS_LE_ERROR, 
                     "Failed to Post ORPGEVT_RESET_RMS (%d).\n", ret_val );
      else
         LE_send_msg(GL_STATUS, "Restarting the RMS interface");

   }

      /* Message being acknowledged is not an rms up message */

   if (msg_type != 39) {

      /* ACK received for this message so remove it from resend list */

      for (i=0; i< MAX_RESEND_ARRAY_SIZE; i++) {

         if (seq_num == resend_array_ptr[i].msg_seq_num) {
            resend_array_ptr[i].msg_seq_num = 0;
            resend_array_ptr[i].lb_location = 0;
            resend_array_ptr[i].time_sent = 0;
            resend_array_ptr[i].retrys = 0;

            if (resend_cnt != 0)
                resend_cnt--;

            break;
         }
      }
   }

      /* Acknowledgement received therefore the interface is up so clear 
         error counter */

   interface_errors = 0;

   return (1);

}
