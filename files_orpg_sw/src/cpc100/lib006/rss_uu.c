/****************************************************************
		
    Module: rss_uu.c	
				
    Description: This module contains remote Unix utility client
	functions implemented on top of RMT.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:42:58 $
 * $Id: rss_uu.c,v 1.21 2005/09/14 15:42:58 jing Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 * $Log: rss_uu.c,v $
 * Revision 1.21  2005/09/14 15:42:58  jing
 * Update
 *
 * Revision 1.17  2002/03/12 17:03:47  jing
 * Update
 *
 * Revision 1.16  1999/12/06 14:51:44  jing
 * @
 *
 * Revision 1.15  1998/12/01 21:18:09  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.12  1998/06/19 21:08:19  hoyt
 * posix update
 *
 * Revision 1.11  1998/06/19 17:05:51  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.7  1997/12/16 23:01:33  dodson
 * added preprocessor directives for LINUX
 *
 * Revision 1.6  1997/09/22 19:43:09  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 15:54:57  cm
 * SunOS 5.5 modifications
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
#include <signal.h>


/* Local include files; The following including order is important. We
   use local rmt_user_def.h and rss_def.h because they must be private 
   and localized. */
#include "rmt_user_def.h"
#include <rmt.h>
#include <rss.h>
#include "rss_def.h"


/* Local references / local variables */
/* table for converting the signal numbers - this table is used by the server
   module; An identical version must exist there. */
typedef struct {
  int rss_sig;
  int loc_sig;
} Signal_map_struct;

static Signal_map_struct Signal_map [] = {  
  { 1001, SIGHUP   },
  { 1002, SIGINT   },
  { 1003, SIGQUIT  },
  { 1004, SIGKILL  },
  { 1005, SIGALRM  },
  { 1006, SIGTERM  },
  { 1007, SIGUSR1  },
  { 1008, SIGUSR2  },
  { 1009, SIGCLD   },
  { 1010, SIGSTOP  },
  { 1011, SIGCONT  },
};		/* signal conversion table */

static int Convert_signal_to_global (int local_signal);


/****************************************************************
					
    Description: This is the client function for sending a signal 
	to a process on a local or remote host.

    Inputs:	host_name - The host name where the signal is to 
			    send.
		pid - Process ID which the signal is sent to
		sig - Signal number

    Returns: It returns the kill return value on success or a 
	negative error number on failure.
	
    Note: Refer to rss.doc.

*****************************************************************/

int RSS_kill
(
    char *host_name,	/* Host name */
    pid_t pid,		/* Process ID which the signal is sent to */
    int	sig		/* Signal number */
)
{
    int	cfd;		/* socket fd */
    int	ret;            /* return value */
    char *ret_val;	/* return string of remote call */
    char tmp_buf[TMP_SIZE];	/* temporary buffer */
    char name[TMP_SIZE];
    char path[TMP_SIZE];	/* dummy path name */
    char h_name[TMP_SIZE];	/* host name */

    /* parse "name" to find the host name and the file path */
    strncpy (name, host_name, TMP_SIZE - 8);
    name[TMP_SIZE - 8] = '\0';
    strcat (name, ":t");
    ret = RSS_find_host_name (name, h_name, path, TMP_SIZE);
    if (ret == RSS_FAILURE)	/* can not find host name */
	return (RSS_HOSTNAME_FAILED);

    if (ret != REMOTE_HOST)	/* Call local kill function */
	return (kill ((pid_t)pid, sig));

    /* open a connection to the remote host */
    cfd = RMT_create_connection (h_name);
    if (cfd < 0)
	return (cfd);

    sig = Convert_signal_to_global (sig);

    if (sig == RSS_FAILURE)
	return (RSS_INVALID_SIGNAL);

    /* Store "pid" and "sig" to an ASCII string and call the remote function */
    sprintf (tmp_buf, "%d %d", (int)pid, sig);
    ret = rss_kill (strlen (tmp_buf) + 1, tmp_buf, &ret_val);
    if (ret < 0)		/* Remote procedure call failed */
	return (ret);

    /* Retrieve the returned value and errno from the remote kill function */
    if (sscanf (ret_val, "%d %d", &ret, &errno) != 2)
	return (RSS_BAD_RET_VALUE);

    return (ret);
}

/****************************************************************
			
    Description: This is the client function for reading the host 
	time on a remote or local machine.

    Inputs:	host_name - The host name where the host time is to 
			    read.

    Outputs:	curr_time - The current time.

    Returns: It returns RSS_SUCCESS on success or a negative error 
	number on failure.
	
    Note: Refer to rss.doc.
	 
*******************************************************************/

int RSS_time
(
    char *host_name,		/* Remote host name */
    time_t *curr_time		/* Remote time value */
) 
{
    char tmp_buf[TMP_SIZE];
    char *ret_val;		/* return string of remote call */
    int ret, cfd;

    /* Open a connection to the remote host */
    cfd = RMT_create_connection (host_name);
    if (cfd < 0)
	return (cfd);

    /* Call remote function, tmp_buf does not store anything there. */
    ret = rss_time (0, tmp_buf, &ret_val);

    if (ret <= 0)		/* Remote process call failed */
	return (ret);

    /* Retrieve the time value from remote host */
    if (sscanf (ret_val, "%ld", (long *)curr_time) != 1)
	return (RSS_BAD_RET_VALUE);

    return (RSS_SUCCESS);
}

/****************************************************************
						
    Description: This function tests the RPC connection of a remote
	host.

    Inputs:	host_name - The name of the host to test.

    Returns: It returns RSS_SUCCESS if the RPC communication is OK
	or, otherwise, a negative error number on failure.
	
    Note: Refer to rss.doc.
	 
*******************************************************************/

int RSS_test_rpc
(
    char *host		/* Remote host name */
) 
{
    int ret;
    time_t tt;

    if ((ret = RSS_time (host, &tt)) < 0)
	return (ret);
    else
	return (RSS_SUCCESS);
}

/****************************************************************
			
    Description: This function converts a signal number to a 
	machine independent format.

    Input:	local_signal - signal number in local format

    Returns: The signal number in machine independent format
	on success or RSS_FAILURE on failure.

****************************************************************/

static int Convert_signal_to_global
(
   int local_signal
)
{
    int i;
    int sz;

    sz = sizeof (Signal_map) / sizeof (Signal_map_struct);

    for (i = 0; i < sz; i++) {
	if (local_signal == Signal_map [i].loc_sig)
	    return (Signal_map [i].rss_sig);
    }

    return (RSS_FAILURE);
}
