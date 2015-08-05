
/******************************************************************

	file: cmc_common.c

	This module contains common routines for comm_manager 
	development. 
	
******************************************************************/

/* 
 * RCS info
 * $Author Jing$
 * $Locker:  $
 * $Date: 2013/01/03 21:47:42 $
 * $Id: cmc_common.c,v 1.41 2013/01/03 21:47:42 jing Exp $
 * $Revision: 1.41 $
 * $State: Exp $
 *
 * 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out.
 *
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include <comm_manager.h>
#ifndef NO_ORPG
#include <orpgerr.h>
#include <orpgdat.h>
#include <orpgevt.h>
#endif
#include <infr.h>

#include <cmc_def.h>

static char Req_lb_name[NAME_LEN] = "";
				/* name core of the request LB */
static char Resp_lb_name[NAME_LEN] = "";
				/* name core of the response LB */
static char Cs_proto_name[NAME_LEN] = "";
				/* comm server/protocol name (cm_simpact,
				   cm_uconx, cm_tcp ...) */

static int Verbose;		/* verbose mode */
static int New_instance;	/* this is a new instance */

static char Link_conf_name [NAME_LEN] = "";
				/* name of the link configuration file */

static int Comm_manager_index;	/* instance index of this comm_manager */
static int Log_size;		/* size of the LE LB */

static int N_added_bytes = 0;	/* number of added bytes in front of each
				   incoming message */
static int En_flag;		/* EN posting flag */
static int Allow_queued_payload = 0;

static int N_links;		/* Number of links managed by this process */
static Link_struct **Links;	/* link struct list */

LB_id_t Msg_id;			/* shared message ID buffer */
static int Comm_req_event;	/* event number for receiving comm_manager 
				   requests */
int CMC_comm_resp_event;	/* event number for sending comm_manager 
				   responses */
static int Test_mode;		/* test mode */

static time_t Termination_start_time = 0;
				/* SIGTERM received, waits for all lines
				   to be terminated and then exits */

static int Argc;		/* save the argc */
static char **Argv;		/* save the argv */

/* local functions */
static void Print_cs_error (char *msg);
static void Print_usage (char **argv);
static void En_callback (EN_id_t evtcd, char *msg, int msglen, void *arg);
static int Read_options (int argc, char **argv);


/**************************************************************************

    Description: This function returns Link_conf_name.

    Return:	Link_conf_name.

**************************************************************************/

char *CMC_get_link_conf_name ()
{

    return (Link_conf_name);
}

/**************************************************************************

    Returns flag "Allow_queued_payload".

**************************************************************************/

int CMC_allow_queued_payload () {

    return (Allow_queued_payload);
}

/**************************************************************************

    Description: This function returns En_flag.

    Return:	En_flag.

**************************************************************************/

int CMC_get_en_flag ()
{

    return (En_flag);
}

/**************************************************************************

    Description: This function returns the New_instance flag.

    Return:	New_instance flag.

**************************************************************************/

int CMC_get_new_instance ()
{

    return (New_instance);
}

/**************************************************************************

    Description: This function returns the comm_manager index number.

    Return:	the comm_manager index number.

**************************************************************************/

int CMC_get_comm_manager_index ()
{

    return (Comm_manager_index);
}

/**************************************************************************

    Description: This function returns Cs_proto_name.

**************************************************************************/

char *CMC_get_proto_name ()
{

    return (Cs_proto_name);
}

/**************************************************************************

    Description: This function registers termination signals. This funcions
	is no longer needed.

    Input:	term_func - termination call back function;

**************************************************************************/

void CMC_register_termination_signals (void (*term_func)()) {
    return;
}

/**************************************************************************

    Description: This function registers termination signals.

    Input:	term_func - termination call back function;

**************************************************************************/

