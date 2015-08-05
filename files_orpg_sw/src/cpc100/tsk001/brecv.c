/******************************************************************

	File: brecv.c 

******************************************************************/
/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 17:30:27 $
 * $Id: brecv.c,v 1.34 2014/03/18 17:30:27 jeffs Exp $
 * $Revision: 1.34 $
 * $State: Exp $
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "bcast_def.h"
#include <lb.h>
#include <misc.h>

#if (defined (SUNOS3) || defined (LINUX))
#define sigset(a,b) signal(a,b)
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif


#define WAIT_MSECONDS	200	/* basic pause time in milliseconds */
#define CONNECT_POLL_TIME 3	/* max time period the connection to bcast
				   is checked (in seconds) */

#define CONNECT_WAIT	500	/* waiting time (in ms) before another
				   connection (to bcast) retry */
#define N_INIT_READS	50	/* max number of tries reading the first
				   message from bcast (a break of 
				   WAIT_MSECONDS ms is inserted after 
				   each try) */

enum {ACK_AND_REQ, CHECK_CONNECT};
				/* values of argument "func" of 
				   Proc_ack_retrans () */

/* return values of Read_a_packet () */
#define NO_PACKET	-1


char Prog_name[NAME_SIZE];	/* name of this program */
static int Msg_mode;		/* statistics print mode */
static int Test_mode;		/* test mode - random packet discard */
static int St_print_period;	/* time period printing statistics 
				   (in seconds) */

static char Out_lb_name[NAME_SIZE] = "";
				/* name of the output LB */
static int Out_lbd;		/* fd of the output LB */

static int Port_number;		/* socket port number */
static int Soc_UDP;		/* broadcast reading socket fd */
static int Soc_TCP;		/* acknowledge and retransmission socket */

static int UDPp_size;		/* maximum UDP packet size */

static char Bcast_host[NAME_SIZE];
				/* name of the host on which bcast is 
				   running */
static char Stop_host[NAME_SIZE];
				/* name of the host to which bcast service 
				   must be stopped; This used when brecv is
				   used as a tool for sending a terminating 
				   control message to bcast */
static int Bcast_id;		/* id number of the bcast connected to */

static char Multicast_ip[NAME_SIZE];
				/* multicast IP address */


static int Pb_size;		/* packet buffer size (in # of packets) */
static unsigned int Prc_seq;	/* sequence number of the latest processed
				   packet (seq # starts with 1. 
				   0 indicates that no packet has ever 
				   been received) */
static unsigned int Bcast_seq;	/* maxmum sequence number known to be bcasted */

static char *P_buf[MAX_PB_SIZE];
				/* the packet buffer */
static unsigned int *P_seq;	/* sequence numbers of the stored packets */
static char *P_flag;		/* flags of the stored packets */
static int *P_len;		/* lengths of the stored packets */

static int Req_window;		/* window size for determining packets 
				   to be requested */
static int Req_trig;		/* window size for triggering a packet 
				   request/Ack */

static unsigned int Req_cnt;	/* number of retrans. requests sent */
static unsigned int Udp_cnt;	/* number of UDP packets received */
static unsigned int Packet_cnt;	/* number of packets received */
static unsigned int Ack_cnt;	/* number of ACK messages sent */
static unsigned int Discard_cnt;/* number of msgs discarded due to output
				   LB full */
static time_t Cr_time;		/* current UNIX time */

static char *Msg_buffer;	/* buffer for the next user message */
static int Msg_buf_size;	/* size of Msg_buffer */

static int Connect_retry_time;	/* connect to bcaster retry time (seconds) */


/* local functions */
static int Open_sockets ();
static int Read_a_message ();
static void Proc_ack_retrans (int func);
static int Read_next_packet (char **packet, int *ret_flag);
static int Read_a_packet (unsigned int *seq_n, int *flag, char **packet);
static int Read_options (int argc, char **argv);
static void Reallocate_msg_buffer ();
static void Save_packet (char *buf, unsigned int seq, int flag, int len);
static void Print_statistics ();
static int Get_address (char *host, struct sockaddr_in *addr);
static void Send_terminating_message ();



/******************************************************************

    Description: The is the main function for brecv.

******************************************************************/

