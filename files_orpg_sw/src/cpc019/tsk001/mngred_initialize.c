/********************************************************************************
 
    file:  mngred_initialize.c

    Description:  This file contains the routines used to intialize the 
                  redundant manager's redundant channel configuration. 
                  Initialization consists of four main parts:

                  1.  opening this channel's linear buffers the redundant manager is 
                      interested in
                      -- any error incurred opening a local LB is a fatal error
                         and will cause the process to abort.
                  2.  opening the redundant channel's linear buffers
                      -- if a redundant channel LB can not be opened, it's LB
                         descriptor is set to "UNINITIALIZED" in the LB 
                         lookup table
                  3.  constructing the linear buffer lookup table. 
                      -- the local channel/redundant channel paired linear buffer
                         information is stored in this table along with their data_id, 
                         relevant message ids, etc. Any error updating the
                         lookup table is fatal and will cause the process
                         to abort
                  4.  registering the relevant local LBs for LB notification
                      - registration failures are fatal and will cause
                        the process to abort
                  5.  initializing this channel to the required startup/restart 
                      state

 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/14 20:58:33 $
 * $Id: mngred_initialize.c,v 1.20 2013/05/14 20:58:33 steves Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

#include <sys/types.h>
#include <sys/utsname.h>
#include <strings.h>

#include <mngred_globals.h>
#include <malrm.h>
#include <misc.h>
#include <mrpg.h>
#include <orpgmisc.h>
#include <siteadp.h>
#include <orpgsite.h>

    /* Define the configuration file for the redundant manager.
       Also, define the different types of updates the redundant manager
       performs */
#define   REDUNDANT_CONFIG_FILE    "redundant.cfg"     /* redun mgr config file */
#define   STATE_DATA               "State_data:"       /* config file key */
#define   MNGRED_ADAPTATION_DATA   "Adaptation_data:"  /* config file key */
#define   SWITCHOVER               "Switchover:"       /* config file key...not 
                                                          currently used */
#define   RDA_CONNECT_WAIT_TIME    20                  /* max time to wait for RDA to 
                                                          connect during startup */


    /* time interval in seconds for the alarm service to call the redundant manager */
#define   UPDATE_TIME_INTERVAL      2
    /* alarm message number used when registering the alarm as an interval timer */
#define   MALRM_CHECK_CHANNEL_LINK  1


/* file scope global variables */

static char *Redundant_hostname;    /* host name of the redundant channel */
static char *System_config_file;    /* the name of the current system 
                                       configuration file */

/* local function prototypes */

static void Channel_restart ();
static void Channel_startup ();
static void Initialize_redundant_channel_msg ();
static char *Get_UIT_hostnames (void);  /* test routine */
static int  Get_hostnames (void);
static int Initialize_lbs (int dataid, int *local_lb_fd, int *redun_lb_fd,
                           char *redundant_lb_path);
static int  Populate_lookup_table (char *lb_type);
static int  Register_cmd_msg_lb (void);
static int  Register_interval_timer (void);
static int  Register_redun_channel_msgs_lb (void);
static void Suspend (int delay_time);


/********************************************************************************

    Description: This routine initializes the channel state on RPG restart

          Input:

         Output:

         Return:

        Globals:
 
          Notes: This routine is currently not used. All restart functions are 
                 performed in the channel startup routine.
 
 ********************************************************************************/

void Channel_restart ()
{
   int ret;

      /* read the last known state of this channel */

   LE_send_msg (MNGRED_OP_VL, "Channel \"Restart\" commencing");

   ret = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS, &CHAnnel_status,
                      sizeof (Channel_status_t),
                      ORPGRED_CHANNEL_STATUS_MSG);

   CST_print_channel_status ("Previous Channel Status:");

   if (ret < 0)
   {
      LE_send_msg (MNGRED_OP_VL, "Error reading lb_id %d, msg_id %d during RPG restart",
                   ORPGDAT_REDMGR_CHAN_MSGS, ORPGRED_CHANNEL_STATUS_MSG);
      LE_send_msg (MNGRED_OP_VL, "Channel initialization for restart failed");
      return;
   }

   if (CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING)
   {
      Comms_relay_state_t relay_state;      

         /* if the comms relay is unassigned, try to acquire the relay */

      relay_state = DIO_read_comms_relay_state ();
      
      if (relay_state == ORPGRED_COMMS_RELAY_UNASSIGNED)
         DIO_acquire_comms_relay ();

         /* if the relay is not acquired, set this channel to inactive. */

      if (relay_state != ORPGRED_COMMS_RELAY_ASSIGNED)
      {
         CHAnnel_state = ORPGRED_CHANNEL_INACTIVE;  /* this may not be right */
      }
      else
      {
         ret = ORPGMGR_send_command (MRPG_ACTIVE);
         
         if (ret < 0)
            LE_send_msg (MNGRED_OP_VL, 
                  "Failed to cmd control_rpg to active during Restart");
      }
      CHAnnel_state = ORPGRED_CHANNEL_ACTIVE;
   }
   return;
}


/********************************************************************************

    Description: This routine initializes the channel state on RPG startup

          Input:

         Output:

         Return:

        Globals: CHAnnel_link_state     - see mngred.h and orpgred.h
                 CHAnnel_status         - see mngred.h and orpgred.h
                 INTerval_timer_expired - see mngred_globals.h
          Notes:
 
 ********************************************************************************/

