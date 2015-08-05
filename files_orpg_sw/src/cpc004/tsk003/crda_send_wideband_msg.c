/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 17:07:07 $
 * $Id: crda_send_wideband_msg.c,v 1.125 2014/05/13 17:07:07 steves Exp $
 * $Revision: 1.125 $
 * $State: Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <crda_control_rda.h>
#include <sys/time.h>
#include <lb.h>
#include <basedata.h>
#include <rpg_request_data.h>
#include <stdarg.h>
#include <missing_proto.h>
#include <orpgda.h>
#include <orpgsite.h>
#include <orpgrda.h>
#include <orpgred.h>

#define SCALED_DEG                  256.0/360.0 /* Conversion factor for Clutter Censor
                                                   Zone azimuth angles. */
#define MAX_CONSOLE_MESSAGE_LENGTH  404  /* Maximum console message size, in bytes. */
#define WAIT_INTERVAL               500  /* Response wait interval, in millisecs. */

/* Structure of RPG to RDA console message. */
typedef struct {

   short    size;        /*  Console Message Size.  The number of bytes in the 
                             message.  (Range 2 to 404) */
   char     message [MAX_CONSOLE_MESSAGE_LENGTH]; 
                         /*  Message.  2 characters/halfword including 
                             non-printing characters. */

} rpg_rda_console_message_t;

/* File scope global data */
static short *RDAtoRPG_loopback_data; 
                             /* Pointer to RDA to RPG loopback data. */
static int RDAtoRPG_loopback_data_size;
                             /* Size, in shorts, of the RDA to RPG loopback data. */
static int N_outstanding;    /* Number of outstanding comm manager requests, i.e.,
                                request which have not yet been responded to. */
static Request_list_t Outstanding_request[ MAXN_OUTSTANDING ]; 
                             /* List of outstanding comm manager requests. */

static int RDA_to_RPG_msg[ INPUT_DATA_SIZE/2 ]; /* RDA to RPG message buffer. */

static Request_info_t Request_info;

/* Local functions. */
static int Send_orda_clutter_censor_zones( int region_count, int file_number,
                                           short **message_data, int *message_size );
static int Send_orda_clutter_bypass_map(short **message_data,int *message_size);
static int Send_rda_console_message( Rda_cmd_t *rda_command, short **message_data,
                                     int *messge_size );
static int Process_request( int data_type, short *message_data, int message_size, 
                            malrm_id_t timer_id, time_t timeout_value );
static int Add_request_struct( int operation, int parameter, short seglen, 
                               char *rpg_to_rda_msg );
static int Insert_msgdata( short seglen, short *message_data, 
                           char *rpg_to_rda_msg, int from_message_index );
static int Request_for_data( int request_data_type, short **message_data,
                             int *message_size );
int Service_comms_manager_response( CM_resp_struct *response );
static void Print_comm_response_data( int link_ind, int type, int ret_code );
int Rda_return_to_previous_state( int state_phase );
int Orda_return_to_previous_state( int state_phase );


/*\///////////////////////////////////////////////////////////////
//
//   Description:   
//      Main processor for RDA Control Commands.  See the individual
//      command processors for more information.
//
//      If the command is valid and the wideband line is connected, 
//      the command is converted to ICD format, and sent to the RDA
//      via the call to module Process_request. 
//
//   Inputs:   
//      rda_command - RDA Control Command.
//      message_data - pointer to pointer where rda control command 
//                     is to be stored
//      message_size - pointer to where size of rda control command
//                     is to be stored
//
//   Outputs:
//      message_data - pointer to pointer where rda control command 
//                     is stored
//      message_size - pointer to where size of rda control command
//                     is stored
//
//   Returns:   
//      Negative number on failure; 0 otherwise.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////\*/
int SWM_process_rda_command( Rda_cmd_t *rda_command ){

   int ret = 0;
   int process_request, data_type, error_code;
   malrm_id_t timer_id;
   time_t timeout_value;

   int message_size = 0;
   short *message_data = NULL;
    
   /* Initialize flags and variables. */
   data_type = -1;
   process_request = 1;
   timeout_value = 0;
   timer_id = (malrm_id_t) -1;
   error_code = SWM_SUCCESS;

   /* Process the command based on type. */
   switch( rda_command->cmd ){
    
      /* Command connection of wideband line. */
      case COM4_WBENABLE:{

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "Commanded Connect Of Wideband Line\n" );
  
         if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                         "RMS Commanded Connect Of Wideband Line\n" );

         /* Post a connect on this line. */
         return ( LC_connect_wb_line() );
      }

      /* Command disconnection of wideband line. */
      case COM4_WBDISABLE:{


         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "Commanded Disconnect Of Wideband Line\n" );

         if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD ){

            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                         "RMS Commanded Disconnect Of Wideband Line\n" );

            /* Post a disconnect on this line by RMS. */
            return ( LC_disconnect_wb_line( RS_DISCONNECTED_RMS ) );

         }
         else{

            /* Post a disconnect on this line by HCI. */
            return ( LC_disconnect_wb_line( RS_DISCONNECTED_HCI ) );

         }

      }

      /* Command RPG->RDA loopback test. */
      case COM4_LBTEST:{

         if( PR_RPGtoRDA_loopback_message( &message_data, &message_size ) == PR_SUCCESS ){

            data_type = LOOPBACK_TEST_RPG_RDA;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Sending RPG/RDA Loopback Message\n" );

            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Sending RPG/RDA Loopback Message\n" );

            /* There is a timer associated with this message. */
            timeout_value = (time_t) LB_TEST_TIMEOUT_VALUE;
            timer_id = (malrm_id_t) MALRM_LB_TEST_TIMEOUT;

            /* Cancel previous timer, if set. */
            TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_LB_TEST_TIMEOUT );

         }
         else
            process_request = 0;

         break;
      }

      /* Command RDA control command. */
      case COM4_RDACOM:{

         ret = RC_orda_control( rda_command, &message_data, &message_size );

         if( ret == RC_SUCCESS ){

            data_type = RDA_CONTROL_COMMANDS;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Sending RDA Control Command; cmd: %d, who: %d, param1: %d, param2: %d, param3: %d\n",
                            rda_command->cmd,
                            rda_command->line_num,
                            rda_command->param1,
                            rda_command->param2,
                            rda_command->param3 );

            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_INFO, "RMS Sending RDA Control Command; cmd: %d, who: %d, param1: %d, param2: %d, param3: %d\n",
                            rda_command->cmd,
                            rda_command->line_num,
                            rda_command->param1,
                            rda_command->param2,
                            rda_command->param3 );

            if( rda_command->line_num == RED_INITIATED_RDA_CTRL_CMD ){

               unsigned char flag;

               if( rda_command->param1 == CRDA_SR_ENAB )
                  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                         ORPGINFO_STATEFL_SET,
                                         &flag );

               else if( rda_command->param1 == CRDA_SR_DISAB )
                  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                         ORPGINFO_STATEFL_CLR,
                                         &flag );

               else if( rda_command->param1 == CRDA_CMD_ENAB )
                  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                         ORPGINFO_STATEFL_SET,
                                         &flag );

               else if( rda_command->param1 == CRDA_CMD_DISAB )
                  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                         ORPGINFO_STATEFL_CLR,
                                         &flag );

               else if( rda_command->param1 == CRDA_AVSET_ENAB )
                  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                         ORPGINFO_STATEFL_SET,
                                         &flag );

               else if( rda_command->param1 == CRDA_AVSET_DISAB )
                  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                         ORPGINFO_STATEFL_CLR,
                                         &flag );

            }

         }
         else
            process_request = 0;
          
         break;

      }

      /* Command request for RDA data. */
      case COM4_REQRDADATA:{

         error_code = Request_for_data( rda_command->param1, &message_data,
                                        &message_size );
         if( error_code == SWM_SUCCESS ){

            data_type = REQUEST_FOR_DATA;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Sending Request for Data: %d\n",
                            rda_command->param1 );

            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "RMS Sending Request for Data: %d\n",
                            rda_command->param1 );

         }
         else
            process_request = 0;


         break;
      }

      /* Command RDA->RPG loopback test. */
      case COM4_LBTEST_RDAtoRPG:{

         message_data = RDAtoRPG_loopback_data;
         message_size = RDAtoRPG_loopback_data_size;

         if( message_data != NULL && message_size != 0 )
            data_type = LOOPBACK_TEST_RDA_RPG;

         else
            process_request = 0;

         break;
      }

      /* Command download of VCP. */
      case COM4_DLOADVCP:{

         if( (error_code = DV_process_vcp_download( rda_command, &message_data,
                                      &message_size )) == DV_SUCCESS ){

            data_type = RPG_RDA_VCP;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Downloading Volume Coverage Pattern %d\n",
                            rda_command->param1  );

         }
         else
            process_request = 0;

         break;
      }
   
      /* Command send clutter censor zones. */
      case COM4_SENDCLCZ:{

         ret = Send_orda_clutter_censor_zones( rda_command->param1,
                                               rda_command->param2, 
                                               &message_data, 
                                               &message_size );
         if( ret == SWM_SUCCESS ){

           data_type = CLUTTER_SENSOR_ZONES;

           if( CR_verbose_mode )
              LE_send_msg( GL_INFO, "Downloading Clutter Sensor Zone File %d\n",
                           rda_command->param2 );

           if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
              LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "RMS Downloading Clutter Sensor Zone File %d\n",
                           rda_command->param2 );
        }
        else
        {
           process_request = 0;
        }

        break;
      }

      /* Command send edited clutter bypass map. */
      case COM4_SENDEDCLBY:{

         ret = Send_orda_clutter_bypass_map( &message_data, &message_size );
          
         if( ret == SWM_SUCCESS ){

            data_type = EDITED_CLUTTER_FILTER_MAP;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Uploading Edited Clutter Bypass Map\n" );

            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Uploading Edited Clutter Bypass Map\n" );
         }
         else
            process_request = 0;

         break;
      }

      /* Command RDA console message. */
      case COM4_WBMSG:{

         if( Send_rda_console_message( rda_command, &message_data,
                                       &message_size ) == SWM_SUCCESS ){

            data_type = CONSOLE_MESSAGE_G2A;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Sending RDA Console Message\n" );

            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "RMS Sending RDA Console Message\n" );
         }
         else
            process_request = 0;

         break;
      }

      /* Spot blanking enable/disable. */
      case COM4_SB_ENAB:
      case COM4_SB_DIS:{

      
         Rda_cmd_t *rda_command;
         rda_command = calloc( 1, sizeof( Rda_cmd_t) );
         if( rda_command == NULL ){

            LE_send_msg( GL_INFO, "calloc Failed For %d bytes\n", sizeof(Rda_cmd_t) );
            process_request = 0;
            break;

         }

         /* Build the RDA control command. */
         if( rda_command->cmd == COM4_SB_ENAB )
            rda_command->param1 = CRDA_SB_ENAB;
         else
            rda_command->param1 = CRDA_SB_DIS;

         rda_command->cmd = COM4_RDACOM;
         ret = RC_orda_control( rda_command, &message_data, &message_size );
          
         if( ret == RC_SUCCESS ){

            data_type = RDA_CONTROL_COMMANDS;

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Sending RDA Control Command; cmd: %d, who: %d, param1: %d, param2: %d, param3: %d\n",
                            rda_command->cmd,
                            rda_command->line_num,
                            rda_command->param1,
                            rda_command->param2,
                            rda_command->param3 );

            free( rda_command );
         }
         else
            process_request = 0;

         break;
      }

      /* Unknown or unsupported command type. */
      default:
      {
         LE_send_msg( GL_INFO, "Unknown Or Unsupported RDA Command (%d)\n",
                      rda_command->cmd );
         error_code = SWM_FAILURE;
         process_request = 0;

         break;
      }

   /* End of "switch" */
   }

   /* If request needs to be processed and the wideband line is 
      connected, process the request. */
   if( (process_request) && (data_type >= 0) 
             && 
       (SCS_get_wb_status( ORPGRDA_WBLNSTAT ) == RS_CONNECTED) ){ 

      return( Process_request( data_type, message_data, message_size,
                               timer_id, timeout_value ) );

   }
   else{

      /* If wideband line is not connected, return error. */
      if( SCS_get_wb_status( ORPGRDA_WBLNSTAT ) != RS_CONNECTED ){

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "Request Not Processed Because Wideband Line Not CONNECTED.\n" );

         /* Return failure. */
         if( message_data != NULL )
            free( message_data );
         return( SWM_FAILURE );

      }

      /* All other errors. */
      else{

         if( message_data != NULL )
            free( message_data );

         /* Insure we return at least some error value ... */
         if( error_code == SWM_SUCCESS )
            error_code = SWM_FAILURE;          

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "Request Not Processed Because Error %d Encountered.\n",
                         error_code );

         return ( error_code );

      }

   }