static void Term_handler (int sig) {
    if (sig == SIGTERM) {
	if (Termination_start_time == 0) {
	    LE_send_msg (0, "Entering termination phase");
	    Termination_start_time = MISC_systime (NULL);
	}
	return;
    }
    CM_terminate ();
    return;
}

/**************************************************************************

    Description: This function reads the next link config line that
		matches specified comm manager index, comm server/protocol 
		name and user type. This function can only be used to 
		scan the link table once since static variables used.

    Inputs:	cs_proto_name - comm server/protocol name (cm_simpact,
				cm_uconx, cm_tcp ...).

    Return:	It returns the structure of the next link on success, 
		CMC_CONF_READ_DONE if no more link found or 
		CMC_CONF_READ_FAILED on fatal error.

**************************************************************************/

#define BUF_SIZE 64

Link_struct *CMC_read_link_config (char *cs_proto_name)
{
    static int tlinks = -1;
    static int link_ind = 0;
    int dn_ind, tmp;

    strncpy (Cs_proto_name, cs_proto_name, NAME_LEN);
    Cs_proto_name[NAME_LEN - 1] = '\0';

    CS_control (CS_COMMENT | '#');
    CS_error (Print_cs_error);

    if (tlinks < 0 &&
	CS_entry ("number_links", 1 | CS_INT, 0, (char *)&tlinks) < 0)
	return (CMC_CONF_READ_FAILED);

    CS_control (CS_KEY_OPTIONAL);
    if (CS_entry ((char *)link_ind, CS_INT_KEY | 13 | CS_INT, 0, 
						(void *)&tmp) >= 0)
	dn_ind = 13;			/* old style link.conf */
    else
	dn_ind = 11;
    CS_control (CS_KEY_REQUIRED);

    while (link_ind < tlinks) {
	char buf [BUF_SIZE];
	int cm;
	Link_struct *link;
	int link_type, device, port;
	int rate, packet_size, npvcs, data_notify;

	/* verify that the link number is specified */
	if (CS_entry ((char *)link_ind, CS_INT_KEY | 0, 0, NULL) < 0)
	    return (CMC_CONF_READ_FAILED);

	/* match the comm manager number and the subsystem */
	if (CS_entry (CS_THIS_LINE, 2 | CS_INT, 0, (char *)&cm) < 0 ||
	    CS_entry (CS_THIS_LINE, 7, BUF_SIZE, buf) < 0) 
	    return (CMC_CONF_READ_FAILED);
	if (cm != Comm_manager_index || strcmp (buf, cs_proto_name) != 0) {
	    link_ind++;
	    continue;
	}

	/* get line type */
	if (CS_entry (CS_THIS_LINE, 5, BUF_SIZE, buf) < 0)
	    return (CMC_CONF_READ_FAILED);
	if (strcmp (buf, "Dedic") == 0)
	    link_type = CM_DEDICATED;
	else if (strcmp (buf, "D-in") == 0)
	    link_type = CM_DIAL_IN;
	else if (strcmp (buf, "D-out") == 0)
	    link_type = CM_DIAL_IN_OUT;
	else if (strcmp (buf, "WAN") == 0)
	    link_type = CM_WAN;
	else {
	    LE_send_msg (GL_ERROR | 2,  "bad link type (%s) (file %s, link %d)\n", 
				buf, CS_cfg_name (NULL), link_ind);
	    return (CMC_CONF_READ_FAILED);
	}

	/* get other config numbers */
	if (CS_entry (CS_THIS_LINE, CS_INT | 3, 0, (char *)&device) < 0 ||
	    CS_entry (CS_THIS_LINE, CS_INT | 4, 0, (char *)&port) < 0 ||
	    CS_entry (CS_THIS_LINE, CS_INT | 6, 0, (char *)&rate) < 0 ||
	    CS_entry (CS_THIS_LINE, CS_INT | 8, 0, (char *)&packet_size) < 0 ||
	    CS_entry (CS_THIS_LINE, CS_INT | 9, 0, (char *)&npvcs) < 0 ||
	    CS_entry (CS_THIS_LINE, CS_INT | dn_ind, 0, 
						(char *)&data_notify) < 0) 
	    return (CMC_CONF_READ_FAILED);

	if (rate < 0) {
	    LE_send_msg (GL_ERROR | 3,  
		"bad data rate (file %s, link %d)\n", 
				CS_cfg_name (NULL), link_ind);
	    return (CMC_CONF_READ_FAILED);
	}	    
	if (packet_size < 32) {
	    LE_send_msg (GL_ERROR | 4,  
		"bad packet size (file %s, link %d)\n", 
				CS_cfg_name (NULL), link_ind);
	    return (CMC_CONF_READ_FAILED);
	}	    
	if (npvcs < 0 || npvcs > MAX_N_STATIONS) {
	    LE_send_msg (GL_ERROR | 5,  
		"bad PVC number (file %s, link %d)\n", 
				CS_cfg_name (NULL), link_ind);
	    return (CMC_CONF_READ_FAILED);
	}	   

	link = (Link_struct *)malloc (sizeof (Link_struct));
	if (link == NULL) {
	    LE_send_msg (GL_ERROR | 6,  "malloc failed\n");
	    return (CMC_CONF_READ_FAILED);
	}
	link->link_ind = link_ind;
	link->device = device;
	link->port = port;
	link->link_type = link_type;
	link->line_rate = rate;
	link->packet_size = packet_size;
	link->data_en = data_notify;

	if (npvcs == 0) {
	    link->n_pvc = 1;
	    link->proto = PROTO_HDLC;
	}
	else {
	    link->n_pvc = npvcs;
	    link->proto = PROTO_PVC;
	}

	/*  the following scenario is for dialout lines */
	/*  here we are using a dummy pvc  */
	if (link_type == CM_DIAL_IN_OUT &&
            strcmp (CMC_get_proto_name (), "cm_uconx") == 0) {
	    link->proto = PROTO_HDLC;
	}
	link_ind++;
	return (link);
    }
    return (CMC_CONF_READ_DONE);
}

