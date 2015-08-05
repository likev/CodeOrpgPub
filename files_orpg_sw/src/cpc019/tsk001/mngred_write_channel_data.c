/********************************************************************************
 
            file:  mngred_write_channel_data.c

     Description:  This file contains all the routines needed to manage 
                   LB writes to the redundant channel.


 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/12 16:21:53 $
 * $Id: mngred_write_channel_data.c,v 1.12 2013/02/12 16:21:53 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */


#include <mngred_globals.h>
#include <orpgadpt.h>

#define ADAPT_DAT_UPDATE_TIME_INTERVAL 2   /* time to wait after Adapt Dat updated */

static time_t Time_adapt_dat_updated = 0;

/* local function prototypes */

static int Copy_lb (Lb_table_entry_t *table_entry, void *tag_addr);
static int Open_redundant_lb (Lb_table_entry_t *table_entry);
static int Set_chan_err_alarm ();
static int Update_adapt_dat_database (void);


/********************************************************************************

    Description: This routine clears the redundant channel error alarm

          Input:

         Output:

         Return: 0 on success; -1 on error

        Globals:
 
          Notes:
 
 ********************************************************************************/

int WCD_clear_chan_err_alarm ()
{
   unsigned char alarm_value;  /* the state of the alarm bit */
   int ret;                    /* function calls return value */

      /* clear the redundant channel error state file alarm */

   ret = ORPGINFO_statefl_rpg_alarm (ORPGINFO_STATEFL_RPGALRM_REDCHNER,
                                     ORPGINFO_STATEFL_CLR, &alarm_value);

   if (ret < 0)
      LE_send_msg (GL_ERROR,
            "Error clearing Redundant Channel Error alarm");

   if (ret < 0)
        return (-1);
   else
      return (0);
}


/********************************************************************************

    Description: This routine sets the time the last time the adaptation
                 data was updated

          Input:

         Output:

         Return: 0 on success; -1 on error

        Globals:
 
          Notes:
 
 ********************************************************************************/

void WCD_set_adapt_data_updated_time (void) {
   Time_adapt_dat_updated = time (NULL);
   return;
}   


/********************************************************************************

    Description: This routine checks each lookup table entry to see if its 
                 "update required" flag is set. If it is set, the LB msg(s) are
                 copied to the other channel.

          Input:

         Output:

         Return:

        Globals: CHAnnel_status  - see mngred_globals.h & orpgred.h
 
          Notes:
 
 ********************************************************************************/

