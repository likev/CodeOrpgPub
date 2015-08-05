
/******************************************************************

	This is the CSS module containing server side routines -
	socket version.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:31 $
 * $Id: css_server.c,v 1.24 2012/07/27 19:33:31 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */  

#include <config.h>
#ifdef __WIN32__
#define __INTERIX
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if !defined(LINUX) && !defined(__WIN32__)
#include <inttypes.h>
#endif
#include <sys/un.h>

#include <rmt.h> 
#include <misc.h> 
#include <net.h> 
#include <css_def.h>


#define CSS_ERR_MSG_SIZE 128	/* max error msg size */

struct User_table {		/* server side user struct */
    struct User_table *next;	/* the next item in the linked list */
    int uid;			/* user ID */
    char *password;		/* user password */
};

typedef struct User_table User_table_t;

/* The first 8 fields must be identical to those in Generic_entry_t */
struct Server_table {		/* server struct */
    struct Server_table *next;	/* the next item in the linked list */
    int id;			/* table entry ID - server ID */
    int poll_flag;		/* to poll: POLL_IN_FLAGS, POLLOUT */
    int ready_flag;		/* poll result: POLL_IN_RFLAGS ... */
    short type;			/* table type */
    short status;		/* connection status */
    short state;		/* the connection state */
    short refuse_time;		/* second the connection was refused */
    int fd;			/* server socket fd */

    unsigned int ipaddr;	/* server IP address */
    int port;			/* server port number */
    int maxn_conn;		/* max number of connections allowed */
    int n_conn;			/* number of connected clients */
    int n_users;		/* number of potential users */
    User_table_t *users;	/* user linked list */
};

typedef struct Server_table Server_table_t;

/* The first 9 fields must be identical to those in Generic_entry_t */
struct Client_table {		/* server side client struct */
    struct Client_table *next;	/* the next item in the linked list */
    int id;			/* table entry ID - connection ID */
    int poll_flag;		/* to poll: POLL_IN_FLAGS, POLLOUT */
    int ready_flag;		/* poll result: POLL_IN_RFLAGS ... */
    short type;			/* table type */
    short status;		/* connection status */
    int state;			/* the connection state */
    int fd;			/* socket fd */
    input_buffer_t req;		/* buffer for request */

    Server_table_t *sv;		/* saver structure */
    unsigned int clip;		/* client IP address */
    int n_omsgs;		/* number of pending outgoing messages */
    Message_list_t *omsg_list;	/* pointers to outgoing messages */
    CSS_client_t cl;		/* client struct passed to CSS users */
};

typedef struct Client_table Client_table_t;

static Client_table_t *Cls = NULL;
				/* server side client table - linked list */
static int N_cls = 0;		/* number of the connected clients */
static Server_table_t *Servers = NULL;
				/* active server table - linked list */
static int N_servers = 0;	/* number of the servers */


static int Close_client (Client_table_t *cl);
static void Accept_client (Server_table_t *sv);
static void Process_log_in (Client_table_t *cl);
static int Close_server (Server_table_t *sv);


/******************************************************************

    Removes all client entries to be deleted and returns Cls and 
    N_cls. Note that we cannot remove an entry in CSS_wait if the
    client connection is lost. So we mark it there and remove here.

******************************************************************/

int CSS_get_sv_client_entries (Generic_entry_t **entries) {
    Client_table_t *cl;

    cl = Cls;
    while (cl != NULL) {
	cl = Cls;
	while (cl != NULL) {
	    if (cl->state == TO_BE_DELETED) {
		Close_client (cl);
		break;
	    }
	    cl = cl->next;
	}
    }
    *entries = (Generic_entry_t *)Cls;
    return (N_cls);
}

/******************************************************************

    Returns Servers and N_servers.

******************************************************************/

int CSS_get_server_entries (Generic_entry_t **entries) {
    *entries = (Generic_entry_t *)Servers;
    return (N_servers);
}

