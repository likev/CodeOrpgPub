/**************************************************************************
   
   Module:  otr_find_product.h
   
   Description:  
   This contains the utility to find a given product at a specific
   time if it was generated. If the time requested is negative, then
   the time constraint is removed and the latest product (if any) is
   returned.
   
   Assumptions:
   
   **************************************************************************/
/*
* RCS info
* $Author: hoytb $
* $Locker:  $
* $Date: 2001/05/04 15:31:33 $
* $Id: otr_find_product.h,v 1.5 2001/05/04 15:31:33 hoytb Exp $
* $Revision: 1.5 $
* $State: Exp $
*/
#ifndef OTR_find_product_h

#define OTR_find_product_h
/*
* System Include Files/Local Include Files
*/
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
/*
* Static Globals
*/
/*
* Static Function Prototypes
*/

/* returns 1 if product is found and the lb_return and msg_return are set
   returns 0 if no product could be found matching the request */
   
int OTR_find_product( Pd_request_products *request_ptr,
                      int *lb_return,
		      int *msg_return,
		      int *volume_number);
/* where 
   request_ptr is pointer to a request message with the product id, 
      parameters, and time for the request 
   lb_return is a pointer to the return value for the LB number that fulfills the request,
   msg_return is a pointer to return value for message id of the product,
   and volume_number is a pointer to the place to return the volume number the product
      was found in. 

   returns 1 if product is found and the lb_return and msg_return are set
   returns 0 if no product could be found matching the request 
*/
   

#endif
