
/******************************************************************

	file: psv_process_events.c

	This module contains functions that process p_server events.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/03/14 17:48:06 $
 * $Id: psv_process_events.c,v 1.54 2006/03/14 17:48:06 steves Exp $
 * $Revision: 1.54 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <orpgerr.h>
#include <prod_status.h>
#include <prod_user_msg.h>

#include "psv_def.h"

static int N_users;		/* number of users (links) */
static User_struct **Users;	/* user structure list */


/* local functions */
static void Call_state_processors (int ev, User_struct *usr, char *ev_data);
static void Process_cmd_shutdown_disconnect 
				(int ev, User_struct *usr, int cmd);
static void Process_ev_lost_conn (int ev, User_struct *usr, char *ev_data);
static void Process_comm_manager_terminate (int ev, 
				User_struct *usr, char *ev_data);
static void Process_comm_manager_start (int ev, 
				User_struct *usr, char *ev_data);


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int PE_initialize (int n_users, User_struct **users)
{

    N_users = n_users;
    Users = users;

    return (0);
}

/**************************************************************************

    Description: This function checks link enable state and updates
		it for each link.

**************************************************************************/

void PE_process_link_enable_change () {
    int i;

    for (i = 0; i < N_users; i++)
	PE_handle_link_enable_change (Users[i]);
}

/**************************************************************************

    Description: This function processes the event "ev".

    Inputs:	ev - event number;
		user_ind - user index associated with the event. -1 means all
			  users.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PE_process_events (int ev, int user_ind, char *ev_data)
{

    /* certain events are processed for all users */
    if (ev == EV_ROUTINE_PROD) {
	HP_handle_routine_products ();
	return;
    }
    if (ev == EV_ONETIME_PROD) {
	HP_handle_one_time_products ();
	return;
    }

    /* other events are processed for each user */
    if (user_ind < 0) {		/* for all users */
	int i;

	for (i = 0; i < N_users; i++)
	    Call_state_processors (ev, Users[i], ev_data);
    }
    else if (user_ind < N_users) {
	Call_state_processors (ev, Users[user_ind], ev_data);
    }
    else
	LE_send_msg (GL_ERROR | 83,  "code error: bad user_ind (%d)", user_ind);

    return;
}

/**************************************************************************

    Description: This function calls the appropriate state processing 
		function in terms of the current state.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

static void Call_state_processors (int ev, User_struct *usr, char *ev_data)
{

    if (ev == EV_USER_CMD) {
	Pd_distri_cmd *cmd;

	cmd = (Pd_distri_cmd *)ev_data;
	switch (cmd->command) {
	    int i;

	    case CMD_SHUTDOWN:
		if (usr->link_state != LINK_ENABLED) {
		    usr->psv_state = ST_TERMINATED;
		    PSO_terminated (ev, usr, ev_data);
		}
		else
		    Process_cmd_shutdown_disconnect (ev, usr, cmd->command);
		return;
	    case CMD_SWITCH_TO_INACTIVE:
	    case CMD_SWITCH_TO_ACTIVE:
		return;
	    case CMD_DISCONNECT:
		for (i = 0; i < cmd->n_lines; i++) {
		    if (cmd->line_ind[i] == usr->line_ind)
			Process_cmd_shutdown_disconnect (ev, usr, cmd->command);
		}
		return;
	    case CMD_CONNECT:
		/* we don't process connect command */
		return;
	}
    }

    if (usr->link_state != LINK_ENABLED)
	return;

    switch (ev) {

	case EV_LOST_CONN:
	    Process_ev_lost_conn (ev, usr, ev_data);
	    return;

	case EV_COMM_MANAGER_TERMINATE:
	    Process_comm_manager_terminate (ev, usr, ev_data);
	    return;

	case EV_COMM_MANAGER_START:
	    Process_comm_manager_start (ev, usr, ev_data);
	    return;
    }

    switch (usr->psv_state) {

	case ST_DISCONNECTED:
	    PSO_disconnected (ev, usr, ev_data);
	    break;
	case ST_CONNECT_PENDING:
	    PSO_connect_pending (ev, usr, ev_data);
	    break;
	case ST_AUTHENTICATION:
	    PSAI_authentication (ev, usr, ev_data);
	    break;
	case ST_INITIAL_PROCEDURE:
	    PSAI_initial_procedure (ev, usr, ev_data);
	    break;
	case ST_ROUTINE:
	    PSR_routine (ev, usr, ev_data);
	    break;
	case ST_DISCONNECTING:
	case ST_TERMINATING:
	    PSO_disconnecting_etc (ev, usr, ev_data);
	    break;
	case ST_TERMINATED:
	    PSO_terminated (ev, usr, ev_data);
	    break;
    }

    /* we disconnect the line after receiving 3 consecutive invalid msgs */
    if (usr->line_type != DEDICATED && usr->bad_msg_cnt >= 3) {
	char *msg;
	LE_send_msg (GL_STATUS | LE_RPG_COMMS | 84,  
		"Narrow Band line %d has made 3 consecutive INVALID REQUESTS",
						usr->line_ind);
	usr->discon_reason = US_INVALID_MSGS;
	MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	msg = RRS_gsm_message (usr);	/* get a general status msg */
	if (msg != NULL) {
	    Pd_general_status_msg *gs;
	    gs = (Pd_general_status_msg *)msg;
	    gs->rpg_nb_status |= HALFWORD_SHIFT(15);/* commanded disconnect */
	    gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
	    HWQ_put_in_queue (usr, 0, msg);		/* send to the user */
	}
	usr->psv_state = ST_DISCONNECTING;
	usr->bad_msg_cnt = 0;		/* to avoid repeated terminations */
    }

    return;
}

