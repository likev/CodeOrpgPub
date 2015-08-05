/****************************************************************
		
    Module: rss_lbd.c	
				
    Description: This module contains remote LB server
	functions implemented on top of RMT.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:58:00 $
 * $Id: rss_lbd.c,v 1.40 2012/06/14 18:58:00 jing Exp $
 * $Revision: 1.40 $
 * $State: Exp $
 * $Log: rss_lbd.c,v $
 * Revision 1.40  2012/06/14 18:58:00  jing
 * Update
 *
 * Revision 1.36  2002/05/20 20:35:13  jing
 * Update
 *
 * Revision 1.36  2002/03/18 22:40:17  jing
 * Update
 *
 * Revision 1.35  2002/03/12 17:03:46  jing
 * Update
 *
 * Revision 1.34  2000/09/14 21:32:13  jing
 * @
 *
 * Revision 1.33  2000/08/21 20:51:54  jing
 * @
 *
 * Revision 1.32  2000/03/24 21:07:16  jing
 * @
 *
 * Revision 1.31  1999/06/25 15:11:52  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.28  1999/04/09 16:09:46  eforren
 * Change RSS_THREADED to THREADED
 *
 * Revision 1.27  1999/03/31 22:55:18  eforren
 * Reverted from version 1.25
 *
 * Revision 1.25  1999/03/31 18:32:36  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.16  1998/07/06 18:24:47  eforren
 * Add compression to the rss layer
 *
 * Revision 1.15  1998/07/02 14:35:46  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.12  1998/06/19 21:07:02  hoyt
 * posix update
 *
 * Revision 1.11  1998/06/19 17:05:26  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/04 16:44:33  cm
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
#include <rmt.h>
#include <rss.h>
#include "rss_def.h"
#include <lb.h>
#include <en.h>

/*** External references / external global variables ***/


static int Tag;
static LB_id_t Msg_id;

/* static void Process_msgs (char *msg, int msg_len, 
					int fd, unsigned int host);
*/

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_open". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_LB_open
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int flags;			/* open flags */
    LB_attr attr;
    int fd;			/* return of open */
    char path[LB_OPEN_MSG];	/* path name */
    int mode;
    int n_items;
    char *buffer;

    arg[len - 1] = '\0';	/* make the next sscanf save */
    if (len > LB_OPEN_MSG)
	return (RSS_BAD_REQUEST_SERVER);

    n_items = sscanf (arg + LB_REMARK_LENGTH, "%s %d %d %d %d %d %d",
		path, &flags, &mode, &(attr.msg_size), 
		&(attr.maxn_msgs), &(attr.types), &(attr.tag_size));
    if (n_items != 2 && n_items != 7)
	return (RSS_BAD_REQUEST_SERVER);

    /* add home dir if needed */
    if (RSS_add_home_path (path, LB_OPEN_MSG) == RSS_FAILURE) 
	return (RSS_HOME_UNDEFINED);

    if (RSS_check_file_permission (path) == RSS_FAILURE) 
        return (RSS_PERMISSION_DENIED);
    
    errno = 0;
    if (n_items == 2)
	fd = LB_open (path, flags, NULL);
    else {
	memcpy (attr.remark, arg, LB_REMARK_LENGTH);
	attr.mode = mode;
	fd = LB_open (path, flags, &attr);
    }

    /* return */
    buffer = RSS_shared_buffer ();
    sprintf (buffer, "%d %d", fd, errno);
    *ret_val = buffer;
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_read". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to Rss_lb_read and rmt.doc.

******************************************************************/

