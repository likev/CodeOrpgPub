/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:32:21 $
 * $Id: rdasim_message_converters.c,v 1.4 2005/06/02 19:32:21 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/


#include <rdasim_simulator.h>
#include <rda_rpg_message_header.h>
#include <basedata.h>
#include <rda_performance_maintenance.h>
#include <rda_rpg_console_message.h>
#include <rda_rpg_loop_back.h>
#include <clutter.h>
#include <rpg_request_data.h>
#include <misc.h>

#define MSG_HDR_BYTES         sizeof(RDA_RPG_message_header_t)
#define MSG_HDR_SHORTS        ((MSG_HDR_BYTES + 1)/sizeof(short))

/* 
  Function Prototypes.
*/
static int RDA_control_commands_convert(char*);
static int Vcp_data_convert(char*);
static int Clutter_censor_zones_convert(char*);
static int Request_for_data_convert(char*);
static int RDA_console_msg_convert(char*);
static int Loopback_test_message_convert(char*);
static int Edited_bypass_map_convert(char*);
static int Basedata_header_convert_to_internal(char*);
static int Basedata_header_convert_to_external(char* data);
static int RDA_status_msg_convert(char*);
static int RDA_performance_convert(char*);
static int RPG_console_msg_convert(char*);
static int Clutter_filter_bypass_map_convert(char*, int);
static int Notchwidth_map_convert(char*, int);
static int Wideband_convert_to_internal(char*);
static int Bitdata_convert_to_internal(char*);
static int Calibration_convert_to_internal(char*);
static int Clutter_check_convert_to_internal(char*);
static int Disk_file_status_convert_to_internal(char*);	
static int Device_init_convert_to_internal(char*);
static int Device_io_error_convert_to_internal(char*);

/* NOTE 1:  There are several assumptions made:

   1) The message data is passed as a char array, but it is assumed
      that this array in aligned on a word boundary.

   2) The message header is an integral multiple of shorts. This 
      implies the message data is also aligned on a short boundary.

   If either of these assumptions is not valid, the routines presented
   here will likely not work.

*/

/* NOTE 2:  These functions were written primarily for Big-Endian to
            Little-Endian (or vice versa) conversion.  As of this
            writing, both the RDA and RPG are Big-Endian machines.  
            Therefore for the most part the functions defined in this
            module do nothing more than return to caller.  

            If the message contains Concurrent Floating Point Number(s),
            then the number is converted to IEEE 754 standard format. */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function is the driver module for converting a RPG->RDA 
