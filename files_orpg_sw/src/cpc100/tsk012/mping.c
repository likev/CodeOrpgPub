
/******************************************************************

    This is the main module for mping - a tool that pings multiple 
    destinations.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/03/05 21:47:44 $
 * $Id: mping.c,v 1.5 2009/03/05 21:47:44 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <malloc.h>
#include <time.h>
#include <arpa/inet.h>
 
#ifdef linux
/*    #include <asm/processor.h>*/
    #include <asm/types.h>
    #include <linux/icmp.h>
    #define icmp icmphdr
    #define ip iphdr

    #define icmp_type type
    #define icmp_code code
    #define icmp_cksum checksum
    #define icmp_id un.echo.id
    #define icmp_seq un.echo.sequence
    #define ip_hl ihl
#else
    #include <netinet/ip_icmp.h>
#endif

typedef struct {
    char *host_name;
    int disabled;
    unsigned int stime;		/* send time of the latest ICMP */
    unsigned int rtime;		/* time of received ICMP with the 
				   latest send time */
    int addr;			/* remote host address (network byte order) */
    struct sockaddr_in dest;	/* remote host address */
} Ping_dest_t;

#define MAX_PING_DESTS 128

static Ping_dest_t Dests[MAX_PING_DESTS];
static int N_dests = 0;

static int Verbose;				/* verbose mode */
static int Test_period;

#define DATALEN		16	/* length of data area after ICMP hd */
#define SIZE_ICMP_HDR	8	/* 8-byte ICMP header */
#define SIZE_TIME_DATA	4	/* first 4 bytes in data area are used for
				   sending time */
#define MAX_IP_HDR	32	/* maximum IP header size */
#define PACKSIZE (DATALEN + SIZE_ICMP_HDR)
				/* ICMP packet size */

static int Pid;			/* pid of this process */
static int Icmp_sockfd = -1;	/* ICMP socket fd; -1 indicates that ICMP
				   is not available */
static unsigned int Cr_time = 0;/* Current time in number of checks */

static unsigned char Icmp_buf [PACKSIZE + MAX_IP_HDR];
				/* buffer for sending and receiving ICMP 
				   packets */

/* local functions */
static int Read_options (int argc, char **argv);
static void Read_input ();
static char *Get_next_token (char *str, int str_len, int *tlen);
static void Add_dest (char *host, int len);
static void Remove_dest (char *dest, int len);
static int Compute_checksum (unsigned char *ptr, int nbytes);
static void Send_icmp (time_t time, struct sockaddr_in *dest);
static void Read_icmp_packets ();
static int Initialize ();
static void CMP_check_connect ();
static void Output ();


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    int fd;

    if (Read_options (argc, argv) < 0)
	exit (1);

    fd = fileno (stdin);
    if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) < 0) {
	fprintf (stderr, "fcntl stdin failed (errno %d)\n", errno);
	exit (1);
    }

    if (Initialize () < 0)
	exit (1);

    while (1) {
	Read_input ();
	CMP_check_connect ();
	Output ();
	if ((Cr_time % 2) == 0)
	    usleep (Test_period * 1000000 - 100000);
	else
	    usleep (Test_period * 1000000 + 100000);
	Cr_time++;
    }
}

/******************************************************************

    Writes the connectivity info to the stdout.

******************************************************************/

static void Output () {
    int i;

    for (i = 0; i < N_dests; i++) {
	int qtime;
	if (Dests[i].disabled)
	    continue;
	qtime = Dests[i].stime - Dests[i].rtime - 1;
	if (qtime < 0)
	    qtime = 0;
	printf ("%s--%d ", Dests[i].host_name, qtime * Test_period);
    }
    printf ("\n");
    fflush (stdout);
}

/******************************************************************

    Reads standard input to get destination names. The input is read
    from stdin. The format is "++add1 ++add2 --add1 " where ++ for
    adding, and -- for removing, an address for monitoring. Note that
    each address, e.g. the last one, must be separated by a space, 
    a tab or a line return.

******************************************************************/

