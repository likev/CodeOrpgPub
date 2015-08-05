
/*******************************************************************

    This module implements the TEST_OPTIONS (TO) functions.
    MISC_test_options tests if a test function is defined at run time.

    The mechanism of simulating networking delay is implemented here.
    All routines that call socket write, accept and connect must include
    misc.h. Environ. variable TEST_OPTIONS must be defined when
    compiling. -lthread must be used in link time.
    SIMULATE_NETWORK_DELAY:ip, where ip is the remote ip, must be part
    of INFR_TEST_OPTIONS in run time (on both sides of the network).

*******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/24 21:05:16 $
 * $Id: misc_test_options.c,v 1.6 2013/04/24 21:05:16 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <misc.h>
#include <en.h>

#ifdef TEST_OPTIONS

/****************************************************************

    Returns the content (pointer to after the "opt") of the test 
    option (by environ variable "INFR_TEST_OPTIONS").

****************************************************************/

char *MISC_test_options (char *opt) {
    static char *opts = (char *)1;
    char *p;

   if (opts == (char *)1) {
	char *env = getenv ("INFR_TEST_OPTIONS");
	opts = env;
	if (env != NULL) {
	    opts = MISC_malloc (strlen (env) + 1);
	    strcpy (opts, env);
	}
    }
    if (opts == NULL)
	return (NULL);
    p = strstr (opts, opt);
    if (p == NULL)
	return (NULL);
    p += strlen (opt);
    if (*p != '\0')
	p++;
    return (p);
}

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>

#include <str.h>

#undef write
#undef connect
#undef accept
#undef close

/* bit flags for L_fd_t.types */
#define NET_DELAY_TYPE 0x1
#define NET_FLOW_TYPE 0x2
#define NET_FD_CLOSE 0x8
#define MAX_SIM_IPS 8

static int Sim_bw = 0;	/* simulated BW - K kit per second */
static int Sim_delay = 0;	/* simulated one-way network delay in ms */
static unsigned int Sim_ip[MAX_SIM_IPS];
				/* IPs for simulated connection */
static int N_sim_ips = 0;	/* # of IPs for simulated connection */

typedef struct {	/* message waiting for writing */
    char *msg;
    int size;
    double ms;
    void *next;
} Wmsg_t;

typedef struct {	/* listed file descriptor type */
    int fd;		/* < 0 indicates a closed fd */
    int types;
    int sent;		/* number of bytes of the first msg sent */
    int bbytes;		/* number of bytes of the first msg blocked */
    Wmsg_t *first;
    Wmsg_t *last;
} L_fd_t;

static L_fd_t *Listed_fds = NULL;
static int N_listed_fds = 0;

static pthread_t Upd_thread;
static pthread_mutex_t Sync_mutex = PTHREAD_MUTEX_INITIALIZER;

static L_fd_t *Get_listed_fd (int fd);
static void Add_listed_fd (int fd, int type);
static int Check_peer_ip (int fd);
static void *Write_update_thread (void *arg);
static double Get_current_time ();
static void Start_update_thread ();

static int Is_simulate_network () {
    static int sim_opt = -1;

    if (sim_opt < 0) {
	char *p = MISC_test_options ("SIMULATE_NETWORK");
	if (p == NULL)
	    sim_opt = 0;
	else {
	    char *vp;
	    int v;
	    unsigned int ip;

	    sim_opt = 1;
	    Sim_bw = 0;
	    if ((vp = strstr (p, "BW")) != NULL &&
		sscanf (vp + 2, "%d", &v) == 1)
		Sim_bw = v;
	    Sim_delay = 0;
	    if ((vp = strstr (p, "DELAY")) != NULL &&
		sscanf (vp + 5, "%d", &v) == 1)
		Sim_delay = v;
	    vp = p;
	    N_sim_ips = 0;
	    while (N_sim_ips < MAX_SIM_IPS &&
		(vp = strstr (vp, "IP")) != NULL &&
		sscanf (vp + 2, "%x", &ip) == 1) {
		Sim_ip[N_sim_ips] = ip;
		vp += 2;
		N_sim_ips++;
	    }

	    if (N_sim_ips == 0 || (Sim_bw == 0 && Sim_delay == 0)) {
		MISC_log ("MISC_TO_connect: NETWORK SIM ignored (%s)\n", p);
		sim_opt = 0;
	    }
	}
    }
    return sim_opt;
}

/************************************************************************

    This is the instrumented "write". If the fd needs delay simulation,
    we saved the data to write and the time called. All saved messages
    are stored in a linked list.

************************************************************************/

