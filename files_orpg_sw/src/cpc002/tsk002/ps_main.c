/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:34:38 $
 * $Id: ps_main.c,v 1.92 2014/12/09 22:34:38 steves Exp $
 * $Revision: 1.92 $
 * $State: Exp $
 */

/* System Include Files/Local Include Files */
#include <stdlib.h>   
#include <unistd.h>  

#include <infr.h>
#include <prod_distri_info.h>
#include <prod_status.h>
#include <orpg.h>

#define PS_MAIN
#include <ps_globals.h>
#undef PS_MAIN

/* gen_control_output_flag */
#define GEN_CONTROL_LIST_FROM_DEFAULT_TABLE 4

/* Static Global Variables */
static unsigned int Ot_schedule_list_event_flag;
static unsigned int Prod_list_event_cnt;
static int Prod_list_updated;
static int Adapt_data_updated;
static unsigned int Rt_request_event_flag=1;
static unsigned int Start_of_volume_event;
static orpgevt_start_of_volume_t Start_of_volume_event_data;
static orpgevt_start_of_volume_t Previous_start_of_volume_event_data;
static int Alert_cat_updated;

static unsigned int Vol_num_beginning;

static int Log_file_nmsgs = 2000;

/* Static Function Prototypes.  NOTE:  Public function prototypes
   are defined in header file ps_def.h. */
static void Encallback(en_t evtcd, void *msg, size_t msglen);
static void Gen_table_notify_func( int fd, LB_id_t msg_id, int msg_info, void *arg );
static void Event_registration();
static void Open_lb();
static void Initialize(void);
static int Read_options(int argc, char **argv);
static void Wait_for_first_volume(void);
static int Cleanup_fxn( int signal, int status );