/**************************************************************************

    Description: This is the CS error call back function.

    Input:	msg - the error msg.

**************************************************************************/

static void Print_cs_error (char *msg)
{

    LE_send_msg (GL_ERROR | 7,  "%s", msg);
    return;
}

/**************************************************************************

    Description: This function initializes the common items of link table 
		and opens the LBs.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Output:	verbose - verbose mode flag;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

void CMC_start_up (int argc, char **argv, int *verbose)
{
    int ret, cnt, i;

    Argc = argc;
    Argv = argv;

    /* close all fd may inherited from parent. We have less than 64 fds */
    for (i = 3; i < 64; i++)
	close (i);

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    /* Initialize the LE and CS services */
    if (Test_mode)
	goto next;
    if (getenv ("LE_DIR_EVENT") != NULL) {
	char tsk_name[NAME_LEN];

	sprintf (tsk_name, "%s.%d", argv[0], Comm_manager_index);
	ret = LE_create_lb (tsk_name, Log_size, LB_SINGLE_WRITER, -1);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR,  "LE_create_lb failed (ret %d)", ret);
	    exit (1);
	}
	LE_set_option ("LE name", tsk_name);
	LE_set_option ("label", tsk_name);

	cnt = 0;
	while (1) {
	    ret = LE_init (argc, argv);
    
	    if (Verbose)
		LE_local_vl (1);
    
	    cnt++;
	    if (cnt < 10 && ret == LE_DUPLI_INSTANCE) {
		LE_send_msg (LE_VL0 | 9,  "LE file locked - will retry");
		sleep (1);
		continue;
	    }
	    else
		break;
	}
	if (ret < 0) {
	    LE_send_msg (GL_ERROR | 10,  "LE_init failed (ret %d)", ret);
	    exit (1);
	}
    }
