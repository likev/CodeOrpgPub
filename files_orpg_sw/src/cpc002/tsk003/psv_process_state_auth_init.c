
/******************************************************************

	file: psv_process_state_auth_init.c

	This module processes the states of ST_AUTHENTICATION and
	ST_INITIAL_PROCEDURE.
	
******************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/10/09 19:25:44 $
 * $Id: psv_process_state_auth_init.c,v 1.63 2009/10/09 19:25:44 jing Exp $
 * $Revision: 1.63 $
 * $State: Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <orpgerr.h>
#include <orpg.h>
#include <prod_status.h>
#include <prod_user_msg.h>

#include "psv_def.h"

typedef struct {		/* local part of the User_struct */
    int session_extended;	/* The connection time has been extended */
    int max_connect_time;	/* connection (session) time in minutes; 0 
				   indicates that there is no limit on it. */
} Psai_local;

/* local functions */
static int Verify_signon (User_struct *usr, void *ev_data);

/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int PSAI_initialize (int n_users, User_struct **users)
{
    int i;

    /* allocate local data structure */
    for (i = 0; i < n_users; i++) {
	Psai_local *psai;

	psai = malloc (sizeof (Psai_local));
	if (psai == NULL) {
	    LE_send_msg (GL_ERROR | 123,  "malloc failed");
	    return (-1);
	}
	users[i]->psai = psai;
	psai->session_extended = 0;
	psai->max_connect_time = 0;
    }

    return (0);
}

/**************************************************************************

    Description: This function resets this module when a new user is 
		connected.

    Inputs:	usr - the user involved.

**************************************************************************/

void PSAI_new_user (User_struct *usr)
{
    Psai_local *psai;

    psai = (Psai_local *)usr->psai;
    psai->session_extended = 0;
    psai->max_connect_time = 0;
    MT_set_timer (usr, EV_PUS_PUBLISH_TIMER, PUS_PUBLISH_TIME);
    return;
}

