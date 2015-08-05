
/*******************************************************************

    Module: net.h

    Description: Public header file for the net library.

*******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/23 19:09:50 $
 * $Id: net.h,v 1.9 2012/08/23 19:09:50 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#ifndef NET_H
#define NET_H

#include <poll.h>


/* net_misc.c return values */
#define NET_DISCONNECTED	-10
#define NET_READ_FAILED		-11
#define NET_WRITE_FAILED	-12
#define NET_MALLOC_FAILED	-13
#define NET_IFCONG_FAILED	-14
#define NET_BAD_LOCAL_IP	-15
#define NET_SETSOCKOPT_FAILED	-16
#define NET_FCNTL_FAILED	-17
#define NET_CANNOT_RESET	-18
#define NET_BIND_LOCAL_IP_FAIED	-1042

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff		/* for unavailable inet address */
#endif

#ifdef LINUX
#define POLL_IN_FLAGS (POLLIN | POLLPRI)
#define POLL_IN_RFLAGS (POLLIN | POLLPRI | POLLHUP)
#else
#define POLL_IN_FLAGS (POLLIN | POLLPRI | POLLRDBAND | POLLRDNORM)
					/* flags for socket input poll */
#define POLL_IN_RFLAGS (POLL_IN_FLAGS | POLLHUP)
#endif
/* we directly use POLLOUT for output bit */

#ifdef __cplusplus
extern "C"
{
#endif


/*
 * socket/networking routines
 */
int NET_write_socket (int fd, char *data, int len);
int NET_read_socket (int fd, char *data, int len) ;
int NET_find_local_ip_address (unsigned int **add);
int NET_network_available ();
unsigned int NET_get_ip_by_name (char *hname);
int NET_get_name_by_ip(unsigned int ip_address, char* name_buf, 
						int name_buf_len);
int NET_set_TCP_NODELAY (int fd);
int NET_set_SO_REUSEADDR (int fd);
int NET_set_non_block (int fd);
int NET_set_linger_off (int fd);
int NET_set_linger_on (int fd);
int NET_set_keepalive_on (int fd);
char *NET_string_IP (unsigned int ip, int need_byte_swap, char *buf);


#ifdef __cplusplus
}
#endif


#endif			/* #ifndef NET_H */
