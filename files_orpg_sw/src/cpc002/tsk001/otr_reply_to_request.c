/**************************************************************************
   
   Module:  otr_reply_to_request.c
   
   Description:
   This routine builds a reply to a request message, puts it on the reply LB
   and sends the event that a new message is on the LB.
   
   Assumptions:
   
   **************************************************************************/
/*
* RCS info
* $Author: steves $
* $Locker:  $
* $Date: 2014/03/12 17:27:58 $
* $Id: otr_reply_to_request.c,v 1.23 2014/03/12 17:27:58 steves Exp $
* $Revision: 1.23 $
* $State: Exp $
*/
/*
* System Include Files/Local Include Files
*/
#include <prod_user_msg.h>
#include <prod_distri_info.h>
#include <prod_status.h>
#include <orpgerr.h>
#include <orpgevt.h>
#include <orpgdat.h>
#include <orpgda.h>
#include <orpgpat.h>

/* Constant Definitions/Macro Definitions/Type Definitions */

/* Static Globals */

/* Static Function Prototypes */

/**************************************************************************
   Description: 
      This builds the reply to request messages and sends it to 
      the appropriate p_server instance.
   Input: 
      p_server_id is the p_server instance number that should receive the reply,
      request is a pointer to the original request for the product,
      message_header is a pointer to the message header that went with the request,
      volume_number is the number of the volume scan the product was made from,
      product_lb_id is the LB id that contains the product 
          (may be zero for no product available),
      message_id is the message id in the LB that contains the product requested,
      error_return is an error number, defined in prod_distri_info.h indicating 
           if the request was fulfilled, or the reason it was not,
           (If it was fulfilled, it should be OTR_PRODUCT_READY)
      end_of_reply_flag indicates if more replies are coming for the request.
           (0 = not last (more replies to come), 1 = last reply)
   
   Output: Reply to the request on the reply LB of the requester 
      (computed from line_ind in the message header).
   Returns: Always returns zero.
   Notes:
**************************************************************************/
int OTR_reply_to_request( int p_server_id, Pd_request_products *request,
			  Pd_msg_header *message_header, unsigned int volume_number,
			  int product_lb_id, int message_id, 
			  int error_return, int end_of_reply_flag){

   One_time_prod_req_response reply;
   int status;   /* status of the attempt to write on the reply LB */
   int reply_lb;   /* LB number to send the reply to */
   int en_status;  /* status of the EN_post call */
   int priority;
   int elevation_index;
   
   if (p_server_id >= 0){
   
      /* first, build the reply message from input information */
      /* data structure defined in prod_distri_info.h */
      reply.prod_id = request->prod_id;
      reply.seq_number = request->seq_number;
      reply.line_ind = message_header->line_ind;
      reply.error = error_return;
      reply.vol_number = volume_number;
      elevation_index =  (int) ORPGPAT_elevation_based(request->prod_id);
      if (elevation_index >= 0)
          reply.elev = request->params[elevation_index];
      
      else
          reply.elev = 0;
      
      if ((request->flag_bits & ALERT_SCHEDULING_BIT) >0)
          priority=OTR_ALERT_PRIORITY;
      
      else if ((request->flag_bits &  PRIORITY_FLAG_BIT) >0)
          priority=OTR_HIGH_PRIORITY;
      
      else 
          priority=OTR_LOW_PRIORITY;
      
      reply.priority = priority;
      reply.last = end_of_reply_flag;
      reply.req_time = message_header->time;
      reply.src_id = message_header->src_id;
      reply.dest_id = message_header->dest_id;
   
      /* if there is an error in the reply, blank out the lb_id and msg_id */
      if (error_return != OTR_PRODUCT_READY){

         reply.lb_id = -1;
         reply.msg_id = -1;

      }
      else{

         reply.lb_id = ORPGDAT_PRODUCTS;
         reply.msg_id = message_id;

      }

      /* put the message on the reply LB */
      reply_lb = ORPGDAT_OT_RESPONSE + p_server_id;
      status = ORPGDA_write(reply_lb, (char *) &reply, sizeof(reply), LB_NEXT);
   
      LE_send_msg( GL_INFO | LE_VL2,"Sent Reply to Request to p_server ID %d (status %d)\n",
      		   p_server_id, status );
             
      LE_send_msg( GL_INFO | LE_VL2,"--->Reply LB: %d, Msg ID: %d\n",
      		   reply.lb_id, reply.msg_id );
             
      LE_send_msg( GL_INFO | LE_VL2,
                   "--->Prod ID: %d, Seq #: %d, Error: %d, Vol: %d, Priority %d\n",
      		   reply.prod_id, reply.seq_number, reply.error, reply.vol_number, 
                   reply.priority );

      /* send event ORPGEVT_OT_RESPONSE to indicate the LB has been updated*/
      en_status = EN_post(ORPGEVT_OT_RESPONSE + p_server_id,0,0,0);
      if( en_status < 0 )
         LE_send_msg( GL_ERROR, "EN_post(ORPGEVT_OT_RESPONSE+%d) Failed\n", p_server_id );

   }

   return(0);

}