/* End of SWM_process_rda_command() */
}

/*\//////////////////////////////////////////////////////////////
//
//   Description:
//      This module processes RPG to RDA messages.  Messages
//      which are to be sent to the RDA has a Comm Manager
//      request structure, a CTM header, and a message header
//      prefixed to the message data.  Each message is segmented, 
//      if required.
//
//      All messages are placed in a transmission queue.  Loopback
//      messages have highest priority so are placed at the front
//      of the queue.  All other messages are placed at the rear.
//      If the transmission queue is full, the request is aborted.
//      If the message is multi-segmented, none of the remaining
//      message are queued. 
//
//
//   Inputs:
//      data_type - type of message to process (see RDA/RPG ICD).
//      message_data - pointer to where rda message data is stored.
//      message_size - size (in shorts) of rda message.
//      timer_id - timer id associated with this message.
//      timeout_value - timeout value associated with the timer 
//                      id.
//
//   Outputs:
//      None.
//
//   Returns:
//      Returns SWM_FAILURE on error, SWM_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////\*/
static int Process_request( int data_type, short *message_data,
                            int message_size, malrm_id_t timer_id, 
                            time_t timeout_value ){

   short totsegs, lastseglen; 
   short seglen, segnum;
   int error, from_message_index = 0;

   Request_list_t *list_ptr;

   /* Initialize error to SWM_SUCCESS. */
   error = SWM_SUCCESS;

   /* Calculate total number of segments required for message. */
   totsegs = (message_size + MSGSIZMAX - 1)/MSGSIZMAX;

   /* Calculate the length of the last message segment. */
   lastseglen = message_size - ( (totsegs-1)*MSGSIZMAX );

   /* Initialize the starting index of data in message_data. */
   from_message_index = 0;

   /* Do for all message segments. */
   for( segnum = 1; segnum <= totsegs; segnum++ ){
 
      int queue_full, request_index; 
      char *mem_ptr, *rda_msg;

      /* Compute size of this message. */
      if( segnum == totsegs )
         seglen = lastseglen;

      else
         seglen = MSGSIZMAX;

      /* Allocate space for this RPG to RDA message.
         If memory allocation failed, set error flag and break out of loop. */
      if( (mem_ptr = (char *) calloc( (size_t) 1, 
                     ((size_t) OUTPUT_DATA_SIZE*sizeof(short)) )) == NULL ){

         LE_send_msg( GL_MEMORY, "RPG/RDA Message calloc Failed\n" );
         error = SWM_FAILURE;
         break;

      }
         
      /* Add the request structure.  If index returned is negative, set
         error flag and break out of loop. */
      if( (request_index = Add_request_struct( RQ_WRITE, (int) 1, seglen, 
                                               mem_ptr )) < 0 ){

         /* Outstanding request list must be full. */
         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "Outstanding Request Queue Is FULL\n" );

         error = SWM_FAILURE;
         break;

      }

      /* Adjust the RDA message pointer by the size of the CM request 
         structure and the size of the CTM header. */
      rda_msg = mem_ptr + sizeof(CM_req_struct) + 
                          (CTM_HEADER_LENGTH*sizeof(short));

      /* Insert the RDA message header. */
      SWM_rda_msg_header( rda_msg, data_type, seglen, segnum, totsegs );

      /* Insert the RDA message data. */
      Insert_msgdata( seglen, message_data, 
                      rda_msg+sizeof(RDA_RPG_message_header_t),
                      from_message_index );
      from_message_index += seglen;

#ifdef LITTLE_ENDIAN_MACHINE
      /* Convert the message header to external format */
      UMC_RDAtoRPG_message_header_convert( rda_msg );

      /* Convert the RDA message to external format (i.e., if this is a 
         Little-Endian machine and RDA is Big-Endian (or vice versa), need 
         to perform byte swapping on the message header and message data. */
      UMC_RPGtoRDA_message_convert_to_external( data_type, rda_msg );
#endif

      /* Set the response timeout value, in milliseconds. */
      Outstanding_request[ request_index ].wait_for_resp = WAIT_FOR_RESP; 
      Outstanding_request[ request_index ].wait_time = 1000; 

      /* Set timer id and timeout value. */
      Outstanding_request[ request_index ].timer_id = timer_id;
      Outstanding_request[ request_index ].timeout_value = timeout_value;

      /* Track the RPG -> RDA message id. */
      Outstanding_request[ request_index ].message_id = (int) data_type;

      /* Add this request to the request queue.  If the message is a 
         loopback message of any type, put at front of queue; otherwise
         place at rear. */
      list_ptr = &Outstanding_request[ request_index ];
      if( data_type == LOOPBACK_TEST_RPG_RDA 
                    ||
          data_type == LOOPBACK_TEST_RDA_RPG ){

         queue_full = QS_insert_front_request_queue( list_ptr );

      }
      else{

         queue_full = QS_insert_rear_request_queue( list_ptr );

      }

      /* If outstanding request queue is full, set error flag, abort this
         request, and break out of loop. */
      if( queue_full == QS_QUEUE_FULL ){
         
          error = SWM_FAILURE;
          LE_send_msg( GL_INFO, "Outstanding Request Queue Is Full\n" );
          Outstanding_request[ request_index ].sequence_number = -1; 

          break;

      }

   /* End of "for" loop */
   }

   /*
     Free the space used for the RPG to RDA message. 
   */
   if( message_data != NULL )
      free( message_data );

   return ( error );

/* End of Process_request() */
}

/*\///////////////////////////////////////////////////////////////
//
//   Description:   
//      Prefixes message buffer with Comm manager request structure,
//      then locates a Outstanding request slot to place message.  
//      If all slots are full, returns error. 
//
//   Inputs:   
//      operation - operation to be performed.
//      parameter - parameter associated with operation type.
//      seglen - length of message segment, in shorts.
//      rpg_to_rda_msg - pointer to rpg to rda message buffer.
//
//   Outputs:
//      data_size - length of total message, in bytes.
//
//   Returns:
//      Returns index to Outstanding_request, or SWM_FAILURE on error.
//
//   Globals:
//      CR_link_index - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////\*/
static int Add_request_struct( int operation, int parameter, 
                               short seglen, char *rpg_to_rda_msg ){
  
   static unsigned int request_sequence = 1;
   int index, data_size;
   CM_req_struct *request;

   /* Set up request structure. */
   request = (CM_req_struct *) rpg_to_rda_msg;
   request->type = operation;
   ++request_sequence; 
   if( request_sequence == (unsigned int) -1 )
      request_sequence = 1;
   request->req_num = request_sequence; 
   request->link_ind = CR_link_index;
   request->time = ( LB_id_t ) time( NULL ); 
   request->parm = parameter;
   if( seglen > 0 )
      request->data_size = (seglen + MSGHDRSZ + CTM_HEADER_LENGTH)
                           *sizeof(short);  /* In bytes. */

   else
      request->data_size = 0;

   /* Calculate the total length of this message, in bytes. */
   data_size = (request->data_size + sizeof( CM_req_struct ));

   /* Track this request. */
   for( index = 0; index < N_outstanding; index++ ){

      if( Outstanding_request[index].sequence_number == -1 )
         break;

   }

   /* Slot found for outstanding request. */
   if( index < MAXN_OUTSTANDING ){

      /* Fill Request List element with tracking data. */
      Outstanding_request[index].sequence_number = request_sequence;
      Outstanding_request[index].operation = operation;
      Outstanding_request[index].timer_id = (malrm_id_t) -1;
      Outstanding_request[index].timeout_value = 0;
      Outstanding_request[index].rpg_to_rda_msg = rpg_to_rda_msg;
      Outstanding_request[index].message_size = data_size;

      if( index >= N_outstanding )
         N_outstanding = index + 1;

   }
   else{

      unsigned int starting_sequence;
      int i, index_found = 0;

      /* Too many unanswered requests!!  Remove some old requests to make room. */
      LE_send_msg( GL_INFO, 
                   "Too Many Unanswered Wideband Comm Manger Requests Outstanding\n" );
      LE_send_msg( GL_INFO, "Previous Requests May Be Lost\n" );

      /* Remove all outstanding requests except the last 3 (arbitrary number).  
         I am assuming that sequence number rollover is never going to happen! */
      starting_sequence = request_sequence - 3;
      for( i = 0; i < N_outstanding; i++ ){

         if( Outstanding_request[i].sequence_number < starting_sequence ){

            Outstanding_request[i].sequence_number = -1;

            if( !index_found ){

               index = i;
               index_found = 1;

               /* Fill Request List element with tracking data. */
               Outstanding_request[index].sequence_number = request_sequence;
               Outstanding_request[index].operation = operation;
               Outstanding_request[index].timer_id = (malrm_id_t) -1;
               Outstanding_request[index].timeout_value = 0;
               Outstanding_request[index].rpg_to_rda_msg = rpg_to_rda_msg;
               Outstanding_request[index].message_size = data_size;

            }
 
         }

      /* End of "for" loop. */
      }

      if( !index_found )
         return ( SWM_FAILURE );

   }
         
   /* Return index to Outstanding Request. */
   return (index);

/* End of Add_request_struct() */
}

