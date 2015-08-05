/********************************************************************************

      Module: mngred_main.c

      Description: This is the main file for redundant configurations.
      Both the redundant NWS and the redundant FAA configurations are
      managed by this file.

********************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2013/03/22 19:26:42 $
 * $Id: mngred_main.c,v 1.21 2013/03/22 19:26:42 garyg Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */


#define MNGRED_MAIN


#include <mngred_globals.h>
#include <misc.h>
#include <orpgsite.h>

#define TERMINATION_TIME_LIMIT     5   /* max time to wait before terminating */

/* file scope global variables */

static int Number_log_msgs = 1000;            /* # of messages to log in task LE log */
static int Terminate_process = MNGRED_FALSE;  /* the process termination flag */
static int Force_adapt_dat_update = MNGRED_FALSE; /* force adapt dat update flag */


/* local function prototypes */

static void Cleanup_for_termination ();
static void Print_cs_error (char *msg);
static void Print_usage (char **argv);
static int  Read_options (int argc, char **argv);
static int  Redun_chanl_stat_msg_is_updated(void);
static void Run_rda_channel_mgr ();
static void Run_rpg_channel_mgr ();
static int Termination_handler (int signal, int sig_type);


/********************************************************************************

    Description: This is the main routine for the redundant manager

          Input: argc, argv - command line options

         Output:

         Return:

        Globals: CONfiguration_type - see mngred_globals.h
 
          Notes:
 
 ********************************************************************************/

