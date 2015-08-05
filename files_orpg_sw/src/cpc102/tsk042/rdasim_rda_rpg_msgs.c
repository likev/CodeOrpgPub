/********************************************************************************

               file: rdasim_rda_rpg_msgs.c

        Description: This file contains all the RDA/RPG message handling 
                     routines. All of the canned messages and message buffers are
                     padded with the ICD message header for convenience

 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:45:40 $
 * $Id: rdasim_rda_rpg_msgs.c,v 1.48 2014/11/07 21:45:40 steves Exp $
 * $Revision: 1.48 $
 * $State: Exp $
 */


#include <stdio.h>
#include <string.h> 
#include <time.h>

#include <rdasim_simulator.h>
#include <rdasim_externals.h>
#include <rda_control.h>
#include <rda_rpg_loop_back.h>
#include <rda_notch_width_map.h>
#include <rpg_request_data.h>
#include <rpg_vcp.h>


#include <errno.h>


#define NUM_HFWDS_TO_DUMP   40          /* number RDA msg halfwords to dump */
#define MSG_DIRECTORY       "CFG_DIR"   /* dierctory where the canned messages are
                                           located */
#define NUMBER_ICD_MSGS     32

   /* canned message file definitions */

#define ORDA_BYPASS_MAP_FILE       "/orda_bypass_map_msg.dat"
#define ORDA_NOTCHWIDTH_MAP_FILE   "/orda_clutter_filter_map_msg.dat"
#define LOOPBACK_MSG_FILE          "/loopback_msg.dat"
#define ORDA_PERF_MAINT_MSG_FILE   "/orda_perf_maint_msg.dat"
#define ORDA_ADAPTATION_DATA_FILE  "/orda_adapt_data_msg.dat"


extern SMI_info_t *SWAP_bytes (char *type_name, void *data);

   /* file scope variable list */

static char *Message_pointer [NUMBER_ICD_MSGS]; /* RDA<->RPG msg pointers array */
static int  Message_size [NUMBER_ICD_MSGS];     /* RDA<->RPG msg sizes array */
static Radial_message_t Radial_message;         /* radial message (only includes headers) */
static char *Remote_vcp = NULL;                 /* pointer to the remote/RPG VCP */

   /* local function list */

static int Byte_swap_msg_hdr (RDA_RPG_message_header_t *msg_hdr_ptr);
static int Byte_swap_ICD_msg (void *msg_ptr, RDA_RPG_message_header_t *msg_hdr);
static void Clear_new_bypass_map_buffers (int number_elements, char **seg_aray, 
                                          int *seg_size, int *number_segments, 
                                          int *msg_processing_flag,
                                          int *number_segs_processed);
static void Clear_status_msg ();
static void Close_msg_files (FILE *bypas_map_fd, FILE *loopbck_msg_fd,
                             FILE *notchwdth_map_fd, FILE *perf_msg_fd,
                             FILE *rda_adapt_dat_file_ptr);
static void Construct_msg_header (RDA_RPG_message_header_t *msg_header_ptr,
                                  short size, unsigned char type, 
                                  short number_segments, short segment_number);
static int Copy_remote_vcp (char *msg);
static void Echo_rpg_rda_loopback_msg ();
static void Initialize_msg_arrays (char *bypass_msg, char *perf_msg, 
                                   char *notchwidth_msg, char *loopback_msg_rda_rpg, 
                                   char *loopback_msg_rpg_rda, char *rda_adapt_data_msg,
                                   int bypass_file_size, int perf_file_size, 
                                   int notchwidth_size,  int loopback_size,
                                   int rda_adapt_data_size);
static void Initialize_status_msg ();
static void Process_orda_message (int message_type);
static int  Read_msg_files (FILE *bypas_ptr, FILE *loopbck_ptr, 
                            FILE *notchwdth_ptr, FILE *perf_ptr,
                            FILE *rda_adapt_dat_ptr);
static void Reallocate_input_buffer ( int len );
static void Rda_msg_dump (char *buffer);  /* used for debugging only */
static void Update_bypass_map (void *msg_segment);
static void Update_map_generation_timestamp (int msg_type);
static void Write_message (char *data_buf, int message_type, int len);


/********************************************************************************
 
     Description:  Pass the address of the radial message back to the calling
                   routine
     
           Input:
     
          Output:  

          Return: Address of the radial message 
     
 ********************************************************************************/

void *RRM_get_radial_msg ()
{
   return (&Radial_message);
} 


/********************************************************************************
 
     Description:  Pass the the RDA VCP buffer pointer back to the calling
                   routine
     
           Input:
     
          Output:  

          Return: Pointer to the RDA VCP msg buffer
     
 ********************************************************************************/

void *RRM_get_rda_vcp_buffer ()
{
   return (Message_pointer [RDA_VCP_MSG]);
}


/********************************************************************************
 
     Description:  Pass the the remote/RPG VCP buffer pointer back to the calling
                   routine
     
           Input:
     
          Output:  

          Return: Pointer to the remote VCP buffer
     
 ********************************************************************************/

void *RRM_get_remote_vcp ()
{
   return (Remote_vcp);
}


/********************************************************************************

    Description: Set the channel control state

          Input: control_state - the RDA Channel Control State for simulator
                                 startup 

 ********************************************************************************/

void RRM_init_chanl_control_state (short control_state)
{
   memset( &RDA_status_msg, 0, sizeof(RDA_status_msg) );
   RDA_status_msg.channel_status = control_state;
   return;
}


/********************************************************************************
 
     Description:  initialize all the RDA_to_RPG messages
     
           Input:
             Globals: RDA_status_msg  (externals.h)
     
          Output:  global message structures

          Return:  0 on successful initialization, or -1 on error
     
 ********************************************************************************/
 
int RRM_initialize_messages ()
{
   FILE *bypass_msg_ptr;                        /* Bypass map msg file pointer */
   FILE *loopback_msg_ptr;                      /* Loopback msg file pointer */
   FILE *notchwidth_msg_ptr;                    /* Notchwidth map msg file pointer */
   FILE *performance_msg_ptr;                   /* Performance msg file pointer */
   FILE *rda_adapt_dat_msg_ptr;                 /* RDA ADaptation Data file pointer */
   char *message_directory;                     /* message directory pointer */
   char bypass_map_msg_file[MAX_NAME_SIZE];     /* bypass msg file name */
   char loopback_msg_file[MAX_NAME_SIZE];       /* loopback msg file name */
   char notchwidth_map_msg_file[MAX_NAME_SIZE]; /* notchwidth map msg file name */
   char performance_msg_file[MAX_NAME_SIZE];    /* performance msg file name */
   char rda_adapt_dat_msg_file[MAX_NAME_SIZE];  /* RDA adapt dat msg file name */
   int str_length;                              /* character string length */


    /* open the message files */

      /* construct the message directory path */

   message_directory = getenv (MSG_DIRECTORY); 

   if (message_directory == NULL)
   {
      fprintf (stderr, "\nthe \"%s\" environment variable specifying the path\n", 
               MSG_DIRECTORY); 
      fprintf (stderr, "where the simulator message files are located is not defined.\n");
      fprintf (stderr, "please define the environment variable then re-run the simulator\n\n");
      fprintf (stderr, "-------------- Program Terminating ----------------\n\n");
      MA_terminate ();
   }

     /* concatenate the message file names to the message directory path */
   
   str_length = strlen(message_directory);

      /* assign the files depending on the system configuration specified
         at tool startup */

   strncpy (bypass_map_msg_file, (const char *) message_directory, str_length + 1);
   strncpy (notchwidth_map_msg_file, (const char *) message_directory, str_length + 1);
   strncpy (performance_msg_file, (const char *) message_directory, str_length + 1);
   strncpy (rda_adapt_dat_msg_file, (const char *) message_directory, str_length + 1);

   strcat(bypass_map_msg_file, ORDA_BYPASS_MAP_FILE);
   strcat(notchwidth_map_msg_file, ORDA_NOTCHWIDTH_MAP_FILE);
   strcat(performance_msg_file, ORDA_PERF_MAINT_MSG_FILE);
   strcat(rda_adapt_dat_msg_file, ORDA_ADAPTATION_DATA_FILE);

   strncpy (loopback_msg_file, (const char *) message_directory, str_length + 1);
   strcat(loopback_msg_file, LOOPBACK_MSG_FILE);
   
     /* open the message files */

   bypass_msg_ptr = fopen (bypass_map_msg_file, "rb");
   loopback_msg_ptr = fopen (loopback_msg_file, "rb");
   notchwidth_msg_ptr = fopen (notchwidth_map_msg_file, "rb");
   performance_msg_ptr = fopen (performance_msg_file, "rb");
   rda_adapt_dat_msg_ptr = fopen (rda_adapt_dat_msg_file, "rb");

   if (bypass_msg_ptr == NULL     || loopback_msg_ptr == NULL    ||
       notchwidth_msg_ptr == NULL || performance_msg_ptr == NULL ||
       rda_adapt_dat_msg_ptr == NULL) {
      fprintf (stderr, "\nError opening the canned message files.\n");
      fprintf (stderr, 
               "The following message files should be located in directory \"%s\":\n",
               message_directory);
      fprintf (stderr, "\n     %s\n", ORDA_BYPASS_MAP_FILE);
      fprintf (stderr, "     %s\n", LOOPBACK_MSG_FILE);
      fprintf (stderr, "     %s\n", ORDA_NOTCHWIDTH_MAP_FILE);
      fprintf (stderr, "     %s\n", ORDA_PERF_MAINT_MSG_FILE);
      fprintf (stderr, "     %s\n", ORDA_ADAPTATION_DATA_FILE);
      return (-1);
   }
   
      /* read the message files */
   
   if (Read_msg_files (bypass_msg_ptr, loopback_msg_ptr, notchwidth_msg_ptr,
       performance_msg_ptr, rda_adapt_dat_msg_ptr) == -1)
         return (-1);

      /* close the message files */

   Close_msg_files (bypass_msg_ptr, loopback_msg_ptr, notchwidth_msg_ptr,
                    performance_msg_ptr, rda_adapt_dat_msg_ptr);

      /* initialize the RDA Status msg */

   Initialize_status_msg ();

      /* Register the byte swapping function if needed */

      if (MISC_i_am_bigendian() == 0) {  /* 0 means little endian machine */
         SMIA_set_smi_func (SWAP_bytes);
   }
 

   return (0);
}



