/****************************************************************
		
	File: sv_register.c	
				
	2/24/94
	Modified: 2/17/99, Venkat Ganti

	Purpose: This module processes server registration for
	the RMT client. It maintains a connected remote host
	table to ensure that there is only one connection is
	built for each remote host. This module also processes
	connection locking.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:59 $
 * $Id: rmt_sv_register.c,v 1.11 2012/06/14 18:57:59 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <unistd.h>


/*** Local include files ***/

#include <rmt.h>
#include <net.h>
#include "rmt_def.h"
#include <misc.h>

#ifdef THREADED
#include <pthread.h>
#endif

/*** Definitions / macros / types ***/

#define MAX_NUM_RHOSTS  32	/* maximum number of remote hosts */
#define RHOSTS_TBL_MIN  3	/* minimum of rhosts in the rhosts table */

#ifdef THREADED
typedef struct {		/* structure for fd registration */
    int fd;
    pthread_mutex_t mt;
} fd_regist;
static pthread_mutex_t Fd_reg_mt = PTHREAD_MUTEX_INITIALIZER;
static void *FdtblId = NULL;
#endif

typedef struct {		/* structure for host registration */
    int fd;			/* socket fd */
    unsigned int ipa;		/* IP address */
    int n_names;		/* number of host names */
    char *name;			/* host names */
} server_regist;


/*** External references / external global variables ***/

/*** Local references / local variables ***/

#ifndef THREADED
static void *TblId=NULL;
#else
#include <pthread.h>
static pthread_key_t  Sv_tbl_key;
static pthread_once_t Sv_tbl_key_init_once = {PTHREAD_ONCE_INIT};
static void Sv_tbl_key_init_func (void);
#endif

static int Store_sv_tbl_id (void *value);
static void *Get_sv_tbl_id (void);


#ifdef THREADED

/****************************************************************/

static void Sv_tbl_key_init_func (void)
{
    int status;
    status = pthread_key_create (&Sv_tbl_key, MISC_free_table);
    if (status != 0)
        MISC_log ("RMT: pthread_key_create failed\n");
}

/*********************************************************************

    Locks ("lock" != 0) /unlocks ("lock" == 0) socket "fd" to a remote 
    RPC server so other thread cannot use it simultaneousely.

*********************************************************************/

void RMT_mt_lock_sv_fd (int fd, int lock) {
    int num_fds, i;
    fd_regist *tblPtr;
    pthread_mutex_t *mt;

    pthread_mutex_lock (&Fd_reg_mt);
    mt = NULL;
    if ((tblPtr = (fd_regist *)MISC_get_table (FdtblId, &num_fds)) != NULL) {
	for (i = 0; i < num_fds; i++) {
	    if (fd == tblPtr[i].fd) {
		mt = &(tblPtr[i].mt);
		break;
	    }
	}
    }
    pthread_mutex_unlock (&Fd_reg_mt);
    if (mt == NULL)
	return;
    if (lock)
	pthread_mutex_lock (mt);
    else
	pthread_mutex_unlock (mt);

    return;
}

/***********************************************************************

    Checks if "fd" is a valid socket fd connecting to server. Return true
    or false.

***********************************************************************/

int RMT_is_fd_valid (int fd) {
    int num_fds, i;
    fd_regist *tblPtr;

    if ((tblPtr = (fd_regist *)MISC_get_table (FdtblId, &num_fds)) == NULL)
	return (0);

    for (i = 0; i < num_fds; i++) {
	if (fd == tblPtr[i].fd)
            return (1);
    }
    return (0);
}

/***********************************************************************

    Checks if "fd" is a valid socket fd connecting to server created from
    this thread. Return true or false.

***********************************************************************/

int RMT_is_fd_of_this_thread (int fd) {
    if (SVRG_get_server_by_fd (fd) == NULL)
	return (0);
    return (1);
}

#endif

/****************************************************************/
static void *Get_sv_tbl_id (void)
{
#ifdef THREADED
    pthread_once (&Sv_tbl_key_init_once, Sv_tbl_key_init_func);
    return (pthread_getspecific (Sv_tbl_key));
#else
    return (TblId);
#endif
}

/****************************************************************/
static int Store_sv_tbl_id (void *value)
{
#ifdef THREADED
    return (pthread_setspecific (Sv_tbl_key, (void*)value));
#else
    TblId = value;
    return (0);
#endif
}

/****************************************************************/
void *get_tblPtr (int *Num_rhosts)
{
    void *tblPtr;

    void *tblId;
    tblId = Get_sv_tbl_id ();

    if (tblId == NULL){
	return (NULL);
    }
    tblPtr = MISC_get_table (tblId, Num_rhosts);
    if (tblPtr == NULL){
	MISC_log ("RMT: MISC_get_table failed\n");
	return (NULL);
    }
    return (tblPtr);
}
/****************************************************************
			
	SVRG_get_fd_by_name()			Date: 2/24/94

	This function looks up the socket fd of a remote host
	"mach_name". 

	It returns the fd if it is found, or FAILURE if the host
	name is not registered.
*/