void Channel_startup ()
{
   int ret;                                 /* function calls return value */
   Previous_channel_state_t previous_state; /* previous channel state from last
                                               system shutdown */
   int rpg_bld_num;
   int adapt_dat_version_num;

   LE_send_msg (MNGRED_OP_VL, "Channel Startup commencing");

      /* Ensure that the redundant channel status msg exists */

   Initialize_redundant_channel_msg ();

      /* set the channel link state to "up" and activate the channel link
         checking mechanism */

   CHAnnel_link_state = ORPGRED_CHANNEL_LINK_UP; 
   CLS_check_channel_link ();

      /* update the channel version numbers with the latest version information */

   rpg_bld_num = ORPGMISC_RPG_build_number();
   adapt_dat_version_num = ORPGMISC_RPG_adapt_version_number();

   LE_send_msg (GL_INFO, "RPG Bld #: %d;  Adapt data version #: %d", rpg_bld_num,
                adapt_dat_version_num);


      /* read the last known state of this channel */

   ret = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS, &previous_state,
                      sizeof (Previous_channel_state_t),
                      ORPGRED_PREVIOUS_CHANL_STATE);

      /* if the sytem is cold starting, initialize the channel status 
         information and return */

   if (ret <= 0)
   {
      int temp;

      LE_send_msg (MNGRED_OP_VL, "Channel startup is a \"Cold\" start");
      CHAnnel_status.rpg_channel_state = ORPGRED_CHANNEL_INACTIVE;
      CHAnnel_status.rda_rpg_wb_link_state = MNGRED_UNINITIALIZED;
      CHAnnel_status.rda_control_state = ORPGRED_RDA_CONTROL_STATE_UNKNOWN;
      CHAnnel_status.stat_msg_seq_num = 0;

         /* update the channel version numbers with the latest version information */

      CHAnnel_status.rpg_sw_version_num = rpg_bld_num;
      CHAnnel_status.adapt_data_version_num = adapt_dat_version_num;

      temp = (int) ORPGRED_adapt_dat_time(ORPGRED_MY_CHANNEL);

      if (temp == -1)
         CHAnnel_status.adapt_dat_update_time = 0;
      else
         CHAnnel_status.adapt_dat_update_time = (time_t) temp;

      if ((ret != LB_TO_COME) && (ret != LB_NOT_FOUND) && ( ret != 0))
         LE_send_msg (MNGRED_OP_VL, 
           "Error initializing channel status from previous state (err %d)",
           ret);

      LE_send_msg (MNGRED_OP_VL, "Channel State: Inactive");

      if (CST_update_channel_status () < 0)
          LE_send_msg (MNGRED_OP_VL, "Failure updating channel status msg");

      return;
   }
   
   LE_send_msg (MNGRED_OP_VL, "Channel startup is a \"Warm\" start");

      /* re-initialize the channel status from the channel status msg */

   ret = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS, &CHAnnel_status,
                      sizeof (Channel_status_t),
                      ORPGRED_CHANNEL_STATUS_MSG);

      /* update the channel version numbers with the latest version information */

   CHAnnel_status.rpg_sw_version_num = rpg_bld_num;
   CHAnnel_status.adapt_data_version_num = adapt_dat_version_num;

     /* print the previous channel state */

   CST_print_previous_state (previous_state);

   Suspend (4);
   CLS_check_channel_link ();

      /* if the previous state of this channel was active, let's 
         see if we can re-establish the Active/Controlling state */
      
   if (previous_state.rpg_channel_state == ORPGRED_CHANNEL_ACTIVE)
   {
      int i = 0;

         /* reset the RDA control state */

      CHAnnel_status.rda_control_state = ORPGRED_RDA_CONTROL_STATE_UNKNOWN;

         /* wait a while to see if the RDA re-connects */

      while (i < RDA_CONNECT_WAIT_TIME) 
      {
            /* update the channel status */

         CST_update_channel_status ();

            /* if the RDA-RPG WB link is connected and this channel's
               RDA is controlling, then break out of the loop and 
               finish initialization...the main loop will take it from here */

         if ((CHAnnel_status.rda_rpg_wb_link_state == RS_CONNECTED)      &&
             (CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING))
               break;

            /* if the interval timer has expired, check the state of 
               the channel link */

         if (INTerval_timer_expired == MNGRED_TRUE)
              CLS_check_channel_link ();
         
            /* suspend for a while, then check again */

         ++i;
         Suspend (1);
      }
   }
   return;
}


/********************************************************************************

    Description:  This routine ensures that the redundant channel status
                  msg exists

          Input:

         Output:

         Return:

        Globals: REDundant_channel_state - see mngred_globals.h
 
          Notes:
 
 ********************************************************************************/

