/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/05 20:35:36 $
 * $Id: orpgumc_rda.c,v 1.61 2013/06/05 20:35:36 steves Exp $
 * $Revision: 1.61 $
 * $State: Exp $
 */
#include <orpgumc_rda.h>

#define MSG_HDR_BYTES         sizeof(RDA_RPG_message_header_t)
#define MSG_HDR_SHORTS        ((MSG_HDR_BYTES + 1)/sizeof(short))

#define BITS_PER_BYTE		8

/* 
  Function Prototypes.
*/
static int Basedata_header_convert_to_external(char*);
static int ORDA_Basedata_header_convert_to_external(char*);
static int Generic_Basedata_convert_to_external(char*);
static int Generic_Basedata_convert(int to_external, char*);
static int RDA_status_msg_convert_to_external(char*);
static int ORDA_status_msg_convert_to_external(char*);
static int RDA_control_commands_convert_to_external(char*);
static int ORDA_control_commands_convert_to_external(char*);
static int Vcp_data_convert_to_external(char*);
static int Vcp_data_convert(int to_external, char*);
static int Clutter_censor_zones_convert_to_external(char*);
static int ORDA_Clutter_censor_zones_convert_to_external(char*);
static int Clutter_censor_zones_convert_to_internal(char*);
static int ORDA_Clutter_censor_zones_convert_to_internal(char*);
static int Request_for_data_convert_to_external(char*);
static int RDA_console_msg_convert_to_external(char*);
static int Loopback_test_message_convert_to_external(char*);
static int Edited_bypass_map_convert_to_external(char*, int);
static int ORDA_Edited_bypass_map_convert_to_external(char*, int);
static int Basedata_header_convert_to_internal(char*);
static int ORDA_Basedata_header_convert_to_internal(char*);
static int Generic_Basedata_convert_to_internal(char*);
static int RDA_status_msg_convert_to_internal(char*);
static int ORDA_status_msg_convert_to_internal(char*);
static int RDA_performance_convert_to_internal(char*);
static int RPG_console_msg_convert_to_internal(char*);
static int Vcp_data_convert_to_internal(char*);
static int Loopback_test_message_convert_to_internal(char*);
static int Clutter_filter_bypass_map_convert_to_internal(char*, int);
static int ORDA_Clutter_filter_bypass_map_convert_to_internal(char*, int);
static int Notchwidth_map_convert_to_internal(char*);
static int ORDA_Clutter_map_convert_to_internal(char*, int);
static int Wideband_convert_to_internal(char*);
static int Bitdata_convert_to_internal(char*);
static int Calibration_convert_to_internal(char*);
static int Clutter_check_convert_to_internal(char*);
static int Disk_file_status_convert_to_internal(char*);	
static int Device_init_convert_to_internal(char*);
static int Device_io_error_convert_to_internal(char*);
static int Comms_convert_to_internal(char* buf);
static int Power_convert_to_internal(char* buf);
static int Transmitter_convert_to_internal(char* buf);
static int Tower_utilities_convert_to_internal(char* buf);
static int Equipment_shelter_convert_to_internal(char* buf);
static int Antenna_pedestal_convert_to_internal(char* buf);
static int Rf_gnrtr_rcvr_convert_to_internal(char* buf);
static int Calib_convert_to_internal(char* buf);
static int File_status_convert_to_internal(char* buf);
static int Device_status_convert_to_internal(char* buf);
static int AME_convert_to_internal(char* buf);
static int ORDA_performance_convert_to_internal(char*);
static int ORDA_performance_convert_to_external(char*);
static int Float_concurrent_to_ieee754 (float* x);
static int Float_ieee754_to_concurrent (float* x);
static int ORDA_adapt_convert_to_internal(char* buf);
static int ORDA_adapt_convert_to_external(char* buf);


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

/*\////////////////////////////////////////////////////////////////////////
//                                                                                         
//   Description: This function converts the RDA messages from the ICD format
//                to the C structure format.
//                                                                                        
//  Input:      msg - the pointer to the ICD message starting with the 16 byte
//              message header (without the comms and CTM headers).
//                                                                                        
//  Output:     msg - the converted message.
//                                                                                        
//  Return:     returns the length of the converted message or a negative
//              error code (See umc manpages) on failure.
//                                                                                        
////////////////////////////////////////////////////////////////////////\*/
int UMC_from_ICD_RDA ( void *msg )
{
   int len = 0;
   RDA_RPG_message_header_t* msg_header = NULL;

   /* Convert the message header from external format to internal format
   (i.e., if the RDA is Big-Endian and the RPG is Little-Endian (or
   vice versa). */
   UMC_RDAtoRPG_message_header_convert( (char *) msg );
    
   /* Cast the byte swapped data to the message header structure */
   msg_header = (RDA_RPG_message_header_t *) msg;

   /* Convert the message data from external format to internal format
   (i.e., if the RDA is Big-Endian and the RPG is Little-Endian (or
   vice versa) based on the message type. */
   len = UMC_RDAtoRPG_message_convert_to_internal( (int)msg_header->type,
                                                   (char *) msg );

   return (len);

} /* end UMC_from_ICD_RDA() */


/*\////////////////////////////////////////////////////////////////////////
//                                                                                         
//   Description: This function converts the RPG messages in internal
//		  format to the ICD format.
//                                                                                        
//  Input:      msg - the pointer to the ICD message.
//                                                                                        
//  Output:     msg - the converted message.
//                                                                                        
//  Return:     returns the length of the converted message or a negative
//              error code (See umc manpages) on failure.
//                                                                                        
////////////////////////////////////////////////////////////////////////\*/
int UMC_to_ICD_RDA ( void *msg )
{
   int                          len		= 0;
   int                          msg_type	= 0;
   RDA_RPG_message_header_t*    msg_header	= NULL;


#ifdef LITTLE_ENDIAN_MACHINE

   CM_req_struct*	req_struct	= NULL;

   if ( (char*) msg == NULL )
      return -1;

   /* Cast it and check data size to see if there's any other data. */
   req_struct = (CM_req_struct *)msg;

   if ( req_struct->data_size > sizeof(CM_req_struct) )
   {
      /* increment pointer past CM and CTM header */
      msg = msg + sizeof(CM_req_struct) + CTM_HDRSZE_BYTES;
   }
   else
   {
      return (0);
   }

#endif   

   msg_header = (RDA_RPG_message_header_t *) msg;
   msg_type = (int) (msg_header->type);

   /* Convert the message data from internal format to external format
     (i.e., if the RDA is Big-Endian and the RPG is Little-Endian (or
     vice versa) based on the message type. */
   len = UMC_RPGtoRDA_message_convert_to_external( msg_type, msg );

   return (len);

} /* end UMC_to_ICD_RDA() */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function is the driver module for converting a RPG->RDA 
//      message to external format if the internal format of
//      the RPG is different than internal format of RDA (i.e.,  
//      Little-Endian vs. Big-Endian).
//      NOTE: this function assumes that the outgoing msg hdr has
//      ALREADY BEEN CONVERTED to Big-Endian format.
//
//   Input:	
//      msg_type - Message type (see RDA/RPG ICD for valid 
//                 message types). 
//      msg_ptr  - Pointer to start of message (start of the RPG->RDA 
//                 message header).
//
//   Output:
//      msg_ptr  - Pointer to converted RPG->RDA message.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
int UMC_RPGtoRDA_message_convert_to_external( int msg_type, char* msg_ptr)
{	

   /*
     Define return value. 
   */
   int ret = 1;

   /*
     Define pointer to message data.
   */
   char *data = (msg_ptr + MSG_HDR_BYTES);

   /*
     Define a variable to hold the RDA configuration value. Init to 0.
   */
   int RDA_config_value = 0;

   /*
     Call the ORPGRDA lib function to return the RDA configuration
   */
   RDA_config_value = ORPGRDA_get_rda_config( NULL );
   if ( ( RDA_config_value != ORPGRDA_LEGACY_CONFIG ) &&
        ( RDA_config_value != ORPGRDA_ORDA_CONFIG ) )
   {
      LE_send_msg(GL_ERROR,
         "Invalid RDA Configuration Value. Resetting to ORPGRDA_ORDA_CONFIG\n" );
      RDA_config_value = ORPGRDA_ORDA_CONFIG;
   }


   /*
     Convert the message data based on message type.
   */

   if ( RDA_config_value == ORPGRDA_LEGACY_CONFIG )
   {
      switch(msg_type)
      {
         case DIGITAL_RADAR_DATA:
         {
            ret = Basedata_header_convert_to_external( msg_ptr );
            break;
         }
         case GENERIC_DIGITAL_RADAR_DATA:
         {
            ret = Generic_Basedata_convert_to_external( msg_ptr );
            break;
         }
         case RDA_STATUS_DATA:
         {
            ret = RDA_status_msg_convert_to_external( msg_ptr );
            break;
         }
         case RDA_CONTROL_COMMANDS:
         {
            ret = RDA_control_commands_convert_to_external( data );
            break;
         }
   
         case RPG_RDA_VCP:
         case RDA_RPG_VCP:
         {
            ret = Vcp_data_convert_to_external( data );
            break;
         }
   
         case CLUTTER_SENSOR_ZONES:
         {
            ret = Clutter_censor_zones_convert_to_external( data );
            break;
         }
   
         case REQUEST_FOR_DATA:
         {
            ret = Request_for_data_convert_to_external( data );
            break;
         }
   
         case CONSOLE_MESSAGE_G2A:
         {
            ret = RDA_console_msg_convert_to_external( msg_ptr );
            break;
         }
   
         case LOOPBACK_TEST_RPG_RDA:

         /*
           We handle the RDA to RPG loopback test as if it were an
           internally generated message.
         */
         case LOOPBACK_TEST_RDA_RPG:
         {
            ret = Loopback_test_message_convert_to_external( msg_ptr );
            break;
         }
   
         case EDITED_CLUTTER_FILTER_MAP:
         {
	    RDA_RPG_message_header_t* hdr = (RDA_RPG_message_header_t*)msg_ptr;
	    short size_shorts = hdr->size;
	    /* Since the msg hdr is in Big Endial format at this point, we must 
	       do some swapping before using this data. */
            MISC_swap_shorts(1, &size_shorts);
	    size_shorts = size_shorts - MSG_HDR_SHORTS;
            ret = Edited_bypass_map_convert_to_external( data, size_shorts );
            break;
         }
   
         default:
         {
            LE_send_msg( GL_ERROR, 
                         "Unrecognized Message To Convert To External.\n" );
            ret = 0;
         }
      }
   } 
   else
   {
      /* RDA is a Open RDA Configuration */
      switch(msg_type)
      {
         case DIGITAL_RADAR_DATA:
         {
            ret = ORDA_Basedata_header_convert_to_external( msg_ptr );
            break;
         }
         case GENERIC_DIGITAL_RADAR_DATA:
         {
            ret = Generic_Basedata_convert_to_external( msg_ptr );
            break;
         }
         case RDA_STATUS_DATA:
         {
            ret = ORDA_status_msg_convert_to_external( msg_ptr );
            break;
         }
         case RDA_CONTROL_COMMANDS:
         {
            ret = ORDA_control_commands_convert_to_external( data );
            break;
         }
   
         case RPG_RDA_VCP:
         case RDA_RPG_VCP:
         {
            ret = Vcp_data_convert_to_external( data );
            break;
         }
   
         case CLUTTER_SENSOR_ZONES:
         {
            ret = ORDA_Clutter_censor_zones_convert_to_external( data );
            break;
         }
   
         case REQUEST_FOR_DATA:
         {
            ret = Request_for_data_convert_to_external( data );
            break;
         }
   
         case CONSOLE_MESSAGE_G2A:
         {
            ret = RDA_console_msg_convert_to_external( msg_ptr );
            break;
         }
   
         case LOOPBACK_TEST_RPG_RDA:
   
         /*
           We handle the RDA to RPG loopback test as if it were an
           internally generated message.
         */
         case LOOPBACK_TEST_RDA_RPG:
         {
            ret = Loopback_test_message_convert_to_external( msg_ptr );
            break;
         }

         case PERFORMANCE_MAINTENANCE_DATA:
         {
            ret = ORDA_performance_convert_to_external( msg_ptr );
            break;
         }
   
         case EDITED_CLUTTER_FILTER_MAP:
         {
	    RDA_RPG_message_header_t* hdr = (RDA_RPG_message_header_t*)msg_ptr;
	    short size_shorts = hdr->size;
	    /* Since the msg hdr is in Big Endial format at this point, we must 
	       do some swapping before using this data. */
            MISC_swap_shorts(1, &size_shorts);
	    size_shorts = size_shorts - MSG_HDR_SHORTS;
            ret = ORDA_Edited_bypass_map_convert_to_external( data, size_shorts );
            break;
         }

         case ADAPTATION_DATA:
         {
            int ret = 0;
            if ( (ret = ORDA_adapt_convert_to_external( data )) < 0)
            {
               LE_send_msg(GL_ERROR,
                  "Error in ORDA_adapt_convert_to_external.\n");
               return(-1);
            }
            break;
         }
   
         default:
         {
            LE_send_msg( GL_ERROR, 
                         "Unrecognized Message %d To Convert To External.\n", msg_type );
            ret = 0;
         }
      }
   }

   return (ret);

} /* end UMC_RPGtoRDA_message_convert_to_external() */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a RDA_RPG_message_header struct
//      from internal to external or from external to internal format.
//
//   Input:
//      data - Char pointer to message header data to convert
//
//   Output:
//      data - Char pointer to converted message header data
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
int UMC_RDAtoRPG_message_header_convert(char* data)
{

#ifdef LITTLE_ENDIAN_MACHINE

 /* Pointer to RDA_RPG_message_header_t structure. */
   RDA_RPG_message_header_t* header = (RDA_RPG_message_header_t*) data; 

   MISC_swap_shorts(1, (short *) &(header->size));
   MISC_swap_shorts(1, (short *) &(header->sequence_num));
   MISC_swap_shorts(1, (short *) &(header->julian_date));
   MISC_swap_longs(1, (long *) &(header->milliseconds));
   MISC_swap_shorts(1, (short *) &(header->num_segs));
   MISC_swap_shorts(1, (short *) &(header->seg_num));

#endif	

   return (1);

} /* end UMC_RDAtoRPG_message_header_convert */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the RDA_basedata_header struct from
//      internal to external format.
//      NOTE: it is assumed that the msg hdr portion has already been
//      converted.
//
//   Input:
//      data - Pointer to basedata radial header (including msg hdr)
//
//   Output:
//      data - Pointer to basedata radial header converted to RPG
//             external format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Basedata_header_convert_to_external(char* data)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = Basedata_header_convert_to_internal(data);

