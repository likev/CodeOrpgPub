static char sccsid[] = "@(#)ipc_sysv.c	1.2	18 Aug 1998";
/*
 *  SpiderSTREAMS IPC Library - System V Message Queue Implementation
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine, loosely based on an emulation of the
 *  QNX 2 message passing interfaces by Nick Felisiak.
 *
 *  @(#)$Id: ipc_sysv.c,v 1.3 2000/07/14 15:57:52 jing Exp $
 */

/*
Modification history:

Chg Date       Init Description
1.  14-AUG-98  mpb  Have to change the structure name MSG to XMSG because
                    MSG is a Windows NT system strucutre.
*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#if 0            /* XXX when we fix the stremul namespace problem */
#include <unistd.h>
#include <string.h>
#endif
#include <errno.h>
#include <debug.h>
#include <ipc.h>
#include "ipc_error.h"

/* declarations missing from system header files */


/*
 * This buffer structure is used for all IPC.  The client uses a
 * single static instance, but only passes the part it needs to.
 * The server allocates a new buffer for the life-time of each
 * request.
 */

#define MAXMSGSZ    10240    /* XXX smaller? */

struct ipc_buffer {
    long    mtype;        /* System V IPC message type */
    pid_t    pid;        /* sender's process id (unauthenticated) */
    key_t    key;        /* sender's reply queue */
    int    len;        /* size of buffer (for reply) */
    char    data[MAXMSGSZ];    /* data buffer */
};

/*
 * Values for message type (must be non-zero).
 *
 * We use flag bits to distinguish between IPC messages types; the
 * lower bits are user-defined (always zero for ordinary messages).
 */

#define IPC_MESSAGE        0x00010000    /* user message */
#define IPC_EXCEPTION        0x00020000    /* exception event */
#define IPC_DIAGNOSTIC        0x00040000    /* short datagram */
#define IPC_EXCEPTION_MASK    0x000000ff    /* user event mask */
#define IPC_DIAGNOSTIC_MASK    0x0000ffff    /* diagnostic word */

/* size of the IPC header from the SYSVMSG perspective - excludes 'mtype' */

#define IPCHDR        (sizeof(struct ipc_buffer) - sizeof(long) - MAXMSGSZ)

/*
 * The server uses an instance of this structure for each outstanding IPC
 * request.  It caches client information and keeps track of how much of
 * the message has been ``read''.
 */

struct ipc_request {
    int        queue;    /* handle on client's queue */
    struct ipc_buffer *ipcb; /* pointer to IPC request buffer */
    int        len;    /* total length of request */
    int        off;    /* IPC read offset */
};

/*
 * Each client needs a reply queue, which is shared among all of its
 * connections.  This is created when making the first connection.
 */

static pid_t ipc_pid;        /* client's process id (temporary) */
static key_t ipc_key;        /* client's (single) reply queue */
static int ipc_queue;        /* client's reply queue */
static int ipc_count = 0;    /* count of client connections */

static void (*ipc_diagnostic_handler)(int) = NULL;
static void (*ipc_exception_handler)(unsigned long) = NULL;

void
ipc_set_diagnostic_handler(void (*handler)(int))
{
    ipc_diagnostic_handler = handler;
}

void
ipc_set_exception_handler(void (*handler)(unsigned long))
{
    ipc_exception_handler = handler;
}

#if 0
#ifdef DEBUG
int ipc_debug = 1;
#endif
#endif

/*
 * trunc() -- truncate string to specified length
 *
 * Returned string is static, and must be copied before next invocation,
 * if required.  It will never be longer than MAXLEN, even if that means
 * truncating shorter than the specified length.
 */

#define MAXLEN  255

static char *
trunc(char *name, int len)
{
    static char s[MAXLEN+1];
    char *p = s;
    char *q = name;
    int n;

    if (len > MAXLEN)
        len = MAXLEN;

    n = len;

    while (n-- > 0 && *q)
        *p++ = *q++;

    if (*q)
    {
        *p = '\0';

        ipc_warning("truncating name \"%s\" to %d chars", name, len);
    }

    return s;
}