static void Initialize_redundant_channel_msg ()
{
   Channel_status_t redun_status_msg; /* the redundant channel status msg */
   int ret;                           /* function call return value */

   ret = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &redun_status_msg,
                      sizeof (Channel_status_t), ORPGRED_REDUN_CHANL_STATUS_MSG);

   if (ret <= 0)
   {
      redun_status_msg.rpg_channel_number = -1;
      redun_status_msg.rpg_state = -1;
      redun_status_msg.rpg_mode = -1;
      redun_status_msg.rpg_channel_state = -1;
      redun_status_msg.rpg_hostname[0] = '\0';
      redun_status_msg.rda_rpg_wb_link_state = -1;
      redun_status_msg.rda_status = -1;
      redun_status_msg.rda_operability_status = -1;
      redun_status_msg.rda_control_state = -1;
      redun_status_msg.comms_relay_state = ORPGRED_COMMS_RELAY_UNKNOWN;
      redun_status_msg.rpg_rpg_link_state = ORPGRED_CHANNEL_LINK_DOWN;
      redun_status_msg.adapt_dat_update_time = 0;
      redun_status_msg.rpg_configuration = -1;
      redun_status_msg.stat_msg_seq_num = -1;
   }

   redun_status_msg.rpg_sw_version_num = 0;
   redun_status_msg.adapt_data_version_num = 0;
      
   ret = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &redun_status_msg,
                       sizeof (Channel_status_t),
                       ORPGRED_REDUN_CHANL_STATUS_MSG);

   if (ret < 0)
       LE_send_msg (MNGRED_OP_VL, 
             "Error initializing the redundant channel status msg");


   REDundant_channel_state = redun_status_msg.rpg_channel_state;

   return;
}


/********************************************************************************

    Description:  This routine performs all the intialization services for
                  this channel.

          Input:

         Output:

         Return:

        Globals: CHAnnel_state          - see mngred_globals.h and orpgred.h
                 CHAnnel_status         - see mngred_globals.h and orpgred.h
                 CONfiguration_type     - see mngred_globals.h 
                 Redundant_hostname     - see file scope global section
                 RDA_download_required  - see mngred_globals.h
                 INTerval_timer_expired - see mngred_globals.h
                 SENd_ipc_cmd           - see mngred_globals.h
                 System_config_file     - see file scope global section
 
          Notes:
 
 ********************************************************************************/

int IC_initialize_channel ()
{
   int  return_val;   /* function call return value */
   char *tmp_char_p;  /* temp char pointer to malloc'ed memory */


      /* set the timeout value for the LB_ services depending on whether we're
         a test environment or an operational environment. if the timeout value
         expires before the LB_ request is serviced, then the LB service returns 
         an error condition */ 

   if (!ORPGMISC_is_operational())
      RMT_time_out (3);
   else
      RMT_time_out (6);

   CHAnnel_state = ORPGRED_CHANNEL_INACTIVE; 
   RDA_download_required = MNGRED_FALSE;
   INTerval_timer_expired = MNGRED_FALSE;
   SENd_ipc_cmd = MNGRED_FALSE;
   memset (&CHAnnel_status, 0, sizeof (CHAnnel_status));

      /* set the RPG channel state to Inactive */

   if (ORPGMGR_send_command (MRPG_INACTIVE) == -1)
       LE_send_msg (GL_ERROR, "Failure sending \"Inactive\" cmd to mrpg");
   else
       LE_send_msg (GL_INFO, "Setting channel state to \"Inactive\"");


   ORPGDA_write_permission (ORPGDAT_REDMGR_CHAN_MSGS);

   return_val = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &CHAnnel_status,
                     sizeof (Channel_status_t), ORPGRED_CHANNEL_STATUS_MSG);

   if (return_val < 0)
       LE_send_msg (GL_ERROR, "Error reading this channel's channel status msg");
   else {
      CHAnnel_status.rpg_channel_state = ORPGRED_CHANNEL_INACTIVE;

      ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &CHAnnel_status,
                    sizeof (Channel_status_t), ORPGRED_CHANNEL_STATUS_MSG);
   }

      /* if this is a NWS redundant configuration, register for the
         orpg generated commands (commands from the HCI) and return;
         otherwise, intialize the channel for a FAA redundant configuration */

   if (CONfiguration_type == ORPGSITE_NWS_REDUNDANT)
   {
      if (Register_cmd_msg_lb () < 0)
      {
         LE_send_msg (GL_ERROR, "Error registering the command message LB");
         return (-1);
      }
      else
         return (0);
   }

      /* get this channel's channel number */

   CHAnnel_status.rpg_channel_number = ORPGSITE_get_int_prop(ORPGSITE_CHANNEL_NO);
 
   if (ORPGSITE_error_occurred ())
   {
      ORPGSITE_log_last_error (GL_ERROR, ORPGSITE_REPORT_DETAILS | ORPGSITE_CLEAR_ERROR);
      LE_send_msg (GL_ERROR, "Error reading the site Adaptation data channel number");
      return (-1);
   }
 
      /* set this channel's sender id. this id is used to determine if
         a callback was generated by some process on this channel writing to a 
         LB we're registered for, or if the callback was generated by the redundant
         manager process on the other channel writing to an LB on this channel */
   
   return_val = LB_NTF_control (LB_NTF_SET_SENDER_ID,
                               (int) CHAnnel_status.rpg_channel_number); 

   if (return_val != LB_SUCCESS)
   {
      LE_send_msg (GL_ERROR, "Error setting this channel's Sender id (err: %d)",
      return_val);
      return (-1);
   }
  
      /* Save a copy of the current CS configuration name, as we'll
         need to restore that when we leave this routine */

   tmp_char_p = CS_cfg_name(NULL);

   if (strlen(tmp_char_p) > 0) 
   {
      System_config_file = malloc(strlen(tmp_char_p) + 1);

      if (System_config_file != NULL) 
      {
         (void) strncpy(System_config_file, (const char *) tmp_char_p,
                        strlen(tmp_char_p) + 1);
      }
      else 
      {
         LE_send_msg(GL_ERROR, "malloc(temporary cfg name) Failed");
         return(-1);
      }
   }
   else 
   {
      System_config_file = malloc(strlen("") + 1);

      if (System_config_file != NULL) 
      {
          (void) strncpy(System_config_file, (const char *) "",
                         strlen("") + 1);
      }
      else 
      {
         LE_send_msg(GL_ERROR, "malloc(temporary cfg name) Failed");
         return(-1);
      }
   }

      /* change the current config file name to the redundant 
         configuration file and specify the comment character 
         used in the config file */

   (void) CS_cfg_name(REDUNDANT_CONFIG_FILE);

   CS_control (CS_COMMENT | '#');

      /* get the channels host names...the host name depends
         on whether we are in a test environment or an operational environment */

   Redundant_hostname = '\0';

   if (!ORPGMISC_is_operational())
      Redundant_hostname = Get_UIT_hostnames();
   else
      Get_hostnames();

   if (Redundant_hostname == NULL)
   {
      LE_send_msg (MNGRED_OP_VL,
            "Fatal Error: Can not determine redundant channel's hostname");
       return (-1);
   }

      /* open the LB lookup table */

   if (MLT_open_lookup_table () < 0)
   {
      LE_send_msg (MNGRED_OP_VL, 
                 "Fatal Error: Unable to initialize the LB lookup table");
      ORPGTASK_exit (EXIT_FAILURE);
   }

   LE_send_msg (MNGRED_OP_VL, "LB table entries:");

      /* populate the lookup table with the state data LBs that will 
         be managed by the redundant manager */

   if (Populate_lookup_table (STATE_DATA) < 0)
   {
      LE_send_msg (GL_ERROR, 
            "Error populating lookup table from the %s cfg file", 
            REDUNDANT_CONFIG_FILE);
      return (-1);
   }

      /* populate the lookup table with the adaptation data LBs that will 
         be managed by the redundant manager */

   if (Populate_lookup_table (MNGRED_ADAPTATION_DATA) < 0)
   {
      LE_send_msg (GL_ERROR, 
            "Error populating lookup table from the %s cfg file", 
            REDUNDANT_CONFIG_FILE);
      return (-1);
   }

      /* set the name back to the system's current cfg file name */

   (void) CS_cfg_name(System_config_file);

      /* register the redundant manager channel messages LB */

   if (Register_redun_channel_msgs_lb () < 0)
   {
      LE_send_msg (GL_ERROR, 
              "Error registering the redundant manager's channel msgs LB"); 
      return (-1);
   }
   
      /* register the orpg-to-redundant mgr command message LB */

   if (Register_cmd_msg_lb () < 0)
   {
      LE_send_msg (GL_ERROR, "Error registering the command message LB");
      return (-1);
   }

      /* initialize the DIO module */

   if ((return_val = DIO_init_dio_module ()) < 0)
      LE_send_msg (GL_ERROR, "Error initializing the DIO module");
   else
      LE_send_msg (MNGRED_OP_VL, "DIO socket initialized");

       /* register and set the channel status update timer */

   if (Register_interval_timer () < 0)
   {
      LE_send_msg (GL_ERROR, "Error registering the interval timer");
      return (-1);
   }

      /* bring the channel up in the last known state */

   Channel_startup ();

      /* if the channel link is down, ensure all the redundant channel lbds
         are uninitialized */

   if (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_DOWN)
      MLT_reset_redun_ch_lbds ();

      /* ensure the Redundant Channel Error alarm is cleared */

   WCD_clear_chan_err_alarm ();

   LE_send_msg (MNGRED_OP_VL, "Initialization completed...");

      /* close the redundant configuration file and set the name back 
         to the system's current cfg file name */

   (void) CS_cfg_name(REDUNDANT_CONFIG_FILE);
   CS_control (CS_CLOSE | CS_DELETE);
   (void) CS_cfg_name(System_config_file);
   free (System_config_file);
   free (Redundant_hostname);
   
   return (0);
}


