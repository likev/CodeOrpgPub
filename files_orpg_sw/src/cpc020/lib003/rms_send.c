/**************************************************************************

   Module:  rms_send.c

   Description:  This is the module for sending messages.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/05/27 17:04:23 $
 * $Id: rms_send.c,v 1.15 2003/05/27 17:04:23 garyg Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define BYTE_1			1

/*
* Task Level Globals
*/
extern int resend_cnt;
extern int resend_lbfd;
extern int out_lbfd;

static struct resend resend_array [MAX_RESEND_ARRAY_SIZE * sizeof(struct resend)];
struct resend *resend_array_ptr = resend_array;
LB_id_t LB_id_num;

/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This routine sends all outgoing messages from ORPG to the
   RMMS.  It writes the message to the outgoing linear buffer and posts an
   event that call the port manager to send it across the interface. It also
   increments the message sequence number and places the outgoing message in
   the resend buffer.

   Input: out_buf - Pointer to the buffer that contains the message.
   	  msg_type - The type of message that is being sent.

   Output:


   Returns: Message sent = 1, problem = -1.

   Notes:

   **************************************************************************/
int send_message (UNSIGNED_BYTE *out_buf, ushort msg_type, int msg_group){

	UNSIGNED_BYTE *buf_ptr;

	int ret_val;
	time_t tm;
	static int array_pos;
	ushort msg_seq_num;

	/* Set pointer to the beginning of the buffer */
	buf_ptr = out_buf;

	/* Place pointer at sequence number */
	buf_ptr += PLUS_SHORT;
	buf_ptr += PLUS_SHORT;
	buf_ptr += PLUS_SHORT;

	/* Get message sequence number */
	msg_seq_num = conv_ushrt (buf_ptr);

	/* Send outgoing messages to the linear buffer for the port manager*/

	/* Place message in ouput buffer for trasmit to FAA/RMMS*/
	ret_val = LB_write (out_lbfd, (char*)out_buf, MAX_BUF_SIZE, LB_id_num);

	if (ret_val != MAX_BUF_SIZE) {
		LE_send_msg (RMS_LE_ERROR,
			"RMS: LB_write rms send failed (ret %d)", ret_val);
		return (ret_val);
    		} /* End if */

    	/* If the message is a retry, rms up, or akcnowledgement message do not put in resend array */
	if(msg_group == RMS_STANDARD){

    		/* Place message in resend buffer in case acknowledement not received*/
    		ret_val = LB_write (resend_lbfd, (char*)out_buf, MAX_BUF_SIZE, LB_id_num);

		if (ret_val != MAX_BUF_SIZE) {
			LE_send_msg (RMS_LE_ERROR,
			"RMS: LB_write rms resend failed (ret %d)", ret_val);
			return (ret_val);
    			} /* End if */

    		/* Put message number into array until acknowlegement message received from FAA/RMMS.
		If message is an acknowledgment, retry, or rms up message do not put in resend array.*/
	 	resend_array_ptr[array_pos].msg_seq_num = msg_seq_num;
		resend_array_ptr[array_pos].lb_location = LB_id_num;
		resend_array_ptr[array_pos].msg_type = msg_type;
		tm = time((time_t*)NULL);
		resend_array_ptr[array_pos].time_sent = (RPG_TIME_IN_SECONDS(tm));
		resend_cnt++;

		/* If size of linear buffer is reached reset array position to zero*/
		if(array_pos < MAX_RESEND_ARRAY_SIZE - 1){
			array_pos++;
			}/* End if */
		else {
			array_pos = 0;
			}/* End else */
		}/* End if */

	/* If the array position is reset then reset the LB id number */
	if(array_pos == 0){
		LB_id_num = 0;
		}/* End if */
	else {
		LB_id_num++;
		}/* End else */

	/*  Post the write for RMS_PTMGR */
	if ((ret_val = EN_post (ORPGEVT_RMS_SEND_MSG, NULL, 0, 0)) < 0)
		  LE_send_msg(RMS_LE_ERROR, "RMS: Failed to Post ORPGEVT_RMS_SEND_MSG (%d).\n", ret_val );

	return (1);
} /* End send_message*/


/**************************************************************************
   Description:  This routine adds the message terminator to the outgoing
   message.  The terminator consist of 0X7F placed in the last 4 bytes.

   Input: buf_ptr - Pointer to the buffer that contains the message.

   Output:

   Returns:

   Notes:

   **************************************************************************/
void add_terminator (UNSIGNED_BYTE *buf_ptr) {

	short terminator_packet = 32639; /* hex 7F7F in last two halfwords of message*/
	int i;

	/* Put terminator packet at end of message. */
	for ( i = 0; i< 2; i++) {
		conv_ushort_unsigned(buf_ptr, (ushort*) &terminator_packet);
		buf_ptr += PLUS_SHORT;

		}/* End for loop*/

}/* End add terminator */


