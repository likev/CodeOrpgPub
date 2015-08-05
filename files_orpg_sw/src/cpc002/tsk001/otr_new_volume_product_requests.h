/**************************************************************************
   
   Module:  otr_new_volume_product_requests.h
   
   Description: 
   These routines handle the processing of requests for current and future
   products.  The routine OTR_compare_parameters is also included here
   because it uses information about scan elevations that is updated
   every volume scan.
     
   Assumptions:
   
   **************************************************************************/
/*
* RCS info
* $Author: steves $
* $Locker:  $
* $Date: 2003/06/12 19:18:18 $
* $Id: otr_new_volume_product_requests.h,v 1.15 2003/06/12 19:18:18 steves Exp $
* $Revision: 1.15 $
* $State: Exp $
*/

#ifndef OTR_NEW_VOLUME_PRODUCT_REQUESTS_H

#define OTR_NEW_VOLUME_PRODUCT_REQUESTS_H
/*
* System Include Files/Local Include Files
*/
#include <prod_user_msg.h>
#include <time.h>

/* Constant Definitions/Macro Definitions/Type Definitions */
#define VOLUME_NUMBER_UNDEFINED 	-999

enum {NO_REQUESTS, REPLAY_REQUEST_MADE, REALTIME_REQUEST, ALERT_SCHEDULED}; 
                       /* states for "request_made" in the Table_list structure */

/* Static Globals */

/* Static Function Prototypes */

void OTR_add_to_volume_requests( int p_server_id,
                                 Pd_msg_header *message_header,
				 Pd_request_products *request);
/* where p_server_id is the instance number of the p_server sending the request,
   (can be -1 to schedule a product without transmission),
   message_header is the header message sent with the request,
   and request is the request message.
   This routine will add a request to those scheduled for this
   and future volumes. 
     */

int OTR_check_product_status( int p_server_id, Pd_msg_header *message_header,
                              Pd_request_products *request, int *last_flag,
                              int *volume_number );

int OTR_make_volume_request( int p_server_id, Pd_msg_header *message_header, 
                             Pd_request_products *request, int sent_message, 
                             int volume_number );

void* OTR_add_request_to_request_list( int p_server_id, Pd_msg_header *message_header, 
                                       Pd_request_products *request, int sent_message,
                                       int *made_request, int volume_number );

void OTR_post_realtime_onetime_request( int p_server_id, Pd_request_products *request,
                                        void *new_entry );

void OTR_remove_volume_requests(Pd_msg_header *message_header);
/* where message_header is the message that canceled the requests 
   This routine will cancel all requests from the line_ind in
   the message header. */

void OTR_new_volume_product_requests();
/* this routine should be called once every new volume to update the volume status, 
   to issue replies to past requests and to schedule current ones.*/

int OTR_compare_parameters (short *params1, short *params2, short prod_id);
/* where params1 is a pointer to the parameter array of the first product,
         params2 is a pointer to the second array, and
         prod_id is the product id.
         Compares a product request parameters, taking into account that
   elevations may be approximate.
   Returns 0 if they are not the same,
       and 1 if they are the same */

void OTR_product_status_update();
/* this routine should be called whenever the prod_status LB is updated
   so that replies to onetime requests that are fulfilled may be sent. */

int OTR_replay_message_received();
/* this routine should be called whenever an event is received 
   indicating that a replay response has been written to
   the LB
   */

void OTR_replay_timeout_check();
/* this routine should be called in every
   otr_main loop to check for timeout of 
   replay reply */
 
void    OTR_update_elevation_data();
/* this routine updates the elevation data
   so that the elevation index can be computed.
   This should be called at startup and at
   each new volume */
#endif
