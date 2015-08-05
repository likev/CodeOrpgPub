/**************************************************************************

   Module:  rms_inhibit.c

   Description:  This module builds an inhibit message.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: dans $
 * $Locker:  $
 * $Date: 2001/04/11 15:39:08 $
 * $Id: rms_inhibit.c,v 1.7 2001/04/11 15:39:08 dans Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define STATUS_TYPE  38
/*
* Static Globals
*/

/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This function sends an inhibit message to the FAA/RMMS..

   Input: Number of seconds FAA/RMMs is inhibited.

   Output:

   Returns:

   Notes:

   **************************************************************************/

int rms_inhibit (int seconds) {

	UNSIGNED_BYTE inhibit_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *inhibit_buf_ptr;
	short inhibit_time;
	int ret;
	ushort num_halfwords;

	/* Set pointer to beginning of buffer */
	inhibit_buf_ptr = inhibit_buf;

	/* Place pointer past header */
   	inhibit_buf_ptr += MESSAGE_START;

	/* Set inhibit time */
	inhibit_time = (short)seconds;

   	/* Put inhibit time in output buffer */
   	conv_short_unsigned(inhibit_buf_ptr,&inhibit_time);
   	inhibit_buf_ptr += PLUS_SHORT;

	/* Add terminator to the message */
	add_terminator(inhibit_buf_ptr);
	inhibit_buf_ptr += PLUS_INT;

	/* Compute number of halfwords in the message */
	num_halfwords = ((inhibit_buf_ptr - inhibit_buf) / 2);

	/* Add header to message */
	ret = build_header(&num_halfwords, STATUS_TYPE, inhibit_buf, 0);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
			"RMS: RMS build header failed for RMS inhibit message");
		} /* End if */

	/* Send message to the FAA/RMMS */
	ret = send_message(inhibit_buf,STATUS_TYPE,RMS_STANDARD);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR, "RMS: Send message inhibit message failed (ret %d).", ret);
		} /* End if */

	return (0);
} /*End rms inhibit */