#endif

   return (ret);

} /* end Basedata_header_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the ORDA_basedata_header struct from
//      internal to external format.
//      NOTE: it is assumed that the msg hdr portion has already been
//      converted.
//
//   Input:
//      data - Pointer to basedata radial header (including msg hdr)
//
//   Output:
//      data - Pointer to basedata radial header converted to RPG
//             external format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Basedata_header_convert_to_external(char* data)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = ORDA_Basedata_header_convert_to_internal(data);

#endif

   return (ret);

} /* end ORDA_Basedata_header_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the ORDA_generic_basedata_header struct
//      from external to internal format.
//      NOTE: it is assumed that the msg hdr portion has already been
//      converted.
//
//   Input:
//      data - Pointer to generic basedata header (including
//             msg hdr)
//
//   Output:
//      data - Pointer to generic basedata header converted to
//             RPG internal format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
int UMC_generic_basedata_header_convert_to_internal(char* data)
{

   int ret = 1;
   Generic_basedata_t* rec = (Generic_basedata_t *) data;
   int no_of_datum;

#ifdef LITTLE_ENDIAN_MACHINE

  /* If this is a Little-Endian machine, perform byte swapping. */
   MISC_swap_longs( 1, (long *) &(rec->base.time) );
   MISC_swap_shorts( 2, (short *) &(rec->base.date) );
   MISC_swap_floats( 1, &(rec->base.azimuth) );
   MISC_swap_shorts( 1, (short *) &(rec->base.radial_length) );
   MISC_swap_floats( 1, &(rec->base.elevation) );
   MISC_swap_shorts( 1, (short *) &(rec->base.no_of_datum) );

   no_of_datum = rec->base.no_of_datum;
   MISC_swap_longs( no_of_datum, (long *) &(rec->base.data[0]) );

#endif

   return (ret);

} /* end UMC_generic_basedata_header_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the ORDA_generic_basedata_header struct
//      from internal to external format.
//      NOTE: it is assumed that the msg hdr portion has already been
//      converted.
//
//   Input:
//      data - Pointer to generic basedata header (including
//             msg hdr)
//
//   Output:
//      data - Pointer to generic basedata header converted to
//             RPG external format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
int UMC_generic_basedata_header_convert_to_external(char* data)
{

   int ret = 1;
   Generic_basedata_t* rec = (Generic_basedata_t *) data; 
   int no_of_datum;

#ifdef LITTLE_ENDIAN_MACHINE

   no_of_datum = rec->base.no_of_datum;

  /* If this is a Little-Endian machine, perform byte swapping. */
   MISC_swap_longs( 1, (long *) &(rec->base.time) );
   MISC_swap_shorts( 2, (short *) &(rec->base.date) );
   MISC_swap_floats( 1, &(rec->base.azimuth) );
   MISC_swap_shorts( 1, (short *) &(rec->base.radial_length) );
   MISC_swap_floats( 1, &(rec->base.elevation) );
   MISC_swap_shorts( 1, (short *) &(rec->base.no_of_datum) );

   MISC_swap_longs( no_of_datum, (long *) &(rec->base.data[0]) );

#endif

   return (ret);

} /* end UMC_generic_basedata_header_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the ORDA_generic_basedata_t struct 
//      from internal to external format.
//      NOTE: it is assumed that the msg hdr portion has already been
//      converted.
//
//   Input:
//      data - Pointer to generic basedata radial (including 
//             msg hdr)
//
//   Output:
//      data - Pointer to generic basedata radial converted to 
//             RPG external format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Generic_Basedata_convert_to_external(char* data)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = Generic_Basedata_convert(1, data);

#endif

   return (ret);

} /* end Generic_Basedata_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the ORDA_generic_basedata_t struct 
//      from external to internal format.
//      NOTE: it is assumed that the msg hdr portion has already been
//      converted.
//
//   Input:
//      data - Pointer to generic basedata radial (including 
//             msg hdr)
//
//   Output:
//      data - Pointer to generic basedata radial converted to 
//             RPG internal format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Generic_Basedata_convert_to_internal(char* data)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = Generic_Basedata_convert(0, data);

#endif

   return (ret);

} /* end Generic_Basedata_convert_to_internal */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts an rda_status_msg struct from internal
//      to external format.
//      NOTE: it is assumed the msg hdr has already been converted.
//
//   Input:
//      buf - Pointer to RDA status data structure to convert 
//            (including msg hdr)
//
//   Output:
//      buf - Pointer to converted RDA status data structure.
//
//   Returns:
//      Positive number on success or a non-positive error
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_status_msg_convert_to_external(char* buf)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = RDA_status_msg_convert_to_internal(buf);

#endif

   return (ret);

} /* end RDA_status_msg_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts an rda_status_msg struct from internal
//      to external format.
//      NOTE: it is assumed the msg hdr has already been converted.
//
//   Input:
//      buf - Pointer to ORDA status data structure to convert (includ msg hdr)
//
//   Output:
//      buf - Pointer to converted ORDA status data structure.
//
//   Returns:
//      Positive number on success or a non-positive error
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_status_msg_convert_to_external(char* buf)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = ORDA_status_msg_convert_to_internal(buf);

#endif

   return (ret);

} /* end ORDA_status_msg_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a RDA control command message from 
//      internal format to external format.
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
static int RDA_control_commands_convert_to_external(char* buf){	


#ifdef LITTLE_ENDIAN_MACHINE	

   /* Pointer to RDA control commands message structure. */
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
//      This function converts a Open RDA control command message from 
//      internal format to external format.
//
//   Input:
//      buf - Pointer to ORDA_control_commands_t structure to convert.
//
//   Output:
//      buf - Pointer to converted ORDA_control_commands_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_control_commands_convert_to_external(char* buf){	


#ifdef LITTLE_ENDIAN_MACHINE	

   /* Pointer to ORDA control commands message structure. */
   ORDA_control_commands_t* control_cmd = (ORDA_control_commands_t*) buf; 

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(26, (short*) &(control_cmd->state));

