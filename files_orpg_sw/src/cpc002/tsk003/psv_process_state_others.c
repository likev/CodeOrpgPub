
/******************************************************************

	file: psv_process_state_others.c

	This module processes other states except ST_AUTHENTICATION,
	ST_INITIAL_PROCEDURE, ST_TEST_MODE and ST_ROUTINE.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/30 18:00:30 $
 * $Id: psv_process_state_others.c,v 1.61 2005/09/30 18:00:30 jing Exp $
 * $Revision: 1.61 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h>
#include <infr.h>
#include <orpgerr.h>
#include <prod_status.h>

#include "psv_def.h"

typedef struct {		/* local part of the User_struct */
    short connect_cnt;		/* connect retrial counter */
    short init_on_discon;	/* flag indicating that cleanup and
				   initialization are needed after a line is 
				   disconnected; It is set when a 
				   connection is made and reset when the 
				   cleanup and initialization are done. */
} Pso_local;


static int N_users;		/* number of users (links) */
static User_struct **Users;	/* user structure list */

/* local functions */
static void Clean_on_disconnection (User_struct *usr);


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int PSO_initialize (int n_users, User_struct **users)
{
    int i;

    N_users = n_users;
    Users = users;

    /* allocate local data structure */
    for (i = 0; i < n_users; i++) {
	Pso_local *pso;

	pso = malloc (sizeof (Pso_local));
	if (pso == NULL) {
	    LE_send_msg (GL_ERROR | 139,  "malloc failed");
	    return (-1);
	}
	users[i]->pso = pso;
	pso->connect_cnt = 0;
	pso->init_on_discon = 1;
    }

    return (0);
}

/**************************************************************************

    Description: This function processes the state ST_DISCONNECTED.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PSO_disconnected (int ev, User_struct *usr, void *ev_data)
{

    LE_send_msg (LE_VL1 | 140,  "in state disconnected, ev = %d, L%d\n", 
							ev, usr->line_ind);

    if (ev != EV_NEXT_STATE && ev != EV_CONNECT_FAILURE_TIMER &&
	ev != EV_EXIT_TEST && ev != EV_EXIT_INACTIVE) {
	LE_send_msg (0, "event %d ignored in PSO_disconnected, L%d", 
							ev, usr->line_ind);
	return;
    }

    if (((Pso_local *)usr->pso)->init_on_discon)
	Clean_on_disconnection (usr);

    if (usr->link_state == LINK_DISABLED &&
	usr->line_tbl->link_state == LINK_ENABLED) {	/* enable the link */
	LE_send_msg (GL_INFO | 141,  "enabling link %d", usr->line_ind);
	SUS_enable_changed (usr, US_ENABLED);
	usr->link_state = LINK_ENABLED;
    }

    if (RRS_rpg_inactive ())  /* if the RPG is inactive, we don't connect */
	return;

    if (RRS_rpg_test_mode ()) {
	if (usr->line_type == DEDICATED) {
	    /* we need to find whether this user is RPGOP */
	    if (RPI_read_user_table (usr, 0) < 0) { /* read the user profile */
		LE_send_msg (GL_INFO,  
			    "user profile for dedicated line not found, L%d", 
						    usr->line_ind);
		return;
	    }
	    if (!(usr->up->cntl & UP_CD_RPGOP)) /* no-rpgop connected */
		return;
	}
	else if (usr->line_type == DIAL_OUT)
	    return;
    }

    if (usr->link_state == LINK_ENABLED) {
	WAN_connect (usr);
	MT_set_timer (usr, EV_CONNECT_TIMER, CONNECT_TRY_TIME);
	((Pso_local *)usr->pso)->connect_cnt++;
	usr->psv_state = ST_CONNECT_PENDING;
	SUS_link_changed (usr, US_CONNECT_PENDING);
	LE_send_msg (PSR_status_log_code (usr), 
		"Narrow Band line %d has CONNECT PENDING", usr->line_ind);	
    }

    return;
}

