/********************************************************************************
                           rdasim_main.c


   This tool is used to simulate a RDA connected to the orpg computer. Ensure
   that a comm manager is not running before starting the simulator.

 ********************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/31 17:59:06 $
 * $Id: rdasim_main.c,v 1.25 2014/07/31 17:59:06 steves Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */ 


#define RDASIM_MAIN


#include <stdio.h>    /* calls: fprintf  */
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#include <rdasim_externals.h>
#include <misc.h>
#include <rdasim_simulator.h>

#define RDA_TEST_EVT      999
#define STR_SIZE          128   /* max size for input strings */

static int Request_lb_fd;       /* request linear buffer file descriptor */
static int Response_lb_fd;      /* response linear buffer file descriptor */
static int Comm_manager_index = 0;  /* instance index of this comm_manager */
static int N_added_bytes = 0;   /* number of bytes added */
static char Req_lb_name [MAX_NAME_SIZE] = "";    /* request linear buffer name */
static char Resp_lb_name [MAX_NAME_SIZE] = "";   /* response linear buffer name */
static int  Gui_flag = 0;
static int Signal_received = 0;		/* a signal is received */
       int           Interactive_mode = 0; /* Interactive mode, when true uses custom data values */
       unsigned char Surv_fixed_1 = 166; /* Default reflectivity data value region 1   */
       unsigned char Surv_fixed_2 = 126; /* Default reflectivity data value region 2   */
       int           Surv_az1 = 90;      /* Azimuth limit for reflectivity region 1    */
       int           Surv_az2 = 180;     /* Azimuth limit for relfectivity region 2    */
       unsigned char Velo_fixed_1 = 180; /* Default velocity data value region 1       */
       unsigned char Velo_fixed_2 = 180; /* Default velocity data value region 2       */
       int           Velo_az1 = 90;      /* Azimuth limit for velocity region 1        */
       int           Velo_az2 = 180;     /* Azimuth limit for velocity region 2        */
       unsigned char Sw_fixed_1 = 132;   /* Default spectrum width data value region 1 */
       unsigned char Sw_fixed_2 = 132;   /* Default spectrum width data value region 2 */
       int           Sw_az1 = 90;        /* Azimuth limit for spectrum width region 1  */
       int           Sw_az2 = 180;       /* Azimuth limit for spectrum width region 2  */
               /* Note:  All PHI data values are set in rdasim_construct_radials.c     */
       int           Phi_az1 = 90;       /* Azimuth limit for PHI region 1             */
       int           Phi_az2 = 180;      /* Azimuth limit for PHI region 2             */
       unsigned char Zdr_fixed_1 = 144;  /* Default ZDR data value region 1            */
       unsigned char Zdr_fixed_2 = 144;  /* Default ZDR data value region 2            */
       int           Zdr_az1 = 90;       /* Azimuth limit for ZDR region 1             */
       int           Zdr_az2 = 180;      /* Azimuth limit for ZDR region 2             */
       unsigned char Rho_fixed_1 = 240;  /* Default RHO data value region 1            */
       unsigned char Rho_fixed_2 = 240;  /* Default RHO data value region 2            */
       int           Rho_az1 = 90;       /* Azimuth limit for RHO region 1             */
       int           Rho_az2 = 180;      /* Azimuth limit for RHO region 2             */

   /* local functions */

static void Process_gui_callback (int lbd, LB_id_t msgid, int msg_len,
                                  void *dataid);
static void Print_runtime_stats ();
static int  Initialize_comm_link ();
static int  Open_lbs ();
static void Print_cs_error (char *msg);
static int Read_options (int argc, char **argv, float *antenna_rate, 
                         float *sample_int, short *control_state,
                         short *channel_number);
static int Register_callback (void);
static void En_callback (en_t evtcd, void *ptr, size_t   msglen);
static void Interactive_options (void);
static void Print_main_menu  (void);
static char *RS_gets (void);
static void Print_data_menu (char *data_name, unsigned char *val1, int *az1, unsigned char *val2, int *az2,
                             char *units, float scale, float offset);
static int Get_values_azimuths (unsigned char *val1, int *az1, unsigned char *val2, int *az2,
                                char *units, float scale, float offset);

/********************************************************************************

                                  main

 ********************************************************************************/

