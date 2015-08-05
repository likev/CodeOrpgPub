/**************************************************************************
   
   Module:  otr_process_onetime_requests.c
   
   Description: 
   This processes the one time request message.  The message is fed in as
   received from the request lb.  There may be more than one request in the
   message.  If the request is for latest product, then the product database is
   queried to find if a product has been generated.  If it's there, 
   then a reply is made with the LB and message.  If it's not there, 
   then a reply is made with the error set to the reason the product could
   not be found. If the request was for a CFC or USP product from a dial-in
   line, then this product is scheduled on the realtime stream, to make the
   product available the next time it is requested.
   
   If the request is for current and future products, then the request is
   passed to a routine that handles this kind of request.
   
   If the request is for a specific time, then the request is passed to
   a routine that will find the product if it was generated at that time.
   If it's there, then a reply is made with
   the product LB and message number.  If it's not, a reply is made
   with the error "product not available".
   
   
   Assumptions:
   It is assumed that the message header and requests have been validated
   before this routine is called.
   
   **************************************************************************/
/*
* RCS info
* $Author: steves $
* $Locker:  $
* $Date: 2012/07/19 22:18:48 $
* $Id: otr_process_onetime_requests.c,v 1.43 2012/07/19 22:18:48 steves Exp $
* $Revision: 1.43 $
* $State: Exp $
*/

/* System Include Files/Local Include Files */
#include <prod_user_msg.h>
#include <orpg.h>
#include <otr_find_product.h>
#include <otr_reply_to_request.h>
#include <otr_process_onetime_requests.h>
#include <otr_new_volume_product_requests.h>
#include <le.h>
#include <mrpg.h>
#include <itc.h>
#include <a309.h>
#include <rpg_vcp.h>
#include <rpgc.h>

/* Constant Definitions/Macro Definitions/Type Definitions */

/* constant to signal p_server that no more replies will be sent
   to this request */
#define END_OF_REPLIES        1

/* paramter subscript for the end time for user selectable precip products */
#define USP_END_TIME          0

/* the value of the last update time (for precip) when no update has occurred */
#define HRDB_INIT_VALUE      -2

/* the various types of how a product can be requested, on a one-time basis. */
#define CURRENT_PRODUCT      -1
#define LATEST_AVAILABLE     -2
#define SPECIFIC_DATE_TIME    0

/* Global variables */
extern long last_volume_time; /* seconds past midnight for start of last volume */
extern unsigned long Vol_number_in_rda_stat; /* Volume scan number. */
Hrdb_date_time hrdb_data = {-1, -1}; /* USP itc data gets written here */
extern Pd_general_status_msg Cr_gsm;

/* Static Function Prototypes */
static void Service_latest_available_request( Pd_request_products *request_ptr,
                                              int p_server_id,
                                              Pd_msg_header *message_header_ptr );
static void Service_specific_time_request( Pd_request_products *request_ptr,
                                           int p_server_id,
                                           Pd_msg_header *message_header_ptr );
static void Service_current_product_request( Pd_request_products *request_ptr,
                                             int p_server_id, 
                                             Pd_msg_header *message_header_ptr );      
static int Special_request_processing( Pd_request_products *request_ptr, 
                                       int p_server_id,
                                       Pd_msg_header *message_header_ptr,
                                       int *modifed_parameter, 
                                       int *error_flag,
                                       int type );
static int Special_reply_processing( Pd_request_products *request_ptr,
                                     int p_server_id,
                                     Pd_msg_header *message_header_ptr,
                                     int error_flag );

