
/******************************************************************

	file: psv_send_user_status.c

	This module publishes the product user status info.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/10/09 19:25:44 $
 * $Id: psv_send_user_status.c,v 1.71 2009/10/09 19:25:44 jing Exp $
 * $Revision: 1.71 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <orpg.h>
#include <prod_status.h>
#include <orpgdat.h>
#include <orpgerr.h>
#include <orpgevt.h>
#include <comm_manager.h>

#include "psv_def.h"

/* Line utilization state (Sus_local.util_state) */
enum {LINE_UTIL_NORMAL, LINE_UTIL_WARNING, LINE_UTIL_ALARM};

typedef struct {		/* local part of the User_struct */

    char enable;		/* current line enable status */
    char link;			/* current link status */
    char loadshed;		/* current load shed status */
    char line_stat;		/* current line status */
    char discon_reason;		/* current disconnection reason */
    char util_state;		/* line util state */
    short uid;			/* current user id (-1 if not available) */
    int rate;			/* current achieved baud rate (bytes/s) */
    int tnb_sent;		/* current number of bytes sent */
    int tnb_received;		/* current number of bytes received */
    short util;			/* current line utilization (in percentage) */
    short uclass;		/* user class number (-1 if not available) */
    short util_published;	/* util published */
    short fail_reported;	/* line reported as failed - need clear */

    Prod_user_status ust[US_STATUS_SIZE];
				/* The product user status message */
} Sus_local;

static void Output_pus (User_struct *usr);


/**************************************************************************

    Description: This function initializes this module. We read in the 
		previous product status messages to start with. If they
		do not exist, we initialized them.

    Inputs:	n_users - number of users.
		users - user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int SUS_initialize (int n_users, User_struct **users)
{
    int i;

    /* allocate local data structure */
    for (i = 0; i < n_users; i++) {
	Sus_local *sus;
	int ret;

	sus = malloc (sizeof (Sus_local));
	if (sus == NULL) {
	    LE_send_msg (GL_ERROR | 207,  "malloc failed");
	    return (-1);
	}
	memset ((char *)sus, 0, sizeof (Sus_local));
	users[i]->sus = sus;

	ORPGDA_open (ORPGDAT_PROD_USER_STATUS, LB_WRITE);
	ret = ORPGDA_read (ORPGDAT_PROD_USER_STATUS, (char *)sus->ust, 
		US_STATUS_SIZE * sizeof (Prod_user_status), users[i]->line_ind);
	if (ret == LB_NOT_FOUND) {
	    memset (sus, 0, sizeof (Sus_local));
	    ret = ORPGDA_write (ORPGDAT_PROD_USER_STATUS, (char *)sus->ust, 
		US_STATUS_SIZE * sizeof (Prod_user_status), users[i]->line_ind);
	}
	if (ret <= 0) {
	    LE_send_msg (GL_ERROR | 208,  "ORPGDAT_PROD_USER_STATUS failed");
	    return (-1);
	}

	if (users[i]->link_state == LINK_ENABLED)
	    sus->enable = US_ENABLED;
	else
	    sus->enable = US_DISABLED;
	sus->link = US_DISCONNECTED;
	sus->loadshed = 0;
	sus->discon_reason = 0;
	sus->util_published = 0;
	sus->line_stat = US_LINE_NORMAL;
	sus->fail_reported = 0;
	if (sus->ust->line_stat == US_LINE_FAILED) {
	    sus->line_stat = US_LINE_FAILED;
	    sus->fail_reported = 1;
	}
	sus->uid = -1;
	sus->rate = -1;
	sus->tnb_sent = 0;
	sus->tnb_received = 0;
	sus->util = 0;
	sus->util_state = LINE_UTIL_NORMAL;
	sus->uclass = -1;
	Output_pus (users[i]);
   }

    return (0);
}

/**************************************************************************

    Description: This function resets this module when a new user is 
		connected.

    Inputs:	usr - the user involved.

**************************************************************************/

void SUS_new_user (User_struct *usr)
{
    return;
}

/**************************************************************************

    Description: This function is called when a link status change is to 
		be reported.

    Inputs:	usr - the user involved.
		new_stat - the new link status (US_CONNECT_PENDING,
			   US_CONNECTED or US_DISCONNECTED);

**************************************************************************/

