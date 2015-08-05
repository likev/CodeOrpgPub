/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/02/15 22:20:47 $
 * $Id: crda_receive_wb_data.c,v 1.58 2011/02/15 22:20:47 steves Exp $
 * $Revision: 1.58 $
 * $State: Exp $
 */

#include <crda_control_rda.h>
#include <lb.h>
#include <basedata.h>
#include <generic_basedata.h>
#include <a309.h> 
#include <orpgevt.h>
#include <rda_rpg_message_header.h>

/* Status buffer for reporting errors and general status. */
static char status_buffer[ 256 ];

/* Current elevation index. */
static int Elev_number;

/* Local functions. */
static void Process_radial( int messasge_type, short *rda_data );
static int Process_rda_radial( int message_type, char *rda_data );
static int Process_orda_radial( int message_type, char *orda_data );
static void Convert_time( unsigned long timevalue, char *time_str );
static void To_ASCII( int value, char *string );


/*\/////////////////////////////////////////////////////////////
//
//   Description:  
//      Process the incoming message.  The appropriate message
//      processor is called based on the message type.
//
//      If the message type is DIGITAL_RADAR_DATA, CR_msg_processed
//      is incremented.  If the communication discontinuity alarm
//      is active, then it is cancelled.
//
//      If the message type is RDA_STATUS and the user requested the
//      status data (i.e., the status data was solicited), then 
//      report to the system status log the data is available.
//
//      If the message type is LOOPBACK_TEST_RPG/RDA, the
//      MALRM_LB_TEST_TIMEOUT alarm is cancelled.  The loopback is
//      validated to ensure correctness.  If not valid, the
//      CR_loopback_failure flag is set.  If the loopback periodic
//      rate is non-zero, schedule another loopback test.
//      
//
//   Inputs:  
//      RDA_to_RPG_msg - Pointer to incoming RDA message.
//      format - Indicator for ORPG message format (i.e,
//      message contains CM_resp_struct and CTM header)
//      or legacy format.
//
//   Outputs:
//
//   Returns:   
//      Currently undefined.
//   
//   Globals:
//      CR_msg_processed - see crda_control_rda.h
//      CR_verbose_mode - see crda_control_rda.h
//      CR_loopback_failure - see crda_control_rda.h
//      CR_loopback_periodic_rate - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////\*/
int RW_receive_wideband_data( int *RDA_to_RPG_msg, short format ){

   int				ret			= 0;
   int				message_seg_size	= 0;
   int 				message_type		= 0;
   short*			rda_data		= NULL;
   static char*			msg			= NULL;
   RDA_RPG_message_header_t*	msg_header		= NULL;
  

   /* Format is used to determine if data is received from comm manager
      or not.   Comm manager data has response structure prepended to 
      message ... we wish to discard this structure. */
   if ( format == 0 ){

      /* Data received from comm manager.  Strip off response structure. */
      msg = (char *) RDA_to_RPG_msg;
      msg += (int) sizeof( CM_resp_struct );
      rda_data = (short *) msg;

      /* Adjust data pointer by CTM header length in halfwords. */
      rda_data += CTM_HEADER_LENGTH;
   }
   else
      rda_data = (short *) RDA_to_RPG_msg;

   /* Convert the message header from external format to internal format 
      (i.e., if the RDA is Big-Endian and the RPG is Little-Endian (or 
      vice versa). */
   UMC_RDAtoRPG_message_header_convert( (char *) rda_data ); 

   /* Extract message size and strip off size of message header. */
   msg_header = (RDA_RPG_message_header_t *) rda_data; 
   message_seg_size = msg_header->size - MSGHDRSZ;

   /* Mask off redundant channel information. */
   message_type = (int) (msg_header->type);

   /* Call the library routine that extracts and stores the new RDA config.
      Check the message type. If playing back data, it is possible that 
      message_type == 0 data is received .... we need to ignore these 
      messages.  For the RDA/RPG loopback messages, only update the 
      RDA Configuration if the message does not come from the Wideband 
      Simulator. */
   if( message_type > 0 ){

      ret = ORPGRDA_set_rda_config( rda_data );
      if ( ret != ORPGRDA_SUCCESS )
         LE_send_msg( GL_INFO,
                      "RW_receive_wideband_data: problem setting RDA config.\n");

   }

   /* Process the data type. */
   switch (message_type){

      case DIGITAL_RADAR_DATA:
      case GENERIC_DIGITAL_RADAR_DATA:{
   
         /* Convert the basedata message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping after all the segments are
            pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type, (char*) rda_data); 

         Process_radial( message_type, rda_data );
        
         /* Increment the number of messages processed. */
         CR_msg_processed++;

         if( (SCS_check_wb_alarm_status( RDA_COMM_DISC_ALARM) )
                              &&
             (!CR_communications_discontinuity) ){

            /* Check if the status is OPERATE, the OPERABILITY_STATUS is
               OPERATIONAL, and all base data moments are enable.  If all
               conditions are met, clear the RDA communications discontinuity
               alarm. */
            if( (CR_operational_mode == OP_OPERATIONAL_MODE) &&
                ((ret=ST_get_status(ORPGRDA_RDA_STATUS)) == RS_OPERATE) &&
                ((ret=ST_get_status(ORPGRDA_DATA_TRANS_ENABLED)) >
                BD_ENABLED_NONE) )
            {
               /* Clear the RPG communications discontinuity alarm in RDA
                  status if it is set. */
               SCS_clear_communications_alarm( RDA_COMM_DISC_ALARM );

               /* Reset display blanking. */
               SCS_update_wb_line_status( RS_CONNECTED,
                                          (int) 0, /* disable display_blanking */
                                          (int) 1 /* post wideband line
                                                     status changed event */ ); 

               /* Set the communication discontinuity timer. */
               if( TS_set_timer( (malrm_id_t) MALRM_RDA_COMM_DISC,
                                 (time_t) RDA_COMM_DISC_VALUE,
                                 (unsigned int) RDA_COMM_DISC_VALUE ) == TS_SUCCESS )
               {
                  if( CR_verbose_mode )
                     LE_send_msg( GL_INFO, "Set RDA_COMM_DISC Timer.\n" );
               }
            }
         }
         else if( (CR_force_connection)
                          &&
                  ((ret = SCS_get_wb_status( ORPGRDA_WBLNSTAT )) != RS_CONNECTED) ){

            LE_send_msg( GL_INFO, "Forcing Wideband Line Connection\n" );
            LC_process_wb_line_connection( CM_CONNECTED );

         }

         break;
      }

      case RDA_STATUS_DATA:{

         /* Convert the status message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping in the PR function after the 
            segments are pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         if( CR_hci_requested_status )
         {
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                         "Requested RDA Status Data is Available\n" );
            CR_hci_requested_status = 0;
         }
         else
         {
            LE_send_msg( GL_INFO, "RDA Status Data Received\n" );
         }

         PR_process_rda_status_data( rda_data );

         break;
      }

      case PERFORMANCE_MAINTENANCE_DATA:{

         /* Convert the PMD message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping in the PR function after the 
            segments are pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "RDA PERF/MAINT DATA RECEIVED.\n" );

         if( CR_hci_requested_pmd )
         {
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS,
               "Requested RDA Performance Maintenance Data is Available\n" );
            CR_hci_requested_pmd = 0;
         }

         PR_process_performance_data( rda_data );

         break;
      }

      case RDA_RPG_VCP:{

         /* Convert the vcp message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping in the PR function after the 
            segments are pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type, (char *) rda_data); 

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "RDA VCP MESSAGE RECEIVED.\n" );

         if( CR_hci_requested_vcp )
         {
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS,
               "Requested VCP Data is Available\n" );
            CR_hci_requested_vcp = 0;
         }

         PR_process_rda_vcp_data( (void *) rda_data );

         break;
      }

      case LOOPBACK_TEST_RDA_RPG:{

         int wblnstat;

         /* Convert the loopback message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping in the PR function after the 
            segments are pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "RDA/RPG LOOPBACK DATA RECEIVED.\n" );

         SWM_RDAtoRPG_loopback_message( (rda_data + MSGHDRSZ), message_seg_size );

         /* Check the wideband line status.  If not connected, then report error. */
         if( (wblnstat = SCS_get_wb_status( ORPGRDA_WBLNSTAT )) != RS_CONNECTED )
            LE_send_msg( GL_INFO, "Wideband Line Not Connected = %d, Check Line!\n",
                         wblnstat );
         break;
      } 

      case LOOPBACK_TEST_RPG_RDA:{

         /* Convert the loopback message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping in the PR function after the 
            segments are pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         /* Cancel loopback test timeout timer. */
         TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_LB_TEST_TIMEOUT );

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "RPG/RDA LOOPBACK DATA RECEIVED.\n" );

         if( PR_validate_RPGtoRDA_loopback_message( rda_data,
                                                     message_seg_size ) == PR_FAILURE )
            CR_loopback_failure = 1;

         /* Check if the periodic loopback rate is non-zero.  If so, start the
            loopback test periodic. */
         if( CR_loopback_periodic_rate > 0 )
         {
            if( (ret = TS_set_timer( (malrm_id_t) MALRM_LOOPBACK_PERIODIC, 
                                     (time_t) CR_loopback_periodic_rate,
                                     (int) MALRM_ONESHOT_NTVL )) == TS_FAILURE )
               LE_send_msg( GL_INFO, "Unable To Activate MALRM_LOOPBACK_PERIODIC\n" );  
         }

         break;
      } 

      case CLUTTER_FILTER_BYPASS_MAP:{

         int last_segment;

         /* Convert the bypass map data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa). NOTE: even though this is a multi-seg msg, we can
            still do the byte-swapping on each segment individually because they
            can be treated as an array of shorts. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         if( (ret = PR_process_bypass_map( rda_data, message_seg_size, 
                                           &last_segment)) >= PR_SUCCESS )
         {
             if( last_segment )
                LE_send_msg( GL_INFO, "FILTER BYPASS MAP RECEIVED.\n" );
         }

         break;
      } 

      case CLUTTER_MAP_DATA:{
    
         int last_segment;

         /* Convert the clutter map data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa). NOTE: even though this is a multi-seg msg, we can
            still do the byte-swapping on each segment individually because they
            can be treated as an array of shorts. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         if( (ret = PR_process_clutter_map( rda_data, message_seg_size, 
                                               &last_segment )) >= PR_SUCCESS )
         {
            if( last_segment )
               LE_send_msg( GL_INFO, "CLUTTER FILTER MAP RECEIVED.\n" );
         }

         break;
      } 
    
      case CONSOLE_MESSAGE_A2G:{

         /* Convert the console message data from external format to internal
            format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
            (or vice versa).  NOTE: if this ever becomes a multi-segment msg, 
            we'll have to do the byte-swapping in the PR function after the 
            segments are pieced together. */
         UMC_RDAtoRPG_message_convert_to_internal(message_type,(char*) rda_data); 

         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "RDA CONSOLE MESSAGE RECEIVED.\n" );

         PR_process_rda_console_msg( rda_data );

         break;
      }

      case ADAPTATION_DATA:{

         int last_segment;

         /* NOTE on byte-swapping: since the adaptation data msg is a
            multi-segment structured data msg, we cannot treat the data as
            an array of shorts and byte-swap each segment separately.  We must
            do the byte swapping after the segments are pieced together in the
            PR_process_adapt_data function. */
         if( (ret = PR_process_adapt_data( rda_data, message_seg_size, 
                                               &last_segment )) >= PR_SUCCESS )
         {
            if( last_segment )
               LE_send_msg( GL_INFO, "RDA ADAPTATION DATA MESSAGE RECEIVED.\n" );
         }

         break;
      }

      default:{

         if( message_type != 0 )
            LE_send_msg( GL_ERROR, "UNKNOWN DATA TYPE %d RECEIVED.\n", message_type );
         break;
      } 

   } /* End of "switch" */

   return (0);

} /* End of "RW_receive_wideband_data() */