/**************************************************************************

    Description: This function processes the state ST_AUTHENTICATION.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

#define WAIT_RPGDBM_TIME 40

void PSAI_authentication (int ev, User_struct *usr, void *ev_data)
{

    LE_send_msg (LE_VL1 | 124,  
		"in state authentication, ev = %d, L%d\n", ev, usr->line_ind);

    /* for dedicated users no signon is needed */
    if (usr->line_type == DEDICATED) {
	int ret;
	time_t st;

	LE_send_msg (PSR_status_log_code (usr),  
			"Narrow Band line %d is CONNECTED", usr->line_ind);
	LE_send_msg (LE_VL1 | 125,  "read user profile on dedicated, L%d\n", 
						usr->line_ind);
	MT_cancel_timer (usr, EV_AUTH_TIMER);

	st = MISC_systime (NULL);	/* read the user profile */
	while ((ret = RPI_read_user_table (usr, 0)) == LB_NOT_ACTIVE &&
				MISC_systime (NULL) < st + WAIT_RPGDBM_TIME)
	    msleep (1000);
	if (ret < 0) {
	    LE_send_msg (GL_INFO | 126,  
		"User profile for dedicated line not found (ret %d), L%d", 
						ret, usr->line_ind);
	    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	    usr->psv_state = ST_DISCONNECTING;
	    usr->next_state = 1;
	    return;
/*	    ORPGTASK_exit (1); */
	}
	((Psai_local *)(usr->psai))->max_connect_time = 0;
/*	SUS_user_id (usr, usr->up->user_id); */
	usr->psv_state = ST_INITIAL_PROCEDURE;
	usr->next_state = 1;
	usr->user_msg_block = 0;
	return;
    }

    switch (ev) {

	case EV_AUTH_TIMER:
	case EV_ENTER_TEST:
	    if (ev == EV_AUTH_TIMER) {
		LE_send_msg (GL_STATUS,  
		    "NARROW BAND LINE %d disconnected: 0 BYTES TRANSFERRED",
						usr->line_ind);
		LE_send_msg (GL_INFO,  
			"signon msg not received, L%d", usr->line_ind);
		usr->discon_reason = US_NOT_RESPONDING;
	    }
	    else 
		usr->discon_reason = US_IN_TEST_MODE;
	    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	    usr->psv_state = ST_DISCONNECTING;
	    usr->next_state = 1;
	    break;

	case EV_ENTER_INACTIVE:
	    WAN_disconnect (usr);
	    usr->discon_reason = US_IN_TEST_MODE;
	    MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
	    usr->psv_state = ST_DISCONNECTING;
	    break;

	case EV_USER_DATA:
	    if (Verify_signon (usr, ev_data) < 0) {	/* invalid user */
		LE_send_msg (PSR_status_log_code (usr),  
			"Narrow Band line %d Sign-on Failed", usr->line_ind);
		MT_cancel_timer (usr, EV_AUTH_TIMER);
		LE_send_msg (GL_INFO,  
			"bad signon msg, L%d", usr->line_ind);
		MT_set_timer (usr, EV_DISCON_TIMER, DISCON_TIME);
		HWQ_put_in_queue (usr, 0, GUM_rr_message 
			(usr, RR_INVALID_PASSWD, MSG_SIGN_ON, 0, 0));
							/* send rr msg */
		usr->psv_state = ST_DISCONNECTING;
	    }
	    else {					/* valid user */
		LE_send_msg (PSR_status_log_code (usr),  
			"Narrow Band line %d is CONNECTED (User %d)", 
					usr->line_ind, usr->up->user_id);
		MT_cancel_timer (usr, EV_AUTH_TIMER);
		SUS_user_id (usr, usr->up->user_id);
		usr->psv_state = ST_INITIAL_PROCEDURE;
		usr->next_state = 1;
		return;
	    }
	    break;

	case EV_NEXT_STATE:
	    break;

	default:
	    LE_send_msg (0, "event %d ignored - PSAI_authentication, L%d", 
							ev, usr->line_ind);
	    break;
    }
    return;
}

/**************************************************************************

    Description: This function returns the user's maximum connection time.

    Inputs:	usr - the user involved.

**************************************************************************/

int PSAI_max_connect_time (User_struct *usr)
{

    return (((Psai_local *)(usr->psai))->max_connect_time);
}

/**************************************************************************

    Description: This function processes the state ST_INITIAL_PROCEDURE.

    Inputs:	ev - event number;
		usr - the user involved.
		ev_data - pointer to additional data structure associated 
			  with the event.

**************************************************************************/

void PSAI_initial_procedure (int ev, User_struct *usr, void *ev_data)
{

    LE_send_msg (LE_VL1 | 129,  "in state initial_procedure, ev = %d, L%d\n", 
							ev, usr->line_ind);

    if (RRS_rpg_test_mode () && !(usr->up->cntl & UP_CD_RPGOP)) {
	PSR_enter_test_mode (usr);
	return;
    }

    switch (ev) {

	case EV_USER_DATA:
	    if (((Pd_msg_header *)ev_data)->msg_code == MSG_MAX_CON_DISABLE)
		PSAI_process_max_connect_time_disable_request_msg (
								usr, ev_data);
	    else
		LE_send_msg (0, 
			"EV_USER_DATA ignored - PSAI_initial_procedure, L%d",
								usr->line_ind);
	    break;

	case EV_NEXT_STATE:
	    /* send status message */
	    if (usr->up->cntl & UP_CD_STATUS) {
		LE_send_msg (LE_VL1 | 130,  
			"sending general status msg, L%d\n", usr->line_ind);
		HWQ_put_in_queue (usr, 0, RRS_gsm_message (usr));
	    }

	    /* send alert adaptation parameter message */
	    if (usr->up->cntl & UP_CD_ALERTS) {
		LE_send_msg (LE_VL1 | 132,  
			"sending alert adapt param msg (on connect), L%d\n", 
							usr->line_ind);
		HWQ_put_in_queue (usr, 0, 
				PWA_alert_adaptation_parameter_msg (usr));
	    }

	    /* send maps */
	    if (usr->up->cntl & UP_CD_MAPS) {
	    }

	    if (usr->up->wait_time_for_rps > 0)
		MT_set_timer (usr, EV_RPS_TIMER, usr->up->wait_time_for_rps);

	    HP_init_prod_table (usr);	/* initialize product tables */
	    usr->psv_state = ST_ROUTINE;
	    usr->next_state = 1;

	    break;

	case EV_SESSION_TIMER:
	    PSR_session_expiration (usr);
	    break;

	case EV_ENTER_TEST:
	case EV_ENTER_INACTIVE:
	    break;

	default:
	    LE_send_msg (0, "event %d ignored - PSAI_initial_procedure, L%d", 
							ev, usr->line_ind);
	    break;
    }
    return;
}

