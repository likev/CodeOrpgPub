
/******************************************************************

	file: psv_get_events.c

	This module contains functions that retrieve p_server events.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 14:55:27 $
 * $Id: psv_get_events.c,v 1.92 2014/05/13 14:55:27 steves Exp $
 * $Revision: 1.92 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FTMD_NEED_TABLE		/* get A309_ftmd_line_spec */
#include <a309.h>

#include <orpg.h>
#include <mrpg.h>
#include <infr.h>
#include <prod_gen_msg.h>

#include "psv_def.h"

#define NB_POLL_TIME	4	/* NB response polling time. In case we lose
				   an NB resp event, we are still fine */

static int N_users;		/* number of users (links) */
static User_struct **Users;	/* user structure list */
static int P_server_index;	/* index of this product server */

/* EN flags */
static int Ev_rda_status_change;/* RDA status updated */
static int Ev_rpg_status_change;/* RPG status updated */
static int Ev_rpg_state_change;	/* RPG state updated */
static int Ev_start_of_volume;	/* A new volume started */
static int Ev_end_avset_volume;	/* End of AVSET volume event */
static int Ev_prod_gen_control;	/* prod generation list updated */
static int *Ev_nb_comm_resp;	/* comm_manager response count */
static int *Ev_nb_comm_read;	/* comm_manager response read count */
static time_t *Nb_poll_time;	/* previous comm_manager response poll time */
static int Ev_line_info;	/* prod distri line info updated */
static int Ev_psv_cmd;		/* p_server command received */
static int Ev_ot_response;	/* ps_onetime responded */
static int Ev_prod_status;	/* product status updated */
static int Ev_wx_alert = 0;	/* alert received */
static int Ev_wx_user_alert = 0;/* user alert msg received */
static int Ev_wx_alert_adapt_update = 0;
				/* alert threshold adaptation data changed */
static int Ev_text_msg = 0;	/* free text message received */
static int Ev_statistics_period = 0;
				/* The statistics report period is to set to
				   this value */
static int Ev_up_msgid;		/* user profile updated */

#define MAX_N_USER_LINES 100
static Pd_distri_cmd *Cr_cmd;	/* the current user command */

static Scan_Summary Avset_scan_summary;	/* msg of ORPGEVT_LAST_ELEV_CUT */

static int Text_ev_time = 0;

