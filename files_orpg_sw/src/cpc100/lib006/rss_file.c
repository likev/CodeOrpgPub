/****************************************************************
		
    Module: rss_file.c	
				
    Description: This module contains remote file access client
	functions implemented on top of RMT.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/08 16:05:18 $
 * $Id: rss_file.c,v 1.23 2012/08/08 16:05:18 jing Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 * Revision 1.1  1996/06/04 16:44:21  cm
 * Initial revision
 *
 * 
*/

/* System include files */

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>


/* Local include files; The following including order is important. We
   use local rmt_user_def.h and rss_def.h because they must be private 
   and localized. */
#include "rmt_user_def.h"
#include <rmt.h>
#include <rss.h>
#include <misc.h>
#include "rss_def.h"

/* Local references / local variables */
static int Get_standard_open_flag (int loc_flag);


/****************************************************************
			
    Description: This is the RPC implementation of "open".

    Input:	name - extended path name
		flag - open flags
		mode - access mode 

    Returns: The file descriptor on success or a negative error 
	number on failure.

    Notes: This function is similar to open. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

****************************************************************/

int
  RSS_open
  (
      char *name,		/* extended path name */
      int flag,			/* open flag */
      mode_t mode		/* open mode */
) {
    int fd;			/* remote file fd */
    int cfd;			/* connection socket fd */
    int m;			/* mode */
    char path[TMP_SIZE];	/* path name */
    char host_name[TMP_SIZE];	/* host name */
    char *ret_val;		/* return string of remote call */
    int ret, errno_ret;
    char tmp[OPEN_MSG];		/* a tmp buffer */

    /* parse "name" to find the host name and the file path */
    ret = RSS_find_host_name (name, host_name, path, TMP_SIZE);
    if (ret == RSS_FAILURE)	/* can not find host name */
	return (RSS_HOSTNAME_FAILED);
    if (strlen (path) == 0)	/* void file name */
	return (RSS_BAD_PATH_NAME);

    if (ret != REMOTE_HOST)		/* local host */
	return (open (MISC_expand_env (path, tmp, OPEN_MSG), flag, mode));

    /* open a connection to the remote host */
    cfd = RMT_create_connection (host_name);
    if (cfd < 0)
	return (cfd);

    m = 0;
    if (flag & O_CREAT)		/* save mode */
	m = (int) mode;
    flag = Get_standard_open_flag (flag);
				/* convert flags to hardware independent form */
   
    /* form the argument and call the RPC function */
    if ((int)strlen (path) > OPEN_MSG - 2 * 24 - 2)
	return (RSS_BAD_PATH_NAME);	/* This is necessary for protecting
				the following sprintf; We assume that each 
				integer uses atmost 24 character */
    sprintf (tmp, "%s %d %d", path, flag, m);
    ret = rss_open (strlen (tmp) + 1, tmp, &ret_val);
    if (ret < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_val, "%d %d", &fd, &errno_ret) != 2)
	return (RSS_BAD_RET_VALUE);
    if (fd < 0)
	return (fd);

    /* return remote errno */
    errno = errno_ret;

    return (COMBINE_FD (cfd, fd));	/* return the combined fd */
}

/****************************************************************
			
    Description: This is the RPC implementation of "read".

    Input:	cmfd - combined fd
		nbyte - number of bytes to read

    Output:	buf - data read

    Returns: The read return value on success or a negative error 
	number on failure.

    Notes: This function is similar to read. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

	For maximum efficiency this function uses RMT_use_buffer call
	to directly put the data read in the user buffer after reading
	the first segment.

****************************************************************/

