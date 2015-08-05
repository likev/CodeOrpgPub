/********************************************************************************
 
     File desciption:  This file contains the routines used to determine the
                       state of the channel link. It also periodically sends 
                       channel status updates to the other channel.
                       
                       The state of the RPG/RPG channel link is determined by 
                       pinging the redundant channel and checking for a ping 
                       response. If the response is not received within a specified
                       time period, then the link is determined to be down.

                       The application layer ping mechanism had to be implemented 
                       because the Remote System Services Daemon (RSSD) is able to 
                       perform valid writes to another host/node even if the 
                       application software on that host/node is not running. RSSD 
                       only requires that its peer process be running on the 
                       host/node being written to for a write to be successful.

 ********************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2009/05/08 17:03:42 $
 * $Id: mngred_channel_link_services.c,v 1.8 2009/05/08 17:03:42 garyg Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include <limits.h>
#include <mngred_globals.h>


#define MAX_DELTA_TIME            3  /* the maximum time allowed in secs to wait 
                                        for a ping response from the other 
                                        channel before declaring the channel link 
                                        down */

#define STATUS_UPDATE_INTERVAL   30  /* the interval to update the redundant channel 
                                        with this channel's channel status */

   /* file scope variables */

static unsigned int Ping_response_seq_num = INT_MAX;  /* ping response sequence 
                                                         number */

   /* file scope function prototypes */

static int Ping_channel_link (unsigned int sequence_number);


/********************************************************************************

    Description: This routine is responsible for the following:
                 1. determining the state of the channel link usng a ping/ping
                    response mechanism. 
                 2. periodically sending the channel status to the other channel 
                    and when the channels initially connect.
                 3. if this channel is the active channel, sending the state file 
                    data to the inactive channel when the channels initially 
                    connect.
                    
                 The application layer ping is used to determine the capacity 
                 of the redundant channel to send and receive data. If the ping 
                 fails to receive a response within MAX_DELTA_TIME seconds, the 
                 link is assumed to be down.

          Input: 

         Output:

         Return:

        Globals: CHAnnel_link_state     - see mngred_globals.h and orpgred.h
                 CHAnnel_state          - see mngred_globals.h and orpgred.h
                 Ping_response_seq_num  - see file scope global section
                 INTerval_timer_expired - see mngred_globals.h


          Notes:

 ********************************************************************************/

