/********************************************************************************

            file:  mngred_processing_states.c
            
     Description:  This file contains the routines that are run for the different
                   processing states the channel can be in.
                   
 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/12 16:21:53 $
 * $Id: mngred_processing_states.c,v 1.9 2013/02/12 16:21:53 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */



#include <unistd.h>
#include <stropts.h>

#include <mngred_globals.h>
#include <mrpg.h>
#include <prod_status.h>


#define SWITCHOVER_TIME_LIMIT       14  /* max time allowed to switch to active */
#define MNGRED_CHANL_CMD_DELAY       5


enum {MNGRED_CHANL_NOT_IN_XSITION, MNGRED_XSITION_TO_INACTIVE, 
      MNGRED_XSITION_TO_ACTIVE};


/* file scope variables */

   /* flag specifying that switchover is pending */
static int Switchover_pending = MNGRED_FALSE; 
static int Xsition_type = -1;

   /* the time the switchover started */
static time_t Switchover_start_time = MNGRED_UNINITIALIZED; 

static int Comms_relay_alarm_set = MNGRED_FALSE;

/* local function prototypes */

static int Get_nb_comm_lines_status ();


/********************************************************************************

    Description: This routine processes the channel when in the active state.

          Input:

         Output:

         Return:

        Globals: CHAnnel_state - see mngred_globals.h & orpgred.h
 
          Notes:
 
 ********************************************************************************/

 #define REACQUISITION_DELAY_TIME  5
 #define TIMER_RESET_DELAY        60

void PS_process_active_state () {

   static time_t err_start_time = 0; 
   static int delay_cnt = 0;
   static time_t last_time_relay_reacquired;
   static int reacquisition_in_progress = MNGRED_FALSE;
   Mrpg_state_t rpg_state;
                                        
       /* if this channel has lost the comms relay, then attempt to re-acquire
          the relay (a time delay is included to ensure a slow switchback 
          occurs to protect the hardware */

   if (CHAnnel_status.comms_relay_state == ORPGRED_COMMS_RELAY_UNASSIGNED) {
      if (CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING) {

          if (err_start_time == 0) {
             LE_send_msg (GL_STATUS | GL_ERROR, 
                          "Control of the comms relay has been lost");
             err_start_time = time (NULL);
          }
          
             /* after a short time delay, attempt to regain control of the 
                comms relay */

          if (time (NULL) > (err_start_time + REACQUISITION_DELAY_TIME)) {
             DIO_reset_dos ();
             msleep (1000);
             DIO_acquire_comms_relay ();
          }
          reacquisition_in_progress = MNGRED_TRUE;
      }
   } else {

      if ((reacquisition_in_progress == MNGRED_TRUE) &&
          (CHAnnel_status.comms_relay_state == ORPGRED_COMMS_RELAY_ASSIGNED)) {
             reacquisition_in_progress = MNGRED_FALSE;
             LE_send_msg (GL_STATUS, "Comms relay re-acquired");
             err_start_time = 0;
             last_time_relay_reacquired = time (NULL);
             DIO_reset_dos ();
      }
         /* wait some time period before resetting the err timer so short duration
            relay switching is prevented...if this problem ever occurs */

      if (time(NULL) > (last_time_relay_reacquired + TIMER_RESET_DELAY))
         err_start_time = 0;
   }

      /* if mrpg doesn't know this channel should be in the active state,
         then command it to active */

   if (ORPGMGR_get_RPG_states (&rpg_state) == 0) {
      ++delay_cnt;
      if ((rpg_state.active != MRPG_ST_ACTIVE) && (delay_cnt / MNGRED_CHANL_CMD_DELAY)) { 
         if (ORPGMGR_send_command (MRPG_ACTIVE) == -1)
             LE_send_msg (GL_ERROR,
                          "Failure sending \"active\" cmd to mrpg");
      } else
         delay_cnt = 0;
   }
   
   if (Xsition_type != MNGRED_CHANL_NOT_IN_XSITION)
      Xsition_type = MNGRED_CHANL_NOT_IN_XSITION;

   return;
}


/********************************************************************************

    Description: This routine processes the channel when in the inactive state.

          Input:

         Output:

         Return:

        Globals: CHAnnel_link_state      - see mngred_globals.h & orpgred.h
                 RDA_download_required   - see mngred_globals.h
                 REDundant_channel_state - see mngred_globals.h
                 Switchover_pending      - see file scope global section
 
          Notes:
 
 ********************************************************************************/