/********************************************************************************

    Description: This routine processes RDA alarms as commanded via the GUI

          Input: start_alarm - the beginning alarm code
                   end_alarm - the ending alarm code
               set_clear_cmd - command to set/clear the alarm

 ********************************************************************************/

void RRM_process_rda_alarms (int start_alarm, int end_alarm, int set_clear_cmd) 
{
   static int processing_alarms = 0;
   static int alarm_to_process = 0;
   static int end_alarm_code = 0;
   static int alarm_cmd = 0;
          char cmd_text [2][9] = {"Clearing",
                                   "Setting"};

   static int transmitter_alarms = 0;
   static int tower_util_alarms = 0;
   static int receiver_alarms = 0;
   static int rda_control_alarms = 0;
   static int pedestal_alarms = 0;
   static int signal_processor_alarms = 0;
   static int communication_alarms = 0;

   static int maint_action_required = 0;
   static int maint_action_mandatory = 0;
   static int inoperable = 0;

   if (start_alarm || end_alarm || set_clear_cmd) {
      alarm_to_process = start_alarm;
      end_alarm_code = end_alarm;
      alarm_cmd = set_clear_cmd;
      processing_alarms = 1;

      if (VErbose_mode >= 3) {
          fprintf (stdout, "Process Alarm Codes: \n");
          fprintf (stdout, "   Start alarm code: %d\n", start_alarm);
          fprintf (stdout, "   End alarm code: %d\n", end_alarm);
          fprintf (stdout, "   Set/Clear command(set = 1; clear = 0): %d\n", set_clear_cmd);
      }
      return;
   }
  
   if (processing_alarms) {
      RDA_alarm_entry_t *alarm_data = NULL;

      alarm_data =
          (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data((int) alarm_to_process);

         /* If the alarm state is N/A, skip it. */

      while ( (alarm_data != NULL ) && 
              (alarm_data->state == ORPGRDA_STATE_NOT_APPLICABLE)  &&
              (alarm_to_process < end_alarm_code)) {

        ++alarm_to_process;

        alarm_data =
             (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data((int) alarm_to_process);

        if( alarm_data == NULL )
           alarm_to_process = end_alarm_code + 1;

      }

      if (alarm_to_process > end_alarm_code) {
         alarm_to_process = 0;
         end_alarm_code = 0;
         alarm_cmd = 0;
         processing_alarms = 0;
      } else {

         if( alarm_data != NULL ){

               /* Process the alarm state ... sets the operability status. */

            unsigned short op_status = RDA_status_msg.op_status & 0xfffe;

            switch( alarm_data->state ){

               case ORPGRDA_STATE_MAINTENANCE_REQUIRED:
               {
                  if( alarm_cmd == 0 ){

                     maint_action_required--;
                     if( maint_action_required <= 0 ){

                        if( op_status == ROS_RDA_MAINTENANCE_REQUIRED )
                           RDA_status_msg.op_status = ROS_RDA_ONLINE;

                        if( maint_action_required < 0 )
                           maint_action_required = 0;

                     }

                  }
                  else{

                     maint_action_required++;
                     if( (op_status != ROS_RDA_MAINTENANCE_MANDATORY) 
                                                   &&
                         (op_status != ROS_RDA_INOPERABLE) )
                        RDA_status_msg.op_status = ROS_RDA_MAINTENANCE_REQUIRED;

                  }

                  break;

               }

               case ORPGRDA_STATE_MAINTENANCE_MANDATORY:
               {
                  if( alarm_cmd == 0 ){

                     maint_action_mandatory--;
                     if( maint_action_mandatory <= 0 ){

                        if( op_status == ROS_RDA_MAINTENANCE_MANDATORY ){

                           if( maint_action_required > 0 )
                              RDA_status_msg.op_status = ROS_RDA_MAINTENANCE_REQUIRED;

                           else
                              RDA_status_msg.op_status = ROS_RDA_ONLINE;

                        }

                        if( maint_action_mandatory < 0 )
                           maint_action_mandatory = 0;

                     }

                  }
                  else{

                     maint_action_mandatory++;
                     if( op_status != ROS_RDA_INOPERABLE ) 
                        RDA_status_msg.op_status = ROS_RDA_MAINTENANCE_MANDATORY;

                  }

                  break;

               }

               case ORPGRDA_STATE_INOPERATIVE:
               {
                  if( alarm_cmd == 0 ){

                     inoperable--;
                     if( inoperable <= 0 ){

                        if( op_status == ROS_RDA_INOPERABLE ){

                           if( maint_action_mandatory > 0 )
                              RDA_status_msg.op_status = ROS_RDA_MAINTENANCE_MANDATORY;

                           else if( maint_action_required > 0 )
                              RDA_status_msg.op_status = ROS_RDA_MAINTENANCE_REQUIRED;

                           else
                              RDA_status_msg.op_status = ROS_RDA_ONLINE;

                        }
                              
                        if( inoperable < 0 )
                           inoperable = 0;

                     }

                  }
                  else{

                     inoperable++;
                     RDA_status_msg.op_status = ROS_RDA_INOPERABLE;

                  }

                  break;

              }

            }

               /* Process the alarm device ... sets the alarm summary. */

            switch( alarm_data->device ){

               case ORPGRDA_DEVICE_XMT:
               {
                  if( alarm_cmd == 0 ){

                     transmitter_alarms--;
                     if( transmitter_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_TRANSMITTER;
                        if( transmitter_alarms < 0 )
                           transmitter_alarms = 0;

                     }

                  }
                  else{

                     transmitter_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_TRANSMITTER;

                  }

                  break;
               }

               case ORPGRDA_DEVICE_UTL:
               {
                  if( alarm_cmd == 0 ){

                     tower_util_alarms--;
                     if( tower_util_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_TOWER_UTILITIES;
                        if( tower_util_alarms < 0 )
                           tower_util_alarms = 0;

                     }

                  }
                  else{

                     tower_util_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_TOWER_UTILITIES;

                  }

                  break;
               }
               case ORPGRDA_DEVICE_RCV:
               {
                  if( alarm_cmd == 0 ){

                     receiver_alarms--;
                     if( receiver_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_RECEIVER;
                        if( receiver_alarms < 0 )
                           receiver_alarms = 0;

                     }

                  }
                  else{

                     receiver_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_RECEIVER;

                  }

                  break;
               }
               case ORPGRDA_DEVICE_CTR:
               {
                  if( alarm_cmd == 0 ){

                     rda_control_alarms--;
                     if( rda_control_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_RDA_CONTROL;

                        if( rda_control_alarms < 0 )
                           rda_control_alarms = 0;

                     }

                  }
                  else{

                     rda_control_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_RDA_CONTROL;

                  }

                  break;
               }
               case ORPGRDA_DEVICE_PED:
               {
                  if( alarm_cmd == 0 ){

                     pedestal_alarms--;
                     if( pedestal_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_PEDESTAL;

                        if( pedestal_alarms < 0 )
                           pedestal_alarms = 0;

                     }

                  }
                  else{

                     pedestal_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_PEDESTAL;

                  }

                  break;
               }
               case ORPGRDA_DEVICE_SIG:
               {
                  if( alarm_cmd == 0 ){

                     signal_processor_alarms--;
                     if( signal_processor_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_SIGNAL_PROCESSOR;
                        if( signal_processor_alarms < 0 )
                           signal_processor_alarms = 0;

                     }

                  }
                  else{

                     signal_processor_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_SIGNAL_PROCESSOR;

                  }

                  break;

               }
               case ORPGRDA_DEVICE_COM:
               {
                  if( alarm_cmd == 0 ){

                     communication_alarms--;
                     if( communication_alarms <= 0 ){

                        RDA_status_msg.rda_alarm &= ~RAS_RPG_COMMUNICATIONS;

                        if( communication_alarms < 0 )
                           communication_alarms = 0;

                     }

                  }
                  else{

                     communication_alarms++;
                     RDA_status_msg.rda_alarm |= RAS_RPG_COMMUNICATIONS;

                  }

                  break;

               }

            }

         }

      }

      if ( alarm_cmd == 0 ) /* clear the alarm */
          RRM_set_rda_alarm (alarm_to_process | 0x8000);
      else /* set the alarm */
          RRM_set_rda_alarm (alarm_to_process);

      if (VErbose_mode >= 3)
          fprintf (stdout, "%s alarm %d\n", cmd_text[alarm_cmd], alarm_to_process);

      ++alarm_to_process;
   }

   return;
}


/********************************************************************************
  
    Description: This routine processes all RDA->RPG message types (e.g. all
                 messages written to the rpg)
   
          Input: message_type - the message type to process

         Output:

         Return:

       Note: all messages are handled the same except the RPG-to-RDA loopback
             and the RDA status messages. Some of the loopback message
             header data is retrieved and sent back to the rpg. For the RDA status
             message, some of the fields are cleared after the message is sent so
             the routine to clear these fields is called from here.

********************************************************************************/

void RRM_process_rda_message (short message_type)
{
   int   i, ret;
   char  *data_buffer;         /*  pointer to the message to process */
   short number_msg_segments;  /* number of segments in this message */
   short msg_segment_number;   /* current segment # being written */
   short msg_size_bytes;       /* size of the message in bytes */
   short msg_size_halfwords;   /* size of message in halfwords */
   int   total_bytes_sent = 0; /* total bytes written for the message */
   RDA_RPG_message_header_t rda_rpg_msg_hdr; /* RDA-RPG msg header */
         
      /* if the RDA-RPG link is disconnected, then return */

   if (LINk.link_state == LINK_DISCONNECTED)
      return;

   if ((VErbose_mode >= 3) && ((message_type != 1) && (message_type != 31)))
        fprintf (stdout, "RPG<-RDA msg sent (msg type: %d)\n", 
                 message_type);

   if ((VErbose_mode == 4) && ((message_type == 1) || (message_type == 31)))
        fprintf (stdout, "RPG<-RDA msg sent (msg type: %d)\n", 
                 message_type);


      /* these ORDA messages do not have the message headers embedded within the message
         so they have to be handled differently */

   if ((message_type == CLUTTER_FILTER_BYPASS_MAP) ||
       (message_type == PERFORMANCE_MAINTENANCE_DATA)   || 
       (message_type == CLUTTER_MAP_DATA)   ||
       (message_type == ADAPTATION_DATA)) {
        Process_orda_message (message_type);
        return;
   }

         /* if the msg size array is 0, then get the msg size from the
            msg header */

   if ( Message_size [message_type] == 0 ) {
      RDA_RPG_message_header_t *msg_hdr;
      msg_hdr = (RDA_RPG_message_header_t *) Message_pointer[message_type];
      Message_size[message_type] = msg_hdr->size*sizeof(short);
   }

         /* If the Remote VCP data is being sent via message RDA_VCP_MSG,
            use the size from the Remote VCP message. */
   if( (message_type == RDA_VCP_MSG) && (Remote_vcp != NULL) ) {
      RDA_RPG_message_header_t *msg_hdr;
      msg_hdr = (RDA_RPG_message_header_t *) Remote_vcp;
      Message_size[message_type] = msg_hdr->size*sizeof(short);
   }

      /* compute the # msg segments for this message */

   if (message_type != GENERIC_DIGITAL_RADAR_DATA){

      number_msg_segments = Message_size [message_type] / MAX_MESSAGE_SIZE;

      if (Message_size [message_type] % MAX_MESSAGE_SIZE)
          number_msg_segments++;

   }
   else{

      number_msg_segments = Message_size [message_type] / MAX_MESSAGE_31_SIZE;

      if (Message_size [GENERIC_DIGITAL_RADAR_DATA] % MAX_MESSAGE_31_SIZE)
          number_msg_segments++;

   }

      /* write all the message segments for this message */

   for (i = 1; i <= number_msg_segments; i++) {
      if ( i < number_msg_segments) {

         if (message_type != GENERIC_DIGITAL_RADAR_DATA)
            msg_size_bytes = MAX_MESSAGE_SIZE;
         else
            msg_size_bytes = MAX_MESSAGE_31_SIZE;
         msg_size_halfwords = msg_size_bytes / sizeof (short); 
      } else {
         msg_size_bytes = Message_size [message_type] - total_bytes_sent;
         msg_size_halfwords = msg_size_bytes / sizeof (short);

         if ((msg_size_bytes % 2) > ZERO)
            msg_size_halfwords++;
      }

         /* For byte swapping purposes, the message structure for
            several messages is copied to the Message_pointer array. 
            The Message_pointer in these cases is used to point to 
            buffer space for byte swapping to be performed so that
            the actual message structure retains its internal format. */

      if ( message_type == GENERIC_DIGITAL_RADAR_DATA ){

         int j, offset, header_size, num_blocks = 0;
         Generic_basedata_t *gbt = (Generic_basedata_t *) Message_pointer[GENERIC_DIGITAL_RADAR_DATA];

            /* Copy the Generic_basedata_t structure (Message Header and Radial Header) */

         header_size  = sizeof(Generic_basedata_t) + 
                         Radial_message.hdr.base.no_of_datum*sizeof(int);

         memcpy (Message_pointer [GENERIC_DIGITAL_RADAR_DATA], &Radial_message, header_size);

         offset = header_size;

            /* Copy the volume header, the elevation header and the radial header. */

         header_size = sizeof(Generic_vol_t);

         memcpy (Message_pointer [GENERIC_DIGITAL_RADAR_DATA] + offset, 
                 &Radial_message.vol_hdr, header_size );

         gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
         offset += header_size;
         num_blocks++;

         header_size = sizeof(Generic_elev_t);
         memcpy (Message_pointer [GENERIC_DIGITAL_RADAR_DATA] + offset, 
                 &Radial_message.elv_hdr, header_size );

         gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
         offset += header_size;
         num_blocks++;

         header_size = sizeof(Generic_rad_t) + sizeof(Generic_rad_dBZ0_t);
         memcpy (Message_pointer [GENERIC_DIGITAL_RADAR_DATA] + offset, 
                 &Radial_message.rad_hdr, header_size );

         gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
         offset += header_size;
         num_blocks++;

         for( j = 0; j < Radial_message.hdr.base.no_of_datum; j++ ){

            if( Radial_message.hdr.base.data[j] != 0 ){

               switch( Radial_message.hdr.base.data[j] ){

                  case RDASIM_REF_DATA:

                     gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
                     num_blocks++;

                        /* Copy Generic_moment_t (header). */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset, 
                             &Radial_message.ref_hdr, sizeof(Generic_moment_t) );

                     offset += sizeof(Generic_moment_t);  
                     
                        /* Copy data. */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset,
                             &Radial_message.ref[0], Radial_message.ref_hdr.no_of_gates );

                     offset += Radial_message.ref_hdr.no_of_gates;

                        /* Ensure the next Generic_moment_t structure is aligned. */

                     offset += sizeof(float)/sizeof(char) - (offset % (sizeof(float)/sizeof(char)));
                     break;

                  case RDASIM_VEL_DATA:

                     gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
                     num_blocks++;

                        /* Copy Generic_moment_t (header). */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset, 
                             &Radial_message.vel_hdr, sizeof(Generic_moment_t) );

                     offset += sizeof(Generic_moment_t);  
                     
                        /* Copy data. */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset,
                             &Radial_message.vel[0], Radial_message.vel_hdr.no_of_gates );

                     offset += Radial_message.vel_hdr.no_of_gates;

                        /* Ensure the next Generic_moment_t structure is aligned. */

                     offset += sizeof(float)/sizeof(char) - (offset % (sizeof(float)/sizeof(char)));
                     break;

                  case RDASIM_WID_DATA:

                     gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
                     num_blocks++;

                        /* Copy Generic_moment_t (header). */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset, 
                             &Radial_message.wid_hdr, sizeof(Generic_moment_t) );

                     offset += sizeof(Generic_moment_t);  
                     
                        /* Copy data. */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset,
                             &Radial_message.wid[0], Radial_message.wid_hdr.no_of_gates );

                     offset += Radial_message.wid_hdr.no_of_gates;

                        /* Ensure the next Generic_moment_t structure is aligned. */

                     offset += sizeof(float)/sizeof(char) - (offset % (sizeof(float)/sizeof(char)));
                     break;

                  case RDASIM_ZDR_DATA:

                     gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
                     num_blocks++;

                        /* Copy Generic_moment_t (header). */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset, 
                             &Radial_message.zdr_hdr, sizeof(Generic_moment_t) );

                     offset += sizeof(Generic_moment_t);  
                     
                        /* Copy data. */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset,
                             &Radial_message.zdr[0], Radial_message.zdr_hdr.no_of_gates );

                        /* Ensure the next Generic_moment_t structure is aligned. */

                     offset += Radial_message.zdr_hdr.no_of_gates;

                     offset += sizeof(float)/sizeof(char) - (offset % (sizeof(float)/sizeof(char)));
                     break;

                  case RDASIM_PHI_DATA:

                     gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
                     num_blocks++;

                        /* Copy Generic_moment_t (header). */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset, 
                             &Radial_message.phi_hdr, sizeof(Generic_moment_t) );

                     offset += sizeof(Generic_moment_t);  
                     
                        /* Copy data. */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset,
                             &Radial_message.phi[0], Radial_message.phi_hdr.no_of_gates * sizeof(short) );

                     offset += Radial_message.zdr_hdr.no_of_gates*sizeof(short);

                        /* Ensure the next Generic_moment_t structure is aligned. */

                     offset += sizeof(float)/sizeof(short) - (offset % (sizeof(float)/sizeof(short)));
                     break;

                  case RDASIM_RHO_DATA:

                     gbt->base.data[num_blocks] = offset - sizeof(RDA_RPG_message_header_t);
                     num_blocks++;

                        /* Copy Generic_moment_t (header). */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset, 
                             &Radial_message.rho_hdr, sizeof(Generic_moment_t) );

                     offset += sizeof(Generic_moment_t);  
                     
                        /* Copy data. */

                     memcpy( Message_pointer[GENERIC_DIGITAL_RADAR_DATA] + offset,
                             &Radial_message.rho[0], Radial_message.rho_hdr.no_of_gates );

                     offset += Radial_message.rho_hdr.no_of_gates;

                        /* Ensure the next Generic_moment_t structure is aligned. */

                     offset += sizeof(float)/sizeof(char) - (offset % (sizeof(float)/sizeof(char)));
                     break;

                  default:
                     break;

               } /* End of "switch( Radial_message.hdr.base.data[j] )" */

            }  /* End of "if( Radial_message.hdr.base.data[j] != 0 )" */

         } /* End of "for( j = 0; j < Radial_message.hdr.base.no_of_datum; j++ )" */

           /* Set the message size based on the actual amount of data within radial. */

         msg_size_bytes = offset;
         msg_size_halfwords = (offset + 1)/sizeof(short);

      }

      else if (message_type == RDA_STATUS_DATA)
         memcpy (Message_pointer [message_type], &RDA_status_msg, 
                 Message_size [message_type]);

      else if ( (message_type == RDA_VCP_MSG) && (Remote_vcp != NULL) )
         memcpy (Message_pointer [message_type], Remote_vcp, 
                 Message_size [message_type]);

      data_buffer = Message_pointer [message_type] + total_bytes_sent; 

      if (message_type == LOOPBACK_TEST_RPG_RDA) {
         RDA_RPG_message_header_t *msg_hdr;

         msg_hdr = (RDA_RPG_message_header_t *) data_buffer;
         msg_size_halfwords = msg_hdr->size;
         msg_size_bytes = msg_size_halfwords * sizeof (short); 
      }
      
      msg_segment_number = i;
      
      Construct_msg_header (&rda_rpg_msg_hdr, msg_size_halfwords, message_type, 
                            number_msg_segments, msg_segment_number);

      memcpy (data_buffer, (char *) &rda_rpg_msg_hdr, 
              sizeof (RDA_RPG_message_header_t));

         /* Byte swap the data if this is a little endian machine */

      if (MISC_i_am_bigendian() == 0) {
         if ((ret = Byte_swap_ICD_msg (data_buffer + sizeof (RDA_RPG_message_header_t),
                                       &rda_rpg_msg_hdr)) < 0) {
            printf ("Error byte swapping RDA->RPG msg type %d (ret: %d)\n", 
                    message_type, ret);
            continue;
         }

         if ((ret = Byte_swap_msg_hdr ((RDA_RPG_message_header_t *) data_buffer)) < 0) {
            printf ("Error byte swapping msg header (ret: %d)\n", ret);
            continue;
         }

      }

      Write_message (data_buffer, message_type, msg_size_bytes);

      total_bytes_sent += msg_size_bytes;
   }

       /* the RDA status message is sent so frequently that special handling 
          was placed in this routine for convenience */
   
   if (message_type == RDA_STATUS_DATA)
       Clear_status_msg ();

   return;
}


