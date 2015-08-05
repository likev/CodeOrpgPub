
/******************************************************************

	This is the CSS module containing routines common to the 
	client and server.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/10/08 19:23:54 $
 * $Id: css_common.c,v 1.9 2011/10/08 19:23:54 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#if !defined(LINUX) && !defined(__WIN32__)
#include <inttypes.h>
#endif

#include <misc.h> 
#include <net.h> 
#include <css_def.h> 

struct Fd_table {		/* external fd to be polled struct */
    struct Fd_table *next;	/* the next item in the linked list */
    int fd;			/* fd */
    int poll_flag;		/* poll flag */
    void (*cb_func)(int, int);	/* fd ready callback function */
};
typedef struct Fd_table Fd_table_t;

static Fd_table_t *Efds = NULL;	/* external fds to be polled */
static int N_efds = 0;		/* number of external fds to be polled */


static int Is_input_ready (Generic_entry_t *ent, int type, int *type_pt);


/****************************************************************************

    Parses the "sv_addr" string to get the IP address, returned in "ipaddr", 
    and the port number.

    Returns the port number on success or a negative error number.

****************************************************************************/

int CSS_get_ipaddr_port (char *sv_addr, unsigned int *ipaddr) {			    char *cpt, hname[128];
    int port;

    cpt = sv_addr;
    while (*cpt != '\0' && *cpt != ':')
	cpt++;
    if (cpt - sv_addr > 128)
	return (CSS_SV_ADDR_ERROR);
    if (*cpt == '\0')
	cpt = sv_addr;
    memcpy (hname, sv_addr, cpt - sv_addr);
    hname[cpt - sv_addr] = '\0';
    if (strcmp (hname, "INADDR_ANY") == 0)
	*ipaddr = INADDR_ANY;
    else if ((*ipaddr = NET_get_ip_by_name (hname)) == INADDR_NONE)
	return (CSS_HOST_NAME_ERROR);
    if (cpt == sv_addr)
	cpt--;
    if (sscanf (cpt + 1, "%d", &port) != 1)
	return (CSS_HOST_NAME_ERROR);
    return (port);
}

/****************************************************************************

    Sends message "msg" of size "msg_len" with sequence number of "seq" on
    connect entry "ent". If the socket writing can not be completed or the
    socket is not ready for write, the message is appended to the list of
    "list_pt". If the list is not empty, the data in the list is sent first.

    Returns the number of pending messages in "list_pt" on success or a 
    negative error number.

****************************************************************************/

int CSS_output_msg (Generic_entry_t *ent, char *msg, int msg_len, int seq, 
						Message_list_t **list_pt) {
    Message_list_t *list;
    Css_msg_header_t hd;
    int fd, cnt, n_saved, ret;

    fd = ent->fd;
    if (ent->state != CONNECTED)	/* not ready for output */
	fd = -1;
    else 
	ent->poll_flag &= ~POLLOUT;

    list = *list_pt;
    while (fd >= 0 && list != NULL) {	/* write saved messages */
	Message_list_t *next;
	cnt = list->size - list->bytes_sent;
	ret = NET_write_socket (fd, list->msg + list->bytes_sent, cnt);
	if (ret < 0)
	    return (ret);
	list->bytes_sent += ret;
	if (list->bytes_sent < list->size) {	/* socket buffer full */
	    ent->poll_flag |= POLLOUT;
	    break;
	}
	next = list->next;
	free (list);
	list = next;
    }

    *list_pt = list;			/* *list_pt - the first */
    if (list == NULL)
	n_saved = 0;
    else
	n_saved = 1;
    while (list != NULL && list->next != NULL) {
	list = list->next;		/* list - the last */
	n_saved++;
    }
    if (msg_len <= 0)
	return (n_saved);

    /* header for the new messages */
    hd.css_msg_id = htonl (CSS_MSG_ID);
    hd.size = htonl (msg_len);
    hd.seq = htonl (seq);

    cnt = 0;
    if (fd >= 0 && *list_pt == NULL) {	/* try to write the new msg */
	ret = NET_write_socket (fd, (char *)&hd, sizeof (Css_msg_header_t));
	if (ret < 0)
	    return (ret);
	cnt += ret;
	if (ret >= (int)sizeof (Css_msg_header_t)) {
	    ret = NET_write_socket (fd, msg, msg_len);
	    if (ret < 0)
		return (ret);
	    cnt += ret;
	}
    }

    if (cnt < msg_len + (int)sizeof (Css_msg_header_t)) {	/* save */
	Message_list_t *new_ent;

	ent->poll_flag |= POLLOUT;	/* write blocked */

	new_ent = (Message_list_t *)malloc (sizeof (Message_list_t) + 
				msg_len + sizeof (Css_msg_header_t));
	if (new_ent == NULL)
	    return (CSS_MALLOC_FAILED);
	new_ent->msg = (char *)new_ent + sizeof (Message_list_t);
	new_ent->size = msg_len + sizeof (Css_msg_header_t);
	new_ent->bytes_sent = cnt;
	new_ent->seq = seq;
	new_ent->next = NULL;
	memcpy (new_ent->msg, &hd, sizeof (Css_msg_header_t));
	memcpy (new_ent->msg + sizeof (Css_msg_header_t), msg, msg_len);
	if (*list_pt == NULL)
	    *list_pt = new_ent;
	else
	    list->next = new_ent;
	n_saved++;
    }

    return (n_saved);
}