/******************************************************************

    Description: This is the CSS server's main loop function. This
		function waits for a request and passes it to the
		request processing function. It also calls the 
		housekeeping function periodically. This function 
		never returns and can only be called once.

    Inputs:	sv_addr - server address (host_name:port_number).
		maxn_conns - max number of client connections.
		hk_seconds - housekeeping period in seconds.
		proc_req_func - function processing a request.
		housekeep_func - house keeping function.

    Return:	returns a CSS error code on failure.

******************************************************************/

int CSS_sv_main (char *sv_addr, int maxn_conns, int hk_seconds,
		int (*proc_req_func)(char *, int, char **), 
		int (*housekeep_func)(void)) {
    unsigned int sv_id;
    time_t prev_hk = 0;		/* previous housekeeping time */

    if (hk_seconds < 0)
	return (CSS_BAD_ARGUMENT);

    sv_id = CSS_sv_init (sv_addr, maxn_conns);
    if (sv_id < 0)
	return (sv_id);

    while (1) {
	char *req, *resp;
	int cid, req_len, resp_len;
	time_t tm;
	CSS_client_t *client;

	cid = CSS_wait (1000, NULL);
	if (cid < 0)
	    continue;

	else {
	    req_len = CSS_sv_get_request (cid, &client, &req);
	    if (req_len > 0) {
		resp_len = proc_req_func (req, req_len, &resp);
		if (resp_len > 0) {
		    CSS_sv_send_response (client, resp_len, resp);
		    if (resp != NULL)
		        free (resp);
		}
		else 
		    MISC_log ("CSS: proc_req_func failed\n");
	    }
	}
	tm = 0;
	if (housekeep_func != NULL &&
	    (hk_seconds == 0 || (tm = MISC_systime (NULL)) - prev_hk >= hk_seconds)) {
	    housekeep_func ();
	    prev_hk = tm;
	}
    }
}

/******************************************************************

    Description: Reads the next request. See "man css" for more details.

    Input/Output: See "man css".

    Return:	length of the request on success or a negative CSS 
		error number.

******************************************************************/

int CSS_sv_get_request (int cid, CSS_client_t **client, char **req_ret) {
    Client_table_t *cl;
    int ret;

    cl = (Client_table_t *)CSS_get_entry_by_id (Cls, cid);
    if (cl == NULL)
	return (CSS_BAD_ARGUMENT);
    if (cl->status == ST_LOST_CONN) {
	if (cl->state == TO_BE_DELETED)
	    Close_client (cl);
	return (CSS_LOST_CONN);
    }
    if (cl->state != CONNECTED)
	return (0);

    ret = CSS_input_data (cl->fd, &(cl->req));
    if (ret > 0) {
	Css_msg_header_t *hd;
	hd = &(cl->req.hd);
	cl->cl.seq = hd->seq;
	*req_ret = cl->req.buf;
	cl->req.n_bytes = 0;
	*client = &(cl->cl);
	return (cl->req.msg_size);
    }
    if (ret == 0)
	return (0);
    if (ret < 0)
	Close_client (cl);
    if (ret == NET_DISCONNECTED)
	return (CSS_LOST_CONN);
    else
	return (ret);
}

/******************************************************************

    Description: Sends a message to a client. See "man css" for more 
		details.

    Input/Output: See "man css".

    Return:	number of queued messages on success or a negative CSS 
		error number.

******************************************************************/

int CSS_sv_send_msg (int sv_id, int conn_n, unsigned int seq,
					int msg_len, char *msg) {
    Client_table_t *cl;

    cl = Cls;
    while (cl != NULL) {
	if (cl->cl.sv_id == sv_id && cl->cl.conn_n == conn_n && conn_n >= 0) {
	    cl->cl.seq = seq;
	    return (CSS_sv_send_response (&(cl->cl), msg_len, msg));
	}
	cl = cl->next;
    }
    return (CSS_BAD_ARGUMENT);
}

