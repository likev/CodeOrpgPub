
/******************************************************************

	This is the CSS module containing client side routines -
	socket version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/19 19:35:07 $
 * $Id: css_client.c,v 1.23 2011/05/19 19:35:07 jing Exp $
 * $Revision: 1.23 $
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
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#if !defined(LINUX) && !defined(__WIN32__)
#include <inttypes.h>
#endif
#include <sys/socket.h>
#include <errno.h>

#include <misc.h>
#include <net.h>
#include <css_def.h>


struct name_list {		/* service name list */
    char *name;
    struct name_list *next;
};
typedef struct name_list Name_list_t;

/* The first 9 fields must be identical to those in Generic_entry_t */
struct Svconn_table {		/* server table on the client side */
    struct Svconn_table *next;	/* the next item in the linked list */
    int id;			/* table entry ID - connection ID */
    int poll_flag;		/* to poll: POLL_IN_FLAGS, POLLOUT */
    int ready_flag;		/* poll result: POLL_IN_RFLAGS ... */
    short type;			/* table type */
    short status;		/* connection status */
    short state;		/* the connection state */
    short refuse_time;		/* second the connection was refused */
    int fd;			/* socket fd */
    input_buffer_t resp;	/* buffer for response */

    Name_list_t *name_list;	/* service name */
    unsigned int ipaddr;	/* server IP address */
    int port;			/* server port number */
    short conn_n;		/* connection number */
    short n_omsgs;		/* number of pending outgoing messages */
    int user_id;		/* ID used for identitifying this conn */
    char *password;		/* password */
    unsigned int req_seq;	/* request sequence number last sent. Used
				   also for login message control. */
    Message_list_t *omsg_list;	/* pointers to outgoing messages */
};

typedef struct Svconn_table Svconn_table_t;

static Svconn_table_t *Svs = NULL;	/* server table (linked list) */
static int N_svs = 0;		/* size of the server table */


static int Open_conn_to_server (Svconn_table_t *sv);
static int Close_connection (Svconn_table_t *sv);
static int Remove_service_name (char *s_name, Svconn_table_t *sv);


/******************************************************************

    Returns Svs and N_svs.

******************************************************************/

int CSS_get_client_entries (Generic_entry_t **cls) {
    *cls = (Generic_entry_t *)Svs;
    return (N_svs);
}

/******************************************************************

    For backward compatibility.

******************************************************************/

int CSS_close_conn (int cid) {
    return (0);
}

/******************************************************************

    Description: Sets a service name and its corresponding server 
		address. A server address is the name of the LB 
		that accepts query requests. Multiple services
		can have the same address.

    Input:	s_name - service name;
		sv_addr - server address.
		conn_n - connection number.

    Return:	The connection ID on success or a negative CSS error 
		number.

******************************************************************/

int CSS_set_server_address (char *s_name, char *sv_addr, int conn_n) {
    Svconn_table_t *new_ent;
    Name_list_t *new_name;
    unsigned int ipaddr;
    int addr_ind, port, id, i;

    if (s_name == NULL)
	return (CSS_BAD_ARGUMENT);

    MISC_sig_sigset (SIGPIPE, SIG_IGN);

    port = 0;		/* not necessary - turn off gcc warning */
    if (sv_addr != NULL &&
	(port = CSS_get_ipaddr_port (sv_addr, &ipaddr)) < 0)
	return (port);

    if ((id = CSS_get_cid (s_name)) >= 0) {
					/* s_name already registered */
	Svconn_table_t *sv;
	sv = (Svconn_table_t *)CSS_get_entry_by_id (Svs, id);
	if (sv_addr == NULL)
	    return (Remove_service_name (s_name, sv));
	else {
	    if (sv->ipaddr == ipaddr && sv->port == port && 
			    (sv->conn_n == conn_n || conn_n == CSS_ANY_LINK))
		return (sv->id);
	    else
		return (CSS_SERVER_ADDR_RESET);
	}
    }
    else if (sv_addr == NULL)
	return (CSS_BAD_ARGUMENT);

    new_name = (Name_list_t *)malloc (sizeof (Name_list_t) + 
						strlen (s_name) + 1);
    if (new_name == NULL)
	return (CSS_MALLOC_FAILED);
    new_name->next = NULL;
    new_name->name = (char *)new_name + sizeof (Name_list_t);
    strcpy (new_name->name, s_name);

    addr_ind = -1;
    for (i = 0; i < N_svs; i++) {
	Svconn_table_t *sv;
	sv = (Svconn_table_t *)CSS_get_indexed_entry (Svs, i);
	if (sv->ipaddr == ipaddr && sv->port == port) {
	    if (sv->conn_n == conn_n || conn_n == CSS_ANY_LINK) {
		Name_list_t *nl;		/* add to name list */

		nl = sv->name_list;
		while (nl->next != NULL)
		    nl = nl->next;
		nl->next = new_name;
		return (sv->id);
	    }
	    addr_ind = i;
	    break;
	}
    }    

    new_ent = (Svconn_table_t *)malloc (sizeof (Svconn_table_t));
    if (new_ent == NULL) {
	free (new_name);
	return (CSS_MALLOC_FAILED);
    }

    new_ent->name_list = new_name;
    new_ent->ipaddr = ipaddr;
    new_ent->port = port;
    new_ent->fd = -1;
    new_ent->type = CLIENT_T;
    new_ent->poll_flag = new_ent->ready_flag = 0;
    new_ent->conn_n = conn_n;
    new_ent->user_id = 0;
    new_ent->password = "";
    new_ent->req_seq = 0;
    new_ent->state = DISCONNECTED;
    new_ent->refuse_time = 0;
    new_ent->status = ST_NORMAL;
    new_ent->n_omsgs = 0;
    new_ent->omsg_list = NULL;
    new_ent->resp.n_bytes = 0;
    new_ent->resp.msg_size = -1;
    new_ent->resp.buf = NULL;
    new_ent->next = NULL;
    new_ent->id = CSS_next_id ();
    if (N_svs == 0)
	Svs = new_ent;
    else
	(((Svconn_table_t *)CSS_get_indexed_entry (Svs, N_svs - 1)))->next 
							= new_ent;
    N_svs++;

    return (new_ent->id);
}