/********************************************************************************

    Description: This routine processes all messages received from the rpg

         Inputs:      

********************************************************************************/

void RRM_process_rpg_msg ()
{
   int ret;
   RDA_RPG_message_header_t *rda_rpg_msg_header;
   RPG_request_data_t *request_type;
   int processing_state;
   int ctm_msg_hdr = sizeof (CTM_header_t);

   processing_state = RD_get_processing_state ();

   if ((processing_state == RDA_RESTART) || (processing_state == START_UP))
      return;

      /* process the message written from the RPG to the RDA */

   rda_rpg_msg_header = (RDA_RPG_message_header_t *) (LINk.w_buf + 
                         ctm_msg_hdr);

      /* Byte swap the message if this is a little endian machine */

   if (MISC_i_am_bigendian() == 0) {
      if ((ret = Byte_swap_msg_hdr (rda_rpg_msg_header)) < 0) {
         printf ("Error byte swapping msg header (ret: %d)\n", ret);
         return;
      }

      if ((ret = Byte_swap_ICD_msg 
                 (LINk.w_buf + ctm_msg_hdr + sizeof (RDA_RPG_message_header_t),
                  rda_rpg_msg_header)) < 0) {
           printf ("Error byte swapping RPG->RDA msg type %d (ret: %d)\n", 
                   rda_rpg_msg_header->type, ret);
           return;
      }
   }

   if (VErbose_mode > ZERO)
        fprintf (stdout, "RPG->RDA msg received (msg type: %d)\n", 
                 rda_rpg_msg_header->type);

   switch (rda_rpg_msg_header->type) {

      case RDA_CONTROL_COMMANDS:
         RD_process_control_command ();
         break;

      case RPG_RDA_VCP:
         if (RDA_status_msg.control_status == RCS_RDA_LOCAL_ONLY)
             RRM_set_rda_alarm (RDA_ALARM_INVALID_REMOTE_VCP_RECEIVED);
         else {
            VCP_message_header_t *vcp_msg_hdr;
            char *vcp_msg;

            vcp_msg = (char *) (LINk.w_buf + ctm_msg_hdr);
            vcp_msg_hdr = (VCP_message_header_t *) (LINk.w_buf +
                           ctm_msg_hdr +
                           sizeof (RDA_RPG_message_header_t));

            if ((Copy_remote_vcp (vcp_msg)) < 0)
                RRM_set_rda_alarm (RDA_ALARM_INVALID_REMOTE_VCP_RECEIVED);
            else {
               if (VErbose_mode >= 3)
                  fprintf (stdout, "          RPG VCP pattern #: %d\n", 
                           vcp_msg_hdr->pattern_number);
               RDA_status_msg.command_status = CA_REMOTE_VCP_RECEIVED;
               RRM_process_rda_message (RDA_STATUS_DATA);
            }
         }
         break;

      case CLUTTER_SENSOR_ZONES:
         if (RDA_status_msg.control_status == RCS_RDA_LOCAL_ONLY)
             RRM_set_rda_alarm (RDA_ALARM_INVALID_CENSOR_ZONE_MESSAGE_RECEIVED);

            /* update the Bypass Map generation Date and time */
  
         Update_map_generation_timestamp (CLUTTER_SENSOR_ZONES);
         RD_set_send_status_msg_flag ();

         break;

      case REQUEST_FOR_DATA:
         
         request_type = (RPG_request_data_t *) (LINk.w_buf + ctm_msg_hdr +
                         sizeof (RDA_RPG_message_header_t)); 

         if (VErbose_mode > ZERO)
            fprintf (stdout, "         data request type: %d\n", 
                     *request_type);

         switch (*request_type) {
            case REQUEST_FOR_STATUS_DATA:
            case REQUEST_FOR_PERFORMANCE_DATA:
            case REQUEST_FOR_BYPASS_MAP_DATA:
            case REQUEST_FOR_NOTCHWIDTH_MAP_DATA:
               RD_set_request_for_data_flag (*request_type);
               break;

            default:
               RRM_set_rda_alarm (RDA_ALARM_INVALID_RPG_COMMAND_RECEIVED);
               fprintf (stderr, 
                     "Unknown request type received from the rpg (type = %d)\n", 
                     *request_type);
         }
         break;

      case CONSOLE_MESSAGE_G2A:
          /* who cares...dump the message */
         break;

      case LOOPBACK_TEST_RDA_RPG:
         RD_set_rda_loopback_ack_flag ();
         break;

      case LOOPBACK_TEST_RPG_RDA:
         Echo_rpg_rda_loopback_msg ();
         break;

      case EDITED_CLUTTER_FILTER_MAP:
/*         if (RDA_status_msg.control_status == RCS_RDA_LOCAL_ONLY)
             RRM_set_rda_alarm (RDA_ALARM_INVALID_CENSOR_ZONE_MESSAGE_RECEIVED);
*/
         Update_bypass_map (LINk.w_buf + ctm_msg_hdr);
         break;

      default:
         fprintf (stderr, "Unknown RPG->RDA message received (msg type: %d)\n",
                  rda_rpg_msg_header->type);
         RRM_set_rda_alarm (RDA_ALARM_INVALID_RPG_COMMAND_RECEIVED);
   }

   return;
}


