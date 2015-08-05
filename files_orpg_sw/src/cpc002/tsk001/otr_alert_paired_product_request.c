/**************************************************************************

   Module:  otr_alert_paired_product_request.c

   Description:
   This routine takes in a request message from the alerting function.
   It then finds the p_server instance number that handles the specified
   line. It turns this into a request for the latest product, and passes
   the request to OTR_add_to_volume_requests for generation and distribution.

   Assumptions:
   The status is stored in the LB with the message ID equal to the Volume ID.

   **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/12 17:27:57 $
 * $Id: otr_alert_paired_product_request.c,v 1.26 2014/03/12 17:27:57 steves Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */

#include <prod_distri_info.h>
#include <prod_status.h>
#include <stdlib.h>
#include <orpgdat.h>
#include <orpgda.h>
#include <otr_alert_paired_product_request.h>
#include <otr_new_volume_product_requests.h>
#include <orpgerr.h>
#include <orpgtask.h>
/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Static Globals
 */
/*
 * Static Function Prototypes
 */

/**************************************************************************
   Description: This finds the p_server instance number that handles the specified
   line to which an alert paired product is to be sent. It turns this into a 
   request for the latest product, and passes the request to 
   OTR_add_to_volume_requests for generation and distribution.


   Input:  The request message for the product, complete with its header.

   Output:
           Replies will be sent to p_server on the appropriate LB by called
           subroutines.

   **************************************************************************/
void
OTR_alert_paired_product_request(Pd_msg_header * message_header_ptr)
{
    int             p_server_id = 0;	/* p_server instance to reply to */
    LB_info         info;	        /* information from LB_info call */
    Pd_request_products *request_ptr;
    char           *line_info_buffer = NULL;
    Pd_distri_info *p_tbl;	       /* pointer to product distribution info in
				        * line_info_buffer */
    Pd_line_entry  *l_tbl;	       /* pointer to line info in line_info_buffer */
    int             line_info_length = 0;	/* length of the line_info_buffer
					 * message */
    int             line_lb_status;	/* status of last operation on the
					 * ORPGDAT_PROD_INFO LB */
    int             i;		/* index for loop */

    /*
     * read in the line vs. p_server instance data
     */
    line_lb_status = ORPGDA_info(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID, &info);
    if ((line_lb_status >= 0) && (info.size > 0)) {
	line_info_length = info.size;
	line_info_buffer = malloc(line_info_length);
	if (line_info_buffer == NULL) {
	    LE_send_msg(GL_INFO | LE_VL0, "memory allocation error in otr_alert_paired_product_request \n");
	    ORPGTASK_exit(GL_MEMORY);
	}
    } else {
	LE_send_msg(GL_INPUT| LE_VL0, "error in PD_LINE_INFO_MSG_ID message");
	ORPGTASK_exit(GL_INPUT);
    }
    line_lb_status = ORPGDA_read(ORPGDAT_PROD_INFO, line_info_buffer,
				 line_info_length, PD_LINE_INFO_MSG_ID);
    if (line_lb_status >= 0) {
        p_tbl = (Pd_distri_info *) line_info_buffer;
	if (line_info_length < sizeof(Pd_distri_info) ||
	    p_tbl->line_list < sizeof(Pd_distri_info) ||
	    p_tbl->line_list +
	    p_tbl->n_lines * sizeof(Pd_line_entry) > line_info_length) {
	    LE_send_msg(GL_INPUT| LE_VL0, 
	       "OTR error in PD_LINE_INFO_MSG_ID message - line_info_length %d, line_list %d, n_lines %d.",
	        line_info_length, p_tbl->line_list, p_tbl->n_lines);
	    ORPGTASK_exit(GL_INPUT);
	}
	l_tbl = (Pd_line_entry *) (line_info_buffer + p_tbl->line_list);
	/*
	 * line info is available, search for line_ind to get p_server_id
	 * note that if it is not found, the default becomes instance zero 
	 */
	for (i = 0; i < p_tbl->n_lines; i++) {
	    if (l_tbl[i].line_ind == message_header_ptr->line_ind) {
		p_server_id = l_tbl[i].p_server_ind;
		break;
	    }
	}
    } else {
	LE_send_msg(GL_INPUT | LE_VL0, "cannot read PD_LINE_INFO_MSG_ID message");
	ORPGTASK_exit(GL_INPUT);
    }
    LE_send_msg(GL_INFO | LE_VL2, "alert product will go to p_server instance %d \n", p_server_id);
    free((void *)line_info_buffer);

    request_ptr = (Pd_request_products *) (((char *) message_header_ptr)
					   + sizeof(Pd_msg_header));

    if (message_header_ptr->n_blocks <= 1) {
	LE_send_msg(GL_INFO | LE_VL2, "message header indicates no request in message, n_blocks %d \n",
		    message_header_ptr->n_blocks);
    }
    /* for every request in the message, generate a request for the latest product */
    while (message_header_ptr->n_blocks > 1) {
	message_header_ptr->n_blocks--;
	LE_send_msg(GL_INFO | LE_VL2, "alert request for prod_id %d ", request_ptr->prod_id);
	LE_send_msg(GL_INFO | LE_VL2, "VS_start_time %d \n", request_ptr->VS_start_time);

	/*
	 * set the sequence number so the p_server & user know that it did
	 * not result from a one-time request from the user
	 */

	request_ptr->seq_number = ALERT_PAIRED_PRODUCT_SEQUENCE_NUMBER;
	request_ptr->flag_bits |= ALERT_SCHEDULING_BIT;


        message_header_ptr->time = time(NULL);
	OTR_add_to_volume_requests(p_server_id,
				       message_header_ptr,
				       request_ptr);
	    
	request_ptr++;		/* point to next request */

    }				/* end do for each request after the header */
}