/******************************************************************

    Description: Sends a response to a client. See "man css" for more 
		details.

    Input/Output: See "man css".

    Return:	number of queued messages on success or a negative CSS 
		error number.

******************************************************************/

int CSS_sv_send_response (CSS_client_t *client, int msg_len, char *msg) {
    Client_table_t *cl;
    int ret;

    cl = (Client_table_t *)CSS_get_entry_by_id (Cls, client->cid);
    if (cl == NULL)
	return (CSS_BAD_ARGUMENT);
    if (cl->status == ST_LOST_CONN) {
	if (cl->state == TO_BE_DELETED)
	    Close_client (cl);
	return (CSS_LOST_CONN);
    }

    ret = CSS_output_msg ((Generic_entry_t *)cl, msg, msg_len, 
					client->seq, &(cl->omsg_list));
    if (ret < 0) {
	Close_client (cl);
	if (ret == NET_DISCONNECTED)
	    return (CSS_LOST_CONN);
	else
	    return (ret);
    }
    cl->n_omsgs = ret;

    return (ret);
}

/******************************************************************

    Performs routine processing on the server side of connection "ent".

******************************************************************/

void CSS_process_server_side (Generic_entry_t *ent) {
    Client_table_t *cl;
    int ret;

    if (ent->type == SERVER_T) {
	Accept_client ((Server_table_t *)ent);
	return;
    }

    cl = (Client_table_t *)ent;
    if (cl->status == ST_LOST_CONN)
	return;
    if (cl->state != CONNECTED)		/* verify the client */
	Process_log_in (cl);

    if (cl->ready_flag & POLL_IN_RFLAGS) {
	ret = CSS_input_data (cl->fd, &(cl->req));
	if (ret < 0)
	    cl->state = TO_BE_DELETED;
	if (ret == NET_DISCONNECTED)
	    cl->status = ST_LOST_CONN;
    }
    if (cl->ready_flag & POLLOUT) {
	ret = CSS_output_msg ((Generic_entry_t *)cl, NULL, 0, 
					0, &(cl->omsg_list));
	if (ret < 0)
	    cl->state = TO_BE_DELETED;
	if (ret == NET_DISCONNECTED)
	    cl->status = ST_LOST_CONN;
    }
}

/******************************************************************

    Description: Initializes a server. See "man css" for more details.

    Input/Output: See "man css".

    Return:	server ID on success or a negative CSS error number.

******************************************************************/