/*\///////////////////////////////////////////////////////////////
//
//   Description:
//     Inserts a message header in the RDA message.  Message
//     header format is described in the RDA/RPG ICD.
//
//   Inputs:   
//     rda_msg - location where message header data is to be stored. 
//     type - message type.
//     length - message length, in shorts.
//     segnum - message segment number.
//     totsegs - total segments for this message.
//
//   Outputs:
//     rda_msg - message header data is stored in message. 
//
//   Returns:
//     Currently undefined.  Always return SWM_SUCCESS.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////\*/
int SWM_rda_msg_header( char *rda_msg, int type, short length, 
                        short segnum, short totsegs ){

   short date;
   RDA_RPG_message_header_t *rda_message;
   unsigned int time_of_day;

   static unsigned short message_sequence = 0;


   /* Add the CTM header offset. */
   rda_message = (RDA_RPG_message_header_t *) rda_msg;
   rda_message->size = length + MSGHDRSZ;
 
   /* Store type of message. */
   rda_message->type = (unsigned char) type;

   /* Store the channel/configuration value. */
   rda_message->rda_channel = (unsigned char) RDA_RPG_MSG_HDR_ORDA_CFG;

   /* Is this an FAA redundant site? */
   if( CR_redundant_type != ORPGSITE_NO_REDUNDANCY )
      rda_message->rda_channel += (unsigned char) CR_channel_num; 

   /* Store sequence number of message. */
   rda_message->sequence_num = message_sequence++;

   /* Store date and time of message. */
   SWM_get_date_time( &date, &time_of_day );
   rda_message->julian_date = date;
   rda_message->milliseconds = time_of_day;

   /* Add number of message segments and message segment number. */
   rda_message->num_segs = totsegs;
   rda_message->seg_num = segnum;

   return ( SWM_SUCCESS );

/* End of SWM_rda_msg_header() */
}

/*\///////////////////////////////////////////////////////////////
//
//   Description:   
//      Transfer message data to RPG to RDA message buffer for 
//      transmission to RDA.
//
//   Inputs:   
//      seglen - length of message segment, in shorts.
//      message_data - pointer to where rda message data is stored.
//      rpg_to_rda_msg - pointer to rpg to rda message buffer.
//      from_message_index - index into message data where data 
//                           transfer to RDA message buffer begins.
//
//   Outputs:
//
//   Returns:    
//      Currently undefined.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////\*/
static int Insert_msgdata( short seglen, short *message_data,
                           char *rpg_to_rda_msg, int from_message_index ){

   /* Insert message data into message buffer. */
   memcpy( rpg_to_rda_msg, &message_data[ from_message_index ], 
           (size_t) (seglen*sizeof(short)) );

   return ( SWM_SUCCESS );

/* End of Insert_msgdata() */
}

/*\////////////////////////////////////////////////////////////
//
//   Description:
//      Build request operation and writes request to request 
//      Linear Buffer.  After write, wait for response if the
//      wait_for_response flag is set.  The length of time to wait 
//      is given by wait_time.
//
//   Inputs:   
//      operation - The type of request operation.
//      parameter - Parameter associated with operation type.
//      wait_for_response - Flag, if set, indicates that once
//                          the request is commpleted, the comm 
//                          manager response is to be waited for. 
//      wait_time - Wait time, in milliseconds.
//
//   Outputs:
//
//   Returns:    
//      SWM_FAILURE; SWM_SUCCESS otherwise.
//
//   Globals:
//      CR_request_LB - see crda_control_rda.h.
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////\*/
int SWM_request_operation( int operation, int parameter,
                           int wait_for_response, int wait_time ){

   int request_index, ret;
   char *msg_c = NULL;
   RDA_RPG_message_header_t *msg_hdr = NULL;
   CM_req_struct request;

   /* Set up request structure.  If request index negative, return error. */
   if( (request_index = Add_request_struct( operation, parameter, (short) 0, 
                                       (char *) &request )) < 0 )
      return (SWM_FAILURE);

   /* Set the response timeout value.  We do this for compatibility with
      other requests which get placed on the request queue. */
   Outstanding_request[ request_index ].wait_for_resp = wait_for_response; 
   Outstanding_request[ request_index ].wait_time = wait_time; 

   /* Set timer id and timeout value.  We do this for compatibility with
      other requests which get placed on the request queue. */
   Outstanding_request[ request_index ].timer_id = (malrm_id_t) -1;
   Outstanding_request[ request_index ].timeout_value = 0;

   /* Set the message id to -1 to indicate it is a comm manager control
      command (i.e., RQ_CONNECT, RQ_DISCONNECT, etc.) */
   Outstanding_request[ request_index ].message_id = operation + REQ_TO_MSGID_BIAS;

   /* Write this request to the request linear buffer.  We do not want to 
      place this request in the request queue since we need immediate 
      response to the request and this module must return the success of
      the operation. */
   if( (ret = ORPGDA_write( CR_request_LB, (char *) &request, 
                            (int) sizeof( CM_req_struct ), LB_ANY )) < 0 ){

      LE_send_msg( GL_LB(ret), "Request LB Write Failed (%d)\n", ret );
      return ( SWM_FAILURE );

   }

   /* Save information about this request. */
   msg_c = ((char *) &request) + sizeof(CM_req_struct);
   msg_hdr = (RDA_RPG_message_header_t *) msg_c;

   /* Is this a VCP message? */
   if( msg_hdr->type == RPG_RDA_VCP ){

      Vcp_struct *vcp = (Vcp_struct *) (msg_c + sizeof(RDA_RPG_message_header_t));
      Request_info.last_downloaded_vcp = SHORT_BSWAP( vcp->vcp_num );
      Request_info.last_download_vcp_time = time(NULL);

      LE_send_msg( GL_INFO, "Last Downloaded VCP %d @ %d\n",
                   Request_info.last_downloaded_vcp, Request_info.last_download_vcp_time );

   }

   /* Wait for response if wait_for_response flag is set. */
   if( wait_for_response )
      return (SWM_check_response_LB( wait_for_response, wait_time ));

   return ( SWM_SUCCESS );

/* End of SWM_request_operation() */
}

/*\////////////////////////////////////////////////////////////////
//
//   Description:   
//      Read the response Linear Buffer.  If read returns LB_TO_COME, 
//      return only if wait_for_response is false.  Otherwise, wait
//      up to wait_time for response.
// 
//      If ORPGDA_read successful, check to data in the response Linear 
//      Buffer.  If not data and a CM_EVENT, process comm manager event.  
//      Otherwise, assume it is s comm manager response and process it.
//
//   Inputs:   
//      wait_for_response - Flag indicating whether or not to 
//                          return if status for ORPGDA_read is LB_TO_COME.     
//      wait_time - Number of milliseconds to wait.  
//
//   Outputs:
//
//   Returns:    
//      SWM_FAILURE on failure; SWM_SUCCESS otherwise.
//
//   Globals:
//      CR_response_LB - see crda_control_rda.h.
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int SWM_check_response_LB( int wait_for_response, int wait_time ){

   int ret, elapsed_time = 0;

   /* Initialize elapsed time. */
   if( wait_for_response == WAIT_FOR_RESP )
      elapsed_time = 0;

   while(1){

      /* Read Comm manager response LB. */
      ret = ORPGDA_read( CR_response_LB, (char *) RDA_to_RPG_msg,
                         (int) sizeof( RDA_to_RPG_msg ), LB_NEXT );

      /* All negative return values other than LB_TO_COME and LB_EXPIRED
         are considered errors. */
      if( (ret < 0) && (ret != LB_TO_COME) && (ret != LB_EXPIRED) ){

         LE_send_msg( GL_ERROR, "Response LB Read Failed (%d)\n", ret );
         return ( SWM_FAILURE );

      }
      else if ( (ret == LB_TO_COME) || (ret == LB_EXPIRED) ){

         /* If message expired, go to the first unread message.
            Otherwise, we'll never catch up. */
         if( ret == LB_EXPIRED )
            ORPGDA_seek( CR_response_LB, 0, LB_FIRST, NULL );

         if( wait_for_response == DONT_WAIT_FOR_RESP )
            break;

         else {

            /* Go to sleep, then wake up and see if LB is available. */
            msleep( (int) 500 );

            /* A negative wait time means to wait until the response is
               received regardless of how long it may take.  Therefore there
               can not be a response time-out in this case. */
            if( wait_time > 0 ){

               elapsed_time += WAIT_INTERVAL;

               if( elapsed_time >= wait_time ){

                  /* Consider this a response time-out.  Response is not
                     comimg. */
                  return ( SWM_FAILURE );

               }

            }

         }

      }

      /* Positive return value indicates read successful, and stores the
         number of bytes read. */
      else if( ret >= (int) sizeof( CM_resp_struct ) ){

         CM_resp_struct *response;
         short msg_len, *ptr_to_short = (short *) RDA_to_RPG_msg;

         /* Check if this is old-style type message. */
#ifdef LITTLE_ENDIAN_MACHINE

         msg_len = *(ptr_to_short+1); 
#else
         msg_len = *(ptr_to_short); 
#endif
         response = (CM_resp_struct *) RDA_to_RPG_msg; 

         /* We assume this must be data. */
         if( msg_len != 0 || response->type == RQ_DATA ){

            /* This is data, so process the wideband message. */
            RW_receive_wideband_data( RDA_to_RPG_msg, msg_len );

         }

         /* If response type is CM_EVENT, process this special. */
         else if( response->type == CM_EVENT )
            SCS_handle_comm_manager_event( response->ret_code );

         /* We assume this must be a communications manager response. */
         else if( (response->type >= CM_CONNECT) 
                                  && 
                  (response->type <= CM_SET_PARAMS) )
            return(Service_comms_manager_response( response ));
  
         /* The msg_len == 0 and response->type is not valid.   This
            can happen playing back metadata in the Level II stream. */
         else
            continue;

      }

   }

   return ( SWM_SUCCESS );

/* End of SWM_check_response_LB() */
}

