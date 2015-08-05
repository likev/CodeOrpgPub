/*   @(#) ipc.h 99/12/23 Version 1.5   */

/*
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: ipc.h,v 1.1 2000/02/25 17:14:21 john Exp $
 */

/*
Modification history:
 
Chg Date       Init Description
1.  12-jun-98  mpb  Compile on Windows NT (WINNT).
2.  14-AUG-98  mpb  have to change the structure name MSG to XMSG because
                    MSG is a Windows NT system strucutre.
*/

#ifndef _ipc_h
#define _ipc_h

#include <sys/types.h>

#if !defined(WINNT) && !defined(VXWORKS)
#include <sys/param.h>
#endif /* !WINNT && !VXWORKS */

/* error values */

#define IPC_ERR_SYSTEM	0	/* system error: errno is valid */
#define IPC_ERR_MEMORY	1	/* memory allocation error */
#define IPC_ERR_MESSAGE	2	/* request/response too big */

/***************** System V Message Queue Implementation *********************/

#ifdef SYSVMSG

typedef unsigned long EVMASK;

/* forward declarations for opaque types */

#ifdef __STDC__
struct ipc_request;
#endif

typedef int QID;			/* ipc communication handle */
typedef struct ipc_request IPC;		/* opaque handle on rpc request */

#define QID_FAIL	(-1)
#define IPC_FAIL	NULL

/* scatter gather list structure */

struct ipc_list {
	void	*msg;
	int	size;
};

typedef struct ipc_list XMSG;

#define SETMSG(m, buf, len)	((m)->msg = (buf), (m)->size = (len))

#endif /* SYSVMSG */

/***************** Windows NT Queue Implementation *********************/

#ifdef WINNT

typedef unsigned long EVMASK;

/* forward declarations for opaque types */

struct ipc_request;


typedef HANDLE QID;                 /* ipc communication handle */
typedef struct ipc_request IPC;     /* opaque handle on rpc request */

#define QID_FAIL	INVALID_HANDLE_VALUE
#define IPC_FAIL	NULL

/* scatter gather list structure */

struct ipc_list {
	void	*msg;
	int	size;
};

typedef struct ipc_list XMSG;

typedef unsigned long  pid_t;
typedef int            key_t;

#define SETMSG(m, buf, len)	((m)->msg = (buf), (m)->size = (len))

#endif /* WINNT */

/******************* QNX Message Passing Implementation **********************/

#ifdef __QNX__

#ifdef __STDC__
struct ipc_request;			/* forward declaration */
#endif

typedef pid_t QID;			/* ipc communication handle */
typedef struct ipc_request IPC;		/* opaque handle on rpc request */

#define QID_FAIL	(-1)
#define IPC_FAIL	NULL

typedef struct _mxfer_entry XMSG;

#define SETMSG(m, buf, len)	_setmx((m), (buf), (len))

#endif /* __QNX__ */


/************************* pSOS Implementation *******************************/

#ifdef PSOS

typedef unsigned long QID;		/* ipc communication handle */

#define QID_FAIL	(0L)

#endif /* PSOS */

/************************ VxWorks Implementation *****************************/

#ifdef VXWORKS

typedef unsigned long QID;

typedef unsigned long EVMASK;

typedef unsigned long EVENT;

/* forward declarations for opaque types */

struct ipc_request;

typedef struct ipc_request IPC;     /* opaque handle on rpc request */

#define QID_FAIL        (-1)
#define IPC_FAIL        NULL

/* scatter gather list structure */

struct ipc_list {
	void	*msg;
	int	size;
};

typedef struct ipc_list XMSG;

typedef int            key_t;

#define SETMSG(m, buf, len)	((m)->msg = (buf), (m)->size = (len))

#endif /* VXWORKS */

/************************** Generic Interfaces *******************************/

extern char *ipc_queue_name(char *var, char *default_);

extern QID ipc_attach(char *server);
extern void ipc_detach(QID queue);
extern QID ipc_connect(char *server);
extern void ipc_terminate(QID queue);
extern void ipc_cleanup(void);

extern void ipc_set_diagnostic_handler(void (*)(int));
extern void ipc_set_exception_handler(void (*)(EVMASK));

extern int ipc_send(QID queue, void *, void *, int, int);
extern int ipc_sendm(QID queue, int, int, XMSG *, XMSG *);
extern int ipc_send_event(QID queue, EVMASK event);
extern int ipc_diagnostic(char *server, int diag);

extern IPC *ipc_receive(QID queue, void *, int);
extern IPC *ipc_receivem(QID queue, int, XMSG *);
/* pid variant is temporary for current stremul implementation */
extern IPC *ipc_receive_pid(QID queue, void *, int, pid_t *);
extern IPC *ipc_receivem_pid(QID queue, int, XMSG *, pid_t *);
extern int ipc_read(IPC *request, void *, int);
extern int ipc_readm(IPC *request, int, XMSG *);
extern int ipc_reply(IPC *request, void *, int);
extern int ipc_replym(IPC *request, int, XMSG *);

extern void ipc_set_program_name(char *progname);
extern int ipc_error(void);

#endif /* _ipc_h */