void SUS_link_changed (User_struct *usr, int new_stat)
{
    Sus_local *sus;

    sus = (Sus_local *)usr->sus;
    sus->link = new_stat;

    if (new_stat == US_CONNECTED) {
	sus->loadshed = US_LOAD_NORMAL;
	sus->line_stat = US_LINE_NORMAL;
	sus->discon_reason = 0;
	sus->util_published = 0;
	sus->uid = -1;
	sus->rate = -1;
	sus->tnb_sent = 0;
	sus->tnb_received = 0;
	sus->util = 0;
	sus->util_state = LINE_UTIL_NORMAL;
	sus->uclass = -1;
    }

    if (new_stat == US_DISCONNECTED) {
	sus->loadshed = 0;
/*	sus->line_stat = US_LINE_NORMAL; */
	sus->discon_reason = usr->discon_reason;
	sus->util_published = 0;
	sus->rate = -1;
	sus->tnb_sent = 0;
	sus->tnb_received = 0;
	sus->util = 0;
	sus->util_state = LINE_UTIL_NORMAL;
	sus->uclass = -1;
    }
    if (new_stat == US_CONNECT_PENDING)
	sus->uid = -1;

    Output_pus (usr);
    return;
}

/**************************************************************************

    Description: This function is called when a enable status change is to 
		be reported.

    Inputs:	usr - the user involved.
		new_stat - the new enable status;

**************************************************************************/

void SUS_enable_changed (User_struct *usr, int new_stat)
{
    Sus_local *sus;

    sus = (Sus_local *)usr->sus;
    sus->enable = new_stat;

    Output_pus (usr);
    return;
}

/**************************************************************************

    Description: This function is called when the user ID is available.

    Inputs:	usr - the user involved.
		uid - the user ID to publish.

**************************************************************************/

void SUS_user_id (User_struct *usr, int uid)
{
    Sus_local *sus;

    sus = (Sus_local *)usr->sus;
    if (sus->uid == uid)
	return;
    if (sus->uid < 0)
	LE_send_msg (GL_STATUS,  
		"NARROW BAND LINE %d MESSAGE RECEIVED (User %d)",
						usr->line_ind, uid);
    sus->uid = uid;
    if (usr->up->defined & UP_DEFINED_CLASS)
	sus->uclass = usr->up->class;
    else
	sus->uclass = -1;

    Output_pus (usr);
    return;
}

/**************************************************************************

    Description: Sets the line status to new_stat.

    Inputs:	usr - the user involved.
		new_stat - the new line status.

**************************************************************************/

void SUS_line_stat_changed (User_struct *usr, int new_stat)
{
    Sus_local *sus;

    sus = (Sus_local *)usr->sus;
    sus->line_stat = new_stat;
    if (new_stat == US_LINE_FAILED) {
	sus->util_published = 0;
	sus->rate = -1;
	sus->tnb_sent = 0;
	sus->tnb_received = 0;
	sus->util = 0;
    }

    Output_pus (usr);
    return;
}

/**************************************************************************

    Description: This function is called when a loadshed status change is to 
		be reported.

    Inputs:	usr - the user involved.
		new_stat - the new loadshed status;
		util - the new line utilization;

**************************************************************************/

void SUS_loadshed_changed (User_struct *usr, int new_stat, int util)
{
    Sus_local *sus;
    int v;

    sus = (Sus_local *)usr->sus;
    sus->loadshed = new_stat;
    sus->util = util;

    if (new_stat != US_LOAD_SHED_DISABLED) {

	if (ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST, 
				    LOAD_SHED_WARNING_THRESHOLD, &v) != 0) {
	    LE_send_msg (GL_ERROR, 
		    "ORPGLOAD_get_data LOAD_SHED_CATEGORY_PROD_DIST failed");
	    return;
	}
	if (util >= v) {
	    if (sus->util_state != LINE_UTIL_WARNING) {
		LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
		    "Narrow Band line %d, Comm. OVERLOAD WARNING", 
						usr->line_ind);
		sus->util_state = LINE_UTIL_WARNING;
	    }
	}
	else {
	    if (sus->util_state != LINE_UTIL_NORMAL) {
		LE_send_msg (GL_STATUS,
		"Narrow Band line %d, Comm. RETURNED to NORMAL from WARNING",
							    usr->line_ind);
		sus->util_state = LINE_UTIL_NORMAL;
	    }
	}
    }

    if (new_stat == US_LOAD_SHED)
	Output_pus (usr);

    return;
}

/**************************************************************************

    Description: This function is called by timer EV_PUS_PUBLISH_TIMER.

    Inputs:	ev - the event number.
		usr - the user involved.

**************************************************************************/

void SUS_process_timer (int ev, User_struct *usr)
{
    Sus_local *sus;

    sus = (Sus_local *)usr->sus;
    if (sus->util_published != sus->util) {
	Output_pus (usr);
    }
    MT_set_timer (usr, EV_PUS_PUBLISH_TIMER, PUS_PUBLISH_TIME);
}

/**************************************************************************

    Description: This function is called when a packet statistics report 
		is received. We compute noise status based on the report.
		The line rate is also obtained from the report. Refer to
		comm_manager.h for the statistics report data structure.
		When a protocol other than X.25 is used, this function needs 
		to be extended to accept different report format and 
		calculate noise status based on different equations.

    Inputs:	usr - the user involved.
		rep - the statistics report;
		rep_len - length the report;

**************************************************************************/

