/**************************************************************************

   Module:  rms_rec_edit_bypass_map_command.c

   Description:  This module builds a bypass map to be sent to RMMS.  Upon
   receipt of this command the bypass map is sent to RMMS for editing.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/05/25 22:00:20 $
 * $Id: rms_edit_bypass_map.c,v 1.22 2004/05/25 22:00:20 ryans Exp $
 * $Revision: 1.22 $
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
#define STATUS_TYPE		22

/*
* Static Globals
*/
int edited_bypass_map;
int bypass_maps_locked;

/*
* Static Function Prototypes
*/

static int rms_send_bypass_map_msg(short rad_num, short seg_num, short bypass);

/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: rda_bypass_buf - Pointer to the message buffer.

   Output: Bypass map sent to FAA/RMMS.

   Returns: 0 = Successful edit.

   Notes:

   **************************************************************************/
int rms_rec_bypass_command (UNSIGNED_BYTE *rda_bypass_buf) {

	UNSIGNED_BYTE *rda_bypass_buf_ptr;
	int           ret;
	short         rad_num, seg_num;
	short         bypass_flag;


	rda_bypass_buf_ptr = rda_bypass_buf;

	/* Place pointer past the header */
	rda_bypass_buf_ptr += MESSAGE_START;

	/* Get the command from the input buffer */
	bypass_flag = conv_shrt(rda_bypass_buf_ptr);
	rda_bypass_buf_ptr += PLUS_SHORT;

	/* Get the segment number */
	seg_num = conv_shrt(rda_bypass_buf_ptr);
	rda_bypass_buf_ptr += PLUS_SHORT;

	/* Get the radial number */
	rad_num = conv_shrt(rda_bypass_buf_ptr);
	rda_bypass_buf_ptr += PLUS_SHORT;

                /* Validation checks for single radial edit */
        	if (bypass_flag == 2){

                                /* See if segment number exceeds max */
                	if ( seg_num  > MAX_BYPASS_MAP_SEGMENTS){
                        		LE_send_msg(RMS_LE_ERROR,"RMS: Edit bypass map segment exceeds max (seg num = %d)", seg_num);
                                        	return (24);
                                                }

                                /* See if segment number less than minimum */
                          	if ( (seg_num - 1) < 0 ){
                                	LE_send_msg(RMS_LE_ERROR,"RMS: Edit bypass map segment less than zero");
                                        	return (24);
                                                }

                                /* See if radial exceeds max */
                          	if ( rad_num  > BYPASS_MAP_RADIALS ){
                                	LE_send_msg(RMS_LE_ERROR,"RMS: Edit bypass map radial greater than max (radial num %d)", rad_num);
                                        	return (24);
                                                }

                                 /* See if radial less than minimum */
                            if ( rad_num  < 1 ){
                                	LE_send_msg(RMS_LE_ERROR,"RMS: Edit bypass map radial less than one (radial num %d)", rad_num);
                                        	return (24);
                                                }

                                 }

         /* Send bypass map information to FAA/RMMS */
         if (bypass_flag == 1 || bypass_flag == 2){

                /* Get lock status of bypass map LB */
         	ret = ORPGEDLOCK_get_edit_status(ORPGDAT_CLUTTERMAP,LBID_EDBYPASSMAP_LGCY);

                                /* If LB locked then cancel edit and return error code */
		if ( ret == ORPGEDLOCK_EDIT_LOCKED){
			LE_send_msg(RMS_LE_ERROR,"RMS: LBID_EDBYPASSMAP_LGCY is being edited");
			return (-1);
			}

                                /* Send requested bypass map radial */
		ret = rms_send_bypass_map_msg (rad_num, seg_num, bypass_flag);

		if (ret != 1){
			return (24);
			}

		return (0);

		} /* call send byapss map */

                /* Cancel edit */
	if (bypass_flag == 3){

                                /* clear the lock on the bypass map LB */
		ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_CLUTTERMAP,LBID_EDBYPASSMAP_LGCY);

		if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
			LE_send_msg(RMS_LE_ERROR,"RMS: LBID_EDBYPASSMAP_LGCY unlock unsuccessful.");
			}
		else{
			LE_send_msg(RMS_LE_ERROR,"RMS: LBID_EDBYPASSMAP_LGCY unlock successful edit cancelled.");
			}

		return (0);

		} /* stop bypass map edit */

		return (0);

} /* End rms rec bypass command */