/*\///////////////////////////////////////////////////////////////////
//
//   Description:   
//      First checks for a matching request to the comm manager
//      response.  If match found, services the response to the 
//      request.
//
//   Inputs:   
//      response - Comm Manager response structure.
//
//   Outputs:
//
//   Returns:    
//      SWM_FAILURE on error, otherwise SWM_SUCCESS.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////\*/
int Service_comms_manager_response( CM_resp_struct *response ){

   int index;

   /* Check for matching request sequence number.  On match, break out 
      of loop. */
   for( index = 0; index < N_outstanding; index++ ){

      if( response->req_num == Outstanding_request[index].sequence_number )
         break;

   /* End of "for" loop */
   }

   /* Take appropriate action based on the response return code. */
   if( index < N_outstanding ){

      if( CR_verbose_mode ){

         LE_send_msg( GL_INFO, 
             "Comm Manager Response MATCHES Request (%d)\n", response->req_num ); 
         Print_comm_response_data( response->link_ind, response->type, response->ret_code );

      }

      switch( Outstanding_request[index].operation ){

         case RQ_CONNECT:
         {

            if( (response->ret_code == CM_FAILED)
                           ||
                (response->ret_code == CM_TRY_LATER) ){

               if( CR_verbose_mode )
                  LE_send_msg( GL_INFO, "Request Failed (%d)!  Re-request Connection?\n",
                               response->ret_code );

               /* Check if disconnection timer active.  If yes, do not reattempt connection. 
                  This attempts to fix the case where comm manager responses to requests are
                  out of sequence.  If the disconnection timer is active, we assume we are
                  in a state of trying to disconnect the wideband line. */
               if( TS_is_timer_active( MALRM_DISCONNECTION_RETRY ) == 0 ){

                  LE_send_msg( GL_INFO, "Attempting Reconnect After Connection Request Failed\n" );

                  if( TS_is_timer_active( MALRM_CONNECTION_RETRY ) == 0 )
                     LE_send_msg( GL_ERROR, "Attempting Reconnect But Connection Timer NOT ACTIVE\n" );

                  /* Re-attempt wideband connection. */
                  SWM_request_operation( RQ_CONNECT, /* type of request */
                                         (int) 0, /* parameter associated with type */
                                         (int) DONT_WAIT_FOR_RESP, (int) 0 );

               }
               else
                  LE_send_msg( GL_INFO, "Not Attempting Reconnect Because Disconnect Timer ACTIVE\n" );
            
            }
            
            else if( response->ret_code != CM_IN_PROCESSING )
               LC_process_wb_line_connection( response->ret_code );

            else
               LE_send_msg( GL_INFO, "Comm Manager Still Processing Connection Request\n" );

            break;
         }

         case RQ_DISCONNECT:
         {
            if( (response->ret_code == CM_FAILED)
                           ||
                (response->ret_code == CM_TRY_LATER) ){

               if( CR_verbose_mode )
                  LE_send_msg( GL_INFO, "Request Failed (%d)!  Re-request Disconnection?\n",
                               response->ret_code );

               /* Check if connection timer active.  If yes, do not reattempt disconnection. 
                  This attempts to fix the case where comm manager responses to requests are
                  out of sequence.  If the connection timer is active, we assume we are
                  in a state of trying to reconnect the wideband line. */
               if( TS_is_timer_active( MALRM_CONNECTION_RETRY ) == 0 ){

                  LE_send_msg( GL_INFO, "Attempting Disconnect After Disconnection Request Failed\n" );

                  if( TS_is_timer_active( MALRM_DISCONNECTION_RETRY ) == 0 )
                     LE_send_msg( GL_ERROR, "Attempting Disconnect But Disconnection Timer NOT ACTIVE\n" );

                  /* Re-attempt wideband disconnection. */
                  SWM_request_operation( RQ_DISCONNECT, /* type of request */
                                         (int) 0, /* parameter associated with type */
                                         (int) DONT_WAIT_FOR_RESP, (int) 0 );

               }
               else
                  LE_send_msg( GL_INFO, "Not Attempting Re-disconnect Because Connect Timer ACTIVE\n" );
            
            }

            else if( response->ret_code != CM_IN_PROCESSING ){

               LC_process_wb_line_disconnection( response->ret_code );

               /* For disconnections in response to a wideband failure,
                  try and reconnect the wideband line. */
               LC_reconnect_line_after_failure();

            }

            break;
         }

         case RQ_STATUS:
         {
            /* The return code from this call is either CM_CONNECTED or 
               CM_DISCONNECTED. */
            if( response->ret_code == CM_CONNECTED )
               SCS_update_wb_line_status( (int) RS_CONNECTED,
                                          (int) -1, /* display blanking no change */
                                          (int) 0 /* do not post event */ );
            else if( response->ret_code == CM_DISCONNECTED )
               SCS_update_wb_line_status( (int) RS_DISCONNECTED_CM,
                                          (int) -1, /* display blanking no change */
                                          (int) 0 /* do not post event */ );
            break;
         }

         case RQ_WRITE:
         {
            /* Report any errors which may have occurred for the write request. */
            if( response->ret_code == CM_FAILED 
                                   || 
                response->ret_code == CM_DISCONNECTED ){

               LE_send_msg( GL_INFO, "Comm Manager Write Failed (%d).  Msg ID - %d\n",
                  response->ret_code, Outstanding_request[index].message_id );

               /* Loopback test write failures are treated as unsolicited 
                  wideband disconnections.  This may need to be done for all 
                  write failures! */
               if( Outstanding_request[index].message_id == LOOPBACK_TEST_RPG_RDA )
                  LC_process_unexpected_line_disconnection( (int) NULL );
               
            }
            /* Write request was apparently successful. */
            else {

               /* If a timer is associated with this message, set it now. */
               if( Outstanding_request[index].timer_id != (malrm_id_t) -1 ){

                  int ret ;

                  ret = TS_set_timer( Outstanding_request[index].timer_id, 
                                      Outstanding_request[index].timeout_value,
                                      (unsigned int) MALRM_ONESHOT_NTVL );

                  if( ret != TS_SUCCESS )
                     LE_send_msg( GL_INFO, "Setting timer %d Failed\n",
                                  Outstanding_request[index].timer_id );

               }

            }

            break;

         }

         default:
            LE_send_msg( GL_ERROR, "Unrecognized Or Unsupported Operation: %d\n",
                         Outstanding_request[index].operation );

      /* End of "switch" statement. */
      }

      /* Recycle matching request of the operation just processed.  If request not
         matched, just recycle it. */
      Outstanding_request[index].sequence_number = -1;

      /* Regardless of operation, report special return codes that currently are not
         handled by control rda. */
      if( response->ret_code == CM_TOO_MANY_REQUESTS )
         LE_send_msg( GL_COMMS, "Comm Manager Reports Too Many Requests\n" );

      else if( response->ret_code == CM_INVALID_PARAMETER )
         LE_send_msg( GL_COMMS, "Comm Manager Reports Invalid Parameter\n" );


      /* Adjust the number of outstanding requests, if necessary. */
      if( index == (N_outstanding - 1) )
         N_outstanding--;

      /* Done with this response ..... */
      return( SWM_SUCCESS );

   }
   else{

      if( CR_verbose_mode ){

         LE_send_msg( GL_INFO, "!!!!  Comm Manager Response With NO MATCHING Request (%d). !!!!\n",
                      response->req_num );
         Print_comm_response_data( response->link_ind, response->type, response->ret_code );

      }
      return ( SWM_FAILURE ); 

   }

/* End of Service_comms_manager_response() */
}

/*\///////////////////////////////////////////////////////////////////
//
//   Description:   
//      Build Request For Data Message according to RDA/RPG ICD format.
//
//   Inputs:   
//      request_data_type - data type to be requested from RDA.
//      request_msg - pointer to pointer where request for data 
//                    message is to be stored.
//      message_size - pointer to where size of request for data 
//                     message (in shorts) is to be stored.
//
//   Outputs:
//      request_msg - pointer to pointer where request for data 
//                    message is stored.
//      message_size - pointer to where size of request for data 
//                     message (in shorts) is stored.
//
//   Returns:    
//      SWM_FAILURE on failure; otherwise SWM_SUCCESS.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////\*/
static int Request_for_data( int request_data_type, short **request_msg,
                             int *message_size ){

   /* Allocate storage for the RDA request for data message. */
   *request_msg = (short *) calloc( (size_t) 1, 
                                    (size_t) sizeof(RPG_request_data_t) );
   if( *request_msg == NULL ){

      *message_size = 0;

      LE_send_msg( GL_MEMORY, "RDA Request For Data Message calloc Failed\n" );
      return ( SWM_FAILURE );

   }

   /* Assign request to request buffer. */
   **request_msg = request_data_type;
      
   /* Assign message size, in shorts. */
   *message_size = sizeof(RPG_request_data_t)/sizeof(short); 

   return ( SWM_SUCCESS );

/* End of Request_for_data() */
}

/*\//////////////////////////////////////////////////////////////////////
//
//   Description:   
//      The current Modified Julian date and time in milliseconds past 
//      midnight is returned.
//
//   Inputs:   
//      date - pointer to where Julian is to be stored.
//      current_time - pointer to where current time in
//                     milliseconds since midnight is to be stored.
//
//   Outputs:
//      *date - contains the current Julian date.
//      *current_time - contains the current time in milliseconds past
//                      midnight.
//
//   Returns:
//      There is not return values defined for this function.
//
//   Globals:
//
//   Notes:
//
//////////////////////////////////////////////////////////////////////\*/
void SWM_get_date_time( short *date, unsigned int *current_time ){

   time_t clock;

   /* Get the time in number of seconds since 1/1/70. */
   clock = time( (time_t *) NULL);

   /* Derive the Julian date. */
   *date = clock/86400;

   /* Convert time to milliseconds since midnight. */
   clock = clock - (*date)*86400;
   clock *= 1000;

   /* Store time as current_time. */
   *current_time = clock;

   /* Adjust the Julian date so the day 1 is Jan 1, 1970. */
   *date += 1;

/* End of SWM_get_date_time() */
}

/*\////////////////////////////////////////////////////////////
//
//   Description:   
//      Copies RDA loopback test data to buffer for transmission 
//      back to RDA, then sends data back to RDA.
//
//   Inputs:   
//      rda_to_rpg_loopback - pointer to loopback test data 
//                            buffer.
//      message_size - size of the message buffer, in shorts.
//
//   Outputs:
//
//   Returns:
//     SWM_FAILURE on failure; SWM_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////\*/
int SWM_RDAtoRPG_loopback_message( short *rda_to_rpg_loopback, 
                                   int message_size ){

   char *loop_back_msg;

   /* Allocate storage for the RDA Loopback message. */
   if( (loop_back_msg = (char *) calloc( (size_t) 1, 
                        (size_t ) (sizeof(short)*message_size) )) == NULL ){

      LE_send_msg( GL_MEMORY, "RDA Loopback Message calloc Failed\n" );
      return ( SWM_FAILURE );

   }

   /* Copy Loopback message test data. */
   memcpy( loop_back_msg, rda_to_rpg_loopback, 
           (size_t) (message_size*sizeof(short)) );

   /* Assign pointer to start of loopback message, and message size, 
      in shorts. */
   RDAtoRPG_loopback_data = (short *) loop_back_msg;
   RDAtoRPG_loopback_data_size = message_size;

   /* Send Loopback Message back to RDA. */
   return ( SWM_send_rda_command( (int) 0, (int) COM4_LBTEST_RDAtoRPG,
                                  (int) RDA_PRIMARY ) );

/* End of SWM_RDAtoRPG_loopback_message() */
}

