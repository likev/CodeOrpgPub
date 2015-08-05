/******************************************************************

	File: bcast.c 

******************************************************************/
/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 17:30:27 $
 * $Id: bcast.c,v 1.39 2014/03/18 17:30:27 jeffs Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef SUNOS
#include <sys/sockio.h>
#endif

#include <netinet/in.h>		/* for IPPROTO_TCP value */
#include <netinet/tcp.h>	/* for TCP_NODELAY value */
#include <arpa/inet.h>
 
#include "bcast_def.h"
#include <lb.h>
#include <misc.h>

#if (defined (SUNOS3) || defined (LINUX))
#define sigset(a,b) signal(a,b)
#endif

#if defined (SLRS_X86)
    #define DEFAULT_INTERFACE "elxl0"
#elif defined (SLRS_SPK)
    #define DEFAULT_INTERFACE "le0"
#elif defined (LNUX_X86)
    #define DEFAULT_INTERFACE "eth0"
#elif defined (HPUX_RSK)
    #define DEFAULT_INTERFACE "lan0"
#else
    #define DEFAULT_INTERFACE ""
#endif

#define MAX_REQ_SIZE	((MAX_NREQUEST + ACK_REQ_HD) * sizeof(int))
				/* maximum size of the request packet */
#define MAX_N_CLIENTS	64	/* max number of clients (brecvs) */

#define WAIT_MSECONDS	200	/* basic pace rate in ms */
#define CONNECT_POLL_TIME 3	/* the min rate (in seconds) of polling 
				   new clients */
#define ACK_POLL_TIME	1	/* the min rate (in seconds) of polling
				   acknowledgement messages */
#define MAX_QUIET_TIME	10	/* maximum no response time from clients
				   (in seconds) */
#define NORESP_CHECK_TIME  2	/* non-responding client check period in 
				   seconds */

/* general control vars */
char Prog_name[NAME_SIZE];	/* name of this program */
static int Msg_mode;		/* statistics print mode */
static int Discard_in_front = 0;/* indicates the number of bytes to discard 
				   at beginning of each message */
static int Wait_time;		/* wait time (MS) after sending each msg */
static int Signal_received;	/* flag indicating a terminating signal 
				   is received */
static int St_print_period;	/* time period printing statistics 
				   (in seconds) */
static int Noresp_time;		/* max time (in seconds) allowed for keeping 
				   a unresponding client */
static int Test_mode;		/* test mode - random packet discard */

/* input LB */
static char Inp_lb_name[NAME_SIZE] = "";
				/* name of the input LB */
static int In_lbd;		/* input LB fd */


/* define the packet buffer */
static int Pb_size;		/* packet buffer size - in number of 
				   packets */
static char *UDP_buf[MAX_PB_SIZE];
				/* pointers to stored UDP packets */
static int *TCP_hd[MAX_PB_SIZE];
				/* pointers to stored TCP packets; The TCP
				   packet has an additional int (total
				   length) in front of the UDP packet */
static int *TCP_len;		/* length of stored TCP packets */

/* packet control vars */
static unsigned int Seq_n;	/* sequence number of the latest 
				   processed packet (seq # starts with 1. 
				   0 indicates that no packet has ever 
				   been sent) */
static int Poll_rate;		/* Max number of packets sent before a 
				   acknowledgement reception */
static unsigned int Echo_n;	/* latest packet number received by all
				   receivers */
static int Bcast_id;		/* an ID number of this bcast */

/* statistic vars */
static unsigned int Send_cnt;	/* packet send count */
static unsigned int Resend_cnt;	/* packet resend count */
static unsigned int Last_cnt;	/* last packet resend count */
static unsigned int Lost_cnt;	/* lost input message count */
static time_t Cr_time;		/* current UNIX time */

/* commu ports */
static char Multicast_ip[NAME_SIZE];	
				/* multicast IP address (ascii) */
static char Multicast_itf[NAME_SIZE];	
				/* local IP of multicast interface (ascii) */
static char Interface[NAME_SIZE];
				/* broadcast network interface name */
static int Port_number;		/* port number used for receiving datagram */
static struct sockaddr_in Br_addr;
				/* broadcast address */
static int Soc_TCP;		/* TCP socket fd */
static int Soc_UDP;		/* UDP socket fd */
static int UDPp_size;		/* max UDP packet size */

