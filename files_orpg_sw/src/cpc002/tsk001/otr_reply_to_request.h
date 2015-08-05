/**************************************************************************
   
   Module:  otr_reply_to_request.h
   
   Description:
   Header file for a function to reply to onetime requests.
   
   
   Assumptions:
   
   **************************************************************************/
/*
* RCS info
* $Author: hoytb $
* $Locker:  $
* $Date: 2001/05/04 15:57:20 $
* $Id: otr_reply_to_request.h,v 1.6 2001/05/04 15:57:20 hoytb Exp $
* $Revision: 1.6 $
* $State: Exp $
*/

#ifndef OTR_REPLY_TO_REQUEST_H

#define OTR_REPLY_TO_REQUEST_H
/*
* System Include Files/Local Include Files
*/
#include <prod_user_msg.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
/*
* Static Globals
*/
/*
* Static Function Prototypes
*/
int OTR_reply_to_request( int p_server_id,
                          Pd_request_products *request,
			  Pd_msg_header *message_header,
			  unsigned int volume_number,
			  int id, 
			  int message_id, 
			  int error_return, 
			  int left_to_go);
/* where 
   p_server_id is the instance number of the p_server that should receive the reply,
   request is a pointer to the original request message,
   message_header is a pointer to the header of the message that made the request,
   volume_number is the number of the volume scan the product was made from,
   id is the product id for the reply, 
   message_id is the LB message number that fulfills the request. 
   error_return will be OTR_PRODUCT_READY if there was no error, or one of the
           other codes listed in prod_distri_info.h if there was an error.
   left_to_go is 0 if no more messages are needed to fulfill this request, and
                 1 if there are more messages to be sent.
   
   This routine always returns a zero.*/


#endif