/**************************************************************************

    Description: This function processes the state ST_CONNECT_PENDING.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PSO_connect_pending (int ev, User_struct *usr, void *ev_data)
{

    LE_send_msg (LE_VL1 | 143,  "in state connect_pending, ev = %d, L%d\n", 
							ev, usr->line_ind);

    switch (ev) {

	case EV_CONNECT_TIMER:
	    if (usr->line_type == DEDICATED &&
		((Pso_local *)usr->pso)->connect_cnt /* >= */ ==
						usr->info->nb_retries) {
/*
		LE_send_msg (GL_STATUS | 144,  
			"Narrow Band line %d has FAILED after %d retries", 
				usr->line_ind, usr->info->nb_retries);
*/
/*		SUS_line_stat_changed (usr, US_UNABLE_CONN_DEDIC); */
	    }
	    (((Pso_local *)usr->pso)->connect_cnt)++;
	    WAN_connect (usr);
	    MT_set_timer (usr, EV_CONNECT_TIMER, CONNECT_TRY_TIME);
	    break;

	case EV_CONNECT_FAILED:
	    usr->discon_reason = US_IO_ERROR;
/*	    SUS_link_changed (usr, US_DISCONNECTED); */
	    MT_set_timer (usr, EV_CONNECT_FAILURE_TIMER, CONNECT_FAILURE_TIME);
	    usr->psv_state = ST_DISCONNECTED;
	    SUS_link_changed (usr, US_DISCONNECTED);
	    break;

	case EV_CONNECT_SUCCESS:
	    MT_cancel_timer (usr, EV_CONNECT_TIMER);
/*
	    LE_send_msg (GL_STATUS,  
			"Narrow Band line %d is CONNECTED in %d Attempt(s)", 
			usr->line_ind, ((Pso_local *)usr->pso)->connect_cnt);
	    LE_send_msg (PSR_status_log_code (usr),  
			"Narrow Band line %d is CONNECTED", usr->line_ind);
*/
	    ((Pso_local *)usr->pso)->connect_cnt = 0;
	    ((Pso_local *)usr->pso)->init_on_discon = 1;
	    usr->session_time = usr->time;
	    if (usr->line_type == DEDICATED) {
		usr->user_msg_block = 1;
		MT_set_timer (usr, EV_AFTER_CONN_TIMER, 4);
	    }
	    else {
		MT_set_timer (usr, EV_AUTH_TIMER, AUTH_TIME);
		usr->next_state = 1;
		usr->psv_state = ST_AUTHENTICATION;
		SUS_link_changed (usr, US_CONNECTED);
	    }
	    break;

	case EV_AFTER_CONN_TIMER:
	    usr->next_state = 1;
	    usr->psv_state = ST_AUTHENTICATION;
	    SUS_link_changed (usr, US_CONNECTED);
	    break;

	case EV_NEW_VOL_SCAN:
	    break;

	case EV_ENTER_TEST:
	case EV_ENTER_INACTIVE:
	    WAN_disconnect (usr);
	    if (ev == EV_ENTER_TEST)
		usr->discon_reason = US_IN_TEST_MODE;
	    else
		usr->discon_reason = US_SHUTDOWN;
	    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	    usr->psv_state = ST_DISCONNECTING;
	    break;

	default:
	    LE_send_msg (0, "event %d ignored - PSO_connect_pending, L%d", 
							ev, usr->line_ind);
	    break;
    }
    return;
}