/*\/////////////////////////////////////////////////////////////
//
//   Description:   
//      Process weather mode change.  The following 
//      steps are performed:
//
//        1)  Validate default VCP for this weather mode.
//        2)  If valid, Build RDA Control Command to download 
//            default VCP.   Otherwise, return error.
//        3)  Build RDA Control Command to Select VCP 
//            for next restart.
//        4)  Build RDA Control Command to restart VCP.
//
//   Inputs:   
//      rda_command - pointer to rda command buffer.
//
//   Outputs:
//
//   Returns:   
//      SWM_FAILURE on failure; otherwise SWM_SUCCESS.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////\*/
int SWM_process_weather_mode_change( Rda_cmd_t *rda_command ){

   int vcp_num, ret;
   unsigned int vol_seq_num = 0;

   /* Get the default VCP associated with this weather mode. */
   vcp_num = DV_get_default_vcp_for_wxmode( rda_command->param1 );

   if( vcp_num != DV_FAILURE ){

      /* Download this VCP to the RDA. */
      ret = SWM_send_rda_command( (int) 1, (int) COM4_DLOADVCP,
                                  (int) RDA_PRIMARY, vcp_num );

   }
   else
      return ( SWM_FAILURE );

   /* Issue an rda control command to select this VCP on next vcp
      restart. */
   if( ret == SWM_SUCCESS )
      ret = SWM_send_rda_command( (int) 2, (int) COM4_RDACOM,
                                  (int) RDA_PRIMARY, (int) CRDA_SELECT_VCP,
                                  (int) RCOM_USE_REMOTE_PATTERN ); 

   else
      return ( ret );

   /* Parameter 2 contains the volume scan number when the command
      was issued.  If this number matches the current volume scan
      number, do not issue restart VCP command ... i.e., let the VCP 
      finish.  If not the other hand the volume scan number has changed,
      issue the command so the operator does not have to wait for the
      current VCP to finish.   This is to handle the case where the 
      the weather mode change determination occurred at the end of the
      volume scan and the command to change the weather mode was received
      the start of the next volume scan. */
   vol_seq_num = ORPGVST_get_volume_number();
   if( rda_command->param2 != ORPGMISC_vol_scan_num( vol_seq_num ) ){

      /* Issue an rda control command to restart the VCP. */
      if( ret == SWM_SUCCESS ) 
         return ( SWM_send_rda_command( (int) 1, (int) COM4_RDACOM,
                                        (int) RDA_PRIMARY, 
                                        (int) CRDA_RESTART_VCP ) );

   }

   /* Return error. */
   return ( SWM_FAILURE );
   
/* End of SWM_process_weather_mode_change() */
}

/*\/////////////////////////////////////////////////////////////
//
//   Description:   
//      Process vcp download command.  The following steps are 
//      performed:
//
//        1)  Build RDA Control Command to download VCP. 
//        2)  Build RDA Control Command to Select VCP for next 
//            restart.
//
//   Inputs:   
//      rda_command - pointer to rda command buffer.
//
//   Outputs:
//
//   Returns:   
//      SWM_FAILURE on failure; otherwise SWM_SUCCESS. 
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////\*/
int SWM_process_vcp_download( Rda_cmd_t *rda_command ){

   int ret;

   /* Download VCP to the RDA. */
   if( (ret = SWM_process_rda_command( rda_command )) >= SWM_SUCCESS ){

      int vcp, cmd_enabled;

      /* The RDA command may indicate VCP 0.   The VCP will then need
         to be determined from the VCP last downloaded. */
      vcp = rda_command->param1;

      if( vcp == 0 )
         vcp = Request_info.last_downloaded_vcp;

      if( vcp != 0 ){

         /* Check if this is an SZ-2 VCP and if CMD is enabled. If CMD is 
            disabled, enable it.  Finally, download the default clutter 
            regions file (bypass map in control). Note: Only want to command
            enabled on CMD and download clutter sensor zones on change to SZ2 
            VCP. */
         if( ORPGVCP_is_SZ2_vcp( vcp ) 
                        && 
             (vcp != CR_previous_ORDA_status.status_msg.vcp_num) ){

            /* CMD is only valid with ORDA. */
            cmd_enabled = CR_ORDA_status.status_msg.cmd;

            /* CMD is not enabled, command RDA to enable CMD. */
            if( !cmd_enabled ){

               unsigned char flag;

               LE_send_msg( GL_INFO, "CMD Not Enabled ... Command RDA to enable.\n" );

               /* Set the RPG state for CMD to enabled.  Note:  This flag needs to 
                  be set ... otherwise, control_rda will think the CMD status is
                  opposite of what the operator wants and will automatically try
                  to disable it. */
               ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                      ORPGINFO_STATEFL_SET,
                                      &flag );

               /* Issue RDA Control Command to Enable CMD. */
               SWM_send_rda_command( (int) 2, (int) COM4_RDACOM,
                                     (int) RDA_PRIMARY, (int) CRDA_CMD_ENAB,
                                     (int) 0 ); 

            }   

            /* Download clutter file 0.  With ORDA, the default is assumed 5 zones. */
            SWM_send_rda_command( (int) 2, (int) COM4_SENDCLCZ, 
                                  (int) RDA_PRIMARY, (int) 5,
                                  (int) 0 ); 
            
            /* This is needed to support FAA Redundant.   On the inactive channel,
               control RDA only knows what VCP was last executed, not which VCP
               or clutter censor zone was last downloaded. */
            if( CR_redundant_type == ORPGSITE_FAA_REDUNDANT ){

               Redundant_cmd_t red_cmd;

               red_cmd.cmd = ORPGRED_DOWNLOAD_CLUTTER_ZONES;
               red_cmd.lb_id = 0;
               red_cmd.msg_id = 0;
               red_cmd.parameter1 = 5; /* Assumes 5 regions. */;
               red_cmd.parameter2 = 0; /* File 0 is always to default. */

               ORPGRED_send_msg( red_cmd );

            }

         }

      }
 
      /* Issue an rda control command to select this VCP on next vcp
         restart. */
      ret = SWM_send_rda_command( (int) 2, (int) COM4_RDACOM,
                                  (int) RDA_PRIMARY, (int) CRDA_SELECT_VCP,
                                  (int) RCOM_USE_REMOTE_PATTERN );

      return (ret);

   }
   else

      /* Return an error. */
      return ( SWM_FAILURE );

} /* End of SWM_process_vcp_download() */

/*\/////////////////////////////////////////////////////////////
//
//   Description:   
//      Process vcp change.  The following steps are performed:
//
//        1)  Build RDA Control Command to change the VCP.
//
//   Inputs:   
//      rda_command - pointer to rda command buffer.
//
//   Outputs:
//
//   Returns:   
//      SWM_FAILURE on failure; otherwise SWM_SUCCESS.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////\*/
int SWM_process_vcp_change( Rda_cmd_t *rda_command ){

   int vcp_num, ret;

   /* Get the VCP from the command. */
   vcp_num = rda_command->param2;

   /* Issue an rda control command to select this VCP on next vcp
      restart. */
   if( (ret = SWM_send_rda_command( (int) 2, (int) COM4_RDACOM,
                               (int) RDA_PRIMARY, (int) CRDA_SELECT_VCP,
                               (int) vcp_num )) == SWM_FAILURE )
      return ( SWM_FAILURE );

   /* Return error. */
   return ( SWM_SUCCESS );

/* End of SWM_process_vcp_change() */
}