static void Read_input () {
    static char buf[512];
    static int n_chars = 0;
    int ret, tlen;
    char *p;

    while (n_chars < 512 && (ret = fgetc (stdin)) != EOF) {
	buf[n_chars] = ret;
	n_chars++;
    }
    if (n_chars >= 512) {
	fprintf (stderr, "Bad input - discarded\n");
	n_chars = 0;
	return;
    }
    while (1) {
	p = Get_next_token (buf, n_chars, &tlen);
	if (p == NULL)
	    return;
	if (tlen > 2 && strncmp (p, "++", 2) == 0)
	    Add_dest (p + 2, tlen - 2);
	else if (tlen > 2 && strncmp (p, "--", 2) == 0)
	    Remove_dest (p + 2, tlen - 2);
	memmove (buf, p + tlen, n_chars - (p - buf + tlen));
	n_chars -= p - buf + tlen;
    }
}

/******************************************************************

    Adds a new destination "dest" of length "len" to the ping list.

******************************************************************/

static void Add_dest (char *host, int len) {
    int addr, i;
    char host_name[256];

    if (len >= 256)
	len = 255;
    memcpy (host_name, host, len);
    host_name[len] = '\0';

    if ((int)(addr = inet_addr (host_name)) == -1) {
	struct hostent *hostinfo = NULL;		/* host info */
	hostinfo = gethostbyname (host_name);
	if (hostinfo == NULL) {
	    fprintf (stderr, 
		"gethostbyname %s failed (errno %d)\n", host_name, errno);
	    return;
	}
	if (hostinfo->h_length != sizeof (int)) {
	    fprintf (stderr, 
			"Address is not 4 bytes (%d)\n", hostinfo->h_length);
	    return;
	}
        memcpy ((char *)&addr, (char *)hostinfo->h_addr_list[0], sizeof (int));
    }

    for (i = 0; i < N_dests; i++) {	/* already in the list */
	if (Dests[i].addr == addr) {
	    if (Dests[i].disabled) {
		Dests[i].stime = Cr_time;
		Dests[i].rtime = Cr_time;
	    }
	    Dests[i].disabled = 0;
	    return;
	}
    }
    if (N_dests >= MAX_PING_DESTS) {
	fprintf (stderr, "Too many destinations - %s not added\n", host_name);
	return;
    }

    Dests[i].host_name = malloc (len + 1);
    if (Dests[i].host_name == NULL) {
	fprintf (stderr, "malloc failed - %s not added\n", host_name);
	return;
    }
    strcpy (Dests[i].host_name, host_name);
    Dests[i].disabled = 0;
    memset ((char *)&(Dests[i].dest), 0, sizeof (struct sockaddr_in));
    Dests[i].addr = addr;
    Dests[i].stime = Cr_time;
    Dests[i].rtime = Cr_time;
    Dests[i].dest.sin_family = AF_INET;
    Dests[i].dest.sin_addr.s_addr = addr;
    N_dests++;
}

/******************************************************************

    Removess destination "dest" of length "len" from the ping list.

******************************************************************/

static void Remove_dest (char *dest, int len) {
    int i;
    char c;

    c = dest[len];
    dest[len] = '\0';
    for (i = 0; i < N_dests; i++) {
	if (strcmp (Dests[i].host_name, dest) == 0) {
	    Dests[i].disabled = 1;
	    dest[len] = c;
	    return;
	}
    }
}

/******************************************************************

    Returns the pointer to the first token in "str" of length 
    "str_len". On success, the length of the token is returned with
    "tlen". Returns NULL on failure.

******************************************************************/

static char *Get_next_token (char *str, int str_len, int *tlen) {
    char *p, *tok;

    p = str;
    while (p - str < str_len && 
		(*p == ' ' || *p == '\t' || *p == '\n'))
	p++;
    if (p - str >= str_len)
	return (NULL);
    tok = p;
    while (p - str < str_len && 
		(*p != ' ' && *p != '\t' && *p != '\n'))
	p++;
    if (p - str >= str_len)
	return (NULL);
    *tlen = p - tok;
    return (tok);
}

