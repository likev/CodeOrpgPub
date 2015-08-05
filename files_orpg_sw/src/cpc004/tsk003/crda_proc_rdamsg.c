/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:22:09 $
 * $Id: crda_proc_rdamsg.c,v 1.86 2014/11/07 21:22:09 steves Exp $
 * $Revision: 1.86 $
 * $State: Exp $
 */
/*******************************************************************

	Main module for Process RDA Message program

*******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <crda_control_rda.h>
#include <orpg.h>
#include <misc.h>
#include <time.h>
#include <basedata.h>
#include <rda_rpg_loop_back.h>
#include <rda_performance_maintenance.h>
#include <rda_rpg_console_message.h>
#include <math.h>


/* RPG to RDA loopback message save area. */
static short RPG_RDA_loopback[ MSIZ_RPGLB ];

/* Clutter filter bypass map generation time. */
static short Rda_bypass_map_generation_time = 0;

/* Clutter filter bypass map generation date. */
static short Rda_bypass_map_generation_date = 0;

/* Local functions */
static int Process_rda_bypass_map( short* rda_data_ptr, int msg_sz, int* last_seg,
                                   int seg_num, int tot_segs);
static int Process_orda_bypass_map( short* rda_data_ptr, int msg_sz, int* last_seg,
                                    int seg_num, int tot_segs);
static int Process_rda_clutter_map( short* rda_data_ptr, int msg_sz,
                                    int* last_seg, int seg_num, int tot_segs);
static int Process_orda_clutter_map( short* rda_data_ptr, int msg_sz,
                                     int* last_seg, int seg_num, int tot_segs);
static int Process_rda_adapt_data( short* rda_data_ptr, int msg_sz,
                                   int* lastseg, int seg_num, int tot_segs);


/*\////////////////////////////////////////////////////////////////////
// 
//   Description:    
//      Processes the RDA status message.  Most of the processing is
//      handled in the SCS modules.  See for further details.
//
//      The bypass map generation date and time are saved.
//
//   Inputs:    
//      rda_data - pointer to RDA status message data.  
//
//   Outputs:
//
//   Returns:     
//      Currently undefined.
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
////////////////////////////////////////////////////////////////////\*/
void PR_process_rda_status_data( short *rda_data ){

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Processing new RDA Status Msg.\n");

   /* Process the rda status message. */
   if( ST_process_rda_status( rda_data ) != ST_FAILURE ){

      Rda_bypass_map_generation_date = ST_get_status(ORPGRDA_BPM_GEN_DATE);
      if ( Rda_bypass_map_generation_date == ST_FAILURE )
         LE_send_msg ( GL_ERROR,
            "In PR_process_rda_status_data, failure to get BPM date.\n");

      Rda_bypass_map_generation_time = ST_get_status(ORPGRDA_BPM_GEN_TIME);
      if ( Rda_bypass_map_generation_time == ST_FAILURE )
         LE_send_msg ( GL_ERROR,
            "In PR_process_rda_status_data, failure to get BPM time.\n");
      
   }

} /* End of PR_process_rda_status_data() */


/*\/////////////////////////////////////////////////////////////////////
//
//   Description:  
//      The RPG/RDA loopback message is validated, .i.e., checks if the 
//      message content received is the same as what was sent out.
//
//      If successful, a request for RDA Status is issued.
//
//   Inputs:  
//      rda_data - pointer to RPG to RDA loopback message data.
//      message_size - length of the message in shorts.
//
//   Outputs:
//
//   Returns:   
//      PR_FAILURE if test failed; otherwise PR_SUCCESS.  
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////////\*/
int PR_validate_RPGtoRDA_loopback_message( short *rda_data, 
                                           int message_size ){

   int i, j;

   for( i = 0, j = MSGHDRSZ; i < message_size; i++, j++ ){

      if( rda_data[j] != RPG_RDA_loopback[i] )
         return ( PR_FAILURE );

   }

   /* Return Normal. */
   return ( PR_SUCCESS );
   
/* End of PR_validate_RPGtoRDA_loopback_message() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Initializes the RPG to RDA loopback test message.  The message
//      contents are arbitrary.  Here we set values the same as
//      legacy.
//
//   Inputs:
//
//   Outputs:
//      RPG_RDA_loopback - contains initialized RPG loopback data.
//
//   Returns:
//      There is no return values defined for this function.
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
void PR_init_RPGtoRDA_loopback_message( ){

   short i, j;

   /* Store Loopback message size, in halfwords. */
   RPG_RDA_loopback[0] = MSIZ_RPGLB;
   RPG_RDA_loopback[1] = 0;

   /* Insert Loopback message test data. */
   for( i = 2, j = 2; i < MSIZ_RPGLB; i+=2, j++ ){
         
      RPG_RDA_loopback[i] = (short) -1;
      RPG_RDA_loopback[i+1] = -j;

   }

/* End of PR_init_RPGtoRDA_loopback_message() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Transfers RPG to RDA loopback message to buffer for transmission 
//      to the RDA.
//
//   Inputs:  
//      message_data - Pointer to pointer where loopback message 
//                     data is to be stored for transmission to the RDA.
//      message_size - Length of the loopback message, in shorts.
//
//   Outputs:
//      message_data -  Pointer to pointer where loopback message 
//                      data is stored for transmission to the RDA.
//      message_size - Length of the loopback message, in shorts.
//
//   Returns:   
//      PR_FAILURE if memory allocation fails; otherwise PR_SUCCESS.  
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
int PR_RPGtoRDA_loopback_message( short **message_data,
                                  int *message_size ){

   char *loop_back_msg;

   /* Allocate storage for the RDA Loopback message. */
   if( (loop_back_msg = (char *) calloc( (size_t) 1, 
                      (size_t) (sizeof(short) * MSIZ_RPGLB) )) == NULL ){

      LE_send_msg( GL_MEMORY, "RPG/RDA Loopback calloc Failed\n" );
      return( PR_FAILURE );

   }

   /* Insert Loopback message test data. */
   memcpy( loop_back_msg, RPG_RDA_loopback, 
           (size_t) (MSIZ_RPGLB*sizeof(short)) );

   /* Assign pointer and message size to message_data and message_size, 
      respectively. */
   *message_data = (short *) loop_back_msg;
   *message_size = MSIZ_RPGLB;  /* In halfwords. */

   return ( PR_SUCCESS );

/* PR_RPGtoRDA_loopback_message() */
}