void WCD_update_redun_ch (void)
{
   int              i;
      /* function calls return value */
   int              return_val;
      /* ptr to table entry being checked/processed */
   Lb_table_entry_t *table_entry;
      /* tag of the LB being updated */
   static int       tag;
      /* flag specifying that Adaptation Data has been updated */
   int              adapt_data_updated         = MNGRED_FALSE;
      /* flag specifying that an Adaptation Data update failed */
   int              adapt_data_update_failed   = MNGRED_FALSE;
      /* number of entries in the lookup table */
   int              number_of_entries;
      /* flag specifying that an Adapt Dat update failed on a previous pass */
   static int       previous_adapt_data_failed = MNGRED_FALSE;
      /* flag specifying that State Data has been updated */
   int              state_data_updated         = MNGRED_FALSE;
      /* flag specifying that a State Data update failed */
   int              state_data_update_failed   = MNGRED_FALSE;
      /* flag specifying that a State Data update failed on a previous pass */
   static int       previous_state_data_failed = MNGRED_FALSE;


   if ((time (NULL) - Time_adapt_dat_updated) < ADAPT_DAT_UPDATE_TIME_INTERVAL)
      return;

   table_entry = MLT_get_table_ptr ();

      /* if the table pointer is NULL, then the LB lookup table has 
         become corrupted. Since there is no way to recover from this, abort the
         task */

   if (table_entry == NULL)
   {
      LE_send_msg (GL_ERROR, 
                   "Fatal Error: LB lookup table pointer has been corrupted");
                   ORPGTASK_exit(EXIT_FAILURE);
   }

      /* scan all the entries in the lookup table to see if any have
         their update_required flag set */

   number_of_entries = MLT_get_number_of_entries ();

   for (i = 1; i <= number_of_entries; i++)
   {
      char *buf; /* ptr to the allocated read buffer */
      int update_error_flag = MNGRED_FALSE; /* flag specifying an update 
                                               error occurred */

         /* if any update_required flag is set, then the LB, message on the
            redundant channel referenced in the lookup table needs updating */

      if (table_entry->update_required == MNGRED_TRUE)
      {
         if ((table_entry->update_type == MNGRED_TRANSFER_STATE_DATA)  &&
             (state_data_updated == MNGRED_FALSE))
                 state_data_updated = MNGRED_TRUE;

         if ((table_entry->update_type == MNGRED_TRANSFER_ADAPT_DATA)  &&
             (adapt_data_updated == MNGRED_FALSE))
                  adapt_data_updated = MNGRED_TRUE;

            /* register the tag address for the local lbd */

         return_val = LB_register (table_entry->local_lb_fd, LB_TAG_ADDRESS, &tag);

         if (return_val != LB_SUCCESS)
         {
            LE_send_msg (GL_ERROR, 
                         "Tag registration failed for local lbd %d (err %d)",
                         table_entry->local_lb_fd, return_val);
            continue;
         }

            /* if the msg id in the lookup table is set to -1, then update
               all the messages in this LB */

         if (table_entry->msg_id == -1)
         {
               /* some of the Adaptation Data are retained in a database and some are not,
                  so determine which way to upate the other channel */
            if (table_entry->data_id == ORPGDAT_ADAPT_DATA) 
               return_val = Update_adapt_dat_database ();
            else
               return_val = Copy_lb (table_entry, &tag);
         }
         else
         {
            return_val = ORPGDA_read (table_entry->data_id, &buf, LB_ALLOC_BUF, 
                                      table_entry->msg_id);

            if (return_val < 0)
            {
               LE_send_msg (GL_ERROR,
                     "Error reading data_id %d, msg_id %d", table_entry->data_id,
                     table_entry->msg_id);
            }
            else
            {
               int msg_len;  /* length of message to write */

               msg_len = return_val;
               
               return_val = WCD_write_redundant_lb_data (table_entry->data_id, buf, 
                                         msg_len, table_entry->msg_id, table_entry,
                                         &tag);
/*               if (return_val >= 0)
                     LE_send_msg (MNGRED_TEST_VL, 
                              "Redundant channel lbd %d, msg_id %d updated",
                              table_entry->data_id, table_entry->msg_id); */

                  /* if this is a VAD data update, post the event on the 
                     other channel */

               if (return_val >= 0) {

                  if ((table_entry->data_id == 100400) && (table_entry->msg_id == 1)) {

                        int group_number;
                        int redundant_chanl_num;

                        redundant_chanl_num = ORPGRED_channel_num (ORPGRED_OTHER_CHANNEL);
                        group_number = EN_control (EN_SET_AN_GROUP, redundant_chanl_num);

                        EN_post (ORPGEVT_ENVWND_UPDATE, (void *) NULL, 0, 0);

                        group_number = EN_control (EN_SET_AN_GROUP, group_number);
                  }
               }
               free (buf);
            }
         }
            /* if an error occurred, set the error flag; otherwise, clear the
               table entry's update_required flag */

         if (return_val < 0)
            update_error_flag = MNGRED_TRUE;
         else
            table_entry->update_required = MNGRED_FALSE;
      }

         /* if an error occurred updating the LB, then set the appropriate
            error failed flag */

      if (update_error_flag == MNGRED_TRUE)
      {
         if (table_entry->update_type == MNGRED_TRANSFER_STATE_DATA)
             state_data_update_failed = MNGRED_TRUE;

         if (table_entry->update_type == MNGRED_TRANSFER_ADAPT_DATA)
             adapt_data_update_failed = MNGRED_TRUE;

         update_error_flag = MNGRED_FALSE;
      }

         /* adjust the pointer to the next table entry */
           
      ++table_entry;
   }

      /* log the appropriate status/alarm msg(s) for state data updates */

   if (state_data_updated == MNGRED_TRUE)
   {
      if ((state_data_update_failed == MNGRED_TRUE)    &&
          (previous_state_data_failed == MNGRED_FALSE))
      {
           LE_send_msg (GL_STATUS | GL_ERROR, 
                        "%s Failure writing State Data to redundant channel", 
                        MNGRED_WARN_ACTIVE);
           previous_state_data_failed = MNGRED_TRUE;
      }
      else if ((state_data_update_failed == MNGRED_FALSE)  &&
               (previous_state_data_failed == MNGRED_TRUE))
      {
         LE_send_msg (GL_STATUS, 
                      "%s Failure writing State Data to redundant channel", 
                      MNGRED_WARN_CLEAR);
         previous_state_data_failed = MNGRED_FALSE;
      }
   }

      /* log the appropriate status/alarm msg(s) for adaptation data updates */

   if (adapt_data_updated == MNGRED_TRUE)
   {
      if ((adapt_data_update_failed == MNGRED_TRUE)   &&
          (previous_adapt_data_failed == MNGRED_FALSE))
      {
         LE_send_msg (GL_STATUS | GL_ERROR, 
                      "%s Failure writing Adaptation Data to redundant channel",
                      MNGRED_WARN_ACTIVE);
         previous_adapt_data_failed = MNGRED_TRUE;
      }
      else if (adapt_data_update_failed == MNGRED_FALSE)
      {
            /* set the IPC command to update the redundant channel
               Adaptation Data's update time */

         CHAnnel_status.adapt_dat_update_time = time (NULL);

         DC_set_IPC_cmd (ORPGRED_UPDATE_ADAPT_DATA_TIME, 
                         CHAnnel_status.adapt_dat_update_time, 0, 0, 0, 0);

         LE_send_msg (GL_STATUS,
                    "Adaptation Data successfully written to redundant channel");
         
         if (previous_adapt_data_failed == MNGRED_TRUE)
         {
            LE_send_msg (GL_STATUS, 
                         "%s Failure writing Adaptation Data to redundant channel",
                         MNGRED_WARN_CLEAR);
            previous_adapt_data_failed = MNGRED_FALSE;
         }
      }
   }
   return;
}