int
  SVRG_get_fd_by_name
  (
      char *mach_name		/* remote host name */
) {
    int Num_rhosts, i, k;
    server_regist *tblPtr;
    unsigned int ip;
    char *name;

    if ((tblPtr = (server_regist *)get_tblPtr (&Num_rhosts)) == NULL)
	return (FAILURE);
		
    for (i = 0; i < Num_rhosts; i++) {
	name = tblPtr[i].name;
	for (k = 0; k < tblPtr[i].n_names; k++) {
	    if (k > 0)
		name += strlen (name) + 1;
	    if (strcmp (mach_name, name) == 0)
        	return (tblPtr[i].fd);
	}
    }
    ip = NET_get_ip_by_name (mach_name);
    for (i = 0; i < Num_rhosts; i++) {
	if (tblPtr[i].ipa == ip) {
	    char *p;
	    int len = 0;
	    name = tblPtr[i].name;
	    for (k = 0; k < tblPtr[i].n_names; k++) {
		len += strlen (name) + 1;
		name += len;
	    }
	    p = MISC_malloc (len + strlen (mach_name) + 1);
	    memcpy (p, tblPtr[i].name, len);
	    strcpy (p + len, mach_name);
	    MISC_free (tblPtr[i].name);
	    tblPtr[i].name = p;
	    tblPtr[i].n_names++;
	    return (tblPtr[i].fd);
	}
    }

    return (FAILURE);
}

/****************************************************************
			
	SVRG_get_server_by_fd ()		Date: 2/17/99

	This function looks up the host name of a remote host
	connected through socket "fd". It returns only the first
	name if the connection has more than one name.

	It returns the pointer to the host name if the connection 
	to remote host is opened and registered. Otherwise it
	returns NULL.
*/

char *
  SVRG_get_server_by_fd
  (
      int fd			/* socket fd */
) {

    int Num_rhosts,i;
    server_regist *tblPtr;

    if ((tblPtr = (server_regist *)get_tblPtr (&Num_rhosts)) == NULL)
	return (NULL);

    for (i = 0; i < Num_rhosts; i++) {
	if (fd == tblPtr[i].fd)
            return (tblPtr[i].name);
    }
    return (NULL);
}

/****************************************************************
			
	SVRG_regist_new_server()		Date: 2/24/94

	This function adds a new item to the remote host table.
	The new host name, IP address and socket fd are registered 
	in the table and the lock count is set. One connection can
	have multiple items in the table because of aliased host
	names (e.g. the formal name and the IP address).

	The implementation uses a predefined host table size. If
	there are too many remote hosts connected, the table will be
	full and no new remote host can be connected.

	It returns SUCCESS on success or FAILURE on failure.
*/

int
  SVRG_regist_new_server
  (
      int fd,			/* socket fd */
      char *mach_name,		/* remote host name */
      unsigned long ipa		/* IP address of the remote host */
) {
    int Num_rhosts, index;
    server_regist *newEnt, *tblPtr;
    void *tblId;

    tblId = Get_sv_tbl_id ();

    if (tblId == NULL){
	tblId = MISC_create_table (sizeof(server_regist), RHOSTS_TBL_MIN);
	if (tblId == NULL){
	    MISC_log ("RMT: MISC_create_table failed\n");
	    return (FAILURE);
	}
    }

    newEnt = (server_regist *)MISC_table_new_entry (tblId,&index);
    if (newEnt == NULL){
	MISC_log ("RMT: MISC_table_new_entry failed\n");
	return (FAILURE);
    }
    newEnt->name = MISC_malloc (strlen (mach_name) + 1);
    strcpy (newEnt->name, mach_name);
    newEnt->n_names = 1;
    newEnt->fd = fd;
    newEnt->ipa = ipa;
    tblPtr = (server_regist *)MISC_get_table (tblId,&Num_rhosts);
    if (tblPtr == NULL){
	MISC_log ("RMT: MISC_get_table failed\n");
	return (FAILURE);
    }

    if (Store_sv_tbl_id (tblId) != 0 ) {
	MISC_log ("RMT: pthread_setspecific failed\n");
	return (FAILURE);
    }

#ifdef THREADED
    {
	fd_regist *newfd;
	pthread_mutex_lock (&Fd_reg_mt);
	if (FdtblId == NULL){
	    FdtblId = MISC_create_table (sizeof (fd_regist), 16);
	    if (FdtblId == NULL) {
		MISC_log ("RMT: MISC_create_table failed\n");
		pthread_mutex_unlock (&Fd_reg_mt);
		return (FAILURE);
	    }
	}
	newfd = (fd_regist *)MISC_table_new_entry (FdtblId, &index);
	if (newfd == NULL) {
	    MISC_log ("RMT: MISC_table_new_entry failed\n");
	    pthread_mutex_unlock (&Fd_reg_mt);
	    return (FAILURE);
	}
	newfd->fd = fd;
	memset ((char *)(&(newfd->mt)), 0, sizeof (pthread_mutex_t));
	if (pthread_mutex_init (&(newfd->mt), NULL) != 0) {
	    MISC_log ("RMT: mutex_init failed\n");
	    pthread_mutex_unlock (&Fd_reg_mt);
	    return (FAILURE);
	}
	pthread_mutex_unlock (&Fd_reg_mt);
    }
#endif

    return (SUCCESS);

}