/**************************************************************************
   Description: see above.

   Input: 

   Arguments: A message containing requests, that is a message header
              followed by the request message.
              The ID of the product server that sent the message.

   Output: (by subroutines) replies to the reply lb.

   Returns: void

   Notes:

**************************************************************************/
void OTR_process_onetime_requests( Pd_msg_header  *message_header_ptr,
                                   int p_server_id ){

   Pd_request_products *request_ptr; /* pointer to the request 
					within the message */
 
   LE_send_msg(GL_INFO | LE_VL3, "Process Onetime Request(s) from User %d\n",
               message_header_ptr->src_id);
      
   /* point to the first request */
   request_ptr = (Pd_request_products *)(((char *) message_header_ptr) 
					 + sizeof(Pd_msg_header));
   /* do for every request in the message */					 
   while (message_header_ptr->n_blocks > 1){

      message_header_ptr->n_blocks--;
      
      LE_send_msg (GL_INFO | LE_VL3,
            "--->Request prod_id: %d, flag bits: %d, num_prod: %d, volume time: %d \n",
            request_ptr->prod_id,  request_ptr->flag_bits, request_ptr->num_products, 
            request_ptr->VS_start_time);
          
      /* if the requested number of products is -1, then the request is for routine 
         product generation, not one-time and it will be ignored. */
      if (request_ptr->num_products > 0){

	 /* the start time will be -1 for a current product request, and -2 for the 
            latest available product request. */
	 if (request_ptr->VS_start_time == LATEST_AVAILABLE){

            Service_latest_available_request( request_ptr, p_server_id,
                                              message_header_ptr );

         }	    
	 else if (request_ptr->VS_start_time == CURRENT_PRODUCT){

	    /* request is for current or next product, add this to the current 
               volume requests */
	    Service_current_product_request( request_ptr, p_server_id, 
                                             message_header_ptr );   

	 } 
	 else {

	    /* find the product in the product database LB for a specific time */
            Service_specific_time_request( request_ptr, p_server_id, 
                                           message_header_ptr );
	       
	 }  /* endif latest, current, or next product, or specific time */

      }  /* endif test for one-time request */
      
      request_ptr++; /* point to next request */
      
   } /* end do for each request after a header */   

} /* End of OTR_process_onetime_requests() */


/**************************************************************************
   Description: Handles all requests for LATEST_AVAILABLE volume time.

   Input: 

   Arguments: request_ptr - A request message.
              p_server_id - The ID of the product server that sent the message.
              message_header_ptr - A pointer to the message header of the request.

   Output: (by subroutines) replies to the reply lb.

   Returns: void

   Notes:

**************************************************************************/
static void Service_latest_available_request( Pd_request_products *request_ptr, 
                                              int p_server_id,
                                              Pd_msg_header *message_header_ptr ){
	    
   /* request was for latest available product find the product in the product 
      database LB */
   int volume_number;
   int status;
   int replied = 0;
   int error_flag = OTR_PRODUCT_READY;
   int parameter_modified = -1, modified_parameter = PARAM_UNUSED;
   int lb_return;   /* LB number found to match the product request */
   int msg_return;  /* message ID number found to match the product request */
   int found = 0;   /* non-zero means a product was found matching the request */
   int task_status; /* holds product generation task status */
        
   /* Is there special processing for this request? */
   parameter_modified = Special_request_processing( request_ptr, p_server_id,
                                                    message_header_ptr, 
                                                    &modified_parameter,
                                                    &error_flag, LATEST_AVAILABLE );
	    
   /* If Special_request_processing did not find an error (e.gi., invalid parameter), 
      search for product in product database. */
   if( error_flag == OTR_PRODUCT_READY ){

      found = OTR_find_product(request_ptr, &lb_return, &msg_return, &volume_number);
      if (parameter_modified >= 0){

         /* set the request parameter back for subsequent processing */
         request_ptr->params[parameter_modified ] = modified_parameter;

      }

   }	

   /* If match found, then tell the product server the product is available for 
      distribution. */	     
   if (found > 0){

      LE_send_msg( GL_INFO | LE_VL2, "--->Prod History Found In Database (msg id %d)\n", 
	           msg_return );    
	       
      OTR_reply_to_request(p_server_id, request_ptr, message_header_ptr, volume_number, 
		    lb_return, msg_return, OTR_PRODUCT_READY, END_OF_REPLIES);
   }
   else{

      /* could not find a generated product or other error occurred.  send the error
         back to requestor */

      /* if error to return is based on the generation task state. assume generation 
         is always the zeroth instance */
      status = ORPGMGR_get_task_status( ORPGPAT_get_gen_task(request_ptr->prod_id),
                                        0, &task_status );
      if( status < 0 )
         error_flag = OTR_PRODUCT_NOT_AVAILABLE;

      else{

         if (task_status == MRPG_PS_ACTIVE){

            /* If task is active and there were not other problems with the
               request (other than the product was not found), return
               "product_not_available". */
            if( error_flag == OTR_PRODUCT_READY )
	       error_flag = OTR_PRODUCT_NOT_AVAILABLE;

         }
	 else if (task_status == MRPG_PS_NOT_STARTED)
	    error_flag = OTR_TASK_NOT_STARTED;

	 else
	    error_flag = OTR_TASK_FAILED;

      }          

      LE_send_msg( GL_INFO | LE_VL2, "--->Could Not Satisfy Latest Request (%d)\n",
                   error_flag );
      if( (replied = Special_reply_processing( request_ptr, p_server_id,
                                               message_header_ptr, error_flag )) == 0 )
         OTR_reply_to_request( p_server_id, request_ptr, message_header_ptr, 0,
			       request_ptr->prod_id, 0, error_flag, END_OF_REPLIES );

   } /* end if found find a product */

}


