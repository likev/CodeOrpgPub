/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/19 14:54:10 $
 * $Id: cldm_init.c,v 1.4 2011/09/19 14:54:10 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 *
 */  


/* cldm_init.c - This file contains the primary initialization routines 
                 for convert_ldm */


#include "cldm.h"
#include <string.h>


/********************************************************************************

    Description: Initialize the Archive II transmission status

          Input: 

         Output: *initialized_status - transmission status of Archive II

         Return: 0 on success; -1 on error
   
 ********************************************************************************/

int INIT_archive_status (int *initialized_status) {

   ArchII_transmit_status_t a2_xmit_status;
   int ret;
   ArchII_command_t a2_cmd;

   if ((ret = ORPGDA_open (ORPGDAT_ARCHIVE_II_INFO, LB_WRITE)) < 0) {
      LE_send_msg (GL_INFO, "ORPGDA_open(ORPGDAT_ARCHIVE_II_INFO) failed (%d)", ret);
      return (-1);
   }

   a2_xmit_status.ctime = (time_t) time (NULL);

      /* Check if LDM transmission is turned On or Off. */

   ret = ORPGDA_read (ORPGDAT_ARCHIVE_II_INFO, &a2_cmd, sizeof (a2_cmd),
                      ARCHIVE_II_COMMAND_ID);

      /* check for read errors */

   if (ret < 0) {
         /* Assume the transmission status is on */
      a2_xmit_status.status = ARCHIVE_II_TRANSMIT_ON;
      LE_send_msg (GL_ERROR, "Error reading ARCHIVE_II_COMMAND_ID (err: %d)",
                   ret);
      LE_send_msg (GL_INFO, "Initializing A2 Xmit Status to \"ON\"");
   } else {
      if (a2_cmd.command == ARCHIVE_II_NEED_TO_STOP) {
         a2_xmit_status.status = ARCHIVE_II_TRANSMIT_OFF;
         LE_send_msg (GL_INFO, "A2 Xmit Status initialized to \"OFF\"");
      } else { /* assume xmit status is on */
         a2_xmit_status.status = ARCHIVE_II_TRANSMIT_ON;
         LE_send_msg (GL_INFO, "A2 Xmit Status initialized to \"ON\"");
      }
   }

      /* write the Xmit status to the Status LB */

   ret = ORPGDA_write (ORPGDAT_ARCHIVE_II_INFO, (char *) &a2_xmit_status,
                       sizeof (a2_xmit_status), ARCHIVE_II_FLOW_ID);

   if (ret < 0)
      LE_send_msg (GL_ERROR, 
                   "Failure writing Xmit status to ARCHIVE_II_FLOW_ID (err: %d)", ret);

   *initialized_status = a2_xmit_status.status;

   return (0);
}


/********************************************************************************

    Description: Initialize the input data LBs

          Input: 

         Output: *realtime_data_id              - Data ID of the realtime datastream LB
                 *recombined_data_id            - Data ID of the recombined DP LB
                 *recombined_dp_removed_data_id - Data ID of the recombined LB with 
                                                  DP removed

         Return: 0 on success; library error code on error
   
 ********************************************************************************/

int INIT_input_lbs (int *recombined_dp_data_id, int *recombined_dp_removed_data_id,
                    int *realtime_data_id) {

   int ret = 0;

   *realtime_data_id = ORPGDAT_CM_RESPONSE;
   *recombined_dp_data_id = CLDM_RECOMBINED_DP;
   *recombined_dp_removed_data_id = ORPGDAT_RECOMBINED_RAWDATA;

      /* Open the realtime datastream LB */

   if ((ret = ORPGDA_open (*realtime_data_id, LB_READ)) < 0) {
      LE_send_msg (GL_ERROR, 
         "ORPGDA_open(%d) failed during initialization (err: %d)", 
         *realtime_data_id, ret);
      return (ret);
   } else if ((ret = ORPGDA_seek (*realtime_data_id, 0, LB_LATEST, NULL)) < 0) {
      LE_send_msg (GL_ERROR, 
         "ORPGDA_seek(%d) failed during initialization (err: %d)", 
         *realtime_data_id, ret);
      return (ret);
   }

      /* Open the DP recombined LB */

   if ((ret = ORPGDA_open (*recombined_dp_data_id, LB_READ)) < 0) {
      LE_send_msg (GL_INFO, 
         "ORPGDA_open(%d) failed during initialization (err: %d)", 
         *recombined_dp_data_id, ret);
      return (ret);
   } else if ((ret = ORPGDA_seek (*recombined_dp_data_id, 0, LB_LATEST, NULL)) < 0) {
      LE_send_msg (GL_ERROR, 
         "ORPGDA_seek(%d) failed during initialization (err: %d)", 
         *recombined_dp_data_id, ret);
      return (ret);
   }

      /* Open the DP removed recombined LB */

   if ((ret = ORPGDA_open (*recombined_dp_removed_data_id, LB_READ)) < 0) {
      LE_send_msg (GL_INFO, 
         "ORPGDA_open(%d) failed during initialization (err: %d)", 
         *recombined_dp_removed_data_id, ret);
      return (ret);
   } else if ((ret = ORPGDA_seek (*recombined_dp_removed_data_id, 0, 
                                  LB_LATEST, NULL)) < 0) {
      LE_send_msg (GL_ERROR, 
         "ORPGDA_seek(%d) failed during intialization (err: %d)", 
         *recombined_dp_removed_data_id, ret);
      return (ret);
   }

   return (ret);
}


/********************************************************************************

    Description: Initialize misc RPG parameters used by convert_ldm

          Input: 

         Output: *site_id         - The site ICAO/ID
                 *rda_channel_num - the RDA channel number

         Return: 0 on success; -1 on error
   
 ********************************************************************************/

int INIT_RPG_parameters (char *site_id, int *rda_channel_num) {
   int  ret;
   char *ptr;

      /* Get site ICAO */

   if ((ret = DEAU_get_string_values( "site_info.rpg_name", &ptr)) < 0) {
      LE_send_msg (GL_ERROR, 
                   "Failure getting site_info.rpg_name (%d)", ret);
      return (-1);
   }

   strncpy (site_id, ptr, 4);
   site_id[4] = '\0';

      /* Set rda channel flag. */

   if ((ret = DEAU_get_string_values ("site_info.is_orda", &ptr)) > 0) {
     if (strcmp( ptr, "Yes") == 0)
        *rda_channel_num = 8; 
     else
        rda_channel_num = 0;
   } else {
      LE_send_msg (GL_ERROR, "Failed getting is_orda? (%d)", ret);
     return(-1);
   }

   if ((ret = DEAU_get_string_values ("Redundant_info.redundant_type", &ptr)) > 0) {
      if (strcmp (ptr, "No Redundancy") != 0) {
         if ((ret = DEAU_get_string_values ("Redundant_info.channel_number", &ptr)) > 0) {
            if (strcmp (ptr, "Channel 1") == 0)
               rda_channel_num += 1;
            else
               rda_channel_num += 2;
         } else {
            LE_send_msg (GL_ERROR, "Failed getting channel_number (%d)", ret);
            return(-1);
         }
      }
   } else {
      LE_send_msg (GL_ERROR, "Failed getting redundant_type (%d)\n", ret);
      return(-1);
   }
   return (0);
}
