/****************************************************************
		
	File: port_number.c	
				
	2/24/94

	Purpose: This module processes the port number for the
	RMT tool.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/12/12 22:30:22 $
 * $Id: rmt_port_number.c,v 1.14 2012/12/12 22:30:22 jing Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>

/*** Local include files ***/

#include <rmt.h>
#include <misc.h>
#include <net.h>
#include "rmt_def.h"


/*** Definitions / macros / types ***/
#define MIN_PORT_NUMBER 0	/* The minimum port number */
#define MAX_PORT_NUMBER 65534	/* The maximum port number */
#define PORT_INVALID	-2	/* invalid port number. We must use a 
				   value different from FAILURE */

/*** Local references / local variables ***/

static int Port_number = PORT_INVALID;
static int Disconnect_time = 0;

static int Default_port_number ();
static char *Get_file_name (char *name);

/****************************************************************

    Accept Disconnect_time.

****************************************************************/

void PNUM_disconnect_time (int t) {
    Disconnect_time = t;
}

/****************************************************************
			
	PNUM_set_port_number ()			Date: 2/24/94

	The user can call this function to set up an user specified
	port number "port_n". The port number, if set, will be 
	used for subsequent PNUM_get_port_number calls until the port 
	number is reset by another call of this function. If port_n
	== RMT_USE_DEFAULT_PORT, the default port number is set.

	If the "port_n" is too small or too large, or it could not set 
	a default port number, the function returns FAILURE. Otherwise 
	the port number is returned.
*/

int 
PNUM_set_port_number 
  (
	int port_n	/* port number to be set */
  )
{
    int port;

    if (port_n == RMT_USE_DEFAULT_PORT) {
	port = Default_port_number ();
	if (port == PORT_INVALID)
	    return (FAILURE);
	Port_number = port;
    }
    else {
	if (port_n < MIN_PORT_NUMBER || port_n > MAX_PORT_NUMBER)
	    return (FAILURE);
	Port_number = port_n;
    }

    return (Port_number);
}

/****************************************************************

    Writes (rssd) and reads (client) the "disc" file which contains
    the host connection info.

****************************************************************/

int RMT_access_disc_file (int is_server, char *buf, int buf_size) {
    static int fd = -2;

    if (buf == NULL) {	/* for disabling write access for rssd child */
	if (fd >= 0)
	    close (fd);
	fd = -2;
	return (0);
    }

    if (fd == -2) {
	char *fname = Get_file_name ("disc");
	if (is_server) {
	    if (Disconnect_time == 0) {
		MISC_unlink (fname);
		fd = -1;
	    }
	    else {
		fd = MISC_open (fname, O_RDWR | O_CREAT, 0664);
		if (fd < 0)
		    MISC_log ("open %s failed (errno %d)\n", fname, errno);
	    }
	}
	else
	    fd = MISC_open (fname, O_RDONLY, 0);
	free (fname);
    }
    if (fd < 0)
	return (-1);
    if (is_server) {
	char zero, one;
	zero = '0';
	one = '1';
	if (lseek (fd, 0, SEEK_SET) < 0 ||
	    MISC_write (fd, &zero, 1) != 1 ||
	    MISC_write (fd, buf, buf_size) != buf_size ||
	    lseek (fd, 0, SEEK_SET) < 0 ||
	    MISC_write (fd, &one, 1) != 1) {
	    MISC_log ("write disc file failed (errno %d)\n", errno);
	    return (-1);
	}
	return (0);
    }
    else {
	int len;
	char hd;
	if (lseek (fd, 0, SEEK_SET) < 0 ||
	    MISC_read (fd, &hd, 1) != 1 ||
	    hd != '1' ||
	    (len = MISC_read (fd, buf, buf_size)) <= 0) {
	    return (-1);
	}
	return (len);
    }
}

/****************************************************************

    Writes (rssd) and reads (client) the "hosts" file which contains
    the hosts name and order info.

****************************************************************/

int PNUM_access_hosts_file (int is_server, char *buf, int buf_size) {
    int fd, len;
    char *fname;

    fname = Get_file_name ("hosts");
    if (is_server) {
	char dir_buf[256];
	MISC_mkdir (MISC_dirname (fname, dir_buf, 256));
	fd = MISC_open (fname, O_RDWR | O_CREAT | O_TRUNC, 0664);
    }
    else
	fd = MISC_open (fname, O_RDONLY, 0);
    if (fd < 0) {
	MISC_log ("RMT: open %s failed (errno %d)\n", fname, errno);
	free (fname);
	return (-1);
    }

    len = 0;
    if (is_server) {
	if (MISC_write (fd, buf, buf_size) != buf_size) {
	    MISC_log ("write hosts file failed (errno %d)\n", errno);
	    len = -1;
	}
	else
	    MISC_log ("rssd hosts info saved in %s\n", fname);
    }
    else {
	if ((len = MISC_read (fd, buf, buf_size)) <= 0) {
	    MISC_log ("RMT: read %s failed (errno %d)\n", fname, errno);
	    len = -1;
	}
    }
    free (fname);
    close (fd);
    return (len);
}

/****************************************************************

    Generates the file name for the "disc" or "hosts" file. We cannot
    call lookup_host_index here because it will cause recursive calls
    on client side. The local name here is to support shared HOME
    by different hosts.

****************************************************************/