/********************************************************************************
 
     Description: Add the rda alarm to the status message

           Input: alrm_code -- The alarm code

          Output: The alarm code added to the status message

          Return:

 ********************************************************************************/

void RRM_set_rda_alarm (int alarm_code)
{
   int i;

   for (i = ZERO; i < MAX_RDA_ALARMS_PER_MESSAGE; i++) {
         /* search for an unused alarm index, or an alarm that has been 
            cleared (ie. alarm code < 0) */

      if ((RDA_status_msg.alarm_code[i] < ZERO) ||
          (RDA_status_msg.alarm_code[i] == ZERO)) {
         RDA_status_msg.alarm_code[i] = alarm_code;
         break;
      }
   }

   RD_set_send_status_msg_flag ();

   return;
}

/********************************************************************************
 
     Description: Update the msg size in the msg array

           Input: msg_type - the msg type to update
                  msg_size - size of the msg

          Output: 

          Return:

 ********************************************************************************/

void RRM_update_msg_size (short msg_type, int msg_size)
{
   Message_size [msg_type] = msg_size;
   return;
}


/********************************************************************************

     Description: This routine clears the temporary buffers used to construct
                  new Bypass Maps received from the rpg

 ********************************************************************************/

static void Clear_new_bypass_map_buffers (int number_elements, char **seg_aray, 
                                          int *seg_size, int *number_segments, 
                                          int *msg_processing_flag, 
                                          int *number_segs_processed)
{
   int i;

   for (i = 0; i < number_elements; i++) {
      seg_aray [i] = NULL;
      seg_size [i] = 0;
   }

   *number_segments = 0;
   *msg_processing_flag = FALSE;
   *number_segs_processed = 0;
   return;
}


/********************************************************************************
 
    Description: Clear all RDA status message fields that should be reset once a 
                 status message is sent.

          Input: RDA_status_msg

         Output:

        Globals: RDA status message (RDA_status_msg)

         Return:

 ********************************************************************************/

static void Clear_status_msg ()
{
   int i;

   RDA_status_msg.command_status = CA_NO_ACKNOWLEDGEMENT;

     /* clear the alarms */

   for (i = ZERO; i <= 13; i++)
       RDA_status_msg.alarm_code[i] = ZERO;

   return;
}