/**************************************************************************
   Description: Handles any special processing for product requests.

   Input: 

   Arguments: A message containing requests, that is a message header
              followed by the request message.
              The type of request.

   Output:  The value of any parameter modified by this module.
            An error flag if error occurred.

   Returns: The parameter modified or -1 if no parameter modified

   Notes:

**************************************************************************/
static int Special_request_processing( Pd_request_products *request_ptr, 
                                       int p_server_id,
                                       Pd_msg_header *message_header_ptr,
                                       int *modified_parameter, 
                                       int *error_flag,
                                       int type ){
 
   int parameter_modified = -1;
   int usp_itc_id = ITC_CD07_USP;
   int itc_read_status = 0;
   int elev_param;

   /* Do any special product dependent processing. */
   switch( request_ptr->prod_id ){

      /* The USP product. */
      case HYUSPACC:
      {

         /* Special request processing for USP product. */
         if( (type != CURRENT_PRODUCT) && (request_ptr->params[USP_END_TIME ] == -1) ){

            /* For specific date/time and end_time the current hour, 
               set to current hour. */
            if( type == SPECIFIC_DATE_TIME ){

               if (last_volume_time != -1) /* hour of last volume scan */
                  request_ptr->params[USP_END_TIME ] = last_volume_time/3600; 

               return(parameter_modified);

            }

            /* If the end time parameter is -1, it means to use the last database update
               time.  We put the real value from the ITC in here so that comparison 
               of products will work and the products can be found.  */          
            parameter_modified = USP_END_TIME;
            *modified_parameter = -1;

            LE_send_msg( GL_INFO | LE_VL3, "Special Request Processing for USP Product\n" );

            /* Get the current end time from the ITC. */
            RPGC_itc_read (usp_itc_id, &itc_read_status);
            if( (itc_read_status== NORMAL) 
                         && 
                (hrdb_data.last_time_hrdb >= 0) 
                         && 
                (hrdb_data.last_time_hrdb <= 24)){

               request_ptr->params[USP_END_TIME ] = hrdb_data.last_time_hrdb;
               LE_send_msg( GL_INFO | LE_VL3, "--->Date: %d, Hour: %d \n", 
                            hrdb_data.last_date_hrdb, hrdb_data.last_time_hrdb );
               LE_send_msg( GL_INFO | LE_VL3, "--->Request for USP End Time Set To %d \n",
                            request_ptr->params[USP_END_TIME] );

            }
            else{

               LE_send_msg( GL_INFO | LE_VL3,"--->Could Not Get ITC_CD07_USP (%d) Hour: %d \n", 
                            itc_read_status, hrdb_data.last_time_hrdb);

               /* System just starting, so last update is not available
                  so accept whatever the latest product has */
               request_ptr->params[USP_END_TIME ] = PARAM_ANY_VALUE;

               LE_send_msg( GL_INFO | LE_VL3, "--->Request for USP End Time Set To %d \n",
                            PARAM_ANY_VALUE );

            }

         }

         break;

      }

      /* Free text message. */
      case FTXTMSG:
      {

         if( type == CURRENT_PRODUCT ){

            int sent_message = 0;
            int last_flag = 0;
            int volume_number = 0;

            /* If product is already generated, send reply. */
            sent_message = OTR_check_product_status( p_server_id, 
                                                     message_header_ptr,
                                                     request_ptr, &last_flag, 
                                                     &volume_number );

            /* reply sent. */
            if( sent_message )
               break;

         }

         *error_flag = OTR_PRODUCT_NOT_AVAILABLE;

         break;

      }

      /* The USW or USD product. */
      case USWACCUM:
      case USDACCUM:
      {

         /* Special request processing for USD/USW product. */
         if( (type != CURRENT_PRODUCT) && (request_ptr->params[USP_END_TIME ] == -1) ){

            /* For specific date/time and end_time the current hour, 
               set to current hour. */
            if( type == SPECIFIC_DATE_TIME ){

               if (last_volume_time != -1) /* hour of last volume scan */
                  request_ptr->params[USP_END_TIME ] = last_volume_time/3600; 

               return(parameter_modified);

            }

            /* If the end time parameter is -1, it means to use the last volume time. */
            parameter_modified = USP_END_TIME;
            *modified_parameter = -1;

            LE_send_msg( GL_INFO | LE_VL3, "Special Request Processing for USD/USW Product\n" );
            request_ptr->params[USP_END_TIME ] = last_volume_time/3600;

            LE_send_msg( GL_INFO | LE_VL3, "--->Request for USD/USW End Time Set To %d\n",
                         request_ptr->params[USP_END_TIME] );

         }

         break;

      }

      default:
      {

         /* Check if product has an elevation parameter. */
         if( (elev_param = ORPGPAT_elevation_based( request_ptr->prod_id )) >= 0 ){

            if( request_ptr->params[elev_param] & ORPGPRQ_ELEV_FLAG_BITS ){

               if( (type == LATEST_AVAILABLE) || (type == SPECIFIC_DATE_TIME) )
                  *error_flag = OTR_INVALID_PARAMS;

               /* NOTE: For CURRENT_PRODUCT, the special request handling is done
                        in the Current Product Service routine. */
            }

         }

         break;

      }

   /* End of "switch" statement. */
   }

   return( parameter_modified );

}