/********************************************************************************

    Description: This routine populates the LB lookup table with the redundant 
                 relevant LB, message id info identified in the redundant 
                 configuration file

          Input: lb_type - specifies the type LB to process (state data 
                           or adpatation data)

         Output:

         Return: 0 on success; -1 on failure

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Populate_lookup_table (char *lb_type)
{
   int return_val;    /* function call return value */
   int id;            /* msg id read from the config file */
   int data_id;       /* data id of the LB to add to the lookup table */

      /* reset the search pointer to start the search at the beginning
         of the configuration file */

   CS_level (CS_TOP_LEVEL);
   
      /* locate the LB type section in the configuration file */

   if ((return_val = CS_entry (lb_type, 0, 0, (char *) NULL)) < 0)
   {
      LE_send_msg (MNGRED_OP_VL, "Error locating %s section in file %s (err %d)",
                   lb_type, REDUNDANT_CONFIG_FILE, return_val);
      return (-1);
   }

   if ((return_val = CS_level (CS_DOWN_LEVEL)) != 0)
   {
      LE_send_msg (MNGRED_OP_VL,
            "Error moving down one level in %s section of file %s (err %d)",
            lb_type, REDUNDANT_CONFIG_FILE, return_val);
      return (-1);
   }

      /* find the first data id for this LB type */

   if ((return_val = CS_entry (CS_THIS_LINE, 0 | CS_INT, 0, (void *) &data_id)) < 0)
   {
      LE_send_msg (GL_ERROR,
        "Valid data_id not found in config file \"%s\" for data type %s (err %d)",
           REDUNDANT_CONFIG_FILE, lb_type, return_val);
      return (-1);
   }

      /* process all the data ids for this data type */

   while (return_val >= 0)
   {
      int     local_lbd;   /* local channel LB descriptor */
      int     redun_lbd;   /* redundant channel LB descriptor */
      int     update_type; /* specifies type of update for this LB */
      LB_id_t msg_id;      /* the msg ids to add to the lb lookup table */
      char    redun_lb_path[MNGRED_MAX_NAME_LENGTH]; /* redundant channel LB path */

         /* specify the update type for this lb type */

      if (strcmp(lb_type, STATE_DATA) == 0)
         update_type = MNGRED_TRANSFER_STATE_DATA;
      else if (strcmp(lb_type, MNGRED_ADAPTATION_DATA) == 0)
         update_type = MNGRED_TRANSFER_ADAPT_DATA;
      else if (strcmp(lb_type,SWITCHOVER) == 0)
         update_type = MNGRED_TRANSFER_AT_SWITCHOVER;
      else
      {
         LE_send_msg (GL_ERROR, 
                     "Unable to specify the update type for lb_type %s", lb_type);
         return (-1);
      }

         /* set the configuration file to access back to the system config file
            so we can extract the LB info of interest to pouplate the lookup table
            with */

      (void) CS_cfg_name(System_config_file);

         /*  initialize the local and redundant channel LBs for this
             data id */

      return_val = Initialize_lbs (data_id, &local_lbd, &redun_lbd, 
                                   (char *) &redun_lb_path);

      if (return_val != 0)
      {
         LE_send_msg (GL_ERROR, "Initialize lbs failed");
         return (-1);
      }

         /* set the configuration file back to the redundant config file
            to continue populating the lookup table */

      (void) CS_cfg_name(REDUNDANT_CONFIG_FILE);

         /* move down one level to the msg ids in the redundant config file */

      CS_level (CS_DOWN_LEVEL); 

         /* find the first message id for this LB */

      if ((return_val = CS_entry (CS_THIS_LINE, 0 | CS_INT, 0, (void *) &id)) < 0)
      {
         LE_send_msg (GL_ERROR,
           "Valid msg_id not found in config file \"%s\" for data_id %d (err %d)",
           REDUNDANT_CONFIG_FILE, data_id, return_val);
         return (-1);
      }
   
      msg_id = (LB_id_t) id;

         /* add each message id as a separate entry in the LB lookup table */

      do
      {
         MLT_add_table_entry (data_id, local_lbd, redun_lbd, msg_id, 
                              redun_lb_path, update_type);

            /* push the data_id onto the stack for registration */

         return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) data_id);

         if (return_val < 0)
         {
            LE_send_msg (GL_ERROR,
               "Failed to push dataid %d onto stack for registration (err %d)",
               data_id, return_val); 
            return (-1);
         }

            /* if the redundant config file msg_id == -1, set the msg_id to LB_ANY */

         if (msg_id == -1)
             msg_id = LB_ANY;
      
            /* register this message for callback */

         if (strcmp(lb_type, STATE_DATA) == 0)
            return_val = LB_UN_register (local_lbd, msg_id, 
                            PC_process_on_update_event);
         else if (strcmp(lb_type, MNGRED_ADAPTATION_DATA) == 0)
            return_val = LB_UN_register (local_lbd, msg_id, 
                            PC_process_adapt_dat_update_event);

         if (return_val < 0)
         {
            LE_send_msg (GL_ERROR,
               "Failed to register local LB %s, msg id %d (err = %d)",
               ORPGCFG_dataid_to_path (data_id, ""), msg_id, return_val); 
            return (-1);
         }

         LE_send_msg (MNGRED_TEST_VL,
             "%s data_id %d, msg_id %d, lbd %d registered for callback",
             lb_type, data_id, msg_id, local_lbd);

         return_val = CS_entry (CS_NEXT_LINE, 0 | CS_INT, 0, (void *) &id);

         if ((return_val < 0) && (return_val != CS_END_OF_TEXT))
         {
            LE_send_msg (GL_ERROR,
               "Error incrementing to next_line in config file (err %d)",
               return_val);
            return (-1);
         }
         else  /* assign the next msg id to process */
            msg_id = (LB_id_t) id;

      } while (return_val >= 0);

         /* increment up one level in the config file to pick up the 
            next data id */

      CS_level (CS_UP_LEVEL);

         /* find the next data id for this data type */

      return_val = CS_entry (CS_NEXT_LINE, 0 | CS_INT, 0, (void *) &data_id);

      if ((return_val < 0) && (return_val != CS_END_OF_TEXT))
      {
         LE_send_msg (GL_ERROR,
           " CS_entry error reading data id for data type %s (err %d)",
           lb_type, return_val);
         return (-1);
      }
   }

   return (0);
}