/********************************************************************************

     Description: Close all the message files
     
           Input: bypas_map_fd     - bypass map file descriptor
                  loopbck_msg_fd   - RDA-to-RPG loopback message file descriptor
                  notchwdth_map_fd - notchwidth map file descriptor
                  perf_msg_fd      - performance/maintenance message file descriptor

          Output:

          Return:

 ********************************************************************************/

static void Close_msg_files (FILE *bypas_map_fd, FILE *loopbck_msg_fd, 
                             FILE *notchwdth_map_fd, FILE *perf_msg_fd,
                             FILE *rda_adapt_dat_file_ptr)
{
   if (fclose (bypas_map_fd) == -1) {
      fprintf (stderr, "error closing bypass map file: ");
      perror ("");
   }

   if (fclose (loopbck_msg_fd) == -1) {
      fprintf (stderr, "error closing loopback message file: ");
      perror ("");
   }

   if (fclose (notchwdth_map_fd) == -1) {
      fprintf (stderr, "error closing notchwidth map file: ");
      perror ("");
   }

   if (fclose (perf_msg_fd) == -1) {
      fprintf (stderr, "error closing performance/maintenance file: ");
      perror ("");
   }

   if (fclose (rda_adapt_dat_file_ptr) == -1) {
      fprintf (stderr, "error closing RDA Adaptation Data file: ");
      perror ("");
   }

   return;
}


/********************************************************************************
  
    Description: This subroutine updates the common message header fields for 
                 all messages.

          Input: *msg_header_ptr - pointer to the message header
                 size            - message size
                 type            - message type
                 number_segments - number of segments in this message
                 segment_number  - segment number of this message

         Output: message header data

         Return:
                     
********************************************************************************/

static void Construct_msg_header (RDA_RPG_message_header_t *msg_header_ptr,
                                  short size, unsigned char type, 
                                  short number_segments, short segment_number)
{
   static short sequence_number = 32500; /* the message sequence number */
   time_t julian_time;          /* the current julian time in seconds */
   unsigned short julian_date;  /* the computed julian date */
   int msec_time;               /* # milliseconds since midnight */   

   msg_header_ptr->sequence_num = sequence_number;

   msg_header_ptr->size = size;
   msg_header_ptr->rda_channel = RDA_channel_number;

      /* Set the msg hdr bit specifying ORDA */

   msg_header_ptr->rda_channel |= 0x08;
      
   msg_header_ptr->type = type;
   msg_header_ptr->num_segs = number_segments;
   msg_header_ptr->seg_num = segment_number;
   
   if (type == CLUTTER_FILTER_BYPASS_MAP      && 
        BAD_segment_bypass == TRUE            && 
       segment_number == number_segments) {
      msg_header_ptr->seg_num = segment_number + 2;
      BAD_segment_bypass = FALSE;
   } else if(type == NOTCHWIDTH_MAP_DATA       && 
             BAD_segment_notch == TRUE         && 
             segment_number == number_segments) {
        msg_header_ptr->seg_num = segment_number + 2;
        BAD_segment_notch = FALSE;
   } else
      msg_header_ptr->seg_num = segment_number;
   
   julian_time = time (NULL);

      /* compute the current julian date */

   julian_date = (unsigned short) ((julian_time / 
                                   (time_t) SECONDS_IN_A_DAY) + ONE);
   msg_header_ptr->julian_date = julian_date;

      /* compute the elapsed milliseconds since midnight */

   msec_time = (int) ((julian_time % (time_t) SECONDS_IN_A_DAY) * 
                       MILLISECONDS_PER_SECOND);
   msg_header_ptr->milliseconds = msec_time;
   
   ++sequence_number;

   return;
}


/********************************************************************************
  
    Description: Copy the remote VCP received from the RPG to the local buffer
   
          Input: msg_ptr - pointer to the RPG VCP msg to copy 

         Output:

         Return:

********************************************************************************/

static int Copy_remote_vcp (char *msg_ptr)
{
   static int msg_size = 0;
   RDA_RPG_message_header_t *msg_hdr;

      /* allocate buffer space for the remote VCP msg if it has not 
         already been allocated */

   if (Remote_vcp == NULL) {
      Remote_vcp = malloc (sizeof (RDA_RPG_message_header_t) +
                           sizeof (VCP_message_header_t)     +
                           sizeof (VCP_elevation_cut_header_t));

      if (Remote_vcp == NULL) {
         fprintf (stderr, "Error malloc'ing remote VCP Msg buffer space\n");
         MA_terminate ();
      }
   }

   msg_hdr = (RDA_RPG_message_header_t *) msg_ptr;

   msg_size = msg_hdr->size * sizeof (short);
   memcpy (Remote_vcp, msg_ptr, msg_size);

   return (0);
}


/********************************************************************************
  
    Description: Send the RPG-to-RDA loopback msg back to the RPG
   
          Input:  

         Output:

         Return:

********************************************************************************/

static void Echo_rpg_rda_loopback_msg ()
{
   int ctm_msg_hdr = sizeof (CTM_header_t);


   if (LINk.w_size > sizeof (RDA_RPG_loop_back_message_t)) {
      fprintf (stderr, "RPG loopback msg size received > msg type size: ");
      fprintf (stderr, "msg received size = %d,  msg type size = %d\n",
               LINk.w_size, sizeof (RDA_RPG_loop_back_message_t));
      return;
   }

   if( !LOOpback_timeout ) {

      memcpy ((char *) Message_pointer [LOOPBACK_TEST_RPG_RDA],
               LINk.w_buf + ctm_msg_hdr, LINk.w_size - ctm_msg_hdr);  

      if( LOOpback_scramble ){

         RDA_RPG_loop_back_message_t *test_data =  (RDA_RPG_loop_back_message_t *) 
                      ((char *) Message_pointer [LOOPBACK_TEST_RPG_RDA] +
                        sizeof( RDA_RPG_message_header_t));

         fprintf (stderr, "Scramble the RPG/RDA loopback message\n" );

         test_data->pattern[0] = 0x9843;
         test_data->pattern[1] = 0x5728;
         LOOpback_scramble = FALSE;
      }

      RRM_process_rda_message (LOOPBACK_TEST_RPG_RDA);
   } else
      LOOpback_timeout = FALSE;

   return;
}


/********************************************************************************

     Initialize the RDA-RPG message arrays

 ********************************************************************************/

static void Initialize_msg_arrays (char *bypass_msg, char *perf_msg, 
                            char *notchwidth_msg, char *loopback_msg_rda_rpg, 
                            char *loopback_msg_rpg_rda, char *rda_adapt_data_msg,
                            int bypass_file_size, int perf_file_size, 
                            int notchwidth_size, int loopback_size,
                            int rda_adapt_data_size)
{
   int i;

   for (i = 0; i < NUMBER_ICD_MSGS; i++) {
      Message_size [i] = 0;
      Message_pointer [i] = NULL;
   }

      /* message size array */

   Message_size [GENERIC_DIGITAL_RADAR_DATA] = sizeof (Radial_message_t);
   Message_size [RDA_STATUS_DATA] = sizeof (RDA_status_msg_t);
   Message_size [PERFORMANCE_MAINTENANCE_DATA] = perf_file_size;
   Message_size [LOOPBACK_TEST_RDA_RPG] = loopback_size;
   Message_size [LOOPBACK_TEST_RPG_RDA] = loopback_size;
   Message_size [CLUTTER_FILTER_BYPASS_MAP] = bypass_file_size;
   Message_size [NOTCHWIDTH_MAP_DATA] = notchwidth_size;
   Message_size [RDA_ADAPTATION_DATA] = rda_adapt_data_size;
   Message_size [RDA_VCP_MSG] = 0;  /* the msg size is updated when the msg is constructed */

       /* pointer array to each message structure */

   Message_pointer [GENERIC_DIGITAL_RADAR_DATA] = 
                   malloc (Message_size [GENERIC_DIGITAL_RADAR_DATA]);

   if ((Message_pointer [GENERIC_DIGITAL_RADAR_DATA] ) == NULL) {
      printf ("Error malloc'ing Message_pointer memory...tool aborting\n");
      exit (1);
   }

   Message_pointer [RDA_STATUS_DATA] = malloc (Message_size [RDA_STATUS_DATA]);
   if ((Message_pointer [RDA_STATUS_DATA] ) == NULL) {
      printf ("Error malloc'ing Message_pointer memory...tool aborting\n");
      exit (1);
   }
   Message_pointer [PERFORMANCE_MAINTENANCE_DATA] = perf_msg;
   Message_pointer [LOOPBACK_TEST_RDA_RPG] = loopback_msg_rda_rpg;
   Message_pointer [LOOPBACK_TEST_RPG_RDA] = loopback_msg_rpg_rda;
   Message_pointer [CLUTTER_FILTER_BYPASS_MAP] = bypass_msg;
   Message_pointer [NOTCHWIDTH_MAP_DATA] = notchwidth_msg;
   Message_pointer [RDA_ADAPTATION_DATA] = rda_adapt_data_msg;

      /* allocate buffer space for the RDA VCP msg */

   Message_pointer [RDA_VCP_MSG] = malloc (sizeof (RDA_RPG_message_header_t) +
                     sizeof (VCP_message_header_t)     +
                     sizeof (VCP_elevation_cut_header_t));

   if (Message_pointer [RDA_VCP_MSG] == NULL) {
      fprintf (stderr, 
          "Copy_remote_vcp: Error malloc'ing local VCP Msg buffer space\n");
      MA_terminate ();
   }

   return;
}


/********************************************************************************
 
    Description: Initialize the RDA Status message

          Input: RDA_status_msg

         Output:

        Globals: RDA status message (RDA_status_msg)

         Return:

 ********************************************************************************/

