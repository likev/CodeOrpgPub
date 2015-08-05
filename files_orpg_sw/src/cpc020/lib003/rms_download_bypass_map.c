/**************************************************************************

   Module:  rms_rec_download_bypass_map_command.c

   Description:  This module recieves bypass map information sent to ORPG.  Upon
   receipt of this information the bypass map is placed into the system.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/05/25 21:57:00 $
 * $Id: rms_download_bypass_map.c,v 1.27 2004/05/25 21:57:00 ryans Exp $
 * $Revision: 1.27 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <rms_message.h>
#include <orpgedlock.h>


/*
* Static Globals
*/

extern int bypass_maps_locked;


/*
* Static Function Prototypes
*/

static int rms_edit_bypass_info (UNSIGNED_BYTE *rda_bypass_buf, 
                                 short seg_num, short rad_num);


/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: rda_bypass_buf - Pointer to the message buffer.

   Output: Edited bypasws map information

   Returns: 1 = Successful Edit

   Notes:

   **************************************************************************/
int rms_rec_download_bypass_command (UNSIGNED_BYTE *rda_bypass_map_buf) {

   UNSIGNED_BYTE *rda_bypass_map_buf_ptr;
   short         radial, segment;
   int           return_code = 0;
   int           ret;

   /* Set pointer to beginning of buffer */
   rda_bypass_map_buf_ptr = rda_bypass_map_buf;

   /* Place pointer past the header */
   rda_bypass_map_buf_ptr += MESSAGE_START;

   /* Get the segment number */
   segment = conv_shrt(rda_bypass_map_buf_ptr);
   rda_bypass_map_buf_ptr += PLUS_SHORT;

   /* Get the radial number */
   radial = conv_shrt(rda_bypass_map_buf_ptr);
   rda_bypass_map_buf_ptr += PLUS_SHORT;

   /* Get lock status of Bypass Map LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_CLUTTERMAP,LBID_EDBYPASSMAP_LGCY);

   /* If LB locked by someone other than RMS cancel and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && bypass_maps_locked == 0){
      LE_send_msg(RMS_LE_ERROR,"LBID_BYPASSMAP_LGCY is being edited");
      return_code = 24;
   }

   /* Get lock status of command LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

   /* If LB locked cancel and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED){
      LE_send_msg(RMS_LE_ERROR,"Unable to download bypass maps RDA commands are locked");
      return_code = 24;
   }

   /* Validation checks */
   if ( segment  > MAX_BYPASS_MAP_SEGMENTS){
      LE_send_msg(RMS_LE_ERROR,
          "Download bypass map segment exceeds max (seg num = %d)", segment);
      return_code = 24;
   } /* End if */
   else if ( (segment - 1) < 0 ){
      LE_send_msg(RMS_LE_ERROR,"Download bypass map segment less than zero");
      return_code = 24;
   } /* End else if */
   else if ( radial  > BYPASS_MAP_RADIALS ){
      LE_send_msg(RMS_LE_ERROR,
         "Download bypass map radial greater than max (radial num %d)", radial);
      return_code = 24;
   } /* End else if */
   else if ( radial  < 1 ){
      LE_send_msg(RMS_LE_ERROR,
         "Download bypass map radial less than one (radial num %d)", radial);
      return_code = 24;
   } /* End else if */

   /* If validation checks passed edit bypass map */
   if (return_code == 0){
      ret = rms_edit_bypass_info (rda_bypass_map_buf_ptr, segment, radial);

      if ( ret != 1)
         return_code = ret;
   }

   /* If bypass map LBs locked by RMS unlock them */
   if ( bypass_maps_locked ){

      ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_CLUTTERMAP, LBID_EDBYPASSMAP_LGCY);

      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
         LE_send_msg(RMS_LE_ERROR,"LBID_EDBYPASSMAP_LGCY unlock unsuccessful.");
      }
      else {
         LE_send_msg(RMS_LE_ERROR,"LBID_EDBYPASSMAP_LGCY unlock successful.");
      }
      bypass_maps_locked = 0;
   }
   return (return_code);

}/*End rms rec download bypass map */


/**************************************************************************
   Description:  This function reads the command from the message buffer.
      
   Input: rda_bypass_buf - Pointer to the message buffer.
      
   Output: Saved edited bypass map.
      
   Returns: 
   
   Notes:  

   **************************************************************************/