ssize_t MISC_TO_write (int fd, const void *buf, size_t count) {
    L_fd_t *crfd;
    Wmsg_t *msg;

    crfd = Get_listed_fd (fd);
    if (crfd == NULL || count <= 0)
	return (write (fd, buf, count));

    pthread_mutex_lock (&Sync_mutex);
    msg = (Wmsg_t *)MISC_malloc (sizeof (Wmsg_t) + count);
    msg->msg = (char *)msg + sizeof (Wmsg_t);
    memcpy (msg->msg, buf, count);
    msg->size = count;
    msg->ms = Get_current_time ();
    msg->next = NULL;
    if (crfd->first == NULL)
	crfd->first = msg;
    else if (crfd->last == NULL) {
	((Wmsg_t *)crfd->first)->next = msg;
	crfd->last = msg;
    }
    else {
	((Wmsg_t *)crfd->last)->next = msg;
	crfd->last = msg;
    }
    pthread_mutex_unlock (&Sync_mutex);
    return (count);
}

/************************************************************************

    Returns the current time in ms.

************************************************************************/

static double Get_current_time () {
    int ms;
    double sec = (double)MISC_systime (&ms);
    return (sec * 1000. + ms);
}

/************************************************************************

    This is the instrumented "close". We mark the fd to be closed. The
    close will be performed after all pending writes are completed.

************************************************************************/

int MISC_TO_close (int fd) {
    L_fd_t *crfd;

    pthread_mutex_lock (&Sync_mutex);
    crfd = Get_listed_fd (fd);
    if (crfd != NULL)
	crfd->types |= NET_FD_CLOSE;
    pthread_mutex_unlock (&Sync_mutex);
    if (crfd == NULL)
	return (close (fd));
    return (0);			/* we delay close */
}

/************************************************************************

    This is the instrumented "connect". When connect succeeds, we put the fd
    in the list for delayed writing if the remote side of the socket is
    connected through delay simulation network.

************************************************************************/

int MISC_TO_connect (int sockfd, const struct sockaddr *serv_addr,
					socklen_t addrlen) {
    int ret;

    if (!Is_simulate_network ())
	return (connect (sockfd, serv_addr, addrlen));

    if ((ret = connect (sockfd, serv_addr, addrlen)) < 0)
	return (ret);
    if (Check_peer_ip (sockfd))
	Add_listed_fd (sockfd, NET_DELAY_TYPE);

    return (ret);
}

/************************************************************************

    This is the instrumented "accept". When accept succeeds, we put the fd
    in the list for delayed writing if the remote side of the socket is
    connected through delay simulation network.

************************************************************************/

int MISC_TO_accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd;

    if (!Is_simulate_network ())
	return (accept (sockfd, addr, addrlen));

    fd = accept (sockfd, addr, addrlen);
    if (fd < 0)
	return (fd);
    if (Check_peer_ip (fd))
	Add_listed_fd (fd, NET_DELAY_TYPE);

    return (fd);
}

/************************************************************************

    This checks socket "fd" and, if it is a long_delay connection, adds
    it to the list. This is needed when a socket fd is created not by
    connect or accpet (e.g. passed to child from its parent). When
    called with fd < 0, this causes the update thread to start if not
    already started.

************************************************************************/

void MISC_TO_add_fd (int fd) {

    if (!Is_simulate_network ())
	return;

    if (fd < 0)
	Start_update_thread ();
    else if (Check_peer_ip (fd))
	Add_listed_fd (fd, NET_DELAY_TYPE);
}

/************************************************************************

    Starts the thread to process the delayed calls to write and close.
    We need to make sure we start such a thread only once for the
    process. In case a child process is forked, we need to restart the
    thread for the child. This function must be called by programs that
    fork a child.

************************************************************************/

static void Start_update_thread () {
    static int pid = -1;
    int ret, sig;
    sigset_t nset, oset;

    if (getpid () == pid)
	return;
    sig = EN_control (EN_GET_SIGNAL);
    if (sig >= 0) {
	sigemptyset (&nset);
	sigaddset (&nset, sig);
	pthread_sigmask (SIG_BLOCK, &nset, &oset);
    }
    ret = pthread_create (&Upd_thread, NULL, Write_update_thread, NULL);
    if (ret != 0) {
	MISC_log ("MISC_TO: pthread_create failed (%d, errno %d)\n", ret, errno);
	exit (1);
    }
    pid = getpid ();
    if (sig >= 0)
	pthread_sigmask (SIG_SETMASK, &oset, NULL);
}

/************************************************************************

    This is the thread that performs the delayed write and close. Because
    the fd may be non-blocking, we must call write repeatedly until all
    bytes are written.

************************************************************************/
#include <poll.h>
#define MAX_BL_FDS 32