#endif

   return (1);

}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a console message from internal 
//      format to external format.
//      NOTE: The msg hdr is assumed already in Big-Endian format.
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
static int RDA_console_msg_convert_to_external(char* buf)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE	

   ret = RPG_console_msg_convert_to_internal( buf );

#endif

   return (ret);

} /* end RDA_console_msg_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts Vcp_struct from internal to external.
//
//   Input:
//      buf - Pointer to Vcp_struct structure (without the msg hdr)
//	      to convert.
//
//   Output:
//     buf - Pointer to converted Vcp_struct structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Vcp_data_convert_to_external(char* buf)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = Vcp_data_convert(1, buf);

#endif	      

   return (ret);
} /* end Vcp_data_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts Vcp_struct from external to internal.
//
//   Input:
//      buf - Pointer to Vcp_struct structure (without the msg hdr)
//	      to convert.
//
//   Output:
//     buf - Pointer to converted Vcp_struct structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Vcp_data_convert_to_internal(char* buf)
{

   int ret = 1;

#ifdef LITTLE_ENDIAN_MACHINE

   ret = Vcp_data_convert(0, buf);

#endif	      

   return (ret);
} /* end Vcp_data_convert_to_internal */

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
static int Clutter_censor_zones_convert_to_external(char* buf)
{
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */

#ifdef LITTLE_ENDIAN_MACHINE

   /* Pointer to RPG_clutter_regions_t struct. */
   RPG_clutter_regions_t *zone_msg = (RPG_clutter_regions_t*) buf;
   short                  num_regions = zone_msg->regions; 
   int i;

   MISC_swap_shorts(1, &(zone_msg->regions));

   /* 
     Do for all censor zones.
   */
   for (i = 0; (i < num_regions); i++)
   {
      MISC_swap_shorts(8, &(zone_msg->data[i].start_range));
   }

#endif	      

   return (1);
}  /* end Clutter_censor_zones_convert_to_external */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts ORDA clutter censor zone structure from 
//      internal to external format.
//
//   Input:
//      buf - Pointer to ORPG_clutter_regions_t structure to convert.
//
//   Output:
//     buf - Pointer to converted ORPG_clutter_regions_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Clutter_censor_zones_convert_to_external(char* buf)
{
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */

#ifdef LITTLE_ENDIAN_MACHINE

   /* Pointer to ORPG_clutter_regions_t struct. */
   ORPG_clutter_regions_t *zone_msg = (ORPG_clutter_regions_t*) buf;
   short num_regions = zone_msg->regions; 
   int i;

   MISC_swap_shorts(1, &(zone_msg->regions));

   /* 
     Do for all censor zones.
   */
   for (i = 0; (i < num_regions); i++)
   {
      MISC_swap_shorts(6, &(zone_msg->data[i].start_range));
   }

#endif	      

   return (1);

}  /* end ORDA_Clutter_censor_zones_convert_to_external */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts clutter censor zone message from 
//      external to internal format.
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
static int Clutter_censor_zones_convert_to_internal(char* buf)
{
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */

#ifdef LITTLE_ENDIAN_MACHINE

   /* Pointer to RPG_clutter_regions_t struct. */
   RPG_clutter_regions_t  *zone_msg = (RPG_clutter_regions_t*) buf;
   int                    i;

   MISC_swap_shorts(1, &(zone_msg->regions));

   /* 
     Do for all censor zones.
   */
   for (i = 0; i < zone_msg->regions ; i++)
   {
      MISC_swap_shorts(8, &(zone_msg->data[i].start_range));
   }

#endif	      

   return (1);

} /* end Clutter_censor_zones_convert_to_internal() */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts ORDA clutter censor zone message from 
//      external to internal format.
//
//   Input:
//      buf - Pointer to ORPG_clutter_regions_t structure to convert.
//
//   Output:
//     buf - Pointer to converted ORPG_clutter_regions_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Clutter_censor_zones_convert_to_internal(char* buf)
{
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */

#ifdef LITTLE_ENDIAN_MACHINE

   /* Pointer to ORPG_clutter_regions_t struct. */
   ORPG_clutter_regions_t  *zone_msg = (ORPG_clutter_regions_t*) buf;
   int                    i;

   MISC_swap_shorts(1, &(zone_msg->regions));

   /* 
     Do for all censor zones.
   */
   for (i = 0; i < zone_msg->regions ; i++)
   {
      MISC_swap_shorts(6, &(zone_msg->data[i].start_range));
   }

#endif	      

   return (1);

} /* end ORDA_Clutter_censor_zones_convert_to_internal() */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a request_data_t structure from internal
//      to external format.
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
static int Request_for_data_convert_to_external(char* buf){

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
//	NOTE: it is assumed the msg hdr has been converted already to
//      Big-Endian format.  Therefore, we must convert the size field
//      before using it.
//
//   Input:
//      buf - Pointer to loopback_msg_t structure to convert (including
//	      msg hdr).
//
//   Output:
//      buf - Pointer to converted loopback_msg_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int Loopback_test_message_convert_to_external(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   short tempBuf;
   int msg_size_shorts;
   RDA_RPG_message_header_t* msg_hdr = (RDA_RPG_message_header_t *)buf;
   RDA_RPG_loop_back_message_t* loopback_msg =
      (RDA_RPG_loop_back_message_t*)(buf + MSG_HDR_BYTES); 

   memcpy(&tempBuf, &(msg_hdr->size), sizeof(tempBuf));
   MISC_swap_shorts(1, &tempBuf);
   msg_size_shorts = tempBuf - MSG_HDR_SHORTS;

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */

   MISC_swap_shorts(msg_size_shorts, &(loopback_msg->size));

#endif

   return (1);

} /* end Loopback_test_message_convert_to_external */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the edited bypass map structure from 
//      internal to external format.
//
//   Input:
//      buf - Pointer to RDA_bypass_map_t structure to convert.
//      seg_size - Number of shorts in message.
//
//   Output:
//     buf - Pointer to converted RDA_bypass_map_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Edited_bypass_map_convert_to_external(char* buf, int seg_size)
{

#ifdef LITTLE_ENDIAN_MACHINE

   MISC_swap_shorts(seg_size, (short *)(buf));

#endif

   return (1);

} /* end Edited_bypass_map_convert_to_external */