/**************************************************************************
   Description: 
      This is the main Schedule Routine Products routine.  This routine
      has two main functions:

         1) Initialization
         2) Event Servicing

      Initialization consists of command line processing, log-error 
      services registration, termination handler registration, 
      ps_routine process initialization, and event registration.

      A polling mechanism is used for event servicing.  Periodically,
      global flags are checked to determine if an event has occurred.
      If the flag is set, an event service routine is called.  Product
      generation notification is not controlled by a event.  The product
      generation message LB is checked periodically for new product
      generation. 

   Input:
      argc - number of command line arguments
      argv - pointer to command line argument strings

   Output: 
      None

   Returns: 
      None

   Notes: 
      This module exits on the following conditions:
  
         - Processing command line arguments fails
         - Registering for log-error services fails
         - Registering for external events fails

      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
int main(int argc, char **argv){

   int retval;

   /* Read command line options. */
   retval = Read_options(argc, argv);
   if (retval < 0){

      LE_send_msg(GL_ERROR, "Read_options failed (retval %d)", retval);
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Initialize the LE services. */
   if( ORPGMISC_init(argc, argv, Log_file_nmsgs, 0, -1, 0) < 0 ){ 

      LE_send_msg(GL_ERROR, "ORPGMISC_init Failed\n") ;
      ORPGTASK_exit(GL_EXIT_FAILURE) ;

   }

   /* Register termination handler. */
   ORPGTASK_reg_term_handler( Cleanup_fxn );

   /* Register the EN_events. This needs to be done before the first
      volume scan starts. */
   Event_registration();

   /* Open all LB's which are either read or written.  Registration
      any LB notification. */
   Open_lb();

   /* Report to RPG manager that ps_routine is initialized. */
   ORPGMGR_report_ready_for_operation();

   /* Wait for the RPG to become operational. */
   ORPGMGR_wait_for_op_state ( (time_t) 120 );

   /*  Do ps_routine process initialization. */
   Initialize();

   /* Wait for the start of the next volume scan. After the new volume
      scan starts, all RRS static global vars will be initialized first,
      then in the Main (infinite) loop, the various event-processing
      routines will be called. */
   Wait_for_first_volume();

   /* Main loop. Do Forever! */
   while (1){

      /* Handle registered events. */

      /* Service receipt of RPS List(s). */
      if (Rt_request_event_flag ){

         if( Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL )
            LE_send_msg( GL_INFO, "Routine Products Requests Event Received\n" );

         Rt_request_event_flag = 0;
         PSPE_proc_rt_request_event();
    
      }

      /* Service receipt of One-time Product Request(s). */
      if (Ot_schedule_list_event_flag ){

         if( Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL )
            LE_send_msg( GL_INFO, "One-Time Product Requests Event Received\n" );

         Ot_schedule_list_event_flag = 0;
         PSPE_proc_ot_schedule_list_event();

      }

      /* Service receipt of Start Of Volume. */
      while( Start_of_volume_event > 0 ){

         int update_dflt_pgt = 0;
         int force_update_all = 0;
         int cflag = 0, pflag = 0;

         if( Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL )
            LE_send_msg( GL_INFO, "Start Of Volume Event Received\n" );

         Start_of_volume_event = 0;

         if( Alert_cat_updated ){

            if( Psg_verbose_level >= PS_DEF_WARN_VERBOSE_LEVEL )
               LE_send_msg( GL_INFO, "Wx Alert Adapt Update Event Received\n" );

         }

         if( Prod_list_updated )
            LE_send_msg( GL_INFO, "Generation List Changed LB Notication Received\n" );

         /* Set arguments based on the value of flags: Prod_list_updated
            Alert_cat_updated and Adapt_data_updated. */
         update_dflt_pgt = (Prod_list_updated | Alert_cat_updated);
         force_update_all = Adapt_data_updated;

         /* Check the Start of Volume event data.  If this VCP contains 
            supplemental scans (e.g. SAILS is active, force an update.  
            Also force an update if the supplemental scans flag has changed. */
         cflag = Start_of_volume_event_data.flags & SOV_VCP_SUPPL_SCANS;
         pflag = Previous_start_of_volume_event_data.flags & SOV_VCP_SUPPL_SCANS;
         if( (cflag) || (cflag != pflag) ){

            LE_send_msg( GL_INFO, "Update Generation List Force By Supplemental Scan(s)\n" );
            force_update_all = 1;

         }

         /* Clear the flags. */
         Prod_list_updated = 0;
         Alert_cat_updated = 0;
         Adapt_data_updated = 0;

         /* Do the start of volume processing. */
         PSPE_proc_start_of_volume_event( (int) update_dflt_pgt,
                                          (int) force_update_all );

      }

      /* Service receipt of Monitor CPU alarm. */
      if (Psg_cpu_monitor_alarm_expired){

         Psg_cpu_monitor_alarm_expired = 0;
         MCPU_monitor_cpu();

      }

      /* Report CPU utilization. */
      if (Psg_cpu_info_avail ){

         Psg_cpu_info_avail = 0;
         MCPU_report_cpu_utilization();

      }

      /* Read Product Generation Messages to see if there is some newly 
         generated product. */
      (void) PSPM_chk_new_prods();

      /* Sleep. */
      msleep(500);

      /* Read Product Generation Messages to see if there is some newly 
         generated product. */
      (void) PSPM_chk_new_prods();

      /* Sleep. */
      msleep(500);

      /* Check every volume scan, mark the products which should have been
         generated for last volume scan, but were not generated as TIMED-OUT. */
      PSVPL_put_failed_ones();

      /* Output product generation status. */
      PSVPL_output_product_status(PS_DEF_FROM_TIMER);

   /* end while loop */
   }    

/* END of main() */
}

