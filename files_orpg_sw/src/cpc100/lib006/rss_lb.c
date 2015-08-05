
/****************************************************************
		
    Module: rss_lb.c	
				
    Description: This module contains remote LB client
	functions implemented on top of RMT.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/19 19:35:12 $
 * $Id: rss_lb.c,v 1.50 2011/05/19 19:35:12 jing Exp $
 * $Revision: 1.50 $
 * $State: Exp $
 * $Log: rss_lb.c,v $
 * Revision 1.50  2011/05/19 19:35:12  jing
 * Update
 *
 * Revision 1.44  2003/11/14 16:08:52  jing
 * Updated
 *
 * Revision 1.42  2002/05/20 20:35:11  jing
 * Update
 *
 * Revision 1.42  2002/03/18 22:40:15  jing
 * Update
 *
 * Revision 1.41  2002/03/12 17:03:44  jing
 * Update
 *
 * Revision 1.40  2000/09/20 21:50:43  jing
 * @
 *
 * Revision 1.39  2000/08/21 20:51:51  jing
 * @
 *
 * Revision 1.38  2000/03/24 21:07:14  jing
 * @
 *
 * Revision 1.36  1999/10/19 16:17:45  jing
 * @
 *
 * Revision 1.35  1999/06/25 15:11:48  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.30  1999/04/09 16:09:46  eforren
 * Change RSS_THREADED to THREADED
 *
 * Revision 1.29  1999/03/31 22:51:28  eforren
 * Reverted from version 1.26
 *
 * Revision 1.26  1999/03/18 22:59:47  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.18  1998/07/06 20:52:53  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.17  1998/07/06 18:24:46  eforren
 * Add compression to the rss layer
 *
 * Revision 1.16  1998/07/02 14:41:18  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.13  1998/06/19 17:05:18  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/04 16:44:30  cm
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
#include <misc.h>
#include <rss.h>
#include "rss_def.h"
#include <lb.h>
#include <en.h>

#ifdef THREADED
    #include <pthread.h>
#endif

#define LONG_TIME_OUT 3600	/* long time-out values for blocking
				   functions, LB_read and LB_seek */

enum {STORE_DATA, DEL_DATA, FIND_DATA};

struct local_pointers {		/* user registered pointers and flags */
    int *tag;			/* user's tag pointer */
    LB_id_t *id;		/* user's msg id pointer */
    int compress;		/* msg compression flag */
};

/* local functions */
static int Reset_time_out_ret (int ret);
static void *Data_store (int function, int key, int len, void *data);
static int Block_NTF_set_current (int fd);
static int Unblock_NTF_ret (int ret);
static int Use_LB_generic (int fd, char *tmp, char **ret_str);


/****************************************************************
			
    Description: This is the RPC implementation of "LB_open".

    Input:	name - the extended path name
		flags - LB open flags
		attr - the LB attributes 

    Returns: The combined fd on success or a negative error number 
	on failure.

    Notes: This function is similar to RSS_open. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

****************************************************************/