/*\///////////////////////////////////////////////////////////
//
//  Description:   
//     Extracts the censor zone data from data store. 
//     Extract file based on file number.
//     Converts to RDA/RPG ICD format based on region count.  
//     Places data in RDA buffer.
//
//  Inputs:
//     region_count - number of censor zones in the censor zone 
//                    file
//     file_number - file number for the censor zone file (same 
//                   as index into censor zone array clcz_zone)
//     message_data - pointer to pointer where censor zone data 
//                    is to be stored
//     message_size - pointer to where size of censor zone data 
//                    (in shorts) is to be stored
//
//  Outputs:
//     message_data - pointer to pointer where censor zone data 
//                    is stored
//     message_size - pointer to where size of censor zone data 
//                    (in shorts) is stored
//
//  Returns:
//    SWM_FAILURE on error, SWM_SUCCESS on success.
//
//  Globals:
//
//  Notes:
//    This is the ORDA version of the "Send_rda_clutter_censor_zones"
//    function.  The primary difference is the lack of scaling in
//    this function.
//
///////////////////////////////////////////////////////////\*/ 
static int Send_orda_clutter_censor_zones( int region_count, int file_number,
                                      short **message_data, int *message_size ){

   int ret, i, k;
   int chan_indx 				        = 0;
   ORPG_clutter_regions_msg_t	*clutter_files		= NULL;
   ORPG_clutter_regions_t	*clutter_regions	= NULL;

   static Clutter_region_download_info_t download_info;

   /* Read in the clutter censor zone file from data store.  Return
      if read fails. */
   if( (ret = ORPGCCZ_get_censor_zones( ORPGCCZ_ORDA_ZONES, (char **) &clutter_files,
                                        ORPGCCZ_DEFAULT )) <= 0 ){

      LE_send_msg( GL_ERROR, "Clutter Censor Zone Read Failed (%d)\n", ret );
      return ( SWM_FAILURE );

   }

   /* Allocate a buffer for clutter censor zones. */
   if( (clutter_regions = (ORPG_clutter_regions_t *) calloc( (size_t) 1,
                             (size_t) sizeof(ORPG_clutter_regions_t) )) == NULL ){

      LE_send_msg( GL_MEMORY, "Clutter Censor Zones Regions calloc Failed\n" );
      return ( SWM_FAILURE );

   }

   /* Copy file data to region data. */
   memcpy( clutter_regions, &clutter_files->file[ file_number ].regions,
           (size_t) sizeof( ORPG_clutter_regions_t ) );

   /* First make sure the number of regions is correct. */
   if( (clutter_regions->regions < 0)
                    ||
       (clutter_regions->regions > MAX_NUMBER_CLUTTER_ZONES)
                    ||
       (region_count != clutter_regions->regions) ){

      LE_send_msg( GL_STATUS, "Bad Clutter Censor Zone File.  Invalid # Regions.\n" );
      LE_send_msg( GL_STATUS, "Clutter Censor Zone File Not Downloaded\n" );

      free( clutter_files );
      return( SWM_FAILURE );
   }

   /* Next make sure all the region attributes are within ICD limits. */
   i = 0;
   while ( i < clutter_regions->regions ){

      int start_az, stop_az, start_rng, stop_rng, segment, osc;

      start_az = clutter_regions->data[i].start_azimuth;
      stop_az = clutter_regions->data[i].stop_azimuth;
      start_rng = clutter_regions->data[i].start_range;
      stop_rng = clutter_regions->data[i].stop_range;
      segment = clutter_regions->data[i].segment;
      osc = clutter_regions->data[i].select_code;

      if( (start_az < 0) || (start_az > 360)
                         ||
          (stop_az < 0) || (stop_az > 360)
                         ||
          (start_rng < 0) || (start_rng > 511)
                         ||
          (stop_rng < 0) || (stop_rng > 511)
                         ||
          (segment < 1) || (segment > 5)
                         ||
          (osc < 0) || (osc > 2) ){

         /* Bad region encountered.   Remove it. */
         k = i;
         LE_send_msg( GL_INFO, "Bad Clutter Censor Zone Found ... Removing Region %d\n", i );
         while( k < clutter_regions->regions-1 ){

            clutter_regions->data[k].start_azimuth = clutter_regions->data[k+1].start_azimuth;
            clutter_regions->data[k].stop_azimuth = clutter_regions->data[k+1].stop_azimuth;
            clutter_regions->data[k].start_range = clutter_regions->data[k+1].start_range;
            clutter_regions->data[k].stop_range = clutter_regions->data[k+1].stop_range;
            clutter_regions->data[k].segment = clutter_regions->data[k+1].segment;
            clutter_regions->data[k].select_code = clutter_regions->data[k+1].select_code;

            k++;

          }

          clutter_regions->regions--;
      }
      else
         i++;

   }

   /* Free file data. */
   free( clutter_files );

   /* Check to make sure there are valid regions defined. */
   if( clutter_regions->regions <= 0 ){

      LE_send_msg( GL_STATUS, "Bad Clutter Censor Zone File.  Invalid # Regions.\n" );
      LE_send_msg( GL_STATUS, "Clutter Censor Zone File Not Downloaded\n" );
      return( SWM_FAILURE );

   }

   /* Assign RDA message pointer and size of RDA message. */
   *message_data = (short *) clutter_regions;
   *message_size = region_count *
                   (sizeof( ORPG_clutter_region_data_t )/sizeof(short)) +
                   sizeof( short );

   /* Set download information. */
   chan_indx = CR_channel_num - 1;
   memset( &download_info.channel[chan_indx].last_download_time, 0, 
           sizeof(Clutter_region_download_data_t) );
   download_info.channel[chan_indx].last_download_time = time(NULL);
   download_info.channel[chan_indx].last_download_file = file_number;
   download_info.channel[chan_indx].regions = clutter_regions->regions;
   memcpy( &download_info.channel[chan_indx].data, clutter_regions->data, 
           sizeof(ORPG_clutter_region_data_t)*download_info.channel[chan_indx].regions );

   LE_send_msg( GL_INFO, "..... Storing CCZ Download Info .....\n" );
   LE_send_msg( GL_INFO, "--->Channel Number:      %d\n", chan_indx+1 );
   LE_send_msg( GL_INFO, "--->Last Download Time:  %u\n", 
                download_info.channel[chan_indx].last_download_time );
   LE_send_msg( GL_INFO, "--->Last Download File:  %d\n", 
                download_info.channel[chan_indx].last_download_file );
   LE_send_msg( GL_INFO, "--->Number of Zones:     %d\n", 
                download_info.channel[chan_indx].regions );
   for( i = 0; i < download_info.channel[chan_indx].regions; i++ ){

       LE_send_msg( GL_INFO,"------>Start Range:   %d\n", 
                    download_info.channel[chan_indx].data[i].start_range ); 
       LE_send_msg( GL_INFO,"------>Stop Range:    %d\n", 
                    download_info.channel[chan_indx].data[i].stop_range ); 
       LE_send_msg( GL_INFO,"------>Start Azimuth  %d\n", 
                    download_info.channel[chan_indx].data[i].start_azimuth ); 
       LE_send_msg( GL_INFO,"------>Stop Azimuth:  %d\n", 
                    download_info.channel[chan_indx].data[i].stop_azimuth ); 
       LE_send_msg( GL_INFO,"------>Segment:       %d\n", 
                    download_info.channel[chan_indx].data[i].segment ); 
       LE_send_msg( GL_INFO,"------>Select Code:   %d\n", 
                    download_info.channel[chan_indx].data[i].select_code ); 

   }

   /* Update the DEAU data.  (Note: Set the Default, instead of Baseline) */
   if( (ret  = DEAU_set_binary_value( "ccz.download_info", &download_info, 
                                      sizeof(Clutter_region_download_info_t), 0 )) < 0 )
      LE_send_msg( GL_ERROR, "DEAU_set_binary_value( ccz.download_info) Failed: %d\n",
                   ret ); 

   return ( SWM_SUCCESS );

} /* End of Send_orda_clutter_censor_zones() */


/*\///////////////////////////////////////////////////////////
//
//  Description:   
//    Extracts the bypass map data from dedicated linear 
//    buffer.  Places data in RDA buffer.
//
//  Inputs:   
//     message_data - pointer to pointer where bypass map data 
//                    is to be stored
//     message_size - pointer to where size of bypass map data (in shorts)
//                    is to be stored
//
//  Outputs:
//     message_data - pointer to pointer where bypass map data 
//                    is stored
//     message_size - pointer to where size of bypass map data (in shorts)
//                    is stored.
//
//  Returns:
//     SWM_FAILURE  on error, SWM_SUCCESS on success.
//
//  Globals:
//
//  Notes:         
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
///////////////////////////////////////////////////////////\*/ 
static int Send_orda_clutter_bypass_map(short **message_data, int *message_size)
{
   int ret;
   ORDA_bypass_map_t *bypass_map = NULL;
   ORDA_bypass_map_msg_t *bypass_map_msg = NULL;

   /* Allocate space for bypass map data.  Return if unable to
      allocate memory. */
   if( (bypass_map = (ORDA_bypass_map_t *) calloc( (size_t) 1, 
      (size_t) sizeof(ORDA_bypass_map_t) )) == NULL )
   {
      LE_send_msg( GL_MEMORY, "Bypass Map calloc Failed\n" );
      return ( SWM_FAILURE );
   }

   /* Read in the bypass map from Linear Buffer. Return if read fails. */
   if( (ret = ORPGDA_read( ORPGDAT_CLUTTERMAP, (char *) &bypass_map_msg,
      LB_ALLOC_BUF, LBID_EDBYPASSMAP_ORDA )) <= 0 )
   {
      LE_send_msg( GL_ORPGDA(ret), "Bypass Map read Failed (%d)\n", ret );
      if( bypass_map != NULL )
      {
         free( bypass_map );
      }

      return ( SWM_FAILURE );
   }

   /* Move the bypass map data to bypass map buffer.  Then free bypass 
      map message. */
   memcpy( (void *) bypass_map, (void *) &bypass_map_msg->bypass_map, 
           sizeof(ORDA_bypass_map_t) ); 
   if( bypass_map_msg != NULL )
   {
      free( bypass_map_msg );
   }

   /* Assign RDA message pointer and size of RDA message. */
   *message_data = (short *) bypass_map;
   *message_size = (int) sizeof( ORDA_bypass_map_t )/sizeof(short);

   return ( SWM_SUCCESS );

} /* End of Send_orda_clutter_bypass_map() */

/*\///////////////////////////////////////////////////////////
//
//  Description:   
//    Places RDA console message data in RDA buffer.
//
//  Inputs:   
//     rda_command - pointer to RDA command buffer.
//     message_data - pointer to pointer where rda console message 
//                    data is to be stored
//     message_size - pointer to where size of rda console message data 
//                    (in short) is to be stored.
//
//  Outputs:
//     message_data - pointer to pointer where rda console message 
//                    data is stored
//     message_size - pointer to where size of rda console message data 
//                    (in short) is stored.
//
//  Returns:
//     SWM_FAILURE on error, SWM_SUCCESS on success.
//
//  Globals:
//
//  Notes:         
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
///////////////////////////////////////////////////////////\*/ 
static int Send_rda_console_message( Rda_cmd_t *rda_command,
                                     short **message_data,
                                     int *message_size ){

   rpg_rda_console_message_t *console_msg;

   /* Allocate space for console message data.  The number of bytes must
      account for the text length field in the message.  Return if 
      unable to allocate memory. */
   if( (console_msg = (rpg_rda_console_message_t *) calloc( (size_t) 1, 
                      (size_t) sizeof(rpg_rda_console_message_t) )) == NULL ){

      LE_send_msg( GL_MEMORY, "RDA Console Message calloc Failed\n" );
      return ( SWM_FAILURE );

   }

   /* Put the number of bytes in the console message. */
   if( rda_command->param2 > MAX_CONSOLE_MESSAGE_LENGTH ){

      LE_send_msg( GL_ERROR, "RDA Console Message Length (%d) > Max Allowed (%d)\n",
                   rda_command->param2, MAX_CONSOLE_MESSAGE_LENGTH );
      rda_command->param2 = MAX_CONSOLE_MESSAGE_LENGTH;

   }

   console_msg->size = (short) rda_command->param2;

   /* Copy RDA console message text into message buffer. */
   memcpy( console_msg->message, rda_command->msg, 
           (size_t) rda_command->param2 );
   
   /* Assign RDA message pointer and size of RDA message. */
   *message_data = (short *) console_msg;

   /* If the message size is an odd number of characters, add 1 to make it
      an integral multiple of shorts. */
   if( (console_msg->size % sizeof(short)) == 0 )
      *message_size = console_msg->size/sizeof(short) + 1;  /* In shorts.  
                                          Includes halfword for text size. */

   else
      *message_size = (console_msg->size+1)/sizeof(short) + 1;  /* In shorts.  
                                          Includes halfword for text size. */
      
   return ( SWM_SUCCESS );

/* End of Send_rda_console_message() */
}