/* local functions */
static void En_callback (EN_id_t evtcd, char *msg, int msglen, void *arg);
static void Un_callback (int fd, LB_id_t msgid, int info, void *arg);
static void Process_text_msg ();
static void Housekeeping ();
static void Process_up_update ();


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int GE_initialize (int n_users, User_struct **users, int p_server_index)
{
    int ret, i;

    N_users = n_users;
    Users = users;
    P_server_index = p_server_index;

    Cr_cmd = (Pd_distri_cmd *)malloc (sizeof (Pd_distri_cmd) + 
					MAX_N_USER_LINES * sizeof (int));
    Ev_nb_comm_resp = (int *)malloc (N_users * sizeof (int));
    Ev_nb_comm_read = (int *)malloc (N_users * sizeof (int));
    Nb_poll_time = (time_t *)malloc (N_users * sizeof (time_t));
    if (Cr_cmd == NULL || Ev_nb_comm_resp == NULL || 
		Ev_nb_comm_read == NULL || Nb_poll_time == NULL) {
	LE_send_msg (GL_ERROR | 9,  "malloc failed");
	return (-1);
    }
    Cr_cmd->n_lines = 0;

    Ev_rda_status_change = 0;
    Ev_rpg_status_change = 0;
    Ev_rpg_state_change = 0;
    Ev_start_of_volume = 0;
    Ev_end_avset_volume = 0;
    Ev_prod_gen_control = 0;
    Ev_line_info = 0;
    Ev_psv_cmd = 0;
    Ev_ot_response = 0;
    Ev_prod_status = 0;
    Ev_wx_alert = 0;
    Ev_wx_user_alert = 0;
    Ev_wx_alert_adapt_update = 0;
    Ev_text_msg = 0;
    Ev_statistics_period = 0;
    Ev_up_msgid = LB_ANY;

    /* register events */
    if ((ret = EN_register (ORPGEVT_RDA_STATUS_CHANGE, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_RPG_STATUS_CHANGE, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_RPG_ALARM, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_RPG_OPSTAT_CHANGE, En_callback)) < 0 ||
	(ret = ORPGDA_UN_register (ORPGDAT_TASK_STATUS, MRPG_RPG_STATE_MSGID,
					Un_callback)) < 0 ||
	(ret = ORPGDA_UN_register (ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID,
					Un_callback)) < 0 ||
	(ret = ORPGDA_UN_register (ORPGDAT_USER_PROFILES, LB_ANY,
					Un_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_START_OF_VOLUME, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_LAST_ELEV_CUT, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_PROD_GEN_CONTROL, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_PD_LINE, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_OT_RESPONSE + p_server_index, 
						En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_PROD_STATUS, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_WX_ALERT_ADAPT_UPDATE, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_WX_USER_ALERT_MSG, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_WX_ALERT_MESSAGE, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_FREE_TXT_MSG, En_callback)) < 0 ||
	(ret = EN_register (ORPGEVT_STATISTICS_PERIOD, En_callback)) < 0) {
	LE_send_msg (GL_ERROR | 10,  "Event register failed (ret %d)", ret);
	return (-1);
    }
    for (i = 0; i < N_users; i++) {
	Ev_nb_comm_resp[i] = 0;
	Ev_nb_comm_read[i] = 0;
	Nb_poll_time[i] = 0;
	if ((ret = EN_register (ORPGEVT_NB_COMM_RESP + Users[i]->line_ind, 
						En_callback)) < 0) {
	    LE_send_msg (GL_ERROR | 11,  "EN_register failed (ret %d)", ret);
	    return (-1);
	}
    }

    ORPGDA_lbfd (FTXTMSG);

    return (0);
}

/**************************************************************************

    Description: This is the UN callback function. It sets appropriate 
		flags.

    Inputs:	See LB manpage

**************************************************************************/

static void Un_callback (int fd, LB_id_t msgid, int info, void *arg) {

    if (fd == ORPGDA_lbfd (ORPGDAT_TASK_STATUS) && 
				msgid == MRPG_RPG_STATE_MSGID) {
	LE_send_msg (LE_VL3,  "RPG state update event received\n");
	Ev_rpg_state_change = 1;
    }
    else if (fd == ORPGDA_lbfd (ORPGDAT_PROD_INFO) && 
				msgid == PD_LINE_INFO_MSG_ID) {
	LE_send_msg (LE_VL3,  "line info update event received\n");
	Ev_line_info = 1;
    }
    else if (fd == ORPGDA_lbfd (ORPGDAT_USER_PROFILES)) {
	Ev_up_msgid = msgid;
	LE_send_msg (LE_VL3,  "user profile update event received\n");
    }
    else
	LE_send_msg (LE_VL3,  "Unexpected UN received (%d %d)\n", fd, msgid);
}

/**************************************************************************

    Description: This is the EN callback function. It sets appropriate 
		flags.

    Inputs:	evtcd - event number.
		msg - event message.
		msglen - length of the message.

**************************************************************************/