/* client registration */
typedef struct {		/* client registration structure */
    int fd;			/* socket fd */
    unsigned int seq;		/* acknowledged sequence number */
    unsigned int address;	/* client internet address */ 
    time_t time;		/* time of receiving recent ack */
} Client_regist;

static Client_regist Client[MAX_N_CLIENTS];
				/* client registration list */
static int N_clients;		/* number of registered clients */


/* local functions */
static int Read_options (int argc, char **argv);
static int Open_sockets ();
static void Signal_handler ();
static void Send_a_msg (int length, char *msg);
static void Resend_last_packet ();
static void Accept_connection ();
static void Deregister_client (int fd);
static void Poll_acknowledgements ();
static unsigned int Resend_packets (char *buf, int fd);
static void Process_control_msg (int fd, char *buf);
static void Print_statistics ();
static void Check_noresp_time ();
static void Update_echo_n ();


/******************************************************************

    Description: This is the main function for bcast.

******************************************************************/

int main (int argc, char **argv, char **envp)
{
    int msgbuf_size;		/* buffer size for user messages */
    int i;
    char *msg_buf;
    char *cpt;

    /* initialize constants and set default values */
    strncpy (Prog_name, argv[0], NAME_SIZE);
    Prog_name[NAME_SIZE - 1] = '\0';
    Port_number = 43333;
    UDPp_size = 1400;
    msgbuf_size = 4096;		/* initial message buffer size */
    strcpy (Multicast_ip, "");
    strcpy (Multicast_itf, "");
    strcpy (Interface, DEFAULT_INTERFACE);
    Pb_size = 64;
    Test_mode = 0;

    Msg_mode = 0;
    Wait_time = 0;
    Signal_received = 0;
    Seq_n = 0;
    Echo_n = 0;
    Send_cnt = 0;
    Resend_cnt = 0;
    Last_cnt = 0;
    Lost_cnt = 0;
    N_clients = 0;
    St_print_period = 10;
    Noresp_time = 60;

    /* get command line options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    /* set ID number */
    Bcast_id = time (NULL) & 0xffff;

    /* open sockets */
    if (Open_sockets () < 0)
	exit (1);

    /* open input Linear Buffer */
    In_lbd = LB_open (Inp_lb_name, LB_READ, NULL);
    if (In_lbd < 0) {
	fprintf (stderr, "LB_open failed (%s, ret = %d) - %s\n",
		Inp_lb_name, In_lbd, Prog_name);
	exit (1);
    }

    /* catch the signals */
    sigset (SIGTERM, Signal_handler); 
    sigset (SIGHUP, Signal_handler);
    sigset (SIGINT, Signal_handler); 
    sigset (SIGPIPE, SOCK_sigpipe_int); 

    /* allocate the packet buffers */
    cpt = malloc (Pb_size * (UDPp_size + sizeof (int)));
    TCP_len = (int *)malloc (Pb_size * sizeof (int));
    if (cpt == NULL || TCP_len == NULL) {
	fprintf (stderr, "malloc failed - %s\n", Prog_name);
	MAIN_exit ();
    }
    for (i = 0; i < Pb_size; i++) {
	TCP_hd[i] = (int *)cpt;
	UDP_buf[i] = cpt + sizeof (int);
	cpt += (UDPp_size + sizeof (int));
	TCP_len[i] = 0;
    }

    /* initialize space for messages */
    msg_buf = malloc (msgbuf_size);
    if (msg_buf == NULL) {
	fprintf (stderr, "malloc failed - %s\n", Prog_name);
	MAIN_exit ();
    }

    /* rate of polling the acknowledgement ports */
    Poll_rate = (int)((double)Pb_size * REQ_WINDOW_CONST);
    if (Poll_rate <= 0)
	Poll_rate = 1;

    SOCK_set_max_seq (Pb_size);		/* init max seq */

    /* the main loop */
    Cr_time = MISC_systime (NULL);
    while (1) {
	int leng;

	if (Signal_received != 0)
	    MAIN_exit ();

	/* read a message */
	leng = LB_read (In_lbd, msg_buf, msgbuf_size, LB_NEXT);

	if (leng == LB_EXPIRED) {
	    Lost_cnt++;
	    continue;
	}
	else if (leng == LB_TO_COME) {
	    static int pre_time = 0;

	    msleep (WAIT_MSECONDS);
	    Cr_time = MISC_systime (NULL);
	    if (Cr_time - pre_time >= ACK_POLL_TIME) {
		Poll_acknowledgements ();
		Resend_last_packet ();
		pre_time = Cr_time;
	    }
	    continue;
	}
	else if (leng == LB_BUF_TOO_SMALL) {
					/* reallocate the buffer */
	    free (msg_buf);
	    msgbuf_size *= 2;
	    msg_buf = malloc (msgbuf_size);
	    if (msg_buf == NULL) {
		fprintf (stderr, "malloc failed - %s\n", Prog_name);
		MAIN_exit ();
	    }
	    LB_seek (In_lbd, -1, LB_CURRENT, NULL);
	    continue;
	}
	else if (leng == 0) {
	    continue;
	}
	else if (leng < 0) {
	    fprintf (stderr, 
		"Fatal error reading input LB (LB_read ret = %d) - %s\n", 
						leng, Prog_name);
	    MAIN_exit ();
	}

	/* broadcast this message */
	if (leng - Discard_in_front > 0)
	    Send_a_msg ((leng - Discard_in_front), 
					(msg_buf + Discard_in_front));
	if (Wait_time > 0)
	    msleep (Wait_time);
    }
    exit (0);
}