static int rms_edit_bypass_info (UNSIGNED_BYTE *rda_bypass_buf, 
                          short seg_num, short rad_num) {

   
   RDA_bypass_map_msg_t bypass_map;
   Redundant_cmd_t      redun_cmd;
   int                  faa_redun, my_channel_status;
   int                  other_channel_status, link_status;
   int                  ret, i;
   
   /* Initialize the edit buffer by reading bypass map data from
      RDA bypass map LB.*/
      
   ret = ORPGDA_read (ORPGDAT_CLUTTERMAP, (char *) &bypass_map,
         sizeof (RDA_bypass_map_msg_t), LBID_EDBYPASSMAP_LGCY);

   if ((ret <= 0) || (bypass_map.bypass_map.num_segs <= 0) || 
       (bypass_map.msg_hdr.julian_date <= 0)) {
      LE_send_msg (RMS_LE_ERROR, 
          "ORPGDA_read failed (ORPGDAT_CLUTTERMAP) in edit bypass map cmd (ret %d)",ret);
          
      return(-1);
   }

   /* Place map & radial into system */
   for (i=0; i<=31; i++){
       bypass_map.bypass_map.segment[seg_num - 1].data[rad_num -1][i]= conv_shrt(rda_bypass_buf);
      rda_bypass_buf += PLUS_SHORT;
   } /* End bin loop */

   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get reundant type for download bypass map");
   }

   /* If not a redudndant configuration just save the maps */
   if(faa_redun != ORPGSITE_FAA_REDUNDANT){

      ret = ORPGDA_write (ORPGDAT_CLUTTERMAP, (char *) &bypass_map,
            sizeof (RDA_bypass_map_msg_t), LBID_EDBYPASSMAP_LGCY);

      if(ret <0){
         LE_send_msg (RMS_LE_ERROR,
            "ORPGDA_write failed (ORPGDAT_CLUTTERMAP) in edit bypass map command (ret %d)",ret);
         return ( ret);
      }/* End if */

      /* Send a command to RDA to update the bypass maps */
      ret = ORPGRDA_send_cmd (COM4_SENDEDCLBY, RMS_INITIATED_RDA_CTRL_CMD, 
                              0, 0, 0, 0, 0, NULL);
         
      if ( ret < 0){
         LE_send_msg (RMS_LE_ERROR, "Unable to download bypass map (%d)", ret);
         return (ret);
      }/* End if */
   }else {
      /* Get other channel status */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

      /* Get this channel status */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

      /* Get ORPG to ORPG link status */
      link_status = ORPGRED_rpg_rpg_link_state();

      /* If this channel active, or both channels inactive, or ORPG to 
         ORPG link down then save maps */
      if(my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
         link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
         (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
          other_channel_status == ORPGRED_CHANNEL_INACTIVE)){

         ret = ORPGDA_write (ORPGDAT_CLUTTERMAP,(char *) &bypass_map,
                   sizeof (RDA_bypass_map_msg_t),LBID_EDBYPASSMAP_LGCY);

         if(ret <0){
            LE_send_msg (RMS_LE_ERROR,
               "ORPGDA_write failed (ORPGDAT_CLUTTERMAP) in edit bypass map cmd (ret %d)",ret);
            return ( ret);
         }/* End if */

         /* Send a command to RDA to update the bypass maps */
         ret = ORPGRDA_send_cmd (COM4_SENDEDCLBY, RMS_INITIATED_RDA_CTRL_CMD, 
                                 0, 0, 0, 0, 0, NULL);

         if ( ret < 0){
            LE_send_msg (RMS_LE_ERROR, "Unable to download bypass map (%d)", ret);
            return (ret);
         }/* End if */

         /* Set up command to update the redundant channel */
         redun_cmd.cmd = ORPGRED_DOWNLOAD_BYPASS_MAP;
         redun_cmd.lb_id  = ORPGDAT_CLUTTERMAP;
         redun_cmd.msg_id = LBID_EDBYPASSMAP_LGCY;

         /* Send the command to update the redundant channel */
         if((ret = ORPGRED_send_msg (redun_cmd)) < 0){
            LE_send_msg (RMS_LE_ERROR,
               "Unable to download bypass map on redundant channel (%d)", ret);
            return (ret);
         }/* End if */
      }else {
         LE_send_msg (RMS_LE_ERROR,
            "Unable to edit bypass map config in FAA redundant configuration.");
      }/*End else */
   }/* End else */

   return (0);
              
} /*End rms edit bypass info */