static void *Write_update_thread (void *arg) {

    double rate = (double)Sim_bw * 1024. / (1000. * 8.);
    if (Sim_bw == 0)
	rate = 100000. * 1024. / (1000. * 8.);	/* a large number */

    while (1) {
	static double prev_t = 0.;
	int i, n_bl;
	double crt;
	struct pollfd pfds[MAX_BL_FDS];

	pthread_mutex_lock (&Sync_mutex);
	crt = Get_current_time ();
	if (prev_t == 0.)
	    prev_t = crt - 100.;
	n_bl = 0;
	for (i = 0; i < N_listed_fds; i++) {
	    Wmsg_t *msgs, *next;
	    int nb;

	    L_fd_t *lfd = Listed_fds + i;
	    if (lfd->fd < 0)
		continue;
	    msgs = lfd->first;

	    nb = (crt - prev_t) * rate + lfd->bbytes;
	    lfd->bbytes = 0;
	    while (msgs != NULL) {
		int nsend, ret;

		if (nb <= 0)
		    break;
		if (lfd->sent > 0) {
		    nsend = msgs->size - lfd->sent;
		}
		else {
		    nsend = (crt - (msgs->ms + Sim_delay)) * rate;
		    if (nsend <= 0)		/* need further delay */
			break;
		    if (nsend > msgs->size)
			nsend = msgs->size;
		}
		if (nsend > nb)
		    nsend = nb;
		ret = 0;
		if (nsend > 0) {
		    ret = write (lfd->fd, msgs->msg + lfd->sent, nsend);
		    if (ret > 0) {
			lfd->sent += nsend;
			nb -= nsend;
		    }
		    if ((ret >= 0 && ret < nsend) || 
			(ret < 0 && errno == EAGAIN)) {	/* write buffer full */
			if (ret >= 0)
			    lfd->bbytes = nsend - ret;
			else
			    lfd->bbytes = nsend;
			if (n_bl < MAX_BL_FDS) {
			    pfds[n_bl].fd = lfd->fd;
			    pfds[n_bl].events = POLLOUT;
			    n_bl++;
			}
			break;
		    }
		}
		if (ret < 0)
		    MISC_log ("MISC_TO: Delayed write failed (fd %d, errno %d)\n", 
			    				lfd->fd, errno);

		if (ret < 0 || lfd->sent >= msgs->size) {
		    next = msgs->next;
		    free (msgs);
		    msgs = next;
		    lfd->sent = 0;	    
		}
	    }

	    lfd->first = msgs;
	    if (msgs == NULL || msgs->next == NULL)
		lfd->last = NULL;
	    if (msgs == NULL && (lfd->types & NET_FD_CLOSE) && lfd->fd >= 0) {
		close (lfd->fd);
		lfd->fd = -1;
	    }
	}
	prev_t = crt;
	pthread_mutex_unlock (&Sync_mutex);
	poll (pfds, n_bl, 100);
    }
}

/************************************************************************

    Checks the peer address of fd against Sim_ip. Returns true if fd is a
    socket to be simulated, or false otherwise.

************************************************************************/

static int Check_peer_ip (int fd) {
    struct sockaddr_storage sadd;
    unsigned int len;

    len = sizeof (sadd);
    if (getpeername (fd, (struct sockaddr *)&sadd, &len) < 0) {
	if (errno == ENOTSOCK)
	    return (0);
	MISC_log ("MISC_TO: getpeername failed (%d, errno %d)\n", fd, errno);
	return (0);
    }
    if (sadd.ss_family == AF_INET) {
	int i;
	struct sockaddr_in *s = (struct sockaddr_in *)&sadd;
	unsigned int ip = ntohl (s->sin_addr.s_addr);
	for (i = 0; i < N_sim_ips; i++) {
	    if (ip == Sim_ip[i])
		return (1);
	}
    }
    return (0);
}

/************************************************************************

    Returns the listed fd struct for "fd". Returns NULL if such a struct
    does not exist.

************************************************************************/

static L_fd_t *Get_listed_fd (int fd) {
    int i;

    for (i = 0; i < N_listed_fds; i++) {
	if (Listed_fds[i].fd == fd)
	    return (Listed_fds + i);
    }
    return (NULL);
}

/************************************************************************

    Adds fd to the simulation fd list. The types is set to "type".

************************************************************************/

static void Add_listed_fd (int fd, int type) {
    L_fd_t nfd, *crfd;
    int i;

    pthread_mutex_lock (&Sync_mutex);

    if (Get_listed_fd (fd) != NULL) {
	MISC_log ("MISC_TO: fd (%d) is already in the list\n", fd);
	pthread_mutex_unlock (&Sync_mutex);
	return;
    }

    for (i = 0; i < N_listed_fds; i++) {
	if (Listed_fds[i].fd < 0)
	    break;
    }
    if (i < N_listed_fds)
	crfd = Listed_fds + i;
    else
	crfd = &nfd;
    memset (crfd, 0, sizeof (L_fd_t));
    crfd->fd = fd;
    crfd->types = type;
    crfd->sent = 0;
    crfd->bbytes = 0;
    if (crfd == &nfd) {
	Listed_fds = (L_fd_t *)STR_append (Listed_fds, crfd, sizeof (L_fd_t));
	N_listed_fds++;
    }

    pthread_mutex_unlock (&Sync_mutex);
    Start_update_thread ();
}

#else

void MISC_TO_add_fd (int fd) {
    return;
}
char *MISC_test_options (char *opt) {
    return (NULL);
}

#endif
