/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 17:04:15 $
 * $Id: net_misc.c,v 1.38 2014/05/13 17:04:15 steves Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 */


#include <config.h>
#ifdef __WIN32__
#define __INTERIX
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/tcp.h>	/* for TCP_NODELAY value */
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>

#ifndef __WIN32__
#include <net/if.h>
#endif

#ifdef SUNOS
    #include <stropts.h>       /* ioctl()                                 */
    #include <sys/sockio.h>
    #include <sys/ioccom.h>
#endif

#ifdef LINUX
    #include <sys/ioctl.h>     /* ioctl()                                 */
    #include <linux/sockios.h> /* SIOCGIFCONF                             */
    #include <netdb.h>         /* gethostbyname_r                         */
#endif

#ifdef IRIX
    #include <net/soioctl.h>
#endif
#include <net.h>
#include <misc.h>
#include <rmt.h>
#include <str.h>

static unsigned int Standalong_IP = 0;
static char *Standalong_name = NULL;

static int Get_local_IP_addresses (unsigned int *addr, int n_addr);

/****************************************************************

    Returns the IP address in string form in "buf".

****************************************************************/

char *NET_string_IP (unsigned int ip, int nbo, char *buf) {

    if (nbo)
	ip = ntohl (ip);
    sprintf (buf, "%d.%d.%d.%d", (ip >> 24), 
		(ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return (buf);
}

/****************************************************************

    Description: This function writes "len" bytes of data to a 
		socket. It does not block. It does not trap 
		SIGPIPE. It is the application program's 
		responsibility to trap the signal. Otherwise, the
		application may terminate due to a broken socket.

    Input:	fd - the file descriptor.
		data - pointer to the data to write.
		len - number of bytes to write.

    Return:	This function returns the number of bytes actually 
		sent or a negative LB error number.

****************************************************************/

int NET_write_socket (int fd, char *data, int len)
{
    int n_written;		/* number of byte written */

    n_written = 0;
    while (1) {
	int k;

	k = write (fd, data + n_written, len - n_written);
	if (k < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		break;
	    else if (errno == EBADF || errno == EPIPE) {
						/* socket disconnected */
		return (NET_DISCONNECTED);
	    }
	    else if (errno != EINTR) {	/* fatal write error */
		MISC_log ( 
		  "NET: write failed (NET_write_socket) (errno %d)\n", errno);
		return (NET_WRITE_FAILED);
	    }
	}
	if (k > 0)
	    n_written += k;
	if (n_written >= len)
	    break;
    }

    return (n_written);
}

/******************************************************************
			
    Description: This function reads at most "n_bytes" bytes of 
		data from socket "fd" into "data". This function 
		will not wait.

    Input:	fd - the file descriptor.
		data - pointer to the buffer for the data.
		len - number of bytes to read.

    Return:	The function returns the number of bytes read or 
		a negative LB error number.

******************************************************************/

int NET_read_socket (int fd, char *data, int len) 
{
    int n_read;			/* number of bytes read */

    n_read = 0;
    while (1) {
	int k;			/* read return value */

	if ((k = read (fd, data + n_read, len - n_read)) == 0) {	
					/* socket disconnected */
	    if (n_read > 0)
		return (n_read);
	    else
		return (NET_DISCONNECTED);
	}
	if (k < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		break;
	    if (errno == ECONNRESET)
		return (NET_DISCONNECTED);
	    if (errno != EINTR) {
		MISC_log (
		    "NET: read failed (NET_read_socket) (errno %d)\n", errno);
		return (NET_READ_FAILED);
	    }
	}
	else {
	    n_read += k;
	    if (n_read >= len)
		break;
	}
    }
    return (n_read);
}

/****************************************************************
			
    Returns the local IP addresses (in network byte order). Returns
    the number of local IP addresses, 0 if non of the local IP
    addresses can be found or a negative error code.

****************************************************************/

#define MAX_N_LOCAL_IP_ADDR	64

int NET_find_local_ip_address (unsigned int **add) 
{
    static unsigned int haddr [MAX_N_LOCAL_IP_ADDR];	
				/* local host inet addresses */
    static int n_haddr = -1;	/* number of local host inet addresses */

    /* search all network interfaces */
    if (n_haddr < 0) {
	n_haddr = Get_local_IP_addresses (haddr, MAX_N_LOCAL_IP_ADDR);
	if (n_haddr < 0)
	    return (NET_IFCONG_FAILED);
	if (n_haddr == 0)
	    MISC_log ("NET: No local IP address found\n");
	if (n_haddr < 0)
	    n_haddr = 0;
    }

    if (add != NULL)
	*add = haddr;
    return (n_haddr);
}

/****************************************************************
			
    Description: This function returns the IP address of host
		named "hname", which can be a ASCII form of IP
		address. The address is in network byte order.

    Output:	hname - host name. NULL or empty string means local
			host.

    Returns: 	the IP address on success or INADDR_NONE on failure.

****************************************************************/

unsigned int NET_get_ip_by_name (char *hname)
{
    static int n_names = 0;
    static char *names = NULL;
    static unsigned int *ips = NULL;
    struct sockaddr_in hostadd;			/* host address */
    struct hostent *hostport = NULL;		/* host port info */
    unsigned int addr;
    char *buffer = NULL, *p;
    int i = 0;
    int buf_size = 2048;
    struct hostent result;
    int herrno;
#ifdef LINUX
    int ret;
#endif

    if (hname == NULL || strcmp (hname, "") == 0) {	/* local host */
	unsigned int ip;
	if (RMT_lookup_host_index (RMT_LHI_IX2I, &ip, 0) <= 0)
	    return (INADDR_NONE);
	return (ip);
    }

    p = names;
    for (i = 0; i < n_names; i++) {
	if (strcmp (p, hname) == 0)
	    return (ips[i]);
	p += strlen (p) + 1;
    }

    if (Standalong_IP != 0 && Standalong_name != NULL &&
	strcmp (hname, Standalong_name) == 0)
	return (Standalong_IP);

    i = 0;
    while (i++ < 5){
	buf_size = buf_size * 2;   
	buffer = (char *)MISC_malloc (buf_size);
#if (defined (SUNOS) || defined (IRIX))
	hostport = gethostbyname_r (hname, &result, buffer, buf_size, &herrno);
#elif defined LINUX
	ret = gethostbyname_r (hname, &result, buffer, buf_size, 
						&hostport, &herrno);	 
#elif defined __WIN32__
        hostport = gethostbyname (hname); /* no reentrant version on Interix */
        herrno = h_errno;
        if (hostport != NULL)
           result = *hostport;
#endif
#ifdef LINUX
	if (ret || hostport == NULL) {
#else
	if (hostport == NULL) {
#endif
	    MISC_free (buffer);
	    buffer = NULL;

#if defined LINUX
	    if (ret != ERANGE) {
#elif defined __WIN32__
	    if (errno != ENOENT) {
#else
	    if (errno != ERANGE) {
#endif
		MISC_log (
	"NET: Failed in gethostbyname_r (host: %s, errno: %d, herrno: %d)\n",
					hname, errno, herrno);
		return (INADDR_NONE);
	    }
	    continue;
	}
	else {
            memcpy ((char *)&hostadd.sin_addr, 
			(char *)result.h_addr_list[0], result.h_length);          
					/* copy the internet address */
	    break;
	}
    }
    addr = hostadd.sin_addr.s_addr;
    if (buffer != NULL)
	MISC_free (buffer);

    if (names == NULL) {
	names = STR_reset (names, 256);
	ips = (unsigned int *)STR_reset (ips, 64);
    }
    names = STR_append (names, hname, strlen (hname) + 1);
    ips = (unsigned int *)STR_append (ips, &addr, sizeof (unsigned int));
    n_names++;
    return (addr);
}

/****************************************************************

    Description: Get a host name from an ip address

    Input:	ip_address - ip_address
		int name_buf_len - length of name_buf

    Output:	name_buf - host name
		
    Returns:	1 if successful, 0 otherwise

****************************************************************/

int NET_get_name_by_ip (unsigned int addr, char* name_buf, int name_buf_len) {
    char *name, buf[64];

    if (Standalong_IP != 0 && addr == Standalong_IP) {
	if (Standalong_name == NULL) {
	    struct utsname os;		/* current os info */
	    if (uname (&os) >= 0 && strlen (os.nodename) > 0)
		name = os.nodename;
	    else
		name = "local_host";
	    Standalong_name = (char *)MISC_malloc (strlen (name) + 1);
	    strcpy (Standalong_name, name);
	}
	name = Standalong_name;
    }
    else {
	struct hostent *hp;
	hp = gethostbyaddr ((char *)&addr, sizeof (addr), AF_INET);
	if ((hp == NULL) || (hp->h_name == NULL)) {
	    name = NET_string_IP (addr, 1, buf);
	    MISC_log ("NET: host name for ip %s not found - use ip\n", name);
	}
	else
	    name = hp->h_name;
    }

    strncpy (name_buf, name, name_buf_len);
    name_buf[name_buf_len - 1] = '\0';
    return (1);
}


/****************************************************************

    Description: This function returns a list of the IP addresses
		of all local IP ports. The list is limited by the
		the size, "n_addr",  of the user buffer "addr".

    Input:	n_addr - length of the "addr" buffer.

    Output:	addr - the list of local IP addresses.

    Returns:	This function returns the number of local addresses
		found on success or -1 on failure.

****************************************************************/

#define MAX_N_IF_PORTS	100

static int Get_local_IP_addresses (unsigned int *addr, int n_addr)
{

#ifdef __WIN32__
    /* the following structures should be defined in system include files.
       Since I could not find them, I define them here */
    struct ifreq {
	#define IFNAMSIZ      16
        char   ifr_name[IFNAMSIZ];                /* if name, for example */
                                                  /* "emd1" */
        union {
           struct sockaddr ifru_addr;
           struct sockaddr ifru_dstaddr;
           char            ifru_oname[IFNAMSIZ];  /* other if name */
           struct sockaddr ifru_broadaddr;
           short           ifru_flags;
           int             ifru_metric;
           char            ifru_data[1];          /* interface dependent data */

           char            ifru_enaddr[6];
           int             if_muxid[2];           /* mux id's for arp and ip */
        } ifr_ifru;
    };

    struct ifconf {
	int ifc_len;
	union {
	    caddr_t ifcu_buf;
	    struct ifreq *ifcu_req;
	} ifc_ifcu;
    };
#endif

    int cnt;
    int s;
    ALIGNED_t buf[ALIGNED_T_SIZE (MAX_N_IF_PORTS * sizeof (struct ifreq))];
    struct ifconf ifc;
    int ret;
    int i;

    s = socket (AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	MISC_log (
	    "NET: socket failed in Get_local_IP_addresses (ret = %d)\n", s);
	return (-1);
    }

    ifc.ifc_len = MAX_N_IF_PORTS * sizeof (struct ifreq);
    ifc.ifc_ifcu.ifcu_buf = (char *)buf;
    for (i = 0; i < MAX_N_IF_PORTS; i++) 
	ifc.ifc_ifcu.ifcu_req[i].ifr_name[0] = '\0';

    /* retrieve the IP port list */
#ifdef __WIN32__
#warning Get_local_IP_addresses is broken on Interix
    return -1; /* SIOCGIFCONF not available in Interix */
#else
    ret = ioctl (s, SIOCGIFCONF, &ifc);	/* this returns the port names */
    if (ret < 0) {
	MISC_log ( 
    "NET: ioctl SIOCGIFCONF in Get_local_IP_addresses failed (errno = %d)\n", 
							errno);
	close (s);
	return (-1);
    }
#endif

    /* get IP addresses of the ports */
    cnt = 0;
    for (i = 0; i < MAX_N_IF_PORTS; i++) {
	struct sockaddr_in *sin;
	struct ifreq ifrcopy;

	if (strcmp (ifc.ifc_ifcu.ifcu_req[i].ifr_name, "") == 0)
	    break;

#ifdef __WIN32__
        ret = -1; /* SIOCGIFADDR not available in Interix */
#else
	ret = ioctl (s, SIOCGIFADDR, &(ifc.ifc_ifcu.ifcu_req[i]));
#endif
	if (ret < 0)		/* this may fail for certain interfaces that 
				   are not of concern */
	    continue;

	ifrcopy = ifc.ifc_ifcu.ifcu_req[i];
	ioctl (s, SIOCGIFFLAGS, &ifrcopy);	/* this returns the flags */

	if (!(ifrcopy.ifr_flags & IFF_UP))
	    continue;

	sin = (struct sockaddr_in *)
			&(ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr);
	if (sin->sin_addr.s_addr != 0) {
	    addr[cnt] = sin->sin_addr.s_addr;
	    cnt++;
	    if (cnt >= n_addr)
		break;
	}
    }
    close (s);
    return (cnt);	
}

/****************************************************************

    Description: This function disables buffering of short data 
		in TCP level.

    Input:	fd - the socket fd.

    Return:	This function returns 0 on success or a negative 
		error code.

****************************************************************/

int NET_set_TCP_NODELAY (int fd)
{
    int i;

    i = 1;
#ifdef LINUX
    if (setsockopt (fd, SOL_TCP, TCP_NODELAY, 
					(char *)&i, sizeof (int)) < 0) {
#else
    if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, 
					(char *)&i, sizeof (int)) < 0) {
#endif
	MISC_log ("NET: setsockopt TCP_NODELAY (errno %d)\n", errno);
	return (NET_SETSOCKOPT_FAILED);
    }
    return (0);
}

/****************************************************************

    Description: This function sets the SO_REUSEADDR feature.

    Input:	fd - the socket fd.

    Return:	This function returns 0 on success or a negative 
		error code.

****************************************************************/

int NET_set_SO_REUSEADDR (int fd)
{
    int i;

    i = 1;
    if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof (int))
	< 0) {
	MISC_log ("NET: setsockopt SO_REUSEADDR failed (errno %d)\n", errno);
	return (NET_SETSOCKOPT_FAILED);
    }
    return (0);
}

/****************************************************************

    Description: This function sets non-blocking mode.

    Input:	fd - the socket fd.

    Return:	This function returns 0 on success or a negative 
		error code.

****************************************************************/

int NET_set_non_block (int fd)
{

    /* set non-blocking IO */
    if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) < 0) {
	MISC_log ("NET: fcntl F_SETFL failed (errno %d)\n", errno);

	return (NET_FCNTL_FAILED);
    }
    return (0);
}