static void En_callback (EN_id_t evtcd, char *msg, int msglen, void *arg)
{

    LE_send_msg (LE_VL3 | 12,  "EV %d received\n", evtcd);

    switch (evtcd) {
	int i;

	case ORPGEVT_RDA_STATUS_CHANGE:
	    Ev_rda_status_change = 1;
	    break;

	case ORPGEVT_RPG_STATUS_CHANGE:
	case ORPGEVT_RPG_ALARM:
	case ORPGEVT_RPG_OPSTAT_CHANGE:
	    Ev_rpg_status_change = 1;
	    break;

	case ORPGEVT_START_OF_VOLUME:
	    Ev_start_of_volume = 1;
	    break;

	case ORPGEVT_LAST_ELEV_CUT:
	    if (msglen != sizeof (Scan_Summary) || msg == NULL) {
		LE_send_msg (GL_ERROR, "Bad msg in event ORPGEVT_LAST_ELEV_CUT - event discarded\n");
		break;
	    }
	    memcpy (&Avset_scan_summary, msg, sizeof (Scan_Summary));
	    Ev_end_avset_volume = 1;
	    break;

	case ORPGEVT_PROD_GEN_CONTROL:
	    Ev_prod_gen_control = 1;
	    break;

	case ORPGEVT_PD_LINE:
	    if (Ev_psv_cmd) {
		LB_NTF_control (LB_NTF_REDELIVER);
		break;
	    }

	    if (msglen > 0 && msglen >= (int)sizeof (Pd_distri_cmd)) {
		Pd_distri_cmd *new;
		int new_len;

		new = (Pd_distri_cmd *)msg;
		if (new->n_lines > 0)
		    new_len = sizeof (Pd_distri_cmd) + 
				(new->n_lines - 1) * sizeof (int);
		else
		    new_len = sizeof (Pd_distri_cmd);
		if (msglen != new_len || new->n_lines > MAX_N_USER_LINES) {
		    LE_send_msg (GL_ERROR | 13,  
			"bad ORPGEVT_PD_LINE event message size (%d)", msglen);
		    break;
		}
		memcpy ((char *)Cr_cmd, msg, new_len);
		if (Cr_cmd->command != CMD_LINK_STATE_CHANGED)
		    Ev_psv_cmd = 1;
	    }
	    else
		Cr_cmd->command = CMD_NO_CMD;
	    break;

	case ORPGEVT_PROD_STATUS:
	    Ev_prod_status = 1;
	    HP_prod_status_updated ();
	    break;

	case ORPGEVT_WX_ALERT_ADAPT_UPDATE:
	    Ev_wx_alert_adapt_update = 1;
	    break;

	case ORPGEVT_WX_USER_ALERT_MSG:
	    Ev_wx_user_alert = 1;
	    break;

	case ORPGEVT_WX_ALERT_MESSAGE:
	    Ev_wx_alert = 1;
	    break;

	case ORPGEVT_FREE_TXT_MSG:
	    Ev_text_msg = 1;
	    break;

	case ORPGEVT_STATISTICS_PERIOD:
	    if (msg != NULL && msglen > 0) {
		msg[msglen - 1] = '\0';
		if (sscanf (msg, "%d", &Ev_statistics_period) != 1)
		    Ev_statistics_period = 0;
	    }
	    break;

	default:
	    for (i = 0; i < N_users; i++) {
		if (evtcd == ORPGEVT_NB_COMM_RESP + Users[i]->line_ind) {
		    Ev_nb_comm_resp[i] += 1;
		    return;
		}
	    }
	    if (evtcd == ORPGEVT_OT_RESPONSE + P_server_index)
		Ev_ot_response = 1;
	    else
		LE_send_msg (GL_ERROR | 14,  
				"unregistered event (%d) received", evtcd);
	    break;
    }
}

/**************************************************************************

    Description: This is the main processing loop of the p_server. This
		function detects any new event and calls the event 
		processing function to process it. Events are processed
		based on their priorities if multiple events exist 
		simultaneously. This is implemented with the variable
		"called".

		This function will not return. On fatal error, it 
		terminates the process.

**************************************************************************/