int main (int argc, char **argv, char **envp)
{

    /* initialize constants and set default values */
    strncpy (Prog_name, argv[0], NAME_SIZE);
    Prog_name[NAME_SIZE - 1] = '\0';
    Port_number = 43333;
    Msg_buf_size = 4096;
    strcpy (Bcast_host, "");
    strcpy (Stop_host, "");
    strcpy (Multicast_ip, "");

    Msg_mode = 0;
    Test_mode = 0;
    Prc_seq = 0;
    Bcast_seq = 0;
    Req_cnt = 0;
    Udp_cnt = 0;
    Packet_cnt = 0;
    Ack_cnt = 0;
    Discard_cnt = 0;
    St_print_period = 10;

    /* get command line options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    /* open the output LB */
    if (Stop_host[0] == '\0') {
	Out_lbd = LB_open (Out_lb_name, LB_WRITE, NULL);
	if (Out_lbd < 0) {
	    fprintf (stderr, "LB_open %s failed (ret = %d) - %s\n",
					Out_lb_name, Out_lbd, Prog_name);
	    exit (1);
	}
    }

    /* open sockets and connect to bcast */
    if (Open_sockets () < 0)
	exit (1);

    /* catch the terminating signals */
    sigset (SIGTERM, MAIN_exit);
    sigset (SIGHUP, MAIN_exit);
    sigset (SIGINT, MAIN_exit);
    sigset (SIGPIPE, SOCK_sigpipe_int); 

    /* initial space for messages */
    Msg_buffer = malloc (Msg_buf_size);
    if (Msg_buffer == NULL) {
	fprintf (stderr, "malloc failed - %s\n", Prog_name);
	MAIN_exit ();
    }

    /* allocate and initialize the packet buffer */
    {
	int i;
	char *buf;

	buf = malloc (UDPp_size * Pb_size);
	P_seq = (unsigned int *)malloc (Pb_size * sizeof (unsigned int));
	P_len = (int *)malloc (Pb_size * sizeof (int));
	P_flag = (char *)malloc (Pb_size * sizeof (char));
	if (buf == NULL || P_seq == NULL || P_len == NULL || P_flag == NULL) {
	    fprintf (stderr, "malloc failed - %s\n", Prog_name);
	    MAIN_exit ();
	}
	for (i = 0; i < Pb_size; i++) {
	    P_buf[i] = buf + (i * UDPp_size);
	    P_seq[i] = 0;
	}
    }
    SOCK_set_max_seq (Pb_size);

    /* set up Ack/request control windows */
    Req_window = (int)((double)Pb_size * REQ_WINDOW_CONST);
    Req_trig = (int)((double)Pb_size * REQ_TRIG_CONST);

    /* the main loop */
    Cr_time = MISC_systime (NULL);
    while (1) {
	int len;
	int ret;

	/* receive a message; retry until success */
	while ((len = Read_a_message ()) <= 0);

	/* write to the LB */
	ret = LB_write (Out_lbd, Msg_buffer, len, LB_ANY);

	if (ret == LB_FULL) {
	    Discard_cnt++;
	    continue;
	}

	if (ret < 0) {
	    fprintf (stderr, "LB_write failed (size = %d, ret = %d) - %s\n",
		    len, ret, Prog_name);
	    MAIN_exit ();
	}
    }
    exit (0);
}

/******************************************************************

    Description: This function closes the output LB and exits.

******************************************************************/

void MAIN_exit ()
{

    if (Msg_mode)
	printf ("%s exits\n", Prog_name);
    LB_close (Out_lbd);
    exit (1);
}

/*******************************************************************

    Description: This function receives a message. 

    Return:	It returns the message length on success or 0 
		otherwise. 

    Notes:	It terminates the program on fatal error conditions.

*******************************************************************/

