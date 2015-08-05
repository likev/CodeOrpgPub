/****************************************************************
		
    This module implements the server part of the onw-way-replicator
    (owr).

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/10/08 19:36:31 $
 * $Id: owr_server.c,v 1.4 2011/10/08 19:36:31 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* System include files */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <orpg.h>
#include <infr.h>
#include "owr_def.h"

static char *Server_ip = NULL;
static int Port_number = 0;
int Verbose = 0;
static int Use_le = 0;
int Terminating = 0;
static int Sv_id;
static int Rpg_cfg_changed = 0;

static int N_clients = 0;
static Client_t *Clients = NULL;

static void Print_usage (char **argv);
static int Read_options (int argc, char **argv);
static void Close_client (int cid, int close);
static Client_t *Get_client (CSS_client_t *client);
static int Process_request (int req_len, char *req, Client_t *cl);
static void Housekeeping (Client_t *cl);
static void En_ready_callback (int fd, int ready_flag);
static void Signal_handler (int sig);
static void Send_keep_alive (Client_t *cl);
static void En_callback (EN_id_t event, char *msg, int len, void *arg);
static void Disconnect_all_clients ();


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    char addr[256];
    int ret, en_fd;

    sigset (SIGTERM, Signal_handler); 
    sigset (SIGHUP, Signal_handler);
    sigignore (SIGPIPE);
    EN_control (EN_SET_SIGNAL, EN_NTF_NO_SIGNAL);

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Use_le) {
	LE_set_option ("LB type", LB_SINGLE_WRITER);
	LE_set_option ("LB size", 5000);
	if ((ret = LE_init (argc, argv)) < 0) {
	    if (ret == LE_DUPLI_INSTANCE)
		MISC_log ("Another owr_server is running\n");
	    else
		MISC_log ("LE_init failed (%d)\n", ret);
	    exit (1);
	}
    }

    ret = EN_register (ORPGEVT_DATA_STORE_CREATED, En_callback);
    if (ret < 0) {
	MISC_log ("Failed in EN_register (%d)\n", ret);
	exit (1);
    }

    if (Port_number == 0)
	Port_number = OC_get_env_port ();
    if (Port_number == 0) {
	MISC_log ("Port number is not found\n");
	exit (1);
    }

    if (Server_ip != NULL)
	sprintf (addr, "%s:%d", Server_ip, Port_number);
    else
	sprintf (addr, "%d", Port_number);
    Sv_id = CSS_sv_init (addr, 1);
    if (Sv_id < 0) {
	MISC_log ("CSS_sv_init failed (ret %d)\n", Sv_id);
	exit (1);
    }
    MISC_log ("CSS_sv_init returns %d (bind to %s)\n", Sv_id, addr);

    en_fd = EN_control (EN_GET_NTF_FD);
    if (en_fd < 0) {
	MISC_log ("Connecting to event server failed (%d)\n", en_fd);
	exit (1);
    }
    ret = CSS_add_poll_fd (en_fd, POLL_IN_RFLAGS, En_ready_callback);
    if (ret < 0) {
	MISC_log ("CSS_add_poll_fd failed (%d)\n", ret);
	exit (1);
    }
    EN_control (EN_SET_SENDER_ID, getpid ());

    while (1) {
	int flag, cid;

	cid = CSS_wait (1000, &flag);
	if (cid < 0) {
	    MISC_log ("CSS_wait failed (ret %d) - owr_server exiting\n", cid);
	    exit (1);
	}

	if (cid > 0) {
	    CSS_client_t *client;
	    Client_t *cl;
	    char *req;

	    if (flag != REQUEST_READY) {
		MISC_log ("Bad CSS_wait return flag %d\n", flag);
		continue;
	    }
	    while (1) {
		int len = CSS_sv_get_request (cid, &client, &req);
		if (len == 0)
		    break;
		if (len == CSS_LOST_CONN) {
		    MISC_log ("Client connetion %d lost\n", cid);
		    Close_client (cid, 0);
		    break;
		}
		else if (len < 0) {
		    MISC_log ("CSS_sv_get_request failed (ret %d)\n", len);
		    Close_client (cid, 1);
		    break;
		}

		cl = Get_client (client);
		Process_request (len, req, cl);
		Housekeeping (cl);
	    }
	}
	Housekeeping (NULL);

	if (Rpg_cfg_changed) {
	    MISC_log ("RPG configuration changed\n");
	    Rpg_cfg_changed = 0;
	    Disconnect_all_clients ();
	}
    }

    exit (0);
}

/******************************************************************

    Signal handler.

******************************************************************/

static void Signal_handler (int sig) {

    MISC_log ("Signal %d received and ignored\n", sig);
}

/************************************************************************

    Callback function for processng RPG events.

************************************************************************/

static void En_callback (EN_id_t event, char *msg, int len, void *arg) {

    if (event == ORPGEVT_DATA_STORE_CREATED) {
	Rpg_cfg_changed = 1;
	return;
    }
}

/************************************************************************

    Callback function when event fd has input data.

************************************************************************/

static void En_ready_callback (int fd, int ready_flag) {

    EN_control (EN_WAIT, 0);
}

/******************************************************************

    Closes all clients and fees the resources for them.

******************************************************************/

static void Disconnect_all_clients () {
    int i;

    for (i = 0; i < N_clients; i++) {
	Client_t *cl = Clients + i;
	if (cl->stat == ST_DELETED)
	    continue;
	Close_client (cl->client.cid, 1);
    }
}

