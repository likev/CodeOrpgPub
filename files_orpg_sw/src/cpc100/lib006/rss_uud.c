/****************************************************************
		
    Module: rss_uud.c	
				
    Description: This module contains remote UNIX utility server
	functions implemented on top of RMT.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/10/04 18:42:01 $
 * $Id: rss_uud.c,v 1.22 2005/10/04 18:42:01 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 * Revision 1.1  1996/06/04 16:44:49  cm
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
#include <signal.h>
#include <time.h>

/* Local include files; The following including order is important. We
   use local rmt_user_def.h and rss_def.h because they must be private 
   and localized. */
#include "rmtd_user_func_set.h"
#include "rmt_user_def.h"
#include <rmt.h>
#include <rss.h>
#include "rss_def.h"

/*** External references / external global variables ***/


/* Local references / local variables */
/* table for converting the signal numbers - this table is used by the client
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

static int Convert_signal_to_local (int global_signal);


/****************************************************************

    Description: This is the remote (server) function of the RPC
	implementation of "kill".

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to RSS_kill, rss.doc and rmt.doc.

*****************************************************************/

int rss_kill
(
    int len,		/* calling argument string length */
    char *arg,		/* calling argument string */
    char **ret_val	/* return string */

)
{
    int	ret;      /* returned value */
    int	pid;      /* process id */
    int	sig;      /* signal number */
    char *buffer;

    /* Retrieve the process id and signal number from client */
    arg[len - 1] = '\0';
    if (sscanf (arg, "%d %d", &pid, &sig) != 2)
	return (RSS_BAD_REQUEST_SERVER);

    sig = Convert_signal_to_local (sig);
    if (sig == RSS_FAILURE)
	return (RSS_INVALID_SIGNAL);

    /* Call kill function */
    ret = kill ((pid_t)pid, sig);

    /* return the returned value and errno */
    buffer = RSS_shared_buffer ();
    sprintf (buffer, "%d %d", ret, errno);
    *ret_val = buffer;

    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This is the remote (server) function of the RPC
	implementation of "time".

    Input:	len - length of string "arg" in number of bytes
		arg - RMT calling argument string

    Output:	ret_str - RMT user function return string

    Returns: The length of the return string on success or a 
	negative error number on failure.

    Notes: Refer to RSS_time, rss.doc and rmt.doc.

******************************************************************/

int rss_time
(
    int len,		/* calling argument string length */
    char *arg,		/* calling argument string */
    char **ret_val	/* return string */
) 
{
    time_t tt;
    char *buffer;

    if (len != 0)	/* The input argument length should be zero */
	return (RSS_BAD_REQUEST_SERVER);

    /* Get the current system time */
    tt = time (NULL);

    /* return the time */
    buffer = RSS_shared_buffer ();
    sprintf (buffer, "%ld", (long)tt);
    *ret_val = buffer;

    return (strlen (buffer) + 1);
}

/****************************************************************
			
    Description: This function converts a machine independent signal 
	to the local signal number.

    Input:	global_signal -  signal in machine independent format

    Globals: Signal_map.

    Returns: The local signal number on success or RSS_FAILURE on
	failure.

****************************************************************/

static int Convert_signal_to_local
(
 int global_signal
)
{
  int i;
  int sz;

  sz = sizeof(Signal_map) / sizeof(Signal_map_struct);
  for (i = 0; i < sz; i++) {
    if (global_signal == Signal_map [i].rss_sig)
      return (Signal_map [i].loc_sig);
  }

  return (RSS_FAILURE);
}

/****************************************************************
			
    Template functions for removed RPC functions.

******************************************************************/

int rss_unused6 (int len, char *arg, char **ret_val) {
    return (0);
}

/****************************************************************
			
    Template functions for removed RPC functions.

******************************************************************/

int rss_unused7 (int len, char *arg, char **ret_val) {
    return (0);
}

/****************************************************************
			
    Template functions for removed RPC functions.

******************************************************************/

int rss_unused8 (int len, char *arg, char **ret_val) {
    return (0);
}

/****************************************************************
			
    Template functions for removed RPC functions.

******************************************************************/

int rss_unused9 (int len, char *arg, char **ret_val) {
    return (0);
}

/****************************************************************
			
    Template functions for removed RPC functions.

******************************************************************/

int rss_unused12 (int len, char *arg, char **ret_val) {
    return (0);
}

