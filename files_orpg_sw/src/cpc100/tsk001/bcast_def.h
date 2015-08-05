/******************************************************************

    Description: This is the shared header file for the BCAST tools.

******************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2000/12/19 15:37:42 $
 * $Id: bcast_def.h,v 1.20 2000/12/19 15:37:42 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */


#ifndef BCAST_DEF_H
#define BCAST_DEF_H

#define VERSION		"V 1.0"	/* The current version number */


#define NAME_SIZE	128	/* max size of name string */

#define REQ_WINDOW_CONST .3	/* constant for deducing control parameters
				   (Refer to bcast.doc for further info) */
#define REQ_TRIG_CONST	.65	/* constant for deducing control parameters
				   (Refer to bcast.doc for further info) */

#define CONTROL_MSG	0	/* code for control massage */
#define TERMINATE_HOST	1	/* code for service termination control */

#define FIRST_MSG_SIZE	4	/* size of the first TCP msg (in # of int) 
				   from bcast */
#define CONTROL_MSG_SIZE 4	/* size of the control msg (in # of int) */
#define BCAST_ID	578392748
				/* a magic number for identifying bcast's 
				   initial message */
#define TRAILER_SIZE	8	/* size of UDP packet trailer */
#define MAX_PB_SIZE	512	/* max packet buffer size (upper limit of 
				   Pb_size) */
#define MAX_NREQUEST	64	/* max number of missing packet sequence
				   numbers sent in one retrans. request */
#define ACK_REQ_HD	2	/* number of ints before the list of seq
				   numbers in an ack/request message */

#define CONTROL_ACK	"Request accepted"
				/* message responding to a control request */

enum {LESS_THAN, GREATER_THAN, GREATER_EQUAL, LESS_EQUAL};
				/* for arg "op" of SOCK_seq_cmp () */

/* global functions */
int SOCK_set_TCP_properties (int fd);
int SOCK_read_tcp (int fd, int n_bytes, char *msg_buf);
int SOCK_write_tcp (int fd, int msg_size, char *msg);
void SOCK_sigpipe_int ();
void MAIN_exit ();
int SOCK_seq_cmp (int op, unsigned int arg1, unsigned int arg2);
unsigned int SOCK_seq_add (unsigned int seq, int inc);
void SOCK_set_max_seq (int pb_size);



#endif 		/* #ifndef BCAST_DEF_H */