static int Read_a_message ()
{
    char *new_pac;
    int offset;

    offset = 0;
    if (Prc_seq == 0) {			 /* brecv just started */
	int len, flag;
	unsigned int seq;

	while (1) {			/* try until a first packet is read */
	    len = Read_a_packet (&seq, &flag, &new_pac);
	    if (len <= 0) {
		Cr_time = MISC_systime (NULL);
		Proc_ack_retrans (CHECK_CONNECT);
		msleep (WAIT_MSECONDS);
		Print_statistics ();
	    }
	    else if (flag == 0 || flag == 1)
		    break;
	    if (len >= 0)
		Prc_seq = seq;		/* Prc_seq must be updated here because
					   it will be used in Read_a_packet */
	}
	Packet_cnt++;
	while (Msg_buf_size < len + offset)
	    Reallocate_msg_buffer ();
	memcpy (Msg_buffer + offset, new_pac, len);
	Prc_seq = seq;
	offset += len;
	if (Msg_mode)
	    printf ("starting seq number = %u - %s\n", Prc_seq, Prog_name);

	if (flag == 0) {
	    return (offset);
	}
    }

    while (1) {
	char *packet;
	int flag;
	int plen;

	/* read next packet */
	plen = Read_next_packet (&packet, &flag);
	Packet_cnt++;

	while (Msg_buf_size < offset + plen)
	    Reallocate_msg_buffer ();
	memcpy (Msg_buffer + offset, packet, plen);
	offset += plen;

	if (flag == 0 || flag == 3) {
	    return (offset);
	}
    }
    return(0); /* should never happen, here for cireport */
}

/*******************************************************************

    Description: This function reallocates the user message buffer.
		It doubles the buffer size.

    Notes:	On fatal error, this function terminates the program.

*******************************************************************/

static void Reallocate_msg_buffer ()
{
    char *cpt;

    cpt = malloc (Msg_buf_size * 2);
    if (cpt == NULL) {
	fprintf (stderr, "malloc failed - %s\n", Prog_name);
	MAIN_exit ();
    }
    memcpy (cpt, Msg_buffer, Msg_buf_size);
    free (Msg_buffer);
    Msg_buffer = cpt;
    Msg_buf_size *= 2;

    return;
}

/*******************************************************************

    Description: This function reads the next packet in terms of 
		the packet sequence number.

    Outputs:	packet - the next packet;
		ret_flag - flag of the next packet;

    Return:	It returns length (excluding the trailer) of the
		next packet on success.

    Notes:	It terminates the program on fatal error conditions.

*******************************************************************/


static int Read_next_packet (char **packet, int *ret_flag)
{
    static int p_cnt = 0;
    unsigned int next_seq;

    next_seq = SOCK_seq_add (Prc_seq, 1);

    while (1) {				/* try to read a new packet */
	char *new_pac;
	int len;
	int loc;
	int flag;
	unsigned int seq;

	p_cnt++;
	if (Msg_mode && (p_cnt % (1000 / WAIT_MSECONDS)) == 0)
					/* print statistics */
	    Print_statistics ();

	/* check whether the packet is in the buffer */
	loc = next_seq % Pb_size;
	if (next_seq == P_seq[loc]) {	/* packet is in the buffer */
	    *ret_flag = P_flag[loc];
	    *packet = P_buf[loc];
	    Prc_seq = next_seq;
	    return (P_len[loc]);
	}
	if (SOCK_seq_cmp (LESS_THAN, next_seq, P_seq[loc])) {
				/* the slot is used by a new packet */
	    fprintf (stderr, "Unexpected next_seq error - %s\n", Prog_name);
	    MAIN_exit ();
	}

	/* read a packet from the UDP socket */
	len = Read_a_packet (&seq, &flag, &new_pac);

	if (len == NO_PACKET) {		/* no packet available */
	    Cr_time = MISC_systime (NULL);
	    Proc_ack_retrans (CHECK_CONNECT); 
	    msleep (WAIT_MSECONDS);
	    continue;
	}

	if (SOCK_seq_cmp (LESS_THAN, Bcast_seq, seq))
					/* new bcasted sequence number */
	    Bcast_seq = seq;

	if (len == 0) {			/* repeated packet is read */
	    Proc_ack_retrans (ACK_AND_REQ);
	    continue;
	}

	if (seq == next_seq) {		/* the next packet is read */
	    *ret_flag = flag;
	    *packet = new_pac;
	    Prc_seq = next_seq;
	    P_seq[seq % Pb_size] = 0;	/* remove the packet in buffer */
	    return (len);
	}

	if (SOCK_seq_cmp (LESS_THAN, seq, next_seq))
					/* the new packet is old */
	    continue;

	/* save the packet in the buffer */
	Save_packet (new_pac, seq, flag, len);

	/* process ack/retrans */
	if (SOCK_seq_cmp (GREATER_THAN, seq, SOCK_seq_add (Prc_seq, Req_trig)))
	    Proc_ack_retrans (ACK_AND_REQ);
    }
    return(0); /* should never happen.  Here for cireport */
}

