/**************************************************************************
   
   Module: rms_alarm_msg.c 
   
   Description:  This is the module for building and sending alarm
   messages.

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/09/19 15:35:48 $
 * $Id: rms_alarm_msg.c,v 1.18 2007/09/19 15:35:48 cmn Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <rda_alarm_table.h>
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define RMS_ALARM_MSG      3
#define MAX_ALARM_MSG_SIZE   82
#define WB1         1
#define WB2         2

/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  Thsi routine is registered as an event and is called whenever
   an alarm event is posted.  It sends the alarm message to RMMS.
      
   Input: None

   Output:  RMMS alarm message.

   Returns: None

   Notes:

   **************************************************************************/
void rms_send_alarm() {

   RDA_status_t *rda;  /*Rda status structure */
   RDA_alarm_t *wb_alarm_stat; /* Rda alarm structure */
   char Alarm_data [sizeof (RDA_alarm_t)];  /* Buffer for LB read */
   int ret, i;
   int msg_size;
   static ushort wb_alarm;
   UNSIGNED_BYTE rms_alarm_buf[MAX_BUF_SIZE]; /* Buffer for RMS message */
   UNSIGNED_BYTE *rms_alarm_buf_ptr; /* Pointer to RMS buffer */
   unsigned int rpg_alarms = 0;
   ushort num_halfwords;
   ushort temp_val;

   /* Retrieve RPG status */
   if((ret = ORPGINFO_statefl_get_rpgalrm(&rpg_alarms)) < 0) {
      LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get RPG alarms");
   }

   rda = ORPGGST_get_rda_stats();

   /* Retrieve alarm data */
   ret = ORPGDA_read (ORPGDAT_RDA_ALARMS, (char *) &Alarm_data,
         sizeof (RDA_alarm_t), LB_NEXT);

   /* Get wideband alarm status */
   wb_alarm_stat = (RDA_alarm_t*) Alarm_data;

   if (rda == NULL)
      LE_send_msg (RMS_LE_ERROR, "RMS: Unable to get rda error status");

   /* Set pointer to buffer start */
   rms_alarm_buf_ptr = rms_alarm_buf;

   /* Place pointer at past header */
   rms_alarm_buf_ptr += MESSAGE_START;

   /* RPG alarm state */
   temp_val = 0;

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_NONE) )
          temp_val |= 1; /*no alarms*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_RPGCTLFL) )
          temp_val |= 8; /*rpg control failure*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_DBFL) )
          temp_val |= 16; /*bad disk*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_WBDLS) )
          temp_val |= 64; /*wide band load shed*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_PRDSTGLS) )
          temp_val |= 256; /*product load shed*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_WBFAILRE) )
          temp_val |= 512; /*wideband fail*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_RPGRPGFL) )
          temp_val |= 4096; /*rpg to rpg interconnect fail*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_REDCHNER) )
          temp_val |= 8192; /*redundant channel alarm*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_FLACCFL) )
          temp_val |= 16384; /*file access failure*/

   conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
   rms_alarm_buf_ptr += PLUS_SHORT;

   /* RPG alarms maintenance required */

   temp_val = 0;

   if( (rpg_alarms & 0x00) )
          temp_val |= 1; /*streamer tape failure*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_DBFL) )
          temp_val |= 32; /*bad disk*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_RPGTSKFL) )
          temp_val |= 64; /*task failure*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_RPGRPGFL) )
          temp_val |= 128; /*rpg to rpg interconnect fail*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_REDCHNER) )
          temp_val |= 256; /*redundant channel alarm*/

   conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
   rms_alarm_buf_ptr += PLUS_SHORT;

   /* RPG alarms maintenance mandatory*/

   temp_val = 0;

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_WBFAILRE) )
          temp_val |= 2; /*wide band failure*/

   conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
   rms_alarm_buf_ptr += PLUS_SHORT;

   temp_val = 0;

   /* RPG alarms  load shedding*/

   if( (rpg_alarms & ORPGINFO_STATEFL_RPGALRM_WBDLS) )
          temp_val |= 8; /*wide band load shed*/

   /*If( (rpg_alarms & 0x0) )
      temp_val |= 16;*/ /*narrowband load shed*/

   conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
   rms_alarm_buf_ptr += PLUS_SHORT;

   /* WB1 alarm status */
   if (wb_alarm_stat->alarm == RDA_ALARM_RPG_LINK_MINOR_ALARM){
      if (wb_alarm_stat->code == 1){
         wb_alarm = 1;
      }
      else {
         wb_alarm = 0;
      }
   }
   else if (wb_alarm_stat->alarm == RDA_ALARM_RPG_LINK_FUSE_ALARM){
      if (wb_alarm_stat->code == 1){
         wb_alarm = 4;
      }
      else {
         wb_alarm = 0;
      }
   }

   else if (wb_alarm_stat->alarm == RDA_ALARM_RPG_LINK_MAJOR_ALARM){
      if (wb_alarm_stat->code == 1){
         wb_alarm = 2;
      }
      else {
         wb_alarm = 0;
      }
   }

   else if (wb_alarm_stat->alarm == RDA_ALARM_ROG_LINK_REMOTE_ALARM){
      if (wb_alarm_stat->code == 1){
         wb_alarm = 3;
      }
      else {
         wb_alarm = 0;
      }
   }

   /* WB1 alarm status */
   conv_ushort_unsigned(rms_alarm_buf_ptr, &wb_alarm);
   rms_alarm_buf_ptr += PLUS_SHORT;

   /* WB2 alarm status */
   temp_val = (ushort)RDA_ALARM_NONE;

   conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
   rms_alarm_buf_ptr += PLUS_SHORT;

   /* RDA alarm summary */
   temp_val = 0;

   if(rda != NULL){
      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG) {
         if( (rda->status_msg.rda_alarm & 0x02) )
                  temp_val |= 2; /*tower/utilities*/

         if( (rda->status_msg.rda_alarm & 0x04) )
                   temp_val |= 4; /*pedestal*/

         if( (rda->status_msg.rda_alarm & 0x08) )
                  temp_val |= 8; /*transmitter*/

         if( (rda->status_msg.rda_alarm & 0x10) )
                   temp_val |= 16; /*receiver*/

         if( (rda->status_msg.rda_alarm & 0x20) )
                   temp_val |= 32; /*rda control*/

         if( (rda->status_msg.rda_alarm & 0x40) )
                   temp_val |= 64; /*communication*/

         if( (rda->status_msg.rda_alarm & 0x80) )
                   temp_val |= 128; /*signal procesor*/
      } else {
         if( (rda->status_msg.rda_alarm & 0x02) )
                  temp_val |= 2; /*tower/utilities*/

         if( (rda->status_msg.rda_alarm & 0x04) )
                   temp_val |= 4; /*pedestal*/

         if( (rda->status_msg.rda_alarm & 0x08) )
                  temp_val |= 8; /*transmitter*/

         if( (rda->status_msg.rda_alarm & 0x10) )
                   temp_val |= 16; /*reveiver/signal processor*/

         if( (rda->status_msg.rda_alarm & 0x20) )
                   temp_val |= 32; /*rda control*/

         if( (rda->status_msg.rda_alarm & 0x40) )
                   temp_val |= 64; /*rpg communication*/

         if( (rda->status_msg.rda_alarm & 0x80) )
                   temp_val |= 128; /*user communication*/

         if( (rda->status_msg.rda_alarm & 0x100) )
                  temp_val |= 256; /*archive II*/
      }
   }

   /* RDA alarm status */
   conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
   rms_alarm_buf_ptr += PLUS_SHORT;

   /* RDA alarm codes */
   if (rda != NULL){
      /* Get the first 14 alarm codes for RMS */
      for (i=0; i<14; i++) {
         temp_val = (ushort)rda->status_msg.alarm_code[i];
         conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
         rms_alarm_buf_ptr += PLUS_SHORT;
      } /* End if*/
   }
   else {
      /* If alarm codes are not found set codes in RMS message to zero  */
      for (i=0; i<14; i++) {
         temp_val = 0;
         conv_ushort_unsigned(rms_alarm_buf_ptr, &temp_val);
         rms_alarm_buf_ptr += PLUS_SHORT;
      } /* End loop*/
   }

   /* Determine message size */
   msg_size = rms_alarm_buf_ptr - rms_alarm_buf;

   /* Pad the message with zeros */
   pad_message (rms_alarm_buf_ptr, msg_size, MAX_ALARM_MSG_SIZE);

   /* Set pointer to the end of message */
   rms_alarm_buf_ptr += (MAX_ALARM_MSG_SIZE - msg_size);

   /* Add the message terminator to the buffer */
   add_terminator(rms_alarm_buf_ptr);
   rms_alarm_buf_ptr += PLUS_INT;

   /* Compute the number of halfwords in the message */
   num_halfwords = ((rms_alarm_buf_ptr - rms_alarm_buf) / 2);

   /* Add the header to the message */
   ret = build_header(&num_halfwords, RMS_ALARM_MSG, rms_alarm_buf, 0);

   if (ret != 1) {
      LE_send_msg (RMS_LE_ERROR,
         "RMS: RMS build header failed for alarm message");
   }

   /* Send the message to the ouput buffer */
   send_message(rms_alarm_buf,RMS_ALARM_MSG,RMS_STANDARD);

   if (ret != 1) {
      LE_send_msg (RMS_LE_ERROR,
         "RMS: Send message failed (ret %d) for rms ack", ret);
   }

   return;

} /* End rms send alarm */