int main (int argc, char *argv[])
{
   int retval;  /* return value from function calls */
        /* char string configuration types */
   char *site_config_type [] = {"Single Channel", /* ORPGSITE_NO_REDUNDANCY */
                                "FAA Redundant",  /* ORPGSITE_FAA_REDUNDANT */
                                "NWS Redundant"}; /* ORPGSITE_NWS_REDUNDANT */

      /* read command line options */

   if (Read_options (argc, argv) != 0)
   {
      LE_send_msg (GL_ERROR, "Read command line options failure");
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

       /* Set LB/EN signal blocking */

    retval = EN_control (EN_SET_SIGNAL, EN_NTF_NO_SIGNAL);  
   
   if (retval < 0) 
   {
      LE_send_msg (GL_ERROR, "EN_control () (ret %d)", retval);
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

      /* set up Log/Error and CS services. */

   retval = ORPGMISC_init (argc, argv, Number_log_msgs, 0, -1, 0);
   
   if (retval < 0) 
   {
      LE_send_msg (GL_ERROR, "ORPGMISC_init failed (ret %d)", retval);
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

   LE_send_msg (MNGRED_OP_VL, "Task log initialized");

   CS_error ((void (*)())Print_cs_error);

      /* register the termination handler */

   ORPGTASK_reg_term_handler (Termination_handler);

   if (retval < 0)
   {
       LE_send_msg(GL_ERROR, "ORPGTASK_reg_term_hdlr failed: %d", retval);
       ORPGTASK_exit (GL_EXIT_FAILURE);
   }

      /* read the type of redundant configuration this site is 
         configured as (ie. FAA redundant or NWS redundant) */
      
   CONfiguration_type = ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred ())
   {
      ORPGSITE_log_last_error (GL_ERROR, ORPGSITE_REPORT_DETAILS | ORPGSITE_CLEAR_ERROR);
      LE_send_msg (GL_ERROR, 
            "Error reading the Adaptation Data site configuration type...Task is Aborting");
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }
   else if (CONfiguration_type == ORPGSITE_NO_REDUNDANCY)
   {
      LE_send_msg (GL_ERROR, 
       "This site is configured for Non-Redundant operation...Task is Aborting");
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }
   else
      LE_send_msg (MNGRED_OP_VL, "This site is configured for \"%s\" operation",
                   site_config_type [CONfiguration_type]);

       /* Initialize this channel then run the channel manager */
    
   if (IC_initialize_channel() == -1)
   {
      LE_send_msg (GL_ERROR, "Task Initialization failed....Task is Aborting");
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

	/* Tell the ORPG manager that the redundant manager is ready. */

   ORPGMGR_report_ready_for_operation();

   	/* Wait for the RPG to enter operational state.  Wait no more
	   than 30 seconds before continuing on. */

   if( ORPGMGR_wait_for_op_state( (time_t) 30 ) < 0 )
      LE_send_msg( GL_INFO, "Waiting For Operational State Timed Out\n" );
   else
      LE_send_msg( GL_INFO, "The RPG is in the OPERATE state\n" );

      /* run the channel manager main loop */

   if (CONfiguration_type == ORPGSITE_FAA_REDUNDANT)
      Run_rpg_channel_mgr ();
   else if (CONfiguration_type == ORPGSITE_NWS_REDUNDANT)
      Run_rda_channel_mgr ();

   exit (EXIT_SUCCESS);
}



/********************************************************************************

    Description: This routine sets the "force_adapt_dat_update" flag. This feature
                 is only used when the two channel's software versions do not 
                 match and the operator explicitly commands the channels to 
                 update.

          Input: 

         Output:

         Return:

 ********************************************************************************/

void MA_force_adapt_dat_update (void){
   Force_adapt_dat_update = MNGRED_TRUE;
   return;
}


/********************************************************************************

    Description: This routine is the main loop for redundant RPG (ie. FAA
                 redundant) configurations.

          Input:

         Output:

         Return:

        Globals: CHAnnel_link_state      - see mngred_globals.h & orpgred.h
                 CHAnnel_state           - see mngred_globals.h & orpgred.h
                 CHAnnel_status          - see mngred_globals.h & orpgred.h
                 INTerval_timer_expired  - see mngred_globals.h
                 RDA_download_required   - see mngred_globals.h
                 REDundant_channel_state - see mngred_globals.h
                 SENd_ipc_cmd            - see mngred_globals.h
                 Terminate_process       - see file scope global section
 
          Notes:
 
 ********************************************************************************/

static void Run_rpg_channel_mgr ()
{
   static int sw_mismatch_logged = MNGRED_FALSE;

   for (;;)
   {
         /* update the channel status */

      CST_update_channel_status ();
      
         /* if the interval timer has expired, check the state of the channel
            link */
      
      if (INTerval_timer_expired == MNGRED_TRUE)
           CLS_check_channel_link ();
           
         /* update all redundant channel LBs that have been flagged as
           requiring update */

      if ((CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP) &&
           (REDundant_channel_state != ORPGRED_CHANNEL_ACTIVE)) {
           if (ORPGRED_version_numbers_match ()       ||
              (!ORPGRED_version_numbers_match ()      &&
               Force_adapt_dat_update == MNGRED_TRUE))
                 WCD_update_redun_ch ();

           if ((!ORPGRED_version_numbers_match () && (sw_mismatch_logged == MNGRED_FALSE)) &&
                 Redun_chanl_stat_msg_is_updated()){
              LE_send_msg (GL_STATUS | GL_ERROR, "Inter-channel S/W version mismatch - Auto updates disabled");
              sw_mismatch_logged = MNGRED_TRUE;
           } else if (ORPGRED_version_numbers_match () && (sw_mismatch_logged == MNGRED_TRUE)) {
              LE_send_msg (GL_STATUS,
                       "Inter-channel S/W versions match - Auto updates re-enabled");
              sw_mismatch_logged = MNGRED_FALSE;
           }
     }      

          /* send any IPC cmds waiting to be sent */

      if ((SENd_ipc_cmd == MNGRED_TRUE) &&
          (CHAnnel_link_state == ORPGRED_CHANNEL_LINK_UP)) {
          if (ORPGRED_version_numbers_match ()       ||
             (!ORPGRED_version_numbers_match ()      &&
              Force_adapt_dat_update == MNGRED_TRUE))
                if (DC_send_IPC_cmds () < 0)
                   LE_send_msg (MNGRED_OP_VL, 
                      "Error sending IPC cmd(s) to redundant channel");
          Force_adapt_dat_update = MNGRED_FALSE;
      }
         /* call the routine that reflects the present state of the RDA
            and the present state of the channel */
      
      switch (CHAnnel_status.rda_control_state)
      {
         case ORPGRED_RDA_CONTROLLING: /* this RDA is the controlling RDA */
            
            if (CHAnnel_state == ORPGRED_CHANNEL_INACTIVE)
            {
                  /* if any RDA download commands are pending, download the
                     command(s) before transitioning to active */

               PS_xsition_to_active ();
            }
            else  /* process the active state */
               PS_process_active_state ();

            break;

         case ORPGRED_RDA_NON_CONTROLLING: /* this RDA is non-controlling */
         
            if (CHAnnel_state == ORPGRED_CHANNEL_ACTIVE)
            {
                if (ORPGRED_version_numbers_match ())
                   WCD_update_redun_ch ();    /* send any pending updates */
                PS_xsition_to_inactive ();
            }
            else  /* process the inactive state */
               PS_process_inactive_state ();

            break;

         case ORPGRED_RDA_CONTROL_STATE_UNKNOWN:

            if ((CHAnnel_state == ORPGRED_CHANNEL_ACTIVE)  &&
                (CHAnnel_status.rda_rpg_wb_link_state != RS_CONNECTED))
            {
               if ((REDundant_channel_state == ORPGRED_CHANNEL_ACTIVE) ||
                   (CHAnnel_status.comms_relay_state == ORPGRED_COMMS_RELAY_UNASSIGNED))
                        PS_xsition_to_inactive ();
            }
            else if (CHAnnel_state == ORPGRED_CHANNEL_INACTIVE)
               PS_process_inactive_state ();

            break;

         default:
            break;
      }

         /* Service any signals that have been blocked */

      EN_control (EN_WAIT, 1000);

      if (Terminate_process == MNGRED_TRUE)
         break;
      
   }
   
      /* perform final housekeeping before termination */

   Cleanup_for_termination();
   exit (0);
}


/********************************************************************************

    Description: This routine is the main loop for redundant RDA (ie. NWS
                 redundant) configurations.

          Input:

         Output:

         Return:

        Globals: CHAnnel_status        - see mngred_globals.h & orpgred.h
                 RDA_download_required - see mngred_globals.h
 
          Notes:
 
 ********************************************************************************/

static void Run_rda_channel_mgr ()
{
   int previous_rda_channel_num = MNGRED_UNINITIALIZED; /* the previous pass rda
                                                           channel number */
   int current_rda_channel_num;  /* the current pass rda channel number */
   
   for (;;)
   {
      current_rda_channel_num = ORPGRDA_channel_num ();

         /* the download_cmds routine is overloaded and used in both 
            the FAA and NWS redundant configurations. As a result, the
            CHAnnel_status.rda-rpg WB link field must be maintained for the
            routine's logic to behave correctly for both configurations */

      CHAnnel_status.rda_rpg_wb_link_state = ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT);
      
      if ((CHAnnel_status.rda_rpg_wb_link_state == RS_CONNECTED) &&
          (current_rda_channel_num > 0))
      {
         if ((previous_rda_channel_num != current_rda_channel_num)  &&
             (RDA_download_required == MNGRED_TRUE))
                 DC_process_download_commands ();

         if (RDA_download_required == MNGRED_FALSE)
             previous_rda_channel_num = current_rda_channel_num;
      }
      msleep (5000);
   }
}


/********************************************************************************

    Description: This routine performs the housekeeping duties before process
                 termination

          Input:

         Output:

         Return:

        Globals: CHAnnel_status - see mngred_globals.h & orpgred.h
 
          Notes:
 
 ********************************************************************************/

static void Cleanup_for_termination ()
{                   
   int termination_start_time; /* time the routine is called */

   termination_start_time = time (NULL);

   while (((time (NULL) - termination_start_time) < TERMINATION_TIME_LIMIT)
                                 &&
           (CHAnnel_status.rda_rpg_wb_link_state != RS_DOWN))
   {
                CST_update_channel_status ();
                msleep (1000);
   }

      /* save the channel state information before terminating */

   CST_print_channel_status ("Channel Status at termination:");

   CST_save_channel_state ();

      /* set the channel state to "inactive" so that mrpg starts the
         load up in the desired state */
         
   if (ORPGMGR_send_command (MRPG_INACTIVE) == -1)
       LE_send_msg (GL_ERROR, "Failure sending \"Inactive\" cmd to mrpg");
   else
       LE_send_msg (GL_INFO, "Setting channel state to \"Inactive\"");

   LE_send_msg (GL_INFO, "Redundant Manager Task is terminating...");

   return;
}


/********************************************************************************

    Description: This routine terminates the redundant manager

          Input: signal   - the signal that caused the termination handler
                            to be called
                 sig_type - the normal/abnormal signal type

         Output:

         Return: 0 - tells the system termination handler to exit this
                     process
                 1 - tells the system termination handler not to exit this
                     process - this process will exit itself

        Globals: CONfiguration_type - see mngred_globals.h
                 Terminate_process  - see file scope global section
 
          Notes:
 
 ********************************************************************************/

static int Termination_handler (int signal, int sig_type)
{
      /* if NWS redundant config, terminate now */
   
   if (CONfiguration_type == ORPGSITE_NWS_REDUNDANT)
   {
      LE_send_msg (GL_INFO, "Redundant manager task is terminating...");
      return (0);
   }

      /* if this is an abnormal termination, save the current channel
         state and terminate now */

   if (sig_type == ORPGTASK_EXIT_ABNORMAL_SIG)
   {
      /* save the channel state information before terminating */

      CST_print_channel_status ("Channel Status at termination:");

      CST_save_channel_state ();
      
      LE_send_msg (GL_ERROR, 
            "Redundant manager abnormally terminating (signqal %d)",
            signal);
      return (0);
   }
   else  /* otherwise, set the termination flag for normal termination */
      Terminate_process = MNGRED_TRUE;

   return (1);
}


/********************************************************************************

    Description: This routine reads the command line arguments

          Input: argc - number of command line arguments
                 argv - the list of command line arguments

         Output:

         Return: 0 on success; -1 on failure.

        Globals: Number_log_msgs - see file scope global section
 
          Notes:
 
 ********************************************************************************/

static int Read_options (int argc, char **argv)
{
   extern char *optarg;  /* used by getopt() */
   extern int optind;    /* used by getopt() */
   int c;                /* used by getopt() */
   int verbosity_level;  /* verbosity level */
   int err = 0;          /* error flag */


   while ((c = getopt (argc, argv, "c:n:v:h?")) != EOF) 
   {
      switch (c) 
      {

         case 'c':
            break;

         case 'n':
            Number_log_msgs = atoi (optarg);
            if (Number_log_msgs < 0)
                Number_log_msgs = 1000;
             break;

         case 'v':
            if ((sscanf (optarg, "%d", &verbosity_level)) != 1)
               err = -1;
            else if (verbosity_level < 0)
               LE_local_vl (0);
            else if (verbosity_level > 3)
               LE_local_vl (3);
            else
               LE_local_vl (verbosity_level);
            break;

         case 'h':
         case '?':
            Print_usage (argv);
            break;
      }
   }

   return (err);
}


/********************************************************************************

    Description: This routine checks the redundant channel's status msg to see
                 if it has been updated since this channel completed initialization

          Input: 

         Output:

         Return: 0 if the redundant channel S/W version information has not been
                 updated, or if the msg read fails; otherwise,
                 1 is returned

        Globals:
 
          Notes:
 
 ********************************************************************************/
static int Redun_chanl_stat_msg_is_updated(void) {

   Channel_status_t redun_status_msg; /* the redundant channel status msg */
   int ret;                           /* function call return value */

   ret = ORPGDA_read (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &redun_status_msg,
                      sizeof (Channel_status_t), ORPGRED_REDUN_CHANL_STATUS_MSG);

   if ((ret <= 0) || 
      ((redun_status_msg.rpg_sw_version_num | redun_status_msg.adapt_data_version_num) == 0))
      return (0);
   else
      return (1);
}


/********************************************************************************

    Description: This routine prints the usage info

          Input: argv - the command line argument list

         Output:

         Return:

        Globals:
 
          Notes:
 
 ********************************************************************************/

static void Print_usage (char **argv)
{
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -n specifies number of messages in the task LE log (default %d)\n",
            Number_log_msgs);
    printf ("       -v sets the verbose level (range: 0 - 3)\n");
    printf ("            0:  Operational/Fielded setting\n");
    printf ("            1:  Not Used\n");
    printf ("            2:  Testing\n");
    printf ("            3:  Debugging\n");
    exit (0);
}


/********************************************************************************

    Description: This routine writes error messages to the LE log when reading 
                 the current configuration file.

          Input:

         Output:

         Return: 0 on success; -1 on failure.

        Globals:
 
          Notes:
 
 ********************************************************************************/

static void Print_cs_error (char *msg)
{
    LE_send_msg (GL_ERROR, msg);
}