static void Initialize_status_msg ()
{
   int i;

      /* initialize the rda status message */

   RDA_status_msg.rda_status = 0;
   RDA_status_msg.op_status = ROS_RDA_ONLINE;
   RDA_status_msg.control_status = RCS_RDA_EITHER;
   RDA_status_msg.aux_pwr_state = APGS_UTILITY_PWR_AVAILABLE;
   RDA_status_msg.ave_trans_pwr = 0x0582;
   DATa_trans_enbld = DTE_NONE_ENABLED;
   RDA_status_msg.data_trans_enbld = DATa_trans_enbld;
   RDA_status_msg.vcp_num = CR_get_current_vcp ();
   RDA_status_msg.rda_control_auth = RCA_NO_ACTION;
   RDA_status_msg.op_mode = ROM_OPERATIONAL;
   RDA_status_msg.super_res = 2;  /* set Super Res to "Enabled" */
   RDA_status_msg.cmd = 2;  /* set CMD to "Enabled" */
   RDA_status_msg.avset = 4;  /* set AVSET to "Disabled" */
   RDA_status_msg.rda_alarm = 0;
   RDA_status_msg.command_status = CA_NO_ACKNOWLEDGEMENT;
   RDA_status_msg.spot_blanking_status = SBS_NOT_INSTALLED;

      /* update the gnereaton date and time for the Bypass Map and 
         Notchwidth map */

   Update_map_generation_timestamp (CLUTTER_FILTER_BYPASS_MAP);
   Update_map_generation_timestamp (CLUTTER_SENSOR_ZONES);
   
      /* assign ORDA unique fields */

   {
      ORDA_status_msg_t *orda_msg_ptr = (ORDA_status_msg_t *) &RDA_status_msg;

         /* set to -1.0 (scaled val = -100) */

      RDA_status_msg.ref_calib_corr = 0xff9c;
      RDA_status_msg.vc_ref_calib_corr = 0xff9c;

         /* set rda build # to 14.0 scaled by 100 */

      orda_msg_ptr->rda_build_num = 1400;  

	/* set super resolution to enabled. */

      orda_msg_ptr->super_res = SR_ENABLED;

      orda_msg_ptr->cmd = CMD_ENABLED;

   }

      /* clear the alarms */

   for (i = 0; i <= 13; i++)
           RDA_status_msg.alarm_code[i] = 0;
  
   return;
}


/********************************************************************************
  
    Description: This routine processes all ORDA->RPG message types that do
                 not have the message headers embedded within the messafe
                 messages written to the rpg)
   
          Input: message_type - the message type to process

         Output:

         Return:

       Note: These messages had the msg headers removed to ease the burden
             of typecasting to a C structure

********************************************************************************/

#define ICD_MSG_HDR_SIZE 16
#define MAX_ICD_MSG_SIZE 2416
#define MAX_ICD_MSG_BODY_SIZE 2400

static void Process_orda_message (message_type)
{
   int  file_size_in_bytes = 0;
   int  number_msg_segments = 0;
   int  total_bytes_written = 0;
   int  bytes_in_file_processed = 0;
   int  msg_segment_number = 1;
   int  msg_size_bytes = 0;
   int  msg_size_halfwords = 0;
   int  i;
   int  ret;
   char msg_buffer [MAX_ICD_MSG_SIZE];

   file_size_in_bytes = Message_size [message_type]; 

   number_msg_segments = (int) (file_size_in_bytes / MAX_ICD_MSG_BODY_SIZE);

   if ((int) (file_size_in_bytes % MAX_ICD_MSG_BODY_SIZE) > 0)
       ++number_msg_segments;

      /* adjust file size to consider the message segment headers */

   file_size_in_bytes += (number_msg_segments * ICD_MSG_HDR_SIZE);

   if (VErbose_mode >= 3)
       fprintf (stdout, 
          "   file_size_in_bytes(including msg headers): %d;   #_msg_segments: %d\n",
          file_size_in_bytes, number_msg_segments);
 
      /* write all the message segments for this message */

   for (i = 1; i <= number_msg_segments; i++) {
      if ( i < number_msg_segments) {
         msg_size_bytes = MAX_ICD_MSG_SIZE;
         msg_size_halfwords = msg_size_bytes / sizeof (short); 
      } else {
         msg_size_bytes = file_size_in_bytes - total_bytes_written;
         msg_size_halfwords = msg_size_bytes / sizeof (short);

         if ((msg_size_bytes % 2) > ZERO)
            msg_size_halfwords++;
      }
     
      memcpy (msg_buffer + ICD_MSG_HDR_SIZE, Message_pointer[message_type] + 
              bytes_in_file_processed, msg_size_bytes - ICD_MSG_HDR_SIZE);

      msg_segment_number = i;
      
      Construct_msg_header ((RDA_RPG_message_header_t *) msg_buffer, msg_size_halfwords, 
                             message_type, number_msg_segments, msg_segment_number);

         /* Byte swap the data if this is a little endian machine */

      if (MISC_i_am_bigendian() == 0) {
         if ((ret = Byte_swap_msg_hdr ((RDA_RPG_message_header_t *) msg_buffer)) < 0) {
            printf ("Error byte swapping msg header (ret: %d)\n", ret);
            continue;
         }
      }

         /* write the data to teh RPG */

      Write_message (msg_buffer, message_type, msg_size_bytes);

      total_bytes_written += msg_size_bytes;
      bytes_in_file_processed += (msg_size_bytes - ICD_MSG_HDR_SIZE);

   }
   return;
}


/********************************************************************************
  
     Description: Read the message files into the message structures
    
           Input:  bypas_ptr     - bypass map file pointer
                   loopbck_ptr   - RDA-to-RPG loopback message file pointer
                   notchwdth_ptr - notchwidth map file descriptor
                   perf_ptr      - performance/maintenance message file pointer

          Output: pointers to the messages, and the message/file sizes
     
          Return: 0 on success, -1 on failure

 *******************************************************************************/

static int Read_msg_files (FILE *bypas_ptr, FILE *loopbck_ptr, FILE *notchwdth_ptr,
                           FILE *perf_ptr, FILE *rda_adapt_dat_ptr)
{
   int ret;                     /* read function return code */
   struct stat stat_buf;        /* file status buffer */
   int  loopback_msg_size;      /* size of the loopback msg */
   int  notchwidth_file_size;   /* size of the notchwidth file */
   int  perf_maint_file_size;   /* size of the performance/maint file */
   int  bypass_map_size;        /* size of the Bypass Map */
   int  rda_adapt_dat_file_size;/* size of the RDA Adapt Data msg file */
   char *performance_msg;       /* Performance/Maintenance message */
   char *bypass_map_msg;        /* Bypass map message */
   char *notchwidth_map_msg;    /* Notchwidth map message */
   char *loopback_msg_rda_rpg;  /* Rda-to-RPG loopback message */
   char *loopback_msg_rpg_rda;  /* Rda-to-RPG loopback message */
   char *rda_adapt_dat_msg;     /* Rda Adapt Data message */

      /* read the bypass map msg file */

   fstat ((fileno (bypas_ptr)), &stat_buf);
   bypass_map_size = (int) stat_buf.st_size;
     
   if (VErbose_mode >= 3)
       fprintf (stdout, "Bypass Map file size: %d\n", bypass_map_size);

   if ((bypass_map_msg = malloc ((size_t) bypass_map_size)) == NULL) {
      fprintf (stderr, "malloc failure for Bypass Map File\n");
      return (-1);
   }

   if ((ret = fread (bypass_map_msg, sizeof (char), 
                     bypass_map_size, bypas_ptr)) == -1) {
      fprintf (stderr, 
               "Error reading the Bypass Map message file (err = %d)\n", errno);
      return (-1);
   }

      /* read the loopback msg file */

   loopback_msg_size = sizeof (RDA_RPG_loop_back_message_t) + 
                       sizeof (RDA_RPG_message_header_t);
     
   if (VErbose_mode >= 3)
      fprintf (stdout, "Loopback Msg size: %d\n", loopback_msg_size);

   if ((loopback_msg_rda_rpg = malloc ((size_t) loopback_msg_size)) == NULL) {
      fprintf (stderr, "malloc failure for RDA-RPG Loopback Msg\n");
      return (-1);
   }

   if ((ret = fread (loopback_msg_rda_rpg, sizeof (char), 
                     loopback_msg_size, loopbck_ptr)) == -1) {
      fprintf (stderr, "Error reading the Loopback message file (err = %d)\n", errno);
      return (-1);
   }

      /* allocate memory for the RPG-RDA loopback message */

   if ((loopback_msg_rpg_rda = malloc ((size_t) loopback_msg_size)) == NULL) {
      fprintf (stderr, "malloc failure for RPG-RDA Loopback Msg\n");
      return (-1);
   }

      /* read the notchwidth map msg file */

   fstat ((fileno(notchwdth_ptr)), &stat_buf);
   notchwidth_file_size = (int) stat_buf.st_size;
     
   if (VErbose_mode >= 3)
      fprintf (stdout, "Notchwidth Map file size: %d\n", notchwidth_file_size);

   if ((notchwidth_map_msg = malloc ((size_t) notchwidth_file_size)) == NULL) {
      fprintf (stderr, "malloc failure for Notchwidth Map File\n");
      return (-1);
   }
   
   if ((ret = fread (notchwidth_map_msg, sizeof (char), 
                     notchwidth_file_size, notchwdth_ptr)) == -1) {
      fprintf (stderr, "Error reading the Bypass Map message file (err = %d)\n", errno);
      return (-1);
   }

      /* read the performance/maintenance msg file */

   fstat ((fileno(perf_ptr)), &stat_buf);
   perf_maint_file_size = (int) stat_buf.st_size;
     
   if (VErbose_mode >= 3)
      fprintf (stdout, "Performance/Maint file size: %d\n", perf_maint_file_size);

   if ((performance_msg = malloc ((size_t) perf_maint_file_size)) == NULL) {
      fprintf (stderr, "malloc failure for Performance/Maint File\n");
      return (-1);
   }

   if ((ret = fread (performance_msg, sizeof (char), 
                     perf_maint_file_size, perf_ptr)) == -1) {
      fprintf (stderr, 
               "Error reading the Performance/Maint message file (err = %d)\n", errno);
      return (-1);
   }

      /* read the RDA Adaptation Data msg file */

   fstat ((fileno (rda_adapt_dat_ptr)), &stat_buf);
   rda_adapt_dat_file_size = (int) stat_buf.st_size;
     
   if (VErbose_mode >= 3)
       fprintf (stdout, "RDA Adaptation Data File size: %d\n", 
                rda_adapt_dat_file_size);

   if ((rda_adapt_dat_msg = malloc ((size_t) rda_adapt_dat_file_size)) == NULL) {
      fprintf (stderr, "malloc failure for RDA Adaptation Data File\n");
      return (-1);
   }

   if ((ret = fread (rda_adapt_dat_msg, sizeof (char), 
                     rda_adapt_dat_file_size, rda_adapt_dat_ptr)) == -1) {
      fprintf (stderr, "Error reading the RDA Adaptation Data message file (err = %d)\n", errno);
      return (-1);
   }

   Initialize_msg_arrays (bypass_map_msg, performance_msg, notchwidth_map_msg, 
                          loopback_msg_rda_rpg, loopback_msg_rpg_rda, 
                          rda_adapt_dat_msg, bypass_map_size, 
                          perf_maint_file_size, notchwidth_file_size, 
                          loopback_msg_size, rda_adapt_dat_file_size);

   return (0);  
}