/*///////////////////////////////////////////////////////////////////
//
//   Description:
//      This function converts an orda_performance struct from internal
//      machine dependent format to an external machine iindependent
//      format.
//      NOTE: it is assumed the msg hdr portion has already been
//      converted.
//
//   Input:
//      buf - Pointer to orda_performance_t structure to convert
//            (including msg hdr).
//
//   Output:
//      buf - Pointer to converted orda_performance_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_performance_convert_to_external(char* buf)
{

   ORDA_performance_convert_to_internal(buf);

   return (1);

} /* end ORDA_performance_convert_to_external */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the ORDA edited bypass map structure from 
//      internal to external format.
//
//   Input:
//      buf - Pointer to ORDA_bypass_map_t structure to convert.
//      seg_size - Number of halfwords in msg.
//
//   Output:
//      buf - Pointer to converted ORDA_bypass_map_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Edited_bypass_map_convert_to_external(char* buf, int seg_size)
{

#ifdef LITTLE_ENDIAN_MACHINE

   MISC_swap_shorts(seg_size, (short *)(buf));

#endif

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a RDA->RPG message to internal format.
//	NOTE: this function assumes the message header has already
//	been converted.  Those bytes are skipped over.
//
//   Input:	
//      int msg_type - Message type (see RDA/RPG ICD for valid 
//                     message types). 
//      char* msg_data - Pointer to start of message (INCLUDING the  RDA->RPG 
//                       message header).
//
//   Output:
//      msg_data - Pointer to converted RDA->RPG message.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
int UMC_RDAtoRPG_message_convert_to_internal(int msg_type, char* msg_data)
{

   /*
     Define return value. 
   */
   int ret = 1;

   /*
     Define a variable to hold the RDA configuration value. Init to 0.
   */
   int RDA_config_value = 0;

   /*
     Define pointer to message data.
   */
   char *data = (msg_data + MSG_HDR_BYTES);

   /*
     Call the ORPGRDA lib function to return the RDA configuration
   */
   RDA_config_value = ORPGRDA_get_rda_config( NULL );
   if ( ( RDA_config_value != ORPGRDA_LEGACY_CONFIG ) &&
        ( RDA_config_value != ORPGRDA_ORDA_CONFIG ) )
   {
      LE_send_msg(GL_ERROR,
         "UMC: Invalid RDA Configuration Value. Resetting to ORPGRDA_ORDA_CONFIG\n" );
      RDA_config_value = ORPGRDA_ORDA_CONFIG;
   }

   if ( RDA_config_value == ORPGRDA_LEGACY_CONFIG )
   {  /* Legacy RDA configuration */
      /*
        Convert the message data based on message type.
      */
      switch(msg_type){
   
         case DIGITAL_RADAR_DATA:
         {
            ret = Basedata_header_convert_to_internal( msg_data );
            break;
         }

         case GENERIC_DIGITAL_RADAR_DATA:
         {
            ret = Generic_Basedata_convert_to_internal( msg_data );
            break;
         }
      
         case RDA_STATUS_DATA:
         {
            ret = RDA_status_msg_convert_to_internal( msg_data );
            break;
         }
   
         case PERFORMANCE_MAINTENANCE_DATA:
         {
            ret = RDA_performance_convert_to_internal( msg_data );
            break;
         }
   
         case CONSOLE_MESSAGE_A2G:
         {
            ret = RPG_console_msg_convert_to_internal( msg_data );
            break;
         }
   
         case CLUTTER_FILTER_BYPASS_MAP:
         {
            RDA_RPG_message_header_t *header =
	       (RDA_RPG_message_header_t *) msg_data; 
            int size; 
            size = header->size - MSG_HDR_SHORTS;
            ret = Clutter_filter_bypass_map_convert_to_internal( msg_data, size );
            break;
         }
   
         case NOTCHWIDTH_MAP_DATA:
         {
            char *nwm = msg_data + sizeof(RDA_RPG_message_header_t);
            ret = Notchwidth_map_convert_to_internal( nwm );
            break;
         }
   
         case RDA_RPG_VCP:
         case RPG_RDA_VCP:
         {
            ret = Vcp_data_convert_to_internal( data );
            break;
         }

         case LOOPBACK_TEST_RDA_RPG:
   
         /*
           We treat the RPG to RDA loopback test as if it were an
           externally generated message.
         */
         case LOOPBACK_TEST_RPG_RDA:
         {
            ret = Loopback_test_message_convert_to_internal( msg_data );
            break;
         }

         /* used by mon_wb */
         case RDA_CONTROL_COMMANDS:
         {
            /* Note: we can use the same function that's used to convert it to
               external format. */
            ret = RDA_control_commands_convert_to_external( data );
            break;
         }
   
         /* used by mon_wb */
         case CLUTTER_SENSOR_ZONES:
         {
            ret = Clutter_censor_zones_convert_to_internal( data );
            break;
         }

         /* used by mon_wb */
         case REQUEST_FOR_DATA:
         {
            /* Note: we can use the same function that's used to convert it to
               external format. */
            ret = Request_for_data_convert_to_external( data );
            break;
         }

         /* used by mon_wb */
         case CONSOLE_MESSAGE_G2A:
         {
            /* Note: we can use the same function that's used to convert it to
               external format. */
            ret = RDA_console_msg_convert_to_external( msg_data );
            break;
         }

         default:
         {
            LE_send_msg( GL_ERROR, 
                         "Unrecognized Message To Convert To Internal.\n" );
            ret = 0;
         }
      }
   }
   else
   {  /* Open RDA configuration */

      switch(msg_type){
   
         case DIGITAL_RADAR_DATA:
         {
	    /* Now convert the data msg */
            ret = ORDA_Basedata_header_convert_to_internal( msg_data );
            break;
         }

         case GENERIC_DIGITAL_RADAR_DATA:
         {
	    /* Now convert the data msg */
            ret = Generic_Basedata_convert_to_internal( msg_data );
            break;
         }
      
         case RDA_STATUS_DATA:
         {
            ret = ORDA_status_msg_convert_to_internal( msg_data );
            break;
         }
   
         case PERFORMANCE_MAINTENANCE_DATA:
         {
            ret = ORDA_performance_convert_to_internal( msg_data );
            break;
         }
   
         case CONSOLE_MESSAGE_A2G:
         {
            ret = RPG_console_msg_convert_to_internal( msg_data );
            break;
         }
   
         case CLUTTER_FILTER_BYPASS_MAP:
         {
            RDA_RPG_message_header_t *header =
	       (RDA_RPG_message_header_t *) msg_data; 
            int size; 
            size = header->size - MSG_HDR_SHORTS;
            ret = ORDA_Clutter_filter_bypass_map_convert_to_internal( msg_data, size );
            break;
         }
   
         case CLUTTER_MAP_DATA:
         {
            RDA_RPG_message_header_t *header = 
                                  (RDA_RPG_message_header_t *) msg_data;
            int size;
   
            size = header->size - MSG_HDR_SHORTS;
            ret = ORDA_Clutter_map_convert_to_internal( msg_data, size );
            break;
         }
   
         case RDA_RPG_VCP:
         case RPG_RDA_VCP:
         {
            ret = Vcp_data_convert_to_internal( data );
            break;
         }

         case LOOPBACK_TEST_RDA_RPG:
   
         /*
           We treat the RPG to RDA looppback test as if it were an
           externally generated message.
         */
         case LOOPBACK_TEST_RPG_RDA:
         {
            ret = Loopback_test_message_convert_to_internal( msg_data );
            break;
         }
   
         case ADAPTATION_DATA:
         {
            int ret = 0;
            if ( (ret = ORDA_adapt_convert_to_internal( data )) < 0)
            {
               LE_send_msg(GL_ERROR,
                  "Error in ORDA_adapt_convert_to_internal.\n");
               return(-1);
            }
            break;
         }

         /* used by mon_wb */
         case RDA_CONTROL_COMMANDS:
         {
            /* Note: we can use the same function that's used to convert it to
               external format. */
            ret = ORDA_control_commands_convert_to_external( data );
            break;
         }
   
         /* used by mon_wb */
         case CLUTTER_SENSOR_ZONES:
         {
            ret = ORDA_Clutter_censor_zones_convert_to_internal( data );
            break;
         }

         /* used by mon_wb */
         case REQUEST_FOR_DATA:
         {
            /* Note: we can use the same function that's used to convert it to
               external format. */
            ret = Request_for_data_convert_to_external( data );
            break;
         }

         /* used by mon_wb */
         case CONSOLE_MESSAGE_G2A:
         {
            /* Note: we can use the same function that's used to convert it to
               external format. */
            ret = RDA_console_msg_convert_to_external( msg_data );
            break;
         }

         default:
         {
            LE_send_msg( GL_ERROR, 
                         "Unrecognized Message To Convert To Internal.\n" );
            ret = 0;
         }
      }
   }
      
   return (ret);

} /* end UMC_RDAtoRPG_message_convert_to_internal */

/********************************************************************

    Description: This function converts n float values to machine
		 dependent format.

    Input:      nooffloats - number of floats to convert
                buf - pointer to the first float to convert

    Output:     buf - buffer containing converted bytes

    Returns:    int - UMC_RDA_SUCCESS on success
                      UMC_RDA_FAILURE on failure
                      these are defined in orpgumc_rda.h

********************************************************************/
int UMC_floats_from_icd (int nooffloats, float* buf)
{
   int ret_val = UMC_RDA_SUCCESS;
   int stat;
   int i;
   float* curval = buf;
   for (i = 0; (i < nooffloats); i++,curval++)
   {
      stat = Float_concurrent_to_ieee754 ( curval );
      if ( stat < 0 )
         ret_val = UMC_RDA_FAILURE;
   }

   return ret_val;

} /* end UMC_floats_from_icd */


/********************************************************************
       
    Description: This function converts n float values to machine
		 independent format.

    Input:      nooffloats - number of floats to convert
                buf - pointer to the first float to convert

    Output:     buf - buffer containing converted bytes

    Returns:    int - UMC_RDA_SUCCESS on success
                      UMC_RDA_FAILURE on failure
                      these values are defined in orpgumc_rda.h

********************************************************************/
int UMC_floats_to_icd (int nooffloats, float* buf)
{ 
   int ret_val = UMC_RDA_SUCCESS;
   int stat;
   int i;
   float* curval = buf;
   for (i = 0; (i < nooffloats); i++, curval++)
   {
      stat = Float_ieee754_to_concurrent ( curval );
      if ( stat < 0 )
         ret_val = UMC_RDA_FAILURE;
   }

   return ret_val;

} /* end UMC_floats_to_icd */


/************************************************************************

    Description: This function converts a Concurrent float to an IEEE
        745 format float. The conversion is done in place.

    Input/output: x - pointer to the number to be converted.

    Return: 0 on success, negative number on failure

    Notes: Since Mc * 2 ** -24 * 16 ** Ec = M * 2 ** -23 * 2 ** E,
        where M is the mantissa and E is the exponent, those with c
        denote the Concurrent format and those without c denote the
        IEEE format, we have E = 4 * Ec - 1 and M = Mc;

************************************************************************/

#define EMAX 127
#define EMIN -126
static int Float_concurrent_to_ieee754 (float* x)
{
    int ret_val = 0;
    unsigned int number;
    int sign;
    int e, ec;
    int m, mc;

    number = *((unsigned int *)x);

    sign = number & 0x80000000;
    ec = ((number & 0x7f000000) >> 24) - 64;
    mc = number & 0xffffff;

    m = mc;
    e = 4 * ec - 1;
    while (m >= 0x1000000) {            /* normalize */
        m /= 2;
        e++;
    }
    if (m != 0) {
        while (m < 0x800000) {
            m *= 2;
            e--;
        }
    }
    else
        e = EMIN - 1;           /* denormalized 0 */

    if (e > EMAX)
        e = EMAX + 1;           /* NAN */
    if (e < EMIN) {
        int shift;

        shift = EMIN - e;
        if (shift >= 32)
            shift = 31;
        m = m >> shift;
        e = EMIN - 1;           /* denormalized */
    }

    *((unsigned int *)x) = sign | ((e + 127) << 23) | (m & 0x7fffff);

    return ret_val;

} /* end Float_concurrent_to_ieee754 */


/************************************************************************

    Description: This function converts an IEEE 745 float to a Concurrent
        format float. The conversion is done in place. If x is NAN, no
        conversion is done.

    Input/output: x - pointer to the number to be converted.

    Returns: 0 on success, negative number on failure

    Notes: We have Ec = k and Mc = M * (2 ** r),
        where E + 1 = 4 * k + r, k is an integer, r >= 0 and r < 4.
        Refer to Float_concurrent_to_ieee754.

************************************************************************/
static int Float_ieee754_to_concurrent (float* x)
{
    int ret_val = 0;
    unsigned int number;
    int sign, r, k;
    int e, ec;
    int m, mc;

    number = *((unsigned int *)x);

    sign = number & 0x80000000;
    e = ((number & 0x7f800000) >> 23) - 127;
    m = (number & 0x7fffff);
    if (e == EMAX + 1)
        return(ret_val);
    if (e >= EMIN)                      /* the implicit bit */
        m |= 0x800000;
    else
        e = EMIN;

    r = (e + 1 + 1000) % 4;             /* always use positive % operation */
    k = (e + 1 - r) / 4;
    ec = k;
    mc = m;
    while (r > 0) {
        mc *= 2;
        r--;
    }

    while (mc != 0 && mc < 0x100000) {          /* normalize */
        mc *= 16;
        ec--;
    }
    while (mc >= 0x1000000) {
        mc /= 16;
        ec++;
    }
       
    *((unsigned int *)x) = sign | ((ec + 64 ) << 24) | mc;

    return ret_val;

} /* end Float_ieee754_to_concurrent */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the RDA_basedata_header struct from 
//      external to internal format.
//	NOTE: it is assumed that the msg hdr portion has already been
//	converted.
//
//   Input:	
//      data - Pointer to basedata radial header (including msg hdr)
//
//   Output:
//      data - Pointer to basedata radial header converted to RPG
//             internal format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Basedata_header_convert_to_internal(char* data)
{	
   RDA_basedata_header* rec = 
      (RDA_basedata_header*) data; /* Pointer to basedata header. */

#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_longs(1, &(rec->time));	
   MISC_swap_shorts(14, &(rec->date));
   MISC_swap_floats(1, &(rec->calib_const));
   MISC_swap_shorts(16, &(rec->ref_ptr));		
   MISC_swap_shorts(16, &(rec->word_43));			

#endif	

   /*
     Convert the calibration constant to internal representation
     of floating point.
   */
   UMC_floats_from_icd(1, &(rec->calib_const));

   return (1);

} /* end Basedata_header_convert_to_internal */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the ORDA_basedata_header struct from 
//      external to internal format.
//	NOTE: it is assumed the msg hdr has already been converted.
//
//   Input:	
//      data - Pointer to basedata radial header (including msg hdr)
//
//   Output:
//      data - Pointer to basedata radial header converted to RPG
//             internal format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Basedata_header_convert_to_internal(char* data)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   ORDA_basedata_header* rec = 
      (ORDA_basedata_header*) data; /* Pointer to basedata header. */

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_longs(1, (long *) &(rec->time));	
   MISC_swap_shorts(14, &(rec->date));
   MISC_swap_floats(1, &(rec->calib_const));
   MISC_swap_shorts(16, &(rec->ref_ptr));		
   MISC_swap_shorts(16, &(rec->word_43));			

#endif	

   /*
     NOTE: Conversion of floating point numbers is not needed because
     the ORPG and the ORDA are both using IEEE format.
   */
   return (1);

} /* end ORDA_Basedata_header_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description:
//      This function converts the ORDA_generic_basedata_t struct 
//      from external to internal format or vice versa.
//      NOTE: it is assumed the msg hdr has already been converted.
//
//   Input:
//      to_external - 1 for to external, 0 for to internal.
//	data - Pointer to generic basedata radial (including 
//             msg hdr)
//
//   Output:
//      data - Pointer to generic basedata radial converted 
//             to RPG internal format.
//
//   Returns:
//      Returns positive value is successful or non-positive error.
//
//////////////////////////////////////////////////////////////////\*/
static int Generic_Basedata_convert (int to_external, char* data)
{

   Generic_basedata_t* rec = (Generic_basedata_t *) data; 
   int no_of_datum, major_version = 1, i;

   no_of_datum = 0;
   if (to_external)
	no_of_datum = rec->base.no_of_datum;

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_longs( 1, (long *) &(rec->base.time) );
   MISC_swap_shorts( 2, (short *) &(rec->base.date) );
   MISC_swap_floats( 1, &(rec->base.azimuth) );
   MISC_swap_shorts( 1, (short *) &(rec->base.radial_length) );
   MISC_swap_floats( 1, &(rec->base.elevation) );
   MISC_swap_shorts( 1, (short *) &(rec->base.no_of_datum) );

   if (!to_external){

	no_of_datum = rec->base.no_of_datum;

       /* 
         Check if the data is compressed.  If so decompress and 
         set the compression flag to 0. 
       */
       if( (rec->base.compress_type == M31_BZIP2_COMP)
                              || 
           (rec->base.compress_type == M31_ZLIB_COMP) ){

          int ret, method, src_len, offset;
          char *src;

          static char *dest = NULL;
          static int dest_len = 0;

          /*
            malloc a temporary buffer to hold the decompressed data. 
          */
          if( (rec->base.radial_length > dest_len)
                              ||
                        (dest == NULL) ){

             dest = realloc( dest, rec->base.radial_length );
             if( dest == NULL ){

                LE_send_msg( GL_INFO, "malloc Failed For %d Bytes.\n", 
                             rec->base.radial_length );
                return(-1);

             }

          }

          /* 
            Set the decompressed size. 
          */
          dest_len = rec->base.radial_length;

          /*
            Determine which compression method was used. 
          */
          if( rec->base.compress_type == MISC_BZIP2 )
             method = MISC_BZIP2;

          else 
             method = MISC_GZIP;

          /* 
            Decompress the data. 
          */
          offset = sizeof(Generic_basedata_t) + no_of_datum*sizeof(int);
          src = data + offset;
          src_len = rec->msg_hdr.size*sizeof(short) - offset;
          ret = MISC_decompress( method, src, src_len, dest, dest_len );
          if( ret < 0 ){

             LE_send_msg( GL_INFO, "MISC_decompress Failed\n" );
             return(-1);

          }

          /* 
            Set the compress_type flag in header to NO COMPRESSION, 
            set the size field in the radial header based on the 
            decompressed size, then copy the decompressed data back
            to the original source buffer.
          */
          rec->base.compress_type = 0;
          rec->msg_hdr.size = (dest_len + 1)/sizeof(short);

          memcpy( src, dest, dest_len );

       }

   }
   
   for( i = 0; i < no_of_datum; i++ ){

      Generic_any_t *data_block;
      char type[5];
      int offset;

      offset = 0;
      if (to_external)
	  offset = rec->base.data[i];
      MISC_swap_longs( 1, (long *) &(rec->base.data[i]) );
      if (!to_external)
	  offset = rec->base.data[i];

      data_block = (Generic_any_t *) 
                 (data + sizeof(RDA_RPG_message_header_t) + offset);

      /* Convert the name to a string so we can do string compares. */
      memset( type, 0, 5 );
      memcpy( type, data_block->name, 4 );

      if( type[0] == 'R' ){

         int len = 0;

         if( strcmp( type, "RRAD" ) == 0 ){

            Generic_rad_t *rad = (Generic_rad_t *) data_block;
            if( to_external )
               len = rad->len;

            MISC_swap_shorts( 2, (short *) &(rad->len) );
            if( !to_external )
               len = rad->len;
            MISC_swap_floats( 2, &(rad->horiz_noise) );
            MISC_swap_shorts( 1, (short *) &(rad->nyquist_vel) );

            if( (major_version == GENERIC_RAD_DBZ0_MAJOR)
                               &&
                (len == (sizeof(Generic_rad_t) + sizeof(Generic_rad_dBZ0_t))) ){

               Generic_rad_dBZ0_t *hdr = 
                      (Generic_rad_dBZ0_t *) ((char *) rad + sizeof(Generic_rad_t) );
               MISC_swap_floats( 2, &(hdr->h_dBZ0) );

            }

         }
         else if( strcmp( type, "RELV" ) == 0 ){

            Generic_elev_t *elev = (Generic_elev_t *) data_block;

            MISC_swap_shorts( 2, (short *) &(elev->len) );
            MISC_swap_floats( 1, &(elev->calib_const) );

         }
         else if( strcmp( type, "RVOL" ) == 0 ){

            Generic_vol_t *vol = (Generic_vol_t *) data_block;

            if( to_external )
               major_version = vol->major_version;
            MISC_swap_shorts( 1, (short *) &(vol->len) );
            MISC_swap_floats( 2, &(vol->lat) );
            MISC_swap_shorts( 2, &(vol->height) );
            MISC_swap_floats( 5, &(vol->calib_const) );
            MISC_swap_shorts( 2, (short *) &(vol->vcp_num) );

            if( !to_external )
               major_version = vol->major_version;

         }
         else
            LE_send_msg( GL_INFO, "Undefined/Unkwown Block Type.\n" );

      }
      else if( type[0] == 'D' ){

         Generic_moment_t *moment = (Generic_moment_t *) data_block;

         MISC_swap_longs( 1, (long *) &(moment->info) );
         MISC_swap_shorts( 4, (short *) &(moment->first_gate_range) );

         /* The number of gates is used for possible byte-swapping the 
            data.  Swap now if to_internal, swap later if to_external. */
         if( !to_external )
            MISC_swap_shorts( 1, (short *) &(moment->no_of_gates) );
         MISC_swap_floats( 2, &(moment->scale) );

         if( moment->data_word_size == (BITS_PER_BYTE*sizeof(short)) )
            MISC_swap_shorts( moment->no_of_gates, (short *) &(moment->gate.u_s[0]) );

         else if( moment->data_word_size == (BITS_PER_BYTE*sizeof(int)) )
            MISC_swap_longs( moment->no_of_gates, (long *) &(moment->gate.u_i[0]) );

         if( moment->info != 0 )
            LE_send_msg( GL_INFO, "Moment Info Pointer Defined But No Moment Info.\n" );

         if( to_external )
            MISC_swap_shorts( 1, (short *) &(moment->no_of_gates) );
      }
      else{

         LE_send_msg( GL_INFO, "Invalid Data Block Type: %c\n", type[0] );
         return(-1);

      }

   }

   return (1);

} /* End of ORDA_Basedata_header_convert_to_internal */