//      message to external format if the internal format of
//      the RPG is different than internal format of RDA (i.e.,  
//      Little-Endian vs. Big-Endian).
//
//   Input:	
//      msg_type - Message type (see RDA/RPG ICD for valid 
//                 message types). 
//      data - Pointer to start of message (start of the RPG->RDA 
//             message header).
//
//   Output:
//      data - Pointer to converted RPG->RDA message.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
int MC_RDAtoRPG_message_convert_to_external( int msg_type, char* msg_hdr){	

   /*
     Define pointer to message data.
   */
   char *data = (msg_hdr + MSG_HDR_BYTES);

   /*
     Perform RPG->RDA message header conversion.
   */
   MC_RDAtoRPG_message_header_convert(msg_hdr); 
    
   /*
     Convert the message data based on message type.
   */
   switch(msg_type){

      case DIGITAL_RADAR_DATA:
      {
         /*
           TEMPORARY ... For the time being, pass in pointer to msg_hdr.
         */
         Basedata_header_convert_to_external( msg_hdr );
         break;
      }

      case RDA_CONTROL_COMMANDS:
      {
         RDA_control_commands_convert( data );
         break;
      }

      case RDA_STATUS_DATA:
      {
         RDA_status_msg_convert( msg_hdr );
         break;
      }

      case PERFORMANCE_MAINTENANCE_DATA:
      {
         /*
           TEMPORARY ... For the time being, pass in pointer to msg_hdr.
         */
	 /*  Canned performance data is already in external format */
         /*  RDA_performance_convert( msg_hdr );		*/
         break;
      }

      case CONSOLE_MESSAGE_G2A:
      {
         RDA_console_msg_convert( msg_hdr );
         break;
      }


      case RPG_RDA_VCP:
      {
         Vcp_data_convert( data );
         break;
      }

      case CLUTTER_SENSOR_ZONES:
      {
         Clutter_censor_zones_convert( data );
         break;
      }

      case CLUTTER_FILTER_BYPASS_MAP:
      {
         RDA_RPG_message_header_t *header = (RDA_RPG_message_header_t *) msg_hdr;
         int size;

         size = header->size - MSG_HDR_SHORTS;
	 /*  Canned bypass map data is already in external format  */
	 /*  Clutter_filter_bypass_map_convert( data, size );  */
         break;
      }

      case NOTCHWIDTH_MAP_DATA:
      {
         RDA_RPG_message_header_t *header = (RDA_RPG_message_header_t *) msg_hdr;
         int size;

         size = header->size - MSG_HDR_SHORTS;
	 /*  Canned notchwidth data is already in external format */
         /*  Notchwidth_map_convert( data, size );  */
         break;
      }


      case REQUEST_FOR_DATA:
      {
         Request_for_data_convert( data );
         break;
         break;
      }


      case LOOPBACK_TEST_RPG_RDA:

      /*
        We handle the RDA to RPG loopback test as if it were an
        internally generated message.
      */
      case LOOPBACK_TEST_RDA_RPG:
      {
	 /*  Loopback data is already in external format  */
         Loopback_test_message_convert( data ); 
         break;
      }

      case EDITED_CLUTTER_FILTER_MAP:
      {
         Edited_bypass_map_convert( data );
         break;
      }

      default:
      {
         LE_send_msg( GL_ERROR, 
                      "Unrecognized Message type %d To Convert To External.\n", msg_type );
      }

   }
      
   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a RDA_RPG_message_header struct
//      from internal to external or from external to internal format.
//
//   Input:
//      buf - Pointer to message header structure to convert
//
//   Output:
//      buf - Pointer to converted message header structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
int MC_RDAtoRPG_message_header_convert(char* data){	

#ifdef LITTLE_ENDIAN_MACHINE
 /* Pointer to RDA_RPG_message_header_t structure. */
   RDA_RPG_message_header_t* header = 
      (RDA_RPG_message_header_t*) data; 
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(1, &(header->size));
   MISC_swap_shorts(1, &(header->sequence_num));
   MISC_swap_shorts(1, &(header->julian_date));
   MISC_swap_longs(1, (long *) &(header->milliseconds));
   MISC_swap_shorts(1, &(header->num_segs));
   MISC_swap_shorts(1, &(header->seg_num));

#endif	
   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a RDA control command message from 
//      internal format to external format or from internal to external
//      format.
//
//   Input:
//      buf - Pointer to RDA_control_commands_t structure to convert.
//
//   Output:
//      buf - Pointer to converted RDA_control_commands_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_control_commands_convert(char* buf){	


#ifdef LITTLE_ENDIAN_MACHINE	


   /* Pointer to console message structure. */
   RDA_control_commands_t* control_cmd = (RDA_control_commands_t*) buf; 

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(14, (short*) &(control_cmd->state));
   MISC_swap_longs(1, (long *) &(control_cmd->start_time));
   MISC_swap_shorts(1, (short*) &(control_cmd->start_date));
   MISC_swap_longs(1, (long *) &(control_cmd->stop_time));
   MISC_swap_shorts(7, (short*) &(control_cmd->stop_date));

#endif

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a console message from internal 
//      format to external format.
//
//   Input:
//      buf - Pointer to console message to convert.
//
//   Output:
//      buf - Pointer to converted console message.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_console_msg_convert(char* buf){	


#ifdef LITTLE_ENDIAN_MACHINE	

   /* Pointer to console message structure. */
   RDA_RPG_console_message_t* console_msg = (RDA_RPG_console_message_t*) buf; 

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(1, &(console_msg->size));

#endif

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts Vcp_struct from external to internal 
//      or from internal to external format.
//
//   Input:
//      buf - Pointer to Vcp_struct structure to convert.
//
//   Output:
//     buf - Pointer to converted Vcp_struct structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Vcp_data_convert(char* buf){
	
#ifdef LITTLE_ENDIAN_MACHINE

   Vcp_struct* rec = (Vcp_struct*) buf; /* Pointer to Vcp_struct struct. */
   int i,j;

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(7, &(rec->msg_size));
/*   MISC_swap_shorts(5, &(rec->unused1)); */
   for (i = 0; (i < VCP_MAXN_CUTS); i++)

      for (j = 0; (j < ELE_ATTR_SIZE); j++)
         MISC_swap_shorts(1, &(rec->vcp_ele[i][j]));

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts clutter censor zone structure from 
//      internal to external format.
//
//   Input:
//      buf - Pointer to RPG_clutter_regions_t structure to convert.
//
//   Output:
//     buf - Pointer to converted RPG_clutter_regions_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Clutter_censor_zones_convert(char* buf){
	
#ifdef LITTLE_ENDIAN_MACHINE

   /* Pointer to RPG_clutter_regions_t struct. */
   RPG_clutter_regions_t *zone_msg = (RPG_clutter_regions_t*) buf;
   short                  num_regions = zone_msg->regions; 
   int i;

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(1, &(zone_msg->regions));

   /* 
     Do for all censor zones.
   */
   for (i = 0; (i < num_regions); i++)

      MISC_swap_shorts(8, &(zone_msg->data[i].start_range));

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a request_data_t structure from internal i
//      to external format or vice-versa.
//
//   Input:
//      buf - Pointer to request_data_t structure to convert.
//
//   Output:
//      buf - Pointer to converted request_data_t structure.
//
//   Returns:
//      Positive integer on success, or negative error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Request_for_data_convert(char* buf){

#ifdef LITTLE_ENDIAN_MACHINE

   /* Pointer to request_data_t data. */
   RPG_request_data_t* request_data = (RPG_request_data_t*) buf;

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(1, (short*) (request_data));

#endif	

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a loopback message from external/internal 
//      format to internal/external format.
//
//   Input:
//      buf - Pointer to loopback_msg_t structure to convert.
//
//   Output:
//      buf - Pointer to converted loopback_msg_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int Loopback_test_message_convert(char* buf){	

#ifdef LITTLE_ENDIAN_MACHINE	

   /* Pointer to loopback message structure. */
   RDA_RPG_loop_back_message_t* loopback_msg = 
                                (RDA_RPG_loop_back_message_t*) buf; 
   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(1, &(loopback_msg->size));

#endif

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the edited bypass map structure from 
//      internal to external format.
//
//   Input:
//      buf - Pointer to RDA_bypass_map_t structure to convert.
//
//   Output:
//     buf - Pointer to converted RDA_bypass_map_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Edited_bypass_map_convert(char* buf){
	
#ifdef LITTLE_ENDIAN_MACHINE

   RDA_bypass_map_t *bypass_map = (RDA_bypass_map_t*) buf;
   short             num_segments = bypass_map->num_segs; 
   int i;

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(1, &(bypass_map->num_segs));

   /*
     Do for all bypass map elevation segments.
   */
   for (i = 0; (i < num_segments); i++){

      MISC_swap_shorts(1, &(bypass_map->segment[i].seg_num));
      MISC_swap_shorts((BYPASS_MAP_RADIALS*32), 
                        &(bypass_map->segment[i].data[0][0]));

   }

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the RDA_basedata_header struct from 
//      internal to external format.
//
//   Input:	
//      data - Pointer to basedata radial header.
//
//   Output:
//      data - Pointer to basedata radial header converted to ICD
//             external format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Basedata_header_convert_to_external(char* data){	

   RDA_basedata_header* rec = 
      (RDA_basedata_header*) data; /* Pointer to basedata header. */

   /*
     Convert the calibration constant to internal representation
     of floating point.
   */
   MISC_floats_to_icd(1, &(rec->calib_const));

#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(1, &(rec->msg_len));
   MISC_swap_shorts(2, &(rec->seq_num));	
   MISC_swap_longs(1, (long *) &(rec->msg_time));
   MISC_swap_shorts(2, &(rec->num_msg_segs));	
   MISC_swap_longs(1, (long *) &(rec->time));	
   MISC_swap_shorts(14, &(rec->date));
   MISC_swap_shorts(16, &(rec->ref_ptr));		
   MISC_swap_shorts(16, &(rec->word_43));			

#endif	

   return (1);

}
/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a RDA->RPG message to internal format.
//
//   Input:	
//      msg_type - Message type (see RDA/RPG ICD for valid 
//                 message types). 
//      data - Pointer to start of message data (past the  RDA->RPG 
//             message header).
//
//   Output:
//      data - Pointer to converted RDA->RPG message.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
int MC_RPGtoRDA_message_convert_to_internal(int msg_type, char* msg_hdr)
{	

   /*
     Define pointer to message data.
   */
   char *data = (msg_hdr + MSG_HDR_BYTES);

   /*
     Convert the message data based on message type.
   */
   switch(msg_type){

      case DIGITAL_RADAR_DATA:
      {
         /*
           TEMPORARY ... For the time being, pass in pointer to msg_hdr.
         */
         Basedata_header_convert_to_internal( msg_hdr );
         break;
      }

      case RDA_CONTROL_COMMANDS:
      {
         RDA_control_commands_convert( data );
         break;
      }

      case RDA_STATUS_DATA:
      {
         RDA_status_msg_convert( msg_hdr );
         break;
      }

      case PERFORMANCE_MAINTENANCE_DATA:
      {
         /*
           TEMPORARY ... For the time being, pass in pointer to msg_hdr.
         */
         RDA_performance_convert( msg_hdr );
         break;
      }

      case CONSOLE_MESSAGE_A2G:
      {
         RPG_console_msg_convert( msg_hdr );
         break;
      }

      case RPG_RDA_VCP:
      {
         Vcp_data_convert( data );
         break;
      }

      case CLUTTER_SENSOR_ZONES:
      {
         Clutter_censor_zones_convert( data );
         break;
      }


      case CLUTTER_FILTER_BYPASS_MAP:
      {
         RDA_RPG_message_header_t *header = (RDA_RPG_message_header_t *) msg_hdr;
         int size;

         size = header->size - MSG_HDR_SHORTS;
         Clutter_filter_bypass_map_convert( data, size );

         break;
      }

      case NOTCHWIDTH_MAP_DATA:
      {
         RDA_RPG_message_header_t *header = (RDA_RPG_message_header_t *) msg_hdr;
	int size;

         size = header->size - MSG_HDR_SHORTS;
         Notchwidth_map_convert( data, size );
         break;
      }

      case LOOPBACK_TEST_RDA_RPG:

      /*
        We treat the RPG to RDA looppback test as if it were an
        externally generated message.
      */
      case LOOPBACK_TEST_RPG_RDA:
      {
	 
         Loopback_test_message_convert( data );
         break;
      }

      case REQUEST_FOR_DATA:
      {
         Request_for_data_convert( data );
         break;
      }

      case EDITED_CLUTTER_FILTER_MAP:
      {
         Edited_bypass_map_convert( data );
         break;
      }

      default:
      {
         LE_send_msg( GL_ERROR, 
                      "Unrecognized Message Type %d To Convert To Internal.\n", msg_type );
      }
   }
   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the RDA_basedata_header struct from 
//      external to internal format.
//
//   Input:	
//      data - Pointer to basedata radial header.
//
//   Output:
//      data - Pointer to basedata radial header converted to RPG
//             internal format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Basedata_header_convert_to_internal(char* data){	

   RDA_basedata_header* rec = 
      (RDA_basedata_header*) data; /* Pointer to basedata header. */

   /*
     Convert the calibration constant to internal representation
     of floating point.
   */
   MISC_floats_from_icd(1, &(rec->calib_const));

#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(1, &(rec->msg_len));
   MISC_swap_shorts(2, &(rec->seq_num));	
   MISC_swap_longs(1, (long *) &(rec->msg_time));
   MISC_swap_shorts(2, &(rec->num_msg_segs));	
   MISC_swap_longs(1, (long *) &(rec->time));	
   MISC_swap_shorts(14, &(rec->date));
   MISC_swap_shorts(16, &(rec->ref_ptr));		
   MISC_swap_shorts(16, &(rec->word_43));			

#endif	

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a rda_status_msg struct from external 
//      to internal format.
//
//   Input:
//      buf - Pointer to RDA status data structure to convert
//
//   Output:
//      buf - Pointer to converted RDA status data structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_status_msg_convert(char* buf){	

#ifdef LITTLE_ENDIAN_MACHINE	

   RDA_status_msg_t* rda_status_msg = 
      (RDA_status_msg_t*) buf;  /* Pointer to rda_status_msg structure. */

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(22, &(rda_status_msg->rda_status));
   MISC_swap_shorts(4, &(rda_status_msg->spare23));
   MISC_swap_shorts(MAX_RDA_ALARMS_PER_MESSAGE,
                     &(rda_status_msg->alarm_code[0]));

#endif

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a console message from external 
//      format to internal format.
//
//   Input:
//      buf - Pointer to console message to convert.
//
//   Output:
//      buf - Pointer to converted console message.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RPG_console_msg_convert(char* buf){	


#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   RDA_console_msg_convert(buf);

#endif

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the edited bypass map structure from 
//      external to internal format.
//
//   Input:
//      buf - Pointer to RDA_bypass_map_t structure to convert.
//      seg_size - Number of shorts in this message segment.
//
//   Output:
//      buf - Pointer to converted RDA_bypass_map_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Clutter_filter_bypass_map_convert(char* buf,
                                                      int seg_size ){
	
#ifdef LITTLE_ENDIAN_MACHINE

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(seg_size, (short *) buf);

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts RDA_notch_map_t structure from 
//      external to internal format.
//
//   Input:
//      buf - Pointer to RDA_notch_map_t structure to convert.
//      seg_size - Number of shorts in this message segment.
//
//   Output:
//      buf - Pointer to converted RDA_notch_map_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Notchwidth_map_convert(char* buf, int seg_size ){
	
#ifdef LITTLE_ENDIAN_MACHINE

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(seg_size, (short*) buf);

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a wideband struct from internal to
//    	external or from external to internal format.
//
//   Input:
//      buf - Pointer to the wideband_t structure to convert
//
//   Output:
//	buf - Pointer to the converted wideband_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Wideband_convert_to_internal(char* buf){
	
#ifdef LITTLE_ENDIAN_MACHINE	

   wideband_t* wideband = 
           (wideband_t*) buf; /* Pointer to wideband_t structure. */

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(1, &(wideband->dcu_status));
   MISC_swap_shorts(1, &(wideband->general_error));
   MISC_swap_shorts(1, &(wideband->svc_15_error));
   MISC_swap_shorts(1, &(wideband->outgoing_frames));
   MISC_swap_shorts(1, &(wideband->frames_fcs_errors));
   MISC_swap_shorts(1, &(wideband->retrans_i_frames));
   MISC_swap_shorts(1, &(wideband->polls_sent_recvd));
   MISC_swap_shorts(1, &(wideband->poll_timeout_exp));
   MISC_swap_shorts(1, &(wideband->min_buf_read_pool));
   MISC_swap_shorts(1, &(wideband->max_buf_read_done));
   MISC_swap_shorts(1, &(wideband->loopback_test));
   MISC_swap_shorts(4, &(wideband->spare12));

#endif	

   return (1);

}

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a bitdata struct from external machine
//      independent format to internal machine dependent format
//
//   Input:
//      buf - Pointer to bitdata_t structure to convert
//
//   Output:
//	buf - Pointer to converted bitdata_t structure.
//
//   Returns:
//	Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Bitdata_convert_to_internal(char* buf){
	
   bitdata_t* bitdata = (bitdata_t*) buf; /* Pointer to bitdata_t
                                             structure. */

   /*
     Convert floats to RPG internal format.
   */
   MISC_floats_from_icd(7, &(bitdata->ant_pk_pwr));		
   MISC_floats_from_icd(5, &(bitdata->sh_pulse_lin_chan_noise));			

#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If machine is Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(7, &(bitdata->data31));
   MISC_swap_shorts(1, &(bitdata->dau_interface));
   MISC_swap_shorts(1, &(bitdata->xmtr_sum_stat));
   MISC_swap_shorts(32, &(bitdata->spare64));
   MISC_swap_shorts(5, &(bitdata->data96));
   MISC_swap_shorts(5, &(bitdata->spare101));		
   MISC_swap_shorts(2, &(bitdata->spare107));
   MISC_swap_shorts(13, &(bitdata->spare110));	
   MISC_swap_longs(1, (long *) &(bitdata->xmtr_rec_cnt));		
   MISC_swap_shorts(2, &(bitdata->spare139));
   MISC_swap_shorts(2, &(bitdata->idu_tst_detect));	

#endif	

   return (1);

}

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a calibration struct to an internal 
//      machine dependent format from an external format.
//
//   Input:
//   	buf - Pointer to calibration_t structure to convert.
//
//   Output:
//	buf - Pointer to converted calibration_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number
//
//////////////////////////////////////////////////////////////////\*/
static int Calibration_convert_to_internal(char* buf){
	
   calibration_t* calibration = 
                  (calibration_t*) buf; /* Pointer to calibration_t
                                           structure. */

   /*
     Convert floats to RPG internal format.
   */
   MISC_floats_from_icd(14, &(calibration->agc_step1_amp));
   MISC_floats_from_icd(16, &(calibration->phase_ram1_exp_vel));	
   MISC_floats_from_icd(16, &(calibration->cw_lin_tgt_exp_amp));
   MISC_floats_from_icd(4, &(calibration->short_pulse_lin_syscal));	
   MISC_floats_from_icd(12, &(calibration->kd1_lin_tgt_exp_amp));

#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(20, &(calibration->spare181));	
   MISC_swap_shorts(18, &(calibration->spare273));	
   MISC_swap_shorts(16, &(calibration->spare315));							

#endif	

   return (1);

}

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts the clutter_check struct from an 
//      external machine independent format to an internal machine 
//      dependent format.
//
//   Input:
//	buf - Pointer to clutter_check_t structure to convert.
//
//   Output:
//	buf - Pointer to converted clutter_check_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Clutter_check_convert_to_internal(char* buf){	

   clutter_check_t* clutter_check = 
                    (clutter_check_t*) buf; /* Pointer to clutter_check_t
                                               structure. */

   /*
     Convert floats to RPG internal format.
   */
   MISC_floats_from_icd(4,&(clutter_check->unfilt_lin_chan_pwr));

