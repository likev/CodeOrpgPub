
/***********************************************************************

    Description: cm_ping public header file.

***********************************************************************/

/* 
 * RCS info
 * $Author Jing $
 * $Locker:  $
 * $Date: 2000/02/22 15:04:02 $
 * $Id: cm_ping.h,v 1.10 2000/02/22 15:04:02 jing Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

#ifndef CM_PING_H

#define CM_PING_H

/* cmp input message types */
#define CMP_IN_TCP_MON   0x01010101	/* request for TCP monitoring */
#define CMP_IN_PROC_MON  0x02020202	/* request for process monitoring */

#define CMP_MAX_CMD_LEN	 256		/* max command line length in request 
					   for process monitoring */

/* cm_ping input message formats */
typedef struct {	/* TCP monitor request */
    int type;		/* CMP_IN_TCP_MON */
    int addr;		/* remote host address in local endian format */
} Cmp_tcp_mon_req_t;

typedef struct {	/* process monitor request */
    int type;		/* CMP_IN_PROC_MON */
    int pid;		/* process pid */
    int wp;		/* waiting period in seconds. -1 cancels the previous
			   monitor/restart request */
    char cmd[CMP_MAX_CMD_LEN];	/* process command line */
} Cmp_proc_mon_req_t;

#define CMP_MAX_INP_MSG_SIZE	(sizeof(Cmp_proc_mon_req_t))
			/* This must be the largest input msg type */

typedef struct {	/* a cm_ping output record */
    int addr;		/* remote host address in local endian format */
    int q_time;		/* remote host quiet time in seconds */
} Cmp_record_t;

/* cm_ping output message format */
typedef struct {
    int n_records;	/* number of records in this message */
    time_t tm;		/* publish time */
    Cmp_record_t rec[1];	/* list of records */
} Cmp_out_msg_t;

#endif		/* #ifndef CM_PING_H */