/*******************************************************************

    Description: This function sends an 
		acknowledgement/retransmission request, waits and
		receives all retransmitted packets. It also checks 
		the connection to bcast.

    Inputs:	func - functional switch: ACK_AND_REQ or CHECK_CONNECT.

    Notes:	On fatal error, this function terminates the program.

*******************************************************************/

static void Proc_ack_retrans (int func)
{
    static unsigned int echo_seq = 0;
				/* maxmum sequence number ever 
				   acknowledged */
    static time_t pre_poll = 0;
    int n_req, i;
    unsigned int req[MAX_NREQUEST + ACK_REQ_HD];
    unsigned int nreq[MAX_NREQUEST + ACK_REQ_HD];

    if (func == CHECK_CONNECT && Cr_time - pre_poll < CONNECT_POLL_TIME)
	return;
    pre_poll = Cr_time;

    n_req = 0;
    if (func == ACK_AND_REQ) {
	for (i = 1; i < Req_window; i++) {
	    int ind, seqi;

	    seqi = SOCK_seq_add (Prc_seq, i);
	    if (SOCK_seq_cmp (GREATER_THAN, seqi, Bcast_seq))
		break;

	    ind = seqi % Pb_size;
	    if (P_seq[ind] != seqi && n_req < MAX_NREQUEST) {
		req[n_req + ACK_REQ_HD] = seqi;
		n_req++;
		Req_cnt++;
	    }
	}
    }

    if (func == ACK_AND_REQ && n_req == 0 && 
		SOCK_seq_cmp (GREATER_EQUAL, echo_seq, Prc_seq))
	return;

    if (Msg_mode && n_req > 0) {	/* print lost msg seq numbers */
	fprintf (stderr, "lost packets: ");
	for (i = 0; i < n_req; i++)
	    fprintf (stderr, "%u ", req[i + ACK_REQ_HD]);
	fprintf (stderr, "\n");
    }

    req[0] = (n_req + ACK_REQ_HD) * sizeof (int);
    req[1] = Prc_seq;
    echo_seq = Prc_seq;

    for (i = 0; i < n_req + ACK_REQ_HD; i++) 	/* network format int */
	nreq[i] = htonl (req[i]);

    if (SOCK_write_tcp (Soc_TCP, req[0], (char *)nreq) < 0) {
	if (Msg_mode)
	    fprintf (stderr, "connection to bcast terminated - %s\n", Prog_name);
	MAIN_exit ();
    }
    Ack_cnt++;

    /* read retransmitted packets */
    for (i = 0; i < n_req; i++) {
	unsigned int seq;
	int flag, len;
	unsigned char *cpt;
	int k;

	cpt = (unsigned char *)P_buf[req[i + ACK_REQ_HD] % Pb_size];
					/* we directly read into the packet 
					   buffer */

	for (k = 0; k < 2; k++) {	/* two-step read is necessary */
	    char *tpt;
	    int nlen, ret;

	    if (k == 0) {		/* read length */
		tpt = (char *)&nlen;
		len = sizeof (int);
	    }
	    else {			/* read packet */
		tpt = (char *)cpt;
		len = ntohl (nlen) - sizeof (int);
		if (len <= 0 || len > UDPp_size) {
		    fprintf (stderr, "bad retrans packet - %s\n", Prog_name);
		    MAIN_exit ();
		}
	    }
	    ret = SOCK_read_tcp (Soc_TCP, len, tpt);
	    if (ret < 0) {
		if (Msg_mode)
		    fprintf (stderr, "connection to bcast terminated - %s\n", 
								Prog_name);
		MAIN_exit ();
	    }
	    if (ret == 0) {		/* packet yet to come */
		int cnt;

		/* read UDP packets */
		cnt = 0;
		while (1) {
		    char *new_pac;
		    unsigned int new_seq;
		    int new_flag, new_len;

		    new_len = Read_a_packet (&new_seq, &new_flag, &new_pac);
		    if (new_len <= 0 || 
			SOCK_seq_cmp (GREATER_EQUAL, 
				new_seq, SOCK_seq_add (Prc_seq, Pb_size)))
			break;
		    Save_packet (new_pac, new_seq, new_flag, new_len);
		    cnt++;
		}

		if (cnt == 0)
		    msleep (WAIT_MSECONDS);
		k--;
		continue;
	    }
	}

	/* a packet is read; get sequence number and flag */
	cpt = cpt + len - TRAILER_SIZE;
	seq = (cpt[0] << 24) + (cpt[1] << 16) + (cpt[2] << 8) + cpt[3];
	flag = cpt[4];
	if (seq != req[i + ACK_REQ_HD]) {
	    fprintf (stderr, "bad retrans packet (seq %u; req[%d] = %u)- %s\n", 
				seq, i, req[i + ACK_REQ_HD], Prog_name);
	    MAIN_exit ();
	}

	Save_packet (NULL, seq, flag, len - TRAILER_SIZE);
    }

    return;
}