next:
    Termination_start_time = 0;
    sigset (SIGTERM, Term_handler); 
    sigset (SIGHUP, Term_handler);
/*    sigset (SIGINT, Term_handler); */

    *verbose = Verbose;
    return;
}

/**************************************************************************

    Returns Termination_start_time.

**************************************************************************/

time_t CMC_term_start_time () {
    return (Termination_start_time);
}

/**************************************************************************

    Description: This function initializes the common items of link table, 
		opens the LBs and registers the events.

    Inputs:	n_links - number of links processed;
		links - the link structure list.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int CMC_initialize (int n_links, Link_struct **links)
{
    int reqfd, i, ret, need_event;
    char name [NAME_LEN + 32];
    LB_status status;
    LB_attr attr;

    N_links = n_links;
    Links = links;

    /* open the request LB */
    if (Req_lb_name[0] == '\0') {
#ifndef NO_ORPG
	if (CS_entry ((char *)(ORPGDAT_CM_REQUEST + Comm_manager_index), 
			CS_INT_KEY | 1, NAME_LEN, name) < 0)
#endif
	    return (-1);
    }
    else 
	sprintf (name, "%s.%d", Req_lb_name, Comm_manager_index);
    reqfd = LB_open (name, LB_READ, NULL);
    if (reqfd < 0) {
	LE_send_msg (GL_ERROR | 11,  "LB_open %s failed (ret %d)", name, reqfd);
	return (-1);
    }
    status.n_check = 0;
    status.attr = &attr;
    ret = LB_stat (reqfd, &status);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "LB_stat %s failed (ret %d)", name, ret);
	return (-1);
    }
    if (attr.types & LB_MUST_READ) {
	char b[8];
	LE_send_msg (GL_INFO, "The request LB (%s) is LB_MUST_READ", name);
	ret = LB_seek (reqfd, 0, LB_LATEST, NULL);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "LB_seek %s failed (ret %d)", name, ret);
	    return (-1);
	}
	LB_read (reqfd, b, 8, LB_NEXT);		/* flush all msgs in buffer */
    }
    LB_register (reqfd, LB_ID_ADDRESS, (void *)&Msg_id);

    need_event = 0;
    for (i = 0; i < n_links; i++) {
	int k;
	Link_struct *link;

	link = links[i];
	link->link_state = LINK_DISCONNECTED;
	link->conn_activity = NO_ACTIVITY;
	link->dial_state = NORMAL;
	link->dial_activity = NO_ACTIVITY;
	link->rep_period = 0;
	link->rw_time = 0;
	if (i == 0)
	    link->reqfd = reqfd;
	else
	    link->reqfd = -reqfd;
	link->must_read = 0;
	if (attr.types & LB_MUST_READ)
	    link->must_read = 1;
	if (link->data_en)
	    need_event = 1;

/*	if (link->proto == PROTO_HDLC) */
	    link->n_added_bytes = N_added_bytes;
/*	else
	    link->n_added_bytes = 0;  */

	link->r_seq_num = 0;
	for (k = 0; k < link->n_pvc; k++) {
	    link->r_buf[k] = NULL;
	    link->r_cnt[k] = 0;
	    link->r_buf_size[k] = 0;
	    link->w_buf[k] = NULL;
	}

	link->n_reqs = 0;
	link->st_ind = 0;
	for (k = 0; k < MAX_N_REQS; k++) {
	    link->req[k].state = CM_DONE;
	    link->req[k].data = NULL;
	    link->req[k].data_size = 0;
	}

	link->compress_method = -1;
	link->w_compressed = 0;
	link->max_pack = 10;
	link->pack_time = 0;
	link->pack_cnt = 0;
	link->n_saved_bytes = 0;
	link->saved_msgs = NULL;
	link->pack_buf = NULL;
	link->pack_st_time = 0.;
	link->comp_bytes = 0.;
	link->org_bytes = 0.;

	/* open the response LBs */
	for (k = 0; k < i; k++)	/* find whether the LB is already opened */
	    if (links[k]->link_ind == link->link_ind)
		break;
	if (k < i) 		/* the LB is already opened */
	    link->respfd = links[k]->respfd;
	else {			/* we have to open it */
	    int cnt;

	    if (Resp_lb_name[0] == '\0') {
#ifndef NO_ORPG
		if (CS_entry ((char *)(ORPGDAT_CM_RESPONSE + link->link_ind), 
			CS_INT_KEY | 1, NAME_LEN, name) < 0)
#endif
		    return (-1);
	    }
	    else 
		sprintf (name, "%s.%d", Resp_lb_name, link->link_ind);
	    cnt = 0;
	    while (1) {	/* we retry until previous instance terminates */
		link->respfd = LB_open (name, LB_WRITE, NULL);
		if (link->respfd < 0) {
		    if (link->respfd == LB_TOO_MANY_WRITERS && cnt < 10) {
			LE_send_msg (LE_VL0 | 12,  
				"LB_open (LB_TOO_MANY_WRITERS) - will retry");
			cnt++;
			sleep (1);
			continue;
		    }
		    else {
			LE_send_msg (GL_ERROR | 13,  "LB_open %s failed (ret %d)", 
						name, link->respfd);
			return (-1);
		    }
		}
		else
		    break;
	    }
	    LB_register (link->respfd, LB_ID_ADDRESS, (void *)&Msg_id);
	}
    }

    /* register event */
    if (need_event && Comm_req_event != 0) {
	ret = EN_register (Comm_req_event + Comm_manager_index, En_callback);
	if (ret < 0) {
	    En_flag = -1;
	    LE_send_msg (GL_ERROR | 14,  
		"EN_register failed (ret %d) - event will be ignored", ret);
	}
    }
    else
	En_flag = -1;

    for (i = 0; i < n_links; i++)
	CMRATE_init (links[i]);

    return (0);
}

