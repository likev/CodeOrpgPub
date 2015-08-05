/****************************************************************
		
    Module: rss_shared.c	
				
    Description: This module contains common internal functions for
	the module RSS client functions.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 21:32:01 $
 * $Id: rss_shared.c,v 1.29 2012/07/27 21:32:01 jing Exp $
 * $Revision: 1.29 $
 * $State: Exp $
 * $Log: rss_shared.c,v $
 * Revision 1.29  2012/07/27 21:32:01  jing
 * Update
 *
 * Revision 1.22  2002/05/20 20:35:16  jing
 * Update
 *
 * Revision 1.21  1999/05/04 22:46:24  eforren
 * Reverted from version 1.19
 *
 * Revision 1.19  1999/04/19 17:16:35  vganti
 * Added functions to store fd and file information for threads
 *
 * Revision 1.18  1998/12/01 21:17:52  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.15  1998/06/19 17:05:32  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/26 15:59:22  cm
 * SunOS 5.5 modifications
 *
*/

/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*** Local include files ***/
#include <rss.h>
#include "rss_def.h"
#include <rmt.h>
#include <net.h>
#include <misc.h>


/*** Definitions / macros / types ***/
#define STRING_SIZE 256 	/* size of the string processed */
#define FD_TBL_MIN 10		/* size of the string processed */

typedef struct {                /* structure for lb/host registration */
    int fd;                     /* combined fd of the file and the socket*/
    char name[NAME_SIZE];       /* lb/file name */
} publishFd;

static int Form_explicit_local_path (char *path, int size);

/*  for maintaining publishFd table */
static void *tblId=NULL;	
static publishFd *entryList;
static int numEntries=0;

#ifdef THREADED
	#include <pthread.h>
	pthread_mutex_t publishFdMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


/****************************************************************

    Description: This function parses the extended path "name" to
	find the host name and the path (file) name. If the host
	name is local and explicitly specified, $(HOME) is added
	in front of the path.

    Input:	name - the extended path
		size - size of the buffers host_name and path

    Output:	host_name - the host name; This is returned only if
		    the host name is explicitly specified in "name".
		path - the path

    Returns:	REMOTE_HOST if the host name is a remote host;
		LOCAL_HOST_EXPLICIT if the host is local and its name
		    is explicitly specified;
		LOCAL_HOST_IMPLICIT if the host is local and its name
		    is not specified;
		RSS_FAILURE if the host can not be determined.

    Notes: For a description of the extended path, refer to rss.doc.
	If the length of the string in "name" is too long in terms of
	"size", "name" will be truncated.
*/

int
  RSS_find_host_name
  (
      const char *name,		/* extended path name */
      char *host_name,		/* host name */
      char *path,		/* path name */
      int size
) {
    unsigned int lip;		/* local host inet address */
    int n, i;
    int colon_cnt;		/* number of ':' found in the extended path */
    char *hn;

    /* remove leading spaces */
    while (*name == ' ') name++;

    strncpy (host_name, name, size);
    host_name[size - 1] = '\0';

    i = 0;
    colon_cnt = 0;
    strncpy (path, name, size);
    while (host_name[i] != '\0' && host_name[i] != '/') {
	if (host_name[i] == ':' && host_name[i+1] != '\\') {
	    host_name[i] = '\0';
	    strncpy (path, host_name + i + 1, size);
            colon_cnt++;
	}
	i++;
    }
    path [size - 1] = '\0';
    
    if (colon_cnt == 0)
	return (LOCAL_HOST_IMPLICIT);

    /* check if it is a local host name */
    lip = INADDR_NONE;
    RMT_lookup_host_index (RMT_LHI_IX2I, &lip, 0);
    hn = host_name;
    for (n = 0; n < colon_cnt; n++) {
	unsigned long addr;         /* inet address of the name */
	addr = NET_get_ip_by_name (hn);
	if (addr == INADDR_NONE)
	    return (RSS_FAILURE);
	if (addr == lip)
	    break;
	hn += strlen (hn) + 1;
    }

    if (n < colon_cnt) {		/* explicit local host */
	if (Form_explicit_local_path (path, size) == RSS_FAILURE)
	    return (RSS_FAILURE);
	return (LOCAL_HOST_EXPLICIT);
    }

    return (REMOTE_HOST);
}

static int is_absolute_path (const char *path) {
   int is_absolute;

   is_absolute = (path[0] == '/' || path[0] == '$');

#ifdef __WIN32__
   if (!is_absolute)
      is_absolute = isalpha(path[0]) && path[1]==':' && path[2]=='\\';
#endif

   return is_absolute;
}

/****************************************************************

    Description: This function generates a full path by adding the 
	$HOME part if the path name is incomplete. The original 
	"path" is replaced by the complete path on success.

    Input:	path - the path to be processed
		size - size of buffer "path"

    Output:	path - the processed full path

    Returns: This function returns RSS_FAILURE if the $HOME 
	environment variable is not defined. It returns RSS_SUCCESS 
	otherwise.

****************************************************************/

