/**************************************************************************
   
   Module: rms_rec_rda_control_command.c 
   
   Description:  This module is to setup an RDA control command	
   to change rda control control.			

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: dans $
 * $Locker:  $
 * $Date: 2001/04/12 21:26:24 $
 * $Id: rms_rda_channel.c,v 1.10 2001/04/12 21:26:24 dans Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <orpgedlock.h>
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define PASSWORD_LENGTH	8
/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function determines the control and sends the RDa command

   Input: rda_channel_buf - Pointer to the message buffer

   Output:  Command to change the RDA channel control

   Returns: 0 = Successful command.

   Notes:

   **************************************************************************/
int rms_rec_rda_channel_command (UNSIGNED_BYTE *rda_channel_buf) {

	UNSIGNED_BYTE *rda_channel_buf_ptr;
	char password[8];
	int ret, i;
	short rda_control_flag;

	/* Set pointer to beginning of buffer */
	rda_channel_buf_ptr = rda_channel_buf;

	/* Place pointer past header */
	rda_channel_buf_ptr += MESSAGE_START;

	/* Get command from RMMS message */
	rda_control_flag = conv_shrt(rda_channel_buf_ptr);
	rda_channel_buf_ptr += PLUS_SHORT;

	/* Get password */
	for (i=0; i< PASSWORD_LENGTH; i++){
	        conv_char (rda_channel_buf_ptr, &password[i],PLUS_BYTE);
	        rda_channel_buf_ptr += PLUS_BYTE;
	        } /* End loop */


	/* Get lock status of RDA command LB */
	ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

	/* If the RDA command LB locked cancel command and return error code */
	 if ( ret == ORPGEDLOCK_EDIT_LOCKED){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change RDA channel (RDA commands are locked)");
		return (24);
		} /* End if */

	/* If the password is good */
	if(rms_validate_password(password) == SECURITY_LEVEL_RMS){

		/* Set the RDA to controlling */
		if (rda_control_flag == 4){
			ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_CHAN_CTL, 0, 0, 0, 0, NULL);
			if (ret < 0)
				return (ret);
			return (0);
			} /* Set to controlling channel */

		/* Set the RDA to non-controlling */
		if (rda_control_flag == 5){
			ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_CHAN_NONCTL, 0, 0, 0, 0, NULL);
			if (ret < 0)
				return (ret);
			return (0);
			} /* Set to non - controlling channel */
		}/* End if */

	else{
		/* Password is not good so cancel command and return error code */
		LE_send_msg( RMS_LE_ERROR, "RMS: Invalid RDA Channel Control Password");
		return (23);
		} /* End else */

	return (24);

} /* End rms rec rda channel command */