int
  RSS_read
  (
      int cmfd,			/* combined fd */
      char *buf,		/* buffer space for data read */
      int nbyte			/* number of bytes to read */
) {
    int fd;			/* socket fd */
    char *ret_val;		/* return string of remote call */
    int n_read;			/* number of bytes read */
    ALIGNED_t tmp[ALIGNED_T_SIZE (TMP_SIZE)];	/* a tmp buffer */
    rmt_t *pt;

    fd = GET_CFD(cmfd);		/* socket (comm) fd */
    if (IS_LOCAL (fd))		/* local host; directly call read */
	return (read (cmfd, buf, nbyte));

    /* choose the right remote connection */
    if (RMT_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    if (nbyte <= 0)
	return (0);

    /* do multiple read to avoid large buffer and data copy */
    n_read = 0;			/* number of bytes read so far */
    while (1) {			
	int len, ret, read_ret;
	char save_ow [READ_HD];	/* storing bytes that will be overwritten */

	/* read length in this step */
	len = RSS_MAX_DATA_LENGTH;
	if (nbyte - n_read < len)
	    len = nbyte - n_read;

	pt = (rmt_t *)tmp;	/* prepare rss_read argument */
        pt [0] = htonrmt ((rmt_t)GET_FFD (cmfd));	/* file fd */
        pt [1] = htonrmt ((rmt_t)(len));		/* read length */
	if (n_read >= (int)(READ_HD))	/* directly use data in buf */
	    RMT_use_buffer (len + READ_HD, buf + n_read - READ_HD);
	ret = rss_read (READ_HD, (char *)tmp, &ret_val); /* call RPC func */
	if (ret < 0)
	    return (ret);

	if (ret < (int)(READ_HD))
	    return (RSS_BAD_RET_VALUE);

	pt = (rmt_t *)ret_val;	/* get remote read return values */
        read_ret = ntohrmt (pt [0]);
        errno = ntohrmt (pt [1]);

	if (read_ret < 0)
	    return (read_ret);

	if (ret != read_ret + (int)(READ_HD) || 
	    (n_read >= (int)(READ_HD) && buf + n_read - READ_HD != ret_val))
	    return (RSS_BAD_RET_VALUE);	/* consistency check failed */

	if (read_ret >= 0) {
	    if (n_read < (int)(READ_HD)) 	/* first time: copy data */
		memcpy (buf + n_read, ret_val + READ_HD, read_ret);
	    else			/* recover the overwritten bytes */
		memcpy (buf + n_read - READ_HD, save_ow, READ_HD);
	    n_read += read_ret;
	    if (n_read >= (int)(READ_HD))/* save the bytes that subject to 
					   overwritten next time */
		memcpy (save_ow, buf + n_read - READ_HD, READ_HD);
	}

	/* we assume here 0 return is EOF - BSD Unix */
	if (n_read >= nbyte || read_ret == 0 || read_ret < len) {
	    return (n_read);
	}
    }
}

/****************************************************************
			
    Description: This is the RPC implementation of "write".

    Input:	cmfd - combined fd
		buf - the data to write
		nbyte - number of bytes to write

    Returns: The write return value on success or a negative error 
	number on failure.

    Notes: This function is similar to write. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

	To maximize efficiency, starting from the second segment, we do 
	not send the header and directly sending the data. We use 
	the first bit in "length" to indicate these kind of request. 
	In the remote site the old fd is used for these segments. 
	In this way we eliminate copy of the data.

****************************************************************/

int
  RSS_write
  (
      int cmfd,			/* combined fd */
      char *buf,		/* the data to write */
      int nbyte			/* number of bytes to write */
) {
    int fd;			/* socket fd */
    char *ret_val;		/* return string of remote call */
    int n_written;		/* number of bytes written */
    int ret;
    rmt_t *pt;
    char *buffer;

    fd = GET_CFD (cmfd);	/* get comm (socket) fd */
    if (IS_LOCAL (fd))		/* local host */
	return (write (cmfd, buf, nbyte));

    /* choose the right remote connection */
    if (RMT_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    if (nbyte <= 0)
	return (0);

    /* do multiple write to avoid large buffer and data copy */
    buffer = RSS_shared_buffer ();
    n_written = 0;		/* number of bytes written so far */
    while (1) {
	int len, write_ret;

	/* write length in this step */
	len = RSS_MAX_DATA_LENGTH;
	if (nbyte - n_written < len)
	    len = nbyte - n_written;
	if (n_written == 0) {	/* first time we use copy */
	    pt = (rmt_t *)buffer;
	    pt [0] = htonrmt ((rmt_t)(GET_FFD (cmfd)));	/* file fd */
	    memcpy (buffer + WRITE_HD, buf + n_written, len);
	    ret = rss_write (len + WRITE_HD, buffer, &ret_val);
	}
	else 			/* we directly use data in buf */
	    ret = rss_write (len | 0x80000000, buf + n_written, &ret_val);

	if (ret < 0)
	    return (ret);

	pt = (rmt_t *)ret_val;
	if (ret < (int)(WRITE_HD))
	    return (RSS_BAD_RET_VALUE);
	write_ret = ntohrmt (pt [0]);	/* remote write return value */
	errno = ntohrmt (pt [1]);
	if (write_ret < 0)
	    return (write_ret);

	if (write_ret > 0)
	    n_written += write_ret;
	if (n_written >= nbyte)		/* enough bytes written - done */
	    return (n_written);
    }
}

/****************************************************************
			
    Description: This is the RPC implementation of "lseek".

    Input:	cmfd - combined fd
		buf - the data to write
		nbyte - number of bytes to write
		off_t offset - lseek offset
		int whence - lseek where to start

    Returns: The lseek return value on success or a negative error 
	number on failure.

    Notes: This function is similar to lseek. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

****************************************************************/

off_t
  RSS_lseek
  (
      int cmfd,			/* combined fd */
      off_t offset,		/* lseek parameter */
      int whence		/* lseek parameter */
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_val;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* a tmp buffer */

    fd = GET_CFD (cmfd);	/* get comm (socket) fd */
    if (IS_LOCAL (fd))		/* local host */
	return (lseek (cmfd, offset, whence));

    /* choose the right remote connection */
    if (RMT_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    /* prepare argument and call the RPC function */
    sprintf (tmp, "%d %d %d", GET_FFD (cmfd), (int)offset, whence);
    ret = rss_lseek (strlen (tmp) + 1, tmp, &ret_val);
    if (ret < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_val, "%d %d", &ret, &errno) != 2)
	return (RSS_BAD_RET_VALUE);
    return (ret);
}

/****************************************************************
			
    Description: This is the RPC implementation of "close".

    Input:	cmfd - combined fd

    Returns: The close return value on success or a negative error 
	number on failure.

    Notes: This function is similar to close. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

****************************************************************/

int
  RSS_close
  (
      int cmfd			/* combined fd */
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_val;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* a tmp buffer */

    fd = GET_CFD (cmfd);	/* get comm (socket) fd */
    if (IS_LOCAL (fd))		/* local host */
	return (close (cmfd));

    /* choose the right remote connection */
    if (RMT_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    /* prepare argument and call the RPC function */
    sprintf (tmp, "%d", GET_FFD (cmfd));
    ret = rss_close (strlen (tmp) + 1, tmp, &ret_val);
    if (ret < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_val, "%d %d", &ret, &errno) != 2)
	return (RSS_BAD_RET_VALUE);
    return (ret);
}

/****************************************************************
			
    Description: This function copies a file.

    Input:	path_form - The extended source file name
		path_to - The extended destination path name
		print_msg - message printing switch: if non-zero
		    RSS_copy will print error messages on failure.

    Returns: RSS_SUCCESS on success or RSS_FAILURE on failure.

    Notes: This is not a RPC function. It calls other RSS functions
	to perform the job.

****************************************************************/

#define RSS_COPY_SIZE 32000

int
  RSS_copy
  (
    char *path_from,		/* source file path name */
    char *path_to		/* destination path name */
) {
    int sfd, dfd;
    int cnt, ret;
    char *buf;

    sfd = dfd = -1;
    buf = NULL;

    /* open source file */
    if ((sfd = RSS_open (path_from, O_RDONLY, 0)) < 0) {
	ret = RSS_COPY_SRC_ERROR;
	goto failed;
    }

    /* open destination file */
    if ((dfd = RSS_open (path_to, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
	ret = RSS_COPY_DEST_ERROR;
	goto failed;
    }

    /* allocate buffer */
    buf = (char *)malloc (RSS_COPY_SIZE);
    if (buf == NULL) {
	ret = RSS_NO_MEMORY_CLIENT;
	goto failed;
    }

    /* copy */
    cnt = 0;
    while (1) {
	int k;

	k = RSS_read (sfd, buf, RSS_COPY_SIZE);
	if (k < 0) {
	    ret = k;
	    goto failed;
	}

	cnt += k;
	if (k == 0) {	/* copy finished */
	    RSS_close (sfd);
	    RSS_close (dfd);
	    free (buf);
	    return (RSS_SUCCESS);
	}

	k = RSS_write (dfd, buf, k);
	if (k < 0) {
	    ret = k;
	    goto failed;
	}
    }

  failed:
    if (sfd >= 0)
	RSS_close (sfd);
    if (dfd >= 0)
	RSS_close (dfd);
    if (buf != NULL)
	free (buf);
    return (ret);
}

/****************************************************************
			
    Description: This function converts the "open" flags to a 
	machine independent format.

    Input:	loc_flag - "open" flags in local format

    Returns: The "open" flags in machine independent format.

****************************************************************/

#define S_O_RDONLY 0
#define S_O_WRONLY 1
#define S_O_RDWR   2
#define S_O_NDELAY 4
#define S_O_APPEND 8
#define S_O_CREAT 16
#define S_O_TRUNC 32
#define S_O_EXCL  64

static int
  Get_standard_open_flag
  (
      int loc_flag		/* local open flag */
) {
    int f = 0;			/* standard open flag */

    if (loc_flag & O_RDONLY)
	f |= S_O_RDONLY;
    if (loc_flag & O_WRONLY)
	f |= S_O_WRONLY;
    if (loc_flag & O_RDWR)
	f |= S_O_RDWR;
    if (loc_flag & O_NDELAY)
	f |= S_O_NDELAY;
    if (loc_flag & O_APPEND)
	f |= S_O_APPEND;
    if (loc_flag & O_CREAT)
	f |= S_O_CREAT;
    if (loc_flag & O_TRUNC)
	f |= S_O_TRUNC;
    if (loc_flag & O_EXCL)
	f |= S_O_EXCL;
    return (f);
}

/****************************************************************
			
    The remote version of MISC_expand_env. It returns buf on sucess
    or str on failure. This can be used for getting remote env.

****************************************************************/

char *RSS_expand_env (char *host, char *str, char *buf, int buf_size) {
    int ret;
    char tmp[TMP_SIZE], path[TMP_SIZE], *rstr;
    char host_name[TMP_SIZE], cmd[TMP_SIZE];

    if (buf_size > 0)
	buf[0] = '\0';
    if (host == NULL || host[0] == '\0' || buf_size == 0)
	strcpy (tmp, "t");
    else {
	if (strlen (host) + 32 >= TMP_SIZE) {
	    MISC_log ("RSS_expand_env: host name too long\n");
	    return (str);
	}
	strcpy (tmp, host);
	strcat (tmp, ":t");
    }
    ret = RSS_find_host_name (tmp, host_name, path, TMP_SIZE);
    if (ret == RSS_FAILURE) {	/* can not find host name */
	MISC_log ("RSS_expand_env: Bad host name - %s\n", host);
	return (str);
    }

    if (ret != REMOTE_HOST)		/* local host */
	return (MISC_expand_env (str, buf, buf_size));

    sprintf (cmd, "%s:MISC_expand_env", host);
    ret = RSS_rpc (cmd, "p-r s-i ba-%d-io i-i",
				buf_size, &rstr, str, buf, buf_size);
    if (ret < 0) {
	MISC_log ("RSS_expand_env: RSS_rpc to %s failed (%d)\n", host, ret);
	return (str);
    }
    if (buf[0] == '\0') {
	MISC_log ("RSS_expand_env: MISC_expand_env failed\n");
	return (str);
    }
    return (buf);
}