static char *Get_file_name (char *name) {
    char *fname, buf[64], h[256];
    unsigned int lip;

    if (SCSH_get_rssd_home (h, 256) < 0)
	exit (1);
    fname = MISC_malloc (strlen (h) + strlen (name) + 128);
    if ((lip = PNUM_get_local_ip_from_rmtport ()) == INADDR_NONE) {
	char lhname[256];
	unsigned int *addrs;
	if (NET_find_local_ip_address (&addrs) <= 0)
	    strcpy (lhname, "local_host");
	else
	    sprintf (lhname, "%x", addrs[0]);
	sprintf (fname, "%s/.rssd/rssd.%s.%d.%s", h, name,
				PNUM_get_port_number (), lhname);
    }
    else
	sprintf (fname, "%s/.rssd/%s/rssd.%s.%d", h,
		NET_string_IP (lip, 1, buf), name, PNUM_get_port_number ());
    return (fname);
}

/****************************************************************

    Return the IP (in NBO) of the local address specified in RMTPORT.
    Returns INADDR_NONE on failure.

****************************************************************/

unsigned int PNUM_get_local_ip_from_rmtport () {
    static unsigned int lip, init = 0;
    char *rmtport;

    if (init)
	return (lip);

    lip = INADDR_NONE;
    init = 1;
    rmtport = getenv ("RMTPORT");
    if (rmtport != NULL) {
	char *p = rmtport;
	while (*p != '\0' && *p != '%')
	    p++;
	if (*p == '%' && strlen (p + 1) > 0) {
	    lip = NET_get_ip_by_name (p + 1);
	    if (lip == INADDR_NONE)
		MISC_log ("RMT: Bad local IP select (%s) in RMTPORT\n", p + 1);
	}
    }
    return (lip);
}

/****************************************************************

    Returns the IP to bind for servers.

****************************************************************/

unsigned int RMT_bind_address () {
    unsigned int ip;

    if (PNUM_get_local_ip_from_rmtport () == INADDR_NONE)
	return (INADDR_ANY);
    RMT_lookup_host_index (RMT_LHI_IX2I, &ip, 0);
	return (ip);
}

/****************************************************************
			
	PNUM_get_port_number ()			Date: 2/24/94

	This function returns the current port number for RMT. If the
	current port number is not set, it tries to set the default
	port_number first. 

	The function returns FAILURE if the port number has never been 
	set and it can not find a default port number. Otherwise it 
	returns the port number.
*/

int
PNUM_get_port_number ()
{
    int port;

    if (Port_number == PORT_INVALID) {
	port = Default_port_number ();
	if (port == PORT_INVALID)
	    return (FAILURE);
	Port_number = port;
    }

    return (Port_number);
}

/****************************************************************
			
	Default_port_number ()			Date: 2/24/94

	This function returns the default port number. 

	The default port number is calculated based on the user name. 
	The directory part in the user name is removed for this purpose. 
	The name string is mapped to an integer number and
	used as the port number. Although it is not possible in general
	to uniquely map a char string to a number, the algorithm tries
	to minimize the ambiguity.

	The user can define an environmental variable "PORTNAME" to
	overwrite the user name.

	The default port number is calculated only once. The port number
	will not change if the environmental variables PORTNAME or HOME
	change.

	The RMT port number is always between MIN_PORT_NUMBER and
	MAX_PORT_NUMBER.

	The function will return PORT_INVALID if it can not find either 
	PORTNAME or HOME environmental variable.
*/

static int
Default_port_number ()
{
    char *str;			/* the user name or PORTNAME */
    int index;			/* where the name started */
    int n;			/* temp port number */
    static int default_port_number = PORT_INVALID; /* default port number */

    if (default_port_number == PORT_INVALID) {
	if ((str = getenv ("RMTPORT")) != NULL) {
	    char *cpt;

	    cpt = str;
	    while (*cpt != '\0') {
		if (*cpt == ':') {
		    str = cpt + 1;
		    break;
		}
		cpt++;
	    }
	    if (str[0] == '%' || str[0] == '@')
		default_port_number = PORT_INVALID;
	    else if (sscanf (str, "%d", &n) == 1) {
		if (n < MIN_PORT_NUMBER || n > MAX_PORT_NUMBER) {
		    MISC_log ("port (%d) in RMTPORT is out of range\n", n);
		    return (PORT_INVALID);
		}
		default_port_number = n;
	    }
	    else
		default_port_number = PORT_INVALID;
	}
	if (default_port_number == PORT_INVALID &&
				(str = getenv ("HOME")) != NULL) {

	    /* we do not use directory part of the user name */
	    for (index = strlen (str) - 1; index >= 0; index--)
		if (str[index] == '/')
		    break;

	    index++;
	    n = 0;
	    while (str[index] != '\0') {
		int k;

		k = (int) str[index++];
		if (k < 65 || k >= 97)
		    k = (k + 536) % 27 + 65;
		n = (n * 5) + (int) (k - 65);
		if (n > 90000)
		    break;
	    }
	    n = n + 1653;
	    if (n < 10000)
		n += 10000;
	    while (n >= 32768)
		n -= 1024;
	    default_port_number = n;
	}
	if (default_port_number == PORT_INVALID)
	    return (PORT_INVALID);
    }

    return (default_port_number);
}

/********************************************************************
			
    Description: This function processes byte swapping.

    Input:	x - the input word;

    Return:	The byte swapped x.

********************************************************************/

rmt_t RMT_byte_swap (rmt_t x)
{
#ifdef LITTLE_ENDIAN_MACHINE
    return (((x & 0xff) << 24) | ((x & 0xff00) << 8) | 
			((x >> 8) & 0xff00) | ((x >> 24) & 0xff));
#else
    return (x);
#endif
}