/*
 * makekey() -- construct a key value from (the initial part of) name
 *
 * This scheme is intended to avoid the need for a QNX-style name
 * locator (even if it would simply be implemented as a random access
 * file).
 */

static key_t
makekey(char *name)
{
    int len, n;
    key_t key = 0;

    /*
     * Use as much of the string as will fit directly in the key,
     * padding with spaces.
     */

    if ((len = strlen(name)) > sizeof(key_t))
    {
        ipc_warning("truncating name \"%s\" to %d chars",
            name, (unsigned) sizeof(key_t));
    }

    for (n = 0; n < sizeof(key_t); n++)
    {
        char c = (n < len? name[n] : ' ');

        key |= (c << ((sizeof(key_t) - 1 - n) * sizeof(char)));
    }
    return key;
}

/*
 * total_size() -- tally the length of a message held in a vector list
 */

static int
total_size(int count, struct ipc_list *msg)
{
    int size = 0;

    while (count--)
        size += (msg++)->size;

    return size;
}

/*
 * copy_in_msg() -- copy msg into single buffer from scatter gather list
 *
 * Returns amount of data copied on success, -1 if the buffer is too small.
 */

static int
copy_in_msg(int count, struct ipc_list *list, char *buf, int len)
{
    int i, off = 0;

    for (i = 0; i < count; i++)
    {
        int sz;

        XTRACE(("[IPC] copy_in_msg: #%d (%d bytes) 0x%x => 0x%x\n",
            i, list[i].size, list[i].msg, buf));

        if ((sz = list[i].size) == 0)
            continue;

        if ((off += sz) > len)
            return -1;

        memcpy(buf, list[i].msg, sz);

        buf += sz;
    }

    return off;
}

/*
 * copy_out_msg() -- copy msg from single buffer into scatter gather list
 */

int
copy_out_msg(int count, struct ipc_list *list, char *buf, int len)
{
    int size = 0;
    int i;

    for (i = 0; i < count && len > 0; i++)
    {
        int sz;

        XTRACE(("[IPC] copy_out_msg: #%d (%d bytes) 0x%x => 0x%x\n",
            i, list[i].size, buf, list[i].msg));

        if ((sz = list[i].size) == 0)
            continue;

        if (sz > len)
            sz = len;

        memcpy(list[i].msg, buf, sz);

        len -= sz;
        buf += sz;
        size += sz;
    }

    /* len may still be > 0 */

    return size;
}

/*
 * ipc_queue_name() -- return an appropriately constructed server queue name
 */

char *
ipc_queue_name(char *var, char *default_)
{
    char *name;

    if (var != NULL && (name = getenv(var)) != NULL)
        return trunc(name, sizeof(key_t));

#ifdef S_BASE_COMPAT
    /*
     * Compatibility with STREMUL.  This should go away.
     */
    if ((name = getenv("S_BASE")) != NULL) {
        char *s = trunc(name, sizeof(key_t));
        /* HACK: write to trunc's static data, differentiate servers */
        *s = toupper(*default_);
        ipc_warning("$S_BASE is deprecated, use $%s instead", var);
        return s;
    }
#endif /* S_BASE_COMPAT */

    return trunc(default_, sizeof(key_t));
}

/*
 * ipc_attach() -- bind a server queue
 */

int
ipc_attach(char *name)
{
    int queue;

    TRACE(("[IPC] attach: \"%s\"\n", name));

    if ((queue = msgget(makekey(name), IPC_CREAT | IPC_EXCL | 0777)) == -1)
    {
        ipc_system_error("can't create queue (%s)", name);
	switch (errno)
 	{
	    case EACCES:
	    printf ("EACCES\n");
	    break;

	    case EEXIST:
	    printf ("EEXIST\n");
	    break;

	    case ENOENT:
	    printf ("ENOENT\n");
	    break;

	    case ENOSPC:
	    printf ("ENOSPC\n");
	    break;
	}
    }
    return queue;
}

