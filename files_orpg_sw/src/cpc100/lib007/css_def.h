
/*******************************************************************

    Module: css_def_s.h

    Description: Local header file for the Client/Server Utility
		(CSS) module - socket version.

*******************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/19 19:35:07 $
 * $Id: css_def.h,v 1.5 2011/05/19 19:35:07 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 * $Log: css_def.h,v $
 * Revision 1.5  2011/05/19 19:35:07  jing
 * Update
 *
 * Revision 1.3  2002/03/12 17:08:26  jing
 * Update
 *
 *
*/

#ifndef CSS_DEF_H
#define CSS_DEF_H

#include <css.h>

#define CSS_MSG_ID 859347912

typedef struct {		/* socket message header */
    int css_msg_id;		/* unique message idenfifier for CSS */
    int size;			/* message size excluding this header */
    unsigned int seq;		/* request sequence number */
} Css_msg_header_t;

struct message_list {		/* outgoing message list */
    char *msg;			/* pointer to the message */
    int size;			/* total size of the message */
    int bytes_sent;		/* number of butes already sent */
    int seq;			/* sequence number */
    struct message_list *next;
};
typedef struct message_list Message_list_t;

typedef struct {		/* buffer struct for socket input */
    int msg_size;		/* size of the incoming msg (< 0 if unknown) */
    int n_bytes;		/* number of bytes in buf; must init to 0 */
    Css_msg_header_t hd;	/* for message header */
    char *buf;			/* buffer for input data; must init to NULL */
} input_buffer_t;

/* for field "type" in Generic_entry_t */
enum {CLIENT_T, 		/* client side table */
      SERVER_T, 		/* server table */
      SV_CL_T			/* server side client table */
};

/* value for Generic_entry_t.status */
enum {ST_NORMAL, ST_SERVER_DOWN, ST_LOST_CONN};

/* value for Generic_entry_t.state */
enum {DISCONNECTED, CONN_PENDING, CONNECTED, TO_BE_DELETED};

struct generic_entry {		/* generic part of a connection entry */
    struct generic_entry *next;	/* next entry */
    int id;			/* entry id */
    int poll_flag;		/* to poll: POLL_IN_FLAGS, POLLOUT */
    int ready_flag;		/* poll result: POLL_IN_RFLAGS ... */
    short type;			/* table type */
    short status;		/* connection status */
    short state;		/* the connection state */
    short refuse_time;		/* second the connection was refused */
    int fd;			/* socket fd */
    input_buffer_t input;	/* buffer for response */
};
typedef struct generic_entry Generic_entry_t;


int CSS_input_data (int fd, input_buffer_t *buffer);
int CSS_output_msg (Generic_entry_t *ent, char *msg, int msg_len, int seq, 
						Message_list_t **list_pt);
void *CSS_get_entry_by_id (void *st_ent, int id);
void *CSS_get_indexed_entry (void *st_ent, int ind);
int CSS_next_id ();
void *CSS_remove_entry (void *st_ent, void *rm_ent);
int CSS_get_client_entries (Generic_entry_t **cls);
int CSS_get_server_entries (Generic_entry_t **svs);
int CSS_get_sv_client_entries (Generic_entry_t **scs);
void CSS_process_client_side (Generic_entry_t *ent);
void CSS_process_server_side (Generic_entry_t *ent);
int CSS_get_ipaddr_port (char *sv_addr, unsigned int *ipaddr);		
int CSS_client_side_close (int cid);
int CSS_server_side_close (int cid);

#endif		/* #ifndef CSS_DEF_H */

