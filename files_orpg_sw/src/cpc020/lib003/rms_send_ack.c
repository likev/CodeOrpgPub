/**************************************************************************

   Module: rms_send_ack.c

   Description:  This is the module for building and sending acknowledgment
   messages.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:34 $
 * $Id: rms_send_ack.c,v 1.11 2003/06/26 14:51:34 garyg Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <infr.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define  ACK_TYPE 	37
#define MAX_ACK_PAD	50
/*
* Task Level Globals
*/
extern ushort reject_flag;

/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This function builds an acknowledgment message to be sent
   to RMMS.

   Input:   msg_struct - Pointer to a message header strucure.

   Output:  RMMS acknowledgement message.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
int build_ack(struct header *msg_header) {

	UNSIGNED_BYTE ack_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *buf_ptr;
	int ret_val;
	ushort num_halfwords = 25;
	int msg_size;


	/* Set pointer to beginning of buffer */
	buf_ptr = ack_buf;

	/* Place pointer past header */
	buf_ptr += MESSAGE_START;

	/* Type of message to be acknowledged */
	conv_ushort_unsigned(buf_ptr,&msg_header->message_type);
	buf_ptr += PLUS_SHORT;

	/* If RMS UP message reset interface before sending ack message*/
	if (msg_header->message_type == 39) {
		/* Clear all buffers and reset all timers and counters */
		if(reset_rms_interface() != 1)
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to reset rms interface.");
		} /* End if */

	/* Get message sequence number */
	conv_ushort_unsigned(buf_ptr,&msg_header->seq_num);
	buf_ptr += PLUS_SHORT;

	/* Get reject flag, if flag is zero it means command successful */
	conv_ushort_unsigned(buf_ptr,&reject_flag);
	buf_ptr += PLUS_SHORT;

	/* Reset reject flag for next message */
	reject_flag =0;

	/* Compute message size */
	msg_size = (buf_ptr - ack_buf);

	/* Pad message with zeros */
	pad_message (buf_ptr, msg_size, MAX_ACK_PAD);
	buf_ptr += (MAX_ACK_PAD - msg_size);

	/* Add terminator to message */
	add_terminator(buf_ptr);
	buf_ptr += PLUS_INT;

	/* Compute the number of halfwords in the message */
	num_halfwords = ((buf_ptr - ack_buf) / 2);

	/* Add header to the message, the message sequence number returned to
	   the FAA/RMMS will have the MSB set, this is done in build_header */
	ret_val = build_header(&num_halfwords, ACK_TYPE, ack_buf, msg_header->seq_num);

	if (ret_val != 1){
		LE_send_msg (RMS_LE_ERROR,"RMS: RMS build header failed for rms ack");
		return (-1);
		} /* End if */

	/* Send the message to the FAA/RMMS */
	ret_val = send_message(ack_buf,ACK_TYPE,RMS_ACK);

	if (ret_val != 1){
		LE_send_msg (RMS_LE_ERROR,"RMS: Send message failed (ret %d) for rms ack", ret_val);
		return (-1);
		} /* End if */

	return (1);
}/* End of send_ack*/
