/**************************************************************************
   
   Module:  rms_loopback_msg.c
   
   Description:  This module is to set up an RDA control command	
   start a loopback message.		
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/09 21:58:57 $
 * $Id: rms_loopback_msg.c,v 1.15 2005/08/09 21:58:57 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <orpgedlock.h>
#include <orpgrda.h>
#include <orpgadpt.h>
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
   Description:  This function reads the message buffer and sends a loopback
   command to RDA.
      
   Input: loopback_buf - Pointer to the message buffer. 
      
   Output: Loopack command sent to the RDA.

   Returns:  0 = Successful command.

   Notes:

   **************************************************************************/
int rms_rec_loopback_command (UNSIGNED_BYTE *loopback_buf) {

	UNSIGNED_BYTE *loopback_buf_ptr;
	int wb_status;
	int loopback_timeout, ret;
	char tbuf [16];
	double value;

	/* Set pointer to beginning of buffer */
	loopback_buf_ptr = loopback_buf;

	/* Place pointer past header */
	loopback_buf_ptr += MESSAGE_START;

	/* Get test period */
	loopback_timeout = conv_intgr(loopback_buf_ptr);

	/* Retrieve RDA status */
	wb_status = ORPGRDA_get_wb_status(ORPGRDA_WBLNSTAT);

  	if(wb_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg (RMS_LE_ERROR,
	  		"RMS: Unable to get wb status for RMS wideband loopback message");
			return (24);
	  	}

	/* Get lock status of RDA command buffer */
	ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

	/* If RDA command buffer locked cancel loopback command and return error code */
	if ( ret == ORPGEDLOCK_EDIT_LOCKED){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to perform loopback test (RDA commands are locked)");
		return (24);
		}

	/* If the loopback message timeout not zero then start the timeout loopback */
	if (loopback_timeout != 0 ){

		/* Validate time out time */
		if((loopback_timeout < 60000) || (loopback_timeout > 300000)){
			LE_send_msg(RMS_LE_ERROR,"RMS: Loopback timeout must be between 60,000 and 300,000 (timeout = %d)",loopback_timeout);
			return (1);
			}

		/* Change the timeout time from milliseconds to seconds */
		loopback_timeout /= 1000;

		/* If the wideband not connected return error code and cancel loopback command */
		if(wb_status != RS_CONNECTED){
			return(21);
			}

		/* Convert the timeout time to ASCII */
		sprintf(tbuf,"%d",loopback_timeout);

		/* Change the loopback rate */
                value = (double) loopback_timeout;
		if( DEAU_set_values( "rda_control_adapt.loopback_rate", 0, &value, 1, 0) < 0 ) {
		 	LE_send_msg (RMS_LE_ERROR, "RMS: Unable to set Loopback Rate properties");
			return (24);
			}

		/* Clear the loopback disabled flag */
		value = 0.0;
	   	if( DEAU_set_values( "rda_control_adapt.loopback_disabled", 0, &value, 1, 0) < 0 ) {
			LE_send_msg (RMS_LE_ERROR, "RMS: Unable to enable Wideband loopback rate");
			return (24);
			}

		/* Send the command to RDA */
		ret = ORPGRDA_send_cmd (COM4_LBTEST, RMS_INITIATED_RDA_CTRL_CMD, 0, 0, 0, 0, 0, NULL);

		if (ret < 0)
			return (ret);

		return (0);
		}

	else {
	                /* Set the loopback disabled flag */
                value = 1.0;
	   	if( DEAU_set_values( "rda_control_adapt.loopback_disabled", 0, &value, 1, 0 ) < 0 ) {
			LE_send_msg (RMS_LE_ERROR, "RMS: Unable to disable Wideband loopback rate");
			return (24);
	    		}

		return (0);
		 }
		 
		
} /* End rms rec loopback command */