/**************************************************************************
   Description: 
      Event notification handler.  In all cases when a registered event
      occurs, a global flag is set to indicate the event occurred.
      Events are actually "serviced" within the main loop of the 
      main module. 

      The following events are checked:

         ORPGEVT_START_OF_VOLUME
         ORPGEVT_PROD_LIST
         ORPGEVT_OT_SCHEDULE_LIST
         ORPGEVT_RT_REQUEST

   Input: 
      evtcd - event code.
      msg - pointer to message associated with event.  NULL if no message.
      msglen - message length.  Undefined if no message.

   Output: 

   Returns: 
      There is no return value defined for this function.

   Notes: 
      This EN handler must follow the rules that apply to any signal 
      handler.

      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static void Encallback(en_t evtcd, void *msg, size_t msglen){

   /* If specified event occurs, set flag. */
   if( ORPGEVT_START_OF_VOLUME == evtcd ){

      Start_of_volume_event = 1;

      /* Make a copy of the Start_of_volume_event_data. */
      memcpy( &Previous_start_of_volume_event_data,
              &Start_of_volume_event_data,
              ORPGEVT_START_OF_VOLUME_DATA_LEN );

      /* Clear out the start of volume event data. */
      memset( &Start_of_volume_event_data, 0, 
              ORPGEVT_START_OF_VOLUME_DATA_LEN );

      /* If message sent with event, copy data to local buffer. */
      if( (msg != NULL)
               &&
          (msglen >= ORPGEVT_START_OF_VOLUME_DATA_LEN) )
         memcpy( (void *) &Start_of_volume_event_data, msg,
                 ORPGEVT_START_OF_VOLUME_DATA_LEN );

   }

   else if( ORPGEVT_OT_SCHEDULE_LIST == evtcd )
      Ot_schedule_list_event_flag = 1;

   else if( ORPGEVT_RT_REQUEST == evtcd )
      Rt_request_event_flag = 1;

   else if( ORPGEVT_WX_ALERT_ADAPT_UPDATE == evtcd )
      Alert_cat_updated = 1;

   return;

/* END of Encallback() */
}

/**************************************************************************
   Description: 
      LB notification handler.  In all cases when a registered notification
      occurs, a global flag is set to indicate the notification occurred.
      Notifications are actually "serviced" within the main loop of the 
      main module. 

      The following notifications are checked:

         ORPGEVT_PROD_LIST

   Input: 
      see LB man for a description of the arguments.

   Output: 

   Returns: 
      There is no return value defined for this function.

   Notes: 
      This LB notification handler must follow the rules that apply to 
      any signal handler.

      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static void Gen_table_notify_func( int fd, LB_id_t msg_id, int msg_info, void *arg ){

   if (++Prod_list_event_cnt == 0)
      Prod_list_event_cnt = 1;
         
   Prod_list_updated = 1;
       
}

/**************************************************************************
   Description: 
      Performs all event registration for the ps_routine process.

   Input: 

   Output: 

   Returns: 
      There is no return value defined for this function.

   Notes: 
      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static void Event_registration(){

   int retval;

   if ((retval = EN_register(ORPGEVT_START_OF_VOLUME, (void *) Encallback)) < 0 ||
       (retval = EN_register(ORPGEVT_OT_SCHEDULE_LIST, (void *) Encallback)) < 0 ||
       (retval = EN_register(ORPGEVT_WX_ALERT_ADAPT_UPDATE, (void *) Encallback)) < 0 ||
       (retval = EN_register(ORPGEVT_RT_REQUEST, (void *) Encallback)) < 0){

      LE_send_msg( GL_ERROR, "EN_register failed (retval %d)", retval);
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

/* End of Event_registration() */
}