void GE_main_loop ()
{
    int cnt, read_event;

    cnt = 0;
    read_event = 1;
    while (1) {
	int i, ev, user_ind;
	int called;
	char *ev_data;

	/* the NEXT_STATE event */
	called = 0;
	for (i = 0; i < N_users; i++) {
	    if (Users[i]->next_state) {
		Users[i]->next_state = 0;
		PE_process_events (EV_NEXT_STATE, i, NULL);
		called = 1;
	    }
	}
	if (called)
	    continue;

	if (read_event)
	    LB_NTF_control (LB_NTF_WAIT, 0);
	read_event = 1;

	/* read RPG state */
	if (Ev_rpg_state_change) {
	    int test, inactive;

	    Ev_rpg_state_change = 0;
	    test = RRS_rpg_test_mode ();
	    inactive = RRS_rpg_inactive ();
	    RRS_update_rpg_state ();
	    if (!test && RRS_rpg_test_mode ()) {
		LE_send_msg (0,  "Entering test mode\n");
		PE_process_events (EV_ENTER_TEST, -1, NULL);
	    }
	    if (!inactive && RRS_rpg_inactive ()) {
		LE_send_msg (0,  "Entering inactive mode\n");
		PE_process_events (EV_ENTER_INACTIVE, -1, NULL);
	    }
	    if (test && !RRS_rpg_test_mode ()) {
		LE_send_msg (0,  "Exiting from test mode\n");
		PE_process_events (EV_EXIT_TEST, -1, NULL);
	    }
	    if (inactive && !RRS_rpg_inactive ()) {
		LE_send_msg (0,  "Exiting from inactive mode\n");
		PE_process_events (EV_EXIT_INACTIVE, -1, NULL);
	    }
	    continue;
	}

	/* prod distri line updated */
	if (Ev_line_info) {
	    Ev_line_info = 0;
	    RPI_read_pd_info ();
	    PE_process_link_enable_change ();
	    LE_send_msg (LE_VL2,  "prod distri line info updated\n");
	    called = 1;
	}

	/* p_server command received */
	if (Ev_psv_cmd) {

	    Ev_psv_cmd = 0;
	    if (Cr_cmd->command != CMD_NO_CMD)
		LE_send_msg (LE_VL2 | 15,  
			"processing p_server command (%d)\n", Cr_cmd->command);

	    if (Cr_cmd->command == CMD_SHUTDOWN ||
		Cr_cmd->command == CMD_SWITCH_TO_INACTIVE ||
		Cr_cmd->command == CMD_SWITCH_TO_ACTIVE ||
		Cr_cmd->command == CMD_CONNECT ||
		Cr_cmd->command == CMD_DISCONNECT) {
		PE_process_events (EV_USER_CMD, -1, (char *)Cr_cmd);
	    }
	    else if (Cr_cmd->command == CMD_VV_ON)
		HWQ_set_vv_mode (1);
	    else if (Cr_cmd->command == CMD_VV_OFF)
		HWQ_set_vv_mode (0);
	    called = 1;
	}
	if (called)
	    continue;

	/* process timers */
	while ((ev = MT_get_timer_event (&user_ind)) >= 0) {
	    if (ev == EV_WAIT_TIMER) {
		PE_process_events (Users[user_ind]->cur_event, 
					user_ind, Users[user_ind]->ev_data);
	    }
	    else if (ev == EV_PUS_PUBLISH_TIMER) {
		SUS_process_timer (ev, Users[user_ind]);
	    }
	    else
		PE_process_events (ev, user_ind, NULL);
	    called = 1;
	}
	if (called)
	    continue;

	/* RDA status changes */
	if (Ev_rda_status_change) {
	    Ev_rda_status_change = 0;
	    RRS_update_rda_status ();
	    called = 1;
	}
	/* RPG status changes */
	if (Ev_rpg_status_change) {
	    Ev_rpg_status_change = 0;
	    RRS_update_rpg_status ();
	    called = 1;
	}
	if (called) {
	    RRS_send_gms ();	/* send updated gen stat msg to the users */
	    continue;
	}

	/* weather alerts */
	if (Ev_wx_alert_adapt_update) {
	    Ev_wx_alert_adapt_update = 0;
	    PWA_alert_adapt_update ();
	    called = 1;
	}
	if (Ev_wx_alert) {
	    Ev_wx_alert = 0;
	    PWA_process_alert ();
	    called = 1;
	}
	if (Ev_wx_user_alert) {
	    Ev_wx_user_alert = 0;
	    PWA_process_user_alert_msg ();
	    called = 1;
	}
	if (called)
	    continue;

	if (Ev_start_of_volume) {

	    Ev_start_of_volume = 0;
	    RRS_update_vol_status (NULL);

	    PE_process_events (EV_NEW_VOL_SCAN, -1, NULL);
	    called = 1;
	}
	if (Ev_end_avset_volume) {
	    Ev_end_avset_volume = 0;
	    RRS_update_vol_status (&Avset_scan_summary);
	    called = 1;
	}
	if (called) {
	    RRS_send_gms ();	/* send updated gen stat msg to the users */
	    continue;
	}

	if (Ev_text_msg) {
	    Ev_text_msg = 0;
	    Text_ev_time = MISC_systime (NULL);
	    Process_text_msg ();
	    continue;
	}

	if (Ev_statistics_period > 0) {
	    int period = Ev_statistics_period;
	    Ev_statistics_period = 0;
	    WAN_change_statistics_report_period (period);
	    continue;
	}

	if (Ev_prod_gen_control) {
	    Ev_prod_gen_control = 0;
	    PE_process_events (EV_PROD_GEN_UPDATED, -1, NULL);
	    continue;
	}

	/* user messages */
	for (i = 0; i < N_users; i++) {
	    if (!Users[i]->user_msg_block) {
		if (Ev_nb_comm_resp[i] > Ev_nb_comm_read[i]) {
		    Ev_nb_comm_read[i] += 1;
		    if ((ev = WAN_read_next_msg (Users[i], &ev_data)) >= 0) {
			PE_process_events (ev, i, ev_data);
			called = 1;
		    }
		}
		if (!called &&
		    Users[i]->time - Nb_poll_time[i] > NB_POLL_TIME) {
		    if ((ev = WAN_read_next_msg (Users[i], &ev_data)) >= 0) {
			PE_process_events (ev, i, ev_data);
			called = 1;
		    }
		    Nb_poll_time[i] = Users[i]->time;
		}
	    }
	    if (Users[i]->msg_queued)
		HWQ_wan_write (Users[i]);
	}
	if (called)
	    continue;

	/* data available */
	if (Ev_prod_status) {
	    Ev_prod_status = 0;
	    PE_process_events (EV_ROUTINE_PROD, -1, NULL);
	    called = 1;
	}
	if (Ev_ot_response) {
	    Ev_ot_response = 0;
	    PE_process_events (EV_ONETIME_PROD, -1, NULL);
	    called = 1;
	}
	if (called)
	    continue;

	if (Ev_up_msgid != LB_ANY)
	    Process_up_update ();

	Housekeeping ();

	read_event = 0;
	LB_NTF_control (LB_NTF_WAIT, 1000);

	cnt++;
    }
}