/*\/////////////////////////////////////////////////////////////////
//
//   Description:   
//	Determine the segment number, total number of segments,
//	RDA configuration, and call the appropriate message
//	handling function (for legacy RDA versus Open RDA bypass
//	map messages).
//
//   Inputs:   
//      rda_data - Pointer to where bypass map data is stored.
//      msg_size - Size of the message, in shorts, w/o msg hdr.
//      last_segment - Set when last bypass map segment received.
//
//   Outputs:
//
//   Returns:    
//      PR_FAILURE or PR_SUCCESS is returned.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int PR_process_bypass_map( short *rda_data, int msg_size,
                           int *last_segment ){

   int segnum, totsegs;
   RDA_RPG_message_header_t *msg_hdr;
   int rda_config;
   int status = 0;

   /* Get the segment number and total number of segments. */
   msg_hdr = (RDA_RPG_message_header_t *) rda_data;
   segnum = msg_hdr->seg_num;
   totsegs = msg_hdr->num_segs;


   /* Validate the segment number in the message. */
   if( segnum < 0 || segnum > totsegs ){

      LE_send_msg( GL_ERROR, "RDA Bypass Map Invalid Segment Number (%d)\n",
                   segnum );
      return( PR_FAILURE );

   }

   rda_config = ST_get_rda_config( NULL );

   if ( rda_config == ORPGRDA_LEGACY_CONFIG )
      status = Process_rda_bypass_map( rda_data, msg_size, last_segment,
         segnum, totsegs);
    
   else if ( rda_config == ORPGRDA_ORDA_CONFIG )
      status = Process_orda_bypass_map( rda_data, msg_size, last_segment,
         segnum, totsegs);
    
   else{

      LE_send_msg( GL_ERROR, "Cannot Process RDA Bypass Map.  Unknown RDA config.\n");
      return( PR_FAILURE );

   }

   if ( status == PR_FAILURE )
      return ( PR_FAILURE );

   /* Return Normal. */
   return ( PR_SUCCESS );

}  /* End of PR_process_bypass_map() */


/*\/////////////////////////////////////////////////////////////////
//
//   Description:   
//	Determine the segment number, total number of segments,
//	RDA configuration, and call the appropriate message
//	handling function (for legacy RDA versus Open RDA
//	Clutter Map messages).
//
//   Inputs:   
//      rda_data - Pointer to clutter map data including message
//                 header.
//      message_size - Size of the message, in shorts, not including
//                     the message header.
//      last_seg - Set when last clutter map segment received.
//
//   Outputs:
//
//   Returns:    
//      PR_FAILURE or PR_SUCCESS is returned.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int PR_process_clutter_map( short *rda_data, int message_size,
                               int *last_seg ){

   RDA_RPG_message_header_t* msg_hdr = NULL;
   int segnum, totsegs;
   int rda_config;
   int status = 0;

   /* Get the segment number and total number of segments from the
      message header. */
   msg_hdr = (RDA_RPG_message_header_t *) rda_data;
   segnum = msg_hdr->seg_num;

   totsegs = msg_hdr->num_segs;

   /* Validate the segment number in the message. */
   if( segnum < 0 || segnum > totsegs ){

      LE_send_msg( GL_ERROR, "Clutter Map - Invalid Segment (%d)\n",
         segnum );
      return( PR_FAILURE );

   }

   rda_config = ST_get_rda_config( NULL );

   if ( rda_config == ORPGRDA_LEGACY_CONFIG )
      status = Process_rda_clutter_map( rda_data, message_size, last_seg,
                                        segnum, totsegs);
    
   else if ( rda_config == ORPGRDA_ORDA_CONFIG )
      status = Process_orda_clutter_map( rda_data, message_size, last_seg,
                                         segnum, totsegs);
    
   else{

      LE_send_msg( GL_ERROR, "Cannot process RDA Clutter Map.  Unknown RDA config.\n");
      return( PR_FAILURE );

   }

   if ( status != PR_SUCCESS )
      return( PR_FAILURE );

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* PR_process_clutter_map() */


