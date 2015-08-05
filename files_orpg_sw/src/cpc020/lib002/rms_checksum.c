/**************************************************************************
   
   Module:  rms_checksum.c	
   
   Description:  This module verifies messages for rms.
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: dans $
 * $Locker:  $
 * $Date: 2001/04/03 20:25:30 $
 * $Id: rms_checksum.c,v 1.9 2001/04/03 20:25:30 dans Exp $
 * $Revision: 1.9 $
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
   Description:  This function verifies the password.
      
   Input: RMS message buffer
      
   Output: None
      
   Returns: Computed Checksum
   
   Notes:  

   **************************************************************************/
   
int rms_checksum (UNSIGNED_BYTE *cksum_buf_ptr) {

	int sum,i;
	short temp_short;	/* Temporary holder for converted checksum numbers */
	short msg_size;	/* Message size */
	
	UNSIGNED_BYTE *checksum_buf_ptr;	/* Pointer to a buffer of UNSIGNED_BYTES */

                /* Set pointer to the start of the buffer */
        	checksum_buf_ptr = cksum_buf_ptr;

                /* Check to see if the pointer is NULL */
	if(checksum_buf_ptr == NULL){
		LE_send_msg(RMS_LE_ERROR,"RMS: Null pointer passed to checksum.");
		return (0);
		}

                /* Get the message size located in the first halfword of the message*/
                msg_size = conv_shrt(checksum_buf_ptr);

	/* If message exceeds max buffer size print error and return */
        	if(msg_size > MAX_BUF_SIZE){
		LE_send_msg(RMS_LE_ERROR,"RMS: Checksum failed message exceeds maximum buffer size (size = %d).",msg_size);
		return (0);
		}

	sum = 0;

        	/* Compute checksum for the message */
	for (i=0; i<= msg_size - 1; i++){
        		/* Don't add the FAA/RMMS checksum located in halfwords 9 & 10 */
                        	if ( i == 9 || i == 10){
			temp_short = 0;
			}
		else{
			temp_short = conv_shrt(checksum_buf_ptr);
			}

		/* Bump the buffer pointer by one halfword */
		checksum_buf_ptr += PLUS_SHORT;

		/* Add the halfword to the checksum */
		sum += temp_short;

		}/* End loop */

                /* Return the checksum */
	return sum;

} /*End rms checksum */