/**************************************************************************
   Description: Handles all requests for specific volume scan time.

   Input: 

   Arguments: A message containing requests, that is a message header
              followed by the request message.
              The ID of the product server that sent the message.
              A pointer to the message header of the request.

   Output: (by subroutines) replies to the reply lb.

   Returns: void

   Notes:
**************************************************************************/
static void Service_current_product_request( Pd_request_products *request_ptr,
                                             int p_server_id,
                                             Pd_msg_header *message_header_ptr ){

   int elev_param;
   int error_flag;
   int vs_num;

   LE_send_msg(GL_INFO | LE_VL2, "Current Product Request Received \n");

   vs_num = Vol_number_in_rda_stat % MAX_VSCAN;
   if( vs_num == 0 )
      vs_num = MAX_VSCAN; 
                	    
   /* Check if product has an elevation parameter. */
   if( (elev_param = ORPGPAT_elevation_based( request_ptr->prod_id )) >= 0 ){

      if( request_ptr->params[elev_param] & ORPGPRQ_ELEV_FLAG_BITS ){

         int num_elevs = 0;
         short vcp_elev_angles[MAX_ELEVATION_CUTS];
         short vcp_elev_inds[MAX_ELEVATION_CUTS];

         if( (num_elevs = ORPGPRQ_get_requested_elevations( (int) Cr_gsm.vcp, 
                                                            request_ptr->params[elev_param],
                                                            (int) MAX_ELEVATION_CUTS,
                                                            vs_num, 
                                                            vcp_elev_angles, 
                                                            vcp_elev_inds )) <= 0 ){

            if( num_elevs < 0 ){

               LE_send_msg( GL_INFO | LE_VL3, "Invalid Parameter (%x) For Prod %d\n",
                            request_ptr->params[elev_param], 
                            request_ptr->prod_id );         

               error_flag = OTR_INVALID_PARAMS;

            }
            else{

               LE_send_msg( GL_INFO | LE_VL3, "No Elevs (%x) For Prod %d\n",
                            request_ptr->params[elev_param], 
                            request_ptr->prod_id );         

               error_flag = OTR_PRODUCT_NOT_AVAILABLE;
         
            }

            OTR_reply_to_request( p_server_id, request_ptr, message_header_ptr, 0,
                                  request_ptr->prod_id, 0, error_flag, END_OF_REPLIES );

            return;

         }
         else{

            int i;
            short supplemental_scans, elev_angle;

            /* Allocate temporary storage for this product request. */
            Pd_request_products *this_request_ptr = 
               (Pd_request_products *) calloc( 1, sizeof(Pd_request_products) );

            if( this_request_ptr == NULL ){

               LE_send_msg( GL_MEMORY, "Memory Allocation Failed for %d Bytes \n",
                            sizeof(Pd_request_products) );
               ORPGTASK_exit(GL_MEMORY);

            }
  
            LE_send_msg( GL_INFO | LE_VL3, 
                   "Product %d Request is a Multiple-Elevation (%d) Request (Parm: %x)\n",
                   request_ptr->prod_id, num_elevs, request_ptr->params[elev_param] );

            /* The following code is primarily used to establish whether there
               are supplemental cuts and if so, we need to flag the request. */
            supplemental_scans = 0;
            if( request_ptr->params[elev_param] & ORPGPRQ_ALL_ELEVATIONS ){

               elev_angle = 0;

               /* If the request contains an elevation angle, then check the 
                  decoded request to see if there are indeed multiple cuts
                  matching this angle. */
               if( (elev_angle = request_ptr->params[elev_param] & 0x01fff) != 0 ){

                  /* For each request, if elevation index values do not match then
                     this indicates supplement scans. */
                  for( i = 1; i < num_elevs; i++ ){

                     if( vcp_elev_inds[i] != vcp_elev_inds[0] ){

                        supplemental_scans = i;
                        break;

                     }

                  }

               }

               /* Announce this request has multiple requests for the same angle AND
                  the VCP contains multiple cuts of this angle. */
               if( supplemental_scans != 0 )
                  LE_send_msg( GL_INFO, "Product %d Request is for All Elevations of %d\n",
                               request_ptr->prod_id, elev_angle );

            }

            /* Do for each elevation ... by virtue of calling OTR_add_to_volume_requests,
               check if product is generated.  If yes, send reply.  Otherwise, 
               schedule for generation if allowed.  */
            for( i = 0; i < num_elevs; i++ ){
               
               /* Copy this request to temporary storage.  This needs to be done
                  inside the loop since the "num_products" field is decremented 
                  by call to OTR_add_to_volume_requests(). */
               memcpy( this_request_ptr, request_ptr, sizeof(Pd_request_products) );

               /* Set the number of products to 1 for this request .... if number of 
                  products is greater than 1, this will be taken care of by the 
                  request with the elevation flag set parameter. */
               this_request_ptr->num_products = 1;
               this_request_ptr->params[elev_param] = vcp_elev_angles[i];
               if( (supplemental_scans) && (supplemental_scans == i) ){

                   this_request_ptr->flag_bits |= SUPPLEMENTAL_SCAN_BIT;
                   LE_send_msg( GL_INFO | LE_VL3, "--->Requesting Supplemental Scan Elevation %d\n",
                                this_request_ptr->params[elev_param] );

               }
               else
                  LE_send_msg( GL_INFO | LE_VL3, "--->Requesting Elevation %d\n",
                               this_request_ptr->params[elev_param] );
               OTR_add_to_volume_requests( p_server_id, message_header_ptr, 
                                           this_request_ptr );	       

            }

            free( this_request_ptr ); 

            /* If this request is for multiple volumes, then send original request. */
            if( request_ptr->num_products > 1 ){

               void *new_entry = NULL;
               int made_request = REALTIME_REQUEST;

               LE_send_msg( GL_INFO | LE_VL3, 
                      "--->Number Products (%d) > 1.  Submitting Real-time Request\n",
                      request_ptr->num_products );
               new_entry = OTR_add_request_to_request_list( p_server_id, message_header_ptr, 
                                                            request_ptr, 0, &made_request,
                                                            VOLUME_NUMBER_UNDEFINED );

               if( new_entry != (void *) NULL )
                  OTR_post_realtime_onetime_request( p_server_id, request_ptr, new_entry );

               else{

                  LE_send_msg( GL_INFO | LE_VL3, "new_entry is NULL .... coding error.\n" );
                  ORPGTASK_exit(GL_MEMORY);

               }

            }

         }

      }
      else{

         /* Check if elevation angle is encoded as negative angle.  if so, decode it
            into a negative angle. */
         if( request_ptr->params[elev_param] > 1800 ){

            LE_send_msg( GL_INFO, "Convert Current Prod Req Elev Param to Neg Ang: %d\n",
                         request_ptr->params[elev_param] );
            request_ptr->params[elev_param] -= 3600;

         }

         OTR_add_to_volume_requests( p_server_id, message_header_ptr, 
                                     request_ptr );	       
      }

   }
   else
      OTR_add_to_volume_requests( p_server_id, message_header_ptr, 
                                  request_ptr );	       

   return;

}