/*\/////////////////////////////////////////////////////////////////
//
//   Description:   
//	Determine the segment number, total number of segments, and
//	read and store the adaptation data messages.
//
//   Inputs:   
//      rda_data - Pointer to the incoming adaptation data including
//		   the message header.
//      message_size - Size of the message, in shorts, not 
//		       including the msg hdr.
//      last_seg - Set when last message segment received.
//
//   Outputs:
//
//   Returns:    
//      PR_FAILURE or PR_SUCCESS is returned.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int PR_process_adapt_data( short *rda_data, int message_size,
                               int *last_seg ){

   RDA_RPG_message_header_t* msg_hdr = NULL;
   int segnum, totsegs;
   int status = 0;

   /* Get the segment number and total number of segments from the
      message header. */
   msg_hdr = (RDA_RPG_message_header_t *) rda_data;
   segnum = msg_hdr->seg_num;

   if( msg_hdr->type != ADAPTATION_DATA ){

      LE_send_msg( GL_ERROR, 
                   "Non RDA Adapt Data Msg (%d) Passed to PR_process_adapt_data\n",
                   msg_hdr->type );
      return( PR_FAILURE );

   }

   totsegs = msg_hdr->num_segs;

   /* Validate the segment number in the message. */
   if( segnum < 0 || segnum > totsegs ){

      LE_send_msg( GL_ERROR,
         "RDA Adaptation Data Msg Contains Invalid Segment (%d)\n", segnum );
      return( PR_FAILURE );

   }

   status = Process_rda_adapt_data( rda_data, message_size, last_seg, segnum,
                                    totsegs);

   if ( status != PR_SUCCESS )
      return( PR_FAILURE );

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* PR_process_adapt_data() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Writes performance data to Linear Buffer.  If write is
//      successful, the event ORPGEVT_PERF_MAIN_RECEIVED is posted. 
//      A system status log message is written indicating the 
//      performance data is available.
//
//   Inputs:   
//      rda_data - pointer to performance data, including msg hdr.
//
//   Outputs:
//
//   Returns:
//      Returns PR_FAILURE if ORPGDA_write fails, PR_SUCCESS otherwise.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////\*/
int PR_process_performance_data( short *rda_data ){

   int ret = 0;
   int ret_val = PR_SUCCESS;
   int rda_config = 0;

   static time_t old_perf_check_time = (time_t) -1;

   rda_config = ST_get_rda_config( NULL );

   if ( rda_config == ORPGRDA_LEGACY_CONFIG ){

      /* Write Legacy RDA performance/maintenance data to LB. */
      if( (ret = ORPGDA_write( ORPGDAT_RDA_PERF_MAIN, (char *) rda_data,
         (int) sizeof( rda_performance_t), LB_ANY )) > 0 ){

         /* Post the Performance/Maintenance Data Received Event. */
         if( (ret = ES_post_event( ORPGEVT_PERF_MAIN_RECEIVED, (char *) NULL, 
                                  (size_t) 0, (int) 0 )) != ES_SUCCESS )
            ret_val = PR_FAILURE;
          
      }
      else{

         LE_send_msg( GL_ORPGDA(ret), "Performance Data Write Failed (%d)\n", ret );
         ret_val = PR_FAILURE;

      }

   }
   else if ( rda_config == ORPGRDA_ORDA_CONFIG ){

      orda_pmd_t *pmd = (orda_pmd_t *) rda_data;
      float noise_value, dBZ0;
      int noise_temp, peak_pwr, cltr_supp_delta;

      /* Extract the time to next performance check. */
      if( pmd->pmd.perf_check_time != 0 ){

         int yr = 0, mon = 0, day = 0, hr = 0, min = 0, sec = 0;

         /* Has the Perf Check Time changed? */
         if( old_perf_check_time != pmd->pmd.perf_check_time ){

            if( old_perf_check_time != (time_t) -1 )
               LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG, 
                            "RDA CAL: Performance Check Completed\n" );

            old_perf_check_time = pmd->pmd.perf_check_time;

         }

         unix_time( &pmd->pmd.perf_check_time, &yr, &mon, &day, &hr, &min, &sec );
         LE_send_msg( GL_INFO, 
                      "RDA PERF: Performance Check Time: %02d/%02d/%02d %02d:%02d:%02d\n",
                      mon, day, yr, hr, min,sec );
      }

      /* Include DP information if requested. */
      if( CR_include_dp_pmd && (CR_rda_build_num >= 12.0f) ){

         noise_temp = (int) roundf( pmd->pmd.v_noise_temp );
         if( pmd->pmd.pfn_swtch_position == 1 ){

            /* Long Pulse. */
            noise_value = pmd->pmd.v_long_pulse_noise;
            dBZ0 = pmd->pmd.long_pls_v_dbz0;

         }
         else{
 
            /* Short Pulse. */
            noise_value = pmd->pmd.v_shrt_pulse_noise;
            dBZ0 = pmd->pmd.shrt_pls_v_dbz0;

         }

         LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG,
                      "RDA CAL (DP): ZDRB=%9.4f, VN=%5.2f, VNT=%d, VdBZ0=%5.2f\n",
                      pmd->pmd.zdr_bias, noise_value, noise_temp, dBZ0 );

      }

      /* Extract PMD fields for CAL System Status Log Informational 
         Message. */
      noise_temp = (int) roundf( pmd->pmd.h_noise_temp );
      peak_pwr = (int) roundf( pmd->pmd.xmtr_peak_pwr );
      cltr_supp_delta = (int) roundf( pmd->pmd.h_cltr_supp_delta );
      if( pmd->pmd.pfn_swtch_position == 1 ){

         /* Long Pulse. */
         noise_value = pmd->pmd.h_long_pulse_noise;
         dBZ0 = pmd->pmd.long_pls_h_dbz0;

      }
      else{
 
         /* Short Pulse. */
         noise_value = pmd->pmd.h_shrt_pulse_noise;
         dBZ0 = pmd->pmd.shrt_pls_h_dbz0;

      }

      LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG,
         "RDA CAL: T=%3d; N=%5.2f; NT=%d; dBZ0=%5.2f; I0=%6.2f; C=%2d; L=%6.4f\n", 
         peak_pwr, noise_value, noise_temp, dBZ0, pmd->pmd.h_i_naught, 
         cltr_supp_delta, pmd->pmd.h_linearity );

      /* Write Open RDA performance/maintenance data to LB. */
      if( (ret = ORPGDA_write( ORPGDAT_RDA_PERF_MAIN, (char *) rda_data,
                               (int) sizeof( orda_pmd_t ), LB_ANY )) > 0 ){

         /* Post the Performance/Maintenance Data Received Event. */
         if( (ret = ES_post_event( ORPGEVT_PERF_MAIN_RECEIVED, (char *) NULL, 
                                   (size_t) 0, (int) 0 )) != ES_SUCCESS )
            ret_val = PR_FAILURE;
         
      }
      else{

         LE_send_msg( GL_ORPGDA(ret), "RDA Performance Data Write Failed (%d)\n", ret );
         ret_val = PR_FAILURE;

      }

   }
   else{

      LE_send_msg( GL_ERROR, "Unknown RDA config.\n");
      ret_val = PR_FAILURE;

   }

   return ret_val;

} /* End of PR_process_performance_data() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Writes RDA console message data to Linear Buffer.
//      A system status log message is written indicating a console
//      message was received.  The console msg is parsed and written
//	to the task log.
//
//   Inputs:   
//      rda_data - pointer to console message data.
//
//   Outputs:
//
//   Returns:
//     PR_FAILURE if ORPGDA_write fails, or PR_SUCCESS otherwise.
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
////////////////////////////////////////////////////////////////////\*/
int PR_process_rda_console_msg( short *rda_data ){

   int				ret;
   int 				num_segs;
   int 				seg_index;
   int 				max_chars_per_line = 54;
   int 				char_index;
   char*			temp_str;
   RDA_RPG_console_message_t	*console_msg;

   /* Cast to console message struct. */
   console_msg = (RDA_RPG_console_message_t *) rda_data;

   /* Write console message data to LB. */
   if( (ret = ORPGDA_write( ORPGDAT_RDA_CONSOLE_MSG, (char *) console_msg,
      (int) ( console_msg->msg_hdr.size * sizeof(short) ), LB_ANY )) < 0 ){

      LE_send_msg( GL_ORPGDA(ret), "RDA Console Message Write Failed (%d)\n", ret );
      return( PR_FAILURE );

   }

   /* If verbose mode, segment the msg, if necessary, and print the msg to 
      the task log */
   if( CR_verbose_mode ){

      /* Store number of chars in msg. */
      int num_chars = console_msg->size;

      /* Allocate sufficient memory to store working copy of msg */
      char* console_buf = (char*) malloc((num_chars + 1) * sizeof(char));

      /* initialize character counter equal to num_chars */
      int char_counter = num_chars;
      
      /* Copy console message to string buffer. */
      memcpy( console_buf, console_msg->message, (size_t) num_chars );

      /* Put string terminator at the end (just in case). */
      console_buf[num_chars] = '\0';
 
      /* Determine the number of 'max_chars_per_line' character segments */
      num_segs = (int)( num_chars / max_chars_per_line ) + 1;

      /* Loop through segments and send msgs to task log */
      LE_send_msg( GL_INFO, "The Following RDA Console Message Was Received:\n" );
      for ( seg_index = 0; seg_index < num_segs; seg_index++ ){

         /* Allocate space for temp_str - the line we'll fill and write */
         temp_str = (char *) calloc(1, (max_chars_per_line + 1) * sizeof(char));

         char_index  = seg_index * max_chars_per_line;

         if ( char_counter >= max_chars_per_line ){

            strncpy(temp_str, (char*)(console_msg->message + char_index),
               max_chars_per_line);
            char_counter = char_counter - max_chars_per_line;
         }
         else{

            strncpy(temp_str, (char*)(console_msg->message + char_index),
               char_counter);
            char_counter = 0;

         }

         /* make sure the string is terminated */
         *(temp_str+max_chars_per_line) = '\0';

         /* write the segment to the task log */
         LE_send_msg( GL_INFO, "%s\n", temp_str );
 
         /* Free temp_str memory */
         free(temp_str);
      }

      /* Free console_buf memory */
      free(console_buf);

   } 

   /* Return Normal. */
   LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "RDA Console Message Received\n" );
   return ( PR_SUCCESS );