/*\///////////////////////////////////////////////////////////
//
//  Description:   
//    Constructs an RDA command buffer, and sends command
//    to RDA.
//
//  Inputs:   
//     num_args - number of arguments or parameters in RDA
//                control command.
//     command - the command type.
//     wideband_line - wideband line to ship the command.
//     ... - variable arguments (command dependent).
//
//  Outputs:
//
//  Returns:
//    SWM_FAILURE on error, SWM_SUCCESS on success.
//
//  Globals:
//
//  Notes:
//     This module is used only for rda control internally
//     generated rda commands.  It is assumed that the 
//     rda control function will not generate a console
//     message.
//
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
///////////////////////////////////////////////////////////\*/ 
int SWM_send_rda_command( int num_args, int command, 
                          int wideband_line, ... ){

   va_list arg_ptr;
   static Rda_cmd_t rda_command;
   int *next_arg, i;

   /* Validate that the number of arguments is no more than 4. */
   if( num_args > 3 ){

      LE_send_msg( GL_ERROR, "Invalid Number Arguments For Send RDA Command\n" );
      return ( SWM_FAILURE );

   }

   /* Enter the command and wideband line number into the
      command buffer.  These are mandatory fields.  Initialize
      the parameter fields to 0. */
   rda_command.cmd = command;
   rda_command.line_num = wideband_line;
   rda_command.param1 = 0;
   rda_command.param2 = 0;
   rda_command.param3 = 0;
   rda_command.msg[0] = '\0';

   /* If there are additional arguments, ... */
   if( num_args > 0 ){

      /* Start a variable argument list. */ 
      va_start( arg_ptr, wideband_line );

      /* Return error if unable to start variable argument list. */
      if( arg_ptr == (va_list) NULL ){

         LE_send_msg( GL_ERROR, "Variable Arg List Bad For Send RDA Command\n" );
         return ( SWM_FAILURE );

      }

      /* Cast rda command structure element address to int pointer. */
      next_arg = &rda_command.param1;

      /* Enter the optional fields in the command buffer. */
      for( i = 0; i < num_args; i++ ){

         next_arg[i] = va_arg( arg_ptr, int );
        
      /* End of "for" loop */
      } 

      /* End the variable argument list. */
      va_end( arg_ptr );

   }

   /* Process RDA command. */
   return ( SWM_process_rda_command( &rda_command ) );

} /* End of SWM_send_rda_command() */


/*\////////////////////////////////////////////////////////////
//
//  Description:   
//    Determines the current RDA configuration (legacy or Open
//    RDA), then calls the appropriate routine to return the 
//    RDA to it previous state (prior to wideband failure or
//    disconnect) after wideband connection.
//
//  Inputs:   
//     phase - Phase of returning to previous RDA state.
//
//  Outputs:
//
//  Returns:
//     SWM_FAILURE on error, or return value from Process_request 
//     call.
//
//  Globals:
//
//  Notes:         
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
///////////////////////////////////////////////////////////\*/ 
int SWM_return_to_previous_state( int phase ){

   int ret = 0;

   ret = ORPGRDA_read_previous_state();
   if ( ret == ORPGRDA_ERROR )
      LE_send_msg( GL_ERROR,
         "SWM_return_to_previous_state: could not read previous state.\n");

   ret = Orda_return_to_previous_state(phase);    
   return ret;

} /* End of SWM_return_to_previous_state() */


/*\////////////////////////////////////////////////////////////
//
//  Description:   
//    Returns the RDA to it previous state (prior to wideband
//    failure or disconnect) after wideband connection.  This
//    is accomplished by reading the previous state, then
//    constructing the appropriate RDA control command.
//
//  Inputs:   
//     state_phase - Phase of returning to previous RDA state.
//
//  Outputs:
//
//  Returns:
//     SWM_FAILURE on error, or return value from Process_request 
//     call.
//
//  Globals:
//
//  Notes:         
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
///////////////////////////////////////////////////////////\*/ 
int Orda_return_to_previous_state( int state_phase )
{
   int				ret			= 0;
   int				message_size		= 0;
   int				orda_chan_stat		= 0;
   int				spot_blank_stat		= 0;
   int				p_chan_control		= 0;
   int				p_rda_stat		= 0;
   int				p_spot_blank_stat	= 0;
   int				p_vcp			= 0;
   ORDA_control_commands_t*	rda_control_message	= NULL;

   Vcp_struct                   p_vcp_data              = { 0 };
   int                          p_vcp_data_size         = 0;


   /* Retrieve and store RDA status values */
   orda_chan_stat = ST_get_status( ORPGRDA_CHAN_CONTROL_STATUS );
   spot_blank_stat = ST_get_status( ORPGRDA_SPOT_BLANKING_STATUS );

   /* Retrieve and store previous RDA status values */
   p_chan_control =
      ORPGRDA_get_previous_state( ORPGRDA_CHAN_CONTROL_STATUS );
   p_rda_stat =
      ORPGRDA_get_previous_state( ORPGRDA_RDA_STATUS );
   p_vcp =
      ORPGRDA_get_previous_state( ORPGRDA_VCP_NUMBER );
   p_spot_blank_stat =
      ORPGRDA_get_previous_state( ORPGRDA_SPOT_BLANKING_STATUS );

   /* Which phase are we in? */
   if( state_phase == PREVIOUS_STATE_PHASE_1 )
   {
      /* Is the RPG restarting (vice starting up)? */
      if( CR_rpg_restarting == MRPG_RESTART )
      {
         int error_occurred = 0, ret = 0;

         /* Is this an FAA redundant site? */
         if( CR_redundant_type == ORPGSITE_FAA_REDUNDANT )
         {
            LE_send_msg( GL_INFO, "Redundant Type = FAA REDUNDANT\n" );

            /* Get the channel number. */
            if( p_chan_control == RDA_IS_CONTROLLING )
            {
               if( orda_chan_stat == RDA_IS_CONTROLLING )
                  LE_send_msg( GL_INFO, 
                           "Channel %d Was Previously CONTROLLING, Is Currently CONTROLLING\n",
                           CR_channel_num );
               else
                  LE_send_msg( GL_INFO, 
                           "Channel %d Was Previously CONTROLLING, Is Currently NON CONTROLLING\n",
                           CR_channel_num );
            }
            else
            {
               if( orda_chan_stat == RDA_IS_CONTROLLING )
                  LE_send_msg( GL_INFO, 
                           "Channel %d Was Previously NOT CONTROLLING, Is Currently CONTROLING\n",
                           CR_channel_num );
               else
                  LE_send_msg( GL_INFO, 
                           "Channel %d Was Previously NOT CONTROLLING, Is Currently NON CONTROLING\n",
                           CR_channel_num );
            }

            /* For FAA redundant sites, return to previous channel 
               control status unless channel control status is 
               non-controlling and channel number is 1. */ 
            if( !( (p_chan_control == RDA_IS_NON_CONTROLLING)
                                   &&
                          (CR_channel_num == 1)) )
            {
               int command_sent = 0;
               int timer_active = TS_is_timer_active( MALRM_CHAN_CTRL );

               LE_send_msg( GL_INFO, "Return To Previous Channel Control\n" );
               if( (p_chan_control == RDA_IS_CONTROLLING)
                                   &&
                   (orda_chan_stat != RDA_IS_CONTROLLING) )
               {
                  /* Has command been sent previously? */
                  if( !timer_active )
                  {
                     ret = SWM_send_rda_command( (int) 1, (int)  COM4_RDACOM,
                                                 (int) RDA_PRIMARY,
                                                 (int) CRDA_CHAN_CTL );
                     LE_send_msg( GL_INFO, "Command RDA To Controlling\n" );
                     command_sent = 1;
                  }
               }
               else if( (p_chan_control == RDA_IS_NON_CONTROLLING)
                                        &&
                        (orda_chan_stat != RDA_IS_NON_CONTROLLING) )
               {

                  /* Has command been sent previously? */
                  if( !timer_active )
                  {
                     ret = SWM_send_rda_command( (int) 1, (int)  COM4_RDACOM,
                                                 (int) RDA_PRIMARY,
                                                 (int) CRDA_CHAN_NONCTL );
                     LE_send_msg( GL_INFO, "Command RDA To Non-Controlling\n" );
                     command_sent = 1;
                  }
               }

               /* On success, set 15 second timer for phase 2. */
               if( command_sent && (ret >= SWM_SUCCESS) )
                  ret = TS_set_timer( (malrm_id_t) MALRM_CHAN_CTRL,
                                      (time_t) CHAN_CTRL_TIMEOUT_VALUE,
                                      (unsigned int) MALRM_ONESHOT_NTVL );
               else if( !timer_active )
                  CR_return_to_previous_state = PREVIOUS_STATE_PHASE_2;

               /* Check for error. */
               if( ret < 0 )
               {
                  error_occurred = 1;
               }
            }
            else
            {
               /* We do not need to return to previous channel control
                  state so immediately go to phase 2. */
               CR_return_to_previous_state = PREVIOUS_STATE_PHASE_2;
            }
         }

         if( (CR_redundant_type == ORPGSITE_NO_REDUNDANCY) ||
             (CR_redundant_type == ORPGSITE_NWS_REDUNDANT) || 
             error_occurred )
         {
            if( error_occurred )
            {
               LE_send_msg( GL_INFO, 
                  "Previous Channel Control State Failed\n" );
               LE_send_msg( GL_INFO, "Redundant Type = %d\n", CR_redundant_type );
               LE_send_msg( GL_INFO, "Channel Number = %d\n", CR_channel_num );
            }

            /* Do previous state phase 2 immediately. */
            CR_return_to_previous_state = PREVIOUS_STATE_PHASE_2;
         }
      }
      else
      {
         /* RPG is not restarting, do previous state phase 2 immediately. */
         CR_return_to_previous_state = PREVIOUS_STATE_PHASE_2;
      }
   }

   /* If not in phase 2, return. */
   if( state_phase != PREVIOUS_STATE_PHASE_2) 
   {
      return (SWM_SUCCESS);
   }

   /* Clear the status protect flag. */
   CR_status_protect = 0;

   /* Allocate space for an RDA Control Buffer. */
   rda_control_message = (ORDA_control_commands_t *) calloc( (size_t) 1,
      (size_t) sizeof(ORDA_control_commands_t) );
   
   if( rda_control_message == NULL )
   {
      LE_send_msg( GL_MEMORY, "RDA Control Commands calloc Failed\n" );
      return ( SWM_FAILURE );
   }

   /* Restore RDA state. */
   if( p_rda_stat == RS_OPERATE )
   {
      rda_control_message->state = RCOM_OPERATE;
      LE_send_msg( GL_INFO, "--->Command RDA to OPERATE\n" );
   }
   else if( p_rda_stat == RS_STANDBY )
   {
      rda_control_message->state = RCOM_STANDBY;
      LE_send_msg( GL_INFO, "--->Command RDA to STANDBY\n" );
   }
   else if( p_rda_stat == RS_OFFOPER )
   {
      rda_control_message->state = RCOM_OFFOPER;
      LE_send_msg( GL_INFO, "--->Command RDA to OFF-LINE OPERATE\n" );
   }

   /* Select the VCP to execute on the next volume scan.  If the previous
      VCP was negative (local pattern), then "Select" the pattern number.  If
      the previous VCP was not negative, download VCP data. */
   if( p_vcp < 0 )
   {
      rda_control_message->select_vcp = abs( p_vcp );
      LE_send_msg( GL_INFO, "--->Command VCP for Next Volume Restart to %d (%d)\n", 
                   rda_control_message->select_vcp, p_vcp );
   }
   else
   {
      int sails_enabled = 0, allow_sails = 0;
      short vcp_flags = 0;

      /* If the previous VCP was an SZ2 VCP, command CMD to enabled and
         download clutter censor zone file 0. */
      if( ORPGVCP_is_SZ2_vcp( p_vcp ) ){

         /* CMD is only valid with ORDA. */
         int cmd_enabled = CR_ORDA_status.status_msg.cmd;

         /* CMD is not enabled, command RDA to enable CMD. */
         if( !cmd_enabled ){

            unsigned char flag;

            LE_send_msg( GL_INFO, "CMD Not Enabled ... Command RDA to enable.\n" );

            /* Set the RPG state for CMD to enabled.  Note:  This flag needs to 
               be set ... otherwise, control_rda will think the CMD status is
               opposite of what the operator wants and will automatically try
               to disable it. */
            ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                   ORPGINFO_STATEFL_SET,
                                   &flag );

            /* Issue RDA Control Command to Enable CMD. */
            SWM_send_rda_command( (int) 2, (int) COM4_RDACOM,
                                  (int) RDA_PRIMARY, (int) CRDA_CMD_ENAB,
                                  (int) 0 );

         }

         /* Download clutter file 0.  With ORDA, the default is assumed 5 zones. */
         SWM_send_rda_command( (int) 2, (int) COM4_SENDCLCZ,
                               (int) RDA_PRIMARY, (int) 5,
                               (int) 0 );

         /* This is needed to support FAA Redundant.   On the inactive channel,
            control RDA only knows what VCP was last executed, not which VCP
            or clutter censor zone was last downloaded. */
         if( CR_redundant_type == ORPGSITE_FAA_REDUNDANT ){

            Redundant_cmd_t red_cmd;

            red_cmd.cmd = ORPGRED_DOWNLOAD_CLUTTER_ZONES;
            red_cmd.lb_id = 0;
            red_cmd.msg_id = 0;
            red_cmd.parameter1 = 5; /* Assumes 5 regions. */;
            red_cmd.parameter2 = 0; /* File 0 is always to default. */

            ORPGRED_send_msg( red_cmd );

         }

      }

      /* Retrieve the previous state VCP data. */
      ret = ORPGRDA_get_previous_state_vcp( (char *) &p_vcp_data,
                                            &p_vcp_data_size );

      /* Check whether the VCP allows SAILS and check whether SAILS
         is enabled.  If SAILS is not enabled or the VCP does not
         allow SAILS, download the baseline VCP definition. */
      sails_enabled = ORPGINFO_is_sails_enabled();
      vcp_flags = ORPGVCP_get_vcp_flags( p_vcp );
      if( vcp_flags & VCP_FLAGS_ALLOW_SAILS )
         allow_sails = 1;

      if( allow_sails && !sails_enabled )
         LE_send_msg( GL_INFO, "Default VCP Being Downloaded Because SAILS not Enabled.\n" );

      /* We will download the previous state VCP data if it exists and
         if the VCP allows SAILS, SAILS is enabled. */
      if( (p_vcp_data_size == 0)
                  ||
          (ret == ORPGRDA_DATA_NOT_FOUND)
                  ||
            (!allow_sails)
                  ||
            (!sails_enabled) ){

         /* Download this VCP to the RDA. */
         ret = SWM_send_rda_command( (int) 1, (int) COM4_DLOADVCP,
                                     (int) RDA_PRIMARY, p_vcp );

      }
      else{ 

         Rda_cmd_t rda_command;

         /* Fill in fields as necessary to download VCP data 
            (see rda_commands.5 man page). */
         rda_command.cmd = COM4_DLOADVCP;
         rda_command.line_num = APP_INITIATED_RDA_CTRL_CMD;
         rda_command.param1 = p_vcp;
         rda_command.param2 = 1;
         rda_command.param3 = VCP_DO_NOT_TRANSLATE;
         rda_command.param4 = 0;
         rda_command.param5 = 0;
         memcpy( &rda_command.msg[0], &p_vcp_data, p_vcp_data_size );
         
         /* Download the VCP data. */
         SWM_process_rda_command( &rda_command );

      }

      rda_control_message->select_vcp = 0;
      LE_send_msg( GL_INFO, "--->Command VCP for Next Volume Restart to 0 (%d)\n",
                   p_vcp );
   }

   /* Restore spot-blanking status. */
   switch( spot_blank_stat )
   {
      case SB_NOT_INSTALLED:
         rda_control_message->spot_blanking = 0;
         LE_send_msg( GL_INFO, "--->Command Spot Blanking to NO CHANGE\n" );
         break;

      case SB_ENABLED:
      case SB_DISABLED:
         /* Note by requirement we are supposed to return to spot blanking
            enabled even if previous state was disabled.  We need to do this 
            if the RPG is starting up, not restarting. */
         if( CR_rpg_restarting == MRPG_STARTUP )
         {
            rda_control_message->spot_blanking = RCOM_SB_ENAB;
            LE_send_msg( GL_INFO, "--->RPG Startup: Command Spot Blanking to ENABLED\n" );
         }
         else
         {
            if( p_spot_blank_stat == SB_ENABLED )
            {
               LE_send_msg( GL_INFO, "--->Command Spot Blanking to Previous State: ENABLED\n" );
               rda_control_message->spot_blanking = RCOM_SB_ENAB;
            }
            else if( p_spot_blank_stat == SB_DISABLED )
            {
               LE_send_msg( GL_INFO, "--->Command Spot Blanking to Previous State: DISABLED\n" );
               rda_control_message->spot_blanking = RCOM_SB_DIS;
            }
         }
         break;
   } /* End of spot_blank_stat switch */

   /* Done with control command. */
   message_size = sizeof(ORDA_control_commands_t)/sizeof(short);

   /* Clear flag for returning to previous state. */
   CR_return_to_previous_state = PREVIOUS_STATE_NO;

   /* Set the CR_rpg_is_restarting flag to invalid value. */
   CR_rpg_restarting = -1;

   /* Execute control command. */
   return ( Process_request( (int) RDA_CONTROL_COMMANDS, 
                             (short *) rda_control_message, message_size, 
                             (malrm_id_t) -1, (time_t) 0 ) );

} /* End of Orda_return_to_previous_state() */