/*******************************************************************

    Description: This function saves a packet in the packet buffer.
		If "buf" = NULL, the data is not actually saved except
		that the seq and flag fields are updated.

    Inputs:	buf - the packet;
		seq - the packet sequence number;
		flag - the packet flag;
		len - the packet length;

*******************************************************************/

static void Save_packet (char *buf, unsigned int seq, int flag, int len)
{
    int ind;

    ind = seq % Pb_size;
    if (P_seq[ind] != seq) {
	P_seq[ind] = seq;
	P_flag[ind] = flag;
	P_len[ind] = len;
	if (buf != NULL)
	    memcpy (P_buf[ind], buf, len);
    }

    return;
}

/*******************************************************************

    Description: This function reads a UDP packet and returns it 
		with its sequence number and packet flag.

    Outputs:	seq_n - pointer returning the sequence number;
		flag - pointer returning the packet flag.
		packet - pointer returning the new packet.

    Return:	The function returns the data length on success, 
		NO_PACKET if there is no packet, 0 if the packet 
		does not contain data (repeated msg).

    Notes:	On fatal error conditions, the function terminates 
		the program.

*******************************************************************/

static int Read_a_packet (unsigned int *seq_n, int *flag, char **packet)
{
    static char *pac_buf = NULL;
				/* buffer for the new packet */
    static unsigned int seq_in_buf = 0;
				/* sequence number of the packet in 
				   pac_buf */
    static int flag_in_buf;	/* flag of the packet in pac_buf */
    static int len_in_buf;	/* length of the packet in pac_buf */

    if (pac_buf == NULL &&	/* allocate buffer space */
	(pac_buf = malloc (UDPp_size)) == NULL) {
	fprintf (stderr, "malloc failed - %s\n", Prog_name);
	MAIN_exit ();
    }

    *packet = pac_buf;

    if (SOCK_seq_cmp (GREATER_EQUAL, 
			seq_in_buf, SOCK_seq_add (Prc_seq, Pb_size))) {
				/* buffer full; no more packet can be 
				   read - we return the same packet */
	*seq_n = seq_in_buf;
	*flag = flag_in_buf;
	return (len_in_buf);
    }

    /* read a packet */
    while (1) {
	socklen_t alen;
	int length;
	unsigned char *cpt;
	struct sockaddr from;

	alen = sizeof (from);
	length = recvfrom (Soc_UDP, pac_buf, UDPp_size, 0,
			   (struct sockaddr *) &from, &alen);

	if (length < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return (NO_PACKET);
	    fprintf (stderr, "recvfrom error (errno = %d) - %s\n", 
							errno, Prog_name);
	    MAIN_exit ();
	}
	if (length >= TRAILER_SIZE) {
	    int id;

	    /* get sequence number and flag */
	    cpt = (unsigned char *) (pac_buf + length - TRAILER_SIZE);
	    *seq_n = (cpt[0] << 24) + (cpt[1] << 16) + (cpt[2] << 8) + cpt[3];
	    *flag = cpt[4];
	    id = (cpt[5] << 8) + cpt[6];
	    if (id != Bcast_id) {	/* packet from another bcast */
		fprintf (stderr, "Bad packet id (id = %d) - %s\n", 
							id, Prog_name);
		continue;
	    }
	    if (Test_mode) {
		int tm;
		static int last_tm = 0;
		static int drop_cnt = 0;

		if (drop_cnt > 0) {
		    drop_cnt--;
		    return (NO_PACKET);
		}
		if ((rand () % 20) == 0) {
		    tm = MISC_systime (NULL);
		    if (tm - last_tm > 61) {
			last_tm = tm;
			drop_cnt = 80;
		    }
		    return (NO_PACKET);
		}
	    }
 	    Udp_cnt++;
	    seq_in_buf = *seq_n;
	    flag_in_buf = *flag;
	    len_in_buf = length - TRAILER_SIZE;
	    return (len_in_buf);
	}

	fprintf (stderr, "Bad packet length (length = %d) - %s\n", 
							length, Prog_name);
    }
    return(0); /* should never happen, here for cireport */
}