/* End of PR_process_rda_console_msg() */
}


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Transfer incoming data to the legacy RDA bypass map data structure.  
//      This message comes in multiple segments.
//              
//      If all message segments received successfully, then:
//
//         1) The ITC containing the bypass_map_request_pending flag
//            (DATAID_A304C2) is read, the flag is set, then the ITC 
//            is written.
//
//         2) The Bypass Map message is written to Linear Buffer.
//
//         3) The ITC containing the bypass map generation date/time
//            (DATAID_CD07_BYPASSMAP) is read, the date/time are set,
//            then the ITC is written.
//
//         4) A system status log message is written indicating the 
//            bypass map is available.
//
//         5) The event ORPGEVT_BYPASS_MAP_RECEIVED is posted.
//
//   Inputs:   
//      rda_data_ptr	- Pointer to where bypass map data is stored.
//      msg_sz		- Size of the message, in shorts, w/o msg hdr.
//      lastseg		- Set when last bypass map segment received.
//      seg_num		- Message segment number.
//      tot_segs	- Message total number of segments.
//
//   Outputs:
//
//   Returns:    
//      PR_FAILURE if either memory allocation failed for local 
//      storage, or too many message segments received, or bypass map 
//      write failed, or date/time stamp write failed, or event notification
//      failed.
//
//      Otherwise, PR_SUCCESS is returned.
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
////////////////////////////////////////////////////////////////////\*/
static int Process_rda_bypass_map( short* rda_data_ptr, int msg_sz,
                                   int* lastseg, int seg_num, int tot_segs){

   static int			clutidx			= 0;
   static RDA_bypass_map_msg_t*	clbp_map_data		= NULL;
   static short*		bypass_map		= NULL;
   size_t			msg_size_bytes		= 0;
   int				clutidx_bytes		= 0;
   int				bpm_msg_size		= 0;
   static cd07_bypassmap	bpm_date_time_stamp;


   /* Initialize the last segment flag. */
   *lastseg = 0;

   /* If first segment, allocate memory and store the msg hdr, date and time. */
   if ( seg_num == 1 ){

      /* First segment only.  Allocate space to store the bypass map 
         data structure.  If map pointer is not NULL, free memory 
         associated with it. */
      if( clbp_map_data != NULL )
         free( clbp_map_data );

      if( (clbp_map_data = (RDA_bypass_map_msg_t *) calloc( (size_t) 1,
                     (size_t) sizeof(RDA_bypass_map_msg_t) )) == NULL ){

         LE_send_msg( GL_MEMORY, "Bypass Map calloc Failed\n" );
         return( PR_FAILURE );

      }

      /* Copy the message header for the first segment only. */
      memcpy( &clbp_map_data->msg_hdr, rda_data_ptr, 
              (size_t) sizeof(RDA_RPG_message_header_t) );

      /* Store the bypass map generation date and time */
      clbp_map_data->bypass_map.date = Rda_bypass_map_generation_date;
      clbp_map_data->bypass_map.time = Rda_bypass_map_generation_time;

      /* NOTE: for legacy bpm msgs, the msg from the RDA does NOT contain
         the date/time, only the internal storage structure does.  So we
         can't just cast the data to the structure. The num_segs field
         directly follows the msg hdr in the Legacy RDA msg. */

      /* Treat the map data as an array of shorts. */
      bypass_map = (short *) &clbp_map_data->bypass_map.num_segs;

      /* Since this is the first segment, initialize clutidx. */
      clutidx = 0;
   }

   /* If initial memory allocation failed, return error. */
   if( clbp_map_data == NULL )
      return( PR_FAILURE );

   /* The bypass map data starts after the message header. */
   rda_data_ptr += sizeof(RDA_RPG_message_header_t)/sizeof(short);

   /* Vars that will be used for checking map size and handling the indexing. */
   msg_size_bytes = (size_t) (msg_sz * sizeof(short));
   clutidx_bytes = clutidx * sizeof(short);

   /* If msg will be too big for structure to hold, reject.  Otherwise copy. */
   if ( (clutidx_bytes + msg_size_bytes) <= sizeof( RDA_bypass_map_t ) ){

      /* Move rda data to bypass map table. */
      memcpy( &bypass_map[ clutidx ], rda_data_ptr, msg_size_bytes ); 
      clutidx += msg_sz;

   }
   else{

      /* send msg to status log saying the bypass map is bad and return */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Invalid RDA Bypass Map Received.\n");
      return ( PR_FAILURE );

   }

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Bypass Map Segment %d Received.\n", seg_num );

   /* If this is the last segment of this message, perform the 
      following.... */
   if( seg_num == tot_segs ){

      A304c2 a304c2_data;
      int ret;

      /* Read in the ITC containing bypass_map_request_pending flag.
         Return on error. */
      if( (ret = ORPGDA_read( DATAID_A304C2, (char *) &a304c2_data, 
                              sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

         if( clbp_map_data != NULL ){

            free( clbp_map_data );
            clbp_map_data = NULL;

         }

         LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Read Failed (%d)\n", ret );
         return ( PR_FAILURE );

      }
      else{

         /* If the bp_map_request_pending flag set, ..... */
         if( a304c2_data.bypass_map_request_pending ){

            a304c2_data.bypass_map_request_pending = 0;

            /* Write the ITC.  Return on error. */
            if( (ret = ORPGDA_write( DATAID_A304C2, (char *) &a304c2_data,
                                     sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

               if( clbp_map_data != NULL ){

                  free( clbp_map_data );
                  clbp_map_data = NULL;

               }

               LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Write Failed (%d)\n", ret );
               return ( PR_FAILURE );

            }

         }

      }

      /* Write the Bypass Map data to LB. */
      /* NOTE: we only want to write the required amount of data, not the 
         entire size of the structure (all bypass map elevation segs are
         usually not used).  The 6 below represents the 6 bytes for the 
         date, time, and number of segments. */
      bpm_msg_size = sizeof(RDA_RPG_message_header_t) + 6 +
                     (clbp_map_data->bypass_map.num_segs *
                     sizeof(RDA_bypass_map_segment_t));
                     
      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) clbp_map_data,
                          bpm_msg_size, LBID_BYPASSMAP_LGCY );

      /* Free the memory associated with the Bypass map. */
      if( clbp_map_data != NULL ){

         free( clbp_map_data );
         clbp_map_data = NULL;

      }

      /* Return if Clutter Bypass Map write occurred. */
      if( ret < 0 ){

         LE_send_msg( GL_ORPGDA(ret), "Bypass Map Write Failed (%d)\n", ret );
         return ( PR_FAILURE );

      }

      /* Write the date/time stamp of the current bypass map to linear
         buffer. */
      bpm_date_time_stamp.bm_gendate = Rda_bypass_map_generation_date;
      bpm_date_time_stamp.bm_gentime = Rda_bypass_map_generation_time;

      if( (ret = ORPGDA_write( DATAID_CD07_BYPASSMAP,
                               (char *) &bpm_date_time_stamp,
                               sizeof( cd07_bypassmap ),
                               LBID_CD07_BYPASSMAP )) < 0 ){

         LE_send_msg( GL_ORPGDA(ret), "DATAID_CD07_BYPASSMAP Write Failed (%d)\n",
                      ret );
         return ( PR_FAILURE );

      }

      /* Bypass map received successfully.  Set last segment flag and
         post event. */
      *lastseg = 1; 

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                   "RDA Clutter Filter Bypass Map is Available\n" );
      if( (ret = ES_post_event( ORPGEVT_BYPASS_MAP_RECEIVED, 
                                (char *) NULL,
                                (size_t) 0, 0 )) != ES_SUCCESS )
         return (PR_FAILURE);

   }

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* end Process_rda_bypass_map */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//
//      Transfer incoming data to the ORDA bypass map data structure.  
//      This message comes in multiple segments.
//              
//      If all message segments received successfully, then:
//
//         1) The ITC containing the bypass_map_request_pending flag
//            (DATAID_A304C2) is read, the flag is set, then the ITC 
//            is written.
//
//         2) The Bypass Map message is written to Linear Buffer.
//
//         3) The ITC containing the bypass map generation date/time
//            (DATAID_CD07_BYPASSMAP) is read, the date/time are set,
//            then the ITC is written.
//
//         4) A system status log message is written indicating the 
//            bypass map is available.
//
//         5) The event ORPGEVT_BYPASS_MAP_RECEIVED is posted.
//
//   Inputs:   
//      rda_data_ptr	- Pointer to where bypass map data is stored.
//      msg_sz		- Size of the message, in shorts.
//      lastseg	- Set when last bypass map segment received.
//      seg_num		- Message segment number.
//      tot_segs	- Message total number of segments.
//
//   Outputs:
//
//   Returns:    
//      PR_FAILURE if either memory allocation failed for local 
//      storage, or too many message segments received, or bypass map 
//      write failed, or date/time stamp write failed, or event notification
//      failed.
//
//      Otherwise, PR_SUCCESS is returned.
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
////////////////////////////////////////////////////////////////////\*/
static int Process_orda_bypass_map( short* rda_data_ptr, int msg_sz,
                                    int* lastseg, int seg_num, int tot_segs){

   static int				clutidx			= 0;
   size_t				msg_size_bytes		= 0;
   int					clutidx_bytes		= 0;
   static short*			bypass_map		= NULL;
   ORDA_bypass_map_msg_t*		new_bypass_map		= NULL;
   static ORDA_bypass_map_msg_t*	clbp_map_data		= NULL;


   /* Cast the incoming bypass msg to the appropriate structure */
   new_bypass_map = (ORDA_bypass_map_msg_t *) rda_data_ptr;

   /* Initialize the last segment flag. */
   *lastseg = 0;

   /* If first segment, allocate memory and store the msg hdr, date and time. */
   if ( seg_num == 1 ){

      /* First segment only.  Allocate space to store the bypass map 
         data structure.  If map pointer is not NULL, free memory 
         associated with it. */
      if( clbp_map_data != NULL )
         free( clbp_map_data );

      if( (clbp_map_data = (ORDA_bypass_map_msg_t *) calloc( (size_t) 1,
                     (size_t) sizeof(ORDA_bypass_map_msg_t) )) == NULL ){

         LE_send_msg( GL_MEMORY, "Bypass Map calloc Failed\n" );
         return( PR_FAILURE );

      }

      /* Copy the message header (first segment only). */
      memcpy( &clbp_map_data->msg_hdr, rda_data_ptr, 
              (size_t) sizeof(RDA_RPG_message_header_t) );

      /* Store the bypass map generation date and time (first seg only) */
      clbp_map_data->bypass_map.date = new_bypass_map->bypass_map.date;
      clbp_map_data->bypass_map.time = new_bypass_map->bypass_map.time;

      /* Treat the map data as an array of shorts. */
      bypass_map = (short *) &clbp_map_data->bypass_map;

      /* Since this is the first segment, initialize clutidx. */
      clutidx = 0;

   }

   /* If initial memory allocation failed, return error. */
   if( clbp_map_data == NULL )
      return( PR_FAILURE );

   /* The bypass map data starts after the message header. */
   rda_data_ptr += sizeof(RDA_RPG_message_header_t)/sizeof(short);

   /* Set vars that will be used for checking map size */
   msg_size_bytes = (size_t) (msg_sz * sizeof(short));
   clutidx_bytes = clutidx * sizeof(short);

   /* If msg will be too big for structure to hold, reject.  Otherwise copy. */
   if ( (clutidx_bytes + msg_size_bytes) <= sizeof( ORDA_bypass_map_t ) ){

      /* Move orda data to bypass map table. */
      memcpy(&bypass_map[clutidx], rda_data_ptr, msg_size_bytes); 
      clutidx += msg_sz;

   }
   else{

      /* send msg to status log saying the orda bypass map is bad and return */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Invalid RDA Bypass Map Received.\n");
      return ( PR_FAILURE );

   }

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Bypass Map Segment %d Received.\n", seg_num );

   /* If this is the last segment of this message, perform the following.... */
   if( seg_num == tot_segs ){

      A304c2 a304c2_data;
      int ret;

      /* Read in the ITC containing bypass_map_request_pending flag.
         Return on error. */
      if( (ret = ORPGDA_read( DATAID_A304C2, (char *) &a304c2_data, 
                              sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

         if( clbp_map_data != NULL ){

            free( clbp_map_data );
            clbp_map_data = NULL;

         }

         LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Read Failed (%d)\n", ret );
         return ( PR_FAILURE );

      }
      else{

         /* If the bp_map_request_pending flag set, ..... */
         if( a304c2_data.bypass_map_request_pending ){

            a304c2_data.bypass_map_request_pending = 0;

            /* Write the ITC.  Return on error. */
            if( (ret = ORPGDA_write( DATAID_A304C2, (char *) &a304c2_data,
                                     sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

               if( clbp_map_data != NULL ){

                  free( clbp_map_data );
                  clbp_map_data = NULL;

               }

               LE_send_msg( GL_ORPGDA(ret),
                  "DATAID_A304C2 Write Failed (%d)\n", ret );
               return ( PR_FAILURE );

            }

         }

      }

      /* Write the Bypass Map data to LB. */
      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) clbp_map_data,
                          sizeof( ORDA_bypass_map_msg_t ), LBID_BYPASSMAP_ORDA );

      /* Free the memory associated with the Bypass map. */
      if( clbp_map_data != NULL ){

         free( clbp_map_data );
         clbp_map_data = NULL;

      }

      /* Return if Clutter Bypass Map write occurred. */
      if( ret < 0 ){

         LE_send_msg( GL_ORPGDA(ret), "Bypass Map Write Failed (%d)\n", ret );
         return ( PR_FAILURE );

      }

      /* Bypass map received successfully. Set last seg flag and post event. */
      *lastseg = 1; 

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                   "RDA Clutter Filter Bypass Map is Available\n" );
      if ( (ret = ES_post_event( ORPGEVT_BYPASS_MAP_RECEIVED, (char *) NULL,
         (size_t) 0, 0 )) != ES_SUCCESS )
         return (PR_FAILURE);
      
   }

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* end Process_orda_bypass_map */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Transfer incoming data to the legacy RDA clutter map data
//	structure.  This message comes in multiple segments.
//              
//      If all message segments received successfully, then:
//
//         1) The ITC containing the nw_map_request_pending flag and
//            the unsolicited_nw_received flag (DATAID_A304C2) is 
//            read.  If there was an outstanding request for the map,
//            the unsolicited_nw_received and nw_map_request_pending 
//            flags are both cleared, otherwise unsolicited_nw_received
//            flag is set. The ITC is then written.
//
//         2) The Notchwidth Map message is written to Linear Buffer.
//
//         3) A system status log message is written indicating the
//            clutter map is available.
//
//         4) The event ORPGEVT_CLUTTER_MAP_RECEIVED is posted.
//
//   Inputs:   
//      rda_data_ptr	- Pointer to where clutter map data is stored.
//      msg_sz		- Size of the message, in shorts.
//      lastseg		- Set when last clutter map segment received.
//      seg_num		- Message segment number.
//      tot_segs	- Message total number of segments.
//
//   Outputs:
//
//   Returns:
//      PR_FAILURE if either memory allocation failed for local 
//      storage, or too many message segments received, or clutter 
//      map write failed, or date/time stamp write failed, or event 
//      notification failed.
//
//      Otherwise, PR_SUCCESS is returned.
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
////////////////////////////////////////////////////////////////////\*/
static int Process_rda_clutter_map( short* rda_data_ptr, int msg_sz,
                                    int* lastseg, int seg_num, int tot_segs){

   RDA_notch_map_msg_t* 	notchwidth_map		= NULL;	
   static RDA_notch_map_msg_t*	clnw_map_data		= NULL;
   static short*		notch_map		= NULL;
   static int			clutidx			= 0;
   size_t			msg_size_bytes		= 0;
   int				clutidx_bytes		= 0;


   /* Get the segment number and total number of segments. */
   notchwidth_map = (RDA_notch_map_msg_t *) rda_data_ptr;

   /* Initialize the last segment flag. */
   *lastseg = 0;

   /* Extract the generation date and time from the RDA message data
      (First segment only). */
   if ( seg_num == 1 ){

      /* Allocate space to store the notchwidth map data structure. If
         map pointer is not NULL, free memory associated with it. */
      if( clnw_map_data != NULL )
         free( clnw_map_data );

      if( (clnw_map_data = (RDA_notch_map_msg_t *) calloc( (size_t) 1,
         (size_t) sizeof(RDA_notch_map_msg_t) )) == NULL ){

         LE_send_msg( GL_MEMORY, "Clutter Filter Map calloc Failed\n" );
         return ( PR_FAILURE );

      }

      /* Copy the message header. */
      memcpy( &clnw_map_data->msg_hdr, rda_data_ptr, 
              (size_t) sizeof(RDA_RPG_message_header_t) );

      /* Extract the notchwidth map generation date and time. */
      clnw_map_data->notchmap.date = notchwidth_map->notchmap.date;
      clnw_map_data->notchmap.time = notchwidth_map->notchmap.time;

      /* Treat the map data as an array of shorts. */
      notch_map = (short *) &clnw_map_data->notchmap;
         
      /* Initialize index into notchwidth map table for this segment */
      clutidx = 0;

   }
   else{

      /* If memory allocation failed for the first segment, can not
         process any subsequent segments. */
      if( clnw_map_data == NULL )
         return ( PR_FAILURE );
      
   }

   if( CR_verbose_mode )
      LE_send_msg(GL_INFO, "Clutter Filter Map Segment %d Received.\n",seg_num);

   /* Adjust the start position of the rda data by the size of the 
      message header. */
   rda_data_ptr += sizeof(RDA_RPG_message_header_t)/sizeof(short);

   /* Set vars that will be used for checking map size */
   msg_size_bytes = (size_t) (msg_sz * sizeof(short));
   clutidx_bytes = clutidx * sizeof(short);

   /* If msg will be too big for structure to hold, reject.  Otherwise copy. */
   if ( (clutidx_bytes + msg_size_bytes) <= sizeof( RDA_notch_map_t ) )
   {
      /* Move rda data to notchwidth map table. */
      memcpy( &notch_map[ clutidx ], rda_data_ptr, msg_size_bytes );
      clutidx += msg_sz;
   }
   else
   {
      /* send msg to log saying the rda notchwidth map is bad and return */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                   "Invalid RDA Clutter Filter Notchwidth Map Received.\n");
      return ( PR_FAILURE );
   }

   /* If the last segment received, perform the following.... */
   if( seg_num == tot_segs )
   {
      A304c2 a304c2_data;
      int ret;

      /* Read in the ITC containing nw_map_request_pending flag.
         Return on error. */
      if( (ret = ORPGDA_read( DATAID_A304C2, (char *) &a304c2_data, 
                              sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

         if( clnw_map_data != NULL ){

            free( clnw_map_data );
            clnw_map_data = NULL;

         }

         LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Read Failed\n" );
         return ( PR_FAILURE );

      }
      else{

         /* If the nw_map_request_pending flag set, map was solicited. */
         if( a304c2_data.nw_map_request_pending ){

            a304c2_data.unsolicited_nw_received = 0;
            a304c2_data.nw_map_request_pending = 0;

         }
         else{
 
            /* Map was automatically sent to RPG by the RDASC. */
            a304c2_data.unsolicited_nw_received = 1;

         }

         /* Write the ITC.  Return on error. */
         if( (ret = ORPGDA_write( DATAID_A304C2, (char *) &a304c2_data,
                                  sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

            if( clnw_map_data != NULL ){

               free( clnw_map_data );
               clnw_map_data = NULL;

            }

            LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Write Failed (%d)\n", ret );
            return ( PR_FAILURE );

         }

      }

      /* Write the Notchwidth Map data to LB. */
      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) clnw_map_data, 
                          sizeof( RDA_notch_map_msg_t ), LBID_CLUTTERMAP_LGCY );

      /* Free the memory associated with the Clutter Map. */
      if( clnw_map_data != NULL ){

         free( clnw_map_data );
         clnw_map_data = NULL;

      }

      /* Exit on Clutter Map write error. */
      if( ret < 0 ){

         LE_send_msg( GL_ORPGDA(ret), "Clutter Map Write Failed (%d)\n", ret );
         return ( PR_FAILURE );

      }

      /* Notchwidth map received successfully.  Set last segment flag and
         post event. */
      *lastseg = 1;

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                   "RDA Clutter Filter Notchwidth Map is Available\n" );
      if( (ret = ES_post_event( ORPGEVT_CLUTTER_MAP_RECEIVED, (char *) NULL,
                                (size_t) 0, 0 )) != ES_SUCCESS )
         return (PR_FAILURE);
      
   }

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* end Process_rda_clutter_map */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Transfer incoming data to the Open RDA Clutter Map data
//	structure.  This message comes in multiple segments.
//              
//      If all message segments received successfully, then:
//
//         1) The ITC containing the nw_map_request_pending flag and
//            the unsolicited_nw_received flag (DATAID_A304C2) is 
//            read.  If there was an outstanding request for the map,
//            the unsolicited_nw_received and nw_map_request_pending 
//            flags are both cleared, otherwise unsolicited_nw_received
//            flag is set. The ITC is then written.
//
//         2) The Clutter Map message is written to Linear Buffer.
//
//         3) A system status log message is written indicating the
//            Clutter Map is available.
//
//         4) The event ORPGEVT_CLUTTER_MAP_RECEIVED is posted.
//
//   Inputs:   
//      rda_data_ptr	- Pointer to clutter map data including msg hdr.
//      msg_sz		- Size of the message, in shorts, not including
//                        message header.
//      lastseg		- Set when last clutter map segment is received.
//      seg_num		- Message segment number.
//      tot_segs	- Message total number of segments.
//
//   Outputs:
//
//   Returns:
//      PR_FAILURE if either memory allocation failed for local 
//      storage, or too many message segments received, or clutter 
//      map write failed, or date/time stamp write failed, or event 
//      notification failed.
//
//      Otherwise, PR_SUCCESS is returned.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//
//   Notes:         
//      - The ORDA Clutter Filter Map is variable length.  Not all
//        range zones must be defined.  Therefore, a fixed sized
//        structure is not used here.  The data is simply pieced 
//        together from the individual message segments and written
//        to the ORPGDAT_CLUTTERMAP LB.
//      - All global variables are defined and described in 
//        crda_control_rda.h.  These will begin with CR_.  All file 
//        scope global variables are defined and described at the 
//        top of the file.
////////////////////////////////////////////////////////////////////\*/
static int Process_orda_clutter_map( short* rda_data_ptr, int msg_sz,
                                     int* lastseg, int seg_num, int tot_segs){

   RDA_RPG_message_header_t*	msg_hdr		   = NULL;
   static short*		clut_map	   = NULL;
   static size_t		max_msg_size_shorts = 0;
   static size_t		max_msg_size_bytes = 0;
   static size_t		msg_size_bytes	   = 0;
   static int			clutidx		   = 0;
   int				clutidx_bytes	   = 0;


   /* Get the segment number and total number of segments. */
   msg_hdr = (RDA_RPG_message_header_t *) rda_data_ptr;

   /* Initialize the last segment flag. */
   *lastseg = 0;

   /* Extract the generation date and time from the ORDA message data
      (First segment only). */
   if ( seg_num == 1 ){

      /* Determine the total size of the message from the number of
         segments defined in the msg hdr. */
      max_msg_size_shorts = (size_t)(((msg_hdr->size - MSGHDRSZ) *
         msg_hdr->num_segs) + MSGHDRSZ);
      max_msg_size_bytes = max_msg_size_shorts * sizeof(short);

      /* Allocate space to store the clutter map data. If
         map pointer is not NULL, free memory associated with it. */
      if( clut_map != NULL )
         free( clut_map );

      if( (clut_map = (short *) calloc( max_msg_size_shorts,
                                        (size_t) sizeof(short))) == NULL ){

         LE_send_msg( GL_ERROR, "Clutter Map calloc Failed\n" );
         return ( PR_FAILURE );

      }

      /* Copy the message header into the memory buffer. */
      memcpy( clut_map, rda_data_ptr, (size_t)sizeof(RDA_RPG_message_header_t));

      /* Initialize index into clutter map table for this segment.  Adjust for
         the size of the msg hdr.*/
      clutidx = MSGHDRSZ;

   }
   else{

      /* If memory allocation failed for the first segment, can not
         process any subsequent segments. */
      if( clut_map == NULL )
         return ( PR_FAILURE );
      
   }

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Clutter Map Segment %d Received.\n", seg_num );

   /* Adjust the start position of the rda data by the size of the 
      message header. */
   rda_data_ptr += sizeof(RDA_RPG_message_header_t)/sizeof(short);

   /* Set vars that will be used for checking map size */
   msg_size_bytes = (size_t) (msg_sz * sizeof(short));
   clutidx_bytes = clutidx * sizeof(short);

   /* If msg will be too big for structure to hold, reject.  Otherwise copy. */
   if ( (clutidx_bytes + msg_size_bytes) <= max_msg_size_bytes ){

      /* Copy clutter map data to memory buffer. */
      memcpy(&clut_map[clutidx], rda_data_ptr, msg_size_bytes);
      clutidx += msg_sz;
      clutidx_bytes = clutidx_bytes + msg_size_bytes;

   }
   else{

      /* send msg to log saying the rda clutter map is bad and return */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Invalid RDA Clutter Map Received.\n");
      return ( PR_FAILURE );

   }

   /* If the last segment received, perform the following.... */
   if( seg_num == tot_segs ){

      A304c2 a304c2_data;
      int ret;

      /* Read in the ITC containing nw_map_request_pending flag.
         Return on error. */
      if( (ret = ORPGDA_read( DATAID_A304C2, (char *) &a304c2_data, 
                              sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

         /* free allocated memory buffer */
         if( clut_map != NULL ){

            free( clut_map );
            clut_map = NULL;

         }

         LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Read Failed\n" );
         return ( PR_FAILURE );

      }
      else{

         /* If the nw_map_request_pending flag set, map was solicited. */
         if( a304c2_data.nw_map_request_pending ){

            a304c2_data.unsolicited_nw_received = 0;
            a304c2_data.nw_map_request_pending = 0;

         }
         else{ 

            /* Map was automatically sent to RPG by the RDASC. */
            a304c2_data.unsolicited_nw_received = 1;

         }

         /* Write the ITC.  Return on error. */
         if( (ret = ORPGDA_write( DATAID_A304C2, (char *) &a304c2_data,
                                  sizeof( A304c2 ), LBID_A304C2 )) < 0 ){

            if( clut_map != NULL ){

               free( clut_map );
               clut_map = NULL;

            }

            LE_send_msg( GL_ORPGDA(ret), "DATAID_A304C2 Write Failed (%d)\n", ret );
            return ( PR_FAILURE );

         }

      }

      /* Write the clutter map data to CLUTTERMAP LB. */
      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) clut_map,
                          clutidx_bytes, LBID_CLUTTERMAP_ORDA );

      /* Free the memory associated with the Clutter Map. */
      if( clut_map != NULL ){

         free( clut_map );
         clut_map = NULL;

      }

      /* Exit on Clutter Map write error. */
      if( ret < 0 ){

         LE_send_msg( GL_ORPGDA(ret), "Clutter Map Write Failed (%d)\n", ret );
         return ( PR_FAILURE );

      }

      /* Clutter Map received successfully.  Set last segment flag and
         post event. */
      *lastseg = 1;

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "RDA Clutter Map is Available\n" );
      if( (ret = ES_post_event( ORPGEVT_CLUTTER_MAP_RECEIVED, (char *) NULL,
                                (size_t) 0, 0 )) != ES_SUCCESS )
         return (PR_FAILURE);
      
   }

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* end Process_orda_clutter_map */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Transfer incoming data to the RDA adaptation data LB. This
//	message comes in multiple segments.
//              
//      If all message segments received successfully, then:
//
//         1) The Adaptation Data message is written to Linear Buffer.
//
//         2) A system status log message is written indicating the
//            adaptation data is available.
//
//   Inputs:   
//      rda_data_ptr	- Pointer to adaptation data, including msg hdr.
//      msg_sz          - Size of the message, in shorts, not incl hdr.
//      lastseg		- Set when last msg segment received.
//      seg_num		- Message segment number.
//      tot_segs	- Message total number of segments.
//
//   Outputs:
//
//   Returns:
//      PR_FAILURE if either memory allocation failed for local 
//      storage, or too many message segments received, or msg write
//	failed, or date/time stamp write failed, or event 
//      notification failed.
//
//      Otherwise, PR_SUCCESS is returned.
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
////////////////////////////////////////////////////////////////////\*/
static int Process_rda_adapt_data( short* rda_data_ptr, int msg_sz,
   int* lastseg, int seg_num, int tot_segs)
{
   static short*		adapt_data		= NULL;
   static short			adaptidx		= 0;
   static size_t		max_msg_size_shorts	= 0;
   size_t			msg_size_bytes		= 0;
   int				adaptidx_bytes		= 0;


   /* Initialize the last segment flag. */
   *lastseg = 0;

   if ( seg_num == 1 ){

      /* Allocate space to store the adaptation data. Need to allocate enough
         space to hold the msg hdr and all the data.  If pointer is not NULL,
         free memory associated with it. */
      if( adapt_data != NULL )
         free( adapt_data );

      max_msg_size_shorts = (size_t)((msg_sz * tot_segs) + MSGHDRSZ);
      if ((adapt_data = (short *) calloc( max_msg_size_shorts,
                                          (size_t) sizeof( short ))) == NULL ){

         LE_send_msg( GL_MEMORY, "Adaptation Data calloc Failed\n" );
         return ( PR_FAILURE );

      }

      /* Copy the message header into the buffer. */
      memcpy( &adapt_data[0], rda_data_ptr,
              (size_t) sizeof(RDA_RPG_message_header_t) );

      /* Initialize index into adaptation data msg for this segment */
      adaptidx = sizeof( RDA_RPG_message_header_t ) / sizeof( short );

   }
   else{

      /* If memory allocation failed for the first segment, can not
         process any subsequent segments. */
      if( adapt_data == NULL )
         return ( PR_FAILURE );
      
   }

   if( CR_verbose_mode )
      LE_send_msg(GL_INFO,"Adaptation Data Msg Segment %d Received.\n", seg_num);

   /* Adjust the rda data pointer past the message header. */
   rda_data_ptr += sizeof(RDA_RPG_message_header_t)/sizeof(short);

   /* Set vars that will be used for checking adapt msg size */
   msg_size_bytes = (size_t) (msg_sz * sizeof(short));
   adaptidx_bytes = adaptidx * sizeof(short);

   /* If msg will be too big for buffer to hold, reject.  Otherwise copy. */
   if ( (adaptidx_bytes + msg_size_bytes) <= (max_msg_size_shorts * sizeof(short)) ){

      /* Move rda data to adapt data buffer */
      memcpy( &adapt_data[ adaptidx ], rda_data_ptr, msg_size_bytes );
      adaptidx += msg_sz;
      adaptidx_bytes = adaptidx_bytes + msg_size_bytes;

   }
   else{

      /* send msg to log saying the rda adaptation data is bad and return */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                   "Invalid RDA Adaptation Data Msg Received.\n");
      return ( PR_FAILURE );

   }

   /* If the last segment received, perform the following.... */
   if( seg_num == tot_segs ){

      int ret;

      /* Convert the adapt message data from external format to internal
         format (i.e., if the RDA is Big-Endian and the RPG is Little-Endian
         (or vice versa). */
      ret = UMC_RDAtoRPG_message_convert_to_internal(ADAPTATION_DATA,(char*) adapt_data);
      if( ret <= 0 ){

         /* If the UMC conversion fails, we don't want to write Adaptation Data because it
            may be corrupted. */
         LE_send_msg( GL_ERROR, 
                      "UMC_RDAtoRPG_message_convert_to_internal(ADAPTATION_DATA) Failed\n" );

      }
      else {

         /* Write the Adaptation Data to the LB. */
         ret = ORPGDA_write( ORPGDAT_RDA_ADAPT_DATA, (char *) adapt_data,
                             adaptidx_bytes, ORPGDAT_RDA_ADAPT_MSG_ID );

      }

      /* Free the memory associated with the Adapt Data. */
      if( adapt_data != NULL ){

         free( adapt_data );
         adapt_data = NULL;

      }

      /* Exit on error.  If UMC_ failed, 0 is returned.  If the write does not
         succeed, a non-positive error is returned. */
      if( ret <= 0 ){

         LE_send_msg(GL_ERROR, "RDA Adaptation Data Write Failed (%d)\n", ret);
         return ( PR_FAILURE );

      }

      /* Adaptation data received successfully.  Set last segment flag and
         post event. */
      *lastseg = 1;

      LE_send_msg(GL_STATUS | LE_RPG_GEN_STATUS, "RDA Adaptation Data is Available\n" );

   }

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* end Process_rda_adapt_data */


/*\////////////////////////////////////////////////////////////////////
// 
//   Description:    
//	The VCP message is written to Linear Buffer.  A system status
//	log message is written indicating the VCP data is available.
//
//   Inputs:    
//      rda_data - pointer to RDA VCP data, including msg hdr.
//
//   Outputs:
//	The RDA VCP data is stored in the RDA VCP LB.
//
//   Returns:     
//	Integer equal to PR_SUCCESS on success, PR_FAILURE upon failure.
//
//   Globals:
//
//   Notes:         
//	This is a variable length message depending on the VCP.  We
//	cannot cast directly to the structure before writing to the 
//	LB.  Rather we have to interrogate the msg hdr to get the 
//	actual msg size.
////////////////////////////////////////////////////////////////////\*/
int PR_process_rda_vcp_data( short *rda_data ){

   int				ret		= 0;
   int				msg_size_bytes	= 0;
   RDA_RPG_message_header_t*	msg_hdr		= NULL;

   /* Cast the incoming data to the msg hdr structure to retrieve the
      msg size. Note: the size in the msg hdr is in shorts and it
      includes the msg hdr size. */
   msg_hdr = (RDA_RPG_message_header_t *) rda_data; 
   msg_size_bytes = msg_hdr->size * sizeof(short);

   /* Write the VCP Data to the LB. */
   ret = ORPGDA_write( ORPGDAT_RDA_VCP_DATA, (char*) rda_data,
      msg_size_bytes, ORPGDAT_RDA_VCP_MSG_ID );

   /* Exit on write error. */
   if( ret < 0 ){

      LE_send_msg(GL_ERROR, "RDA VCP Data Write Failed (%d)\n", ret);
      return ( PR_FAILURE );

   }

   /* Set the Previous State VCP definition. */
   
   ORPGRDA_set_state_vcp( (char *) ((char *) rda_data + sizeof(RDA_RPG_message_header_t)),
                          msg_size_bytes-sizeof(RDA_RPG_message_header_t) );

   /* Return Normal. */
   return ( PR_SUCCESS );

} /* end PR_process_rda_vcp_data() */