/************************************************************************

    Description: The p_server housekeeping function.

************************************************************************/

static void Housekeeping () {
    User_struct *usr;
    time_t t;
    static time_t last_test_try_time = 0, last_text_time = 0;

    t = MISC_systime (NULL);

    RRS_update_time( t );
    
    if (RRS_rpg_test_mode ()) {	/* periodically call PSO_disconnected to
				   start connect in case UP changes */
	if (last_test_try_time == 0)
	    last_test_try_time = t;
	if (t >= last_test_try_time + 10) {
	    int i;
	    for (i = 0; i < N_users; i++) {
		usr = Users[i];
		if (usr->psv_state == ST_DISCONNECTED)
		    PSO_disconnected (EV_NEXT_STATE, usr, NULL);
	    }
	    last_test_try_time = t;
	}
    }
    else
	last_test_try_time = t;

    /* reports max NB utilization. Only the first p_server instance does
       this job. It reports every MAX_NB_UTIL_PERIOD seconds. */
    usr = Users[0];
    if (usr->first_psv) {
	static time_t last = 0;

	if (usr->time > last + MAX_NB_UTIL_PERIOD) {
	    last = usr->time;
	    SUS_report_max_nb_util (usr);
	}
    }

    if (t <= Text_ev_time + 8 && t >= last_text_time + 2) {
					/* event may come before FTXTMSG msg */
	Process_text_msg ();		/* with satellite oneway replication */
	last_text_time = t;
    }
}

/**************************************************************************

    Description: Reads text messages and processes them.

**************************************************************************/