/**************************************************************************

    Description: This function processes the CMD_SHUTDOWN
		and CMD_CONNECT_DISCONNECT commands for all states. 

    Inputs:	ev - event number;
		usr - the user involved.
		cmd - the user command.

**************************************************************************/

static void Process_cmd_shutdown_disconnect 
					(int ev, User_struct *usr, int cmd)
{
    int new_state;

    if (cmd == CMD_SHUTDOWN) {		/* shutdown */
	new_state = ST_TERMINATING;
	switch (usr->psv_state) {

	    case ST_DISCONNECTED:
        	usr->psv_state = ST_TERMINATED;
		usr->next_state = 1;
		return;

	    case ST_TERMINATING:
	    case ST_TERMINATED:
		return;
	}
    }
    else {					/* CMD_CONNECT_DISCONNECT */
	new_state = ST_DISCONNECTING;
	switch (usr->psv_state) {
	    case ST_INITIAL_PROCEDURE:
	    case ST_ROUTINE:
		LE_send_msg (PSR_status_log_code (usr), 
		"Narrow Band line %d has DISCONNECT PENDING", usr->line_ind);
		break;

	    default:
		return;
	}
    }

    switch (usr->psv_state) {

	case ST_INITIAL_PROCEDURE:
	case ST_ROUTINE:
	    {
    		char *msg;
		Pd_general_status_msg *gs;

		HWQ_shed_all (usr);
		/* get a general status message, set commanded disconnect 
		   and send to the user */
		msg = RRS_gsm_message (usr);	/* get a general status msg */
		if (msg != NULL) {
	            gs = (Pd_general_status_msg *)msg;
		    if (cmd == CMD_SHUTDOWN)	/* shutdown */
	        	gs->rpg_op_status |= HALFWORD_SHIFT(11);
	            gs->rpg_nb_status |= HALFWORD_SHIFT(15);
	            gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
		    HWQ_put_in_queue (usr, 0, msg);	/* send to the user */
		}
	    }
	    usr->discon_reason = US_SHUTDOWN;
	    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	    usr->psv_state = new_state;
	    usr->next_state = 1;
	    break;

	default:
            WAN_disconnect (usr);
	    usr->discon_reason = US_SHUTDOWN;
	    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	    usr->psv_state = new_state;
	    break;
    }
    return;
}

/**************************************************************************

    Description: This function processes the link enable/disable states.

    Inputs:	usr - the user involved.

**************************************************************************/