int CSS_sv_init (char *sv_addr, int maxn_conns) {
    unsigned int ipaddr;
    unsigned short usport;
    int port, fd, i;
    struct sockaddr_in loc_soc;	/* local socket info */
    Server_table_t *sv;
    char *cpt;

    if (sv_addr == NULL || maxn_conns <= 0)
	return (CSS_BAD_ARGUMENT);

    MISC_sig_sigset (SIGPIPE, SIG_IGN);

    port = CSS_get_ipaddr_port (sv_addr, &ipaddr);
    if (port < 0)
	return (port);

    cpt = sv_addr;
    while (*cpt != '\0' && *cpt != ':')
	cpt++;
    if (*cpt == '\0' || cpt == sv_addr)
	ipaddr = RMT_bind_address ();

    for (i = 0; i < N_servers; i++) {
	if (Servers[i].ipaddr == ipaddr && Servers[i].port == port) {
	    if (Servers[i].maxn_conn == maxn_conns)
		return (Servers[i].id);
	    else
		return (CSS_DUPLICATED_SERVER);
	}
    }

    if ((sv = (Server_table_t *)malloc (sizeof (Server_table_t))) == NULL)
	return (CSS_MALLOC_FAILED);

    /* get a file descriptor for the server */
    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	MISC_log ("CSS: socket failed (errno %d)\n", errno);
	return (CSS_OPEN_SOCKET_FAILED);
    }

    loc_soc.sin_family = AF_INET;
    loc_soc.sin_addr.s_addr = ipaddr;
    usport = port;
    loc_soc.sin_port = htons (usport);

    i = 1;
    if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, 
					(char *)&i, sizeof (int)) < 0) {
	MISC_log ("CSS: setsockopt SO_REUSEADDR failed (errno %d)\n", errno);
	close (fd);
	return (CSS_SET_SOCK_PROP_FAILED);
    }

    /* bind to a local port */
    errno = 0;
    if (bind (fd, (struct sockaddr *)&loc_soc, sizeof (loc_soc)) < 0) {
	MISC_log ("CSS: bind failed (errno %d)\n", errno);
	close (fd);
	return (CSS_BIND_FAILED);
    }

    if (NET_set_TCP_NODELAY (fd) < 0 ||
	NET_set_linger_off (fd) < 0 ||
	NET_set_non_block (fd) < 0) {
	close (fd);
	return (CSS_SET_SOCK_PROP_FAILED);
    }

    /* Wait for remote connection request */
    errno = 0;
    if (listen (fd, 5) < 0) {
	MISC_log ("CSS: listen failed (errno %d)\n", errno);
	close (fd);
	return (CSS_LISTEN_FAILED);
    }

    sv->next = NULL;
    sv->id = CSS_next_id ();
    sv->poll_flag = POLL_IN_FLAGS;
    sv->ready_flag = 0;
    sv->type = SERVER_T;
    sv->status = ST_NORMAL;
    sv->state = CONNECTED;
    sv->fd = fd;
    sv->ipaddr = ipaddr;
    sv->port = port;
    sv->maxn_conn = maxn_conns;
    sv->n_conn = 0;
    sv->n_users = 0;
    sv->users = NULL;
    if (N_servers == 0)
	Servers = sv;
    else
	(((Server_table_t *)CSS_get_indexed_entry 
				(Servers, N_servers - 1)))->next = sv;
    N_servers++;

    return (sv->id);
}

/**************************************************************************

    Accepts a client connection request.

    Inputs:	sv - the server struct.

**************************************************************************/

static void Accept_client (Server_table_t *sv) {
    int sockfd;			/* client socket fd */
    int len;			/* address buffer length */
    static union sunion {
	struct sockaddr_in sin;
	struct sockaddr_un sund;
    } sadd;			/* the client address */
    Client_table_t *cl;

    len = sizeof (struct sockaddr_in);
    while (1) {
	errno = 0;
	sockfd = accept (sv->fd, (struct sockaddr *) &sadd, (socklen_t *)&len);
	if (sockfd < 0) {
	    if (errno == EINTR) continue;   /* retry */
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return;		/* not yet connected */
	    MISC_log ("CSS: accept failed (errno %d)\n", errno);
	    return;
	}
	break;
    }

    if (NET_set_TCP_NODELAY (sockfd) < 0 ||
	NET_set_linger_off (sockfd) < 0 ||
	NET_set_non_block (sockfd) < 0) {
	close (sockfd);
	return;
    }

    if ((cl = (Client_table_t *)malloc (sizeof (Client_table_t))) == NULL) {
	close (sockfd);
	MISC_log ("CSS: malloc failed\n");
	return;
    }

    cl->next = NULL;
    cl->id = CSS_next_id ();
    cl->poll_flag = POLL_IN_FLAGS;
    cl->ready_flag = 0;
    cl->type = SV_CL_T;
    cl->status = ST_NORMAL;
    cl->state = CONN_PENDING;
    cl->fd = sockfd;
    cl->req.n_bytes = 0;
    cl->req.msg_size = -1;
    cl->req.buf = NULL;
    cl->sv = sv;
    cl->clip = ntohl (sadd.sin.sin_addr.s_addr);/* client host address */
    cl->n_omsgs = 0;
    cl->omsg_list = NULL;
    cl->cl.sv_id = sv->id;
    cl->cl.cid = cl->id;
    /* cl->cl.conn_n, cl->cl.seq and cl->cl.user_id will be set later */
    if (N_cls == 0)
	Cls = cl;
    else
	(((Client_table_t *)CSS_get_indexed_entry (Cls, N_cls - 1)))->next 
							= cl;
    N_cls++;

    return;
}