/**************************************************************************

    Description: This function reallocates the input buffer when more 
                 space is needed.

          Input:

**************************************************************************/

#define ENLARGE_FACTOR 2        /* factor used to increase the input
                                   buffer size */

static void Reallocate_input_buffer ( int len )
{
    int new_size;    /* new buffer size */
    char *new_buf;   /* new buffer created */
    int extra_bytes; /* bytes added for padding or boundary alignment */

    extra_bytes = sizeof (CM_resp_struct) +
                  MA_align_bytes (LINk.n_added_bytes);

    while(1){

        if (LINk.r_buf_size == ZERO)
            new_size = ENLARGE_FACTOR * LINk.packet_size;
        else
            new_size = LINk.r_buf_size * ENLARGE_FACTOR;

        if( new_size >= len )
            break;

    }

    new_buf = malloc (new_size + extra_bytes);

       /* allocate extra space for the response header and the extra bytes */

    if (new_buf == NULL) {
        fprintf (stderr, "malloc failed reallocating input buffer\n");
        MA_terminate (); /* terminate the comm manager */
    }

    if (LINk.r_cnt > ZERO)
        memcpy (new_buf + extra_bytes, LINk.r_buf, LINk.r_cnt);

    if (LINk.r_buf != NULL)
        free (LINk.r_buf - extra_bytes);

    LINk.r_buf = new_buf + extra_bytes;
    LINk.r_buf_size = new_size;

    return;
}


/********************************************************************************

     Dump x number halfwords of all the RDA messages written to the ORPG


 ********************************************************************************/   

static void Rda_msg_dump (char *buffer)
{
   int i; 
   short short_wd;

   short_wd = *((short *) buffer);

   for (i = 0; i < NUM_HFWDS_TO_DUMP; i++) {
      if ((i % 4) == 0) {
         fprintf (stdout, "\n");
         fprintf (stdout, "%04x     %04hx", i, short_wd);
      } else
         fprintf (stdout, "     %04hx", short_wd);

      buffer = buffer + 2;
      short_wd = *((short *) buffer);
   }

   fprintf (stdout, "\n");
   return;
}


/********************************************************************************

    Description: Read the Bypass map msg segments from the rpg and construct
                 a new Bypass map.

          Input: pointer to the bypass map msg segment

 ********************************************************************************/

#define MAX_NUMBER_ELEMENTS     40

static void Update_bypass_map (void *msg_segment)
{
   static char *segment_array [MAX_NUMBER_ELEMENTS] = {NULL, NULL, NULL, NULL, 
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL, 
                                                       NULL, NULL, NULL, NULL,
                                                       NULL, NULL, NULL, NULL};
   static int segment_size [MAX_NUMBER_ELEMENTS];
   static int number_msg_segments;
   static int message_processing_in_progress = FALSE;
   static int number_segments_processed = 0;
   char *tmp_ptr;
   RDA_RPG_message_header_t *msg_hdr;
   int i;

   msg_hdr = (RDA_RPG_message_header_t *) (msg_segment);

      /* if this is the first msg segment to process or if we had a 
         a previous partial read that did not complete, clear the
         new map buffers and re-initialize the message segment info */

   if ((message_processing_in_progress == FALSE)  ||
       (segment_array [msg_hdr->seg_num] != NULL)) {
      Clear_new_bypass_map_buffers (MAX_NUMBER_ELEMENTS, segment_array, 
             segment_size, &number_msg_segments, 
             &message_processing_in_progress, &number_segments_processed);

      message_processing_in_progress = TRUE;
      number_msg_segments = msg_hdr->num_segs;
      number_segments_processed = 0;
   }
      
   segment_size [msg_hdr->seg_num] = msg_hdr->size * sizeof (short);
   segment_array [msg_hdr->seg_num] = malloc (segment_size [msg_hdr->seg_num]);

      /* if malloc failed, then clear everything out...we can't do anything 
         about this */

   if (segment_array [msg_hdr->seg_num] == NULL) {
      Clear_new_bypass_map_buffers (MAX_NUMBER_ELEMENTS, segment_array, 
             segment_size, &number_msg_segments, 
             &message_processing_in_progress, &number_segments_processed);
      return;
   }else {
      memcpy (segment_array [msg_hdr->seg_num], msg_segment, 
              segment_size [msg_hdr->seg_num]);
      ++number_segments_processed;
   }

      /* if all msg segments have been received, then perform a validation check 
         before updating the simulator's map */

   if (number_segments_processed == number_msg_segments) {
      int total_map_size = 0;

      for (i = 0; i < MAX_NUMBER_ELEMENTS; i++) {
         if (((segment_array [i] == NULL) && (segment_size [i] != 0))  ||
             ((segment_array [i] != NULL) && (segment_size [i] == 0))) {
            Clear_new_bypass_map_buffers (MAX_NUMBER_ELEMENTS, segment_array, 
                   segment_size, &number_msg_segments, 
                   &message_processing_in_progress, &number_segments_processed);
            return;
         }
      }

         /* if we've made it this far, then we have received a complete map and 
            it's time to update the simulator's map */

      for (i = 0; i < MAX_NUMBER_ELEMENTS; i++)
         total_map_size += segment_size [i];

      if (total_map_size > Message_size [CLUTTER_FILTER_BYPASS_MAP]) {

         tmp_ptr = (char *) realloc ((short *) Message_pointer [CLUTTER_FILTER_BYPASS_MAP],
                                     (size_t) total_map_size);

            /* clear all the buffers and forget about the update if the
               reallocation fails */

         if (tmp_ptr == NULL) {
            Clear_new_bypass_map_buffers (MAX_NUMBER_ELEMENTS, segment_array, 
                   segment_size, &number_msg_segments, 
                   &message_processing_in_progress, &number_segments_processed);
            return;
         }else { 
            Message_pointer [CLUTTER_FILTER_BYPASS_MAP] = tmp_ptr;
            Message_size [CLUTTER_FILTER_BYPASS_MAP] = total_map_size;
         }
      }

      tmp_ptr = Message_pointer [CLUTTER_FILTER_BYPASS_MAP];
 
      for (i = 0; i < MAX_NUMBER_ELEMENTS; i++) {
         if (segment_array [i] != NULL) {
            memcpy (tmp_ptr, segment_array [i], segment_size [i]);
            tmp_ptr += segment_size [i];
         }
      }

         /* clean up after updating the map */

      Clear_new_bypass_map_buffers (MAX_NUMBER_ELEMENTS, segment_array, 
             segment_size, &number_msg_segments, 
             &message_processing_in_progress, &number_segments_processed);

         /* update the Bypass Map generation Date and time */

      Update_map_generation_timestamp (CLUTTER_FILTER_BYPASS_MAP);
      RD_set_send_status_msg_flag ();

   }
   else  /* we have not received all the msg segments yet */
      return;
}


/********************************************************************************

     Description: This function updates the message generation date and time for 
                  the Notchwidth map and Bypass Map

          Inputs: msg_type - specifies which map info to update 

 ********************************************************************************/

