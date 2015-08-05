/**************************************************************************
   
   Module: rms_force_adaptation_msg.c 
   
   Description:  This module is to set up an RDA control command	
   to start an adaptation update.	
 
   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: dans $
 * $Locker:  $
 * $Date: 2001/04/10 16:21:22 $
 * $Id: rms_force_adaptation_msg.c,v 1.14 2001/04/10 16:21:22 dans Exp $
 * $Revision: 1.14 $
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
   Description:  This function reads the input message and forces an adaptation
   update.  Adaptation data from the active ORPG is forced to the FAA redundant
   ORPG.
      
   Input: adaptation_buf - Pointer to the message buffer.
      
   Output:  Force adaptation command.
      
   Returns: 0 = Successful command.
   
   Notes:  

   **************************************************************************/
int rms_rec_adaptation_command (UNSIGNED_BYTE *adaptation_buf) {

	UNSIGNED_BYTE *adaptation_buf_ptr;
	
	short adaptation_flag;

	int channel_status, faa_redun;

	Redundant_cmd_t adapt_cmd;

	/* Set pointer to beginning of buffer */
	adaptation_buf_ptr = adaptation_buf;

	/* Set pointer past message header */
	adaptation_buf_ptr += MESSAGE_START;

	/* Get adaptation command */
	adaptation_flag = conv_shrt(adaptation_buf_ptr);
	adaptation_buf_ptr += PLUS_INT;
	
	/* Get redundant type */
	faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);
	
	if (ORPGSITE_error_occurred()){
		faa_redun = 0;
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get reundant type for force adaptation");
		}

                /* If this is not an FAA site cancel force adaptation and return error code */
	if(faa_redun != ORPGSITE_FAA_REDUNDANT){
		LE_send_msg(RMS_LE_ERROR,"RMS: Non redundant configuration force adaptation command rejected");
		return (28);
		}

	/* Get the channel state for the other channel */
	channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

	if(channel_status == ORPGRED_ERROR){
		LE_send_msg (RMS_LE_ERROR,"RMS: Unable to get channel status for RMS force adaptation command");
		return (24);
		}

	/* If this is a force adaptation command */
	if (adaptation_flag == 1){

	                /* If the other channel is active then cancel force adaptation and return error code */
		if(channel_status == ORPGRED_CHANNEL_ACTIVE){
			LE_send_msg (RMS_LE_ERROR, 
	  		"RMS: Update adaptation rejected other channel is active");
	  		return (26);
			}

		/* Set up command to update all messages to the other channel */
		adapt_cmd.cmd = ORPGRED_UPDATE_ALL_MESSAGES;

		/* Send command to update all messages to the other channel */
		if(ORPGRED_send_msg (adapt_cmd)< 0){
			LE_send_msg (RMS_LE_ERROR, 
				"RMS: Unable to update adaptation information");
			return (-1);
			}
			
		return (0);
		} /* End if */
	
	return (24);
} /* End rms rec adaptation command */
