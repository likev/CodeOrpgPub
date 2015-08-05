/****************************************************************
		
    Module: rss.h	
				
    Description: This is the header file for the RSS module. This 
	file defines all public objects that used by the user of 
	the RSS library. This file must be included in all RSS 
	source code files and all application programs that use
	RSS library routines.

****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:36 $
 * $Id: rss.h,v 1.35 2012/07/27 19:33:36 jing Exp $
 * $Revision: 1.35 $
 * $State: Exp $
 * $Log: rss.h,v $
 * Revision 1.35  2012/07/27 19:33:36  jing
 * Update
 *
 * Revision 1.26  2002/03/18 22:31:05  jing
 * Update
 *
 * Revision 1.25  2002/03/12 16:45:04  jing
 * Update
 *
 * Revision 1.23  2000/09/25 21:29:43  jing
 * @
 *
 * Revision 1.21  2000/08/21 20:42:45  jing
 * @
 *
 * Revision 1.20  1999/04/19 17:16:11  vganti
 * Added functions to store fd and file information for threads
 *
 * Revision 1.19  1999/04/09 16:09:52  eforren
 * Remove compression from the RSS libraries
 *
 * Revision 1.18  1999/03/31 22:57:09  eforren
 * Reverted from version 1.16
 *
 * Revision 1.16  1999/03/03 20:28:07  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.13  1998/12/22 20:28:43  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.12  1998/12/02 22:07:11  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.11  1998/12/01 20:42:45  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.10  1998/07/02 14:36:44  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.9  1998/06/19 16:55:36  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.8  1998/06/18 13:46:52  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.7  1998/06/08 14:04:32  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.6  1998/03/13 16:59:13  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.5  1997/11/20 15:37:06  eforren
 * Allow rss functions to work from C++
 *
 * Revision 1.4  97/01/21  22:25:07  22:25:07  jing (Zhongqi Jing)
 * NO COMMENT SUPPLIED
 * 
 * Revision 1.3  96/10/14  14:31:41  14:31:41  jing (Zhongqi Jing)
 * NO COMMENT SUPPLIED
 * 
 * Revision 1.1  1996/06/04 16:49:52  cm
 * Initial revision
 *
*/

#ifndef RSS_H
#define RSS_H

#include <fcntl.h>
#include <rmt.h>
#include <lb.h>

#define RSS_FAILURE -1		/* general return value */
#define RSS_SUCCESS  0		/* general return value */

#define RSS_COPY_SRC_ERROR		-19
#define RSS_COPY_DEST_ERROR		-20
#define RSS_MALLOC_FAILED		-21
#define RSS_RPC_FORMAT_ERROR		-22
#define RSS_RPC_UNKNOWN_ARG_TYPE	-23
#define RSS_RPC_NAME_ERROR		-24
#define RSS_RPC_DLOPEN			-25
#define RSS_RPC_DLSYM			-26
#define RSS_RPC_NOT_IMPLEMENTED		-27
#define RSS_RPC_GETHOSTNAME_FAILED	-28
#define RSS_RPC_RECURSIVE_CALLS		-29
#define RSS_RPC_SMI_UNSET		-30
#define RSS_RPC_CLOSED_FOR_RESERVED_PORT	-31

typedef struct {		/* struct for variable size argument (type v) 
				   */
    int size;			/* number of bytes in "data" */
    short free_this;		/* This struct needs to be freed if not NULL */
    short free_data;		/* "data" needs to be freed if not NULL */
    char *data;			/* pointer to the data - the variable size 
				   argument */
} RSS_variable_size_arg;


