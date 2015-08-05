
/******************************************************************

	file: cmt_main.c

	This is the main module for the comm_manager - TCP
	version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/08/18 22:30:34 $
 * $Id: cmt_main.c,v 1.26 2010/08/18 22:30:34 ccalvert Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 *
 *
 * 11JUN2003 - Chris Gilbert - CCR #NA03-06201 Issue 2-150. Add "faaclient"
 *             support going through a proxy firewall.
 *
 * 06FEB2003 Chris Gilbert - CCR NA03-03502 Issue 2-129 - Add connection_procedure_
 *           time_limit to increase amount of time to go through a firewall.
 *
 * 06FEB2003 Chris Gilbert - CCR NA03-02901 Issue 2-130 - Add no_keepalive_
 *           response_disconnect_time in order to disconect in a timely matter
 *           in case of link breaks.
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <comm_manager.h>
#include "cmt_def.h"

static int Verbosity_level = -1;
int MAXIF = 110; /* max num of interface index numbers for SNMP device */
int connection_procedure_time_limit = CONNECTION_PROCEDURE_TIME_LIMIT;  /* LOGIN message ack time */
int no_keepalive_response_disconnect_time = -1; /* disconnect time if no keepalive messages are acked */

int Simple_code = 1;

static int Comp_method = -1;	/* Compress method. < 0: Not compressed */
static int Pack_ms = 0;		/* Message packing time, ms. = 0: Not packed */
static int Max_pack = 50;	/* Max number of msgs in each pack */

static int N_links;		/* Number of links managed by this process */
static Link_struct *Links [MAX_N_LINKS];
				/* link struct list */

/* local functions */
static int Initialize ();


/******************************************************************

    Description: The main function of comm_manager.

    Input:	argc - argument count;
		argv - argument array;

******************************************************************/

int main (int argc, char **argv)
{
    int Verbose, i, cnt, poll_period, pnps;

    LB_NTF_control (LB_NTF_SIGNAL, LB_NTF_NO_SIGNAL);

    CMC_start_up (argc, argv, &Verbose);
    if (Verbosity_level >= 0)
	LE_local_vl (Verbosity_level);

    /* read link configuration file */
    if ((N_links = CC_read_link_config (Links)) < 0)
	exit (1);
    if (N_links == 0) {
	LE_send_msg (LE_VL0 | 1021,  
			"no link to be managed by this comm_manager\n");
	exit (0);
    }

    /* initialize this task */
    if (CMC_initialize (N_links, Links) != 0 ||
	Initialize () != 0)
	exit (1);

    /* catch the signals */
    CMC_register_termination_signals (CM_terminate); 

    /* initialize the protocol modules */
    if (SH_initialize (N_links, Links) < 0)
	CM_terminate ();

    /* send CM_START message */
    poll_period = 100;
    for (i = 0; i < N_links; i++) {
	CMPR_send_event_response (Links[i], CM_START, NULL);

       if (Links[i]->network == CMT_PPP) {
          SNMP_disable (Links[i]);
       } /* end if */

	if (Links[i]->data_en == 0)
	    poll_period = 10;
    } /* end for */

    /* the main loop */
    pnps = 0;		/* previous total number of processed requests */
    cnt = 0;		/* prevent from calling CMC_house_keeping too often */
    while (1) {
	int ms, nps;

	if (CMRR_get_requests (N_links, Links) > 0)
	    ms = 4;
	else
	    ms = poll_period;	/* maximum blocking time in SH_poll */
	if ((nps = CMPR_process_requests (N_links, Links)) > pnps) {
	    ms = 0;
	    pnps = nps;
	}
	if (cnt >= 500) {
	    CMC_house_keeping (N_links, Links);
	    cnt = 0;
	}
	TCP_process_flow_control ();
	SH_poll (ms);
	cnt += ms + 50;;
    }
}