/**************************************************************************

    Description: This function retrieves the user's profile and verifies 
		the passwords.

    Inputs:	usr - the user involved.
		ev_data - pointer to the signon message.

    Return:	0 on success or -1 on failure.

**************************************************************************/

static int Verify_signon (User_struct *usr, void *ev_data)
{
    Pd_sign_on_msg *msg;
    int time_limit, max_time;

    msg = (Pd_sign_on_msg *)ev_data;

    if (msg->mhb.msg_code != MSG_SIGN_ON) {
	LE_send_msg (GL_STATUS | 133,  
		"NARROW BAND LINE %d disconnected: 0 BYTES TRANSFERRED",
						usr->line_ind);
	usr->discon_reason = US_INVALID_MSGS;
	return (-1);
    }

    /* get the user profile */
    LE_send_msg (LE_VL1 | 134,  "read user profile (dial %d-%s-%s-)..., L%d\n", 
			msg->mhb.src_id, msg->port_passwd, 
			msg->user_passwd, usr->line_ind);
    if (RPI_read_user_table (usr, msg->mhb.src_id) < 0) {
	LE_send_msg (GL_STATUS | 135,  
		"NARROW BAND LINE %d disconnected: USER %d NOT FOUND",
					usr->line_ind, msg->mhb.src_id);
/*	usr->up->user_id = msg->mhb.src_id; */
	usr->discon_reason = US_BAD_ID;
	return (-1);
    }

    /* check password */
    if (strlen (usr->line_tbl->port_password) == 0) {
	LE_send_msg (GL_INFO, 
			"Port access word not defined. L%d", usr->line_ind);
	usr->discon_reason = US_BAD_PASSWORD;
	return (-1);
    }
    if (strlen (usr->up->user_password) == 0 ) {
	LE_send_msg (GL_INFO, "User (id %d) access word not defined. L%d", 
				usr->up->user_id, usr->line_ind);
	usr->discon_reason = US_BAD_PASSWORD;
	return (-1);
    }
    if (strcmp (msg->port_passwd, usr->line_tbl->port_password) != 0) {
	LE_send_msg (GL_STATUS,  
		"INVALID PORT PASSWORD: %s ON LINE %d", 
					msg->port_passwd, usr->line_ind);
	usr->discon_reason = US_BAD_PASSWORD;
	return (-1);
    }
    if (strcmp (msg->user_passwd, usr->up->user_password) != 0) {
	LE_send_msg (GL_STATUS,  
		"INVALID USER ID/PASSWORD: %d / %s ON LINE %d", 
			msg->mhb.src_id, msg->user_passwd, usr->line_ind);
	usr->discon_reason = US_BAD_PASSWORD;
	return (-1);
    }

    /* set the session timer */
    max_time = usr->line_tbl->conn_time_limit;		/* per line limit */
    time_limit = usr->up->max_connect_time;		/* per user limit */
    if (time_limit == 0 ||
	(max_time > 0 && time_limit > max_time))
	time_limit = max_time;

    max_time = usr->info->connect_time_limit;		/* general limit */
    if (time_limit == 0 ||
	(max_time > 0 && time_limit > max_time))
	time_limit = max_time;

    if (msg->disconn_override_flag) {
	if (usr->up->cntl & UP_CD_OVERRIDE) {	/* override max_connect_time */
	    if (time_limit < max_time) {
		time_limit = max_time;
		LE_send_msg (LE_VL1, 
			"connection time extended to %d minutes, L%d", 
					time_limit, usr->line_ind);
	    }
	    ((Psai_local *)(usr->psai))->session_extended = 1;
	    MT_set_timer (usr, EV_SESSION_TIMER, time_limit * 60);

	    ((Psai_local *)(usr->psai))->max_connect_time = time_limit;
	    usr->bad_msg_cnt = 0;
	}
	else {				/* override is not granted */
	    LE_send_msg (LE_VL1, 
		"connection time override rejected (use %d), L%d", 
						time_limit, usr->line_ind);
	    MT_set_timer (usr, EV_SESSION_TIMER, time_limit * 60);
	    ((Psai_local *)(usr->psai))->max_connect_time = time_limit;
	    HWQ_put_in_queue (usr, 0, GUM_rr_message 
			(usr, RR_ILLEGAL_REQ, MSG_MAX_CON_DISABLE, 0, 0));
							/* send rr msg */
	    usr->bad_msg_cnt++;
	}
    }
    else {
	LE_send_msg (LE_VL1, 
		"connection time set to %d, L%d", time_limit, usr->line_ind);
	MT_set_timer (usr, EV_SESSION_TIMER, time_limit * 60);
	((Psai_local *)(usr->psai))->max_connect_time = time_limit;
	usr->bad_msg_cnt = 0;
    }

    return (0);
}