#ifndef RKC
#ifdef __cplusplus
extern "C"
{
#endif
/* public functions */
int RSS_open (char *path, int flag, mode_t mode);
int RSS_read (int fd, char *buf, int nbyte);
int RSS_write (int fd, char *buf, int nbyte);
off_t RSS_lseek (int fd, off_t offset, int whence);
int RSS_close (int fd);
int RSS_copy (char *path_from, char *path_to);
char *RSS_expand_env (char *host, char *str, char *buf, int buf_size);

int RSS_kill (char *host, pid_t pid, int sig);
int RSS_time (char *host_name, time_t *curr_time);
int RSS_test_rpc (char *host);
int RSS_rpc (char *name, char *arg_f, ...);
int RSS_lpc (char *name, char *arg_f, ...);
int RSS_pc (char *name, char *arg_f, ...);
int RSS_rpc_need_byteswap ();
int RSS_set_SMI_func_name (char *name);
void RSS_rpc_stdout_redirect (void (*get_stdout) (char *));


int RSS_LB_open (const char *path, int flags, LB_attr *attr);
int RSS_LB_read (int lbd, void *buf, int buf_size, LB_id_t msg_id);
int RSS_LB_write (int lbd, char *msg, int length, LB_id_t msg_id);
int RSS_LB_close (int lbd);
int RSS_LB_remove (const char *lb_name);
int RSS_LB_seek (int lbd, int offset, LB_id_t id, LB_info *info);
int RSS_LB_clear (int lbd, int nrm);
int RSS_LB_stat (int lbd, LB_status *status);
int RSS_LB_list (int lbd, LB_info *list, int nlist);
int RSS_LB_misc (int lbd, int cmd);
int RSS_LB_direct (int lbd, char **ptr, LB_id_t msg_id);
LB_id_t RSS_LB_previous_msgid (int lbd);
int RSS_LB_write_failed_host (int cmfd);
int RSS_LB_set_poll (int lbd, int max_poll, int wait_time);
int RSS_LB_delete (int lbd, LB_id_t id);
int RSS_LB_sdqs_address (int cmfd, int func, int *port, unsigned int *ip);

int RSS_LB_lock (int lbd, int command, LB_id_t id);
int RSS_LB_set_tag (int lbd, LB_id_t id, int tag);
int RSS_LB_register (int lbd, int type, void *address);
int RSS_LB_read_window (int lbd, int offset, int size);
int RSS_LB_msg_info (int lbd, LB_id_t id, LB_info *info);
int RSS_orpgda_lb_open (const char *name, int flags, void *address, 
					int *endian);
int RSS_LB_compress (int lbd, int sw);


/* RPC functions */
int rss_open (int len, char *arg, char **ret_val);
int rss_read (int len, char *arg, char **ret_val);
int rss_write (int len, char *arg, char **ret_val);
int rss_lseek (int len, char *arg, char **ret_val);
int rss_close (int len, char *arg, char **ret_val);
int rss_kill (int len, char *arg, char **ret_val);
int rss_time (int len, char *arg, char **ret_val);

int rss_LB_open (int len, char *arg, char **ret_val);
int rss_LB_read (int len, char *arg, char **ret_val);
int rss_LB_write (int len, char *arg, char **ret_val);
int rss_LB_generic (int len, char *arg, char **ret_val);
int rss_LB_remove (int len, char *arg, char **ret_val);
int rss_LB_seek (int len, char *arg, char **ret_val);
int rss_LB_set_nr (int len, char *arg, char **ret_val);
int rss_LB_stat (int len, char *arg, char **ret_val);
int rss_LB_list (int len, char *arg, char **ret_val);

/* Rss_rpc - they are in the same file */
int Rmt_user_func_22_server (int arglen, char *arg, char **ret_str);
int Rmt_user_func_22 (int arglen, char *arg, char **ret_str);

/** functions for accessing lbs from differnet therads **/
int RSS_getCombinedFd(char *name,int fd); /* gets the actual combined fd of the lb */
int RSS_getPublishedFd(int fd);  /* same as the above funtion, but a different 
					interface*/
int RSS_deletePublishedFd(int fd); /* deletes the stored fd from the table */
int RSS_storePublishFd(char *name,int fd); /* stores the fd in  a table */


#ifdef __cplusplus
}
#endif

#endif


/* error code */

/* error detected in server */
#define RSS_BAD_REQUEST_SERVER -210
#define RSS_PERMISSION_DENIED  -211
#define RSS_HOME_UNDEFINED     -219
#define RSS_NO_MEMORY_SERVER    -213
#define RSS_INVALID_SIGNAL	-220

/* error detected in client */
#define RSS_HOSTNAME_FAILED    -215
#define RSS_BAD_PATH_NAME      -216
#define RSS_BAD_RET_VALUE      -217
#define RSS_BAD_SOCKET_FD      -218
#define RSS_DIFFERENT_HOST     -214
#define RSS_NO_MEMORY_CLIENT    -212
#define RSS_TOO_MANY_CHECKS	-221
#define RSS_NOT_SUPPORTED	-222
#define RSS_LOCAL_IP_ADDRESS_FAILED    -229



#endif 		/* RSS_H */