/********************************************************************************

    Description: This module opens the local and redundant channel LBs. The 
                 local LBs are the LBs that are read from and the redundant 
                 LBs are the LBs written to.

          Input: dataid -  data id of the local LB

         Output: local_lb_fd       - local channel LB descriptor   
                 redun_lb_fd       - redundant channel LB descriptor   
                 redundand_lb_path - redundant channel LB path   

         Return: 0 on success; -1 on failure

        Globals: Redundant_hostname - see file scope global section
 
          Notes:
 
 ********************************************************************************/
 
static int Initialize_lbs (int dataid, int *local_lb_fd, int *redun_lb_fd,
                           char *redundant_lb_path)
{
   char *lb_path;             /* path of the local LB */
   char *local_hostname_ptr;
  
      /* open the LB with read/write permissions */

   ORPGDA_write_permission (dataid);

      /* get the local LB descriptor */
   
   *local_lb_fd = ORPGDA_lbfd (dataid);
      
      /* get the local lb name */

   lb_path = ORPGDA_lbname (dataid);

   if (*local_lb_fd < 0)
   {
      LE_send_msg (GL_ERROR,
         "Failed to open local lb %s during initialization (err %d)", 
         lb_path, *local_lb_fd);
      return (-1);
   }

     /* construct the lb path for the redundant channel */

     /* 
      * ASSUMPTION -- IT'S ASSUMED THAT THE LINEAR BUFFER PATHS ARE IDENTICAL
      * ON BOTH RPGs. USING THIS ASSUMPTION FOR LB INITIALIZATION, THIS RPG
      * WILL READ THE LB PATHS FROM ITS OWN SYSTEM CONFIGURATION FILE, OPEN ITS
      * OWN LBS, THEN, ASSUMING THE PATH IS THE SAME ON THE REDUNDANT CHANNEL,
      * OPEN THE LBS ON THE REDUNDANT CHANNEL USING THE SAME PATH.
      *
      * IF THE SYSTEM CONFIGURATION FILES BETWEEN THE TWO RPGS EVER BECOME
      * DIFFERENT, EITHER THROUGH STATIC OR DYNAMIC RECONFIGURATION, THEN THIS
      * ASSUMPTION, AND CONSEQUENTLY, THE REDUNDANT MANAGER TASK WILL FAIL.
      *
      */
   
   if ((strlen (Redundant_hostname) + strlen (lb_path) + 3) > 
                                      MNGRED_MAX_NAME_LENGTH)
   {
      LE_send_msg (GL_ERROR,
         "Redundant channel LB path length is greater than max allowed (max = %d)",
         MNGRED_MAX_NAME_LENGTH);
      return (-1);
   }

   strcpy (redundant_lb_path, Redundant_hostname);

      /* if the lb_path returns the path without the local hostname prepended,
         then strip the name before constructing the redundant channel's
         qualified lb path; otherwise, use the complete local lb_path */

   if ((local_hostname_ptr = strstr (lb_path, (char *)&":")) == NULL) {
      strncat (redundant_lb_path, ":\0", 2);
      strcat (redundant_lb_path, lb_path);
   } else /* remove the local hostname from the lb path */
      strcat (redundant_lb_path, local_hostname_ptr);

      /* open the redundant LB */

   *redun_lb_fd = LB_open(redundant_lb_path, LB_WRITE, NULL);

      /* if lb fails to open, set the lb_fd to the uninitialized state...we'll
         open it later */

   if (*redun_lb_fd < 0)
   {
      LE_send_msg (MNGRED_TEST_VL,
         "Can not open redundant channel LB %s during initialization (err: %d)", 
         redundant_lb_path, *redun_lb_fd);
      *redun_lb_fd = MNGRED_UNINITIALIZED;
   }

   return (0);
}


