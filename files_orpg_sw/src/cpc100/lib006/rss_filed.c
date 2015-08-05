/****************************************************************
		
    Module: rss_filed.c	
				
    Description: This module contains remote file access server
	functions implemented on top of RMT.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/08 16:05:18 $
 * $Id: rss_filed.c,v 1.20 2012/08/08 16:05:18 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 * Revision 1.1  1996/06/04 16:44:24  cm
 * Initial revision
 *
 * 
*/

/* System include files */

#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>


/* Local include files; The following including order is important. We
   use local rmt_user_def.h and rss_def.h because they must be private 
   and localized. */
#include "rmtd_user_func_set.h"
#include "rmt_user_def.h"
#include <misc.h>
#include <rmt.h>
#include <rss.h>
#include "rss_def.h"

#define FULL_NAME_SIZE 128

/*** External references / external global variables ***/



/*** Local references / local variables ***/

static int Get_local_open_flag (int std_flag);

/* for rss_rpc testing 
char *Test_func (int i) {
    printf ("i = %d\n", i);
    return ("TTTTTTT\n");
}
*/

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "open". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_open
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_str		/* return string */
) {
    int mode;			/* open mode */
    int flag;			/* open flag */
    int fd;			/* return of open */
    char path[OPEN_MSG];	/* path name */
    char tmp[OPEN_MSG];		/* path name buffer */
    char *buffer;

    arg[len - 1] = '\0';	/* make the next sscanf save */
    if (len > OPEN_MSG ||	/* get the arguments */
	sscanf (arg, "%s %d %d", path, &flag, &mode) != 3)
	return (RSS_BAD_REQUEST_SERVER);
    /* convert the "open" flags to local format */
    flag = Get_local_open_flag (flag);

    /* add the part of home dir if needed */
    if (RSS_add_home_path (path, OPEN_MSG) == RSS_FAILURE) 
	return (RSS_HOME_UNDEFINED);

    if (RSS_check_file_permission (path) == RSS_FAILURE) 
        return (RSS_PERMISSION_DENIED);
    
    errno = 0;
    fd = open (MISC_expand_env (path, tmp, OPEN_MSG), flag, mode);

    buffer = RSS_shared_buffer ();

    /* prepare return values and return them */
    sprintf (buffer, "%d %d", fd, errno);
    *ret_str = buffer;
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "read". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: To be more efficient, this remote function processes a 
	maximum data size of RSS_MAX_DATA_LENGTH bytes. This size 
	will allow the RMT to use its static buffer. This will also 
	prevent the RMT from using too much memory.
	Also refer to rmt.doc.

******************************************************************/

int
  rss_read
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_str		/* return string */
) {
    int fd;			/* file fd */
    int length;			/* data length to read */
    int ret;
    rmt_t *pt;
    char *buffer;

    /* get the arguments */
    if (len != READ_HD)
	return (RSS_BAD_REQUEST_SERVER);
    pt = (rmt_t *)arg;
    fd = ntohrmt (pt [0]);
    length = ntohrmt (pt [1]);

    if (length < 0 || length > RSS_MAX_DATA_LENGTH)
	return (RSS_BAD_REQUEST_SERVER);

    buffer = RSS_shared_buffer ();

    /* read */
    errno = 0;
    ret = read (fd, buffer + READ_HD, length);

    /* return */
    pt = (rmt_t *)buffer;
    pt [0] = htonrmt ((rmt_t)ret);
    pt [1] = htonrmt ((rmt_t)errno);

    *ret_str = buffer;
    if (ret < 0)
	ret = 0;
    return (ret + READ_HD);
}

/****************************************************************

    Description: This is the remote (server) function of the RPC
	implementation of "write". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: To be more efficient, this remote function processes a 
	maximum data size of RSS_MAX_DATA_LENGTH bytes. This size 
	will allow the RMT to use its static buffer. This will also 
	prevent the RMT from using too much memory.
	Also refer to rmt.doc.
	
****************************************************************/

int
  rss_write
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_str		/* return string */
) {
    static int fd;		/* file fd */
    int ret, wlen;
    rmt_t *pt, *pt_ret;
    char *start;
    char *buffer;

    /* get fd */
    if ((len & 0x80000000) == 0) {	/* first segment write */
	if (len < (int)(WRITE_HD))
	    return (RSS_BAD_REQUEST_SERVER);
	pt = (rmt_t *)arg;
	fd = ntohrmt (pt [0]);
	start = arg + WRITE_HD;
	wlen = len - WRITE_HD;
    }
    else {				/* other segment writes */
	start = arg;
	wlen = len & 0xfffffff;
    }

    errno = 0;
    ret = write (fd, start, wlen);

    buffer = RSS_shared_buffer ();

    /* return */
    pt_ret = (rmt_t *)buffer;
    *ret_str = buffer;
    pt_ret [0] = htonrmt ((rmt_t)ret);
    pt_ret [1] = htonrmt ((rmt_t)errno);

    return (WRITE_HD);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "lseek". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_lseek
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_str		/* return string */
) {
    int fd, off, where;		/* lseek arguments */
    int ret;
    char *buffer;

    /* get the arguments */
    arg[len - 1] = '\0';
    if (sscanf (arg, "%d %d %d", &fd, &off, &where) != 3)
	return (RSS_BAD_REQUEST_SERVER);

    /* lseek */
    errno = 0;
    ret = lseek (fd, off, where);

    buffer = RSS_shared_buffer ();

    /* return */
    *ret_str = buffer;
    sprintf (buffer, "%d %d", ret, errno);
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "close". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_close
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_str		/* return string */
) {
    int fd;			/* file fd */
    int ret;
    char *buffer;

    /* get the arguments */
    arg[len - 1] = '\0';
    if (sscanf (arg, "%d", &fd) != 1)
	return (RSS_BAD_REQUEST_SERVER);

    /* close */
    errno = 0;
    ret = close (fd);

    buffer = RSS_shared_buffer ();

    /* return */
    *ret_str = buffer;
    sprintf (buffer, "%d %d", ret, errno);
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This function converts the "open" flags to the 
	local format form the machine independent format.

    Input:	std_flag - "open" flags in standard format

    Returns: The "open" flags in local format.

*****************************************************************/

#define S_O_RDONLY 0
#define S_O_WRONLY 1
#define S_O_RDWR   2
#define S_O_NDELAY 4
#define S_O_APPEND 8
#define S_O_CREAT 16
#define S_O_TRUNC 32
#define S_O_EXCL  64

static int
  Get_local_open_flag
  (
      int std_flag		/* standard open flag */
) {
    int f = 0;			/* local open flag */

    if (std_flag & S_O_RDONLY)
	f |= O_RDONLY;
    if (std_flag & S_O_WRONLY)
	f |= O_WRONLY;
    if (std_flag & S_O_RDWR)
	f |= O_RDWR;
    if (std_flag & S_O_NDELAY)
	f |= O_NDELAY;
    if (std_flag & S_O_APPEND)
	f |= O_APPEND;
    if (std_flag & S_O_CREAT)
	f |= O_CREAT;
    if (std_flag & S_O_TRUNC)
	f |= O_TRUNC;
    if (std_flag & S_O_EXCL)
	f |= O_EXCL;

    return (f);
}