void PS_process_inactive_state ()
{
   static int delay_cnt = 0;
   Mrpg_state_t rpg_state;

      /* download any RDA commands pending */

   if (RDA_download_required)
       DC_process_download_commands ();

      /* if the redundant channel did not go active within the 
         "switchover" time limit, write an alarm msg */

   if ((REDundant_channel_state == ORPGRED_CHANNEL_ACTIVE)  &&
       (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)      &&
       (Switchover_pending == MNGRED_TRUE))
           Switchover_pending = MNGRED_FALSE;
   else if ((Switchover_pending == MNGRED_TRUE)  &&
           ((time (NULL) - Switchover_start_time) > SWITCHOVER_TIME_LIMIT))
   {
      if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)
         LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
                "%s Redundant channel switchover time limit exceeded",
                MNGRED_WARN);
      Switchover_pending = MNGRED_FALSE;
   }

      /* if mrpg doesn't know this channel should be in the inactive state,
         then command it to inactive */

   if (ORPGMGR_get_RPG_states (&rpg_state) == 0) {
      ++delay_cnt;
      if ((rpg_state.active != MRPG_ST_INACTIVE) && (delay_cnt / MNGRED_CHANL_CMD_DELAY)) { 
         if (ORPGMGR_send_command (MRPG_INACTIVE) == -1)
             LE_send_msg (GL_ERROR,
                          "Failure sending \"inactive\" cmd to mrpg");
      } else
         delay_cnt = 0;
   }
   
   if (Xsition_type != MNGRED_CHANL_NOT_IN_XSITION)
      Xsition_type = MNGRED_CHANL_NOT_IN_XSITION;

   return;
}


/********************************************************************************

    Description: This routine manages the channel state when transitioning
                 to Active

          Input:

         Output:

         Return:

        Globals: CHAnnel_link_state      - see mngred_globals.h & orpgred.h
                 CHAnnel_state           - see mngred_globals.h & orpgred.h
                 RDA_download_required   - see mngred_globals.h
                 REDundant_channel_state - see mngred_globals.h
 
          Notes:
 
 ********************************************************************************/
 
void PS_xsition_to_active ()
{
   int return_value;                  /* return value from function calls */
   static time_t xsition_start_time;         /* transition-to-active start time */
   time_t current_time;               /* current time */
      /* flag to prevent alarm msg from being posted more than once */
   static int switchover_channel_error = MNGRED_FALSE;
   int xsition_time_limit_exceeded = MNGRED_FALSE;

      /* intialize the switchover timeout timers */
      
   current_time = time (NULL);

   if (Xsition_type != MNGRED_XSITION_TO_ACTIVE) {
      xsition_start_time = current_time;
      Xsition_type = MNGRED_XSITION_TO_ACTIVE;
      LE_send_msg (MNGRED_OP_VL, 
                   "Channel state change commanded: Transition to Active");
   }

   xsition_time_limit_exceeded = ((current_time - xsition_start_time) >= 
                                   SWITCHOVER_TIME_LIMIT);

      /*  monitor the other RPG channel to go inactive only if the
          channel link is up */

   if ((CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP) &&
        (!xsition_time_limit_exceeded)) {

      if (REDundant_channel_state != ORPGRED_CHANNEL_INACTIVE) {
         return;
      }
   }

      /* if the other channel did not go inactive within the required 
         time limit, write an alarm msg */

   if ((CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)       && 
       (REDundant_channel_state != ORPGRED_CHANNEL_INACTIVE) &&
      xsition_time_limit_exceeded && !(switchover_channel_error)) {
      LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
           "%s Redundant channel switchover time limit exceeded",
            MNGRED_WARN);

      switchover_channel_error = MNGRED_TRUE;
   }

      /* acquire the comms relay */

   DIO_acquire_comms_relay ();

   msleep (1000);  /* give the hardware time to set the D/O */

      /*  if the comms relay was not acquired, set the appropriate 
          flags and exit (ie. do not go active) */

   if ((DIO_read_comms_relay_state ()) != ORPGRED_COMMS_RELAY_ASSIGNED) {
      if (!Comms_relay_alarm_set && xsition_time_limit_exceeded) {
         Comms_relay_alarm_set = MNGRED_TRUE;
         LE_send_msg (GL_STATUS | GL_ERROR, 
                      "%s Failed to acquire the NB/WB Comms Relay",
                      MNGRED_WARN_ACTIVE);
      }
      return;
   }

      /* if we made it this far, then channel switching succeeded */

   LE_send_msg (MNGRED_OP_VL, "Comms relay acquired");
      
     /* the comms relay is a latch and hold circuit, so reset the D/O */
        
   DIO_reset_dos ();

      /* clear the misc flags if needed */

   switchover_channel_error = MNGRED_FALSE;

   if (Comms_relay_alarm_set) {
      LE_send_msg (GL_STATUS | GL_ERROR, 
                   "%s Failed to acquire the NB/WB Comms Relay",
                   MNGRED_WARN_CLEAR);
      Comms_relay_alarm_set = MNGRED_FALSE;
   }

      /* send event to the ORPG to go active (ie. connect the 
         user comm lines) */

   if ((return_value = ORPGMGR_send_command (MRPG_ACTIVE)) == -1)
       LE_send_msg (GL_ERROR,
                    "Failure sending \"go-active\" cmd to mrpg");

   CHAnnel_state = ORPGRED_CHANNEL_ACTIVE;

      /* process the download commands that are dependent on the channel 
         state being active (i.e. VCP downloads, etc.) */

   if (RDA_download_required)
       DC_process_download_commands ();


      /* if RDA commands still pending, report the unsent commands then clear
         them out before reporting the channel state change */
   
   if (RDA_download_required)
      DC_clear_channel_cmds ();

   LE_send_msg (GL_STATUS, "Channel state changed: Active");

      /* update the channel status */

   CST_update_channel_status ();

      /* send channel status to other channel */

   if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)
        CST_transmit_channel_status ();

   return;
}