/********************************************************************************

    Description: This routine constructs the channels hostnames
                 using this channel's host name as the reference

          Input:

         Output:

         Return: 0 on success; -1 on error

        Globals: CHAnnel_status     - see mngred_globals.h and orpgred.h
                 Redundant_hostname - see file scope global section
 
          Notes:
 
 ********************************************************************************/

static int Get_hostnames (void) {

   char *hostname_ptr;   /* pointer to the hostname string */

   if ((hostname_ptr = malloc (MAX_HOSTNAME_LEN)) == NULL) {
      LE_send_msg (GL_ERROR,
                   "malloc failure for hostname initialization");
      return (-1);
   }

   strcpy (hostname_ptr, "rpg\0");
   strcpy (CHAnnel_status.rpg_hostname, hostname_ptr);
   Redundant_hostname = hostname_ptr;

   
   if (CHAnnel_status.rpg_channel_number == 1) {
      strncat (&CHAnnel_status.rpg_hostname[strlen(hostname_ptr)], "1", 1);
      strncat (Redundant_hostname, "2", 1);
   } else {
      strncat (&CHAnnel_status.rpg_hostname[strlen(hostname_ptr)], "2", 1);
      strncat (Redundant_hostname, "1", 1);
   }
   
   LE_send_msg (MNGRED_OP_VL, "Local channel hostname: %s", 
                CHAnnel_status.rpg_hostname);
   LE_send_msg (MNGRED_OP_VL, "Redundant channel hostname: %s", 
                Redundant_hostname);

   return (0);
}


/********************************************************************************

    Description: This routine obtains the channels hostnames
                 from the redundant manager's configuration file.

          Input:

         Output:

         Return: pointer to hostname string on success; NULL on failure

        Globals: Redundant_hostname - see file scope global section
 
          Notes: This routine is used for testing purposes only
 
 ********************************************************************************/