/****************************************************************************

    Reads data from socket "fd" and puts in buffer "buffer". To discard the
    data, one sets buffer->n_bytes = 0. If the data is not discarded, this
    function will not read the next message.

    Returns 1 if the message reading is completed, 0 if not completed, or
    a negative error number.

****************************************************************************/

#define EXPECTED_MSG_SIZE 2048

int CSS_input_data (int fd, input_buffer_t *buffer) {
    int total, ret;

    if (buffer->msg_size >= 0 && 
	buffer->n_bytes >= buffer->msg_size + (int)sizeof (Css_msg_header_t))
	return (1);

    if (buffer->n_bytes < (int)sizeof (Css_msg_header_t)) {
	Css_msg_header_t *hd;
	buffer->msg_size = -1;
	ret = NET_read_socket (fd, (char *)(&(buffer->hd)) + buffer->n_bytes, 
				sizeof (Css_msg_header_t) - buffer->n_bytes);
	if (ret < 0)
	    return (ret);
	buffer->n_bytes += ret;
	if (buffer->n_bytes < (int)sizeof (Css_msg_header_t))
	    return (0);
	hd = &(buffer->hd);
	hd->css_msg_id = ntohl (hd->css_msg_id);
	hd->size = ntohl (hd->size);
	hd->seq = ntohl (hd->seq);
	if (hd->css_msg_id != CSS_MSG_ID ||
	    hd->size < 0) {
	    buffer->n_bytes = 0;
	    MISC_log ("CSS: Bad message received\n");
	    return (CSS_BAD_MSG_HD);
	}
	buffer->msg_size = hd->size;
	if (buffer->buf != NULL)
	    free (buffer->buf);
	buffer->buf = NULL;
    }

    if (buffer->buf == NULL) {
	buffer->buf = (char *)malloc (buffer->msg_size);
	if (buffer->buf == NULL)
	    return (CSS_MALLOC_FAILED);
    }

    total = sizeof (Css_msg_header_t) + buffer->msg_size;
    if (buffer->n_bytes < total) {
	ret = NET_read_socket (fd, 
	    buffer->buf + (buffer->n_bytes - sizeof (Css_msg_header_t)), 
	    total - buffer->n_bytes);
	if (ret < 0)
	    return (ret);
	buffer->n_bytes += ret;
	if (buffer->n_bytes < total)
	    return (0);
	return (1);
    }
    return (0);
}

/**************************************************************************

    Returns the pointer to the "ind"-th entry starting from "st_ent". NULL
    is returned if there is no such entry.

**************************************************************************/

void *CSS_get_indexed_entry (void *st_ent, int ind) {
    int cnt;
    Generic_entry_t *ent;

    cnt = 0;
    ent = (Generic_entry_t *)st_ent;
    while (ent != NULL && cnt < ind) {
	ent = ent->next;
	cnt++;
    }
    if (cnt != ind)
	return (NULL);
    return ((void *)ent);
}

/**************************************************************************

    Returns the pointer to the entry of "id" starting from "st_ent". NULL
    is returned if there is no such entry.

**************************************************************************/