int
  rss_LB_read
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_str		/* return string */
) {
    static char *l_buf = NULL;	/* large buffer allocated */
    static char *part_2 = NULL;	/* second part msg */
    static int n_left = 0;	/* data left in buffer */
    static int o_fd;		/* store this for verification */
    static LB_id_t o_msg_id;	/* store this for verification */
    int fd;			/* file fd */
    int n_expect;		/* number of data bytes expected to return */
    int buf_size;		/* user buffer size; 0 means the second part */
    LB_id_t msg_id;		/* message id requested */
    int t_len;			/* total read length */
    char *buf;			/* buffer holding the message */
    int ret;
    rmt_t *pt;
    rmt_t *pt_ret;
    char *buffer;

    /* get the arguments */
    if (len != LB_READ_HD) 
	return (RSS_BAD_REQUEST_SERVER);

    pt = (rmt_t *)arg;
    fd = ntohrmt(pt [LRRQ_FD]);
    buf_size = ntohrmt (pt [LRRQ_BSIZE]);
    msg_id = (LB_id_t)ntohrmt (pt [LRRQ_ID]);
    n_expect = ntohrmt (pt [LRRQ_NB]);

    if (n_expect <= 0 || buf_size < -1)
	return (RSS_BAD_REQUEST_SERVER);

    /* read */
    buffer = RSS_shared_buffer ();
    if (buf_size >= 0) {	/* first part */
	int d_len;

	/* free old buffer */
	if (l_buf != NULL)
	    free (l_buf);
	part_2 = NULL;
	l_buf = NULL;

	/* determine the buffer to use */
	if (buf_size > RMT_PREFERRED_SIZE) {	/* need allocate */
	    l_buf = (char *)malloc (buf_size + LB_READ_HD);
	    if (l_buf == NULL) 
		return (RSS_NO_MEMORY_SERVER);
	    buf = l_buf;
	}
	else					/* use static buffer */
	    buf = buffer;

	/* read the message */
	if (buf_size != LB_ALLOC_BUF)
	    ret = LB_read (fd, buf + LB_READ_HD, buf_size, msg_id);
	else {
	    char *message;

	    ret = LB_read (fd, &message, buf_size, msg_id);
	    if (ret >= 0) {
		buf = (char *)malloc (ret + LB_READ_HD);
		if (buf == NULL)
		    return (RSS_NO_MEMORY_SERVER);
		if (ret > 0)
		    memcpy (buf + LB_READ_HD, message, ret);
		free (message);
		l_buf = buf;
	    }
	}

	pt_ret = (rmt_t *)buf;
	pt_ret [LRRT_LBRET] = htonrmt ((rmt_t)ret);
	pt_ret [LRRT_ERRNO] = htonrmt ((rmt_t)errno);
	pt_ret [LRRT_ID] = htonrmt ((rmt_t)Msg_id);
	pt_ret [LRRT_TAG] = htonrmt ((rmt_t)Tag);

	t_len = ret;
	if (ret == LB_BUF_TOO_SMALL)
	    t_len = buf_size;
	if (t_len < 0)
	    t_len = 0;

	d_len = t_len;	/* data length in the first return */	    
	if (d_len > n_expect)
	    d_len = n_expect;
	n_left = t_len - d_len;
	part_2 = buf + d_len + LB_READ_HD;
	pt_ret [LRRT_N_LEFT] = htonrmt ((rmt_t)n_left);
	pt_ret [LRRT_T_LEN] = htonrmt ((rmt_t)t_len);

	if (n_left <= 0)
	    RMT_free_user_buffer (&l_buf);	/* let the server free l_buf */

	o_fd = fd;
	o_msg_id = msg_id;
	*ret_str = buf;
	return (d_len + LB_READ_HD);
    }
    else {			/* the second part */
	int len;

	RMT_free_user_buffer (&l_buf);	/* let the server free l_buf */
	len = n_left;
	n_left = 0;		/* discard the data */

	/* verify arguments */
	if (fd != o_fd || msg_id != o_msg_id || n_expect != len ||
		len <= 0 || part_2 == NULL)
	    return (RSS_BAD_REQUEST_SERVER);

	*ret_str = part_2;	/* return the second part of the data */
	return (len);
    }
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_write". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to RSS_LB_write and rmt.doc.

****************************************************************/

int
  rss_LB_write
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    static int fd;		/* file fd */
    static int length = 0;	/* message length */
    static LB_id_t msg_id;	/* requested message id */
    static unsigned int lb_params;    
    int ret;
    rmt_t *pt, *pt_ret;
    char *buffer;
    unsigned int fhost;

    /* get fd */
    if ((len & 0x80000000) == 0) {	/* first step */
	int n_steps;

	if (len < (int)(LB_WRITE_HD))
	    return (RSS_BAD_REQUEST_SERVER);
	pt = (rmt_t *)arg;
	fd = ntohrmt (pt [LWRQ_FD]);
	n_steps = ntohrmt (pt [LWRQ_STEP]);
	length = ntohrmt (pt [LWRQ_LEN]);
	msg_id = (LB_id_t)ntohrmt (pt [LWRQ_ID]);
	Tag = ntohrmt (pt [LWRQ_TAG]);
	lb_params = ntohrmt (pt [LWRQ_PARAMS]);
	LB_UN_parameters (lb_params);
	if (n_steps < 1 || n_steps > 2) {
	    length = 0;
	    return (RSS_BAD_REQUEST_SERVER);
	}
	if (n_steps == 2) 		/* two step write */
	    return (0);
	else {				/* single step write */
	    if (length + (int)(LB_WRITE_HD) != len) {
		length = 0;
		return (RSS_BAD_REQUEST_SERVER);
	    }
	    ret = LB_write (fd, arg + LB_WRITE_HD, length, msg_id);
	}
    }
    else {				/* second step */
	if (length <= 0 || (len & 0xfffffff) != length)
	    return (RSS_BAD_REQUEST_SERVER);
	ret = LB_write (fd, arg, length, msg_id);
    }
    fhost = 0;			/* UN sending failure host */
    if (ret == LB_NTF_SEND_FAILED)
	fhost = LB_write_failed_host (fd);

    /* return values */
    buffer = RSS_shared_buffer ();
    pt_ret = (rmt_t *)buffer;
    *ret_val = buffer;
    pt_ret [LWRT_LBRET] = htonrmt ((rmt_t)ret);
    pt_ret [LWRT_ERRNO] = htonrmt ((rmt_t)errno);
    pt_ret [LWRT_ID] = htonrmt ((rmt_t)Msg_id);
    pt_ret [LWRT_FHOST] = fhost;

    return (LB_WRITE_HD);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_seek". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_LB_seek
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int fd, off;		/* LB_seek arguments */
    LB_id_t id;			/* LB_seek arguments */
    int ret;
    LB_info info;
    char *buffer;
    int flag;

    /* get the arguments */
    arg[len - 1] = '\0';
    if (sscanf (arg, "%d %d %u %d", &fd, &off, &id, &flag) != 4)
	return (RSS_BAD_REQUEST_SERVER);

    /* LB_seek */
    if (flag == CALL_LB_SEEK)
	ret = LB_seek (fd, off, id, &info);
    else
	ret = LB_msg_info (fd, id, &info);

    /* return */
    buffer = RSS_shared_buffer ();
    *ret_val = buffer;
    sprintf (buffer, "%d %d %d %u %d", 
			ret, errno, info.size, info.id, info.mark);
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of several LB functions. 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_LB_generic
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int fd;			/* file fd */
    int command;
    int ret;
    char *buffer;

    /* get the arguments */
    arg[len - 1] = '\0';
    if (sscanf (arg, "%d %d", &command, &fd) != 2)
	return (RSS_BAD_REQUEST_SERVER);

    switch (command) {
	int max_poll, wait_time;
	int offset, size, lock_command;
	unsigned int id, tag, un_params;
	int type, nrms, flag;
	void *local_add;
	int misc_cmd, address;
	int func, sdqs_port;
	unsigned int sdqs_ip;
	char path[256];

	case CALL_LB_CLOSE:	/* LB_close */
	    ret = LB_close (fd);
	    break;

	case CALL_LB_MISC:	/* LB_misc */
	    if (sscanf (arg, "%*d %*d %d", &misc_cmd) != 1)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_misc (fd, misc_cmd);
	    break;

	case CALL_LB_PREVIOUS_MSGID: /* LB_previous_msgid */
	    ret = LB_previous_msgid (fd);
	    break;

	case CALL_LB_WRITE_FAILED_HOST: 
	    ret = LB_write_failed_host (fd);
	    break;

	case CALL_LB_SET_POLL:	/* LB_set_poll */
	    if (sscanf (arg, "%d %d %d %d", &command, &fd, 
					&max_poll, &wait_time) != 4)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_set_poll (fd, max_poll, wait_time);
	    break;

	case CALL_LB_READ_WINDOW:	/* LB_read_window */
	    if (sscanf (arg, "%d %d %d %d", &command, &fd, 
						&offset, &size) != 4)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_read_window (fd, offset, size);
	    break;

	case CALL_LB_SET_TAG:		/* LB_set_tag */
	    if (sscanf (arg, "%d %d %x %x", &command, &fd, 
						&id, &tag) != 4)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_set_tag (fd, id, tag);
	    break;

	case CALL_LB_LOCK:		/* LB_lock */
	    if (sscanf (arg, "%d %d %d %x", &command, &fd, 
						&lock_command, &id) != 4)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_lock (fd, lock_command, id);
	    break;

	case CALL_LB_REGISTER:		/* LB_register */
	    if (sscanf (arg, "%d %d %d %d", &command, &fd, 
						&type, &address) != 4)
		return (RSS_BAD_REQUEST_SERVER);
	    if (type == LB_ID_ADDRESS)
		local_add = &Msg_id;
	    else if (type == LB_TAG_ADDRESS)
		local_add = &Tag;
	    else
		return (RSS_BAD_REQUEST_SERVER);
	    if (address == 0)
		local_add = NULL;
	    ret = LB_register (fd, type, local_add);
	    break;

	case CALL_ORPGDA_LB_OPEN:		/* ORPGDA LB open */
	    if (sscanf (arg, "%d %d %s", &command, &flag, path) != 3)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_open (path, flag, NULL);
	    if (ret < 0)
		break;
	    fd = ret;
	    ret = LB_register (fd, LB_ID_ADDRESS, &Msg_id);
	    if (ret < 0) {
		LB_close (fd);
		break;
	    }
	    ret = LB_misc (fd, LB_IS_BIGENDIAN);
	    if (ret < 0) {
		LB_close (fd);
		break;
	    }
	    errno = ret;
	    ret = fd;
	    break;

	case CALL_LB_CLEAR:		/* LB_clear */
	    if (sscanf (arg, "%d %d %d %d", &command, &fd, 
						&nrms, &un_params) != 4)
		return (RSS_BAD_REQUEST_SERVER);
	    LB_UN_parameters (un_params);
	    ret = LB_clear (fd, nrms);
	    break;

	case CALL_LB_DELETE:		/* LB_delete */
	    if (sscanf (arg, "%d %d %d", &command, &fd, &id) != 3)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_delete (fd, id);
	    break;

	case CALL_LB_SDQS_ADDRESS:	/* LB_sdqs_address */
	    if (sscanf (arg, "%d %d %d %d %x", 
			&command, &fd, &func, &sdqs_port, &sdqs_ip) != 5)
		return (RSS_BAD_REQUEST_SERVER);
	    ret = LB_sdqs_address (fd, func, &sdqs_port, &sdqs_ip);
	    buffer = RSS_shared_buffer ();
	    *ret_val = buffer;
	    sprintf (buffer, "%d %d %d %x", ret, errno, sdqs_port, sdqs_ip);
	    return (strlen (buffer) + 1);

	default:
	    return (RSS_BAD_REQUEST_SERVER);
    }

    /* return */
    buffer = RSS_shared_buffer ();
    *ret_val = buffer;
    sprintf (buffer, "%d %d", ret, errno);
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_list". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes:	Refer to rmt.doc.

		We assume that rmt_t is 4 bytes and LB_info.id and
		LB_info.size are at least 4 bytes.

****************************************************************/

int
  rss_LB_list
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int fd, nmsgs;		/* LB_list arguments */
    LB_info *list;
    int ret;
    rmt_t *pt;
    int cnt, i;
    static char *cpt = NULL;
    int bufsize;
    char *buffer;

    buffer = RSS_shared_buffer ();
    if (cpt != buffer && cpt != NULL)
	free (cpt);
    cpt = NULL;

    /* get the arguments */
    arg[len - 1] = '\0';
    if (sscanf (arg, "%d %d", &fd, &nmsgs) != 2)
	return (RSS_BAD_REQUEST_SERVER);

    bufsize = nmsgs * sizeof (LB_info) + LB_LIST_HD;
    if (bufsize <= RMT_PREFERRED_SIZE)
	cpt = buffer;
    else {
	cpt = (char *)malloc (bufsize);
	if (cpt == NULL)
	    return (RSS_NO_MEMORY_SERVER);
	RMT_free_user_buffer (&cpt);	/* let the server free buf */
    }

    list = (LB_info *)(cpt + LB_LIST_HD);
    ret = LB_list (fd, list, nmsgs);

    /* return */
    *ret_val = cpt;
    pt = (rmt_t *)cpt;
    pt [0] = htonrmt (ret);
    pt [1] = htonrmt (errno);
    cnt = 2;

    if (ret < 0)
	return (cnt * sizeof (rmt_t));

    for (i = 0 ;i < ret; i++) {

	pt [cnt] = htonrmt ((rmt_t)list[i].id);
	pt [cnt + 1] = htonrmt ((rmt_t)list[i].size);
	pt [cnt + 2] = htonrmt ((rmt_t)list[i].mark);
	cnt += 3;
    }

    return (cnt * sizeof (rmt_t));
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_stat". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_LB_stat
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int fd;			/* LB_stat arguments */
    rmt_t *pt;
    LB_status status;
    LB_attr attr;
    LB_check_list list [LB_STAT_N_IDS];
    int n_check;
    int ret, i;
    char *buffer;

    status.attr = &attr;
    status.check_list = list;

    /* get the arguments */
    if (len < 2 * (int)sizeof (rmt_t))
	return (RSS_BAD_REQUEST_SERVER);
    pt = (rmt_t *)arg;
    fd = ntohrmt (pt[0]);
    n_check = ntohrmt (pt[1]);
    status.n_check = n_check;
    if (len != (2 + n_check) * (int)sizeof (rmt_t) || 
	n_check > LB_STAT_N_IDS)
	return (RSS_BAD_REQUEST_SERVER);
    for (i = 0; i < n_check; i++)
	list[i].id = (LB_id_t) (ntohrmt (pt[i + 2]));

    if ((n_check + 10) * (int)sizeof (rmt_t) + LB_REMARK_LENGTH > 
						RMT_PREFERRED_SIZE)
	return (RSS_BAD_REQUEST_SERVER);

    ret = LB_stat (fd, &status);

    /* return */
    buffer = RSS_shared_buffer ();
    *ret_val = buffer;
    pt = (rmt_t *)(buffer);

    pt [0] = htonrmt ((rmt_t)ret);
    pt [1] = htonrmt ((rmt_t)errno);
    len = 2;

    if (ret < 0)
	return (len * sizeof (rmt_t));

    pt [2] = htonrmt ((rmt_t)attr.mode);
    pt [3] = htonrmt ((rmt_t)attr.msg_size);
    pt [4] = htonrmt ((rmt_t)attr.maxn_msgs);
    pt [5] = htonrmt ((rmt_t)attr.types);
    pt [6] = htonrmt ((rmt_t)attr.tag_size);
    pt [7] = htonrmt ((rmt_t)attr.version);

    pt [8] = htonrmt ((rmt_t)status.time);
    pt [9] = htonrmt ((rmt_t)status.n_msgs);
    pt [10] = htonrmt ((rmt_t)status.updated);
    pt [11] = htonrmt ((rmt_t)n_check);
    len += 10;

    for (i = 0 ;i < n_check; i++) {
	pt [len] = htonrmt ((rmt_t)list[i].status);
	len++;
    }

    memcpy (buffer + len * sizeof (rmt_t), attr.remark, LB_REMARK_LENGTH);

    return (len * sizeof (rmt_t) + LB_REMARK_LENGTH);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_remove". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to rmt.doc.

****************************************************************/

int
  rss_LB_remove
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int ret;
    char *buffer;

    arg[len - 1] = '\0';
    ret = LB_remove (arg);

    /* return */
    buffer = RSS_shared_buffer ();
    *ret_val = buffer;
    sprintf (buffer, "%d %d", ret, errno);
    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "LB_set_nr". 

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_val - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

****************************************************************/

int
  rss_LB_set_nr
  (
      int len,			/* calling argument string length */
      char *arg,		/* calling argument string */
      char **ret_val		/* return string */
) {
    int a_pid, a_fd, type;
    int ret;
    rmt_t *pt, *pt_ret;
    char *buffer;

    /* get parameters */
    if (len != LB_WUR_HD)
	return (RSS_BAD_REQUEST_SERVER);
    pt = (rmt_t *)arg;
    type = ntohrmt (pt[LNTFQ_TYPE]);
    if (type == WUR_NOTIFY) {	/* LB_set_nr */
	int fd, pid;
	LB_id_t msgid;
	unsigned int host;

	fd = ntohrmt (pt[LNTFQ_FD]);
	msgid = ntohrmt (pt[LNTFQ_MSGID]);
	host = pt[LNTFQ_HOST];
	pid = ntohrmt (pt[LNTFQ_PID]);
	ret = LB_set_nr (fd, msgid, host, pid, &a_pid, &a_fd);
    }
    else {			/* not expected */
	return (RSS_BAD_REQUEST_SERVER);
    }

    /* return values */
    buffer = RSS_shared_buffer ();
    pt_ret = (rmt_t *)buffer;
    *ret_val = buffer;
    pt_ret[LNTFT_RET] = htonrmt ((rmt_t)ret);
    pt_ret[LNTFT_APID] = htonrmt ((rmt_t)a_pid);
    pt_ret[LNTFT_AFD] = htonrmt ((rmt_t)a_fd);

    return (LB_WUR_RT_LEN);
}

/****************************************************************
			
    Description: This is the user function initialization routine. 
		This is called when server starts.

		Because LB_ntf_server and RMT messaging callback 
		function are identical, we directly register it.
		This assumes that 
		LB_NTF_WRITE_READY == RMT_MSG_WRITE_READY and
		LB_NTF_CONN_LOST == RMT_MSG_CONN_LOST and
		LB_NTF_TIMER == RMT_MSG_TIMER.
		Otherwise an intermediate function must be defined.

		Because RMT_send_msg is identical to LB_EXT_send_msg
		we dont need an LB_EXT_send_msg implementation.

    Returns: 	0 or RMT_ABORT.

****************************************************************/

int RMT_initialize_user_funcs ()
{

    RMT_register_msg_callback (EN_ntf_server);
    RSS_check_file_permission (NULL);
    return (0);
}