/******************************************************************

    Description: Searches for the service name "s_name" in the conn 
		registration table. 

    Input:	s_name - service name;

    Return:	The conn registration table entry ID if found 
		or CSS_S_NAME_NOT_FOUND otherwise.

******************************************************************/

int CSS_get_cid (char *s_name) {
    int i;
    Svconn_table_t *sv;

    sv = Svs;
    for (i = 0; i < N_svs; i++) {
	Name_list_t *nl;

	nl = sv->name_list;
	while (nl != NULL) {
	    if (strcmp (nl->name, s_name) == 0)
		return (sv->id);
	    nl = nl->next;
	}
	sv = sv->next;
/*	sv = (Svconn_table_t *)CSS_get_indexed_entry (sv, 1); */
    }
    return (CSS_S_NAME_NOT_FOUND);
}

/******************************************************************

    Description: Opens the connection to the server "sv".

    Inputs:	sv - the server connection struct.

    Return:	0 on success or a negative CSS error number.

******************************************************************/

static int Open_conn_to_server (Svconn_table_t *sv) {
    struct sockaddr_in rsadd;	/* remote server address */

    if (sv->state == CONNECTED)
	return (0);

    memset ((char *) &rsadd, 0, sizeof (rsadd));
    rsadd.sin_family = AF_INET;
    rsadd.sin_addr.s_addr = sv->ipaddr;
    rsadd.sin_port = htons ((unsigned short)sv->port); 

    if (sv->state == DISCONNECTED) {

	if (sv->fd >= 0)
	    Close_connection (sv);

	/* open a socket for connecting to the remote port */
	if ((sv->fd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
	    MISC_log ("CSS: socket failed (errno %d)\n", errno);
	    return (CSS_OPEN_SOCKET_FAILED);
	}

	/* set socket properties */
	if (NET_set_TCP_NODELAY (sv->fd) < 0 || 
	    NET_set_non_block (sv->fd) < 0 ||   
	    NET_set_linger_on (sv->fd) < 0) {
	    Close_connection (sv);
	    return (CSS_SET_SOCK_PROP_FAILED);
	}
    }

    while (connect (sv->fd, 
		    (struct sockaddr *)&rsadd, sizeof (rsadd)) < 0) {
	if (errno == EISCONN)
	    break;
	if (errno == EINTR)
	    continue;
	if (errno == EINPROGRESS || errno == EALREADY) {
	    sv->state = CONN_PENDING;
	    return (0);
	}

	if (errno == ECONNREFUSED) { /* server not running or not accepting */
	    close (sv->fd);
	    sv->fd = -1;
	    sv->state = DISCONNECTED;
	    sv->status = ST_SERVER_DOWN;
	    sv->refuse_time = (time (NULL) % 1024) + 1;
	    return (0);
	}

	MISC_log ("CSS: connect failed (errno %d)\n", errno);
	Close_connection (sv);
	return (CSS_CONNECT_FAILED);
    }
    sv->state = CONNECTED;
    sv->status = ST_NORMAL;
    sv->poll_flag |= POLL_IN_FLAGS;

    return (0);
}

/******************************************************************

    Performs routine processing on the client side, connection "ent".

******************************************************************/

void CSS_process_client_side (Generic_entry_t *ent) {
    Svconn_table_t *sv;
    int ret;

    sv = (Svconn_table_t *)ent;
    if (sv->status == ST_LOST_CONN)
	return;
    if (sv->state != CONNECTED)		/* continue connecting */
	Open_conn_to_server (sv);
    if (sv->state != CONNECTED)
	return;

    if (sv->ready_flag & POLL_IN_RFLAGS) {
	ret = CSS_input_data (sv->fd, &(sv->resp));
	if (ret < 0)
	    Close_connection (sv);
	if (ret == NET_DISCONNECTED)
	    sv->status = ST_LOST_CONN;
    }
    if (sv->ready_flag & POLLOUT) {
	ret = CSS_output_msg ((Generic_entry_t *)sv, NULL, 0, 
					0, &(sv->omsg_list));
	if (ret < 0)
	    Close_connection (sv);
	if (ret == NET_DISCONNECTED)
	    sv->status = ST_LOST_CONN;
    }
}

/******************************************************************

    Description: This is the client side function for getting a 
		service from a server. This is the blocking version.
		It sends a request to the server and waits until a 
		response is received. The caller is responsible
		for freeing the "resp" buffer.

    Inputs:	s_name - service name;
		req - request message;
		req_len - length of the request message;
		wait_seconds - max waiting time in seconfs.

    Output:	resp - the response msg on success.

    Return:	The response message length on success or a 
		negative CSS error number.

******************************************************************/

int CSS_get_service (char *s_name, char *req, int req_len, 
				int wait_seconds, char **resp) {
    Svconn_table_t *sv;
    time_t st_time;
    int ret, cid, req_seq;

    if (resp == NULL || s_name == NULL || req == NULL)
	return (CSS_BAD_ARGUMENT);
    *resp = NULL;
    cid = CSS_get_cid (s_name);
    if (cid < 0)
	return (cid);
    sv = (Svconn_table_t *)CSS_get_entry_by_id (Svs, cid);

    req_seq = CSS_send_request (cid, req, req_len);
    if (req_seq < 0)
	return (req_seq);

    st_time = MISC_systime (NULL);
    while (1) {
	char *r;
	unsigned int req_n;

	if (sv->status == ST_SERVER_DOWN) {
	    Close_connection (sv);
	    return (CSS_SERVER_DOWN);
	}
	if (CSS_wait (1000, NULL) == cid) {
	    ret = CSS_get_response (cid, &r, &req_n);
	    if (ret < 0)
		return (ret);
	    if (ret > 0) {
		if (req_n != (unsigned int)req_seq) {
		    Close_connection (sv);
		    return (CSS_MSG_OUT_SEQ);
		}
		*resp = r;
		return (ret);
	    }
	}
	if (MISC_systime (NULL) >= st_time + wait_seconds) {
	    Close_connection (sv);
	    return (CSS_TIMED_OUT);
	}
    }
}

/******************************************************************

    Description: Sends a request message to the server. See 
		CSS manpage for details.

    Inputs:	cid - the connetion index.
		req - the request message.
		req_len - length of the request message.

    Return:	the sequence number of the new request on success 
		or a negative CSS error number.

******************************************************************/

int CSS_send_request (int cid, char *req, int req_len) {
    Svconn_table_t *sv;
    int ret;
    unsigned int new_seq;

    sv = (Svconn_table_t *)CSS_get_entry_by_id (Svs, cid);
    if (sv == NULL)
	return (CSS_BAD_ARGUMENT);
    sv->status = ST_NORMAL;
    if (sv->state != CONNECTED) {
	if ((ret = Open_conn_to_server (sv)) < 0)
	    return (ret);
    }

    if (sv->req_seq == 0) {	/* send sign-on message */
	char buf[128];
	sprintf (buf, "%d %d %s", sv->user_id, sv->conn_n, sv->password);
	ret = CSS_output_msg ((Generic_entry_t *)sv, buf, strlen (buf) + 1, 
					    0, &(sv->omsg_list));
	if (ret < 0) {
	    Close_connection (sv);
	    MISC_log ("CSS: sending login message failed (ret %d)\n", ret);
	    return (CSS_WRITE_FAILED);
	}
    }

    new_seq = sv->req_seq + 1;
    if (new_seq == 0)
	new_seq++;
    ret = CSS_output_msg ((Generic_entry_t *)sv, req, req_len, 
					new_seq, &(sv->omsg_list));
    if (ret < 0) {
	Close_connection (sv);
	if (ret == NET_DISCONNECTED)
	    return (CSS_LOST_CONN);
	else
	    return (ret);
    }
    sv->req_seq = new_seq;
    sv->n_omsgs = ret;

    return (new_seq);
}

/******************************************************************

    Description: Reads a response message from the server. See 
		CSS manpage for details.

    Inputs:	cid - the connetion index.

    Output:	resp - the response message.
		req_seq - the request seq number of the msg.

    Return:	Response message length on success, 0 if data not 
		ready or a negative CSS error number.

******************************************************************/

int CSS_get_response (int cid, char **resp, unsigned int *req_seq) {
    Svconn_table_t *sv;
    int ret;

    *resp = NULL;
    sv = (Svconn_table_t *)CSS_get_entry_by_id (Svs, cid);
    if (sv == NULL)
	return (CSS_BAD_ARGUMENT);
    if (sv->status == ST_LOST_CONN) {
	sv->status = ST_NORMAL;
	return (CSS_LOST_CONN);
    }
    if (sv->status == ST_SERVER_DOWN)
	return (CSS_SERVER_DOWN);
    if (sv->state != CONNECTED)
	return (0);

    ret = CSS_input_data (sv->fd, &(sv->resp));
    if (ret > 0) {
	Css_msg_header_t *hd;
	hd = &(sv->resp.hd);
	*req_seq = hd->seq;
	*resp = sv->resp.buf;
	sv->resp.n_bytes = 0;
	sv->resp.buf = NULL;		/* let the caller to free */
	return (sv->resp.msg_size);
    }
    if (ret == 0)
	return (0);
    if (ret < 0)
	Close_connection (sv);
    if (ret == NET_DISCONNECTED)
	return (CSS_LOST_CONN);
    else
	return (ret);
}

/******************************************************************

    Description: Sets a user ID for a connection.

    Inputs:	cid - the connetion index.
		user_id - the user ID for the conneciton.
		password - password for the user.

    Return:	0 on success or a negative CSS error number.

******************************************************************/

int CSS_set_user_id (int cid, int user_id, char *password) {
    Svconn_table_t *sv;

    sv = (Svconn_table_t *)CSS_get_entry_by_id (Svs, cid);
    if (sv == NULL)
	return (CSS_BAD_ARGUMENT);
    sv->user_id = user_id;
    if (password != NULL && password[0] != '\0') {
	if (sv->password[0] != '\0')
	    free (sv->password);
	sv->password = (char *)malloc (strlen (password) + 1);
	if (sv->password == NULL) {
	    sv->password = "";
	    return (CSS_MALLOC_FAILED);
	}
	strcpy (sv->password, password);
    }
    return (0);
}

/******************************************************************

    Closes connection of ID "cid" on client side.

    Return:	0 on success or, otherwise a CSS error code.

******************************************************************/

int CSS_client_side_close (int cid) {
    Svconn_table_t *sv;

    sv = (Svconn_table_t *)CSS_get_entry_by_id (Svs, cid);
    if (sv == NULL)
	return (CSS_BAD_ARGUMENT);
    Close_connection (sv);
    return (0);
}

/******************************************************************

    Removes a service name "s_name" from the server connection "sv".
    "sv" is removed from the table if all service names are removed.

    Return:	0 on success or a CSS error code otherwise.

******************************************************************/

static int Remove_service_name (char *s_name, Svconn_table_t *sv) {
    Name_list_t *name, *prev;
    int found = 0;

    name = sv->name_list;
    prev = NULL;
    while (name != NULL) {
	if (strcmp (name->name, s_name) == 0) {
	    if (prev == NULL)
		sv->name_list = name->next;
	    else
		prev->next = name->next;
	    free (name);
	    found = 1;
	}
	prev = name;
	name = name->next;
    }
    if (!found)
	return (CSS_BAD_ARGUMENT);
    if (sv->name_list != NULL)
	return (0);		/* done */

    Close_connection (sv);		/* all names are gone - remove sv */
    if (sv->password[0] != '\0')
	free (sv->password);
    if (sv->resp.buf != NULL)
	free (sv->resp.buf);
    Svs = (Svconn_table_t *)CSS_remove_entry (Svs, sv);
    N_svs--;

    return (0);
}

/******************************************************************

    Clean up resources for connection "sv".

    Return:	0 on success or a CSS error code otherwise.

******************************************************************/

static int Close_connection (Svconn_table_t *sv) {
    Message_list_t *list;

    if (sv->fd >= 0) {
	close (sv->fd);
	sv->fd = -1;
    }
    list = sv->omsg_list;
    while (list != NULL) {
	Message_list_t *next;
	next = list->next;
	free (list);
	list = next;
    }
    sv->omsg_list = NULL;
    sv->n_omsgs = 0;
    sv->resp.n_bytes = 0;
    sv->resp.msg_size = -1;

    sv->req_seq = 0;
    sv->poll_flag = sv->ready_flag = 0;
    sv->state = DISCONNECTED;
    sv->refuse_time = 0;
    return (0);
} 





