/**************************************************************************

   Module: rms_rec_rda_mode_command.c

   Description:  This module is to setup an RDA control command
   to change rda mode.

   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:27 $
 * $Id: rms_rda_mode.c,v 1.9 2003/06/26 14:51:27 garyg Exp $
 * $Revision: 1.9 $
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
   Description:  This function determines the mode and sends the RDa command

   Input: rda_mode_buf - Pointer to the message buffer

   Output:  RDA command to change the RDA mode

   Returns: 0 = Successful command.

   Notes:

   **************************************************************************/
int rms_rec_rda_mode_command (UNSIGNED_BYTE *rda_mode_buf) {

	UNSIGNED_BYTE *rda_mode_buf_ptr;
	int           ret;
	short         rda_mode_flag;

	/* Set pointer to beginning of buffer */
	rda_mode_buf_ptr = rda_mode_buf;

	/* Place pointer past header */
	rda_mode_buf_ptr += MESSAGE_START;

	/* Get command from RMMS message */
	rda_mode_flag = conv_shrt(rda_mode_buf_ptr);
	rda_mode_buf_ptr += PLUS_SHORT;

	/* Get lock status of the RDA command buffer */
	ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

	/* If RDA command buffer locked cancel command and return error code */
	if ( ret == ORPGEDLOCK_EDIT_LOCKED){

		if (rda_mode_flag == 2)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change RDA to maintenance mode (RDA commands are locked)");

		if (rda_mode_flag == 4)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change RDA to operational mode (RDA commands are locked)");

		return (24);
		}

	/* Command the RDA to Maintenance mode */
	if (rda_mode_flag == 2){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_MODE_MNT, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* maintenance mode */

	/* Command the RDA to Operational mode */
	if (rda_mode_flag == 4){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_MODE_OP, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* operational mode */

	return (24);

} /* End rms rec rda mode command */