/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts an rda_status_msg struct from external 
//      to internal format.
//	NOTE: it is assumed the msg hdr has already been converted.
//
//   Input:
//      buf - Pointer to RDA status data structure to convert (includ msg hdr)
//
//   Output:
//      buf - Pointer to converted RDA status data structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_status_msg_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   RDA_status_msg_t* rda_status_msg = 
      (RDA_status_msg_t*) buf;  /* Pointer to rda_status_msg structure. */

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(22, (short *) &(rda_status_msg->rda_status));
   MISC_swap_shorts(4, &(rda_status_msg->spare23));
   MISC_swap_shorts(MAX_RDA_ALARMS_PER_MESSAGE,
                     &(rda_status_msg->alarm_code[0]));

#endif

   return (1);

} /* end RDA_status_msg_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a ORDA status msg struct from external 
//      to internal format.
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:
//      buf - Pointer to ORDA status data structure to convert
//	      (including msg hdr).
//
//   Output:
//      buf - Pointer to converted ORDA status data structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_status_msg_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   ORDA_status_msg_t* rda_status_msg = 
      (ORDA_status_msg_t*) buf;  /* Pointer to rda_status_msg structure. */

   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */
   MISC_swap_shorts(22, (short *) &(rda_status_msg->rda_status));
   MISC_swap_shorts(4, &(rda_status_msg->vc_ref_calib_corr));
   MISC_swap_shorts(MAX_RDA_ALARMS_PER_MESSAGE,
                     &(rda_status_msg->alarm_code[0]));

#endif

   return (1);

} /* end ORDA_status_msg_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a console message from external 
//      format to internal format.
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:
//      buf - Pointer to console message to convert (including msg hdr)
//
//   Output:
//      buf - Pointer to converted console message.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int RPG_console_msg_convert_to_internal(char* buf)
{
   /*
     If this is a Little-Endian machine, perform bytes swapping.   
   */

#ifdef LITTLE_ENDIAN_MACHINE	
   
   RDA_RPG_console_message_t* console_msg = (RDA_RPG_console_message_t *)buf;

   MISC_swap_shorts(1, &(console_msg->size));