/**************************************************************************
   Description: Handles all requests for specific volume scan time.

   Input: 

   Arguments: A message containing requests, that is a message header
              followed by the request message.
              The ID of the product server that sent the message.
              A pointer to the message header of the request.

   Output: (by subroutines) replies to the reply lb.

   Returns: void

   Notes:
**************************************************************************/
static void Service_specific_time_request( Pd_request_products *request_ptr,
                                           int p_server_id,
                                           Pd_msg_header *message_header_ptr ){

   /* request was for latest available product find the product in the product 
      database LB */
   int volume_number;
   int error_flag = OTR_PRODUCT_READY;
   int parameter_modified = -1, modified_parameter = PARAM_UNUSED;
   int lb_return;   /* LB number found to match the product request */
   int msg_return;  /* message ID number found to match the product 
		       request */
   int found;       /* non-zero means a product was found matching 
		       the request */
        
   LE_send_msg( GL_INFO | LE_VL3, "Service Specific Time Request\n" );

   /* Is there special processing for this request? */
   parameter_modified = Special_request_processing( request_ptr, p_server_id,
                                                    message_header_ptr,
                                                    &modified_parameter,
                                                    &error_flag, SPECIFIC_DATE_TIME );
	    	    	    
   if( parameter_modified >= 0 )
      request_ptr->params[parameter_modified] = modified_parameter;

   /* Search the data base for this product. */
   if( error_flag == OTR_PRODUCT_READY ){

      found = OTR_find_product(request_ptr, &lb_return,& msg_return, &volume_number);
      if (found > 0){

         LE_send_msg( GL_INFO | LE_VL3,"--->Prod History Found Database (msg id: %d)\n", 
                      msg_return );    
	       
         OTR_reply_to_request( p_server_id, request_ptr, message_header_ptr, 
                               volume_number, lb_return, msg_return, error_flag, 
                               END_OF_REPLIES );

         return;

      }

   }

   /* no existing product found for the request or error occurred, send error back. */
   /* if error_flag is not already set, then set the error_flag to "not available". */
   if( error_flag == OTR_PRODUCT_READY )
      error_flag = OTR_PRODUCT_NOT_AVAILABLE;

   LE_send_msg( GL_INFO | LE_VL2, "--->Prod History Not Found or Error Occurred (%d)\n",
                error_flag );

   OTR_reply_to_request( p_server_id, request_ptr, message_header_ptr,
	                 0, 0, 0, error_flag, END_OF_REPLIES );

}  /* End of Service_specific_date_time(). */