int main (int argc, char **argv)
{
   float sample_interval;       /* sample interval used to compute radial data */
   float antenna_rotation_rate; /* antenna rotation rate used to compute radial data */
   short chanl_control_state;   /* RDA channel control state */
   short rda_channel;           /* RDA channel number */


      /* initialize process scope variables */

   VErbose_mode = 0;
   antenna_rotation_rate = 0.0;
   sample_interval = 1.0;  /* average sample interval */
   chanl_control_state = CCS_CONTROLLING;
   rda_channel = 0;
   Interactive_mode = 0;

      /* initialize the simulator */

    CS_error (Print_cs_error);  /* ask CS to print error via
                                   Print_cs_error */

       /* read options */

   if (Read_options (argc, argv, &antenna_rotation_rate, &sample_interval,
                     &chanl_control_state, &rda_channel) != 0)
        exit (1);

      /* register the GUI callback routine */
  
   if (Gui_flag) { 
      if (Register_callback () < 0) {
         fprintf (stderr, "Error registering GUI LB callback routine\n");
         exit (1);
      }
   }

      /* set the user changeable radar parameters */

   CR_set_radar_parameters (antenna_rotation_rate, sample_interval); 
   RRM_init_chanl_control_state (chanl_control_state);

      /* assign the channel number relative to the configuration selected */

   RDA_channel_number = rda_channel;

   fprintf (stdout, "RDA Configuration: Open RDA\n");

   fprintf (stdout, "RDA channel number: %d\n", RDA_channel_number);

      /* peform task initialization */

   if ((Initialize_comm_link ()) == -1) {
      fprintf (stderr, "\n\nError initializing the comms\n");
      MA_terminate ();
   }

   if (RRM_initialize_messages () == -1) {
      fprintf (stderr, "\n\nError initializing the RDA canned messages\n");
      MA_terminate ();
   }

       /* User requests interactive mode */
   if (Interactive_mode)     
      Interactive_options();

   CR_initialize_first_vcp ();

       /* catch the signals */

   sigset (SIGTERM, MA_terminate);
   sigset (SIGHUP, MA_terminate);
   sigset (SIGINT, MA_terminate);

       /* simulation loop */

   while (1) {
      RR_get_requests ();
      PR_process_requests ();
      RD_run_simulator ();
      Print_runtime_stats ();

      if ( LINk.link_state == LINK_CONNECTED)
         RRM_process_rda_alarms (0, 0, 0);

      if ( LINk.link_state == LINK_DISCONNECTED)
         msleep (1000);

   }
}


/********************************************************************************
 
     Description:  this routine prints the simulator run time every 5 minutes

 ********************************************************************************/

static void Print_runtime_stats ()
{
   static struct timeval start_time;
   static struct timeval last_print_time;
   static struct timeval current_time;
   uint runtime_hrs;
   uint runtime_minutes;
   uint delta_time;
   uint partial_hrs;
   static short first_pass = TRUE;

   gettimeofday (&current_time, NULL);

   if (first_pass == TRUE) {
      start_time = current_time;
      last_print_time = current_time;
      first_pass = FALSE;
      fprintf (stdout, "\nRDA Simulator Running\n\n");
      fflush (stdout);
   }

   if ((current_time.tv_sec - last_print_time.tv_sec) >= 300) {
      delta_time = (uint) (current_time.tv_sec - start_time.tv_sec);
      runtime_hrs = delta_time / SECONDS_IN_A_HOUR;

      partial_hrs = delta_time % SECONDS_IN_A_HOUR;
      runtime_minutes = partial_hrs / SECONDS_IN_A_MINUTE;

      if (VErbose_mode > 0) {
         fprintf (stdout,"\n\n*****  RDA Simulator Runtime: ");
         fprintf (stdout, "%3u hrs  %2u min  *****\n", runtime_hrs,
                    runtime_minutes);
         fflush (stdout);
      }else {
         fprintf (stdout,"\r*****  RDA Simulator Runtime: ");
         fprintf (stdout, "%3u hrs  %2u min  *****", runtime_hrs,
                    runtime_minutes);
         fflush (stdout);
      }

      last_print_time = current_time;
   }
   return;
}


/********************************************************************************

     Description: Initialize global variables and the link structure.
     
           Input:
     
          Output:

          Return: 0 on successful initialization, or -1 on error
     
 ********************************************************************************/
 