/******************************************************************

    Description: This function closes the input LB and exits.

******************************************************************/

void MAIN_exit ()
{

    LB_close (In_lbd);
    if (Msg_mode)
	printf ("%s exits\n", Prog_name);
    exit (0);
}

/******************************************************************

    Description: This is the callback function when a terminating
		signal is received. It sets flag Signal_received.

    Inputs:	sig - signal number;

******************************************************************/

static void Signal_handler (int sig)
{

    Signal_received = 1;
}

/******************************************************************

    Description: This function segments and broadcasts a message.
		All message segments are recorded for retransmission.

    Inputs:	length - length of the message to be sent;
		msg - pointer to the message to be sent;

    Notes:	This function terminates the program on fatal error
		conditions.

******************************************************************/

static void Send_a_msg (int length, char *msg)
{
    int leng, offset, left;

    offset = 0;
    leng = UDPp_size - TRAILER_SIZE;
    left = length;

    while (1) {
        unsigned char *cpt, flag;
        int loc;

	if (SOCK_seq_cmp (GREATER_EQUAL, Seq_n, 
		SOCK_seq_add (Echo_n, Pb_size))) { /* packet buffer full */
	    Poll_acknowledgements ();
	    Resend_last_packet ();
	    msleep (WAIT_MSECONDS);
	    Cr_time = MISC_systime (NULL);
	    continue; 
	}

	Seq_n = SOCK_seq_add (Seq_n, 1);

	loc = Seq_n % Pb_size;

	if (left <= leng)
	    leng = left;	/* single packet msg or the last packet */

	/* save the packet */
	memcpy (UDP_buf[loc], msg + offset, leng);
	TCP_len[loc] = leng + TRAILER_SIZE + sizeof (int);
	*(TCP_hd[loc]) = htonl(TCP_len[loc]);

	/* set packet trailer */
	cpt = (unsigned char *) (UDP_buf[loc] + leng);
	cpt[0] = Seq_n >> 24;
	cpt[1] = (Seq_n >> 16) & 0xff;
	cpt[2] = (Seq_n >> 8) & 0xff;
	cpt[3] = Seq_n & 0xff;
	if (left <= leng) {
	    if (offset == 0)
		flag = 0;
	    else
		flag = 3;
	}
	else {
	    if (offset == 0)
		flag = 1;
	    else
		flag = 2;
	}
	cpt[4] = flag;
	cpt[5] = (Bcast_id >> 8) & 0xff;
	cpt[6] = Bcast_id & 0xff;

	/* send the packet */
	if (N_clients > 0) {
	    int ret, drop;

	    drop = 0;
	    if (Test_mode) {
		int tm;
		static int last_tm = 0;
		static int drop_cnt = 0;

		if (drop_cnt > 0) {
		    drop_cnt--;
		    drop = 1;
		}
		else if ((rand () % 20) == 0) {
		    tm = MISC_systime (NULL);
		    if (tm - last_tm > 61) {
			last_tm = tm;
			drop_cnt = 80;
		    }
		    drop = 1;;
		}
	    }

	    if (!drop) {
		ret = sendto (Soc_UDP, UDP_buf[loc], 
			TCP_len[loc] - sizeof (int),
			0, (struct sockaddr *) &Br_addr, sizeof (Br_addr));
		if (ret != TCP_len[loc] - sizeof (int)) {
		    fprintf (stderr, "sendto failed (errno = %d) - %s\n", 
							errno, Prog_name);
		    MAIN_exit ();
		}
	    }
	    Send_cnt++;
	}

	if (Seq_n % Poll_rate == 0) {	/* get acknowledgments */
	    Cr_time = MISC_systime (NULL);
	    Poll_acknowledgements ();
	}

	offset += leng;
	left -= leng;
	if (left <= 0)
	    return;			/* done */
    }
}