void *CSS_get_entry_by_id (void *st_ent, int id) {
    Generic_entry_t *ent;

    ent = (Generic_entry_t *)st_ent;
    while (ent != NULL && ent->id != id)
	ent = ent->next;
    if (ent == NULL)
	return (NULL);
    return ((void *)ent);
}

/**************************************************************************

    Removes entry "rm_ent" in the linked list table started with "st_ent".
    It returns the start entry which may be different from "st_ent".

**************************************************************************/

void *CSS_remove_entry (void *st_ent, void *rm_ent) {
    Generic_entry_t *se, *re, *t, *prv;

    se = (Generic_entry_t *)st_ent;
    re = (Generic_entry_t *)rm_ent;
    if (se == NULL)
	return (NULL);
    if (se == re) {
	t = se->next;
	free (se);
	return ((void *)t);
    }
    prv = NULL;			/* not necessary - turn off gcc warning */
    t = se;
    while (t != NULL && t != re) {
	prv = t;
	t = t->next;
    }
    if (t != NULL) {		/* found */
	prv->next = t->next;	/* prv is always asigned */
	free (t);
    }
    return (st_ent);
}

/**************************************************************************

    Returns the next unique ID.

**************************************************************************/

int CSS_next_id () {
    static int id = 0;
    id++;
    if (id < 0)
	id = 1;
    return (id);
}

/**************************************************************************

    This function returns immediately if a response or request is ready on any
    of the connections. Otherwise, it will poll all available connections for
    at most "wait_ms" milli-seconds. If action (read/write) is ready on any of
    the connections, it calls either the client or the server side processing
    function to perform the action. After all active connections are processed,
    the function checks again if any response of request is ready. If argument
    "type" is not NULL, it returns REQUEST_READY or RESPONSE_READY to indicate
    the type of the response or request ready connection.

    Checking message ready before polling is necessary because in a multiple
    connection environment, input may be ready on several connections while
    CSS_wait only returns one connection.

    Returns the connection ID on which either response (client side) or 
    request (server side) is ready. If nothing is ready, it returns 0. It
    returns a negative error number on failure.

**************************************************************************/

#define MAX_N_SOCKETS 	64

int CSS_wait (int wait_ms, int *type) {
    struct pollfd pfds[MAX_N_SOCKETS];
    Generic_entry_t *ents[MAX_N_SOCKETS];
    Generic_entry_t *cls, *svs, *scs;
    int n_cls, n_svs, n_scs, cnt, ready_cnt, i;
    Fd_table_t *efd;

    n_cls = CSS_get_client_entries (&cls);
    n_svs = CSS_get_server_entries (&svs);
    n_scs = CSS_get_sv_client_entries (&scs);
    if (n_cls + n_svs + n_scs + N_efds > MAX_N_SOCKETS)
	return (CSS_BUFFER_ERROR);
    cnt = 0;
    for (i = 0; i < n_cls; i++) {	/* client side connections */
	if (Is_input_ready (cls, RESPONSE_READY, type))
	    return (cls->id);
	if (cls->status == ST_SERVER_DOWN &&
	    (cls->refuse_time == 0 ||
		(cls->refuse_time > 0 && 
			((time (NULL) % 1024) + 1) != cls->refuse_time)))
	    CSS_process_client_side (cls);
	if (cls->fd >= 0 && cls->poll_flag != 0) {
	    ents[cnt] = cls;
	    cnt++;
	}
	cls = cls->next;
    }
    for (i = 0; i < n_scs; i++) {	/* server side clients connections */
	if (Is_input_ready (scs, REQUEST_READY, type))
	    return (scs->id);
	if (scs->fd >= 0 && scs->poll_flag != 0) {
	    ents[cnt] = scs;
	    cnt++;
	}
	scs = scs->next;
    }
    for (i = 0; i < n_svs; i++) {	/* server sockets */
	if (svs->fd >= 0 && svs->poll_flag != 0) {
	    ents[cnt] = svs;
	    cnt++;
	}
	svs = svs->next;
    }

    for (i = 0; i < cnt; i++) {
	pfds[i].fd = ents[i]->fd; 
	pfds[i].events = ents[i]->poll_flag;
	pfds[i].revents = 0;
	ents[i]->ready_flag = 0;
    }

    efd = Efds;
    while (efd != NULL) {
	if (efd->fd >= 0) {
	    pfds[i].fd = efd->fd; 
	    pfds[i].events = efd->poll_flag;
	    pfds[i].revents = 0;
	    i++;
	}
	efd = efd->next;
    }

    while ((ready_cnt = poll (pfds, i, wait_ms)) < 0 && errno == EINTR)
	msleep (10);
    if (ready_cnt < 0)
	return (ready_cnt);
    if (ready_cnt == 0)
	return (0);

    for (i = 0; i < cnt; i++) {
	ents[i]->ready_flag = pfds[i].revents;
	if (ents[i]->ready_flag) {
	    if (ents[i]->type == CLIENT_T)
		CSS_process_client_side (ents[i]);
	    else
		CSS_process_server_side (ents[i]);
	}
    }

    efd = Efds;
    while (efd != NULL) {
	if (efd->fd >= 0) {
	    if (pfds[i].revents & (POLLNVAL | POLLERR)) {
		MISC_log ("CSS: User fd (%d) for polling discarded\n", efd->fd);
		efd->fd = -1;
		return (-179);
	    }
	    else if (pfds[i].revents)
		efd->cb_func (pfds[i].fd, pfds[i].revents);
	    i++;
	}
	efd = efd->next;
    }

    for (i = 0; i < cnt; i++) {
	if (ents[i]->type == CLIENT_T) {
	    if (Is_input_ready (ents[i], RESPONSE_READY, type))
		return (ents[i]->id);
	}
	else if (ents[i]->type == SV_CL_T) {
	    if (Is_input_ready (ents[i], REQUEST_READY, type))
		return (ents[i]->id);
	}
    }
    return (0);
}