void SUS_stat_report (User_struct *usr, char *rep, int rep_len)
{
    Sus_local *sus;
    int old_rate, old_noise, old_tnb_sent, old_tnb_received;
    CM_resp_struct *resp;

    sus = (Sus_local *)usr->sus;
    if (sus->link != US_CONNECTED)
	return;

    /* verify report length */
    resp = (CM_resp_struct *)rep;
    if (rep_len < (int)sizeof (CM_resp_struct) ||
	resp->data_size + sizeof (CM_resp_struct) != rep_len) {
	LE_send_msg (GL_ERROR,  "bad statistics message size");
	return;
    }
    
    old_rate = sus->rate;
    old_noise = sus->line_stat;
    old_tnb_sent = sus->tnb_sent;
    old_tnb_received = sus->tnb_received;

    if (resp->data_size == sizeof (X25_statistics)) {
	X25_statistics *st;
	double c1, c2;		/* criteria 1 and 2 */
	int ni, nr, n;

	st = (X25_statistics *)(rep + sizeof (CM_resp_struct));
    
	ni = 0;			/* all information packets */
	if (st->R_I != CM_SF_UNAVAILABLE)
	    ni += st->R_I;
	if (st->X_I != CM_SF_UNAVAILABLE)
	    ni += st->X_I;
    
	nr = 0;			/* all rejected packets */
	if (st->R_REJ != CM_SF_UNAVAILABLE)
	    nr += st->R_REJ;
	if (st->X_REJ != CM_SF_UNAVAILABLE)
	    nr += st->X_REJ;
	if (st->R_ERROR != CM_SF_UNAVAILABLE)
	    nr += st->R_ERROR;
	if (st->X_ERROR != CM_SF_UNAVAILABLE)
	    nr += st->X_ERROR;
	if (st->R_FCS != CM_SF_UNAVAILABLE)
	    nr += st->R_FCS;
    
	if (ni != 0)
	    c1 = (double)nr / (double)ni;
	else
	    c1 = 0.;
    
	n = 0;			/* denominator for the second criterion */
	if (st->R_I != CM_SF_UNAVAILABLE)
	    n += st->R_I;
	if (st->R_RR != CM_SF_UNAVAILABLE)
	    n += st->R_RR;
	if (st->R_RNR != CM_SF_UNAVAILABLE)
	    n += st->R_RNR;
	if (st->R_REJ != CM_SF_UNAVAILABLE)
	    n += st->R_REJ;
    
	if (n != 0 && st->R_FCS != CM_SF_UNAVAILABLE)
	    c2 = (double)st->R_FCS / (double)n;
	else
	    c2 = 0.;
    
	if (c1 > .2 || c2 > .05)
	    sus->line_stat = US_LINE_NOISY;
	if (c1 < .1 && c2 < .01)
	    sus->line_stat = US_LINE_NORMAL;
    
	if (st->rate != CM_SF_UNAVAILABLE)
	    sus->rate = st->rate;
	else
	    sus->rate = -1;
	sus->tnb_sent = 0;
	sus->tnb_received = 0;

	LE_send_msg (LE_VL1,  
		"STT ni %d, nr %d, n %d, R_FCS %d, rate %d, L%d\n", 
			ni, nr, n, st->R_REJ, st->rate, usr->line_ind);
    }
    else if (resp->data_size == sizeof (TCP_statistics)) {
	TCP_statistics *st;

	st = (TCP_statistics *)(rep + sizeof (CM_resp_struct));

	/* we consider a link is noisy if it is blocked for 10 seconds */
	if (st->no_input_time > st->keep_alive_test_period + 10)
	    sus->line_stat = US_LINE_NOISY;
	else
	    sus->line_stat = US_LINE_NORMAL;

	if (st->rate != CM_SF_UNAVAILABLE)
	    sus->rate = st->rate;
	else
	    sus->rate = -1;
	sus->tnb_sent = st->tnb_sent;
	sus->tnb_received = st->tnb_received;

	LE_send_msg (LE_VL1,  
		"STT no_input_time %d, test_period %d, rate %d, tnb_sent %d, tnb_rcvd %d, L%d\n", 
		st->no_input_time, st->keep_alive_test_period, 
		st->rate, st->tnb_sent, st->tnb_received, usr->line_ind);
    }
    else {
	LE_send_msg (GL_ERROR, "Unexpected statistics message\n");
	return;
    }

    /* output if anything changes */
    if (sus->rate != old_rate || sus->line_stat != old_noise ||
	sus->tnb_sent != old_tnb_sent || sus->tnb_received != old_tnb_received)
	Output_pus (usr);
    return;
}