static int Initialize_comm_link ()
{
   int request_data_id;
   int response_data_id;

      /* open the request and response linear buffers */

   if (Open_lbs (&request_data_id, &response_data_id) == -1)
       return (-1);

       /* Initialize the comm link structure */

   LINk.conn_activity = NO_ACTIVITY;
   LINk.link_state = LINK_DISCONNECTED;
   LINk.n_added_bytes = N_added_bytes;
   LINk.conn_req_ind = 0;
   LINk.n_reqs = 0;
   LINk.r_seq_num = 0;
   LINk.r_buf = NULL;
   LINk.r_buf_size = 0;
   LINk.r_cnt = 0;
   LINk.reqfd = Request_lb_fd;
   LINk.respfd = Response_lb_fd;
   LINk.st_ind = 0;
   LINk.w_buf = NULL;

   LINk.link_ind = request_data_id - ORPGDAT_CM_REQUEST;
   LINk.packet_size = 4096;
 
      /* register for the rdasim_tst tool events */
  
   EN_register (RDA_TEST_EVT, (void *) En_callback);
   
   return (0);
}


/********************************************************************************

     Description: Open the request and response Linear Buffers

           Input:  

          Output:
            Global Variables: 
                Request_lb_fd  -- Request linear buffer file descriptor
                Response_lb_fd -- Response linear buffer file descriptor

                    req_dataid -- the data id of the comm mgr request LB
                   resp_dataid -- the data id of the comm mgr response LB

          Return:  0 on success; -1 on failure

 ********************************************************************************/

static int Open_lbs (int *req_dataid, int *resp_dataid)
{
   char lb_path[MAX_NAME_SIZE];  /* Linear Buffer directory path */

      /* open the request LB */

   *req_dataid = ORPGCMI_rda_request();

   if( strlen( Req_lb_name ) == 0 )
      CS_entry ((char *) *req_dataid, CS_INT_KEY | 1, MAX_NAME_SIZE, lb_path);

   else
      strcpy( lb_path, Req_lb_name );

   fprintf (stdout, "RDA Simulator:  Request LB key = %d\n", *req_dataid);
   fprintf (stdout, "rda Simulator:  Request lb_path = %s\n", lb_path);

   if ((Request_lb_fd = LB_open (lb_path, LB_READ, NULL)) < 0)
   {
      fprintf (stderr, "RDA Simulator:  Error opening request LB (err = %d)\n", 
               Request_lb_fd);
      return (-1);
   }

      /* open the response LB */

   *resp_dataid = ORPGCMI_rda_response();
   if( strlen( Resp_lb_name ) == 0 )
      CS_entry ((char *) *resp_dataid, CS_INT_KEY | 1, MAX_NAME_SIZE, lb_path);

   else
      strcpy( lb_path, Resp_lb_name );

   fprintf (stdout, "RDA Simulator:  Response LB key = %d\n", *resp_dataid);
   fprintf (stdout, "RDA Simulator:  Response lb_path = %s\n", lb_path);


   if ((Response_lb_fd = LB_open (lb_path, LB_WRITE, NULL)) < 0)
   {
      fprintf (stderr, "RDA Simulator:  Error opening response LB (err = %d)\n", 
               Response_lb_fd);
      return (-1);
   }
   return (0);
}


/********************************************************************************
  
     Description:  Terminate the simulator

     Input:

     Output:

     Return:

 ********************************************************************************/

void MA_terminate ()
{
      /* disconnect the WB link before terminating */

   PR_process_exception (); 

   fprintf (stdout, "\n----- RDA Simulator Terminating -----\n\n"); 
   exit (0);
}


/**************************************************************************

    Description: This function computes the number of alignment bytes for
                 n_bytes. It pads the legacy RDA messages with 10 leading
                 bytes to fulfill interface requirements. It also can be
                 used to perform memory boundary alignments.

    Inputs:      n_bytes - number of bytes;

    Return:      The number of alignment bytes for n_bytes.

**************************************************************************/

int MA_align_bytes (int n_bytes)
{
   int tmp;

   tmp = n_bytes;
   if (tmp < 0)
       tmp = -tmp;

   tmp = ((tmp + ALIGNMENT_SIZE - 1) / ALIGNMENT_SIZE) * ALIGNMENT_SIZE;

   if (n_bytes >= 0)
       return (tmp);
   else
       return (-tmp);
}


/**************************************************************************

    Description: This function prints CS errors.


**************************************************************************/

static void Print_cs_error (char *msg)
{
   LE_send_msg (0, msg);
   return;
}


/**************************************************************************

    Description: This function sets the flags for the event callback.


**************************************************************************/