#endif

   return (1);

} /* end RPG_console_msg_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts VCP message from external 
//      format to internal format or vice versa.
//	NOTE: the msg hdr has been removed.
//
//   Input:
//	to_external - 1 for to external; 0 for to internal.
//      buf - Pointer to VCP message to convert (excluding msg hdr).
//
//   Output:
//      buf - Pointer to converted VCP message.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int Vcp_data_convert(int to_external, char* buf)
{
   int n_cuts, i;

   VCP_elevation_cut_header_t *vcp_hd;
   
   MISC_swap_shorts (sizeof (VCP_message_header_t) / sizeof (short), (short *)buf);
   vcp_hd = &(((VCP_ICD_msg_t *)buf)->vcp_elev_data);
   n_cuts = 0;
   if (to_external)
      n_cuts = vcp_hd->number_cuts;
   MISC_swap_shorts (8, (short *)vcp_hd);
   MISC_swap_shorts (1, (short *)vcp_hd + 2);
   if (!to_external)
      n_cuts = vcp_hd->number_cuts;
   
   for (i = 0; i < n_cuts; i++) {
       VCP_elevation_cut_data_t *h;
   
       h = &(vcp_hd->data[i]);
       MISC_swap_shorts (sizeof (VCP_elevation_cut_data_t) / sizeof (short), (short *)h);
       MISC_swap_shorts (2, (short *)h + 1);
   }	      

   return (1);

} /* end Vcp_data_convert */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts a loopback message from external 
//      format to internal format.
//	NOTE: it is assumed the msg hdr has already been converted to
//	internal format.
//
//   Input:
//      buf - Pointer to loopback_msg_t structure to convert
//	      (including msg hdr).
//
//   Output:
//      buf - Pointer to converted loopback_msg_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error 
//      number on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int Loopback_test_message_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	
   int msg_size_shorts;
   RDA_RPG_message_header_t* msg_hdr = (RDA_RPG_message_header_t *)buf;
   RDA_RPG_loop_back_message_t* loopback_msg =
      (RDA_RPG_loop_back_message_t*)(buf + MSG_HDR_BYTES);

   msg_size_shorts = msg_hdr->size - MSG_HDR_SHORTS;
   MISC_swap_shorts(msg_size_shorts, &(loopback_msg->size));

#endif

   return (1);

} /* end Loopback_test_message_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the bypass map structure from 
//      external to internal format.
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:
//      buf - Pointer to RDA_bypass_map_msg_t structure to convert
//	      (including msg hdr).
//      seg_size - Number of shorts in this message segment.
//
//   Output:
//      buf - Pointer to converted RDA_bypass_map_msg_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Clutter_filter_bypass_map_convert_to_internal(char* buf,
                                                      int seg_size )
{
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */

#ifdef LITTLE_ENDIAN_MACHINE

   MISC_swap_shorts(seg_size, (short *)(buf + MSG_HDR_BYTES));

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts the bypass map structure from 
//      external to internal format.
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:
//      buf - Pointer to ORDA_bypass_map_msg_t structure to convert
//	      (including msg hdr).
//      seg_size - Number of shorts in this message segment (not incl
//                  msg hdr).
//
//   Output:
//      buf - Pointer to converted ORDA_bypass_map_msg_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Clutter_filter_bypass_map_convert_to_internal(char* buf,
                                                      int seg_size )
{
   /*
     If this is a Little-Endian machine, perform byte swapping.
   */

#ifdef LITTLE_ENDIAN_MACHINE

   MISC_swap_shorts(seg_size, (short *) (buf + MSG_HDR_BYTES));

#endif	      

   return (1);
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts RDA_notch_map_t structure from 
//      external to internal format.
//	NOTE: it is assumed the msg hdr has already been converted.
//
//   Input:
//      buf - Pointer to RDA_notch_map_t structure to convert
//            (including msg hdr).
//
//   Output:
//      buf - Pointer to converted RDA_notch_map_t structure.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Notchwidth_map_convert_to_internal( char* buf )
{
	
#ifdef LITTLE_ENDIAN_MACHINE

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts( 2, (short*)buf );

#endif	      

   return (1);
} /* end Notchwidth_map_convert_to_internal */

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//
//   Description: 
//      This function converts ORDA Clutter Filter map data from 
//      external to internal format.
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:
//      buf - Pointer to ORDA Clutter Filter map data to convert
//	      (including msg hdr)
//      seg_size - Number of shorts in this message segment.
//
//   Output:
//      buf - Pointer to converted data.
//
//   Returns:
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_Clutter_map_convert_to_internal(char* buf, int seg_size)
{
	
#ifdef LITTLE_ENDIAN_MACHINE

   /*
     If this is a Little-Endian machine, perform byte swapping.
   */
   MISC_swap_shorts(seg_size, (short*)(buf + MSG_HDR_BYTES));

#endif	      

   return (1);
} /* end ORDA_Clutter_map_convert_to_internal */

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

   wideband_t* wideband = (wideband_t*)buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(1, &(wideband->dcu_status));
   MISC_swap_shorts(1, &(wideband->general_error));
   MISC_swap_shorts(1, &(wideband->svc_15_error));
   MISC_swap_shorts(1, (short *) &(wideband->outgoing_frames));
   MISC_swap_shorts(1, (short *) &(wideband->frames_fcs_errors));
   MISC_swap_shorts(1, (short *) &(wideband->retrans_i_frames));
   MISC_swap_shorts(1, (short *) &(wideband->polls_sent_recvd));
   MISC_swap_shorts(1, &(wideband->poll_timeout_exp));
   MISC_swap_shorts(1, (short *) &(wideband->min_buf_read_pool));
   MISC_swap_shorts(1, (short *) &(wideband->max_buf_read_done));
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
   MISC_swap_floats(7, &(bitdata->ant_pk_pwr));
   MISC_swap_longs(1, (long *) &(bitdata->xmtr_rec_cnt));		
   MISC_swap_shorts(2, &(bitdata->spare139));
   MISC_swap_floats(5, &(bitdata->sh_pulse_lin_chan_noise));
   MISC_swap_shorts(2, &(bitdata->idu_tst_detect));	

#endif	

   /*
     Convert floats to RPG internal IEEE format.
   */
   UMC_floats_from_icd(7, &(bitdata->ant_pk_pwr));		
   UMC_floats_from_icd(5, &(bitdata->sh_pulse_lin_chan_noise));			

   return (1);

} /* end Bitdata_convert_to_internal */

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

#ifdef LITTLE_ENDIAN_MACHINE	

   /*
     If this machine Little-Endian, perform byte swapping.
   */

   MISC_swap_floats(14, &(calibration->agc_step1_amp));
   MISC_swap_shorts(20, &(calibration->spare181));	
   MISC_swap_floats(36, &(calibration->cw_lin_tgt_exp_amp));
   MISC_swap_shorts(18, &(calibration->spare273));	
   MISC_swap_floats(12, &(calibration->kd1_lin_tgt_exp_amp));
   MISC_swap_shorts(16, &(calibration->spare315));							

#endif	

   /*
     Convert floats to RPG internal IEEE format.
   */
   UMC_floats_from_icd(14, &(calibration->agc_step1_amp));
   UMC_floats_from_icd(16, &(calibration->phase_ram1_exp_vel));	
   UMC_floats_from_icd(16, &(calibration->cw_lin_tgt_exp_amp));
   UMC_floats_from_icd(4, &(calibration->short_pulse_lin_syscal));	
   UMC_floats_from_icd(12, &(calibration->kd1_lin_tgt_exp_amp));

   return (1);

} /* end Calibration_convert_to_internal */

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

#ifdef LITTLE_ENDIAN_MACHINE

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_floats(4, &(clutter_check->unfilt_lin_chan_pwr));
   MISC_swap_shorts(22, &(clutter_check->spare339));

#endif	

   /*
     Convert floats to RPG internal format.
   */
   UMC_floats_from_icd(4, &(clutter_check->unfilt_lin_chan_pwr));

   return (1);

} /* end Clutter_check_convert_to_internal */

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
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:	
//      buf - Pointer to rda_performance_t structure to convert
//	      (including msg hdr).
//
//   Output:
//      buf - Pointer to converted rda_performance_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number
//
//////////////////////////////////////////////////////////////////\*/
static int RDA_performance_convert_to_internal(char* buf)
{
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
} /* end RDA_performance_convert_to_internal() */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Communications from external 
//      to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Comms_convert_to_internal( char* buf )
{

#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *comms = (Pmd_t *) buf;
   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(2, &(comms->spare1));	
   MISC_swap_longs( 4, (long *) &(comms->t1_output_frames));		
   MISC_swap_shorts(2, &(comms->router_mem_util));	
   MISC_swap_longs(15, (long *) &(comms->csu_loss_of_signal));		
   MISC_swap_shorts(2, &(comms->lan_switch_mem_util));	
   MISC_swap_longs( 4, (long *) &(comms->ntp_rejected_packets));		
   MISC_swap_shorts(5, &(comms->ipc_status));	

#endif	

   return (1);

} /* end Comms_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Power from external 
//      to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Power_convert_to_internal( char* buf )
{
#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *power = (Pmd_t *) buf; /* Pointer to power_t struct */

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_longs( 2, (long *) &(power->ups_batt_status));		
   MISC_swap_floats( 5, (float *) &(power->ups_batt_temp));		
   MISC_swap_shorts(24, (short *) &(power->spare113));	

#endif	

   return (1);

} /* end Power_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Transmitter from external 
//      to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Transmitter_convert_to_internal( char* buf )
{

#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *xmtr = (Pmd_t *) buf; /* Pointer to xmtr struct */

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(68, &(xmtr->p_5vdc_ps));	
   MISC_swap_floats(5, &(xmtr->trsmttr_rf_pwr));
   MISC_swap_shorts(2, (short *) &(xmtr->xmtr_pwr_mtr_zero));	
   MISC_swap_longs( 1, (long *) &(xmtr->xmtr_recycle_cnt));		
   MISC_swap_floats(2, &(xmtr->receiver_bias));
   MISC_swap_shorts(6, (short *) &(xmtr->spare223));	

