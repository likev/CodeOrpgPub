
/******************************************************************

	file: psv_manage_timers.c

	This module manages all timers.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/15 16:52:57 $
 * $Id: psv_manage_timers.c,v 1.41 2007/02/15 16:52:57 jing Exp $
 * $Revision: 1.41 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <orpg.h>
#include <orpgerr.h>

#include "psv_def.h"


typedef struct {		/* local part of the User_struct */
    int timer[MAX_N_TIMERS];	/* recoding the time to go; 0 means expired 
				   and -1 means processed */
} Mt_local;

static time_t Cr_time;		/* current time */
static time_t Next_exp_time;	/* next expiration time */
static time_t Last_timer_update;
				/* time of last timer update */
static int N_expired;		/* number of timers expired */

static int N_users;		/* number of users (links) */
static User_struct **Users;	/* the user structure list */


static int Get_time_event (int *user_ind);

/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int MT_initialize (int n_users, User_struct **users)
{
    int i;

    N_users = n_users;
    Users = users;

    /* allocate local data structure */
    Cr_time = MISC_systime (NULL);
    for (i = 0; i < n_users; i++) {
	Mt_local *mt;
	int k;

	mt = malloc (sizeof (Mt_local));
	if (mt == NULL) {
	    LE_send_msg (GL_ERROR | 77,  "malloc failed");
	    return (-1);
	}
	users[i]->mt = mt;
	for (k = 0; k < MAX_N_TIMERS; k++)
	    mt->timer[k] = -1;
	Users[i]->time = Cr_time;
    }
    Next_exp_time = 0;
    N_expired = 0;
    Last_timer_update = Cr_time;

    return (0);
}

/**************************************************************************

    Description: This function sets a timer. The previous set of the timer
		is canceled.

    Inputs:	usr - the user involved.
		timer - the timer name (macro).
		period - time period, in seconds, to set.

**************************************************************************/

void MT_set_timer (User_struct *usr, int timer, int period)
{
    int *tm_ptr;

    if (timer < FIRST_TIMER_EVENT || 
	timer >= FIRST_TIMER_EVENT + MAX_N_TIMERS) {
	LE_send_msg (GL_ERROR | 78,  "timer %d not defined", timer);
	ORPGTASK_exit (1);
    }

    if (period <= 0)
	return;

    tm_ptr = ((Mt_local *)(usr->mt))->timer + (timer - FIRST_TIMER_EVENT);
    if (*tm_ptr == 0)
	N_expired--;
    *tm_ptr = period + (Cr_time - Last_timer_update);
    
    if (Next_exp_time > 0 && Next_exp_time < Cr_time)
	Next_exp_time = Cr_time;

    if (Next_exp_time == 0 || period < Next_exp_time - Cr_time)
	Next_exp_time = Cr_time + period;

    return;
}

/**************************************************************************

    Description: This function cancels a timer.

    Inputs:	usr - the user involved.
		timer - the timer name (macro).

**************************************************************************/

void MT_cancel_timer (User_struct *usr, int timer)
{
    int *tm_ptr;

    if (timer < FIRST_TIMER_EVENT || 
	timer >= FIRST_TIMER_EVENT + MAX_N_TIMERS) {
	LE_send_msg (GL_ERROR | 79,  "timer %d not defined", timer);
	ORPGTASK_exit (1);
    }

    tm_ptr = ((Mt_local *)(usr->mt))->timer + (timer - FIRST_TIMER_EVENT);
    if (*tm_ptr == 0)
	N_expired--;
    *tm_ptr = -1;

    return;
}

/**************************************************************************

    Description: This function returns the time to expire of "timer".

    Inputs:	usr - the user involved.
		timer - the timer name (macro).

    Return:	It returns the remaining time in seconds for the timer
		"timer" to expire (0 if it is already expired).

**************************************************************************/