/*******************************************************************

    Description: This function re-broadcast the last packet. Only the 
		trailer part is actually broadcasted.

*******************************************************************/

static void Resend_last_packet ()
{
    int loc;

    if (Seq_n == 0 || N_clients <= 0 || 
	SOCK_seq_cmp (GREATER_EQUAL, Echo_n, Seq_n))
	return;

    loc = Seq_n % Pb_size;
    if (sendto (Soc_UDP, 
	UDP_buf[loc] + (TCP_len[loc] - sizeof (int)) - TRAILER_SIZE, 
	TRAILER_SIZE, 0,
	(struct sockaddr *) &Br_addr, sizeof (Br_addr)) != TRAILER_SIZE) 
	fprintf (stderr, "sendto failed (errno = %d) - %s\n", errno, Prog_name);
    Last_cnt++;

    return;
}

/*******************************************************************

    Description: This function polls all acknowledgement TCP port
		and updates Echo_n, the packet number received by 
		all receivers. It also processes new connections.

*******************************************************************/

static void Poll_acknowledgements ()
{
    static time_t pre_connect = 0;	/* time of previous client 
					   connection try */
    static time_t pre_ack_poll = 0;	/* time of previous client 
					   ack read */
    ALIGNED_t buf[ALIGNED_T_SIZE (MAX_REQ_SIZE)];
    int hdsize;

    if (Signal_received != 0)
	MAIN_exit ();

    /* print statistics and get new connections */
    if (Msg_mode)
	Print_statistics ();
    Check_noresp_time ();
    if (Cr_time - pre_connect >= CONNECT_POLL_TIME) {
	Accept_connection ();
	pre_connect = Cr_time;
    }

    if (N_clients <= 0) {
	Echo_n = Seq_n;		/* no ack expected, we bring Echo_n to date */
	return;
    }

    hdsize = ACK_REQ_HD * sizeof (int);
    if ((Seq_n > 0 && Seq_n != Echo_n) || 
	Cr_time - pre_ack_poll > ACK_POLL_TIME) {
	int i, ret, *ipt;
	int updated;

	pre_ack_poll = Cr_time;

	ipt = (int *)buf;
	updated = 0;
	for (i = 0; i < N_clients; i++) {
	    int len;
	    unsigned int new_seq;

	    ret = SOCK_read_tcp (Client[i].fd, hdsize, (char *)buf);
	    if (ret == 0)
		continue;
	    else if (ret < 0) {		/* disconnected */
		Deregister_client (Client[i].fd);
		i--;
		continue;
	    }

	    len = ntohl (ipt[0]);
	    if (len > MAX_REQ_SIZE || len < hdsize) {
					/* unexpected message length */
		fprintf (stderr, "bad msg (len = %d) received - %s\n", 
							len, Prog_name);
		Deregister_client (Client[i].fd);
		i--;
		continue;
	    }

	    if (len > hdsize) {		/* read remaining part */
		ret = 0;
		while (ret == 0) {
		    ret = SOCK_read_tcp (Client[i].fd, len - hdsize, 
							(char *)buf + hdsize);
		}
		if (ret < 0) {		/* disconnected */
		    Deregister_client (Client[i].fd);
		    i--;
		    continue;
		}
	    }
	    Client[i].time = Cr_time;

	    new_seq = ntohl (ipt[1]);
	    if (SOCK_seq_cmp (GREATER_THAN, new_seq, Seq_n)) {
					/* unexpected sequence number */
		fprintf (stderr, "bad echo seq number (%u) received - %s\n", 
							new_seq, Prog_name);
		Deregister_client (Client[i].fd);
		i--;
		continue;
	    }
	    if (new_seq == CONTROL_MSG) {
					/* the new_seq == 0 && len == hdsize
					   msg has no meaning; It however can
					   be generated by brecv when it does
					   connection check while no packet
					   has ever received */
		if (len > hdsize)
		    Process_control_msg (Client[i].fd, (char *)buf);
	    }
	    else {

		Client[i].seq = new_seq;
		updated = 1;
		if (len > 2 * (int)sizeof (int)) {
		    unsigned int max_seq;

		    max_seq = Resend_packets ((char *)buf, Client[i].fd);
		    if (SOCK_seq_cmp (GREATER_THAN, max_seq, Client[i].seq))
			Client[i].seq = max_seq;
		}
	    }
	}

	if (updated && N_clients > 0)
	    Update_echo_n ();
    }

    return;
}