int RSS_LB_open (
      const char *name,		/* extended path name */
      int flags,		/* LB open flags */
      LB_attr *attr		/* the LB attributes */
) {
    int fd;			/* remote LB file fd */
    int cfd;			/* connection socket fd */
    char path[TMP_SIZE];	/* path name */
    char host_name[TMP_SIZE];	/* host name */
    char *ret_str;		/* return string of remote call */
    int ret, errno_ret;
    char tmp[LB_OPEN_MSG];	/* a local buffer */

#ifdef TEST_OPTIONS
    if (MISC_test_options ("PROFILE")) {
	char b[128];
	MISC_string_date_time (b, 128, (const time_t *)NULL);
	fprintf (stderr, "%s PROF: RSS_LB_open %s\n", b, name);
    }
#endif

    /* parse "name" to find the host name and the file path */
    ret = RSS_find_host_name (name, host_name, path, TMP_SIZE);

    if (ret == RSS_FAILURE)	/* can not find host name */
	return (RSS_HOSTNAME_FAILED);
    if (strlen (path) == 0)	/* void file name */
	return (RSS_BAD_PATH_NAME);

    if (ret != REMOTE_HOST)	/* local host */
	return (LB_open (path, flags, attr));

    /* open a connection to the remote host */
    EN_internal_block_NTF ();
    cfd = RMT_create_connection (host_name);
    if (cfd < 0)
	return (Unblock_NTF_ret (cfd));

    /* compose the argument string and call the remote function */
    if ((int)strlen (path) > LB_OPEN_MSG - 6 * 20 - LB_REMARK_LENGTH - 3)
	return (Unblock_NTF_ret (RSS_BAD_PATH_NAME));
				/* This is necessary for protecting 
				the following sprintf; We assume that each 
				integer uses atmost 24 character */
    if (attr != NULL) {
	memcpy (tmp, attr->remark, LB_REMARK_LENGTH);
	sprintf (tmp + LB_REMARK_LENGTH, "%s %d %d %d %d %d %d", path, flags, 
		(int)attr->mode, attr->msg_size, attr->maxn_msgs, attr->types, 
		attr->tag_size);
    }
    else {
	sprintf (tmp + LB_REMARK_LENGTH, "%s %d", path, flags);
    }

    ret = rss_LB_open (LB_REMARK_LENGTH + strlen (tmp + LB_REMARK_LENGTH) + 1, 
					tmp, &ret_str);
    if (ret < 0)
	return (Unblock_NTF_ret (ret));

    /* get return values */
    if (sscanf (ret_str, "%d %d", &fd, &errno_ret) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    /* return remote errno */
    errno = errno_ret;
    if (fd < 0)
	return (Unblock_NTF_ret (fd));

    return (Unblock_NTF_ret (COMBINE_FD (cfd, fd)));	
					/* return the combined fd */
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_read".

    Input:	cmfd - the combined fd
		buf_size - size of "buf" in number of bytes
		msg_id - requested message id

    Output:	buf - the message read

    Returns: The length of the message read on success or a negative
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the function
	interface and return values.

****************************************************************/

int
  RSS_LB_read
  (
    int cmfd,			/* combined fd */
    void *buf,			/* buffer space for data read */
    int buf_size,		/* size of "buf" */
    LB_id_t msg_id 
) {
    int fd;			/* socket fd */
    char *ret_str;		/* return string of remote call */
    int n_first;		/* number of bytes in first read */
    ALIGNED_t tmp[ALIGNED_T_SIZE (TMP_SIZE)];	/* a local buffer */
    rmt_t *pt, *pt_ret;
    int n_left, lb_read_ret, t_len;
    int ret, d_len;
    struct local_pointers *lppt;
    int prev_compress;
    int compress;
    char *alloc_buf;
    char *msg_buf;
    int errno_returned;

    fd = GET_CFD(cmfd);		/* socket (comm) fd */
    if (IS_LOCAL (fd) || buf_size < 0)	/* local host; If buf_size is bad we 
					   LB_read to return the error code */
	return (LB_read (cmfd, buf, buf_size, msg_id));

    /* block UN and choose the right remote connection */
    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);
/*    RMT_time_out (LONG_TIME_OUT); */	/* set the time out */

    lppt = (struct local_pointers *)Data_store (FIND_DATA, cmfd, 0, NULL);
    if (lppt != NULL)
	compress = lppt->compress;
    else
	compress = 0;

    /* first read reads the first part of the message, which needs a copy */
    n_first = RSS_MAX_DATA_LENGTH;
    pt = (rmt_t *)tmp;
    pt [LRRQ_FD] = htonrmt ((rmt_t)GET_FFD (cmfd));
    pt [LRRQ_BSIZE] = htonrmt ((rmt_t)buf_size);
    pt [LRRQ_ID] = htonrmt ((rmt_t)msg_id);
    pt [LRRQ_NB] = htonrmt ((rmt_t)n_first);
    
    prev_compress = 0;		/* not necessary - shut up gcc */
    if (compress)
       prev_compress = RMT_set_compression(RMT_COMPRESSION_ON);
           
    ret = rss_LB_read (LB_READ_HD, (char *)tmp, &ret_str);
    
    /*  Reset compression to its previous state */
    if (compress)
	RMT_set_compression(prev_compress);    

    if (ret < 0)
	return (Reset_time_out_ret (ret));
    if (ret < (int)(LB_READ_HD))
	return (Reset_time_out_ret (RSS_BAD_RET_VALUE));
    pt_ret = (rmt_t *)ret_str;
    lb_read_ret = ntohrmt (pt_ret [LRRT_LBRET]);	
			/* remote LB_read return value */
    errno_returned = errno = ntohrmt (pt_ret [LRRT_ERRNO]);
    n_left = ntohrmt (pt_ret [LRRT_N_LEFT]); /* number of bytes left in the server */
    
    t_len = ntohrmt (pt_ret [LRRT_T_LEN]); /* total data bytes to return */
    /* assign ID and tag */
    if (lppt != NULL) {
	if (lppt->id != NULL)
	    *(lppt->id) = ntohrmt (pt_ret [LRRT_ID]);
	if (lppt->tag != NULL)
	    *(lppt->tag) = ntohrmt (pt_ret [LRRT_TAG]);
    }
    if (t_len <= 0)
	return (Reset_time_out_ret (lb_read_ret));
    d_len = t_len - n_left;		/* data length in the return string */
    if (ret != d_len + (int)(LB_READ_HD) || 	/* consistency check */
	(buf_size != LB_ALLOC_BUF && t_len > buf_size))
	return (Reset_time_out_ret (RSS_BAD_RET_VALUE));

    if (buf_size != LB_ALLOC_BUF) {
	msg_buf = (char *)buf;
	alloc_buf = NULL;
    }
    else {
	alloc_buf = (char *)malloc (t_len);
	if (alloc_buf == NULL)
	    return (Reset_time_out_ret (RSS_NO_MEMORY_CLIENT));
	msg_buf = alloc_buf;
    }
    memcpy (msg_buf, ret_str + LB_READ_HD, d_len);

    /* second read reads the remaining part without copy */
    if (n_left > 0) {	
	RMT_use_buffer (t_len - d_len, msg_buf + d_len);
	pt [LRRQ_BSIZE] = htonrmt ((rmt_t)-1);	/* -1 means the second step */
	pt [LRRQ_NB] = htonrmt ((rmt_t)n_left);

        if (compress)
             prev_compress = RMT_set_compression(RMT_COMPRESSION_ON);
       	     
	ret = rss_LB_read (LB_READ_HD, (char *)tmp, &ret_str);
	
	/*  Set compression to its previous state */
        if (compress)
	    RMT_set_compression(prev_compress);	    	
	
	if (ret < 0) {
	    if (alloc_buf != NULL)
		free (alloc_buf);
	    return (Reset_time_out_ret (ret));
	}
   	if (ret != n_left) {
	    if (alloc_buf != NULL)
		free (alloc_buf);
	    return (Reset_time_out_ret (RSS_BAD_RET_VALUE));
	}
	errno = errno_returned;
    }
    
    if (buf_size == LB_ALLOC_BUF)
	*((char **)buf) = alloc_buf;

    return (Reset_time_out_ret (lb_read_ret));
}

/****************************************************************
					
    Description: This function resets the RMT time-out value to 
		its default and then returns "ret". 

    Input:	ret - the value to return;

    Output:	It returns "ret".

****************************************************************/

static int Reset_time_out_ret (int ret)
{

/*    RMT_time_out (0); */
    return (Unblock_NTF_ret (ret));
}

/****************************************************************
					
    Description: This is the RPC implementation of "LB_write". 

    Input:	cmfd - the combined fd
		data - the message to be written to the LB
		length - size of the message in number of bytes
		msg_id - message id

    Returns: The length of the message written on success or a 
	negative error number on failure.

    Notes: This function is similar to RSS_write. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

	Two step writing is used for messages that are larger than
	RSS_MAX_DATA_LENGTH. This eliminates copy of large data and 
	malloc of a large buffer.

****************************************************************/

int
  RSS_LB_write
  (
    int cmfd,			/* combined fd */
    char *data,			/* the message to write */
    int length,			/* message length in number of bytes */
    LB_id_t msg_id		/* message id */
) {
    int fd;			/* socket fd */
    char *ret_str;		/* return string of remote call */
    int ret;
    rmt_t *pt;
    int len, tag;
    char *buffer;
    struct local_pointers *lppt;
    int prev_compress;
    int compress;		/* TRUE if compression is on */
    int params;			/* LB params */

    int org_len;

    fd = GET_CFD(cmfd);		/* socket (comm) fd */
    if (IS_LOCAL (fd))
	return (LB_write (cmfd, data, length, msg_id));

    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    if (length <= 0)
	return (Unblock_NTF_ret (0));

    lppt = (struct local_pointers *)Data_store (FIND_DATA, cmfd, 0, NULL);
    compress = 0;
    tag = 0;
    if (lppt != NULL) {
	if (lppt->tag != NULL)		/* get tag */
	    tag = *(lppt->tag);
	compress = lppt->compress;
    }

    org_len = length;

    /* request header */
    buffer = RSS_shared_buffer ();
    pt = (rmt_t *)buffer;
    pt [LWRQ_FD] = htonrmt ((rmt_t)(GET_FFD (cmfd)));
    pt [LWRQ_LEN] = htonrmt ((rmt_t)length);
    pt [LWRQ_ID] = htonrmt ((rmt_t)msg_id);
    pt [LWRQ_TAG] = htonrmt ((rmt_t)tag);
    params = LB_UN_parameters (LB_UN_PARAMS_GET);
    pt [LWRQ_PARAMS] = htonrmt ((rmt_t)params);

    /*  Turn compression on or off*/
    if (compress)
       prev_compress = RMT_set_compression(RMT_COMPRESSION_ON);
    else
       prev_compress = RMT_compression();

    if (length <= RSS_MAX_DATA_LENGTH) {/* small message - single call */
	pt [LWRQ_STEP] = htonrmt ((rmt_t)1);/* indicates single step write */
	memcpy (buffer + LB_WRITE_HD, data, length);
	ret = rss_LB_write (length + LB_WRITE_HD, buffer, &ret_str);
    }
    else { 			/* large message - two step write */

	/* first call - sending the request */
	pt [LWRQ_STEP] = htonrmt ((rmt_t)2);	/* indicates two step write */
	ret = rss_LB_write (LB_WRITE_HD, buffer, &ret_str);
	if (ret >= 0)
	{
	   /* second call - sending the data */
	   ret = rss_LB_write (length | 0x80000000, data, &ret_str);
	}
    }
    
    /*  Reset compression to its previous setting */
    RMT_set_compression(prev_compress);
    
    if (ret < 0)
       return (Unblock_NTF_ret (ret));

    pt = (rmt_t *)ret_str;
    if (ret != LB_WRITE_HD)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    len = ntohrmt (pt [LWRT_LBRET]);
    errno = ntohrmt (pt [LWRT_ERRNO]);
    if (len <= 0) {
	if (len == LB_NTF_SEND_FAILED)
	    EN_print_unreached_host (pt[LWRT_FHOST]);
	return (Unblock_NTF_ret (len));
    }
    if (len != org_len)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    if (lppt != NULL && lppt->id != NULL) /* assign id to registered addr. */
	*(lppt->id) = ntohrmt (pt [LWRT_ID]);

    return (Unblock_NTF_ret (org_len));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_seek".

    Input:	cmfd - the combined fd
		offset - seek offset in number of messages
		id - seek starting message id

    Output:	info - message info pointed by the new pointer

    Returns: The LB_seek return value on success or a negative error
	number on failure.

    Notes: This function is similar to RSS_lseek. Refer to that function
	for additional details.	Refer to rss.doc for a complete
	description of the function interface and return values.

****************************************************************/

int
  RSS_LB_seek
  (
      int cmfd,			/* combined fd */
      int offset,		/* offset in number of messages */
      LB_id_t id,		/* seek starting message id */
      LB_info *info		/* message info pointed by the new pointer */
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* a local buffer */
    LB_id_t ret_id;
    int mark, size;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_seek (cmfd, offset, id, info));

    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);
/*    RMT_time_out (LONG_TIME_OUT); */

    sprintf (tmp, "%d %d %d %d", GET_FFD (cmfd), offset, id, CALL_LB_SEEK);
    ret = rss_LB_seek (strlen (tmp) + 1, tmp, &ret_str);
    if (ret < 0)
	return (Reset_time_out_ret (ret));

    if (sscanf (ret_str, "%d %d %d %d %d", 
				&ret, &errno, &size, &ret_id, &mark) != 5)
	return (Reset_time_out_ret (RSS_BAD_RET_VALUE));

    if (info != NULL) {
	info->id = ret_id;
	info->size = size;
	info->mark = mark;
    }

    return (Reset_time_out_ret (ret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_msg_info".
		It shares with RSS_LB_seek the same remote function
		rss_LB_seek.

    Input:	cmfd - the combined fd
		id - seek starting message id

    Output:	info - message info pointed by the new pointer

    Returns: The LB_msg_info return value on success or a negative error
	number on failure.

****************************************************************/

int
  RSS_LB_msg_info
  (
      int cmfd,			/* combined fd */
      LB_id_t id,		/* seek starting message id */
      LB_info *info		/* message info pointed by the new pointer */
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* a local buffer */
    LB_id_t ret_id;
    int mark, size;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_msg_info (cmfd, id, info));

    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);
/*    RMT_time_out (LONG_TIME_OUT); */

    sprintf (tmp, "%d %d %d %d", GET_FFD (cmfd), 0, id, CALL_LB_MSG_INFO);
    ret = rss_LB_seek (strlen (tmp) + 1, tmp, &ret_str);
    if (ret < 0)
	return (Reset_time_out_ret (ret));

    if (sscanf (ret_str, "%d %d %d %d %d", 
				&ret, &errno, &size, &ret_id, &mark) != 5)
	return (Reset_time_out_ret (RSS_BAD_RET_VALUE));

    if (info != NULL) {
	info->id = ret_id;
	info->size = size;
	info->mark = mark;
    }

    return (Reset_time_out_ret (ret));
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_close".

    Input:	cmfd - the combined fd

    Returns: The LB_close return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_close
  (
      int cmfd			/* combined fd */
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */
    int errno_ret;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_close (cmfd));

#ifdef THREADED
    if (!RMT_is_fd_of_this_thread (fd))
	return (RMT_CLOSE_IN_OTHER_THREAD);
#endif

    EN_close_notify (cmfd);
    sprintf (tmp, "%d %d", CALL_LB_CLOSE, GET_FFD (cmfd));
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* deregister any local pointers */
    Data_store (DEL_DATA, cmfd, 0, NULL);

    /* get return values */
    if (sscanf (ret_str, "%d %d", &ret, &errno_ret) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the generic LB RPC function. This function
		blocks UN. It unblocks UN on failure but not on
		success.

    Input:	fd - the socket fd.
		tmp - calling argument.

    Output:	ret_str - returns the return string.

    Returns:	Lenghth of return string on success or a negative 
		error number on failure.

****************************************************************/

static int Use_LB_generic (int fd, char *tmp, char **ret_str) 
{
    int ret;

    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    ret = rss_LB_generic (strlen (tmp) + 1, tmp, ret_str);
    if (ret < 0)
	return (Unblock_NTF_ret (ret));
    return (ret);
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_misc".

    Input:	cmfd - the combined fd

    Returns: The LB_misc return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_misc
  (
      int cmfd,			/* combined fd */
      int cmd
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_misc (cmfd, cmd));

    sprintf (tmp, "%d %d %d", CALL_LB_MISC, GET_FFD (cmfd), cmd);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);
    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_previous_msgid".

    Input:	cmfd - the combined fd

    Returns: The message id returned by the previous LB_read.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

LB_id_t
  RSS_LB_previous_msgid (int cmfd)
{
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_previous_msgid (cmfd));

    sprintf (tmp, "%d %d", CALL_LB_PREVIOUS_MSGID, GET_FFD (cmfd));
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the RPC implementation of 
		"LB_write_failed_host".

    Input:	cmfd - the combined fd

    Returns:	The IP address of the failed LB_write.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_write_failed_host (int cmfd)
{
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_write_failed_host (cmfd));

    sprintf (tmp, "%d %d", CALL_LB_WRITE_FAILED_HOST, GET_FFD (cmfd));
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_clear".

    Input:	cmfd - the combined fd
		nrms - number messages to remove

    Returns: The LB_clear return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_clear
  (
      int cmfd,			/* combined fd */
      int nrms
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_clear (cmfd, nrms));

    sprintf (tmp, "%d %d %d %d", CALL_LB_CLEAR, GET_FFD (cmfd), nrms, 
					LB_UN_parameters (LB_UN_PARAMS_GET));
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    if (sscanf (ret_str, "%d %d", &ret, &errno) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    return (Unblock_NTF_ret (ret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_delete".

    Input:	cmfd - the combined fd
		nrms - number messages to remove

    Returns: The LB_delete return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_delete
  (
      int cmfd,			/* combined fd */
      LB_id_t id
) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_delete (cmfd, id));

    sprintf (tmp, "%d %d %d", CALL_LB_DELETE, GET_FFD (cmfd), id);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    if (sscanf (ret_str, "%d %d", &ret, &errno) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    return (Unblock_NTF_ret (ret));
}

/****************************************************************
			
    This is the RPC implementation of "LB_sdqs_address".

****************************************************************/

int RSS_LB_sdqs_address (int cmfd, int func, int *port, unsigned int *ip) {
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_sdqs_address (cmfd, func, port, ip));

    sprintf (tmp, "%d %d %d %d %x", 
		CALL_LB_SDQS_ADDRESS, GET_FFD (cmfd), func, *port, *ip);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    if (sscanf (ret_str, "%d %d %d %x", &ret, &errno, port, ip) != 4)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    return (Unblock_NTF_ret (ret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_list".

    Input:	cmfd - the combined fd
		nmsgs - number of messages which info needs to be
			retrieved

    Output:	list - message info retrieved

    Returns: The LB_list return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_list
  (
    int cmfd,			/* combined fd */
    LB_info *list,
    int nmsgs
) {
    int fd;			/* socket fd */
    int ret, fret, cnt, i;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* a local buffer */
    rmt_t *pt;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_list (cmfd, list, nmsgs));

    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    sprintf (tmp, "%d %d", GET_FFD (cmfd), nmsgs);
    ret = rss_LB_list (strlen (tmp) + 1, tmp, &ret_str);
    if (ret < 0)
	return (Unblock_NTF_ret (ret));

    pt = (rmt_t *)ret_str;
    if (ret < 2 * (int)sizeof (rmt_t))
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    fret = ntohrmt (pt [0]);		/* remote function return */
    errno = ntohrmt (pt [1]);
    if (fret < 0)
	return (Unblock_NTF_ret (fret));

    if ((fret * 3 + 2) * (int)sizeof (rmt_t) != ret)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    cnt = 2;
    for (i = 0 ;i < fret; i++) {
	list[i].id = ntohrmt (pt [cnt]);
	list[i].size = ntohrmt (pt [cnt + 1]);
	list[i].mark = ntohrmt (pt [cnt + 2]);
	cnt += 3;
    }

    return (Unblock_NTF_ret (fret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_stat".

    Input:	cmfd - the combined fd

    Output:	st_buf - status info retrieved

    Returns: The LB_stat return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_stat
  (
    int cmfd,			/* combined fd */
    LB_status *st_buf
) {
    int fd;			/* socket fd */
    int ret, fret, i;
    char *ret_str;		/* return string of remote call */
    rmt_t arg[LB_STAT_N_IDS + 2];
				/* a local buffer for calling arg str; 2
				   positions are reserved for fd and n_check */
    rmt_t *pt;
    int n_check;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd) || st_buf == NULL)
	return (LB_stat (cmfd, st_buf));

    if (st_buf->n_check > LB_STAT_N_IDS)
	return (RSS_TOO_MANY_CHECKS);

    if (Block_NTF_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    arg [0] = htonrmt ((rmt_t)GET_FFD (cmfd));
    arg [1] = htonrmt ((rmt_t)st_buf->n_check);
    for (i = 0 ; i < st_buf->n_check; i++)
	arg [i + 2] = htonrmt ((rmt_t)st_buf->check_list[i].id);
    
    ret = rss_LB_stat (sizeof (rmt_t) * (st_buf->n_check + 2), 
						(char *)arg, &ret_str);
    if (ret < 0)
	return (Unblock_NTF_ret (ret));

    pt = (rmt_t *)ret_str;
    if (ret < 2 * (int)sizeof (rmt_t))
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    fret = ntohrmt (pt [0]);		/* remote function return */
    errno = ntohrmt (pt [1]);
    if (fret < 0)
	return (Unblock_NTF_ret (fret));

    n_check = ntohrmt (pt [11]);
    if (n_check != st_buf->n_check ||	/* consistency check */
	ret != (12 + n_check) * (int)sizeof (rmt_t) + LB_REMARK_LENGTH)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    if (st_buf->attr != NULL) {
	LB_attr *attr;

	attr = st_buf->attr;

	attr->mode = ntohrmt (pt [2]);
	attr->msg_size = ntohrmt (pt [3]);
	attr->maxn_msgs = ntohrmt (pt [4]);
	attr->types = ntohrmt (pt [5]);
	attr->tag_size = ntohrmt (pt [6]);
	attr->version = ntohrmt (pt [7]);
	memcpy (attr->remark, ret_str + ((12 + n_check) * sizeof (rmt_t)),
		LB_REMARK_LENGTH);
    }

    st_buf->time = ntohrmt (pt [8]);
    st_buf->n_msgs = ntohrmt (pt [9]);
    st_buf->updated = ntohrmt (pt [10]);

    for (i = 0 ;i < n_check; i++)
	st_buf->check_list[i].status = ntohrmt (pt [12 + i]);

    return (Unblock_NTF_ret (fret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_remove".

    Input:	lb_name - LB name

    Returns: The LB_remove return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int
  RSS_LB_remove
  (
      const char *lb_name
) {
    int cfd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char path[TMP_SIZE];	/* path name */
    char host_name[TMP_SIZE];	/* host name */

    /* parse "lb_name" to find the host name and the file path */
    ret = RSS_find_host_name (lb_name, host_name, path, TMP_SIZE);

    if (ret == RSS_FAILURE)	/* can not find host name */
	return (RSS_HOSTNAME_FAILED);
    if (strlen (path) == 0)	/* void file name */
	return (RSS_BAD_PATH_NAME);

    if (ret != REMOTE_HOST)	/* local host */
	return (LB_remove (path));

    /* open a connection to the remote host */
    EN_internal_block_NTF ();
    cfd = RMT_create_connection (host_name);
    if (cfd < 0)
	return (Unblock_NTF_ret (cfd));

    ret = rss_LB_remove (strlen (path) + 1, path, &ret_str);
    if (ret < 0)
	return (Unblock_NTF_ret (ret));

    if (sscanf (ret_str, "%d %d", &ret, &errno) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    return (Unblock_NTF_ret (ret));
}

/****************************************************************
			
    Description: This is the RPC implementation of "LB_direct".

    Input:	cmfd - the combined fd
		msg_id - requested message id

    Output:	ptr - the message pointer

    Returns: The length of the message read on success or a negative
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the function
	interface and return values. We do not implement this function
	as a remote procedure call because there is no performance
	advantage of doing that.

****************************************************************/

int
  RSS_LB_direct
  (
    int cmfd,			/* combined fd */
    char **ptr,
    LB_id_t msg_id 
) {
    int fd;			/* socket fd */

    fd = GET_CFD(cmfd);		/* socket (comm) fd */
    if (IS_LOCAL (fd))		/* local host */
	return (LB_direct (cmfd, ptr, msg_id));

    /* We don't support the remote access of this function */
    return (RSS_NOT_SUPPORTED);
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_set_poll".

    Input:	cmfd - the combined fd
		max_poll - maximum number of polls
		wait_time - waiting time in ms between poll.

    Returns: The LB_set_poll return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int RSS_LB_set_poll (int cmfd, int max_poll, int wait_time) 
{
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_set_poll (cmfd, max_poll, wait_time));

    sprintf (tmp, "%d %d %d %d", CALL_LB_SET_POLL, GET_FFD (cmfd), 
						max_poll, wait_time);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_read_window".

    Input:	cmfd - the combined fd
		offset - window offset
		size - window size

    Returns: The LB_read_window return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int RSS_LB_read_window (int cmfd, int offset, int size) 
{
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_read_window (cmfd, offset, size));

    sprintf (tmp, "%d %d %d %d", CALL_LB_READ_WINDOW, GET_FFD (cmfd), 
						offset, size);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_set_tag".

    Input:	cmfd - the combined fd
		id - message ID
		tag - tag value

    Returns: The LB_set_tag return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int RSS_LB_set_tag (int cmfd, LB_id_t id, int tag) 
{
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_set_tag (cmfd, id, tag));

    sprintf (tmp, "%d %d %x %x", CALL_LB_SET_TAG, GET_FFD (cmfd), 
				(unsigned int)id, (unsigned int)tag);

    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_register".

    Input:	cmfd - the combined fd
		type - address type
		address - address

    Returns: The LB_register return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int RSS_LB_register (int cmfd, int type, void *address) 
{
    struct local_pointers local_pts;	/* user registered pointers */
    struct local_pointers *pts;
    void *pt;
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */
    int errno_ret;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_register (cmfd, type, address));

    sprintf (tmp, "%d %d %d %d", CALL_LB_REGISTER, GET_FFD (cmfd), 
					type, (int)address);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d %d", &ret, &errno_ret) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    if (ret < 0)
	return (Unblock_NTF_ret (ret));

    /* we store the pointers locally */
    if ((pt = Data_store (FIND_DATA, cmfd, 0, NULL)) != NULL)
	pts = (struct local_pointers *)pt;
    else {
	pts = &local_pts;
	pts->tag = NULL;
	pts->id = NULL;
	pts->compress = 0;
    }

    if (type == LB_ID_ADDRESS)
	pts->id = (LB_id_t *)address;
    else if (type == LB_TAG_ADDRESS)
	pts->tag = (int *)address;
    else
	return (Unblock_NTF_ret (LB_BAD_ARGUMENT));

    if (pt == NULL &&
	Data_store (STORE_DATA, cmfd, 
		sizeof (struct local_pointers), (void *)pts) == NULL)
	return (Unblock_NTF_ret (RSS_NO_MEMORY_CLIENT));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    Description: This is the set/reset LB msg compression function.

    Input:	cmfd - the combined fd
		yes - set(non-zero)/reset(zero) compression

    Returns: 0 on success or a negative error number on failure.

****************************************************************/

int RSS_LB_compress (int cmfd, int yes) 
{
    struct local_pointers local_pts;	/* user registered pointers */
    struct local_pointers *pts;
    void *pt;

    /* we store the compression flag locally */
    EN_internal_block_NTF ();
    if ((pt = Data_store (FIND_DATA, cmfd, 0, NULL)) != NULL)
	pts = (struct local_pointers *)pt;
    else {
	pts = &local_pts;
	pts->tag = NULL;
	pts->id = NULL;
	pts->compress = 0;
    }
    pts->compress = yes;

    if (pt == NULL &&
	Data_store (STORE_DATA, cmfd, 
		sizeof (struct local_pointers), (void *)pts) == NULL)
	return (Unblock_NTF_ret (RSS_NO_MEMORY_CLIENT));

    return (Unblock_NTF_ret (0));
}

/****************************************************************
						
    Description: This is the RPC implementation of "LB_lock".

    Input:	cmfd - the combined fd
		command - lock command
		id - message ID

    Returns: The LB_lock return value on success or a negative 
	error number on failure.

    Notes: Refer to rss.doc for a complete description of the 
	function interface and return values.

****************************************************************/

int RSS_LB_lock (int cmfd, int command, LB_id_t id) 
{
    int fd;			/* socket fd */
    int ret;
    char *ret_str;		/* return string of remote call */
    char tmp[TMP_SIZE];		/* tmp buffer */

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_lock (cmfd, command, id));

    sprintf (tmp, "%d %d %d %x", CALL_LB_LOCK, GET_FFD (cmfd), 
					command, (unsigned int)id);
    if ((ret = Use_LB_generic (fd, tmp, &ret_str)) < 0)
	return (ret);

    /* get return values */
    if (sscanf (ret_str, "%d", &ret) != 1)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));

    return (Unblock_NTF_ret (ret));
}

/****************************************************************
						
    This is the implementation of the LB open function used by ORPGDA
    which calls three LB functions. This is more efficient in case
    of the satellite comms.

****************************************************************/

int RSS_orpgda_lb_open (const char *name, int flags, void *address, 
					int *endian) {
    int fd;			/* remote LB file fd */
    int cfd;			/* connection socket fd */
    char path[TMP_SIZE];	/* path name */
    char host_name[TMP_SIZE];	/* host name */
    char *ret_str;		/* return string of remote call */
    int ret, errno_ret, cmfd;
    char tmp[LB_OPEN_MSG];	/* a local buffer */
    struct local_pointers local_pts;	/* user registered pointers */
    struct local_pointers *pts;
    void *pt;

    /* parse "name" to find the host name and the file path */
    ret = RSS_find_host_name (name, host_name, path, TMP_SIZE);

    if (ret == RSS_FAILURE)	/* can not find host name */
	return (RSS_HOSTNAME_FAILED);
    if (strlen (path) == 0)	/* void file name */
	return (RSS_BAD_PATH_NAME);

    if (ret != REMOTE_HOST) {	/* local host */
	fd = LB_open (path, flags, NULL);
	if (fd < 0)
	    return (fd);
	ret = LB_register (fd, LB_ID_ADDRESS, address);
	if (ret < 0) {
	    LB_close (fd);
	    return (ret);
	}
	*endian = LB_misc (fd, LB_IS_BIGENDIAN);
	if (*endian < 0) {
	    LB_close (fd);
	    return (*endian);
	}
	return (fd);
    }

    /* open a connection to the remote host */
    EN_internal_block_NTF ();
    cfd = RMT_create_connection (host_name);
    if (cfd < 0)
	return (Unblock_NTF_ret (cfd));

    sprintf (tmp, "%d %d %s", CALL_ORPGDA_LB_OPEN, flags, path);
    if ((ret = rss_LB_generic (strlen (tmp) + 1, tmp, &ret_str)) < 0)
	return (Unblock_NTF_ret (ret));

    /* get return values */
    if (sscanf (ret_str, "%d %d", &ret, &errno_ret) != 2)
	return (Unblock_NTF_ret (RSS_BAD_RET_VALUE));
    if (ret < 0) {
	errno = errno_ret;
	return (Unblock_NTF_ret (ret));
    }
    fd = ret;
    *endian = errno_ret;

    /* we store the pointers locally */
    cmfd = COMBINE_FD (cfd, fd);
    if ((pt = Data_store (FIND_DATA, cmfd, 0, NULL)) != NULL)
	pts = (struct local_pointers *)pt;
    else {
	pts = &local_pts;
	pts->tag = NULL;
	pts->id = NULL;
	pts->compress = 0;
    }
    pts->id = (LB_id_t *)address;

    if (pt == NULL &&
	Data_store (STORE_DATA, cmfd, 
		sizeof (struct local_pointers), (void *)pts) == NULL)
	return (Unblock_NTF_ret (RSS_NO_MEMORY_CLIENT));

    return (Unblock_NTF_ret (cmfd));
}

/****************************************************************
						
    Description: This sets the LB NTF external functions.

		Because RMT_send_msg is identical to LB_EXT_send_msg
		we dont need an LB_EXT_send_msg implementation.

****************************************************************/

void LB_extern_service ()
{
    RSS_shared_buffer ();	/* initialize the buffer (malloc) */
    return;
}

/****************************************************************
				LB_EXT_send_msg		
    Description: This is the RPC implementation of "LB_set_un_req".
		Refer to LB_set_un_req.

    Input:	cmfd - the combined fd;
		host - callers host address;
		pid - callers pid; < 0 indicates resetting the record.
		msgid - the LB message id;

    Output:	a_pid - aliased pid;
		a_fd - aliased LB fd;

    Returns:	This function returns a LB_set_un_req return
		value or a negative RSS error number.

****************************************************************/

int RSS_LB_set_nr (int cmfd, LB_id_t msgid, 
		unsigned int host, int pid, int *a_pid, int *a_fd)
{
    int fd, ret;
    char *ret_str;		/* return string of remote call */
    char *buffer;		/* tmp buffer */
    rmt_t *pt;

    fd = GET_CFD (cmfd);
    if (IS_LOCAL (fd))
	return (LB_set_nr (cmfd, msgid, host, pid, a_pid, a_fd));

    if (RMT_set_current (fd) < 0)
	return (RSS_BAD_SOCKET_FD);

    buffer = RSS_shared_buffer ();
    pt = (rmt_t *)buffer;
    pt[LNTFQ_TYPE] = htonrmt ((rmt_t)WUR_NOTIFY);
    pt[LNTFQ_FD] = htonrmt ((rmt_t)cmfd);
    pt[LNTFQ_HOST] = host;
    pt[LNTFQ_PID] = htonrmt ((rmt_t)pid);
    pt[LNTFQ_MSGID] = htonrmt ((rmt_t)msgid);

    ret = rss_LB_set_nr (LB_WUR_HD, buffer, &ret_str);
    if (ret < 0)
	return (ret);

    pt = (rmt_t *)ret_str;
    if (ret != LB_WUR_RT_LEN)
	return (RSS_BAD_RET_VALUE);

    if (a_pid != NULL)
	*a_pid = ntohrmt (pt[LNTFT_APID]);
    if (a_fd != NULL)
	*a_fd = ntohrmt (pt[LNTFT_AFD]);
    return (ntohrmt (pt[LNTFT_RET]));
}

/****************************************************************
						
    Description: This function stores data structures. Each 
		structure is tagged with a key. Data structure 
		stored then can be retrieved with a key. The 
		entry is reused if a data is removed and its 
		length is large enough. This function can be used
		for other general purposes.

    Input:	function - function switch: STORE_DATA, 
			DEL_DATA and FIND_DATA.
		key - the key associated with the data.
		len - length of the data.
		data - pointer to the data.

    Returns:	STORE_DATA - pointer to stored "data" on success 
		or NULL on failure.
		DEL_DATA - always returns NULL.
		FIND_DATA - pointer to the data on success or 
		NULL on failure.

****************************************************************/

static void *Data_store (int function, int key, int len, void *data) 
{
    struct data_record {
	int key;
	struct data_record *next;
	int len;
	char *data;
    };
    static struct data_record *records = NULL;
    struct data_record *current, *previous;
    void *ret;
#ifdef THREADED
    static pthread_mutex_t mut;

    pthread_mutex_lock (&mut);
#endif
    ret = NULL;
    current = records;
    switch (function) {

	case STORE_DATA:
	    /* search for a existing record */
	    previous = NULL;
	    while (current != NULL) {
		if (current->data == NULL && current->len >= len)
		    break;
		previous = current;
		current = current->next;
	    }
	    if (current == NULL) { /* not found, we allocate a new record */
		struct data_record *new_rec;

		new_rec = (struct data_record *)
				malloc (sizeof (struct data_record) + len);
		if (new_rec == NULL)
		    break;
		if (previous == NULL)
		    records = new_rec;
		else 
		    previous->next = new_rec;
		current = new_rec;
		new_rec->len = len;
		new_rec->next = NULL;
	    }
	    current->data = (char *)current + sizeof (struct data_record);
	    memcpy (current->data, data, len);
	    current->key = key;
	    ret = (void *)current->data;
	    break;
	
	case DEL_DATA:
	    while (current != NULL) {
		if (current->key == key) {
		    current->data = NULL;
		}
		current = current->next;
	    }
	    break;

	case FIND_DATA:
	    while (current != NULL) {
		if (current->key == key && current->data != NULL) {
		    ret = (void *)current->data;
		    break;
		}
		current = current->next;
	    }
	    break;

	default:
	    break;
    }
#ifdef THREADED
    pthread_mutex_unlock (&mut);
#endif
    return (ret);
}

/****************************************************************
						
    Description: This function blocks the UN and sets the current
		remote host.

    Input:	fd - the remote host socket fd.

    Returns: 	0 on success or an RMT error number.

****************************************************************/

static int Block_NTF_set_current (int fd)
{

    EN_internal_block_NTF ();
    if (RMT_set_current (fd) < 0) {
	EN_internal_unblock_NTF ();
	return (RSS_BAD_SOCKET_FD);
    }
    return (0);
}

/****************************************************************
						
    Description: This function unblocks the UN and returns "ret".

    Input:	ret - the return value.

    Returns: 	argument "ret".

****************************************************************/

static int Unblock_NTF_ret (int ret)
{

    EN_internal_unblock_NTF ();
    return (ret);
}