/******************************************************************

    Closes connection of ID "cid" on server side or a server.

    Return:	0 on success or, otherwise a CSS error code.

******************************************************************/

int CSS_server_side_close (int cid) {
    Client_table_t *cl;
    Server_table_t *sv;

    cl = (Client_table_t *)CSS_get_entry_by_id (Cls, cid);
    if (cl != NULL)
	return (Close_client (cl));
    sv = (Server_table_t *)CSS_get_entry_by_id (Servers, cid);
    if (sv != NULL)
	return (Close_server (sv));
    return (CSS_BAD_ARGUMENT);
}

/******************************************************************

    Closes all clients served by server "sv" and cleans up resources 
    for the server.

    Return:	0 on success or a CSS error code otherwise.

******************************************************************/

static int Close_server (Server_table_t *sv) {
    Client_table_t *cl, *next;
    User_table_t *user;

    cl = Cls;
    while (cl != NULL) {	/* close all clients */
	if (cl->sv == sv) {
	    next = cl->next;
	    Close_client (cl);
	    cl = next;
	}
	else
	    cl = cl->next;
    }

    user = sv->users;
    while (user != NULL) {
	User_table_t *next;
	next = user->next;
	free (user);
	user = next;
    }
    Servers = (Server_table_t *)CSS_remove_entry (Servers, sv);
    N_servers--;
    return (0);
}

/******************************************************************

    Cleans up resources for server side client "cl".

    Return:	0 on success or a CSS error code otherwise.

******************************************************************/

static int Close_client (Client_table_t *cl) {
    Message_list_t *list;

    if (cl->fd >= 0)
	close (cl->fd);
    list = cl->omsg_list;
    while (list != NULL) {
	Message_list_t *next;
	next = list->next;
	free (list);
	list = next;
    }
    if (cl->req.buf != NULL)
	free (cl->req.buf);
    Cls = (Client_table_t *)CSS_remove_entry (Cls, cl);
    N_cls--;
    return (0);
} 

/******************************************************************

    Reads and verifies the log in message.

    Return:	0 on success or a CSS error code otherwise.

******************************************************************/

static void Process_log_in (Client_table_t *cl) {
    int ret;

    ret = CSS_input_data (cl->fd, &(cl->req));
    if (ret > 0) {
	char *login_msg;

	login_msg = cl->req.buf;
	login_msg[cl->req.msg_size - 1] = '\0';
	if (cl->req.msg_size < 128) {
	    char password[128];
	    int user_id, conn_n;

	    login_msg = cl->req.buf;
	    login_msg[cl->req.msg_size - 1] = '\0';
	    password[0] = '\0';
	    if (sscanf (login_msg, "%d %d %s", 
				&user_id, &conn_n, password) >= 2) {
		int user_ok = 1;
		/* password verification here */

		if (user_ok && conn_n >= 0) {	/* check if conn_n is used */
		    Client_table_t *c = Cls;
		    while (c != NULL) {
			if (c->sv == cl->sv && c->cl.conn_n == conn_n &&
					c->state == CONNECTED) {
			    MISC_log (
				"CSS: conn number already in use (%d)\n", 
								conn_n);
			    user_ok = 0;
			    break;
			}
			c = c->next;
		    }
		}
		if (user_ok) {
		    cl->cl.user_id = user_id;
		    cl->cl.conn_n = conn_n;
		    cl->state = CONNECTED;
		    cl->req.n_bytes = 0;	/* discard the msg */
		    return;
		}
	    }
	}
	else
	    login_msg[127] = '\0';
	MISC_log ("CSS: bad login message (%s)\n", login_msg);
	Close_client (cl);
	return;
    }
}


