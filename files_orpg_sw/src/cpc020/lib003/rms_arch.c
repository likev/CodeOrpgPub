/**************************************************************************

   Module:  rms_rec_arch_command.c

   Description:  This module is to setup an RDA control command
   to start or stop archive II recording.

   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:50:54 $
 * $Id: rms_arch.c,v 1.10 2003/06/26 14:50:54 garyg Exp $
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


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This routine reads the message and send the command to
   start or stop Archive II recording.

   Input: rda_arch_buf - Pointer to message buffer.

   Output: RDA command for Archive II.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
int rms_rec_arch_command (UNSIGNED_BYTE *rda_arch_buf) {

      /* Buffer pointer for RMS message */
	UNSIGNED_BYTE *rda_arch_buf_ptr;
	int           ret;
	int           rda_arch_flag;
	int           vol_scans;

                /* Set pointer to start of buffer */
	rda_arch_buf_ptr = rda_arch_buf;

                /* Place pointer past message header */
	rda_arch_buf_ptr += MESSAGE_START;

	/* Convert archive flag */
	rda_arch_flag = conv_shrt(rda_arch_buf_ptr);
	rda_arch_buf_ptr += PLUS_SHORT;

	/* Convert number of volume scans */
	vol_scans = conv_shrt(rda_arch_buf_ptr);
	rda_arch_buf_ptr += PLUS_SHORT;

                /* Get lock status of RDA command buffer */
	ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

                /* If RDA command buffer locked return error */
	if ( ret == ORPGEDLOCK_EDIT_LOCKED){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to access archive RDA commands are locked");
		return (24);
		}

                /* Validation check for number of volume scans */
                if (vol_scans < 0){
		LE_send_msg(RMS_LE_ERROR,"RMS: Number of volume scans less than 0 (#scans %d)", vol_scans);
		return (24);
		}

	if ( vol_scans > 255){
		LE_send_msg(RMS_LE_ERROR,"RMS: Number of volume scans greater than 255 (#scans %d)", vol_scans);
		return (24);
		}

                /* FAA/RMMS command to start Archive II recording */
	if (rda_arch_flag == 4){

                                /* If vol_scans equal zero
	                     Archive II will do continuous recording */
 		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_ARCH_COLLECT,
		                                	vol_scans, 0, 0, 0, NULL);

		if (ret < 0)
			return (ret);

		return (0);
		} /* start archive II */

                /* FAA/RMMS command to stop Archive II recording */
	if (rda_arch_flag == 6){
		ret =  ORPGRDA_send_cmd(COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_ARCH_STOP, 0,
					0, 0, 0, NULL);
		if (ret < 0)
			return (ret);

		return (0);
		} /* stop archive II */

                /* If FAA/RMMS command not valid return error code */
	return (24);

} /* End rms rec arch command */