/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Returns the current elevation index.
//
//   Inputs:  
//
//   Outputs:
//
//   Returns:
//      Returns the current elevation index.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////////\*/
int RW_get_current_elev_index(){

   /* Return the current elevation index. */
   return( Elev_number );

/* End of RW_get_current_elev_index() */
}

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Determines the RDA configuration (legacy or ORDA) and calls the
//	appropriate function to handle the radial message.
//
//   Inputs:  
//      message_type - Either GENERIC_DIGITAL_RADAR_DATA or 
//                     DIGITAL_RADAR_DATA.
//      rda_data - Pointer to Radial data message.
//
//   Outputs:
//
//   Returns:
//
//   Globals:
//
////////////////////////////////////////////////////////////////////////\*/
static void Process_radial( int message_type, short *rda_data )
{
   int ret = 0;

   if ( (ret = ST_get_rda_config( NULL )) == ORPGRDA_LEGACY_CONFIG ){

      ret = Process_rda_radial( message_type, (char *) rda_data );
      if ( ret == PR_FAILURE )
         LE_send_msg( GL_INFO, "Problem in Process_rda_radial().\n" );

   }
   else{

      /* If this is GENERIC_DIGITAL_RADAR_DATA, check if the message is internally
         compressed.   If so, need to decompress it. NOTE: We don't normally expect
         the data to be compressed.  Therefore we malloc a buffer only when we
         determine this to be the case. */
      if( message_type == GENERIC_DIGITAL_RADAR_DATA ){

         static char *dest = NULL;
         static int dest_len = 0;

         Generic_basedata_t *rec = (Generic_basedata_t *) rda_data;
         int method, ret;

         if( (rec->base.compress_type == M31_BZIP2_COMP )
                         ||
             (rec->base.compress_type == M31_ZLIB_COMP ) ){

            int src_len = rec->msg_hdr.size;

            if( (dest == NULL) || (dest_len < rec->base.radial_length) ){

               /* Allocate more space than necessary.  This hopefully
                  prevents a whole lot of reallocations.  This also
                  prevents realloc from actually freeing the memory
                  which is the behavior when dest_len = 0. */
               dest_len = rec->base.radial_length + 1000;
               dest = realloc( dest, dest_len );
               if( dest == NULL ){

                  LE_send_msg( GL_INFO, "realloc Failed for %d Bytes\n", dest_len );
                  return;

               }

            }

            if( rec->base.compress_type == M31_ZLIB_COMP )
               method = MISC_GZIP;

            else if( rec->base.compress_type == M31_BZIP2_COMP )
               method = MISC_BZIP2;

            else{

               LE_send_msg( GL_INFO, "Unknown Compression Method: %d.  Skip Radial.\n",
                            rec->base.compress_type );
               return;

            }

            /* The data that is compressed occurs after the Generic_basedata_header_t
               structure. */
            memcpy( dest, (char *) rda_data, sizeof(Generic_basedata_t) );
            src_len -= sizeof(Generic_basedata_t);
            ret = MISC_decompress( method, (char *) rda_data + sizeof(Generic_basedata_t),
                                   src_len, dest + sizeof(Generic_basedata_t),
                                   dest_len );
            if( ret < 0 ){

               LE_send_msg( GL_INFO, "MISC_decompress Failed: %d.  Skip Radial.\n", ret );
               return;

            }

            /* Set "radar_data" pointer to destination buffer. */
            rda_data = (short *) dest;

         }

      }

      ret = Process_orda_radial( message_type, (char *) rda_data );
      if ( ret == PR_FAILURE )
         LE_send_msg( GL_INFO, "Problem in Process_orda_radial().\n" );
      
   }

} /* End of Process_radial() */

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Saves the elevation index in case of an elevation restart.  This
//      module also reports on azimuth numbers out-of-sequence.
//
//   Inputs:  
//      rda_data - Pointer to Legacy RDA Radial data message.
//
//   Outputs:
//
//   Returns:
//	PR_FAILURE or PR_SUCCESS - integer representing failure or success
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////////\*/
static int Process_rda_radial( int message_type, char *rda_data )
{
   float elev, azm;
   char time_string[15];

   short tmp_status, tmp_elev_num, tmp_azi_num;
   unsigned int tmp_time;
   
   static short tmp_vcp_num = -1;
   static int azimuth_number = -1;

   /* Have to treat DIGITAL_RADAR_DATA and GENERIC_DIGITAL_RADAR_DATA
      differently. */
   if( message_type == DIGITAL_RADAR_DATA ){

      /* Cast RDA message to radial message header structure. */
      RDA_basedata_header *header = (RDA_basedata_header *) rda_data;

      /* Extract fields used in this module. */
      tmp_status = header->status;
      tmp_elev_num = header->elev_num;
      tmp_azi_num = header->azi_num;

      /* Extract elevation and azimuth angles, in degrees. */
      elev = (double) ((unsigned short) header->elevation * RDA_ANG_TO_DEGREE);
      azm = (double) ((unsigned short) header->azimuth * RDA_ANG_TO_DEGREE);

      tmp_time = header->time;

   }
   else if( message_type == GENERIC_DIGITAL_RADAR_DATA ){

      /* Cast RDA message to generic radial message header structure. */
      Generic_basedata_t *header = (Generic_basedata_t *) rda_data;

      /* Extract fields used in this module. */
      tmp_status = header->base.status;
      tmp_elev_num = header->base.elev_num;
      tmp_azi_num = header->base.azi_num;
      elev = header->base.elevation;
      azm = header->base.azimuth;
      tmp_time = header->base.time;

   }
   else
      return PR_FAILURE;

   /* Set the current elevation number.  This will be used in case of an
      elevation restart. */
   Elev_number = tmp_elev_num;

   /* If beginning or volume or elevation, display message. */
   if( (tmp_status == GOODBVOL) && (tmp_elev_num == 1) ){
 
      /* Beginning of volume scan. */
      if( message_type == DIGITAL_RADAR_DATA ){

         /* Cast RDA message to radial message header structure. */
         RDA_basedata_header *header = (RDA_basedata_header *) rda_data;

         tmp_vcp_num = header->vcp_num;

      }
      else{ 

         Generic_basedata_t* rec = (Generic_basedata_t *) rda_data;
         int i;

         for( i = 0; i < rec->base.no_of_datum; i++ ){

            Generic_any_t *data_block;
            char type[5];

            data_block = (Generic_any_t *)
                 (rda_data + sizeof(RDA_RPG_message_header_t) + rec->base.data[i]);

            /* Convert the name to a string so we can do string compares. */
            memset( type, 0, 5 );
            memcpy( type, data_block->name, 4 );

            if( strcmp( type, "RVOL" ) == 0 ){

               Generic_vol_t *vol = (Generic_vol_t *) data_block;
               tmp_vcp_num = vol->vcp_num;
               break;

            }

         }

      }

      /* Get radial time. */
      Convert_time( tmp_time, time_string );

      sprintf( status_buffer, 
         "Time: %s Beg Vol: VCP# %d  Cut#/El: %d/%4.1f Az: %6.2f",
         time_string, tmp_vcp_num, tmp_elev_num, elev, azm );

      LE_send_msg( GL_INFO, "%s\n", status_buffer );
 
      /* Save azimuth number for radial sequence checking. */
      azimuth_number = tmp_azi_num;
   }
   else if( (tmp_status == GOODBEL)
                        || 
            ( (tmp_status == GOODBVOL) && (tmp_elev_num == 2) ) )
   { 
      /* Beginning of elevation cut. */

      /* Get radial time. */
      Convert_time( tmp_time, time_string );

      sprintf( status_buffer, 
         "Time: %s Beg Elev: Cut#/El: %d/%4.1f Az: %6.2f",
         time_string, tmp_elev_num, elev, azm );

      LE_send_msg( GL_INFO, "%s\n", status_buffer );

      /* Save azimuth number for radial sequence checking. */
      azimuth_number = tmp_azi_num;
   }
   else
   {
      /* Check if azimuth number of this radial is one more than previous radial. */
      if( (tmp_azi_num != azimuth_number + 1) && (azimuth_number != -1) )
      {
         /* Azimuth number sequence errors occur when a radial message is lost.
            If the radial gap is not too large, we don't usually care. */
         LE_send_msg( GL_INFO, 
              "Azimuth Number (%d) Sequence Error @ EL/AZ/AZ# %4.1f/%6.2f/%2d\n",
                      azimuth_number, elev, azm, tmp_azi_num );

         if( (tmp_status == GENDEL) || (tmp_status == GENDVOL) )
            LE_send_msg( GL_INFO, "Sequence Error Occurred At %d\n", tmp_status );
      }

      azimuth_number = tmp_azi_num;
   }

   if( (tmp_azi_num == 1) && (tmp_status != GOODBVOL) && 
       (tmp_status != GOODBEL) )
      LE_send_msg( GL_INFO, "Azimuth Number Is 1 But Radial Status Is %d\n",
                   tmp_status );

   return PR_SUCCESS;

} /* End of Process_rda_radial() */

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Saves the elevation index in case of an elevation restart.  This
//      module also reports on azimuth numbers out-of-sequence.
//
//   Inputs:  
//      message_type - either DIGITAL_RADAR_DATA or 
//                     GENERIC_DIGITAL_RADAR_DATA
//      orda_data - Pointer to Open RDA Radial data message.
//
//   Outputs:
//
//   Returns:
//	PR_FAILURE or PR_SUCCESS - integer representing failure or success
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
////////////////////////////////////////////////////////////////////////\*/
static int Process_orda_radial( int message_type, char* orda_data )
{
   float elev, azm;
   char time_string[15];

   short tmp_status, tmp_elev_num, tmp_azi_num;
   unsigned int tmp_time;
   
   static short tmp_vcp_num = -1;
   static int azimuth_number = -1;

   /* Have to treat DIGITAL_RADAR_DATA and GENERIC_DIGITAL_RADAR_DATA
      differently. */
   if( message_type == DIGITAL_RADAR_DATA ){

      /* Cast RDA message to radial message header structure. */
      RDA_basedata_header *header = (RDA_basedata_header *) orda_data;

      /* Extract fields used in this module. */
      tmp_status = header->status;
      tmp_elev_num = header->elev_num;
      tmp_azi_num = header->azi_num;

      /* Extract elevation and azimuth angles, in degrees. */
      elev = (double) ((unsigned short) header->elevation * RDA_ANG_TO_DEGREE);
      azm = (double) ((unsigned short) header->azimuth * RDA_ANG_TO_DEGREE);

      tmp_time = header->time;

   }
   else if( message_type == GENERIC_DIGITAL_RADAR_DATA ){

      /* Cast RDA message to generic radial message header structure. */
      Generic_basedata_t *header = (Generic_basedata_t *) orda_data;

      /* Extract fields used in this module. */
      tmp_status = header->base.status;
      tmp_elev_num = header->base.elev_num;
      tmp_azi_num = header->base.azi_num;
      elev = header->base.elevation;
      azm = header->base.azimuth;
      tmp_time = header->base.time;

   }
   else
      return PR_FAILURE;

   /* Set the current elevation number.  This will be used in case of an
      elevation restart. */
   Elev_number = tmp_elev_num;

   /* If beginning or volume or elevation, display message. */
   if( (tmp_status == GOODBVOL) && (tmp_elev_num == 1) )
   { 
      /* Beginning of volume scan. */
      if( message_type == DIGITAL_RADAR_DATA ){

         /* Cast RDA message to radial message header structure. */
         RDA_basedata_header *header = (RDA_basedata_header *) orda_data;

         tmp_vcp_num = header->vcp_num;

      }
      else{

         Generic_basedata_t* rec = (Generic_basedata_t *) orda_data;
         int i;

         for( i = 0; i < rec->base.no_of_datum; i++ ){

            Generic_any_t *data_block;
            char type[5];

            data_block = (Generic_any_t *)
                 (orda_data + sizeof(RDA_RPG_message_header_t) + rec->base.data[i]);

            /* Convert the name to a string so we can do string compares. */
            memset( type, 0, 5 );
            memcpy( type, data_block->name, 4 );

            if( strcmp( type, "RVOL" ) == 0 ){

               Generic_vol_t *vol = (Generic_vol_t *) data_block;
               tmp_vcp_num = vol->vcp_num;
               break;

            }

         }

      }

      /* Get radial time. */
      Convert_time( tmp_time, time_string );

      sprintf( status_buffer, 
         "Time: %s Beg Vol: VCP# %d  Cut#/El: %d/%4.1f Az: %6.2f",
         time_string, tmp_vcp_num, tmp_elev_num, elev, azm );

      LE_send_msg( GL_INFO, "%s\n", status_buffer );
 
      /* Save azimuth number for radial sequence checking. */
      azimuth_number = tmp_azi_num;

   }
   else if( ((tmp_status == GOODBEL) || (tmp_status == GOODBELLC))
                      || 
            ((tmp_status == GOODBVOL) && (tmp_elev_num == 2)) )
   { 
      /* Beginning of elevation cut. */

      /* Get radial time. */
      Convert_time( tmp_time, time_string );

      sprintf( status_buffer, 
         "Time: %s Beg Elev: Cut#/El: %d/%4.1f Az: %6.2f",
         time_string, tmp_elev_num, elev, azm );

      LE_send_msg( GL_INFO, "%s\n", status_buffer );

      /* Save azimuth number for radial sequence checking. */
      azimuth_number = tmp_azi_num;
   }
   else
   {
      /* Check if azimuth number of this radial is one more than previous radial. */
      if( (tmp_azi_num != azimuth_number + 1) && (azimuth_number != -1) )
      {
         /* Azimuth number sequence errors occur when a radial message is lost.
            If the radial gap is not too large, we don't usually care. */
         LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
              "Azimuth Number (%3d) SEQUENCE ERROR (EL/AZ/AZ#):  %4.1f/%6.2f/%3d\n",
                      azimuth_number, elev, azm, tmp_azi_num );

         if( (tmp_status == GENDEL) || (tmp_status == GENDVOL) )
            LE_send_msg( GL_INFO, "Sequence Error Occurred At %d\n", tmp_status );
      }

      azimuth_number = tmp_azi_num;
   }

   if( (tmp_azi_num == 1) && (tmp_status != GOODBVOL) && 
       (tmp_status != GOODBEL) && (tmp_status != GOODBELLC) )
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                   "Azimuth Number Is 1 But Radial Status %d Is UNEXPECTED\n", tmp_status );

   return PR_SUCCESS;

} /* End of Process_orda_radial() */

