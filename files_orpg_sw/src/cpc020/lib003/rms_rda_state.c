/**************************************************************************

   Module:  rms_rec_rda_state_command.c

   Description:  This module is to setup an RDA control command
   to change rda state.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2007/01/05 18:02:27 $
 * $Id: rms_rda_state.c,v 1.15 2007/01/05 18:02:27 garyg Exp $
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

#define STRING_SIZE		30
#define OVERRIDE_VALUE	20
#define PASSWORD_SIZE	8


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  Determines the state change and palces the command in the
   RDA command buffer.

   Input:  rda_change_buf - Pointer to the message buffer

   Output:  RDA state change command

   Returns: Command sent = 0

   Notes:

   **************************************************************************/
int rms_rec_rda_state_command (UNSIGNED_BYTE *rda_change_buf) {

	int spot_blanking_status;
	int aux_pwr_state;
	int ret, i;
	int this_channel_status;
	int faa_redun;
	UNSIGNED_BYTE *rda_change_buf_ptr;
	short rda_change_flag;
	short temp_short;
	int bdata_flag = 0;
	float autocal_override;
	char state_str[31];
	char password[9];
	char out_char;


	/* Set pointer to beginning of buffer */
	rda_change_buf_ptr = rda_change_buf;

	/* Place pointer past header */
	rda_change_buf_ptr += MESSAGE_START;

	/* Get command from RMMS message */
	rda_change_flag = conv_shrt(rda_change_buf_ptr);
	rda_change_buf_ptr += PLUS_SHORT;

	/*Get lock status of RDA command buffer */
	ret = rms_get_lock_status (RMS_RDA_LOCK);

	/* If the RDA command buffer locked cancel command and return error code */
	if ( ret == RMS_COMMAND_LOCKED){

		if (rda_change_flag == 1)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to set RDA to standby (RDA commands are locked)");

		if (rda_change_flag == 2)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to set RDA to restart (RDA commands are locked)");

		if (rda_change_flag == 5)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to set RDA to operate (RDA commands are locked)");

		if (rda_change_flag == 6)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to set RDA to offline operate (RDA commands are locked)");

		if (rda_change_flag == 10)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get performance data (RDA commands are locked)");

		if (rda_change_flag == 12)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change power source (RDA commands are locked)");

		if (rda_change_flag == 13)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to select remote control (RDA commands are locked)");

		if (rda_change_flag == 15)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to enable local control (RDA commands are locked)");

		if (rda_change_flag == 16)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to override Auto Calibration (RDA commands are locked)");

      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_LEGACY_CONFIG) {
   		if (rda_change_flag == 17)
	   		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to enable/disable ISU (RDA commands are locked)");
      }

		if (rda_change_flag == 18)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change base data transmission (RDA commands are locked)");

		if (rda_change_flag == 19)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to request status data (RDA commands are locked)");

		if (rda_change_flag == 20)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change Auto Calibration (RDA commands are locked)");

		if (rda_change_flag == 22)
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to change spot blanking (RDA commands are locked)");

		return (24);
		} /* End if */

	  /* Get redundant status*/
	faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

	if (ORPGSITE_error_occurred()){
		faa_redun = 0;
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get reundant type for RDA state command");
		}

	/* If the message is spot blanking or base data enable get the string that accompanies the command */
	if ((rda_change_flag == 18) || (rda_change_flag == 22)){
		for (i=0; i<=STRING_SIZE - 1; i++){
       			conv_char (rda_change_buf_ptr, &state_str[i],PLUS_BYTE);
       			rda_change_buf_ptr += PLUS_BYTE;
        		        } /* End loop */
       		} /* End if */

	/* Command RDA to standby */
	if (rda_change_flag == 1){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_STANDBY, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Command RDA to restart */
	if (rda_change_flag == 2){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_RESTART, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Command RDA to operate */
	if (rda_change_flag == 5){
	        if  (faa_redun != ORPGSITE_FAA_REDUNDANT){
                                ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_OPERATE, 0, 0, 0, 0, NULL);
                                if (ret < 0)
		        return (ret);
	                return (0);
                                }/* End if */
                        else{
	                 /* Get channel state for this channel */
	                this_channel_status =  ORPGRED_channel_state(ORPGRED_MY_CHANNEL);
                                /* This is a redundant system so check if this is the active channel */
	                if (this_channel_status == ORPGRED_CHANNEL_ACTIVE){
	                        ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_OPERATE, 0, 0, 0, 0, NULL);
	                        if (ret < 0)
		                return (ret);
	                        return (0);
                                        } /* End if */
                                else {
                                        return (26);
                                        }/* End else */
                                } /* End else */
                    } /* End if */

	/* Command RDA to offline operate */
	if (rda_change_flag == 6){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_OFFOPER, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Command RDA to send performance data */
	if (rda_change_flag == 10){
		ret = ORPGRDA_send_cmd (COM4_REQRDADATA, RMS_INITIATED_RDA_CTRL_CMD, DREQ_PERFMAINT, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Command RDA to switch power */
	if (rda_change_flag == 12){
		/* Get the current stae of aux power */
		aux_pwr_state = ORPGRDA_get_status (RS_AUX_POWER_GEN_STATE);

		if(aux_pwr_state == ORPGRDA_DATA_NOT_FOUND){
			LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get auxillary power state.");
			return (24);
			} /* End if */

		/* If generator power on set to utility power */
		if( (aux_pwr_state & 0x01)){
			ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_UTIL, 0, 0, 0, 0, NULL);
			if (ret < 0)
				return (ret);
			return (0);
			} /* End if */
		else{
			/* If utility power on set to generator power */
			ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_AUXGEN, 0, 0, 0, 0, NULL);
			if (ret < 0)
				return (ret);
			return (0);
			} /* End else */

		} /* End if */

	/* Request remote control */
	if (rda_change_flag == 13){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_REQREMOTE, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Enable local control */
	if (rda_change_flag == 15){
		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_ENALOCAL, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */


	/* Automatic calibration override */
	if (rda_change_flag == 16){

		/* Get autocal override */
		temp_short = conv_shrt(rda_change_buf_ptr);

		if((temp_short < -10) || (temp_short > 10) ){
			LE_send_msg (RMS_LE_ERROR,
	  			"RMS: Unable to get rda status for RMS status message");
	  		return(24);
	  		} /* End if */

	  	/* Convert to float */
		autocal_override = (float)(temp_short);

		/* Set autocal override for RDA command */
		autocal_override = (autocal_override * 4);

		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_AUTOCAL, autocal_override, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* ISU enable/disable */
   if (rda_change_flag == 17) {
      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
         return (19);  /* return value: Invalid Command Received */
      else 
      {
         ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_CTLISU, 0, 0, 0, 0, NULL);
            if (ret < 0)
               return (ret);
         return (0);
      }
   }

	/* Base data enable */
	if (rda_change_flag == 18){
		/* Loop to get base data enable */
		for (i =0; i<=5; i++){
			out_char = toupper(state_str[i]);
			switch (out_char) {
			        case 'R':
				bdata_flag |= RCOM_BDENABLER; /* reflectivity enabled */
				break;
			        case 'V':
				bdata_flag |= RCOM_BDENABLEV; /* velocity enabled */
				break;
			        case 'W':
				bdata_flag |= RCOM_BDENABLEW; /* width enabled */
				break;
			        case 'N':
				bdata_flag |= RCOM_BDENABLEN; /* none enabled */
				break;
			        case 'A':
				bdata_flag |= RCOM_BDENABLE; /* all enabled */
				break;
			        default:
				break;
				} /* End switch */

			} /* End loop */

		ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_BDENABLE, bdata_flag, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Request status data */
	if (rda_change_flag == 19){
		ret = ORPGRDA_send_cmd (COM4_REQRDADATA, RMS_INITIATED_RDA_CTRL_CMD, DREQ_STATUS, 0, 0, 0, 0, NULL);
		if (ret < 0)
			return (ret);
		return (0);
		} /* End if */

	/* Autocalibration */
	if (rda_change_flag == 20){
           if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_LEGACY_CONFIG) 
		ret = ORPGRDA_send_cmd( COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, 
                                        CRDA_AUTOCAL, RCOM_AUTOCAL, 0, 0, 0, NULL);
           else
		ret = ORPGRDA_send_cmd( COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, 
                                        CRDA_AUTOCAL, RCOM_AUTOCAL_ORDA, 0, 0, 0, NULL);
	   if (ret < 0)
	      return (ret);

	   return (0);
	} /* End if */

	/* Spot blanking */
	if (rda_change_flag == 22){

		/* Get password for spot blanking */
		strncpy(password, state_str, PASSWORD_SIZE);
        strncpy (&password [PASSWORD_SIZE], "\0", 1);

		/* Validate password */
		if(rms_validate_password(password) == SECURITY_LEVEL_RMS){

			/* Get current spot blanking status */
			spot_blanking_status = ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS);

			if(spot_blanking_status == ORPGRDA_DATA_NOT_FOUND){
				LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get spot blanking state.");
				return (24);
				} /* End if */

			/* Spot blanking not installed */
			if (spot_blanking_status == 0) {
				LE_send_msg( RMS_LE_ERROR, "RMS: Spot blanking not installed");
				return (24);
				} /* End if */

			/* Spot blanking on so toggle off */
			if (spot_blanking_status == 4) {
				ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_SB_ENAB, 0, 0, 0, 0, NULL);
					if (ret < 0)
						return (ret);
				return (0);
				} /* End if */

			/* Spot blanking off so toggle on */
			if (spot_blanking_status == 2) {
				ret = ORPGRDA_send_cmd (COM4_RDACOM, RMS_INITIATED_RDA_CTRL_CMD, CRDA_SB_DIS, 0, 0, 0, 0, NULL);
					if (ret < 0)
						return (ret);
				return (0);
				} /* End if */
			} /* End if */

		else{
			LE_send_msg( RMS_LE_ERROR, "RMS: Invalid Spot Blanking Password");
			return (23);
			} /* End else */
		} /* End if */

	return (24);
} /* End rms rec rda state command */
