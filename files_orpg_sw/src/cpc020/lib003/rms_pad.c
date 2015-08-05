/**************************************************************************

   Module: rms_pad.c

   Description:  This is the module for padding messages to desired length.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: dans $
 * $Locker:  $
 * $Date: 2001/04/12 19:33:53 $
 * $Id: rms_pad.c,v 1.3 2001/04/12 19:33:53 dans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  Pads the message to the desired length.


   Input: buf_ptr - Pointer to the buffer to be padded.
   	  message_size - Size of the message being padded.
   	  final_length - The ending length of the message.

   Output:


   Returns:

   Notes: Used because some of the message to RMMS must be padded to 26
   halfwords.

   **************************************************************************/

void pad_message(UNSIGNED_BYTE *buf_ptr, int message_size, int final_length) {

	int i;
	const UNSIGNED_BYTE pad = 0;

	/* Put nulls into messsage to pad it to size required by RMMS */
	for (i = message_size; i<=final_length; i++){

		/* Place a zero in the buffer to pad the message */
		memcpy (buf_ptr, &pad, 1);

		/* Increment the pointer by one byte */
		buf_ptr += 1;

		}/* End loop */

	}/* End pad_message */
