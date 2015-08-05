/**************************************************************************

   Module: rms_rec_wb_command.c

   Description:  This module is to setup an RDA control command
   to enable or disable the wideband.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/07 19:06:45 $
 * $Id: rms_wb.c,v 1.13 2007/03/07 19:06:45 steves Exp $
 * $Revision: 1.13 $
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


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function sends a command to enable or disable the
   wideband connection.

   Input: rda_wb_buf - Pointer to the buffer containing the message

   Output: None

   Returns: Command sent = 0

   Notes:

   **************************************************************************/

int rms_rec_wb_command (UNSIGNED_BYTE *rda_wb_buf) {

	UNSIGNED_BYTE *rda_wb_buf_ptr;
	int           wb_status;
	int           ret;
	int           rda_wb_flag;


	/* Set pointer to beginning of beffer */
	rda_wb_buf_ptr = rda_wb_buf;

	/* Place pointer past header */
	rda_wb_buf_ptr += MESSAGE_START;

	/* Get connect/disconnect command number */
	rda_wb_flag = conv_shrt(rda_wb_buf_ptr);
	rda_wb_buf_ptr += PLUS_SHORT;

	/* Get the lock status of the RDA command LB */
	ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

	/* If LB locked cancel command and return error code */
	if ( ret == ORPGEDLOCK_EDIT_LOCKED){

		if (rda_wb_flag == 1)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to disconnect wide band (RDA commands are locked)");

		if (rda_wb_flag == 2)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to connect wide band (RDA commands are locked)");

		return (24);
		} /* End if */

	/* Retrieve wideband status */
	wb_status = ORPGRDA_get_wb_status(ORPGRDA_WBLNSTAT);

	/* if unable to get wideband status cancel edit and return error code */
	if(wb_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg (RMS_LE_ERROR,
	  		"RMS: Unable to get wb status for RMS wideband message");
			return (24);
	  	} /* End if */

                /* If disconnect command */
	if (rda_wb_flag == 1){

                	/* If any of these conditions exist cancel command and return error code */
	                if((wb_status == RS_DISCONNECT_PENDING) ||
		        (wb_status == RS_DISCONNECTED_HCI) ||
		        (wb_status == RS_DISCONNECTED_RMS) ||
		        (wb_status == RS_DISCONNECTED_CM) ){
                   	        LE_send_msg (RMS_LE_ERROR,"RMS: Wideband line already disconnected");
		        return (6);
		        } /*End if */

		/* Send command to disconnect the wideband */
		ret = ORPGRDA_send_cmd (COM4_WBDISABLE, RMS_INITIATED_RDA_CTRL_CMD, 0, 0, 0, 0, 0, NULL);

		if (ret < 0 )
			return (ret);

                        	return (0);

                	} /* End if */

                 /* Connect command */
	else if (rda_wb_flag == 2){

                        	/* Connected cancel command and return error code */
	                if(wb_status == RS_CONNECTED){
                		LE_send_msg (RMS_LE_ERROR,"RMS: Wideband line already connected");
			return (5);
			} /* End if */

                        	/* Connect pending cancel command and return error code */
	                if(wb_status == RS_CONNECT_PENDING){
                                	LE_send_msg (RMS_LE_ERROR,"RMS: Wideband line connect pending");
		   	return (4);
		  	} /* End if */

		/* Send command to disconnect wideband */
		ret = ORPGRDA_send_cmd (COM4_WBENABLE, RMS_INITIATED_RDA_CTRL_CMD, 0, 0, 0, 0, 0, NULL);

                	if (ret < 0)
			return (ret);

		return (0);
                	} /* End else if */
                 else{
		LE_send_msg(RMS_LE_ERROR,"RMS: Invalid wideband command (%d)", rda_wb_flag);
		return (24);
		} /* End else */
} /* End rms rec wb command */