/********************************************************************************

    Description: This routine manages the channel state when transitioning
                 to Inactive

          Input:

         Output:

         Return:

        Globals: CHAnnel_link_state      - see mngred_globals.h & orpgred.h
                 CHAnnel_state           - see mngred_globals.h & orpgred.h
                 REDundant_channel_state - see mngred_globals.h
                 SENd_ipc_cmd            - see mngred_globals.h
                 Switchover_pending      - see file scope global section
                 Switchover_start_time   - see file scope global section
 
          Notes:
 
 ********************************************************************************/

void PS_xsition_to_inactive ()
{
   int ret = 0;                       /* value returned from function calls */
   int nb_comm_lines_status;          /* connect/disconnect status of the nb
                                         comm lines */
   time_t current_time;               /* current time */
   static int cmd_error_logged = MNGRED_FALSE; /* RPG cmd err previously logged flag */


   current_time = time (NULL);

      /* Send event to the ORPG to go inactive */

   if (Xsition_type != MNGRED_XSITION_TO_INACTIVE) {
       Switchover_start_time = current_time;
       Switchover_pending = MNGRED_TRUE;
       Xsition_type = MNGRED_XSITION_TO_INACTIVE;
       LE_send_msg (MNGRED_OP_VL, 
                    "Channel state change commanded:  Transition to Inactive");

       if ((ret = ORPGMGR_send_command (MRPG_INACTIVE)) == -1)
           LE_send_msg (GL_ERROR,
                 "Failure sending \"go_inactive\" cmd to the RPG (err %d)",
                 ret);
   }

      /* get the status of the NB comm lines */

   nb_comm_lines_status = Get_nb_comm_lines_status ();

      /* set this channel to inactive when all NB/User lines have been
         disconnected, the comms relay is no longer assigned to this channel,
         or the switchover time limit has been exceeded */

   if ((CHAnnel_status.comms_relay_state == ORPGRED_COMMS_RELAY_ASSIGNED) &&
/*   if ((DIO_read_comms_relay_state () == ORPGRED_COMMS_RELAY_ASSIGNED) && */
      ((current_time - Switchover_start_time) < SWITCHOVER_TIME_LIMIT)) {
         if (nb_comm_lines_status == 0)
               CHAnnel_state = ORPGRED_CHANNEL_INACTIVE;
   } else {
      CHAnnel_state = ORPGRED_CHANNEL_INACTIVE;

      if (nb_comm_lines_status > 0)
          LE_send_msg (MNGRED_OP_VL,
               "%d NB line(s) failed to disconnect before switching to inactive",
               nb_comm_lines_status);
   }

   if (CHAnnel_state == ORPGRED_CHANNEL_INACTIVE)
   {
         /* log an alarm if any State Data updates are pending */

      if ((DC_are_cmds_pending (MNGRED_STATE_DAT) == MNGRED_TRUE) ||
          (MLT_are_updates_pending (MNGRED_STATE_DAT) == MNGRED_TRUE))
            LE_send_msg (GL_ERROR | GL_STATUS,
                         "Failed to update State Data on other channel" );

         /* log an alarm if any Adaptation Data updates are pending */

      if ((DC_are_cmds_pending (MNGRED_ADAPT_DAT) == MNGRED_TRUE) ||
          (MLT_are_updates_pending (MNGRED_ADAPT_DAT) == MNGRED_TRUE))
            LE_send_msg (GL_ERROR | GL_STATUS,
                         "Failed to update Adaptation Data on other channel");

         /* clear any pending IPC channel commands */

      if (SENd_ipc_cmd == MNGRED_TRUE)
         DC_clear_channel_cmds ();

         /* clear any pending LB updates */

      MLT_clear_update_required_flags ();

      LE_send_msg (GL_STATUS, "Channel state changed: Inactive");

      if ((REDundant_channel_state == ORPGRED_CHANNEL_ACTIVE)  &&
          (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP))
               Switchover_pending = MNGRED_FALSE;

/*      cmd_error_logged = MNGRED_FALSE; */
      
         /* update the channel status */

      CST_update_channel_status ();

         /* send channel status to other channel */
      
      if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)
          CST_transmit_channel_status ();
   }

   return;
}