/******************************************************************
			
    Description: This function updates Echo_n.

******************************************************************/

static void Update_echo_n ()
{
    int i, found;
    unsigned int min, seq;

    found = 0;
    for (i = 0; i < N_clients; i++) {
	seq = Client[i].seq;
	if (seq == 0)		/* 0 is not a normal ACKd */
	    continue;
	if (!found) {
	    min = seq;
	    found = 1;
	}
	else if (SOCK_seq_cmp (GREATER_THAN, min, seq))
	    min = seq;
    }

    if (found)
	Echo_n = min;
    return;
}

/******************************************************************
			
    Description: This function processes a control message stored
		in "buf".

    Inputs:	fd - fd of the client that sent the control request;
		buf - buffer holding the control message;

******************************************************************/

static void Process_control_msg (int fd, char *buf)
{
    int *ipt;

    ipt = (int *)buf;
    if (ntohl (ipt[2]) == TERMINATE_HOST) {
	int address;
	int i;

	/* send acknowledgment */
	SOCK_write_tcp (fd, sizeof (CONTROL_ACK) + 1, CONTROL_ACK);

	address = ntohl (ipt[3]);
	for (i = 0; i < N_clients; i++) {

	    if (Client[i].address == address) {
		Deregister_client (Client[i].fd);
		i--;
	    }
	}

	return;
    }

    if (Msg_mode)
	printf ("unknown control message - %s\n", Prog_name);

    return;
}

/******************************************************************
			
    Description: This function resends packets according to the 
		client request message stored in "buf", to the
		client through socket "fd".

    Inputs:	buf - buffer holding the request message;
		fd - the socket fd connecting to the client;

    Return:	This function returns the maximum sequence number
		of the packets resent or 0 if no packet is resent 
		or the client is gone.

******************************************************************/

static unsigned int Resend_packets (char *buf, int fd)
{
    int *ipt, nreq;
    int i;
    unsigned int max_seq;

    ipt = (int *)buf;
    nreq = (ntohl (ipt[0]) / sizeof (int)) - 2;
    max_seq = 0;

    for (i = 0; i < nreq; i++) {
	unsigned int seq, ind;

	seq = ntohl (ipt[i + 2]);
	if (SOCK_seq_cmp (LESS_EQUAL, SOCK_seq_add (seq, Pb_size), Seq_n) || 
	    SOCK_seq_cmp (GREATER_THAN, seq, Seq_n) || 
	    seq == 0) {

	    fprintf (stderr, "bad request number received: - %s\n", Prog_name);
	    fprintf (stderr, "seq = %u; Seq_n = %u; Pb_size = %d\n", 
					seq, Seq_n, Pb_size);
	    Deregister_client (fd);
	    return (0);
	}
	ind = seq % Pb_size;
	if (SOCK_write_tcp (fd, TCP_len[ind], (char *)TCP_hd[ind]) < 0) {
	    Deregister_client (fd);
	    return (0);
	}
	if (SOCK_seq_cmp (GREATER_THAN, seq, max_seq))
	    max_seq = seq;
	Resend_cnt++;
    }

    return (max_seq);
}

/******************************************************************
			
    Description: This function accepts a client connection request
		and returns the client socket fd. Appropriate socket 
		properties are set for the client socket. The new 
		client is registered in the client table. It also
		sends several configuration constants to the new 
		client.

    Notes:	This function terminates this program on fatal error 
		conditions.

******************************************************************/