void CLS_check_channel_link ()
{
   time_t              current_time;              /* the current time */
   int                 ret                   = 0; /* function call return value */
   static time_t       time_ping_sent        = 0; /* time the ping was sent */
   static time_t       last_time_status_sent = 0; /* time last status msg was sent */
   static unsigned int ping_seq_num          = 0; /* the current ping sequence # */
       /* last pass's channel link state */
   static Channel_link_state_t last_times_link_state = ORPGRED_CHANNEL_LINK_DOWN;


   current_time = time (NULL);

      /* reset the interval timer expired flag */

   INTerval_timer_expired = MNGRED_FALSE;

      /* if the ping and ping response sequence #s match and the link was 
         down the last pass, then we're transitioning from the link 
         being down to the link being up */

   if ((last_times_link_state == ORPGRED_CHANNEL_LINK_DOWN) &&
       (ping_seq_num == Ping_response_seq_num))
   {
      unsigned char alarm_value; /* RPG/RPG link alarm state */
   
         /* clear the RPG/PRG state file alarm */

      ret = ORPGINFO_statefl_rpg_alarm (ORPGINFO_STATEFL_RPGALRM_RPGRPGFL,
                                        ORPGINFO_STATEFL_CLR, &alarm_value);

      if (ret < 0)
         LE_send_msg (GL_STATUS | GL_ERROR, 
                  "Error clearing RPG/RPG channel link alarm");
         
      CHAnnel_link_state = ORPGRED_CHANNEL_LINK_UP;

         /* update last time's status sent time */

      last_time_status_sent = current_time;

         /* send this channels status to the redundant channel */

      CST_transmit_channel_status ();

         /* if this is the active channel, send the state data to the other
            channel */

      if (CHAnnel_state == ORPGRED_CHANNEL_ACTIVE)
      {
          MLT_set_update_required_flag (NULL, NULL, NULL, MNGRED_UPDATE_ALL_LBS, 
                                        MNGRED_TRANSFER_STATE_DATA);
          CST_set_misc_state_data_flag ();
      }
   }
      
      /*  if the ping delta time has been exceeded and a ping response is 
          pending, then the channel link is considered down */

   if ((current_time - time_ping_sent) >= MAX_DELTA_TIME)
   {
      if ((ping_seq_num != Ping_response_seq_num) &&
          (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP))
      {
         unsigned char alarm_value; /* RPG/RPG link alarm state */

            /* set the rpg/rpg channel link state file alarm */

         ret = ORPGINFO_statefl_rpg_alarm (ORPGINFO_STATEFL_RPGALRM_RPGRPGFL,
                                           ORPGINFO_STATEFL_SET, &alarm_value);

         if (ret < 0)
            LE_send_msg (GL_STATUS | GL_ERROR, 
                  "Error setting RPG/RPG channel link alarm");
         
         LE_send_msg (MNGRED_DEBUG_VL, 
               "       Channel link failed stats (ping seq # %d; response seq # %d)",
               ping_seq_num, Ping_response_seq_num);

         CHAnnel_link_state = ORPGRED_CHANNEL_LINK_DOWN;
         
            /* when the link goes down, all redundant channel LB descriptors
               must be considered invalid, so reset all redundant ch lbds in the
               lookup table */

         MLT_reset_redun_ch_lbds ();
      }

         /* ping the link to check the link state */

      if (Ping_channel_link (++ping_seq_num) < 0)
        LE_send_msg (MNGRED_DEBUG_VL, "Failure pinging channel link (err %d)", ret);

      time_ping_sent = current_time;
   }

      /* if the required status update time interval has expired,
         send the channel status to the other channel */

   if ((current_time - last_time_status_sent) >= STATUS_UPDATE_INTERVAL)
   {
      last_time_status_sent = current_time;

      if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)
      {
            /* send status to other channel */

         CST_transmit_channel_status ();
      }

   }
      /* update last time's channel link state */

   last_times_link_state = CHAnnel_link_state;

   return;
}


/********************************************************************************

    Description: This routine updates the ping response sequence number. It is
                 called by a callback routine when a ping response message is
                 received from the redundant channel.

          Input: seq_num - the sequence number of the ping response (this should
                           match the originating ping's sequence number)

         Output: Ping_response_seq_num - see the file scope global section

         Return:

        Globals:

          Notes:

 ********************************************************************************/

void CLS_update_ping_response (unsigned int seq_num)
{
   Ping_response_seq_num = seq_num;

   LE_send_msg (MNGRED_DEBUG_VL, "Ping response recv'd (seq #: %d)", seq_num);

   return;
}


/********************************************************************************

    Description: This routine pings the channel link to see if the redundant 
                 channel is able to send a response

          Input: sequence_number - the ping sequence number

         Output:

         Return: ret_val - return value from the LB write function (value < 0
                           means the write failed )

        Globals:

          Notes:

 ********************************************************************************/

static int Ping_channel_link (unsigned int sequence_number) 
{
   Redundant_channel_msg_t channel_msg; /* the channel message to write */
   int ret_val;                         /* function call return value */

   LE_send_msg (MNGRED_DEBUG_VL, "Ping the channel link (seq #: %d)", sequence_number);

      /* construct the redundant manager IPC message and write it to the
         redundant channel */

   channel_msg.parameter1 = sequence_number;
   channel_msg.parameter2 = NULL;
   channel_msg.parameter3 = NULL;
   channel_msg.parameter4 = NULL;
   channel_msg.parameter5 = NULL;

   ret_val = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS,
                                   (char *) &channel_msg,
                                   sizeof (Redundant_channel_msg_t),
                                   ORPGRED_PING_CHANNEL_LINK,
                                   NULL, NULL);

      /* if the channel link is down, close and uninitialize the redundant 
         channel redundant manager's channel msgs LB descriptor */

   if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_DOWN)
       MLT_update_table_redun_ch_lbd (ORPGDAT_REDMGR_CHAN_MSGS, MNGRED_UNINITIALIZED);

   return (ret_val);
}