/**************************************************************************

    Description: This function outputs the new product user status msg
		for "usr".

    Inputs:	usr - the user involved.

**************************************************************************/

static void Output_pus (User_struct *usr)
{
    Sus_local *sus;
    Prod_user_status *ust;
    int i;
    char line_ind;

    sus = (Sus_local *)usr->sus;
    for (i = US_STATUS_SIZE - 1; i > 0; i--)
	sus->ust[i] = sus->ust[i - 1];

    ust = sus->ust;
    ust->line_ind = usr->line_ind;
    ust->enable = sus->enable;
    ust->link = sus->link;
    ust->loadshed = sus->loadshed;
    ust->line_stat = sus->line_stat;
    ust->discon_reason = sus->discon_reason;
    ust->uid = sus->uid;
    ust->rate = sus->rate;
    ust->tnb_sent = sus->tnb_sent;
    ust->tnb_received = sus->tnb_received;
    ust->util = sus->util;
    ust->uclass = sus->uclass;
    ust->time = time (NULL);

    LE_send_msg (LE_VL1 | 212,  
"PUS en %d lnk %d shd %d line_stat %d disc_rsn %d uid %d rate %d, util %d, class %d, L%d\n", 
ust->enable, ust->link, ust->loadshed, ust->line_stat, ust->discon_reason, ust->uid, ust->rate, ust->util, ust->uclass, usr->line_ind);

    ORPGDA_write (ORPGDAT_PROD_USER_STATUS, (char *)sus->ust, 
		US_STATUS_SIZE * sizeof (Prod_user_status), usr->line_ind);
    line_ind = usr->line_ind;
    EN_post (ORPGEVT_PROD_USER_STATUS, &line_ind, 1 ,0);

/*  We also post the data via an event for special users such as the	*
 *  HCI (to avoid RPC call at MSCF in satellite configurations).	*/
    EN_post (ORPGEVT_PROD_USER_STATUS_DATA, ust, sizeof (Prod_user_status) ,0);

    sus->util_published = sus->util;
    if (sus->line_stat == US_LINE_FAILED && sus->enable == US_ENABLED &&
				!sus->fail_reported) {
	LE_send_msg (GL_STATUS | LE_RPG_COMMS, 
		    "Narrow Band line %d has FAILED", usr->line_ind);
	sus->fail_reported = 1;
    }
    else if (sus->line_stat != US_LINE_FAILED && sus->fail_reported) {
 	LE_send_msg (GL_STATUS | LE_RPG_COMMS, 
	"Narrow Band line %d returned to NORMAL from FAILED", usr->line_ind);
	sus->fail_reported = 0;
    }

    return;
}

/************************************************************************

    Description: Reads NB util info from all lines, finds out the 
		maximum NB utilization and reports it.

    Input:	usr - the first user of this p_server instance.

************************************************************************/

void SUS_report_max_nb_util (User_struct *usr) {
    static int distribution_alarm = -1;
    int i, ret, max_util, line_failed;
    Pd_distri_info *p_tbl;
    unsigned char alarm_flag;

    p_tbl = (Pd_distri_info *)usr->info;
    max_util = 0;
    line_failed = 0;
    for (i = 0; i < p_tbl->n_lines; i++) {
	Prod_user_status ust;

	ret = ORPGDA_read (ORPGDAT_PROD_USER_STATUS, (char *)&ust, 
				sizeof (Prod_user_status), i);
	if (ret == LB_BUF_TOO_SMALL || 
	    (ret > 0 && ret >= (int)sizeof (Prod_user_status))) {							/* read success */
	    if (ust.enable != US_ENABLED)
		continue;
	    if (ust.line_stat == US_LINE_FAILED) {
		line_failed = 1;
		continue;
	    }
	    if (ust.link == US_CONNECTED && 
		ust.loadshed != US_LOAD_SHED_DISABLED &&
		ust.util > max_util)
		max_util = ust.util;
	}
    }
    ret = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST, 
					LOAD_SHED_CURRENT_VALUE, max_util);
		/* this function processes loadshed warning and alarms */
    if (ret < 0)
	LE_send_msg (GL_ERROR | 251,  "ORPGLOAD_set_data failed (ret %d)", ret);
    ret = 0;
    if (line_failed && line_failed != distribution_alarm)
	ret = ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_DISTRI,
			ORPGINFO_STATEFL_SET, &alarm_flag);
    if (!line_failed && line_failed != distribution_alarm)
	ret = ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_DISTRI,
			ORPGINFO_STATEFL_CLR, &alarm_flag);
    if (ret < 0)
	LE_send_msg (GL_ERROR,
	    "ORPGINFO_statefl_rpg_alarm failed (ret %d)", ret);
    distribution_alarm = line_failed;
}