/*******************************************************************

    Description: This function opens a UDP socket for receiving 
		broadcasted packets and a TCP socket for 
		acknowledgement and packet retransmission. The 
		first message is read and interpreted after the TCP 
		connection is made.

    Return:	It returns 0 on success or -1 on error conditions.

*******************************************************************/

static int Open_sockets ()
{
    int tmp;
    struct sockaddr_in recv_addr;
    struct sockaddr_in rem_soc;	/* remote host address */
    int msg[FIRST_MSG_SIZE];
    int len, i, ret = 0;
    time_t st_t;

    /* open TCP socket for acknowledgment and packet retransmission */
    if (Get_address (Bcast_host, &rem_soc) < 0)	/* get address structure */
	return (-1);

    rem_soc.sin_port = htons ((unsigned short)Port_number); 
						/* fill in port number */

    /* Open the local socket and connect it to the remote server */
    st_t = MISC_systime (NULL);
    while (1) {

	if ((Soc_TCP = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
	    fprintf (stderr, "socket SOCK_STREAM failed (errno = %d) - %s\n", 
						errno, Prog_name);
	    return (-1);
	}

	if (connect (Soc_TCP, (struct sockaddr *) &rem_soc,
		 			sizeof (rem_soc)) < 0) {
	    if (errno != ECONNREFUSED && errno != EINTR) {
		fprintf (stderr, "connect failed (errno = %d) - %s\n", 
						errno, Prog_name);
		return (-1);
	    }
	    close (Soc_TCP);
	    if (Connect_retry_time > 0 && 
				MISC_systime (NULL) > st_t + Connect_retry_time)
		return (-1);
	    msleep (CONNECT_WAIT);
	}
	else
	    break;
    }

    if (SOCK_set_TCP_properties (Soc_TCP) < 0) {
	close (Soc_TCP);
	return (-1);
    }

    /* read in packet buffer size, packet size and the bcast id */
    for (i = 0; i < N_INIT_READS; i++) {
	len = SOCK_read_tcp (Soc_TCP, FIRST_MSG_SIZE * sizeof (int), 
						(char *)msg);
	if (len < 0) {
	    if (Msg_mode)
		fprintf (stderr, "connection to bcast terminated - %s\n", 
								Prog_name);
	    MAIN_exit ();
	}
	else if (len != 0)
	    break;
	msleep (WAIT_MSECONDS);
    }
    if (i >= N_INIT_READS || len != FIRST_MSG_SIZE * sizeof (int) || 
					ntohl (msg[3]) != BCAST_ID) {
	fprintf (stderr, "connection to a bad bcast - %s\n", Prog_name);
	close (Soc_TCP);
	return (-1);
    }
    Pb_size = ntohl (msg[0]);
    UDPp_size = ntohl (msg[1]);
    Bcast_id = ntohl (msg[2]);

    if (Msg_mode)
	printf ("Connected to bcast; Packet buf size = %d, UDP packet size = %d; - %s\n", 
					Pb_size, UDPp_size, Prog_name);

    /* brecv is used as a tool for sending a terminating control message to 
       bcast. This call will not return. */
    if (Stop_host[0] != '\0')
	Send_terminating_message ();

    /* open UDP broadcast port */
    if ((Soc_UDP = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {/* open a socket */
	fprintf (stderr, "socket failed - %s\n", Prog_name);
	return (-1);
    }

    memset ((char *) &recv_addr, 0, sizeof (recv_addr));
    recv_addr.sin_port = htons (Port_number);
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);

    /* bind to a local port */
    if (bind (Soc_UDP,
	      (struct sockaddr *) &recv_addr, sizeof (recv_addr)) < 0) {
	fprintf (stderr, "bind failed (errno = %d) - %s\n", errno, Prog_name);
	close (Soc_UDP);
	return (-1);
    }

    /* set broadcast mode */
    if (Multicast_ip[0] == '\0') {	/* broadcast */
	tmp = 1;
	if (setsockopt (Soc_UDP, SOL_SOCKET, SO_BROADCAST, 
					(char *)&tmp, sizeof(int)) < 0) {
	    fprintf (stderr, 
		"setsockopt SO_BROADCAST failed (errno = %d) - %s\n", 
						    errno, Prog_name);
	    close( Soc_UDP);
	    return (-1);
	}
    }
    else {
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr (Multicast_ip);	/* multicast */
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt (Soc_UDP, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					(char *)&mreq, sizeof(mreq)) < 0) {
	    fprintf (stderr, 
		"setsockopt IP_ADD_MEMBERSHIP failed (errno = %d) - %s\n", 
						    errno, Prog_name);
	    close( Soc_UDP);
	    return (-1);
	}
    }		

    /* set non-block mode */
#ifdef HPUX
    if (fcntl (Soc_UDP, F_SETFL, O_NONBLOCK) < 0) {
#else
    if (fcntl (Soc_UDP, F_SETFL, O_NDELAY) < 0) {
#endif 
	close (Soc_UDP);
	fprintf (stderr, "fcntl O_NDELAY failed (errno = %d) - %s\n", 
							errno, Prog_name);
	return (-1);
    }

    /* set UDP buffer size */
    tmp = UDPp_size * Pb_size;		/* using full buffer size eliminates
					   most of the packet drops */
    if (tmp > 12800)			/* we put some limit to avoid using 
	tmp = 12800;			   too much system resources */
    while (tmp >= 4096 && (ret = setsockopt (Soc_UDP, SOL_SOCKET, SO_RCVBUF, 
				(char *)&tmp, sizeof(int))) < 0)
	tmp = tmp * 4 / 5;		/* reduce the size until success */
    if (ret < 0) {
        fprintf (stderr, "setsockopt SO_RCVBUF failed (errno = %d, size = %d) - %s\n",
						errno, tmp, Prog_name);
        close( Soc_UDP);
	return (-1);
    } 

    return (0);
}

/*******************************************************************

    Description: This function gets the internet address of "host".

    Inputs:	host - host name or INTERNET address;

    Outputs:	addr - socket address;

    Return:	0 on success or -1 on failure.

*******************************************************************/

static int Get_address (char *host, struct sockaddr_in *addr)
{
    struct hostent *hostport;		/* host port info */

    memset ((char *) addr, 0, sizeof (struct sockaddr_in));
    addr->sin_family = AF_INET;

    if ( (addr->sin_addr.s_addr = inet_addr (host)) == INADDR_NONE) {
        hostport = gethostbyname (host);
        if (!hostport) {
	    fprintf (stderr, "gethostbyname failed (host: %s) - %s\n", 
						Bcast_host, Prog_name);
	    return (-1);
	}

        /* copy the internet address to the address struct */
        memcpy ((char *) &(addr->sin_addr), (char *) hostport->h_addr,
	        hostport->h_length);
    }

    return (0);
}

/*******************************************************************

    Description: This function prints out the packet statistics.

*******************************************************************/

static void Print_statistics ()
{
    static time_t last_tm = 0;	/* time of the last statistics print */

    Cr_time = MISC_systime (NULL);

    if (Msg_mode && Cr_time - last_tm > St_print_period) {
	int mon, dd, yy, hh, min, ss;

	unix_time (&Cr_time, &yy, &mon, &dd, &hh, &min, &ss);
	printf ("%.2d %.2d:%.2d:%.2d pckt recvd %u, retran_req %u, ACK sent %u, msg discarded %u\n", 
			dd, hh, min, ss, Packet_cnt, Req_cnt, 
			Ack_cnt, Discard_cnt);
	last_tm = Cr_time;
    }
    return;
}

/*******************************************************************

    Description: This function sends a terminating control message
		to bcast and then exits.

*******************************************************************/

static void Send_terminating_message ()
{
    struct sockaddr_in addr;	/* address of the host to which bcast
				   service to be stopped */
    int msg[CONTROL_MSG_SIZE];
    char ack[sizeof (CONTROL_ACK) + 1];
    int len;

    /* get address structure of Stop_host */
    if (Get_address (Stop_host, &addr) < 0)
	exit (1);

    msg[1] = htonl (CONTROL_MSG);
    msg[2] = htonl (TERMINATE_HOST);
    msg[3] = htonl (addr.sin_addr.s_addr);
    len = CONTROL_MSG_SIZE * sizeof (int);
    msg[0] = htonl (len);

    if (SOCK_write_tcp (Soc_TCP, len, (char *)msg) < 0) {
	fprintf (stderr, "failed in sending control msg to bcast - %s\n", 
								Prog_name);
	exit (1);
    }

    /* read acknowledgement */
    ack[0] = '\0';
    while (SOCK_read_tcp (Soc_TCP, sizeof (CONTROL_ACK) + 1, ack) == 0);
    if (strncmp (ack, CONTROL_ACK, strlen (CONTROL_ACK)) == 0) {
	if (Msg_mode)
	    printf ("service termination (on %x) request sent to bcast - %s\n", 
			(unsigned int)addr.sin_addr.s_addr, Prog_name);
	exit (0);
    }
    else {
	if (Msg_mode)
	    printf ("failed in terminating service (on %x) - %s\n", 
			(unsigned int)addr.sin_addr.s_addr, Prog_name);
	exit (1);
    }
}

/**************************************************************************

    Description: This function interprets command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;	/* used by getopt */
    extern int optind;		/* used by getopt */
    int c;
    int err;			/* error flag */

    err = 0;
    Connect_retry_time = 0;
    while ((c = getopt (argc, argv, "p:b:s:t:m:n:vhd?")) != EOF) {
	switch (c) {

	    case 'p':
		if (sscanf (optarg, "%d", &Port_number) != 1)
		    err = 1;
		break;
	    case 'b':
		strncpy (Bcast_host, optarg, NAME_SIZE);
		Bcast_host[NAME_SIZE - 1] = '\0';
		break;
	    case 'm':
		strncpy (Multicast_ip, optarg, NAME_SIZE);
		Multicast_ip[NAME_SIZE - 1] = '\0';
		break;
	    case 's':
		strncpy (Stop_host, optarg, NAME_SIZE);
		Stop_host[NAME_SIZE - 1] = '\0';
		break;
	    case 'n':
		if (sscanf (optarg, "%d", &Connect_retry_time) != 1)
		    err = 1;
		break;
	    case 'v':
		Msg_mode = 1;
		break;
	    case 'd':
		Test_mode = 1;
		break;
	    case 't':
		if (sscanf (optarg, "%d", &St_print_period) != 1)
		    err = 1;
		if (St_print_period <= 0)
		    err = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;
	    default:
		fprintf (stderr, "Unexpected option (%c) - %s\n", c, Prog_name);
		err = 1;
		break;
	}
    }

    if (optind == argc - 1) {		/* get the LB file name  */
	strncpy (Out_lb_name, argv[optind], NAME_SIZE);
	Out_lb_name[NAME_SIZE - 1] = '\0';
    }

    if (err == 0 && strlen (Stop_host) == 0 && strlen (Out_lb_name) == 0) {
	fprintf (stderr, "Output LB name must be specified - %s\n", Prog_name);
	err = 1;
    }

    if (err == 0 && strlen (Bcast_host) == 0) {
	fprintf (stderr, "bcast host name must be specified - %s\n", Prog_name);
	err = 1;
    }

    if (err == 1) {			/* Print usage message */
	printf ("Usage: %s options -b Bcast_host output_LB_name\n", Prog_name);
	printf ("       options: (%s)\n", VERSION);
	printf ("       -p port number (default: 43333)\n");
	printf ("       -m multicast address (default: using broadcast)\n");
	printf ("       -s Stop_host (name of the host to which bcast\n");
	printf ("          service must be stopped; This option turns brecv\n");
	printf ("          into a tool that sends a terminating control\n");
	printf ("          message to bcast. After sending the message,\n");
	printf ("          brecv will exit.)\n");
	printf ("       -n connect retry time (in seconds.\n");
	printf ("          default: retry forever)\n");
	printf ("       -v (print statistics)\n");
	printf ("       -t period of printing statistics (default: 10 seconds)\n");
	printf ("       -d (debug mode - simulating noisy connection)\n");
	return (-1);
    }

    return (0);
}
