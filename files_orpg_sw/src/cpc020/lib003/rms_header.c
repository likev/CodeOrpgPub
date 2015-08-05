/**************************************************************************
   
   Module:  rms_header.c
   
   Description:  This module handles all header processing for RMS
   messages.

   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:19 $
 * $Id: rms_header.c,v 1.13 2003/06/26 14:51:19 garyg Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <rda_rpg_message_header.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define MAX_SEQUENCE_NUMBER 32767
#define ACK_MSB		    	 32768

/*
* Static Globals
*/
ushort rms_msg_seq_number;


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This routine strips the header and places the header
   information into a structure for easier use by the calling routine.

   Input: msg_buffer - a pointer to the buffer containing the message.
   	  header_values - A pointer to the structure where the header
   	  will be stored.

   Output:  The header values placed into a header structure.


   Returns: 1 = Successful processing

   Notes:

   **************************************************************************/

int strip_header( UNSIGNED_BYTE *msg_buffer, struct header* header_values) {

	UNSIGNED_BYTE *buf_ptr;

	buf_ptr = msg_buffer;


	/* Read header values and place in header structure for easier processing.*/

	/*Message size in halfwords including message header */
	header_values->message_size = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

	/* RDA channel number */
	header_values->rda_redun = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

	/* ORPG channel number */
	header_values->rpg_redun = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

	/* Sequence number of message */
	header_values->seq_num = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

                /* Message type */
	header_values->message_type = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

	/* Flag indicating whether ORPG is in redundant configuration */
	header_values->redun_confg = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

	/* Julian date */
	header_values->julian_date = conv_ushrt(buf_ptr);
	buf_ptr += PLUS_SHORT;

	/* Seconds past midnight (GMT) */
	header_values->seconds = conv_intgr(buf_ptr);
	buf_ptr += PLUS_INT;

	/* Summation of halfwords in message */
	header_values->checksum = conv_intgr(buf_ptr);
	buf_ptr += PLUS_INT;

	return (1);

} /* End of strip_header */


/**************************************************************************
   Description:  This routine composes the message header and places
   it at the beginning of the outgoing message.

   Input: message_size - The size of the message being sent.
   	  message_type - The message number being sent to RMMS
   	  msg_buf - Pointer to the buffer containing the outgoing message.

   Output: The message header.


   Returns: Good header = 1, problem = -1.

   Notes:

   **************************************************************************/
int build_header(ushort *message_size, ushort message_type, UNSIGNED_BYTE *msg_buf, ushort ack_seq) {

	ushort rda_redun;
	ushort rms_up_seq_num = 0;
	ushort temp_short;
	ushort jul_date;

	char	Status_data [sizeof (RDA_status_t)];
	RDA_status_t	 *Rda_status;
	RDA_status_msg_t *rda_status;

	time_t tm;

	int time_in_seconds;
	int ret;
	int faa_redun;
	int rpg_redun;
	int checksum;
	UNSIGNED_BYTE *buf_ptr;

	/* Get the channel number for the RDA */
	ret = ORPGDA_read (ORPGDAT_GSM_DATA,
			(char *) Status_data,
			sizeof (RDA_status_t),
			RDA_STATUS_ID);

	if (ret < 0) {

	    LE_send_msg (RMS_LE_ERROR,"Failed ORPGDA_read (RDA_STATUS_ID) in header: %d\n", ret);
	    	rda_status = NULL;
	    	}
	else {
	                /* Set up RDA status information */
		Rda_status = (RDA_status_t *) Status_data;
		rda_status = (RDA_status_msg_t *) &Rda_status->status_msg;
		}

	/* Set the pointer to the beginning of the buffer */
	buf_ptr = msg_buf;

	/* Number of halfwords of two bytes each*/
	conv_ushort_unsigned(buf_ptr, message_size);
	buf_ptr += PLUS_SHORT;

	if(rda_status == NULL){
		rda_redun = 0;
	  	} /* End if */

	/* If redundant channel indicates zero then make it channel 1 */
	else if (rda_status->msg_hdr.rda_channel == 0){
		rda_redun = 1;
		} /* End else if */

	else {
	                /* Get RDA channel number */
		rda_redun = (ushort)rda_status->msg_hdr.rda_channel;
		} /* End else */

	/* Redundant Channel */
	conv_ushort_unsigned(buf_ptr, &rda_redun);
	buf_ptr += PLUS_SHORT;

	/* Get ORPG redundant channel */
/*	rpg_redun = ORPGSITE_get_int_prop(ORPGSITE_CHANNEL_NO); */
	rpg_redun = 2;

	if (ORPGSITE_error_occurred()){
		rpg_redun = 0;
		}

	/* Place ORPG channel number in message */
	temp_short = (ushort)rpg_redun;
	conv_ushort_unsigned(buf_ptr, &temp_short);
	buf_ptr += PLUS_SHORT;

	/* RPG sequence number*/
	if (message_type == 39) {

		/* If RMS UP message sequence number is zero */
		conv_ushort_unsigned(buf_ptr, &rms_up_seq_num);
		buf_ptr += PLUS_SHORT;

		} /* End if */


	else if ( message_type == 37){

	                /* Acknowledgements to a message from
	                the FAA/RMMS will have the sequence number
	                of the original message with the most significant bit set */
		temp_short = (ack_seq |= ACK_MSB);
		conv_ushort_unsigned(buf_ptr, &temp_short);
		buf_ptr += PLUS_SHORT;
		}

	else {
                                /* Place sequence number in the message */
		conv_ushort_unsigned(buf_ptr, &rms_msg_seq_number);
		buf_ptr += PLUS_SHORT;

		/* If sequence number max is reached reset to one*/
		if (rms_msg_seq_number == MAX_SEQUENCE_NUMBER){
			rms_msg_seq_number = 1;
			}
		else {
			rms_msg_seq_number++;
			} /* End else */

		} /* End else */


	/* Message type from header structure*/
	conv_ushort_unsigned(buf_ptr, &message_type);
	buf_ptr += PLUS_SHORT;

	/* Get redundant type */
/*	faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE); */
	faa_redun = 1;

	if (ORPGSITE_error_occurred()){
		faa_redun = 0;
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get reundant type for header");
		}

	/* Place redundant flag in message */
	temp_short = (ushort)faa_redun;
	conv_ushort_unsigned(buf_ptr, &temp_short);
	buf_ptr += PLUS_SHORT;

	/* Get time past midnight in seconds (GMT) */
	tm = time((time_t*)NULL);

	/* Get Julian date*/
	jul_date = RPG_JULIAN_DATE(tm);
	time_in_seconds = (RPG_TIME_IN_SECONDS(tm));

	/* Place Julian date in message */
	conv_ushort_unsigned(buf_ptr,&jul_date);
	buf_ptr += PLUS_SHORT;

	/* Place time in message */
	conv_int_unsigned(buf_ptr,&time_in_seconds);
	buf_ptr += PLUS_INT;

	/* Compute checksum for message */
	checksum = rms_checksum(msg_buf);

	conv_int_unsigned(buf_ptr,&checksum);
	buf_ptr += PLUS_INT;

	return (1);

	} /* End of build_header */
