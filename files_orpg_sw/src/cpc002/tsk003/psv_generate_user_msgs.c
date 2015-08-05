
/******************************************************************

	file: psv_generate_user_msgs.c

	This module contains functions that generates several types
	of product user messages to be sent to the user.
	
******************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <orpg.h>
#include <orpgerr.h>
#include <prod_user_msg.h>
#include <prod_status.h>
#include <gen_stat_msg.h>

#include "psv_def.h"

/* local functions */


/**************************************************************************

    Description: This function generates a request Response message. The
		buffer for this message is allocated and expected to be 
		freed by the caller.

    Inputs:	usr - the user involved.
		error_code - the number of bit shifts of the error code.
		msg_code - product/message code.
		seq_number - sequence number.
		elev_angle - elevation angle.

		For further details about these argument, refer to 
		RPG/APUP ICD, Fig 3-18. 

    Return:	Pointer to the message on success or NULL on failure.

**************************************************************************/

char *GUM_rr_message (User_struct *usr, int error_bit, int msg_code, 
					int seq_number, int elev_angle)
{
    Pd_request_response_msg *rr;
    Scan_Summary *summary = NULL;
    short vol_num;
    char *buf;

    buf = WAN_usr_msg_malloc (sizeof (Pd_request_response_msg));
    if (buf == NULL)
	return (NULL);

    rr = (Pd_request_response_msg *)buf;

    rr->mhb.msg_code = MSG_REQ_RESPONSE;
    GUM_date_time (usr->time, &(rr->mhb.date), &(rr->mhb.time));
    rr->mhb.length = sizeof (Pd_request_response_msg);
    rr->mhb.dest_id = -1;
    rr->mhb.n_blocks = 2;

    rr->divider = -1;
    rr->length = 26;
    rr->error_code = FULLWORD_SHIFT (error_bit);
    rr->seq_number = seq_number;
    if (msg_code & THIS_IS_PROD_ID) {
	int prod_id;

	prod_id = msg_code & (~THIS_IS_PROD_ID);
	rr->msg_code = ORPGPAT_get_code (prod_id);

	if (rr->msg_code == 0)
	    LE_send_msg (GL_ERROR | 2,  
		"warning - product code not found (prod_id %d) L%d", 
						prod_id, usr->line_ind);
    }
    else 
	rr->msg_code = msg_code;

    rr->elev_angle = elev_angle & 0xffff;
    vol_num = (elev_angle >> 16) & 0xffff;

    summary = ORPGSUM_get_scan_summary( (int) vol_num );
    if( (summary != NULL) && (vol_num > 0) ){

       rr->vol_date = (short) summary->volume_start_date;
       ORPGMISC_pack_ushorts_with_value( (void *) &rr->vol_time_msw, 
                                         (void *) &summary->volume_start_time ); 

    }
    else{

       rr->vol_date = rr->mhb.date;
       ORPGMISC_pack_ushorts_with_value( (void *) &rr->vol_time_msw, 
                                         (void *) &rr->mhb.time );

    }

    return (buf);
}

/**************************************************************************

    Description: This function calculates the date and time fields in the
		user message header block.

    Inputs:	cr_time - the current time;

    Outputs:	date - The modified Julian date;
		time - number of seconds from mid-night.

**************************************************************************/

void GUM_date_time (time_t cr_time, short *date, int *time)
{

    *date = RPG_JULIAN_DATE (cr_time);
    *time = RPG_TIME_IN_SECONDS (cr_time);
    return;
}