/*\///////////////////////////////////////////////////////////////
//
//   Description:   
//      Time in milliseconds past midnight is converted to 
//      hh:mm:ss format.
//
//   Inputs:   
//      timevalue - time in milliseconds past midnight
//      time_str - pointer to time string.
//
//   Outputs: 
//      *time_str - contains time in hh:mm:ss format.
//
//   Returns:
//      There are not return values defined for this function.
//
//   Globals:
//      
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
static void Convert_time( unsigned long timevalue, char *time_str ){
 
   int hrs, mins, secs;
   char seconds[20];

   /* Extract the number of hours. */
   hrs = timevalue/3600000;

   /* Extract the number of minutes. */
   timevalue = timevalue - hrs*3600000;
   mins = timevalue/60000;

   /* Extract the number of seconds. */
   secs = timevalue - mins*60000;

   /* Convert numbers to ASCII. */
   To_ASCII( secs, seconds );
   sprintf( time_str, "%02d:%02d:%s", hrs, mins, seconds );

/* End of Convert_time() */
}

/*\////////////////////////////////////////////////////////////
//
//   Description:  
//      Input value is converted from integer to xx.xxx format.
//
//   Inputs:  
//      value - number to be converted to ASCII
//      string - output string.
//
//   Outputs:
//      *sting - contains value converted to xx.xxx format.
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////\*/
static void To_ASCII( int value, char *string ){

   int i; 
   unsigned char digit;

   /* Process real value. */

   for( i = 5; i >= 3; i-- ){
   
      /* Produce the fractional portion of text string. */
      digit = (unsigned char) value%10;
      string[i] = digit + '0';

      value = value/10; 

   }

   /* Put in the decimal point. */
   string[2] = '.';

   for( i = 1; i >= 0; i-- ){
   
      /* Produce the fractional portion of text string. */
      digit = (unsigned char) value%10;
      string[i] = digit + '0';

      value = value/10; 

   }

   /* Pad the string with string terminator. */
   string[6] = '\0';

/* End of To_ASCII() */
}