/********************************************************************************

    Description: This routine performs the physical LB writes to the 
                 redundant channel

          Input: data_id        - data id of the local channel LB
                 buffer         - buffer pointer to the message to write
                 msg_length     - length of the message 
                 msg_id         - id of the message
                 lb_table_entry - pointer to LB lookup table entries
                 tag_address    - pointer to the message tag address

         Output:

         Return: 0 on success; -1 on error 

        Globals: CHAnnel_link_state - see mngred_globals.h & orgpred.h
 
          Notes: The redundant manager process is unique in that it must
                 call the LB_write routine explicitly to perform data writes 
                 to the other channel. All other ORPG processes perform data 
                 writes by calling ORPGDA_write (which in turn calls 
                 LB_write to accomplish the actual write). Currently, 
                 ORPGDA_... routines are limited and can perform I/O only on 
                 the local node; whereas, LB_... routines have the capability 
                 to perform I/O on any node (remote or local). When the
                 ORPGDA_... routines evolve and have the capacitiy to perform
                 I/O on remote nodes in a distributed environment, then the
                 LB_writes in this file should be replaced with ORPGDA_write.
 
 ********************************************************************************/

int WCD_write_redundant_lb_data (int data_id, char *buffer, int msg_length,
                                 LB_id_t msg_id, Lb_table_entry_t *lb_table_entry,
                                 void *tag_address)
{
   int  return_val;                        /* return value from function calls */
   int  redun_lb_fd = MNGRED_UNINITIALIZED;/* the redundant channel's lb_fd */
   Lb_table_entry_t *table_entry;          /* pointer to LB lookup table entries */
   static int data_id_on_error = -1;       /* data_id when error occurred */
   static int msg_id_on_error = -1;        /* msg_id when error occurred */
   int error = MNGRED_FALSE;               /* this pass error flag */
   static int previous_write_error = MNGRED_FALSE;/* previous pass write error
                                                     flag */

   table_entry = lb_table_entry;

      /* find the table entry for this LB if the table entry is 
         not supplied by the calling routine */

   if (table_entry == NULL)
        table_entry = MLT_find_table_entry (data_id, msg_id);

      /* ensure we found an entry in the local LB table */

   if (table_entry == NULL)
   {
      if (previous_write_error == MNGRED_FALSE)
         LE_send_msg (GL_ERROR, 
               "Failed to find data_id %d, msg_id %d in lb lookup table",
               data_id, msg_id);
      error = MNGRED_TRUE;
   }

   if (error == MNGRED_FALSE)
   {
      redun_lb_fd = table_entry->redundant_lb_fd;

         /* if the redundant channel LB is not opened, try to open it now */
   
      if (redun_lb_fd == MNGRED_UNINITIALIZED)
      {
         if ((return_val = Open_redundant_lb (table_entry)) < 0)
         {
            if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)
               if (previous_write_error == MNGRED_FALSE)
                  LE_send_msg (GL_ERROR,
                     "Failed to re-open the redun ch. LB %s, lb_fd %d (open err %d)",
                     table_entry->redundant_lb_name, table_entry->redundant_lb_fd,
                     return_val);
            error = MNGRED_TRUE;
         }
         else
            redun_lb_fd = return_val;
      }
   }
    
      /* register the redundant channel lb with the tag address */

   if ((tag_address != NULL) && (error == MNGRED_FALSE))
   {
      return_val = LB_register (redun_lb_fd, LB_TAG_ADDRESS, tag_address);

      if (return_val != LB_SUCCESS)
      {
         if (previous_write_error == MNGRED_FALSE)
            LE_send_msg (GL_ERROR,
               "Tag registration failed for redun channel lbd %d (err %d)",
               redun_lb_fd, return_val);

            /* if the tag registration failed, set the table entry 
               to uninitialized */

         MLT_update_table_redun_ch_lbd (table_entry->data_id, MNGRED_UNINITIALIZED);
         error = MNGRED_TRUE;
      }
   }

      /* if no errors detected, write the data to the redundant channel LB */

   if (error == MNGRED_FALSE)
   {
      return_val = LB_write (redun_lb_fd, buffer, msg_length, msg_id);

      if (data_id != ORPGDAT_REDMGR_CHAN_MSGS) {
         LE_send_msg (MNGRED_DEBUG_VL,
            "LB_write returned: %d; data_id: %d, redun_lbd: %d, msg_id: %d, msg_length to write: %d",
            return_val, table_entry->data_id, redun_lb_fd, msg_id, msg_length);
         LE_send_msg (MNGRED_DEBUG_VL, ".....remote lb name: %s", table_entry->redundant_lb_name);
      }

         /* if a write failure occurs, uninitialize the redundant channel's LB
            descriptor */

      if (return_val < 0)
      {
         if (previous_write_error == MNGRED_FALSE)
               LE_send_msg (GL_ERROR,
                  "LB_write error (err %d);   data_id: %d;   msg_id: %d",
                  return_val, data_id, msg_id);

/*         MLT_update_table_redun_ch_lbd (table_entry->data_id, MNGRED_UNINITIALIZED); */
            /* Since a remote write error occurred, none of the remote LB descriptors
               can be trusted so close them all */
         MLT_reset_redun_ch_lbds ();
         error = MNGRED_TRUE;
      }
   }

      /* if an error occurred, set the redundant channel error alarm */

   if (error == MNGRED_TRUE)
   {
      /* set the redundant channel error alarm bit if the channel link 
         is up and if the message is not a ping/ping response IPC 
         channel message */

      if ((CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP) &&
          (previous_write_error == MNGRED_FALSE)          &&
         ((data_id != ORPGDAT_REDMGR_CHAN_MSGS)           ||
         ((data_id == ORPGDAT_REDMGR_CHAN_MSGS)           &&
          (msg_id  != ORPGRED_PING_CHANNEL_LINK)          &&
          (msg_id  != ORPGRED_PING_RESPONSE))))
      {
         if (Set_chan_err_alarm (data_id, msg_id) == 0)
         {
            previous_write_error = MNGRED_TRUE;
            data_id_on_error = data_id;
            msg_id_on_error = msg_id;
         }
      }
      return (-1);
   }
   else  /* process the state for a good write */
   {
      if ((previous_write_error == MNGRED_TRUE) &&
          (data_id == data_id_on_error)  &&
          (msg_id == msg_id_on_error)    &&
          (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP))
      {
         if (WCD_clear_chan_err_alarm () == 0)
         {
            previous_write_error = MNGRED_FALSE;
            data_id_on_error = MNGRED_FALSE;
            msg_id_on_error = MNGRED_FALSE;
         }
      }
      return (0);
   }
}


