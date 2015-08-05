
/******************************************************************

	file: cmp_tcp_mon.c

	This is the cm_ping module for TCP connection monitoring.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/16 20:54:32 $
 * $Id: cmp_tcp_mon.c,v 1.9 2007/02/16 20:54:32 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <cmp_def.h>
#include <orpgerr.h>
#include <cm_ping.h>

extern int Verbose;

static Remote_host *Rhosts = NULL;	/* remote host table */
static void *Tblid = NULL;		/* table id for Rhosts */
static int N_rhosts = 0;		/* # of entries in Rhosts array */

static int Exp_period = 600;		/* registration expiration time */

static void Output_results ();


/**************************************************************************

    Description: This function performs the routine work for TCP
		connection monitoring. This function must be called 
		periodically.

**************************************************************************/

void CMPT_check_TCP ()
{
    time_t cur_tm;
    int i;

    cur_tm = MISC_systime (NULL);

    /* check expired registration */
    for (i = N_rhosts - 1; i >= 0; i--) {

	if (cur_tm - Rhosts[i].reg_time > Exp_period) {
	    MISC_table_free_entry (Tblid, i);
	    Rhosts = (Remote_host *)MISC_get_table (Tblid, &N_rhosts);
	}
    }

    /* check TCP connectivity */
    if (N_rhosts > 0)
	CMP_check_connect (N_rhosts, Rhosts);

    /* generate TCP monitoring info output */
    Output_results ();

    return;
}

/**************************************************************************

    Description: This function processes the TCP monitoring request
		messages.

    Inputs:	buf - buffer holding the TCP monitoring request message.
		len - size of the message.

**************************************************************************/

void CMPT_process_input (char *buf, int len)
{
    int addr, i;
    Cmp_tcp_mon_req_t *msg;
    time_t cr_tm;

    if (len != sizeof (Cmp_tcp_mon_req_t)) {
	LE_send_msg (GL_ERROR | 1027,  "bad TCP mon req msg lenghth (%d)", len);
	return;
    }
    msg = (Cmp_tcp_mon_req_t *)buf;
    addr = msg->addr;
    for (i = 0; i < N_rhosts; i++) 
	if (addr == Rhosts[i].addr)
	    break;

    cr_tm = MISC_systime (NULL);
    if (i >= N_rhosts) {		/* a new remote host */
	Remote_host *rh;

	if (Tblid == NULL)
	    Tblid = MISC_create_table (sizeof (Remote_host), 16);

	/* get a new entry */
	rh = (Remote_host *)MISC_table_new_entry (Tblid, NULL);
	if (rh == NULL) {
	    LE_send_msg (GL_ERROR | 1028,  "malloc failed (TCP)");
	    return;
	}
	Rhosts = (Remote_host *)MISC_get_table (Tblid, &N_rhosts);
	rh->addr = addr;
	rh->qtime = 0;
	rh->reg_time = cr_tm;
    }
    else {				/* existing */
	Rhosts[i].reg_time = cr_tm;
    }
    return;
}

/**************************************************************************

    Description: This function generates a TCP connection status message
		and sends to the cm_ping output.

**************************************************************************/

#define BUF_SIZE_INC 16

static void Output_results ()
{
    static char *buf = NULL;
    static int buf_size = 0;
    int i, size;
    Cmp_out_msg_t *msg;
    char tmp[256];

    /* allocate the buffer */
    if (N_rhosts > buf_size) {
	int new_size;

	if (buf != NULL)
	    free (buf);
	buf_size = 0;
	new_size = N_rhosts + BUF_SIZE_INC;
	buf = malloc (sizeof (Cmp_out_msg_t) + 
				new_size * sizeof (Cmp_record_t));
	if (buf == NULL) {
	    LE_send_msg (GL_ERROR | 1029,  "malloc (%d) failed", new_size);
	    return;
	}
	buf_size = new_size;
    }
    if (N_rhosts <= 0)
	return;

    msg = (Cmp_out_msg_t *)buf;
    msg->n_records = N_rhosts;
    msg->tm = MISC_systime (NULL);
    if (Verbose)
	sprintf (tmp, "%d %d - ", (int)msg->tm, msg->n_records);
    for (i = 0; i < N_rhosts; i++) {
	msg->rec[i].addr = Rhosts[i].addr;
	msg->rec[i].q_time = Rhosts[i].qtime;
	if (Verbose && i <= 2)
	    sprintf (tmp + strlen (tmp), "%x %d, ", 
			(unsigned int)msg->rec[i].addr, msg->rec[i].q_time);
    }
    if (Verbose) {
	if (i > 3)
	    strcpy (tmp + strlen (tmp), "...");
	else
	    strcpy (tmp + strlen (tmp) - 2, "");
	LE_send_msg (LE_VL1 | 1030,  "%s", tmp);
    }

    size = sizeof (Cmp_out_msg_t) + (N_rhosts - 1) * sizeof (Cmp_record_t);
    CMP_output (buf, size);

    return;
}