/**************************************************************************
   Description: 
      Performs all LB opens with proper access mode. 

   Input: 

   Output: 

   Returns: 
      There is no return value defined for this function.

   Notes: 
      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static void Open_lb(){

   int retval;

   if ((retval = ORPGDA_open( ORPGDAT_TASK_STATUS, LB_READ )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_RT_REQUEST, LB_READ )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_GSM_DATA, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_PROD_GEN_MSGS, LB_READ )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_PROD_REQUESTS, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_LOAD_SHED_CAT, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_PROD_STATUS, LB_WRITE )) < 0 ){

      LE_send_msg( GL_ERROR, "Open_lb failed (retval %d)", retval);
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Register for LB notification.  To prevent losing registration, open the LB first
      with WRITE permission. */
   if( (retval = ORPGDA_open( ORPGDAT_PROD_INFO, LB_WRITE )) < 0 ){

      LE_send_msg( GL_ERROR, "Open_lb failed (retval %d)", retval);
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   if( (retval = ORPGDA_UN_register( ORPGDAT_PROD_INFO, PD_CURRENT_PROD_MSG_ID,
                                     Gen_table_notify_func )) < 0 
                                    || 
       (retval = ORPGDA_UN_register( ORPGDAT_PROD_INFO, PD_DEFAULT_A_PROD_MSG_ID,
                                     Gen_table_notify_func )) < 0 
                                    || 
       (retval = ORPGDA_UN_register( ORPGDAT_PROD_INFO, PD_DEFAULT_B_PROD_MSG_ID,
                                     Gen_table_notify_func )) < 0 
                                    || 
       (retval = ORPGDA_UN_register( ORPGDAT_PROD_INFO, PD_DEFAULT_M_PROD_MSG_ID,
                                     Gen_table_notify_func )) < 0 ){

      LE_send_msg( GL_ERROR, 
              "LB Nofitication Registration Failed For Generation List Updates\n",
              retval);
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

/* End of Open_lb() */
}

/**************************************************************************
   Description: 
      Perform initialization for the ps_routine process.

   Input: 

   Output: 

   Returns: 
      There is no return value defined for this function.

   Notes: 
      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static void Initialize(void){

   /* Initialize global static variables ... */
   Psg_wx_mode_beginning = PS_DEF_WXMODE_UNKNOWN;

   Vol_num_beginning = 0;

   /* Perform module initialization. */
   PSTS_initialize();
   RRS_init_read();
   PD_initialize();
   PSVPL_initialize();
   PSPE_initialize();
   PSPTT_initialize();
   MCPU_initialize();

   /* No need to output ORPGDAT_PROD_STATUS data ... */
   Psg_output_prod_status_flag = 0;

   /* Initialize the previous and current start of volume event data. */
   memset( &Previous_start_of_volume_event_data, 0, ORPGEVT_START_OF_VOLUME_DATA_LEN );
   memset( &Start_of_volume_event_data, 0, ORPGEVT_START_OF_VOLUME_DATA_LEN );

   return;

/* END of Initialize() */
}

/**************************************************************************
   Description: 
      Read and process the command-line options.

   Input: 
      argc - number of command line options.
      argv - command line options.

   Output:

   Returns: 
      PS_DEF_SUCCESS if successful; PS_DEF_FAILED otherwise.

   Notes:
      If the -h (help) option is specified, this process exits.

      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static int Read_options(int argc, char **argv){

   int input;
   int verbose_level = PS_DEF_DFLT_VERBOSE_LEVEL;

   Psg_cpu_monitoring_rate = 0;
   Psg_cpu_overhead = CPU_OVERHEAD;

   /* Do For All command line arguments. */
   while ((input = getopt(argc, argv, "l:m:o:hv:")) != -1){

      switch (input){

      case 'h':
         (void) fprintf( stdout, "Usage: ps_routine (options)\n" );
         (void) fprintf( stdout, "\toptions:\n" );
         (void) fprintf( stdout, "\t-l Log File Number LE Messages (%d)\n", 
                         Log_file_nmsgs );
         (void) fprintf( stdout, "\t-m CPU Monitoring Rate (Secs).  0 = DISABLE (%d)\n", 
                         Psg_cpu_monitoring_rate );
         (void) fprintf( stdout, "\t-o CPU Overhead (%d)\n", Psg_cpu_overhead );
         (void) fprintf( stdout, "\t-h Help\n" );
         (void) fprintf( stdout,
                         "\t-v LE message verbosity level (default: %d, max: %d)\n",
                         PS_DEF_DFLT_VERBOSE_LEVEL, PS_DEF_MAX_VERBOSE_LEVEL );

         exit(EXIT_FAILURE);

      case 'v':
         verbose_level = atoi(optarg);
         break;

      case 'l':
         Log_file_nmsgs = atoi(optarg);
         if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
            Log_file_nmsgs = 2000;
         break;

      case 'm':
         Psg_cpu_monitoring_rate = atoi(optarg);
         if( Psg_cpu_monitoring_rate < 0 )
            Psg_cpu_monitoring_rate = 0;

         else if( Psg_cpu_monitoring_rate > 90 )
            Psg_cpu_monitoring_rate = 90;

         else if( (Psg_cpu_monitoring_rate > 0) && (Psg_cpu_monitoring_rate < 5) )
            Psg_cpu_monitoring_rate = 5;

         break;

      case 'o':
         Psg_cpu_overhead = atoi(optarg);
         if( Psg_cpu_overhead < 0 )
            Psg_cpu_overhead = 0;

         else if( Psg_cpu_overhead > 20 )
            Psg_cpu_overhead = 20;

         break;

      default:
         return (PS_DEF_FAILED);

      }     

   }       

   /* Ensure we have a reasonable level of verbosity ... */
   if (verbose_level < PS_DEF_MIN_VERBOSE_LEVEL)
      Psg_verbose_level = PS_DEF_MIN_VERBOSE_LEVEL;

   else if (verbose_level > PS_DEF_MAX_VERBOSE_LEVEL)
      Psg_verbose_level = PS_DEF_MAX_VERBOSE_LEVEL;

   else
      Psg_verbose_level = verbose_level;

   LE_send_msg(GL_INFO, "Verbose Level: %d", Psg_verbose_level);

   return (PS_DEF_SUCCESS);