/**************************************************************************

    Checks if an input message is ready or the connection is lost on 
    connection "ent". It sets value at "type_pt", if not NULL, to "value" 
    if ready.

    Returns 1 if ready 0 otherwise.

**************************************************************************/

static int Is_input_ready (Generic_entry_t *ent, int type, int *type_pt) {

    if (ent->status == ST_LOST_CONN ||
	(ent->input.msg_size > 0 && ent->input.n_bytes >= 
		ent->input.msg_size + (int)sizeof (Css_msg_header_t))) {
	if (type_pt != NULL)
	    *type_pt = type;
	return (1);
    }
    return (0);
}

/******************************************************************

    Closes a server connection on clent side, a client connection on
    server side or a server.

    Input/Output: See "man css".

    Return:	0 on success or a negative CSS error number.

******************************************************************/

int CSS_close (int id) {
    int ret;

    ret = CSS_client_side_close (id);
    if (ret != CSS_BAD_ARGUMENT)
	return (ret);
    return (CSS_server_side_close (id));
}

/******************************************************************

    Add (or remove if cb_func == NULL) an fd to be polled.

    Input/Output: See "man css".

    Return:	0 on success or a negative CSS error number.

******************************************************************/

int CSS_add_poll_fd (int fd, int poll_flag, 
			void (*cb_func)(int fd, int ready_flag)) {
    Fd_table_t *efd, *prev;

    prev = NULL;		/* not necessary - turn off gcc warning */
    efd = Efds;
    while (efd != NULL) {
	if (efd->fd == fd)
	    break;
	prev = efd;
	efd = efd->next;
    }

    if (efd != NULL) {		/* found */
	if (cb_func == NULL) {	/* remove this */
	    if (efd == Efds)
		Efds = efd->next;
	    else
		prev->next = efd->next;
	    free (efd);
	    N_efds--;
	}
	else {			/* replace */
	    efd->poll_flag = poll_flag;
	    efd->cb_func = cb_func;
	}
    }
    else if (cb_func != NULL) {		/* add a new entry */
	efd = (Fd_table_t *)malloc (sizeof (Fd_table_t));
	if (efd == NULL)
	    return (CSS_MALLOC_FAILED);
	efd->fd = fd;		/* add a new entry */
	efd->poll_flag = poll_flag;
	efd->cb_func = cb_func;
	efd->next = Efds;
	Efds = efd;
	N_efds++;
    }
    return (0);
}




