/**************************************************************************
   
   Module: rms_send_status.c 
   
   Description:  This is the module for sending status messages. Status
   message are sent to RMMs every 60 seconds.
   

   Assumptions:  RMMs interface is up.
      
   **************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 17:34:29 $
 * $Id: rms_send_status.c,v 1.34 2014/10/03 17:34:29 steves Exp $
 * $Revision: 1.34 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <rda_status.h>
#include <gen_stat_msg.h>
#include <itc.h>
#include <orpgvst.h>
#include <basedata.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define STATUS_TYPE             2
/* #define	SIZEOF_BASEDATA      4720 */
#define	SIZEOF_BASEDATA        MAX_GENERIC_BASEDATA_SIZE * 2
#define BASEDATA               55

/*
* Static Globals
*/

/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function builds and send the ORPG status message to
   the RMMS.  It is set on a timer and sends the message every 60 seconds.
      
   Input: 
      
   Output:  ORPG status message.
   
   Returns: 
   
   Notes:  

   **************************************************************************/

void send_status_msg() {

   Base_data_header *base_data;
   unsigned int     rpg_status;
   Nb_status        nb_stat[MAX_FAA_LINES];
   Nb_status        *nb_ptr;
   Vol_stat_gsm_t   vol;
   unsigned char    prf_flag;
   unsigned long    temp_long;
   int              ret, i;
   int              n_lines;
   int              basedata_lbd;
   int              loadshed_value;
   int              rda_status;
   int              wb_status;
   int              rpg_control_state;
   int              comms_relay_status;
   ushort           highest_line;
   ushort           temp_val;
   char             basedata_buf[SIZEOF_BASEDATA];
   UNSIGNED_BYTE    status_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE    *status_buf_ptr;
   ushort           num_halfwords;
   ushort           wb1_logical;
   ushort           wb2_logical;
   int              wb_primary = 1;
   int              wb_secondary = 0;
   short            highest_util = 0;
   ushort           blank = 0;
   float            temp_flt = 0;

	/* Set pointer to beginning of buffer */
	status_buf_ptr = status_buf;

	/* Set pointer to start of array */
	nb_ptr = nb_stat;

	/* Place pointer at begining of message*/
	status_buf_ptr += MESSAGE_START;

	/* Retrieve RPG status */
	if((ret = ORPGINFO_statefl_get_rpgopst(&rpg_status)) < 0) {
	        LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get RPG operability status");
	        } /* End if */

	/* Retrieve Volume status */
	ret = ORPGDA_read (ORPGDAT_GSM_DATA, (char *)&vol,
			sizeof (Vol_stat_gsm_t), VOL_STAT_GSM_ID);

	if (ret != sizeof (Vol_stat_gsm_t)) {
		LE_send_msg (RMS_LE_ERROR,
	  "RMS: ORPGDA_read failed (VOLUME_DATA) in status message (ret %d)",ret);
                        } /* End if */

	/*Retrieve Narrowband status and number of narrowband lines */
	n_lines = rms_get_nb_status(nb_ptr);

	if (n_lines <= 0){
		LE_send_msg (RMS_LE_ERROR, "RMS: RMS get narrowband status failed");
		} /* End if */


	/* Retrieve PRF status */
	prf_flag = ORPGINFO_is_prf_select();

/* according to the man page, only 0 or 1 can be returned from function
   ORPGINFO_is_prf_select(). as a result, this code has been commented
   out until it is verified the man page is correct .
	if (prf_flag < 0 || prf_flag >1) {
		LE_send_msg (RMS_LE_ERROR,
			"RMS: ORPGINFO_is_prf_select failed in status message (ret %d)",ret);
		prf_flag = 0;
                        } */

                /* Retrieve base data information */

                /* Get the LB id of the basedata LB */
	basedata_lbd = ORPGDA_lbfd (BASEDATA);

                /* Seek the latest basedata entry in the LB */
	ret = LB_seek(basedata_lbd,0, LB_LATEST, NULL);

                /* Read the latest basedata information */
	ret = ORPGDA_read (BASEDATA, basedata_buf, SIZEOF_BASEDATA, LB_ANY);

    if (ret < 0) {
        if (ret != LB_TO_COME)
           LE_send_msg (GL_ERROR,
              "ORPGDA_read base data failed in status message (ret %d)",ret);
        base_data = NULL;
    }
    else /* Set the basedata pointer equal to the buffer */
      base_data = (Base_data_header *)basedata_buf;

	/*ORPG equipment operability status*/
	temp_val = 0;
	/* Caste the value to an unsigned short */
	temp_val = (ushort)rpg_status;

	/* Place ORPG operability in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/*Place a Spare ( HALFWORD ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/*ORPG channel control state*/
	rpg_control_state = ORPGRDA_get_status (RS_CHAN_CONTROL_STATUS);

	if(rpg_control_state == RDA_IS_CONTROLLING){
		temp_val = 1;
		} /* End if */
	else if (rpg_control_state == RDA_IS_NON_CONTROLLING){
		temp_val = 2;
		} /* End else if */
	else {
		temp_val = 0;
		} /* End else */

	/* Place ORPG channel control state in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get RDA channel control status*/
	rda_status = ORPGRDA_get_status (RS_CHAN_CONTROL_STATUS);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get channel control status for RMS status message.");
		/* Control status unknown */
		rda_status = 2;
		} /* End if */

	temp_val = (ushort)rda_status;

	/* Place RDA channel control status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/*Place a Spare ( HALFWORD ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	temp_val = 0;

	/*Get Narrowband/Wideband relay status*/
	comms_relay_status = ORPGRED_comms_relay_state (ORPGRED_MY_CHANNEL);

	if(comms_relay_status == ORPGRED_COMMS_RELAY_UNASSIGNED){
		                /* Comms unassigned */
			temp_val = 2;
			} /* End if */
	else if(comms_relay_status == ORPGRED_COMMS_RELAY_ASSIGNED){
		                /* Comms assigned */
			temp_val = 1;
			} /* End else if */
	else if (comms_relay_status == ORPGRED_COMMS_RELAY_UNKNOWN){
		                /* Comms unknown */
			temp_val = 3;
			} /* End else */

	/* Place comms relay status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/*Auto PRF enabled, disabled*/
	temp_val = 0;
	/* Caste the value to an unsigned short */
	temp_val = (ushort)prf_flag;

	/* Place auto PRF in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/*Weather mode*/
                if (ORPGVST_get_mode() == ORPGVST_CLEAR_AIR_MODE){
	                /* Clear air */
	                temp_val = 2;
		}/* End if */
                else if (ORPGVST_get_mode() == ORPGVST_PRECIPITATION_MODE){
	                /* Precip/Severe weather */
	                temp_val = 1;
		} /* End else if */

	else if (ORPGVST_get_mode() == ORPGVST_MAINTENANCE_MODE){
		/* Maintenance */
		temp_val = 0;
		}/* End else */

	/* Place weather mode in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Place a Spare ( HALFWORD ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/*Current ORPG volume coverage pattern*/
	temp_val = 0;
	/* Caste the value to an unsigned short */
	temp_val = (ushort)vol.vol_cov_patt;

	/* Place ORPG volume coverage pattern in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Place ORPG vcp start time in the message */
	temp_long = ORPGVST_get_volume_time();
	conv_int_unsigned(status_buf_ptr, (int*)&temp_long);
	status_buf_ptr += PLUS_INT;

	/*Place the current ORPG elevation cut in the message */
    if (base_data == NULL)
       temp_flt = 0.0;
    else
       temp_flt = (float)base_data->elevation;
	conv_real_unsigned(status_buf_ptr, &temp_flt);
	status_buf_ptr += PLUS_INT;

	/* Place a Spare ( HALFWORD ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/* CPU utilization is no longer supported.   Set level to 0. */
	temp_val = 0;

	/* Place the current ORPG cpu utilization in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Memory utilization is no longer supported.   Set level to 0. */
	temp_val = 0;

	/* Place the memory utilization in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get current ORPG on-line storage utilization level*/
	ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_PROD_STORAGE, LOAD_SHED_CURRENT_VALUE,&loadshed_value);
	temp_val = 0;
	/* Caste the value to an unsigned short */
	temp_val = (ushort)loadshed_value;

	/* Place the current ORPG on-line storage utilization in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get the current ORPG input buffer utilization level*/
	ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_INPUT_BUF, LOAD_SHED_CURRENT_VALUE,&loadshed_value);
	temp_val = 0;
	/* Caste the value to an unsigned short */
	temp_val = (ushort)loadshed_value;

	/* Place the current ORPG input buffer utilization in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Place a Spare ( HALFWORD ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	
	/*Current ORPG narrowband line utilization level for NB#1 - 32*/
	for (i=0; i<=MAX_FAA_LINES - 1; i++){

		/* Sort for the highets utilization level */
		if(nb_stat[i].nb_utilization > highest_util){
			highest_line = (ushort)nb_stat[i].line_num;
			highest_util = nb_stat[i].nb_utilization;
			} /* End if */

		/* Place the ORPG narerowband line utilization in the message */
		conv_ushort_unsigned(status_buf_ptr, (ushort*)&nb_stat[i].nb_utilization);
		status_buf_ptr += PLUS_SHORT;
		} /* End for loop */

	/*Place Spares ( 4 HALFWORDS ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/*Place the narrowband line with the highest utilization percentage in the message */
	conv_ushort_unsigned(status_buf_ptr, &highest_line);
	status_buf_ptr += PLUS_SHORT;

	/* Get wideband communication status for WB#1*/
	wb_status = ORPGRDA_get_wb_status(ORPGRDA_WBLNSTAT);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get WB #1 status for RMS status message.");
		/* Not implemented */
		wb_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)wb_status;
	/* Subtract one to emulate the FAA/RMMS values */
	if(temp_val >= 5)
		temp_val --;

	/* Place wideband communications WB#1 status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/*Wideband communication status for WB#2 (not implemented in ORPG)*/
	/* Hardcoded not implemented */
	ret = 0;
	/* Place wideband communications WB#2 status in the message */
	conv_ushort_unsigned(status_buf_ptr, (ushort*)&ret);
	status_buf_ptr += PLUS_SHORT;

	/* Place Spares ( 2 HALFWORDS ) in the message*/
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/* Set wideband logical status for WB#1*/
	if(wb_primary ==1){
		/* Primary */
		wb1_logical = 1;
		} /* End if */
	else if (wb_secondary == 1){
		/* Secondary */
		wb1_logical = 2;
		} /* End else if */
	else{
		/* Unused */
		wb1_logical = 0;
		} /* End else */

	/* Place wideband logical status WB#1 in message */
	conv_ushort_unsigned(status_buf_ptr, &wb1_logical);
	status_buf_ptr += PLUS_SHORT;

	/*Wideband logical status for WB#2 - Hardcoded unused*/
	if(wb_primary == 2){
	                /* Primary */
	                wb2_logical = 1;
		} /* End if */
	else if (wb_secondary == 2){
		/* Secondary */
		wb2_logical = 2;
		} /* End else if */
	else{
		/* Unused */
		wb2_logical = 0;
		} /* End else */
	
	/* Place wideband logical status WB#2 in message */
	conv_ushort_unsigned(status_buf_ptr, &wb2_logical);
	status_buf_ptr += PLUS_SHORT;
	
	/* Place Spares ( 2 HALFWORDS) in message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/*Narrowband status for NB#1 - 32*/
	for (i=0; i<=(MAX_FAA_LINES -1); i++){
		if (i <= n_lines -1){
			/* Place narrowband status in the message */
			conv_ushort_unsigned(status_buf_ptr,(ushort*)&nb_stat[i].nb_status);
			status_buf_ptr += PLUS_SHORT;
			} /* End if */
		else {
			/* if the number of lines is less than the number of max faa
			    lines then fill with zeros */
			conv_ushort_unsigned(status_buf_ptr, &blank);
			status_buf_ptr += PLUS_SHORT;
			} /* End else */
		} /* End for loop */

	/* Place Spares ( 4 HALFWORDS ) in message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;

	/*Rda status*/
	rda_status = 0;
	temp_val = 0;

	/* Get the RDA status */
	rda_status = ORPGRDA_get_status (RS_RDA_STATUS);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get RDA status for RMS status message.");
		/* Indeterminant */
		rda_status = 0;
		} /* End if */

	if(wb_status == 3 || wb_status == 4)
		 /*Wide band disconnect RDA status indeterminant*/
		 rda_status = 0;

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place RDA status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* RDA operability status*/
	rda_status = 0;
	temp_val = 0;

	/* Get RDA operability status */
	rda_status = ORPGRDA_get_status (RS_OPERABILITY_STATUS);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get operability status for RMS status message.");
		/* Indeterminant */
		rda_status = 0;
		} /* End if */

	if(wb_status == 3 || wb_status == 4)
		/* RDA operability status wideband disconnect*/
		rda_status = 128;

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place RDA operability status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get RDA control status */
	rda_status = ORPGRDA_get_status (RS_CONTROL_STATUS);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get RDA control status for RMS status message.");
		/* Unknown */
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place RDA control status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get auxillary power generator state */
	rda_status = ORPGRDA_get_status (RS_AUX_POWER_GEN_STATE);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get auxillary power state for RMS status message.");
		/* Unknown */
		rda_status = 0;
		} /* End if */

	temp_val = 0;

	temp_val = (ushort)rda_status;
	/* Place Auxillary power generator state in message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get average transmitter power (watts) */
	rda_status = ORPGRDA_get_status (RS_AVE_TRANS_POWER);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get transmitter power status for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place average transmitter power in message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get reflectivity calibration correction */
	rda_status = ORPGRDA_get_status (RS_REFL_CALIB_CORRECTION);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get reflectivity calibration correction for RMS status message.");
		rda_status = 0;
		} /* End if */

	if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG){

	    /* The value is stored as db*100. */
            temp_val = (ushort) rda_status;

	}
	else{

            /* The value is stored in 1/4 dB.  Convert to units of db*100. */
            temp_flt = ((float) rda_status * 0.25);
            temp_val = temp_flt * 100.0;

	}

	/* Place reflectivity calibration correction in message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get data transmission enabled*/
	rda_status = ORPGRDA_get_status (RS_DATA_TRANS_ENABLED);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get Data transmission enabled for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place data transmission enabled in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get RDA volume coverage pattern identifier*/
	rda_status = ORPGRDA_get_status (RS_VCP_NUMBER);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get vcp for RMS status message.");
		/* No Pattern */
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place RDA volume coverage pattern identifier in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get RDA control authorization */
	rda_status = ORPGRDA_get_status (RS_RDA_CONTROL_AUTH);

	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get Rda control authorization for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place RDA control authorization in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
      /* Get interference detection rate - ORDA does not have an ISU */
   if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
      rda_status = 0;
   else {
	   rda_status = ORPGRDA_get_status (RS_INTERFERENCE_DETECT_RATE);

      if (rda_status == ORPGRDA_DATA_NOT_FOUND) {
         LE_send_msg(RMS_LE_ERROR,
             "Unable to get interference detection rate for RMS status message.");
         rda_status = 0;
		}
   }

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place interference detection rate in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;

	/* Get RDA operational mode */
	rda_status = ORPGRDA_get_status (RS_OPERATIONAL_MODE);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get operational mode for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place RDA operational mode in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
      /* Get ISU enabled, disabled - there is not an ISU on the ORDA */
   if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
      rda_status = 0;
   else {
      rda_status = ORPGRDA_get_status (RS_ISU);

      if (rda_status == ORPGRDA_DATA_NOT_FOUND) {
         LE_send_msg(RMS_LE_ERROR, "Unable to get ISU for RMS status message.");
         rda_status = 0;
      }
   }

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place ISU enabled, disabled in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
      /* Get Archive II as received from RDA - ORDA does not have an archive device */
   if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
      rda_status = 0;
   else {
      rda_status = ORPGRDA_get_status (RS_ARCHIVE_II_STATUS);
	
      if (rda_status == ORPGRDA_DATA_NOT_FOUND) {
         LE_send_msg(RMS_LE_ERROR, "Unable to get archive II status for RMS status message.");
         rda_status = 0;
      }
   }

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place Archive II as received from RDA in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
      /* Get Archive II estimated remaining capacity - ORDA does not have an archive device */
   if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
      rda_status = 0;
   else {
      rda_status = ORPGRDA_get_status (RS_ARCHIVE_II_CAPACITY);
	
      if (rda_status == ORPGRDA_DATA_NOT_FOUND) {
         LE_send_msg(RMS_LE_ERROR, "Unable to get archive II capacity for RMS status message.");
         rda_status = 0;
      }
   }

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place Archive II estimated remaining capacity in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Place Spares ( 2 HALFWORDS ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	
	/* Get channel control status */
	rda_status = ORPGRDA_get_status (RS_CHAN_CONTROL_STATUS);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get channel control status for RMS status message.");
		/* Controlling */
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place channel control status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Get spot blanking status */
	rda_status = ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get spot blanking status for RMS status message.");
		/* Not implemented */
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place spot blanking status in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Get the bypass map generation date */
	rda_status = ORPGRDA_get_status (RS_BPM_GEN_DATE);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get bypass map generation date for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place bypass map generation date in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Get the bypass map generation time */
	rda_status = ORPGRDA_get_status (RS_BPM_GEN_TIME);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get bypass map generation time for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place the bypass map generation time in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Get the notchwidth map generation date */
	rda_status = ORPGRDA_get_status (RS_NWM_GEN_DATE);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get notchwidth map generation date for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place the notchwidth map generation date in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Get the notchwidth map generation time */
	rda_status = ORPGRDA_get_status (RS_NWM_GEN_TIME);
	
	if(rda_status == ORPGRDA_DATA_NOT_FOUND){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get notchwidth map generation time for RMS status message.");
		rda_status = 0;
		} /* End if */

	/* Caste the value to an unsigned short */
	temp_val = (ushort)rda_status;
	/* Place the notchwidth map generation time in the message */
	conv_ushort_unsigned(status_buf_ptr, &temp_val);
	status_buf_ptr += PLUS_SHORT;
	
	/* Place Spares ( 3 HALFWORDS ) in the message */
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	conv_ushort_unsigned(status_buf_ptr, &blank);
	status_buf_ptr += PLUS_SHORT;
	
		   				
	/* Add the message terminator */
	add_terminator(status_buf_ptr);
	status_buf_ptr += PLUS_INT;
		
	/* Compute the number of halfwords */
	num_halfwords = ((status_buf_ptr - status_buf) / 2);

	/* Add the header to the message */
	ret = build_header(&num_halfwords, STATUS_TYPE, status_buf, 0);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
			"RMS: RMS build header failed for send status");
		} /* End if */
		
	/* Send the message to the FAA/RMMS */
	ret = send_message(status_buf,STATUS_TYPE,RMS_STANDARD);
	
	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR, 
			"RMS: Send message failed (ret %d) for send status", ret);
		} /* End if */

}/* End send status message */
