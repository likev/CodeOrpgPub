
/****************************************************************
		
    This module contains the internal shared definitions
	for the EN (Event Notification) library module.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/01/16 21:59:56 $
 * $Id: en_def.h,v 1.3 2007/01/16 21:59:56 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 * $Log: en_def.h,v $
 * Revision 1.3  2007/01/16 21:59:56  jing
 * Update
 *
 * Revision 1.1  2002/03/12 16:39:48  jing
 * Initial revision
 *
 */

#ifndef EN_DEF_H
#define EN_DEF_H

#include <en.h>

#ifdef LITTLE_ENDIAN_MACHINE
    #include <misc.h>
    #define EN_T_BSWAP(a) INT_BSWAP(a)
    #define EN_SHORT_BSWAP(a) SHORT_BSWAP(a)
#else
    #define EN_T_BSWAP(a) (a)
    #define EN_SHORT_BSWAP(a) ((a) & 0xffff)
#endif

/* message types - no byte swap needed */
#define TO_CLIENT_UN 0x02020202
#define FROM_CLIENT_REG 0x03030303
#define FROM_LB_WRITE_UN 0x04040404		/* for update notification */
#define TO_CLIENT_ACK 0x05050505
#define TO_CLIENT_AN 0x07070707
#define FROM_CLIENT_AN 0x08080808
#define FROM_SV_REG 0x09090909
#define TO_SV_ACK 0x0a0a0a0a
#define FROM_SV_AN 0x0b0b0b0b
#define FROM_SV_CONN 0x0c0c0c0c			/* connection init/resp msgs */
#define KEEP_ALIVE_TEST 0x0d0d0d0d

/* values for msg_len field of To_client_msg_t */
enum {TO_CLIENT_ACK_DONE, TO_CLIENT_ACK_FAILED};

/* values for field "fd" for FROM_SV_CONN type messages */
enum {SV_CONN_INIT, SV_CONN_RESP, SV_CONN_REGS};

typedef struct {			/* struct for various NTF messages;
					   AN msg follows this for *_AN types;
					   in big endian for FROM_SV_AN and
					   TO_SV_ACK types */
    en_t msg_type;			/* message type: TO_CLIENT_UN, 
					   TO_CLIENT_AN or 
					   FROM_CLIENT_AN or 
					   FROM_SV_AN or
					   TO_CLIENT_ACK or
					   TO_SV_ACK or
					   FROM_SV_CONN */
    en_t code;				/* EN code (LB fd or EN_AN_CODE) 
					   FROM_SV_CONN:
						SV_CONN_INIT: conn init msg; 
						SV_CONN_RESP: response msg;
						SV_CONN_REGS: AN reg info */
    en_t evtid;				/* LB message id (or AN event id) */
    en_t lbmsgid;			/* The LB message id in case evtid is
					   EN_ANY or LB_MSG_EXPIRED */
    en_t msg_len;			/* message length;
					   TO_CLIENT_ACK: 0 or EN_NTF_FAILED */
    en_t lost_cnt;			/* number of NTF lost;
					   TO_SV_ACK: echo FROM_SV_REG.a_pid */
    unsigned short sender_id;		/* the NTF sender's ID */
    short reserved;
} Ntf_msg_t;				/* This struct must be no larger than 
					   any of the following two structs */

typedef struct {			/* struct for NTF registration msgs */
    en_t msg_type;			/* message type: FROM_CLIENT_REG or
					   FROM_SV_REG */
    en_t signal;			/* the signal to use; < 0 indicates
					   no signal needed; == -1 indicates
					   deregisteration */
    en_t pid;				/* process id */
    en_t code;				/* event code */
    en_t evtid;				/* event id */
    en_t a_pid;				/* aliased pid - shared with other
					   pid-fd pair for identifying the LB;
					   FROM_SV_REG: the host index */
    en_t a_code;			/* aliased code (fd; -1 indicates no 
					   alias */
} Ntf_regist_msg_t;

typedef struct {			/* message struct from LB_write to 
					   the server; All fields must in 
					   big endian byte order */
    en_t msg_type;			/* message type: FROM_LB_WRITE_UN or
					   FROM_LB_WRITE_WUR */
    en_t pid;				/* process id to notify */
    en_t fd;				/* LB fd to notify */
    en_t msgid;				/* the new message id */
    en_t lbmsgid;			/* The LB message id in case msgid is
					   EN_ANY or LB_MSG_EXPIRED */
    en_t msg_len;			/* the new message length */
    unsigned short sender_id;		/* the NTF sender's ID */
    short reserved;
} From_lb_write_t;

/* This section is used for passing large pid, which is currently used by IRIX.
   It is not need otherwise. We may remove this in the future when IRIX support
   no longer needed. */
#define Get_big_pid(pid,lock,flag) \
    ((pid) | ((((lock) >> 1) & 0x7f) << 16) | ((((flag) >> 2) & 0x3f) << 23))
#define Get_nr_lock(lock) ((lock) & 0x1)
#define Get_nr_flag(flag) ((flag) & 0x3)
#define Pid_in_lock(pid) ((((pid) >> 16) & 0x7f) << 1)
#define Pid_in_flag(pid) ((((pid) >> 23) & 0x3f) << 2)


/* bit masks for overloading on Ntf_msg_t.msg_len of Ntf_msg_t.msg_type */
#define AN_NOT_TO_SELF 0x80000000
				/* not sending this AN to this process */
#define AN_SELF_ONLY 0x40000000
				/* sending this AN to this process only */
#define AN_MSG_LEN_MASK 0x3fffffff
				/* mask for getting the msg length */

#endif		/* #ifndef EN_DEF_H */