static void En_callback (en_t evtcd, void *ptr, size_t msglen)
{
   int event_flag;
   event_flag = *(int*)ptr;

   switch (event_flag){

        /* the follwoing events are RDA exceptions */

      case RDA_ALARM_TEST:
         RDA_alarm_code = *( ((int *)ptr) + 1);
         if( RDA_alarm_code < 0 )
            RDA_alarm_code = ((-RDA_alarm_code) | 0x8000);
         RRM_set_rda_alarm( RDA_alarm_code );
         break;
   
      case FAT_RADIAL_FORWARD:
         FAT_forward = TRUE;
         break;
               
      case FAT_RADIAL_BACK:
         FAT_backward = TRUE;
         break;
               
      case NEGATIVE_START_ANGLE:
         NEG_start_angle = TRUE;
         break;
                     
      case BAD_ELEVATION_CUT:
         BAD_elevation_cut = TRUE;
         break;
               
      case MAX_RADIAL:
         MAX_radial_exceeded = TRUE;
         break;
               
      case UNEXPECTED_BEGINNING_ELEVATION:
         UNExpected_elevation = TRUE;
         break;
            
      case UNEXPECTED_BEGINNING_VOLUME:
         UNExpected_volume = TRUE;
         break;
            
      case BAD_SEGMENT_BYPASS:
         BAD_segment_bypass = TRUE;
         break;
            
      case BAD_SEGMENT_NOTCHWIDTH:
         BAD_segment_notch = TRUE;
         break;
                  
      case BAD_START_VCP:
         BAD_start_vcp = TRUE;
         break;
                  
      case IGNORE_VOLUME_ELEVATION_RESTART:
         IGNore_volume_elevation_restart = TRUE;
         break;

      case SKIP_START_OF_VOLUME_MSG:
         SKIp_start_of_vol = TRUE;
         break;

      case LOOPBACK_TIMEOUT:
         LOOpback_timeout = TRUE;
         break;

      case LOOPBACK_SCRAMBLE:
         LOOpback_scramble = TRUE;
         break;

            /* the following events are RDA commands */

      case COMMAND_RDA_TO_LOCAL:
         RD_process_tst_tool_cmd (COMMAND_RDA_TO_LOCAL);
         break;

      case COMMAND_RDA_TO_REMOTE:
         RD_process_tst_tool_cmd (COMMAND_RDA_TO_REMOTE);
         break;

      case TOGGLE_CHANNEL_NUMBER:
         RD_process_tst_tool_cmd (TOGGLE_CHANNEL_NUMBER);
         break;

      case CHANGE_CHANNEL_CONTROL_STATUS:
         RD_process_tst_tool_cmd (CHANGE_CHANNEL_CONTROL_STATUS);
         break;
         
      case CHANGE_REFL_CALIB_CORR:
         RRM_set_refl_calib_corr(*( ((int *)ptr) + 1));  
         break;

      case TOGGLE_MAINTENANCE_MODE:
         RD_process_tst_tool_cmd(TOGGLE_MAINTENANCE_MODE);
         break;

      case AVSET_TERMINATION_CUT:
         CR_set_termination_cut( *(((int *) ptr) + 1));
         break;
         
      default:
         break;
            
   }
   return;      
}