/* END of Read_options() */
}

/**************************************************************************
   Description:
      Wait for first volume scan is kind of a misnomer.  This function
      is called initially to build the master generation list from the
      default product generation list and then publish the product status.
      The only waiting involved is waiting for the ORPG to become 
      operational.

   Input:

   Output:

   Returns:
      There is no return values defined for this function.

   Notes:
      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static void Wait_for_first_volume(void){

   int default_pgt_num_prods;

   /* Perform initial read of the volume status. */
   RRS_init_read();

   /* Retrieve product attributes data from ORPGDAT_PROD_INFO ...
      Form the default product generation list ... Initialize the
      ORPGDAT_PROD_REQUESTS data (re: ORPG Manpage "rpg") ... */
   PSPTT_build_prod_task_tables();
   PD_form_default_prod_gen_list( &default_pgt_num_prods );
   PSPE_init_gen_control_lb();

   /* Set the starting weather mode, beginning volume time, and 
      beginning volume number.  */
   Psg_wx_mode_beginning = RRS_get_current_weather_mode();
   Vol_num_beginning = RRS_get_volume_num(NULL);

   if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL)
      LE_send_msg( GL_INFO,
                   "Init Successful. Current Wx Mode: %d, Volume # %d\n",
                   Psg_wx_mode_beginning, Vol_num_beginning );
    
   /* Initialize the product generation status for the current volume
      scan, i.e., vol_list[0]. */
   PSVPL_update_vol_list();

   /* Build the output generation control list.  This list is used 
      to construct the product request list which is written to the
      product request LB. */
   PD_gen_output_gen_control_list(GEN_CONTROL_LIST_FROM_DEFAULT_TABLE);
   PSPE_prod_list_to_prod_request( PD_get_output_gen_control_list(),
                                   PD_get_output_gen_control_list_len() );
   PD_free_gen_control_list( CUR_GEN_CONTROL );

   /* Set flag to trigger output of ORPGDAT_PROD_STATUS data.  Write
      the product status. */
   Psg_output_prod_status_flag = 1;
   PSVPL_output_product_status(PS_DEF_NEW_PRODUCT);

/* END of Wait_for_first_volume() */
}

/**************************************************************************
   Description:
      ps_routine termination handler.  Write signal number, then return.

   Input:
      see ORPGTASK man page.

   Output:

   Returns:

   Notes:
      File scope global variables have first character capitalized and are
      defined at the top of this file.  Process scope globals variables 
      begin with Psg_ and are define in ps_globals.h

**************************************************************************/
static int Cleanup_fxn( int signal, int status ){

   /* This function currently does nothing. */
   LE_send_msg( GL_INFO, "Signal %d Received\n", signal );
   return (0);

/* End of Cleanup_fxn() */
}