/*
 * ipc_detach() -- detach server queue
 */

void
ipc_detach(int queue)
{
    TRACE(("[IPC] detach: %x\n", queue));

    if (msgctl(queue, IPC_RMID, NULL) == -1)
        ipc_system_error("can't remove queue");
}

/*
 * ipc_connect() -- return a server IPC handle
 *
 * This function has the side effect of creating the client's reply queue
 * on the first invocation.
 */

int
ipc_connect(char *name)
{
    int queue;

    TRACE(("[IPC] locate: \"%s\"\n", name));

    /*
     * Get a handle on the server's queue.
     */

    if ((queue = msgget(makekey(name), 0)) == -1)
    {
        ipc_system_error("can't find queue (%s)", name);
        return -1;
    }

    TRACE(("[IPC] server: name %s, queue %d\n", name, queue));

    if (ipc_count == 0)
    {
        /*
         * Bind a reply queue.  The key is intended to be unique,
         * but this isn't necessarily the case on systems with
         * >16-bit pids.  'SQxx' is the namespace we claim here.
         */

        ipc_pid = getpid();
        ipc_key = ('S' << 24) | ('Q' << 16) | (ipc_pid & 0xffff);
        ipc_queue = msgget(ipc_key, IPC_CREAT | IPC_EXCL | 0777);

        if (ipc_queue == -1)
        {
            ipc_system_error("can't create reply queue");
            return -1;
        }

        TRACE(("[IPC] client: pid %ld, key 0x0%lx, handle %d\n",
            (long) ipc_pid, (long) ipc_key, ipc_queue));
    }

    ipc_count++;

    return queue;
}

/*
 * ipc_terminate() -- shut down a connection to the IPC server
 *
 * There's nothing to do to "forget" a server queue handle, but
 * we have to clean up our reply queue when we no longer have any
 * active connections.
 */

void
ipc_terminate(int queue)
{
    if (--ipc_count == 0) {
        if (msgctl(ipc_queue, IPC_RMID, NULL) == -1)
            ipc_system_error("can't remove reply queue");
    }
}

/*
 * ipc_cleanup() -- tidy up all of a client's connections
 *
 * Tear down all active connections (no work there :-), and remove
 * our reply queue.
 */

void
ipc_cleanup(void)
{
    if (ipc_count > 0) {
        if (msgctl(ipc_queue, IPC_RMID, NULL) == -1)
            ipc_system_error("can't remove queue");
    }

    ipc_count = 0;
}

/*
 * ipc_diagnostic() -- send a short datagram
 *
 * The 'datagram' fits into the lower half of a long, and requires
 * no reply.  It is intended for infrequent use, and looks up the
 * recipient by name on each invocation (not that this is all that
 * inefficient).
 */

int
ipc_diagnostic(char *name, int diag)
{
    int id;
    long mtype;

    TRACE(("[IPC] diagnostic: %d => %s\n", diag, name));

    if ((id = msgget(makekey(name), 0)) == -1)
    {
        ipc_system_error("can't find queue (%s)", name);
        return -1;
    }

    mtype = IPC_DIAGNOSTIC | (diag & IPC_DIAGNOSTIC_MASK);

    if (msgsnd(id, (struct msgbuf *) &mtype, sizeof(long), 0) == -1)
    {
        ipc_system_error("error sending message");
        return -1;
    }

    return 0;
}

/*
 * ipc_send_event() -- send an exception event
 *
 * XXX this might block if the queue is full, which is critical if
 * sending to yourself, in an ISR for example.  The solution may be
 * to use signals.
 */

int
ipc_send_event(int queue, unsigned long event)
{
    long mtype;

    TRACE(("[IPC] send_event: %lx => %x\n", event, queue));

    mtype = IPC_EXCEPTION | (event & IPC_EXCEPTION_MASK);

    if (msgsnd(queue, (struct msgbuf *) &mtype, sizeof(event), 0) == -1)
    {
        ipc_system_error("error sending event");
        return -1;
    }

    return 0;
}

/*
 * ipc_send() -- issue a simple IPC request
 */