/**************************************************************************

    Description: This function processes the maximum connect time disable 
		request message. To extend the time, override privilege
		is required. One can only extend once. It is impossible to 
		extend beyond the limit specified by min (per line time limit,
		general time limit). If the request is not granted, an RR 
		message is sent to the user. The message is ignored if the 
		user does not have a connect time limit.

    Inputs:	usr - the user involved.
		msg - the maximum connect time disable request message.

**************************************************************************/

void PSAI_process_max_connect_time_disable_request_msg (
					User_struct *usr, char *msg)
{
    Pd_max_conn_time_disable_msg *mcd;
    Psai_local *psai;

    mcd = (Pd_max_conn_time_disable_msg *)msg;
    psai = (Psai_local *)usr->psai;
    if (psai->max_connect_time == 0)
	return;

    if (usr->up->cntl & UP_CD_OVERRIDE) {
					/* override the max_connect_time */
	int left, add, max_time;

	if (psai->session_extended)
	    return;

	max_time = usr->info->connect_time_limit;	/* general limit */
	add = mcd->add_conn_time;
	if (add == 0)
	    add = max_time;
	if (add + psai->max_connect_time > max_time)
	    add = max_time - psai->max_connect_time;
	if (add > 0) {
	    left = MT_read_timer (usr, EV_SESSION_TIMER) / 60 + 1;
	    MT_set_timer (usr, EV_SESSION_TIMER, (left + add) * 60);
	    psai->max_connect_time += add;
	}
	psai->session_extended = 1;
	usr->bad_msg_cnt = 0;
    }
    else {				/* override is not granted */

	LE_send_msg (GL_INFO | 138,  "connection time override rejected, L%d", 
							usr->line_ind);
	HWQ_put_in_queue (usr, 0, GUM_rr_message 
			(usr, RR_ILLEGAL_REQ, MSG_MAX_CON_DISABLE, 0, 0));	
					/* send rr msg */
	usr->bad_msg_cnt++;
    }
    return;
}