#endif	

   return (1);

} /* end Transmitter_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Tower/Utilities from external 
//      to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted tower_utilities_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Tower_utilities_convert_to_internal(char* buf)
{
#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *tower = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(21, &(tower->ac_1_cmprsr_shut_off));	

#endif	

   return (1);

} /* end Tower_utilities_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Equipment/Shelter from
//	external to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Equipment_shelter_convert_to_internal(char* buf)
{
#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *equip = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(11, &(equip->equip_shltr_fire_sys));	
   MISC_swap_floats( 11, &(equip->equip_shltr_temp));
   MISC_swap_shorts(8, &(equip->cnvrtd_gnrtr_fuel_lvl));	

#endif	

   return (1);

} /* end Equipment_shelter_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Antenna/Pedestal from
//	external to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Antenna_pedestal_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *ant_ped = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_floats( 5, &(ant_ped->pdstl_p_28v_ps));
   MISC_swap_shorts(40, &(ant_ped->p_150v_ovrvlt));	

#endif	

   return (1);

} /* end Antenna_pedestal_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Receiver from
//	external to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Rf_gnrtr_rcvr_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *rf_gen_rec = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(10, &(rf_gen_rec->coho_clock));	
   MISC_swap_floats( 6, &(rf_gen_rec->h_shrt_pulse_noise));

#endif	

   return (1);

} /* end Rf_gnrtr_rcvr_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Calibration from
//	external to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Calib_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *cal = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_floats( 5, &(cal->h_linearity));
   MISC_swap_shorts( 2, &(cal->spare373[0]));	
   MISC_swap_floats( 2, &(cal->shrt_pls_h_dbz0));
   MISC_swap_shorts( 4, &(cal->velocity_prcssd));	
   MISC_swap_floats( 5, &(cal->h_i_naught));
   MISC_swap_shorts( 4, (short *) &(cal->spare393[0]));	
   MISC_swap_floats( 3, &(cal->h_pwr_sense));
   MISC_swap_shorts( 6, (short *) &(cal->spare385[0]));	
   MISC_swap_floats( 5, &(cal->h_cltr_supp_delta));
   MISC_swap_shorts( 6, (short *) &(cal->spare419[0]));	
   MISC_swap_floats( 1, &(cal->v_linearity));
   MISC_swap_shorts( 4, (short *) &(cal->spare427[0]));	

#endif	

   return (1);

} /* end Calibration_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD File Status from
//	external to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int File_status_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *file_stat = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(30, &(file_stat->state_file_rd_stat));	

#endif	

   return (1);

} /* end File_status_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD Device Status from
//	external to internal or from internal to external.
//    
//   Input:
//	buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//	buf - Pointer to converted Pmd_t structure.
//   	
//
//   Returns:	
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int Device_status_convert_to_internal(char* buf)
{
#ifdef LITTLE_ENDIAN_MACHINE	

   Pmd_t *device_stat = (Pmd_t *) buf; 

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(8, &(device_stat->dau_comm_stat));
   MISC_swap_longs(1, &(device_stat->perf_check_time));
   MISC_swap_shorts(10, &(device_stat->spare471[0]));

#endif	

   return (1);

} /* end Device_status_convert_to_internal */

/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts a PMD AME from external to 
//      internal or from internal to external.
//    
//   Input:
//      buf - Pointer to Pmd_t structure to convert.
//
//   Output:
//      buf - Pointer to converted Pmd_t structure.
//      
//
//   Returns:   
//      Positive number on success or a non-positive error number.
//
//////////////////////////////////////////////////////////////////\*/
static int AME_convert_to_internal(char* buf)
{
#ifdef LITTLE_ENDIAN_MACHINE    

   Pmd_t *ame = (Pmd_t *) buf;

   /*
     If this machine Little-Endian, perform byte swapping.
   */
   MISC_swap_shorts(1, (short *) &(ame->polarization));
   MISC_swap_floats(3, &(ame->internal_temp));
   MISC_swap_shorts(4, (short *) &(ame->peltier_pulse_width_modulation));
   MISC_swap_floats(8, &(ame->p_33vdc_ps));
   MISC_swap_shorts(2, (short *) &(ame->mode));
   MISC_swap_floats(6, &(ame->peltier_inside_fan_current));

#endif

   return (1);

} /* end AME_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts an orda_performance struct from external 
//      machine independent format to an internal machine dependent 
//      format.
//	NOTE: it is assumed the msg hdr portion has already been
//	converted.
//
//   Input:	
//      buf - Pointer to orda_performance_t structure to convert
//	      (including msg hdr).
//
//   Output:
//      buf - Pointer to converted orda_performance_t structure.
//
//   Returns:	
//      Positive number on success or a non-positive error number
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_performance_convert_to_internal(char* buf)
{
   /* Pointer to orda_performance_t structure. */
   orda_pmd_t* orda_performance = (orda_pmd_t*) buf; 

   /*
     Perform necessary byte swapping. 
   */

   Comms_convert_to_internal( (char*) &orda_performance->pmd );
   AME_convert_to_internal( (char*) &orda_performance->pmd );
   Power_convert_to_internal( (char*) &orda_performance->pmd);
   Transmitter_convert_to_internal( (char*) &orda_performance->pmd );
   Tower_utilities_convert_to_internal( (char*) &orda_performance->pmd );
   Equipment_shelter_convert_to_internal( (char*) &orda_performance->pmd );
   Antenna_pedestal_convert_to_internal( (char*) &orda_performance->pmd );
   Rf_gnrtr_rcvr_convert_to_internal( (char*) &orda_performance->pmd );
   Calib_convert_to_internal( (char*) &orda_performance->pmd );
   File_status_convert_to_internal( (char*) &orda_performance->pmd );
   Device_status_convert_to_internal( (char*) &orda_performance->pmd );

   return (1);

} /* end ORDA_performance_convert_to_internal */


/*///////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function converts an ORDA Adaptation data msg from external 
//      machine independent format to an internal machine dependent 
//      format.
//	NOTE: The msg hdr is not included.
//
//   Input:	
//      buf - Pointer to ORDA Adapt data msg to convert
//
//   Output:
//      buf - Pointer to converted ORDA_adpt_data_t structure.
//
//   Returns:	
//      0 on success or -1 on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_adapt_convert_to_internal(char* buf)
{

#ifdef LITTLE_ENDIAN_MACHINE	

   ORDA_adpt_data_t* orda_adapt;

   if (buf == NULL)
   {
      LE_send_msg(GL_ERROR, "orpgumc_rda: error converting rda adapt data.\n");
      return (-1);
   }

   /* Pointer to orda_adpt_data_t structure. */
   orda_adapt = (ORDA_adpt_data_t*) buf; 

   /*
     Perform necessary byte swapping. 
   */
   MISC_swap_floats( 6, (float*)&(orda_adapt->k1));
   MISC_swap_floats( 11, (float*)&(orda_adapt->a_fuel_conv));
   MISC_swap_floats( 16, (float*)&(orda_adapt->a_min_shelter_temp));
   MISC_swap_longs( 5, (long*)&(orda_adapt->a_hvdl_tst_int));	
   MISC_swap_floats( 1, (float*)&(orda_adapt->a_low_fuel_level));
   MISC_swap_longs( 1, (long*)&(orda_adapt->config_chan_number));	
   MISC_swap_floats( 1, (float*)&(orda_adapt->spare_220));
   MISC_swap_longs( 1, (long*)&(orda_adapt->redundant_chan_config));	
   MISC_swap_floats( 104, (float*)&(orda_adapt->atten_table));
   MISC_swap_floats( 71, (float*)&(orda_adapt->path_losses));
   MISC_swap_floats( 3, (float*)&(orda_adapt->log_amp_factor));
   MISC_swap_floats( 13, (float*)&(orda_adapt->rnscale));
   MISC_swap_floats( 13, (float*)&(orda_adapt->atmos));
   MISC_swap_floats( 12, (float*)&(orda_adapt->el_index));
   MISC_swap_longs( 1, (long*)&(orda_adapt->tfreq_mhz));	
   MISC_swap_floats( 39, (float*)&(orda_adapt->base_data_tcn));
   MISC_swap_longs( 8, (long*)&(orda_adapt->deltaprf));	
   MISC_swap_floats( 3, (float*)&(orda_adapt->seg1lim));
   MISC_swap_longs( 5, (long*)&(orda_adapt->phi_clutter_range));	
   MISC_swap_longs( 1, (long*)&(orda_adapt->vc_ndx));	
   MISC_swap_floats( 2, (float*)&(orda_adapt->az_correction_factor));
   MISC_swap_longs( 6, (long*)&(orda_adapt->ant_manual_setup_ielmin));	
   MISC_swap_longs( 75, (long*)&(orda_adapt->spare8496));	
   MISC_swap_longs( 1, (long*)&(orda_adapt->rvp8NV_iwaveguide_length));	
   MISC_swap_longs( 11, (long*)&(orda_adapt->v_rnscale[0]));	
   MISC_swap_floats( 2, (float*)&(orda_adapt->vel_data_tover));
   MISC_swap_longs( 3, (long*)&(orda_adapt->spare_8752));	
   MISC_swap_floats( 1, (float*)&(orda_adapt->doppler_range_start));
   MISC_swap_longs( 1, (long*)&(orda_adapt->max_el_index));	
   MISC_swap_floats( 3, (float*)&(orda_adapt->seg2lim));
   MISC_swap_longs( 1, (long*)&(orda_adapt->nbr_el_segments));	
   MISC_swap_floats( 5, (float*)&(orda_adapt->h_noise_long));
   MISC_swap_floats( 21, (float*)&(orda_adapt->v_noise_tolerance));
   MISC_swap_longs( 13, (long*)&(orda_adapt->spare_8808));	
   MISC_swap_floats( 7, (float*)&(orda_adapt->ame_ps_tolerance));
   MISC_swap_longs( 1, (long*)&(orda_adapt->default_polarization));	
   MISC_swap_floats( 5, (float*)&(orda_adapt->h_tr_limit_degraded_lim));
   MISC_swap_longs( 2, (long*)&(orda_adapt->h_only_polarization));	
   MISC_swap_longs( 2, (long*)&(orda_adapt->spare_8809));	
   MISC_swap_floats( 1, (float*)&(orda_adapt->reflector_bias));	
   MISC_swap_longs( 109, (long*)&(orda_adapt->spare_8810));	

#endif	

   return (1);
} /* end ORDA_adapt_convert_to_internal */

/******************************************************************

    Reads the radar location for "radar_id" from file "change_radar.dat"
    in the configuration dir. If failed, the value of 0 is used.

******************************************************************/

#include <math.h>

