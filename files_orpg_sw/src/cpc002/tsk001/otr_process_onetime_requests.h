/**************************************************************************
   
   Module:  otr_process_onetime_requests.h
   
   Description: Processes the onetime request message.
   
   
   
   Assumptions: This assumes that all fields have already been validated.
   
   **************************************************************************/
/*
* RCS info
* $Author: hoytb $
* $Locker:  $
* $Date: 2001/05/04 15:55:20 $
* $Id: otr_process_onetime_requests.h,v 1.5 2001/05/04 15:55:20 hoytb Exp $
* $Revision: 1.5 $
* $State: Exp $
*/
#ifndef OTR_PROCESS_ONETIME_REQUESTS_H

#define OTR_PROCESS_ONETIME_REQUESTS_H
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

void OTR_process_onetime_requests(Pd_msg_header  *message_header, 
                                  int p_server_id);
/* where message_header is the pointer to the start of the request message,
         containing a message header followed by an array of requests 
   p_server_id is the instance number of the p_server handling communications
      whith the user that made this request.
      If the p_server instance is negative, then the product will be scheduled,
      but not sent to anyone.  (This is to allow generation of CFC and USP by
      dial-in users for later retrieval. )*/

#endif