static char *Get_UIT_hostnames (void)
{
   char buffer [MNGRED_MAX_NAME_LENGTH];  /* buffer to read the name into */
   int  return_val;                       /* function call return value */
   char *string_ptr;                      /* pointer to the hostname string */
   struct utsname uts;                    /* structure containing the hostname */


   if ((CHAnnel_status.rpg_channel_number != 1)  &&
       (CHAnnel_status.rpg_channel_number != 2))
   {
      LE_send_msg (GL_ERROR, 
        "Invalid channel number defined in the site configuration file (chan #: %d)",
            CHAnnel_status.rpg_channel_number);
      return ('\0');
   }

      /* find the "hostnames" section in the redundant config file */

   if ((return_val = CS_entry ("hostnames:", 0, 0, NULL)) < 0) 
   {
      LE_send_msg (GL_ERROR,
         "Error locating \"hostnames\" section in the redun config file (err %d)",
         return_val);
      return ('\0');
   }

   if ((return_val = CS_level (CS_DOWN_LEVEL)) != 0)
   {
      LE_send_msg (GL_ERROR,
         "Error moving down one level in \"hostnames:\" config file (err %d)",
         return_val);
      return ('\0');
   }

      /* get the hostname of the redundant (e.g. other) channel */

   if (CHAnnel_status.rpg_channel_number == 1)
      return_val = CS_entry ("channel_2:", 1, MNGRED_MAX_NAME_LENGTH-1, buffer);
   else 
      return_val = CS_entry ("channel_1:", 1, MNGRED_MAX_NAME_LENGTH-1, buffer);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR, 
            "Error reading redundant channel hostname in the config file (err %d)",
            return_val);
      return ('\0');
   }

   if (return_val > MNGRED_MAX_NAME_LENGTH - 1)
   {
      LE_send_msg (GL_ERROR,
            "hostname path length is > than max name length allowed (err %d)",
            return_val);
      return ('\0');
   }

   string_ptr = malloc (strlen (buffer) + 1);

   if (string_ptr == NULL)
   {
      LE_send_msg (GL_ERROR, "malloc allocation failure for hostname initialization");
      return ('\0');
   }

   strcpy (string_ptr, buffer);

   LE_send_msg (MNGRED_OP_VL, 
              "This channel's channel #: %d", CHAnnel_status.rpg_channel_number);
   
      /* update the channel status RPG node hostname */

   if (uname (&uts) == -1) {
      LE_send_msg (GL_ERROR, "initialize:uname - error reading host name");
      CHAnnel_status.rpg_hostname[0] = '\0';
   }
      /* update the channel status RPG node hostname */

   else if ((strlen(uts.nodename)) > MAX_HOSTNAME_LEN) {
      CHAnnel_status.rpg_hostname[0] = '\0';
      LE_send_msg (GL_ERROR, 
           "RPG hostname length error (len read: %d); (max len: %d)", 
           strlen(uts.nodename), MAX_HOSTNAME_LEN);
   }
   else
      strcpy (CHAnnel_status.rpg_hostname, uts.nodename);

   LE_send_msg (MNGRED_OP_VL, "This channel's hostname: %s", 
                CHAnnel_status.rpg_hostname);

   LE_send_msg (MNGRED_OP_VL, 
              "Redundant channel's hostname: %s", string_ptr);

   return (string_ptr);
}


/********************************************************************************

    Description: This routine registers the local LB that HCI, RMS and
                 Control_RDA commands are read from. If the configuration is NWS
                 redundant, then only the HCI will send commands

          Input:

         Output:

         Return: 0 on success; -1 on failure

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Register_cmd_msg_lb (void)
{
   int  command_msg_lbfd; /* the command message LB descriptor */
   int  return_value;     /* return value from the function calls */
   const char *lb_path;   /* pointer to the command msg LB path */
 
      /* open the command message LB */
   
   lb_path = ORPGCFG_dataid_to_path (ORPGDAT_REDMGR_CMDS, "");

   command_msg_lbfd = ORPGDA_lbfd (ORPGDAT_REDMGR_CMDS);

   if (command_msg_lbfd < 0)
   {
      LE_send_msg (MNGRED_OP_VL,
         "Failed to obtain descriptor for local ORPG cmd msg lb %s (err %d)",
         lb_path, command_msg_lbfd);
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_value = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) ORPGDAT_REDMGR_CMDS);

   if (return_value < 0)
   {
      LE_send_msg (GL_ERROR,
          "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
          ORPGDAT_REDMGR_CMDS, return_value); 
      return (-1);
   }

      /* register the local LB callback routine */

   return_value = LB_UN_register (command_msg_lbfd, LB_ANY, 
                              PC_process_on_demand_event);

      /* ensure the callback properly registered */

   if (return_value < 0)
   {
      LE_send_msg (GL_ERROR,
                   "Failed to register local ORPG cmd msg LB %s (err = %d)",
                   lb_path, return_value); 
      return (-1);
   }
   else
      LE_send_msg (MNGRED_OP_VL, "ORPG command message LB registered");

   return (0);
}


/********************************************************************************

    Description: This module registers the interval timer. The timer is used to
                 determine when to check the channel link and when to send channel
                 status to the other channel

          Input:

         Output:

         Return: 0 on success; -1 on failure

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Register_interval_timer (void)
{
   int return_val;    /* return value from the function calls */

      /* register the timer (we're using the alarm timer as an interval 
         timer) */

   return_val = MALRM_register (MALRM_CHECK_CHANNEL_LINK, 
                                (void (*)())PC_process_timer_expired_event);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
                   "Registering MALRM_CHECK_CHANNEL_LINK failed (err %d)",
                   return_val);
      return (-1);
   }

      /* set the time interval */

   return_val = MALRM_set (MALRM_CHECK_CHANNEL_LINK, MALRM_START_TIME_NOW,
                           UPDATE_TIME_INTERVAL);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
                "MALRM_SET failed setting the %d second interval timer (err %d)",
                UPDATE_TIME_INTERVAL, return_val);
      return (-1);
   }
   else
      LE_send_msg (MNGRED_OP_VL, "%d second Interval Timer registered", 
                   UPDATE_TIME_INTERVAL);

   return (0);
}