/************************************************************************

    Processes request "req" of "req_len" bytes. Generates the response
    and sends it to the client.

************************************************************************/

static int Process_request (int req_len, char *req, Client_t *cl) {
    Message_t *rq;
    
    cl->receive_tm = MISC_systime (NULL);
    rq = (Message_t *)req;
    switch (rq->type) {

	case REQ_LB_REP:
	    SF_process_req_lb_rep (cl, req_len, rq);
	    break;

	case LB_UPDATE:
	    SF_lb_update (cl, req_len, rq);
	    break;

	case KEEP_ALIVE:
	    break;

	case REQ_NOT_REP:
	    SF_procedss_not_rep (cl, req_len, rq);
	    break;

	default:
	    MISC_log ("Unexpected message type %d from client\n", rq->type);
	    break;
    }
    return (0);
}

/************************************************************************

    Processes house keeping for client "cl" or all clients if cl is NULL.

************************************************************************/

static void Housekeeping (Client_t *cl) {
    time_t crt;
    int i;

    crt = MISC_systime (NULL);
    for (i = 0; i < N_clients; i++) {
	Client_t *c = Clients + i;
	if (cl != NULL && c != cl)
	    continue;
	if (c->stat == ST_DELETED)
	    continue;
	if (crt > c->receive_tm + 2 * KEEP_ALIVE_TIME) {
	    MISC_log ("Connection to client %d not kept\n", c->client.cid);
	    Close_client (c->client.cid, 1);
	}
	if (crt >= c->send_tm + KEEP_ALIVE_TIME)
	    Send_keep_alive (c);
    }
    return;
}

/******************************************************************

    Sends a keep-alive message to the server.

******************************************************************/

static void Send_keep_alive (Client_t *cl) {
    Message_t msg;

    memset (&msg, 0, sizeof (Message_t));
    msg.type = KEEP_ALIVE;
    msg.size = sizeof (Message_t);
    SV_send_msg_to_client (cl, (char *)&msg, msg.size);
}

/************************************************************************

    Sends message "msg" of "len" bytes to client "client".

************************************************************************/

void SV_send_msg_to_client (Client_t *cl, char *msg, int len) {
    int ret;

    ret = CSS_sv_send_response (&cl->client, len, msg);
    if (ret == CSS_LOST_CONN) {
	MISC_log ("Client %d connetion lost\n", cl->client.cid);
	Close_client (cl->client.cid, 0);
    }
    else if (ret < 0) {
	MISC_log ("CSS_sv_send_response failed (ret %d)\n", ret);
	Close_client (cl->client.cid, 1);
    }
    cl->send_tm = MISC_systime (NULL);
}

/************************************************************************

    Returns the pointer to the client struct for client id cid.

************************************************************************/

static Client_t *Get_client (CSS_client_t *client) {
    int i;
    Client_t cl;

    for (i = 0; i < N_clients; i++) {
	if (Clients[i].stat != ST_DELETED && 
				Clients[i].client.cid == client->cid)
	    return (Clients + i);
    }
    for (i = 0; i < N_clients; i++) {
	if (Clients[i].stat == ST_DELETED)
	    break;
    }

    memset (&cl, 0, sizeof (Client_t));
    memcpy (&(cl.client), client, sizeof (CSS_client_t));
    cl.stat = ST_CONN;
    cl.receive_tm = MISC_systime (NULL);
    if (i >= N_clients) {
	Clients = (Client_t *)STR_append (Clients, &cl, sizeof (Client_t));
	N_clients++;
    }
    else {
	memcpy (Clients + i, &cl, sizeof (Client_t));
    }
    MISC_log ("New client %d connected\n", client->cid);

    return (Clients + i);
}

/************************************************************************

    Frees the client struct for client ID "cid". If "close" is true, the
    CSS connection is closed first.

************************************************************************/

static void Close_client (int cid, int close) {
    int i;

    if (close)
	CSS_close (cid);
    for (i = 0; i < N_clients; i++) {
	if (Clients[i].stat != ST_DELETED && Clients[i].client.cid == cid)
	    break;
    }
    if (i >= N_clients)
	return;
    SF_free_resources (Clients + i);
    if (Clients[i].ds != NULL)
	STR_free ((char *)Clients[i].ds);
    Clients[i].ds = NULL;
    Clients[i].stat = ST_DELETED;
    return;
}

/************************************************************************

    Returns the client table.

************************************************************************/

int SV_get_clients (Client_t **clients) {

    *clients = Clients;
    return (N_clients);
}

/**************************************************************************

    Reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "i:p:lvh?")) != EOF) {

	switch (c) {

            case 'p':
		if (sscanf (optarg, "%d", &Port_number) != 1) {
		    fprintf (stderr, 
				"unexpected -p specification (%s)\n", optarg);
		    exit (1);
		}
                break;

            case 'i':
		Server_ip = STR_copy (Server_ip, optarg);
                break;

            case 'l':
		Use_le = 1;
                break;

            case 'v':
		Verbose = 1;
                break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind < argc)	{	/* any extra command line item  */
	fprintf (stderr, "unexpected command line item (%s)\n", argv[optind]);
	exit (1);
    }

    return (err);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        The server of the owr service.\n\
        Options:\n\
          -p port_number (Server's TCP port number. The default is \n\
             RMTPORT + 3 or 40000)\n\
          -i IP (IP to bind to. The default is any local address)\n\
          -v (Verbose mode)\n\
          -l (Logs to LE file)\n\
          -h (Prints usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}

