/**************************************************************************
   
   Module:  rms_rpg_state.c
   
   Description:  This module handles messages to RMMS whenever an RPG state
   change occurs
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2005/02/09 22:43:56 $
 * $Id: rms_rpg_state.c,v 1.17 2005/02/09 22:43:56 garyg Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <orpginfo.h>
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define RPG_STATE_DATA	1

/*
* Task Level Globals
*/
extern int rms_shutdown_flag;


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function builds and sends a change of state messge
   to RMMS.  
      
   Input: 
      
   Output:  RPG state change message
   
   
   Returns: 
   
   Notes:  

   **************************************************************************/
void rms_send_rpg_state(){
	
	Mrpg_state_t  rpg_state;
	int           ret;
	UNSIGNED_BYTE rpg_state_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *rpg_state_buf_ptr;
	ushort        num_halfwords;
	short         temp_val;


	/* Set pointer to beginning of buffer */
	rpg_state_buf_ptr = rpg_state_buf;

	/* Get current state of the ORPG */
	if((ret = ORPGMGR_get_RPG_states(&rpg_state)) < 0) {
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get RPG state");
		} /* End if */

	temp_val = 0;

	if ( (rpg_state.test_mode != MRPG_TM_NONE) )
	       temp_val = 16; /*test mode*/

	else if( (rpg_state.state == MRPG_ST_TRANSITION && rpg_state.cmd == MRPG_SHUTDOWN) ||
	        (rpg_state.state == MRPG_ST_SHUTDOWN) || (rms_shutdown_flag == 1))
	       temp_val = 8; /*shutdown*/

	else if( (rpg_state.state == MRPG_ST_TRANSITION) )
	       temp_val = 1; /*restart*/

	else if( (rpg_state.state == MRPG_ST_OPERATING) )
	       temp_val = 2; /*operate*/

	else if( (rpg_state.state == MRPG_ST_STANDBY) )
	       temp_val = 4; /*standby*/

	else
	        LE_send_msg(RMS_LE_ERROR, "RMS: Unable to send RPG state message unknown state (%d)",rpg_state.state);

	/* Place pointer at past header */
	rpg_state_buf_ptr += MESSAGE_START;

	/* Place current state of the ORPG into the buffer */
	conv_ushort_unsigned(rpg_state_buf_ptr, (ushort*) &temp_val);
	rpg_state_buf_ptr += PLUS_SHORT;

	/* Add terminator to the message */
	add_terminator(rpg_state_buf_ptr);
	rpg_state_buf_ptr += PLUS_INT;

	/* Compute the number of halfwords in the message */
	num_halfwords = ((rpg_state_buf_ptr - rpg_state_buf) / 2);

	/*  Add header to the message */
	ret = build_header(&num_halfwords, RPG_STATE_DATA, rpg_state_buf, 0);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
			"RMS: RMS build header failed for RPG state");
		} /* End if */

	if ( temp_val != 8){
	        /* Send the message to the FAA/RMMS */
	        ret = send_message(rpg_state_buf,RPG_STATE_DATA,RMS_STANDARD);

	        if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
			"RMS: Send message failed (ret %d) for RPG state", ret);
		} /* End if */
	        }/* End if */

	else {
                       /* Send the message to the FAA/RMMS do not put in resend buffer because of shutdown */
	        ret = send_message(rpg_state_buf,RPG_STATE_DATA,RMS_UP);

	        if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
			"RMS: Send message failed (ret %d) for RPG state", ret);
		} /* End if */
	        }/* End else */
}/* End send rpg state */