static void Process_text_msg ()
{

    while (1) {
	char *msg;
	int len;
	
	len = ORPGDA_read (FTXTMSG, &msg, LB_ALLOC_BUF, LB_NEXT);
	if (len == LB_TO_COME)
	    break;
	if (len == LB_EXPIRED) {
	    LE_send_msg (GL_ERROR,  
			"ORPGDA_read FTXTMSG - message lost");
	    continue;
	}
	if (len < 0) {
	    LE_send_msg (GL_ERROR | 18,  
			"ORPGDA_read FTXTMSG failed (ret %d)", len);
	    break;
	}
	if (len < PHEADLNG * (int)sizeof (short) + (int)sizeof (Prod_header)) {
	    LE_send_msg (GL_ERROR | 19,  
			"text msg too short (%d)", len);
	    continue;
	}
	if (UMC_product_from_icd (msg + sizeof (Prod_header), 
					len - sizeof (Prod_header)) > 0) {

	    Send_text_msg_t stm;
	    unsigned short *spt;
	    int i;

	    spt = (unsigned short *)(msg + sizeof (Prod_header));
	    for (i = 0; i < FTMD_LINE_SPEC_SIZE; i++)
		stm.ftmd_line_spec[i] = spt[A309_ftmd_line_spec[i]];

	    stm.ftmd_type_spec = spt[FTMD_TYPE_SPEC];
	    stm.data_type = FTXTMSG;
	    stm.msgid = ORPGDA_get_msg_id (FTXTMSG);
	    stm.hd = msg;

	    LE_send_msg (LE_VL1 | 20,  "read text msg, data type %d, msgid %d\n", 
						stm.data_type, stm.msgid);
	    PE_process_events (EV_TEXT_MSG, -1, (char *)&stm);
	}
	free (msg);
    }
}

/*********************************************************************

    This is for legacy compatibility. In the new RPG, class 4 users, 
    like other users, can send their product requests instead having 
    to call the RPG operator for any new/additional products. Thus 
    updating UP while connected is not necessary.

    Processes user profile update while the user is connected.

*********************************************************************/

static void Process_up_update () {
    int len, i;
    char *buf;
    Pd_user_entry *nclass;

    len = ORPGDA_read (ORPGDAT_USER_PROFILES, 
				&buf, LB_ALLOC_BUF, Ev_up_msgid);
    Ev_up_msgid = LB_ANY;
    if (len <= 0) {		/* This should not happen normally */
	LE_send_msg (GL_ERROR, 
	    "ORPGDA_read (ORPGDAT_USER_PROFILES %d) failed (ret %d)\n", 
						    Ev_up_msgid, len);
	return;
    }

    nclass = (Pd_user_entry *)buf;	/* new class user profile entry */
    if (nclass->up_type != UP_CLASS || 
			nclass->class != 4 ||
			!(nclass->defined & UP_DEFINED_DISTRI_METHOD)) {
	free (buf);
	return;
    }
    for (i = 0; i < N_users; i++) {
	User_struct *usr;
	int class_off;
	Pd_user_entry *nup;

	usr = Users[i];
	if (usr->psv_state != ST_ROUTINE || usr->up == NULL ||
			!(usr->up->defined & UP_DEFINED_CLASS) ||
			!(usr->up->defined & UP_DEFINED_DISTRI_METHOD) ||
			usr->up->class != 4 || 
			usr->up->distri_method != nclass->distri_method)
	    continue;
	/* update the user's distribution and permission tables */
	nup = (Pd_user_entry *)malloc (usr->up->entry_size + 
							nclass->entry_size);
	if (nup == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed");
	    break;
	}
	memcpy ((char *)nup, (char *)usr->up, usr->up->entry_size);
	class_off = usr->up->entry_size;
	memcpy ((char *)nup + class_off, (char *)nclass, nclass->entry_size);
	if (nclass->defined & UP_DEFINED_PMS) {
	    nup->pms_len = nclass->pms_len;
	    nup->pms_list = nclass->pms_list + class_off;
	    nup->defined |= UP_DEFINED_PMS;
	}
	if (nclass->defined & UP_DEFINED_DD) {
	    nup->dd_len = nclass->dd_len;
	    nup->dd_list = nclass->dd_list + class_off;
	    nup->defined |= UP_DEFINED_DD;
	}
	free (usr->up);
	usr->up = nup;

	LE_send_msg (GL_INFO, "user profile updated, L%d", usr->line_ind);
	HP_init_prod_table (usr);    /* update scheduling and distribution */
    }
    free (buf);
}