static void Get_radar_location (char *radar_id, 
				float *lat, float *lon, short *height) {
    char name[256], *fname;
    FILE *fd;
    int done;

    done = 0;
    fname = "change_radar.dat";
    if (radar_id[0] != '\0' &&
	MISC_get_cfg_dir (name, 256) >= 0 &&
	strlen (name) + strlen (fname) + 2 < 256 &&
	strcat (name, "/") &&
	strcat (name, fname) &&
	(fd = fopen (name, "r")) >= 0) {
	char buf[256], tk[64];

	while (fgets (buf, 256, fd) != NULL) {
	    buf[255] = '\0';
	    int ht;
	    if (MISC_get_token (buf, "S,", 0, tk, 64) > 0 &&
		strcasecmp (tk, radar_id) == 0 &&
		MISC_get_token (buf, "S,Cf", 1, lat, 0) > 0 &&
		MISC_get_token (buf, "S,Cf", 2, lon, 0) > 0 &&
		MISC_get_token (buf, "S,Ci", 3, &ht, 0) > 0) {
		*lat = *lat * .001f;
		*lon = *lon * .001f;
		if (ht < 0)
		    *height = -((int) ((double)(-ht) * FT_TO_M + .5));
		else
		    *height = (int) ((double)ht * FT_TO_M + .5);
		done = 1;
		break;
	    }
	}
	fclose (fd);
    }
    if (!done) {
	*lat = *lon = 0.f;
	*height = 0;
    }
}

/******************************************************************

    Fills out the moment structure "hd" with values of the other
    parameters. "data" is the pointer to the array of bins.

******************************************************************/

static void Fill_moment (Generic_moment_t *hd, char *name, 
		unsigned short no_of_gates, short first_gate_range, 
		short bin_size, short tover, short SNR_threshold, 
		float scale, float offset, char *data) {

    strcpy (hd->name, name);
    hd->info = 0;
    hd->no_of_gates = no_of_gates;
    hd->first_gate_range = first_gate_range;
    hd->bin_size = bin_size;
    hd->tover = tover;
    hd->SNR_threshold = SNR_threshold;
    hd->control_flag = 0;
    hd->data_word_size = 8;
    hd->scale = scale;
    hd->offset = offset;
    memcpy (hd->gate.b, data, no_of_gates);
}

/******************************************************************

    Converts message 1 radial data "msg_header" to message 31. The
    pointer to the message 31 is returned. Both input message 1 and
    output message 31 are in ICD byte order. The buffer for the message
    31 is allocated and reused in this function. The caller should not
    try to free it. "radar_id" is the 4-letter radar ID.

    Note: Message 1 is converted to internal byte order to perform 
          the message conversion.  Message 1 is not converted back 
          to ICD byte order.
******************************************************************/

char *UMC_convert_to_31 (RDA_RPG_message_header_t *msg_header, char *radar_id) {
    static int buf_size = 0;
    static char *buf = NULL;
    ORDA_basedata_header *d_hd;			/* message 1 data hd */
    RDA_RPG_message_header_t *msg_hd;
    Generic_basedata_header_t *gbhd;
    Generic_vol_t *vhd;
    Generic_elev_t *ehd;
    Generic_rad_t *rhd;
    Generic_moment_t *refhd, *velhd, *spwhd;
    int n_bins, size, cnt, i;

    d_hd = (ORDA_basedata_header *)msg_header;
    UMC_RDAtoRPG_message_header_convert ((char *)d_hd);
    ORDA_Basedata_header_convert_to_internal ((char *)d_hd);

    /* allocate/reallocate the buffer for the message 31 radial */
    n_bins = d_hd->n_surv_bins;
    if (d_hd->n_dop_bins > n_bins)
	n_bins = d_hd->n_dop_bins;
    if (n_bins > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = MISC_malloc (sizeof (RDA_RPG_message_header_t) + 
			sizeof (Generic_basedata_header_t) + 64 +
			sizeof (Generic_vol_t) + 
			sizeof (Generic_elev_t) + 
			sizeof (Generic_rad_t) + 
			3 * (sizeof (Generic_moment_t) + n_bins + 4));
	buf_size = n_bins;
    }

    /* Fill out the message header */
    memcpy (buf, msg_header, sizeof (RDA_RPG_message_header_t));
    msg_hd = (RDA_RPG_message_header_t *)buf;
    msg_hd->type = GENERIC_DIGITAL_RADAR_DATA;
    msg_hd->num_segs = 1;
    msg_hd->seg_num = 1;

    /* Fill out the generic basedata header */
    gbhd = (Generic_basedata_header_t *)
				(buf + sizeof (RDA_RPG_message_header_t));
    strncpy (gbhd->radar_id, radar_id, 4);
    for (i = strlen (radar_id); i < 4; i++)
	gbhd->radar_id[i] = ' ';
    gbhd->time = d_hd->time;
    gbhd->date = d_hd->date;
    gbhd->azi_num = d_hd->azi_num;
    gbhd->azimuth = ORPGVCP_ICD_angle_to_deg (ORPGVCP_AZIMUTH_ANGLE, 
						d_hd->azimuth);
    gbhd->compress_type = 0;
    gbhd->azimuth_res = ONE_DEGREE_AZM;
    gbhd->status = d_hd->status;
    gbhd->elev_num = d_hd->elev_num;
    gbhd->sector_num = d_hd->sector_num;
    gbhd->elevation = ORPGVCP_ICD_angle_to_deg (ORPGVCP_ELEVATION_ANGLE, 
						d_hd->elevation);
    gbhd->spot_blank_flag = d_hd->spot_blank_flag;
    gbhd->azimuth_index = 0;

    /* sets pointers to other headers */
    size = ALIGNED_SIZE (sizeof (Generic_basedata_header_t) + 6 * sizeof (int));
    gbhd->data[0] = size;
    vhd = (Generic_vol_t *)((char *)gbhd + size);
    size += ALIGNED_SIZE (sizeof (Generic_vol_t));
    gbhd->data[1] = size;
    ehd = (Generic_elev_t *)((char *)gbhd + size);
    size += ALIGNED_SIZE (sizeof (Generic_elev_t));
    gbhd->data[2] = size;
    rhd = (Generic_rad_t *)((char *)gbhd + size);
    size += ALIGNED_SIZE (sizeof (Generic_rad_t));
    cnt = 3;
    refhd = velhd = spwhd = NULL;
    if (d_hd->ref_ptr > 0) {
	gbhd->data[cnt] = size;
	refhd = (Generic_moment_t *)((char *)gbhd + size);
	size += ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						d_hd->n_surv_bins);
	cnt++;
    }
    if (d_hd->vel_ptr > 0) {
	gbhd->data[cnt] = size;
	velhd = (Generic_moment_t *)((char *)gbhd + size);
	size += ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						d_hd->n_dop_bins);

        cnt++;
    }
    if (d_hd->spw_ptr > 0) {
	gbhd->data[cnt] = size;
	spwhd = (Generic_moment_t *)((char *)gbhd + size);
	size += ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						d_hd->n_dop_bins);
	cnt++;
    }
    gbhd->no_of_datum = cnt;
    msg_hd->size = (size + sizeof(RDA_RPG_message_header_t)) / sizeof (short);
    gbhd->radial_length = size;

    /* Fill out the volume header */
    strcpy (vhd->type, "RVOL");
    vhd->len = sizeof (Generic_vol_t);
    vhd->major_version = 1;
    vhd->minor_version = 0;
    Get_radar_location (radar_id, &(vhd->lat), &(vhd->lon), &(vhd->height));
    vhd->feedhorn_height = vhd->height;
    vhd->calib_const = d_hd->calib_const;
    vhd->horiz_shv_tx_power = 750.f;
    vhd->vert_shv_tx_power = 0.f;
    vhd->sys_diff_refl = 0.f;
    vhd->sys_diff_phase = 0.f;
    vhd->vcp_num = d_hd->vcp_num;
    vhd->sig_proc_states = 0;
    
    /* Fill out the elevation header */
    strcpy (ehd->type, "RELV");
    ehd->len = sizeof (Generic_elev_t);
    ehd->atmos = d_hd->atmos_atten;
    ehd->calib_const = d_hd->calib_const;
    
    /* Fill out the radial header */
    strcpy (rhd->type, "RRAD");
    rhd->len = sizeof (Generic_rad_t);
    rhd->unamb_range = d_hd->unamb_range;
    rhd->horiz_noise = -60.f;
    rhd->vert_noise = -60.f;
    rhd->nyquist_vel = d_hd->nyquist_vel;
    rhd->spare = 0;

    /* Fill out the moments */
    if (refhd)
	Fill_moment (refhd, "DREF", d_hd->n_surv_bins, d_hd->surv_range,
		     d_hd->surv_bin_size, d_hd->threshold_param, 8 * 2,
		     2.f, 66.f, 
	(char *)d_hd + sizeof (RDA_RPG_message_header_t) + d_hd->ref_ptr);
    if (velhd) {
	float scale;

	if (d_hd->vel_resolution == 2)
	    scale = 2.f;
	else
	    scale = 1.f;
	Fill_moment (velhd, "DVEL", d_hd->n_dop_bins, d_hd->dop_range,
		     d_hd->dop_bin_size, d_hd->threshold_param, 8 * 3,
		     scale, 129.f, 
	(char *)d_hd + sizeof (RDA_RPG_message_header_t) + d_hd->vel_ptr);


    }
    if(spwhd) {
	Fill_moment (spwhd, "DSW ", d_hd->n_dop_bins, d_hd->dop_range,
		     d_hd->dop_bin_size, d_hd->threshold_param, 8 * 3,
		     2.f, 129.f, 
	(char *)d_hd + sizeof (RDA_RPG_message_header_t) + d_hd->spw_ptr);
    }

    /* byte swap */
    UMC_RDAtoRPG_message_header_convert ((char *)buf);
    Generic_Basedata_convert_to_external ((char *)buf);

    return (buf);
}

/*///////////////////////////////////////////////////////////////////
//
//   Description:
//      This function converts an ORDA Adaptation data msg from internal
//      machine independent format to an external machine dependent
//      format.
//      NOTE: The msg hdr is not included.
//
//   Input:
//      buf - Pointer to ORDA Adapt data msg to convert
//
//   Output:
//      buf - Pointer to converted ORDA_adpt_data_t structure.
//
//   Returns:
//      0 on success or -1 on failure.
//
//////////////////////////////////////////////////////////////////\*/
static int ORDA_adapt_convert_to_external(char* buf)
{
   int ret = 0;

   ret = ORDA_adapt_convert_to_internal(buf);

   return (ret);
} /* end ORDA_adapt_convert_to_external */