int
ipc_send(int queue, void *sbuf, void *rbuf, int slen, int rlen)
{
    XMSG smsg, rmsg;
    SETMSG(&smsg, sbuf, slen);
    SETMSG(&rmsg, rbuf, rlen);
    return ipc_sendm(queue, 1, 1, &smsg, &rmsg);
}

/*
 * ipc_sendm() -- issue a multi-part IPC request
 *
 * Since the System V message passing primitives can't do scatter-gather
 * I/O, we have to copy everything into a single message.  (NOTE: even
 * with only a single block to send, we still need to insert a type field
 * for the start of the 'msgbuf' structure - make sure it's never zero!)
 */

int
ipc_sendm(int queue, int scount, int rcount,
      struct ipc_list *smsg, struct ipc_list *rmsg)
{
    static struct ipc_buffer ipcbuf;
    int len;

    /* set up the IPC header */

    ipcbuf.mtype = IPC_MESSAGE;
    ipcbuf.pid = ipc_pid;
    ipcbuf.key = ipc_key;

    TRACE(("[IPC] sendm: pid %lu, key 0x0%lx, count %d/%d\n",
        (unsigned long) ipcbuf.pid, (long) ipcbuf.key, scount, rcount));

    /* copy the request into the IPC buffer */

    if ((len = copy_in_msg(scount, smsg, ipcbuf.data, MAXMSGSZ)) == -1) {
        ipc_user_error(IPC_ERR_MESSAGE, "request too big");
        return -1;
    }

    /* let the server know how large a response we can handle */

    ipcbuf.len = total_size(rcount, rmsg);

    XTRACE(("[IPC] sendm: pid %d, key 0x%x, len %d/%d (SND)\n",
        ipcbuf.pid, ipcbuf.key, len, ipcbuf.len));

    /* send the request */
    if (msgsnd(queue, (struct msgbuf *) &ipcbuf, IPCHDR+len, 0) == -1)
    {
        ipc_system_error("error sending request");
        return -1;
    }

    /* wait for the response */
    if ((len = msgrcv(ipc_queue, (struct msgbuf *) &ipcbuf,
	    IPCHDR+ipcbuf.len, 0, 0)) == -1)
    {
        ipc_system_error("error receiving reply");
        return -1;
    }
    /* XXX assert(ipcbuf.mtype & IPC_MESSAGE) */

    XTRACE(("[IPC] sendm: pid %d, key 0x%x, len %d/%d (RCV)\n",
        ipcbuf.pid, ipcbuf.key, len-IPCHDR, MAXMSGSZ));

    /* copy the reply back out */
    (void) copy_out_msg(rcount, rmsg, ipcbuf.data, len-IPCHDR);

    return 0;
}

/*
 * ipc_receive*() -- listen for an incoming IPC request
 *
 * The pid-returning variants are temporary for the current msgserver
 * implementation of stremul; when they go away, the code just moves
 * into the non-pid versions.
 */

struct ipc_request *
ipc_receive(int queue, void *buf, int len)
{
    return ipc_receive_pid(queue, buf, len, NULL);
}

struct ipc_request *
ipc_receivem(int queue, int count, struct ipc_list *msg)
{
    return ipc_receivem_pid(queue, count, msg, NULL);
}

struct ipc_request *
ipc_receive_pid(int queue, void *buf, int len, pid_t *pidp)
{
    XMSG msg;

    SETMSG(&msg, buf, len);

    return ipc_receivem_pid(queue, 1, &msg, pidp);
}