static int
  Form_explicit_local_path
  (
    char *path,		/* input/output path */
    int size		/* length of the path buffer */
) {
    int len;

    if (!is_absolute_path (path)) {
        char *home;
	char tmp [TMP_SIZE];

	home = getenv ("HOME");
	if (home == NULL)
	    return (RSS_FAILURE);
        strncpy (tmp, home, TMP_SIZE);
	tmp[TMP_SIZE - 3] = '\0';
	if (tmp[strlen (tmp) - 1] == '/') 
	    tmp[strlen (tmp) - 1] = '\0';
        strcat (tmp, "/");
	len = strlen (tmp);
	strncpy (tmp + len, path, size - len - 1);
	tmp[TMP_SIZE - 1] = '\0';
	strncpy (path, tmp, size);
	path [size - 1] = '\0';
    }
    return (RSS_SUCCESS);
}

/***********************************************************************
	stores information about the lb/file and the correspondig combined
	fd of the socked and the lb/file
***********************************************************************/
int RSS_storePublishFd(char *name,int fd)
{
publishFd *newEnt;
int index;
#ifdef THREADED
	pthread_mutex_lock (&publishFdMutex);
#endif

	while (tblId == NULL){
                tblId = MISC_open_table (
                sizeof(publishFd),FD_TBL_MIN,0,&numEntries,(char **)&entryList);
                if (entryList == NULL)
                        msleep(100);
        }

        newEnt = (publishFd *)MISC_table_new_entry(tblId,&index);
        if (newEnt == NULL){
                MISC_log ("RSS_publish_fd:Error in MISC_table_new_entry \n");
		#ifdef THREADED
			pthread_mutex_unlock (&publishFdMutex);
		#endif
                return (RSS_FAILURE);
        }
        memset(newEnt,0,sizeof(*newEnt));
        strncpy (newEnt->name, name, NAME_SIZE);
        newEnt->name[NAME_SIZE - 1] = '\0';
        newEnt->fd = fd;
#ifdef THREADED
	pthread_mutex_unlock (&publishFdMutex);
#endif
    return (RSS_SUCCESS);
}

/**********************************************************************
	Retrieves the actual connection fd for this thread on the remote
	host
**********************************************************************/
int RSS_getPublishedFd(int fd)
{
int i;
char name[TMP_SIZE];   /* host name */
        if (tblId == NULL)
		return (RSS_FAILURE);
#ifdef THREADED
	pthread_mutex_lock (&publishFdMutex);
#endif
	for (i = 0; i < numEntries; i++) {
		if (fd == entryList[i].fd){
			strcpy(name,entryList[i].name);
			#ifdef THREADED
				pthread_mutex_unlock (&publishFdMutex);
			#endif
			/* make a connection for this thread */
			return (RSS_getCombinedFd(name,fd));
		}
	}
	#ifdef THREADED
		pthread_mutex_unlock (&publishFdMutex);
	#endif
	MISC_log ("RSS_getPublishedFd:No matching host name for the fd %d\n",fd);
	return (RSS_FAILURE);
}
/**********************************************************************
	deletes the publishedfd information from the table.
**********************************************************************/
int RSS_deletePublishedFd(int fd)
{
int i;
        if (tblId == NULL)
		return (RSS_FAILURE);
#ifdef THREADED
	pthread_mutex_lock (&publishFdMutex);
#endif
	for (i = numEntries - 1; i >= 0; i--) {
		if (fd == entryList[i].fd){
			MISC_table_free_entry(tblId,i);
			if (numEntries == 0)
				MISC_free_table(tblId);
			#ifdef THREADED
				pthread_mutex_unlock (&publishFdMutex);
			#endif
			return (RSS_SUCCESS);
		}
	}
#ifdef THREADED
	pthread_mutex_unlock (&publishFdMutex);
#endif
	MISC_log ("RSS_deletePublishedFd:No matching host name for the fd %d\n",fd);
	return (RSS_FAILURE);
}

/**********************************************************************
Retrieves the actual connection fd for this thread on the remote
host. For this interface, the client supplies both the lb name (including the path)
and the combined fd.  So there is no need to store the information
**********************************************************************/
int RSS_getCombinedFd(char *name,int fd)
{
int sockFd,ret;
char host_name[TMP_SIZE];   /* host name */
char path[TMP_SIZE];        /* path name */
	if (fd <= 0)
		return (fd);
	ret = RSS_find_host_name (name, host_name, path, TMP_SIZE);
	if (ret == RSS_FAILURE)     /* can not find host name */
        	return (RSS_HOSTNAME_FAILED);

	if (ret != REMOTE_HOST)     /* local host */
        	return (fd);
	sockFd = RMT_create_connection(host_name);
	if (sockFd < 0)
		return (sockFd);
	return (COMBINE_FD (sockFd, GET_FFD(fd)));
}
