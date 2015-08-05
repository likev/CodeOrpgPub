/****************************************************************
		
    Module: le_test.c	
		
    Description: This is the test program for the LE module.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2000/03/24 21:02:02 $
 * $Id: le_test.c,v 1.13 2000/03/24 21:02:02 jing Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <stdlib.h>

#include <le.h>
#include <infr.h>


#define OLE_MEMORY (LE_CRITICAL | 1)
#define OLE_OS (LE_CRITICAL | 2)
#define OLE_TERM (1 << 8)

static void callback (en_t evtcd, void *msg, size_t msglen);
static void term_callback (int sig);

/********************************************************************
			
    Description: The main function.

********************************************************************/

void main (int argc, char **argv)
{
    int i;

    LE_instance (34); 
    LE_init (argc, argv); 

    EN_register (1300, callback);
    alarm (1);
    sigset (SIGALRM, callback);
    sigset (SIGTERM, term_callback);
    sigset (SIGINT, term_callback);

    i = 0;
    while (1) {
	LE_send_msg (OLE_OS, "This is a %s # %d", "test msg", i);
	i++;
	if (i >= 50)
	    exit (0);
    }

    LE_send_msg (1123, "");

    LE_send_msg (OLE_OS | OLE_TERM, NULL);

    LE_send_msg (OLE_MEMORY | OLE_TERM, "malloc failed (error = %d)\n\n", 34);

    for (i = 0 ; i < 3; i++)
	LE_send_msg (i, "This is a %s # %d", "test msg", i);

    for (i = 0 ; i < 5; i++) {
	LE_send_msg (0, "This is a %s", "test msg");
	msleep (10); 
    }
  
    for (i = 0 ; i < 50; i++) {
        LE_send_msg (OLE_OS | OLE_TERM, "This terminates %d", i);
    }
    LE_send_msg (OLE_MEMORY | OLE_TERM, "malloc failed (error = %d)", 34);

}

void tcallback ()
{
    printf ("In tcallback\n");
    exit (0);
}

/************************************************************************

    Description: The termination callback function.

************************************************************************/

static void term_callback (int sig)
{

    printf ("term_callback %d\n", sig);
    LE_send_msg (OLE_OS, "%s %d %d", "LE_send_msg in termination callback",
					999, 000);
    LE_terminate (NULL, 1);
    return;
}

/************************************************************************

    Description: The LE event callback function.

************************************************************************/

static void callback (en_t evtcd, void *msg, size_t msglen)
{
    LE_event_message *emsg;
    LE_critical_message *cmsg;
    int i;

    printf ("callback %d\n", evtcd);
    if (evtcd == SIGALRM)
	alarm (3);
    LE_send_msg (OLE_OS, "%s", "LE_send_msg in callback");
printf ("first done\n");
    LE_send_msg (OLE_OS, "%s", "LE_send_msg in callback A");
printf ("second done\n");
    return;

    printf ("event %d, len %d\n", evtcd, msglen);

    emsg = (LE_event_message *)msg;
    if (msglen < (int)sizeof (LE_event_message) ||
	emsg->le_msg_off < (int)sizeof (LE_event_message) ||
	emsg->le_msg_off + (int)sizeof (LE_critical_message) > msglen) {
	printf ("event msg length error\n");
printf ("%d %d %d %d\n", msglen, sizeof (LE_event_message), emsg->le_msg_off, sizeof (LE_critical_message));
	return;
    }

    printf ("event msg: id %d, le_msg_off %d, sender %s@\n", emsg->id, emsg->le_msg_off, emsg->sender);

    cmsg = (LE_critical_message *)((char *)msg + emsg->le_msg_off);
    printf ("cri msg: code %x, time %d, pid %d, n_reps %d, line_num %d, fname %s, text %s@\n", cmsg->code, (int)cmsg->time, cmsg->pid, cmsg->n_reps, cmsg->line_num, cmsg->fname, cmsg->text);
}


