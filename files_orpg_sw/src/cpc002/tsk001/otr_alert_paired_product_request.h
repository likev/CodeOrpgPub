/**************************************************************************
   
   Module:  otr_alert_paired_product_request.h
   
   Description:  
   This contains the utility handle requests for alert paired products.
   
   Assumptions:
   
   **************************************************************************/
/*
* RCS info
* $Author: hoytb $
* $Locker:  $
* $Date: 2001/05/04 15:30:00 $
* $Id: otr_alert_paired_product_request.h,v 1.4 2001/05/04 15:30:00 hoytb Exp $
* $Revision: 1.4 $
* $State: Exp $
*/
#ifndef OTR_alert_paired_product_request_h

#define OTR_alert_paired_product_request_h
#include <prod_user_msg.h>
/*
* System Include Files/Local Include Files
*/
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
/*
* Static Globals
*/

/* this is a special sequence number.  By putting it in the
   response, the user knows that it comes as a result of an
   alert (not from a one time or routine request */
   
#define ALERT_PAIRED_PRODUCT_SEQUENCE_NUMBER -13

/*
* Static Function Prototypes
*/

/* the following routine takes an alert paired product request, figures
out which p_server instance the line is connected to, and calls routines
to send the product to the appropriate user. */
void OTR_alert_paired_product_request(Pd_msg_header  *message_header_ptr);
   
#endif