/**************************************************************************

    Description: This function processes the state ST_TERMINATING, 
		ST_DISCONNECTING. Multiple disconnect 
		requests may be sent to the comm_manager.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PSO_disconnecting_etc (int ev, User_struct *usr, void *ev_data)
{
    int done;

    if (usr->psv_state == ST_TERMINATING)
	LE_send_msg (LE_VL1 | 145,  "in state terminating, ev = %d, L%d\n", 
							ev, usr->line_ind);
    else
	LE_send_msg (LE_VL1 | 146,  "in state disconnecting, ev = %d, L%d\n", 
							ev, usr->line_ind);

    done = 0;
    switch (ev) {

	case EV_DISCON_TIMER:
	    WAN_disconnect (usr);
	    done = 1;
	    break;

	case EV_NEXT_STATE:
	case EV_WRITE_COMPLETED:
	    if (usr->up == NULL ||
		(usr->up->cntl & UP_CD_IMM_DISCON) ||
						/* immediate disconnect */
		(HWQ_get_wq_size (usr) == 0 &&	/* nothing left in queue */
		WAN_circuit_ready (usr, NORMAL_PRIORITY))) {
						/* all writes done */
		WAN_disconnect (usr);
	    }
	    else
	        HWQ_wan_write (usr);
	    break;

	case EV_DISCON_FAILED:
	    LE_send_msg (GL_ERROR | 148,  
		"disconnect req failed - unexpected, L%d", usr->line_ind);
	    done = 1;
	    /* no break here! */

	case EV_DISCON_SUCCESS:
	    done = 1;
	    break;

	default:
	    LE_send_msg (LE_VL1 | 149,  
		"event %d ignored - PSO_disconnecting_etc, L%d", 
							ev, usr->line_ind);
	    break;
    }

    if (done) {
	MT_cancel_timer (usr, EV_DISCON_TIMER);
	SUS_link_changed (usr, US_DISCONNECTED); /* update prod user status */
	if (usr->psv_state == ST_TERMINATING) {
	    LE_send_msg (PSR_status_log_code (usr), 
		"Narrow Band line %d is DISCONNECTED", usr->line_ind);
	    usr->psv_state = ST_TERMINATED;
/*	    SUS_link_changed (usr, US_DISCONNECTED); */
	}
	else if (usr->psv_state == ST_DISCONNECTING) {
	    LE_send_msg (PSR_status_log_code (usr), 
		"Narrow Band line %d is DISCONNECTED", usr->line_ind);
	    usr->psv_state = ST_DISCONNECTED;
/*	    SUS_link_changed (usr, US_DISCONNECTED); */
	    Clean_on_disconnection (usr);
	}
	LE_send_msg (LE_VL1 | 150,  
		"disconnect done. New state %d, L%d", usr->psv_state, usr->line_ind);
	usr->next_state = 1;
	if (usr->discon_reason == US_LINK_DISABLED) {
	    usr->link_state = usr->line_tbl->link_state;
	    if (usr->link_state == LINK_ENABLED)
		SUS_enable_changed (usr, US_ENABLED);
	    else
		SUS_enable_changed (usr, US_DISABLED);
	}
    }

    return;
}

/**************************************************************************

    Description: This function performs cleanup and initialization jobs 
		when a line is disconnected and ready for connecting to 
		the next user.

    Inputs:	usr - the user involved.

**************************************************************************/

static void Clean_on_disconnection (User_struct *usr)
{
    if (((Pso_local *)usr->pso)->init_on_discon == 0)
	return;

    HWQ_empty_queue (usr);
    MT_cancel_all_timers (usr);
    RPI_update_config_on_disconnect (usr);
    WAN_clear_responses (usr);
    HP_clear_prod_table (usr);
    SUS_new_user (usr);
    PSAI_new_user (usr);
    PSR_new_user (usr);
    RRS_new_user (usr);
    PE_handle_link_enable_change (usr);
    PWA_line_disconnected (usr);

    RPI_get_empty_up (usr);

    usr->user_msg_block = 0;
    usr->bad_msg_cnt = 0;

    ((Pso_local *)usr->pso)->init_on_discon = 0;
    ((Pso_local *)usr->pso)->connect_cnt = 0;

    return;
}

/**************************************************************************

    Description: This function processes the state ST_TERMINATED.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PSO_terminated (int ev, User_struct *usr, void *ev_data)
{
    int i;

    /* shutdown */
    for (i = 0; i < N_users; i++)
	if (Users[i]->psv_state != ST_TERMINATED)
	    break;
    if (i >= N_users) {		/* all links are shutdown */
	LE_send_msg (GL_INFO | 151,  "p_server shutdown");
	ORPGTASK_exit (0);
    }
    return;
}