static void Update_map_generation_timestamp (int msg_type)
{
   time_t julian_time;                 /* the current julian time in seconds */
   unsigned short julian_date;         /* the computed julian date */
   unsigned short time_of_day_in_min;  /* # minutes since midnight */

      /* compute the current julian date */

   julian_time = time (NULL);

   julian_date = (unsigned short) ((julian_time / 
                                   (time_t) SECONDS_IN_A_DAY) + ONE);

      /* compute the elapsed minutes since midnight */

   time_of_day_in_min = (unsigned short) ((julian_time % (time_t) SECONDS_IN_A_DAY) /
                                           SECONDS_IN_A_MINUTE);

   switch (msg_type) {

      case EDITED_CLUTTER_FILTER_MAP:
      case CLUTTER_FILTER_BYPASS_MAP:
         RDA_status_msg.bypass_map_date = julian_date;
         RDA_status_msg.bypass_map_time = time_of_day_in_min;

         ORDA_bypass_map_t *msg_ptr;

/*         msg_ptr = (ORDA_bypass_map_t *) (Message_pointer [CLUTTER_FILTER_BYPASS_MAP] +
                      sizeof (RDA_RPG_message_header_t)); */

         msg_ptr = (ORDA_bypass_map_t *) Message_pointer [CLUTTER_FILTER_BYPASS_MAP];

            
            /* Byte swap the data if this is a little endian machine */

         if (MISC_i_am_bigendian() == 0) {  /* 0 means little endian machine */
            msg_ptr->date = SHORT_BSWAP (julian_date);
            msg_ptr->time = SHORT_BSWAP (time_of_day_in_min);
         } else {
            msg_ptr->date = julian_date;
            msg_ptr->time = time_of_day_in_min;
         }
 
      break;

      case CLUTTER_SENSOR_ZONES:
      {
         ushort *msg_ptr;

            /* update the RDA status msg notchwidth map gen date and time */

         RDA_status_msg.clutter_map_date = julian_date;
         RDA_status_msg.clutter_map_time = time_of_day_in_min;

            /* update the msg header notchwidth gen date and time */

         msg_ptr = (ushort *) Message_pointer [NOTCHWIDTH_MAP_DATA];

            /* Byte swap the data if this is a little endian machine */

         if (MISC_i_am_bigendian() == 0) {  /* 0 means little endian machine */
            *msg_ptr = (ushort) SHORT_BSWAP (julian_date);
            ++msg_ptr;
            *msg_ptr = (short) SHORT_BSWAP (time_of_day_in_min);
         } else {
            *msg_ptr = (ushort) julian_date;
            ++msg_ptr;
            *msg_ptr = (short) time_of_day_in_min;
         }
      }
      break;
   }
   return;
}


/**************************************************************************

    Description: This function writes data to the rpg by copying the data 
                 to the comm link's read buffer, then calling PR_send_data.

         Inputs: data_buff - pointer to the data. 
                 message_type - type of data message (ICD message code).
                 len       - data length.

**************************************************************************/

static void Write_message (char *data_buf, int message_type, int len)
{
       /* reallocate the buffer */

    if (LINk.r_cnt + len > LINk.r_buf_size) 
        Reallocate_input_buffer ( len );

    memcpy (LINk.r_buf + LINk.r_cnt, data_buf, len);
    LINk.r_cnt += len;

    if (VErbose_mode == 4) {
       fprintf (stdout, "Write_msg:    1st %d halfwords of the Response LB data:\n",
                NUM_HFWDS_TO_DUMP);
       Rda_msg_dump (LINk.r_buf);
    }

    PR_send_data (LINk.r_buf, message_type, LINk.r_cnt);
    LINk.r_cnt = ZERO;

    return;
}


/********************************************************************************

     Description: this routine calls the smi functions to perform the byte swapping
                  for the RDA/RPG ICD message header

           Input: msg_hdr_ptr - pointer to the ICD message header

          Return: value >= 0 on success; value < 0 on error

 ********************************************************************************/

static int Byte_swap_msg_hdr (RDA_RPG_message_header_t *msg_hdr_ptr)
{
   int ret;

   ret = SMIA_bswap_output ("RDA_RPG_message_header_t", msg_hdr_ptr,
                            sizeof (RDA_RPG_message_header_t));
   return ret;
}


/********************************************************************************

     Description: this routine calls the smi functions to perform the byte swapping
                  for the RDA/RPG ICD messages

           Input: msg_ptr - pointer to the message to byte swap
                  msg_hdr - pointer to the ICD message header

          Return: 0 on success; return value < 0 on error

 ********************************************************************************/

static int Byte_swap_ICD_msg (void *msg_ptr, RDA_RPG_message_header_t *msg_hdr)
{
   int i, ret = 0;
   int msg_hdr_size = sizeof (RDA_RPG_message_header_t); 
   int msg_size = (sizeof (short) * msg_hdr->size) - msg_hdr_size;

   switch (msg_hdr->type) {

      case GENERIC_DIGITAL_RADAR_DATA:
      {
          Generic_basedata_header_t *rec = (Generic_basedata_header_t *) msg_ptr; 
          int no_of_datum = rec->no_of_datum;

         /*
           If this is a Little-Endian machine, perform byte swapping.
          */
         MISC_swap_longs( 1, (long *) &(rec->time) );
         MISC_swap_shorts( 2, &(rec->date) );
         MISC_swap_floats( 1, &(rec->azimuth) );
         MISC_swap_shorts( 1, &(rec->radial_length) );
         MISC_swap_floats( 1, &(rec->elevation) );
         MISC_swap_shorts( 1, &(rec->no_of_datum) );

         for( i = 0; i < no_of_datum; i++ ){

             Generic_any_t *data_block;
             char type[5];
             int offset;

             offset = rec->data[i];
             MISC_swap_longs( 1, (long *) &(rec->data[i]) );

             data_block = (Generic_any_t *) ((char *)msg_ptr + offset);

             /* Convert the name to a string so we can do string compares. */
             memset( type, 0, 5 );
             memcpy( type, data_block->name, 4 );

             if( type[0] == 'R' ){

                if( strcmp( type, "RRAD" ) == 0 ){

                   Generic_rad_t *rad = (Generic_rad_t *) data_block;
                   Generic_rad_dBZ0_t *dBZ0 = 
                          (Generic_rad_dBZ0_t *) ((char *) rad + sizeof(Generic_rad_t));

                   MISC_swap_shorts( 2, &(rad->len) );
                   MISC_swap_floats( 2, &(rad->horiz_noise) );
                   MISC_swap_shorts( 2, &(rad->nyquist_vel) );
                   MISC_swap_floats( 2, &(dBZ0->h_dBZ0) );

                }
                else if( strcmp( type, "RELV" ) == 0 ){

                   Generic_elev_t *elev = (Generic_elev_t *) data_block;

                   MISC_swap_shorts( 2, &(elev->len) );
                   MISC_swap_floats( 1, &(elev->calib_const) );

               }
               else if( strcmp( type, "RVOL" ) == 0 ){

                  Generic_vol_t *vol = (Generic_vol_t *) data_block;

                  MISC_swap_shorts( 1, &(vol->len) );
                  MISC_swap_floats( 2, &(vol->lat) );
                  MISC_swap_shorts( 2, &(vol->height) );
                  MISC_swap_floats( 5, &(vol->calib_const) );
                  MISC_swap_shorts( 2, &(vol->vcp_num) );

               }
               else
                  LE_send_msg( GL_INFO, "Undefined/Unkwown Block Type.\n" );

            }
            else if( type[0] == 'D' ){

               Generic_moment_t *moment = (Generic_moment_t *) data_block;
               int no_of_gates = moment->no_of_gates;

               MISC_swap_longs( 1, (long *) &(moment->info) );
               MISC_swap_shorts( 5, &(moment->no_of_gates) );
               MISC_swap_floats( 2, &(moment->scale) );

               if( moment->data_word_size == (8*sizeof(short)) )
                  MISC_swap_shorts( no_of_gates, &(moment->gate.u_s[0]) );

            }

         }

       }

       break;

      case RDA_STATUS_DATA:
         ret = SMIA_bswap_output ("RDA_status_ICD_msg_t", msg_ptr,
                                  sizeof (RDA_status_ICD_msg_t));
       break;

      case RDA_CONTROL_COMMANDS:
         ret = SMIA_bswap_output ("ORDA_control_commands_t", msg_ptr,
                                  sizeof (ORDA_control_commands_t));
       break;

      case PERFORMANCE_MAINTENANCE_DATA:
/*         ret = SMIA_bswap_output
                 ("ORDA_status_msg_t", msg_prt + msg_hdr_size,
                  sizeof (ORDA_status_msg_t) - msg_hdr_size);
*/
       break;

      case LOOPBACK_TEST_RDA_RPG: 
      case LOOPBACK_TEST_RPG_RDA: 
       break;

      case REQUEST_FOR_DATA:
          ret = SMIA_bswap_output ("RPG_request_data_struct_t", msg_ptr,
                                   sizeof ( RPG_request_data_struct_t));
       break;

         /* canned binary messages are stored in big endian format */

      case CLUTTER_FILTER_BYPASS_MAP:
      case NOTCHWIDTH_MAP_DATA:
      case RDA_ADAPTATION_DATA:
       break;

      case RDA_VCP_MSG:
      case RPG_RDA_VCP: {
         char *temp_buffer;

         temp_buffer = malloc (sizeof (VCP_ICD_msg_t));

         if (temp_buffer == NULL) {
            printf ("Error malloc'ing %d bytes buffer space\n", 
                    sizeof (VCP_ICD_msg_t));
            ret = -1;
            break;
         }

         if ((memcpy (temp_buffer, msg_ptr, msg_size)) == NULL) {
            printf ("Error memcpy'ing %d msg to buffer\n", msg_size);
            ret = -1;
         } else {
            ret = SMIA_bswap_output ("VCP_ICD_msg_t", temp_buffer, 
                                     sizeof (VCP_ICD_msg_t));
         
            if (ret > 0) {
               if ((memcpy (msg_ptr, temp_buffer, msg_size)) == NULL) {
                  printf ("Error memcpy'ing msg %d from buffer\n", msg_hdr->type);
                  ret = -1;
               }
            }
         }
         free (temp_buffer);
       }
       break;

      default:
         printf ("Byte_swap_ICD_msg: Error matching message type %d\n", 
                 msg_hdr->type);
         ret = -1;
       break;
   }

   return ret;
}


/********************************************************************************
 
     Description: Change the reflectivity calib corr value in the status message

           Input: new refl calib corr value

          Output: The refl calib corr value is changed in the status message

          Return:

 ********************************************************************************/

void RRM_set_refl_calib_corr ( int refl_calib_val )
{

   RDA_status_msg.ref_calib_corr = refl_calib_val;
   RD_set_send_status_msg_flag ();

   return;
}