/****************************************************************

    Description: This function disables the socket linger feature.

    Input:	fd - the socket fd.

    Return:	This function returns 0 on success or a negative 
		error code.

****************************************************************/

int NET_set_linger_off (int fd)
{
    struct linger lig;

    /* turn off linger */
    lig.l_onoff = 0;
    lig.l_linger = 0;
    if (setsockopt (fd, SOL_SOCKET, SO_LINGER, 
			(char *)&lig, sizeof (struct linger)) < 0) {
	MISC_log ("NET: setsockopt SO_LINGER failed (errno %d)\n", errno);

	return (NET_SETSOCKOPT_FAILED);
    }
    return (0);
}

/****************************************************************

    Description: This function enables the socket linger feature.
		This is however does not work for UNIX on Solaris
		X86. This does not return an error but the problem
		can be found by calling getsockopt, which returns
		-1 with errno = 22.

    Input:	fd - the socket fd.

    Return:	This function returns 0 on success or a negative 
		error code.

****************************************************************/

int NET_set_linger_on (int fd)
{
    struct linger lig;

    /* turn on linger */
    lig.l_onoff = 1;
    lig.l_linger = 10;
    if (setsockopt (fd, SOL_SOCKET, SO_LINGER, 
			(char *)&lig, sizeof (struct linger)) < 0) {
	MISC_log ("NET: setsockopt SO_LINGER failed (errno %d)\n", errno);

	return (NET_SETSOCKOPT_FAILED);
    }

    return (0);
}

/****************************************************************

    Description: This function turns on the KEEPALIVE feature.

    Input:	fd - the socket fd.

    Return:	This function returns 0 on success or a negative 
		error code.

****************************************************************/

int NET_set_keepalive_on (int fd)
{
    int i;

    /* switch on SO_KEEPALIVE */
    i = 1;
    if (setsockopt (fd, SOL_SOCKET, SO_KEEPALIVE, 
					(char *)&i, sizeof (int)) < 0) {
	MISC_log ("NET: setsockopt SO_KEEPALIVE failed (errno %d)", errno);
	return (NET_SETSOCKOPT_FAILED);
    }
    return (0);
}