/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:      argc - number of command arguments
                 argv - the list of command arguments

    Return:      It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv, float *antenna_rate, 
                         float *sample_int, short *control_state, 
                         short *channel_number)
{
   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */
   int int_temp;           /* temp integer to read in options */
   float float_temp;       /* temp float to read in options */

   err = 0;
   Comm_manager_index = -1;
   COmpress_radials = 0;

   while ((c = getopt (argc, argv, "a:i:c:o:s:v:IgnOCh?")) != EOF) {
      switch (c) {
         case 'a':
            if (sscanf (optarg, "%f", &float_temp) != 1) {
               fprintf (stderr, "invalid antenna rotation rate entered: %f\n",
                        float_temp);
               err = 1;
            }
            else
               *antenna_rate = float_temp;
            break;

         case 'c':
            if (sscanf (optarg, "%d", &int_temp) != 1)
               err = 1;
            else {
               if ((int_temp < 0) || (int_temp > 2)) {
                  fprintf (stderr, 
                        "Invalid channel number entered; setting ch. number to 0");
               }
               else
                  *channel_number = int_temp;
            }
            break;

         case 'i':
            strncpy (Req_lb_name, optarg, MAX_NAME_SIZE);
                  /* 3 characters reserved for index */
            Req_lb_name [MAX_NAME_SIZE - 1] = 0;
            break;

         case 'I':
            Interactive_mode = 1;
            break;

         case 'g':
            Gui_flag = 1;
            break;

         case 'n':
            *control_state = CCS_NON_CONTROLLING;
            break;

         case 'o':
            strncpy (Resp_lb_name, optarg, MAX_NAME_SIZE);
                /* 3 characters reserved for index */
            Resp_lb_name [MAX_NAME_SIZE - 1] = 0;
            break;

         case 's':
            if (sscanf (optarg, "%f", &float_temp) != 1) {
               fprintf (stderr, "invalid sample interval entered: %f\n",
                        float_temp);
               err = 1;
            } else
               *sample_int = float_temp;
            break;

         case 'v':
            if (sscanf (optarg, "%d", &int_temp) != 1)
                err = 1;
            else {
               VErbose_mode = int_temp;
               if ((VErbose_mode < 0) || (VErbose_mode > 4)) {
                  fprintf (stderr, "Invalid verbosity level requested (level = %d)\n",
                           VErbose_mode);
                  fprintf (stderr, "Setting verbosity level to 3\n");

                  VErbose_mode = 3;
               }
            }
            break;

         case 'C':
            COmpress_radials = 1;
            break;

         case 'h':
         case '?':
            err = 1;
            break;
      }
   }


   if (err == 1)     /* Print usage message */
   {
      fprintf (stdout, "Usage: %s (options)\n", argv[0]);
      fprintf (stdout, "       Options:\n");
      fprintf (stdout, "    -a -- antenna rotation rate (deg/sec);\n");
      fprintf (stdout, "          (default: standard rates as defined per elevation cut per VCP)\n");
      fprintf (stdout, "    -c -- RDA channel number (range 0-2;  default: 0)\n");
      fprintf (stdout, "    -g -- flag specifying the GUI interface will be used\n");
      fprintf (stdout, "    -i -- req_lb_name (alternative request LB name;\n");
      fprintf (stdout, "          default: specified in the system conf text)\n");
      fprintf (stdout, "    -I -- Start interactive mode to specify custom input data values\n");
      fprintf (stdout, "    -n -- Set RDA to Non-controlling (default: Controlling)\n");
      fprintf (stdout, "    -o -- resp_lb_name (alternative response LB name\n");
      fprintf (stdout, "          default: specified in the system conf text)\n");
      fprintf (stdout, "    -s -- sample interval (default: 1.0 deg)\n");
      fprintf (stdout, "    -C -- flag specifying radial compression using BZIP2\n");
      fprintf (stdout, "          (default: No Compression)\n");

      fprintf (stdout, "    -v -- verbosity level ( ranges from 0-4;\n");
      fprintf (stdout, "          default: 0)\n");
      fprintf (stdout, "          0 - prints minimum initialization and runtime information\n");
      fprintf (stdout, "          1 - prints the orpg-to-rda request message types\n");
      fprintf (stdout, "          2 - prints the simulator processing state + verbosity 1 info\n");
      fprintf (stdout, "          3 - prints verbosity 1 & 2 info plus:\n");
      fprintf (stdout, "            -   control commands\n");
      fprintf (stdout, "            -   next VCP data to execute\n");
      fprintf (stdout, "          4 - used for debugging. all information described above \n");
      fprintf (stdout, "              is printed, plus all moment initialization data and all \n");
      fprintf (stdout, "              messages written from the simulator are printed\n");

      return (-1);
   }
   return (0);
}

/********************************************************************************


 ********************************************************************************/