/********************************************************************************

    Description: This routine determines the number of NB/user comms lines 
                 connected

          Input:

         Output:

         Return: line_count - return -1 on error; otherwise,
                              return # of comm lines not disconnected

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Get_nb_comm_lines_status ()
{
   int i;
   int line_count = 0;            /* count of nb comm lines not disconnected */
   int number_nb_comm_lines;      /* the # of nb comm lines available */
   int buffer_size;               /* size of the line status msg */
   int ret;                       /* function call ret value */
   int lbd;                       /* the status msg lbd */
   Prod_user_status *line_status; /* the status of each nb comm line */

      /* set the buffer size of a single status msg */

   buffer_size = sizeof (Prod_user_status);

  line_status = malloc (buffer_size);

  if (line_status == NULL)
  {
     LE_send_msg (GL_ERROR, "Get_nb_comm_lines_status: malloc fail (errno %d)",
                  errno);
     return (-1);
  }

      /* setup the window to read only the latest status in the product 
         status msg */

   lbd = ORPGDA_lbfd (ORPGDAT_PROD_USER_STATUS);

   if (lbd < 0)
   {
      LE_send_msg (GL_ERROR, 
            "Failure getting lbd to read NB lines state (err %d)",
            lbd);
      free (line_status);
      return (-1);
   }

      /* set the window size to read just the structure that contains the line
         state */

   ret = LB_read_window (lbd, 0, buffer_size);

   if (ret < 0)
   {
      LE_send_msg (GL_ERROR, 
                   "Failure setting msg window size to read NB lines state");
      ORPGDA_close (ORPGDAT_PROD_USER_STATUS);
      free (line_status);
      return (-1);
   }
      
      /* get the # nb comm lines available and traverse the list checking 
         the disconnect state of all lines */

   number_nb_comm_lines = ORPGNBC_n_lines ();

   for (i = 0; i < number_nb_comm_lines; i++)
   {
      ret = ORPGDA_read (ORPGDAT_PROD_USER_STATUS, line_status, buffer_size,
                         (LB_id_t) i);

         /* continue if this line is not implemented */

      if ((ret == LB_NOT_FOUND) || (line_status->enable == '0'))
         continue;

         /* if a line is found not to be disconnected, increment the line
            count */   

      if (line_status->link != US_DISCONNECTED)
      {
         ++line_count;
         LE_send_msg (MNGRED_DEBUG_VL, 
                      "Get_nb_lines: NB line %d is not disconnected", i);
      }
   }

   ORPGDA_close (ORPGDAT_PROD_USER_STATUS);

   free (line_status);

   return (line_count);
}
