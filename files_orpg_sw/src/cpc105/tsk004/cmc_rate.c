
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
 * $Date: 2007/02/14 21:43:43 $
 * $Id: cmc_rate.c,v 1.12 2007/02/14 21:43:43 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmc_def.h>

#define MAXN_MSG_ITEMS 36	/* max number of msg items in a link record */

#define MIN_MSG_SIZE 1000	/* minimum message size to be used for 
				   estimating rates */
#define TIME_WINDOW 300		/* time window for estimating rates */

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

    Return:	The estimated achieved rate (in Bytes / s) on success or
		CM_SF_UNAVAILABLE on failure.

**************************************************************************/

int CMRATE_get_rate (Link_struct *link)
{
    Msg_record *rec;
    int delta, bytes, i, ind;
    time_t ct;
    double rate;

    if ((rec = Get_record (link)) == NULL)
	return (CM_SF_UNAVAILABLE);

    bytes = delta = 0;
    ind = rec->st_ind;
    ct = MISC_systime (NULL);
    for (i = 0; i < rec->n_msgs; i++) {
	if (ct - rec->msgs[ind].t <= TIME_WINDOW) {
	    bytes += rec->msgs[ind].bytes;
	    delta += rec->msgs[ind].delta;
	}
	ind = (ind + 1) % MAXN_MSG_ITEMS;
    }

    if (delta <= 0)
	return (CM_SF_UNAVAILABLE);

    rate = (double)bytes * 1000000. / (double)delta;
    return ((int)(rate + .5));
}

/**************************************************************************

    Description: This function resets the record for the link.

    Inputs:	link - the link involved;

**************************************************************************/

void CMRATE_reset (Link_struct *link)
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

void CMRATE_start_write (Link_struct *link, int bytes)
{
    Msg_record *rec;
    time_t t;
    int ms;

    if ((rec = Get_record (link)) == NULL)
	return;
    if (bytes < MIN_MSG_SIZE) {
	rec->cr_ind = -2;
	return;
    }
    rec->cr_ind = (rec->st_ind + rec->n_msgs) % MAXN_MSG_ITEMS;
    if (rec->n_msgs > 0 && rec->cr_ind == rec->st_ind) {
	rec->st_ind = (rec->st_ind + 1) % MAXN_MSG_ITEMS;
	rec->n_msgs--;
    }
    if (strcmp (CMC_get_proto_name (), "cm_uconx") == 0) {
	int last_packet;
	last_packet = bytes % link->packet_size;
	if (last_packet == 0)
	    last_packet = link->packet_size;
	if (bytes > last_packet)
	    rec->msgs[rec->cr_ind].bytes = bytes - last_packet;
	else
	    rec->msgs[rec->cr_ind].bytes = 0;
    }
    else 
	rec->msgs[rec->cr_ind].bytes = bytes;

    t = MISC_systime (&ms);
    rec->msgs[rec->cr_ind].t = t;
    rec->us = ms * 1000;

    return;
}

/**************************************************************************

    Description: This function is called when a message transmission is 
		completed.

    Inputs:	link - the link involved;

**************************************************************************/

void CMRATE_write_done (Link_struct *link)
{
    Msg_record *rec;
    int d, ms;
    time_t t;

    if ((rec = Get_record (link)) == NULL)
	return;
    if (rec->cr_ind == -2) {
	rec->cr_ind = -1;
	return;
    }
    if (rec->cr_ind < 0)
	return;

    t = MISC_systime (&ms);
    d = (t - rec->msgs[rec->cr_ind].t) * 1000000 + 
					ms * 1000. - rec->us;
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
    Msg_record *rec, *last;
    int i;

    rec = Record;
    for (i = 0; i < N_links; i++) {
	if (rec->link_ind == link->link_ind)
	    return (rec);
	last = rec;
	rec = (Msg_record *)rec->next;
    }
    return (NULL);
}


/**************************************************************************

    Description: Initialize the Msg_record array.

    Inputs:	link - the link involved;

    Return:	0 on success or -1 on failure.

**************************************************************************/

int CMRATE_init (Link_struct *link)
{
    Msg_record *rec, *last, *new;
    int i;

    last = NULL; 
    rec = Record;
    for (i = 0; i < N_links; i++) {
	if (rec->link_ind == link->link_ind)
	    return (0);
	last = rec;
	rec = (Msg_record *)rec->next;
    }

    new = (Msg_record *)malloc (sizeof (Msg_record));
    if (new == NULL)
	return (-1);
    if (N_links == 0)
	Record = new;
    else
	last->next = (void *)new;
    N_links++;

    new->link_ind = link->link_ind;
    new->n_msgs = new->st_ind = 0;
    new->cr_ind = -1;

    return (0);
}









