
/******************************************************************

    Description: This module contains the functions for estimating
	the achieved data rate. Recent message transmission times 
	are recorded along with their sizes for each comms link. 
	These are then used for estimating the achieved data rate.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 1997/12/19 23:36:05 $
 * $Id: cmu_rate.c,v 1.5 1997/12/19 23:36:05 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>
#include "cmu_def.h"


#define MAXN_MSG_ITEMS 16	/* max number of msg items in a link record */

#define MIN_MSG_SIZE 4000	/* minimum message size to be used for 
				   estimating rates */
#define TIME_WINDOW 60		/* time window for estimating rates */

typedef struct {		/* a message item */
    int bytes;			/* message size; number of bytes */
    int delta;			/* time (us) used for transmitting the msg */
    time_t t;			/* the message time (unix seconds) */
} Msg_item;

typedef struct {		/* message record for each link */
    int link_ind;		/* link index */
    int us;			/* current msg time; microseconds */
    int st_ind;			/* index of the first entry in msgs array */
    int n_msgs;			/* number of messages in msgs array */
    int cr_ind;			/* current message index in msgs; -1 indicates
				   no message writing is going on. */
    Msg_item msgs[MAXN_MSG_ITEMS];
				/* array for storing the messages */
    void *next;			/* pointer to the next Msg_record structure */
} Msg_record;

static int N_links = 0;
static Msg_record *Record;

static Msg_record *Get_record (Link_struct *link);


/**************************************************************************

    Description: This function returns the estimated achieved data rate.

    Inputs:	link - the link involved;

    Return:	The estimated achieved percentage rate on success or
		CM_SF_UNAVAILABLE on failure.

**************************************************************************/

int RATE_get_rate (Link_struct *link)
{
    Msg_record *rec;
    int delta, bytes, i, ind;
    time_t ct;
    double rate;

    if ((rec = Get_record (link)) == NULL)
	return (CM_SF_UNAVAILABLE);

    bytes = delta = 0;
    ind = rec->st_ind;
    ct = time (NULL);
    for (i = 0; i < rec->n_msgs; i++) {
	if (ct - rec->msgs[ind].t <= TIME_WINDOW) {
	    bytes += rec->msgs[ind].bytes;
	    delta += rec->msgs[ind].delta;
	}
	ind = (ind + 1) % MAXN_MSG_ITEMS;
    }

    if (delta <= 0 || link->line_rate <= 0)
	return (CM_SF_UNAVAILABLE);

    rate = (double)bytes * 800. * 1000000. / 
				((double)delta * (double)link->line_rate);
    return ((int)(rate + .5));
}

/**************************************************************************

    Description: This function resets the record for the link.

    Inputs:	link - the link involved;

**************************************************************************/

void RATE_reset (Link_struct *link)
{
    Msg_record *rec;

    if ((rec = Get_record (link)) == NULL)
	return;
    rec->n_msgs = rec->st_ind = 0;
    rec->cr_ind = -1;
    return;
}

/**************************************************************************

    Description: This function is called when a message transmission starts.

    Inputs:	link - the link involved;
		bytes - number of bytes of the message;

**************************************************************************/

void RATE_start_write (Link_struct *link, int bytes)
{
    Msg_record *rec;
    struct timeval tp;

    if ((rec = Get_record (link)) == NULL)
	return;
    if (bytes < MIN_MSG_SIZE) {
	rec->cr_ind = -2;
	return;
    }
    rec->cr_ind = (rec->st_ind + rec->n_msgs) % MAXN_MSG_ITEMS;
    if (rec->cr_ind == rec->st_ind) {
	rec->st_ind = (rec->st_ind + 1) % MAXN_MSG_ITEMS;
	rec->n_msgs--;
    }
    rec->msgs[rec->cr_ind].bytes = bytes;
    gettimeofday (&tp, NULL);
    rec->msgs[rec->cr_ind].t = tp.tv_sec;
    rec->us = tp.tv_usec;

    return;
}

/**************************************************************************

    Description: This function is called when a message transmission is 
		completed.

    Inputs:	link - the link involved;

**************************************************************************/

void RATE_write_done (Link_struct *link)
{
    Msg_record *rec;
    struct timeval tp;
    int d;

    if ((rec = Get_record (link)) == NULL)
	return;
    if (rec->cr_ind == -2) {
	rec->cr_ind = -1;
	return;
    }
    if (rec->cr_ind < 0)
	return;

    gettimeofday (&tp, NULL);
    d = (tp.tv_sec - rec->msgs[rec->cr_ind].t) * 1000000 + 
						tp.tv_usec - rec->us;
    if (d <= 0) {
	rec->cr_ind = -1;
	return;
    }
    rec->msgs[rec->cr_ind].delta = d;
    rec->n_msgs++;
    rec->cr_ind = -1;

    return;
}

/**************************************************************************

    Description: This function returns the record structure for "link".

    Inputs:	link - the link involved;

    Return:	The pointer to the record on success or NULL on failure.

**************************************************************************/

static Msg_record *Get_record (Link_struct *link)
{
    Msg_record *rec, *last, *new;
    int i;

    rec = Record;
    for (i = 0; i < N_links; i++) {
	if (rec->link_ind == link->link_ind)
	    return (rec);
	last = rec;
	rec = (Msg_record *)rec->next;
    }

    new = (Msg_record *)malloc (sizeof (Msg_record));
    if (new == NULL)
	return (NULL);
    if (N_links == 0)
	Record = new;
    else
	last->next = (void *)new;
    N_links++;

    new->n_msgs = new->st_ind = 0;
    new->cr_ind = -1;

    return (new);
}