/*\////////////////////////////////////////////////////////////
//
//  Description:   
//     Initializes the Outstanding_request structure.
//
//     Sets the number of outstanding requests to 0.
//     Sets the rpg_to_rda_msg element to NULL, and the 
//     sequence number to an improbable value.
//
//  Inputs:   
//
//  Outputs:
//
//  Returns:
//     This module does not return a value.
//
//  Globals:
//
//  Notes:         
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
///////////////////////////////////////////////////////////\*/ 
void SWM_init_outstanding_requests(){

   int i;

   N_outstanding = 0;
   
   for( i = 0; i < MAXN_OUTSTANDING; i++ ){
   
       Outstanding_request[i].rpg_to_rda_msg = NULL;
       Outstanding_request[i].sequence_number = -1;
  
   /* End of "for" loop */
   }

/* End of SWM_init_outstanding_requests() */
}

/*\/////////////////////////////////////////////////////////////////
//
//  Description:
//     Prints data from the communications manager response.
//
//  Inputs:
//     link_ind - link index for the response.
//     type - "type" field in the response.
//     ret_code - "ret_code" field in the response.
//
//  Outputs:
//
//  Returns:
//
//  Notes:         
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
/////////////////////////////////////////////////////////////////\*/
static void Print_comm_response_data( int link_ind, int type, int ret_code ){

   LE_send_msg( GL_INFO, "--->Link Index:  %d\n", link_ind );

   switch( type ){

      case CM_CONNECT:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_CONNECT\n" );
         break;
      case CM_DISCONNECT:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_DISCONNECT\n" );
         break;
      case CM_WRITE:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_WRITE\n" );
         break;
      case CM_STATUS:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_STATUS\n" );
         break;
      case CM_CANCEL:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_CANCEL\n" );
         break;
      case CM_DATA:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_DATA\n" );
         break;
      case CM_EVENT:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_EVENT\n" );
         break;
      case CM_SET_PARAMS:
         LE_send_msg( GL_INFO, "--->Response Type:  CM_SET_PARAMS\n" );
         break;
      default:
         LE_send_msg( GL_INFO, "--->Response Type:  %d\n", type );

   /* End of "switch" */
   }

   switch( ret_code ){

      case CM_SUCCESS:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_SUCCESS\n" );
         break;
      case CM_TIMED_OUT:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_TIMED_OUT\n" );
         break;
      case CM_NOT_CONFIGURED:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_NOT_CONFIGURED\n" );
         break;
      case CM_DISCONNECTED:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_DISCONNECTED\n" );
         break;
      case CM_BAD_LINK_NUMBER:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_BAD_LINK_NUMBER\n" );
         break;
      case CM_INVALID_PARAMETER:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_INVALID_PARAMETER\n" );
         break;
      case CM_TOO_MANY_REQUESTS:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_TOO_MANY_REQUESTS\n" );
         break;
      case CM_IN_PROCESSING:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_IN_PROCESSING\n" );
         break;
      case CM_TERMINATED:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_TERMINATED\n" );
         break;
      case CM_FAILED:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_FAILED\n" );
         break;
      case CM_REJECTED:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_REJECTED\n" );
         break;
      case CM_LOST_CONN:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_LOST_CONN\n" );
         break;
      case CM_CONN_RESTORED:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_CONN_RESTORED\n" );
         break;
      case CM_LINK_ERROR:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_LINK_ERROR\n" );
         break;
      case CM_START:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_START\n" );
         break;
      case CM_TERMINATE:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_TERMINATE\n" );
         break;
      case CM_STATISTICS:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_STATISTICS\n" );
         break;
      case CM_EXCEPTION:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_EXCEPTION\n" );
         break;
      case CM_NORMAL:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_NORMAL\n" );
         break;
      case CM_INVALID_COMMAND:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_INVALID_COMMAND\n" );
         break;
      case CM_TRY_LATER:
         LE_send_msg( GL_INFO, "--->Return Code:  CM_TRY_LATER\n" );
         break;
      default:
         LE_send_msg( GL_INFO, "--->Return Code:  %d\n", ret_code );

   /* End of "switch" */
   }

/* End of Print_comm_response_data() */
}