/******************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

******************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    Test_period = 5;
    Verbose = 0;
    err = 0;
    while ((c = getopt (argc, argv, "i:o:p:d:vhtab?")) != EOF) {
	switch (c) {

	    case 'p':
		if (sscanf (optarg, "%d", &Test_period) != 1 ||
		    Test_period <= 0)
		    err = 1;
		break;

	    case 'v':
		Verbose = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;
	}
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s [options]\n", argv[0]);
	printf ("       pings multiple destinations\n");
	printf ("       options:\n");
	printf ("       -p test_period (in seconds; default: 5);\n");
	printf ("       -v (verbose mode);\n");
	printf ("       -h (print usage info);\n");

	return (-1);
    }

    return (0);
}

/*****************************************************************************

    Opens the ICMP socket.

*****************************************************************************/

static int Initialize () {
    int fd;
    int uid;
    struct protoent *proto;

    Pid = getpid() & 0xffff;
    uid = getuid();

    /* set root */
    if (seteuid ((uid_t)0) < 0){
	fprintf (stderr, "setuid (to 0) failed\n");
	return (-1);
    }

    /* open a ICMP socket */
    if ((proto = getprotobyname ("icmp")) == NULL) {
	fprintf (stderr, "getprotobyname failed\n");
	return (-1);
    }
    if ((fd = socket (AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
	fprintf (stderr, "open socket (raw) failed (errno %d)\n", errno);
	return (-1);
    }

    /* set non-block IO */
    if (fcntl (fd, F_SETFL, O_NONBLOCK) < 0) {
	close (fd);
	fprintf (stderr, "fcntl O_NDELAY (raw) failed (errno %d)\n", errno);
	return (-1);
    }

    /* set uid to the user */
    if (setuid ((uid_t)uid) < 0){
	fprintf (stderr, "setuid (to user) failed\n");
	return (-1);
    }
    Icmp_sockfd = fd;

    return (0);
}

/*****************************************************************************

    Description: This function is called frequently by the application.

    Inputs:	n_rhosts - number of remote hosts;
		rhosts - list of remote hosts;

*****************************************************************************/

static void CMP_check_connect () {
    int i;

    /* receive all ICMP packet in the socket and update rhosts */
    Read_icmp_packets ();

    /* send an ICMP packet to each client hosts */
    for (i = 0; i < N_dests; i++) {
	if (Dests[i].disabled)
	    continue;
	Send_icmp (Cr_time, &(Dests[i].dest));
	Dests[i].stime = Cr_time;
    }
    return;
}

/*****************************************************************************

    Sends an ICMP packet stamped with "time" to "dest".

*****************************************************************************/

static void Send_icmp (time_t time, struct sockaddr_in *dest) {
    static int ntransmitted = 0;	/* sender sequence number */
    int i;
    struct icmp *icp;			/* ICMP header */
    unsigned char *uptr;		/* start of user data */

    /* fill in the ICMP header */
    icp = (struct icmp *) Icmp_buf;
    icp->icmp_type = ICMP_ECHO;		/* ICMP packet type */
    icp->icmp_code = 0;			/*  */
    icp->icmp_cksum = 0;		/* check sum; set later */
    icp->icmp_id = Pid;			/* our pid to identify on return */
    icp->icmp_seq = ntransmitted++;

    /* put time in the packet after the ICMP header */
    *((int *)&(Icmp_buf [SIZE_ICMP_HDR])) = time;

    /* fill in the remainder of the packet with the user data. we set each 
       byte of udata[i] to i (although this is not verified when the echoed 
       packet is receiver back) */
    uptr = &Icmp_buf [SIZE_ICMP_HDR + SIZE_TIME_DATA];
    for (i = SIZE_TIME_DATA; i < DATALEN; i++)
	*uptr++ = i;

    /* compute and store the ICMP checksum (include the ICMP header);
       This field must be set correctly to get an echo from the remote host. */
    icp->icmp_cksum = Compute_checksum (Icmp_buf, PACKSIZE);

    /* send the datagram */
    i = sendto (Icmp_sockfd, (char *)Icmp_buf, PACKSIZE, 0, 
			(struct sockaddr *)dest, sizeof (struct sockaddr));
    if (i < 0) {
	fprintf (stderr, "sendto failed (errno %d)\n", errno);
    }
    else if (i != PACKSIZE) {
	fprintf (stderr, "sendto error (write %d, return %d)\n", PACKSIZE, i);
    }

    return;
}

/*****************************************************************************

    Description: This function computes checksum for IP packet.

    Inputs:	ptr - pointer to the packet;
		nbytes - number of bytes in the packet;

    Return:	The check sum of the packet.

*****************************************************************************/

static int Compute_checksum (unsigned char *cptr, int nbytes) {
    int sum, last;
    unsigned short answer;
    unsigned short *ptr;

    /* our algorithm is simple, using a 32-bit accumulator (sum), we add 
       sequential 16-bit words to it, and at the end, fold back all the carry 
       bits from the top 16 bits into the lower 16 bits */
    sum = 0;
    ptr = (unsigned short *)(cptr);	/* skip the ICMP type field */
    last = nbytes - 1;
    while (nbytes > 1) {
	sum += *ptr++;
	nbytes -= 2;
    }

    if (nbytes == 1)
	sum += cptr[last];

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);		/* add carry */
    answer = ~sum;		/* ones-complement, then truncate to 16 bits */

    return (answer);
}

/*****************************************************************************

    Reads all ICMP messages and updates the rtime field in the host table.

*****************************************************************************/

static void Read_icmp_packets () {
    unsigned int fromlen;
    struct sockaddr_in from;
    struct ip *ip;
    struct icmp *icp;

    while (1) {
	int n, i;
	int iphdrlen;
	int addr, cksum;

	fromlen = sizeof (from);
	n = recvfrom (Icmp_sockfd, (char *)Icmp_buf, PACKSIZE + MAX_IP_HDR, 0, 
				(struct sockaddr *)&from, &fromlen);
	if (n < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return;			/* no packet to read */
	    else if (errno == EINTR)
		continue;		/* interrupted - try again */
	    else {
		fprintf (stderr, "recvfrom failed (errno %d)\n", errno);
	    }
	}

	addr = from.sin_addr.s_addr;	/* client address */

	/* the ip header (received ICMP packet contains the IP header) */
	ip = (struct ip *)Icmp_buf;
	iphdrlen = ip->ip_hl << 2;	/* convert # 32-bit words to # bytes */
	if (n < iphdrlen + PACKSIZE) {
/*
	    fprintf (stderr,  
		"ICMP packet too short (%d, expect %d) from host %x\n", 
		n, iphdrlen + PACKSIZE, (unsigned int)addr);
*/
	    continue;
	}
	n -= iphdrlen;

	icp = (struct icmp *) (Icmp_buf + iphdrlen);
	if (icp->icmp_type != ICMP_ECHOREPLY)
					/* non echo ICMP message */
	    continue;

	if (icp->icmp_id != Pid)	/* not our message */
	    continue;

	cksum = icp->icmp_cksum;
	icp->icmp_cksum = 0;
	if (cksum != Compute_checksum ((unsigned char *)icp, PACKSIZE)) {
					/* bad check sum */
	    fprintf (stderr, "received bad ICMP (check sum)\n");
	    continue;
	}

	for (i = 0; i < N_dests; i++) {
	    if (Dests[i].disabled)
		continue;
	    if (addr == Dests[i].addr) {
		unsigned int tm;

		/* get time */
		tm = *((unsigned int *)&(Icmp_buf [iphdrlen + SIZE_ICMP_HDR]));
		if (tm > Dests[i].rtime && tm <= Dests[i].stime)
		    Dests[i].rtime = tm;
	    }
	}
    }
}