/********************************************************************************

    Description: This routine copies all messages from the local channel LB to
                 its peer LB on the redundant channel.

          Input: The local LB table entry to copy

         Output:

         Return: 0 on success; -1 on failure

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Copy_lb (Lb_table_entry_t *table_entry, void *tag_addr)
{
   int  return_val;           /* function calls return value */
   int  msg_length_read;      /* length of msg read */
   int  lbd;                  /* local LB descriptor */
   char *buf;                 /* read buffer pointer */
   LB_id_t msg_id;            /* id of message to process */
/*   LB_id_t msg_id_to_skip = 0; id of msg not to write to redun chanl */
   int error = 0;             /* error flag */

      /* position the pointer to the first message in the LB */
                    
   lbd = table_entry->local_lb_fd;

   return_val = LB_seek (lbd, 0, LB_FIRST, NULL);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
            "Copy_lb: LB_seek to first msg failed for lb_id %d (ret %d)", 
            table_entry->data_id, return_val);
      return (-1);
   }

      /* check for channel specific messages...this data is not
         written to the redundant channel */

/* LBs ORPGDAT_ADAPT & ORPGDAT_BASELINE_ADAPT have been removed, so remove this section 
   of code 08/09/04 - Bld 7 

   if ((table_entry->data_id == ORPGDAT_ADAPT) ||
       (table_entry->data_id == ORPGDAT_BASELINE_ADAPT))
   {
          obtain the id of the msg to skip for this LB (if there is one) 

      msg_id_to_skip = ORPGADPT_get_msg_id ("Redundant Information");

          if an error occurred, log then clear the error 

      if ((msg_id_to_skip < 0) && ORPGADPT_error_occurred())
      {
         ORPGADPT_log_last_error(GL_ERROR, ORPGADPT_REPORT_DETAILS | 
                                 ORPGADPT_CLEAR_ERROR);
         return (-1);
      }
   }
*/

      /* block notifications until all messages have been written
         for this LB */

   LB_NTF_control (LB_NTF_BLOCK);

      /* copy all the messages in the LB to the redundant channel */
       
   while ((return_val = ORPGDA_read (table_entry->data_id, &buf, 
                                     LB_ALLOC_BUF, LB_NEXT)) >= 0)
   {
         /* get the id of the message just read */

      if ((msg_id = LB_previous_msgid (lbd)) < 0)
      {
         LE_send_msg (GL_ERROR,
               "Copy_lb: LB_previous_msgid failed for lb_id %d (msg_id %d)",
               table_entry->data_id, msg_id);
         free (buf);
         error = -1;
         continue;
      }

         /* see if this is a message to skip */
/*      
      if (((table_entry->data_id == ORPGDAT_ADAPT)          ||
          (table_entry->data_id == ORPGDAT_BASELINE_ADAPT)) &&
          (msg_id_to_skip == msg_id))
      {
            LE_send_msg (MNGRED_TEST_VL, 
                         "Channel specific msg_id %d skipped for data_id %d",
                         msg_id, table_entry->data_id);
            continue;
      }
*/      
         /* write the LB message to the redundant channel */

      msg_length_read = return_val;
     
      return_val = WCD_write_redundant_lb_data (table_entry->data_id,
                                            buf, msg_length_read, msg_id,
                                            table_entry, tag_addr);

      if (return_val < 0)
      {
         LE_send_msg (GL_ERROR,
           "Copy_lb: LB_write failed for lb \"%s\" (ret = %d), local lb msg len = %d",
           table_entry->redundant_lb_name, return_val, msg_length_read); 
         error = -1;
      }
/*      else
      {
         LE_send_msg (MNGRED_TEST_VL, 
                      "Redundant channel lbd %d, msg_id %d updated",
                      table_entry->data_id, msg_id);
      } */
      free (buf);
   }

      /* resume notifications */

   LB_NTF_control (LB_NTF_UNBLOCK);

      /* if the last read returns anything but LB_TO_COME, then an 
         error occurred */

   if (return_val != LB_TO_COME)
   {
       LE_send_msg (GL_ERROR, "Error reading local LB_id %d (err %d)",
                    lbd, return_val);
       error = -1;
   }
   return (error);
}


