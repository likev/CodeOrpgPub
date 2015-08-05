/****************************************************************
		
	File: sock_shared.c	
				
	Purpose: This module contains the socket/IP functions shared 
		by the client and the server.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/12/12 22:30:22 $
 * $Id: rmt_sock_shared.c,v 1.12 2012/12/12 22:30:22 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#ifdef LINUX
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/tcp.h>	/* for TCP_NODELAY value */

#include <misc.h>
#include <net.h>
#include "rmt_def.h"

#define CMP_MAGIC_NUMBER 0x74928356
#define MIN_COMPRESS_SIZE 256

static char *Sbuf = NULL;	/* shared buffer */
static int Sb_size = 0;		/* size of the shared buffer */

static void Maintenance (int new_size);

/****************************************************************

    Description: This function returns the socket address for 
		local messaging.

    Output:	add - the socket address.
		so_name - socket name.

    Return:	This function returns the address size on success
		or a negative RMT error number.

****************************************************************/

#define SOCKET_NAME_SIZE 108
#include <sys/utsname.h>

int SCSH_local_socket_address (void **add, char **so_name)
{
    static struct sockaddr_un lsadd;	/* local server address */
    char *name, sock_name[128], nb[64], hname[64];
    int max_dir_len;
    unsigned int lip;

    if (RMT_lookup_host_index (RMT_LHI_IX2I, &lip, 0) > 0)
	sprintf (hname, "%x", lip);
    else
	strcpy (hname, "local_host");
    if ((lip = PNUM_get_local_ip_from_rmtport ()) == INADDR_NONE)
	sprintf (sock_name, ".rssd/.rmt.%d.%s",
		PNUM_get_port_number (), hname);
    else
	sprintf (sock_name, ".rssd/%s/.rmt.%d", NET_string_IP (lip, 1, nb),
				PNUM_get_port_number ());
    max_dir_len = SOCKET_NAME_SIZE - strlen (sock_name) - 2;

    name = (char *)&(lsadd.sun_path);
    if (SCSH_get_rssd_home (name, max_dir_len) < 0)
	exit (1);

    if (name[strlen (name) - 1] != '/')
	strcat (name, "/");
    strcat (name, sock_name);

    lsadd.sun_family = AF_UNIX;
    *add = (struct sockaddr *)&lsadd;
    if (so_name != NULL)
	*so_name = name;

    return (sizeof (struct sockaddr_un));
}

/****************************************************************

    Returns the rssd home dir defined in RMTPORT or HOME. Returns
    0 on success or a negative error number.

****************************************************************/

int SCSH_get_rssd_home (char *buf, int buf_size) {
    int dir_len;
    char *dir;

    dir_len = 0;
    if ((dir = getenv ("RMTPORT")) != NULL) {
	char *cpt;

	cpt = dir;
	while (*cpt != '\0') {
	    if (*cpt == ':' || *cpt == '@' || *cpt == '%') {
		dir_len = cpt - dir;
		break;
	    }
	    cpt++;
	}
	if (dir_len > 0 && dir[0] >= '0' && dir[0] <= '9')
	    dir_len = 0;
    }
    if (dir_len <= 0) {
	dir = getenv ("HOME");
	if (dir == NULL) {
	    MISC_log ("HOME undefined\n");
	    return (RMT_HOME_UNDEFINED);
	}
	dir_len = strlen (dir);
    }
    if (dir_len >= buf_size) {
	MISC_log ("rssd HOME too long\n");
	return (RMT_HOME_UNDEFINED);
    }
    memcpy (buf, dir, dir_len);
    buf[dir_len] = '\0';
    return (0);
}

/****************************************************************
			
	Select_wait()				Date: 2/24/94

	This function calls the select system call to wait until a
	socket is available for write (sw = SELECT_WRITE) or read 
	(sw = SELECT_READ). 

	The function returns TIME_EXPIRED if the specified time is
	expired, or IO_READY if the socket is ready or 
	FAILURE if the select system call failed.
*/

#ifdef HPUX_OLD

#define NINTS 4
#define BITS_PER_INT (8 * sizeof (int))

int SCSH_wait
  (
      int fd,			/* the socket fd */
      int sw,			/* a control switch */
      int time			/* maximum waiting time in seconds */
) {
    struct timeval wait;	/* time structure required by select */
    int ret;			/* return value */
    int rfd_set[NINTS];		/* the file descriptors for select */
    int i;

    if (fd > NINTS * BITS_PER_INT)
	return (FAILURE);

    for (i = 0; i < NINTS; i++)
	rfd_set [i] = 0;
    rfd_set[(fd / BITS_PER_INT)] |= (1 << (fd % BITS_PER_INT));

    wait.tv_sec = time;
    wait.tv_usec = 0;

    errno = 0;
    if (sw == SELECT_READ)	/* wait for data ready */
	ret = select (fd + 1, rfd_set, 0, 0, &wait);
    else			/* wait for write availability */
	ret = select (fd + 1, 0, rfd_set, 0, &wait);

    if (ret == 0 || (ret < 0 && errno == EINTR))
	return (TIME_EXPIRED);

    if (ret < 0) {		/* system call error */
	MISC_log ("select failed (errno %d)\n", errno);
	return (FAILURE);
    }
    return (IO_READY);
}