/**************************************************************************

    Description: This function performs initialization works specific to
		this comm_manager.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Initialize ()
{
    int i;

    for (i = 0; i < N_links; i++) {
	int k;
	Link_struct *link;

	link = Links[i];
	link->server_fd = -1;
	link->ch_num = 1;
	if (Simple_code)
	    link->ch_num = 0;
	link->address = NULL;
	link->ready_pvc = NOT_READY;
	link->client_ip = CLIENT_ADDR_UNDEFINED;
	link->connect_st_time = 0;
	link->last_conn_time = 0;
	link->n_bytes_in_window = 0;

	for (k = 0; k < link->n_pvc; k++) {
	    link->pvc_fd[k] = -1;
	    link->ch2_fd[k] = -1;
	    link->login_state[k] = DISABLED;
	    link->connect_state[k] = DISABLED;
	    link->log_cnt[k] = 0;
	    link->r_msg_size[k] = 0;
	    link->alive_time[k] = 0;
	    link->sent_seq_num[k] = 1;
	    link->w_blocked[k] = 0;
	    link->read_ready[k] = 0;
	}
        if ( link->server == CMT_FAACLIENT && link->ch2_link != NULL) {
	   link->ch2_link->server_fd = -1;
	   link->ch2_link->ch_num = 2;
           link->ch2_link->address = NULL;
           link->ch2_link->ready_pvc = NOT_READY;
           link->ch2_link->client_ip = CLIENT_ADDR_UNDEFINED;
           link->ch2_link->connect_st_time = 0;
           for (k = 0; k < link->n_pvc; k++) {
              link->ch2_link->pvc_fd[k] = -1;
              link->ch2_link->ch2_fd[k] = -1;
              link->ch2_link->login_state[k] = DISABLED;
              link->ch2_link->connect_state[k] = DISABLED;
              link->ch2_link->r_msg_size[k] = 0;
              link->ch2_link->alive_time[k] = 0;
              link->ch2_link->sent_seq_num[k] = 1;
              link->ch2_link->w_blocked[k] = 0;
              link->ch2_link->read_ready[k] = 0;
           }
           
        }
	link->n_rss = 0;
	if (link->server == CMT_FAACLIENT) {
	    link->n_rss = 2;
	    for (k = 0; k < link->n_rss; k++) {
		link->tfd[k] = -1;
		link->rs_name[k] = malloc (strlen (link->conf_rs_name[k]) + 1);
		if (link->rs_name[k] == NULL) {
		    LE_send_msg (GL_ERROR, "malloc failed");
		    return (-1);
		}
		strcpy (link->rs_name[k], link->conf_rs_name[k]);
		link->rs_addrs[k] = NULL;
	    }
	}
    }

    return (0);
}

/**************************************************************************

    Returns the compression method.

**************************************************************************/

int CM_compress_method () {

    return (Comp_method);
}

/**************************************************************************

    Returns Pack_ms.

**************************************************************************/

int CM_pack_ms () {

    return (Pack_ms);
}

/**************************************************************************

    Returns Max_pack.

**************************************************************************/

int CM_max_pack () {

    return (Max_pack);
}

/**************************************************************************

    Description: This function returns additional specific option flags
		for this comm_manager.

    Return:	The flag string.

**************************************************************************/

char *CM_additional_flags ()
{

    return ("l:c:sO");
}

/**************************************************************************

    Description: This function reads additional specific command line 
		arguments for this comm_manager.

    Inputs:	c - option flag;
		optarg - option string;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int CM_parse_additional_args (int c, char *optarg)
{

    switch (c) {
	int ret;

	case 's':
	    TCP_set_share_bandwidth ();
	    break;

	case 'O':
	    Simple_code = 0;
	    break;

	case 'l':
	    if (sscanf (optarg, "%d", &Verbosity_level) != 1 ||
		Verbosity_level < 0 || Verbosity_level > 3) {
		LE_send_msg (GL_ERROR, "option -l error");
		return (-1);
	    }
	    break;

	case 'c':
	    ret = sscanf (optarg, "%d-%d-%d", 
				&Comp_method, &Pack_ms, &Max_pack);
	    if (ret < 1 ||
		Comp_method < 0 || Comp_method > 1 ||
		Pack_ms < 0 ||
		Max_pack <= 0) {
		LE_send_msg (GL_ERROR, "option -c error");
		return (-1);
	    }
	    break;
    }
    return (0);
}

/**************************************************************************

    Description: This function prints the additional specific usage info
		for this comm_manager.

**************************************************************************/

void CM_print_additional_usage ()
{
    printf ("       -s (share bandwidth among all managed links)\n");
    printf ("       -l verbosity level (0 - 3, default is 0, -v sets to 1)\n");
    printf ("       -c method[-pack_ms[-pack_max]] (Compress outgoing payload.\n");
    printf ("          method: 0 - gzip, 1 - bzip2. If pack_ms is specified,\n");
    printf ("          messages in \"pack_ms\" milli-seconds are packed and sent\n");
    printf ("          as a single message. pack_max specifies the max number of\n");
    printf ("          messages packed. Its default is 50.)\n");
    printf ("          \n");
    printf ("          Notes: If -Q or message packing is selected, each request \n");
    printf ("          of writing data is responded immediately with SUCCESS.\n");
    printf ("          Any later data sending failure will generate an event.\n");
    printf ("          Only outgoing payload on the lowest priority VC is subject\n");
    printf ("          to buffering, delay and packing. All other requests are\n");
    printf ("          processed as soon as received. Payload on all VCs are\n");
    printf ("          subject to compression.\n");
    printf ("       -O (Use older faaclient code)\n");
    return;
}

/**************************************************************************

    Description: This function performs clean-up works and then 
		terminates this process.

**************************************************************************/

void CM_terminate ()
{

    CMC_terminate ();

    LE_send_msg (GL_INFO, "cm_tcp exits");
    exit (0);
}