/**************************************************************************
   Description:  This function builds and sends the message with the selected
   bypass map.

   Input: rad_num - Radial number.
   	  seg_num - segment number.
   	  all - Flag indicating all radials.

   Output:  Bypass map message.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/

static int rms_send_bypass_map_msg(short rad_num, short seg_num, short bypass){

	RDA_bypass_map_msg_t bypass_map;
	UNSIGNED_BYTE        msg_buf[MAX_BUF_SIZE];
  	UNSIGNED_BYTE        *msg_buf_ptr;
	int                  faa_redun;
   int                  my_channel_status;
   int                  other_channel_status;
   int                  link_status;
  	int                  ret;
  	int                  i, j, k;
  	short                segments;
  	ushort               num_halfwords;
  	short                temp_short;

                /* Set pointer to beginning of buffer */
   	msg_buf_ptr = msg_buf;

	/* Set pointer past header to beginning of message */
   	msg_buf_ptr += MESSAGE_START;

	/* Initialize the edit buffer by reading bypass map data from RDA bypass map LB.*/
	ret = ORPGDA_read (ORPGDAT_CLUTTERMAP,
			(char *) &bypass_map,
			sizeof (RDA_bypass_map_msg_t),
			LBID_EDBYPASSMAP_LGCY);

	if ((ret <= 0) || (bypass_map.bypass_map.num_segs <= 0)|| (bypass_map.msg_hdr.julian_date <= 0)) {
                        LE_send_msg (RMS_LE_ERROR,"RMS: Using baseline ORPGDAT_CLUTTERMAP");

                        ret = ORPGDA_read (ORPGDAT_CLUTTERMAP,
			(char *) &bypass_map,
			sizeof (RDA_bypass_map_msg_t),
			LBID_BYPASSMAP_LGCY);

                        if ((ret <= 0) || (bypass_map.bypass_map.num_segs <= 0)|| (bypass_map.msg_hdr.julian_date <= 0)) {
                                LE_send_msg (RMS_LE_ERROR,"RMS: Unable to get baseline ORPGDAT_CLUTTERMAP");
                                return(-1);
		} /* End if */

	        else {
                                /* Get redundant type */
	                faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

	                if (ORPGSITE_error_occurred()){
		        faa_redun = 0;
		        LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get reundant type for download bypass map");
		        }

                                /* If not a redundant configuration just save the maps */
	                if(faa_redun != ORPGSITE_FAA_REDUNDANT){
		        ret = ORPGDA_write (ORPGDAT_CLUTTERMAP,
			(char *) &bypass_map,
			sizeof (RDA_bypass_map_msg_t),
			LBID_EDBYPASSMAP_LGCY);

		        if(ret <0){
			LE_send_msg (RMS_LE_ERROR,"RMS: ORPGDA_write failed (ORPGDAT_CLUTTERMAP) in edit bypass map command (ret %d)",ret);
       	  		return ( -1);
       	  		}/* End if */
		        } /* End if */

                                else {

                                        /* Get other channel status */
		        other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

		        /* Get this channel status */
		        my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);


                                         /* Get ORPG to ORPG link status */
		        link_status = ORPGRED_rpg_rpg_link_state();

                                        /* If this channel active, or both channels inactive, or ORPG to ORPG link down then save maps */
		        if(my_channel_status == ORPGRED_CHANNEL_ACTIVE || link_status == ORPGRED_CHANNEL_LINK_DOWN ||
			(my_channel_status == ORPGRED_CHANNEL_INACTIVE && other_channel_status == ORPGRED_CHANNEL_INACTIVE)){

			ret = ORPGDA_write (ORPGDAT_CLUTTERMAP,
			                (char *) &bypass_map,
			                sizeof (RDA_bypass_map_msg_t),
			                LBID_EDBYPASSMAP_LGCY);

		              if(ret <0){
			                LE_send_msg (RMS_LE_ERROR,"RMS: ORPGDA_write failed (ORPGDAT_CLUTTERMAP) in edit bypass map command (ret %d)",ret);
       	  		                return ( -1);
       	  		                }/* End if */
			} /*End if */

                                        } /* End else */

		}/* End else */

                } /* End if */

                /* Get the number of segments */
	segments = bypass_map.bypass_map.num_segs;

        if ( bypass ==1) {
                LE_send_msg (RMS_LE_ERROR,"RMS: Unable to send all maps");
                return(-1);
                }

        /* If the command is for all the bypass maps then send them. The FAA has decided to not send all the radials and maps
            at this time. This command will never be executed because bypass will never be zero. However the code is being retained
            until the ICD is changed. */
        if ( bypass == 0 ){
               	for (i=0; i<segments; i++) {
			for (j=1; j<=BYPASS_MAP_RADIALS; j++){

                                                        /* Send a message for each radial in the segment */
				temp_short = (short)bypass_map.bypass_map.segment[i].seg_num;

				conv_short_unsigned(msg_buf_ptr,&temp_short);
                 		               msg_buf_ptr += PLUS_SHORT;

                 		               temp_short = (short)j;

                		               conv_short_unsigned(msg_buf_ptr, &temp_short);
	               	 	msg_buf_ptr += PLUS_SHORT;

				for (k=0; k<=31; k++){
	               	  		temp_short = (short)bypass_map.bypass_map.segment[i].data[j][k];
					conv_short_unsigned(msg_buf_ptr,&temp_short);
					msg_buf_ptr += PLUS_SHORT;
        				} /* End bin loop */

                                                        /* Add terminator to the end of the message */
			              add_terminator(msg_buf_ptr);
        			              msg_buf_ptr += PLUS_INT;

                                                        /* Compute the number of halfwords in the message */
        			              num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

                                                        /* Add the header to the message */
				ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

				if (ret != 1){
					LE_send_msg (RMS_LE_ERROR,
						"RMS: RMS build header failed for bypass map");
					return (-1);
					}
                                                        /* Send the message to the FAA/RMMS */
				ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

				if (ret != 1){
					LE_send_msg (RMS_LE_ERROR,
						"RMS: Send message failed (ret %d) for bypass map", ret);
					return (-1);
					}

				msg_buf_ptr = msg_buf;

				msg_buf_ptr += MESSAGE_START;

				}/*End radial loop*/

                                                        /* Wait before sending next radial so the FAA/RMMS is not overwhelmed */
	               	 	msleep(1000);

                	} /* End segments loop */
                } /* End if */

	else if ( bypass == 2 ){

		/* If the command is for a particular map & radial then just get what is needed */
		temp_short = (short)bypass_map.bypass_map.segment[seg_num - 1].seg_num;
		conv_short_unsigned(msg_buf_ptr, &temp_short);
		msg_buf_ptr += PLUS_SHORT;

		temp_short = (short) rad_num;
		conv_short_unsigned(msg_buf_ptr,&temp_short);
		msg_buf_ptr += PLUS_SHORT;

		for (k=0; k<=31; k++){
			temp_short = (short)bypass_map.bypass_map.segment[seg_num - 1].data[rad_num - 1][k];
			conv_short_unsigned(msg_buf_ptr,&temp_short);
			msg_buf_ptr += PLUS_SHORT;
			} /* End bin loop */

		/* Add terminator to the end of the message */
		add_terminator(msg_buf_ptr);
		msg_buf_ptr += PLUS_INT;

		/* Compute the number of halfwords in the message */
		num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

		/* Add the header to the message */
		ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

		if (ret != 1){
			LE_send_msg (RMS_LE_ERROR,
				"RMS: RMS build header failed for bypass map");
			return (-1);
			}

		/* Send the message to the FAA/RMMS */
 		ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

		if (ret != 1){
			LE_send_msg (RMS_LE_ERROR,"RMS: Send message failed (ret %d) for bypass map", ret);
			return (-1);
			}
		} /* End else */

                /* Lock bypass map LB to prevent someone else from editing until RMS is finished */
	ret = ORPGEDLOCK_set_edit_lock(ORPGDAT_CLUTTERMAP,LBID_EDBYPASSMAP_LGCY);

	if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL){
		LE_send_msg(RMS_LE_ERROR,"RMS: Unable to lock LBID_EDBYPASSMAP_LGCY ");
		return (-1);
		}

	else{
		LE_send_msg(RMS_LE_ERROR,"RMS: LBID_EDBYPASSMAP_LGCY locked");
		bypass_maps_locked = 1;
		}

	return (1);
} /*End rms send bypass map */
