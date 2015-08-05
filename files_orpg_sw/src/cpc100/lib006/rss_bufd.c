/****************************************************************
		
    Module: rss_bufd.c	
				
    Description: This module contains function RSS_shared_buffer.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:54:58 $
 * $Id: rss_bufd.c,v 1.21 2004/05/27 16:54:58 jing Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 * Revision 1.1  1996/06/04 16:44:24  cm
 * Initial revision
 *
 * 
*/

/* System include files */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <rmt.h>
#include <misc.h>
#include "rss_def.h"

#ifdef THREADED
#include <pthread.h>
 
static pthread_key_t  Shared_buffer_key;
static pthread_once_t Shared_buffer_key_init_once = {PTHREAD_ONCE_INIT};
static void Shared_buffer_key_init_func (void);
#endif


/****************************************************************
			
    This function is removed. 

****************************************************************/

void RSS_set_send_log (void (*send_log)())
{
}

/****************************************************************
			
    Description: This function returns a shared buffer space of
		size STATIC_BUFFER_SIZE. 

    Returns: The pointer to the buffer or NULL on failure.

****************************************************************/

char *RSS_shared_buffer ()
{
#ifndef THREADED
    static ALIGNED_t buffer [ALIGNED_T_SIZE (STATIC_BUFFER_SIZE)];
#else
    char *buffer;
    pthread_once (&Shared_buffer_key_init_once, Shared_buffer_key_init_func);
    buffer = (char *)pthread_getspecific (Shared_buffer_key);
    if (buffer == NULL) {
	buffer = (char *)malloc (STATIC_BUFFER_SIZE);
	pthread_setspecific (Shared_buffer_key, (void*)buffer);
    }
#endif

    return ((char *)buffer);
}

#ifdef THREADED

static void Shared_buffer_key_init_func (void)
{
    int status;

    status = 0;
    while ((status = pthread_key_create (&Shared_buffer_key, free)) != 0) {
	if (status == ENOMEM) {
	    msleep (200);
	    continue;
	}
	MISC_log ("pthread_key_create failed (ret %d)\n", status);
	exit (1);
    }
}
#endif