/********************************************************************************

    Description: This routine attempts to open a LB on the redundant
                 channel.

          Input: table_entry - the table entry of the redundant channel LB 
                               which we're attempting to open

         Output:

         Return: The redundant channel's LB descriptor on success, or the 
                 LB_open error code on failure.

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Open_redundant_lb (Lb_table_entry_t *table_entry)
{
   int ret;  /* function call return value */

   ret = LB_open (table_entry->redundant_lb_name, LB_WRITE, NULL);

   LE_send_msg (MNGRED_DEBUG_VL,
          "Open_redundant_lb: LB_open returned redun_lbd %d for data_id %d",
          ret, table_entry->data_id);

      /* if the LB opened, update the local lb_table entry */

   if (ret >= 0)
      MLT_update_table_redun_ch_lbd (table_entry->data_id, ret);

   return (ret);
}


/********************************************************************************

    Description: This routine sets the redundant channel error alarm

          Input:

         Output:

         Return: 0 on success; -1 on error

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Set_chan_err_alarm ()
{
   unsigned char alarm_value;  /* the state of the alarm bit */
   int ret;

      /* set the redundant channel error state file alarm */

   ret = ORPGINFO_statefl_rpg_alarm (ORPGINFO_STATEFL_RPGALRM_REDCHNER,
                                     ORPGINFO_STATEFL_SET, &alarm_value);

   if (ret < 0)
      LE_send_msg (GL_ERROR,
               "Error setting Redundant Channel Error alarm");

   if (ret < 0)
        return (-1);
   else
      return (0);
}


/********************************************************************************

    Description: This routine updates the Adaption Data that are retained in
                 the Adaptaion Data database

          Input:

         Output:

         Return: 0 on success; -1 on error

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Update_adapt_dat_database (void)
{
   int ret;

   ret = DEAU_update_dea_db (ORPGRED_get_hostname (ORPGRED_OTHER_CHANNEL), 
                             NULL, LB_ALL);

   if (ret < 0) {
      LE_send_msg (GL_ERROR, "Error updating redundant channel Adapt Dat database (err: %d)",
                   ret);
      ret = -1;
   } 
   else
      ret = 0;

   return (ret);
}