void PE_handle_link_enable_change (User_struct *usr)
{
    int new_link_state;

    if (usr->psv_state == ST_TERMINATING || usr->psv_state == ST_TERMINATED)
	return;

    new_link_state = usr->line_tbl->link_state;

    if (usr->link_state == new_link_state)	/* no link state change */
	return;

    if (usr->link_state == LINK_ENABLED &&
	new_link_state == LINK_DISABLED) {	/* disable the link;  */

	LE_send_msg (GL_INFO | 85,  "disabling link %d", usr->line_ind);
	SUS_enable_changed (usr, US_DISABLED);

	switch (usr->psv_state) {
	    case ST_DISCONNECTED:
		usr->link_state = LINK_DISABLED;
		break;

	    case ST_INITIAL_PROCEDURE:
	    case ST_ROUTINE:
		{
		    char *msg;
		    Pd_general_status_msg *gs;

		    HWQ_shed_all (usr);
		    /* get a general status message, set commanded disconnect 
		       and send to the user */
		    msg = RRS_gsm_message (usr);/* get a general status msg */
		    if (msg != NULL) {
			gs = (Pd_general_status_msg *)msg;
			gs->rpg_nb_status |= HALFWORD_SHIFT(15);
						/* commanded disconnect */
			gs->prod_avail = GSM_PA_PROD_NOT_AVAIL;
			HWQ_put_in_queue (usr, 0, msg);	/* send to the user */
		    }
		}
		usr->discon_reason = US_LINK_DISABLED;
		MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
		usr->psv_state = ST_DISCONNECTING;
		usr->next_state = 1;
		break;

	    default:
		WAN_disconnect (usr);
		usr->discon_reason = US_LINK_DISABLED;
		MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
		usr->psv_state = ST_DISCONNECTING;
		break;
	}
	return;
    }

    if (usr->link_state == LINK_DISABLED && 
	usr->psv_state == ST_DISCONNECTED &&
	new_link_state == LINK_ENABLED) {	/* enable the link */
	LE_send_msg (GL_INFO | 86,  "enabling link %d", usr->line_ind);
	SUS_enable_changed (usr, US_ENABLED);
	usr->link_state = LINK_ENABLED;
	usr->next_state = 1;
    }

    return;
}

/**************************************************************************

    Description: This function processes the EV_LOST_CONN event for all 
		states. This event indicates that the line is disconnected
		unexpectedly.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

static void Process_ev_lost_conn (int ev, User_struct *usr, char *ev_data)
{

    if (usr->psv_state == ST_CONNECT_PENDING ||
	usr->psv_state == ST_AUTHENTICATION ||
	usr->psv_state == ST_INITIAL_PROCEDURE ||
	usr->psv_state == ST_ROUTINE) {
	LE_send_msg (PSR_status_log_code (usr), 
		"Narrow Band line %d experienced UNSOLICITED DISCONNECT",
					usr->line_ind);
/*
	HWQ_empty_queue (usr);
	usr->discon_reason = US_IO_ERROR;
	MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	usr->psv_state = ST_DICONNECTING;
	usr->next_state = 1;
*/
        WAN_disconnect (usr);
	MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
        usr->discon_reason = US_IO_ERROR;
        usr->psv_state = ST_DISCONNECTING;
    }

    return;
}

/**************************************************************************

    Description: This function processes the EV_COMM_MANAGER_TERMINATE 
		event for all states. This event indicates that the 
		comm_manager is terminated.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

static void Process_comm_manager_terminate (int ev, 
					User_struct *usr, char *ev_data)
{

    if (usr->psv_state == ST_TERMINATED || usr->psv_state == ST_TERMINATING)
	return;

    usr->discon_reason = US_IO_ERROR;
    usr->psv_state = ST_DISCONNECTED;
    SUS_link_changed (usr, US_DISCONNECTED);
    usr->next_state = 1;

    return;
}

/**************************************************************************

    Description: This function processes the EV_COMM_MANAGER_START 
		event for all states. This event indicates that the 
		comm_manager is started.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

static void Process_comm_manager_start (int ev, 
					User_struct *usr, char *ev_data)
{

    if (usr->psv_state == ST_TERMINATED || usr->psv_state == ST_TERMINATING)
	return;

    usr->psv_state = ST_DISCONNECTED;
    usr->next_state = 1;	/* trig an immediate connect request */
    return;
}