static void Accept_connection () 
{
    int fd;
    socklen_t len;
    static union sunion {
	struct sockaddr_in sin;
	struct sockaddr_un sund;
    } sadd;			/* the client address */

    len = sizeof (struct sockaddr_in);
				/* address buffer length */

    while (1) {
	int msg[FIRST_MSG_SIZE];

        errno = 0;
        fd = accept (Soc_TCP, (struct sockaddr *) &sadd, &len);

	if (fd < 0) {

	    if (errno == EINTR) continue;   /* retry */
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return;

	    fprintf (stderr, "accept failed (errno = %d) - %s\n", 
							errno, Prog_name);
	    MAIN_exit ();
	}

	/* set socket properties */
	if (SOCK_set_TCP_properties (fd) < 0) {
	    close (fd);
	    return;
	}

	/* register the new client */
	if (N_clients >= MAX_N_CLIENTS) {	/* too many clients */
	    fprintf (stderr, "too many clients - %s\n", Prog_name);
	    close (fd);
	    return;
	}
	Client[N_clients].fd = fd;
	Client[N_clients].address = ntohl (sadd.sin.sin_addr.s_addr);
	Client[N_clients].seq = 0;
	Client[N_clients].time = MISC_systime (NULL);
	N_clients++;

	/* send constants to the new client */
	msg[0] = htonl (Pb_size);
	msg[1] = htonl (UDPp_size);
	msg[2] = htonl (Bcast_id);
	msg[3] = htonl (BCAST_ID);
	if (SOCK_write_tcp (fd, FIRST_MSG_SIZE * sizeof (int), 
						(char *)msg) < 0)
	    Deregister_client (fd);
    }
}

/**********************************************************************

    Description: This function removes a client from the client table.

    Input:	fd - socket fd of the client to be removed.

**********************************************************************/

static void Deregister_client (int fd)
{
    int i, k;

    for (i = 0; i < N_clients; i++) {
	if (Client[i].fd == fd) {

	    if (Msg_mode)
		printf ("client at (%x) disconnected - %s\n", 
					Client[i].address, Prog_name);
	    for (k = i; k < N_clients - 1; k++)
		Client[k] = Client[k + 1];
	    i--;
	    N_clients--;
	    Update_echo_n ();
	}
    }
    close (fd);

    return;
}

/**********************************************************************

    Description: This function opens a UDP socket for broadcasting 
		packets and a TCP socket for acknowledgement and 
		retransmission. It also accepts connections from 
		all running brecvs at the time bcast starts.

    Return:	It returns 0 on success or -1 on error conditions.

**********************************************************************/