static void Process_gui_callback (int lbd, LB_id_t msgid, int msg_len,
                                  void *dataid)
{
   Rdasim_gui_t gui_cmd;
   int ret;

      /* read the command msg */

   ret = LB_read (lbd, &gui_cmd, msg_len, RDASIM_GUI_MSG_ID);

   if (ret < 0) 
      fprintf (stderr, "Error reading command Lb %s (err: %d)\n", RDA_SIMULATOR_LB, ret);

     /* process simulator exceptions */

  if (gui_cmd.cmd_type == EXCEPTION) {
      switch (gui_cmd.command) {

           /* the following events are RDA exceptions */

         case RDA_ALARM_TEST:
         {
            int start_alarm;
            int end_alarm;
            int set_clear_cmd;

            memcpy (&start_alarm, gui_cmd.parameters, 4);
            memcpy (&end_alarm, gui_cmd.parameters + 4, 4);
            memcpy (&set_clear_cmd, gui_cmd.parameters + 8, 4);

            RRM_process_rda_alarms (start_alarm, end_alarm, set_clear_cmd); 
      
         }
            break;
   
         case FAT_RADIAL_FORWARD:
            FAT_forward = TRUE;
            break;
               
         case FAT_RADIAL_BACK:
            FAT_backward = TRUE;
            break;
               
         case NEGATIVE_START_ANGLE:
            NEG_start_angle = TRUE;
            break;
                     
         case BAD_ELEVATION_CUT:
            BAD_elevation_cut = TRUE;
            FAT_forward = TRUE;
            break;
               
         case MAX_RADIAL:
            MAX_radial_exceeded = TRUE;
            break;
               
         case UNEXPECTED_BEGINNING_ELEVATION:
            UNExpected_elevation = TRUE;
         break;
            
         case UNEXPECTED_BEGINNING_VOLUME:
            UNExpected_volume = TRUE;
            break;
            
         case BAD_SEGMENT_BYPASS:
            BAD_segment_bypass = TRUE;
            break;
            
         case BAD_SEGMENT_NOTCHWIDTH:
            BAD_segment_notch = TRUE;
            break;
                  
         case BAD_START_VCP:
            BAD_start_vcp = TRUE;
            break;
                  
         case IGNORE_VOLUME_ELEVATION_RESTART:
            IGNore_volume_elevation_restart = TRUE;
            break;

         case SKIP_START_OF_VOLUME_MSG:
            SKIp_start_of_vol = TRUE;
            break;

         case LOOPBACK_TIMEOUT:
            LOOpback_timeout = TRUE;
            break;

         case LOOPBACK_SCRAMBLE:
            LOOpback_scramble = TRUE;
            break;
         
         default:
            fprintf (stderr, "Invalid GUI command received (cmd: %d)\n", gui_cmd.command);
         break;
      }   

      /* the following events are RDA commands */

   } else if (gui_cmd.cmd_type == COMMAND) {

      switch (gui_cmd.command) {

         case COMMAND_RDA_TO_LOCAL:
            RD_process_tst_tool_cmd (COMMAND_RDA_TO_LOCAL);
            break;

         case COMMAND_RDA_TO_REMOTE:
            RD_process_tst_tool_cmd (COMMAND_RDA_TO_REMOTE);
            break;

         case TOGGLE_CHANNEL_NUMBER:
            RD_process_tst_tool_cmd (TOGGLE_CHANNEL_NUMBER);
            break;

         case CHANGE_CHANNEL_CONTROL_STATUS:
            RD_process_tst_tool_cmd (CHANGE_CHANNEL_CONTROL_STATUS);
            break;
         
         case CHANGE_REFL_CALIB_CORR:
         {
            float tmp_float;

            memcpy (&tmp_float, & gui_cmd.parameters[0], 4);

            RRM_set_refl_calib_corr(((int) (tmp_float * 100))); /* ORDA scaled by 100 */
         }
            break;

         case TOGGLE_MAINTENANCE_MODE:
            RD_process_tst_tool_cmd(TOGGLE_MAINTENANCE_MODE);
            break;

         case AVSET_TERMINATION_CUT:
         {
            int tmp_int;

            memcpy (&tmp_int, & gui_cmd.parameters[0], 4);
            fprintf( stderr, "AVSET Termination Cut: %d\n", tmp_int );

            CR_set_termination_cut(((int) (tmp_int))); 
         }
            break;
         
         default:
            fprintf (stderr, "Invalid GUI command received (cmd: %d)\n", gui_cmd.command);
         break;
            
      }
   } else
      fprintf (stderr, "Error: Invalid GUI command type received (cmd type: %d)\n",
               gui_cmd.cmd_type);

   return;
}


/********************************************************************************

 ********************************************************************************/

