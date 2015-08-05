/**************************************************************************
   
   Module: rms_up.c 
   
   Description:  This is the module for building and sending RMS UP
	messages. 
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2007/01/05 18:02:31 $
 * $Id: rms_up.c,v 1.12 2007/01/05 18:02:31 garyg Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define RMS_UP_TYPE 	39
#define RMS_UP_PAD_SIZE	50
/*
* Static Globals
*/

extern int out_lbfd;

ushort num_retrys;	
/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  When the ORPG interface detects and interface failure it
   sends and RMS up message every 60 seconds until an ack message is 
   received from RMMS.  This function builds that message and sends it.
      
   Input: None

   Output: RMs up message sent to the FAA/RMMS.

   Returns: None

   Notes:

   **************************************************************************/
void send_rms_up_msg() {

	UNSIGNED_BYTE rms_up_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *buf_ptr;

	int msg_size;
	int ret;
	int rms_down;

	ushort num_halfwords;

	/* Set buffer pointer to buffer start. */
	buf_ptr = rms_up_buf;

	/* Set the pointer past the header to the start of the message area */
	buf_ptr += MESSAGE_START;

	/* Place the number of retrys in the message */
	conv_ushort_unsigned(buf_ptr,&num_retrys);
	buf_ptr += PLUS_SHORT;

	/* Compute the size of the message */
	msg_size = buf_ptr - rms_up_buf;

	/* Pad the message with zeros */
	pad_message (buf_ptr, msg_size, RMS_UP_PAD_SIZE);
	buf_ptr += (RMS_UP_PAD_SIZE - msg_size);

	/* Add the terminator to the message */
	add_terminator(buf_ptr);
	buf_ptr += PLUS_INT;

	/* Compute the number of halfwords in the message */
	num_halfwords = ((buf_ptr - rms_up_buf) / 2);

	/* Add the header to the message */
	ret = build_header(&num_halfwords, RMS_UP_TYPE, rms_up_buf, 0);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
			"RMS: RMS build header failed for rms up");
		} /* End if */

    if ((ret = LB_clear (out_lbfd, LB_ALL)) < 0 )
         LE_send_msg(GL_ERROR, 
                     "Unable to clear output LB before sending RMS UP Msg (ret %d)\n", ret);

	/* Send the message to the FAA/RMMS */
	ret = send_message(rms_up_buf, RMS_UP_TYPE, RMS_UP);

	if (ret != 1) {
		LE_send_msg (RMS_LE_ERROR,
			"RMS: Send message rms up message failed (ret %d)", ret);
		} /* End if */

	/* Compute the number of minutes the interface has been down and send it to the log error LB */
	if((num_retrys % 6) == 0){
		LE_send_msg(RMS_LE_LOG_MSG,"RMS: Interface down for %d minute(s).", (num_retrys/6));

	        /* Reset interface down flag for HCI every minute to account
	            for starting the HCI while interface is down.  */
        	        rms_down = 1;

        	        /* Post an event to update HCI and change RMS button to red. */
        	        EN_post (ORPGEVT_RMS_CHANGE, &rms_down, sizeof(rms_down),0);
        	        } /* End if */
   
	/* Increment retrys */
	num_retrys ++;

} /* End send rms up message */