static int Open_sockets ()
{
    static struct sockaddr_in loc_soc;	/* local socket info */
    int tmp, i;

    /* open a broadcast socket */
    if ((Soc_UDP = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
	fprintf (stderr, "socket failed (errno = %d) - %s\n", errno, Prog_name);
	return (-1);
    }

    /* broadcast address */
    memset ((char *) &Br_addr, 0, sizeof (Br_addr));
    if (Multicast_ip[0] != '\0')
	Br_addr.sin_addr.s_addr = inet_addr (Multicast_ip);
    else {		/* get address on Interface */
	struct ifreq ifr;

	strncpy (ifr.ifr_name, Interface, sizeof (ifr.ifr_name));
#ifdef LINUX
	if (ioctl (Soc_UDP, SIOCGIFBRDADDR, (char *)&ifr) < 0) {
#else
	if (ioctl (Soc_UDP, SIOCGIFBRDADDR, (caddr_t)&ifr) < 0) {
#endif
	    fprintf (stderr, 
		"ioctl (SIOCGIFBRDADDR) failed (itfc = %s, errno = %d) - %s\n",
					Interface, errno, Prog_name);
	    return (-1);
	}
	memcpy ((char *)&Br_addr, (char *)&ifr.ifr_addr, 
					sizeof (struct sockaddr_in));
    }
    Br_addr.sin_port = htons (Port_number);
    Br_addr.sin_family = AF_INET;

    if (Multicast_ip[0] != '\0') {
	if (Multicast_itf[0] != '\0') {	/* select the interface */
	    struct in_addr addr;
	    addr.s_addr = inet_addr (Multicast_itf);
	    if (setsockopt (Soc_UDP, IPPROTO_IP, IP_MULTICAST_IF, 
				(char *)&addr, sizeof (addr)) < 0) {
		fprintf (stderr, 
		    "setsockopt IP_MULTICAST_IF failed (errno = %d) - %s\n", 
						errno, Prog_name);
		close (Soc_UDP);
		return (-1);
	    }
	}
    }
    else {			/* set broadcast mode */
	tmp = 1;
	if (setsockopt (Soc_UDP, SOL_SOCKET, SO_BROADCAST, (char *)&tmp, 
					    sizeof (int)) < 0) {
	    fprintf (stderr, 
			"setsockopt SO_BROADCAST failed (errno = %d) - %s\n", 
					    errno, Prog_name);
	    close (Soc_UDP);
	    return (-1);
	}
    }

    /* open the TCP socket */
    if ((Soc_TCP = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf (stderr, "socket (TCP) failed (errno = %d) - %s\n", 
							errno, Prog_name);
	return (-1);
    }

    loc_soc.sin_port = htons ((unsigned short)Port_number);
    loc_soc.sin_family = AF_INET;
    loc_soc.sin_addr.s_addr = htonl (INADDR_ANY);

    tmp = 1;
    if (setsockopt (Soc_TCP, SOL_SOCKET, SO_REUSEADDR, (char *)&tmp, 
					sizeof (int)) < 0) {
	fprintf (stderr, "setsockopt SO_REUSEADDR failed (errno = %d) - %s\n", 
					errno, Prog_name);
	close (Soc_TCP);
	return (-1);
    }

    /* bind to a local port */
    errno = 0;
    if (bind (Soc_TCP, (struct sockaddr *) &loc_soc, sizeof (loc_soc)) < 0) {
	fprintf (stderr, "bind failed (port = %d, errno = %d) - %s\n", 
					Port_number, errno, Prog_name);
	close (Soc_TCP);
	return (-1);
    }

    if (SOCK_set_TCP_properties (Soc_TCP) < 0)
	return (-1);

    /* listen to remote connection request */
    errno = 0;
    if (listen (Soc_TCP, 5) < 0) {
	fprintf (stderr, "listen failed (port = %d, errno = %d) - %s\n", 
					Port_number, errno, Prog_name);
	close (Soc_TCP);
	return (-1);
    }

    /* accept connections from all running brecvs */
    for (i = 0; i < 5; i++) {
	Accept_connection ();
	msleep (500);
    }

    return (0);
}

/*******************************************************************

    Description: This function prints out the packet statistics.

*******************************************************************/

static void Print_statistics ()
{
    static time_t last_tm = 0;	/* time of the last statistics print */

    if (Cr_time - last_tm > St_print_period) {
	int i;
	int mon, dd, yy, hh, min, ss;

	unix_time (&Cr_time, &yy, &mon, &dd, &hh, &min, &ss);
	printf (
		"%.2d %.2d:%.2d:%.2d pckts sent %u, resent %u, repeat %u, msg lost %u,", 
			dd, hh, min, ss, 
			Send_cnt, Resend_cnt, Last_cnt, Lost_cnt);

	for (i = 0; i < N_clients; i++)	/* print receivers */
	    printf (" %x", (Client[i].address & 0xffff));
	printf ("\n");

	last_tm = Cr_time;
    }
    return;
}

/*******************************************************************

    Description: This function checks non-responding clients and
		remove it if the non-responding time is too long.

*******************************************************************/

static void Check_noresp_time ()
{
    static time_t last_tm = 0;	/* time of the last check */

    if (Cr_time - last_tm > NORESP_CHECK_TIME) {
	int i, max_diff, diff;

	/* check hosts that are not responding */
	max_diff = 0;
	for (i = 0; i < N_clients; i++) {
	    diff = Cr_time - Client[i].time;
	    if (diff > max_diff &&
			SOCK_seq_cmp (LESS_THAN, Client[i].seq, Seq_n))
		max_diff = diff;
	}

	/* print warning message */
	if (max_diff > MAX_QUIET_TIME) {
	    for (i = 0; i < N_clients; i++) {
		if (Cr_time - Client[i].time >= max_diff)
		    fprintf (stderr, "%x (%d) ", Client[i].address, 
					(int)(Cr_time - Client[i].time));
	    }
	    fprintf (stderr, "are not responding (time %d)\n", (int)Cr_time);
	}

	/* remove a client */
	if (max_diff > Noresp_time) {
	    for (i = 0; i < N_clients; i++) {
		if (Cr_time - Client[i].time > Noresp_time && 
			SOCK_seq_cmp (LESS_THAN, Client[i].seq, Seq_n))
		fprintf (stderr, "client at %x is removed\n", 
					Client[i].address & 0xffff);
		Deregister_client (Client[i].fd);
	    }
	}

	last_tm = Cr_time;
    }
    return;
}
/*******************************************************************

    Description: This function interprets command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

*******************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;      /* used by getopt */
    int c;            
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "p:m:s:b:i:w:t:n:c:l:dvh?")) != EOF) {
	switch (c) {

	    case 'p':
		if (sscanf (optarg, "%d", &Port_number) != 1)
		    err = 1;
		break;
	    case 'm':
		strncpy (Multicast_ip, optarg, NAME_SIZE);
		Multicast_ip[NAME_SIZE - 1] = '\0';
		break;
	    case 'l':
		strncpy (Multicast_itf, optarg, NAME_SIZE);
		Multicast_itf[NAME_SIZE - 1] = '\0';
		break;
	    case 'i':
		strncpy (Interface, optarg, NAME_SIZE);
		Interface[NAME_SIZE - 1] = '\0';
		break;
	    case 's':
		if (sscanf (optarg, "%d", &UDPp_size) != 1)
		    err = 1;
		break;
            case 'c':
		if (sscanf (optarg, "%d", &Discard_in_front) != 1)
		    err = 1;
                break;
	    case 'v':
		Msg_mode = 1;
		break;
	    case 'b':
		if (sscanf (optarg, "%d", &Pb_size) != 1)
		    err = 1;
		if (Pb_size <= 0 || Pb_size > MAX_PB_SIZE)
		    err = 1;
		break;
	    case 'w':
		if (sscanf (optarg, "%d", &Wait_time) != 1)
		    err = 1;
		if (Wait_time < 0)
		    err = 1;
		break;
	    case 't':
		if (sscanf (optarg, "%d", &St_print_period) != 1)
		    err = 1;
		if (St_print_period <= 0)
		    err = 1;
		break;
	    case 'n':
		if (sscanf (optarg, "%d", &Noresp_time) != 1)
		    err = 1;
		if (Noresp_time <= 0)
		    err = 1;
		break;
	    case 'd':
		Test_mode = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;
	    default:
		fprintf (stderr, "Unexpected option (-%c) - %s\n", c, Prog_name);
		err = 1;
		break;
	}
    }

    if (optind == argc - 1) {       /* get the input LB name  */
	strncpy (Inp_lb_name, argv[optind], NAME_SIZE);
	Inp_lb_name[NAME_SIZE - 1] = '\0';
    }

    if (err == 0 && strlen (Inp_lb_name) == 0) {
	fprintf (stderr, "Input LB name must be specified - %s\n", Prog_name);
	err = 1;
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s options input_LB_name\n", Prog_name);
	printf ("       options: (%s)\n", VERSION);
	printf ("       -i broadcast network interface name (default: %s)\n", 
							DEFAULT_INTERFACE);
	printf ("       -m multicast_address (default: use broadcast)\n");
	printf ("          suggested address range: 225.0.0.0 - 239.255.255.255\n");
	printf ("       -l local interface address for multicast (default: OS default)\n");
	printf ("          useful if you have multiple network interfaces\n");
	printf ("          e.g. -l 129.15.68.155\n");
	printf ("       -p port number (default: 43333)\n");
	printf ("       -s UDP packet size (default: 1400, system dependent)\n");
	printf ("       -b bcast packet buffer size (default: 64 packets)\n");
	printf ("       -w wait time after each message (MS, default: 0)\n");
	printf ("       -t period of printing statistics (default: 10 seconds)\n");
	printf ("       -n response time (max time (seconds) allowed for \n");
	printf ("          keeping a unresponding client; default: 60)\n");
	printf ("       -c bytes discard (number of bytes discarded at\n");
	printf ("          beginning of each message; default: 0)\n");
	printf ("       -d (debug mode - simulating noisy connection)\n");
	printf ("       -v (print statistics and current receivers)\n");
	return (-1);
    }

    return (0);
}