static int Register_callback ()
{
   int ret = 0;
   int lb_fd;
   char lb_name [MAX_NAME_SIZE];
   char tmp [MAX_NAME_SIZE];

   MISC_get_work_dir (tmp, MAX_NAME_SIZE);
   sprintf (lb_name, "%s/" , tmp);
   strcat (lb_name, RDA_SIMULATOR_LB); 

      /* register callback rtn for simulator gui commands */

   if ((lb_fd = LB_open (lb_name, LB_READ, NULL)) < 0) {
      fprintf (stderr, "Error opening LB %s (err: %d)\n", lb_name, lb_fd);
       return (lb_fd);
   }


   ret = LB_UN_register (lb_fd, RDASIM_GUI_MSG_ID, Process_gui_callback);

      /* ensure the callback properly registered */

   if (ret < 0)
      fprintf (stderr, "Failure registering LB (name: %s;  lbd: %d;  err: %d)\n", 
              lb_name, lb_fd, ret);

   return (ret);
}

/******************************************************************

    The main loop of the interactive operation.  This mode allows
    the user to set fixed values for all data fields (except PHI)
    within two regions defined by a user selected ending azimuth.
	
******************************************************************/

static void Interactive_options (void) {


    while (1) {				/* The main loop */
	int func;

	Signal_received = 0;
	Print_main_menu ();

	while (sscanf (RS_gets (), "%d", &func) != 1 || 
					func <= 0 || func > 8) {
	    printf ("BAD INPUT - not accepted - Enter a selection: ");
	}

	switch (func) {
	    int  stat;
         unsigned char dummy;

	    case 1:
         /* Reflectivity */
		while (1) {
              Print_data_menu("reflectivity", &Surv_fixed_1, &Surv_az1, &Surv_fixed_2, &Surv_az2,
                              "dBZ", (float) 2.0, (float) 66.0);
              stat = Get_values_azimuths(&Surv_fixed_1, &Surv_az1, &Surv_fixed_2, &Surv_az2,
                                         "dBZ", (float) 2.0, (float) 66.0);
              if (stat < 0){
                 break;
              }
		}/* end while */
		break;

	    case 2:
         /* Velocity */
		while (1) {
              Print_data_menu("velocity", &Velo_fixed_1, &Velo_az1, &Velo_fixed_2, &Velo_az2,
                              "m/s", (float) 2.0, (float) 129.0);
              stat = Get_values_azimuths(&Velo_fixed_1, &Velo_az1, &Velo_fixed_2, &Velo_az2,
                                         "m/s", (float) 2.0, (float) 129.0);
              if (stat < 0){
                 break;
              }
		}/* end while */
		break;

	    case 3:
         /* Spectrum Width */
		while (1) {
              Print_data_menu("spectrum width", &Sw_fixed_1, &Sw_az1, &Sw_fixed_2, &Sw_az2,
                              "m/s", (float) 2.0, (float) 129.0);
              stat = Get_values_azimuths(&Sw_fixed_1, &Sw_az1, &Sw_fixed_2, &Sw_az2,
                                         "m/s", (float) 2.0, (float) 129.0);
              if (stat < 0){
                 break;
              }
		}/* end while */
		break;

	    case 4:
         /* Differential Reflectivity (ZDR) */
		while (1) {
              Print_data_menu("differential reflectivity", &Zdr_fixed_1, &Zdr_az1, &Zdr_fixed_2, &Zdr_az2,
                              "dB", (float) 16.0, (float) 128.0);
              stat = Get_values_azimuths(&Zdr_fixed_1, &Zdr_az1, &Zdr_fixed_2, &Zdr_az2,
                                         "dB", (float) 16.0, (float) 128.0);
              if (stat < 0){
                 break;
              }
		}/* end while */
		break;

	    case 5:
         /* Correlation Coefficient (RHO) */
		while (1) {
              Print_data_menu("correlation coefficient", &Rho_fixed_1, &Rho_az1, &Rho_fixed_2, &Rho_az2,
                              "unitless", (float) 300.0, (float) -60.0);
              stat = Get_values_azimuths(&Rho_fixed_1, &Rho_az1, &Rho_fixed_2, &Rho_az2,
                                         "unitless", (float) 300.0, (float) -60.0);
              if (stat < 0){
                 break;
              }
		}/* end while */
		break;

         case 6:
         /* Differential Phase (PHI) */
		while (1) {
              Print_data_menu("differential phase", &dummy, &Phi_az1, &dummy, &Phi_az2,
                              "dummy", (float) 0.0, (float) 0.0);
              stat = Get_values_azimuths(&dummy, &Phi_az1, &dummy, &Phi_az2,
                                         "dummy", (float) 0.0, (float) 0.0);
              if (stat < 0){
                 break;
              }
		}/* end while */
          break;

         case 7:
          return;
          break;

	    case 8:
		exit (0);

	    default:
		break;
	}
    }
}