/****************************************************************
			
	SVRG_remove_a_server()			Date: 2/24/94

	This function removes an connection from the remote host 
	registration table. All items in the table corresponding 
	to the connection are removed.
*/

void
  SVRG_remove_a_server 
  (
      int fd	                /* socket fd */
  )
{
    int Num_rhosts,i;
    server_regist *tblPtr;
    void *tblId;

    if ((tblPtr = (server_regist *)get_tblPtr(&Num_rhosts)) == NULL)
	return ;

    tblId = Get_sv_tbl_id ();
    if (tblId == NULL)
	return;
    for (i = Num_rhosts - 1; i >= 0; i--) {
	if (tblPtr[i].fd == fd) {
	    MISC_free (tblPtr[i].name);
	    MISC_table_free_entry(tblId,i);
	    /* the following is needed to update the */
	    /* table once an entry is deleted */
	    tblPtr = (server_regist *)MISC_get_table (tblId, &Num_rhosts);
	    if (tblPtr == NULL){
		MISC_log ("RMT: MISC_get_table failed\n");
		return ;
	    }
	}
    }

#ifdef THREADED
    {
	fd_regist *fdPtr;
	int num_fds;

	pthread_mutex_lock (&Fd_reg_mt);
	fdPtr = (fd_regist *)MISC_get_table (FdtblId, &num_fds);
	if (fdPtr == NULL) {
	    pthread_mutex_unlock (&Fd_reg_mt);
	    return;
	}
    
	for (i = num_fds - 1; i >= 0; i--) {
	    if (fdPtr[i].fd == fd) {
		pthread_mutex_destroy (&(fdPtr[i].mt));
		MISC_table_free_entry (FdtblId, i);
		/* the following is needed to update the */
		/* table once an entry is deleted */
		fdPtr = MISC_get_table (FdtblId, &num_fds);
		if (fdPtr == NULL) {
		    MISC_log ("RMT: MISC_get_table failed\n");
		    break;
		}
	    }
	}
	pthread_mutex_unlock (&Fd_reg_mt);
    }
#endif

    return ;
}

/****************************************************************

	SVRG_check_ip ()

	This function checks if an IP address is already used by
	an existing connection. It returns FAILURE if the address
	is not connected. Otherwise it returns the fd for that
	connection.
*/

int SVRG_check_ip (unsigned long ipa)
{
    int Num_rhosts,i;
    server_regist *tblPtr;

    if ((tblPtr = (server_regist *)get_tblPtr (&Num_rhosts)) == NULL)
	return (FAILURE);

    for (i = 0; i < Num_rhosts; i++) {
	if (ipa == tblPtr[i].ipa) {
            return (tblPtr[i].fd);
        }
    }
    return (FAILURE);
}

/****************************************************************

    Tests fd "test_fd" to find out if it is disconnected. It it
    is disconnected, calls "cb" to pass the fd and the host name.
    If "test_fd" is negative, tests all fds.

****************************************************************/

int SVRG_test_lost_conns (int test_fd, void (*cb) (int, char *)) {
    int n_rhosts, fd, k, i, ret;
    server_regist *tblPtr;
    char buf[128];

    if ((tblPtr = (server_regist *)get_tblPtr (&n_rhosts)) == NULL)
	return (FAILURE);

    for (i = 0; i < n_rhosts; i++) {
	fd = tblPtr[i].fd;
	if (test_fd >= 0 && fd != test_fd)
	    continue;
	for (k = 0; k < i; k++) {
	    if (tblPtr[k].fd == fd)
		break;
	}
	if (k < i)		/* the same fd */
	    continue;
	while ((ret = read (fd, buf, 128)) > 0);
	if (ret == 0)			/* socket disconnected */
	    cb (tblPtr[i].fd, tblPtr[i].name);
    }
    return (SUCCESS);
}