#ifdef LITTLE_ENDIAN_MACHINE

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(22, &(clutter_check->spare339));

#endif	

   return (1);

}

/*////////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a disk_file_status struct from external 
//      to internal or from internal to external.
//
//   Input:	
//      buf - Pointer to disk_file_status_t structure to convert.
//
//   Output:
//	buf - Pointer to converted disk_file_status_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Disk_file_status_convert_to_internal(char* buf){

	
#ifdef LITTLE_ENDIAN_MACHINE

   disk_file_status_t* disk_file_status = 
        (disk_file_status_t*) buf; /* Pointer to disk_file_status_t 
                                      structure. */
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(13, &(disk_file_status->state_fil_rd_stat));
   MISC_swap_shorts(27, &(disk_file_status->spare374));

#endif	

   return (1);

}

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a device_init struct from external to 
//      internal or from internal to external.
//
//   Input:	
//      buf - Pointer to device_init_t structure to convert.
//
//   Output:
//      buf - Pointer to converted device_init_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Device_init_convert_to_internal(char* buf){


#ifdef LITTLE_ENDIAN_MACHINE	

   device_init_t* device_init = 
          (device_init_t*) buf; /* Pointer to device_init_t structure. */

   /* 
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(13, &(device_init->dau_init_stat));
   MISC_swap_shorts(17, &(device_init->spare414));

#endif	

   return (1);

}

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a device_io_error struct from external 
//      to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to device_io_error_t structure to convert.
//
//   Output:
//	buf - Pointer to converted device_io_error_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Device_io_error_convert_to_internal(char* buf){
	
#ifdef LITTLE_ENDIAN_MACHINE

   device_io_error_t* device_io_error = 
          (device_io_error_t*)buf;  /* Pointer to device_io_error_t
                                       structure. */

   /*
     If machine is Little-Endian, perform byte swapping.
   */
   MISC_swap_longs(2, (long *) &(device_io_error->dau_io_err_stat));
   MISC_swap_longs(2, (long *) &(device_io_error->mc_io_err_stat));	
   MISC_swap_longs(2, (long *) &(device_io_error->ped_io_err_stat));	
   MISC_swap_longs(2, (long *) &(device_io_error->sps_io_err_stat));		
   MISC_swap_longs(2, (long *) &(device_io_error->arch2_io_err_stat));		
   MISC_swap_longs(2, (long *) &(device_io_error->disk_io_err_stat));		
   MISC_swap_longs(3, (long *) &(device_io_error->arch2_sum_err_stat));		
   MISC_swap_shorts(14, &(device_io_error->spare487));		

#endif

   return (1);

}

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts an rda_performance struct from external 
//      machine independent format to an internal machine dependent 
//      format.
//
//   Input:	
//      buf - Pointer to rda_performance_t structure to convert.
//
//   Output:
//      buf - Pointer to converted rda_performance_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_performance_convert(char* buf){
	
   rda_performance_t* rda_performance =
            (rda_performance_t*) buf; /* Pointer to rda_performance_t
                                         structure. */

   /*
     Perform necessary byte swapping. 
   */
   Wideband_convert_to_internal((char*)&(rda_performance->rpg));
   Wideband_convert_to_internal((char*)&(rda_performance->user));	
   Bitdata_convert_to_internal((char*)&(rda_performance->data));
   Calibration_convert_to_internal((char*)&(rda_performance->calibration));
   Clutter_check_convert_to_internal((char*)&(rda_performance->clutter_check));
   Disk_file_status_convert_to_internal((char*)&(rda_performance->disk_status));	
   Device_init_convert_to_internal((char*)&(rda_performance->device_init));
   Device_io_error_convert_to_internal((char*)&(rda_performance->device_error));

   return (1);

}