struct ipc_request *
ipc_receivem_pid(int queue, int count, struct ipc_list *msg, pid_t *pidp)
{
    struct ipc_buffer *ipcbuf;
    struct ipc_request *request;
    int len;

    TRACE(("[IPC] receivem: queue id %x, count %d\n", queue, count));

    /* allocate a fresh IPC buffer and state structure */

    if ((ipcbuf = malloc(sizeof(struct ipc_buffer))) == NULL)
    {
        ipc_memory_error("request buffer");
        return IPC_FAIL;
    }

    if ((request = malloc(sizeof(struct ipc_request))) == NULL)
    {
        ipc_memory_error("request structure");
        free(ipcbuf);
        return IPC_FAIL;
    }

    request->ipcb = ipcbuf;

    /* wait for an IPC request */

rcv:
    if ((len =
       msgrcv(queue, (struct msgbuf *) ipcbuf, IPCHDR+MAXMSGSZ, 0, 0)) == -1)
    {
        ipc_system_error("error receiving request");
        free(ipcbuf);
        free(request);
        return IPC_FAIL;
    }

    if (ipcbuf->mtype & IPC_EXCEPTION)
    {
        TRACE(("[IPC] receivem: exception %lx\n",
            ipcbuf->mtype & IPC_EXCEPTION_MASK));

        if (ipc_exception_handler)
            (*ipc_exception_handler)(ipcbuf->mtype & IPC_EXCEPTION_MASK);

        goto rcv;
    }

    if (ipcbuf->mtype & IPC_DIAGNOSTIC)
    {
        TRACE(("[IPC] receivem: diagnostic %ld\n",
            ipcbuf->mtype & IPC_DIAGNOSTIC_MASK));

        if (ipc_diagnostic_handler)
            (*ipc_diagnostic_handler)(ipcbuf->mtype & IPC_DIAGNOSTIC_MASK);
        goto rcv;
    }

    /* XXX assert(ipcbuf->mtype & IPC_MESSAGE) */

    request->len = len-IPCHDR;

    XTRACE(("[IPC] receivem: mtype %d, pid %d, key 0x%x, len %d/%d\n",
        ipcbuf->mtype, ipcbuf->pid, ipcbuf->key, request->len, ipcbuf->len));

    if (pidp)
        *pidp = ipcbuf->pid;    /* XXX not authenticated */

    /*
     * Get a handle on the requester's queue.
     *
     * Note: we could avoid doing this for every request by adding
     * an additional bit of handshake during ipc_connect(), whereby
     * the client contacts the server, tells it its pid and key,
     * and the server does the msgget() once, caches the pid and
     * queue identifier, and returns a pointer to this data as
     * a handle to the client, which passes it back on each request
     * in place of the pid/key pair.
     */

    if ((request->queue = msgget(ipcbuf->key, 0)) == -1)
    {
        ipc_system_error("can't find client's reply queue (key %lx)",
            (unsigned long) ipcbuf->key);
        free(ipcbuf);
        free(request);
        return IPC_FAIL;
    }

    /* give the server as much as requested */

    request->off = copy_out_msg(count, msg, ipcbuf->data, request->len);

    return request;
}

int
ipc_read(struct ipc_request *request, void *buf, int len)
{
    XMSG msg;

    SETMSG(&msg, buf, len);

    return ipc_readm(request, 1, &msg);
}

int
ipc_readm(struct ipc_request *request, int count, struct ipc_list *msg)
{
    int sz;

    XTRACE(("[IPC] readm(%d)\n", count));

    sz = copy_out_msg(count, msg, request->ipcb->data + request->off,
                         request->len - request->off);

    XTRACE(("[IPC] readm: got %d bytes\n", sz));

    request->off += sz;

    return sz;
}

int
ipc_reply(struct ipc_request *request, void *buf, int len)
{
    XMSG msg;

    SETMSG(&msg, buf, len);

    return ipc_replym(request, 1, &msg);
}

int
ipc_replym(struct ipc_request *request, int count, struct ipc_list *msg)
{
    int len;

    XTRACE(("[IPC] replym(%d)\n", count));

    if ((len = copy_in_msg(count, msg, request->ipcb->data, request->ipcb->len)) == -1)
    {
        ipc_user_error(IPC_ERR_MESSAGE, "response too big");
        return -1;
    }

    if (msgsnd(request->queue, request->ipcb, IPCHDR+len, 0) == -1)
    {
        ipc_system_error("error sending reply");
        return -1;
    }

    free(request->ipcb);
    free(request);

    return 0;
}