/********************************************************************************

    Description: This routine registers the redundant manager's channel 
                 messages

          Input:

         Output:

         Return: 0 on success; -1 on failure

        Globals:
 
          Notes:
 
 ********************************************************************************/

static int Register_redun_channel_msgs_lb (void)
{  
   int  local_lb_fd;  /* local channel LB descriptor */
   int  redun_lb_fd;  /* redundant channel LB descriptor */
   int  dataid;       /* redundant mgr IPC LB data id */
   int  return_val;   /* return value from the function calls */
   char lb_path[MNGRED_MAX_NAME_LENGTH]; /* path to the redun channel's redun mgr 
                                            IPC LB */


      /* get the data_id for the redundant manager channel messages LB */

   dataid = ORPGDAT_REDMGR_CHAN_MSGS;

      /*  initialize the local and redundant channel LBs for the redundant
          manager data id */

   return_val = Initialize_lbs (dataid, &local_lb_fd, 
                                 &redun_lb_fd, (char *) &lb_path);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, "Error initializing the lbs");
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
          "Failed to push data_id \"%d\" onto stack for registration (err %d)",
          dataid, return_val); 
      return (-1);
   }

      /* this section registers the local LB callback routine that processes 
         redundant channel messages */

      /* register the callback routine for redundant channel status
         messages */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_REDUN_CHANL_STATUS_MSG, 
                                PC_process_channel_msg);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redundant channel's status messages: %s (err = %d)",
           lb_path, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

       /* register the callback routine for the channel ping messsage  */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_PING_CHANNEL_LINK,
                                PC_process_channel_msg); 

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for the redun mgr IPC ping message: %s (err = %d)",
           lb_path, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

       /* register the callback routine for the channel ping response message  */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_PING_RESPONSE,
                                PC_process_channel_msg); 

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC ping response message: %s (err = %d)",
           lb_path, return_val); 
      return (-1);
   }

      /* this section registers the local LB callback routine that processes 
         redundant channel commands (commands received from the other channel) */

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

       /* register the callback routine for the Download Clutter Censor 
          Zones command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

       /* register the callback routine for the Select VCP command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_SELECT_VCP,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_SELECT_VCP, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

       /* register the callback routine for the Download VCP command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_DOWNLOAD_VCP,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_DOWNLOAD_VCP, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Update Spot Blanking
         command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_UPDATE_SPOT_BLANKING,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_UPDATE_SPOT_BLANKING, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Update Super Res
         command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_UPDATE_SR,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_UPDATE_SR, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Update CMD command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_UPDATE_CMD,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_UPDATE_CMD, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Update Adaptation Data Time
         command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_UPDATE_ADAPT_DATA_TIME,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_UPDATE_ADAPT_DATA_TIME, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Update AVSET command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_UPDATE_AVSET,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_UPDATE_AVSET, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Update SAILS command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_UPDATE_SAILS,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_UPDATE_SAILS, return_val); 
      return (-1);
   }

      /* push the data_id onto the stack for this registration */
      
   return_val = LB_NTF_control (LB_NTF_PUSH_ARG, (void *) dataid);

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to push data_id \"%d\" onto stack for callback registration (err %d)",
           dataid, return_val); 
      return (-1);
   }

      /* register the callback routine for the Send Channel Status command */

   return_val = LB_UN_register (local_lb_fd, ORPGRED_SEND_CHANNEL_STATUS,
                                PC_process_channel_cmd);

      /* ensure the callback properly registered */

   if (return_val < 0)
   {
      LE_send_msg (GL_ERROR,
           "Failed to register for redun mgr IPC cmd: %d (err = %d)",
           ORPGRED_SEND_CHANNEL_STATUS, return_val); 
      return (-1);
   }

      /* add the LB, msg ids to the lookup table */

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                              ORPGRED_REDUN_CHANL_STATUS_MSG, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant channel status msg entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_PING_CHANNEL_LINK, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr ping msg to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_PING_RESPONSE, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr ping response msg to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_SELECT_VCP, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_DOWNLOAD_VCP, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_UPDATE_SPOT_BLANKING, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_UPDATE_SR, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_UPDATE_CMD, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_UPDATE_ADAPT_DATA_TIME, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_UPDATE_SAILS, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_UPDATE_AVSET, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return_val = MLT_add_table_entry (dataid, local_lb_fd, redun_lb_fd, 
                                 ORPGRED_SEND_CHANNEL_STATUS, lb_path, 0);

   if (return_val != 0)
   {
      LE_send_msg (GL_ERROR, 
             "Error adding redundant mgr IPC cmd entry to lookup table");
      return (-1);
   }

   return (0);
}


/********************************************************************************

    Description: This routine suspends execution for x seconds

          Input: suspend_time - the amt of time in seconds to suspend

         Output:

         Return:

        Globals:
 
          Notes:
 
 ********************************************************************************/

void Suspend (int delay_time)
{
   time_t current_time;  /* the current time */
   time_t start_time;    /* the time the suspend is started */

      /* initialize the current and start times */

   current_time = time (NULL);

   start_time = current_time;

      /* wait for the required time to elapse before returning */

   while ((current_time - start_time) < delay_time)
   {
      msleep (1000);  
      current_time = time (NULL);
   }
   return;
}