#else

#ifdef IBM
#include <values.h>
typedef struct sellist {int fdsmask [2];} fd_set;
#define FD_ZERO(a) ((a)->fdsmask[0] = (a)->fdsmask[1] = 0)
#define FD_SET(a,b) ((b)->fdsmask[a / BITS(int)] |= (1 << (a % BITS(int))))
#endif

int SCSH_wait
  (
      int fd,			/* the socket fd */
      int sw,			/* a control switch */
      int time			/* maximum waiting time in seconds */
) {
    struct timeval wait;	/* time structure required by select */
    int ret;			/* return value */
    fd_set rfd_set;		/* the file descriptors for select */

    FD_ZERO (&rfd_set);
    FD_SET (fd, &rfd_set);
    wait.tv_sec = time;
    wait.tv_usec = 0;

    errno = 0;
    if (sw == SELECT_READ)	/* wait for data ready */
	ret = select (fd + 1, &rfd_set, NULL, NULL, &wait);
    else			/* wait for write availability */
	ret = select (fd + 1, NULL, &rfd_set, NULL, &wait);

    if (ret == 0 || (ret < 0 && errno == EINTR)) {
	return (TIME_EXPIRED);
    }

    if (ret < 0) {		/* system call error */
	MISC_log ("select failed (errno %d)\n", errno);
	return (FAILURE);
    }
    return (IO_READY);
}

#endif		/* #ifdef HPUX */


/****************************************************************
			
    Compresses "data" of "size" bytes and returns the pointer to the
    compressed data. The size of the compressed data is returned with
    "csize". NULL is returned in case of failure and *csize is set to
    the error code. If "size" is too small or the compressed text is
    larger than the original, NULL is returned and *csize is set to 0.
    A magic number and the size of original data is put in front of
    the compressed data. This and RMT_decompress share an internal
    buffer for returning data. The buffer can be freed with
    RMT_free_buffer. Because the buffer is shared, one cannot pass the
    data returned from one of the two functions to another.

****************************************************************/

char *RMT_compress (int size, char *data, int *csize) {
    int ret, *ip;

    *csize = 0;
    if (size >= MIN_COMPRESS_SIZE) {
	int s = (int)((float)size * 1.01f) + 1024;
	if (s > Sb_size)
	    Maintenance (s);
	ret = MISC_compress (MISC_GZIP, data, size,
			Sbuf + 2 * sizeof (int), Sb_size - 2 * sizeof (int));
	if (ret < 0 || ret + 2 * sizeof (int) > size) {
	    if (ret < 0)
		*csize = ret;
	    return (NULL);
	}
    }
    else 
	return (NULL);

    ip = (int *)Sbuf;
    ip[1] = htonl (size);
    *csize = ret + 2 * sizeof (int);
    ip[0] = htonl (CMP_MAGIC_NUMBER);

    return (Sbuf);
}

/****************************************************************
			
    Decompresses "data" of "size" bytes and returns the pointer to the
    decompressed data. The size of the decompressed data is returned
    with "dsize". This and RMT_compress share an internal buffer for
    returning data. The buffer can be freed with RMT_free_buffer.
    Because the buffer is shared, one cannot pass the data returned
    from one of the two functions to another.

****************************************************************/

char *RMT_decompress (int size, char *data, int *dsize) {
    int ret, s, *ip;

    if (size == 0) {
	*dsize = 0;
	return (data);
    }

    ip = (int *)data;
    s = ntohl (ip[1]);
    if (ntohl (ip[0]) == CMP_MAGIC_NUMBER + 1) {	/* no compress */
	if (size != s + 2 * (int)sizeof (int))
	    return (NULL);
	if (s > Sb_size)
	    Maintenance (s);
	memcpy (Sbuf, data + 2 * sizeof (int), s);
	*dsize = s;
	return (Sbuf);
    }

    if (ntohl (ip[0]) != CMP_MAGIC_NUMBER)
	return (NULL);
    if (s <= 0)
	return (NULL);
    if (s > Sb_size)
	Maintenance (s);
    ret = MISC_decompress (MISC_GZIP, data + 2 * sizeof (int), 
			size - 2 * (int)sizeof (int), Sbuf, Sb_size);
    if (ret != s)
	return (NULL);
    *dsize = ret;
    return (Sbuf);
}

/****************************************************************
			
    Frees the shared buffer.

****************************************************************/

void RMT_free_buffer (void *buf) {

    if (Sbuf != NULL)
	free (Sbuf);
    Sbuf = NULL;
    Sb_size = 0;
}

/****************************************************************
			
    The maintenance function: Extends the size of the shared buffer 
    to "new_size" and load the compression functions.

****************************************************************/

static void Maintenance (int new_size) {

    if (Sbuf != NULL)
	free (Sbuf);
    Sbuf = (char *)MISC_malloc (new_size + 1024);
    Sb_size = new_size + 1024;
    return;
}