/**************************************************************************

    Description: This function performs common clean-up works and must be 
		called from comm_manager's termination clean-up function.

**************************************************************************/

void CMC_terminate ()
{
    int i;

    for (i = 0; i < N_links; i++)
	CMPR_send_event_response (Links[i], CM_TERMINATE, NULL);

    return;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

#define OPT_FLAGS_SIZE 64

static int Read_options (int argc, char **argv)
{
    extern char *optarg;	/* used by getopt */
    extern int optind;
    int c;			/* used by getopt */
    int err;			/* error flag */
    char opt_flags[OPT_FLAGS_SIZE] = "I:f:i:o:a:q:r:S:T:Qghvnt?";
    char tc;

    Verbose = 0;
    N_added_bytes = 0;
    Comm_req_event = 0;
    CMC_comm_resp_event = 0;
#ifndef NO_ORPG
    Comm_req_event = ORPGEVT_NB_COMM_REQ;
    CMC_comm_resp_event = ORPGEVT_NB_COMM_RESP;
#endif
    New_instance = 0;
    En_flag = EN_POST_FLAG_LOCAL;

    err = 0;
    Log_size = 2000;
    Comm_manager_index = -1;
    Test_mode = 0;
    strcat (opt_flags, CM_additional_flags ());
    while ((c = getopt (argc, argv, opt_flags)) != EOF) {

	if (CM_parse_additional_args (c, optarg) != 0)
	    err = -1;

	switch (c) {

	    case 'I':
		if (sscanf (optarg, "%d%c", &Comm_manager_index, &tc) != 1)
		    err = -1;
		break;

	    case 'f':
		strncpy (Link_conf_name, optarg, NAME_LEN);
		Link_conf_name [NAME_LEN - 1] = 0;
		break;

	    case 'i':
		strncpy (Req_lb_name, optarg, NAME_LEN);
				/* 3 characters reserved for index */
		Req_lb_name [NAME_LEN - 1] = 0;
		break;

	    case 'o':
		strncpy (Resp_lb_name, optarg, NAME_LEN);
				/* 3 characters reserved for index */
		Resp_lb_name [NAME_LEN - 1] = 0;
		break;

	    case 'q':
		if (sscanf (optarg, "%d", &Comm_req_event) != 1)
		    err = -1;
		break;

	    case 'r':
		if (sscanf (optarg, "%d", &CMC_comm_resp_event) != 1)
		    err = -1;
		break;

	    case 'a':
		if (sscanf (optarg, "%d", &N_added_bytes) != 1 ||
		    N_added_bytes < 0)
		    err = -1;
		break;

	    case 'S':
		if (sscanf (optarg, "%d%c", &Log_size, &tc) != 1)
		    err = -1;
		break;

	    case 'g':
		En_flag = 0;
		break;

	    case 'Q':
		Allow_queued_payload = 1;
		break;

	    case 'v':
		Verbose = 1;
		break;

	    case 'n':
		New_instance = 1;
		break;

	    case 't':
		Test_mode = 1;
		break;

	    case 'T':		/* ingored (for task name) */
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (err == 0 && optind == argc - 1) { /* get the comm manager index  */
	if (Comm_manager_index >= 0) {
	    LE_send_msg (GL_ERROR, "Comm_manager_index doublly specified\n");
	    err = -1;
	}
	else if (sscanf (argv[optind], "%d%c", &Comm_manager_index, &tc) != 1)
	    err = -1;
    }

    if (err == 0 && Comm_manager_index < 0) {
	LE_send_msg (GL_ERROR | 15,  
		"Comm_manager_index not specified or incorrect\n");
	err = -1;
    }

    if (err == 0 && strlen (Link_conf_name) == 0) {
	char *name = "/comms_link.conf";
	int ret;

	ret = MISC_get_cfg_dir (Link_conf_name, NAME_LEN);
	if (ret < 0 || ret + strlen (name) >= NAME_LEN) {
	    LE_send_msg (GL_ERROR | 16,  "link configuration file not specified\n");
	    err = -1;
	}
	else
	    strcat (Link_conf_name, name);
    }

#ifdef NO_ORPG
    if (Req_lb_name[0] == '\0') {
	LE_send_msg (GL_ERROR,  "Request LB not specified\n");
	err = -1;
    }
    if (Resp_lb_name[0] == '\0') {
	LE_send_msg (GL_ERROR,  "Responce LB not specified\n");
	err = -1;
    }
#endif

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

    Inputs:	argv - the list of command arguments

**************************************************************************/

static void Print_usage (char **argv)
{
    printf ("Usage: %s (options) comm_manager_index\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -I comm_manager_index (you can specify comm_manager_index\n");
    printf ("        either this way or as the last argument, but not both) \n");	
    printf ("       -f conf_file_name (alternative link conf file name;\n");	
    printf ("          default: $CFG_DIR/comms_link.conf)\n");
    printf ("       -i req_lb_name (alternative request LB name core;\n");	
    printf ("          default: specified in the system conf text)\n");
    printf ("       -o resp_lb_name (alternative response LB name core;\n");	
    printf ("          default: specified in the system conf text)\n");
    printf ("       -q req_event (alternative request event;\n");	
    printf ("          default: ORPGEVT_NB_COMM_REQ)\n");
    printf ("       -r resp_event (alternative response event;\n");	
    printf ("          default: ORPGEVT_NB_COMM_RESP)\n");
    printf ("       -g (events are sent globally)\n");	
    printf ("       -a number_of_bytes (adding number_of_bytes bytes\n");
    printf ("          in front of each incoming message. A negative\n");
    printf ("          number_of_bytes means stripping bytes. Outgoing\n");
    printf ("          messages are processed accordingly. The default is 0.\n");
    printf ("          This option applies only to the HDLC/ABM protocol.)\n");
    printf ("       -Q (Allows multiple payload messages to be queued in the\n");
    printf ("          request LB)\n");
    printf ("       -S log_size (size of the log-error (LE) data store;\n");
    printf ("          default: 2000 messages)\n");
    printf ("       -v (verbose mode)\n");
    printf ("       -t (test mode: Messages go to stdout instead of the LE file)\n");
    printf ("       -n (a new instance started by this cm_manager)\n");
    CM_print_additional_usage ();

    exit (0);
}

/**************************************************************************

    Description: This is the EN callback function. It does nothing.

    Inputs:	evtcd - event number.
		msg - event message.
		msglen - length of the message.

**************************************************************************/

static void En_callback (EN_id_t evtcd, char *msg, int msglen, void *arg)
{

    return;
}

/**************************************************************************

    Description: This function computes the number of alignment bytes for
		n_bytes.

    Inputs:	n_bytes - number of bytes;

    Return:	The number of alignment bytes for n_bytes.

**************************************************************************/

int CMC_align_bytes (int n_bytes)
{
    int tmp;

    tmp = n_bytes;
    if (tmp < 0)
	tmp = -tmp;
    tmp = ALIGNED_SIZE (tmp);
    if (n_bytes >= 0)
	return (tmp);
    else
	return (-tmp);
}

/**************************************************************************

    Description: This function reallocate the input buffer on link "link",
		PVC "pvc".

    Inputs:	link - the link involved;
		pvc - the index of the PVC.
		size - the new buffer size if non-zero;

    Return:	0 on success or -1 on failure. When it fails, the original
		buffer is untouched.

**************************************************************************/

#define ENLARGE_FACTOR 2	/* factor used for increasing the input 
				   buffers */
#define MAX_ENLARGE_BYTES	16000
				/* maximum number of bytes enlarged */

int CMC_reallocate_input_buffer (Link_struct *link, int pvc, int size)
{
    int new_size;
    char *new_buf;
    int extra_bytes;

    extra_bytes = sizeof (CM_resp_struct) + 
			CMC_align_bytes (link->n_added_bytes);

    if (size > 0)
	new_size = size;
    else {
	int inc;

	inc = link->r_buf_size[pvc] * ENLARGE_FACTOR - link->r_buf_size[pvc];
 	if (inc > MAX_ENLARGE_BYTES)
	    inc = MAX_ENLARGE_BYTES;

	new_size = link->r_buf_size[pvc] + inc + link->packet_size;
    }
    new_buf = malloc (new_size + extra_bytes);
                		/* we allocated extra space for the
	    			   response header and the extra bytes */

    if (new_buf == NULL) {
	LE_send_msg (GL_ERROR | 17,  "malloc failed in Read_data - we continue");
	return (-1);
    }
    if (link->r_cnt[pvc] > 0)
	memcpy (new_buf + extra_bytes, 
        		link->r_buf[pvc], link->r_cnt[pvc]);
    if (link->r_buf[pvc] != NULL)
	CMC_free (link->r_buf[pvc] - extra_bytes);
    link->r_buf[pvc] = new_buf + extra_bytes;
    link->r_buf_size[pvc] = new_size;

    return (0);
}

/**************************************************************************

    Description: This function performs house keeping works. It must be 
		called frequently. It sends statistics report and checks 
		for read/write timed-out. In termination phase, it 
		terminate the process if all links are disconnected.

    Inputs:	n_links - number of links;
		links - the list of the link structure;

**************************************************************************/

#define COMP_REP_TIME 4

void CMC_house_keeping (int n_links, Link_struct **links)
{
    static time_t prev_tm = 0, prev_comp_report_tm = 0;
    time_t cr_time;
    int i;

    if (Termination_start_time) {
	for (i = 0; i < n_links; i++)  {
	    if (links[i]->link_state == LINK_CONNECTED ||
		links[i]->conn_activity != NO_ACTIVITY)
		break;
	}
	if (i >= n_links) {	/* all links are disconnected */
	    LE_send_msg (0,  
		"All links disconnected - process going to terminate");
	    CM_terminate ();
	}
	if (MISC_systime (NULL) > Termination_start_time + 60) {	/*  */
	    LE_send_msg (0,  
		"Termination timer expired - process going to terminate");
	    CM_terminate ();
	}
    }

    cr_time = MISC_systime (NULL);

    CMMON_update (cr_time);
    if (cr_time - prev_tm > 0) {

	for (i = 0; i < n_links; i++)  {
	    Link_struct *link;

	    link = links[i];
	    if (link->rep_period > 0 &&		/* request statistics */
		link->link_state == LINK_CONNECTED &&
		cr_time - link->rep_time >= link->rep_period) {
		XP_statistics_request (link);
		link->rep_time = cr_time;
	    }

	    if (link->rw_time > 0) {		/* check read/write time out */
		int pvc;

		for (pvc = 0; pvc < link->n_pvc; pvc++) {
						/* check read/write time out */

		    if (link->w_buf[pvc] != NULL &&
			link->w_time_out[pvc] == 0 &&
			cr_time - (link->req[(int)link->w_req_ind[pvc]]).state 
							> link->rw_time) {
			CMPR_send_event_response (link, CM_TIMED_OUT, NULL);
			link->w_time_out[pvc] = 1;
		    }
		    if (link->r_cnt[pvc] > 0 &&
			link->read_start_time[pvc] > 0 &&
			cr_time - link->read_start_time[pvc] > link->rw_time) {
			CMPR_send_event_response (link, CM_TIMED_OUT, NULL);
			link->read_start_time[pvc] = 0;
		    }
		}
	    }
	    SH_house_keeping (link, cr_time);
	}
	prev_tm = cr_time;
    }

    /* print compression ratio */
    if (cr_time - prev_comp_report_tm > COMP_REP_TIME) {
	for (i = 0; i < n_links; i++)  {
	    Link_struct *link;

	    link = links[i];
	    if (link->comp_bytes > 0.) {
		char buf[256];
		sprintf (buf, "    Compression ratio %4.2f",  
			link->org_bytes / link->comp_bytes);
		if (link->pack_time > 0)
		    sprintf (buf + strlen (buf), " - msg pack %d ms", 
						link->pack_time);
		LE_send_msg (LE_VL1, "%s on link %d", buf, link->link_ind);
	    }
	}
	prev_comp_report_tm = cr_time;
    }

    return;
}

/**************************************************************************

    Description: This function sends a request for starting a new 
		instance of this comm_manager.

    Input:	wp - waiting period in seconds before starting a new
			instance. -1 cancels the previous restart request.

**************************************************************************/

#define CMD_SIZE 256

void CMC_start_new_instance (int wp)
{
    char cmd[CMD_SIZE];
    int i, len;
    char *cpt;

    len = 0;
    for (i = 0; i <= Argc + 1; i++) {
	if (i < Argc - 1)
	    cpt = Argv[i];
	else if (i == Argc - 1)
	    cpt = "-n ";
	else if (i == Argc)
	    cpt = Argv[Argc - 1];
	else
	    cpt = "&";
	if (strcmp (cpt, "-n") == 0)
	    cpt = "";
	if (len + strlen (cpt) + 1 > CMD_SIZE) {
	    LE_send_msg (GL_ERROR | 18,  "command line too long");
	    CM_terminate ();
	}
	else {
	    sprintf (cmd + len, "%s ", cpt);
	    len += strlen (cpt) + 1;
	}
    }

/*
    LE_send_msg (LE_VL1 | 19,  "new comm_manager instance executed");
    LE_send_msg (LE_VL1 | 20,  "    - %s", cmd);
    system (cmd);
*/

    CMMON_restart_request (cmd, wp);
/*    CM_terminate (); */
    return;			
}