/*******************************************************************

    Prints the main menu.

*******************************************************************/

static void Print_main_menu  (void) {

    printf ("\n\n");
    printf (" Select a data field to modify or run simulator\n");
    printf ("        1: Reflectivity\n");
    printf ("        2: Velocity\n");
    printf ("        3: Spectrum Width\n");
    printf ("        4: Differential Reflectivity\n");
    printf ("        5: Correlation Coefficient\n");
    printf ("        6: Differential Phase\n");
    printf ("        7: Run simulator\n");
    printf ("        8: Exit program\n");
    printf ("Enter a selection (1 to 8): ");
}

/*******************************************************************

    Reads standard input up to 127 characters. It discards any 
    remaining characters in the buffer before reading.

*******************************************************************/

static char *RS_gets () {
    static char buf[STR_SIZE];

    fseek (stdin, 0, 2);
    buf[0] = '\0';
    fgets (buf, STR_SIZE, stdin);
    return (buf);
}

/*******************************************************************

    Interface for changing data values and ending azimuths.

*******************************************************************/

static int Get_values_azimuths (unsigned char *val1, int *az1, unsigned char *val2, int *az2,
                                char *units, float scale, float offset ) {
    int t=0;
    float f;

    sscanf (RS_gets (), "%d", &t);
    if (Signal_received || t <= 0) {
       return -1;
    }
    else {
       if (t > 4)
	     printf ("\nBAD INPUT - not accepted\n");
	  else {
          switch (t) {
             case 1:
                printf ("Enter new region 1 data value (%s): ",units);
                sscanf (RS_gets (), "%f", &f);
                t = (f * scale) + offset;
                if (t < 2 || t > 255){
                   printf ("\nINPUT OUT OF RANGE - not accepted\n");
                }
                else {
                   *val1 = t;
                }
                break;
                     
             case 2:
                printf ("Enter new ending azimuth for region 1 (0 to 360): ");
                sscanf (RS_gets (), "%d", &t);
                if (t < 0 || t > 360){
                   printf ("\nINPUT OUT OF RANGE - not accepted\n");
                }
                else {
                   if ( t > *az2 ) 
                      printf ("\nBAD INPUT - value must be less than region 2 ending azimuth");
                   else 
                      *az1 = t;
                }
                break;

             case 3:
                printf ("Enter new region 2 value (%s): ",units);
                sscanf (RS_gets (), "%f", &f);
                t = (f * scale) + offset;
                if (t < 2 || t > 255){
                   printf ("\nINPUT OUT OF RANGE - not accepted\n");
                }
                else {
                   *val2 = t;
                }
                break;

             case 4:
                printf ("Enter new ending azimuth for region 2 (0 to 360): ");
                sscanf (RS_gets (), "%d", &t);
                if (t < 0 || t > 360){
                   printf ("\nINPUT OUT OF RANGE - not accepted\n");
                }
                else {
                   if ( t < *az1 )
                      printf ("\nBAD INPUT - value must be greater than region 1 ending azimuth");
                   else
                      *az2 = t;
                }
                break; 
             
             }/* end switch */
          }/* end if */                    
       }/* end if */
     return 0;
}

/*******************************************************************

    Prints the data menu.

*******************************************************************/

static void Print_data_menu (char *data_name, unsigned char *val1, int *az1, unsigned char *val2, int *az2,
                             char *units, float scale, float offset ) {

     float fval1 = 0;
     float fval2 = 0;

     if (scale != 0) {
        fval1 = (*val1 - offset) / scale;
        fval2 = (*val2 - offset) / scale;
     }

	printf ("\nCurrent custom %s settings:\n", data_name);
     printf ("   Region 1:\n");
     if (scale != 0)
        printf ("   1: Data value = %8.3f %s\n", fval1, units);
     else
        printf ("   1: Data value = Istok eqn.\n");
     printf ("   2: Ending Azimuth = %d\n", *az1);
     printf ("   Region 2:\n");
     if (scale != 0)
        printf ("   3: Data value = %8.3f %s\n", fval2, units);
     else
        printf ("   3: Data value = Krause eqn.\n");
     printf ("   4: Ending Azimuth = %d\n", *az2);
     printf ("\nSelect a value to change (1 to 4)\n or press Enter to return: ");
     return;
}     