int MT_read_timer (User_struct *usr, int timer)
{
    int *tm_ptr, to_go;

    if (timer < FIRST_TIMER_EVENT || 
	timer >= FIRST_TIMER_EVENT + MAX_N_TIMERS) {
	LE_send_msg (GL_ERROR | 80,  "timer %d not defined", timer);
	ORPGTASK_exit (1);
    }

    tm_ptr = ((Mt_local *)(usr->mt))->timer + (timer - FIRST_TIMER_EVENT);
    to_go = *tm_ptr - (Cr_time - Last_timer_update);
    if (to_go <= 0)
	return (0);
    else
	return (to_go);
}

/**************************************************************************

    Description: This function cancels all timers for a user.

    Inputs:	usr - the user involved.

**************************************************************************/

void MT_cancel_all_timers (User_struct *usr)
{
    int k;

    for (k = 0; k < MAX_N_TIMERS; k++)
	MT_cancel_timer (usr, FIRST_TIMER_EVENT + k);

    return;
}

/**************************************************************************

    Description: This function updates all timers and returns an event if 
		a timer is expired. This function must be called at least 
		once per second.

    Outputs:	user_ind - user index associated with the event.

    Return:	Returns the expired timer event or -1 if no timer is expired.

**************************************************************************/

int MT_get_timer_event (int *user_ind)
{
    static time_t last_time = 0;	/* time of previous call of this 
					   function */
    int delta;
    int min, i;

    Cr_time = MISC_systime (NULL);
    if (Cr_time != last_time) {
	last_time = Cr_time;
	for (i = 0; i < N_users; i++)
	    Users[i]->time = Cr_time;
    }

    if (N_expired != 0) 
	return (Get_time_event (user_ind));

    if (Next_exp_time == 0) {
	Last_timer_update = Cr_time;
	return (-1);
    }

    if (Next_exp_time > Cr_time)
	return (-1);

    delta = Cr_time - Last_timer_update;
    Last_timer_update = Cr_time;

    min = 0x7fffffff;
    for (i = 0; i < N_users; i++) {
	int *tmr;
	int k;

	tmr = (int *)(Users[i]->mt);
	for (k = 0; k < MAX_N_TIMERS; k++) {
	    if (tmr[k] > 0) {
		tmr[k] -= delta;
		if (tmr[k] <= 0) {
		    tmr[k] = 0;
		    N_expired++;
		}
		else if (tmr[k] < min)
		    min = tmr[k];
	    }
	}
    }
    if (min < 0x7fffffff)
	Next_exp_time = Cr_time + min;
    else
	Next_exp_time = 0;

    if (N_expired != 0) 
	return (Get_time_event (user_ind));

    return (-1);  
}

/**************************************************************************

    Description: This function returns one timer expiration event. This 
		function also verifies N_expired. A bad N_expired indicates
		a coding error in this module.

    Outputs:	user_ind - user index associated with the event.

    Return:	Returns the expired timer event or -1 if no timer is expired.

**************************************************************************/

static int Get_time_event (int *user_ind)
{
    if (N_expired > 0) {
	int i, k;

	for (i = 0; i < N_users; i++) {
	    User_struct *usr;
	    int *tmr;

	    usr = Users[i];
	    *user_ind = i;
	    tmr = ((Mt_local *)(usr->mt))->timer;
	    for (k = 0; k < MAX_N_TIMERS; k++) {
		if (tmr[k] == 0) {
		    tmr[k] = -1;
		    N_expired--;
/* printf ("      expired timer %d, usr %d, cr_time %d\n", 
					k, usr->line_ind, Cr_time); */
		    return (FIRST_TIMER_EVENT + k);
		}
	    }
	}
	LE_send_msg (GL_ERROR | 81,  
		"bad N_expired (%d) in Get_time_event", N_expired);
	N_expired = 0;
    }
    else if (N_expired < 0) {
	LE_send_msg (GL_ERROR | 82,  
			"bad N_expired (%d) in Get_time_event", N_expired);
	N_expired = 0;
    }

    return (-1);
}