/**************************************************************************
   Description: Handles special request reply processing.

   Input: 

   Arguments: A message containing requests, that is a message header
              followed by the request message.
              The ID of the product server that sent the message.
              A pointer to the message header of the request.
              An error value.

   Output: (by subroutines) replies to the reply lb.

   Returns: 1 if reply to request made, 0 otherwise.

   Notes:
**************************************************************************/
static int Special_reply_processing( Pd_request_products *request_ptr, 
                                     int p_server_id,
                                     Pd_msg_header *message_header_ptr,
                                     int error_flag ){

   int replied = 0;

   /* Case statement for special reply processing (on a product by product basis) */
   switch( request_ptr->prod_id ){

      case CFCPROD:
      case HYUSPACC:
      {

         /* if CFCPROD or USP and user has scheduling permission, generate a 
            "current product" request.  If user doesn't have scheduling permission, 
            respond with "not generated".  For CFC and no schedulei permission,
            schedule for generation but not distribution. */

         char *line_info_buffer;
         int line_lb_status;
         Pd_distri_info *p_tbl;
         Pd_line_entry *l_tbl;
         int i;
         int dial_in_line = 0;
		    
         replied = 1;

         /* read in the line status to find if it's dial-in or not */
         line_lb_status = ORPGDA_read(ORPGDAT_PROD_INFO, &line_info_buffer,
	         	              LB_ALLOC_BUF, PD_LINE_INFO_MSG_ID);
         if (line_lb_status >= 0) {

            p_tbl = (Pd_distri_info *) line_info_buffer;
	    if( (line_lb_status < sizeof(Pd_distri_info))
                               ||
	        (p_tbl->line_list < sizeof(Pd_distri_info)) 
                               ||
	        (p_tbl->line_list + p_tbl->n_lines * sizeof(Pd_line_entry) > line_lb_status)) {

               LE_send_msg(GL_INPUT| LE_VL1, 
	                   "Error in PD_LINE_INFO_MSG_ID msg - line_lb_status %d, line_list %d, n_lines %d.",
	                   line_lb_status, p_tbl->line_list, p_tbl->n_lines);
            }

	    l_tbl = (Pd_line_entry *) (line_info_buffer + p_tbl->line_list);

	    /* line info is available, search for line_ind to get the line_type */
	    for (i = 0; i < p_tbl->n_lines; i++) {

	       if (l_tbl[i].line_ind == message_header_ptr->line_ind) {

	          if( l_tbl[i].line_type == DIAL_IN){

                     Pd_user_entry *up = NULL;
                     int user_id = message_header_ptr->src_id;

                     if( ORPGNBC_get_user_profile( DIAL_IN, user_id, 0, &up ) < 0 )
	                dial_in_line = 1;

                     else{

                        /* If user doesn't have scheduling permission, treat as 
                           a dial-in user .... otherwise treat as a dedicated user. */ 
                        if( (up->cntl & UP_CD_NO_SCHEDULE) )
                           dial_in_line = 1;                     

                        free( up );

                     }

                  }

	          break;

	       }

	    }

            free ((void *)line_info_buffer);
	                
         }

         if (dial_in_line == 1){
	                
	    /* simulate a current product request */

	    /* product will not be distributed to dial-in, so make the reply */
	    OTR_reply_to_request(p_server_id, request_ptr, message_header_ptr, 0,
	                         request_ptr->prod_id, 0, error_flag, END_OF_REPLIES);		    		    	                        	                    
	                    
            if (request_ptr->prod_id == CFCPROD){

	       request_ptr->VS_start_time =-1;
	                    
	       /* p_server id of -1 means do not send reply, just schedule the product 
                  for generation. */
	       LE_send_msg( GL_INFO | LE_VL3,
	                    "Scheduling CFC Prod For Generation, Not Distribution\n");
               OTR_add_to_volume_requests(-1, message_header_ptr, request_ptr);

            }

         }
         else {

	     /* request from dedicated line, schedule product for distribution */
	     request_ptr->VS_start_time =-1;
	     LE_send_msg( GL_INFO | LE_VL3,
	                  "Scheduling CFC or USP Product For Generation and Distribution\n");
	                  OTR_add_to_volume_requests(p_server_id, message_header_ptr,
	                  request_ptr );
	                   
         }

      } /* end of special CFC and USP processing */               			         

      default:
      {
         replied = 0;
         break;

      }

   }

   return(replied);

} /* End of Special_reply_processing() */
