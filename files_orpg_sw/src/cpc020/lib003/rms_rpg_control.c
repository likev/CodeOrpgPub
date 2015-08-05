/**************************************************************************
   
   Module:  rms_rec_rpg_control_command.c
   
   Description:  This module is to setup an RPG control command	
   to change rpg state.			
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/08/07 21:19:11 $
 * $Id: rms_rpg_control.c,v 1.15 2003/08/07 21:19:11 garyg Exp $
 * $Revision: 1.15 $
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
   Description:  Determines the state change and places the command in the
   RPG command buffer.

   Input:  rpg_change_buf - Pointer to the message buffer

   Output:  RPG state change command

   Returns: Command sent = 0

   Notes:

   **************************************************************************/
int rms_rec_rpg_control_command (UNSIGNED_BYTE *rpg_change_buf) {

	UNSIGNED_BYTE *rpg_change_buf_ptr;

	Mrpg_state_t rpg_state;

	int vcp_num;
	int control_status;
	int op_mode;
	int ret;

	short rpg_change_flag;

	/* Set pointer to beginning of buffer */
	rpg_change_buf_ptr = rpg_change_buf;

	/* Place pointer past header */
	rpg_change_buf_ptr += MESSAGE_START;

	/* Get command from RMMS message */
	rpg_change_flag = conv_shrt(rpg_change_buf_ptr);
	rpg_change_buf_ptr += PLUS_SHORT;

	/* Check to see if ORPG command buffer is locked */
	ret = rms_get_lock_status (RMS_RPG_LOCK);

	/* If ORPG command buffer is locked cancel command and return error code */
	if ( ret == RMS_COMMAND_LOCKED){

		if (rpg_change_flag == 1)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to set RPG to operational mode (RPG commands are locked)");

		if (rpg_change_flag == 2)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to set RPG to test mode (RPG commands are locked)");

		if (rpg_change_flag == 4)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to restart the RPG (RPG commands are locked)");

		if (rpg_change_flag == 5)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to shutdown to standby (RPG commands are locked)");

		if (rpg_change_flag == 6)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to shutdown to off (RPG commands are locked)");

		return (24);
		} /* End if */

	/* Retrieve RPG status */
	ret = ORPGMGR_get_RPG_states (&rpg_state);

	if(ret != 0){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get RPG state.");
		return (24);
		} /* End if */

	/* Command ORPG to operational mode */
	if (rpg_change_flag == 1){

		/* If  ORPG in standy mode cancel command and return error code */
		if(rpg_state.state == MRPG_ST_STANDBY){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to accept command operate mode RPG in standby mode");
			return (24);
			} /* End if */

		/* If  ORPG already in operational mode cancel command and return error code */
		if(rpg_state.test_mode == MRPG_TM_NONE){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG already in operational mode");
			return (14);
			} /* End if */

		/* Get the current VCP number */
		vcp_num = ORPGRDA_get_status(RS_VCP_NUMBER);

		if (vcp_num == ORPGRDA_DATA_NOT_FOUND){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to get vcp number for rpg control");
			return (24);
			} /* End if */

		/* If maintenance VCP cancel command and return error code */
		if (vcp_num > 255){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to accept command operate mode RDA in maintenance mode");
			return (20);
			} /* End if */

		/* Get current ORPG control status */
		control_status = ORPGRDA_get_status(RS_CONTROL_STATUS);

		if (control_status == ORPGRDA_DATA_NOT_FOUND){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to get rda control status for rpg control");
			return (24);
			} /* End if */

		/* Get current ORPG operational mode */
		op_mode = ORPGRDA_get_status(RS_OPERATIONAL_MODE);

		if (op_mode == ORPGRDA_DATA_NOT_FOUND){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to get rda operational mode for rpg control");
			return (24);
			} /* End if */

		/* If  ORPG in local control and maintenance mode cancel command and return error code */
		if((control_status & CS_LOCAL_ONLY) &&
		   (op_mode & OP_MAINTENANCE_MODE)){
		   	LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to accept command operate mode maintenance VCP commanded");
		   	return (18);
		   	} /* End if */

		/* Send command to set ORPG to operational mode */
		if( (ret = ORPGMGR_send_command(MRPG_EXIT_TM)) != 0){
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to accept command operate mode for RPG (ret %d)", ret);
			return (24);
			} /* End if */

		return (0);

		} /* End if */

	/* Command ORPG to test mode */
	if (rpg_change_flag == 2){

		/* If  ORPG in standy mode cancel command and return error code */
		if(rpg_state.state == MRPG_ST_STANDBY){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG unable to accept command test mode RPG in standby mode");
			return (24);
			} /* End if */

		/* If ORPG already in test mode cancel command and return error code */
		if(rpg_state.test_mode == MRPG_TM_RPG){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG already in test mode");
			return (15);
			} /* End if */

		/* If RDA in test mode cancel command and return error code */
		if(rpg_state.test_mode == MRPG_TM_RDA){
			LE_send_msg(RMS_LE_ERROR,"RMS: RDA in test mode");
			return (15);
			} /* End if */

		/* Send command to set ORPG to test mode */
		if( (ret = ORPGMGR_send_command(MRPG_ENTER_TM)) != 0){
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to accept command test mode for RPG (ret %d)", ret);
			return (24);
			} /* End if */

		return (0);

		}  /* End if */

	/* Command ORPG to restart mode */
	if (rpg_change_flag == 4) {

      Mrpg_state_t mrpg;

      ret = ORPGMGR_get_RPG_states (&mrpg);

      if (ret) {
          LE_send_msg (RMS_LE_ERROR, "ORPGMGR_get_RPG_states() failed");
          return (24);

      }

      LE_send_msg (RMS_LE_LOG_MSG, "RMMS: RPG Restart commanded\n");

      if (mrpg.state == MRPG_ST_STANDBY) {

         if ((ret = ORPGMGR_send_command (MRPG_RESTART)) != 0)
             LE_send_msg (RMS_LE_ERROR,
                 "Error sending RPG \"MRPG_RESTART\" command: %d", ret);

      } else {

            if ((ret = ORPGMGR_send_command (MRPG_STANDBY_RESTART)) != 0)
                 LE_send_msg (RMS_LE_ERROR,
                   "Error sending RPG \"MRPG_STANDBY_RESTART\" command: %d", ret);
      }

		if (ret == 0) {
         LE_send_msg (RMS_LE_LOG_MSG, "mrpg \"RESTART\" command sent");
         return (0);
      }
      else
			return (24);

   }  /* End if */

	/* Command ORPG to shutdown to standby mode */
	if (rpg_change_flag == 5){

		/* If  ORPG already in standy mode cancel command and return error code */
		if(rpg_state.state == MRPG_ST_STANDBY){
			LE_send_msg(RMS_LE_ERROR,"RMS: RPG already in standby mode");
			return (24);
			} /* End if */

		/* Send command to set ORPG to standby mode */
		if( (ret = ORPGMGR_send_command(MRPG_STANDBY)) != 0){
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to accept command standby for RPG (ret %d)", ret);
			return (24);
			} /* End if */

		return (0);

		} /* End if */

	/* Command ORPG to shutdown to off mode */
	if (rpg_change_flag == 6){

		/* Send command to set ORPG to off mode */
		if( (ret =  ORPGMGR_send_command(MRPG_SHUTDOWN)) != 0){
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to accept command halt for RPG (ret %d)", ret);
			return (24);
			} /* End if */

		return (0);

		} /* End if */

	return (24);
} /* End rms rec rpg control command */
