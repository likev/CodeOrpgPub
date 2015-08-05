
/******************************************************************

	file: psv_process_wx_alerts.c

	This module handles the weather alerts.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/08/17 22:19:50 $
 * $Id: psv_process_wx_alerts.c,v 1.58 2009/08/17 22:19:50 jing Exp $
 * $Revision: 1.58 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <prod_user_msg.h>
#include <orpg.h>
#include "orpgalt.h"
#include <orpgdat.h>
#include <orpgevt.h>
#include <orpgerr.h>
#include <a309.h>
#include <alert_threshold.h>
#include <prod_gen_msg.h>

#include "psv_def.h"

static int N_users;		/* number of users (links) */
static User_struct **Users;	/* the user structure list */
static int P_server_index;	/* instance index of this p_server */

static int Alert_adapt_update = 1;
				/* alert adaptation data has been updated */


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int PWA_initialize (int n_users, User_struct **users, int p_server_index)
{
    int i;

    N_users = n_users;
    Users = users;
    P_server_index = p_server_index;

    /* clean up all alert request messages */
    for (i = 0; i < N_users; i++)
	PWA_line_disconnected (Users[i]);

    /* open alert data stores */
    ORPGDA_open (ALRTMSG, LB_READ);
    ORPGDA_open (ALRTPROD, LB_READ);

    return (0);
}

/**************************************************************************

    Description: This function is called when alert adaptation data is 
		updated. I sends new alert adaptation parameter msgs to
		the users.

**************************************************************************/

void PWA_alert_adapt_update ()
{
    int i;

    Alert_adapt_update = 1;

    LE_send_msg (LE_VL1 | 172,  "alert adaptation updated\n");
    for (i = 0; i < N_users; i++) {
	User_struct *usr;

	usr = Users[i];
	if (usr->psv_state == ST_ROUTINE) {

	    if (usr->up->cntl & UP_CD_ALERTS) {
		LE_send_msg (LE_VL1 | 173,  
			"sending alert adapt param msg (changed), L%d\n", 
							usr->line_ind);
		HWQ_put_in_queue (usr, 0, 
				PWA_alert_adaptation_parameter_msg (usr));
	    }
	}
    }
}

/**************************************************************************

    Description: This function reads alert messages and sends them to the
		appropriate users.

**************************************************************************/

#define ALERT_PROD_SIZE 50 + sizeof (Prod_header)

void PWA_process_alert ()
{
    static ALIGNED_t buffer[ALIGNED_T_SIZE (ALERT_PROD_SIZE)];

    while (1) {
	int line_ind, ret, i;
	User_struct *usr;
	Prod_header *phd;
	Pd_prod_header *mhb;
	char *buf;

	ret = ORPGDA_read (ALRTMSG, (char *)buffer, ALERT_PROD_SIZE, LB_NEXT);
	if (ret <= 0) {
	    if (ret == LB_TO_COME)
		return;
	    LE_send_msg (GL_ERROR | 174,  "ORPGDA_read ALRTMSG failed (ret %d)\n", ret);
	    continue;
	}
	if (ret != ALERT_PROD_SIZE) {
            if (ret != sizeof(Prod_header))
                LE_send_msg (GL_ERROR | 175, 
                       "bad alert message size (%d, expect %d)\n",
                                                ret, ALERT_PROD_SIZE);
            else {
                Prod_header *hd = (Prod_header *)buffer;
   
                if (hd->g.len >= 0 )
                    LE_send_msg (GL_ERROR | 176, 
                           "bad alert message abort message (len %d)\n",
                           hd->g.len );
  
            }
	    continue;
	}
	phd = (Prod_header *)buffer;
	mhb = (Pd_prod_header *)((char *)buffer + sizeof (Prod_header));
	UMC_message_header_block_swap (mhb);
	line_ind = mhb->dest_id;

	for (i = 0; i < N_users; i++)
	     if (Users[i]->line_ind == line_ind)
		break;
	if (i >= N_users)		/* not found */
	    continue;

	usr = Users[i];
	if (usr->psv_state != ST_ROUTINE ||
		phd->g.gen_t < usr->session_time) {
	    LE_send_msg (0, 
			"expired alert message L%d\n", line_ind);
	    continue;
	}

	buf = WAN_usr_msg_malloc (ALERT_PROD_SIZE - sizeof (Prod_header));
	if (buf == NULL)
	    return;

	memcpy (buf - sizeof (Prod_header), (char *)buffer, 
						ALERT_PROD_SIZE);
	HWQ_put_in_queue (usr, 0, buf);
    }
}

/**************************************************************************

    Description: This function puts user alert messages into the user
		message queue for distribution.

**************************************************************************/

#define USER_ALERT_HD_SIZE sizeof (Prod_header) + sizeof (Pd_prod_header)

void PWA_process_user_alert_msg ()
{
    static ALIGNED_t buffer[ALIGNED_T_SIZE (USER_ALERT_HD_SIZE)];
    static LB_id_t msg_id_read = LB_MSG_NOT_FOUND;

    /* bring back the LB read pointer moved when queued ALRTPROD products 
       are transmitted */
    if (msg_id_read != LB_MSG_NOT_FOUND)
	ORPGDA_seek (ALRTPROD, 1, msg_id_read, NULL);

    while (1) {
	int line_ind, msg_id, ret, i;
	User_struct *usr;
	Prod_header *phd;
	Pd_prod_header *mhb;
	Indirect_rpg_msg *indirect;
	char *buf;

	ret = ORPGDA_read (ALRTPROD, 
			(char *)buffer, USER_ALERT_HD_SIZE, LB_NEXT);
	if (ret == LB_TO_COME)
	    return;
	if (ret < USER_ALERT_HD_SIZE && ret != LB_BUF_TOO_SMALL) {
	    if (ret != sizeof (Prod_header))	/* == is an aborted product */
		LE_send_msg (GL_INFO | 178,  
		"ORPGDA_read ALRTPROD failed (ret %d)\n", ret);
	    continue;
	}
	msg_id = ORPGDA_get_msg_id ();
	msg_id_read = msg_id;
	phd = (Prod_header *)buffer;
	mhb = (Pd_prod_header *)((char *)buffer + sizeof (Prod_header));
	UMC_message_header_block_swap (mhb);
	line_ind = mhb->dest_id;

	for (i = 0; i < N_users; i++)
	     if (Users[i]->line_ind == line_ind)
		break;
	if (i >= N_users) {
/*	    LE_send_msg (GL_ERROR | 179,  
			"user alert line index (%d) not found\n", line_ind); */
	    continue;
	}

	usr = Users[i];
	if (usr->psv_state != ST_ROUTINE ||
		phd->g.gen_t < usr->session_time) {
	    LE_send_msg (0, 
			"expired user alert message L%d\n", line_ind);
	    continue;
	}

	buf = WAN_usr_msg_malloc (sizeof (Pd_prod_header) + 
					sizeof (Indirect_rpg_msg));
	if (buf == NULL)
	    return;

	memcpy (buf - sizeof (Prod_header), (char *)buffer, USER_ALERT_HD_SIZE);

	indirect = (Indirect_rpg_msg *)(buf + sizeof (Pd_prod_header));
	indirect->orpgdat = phd->g.prod_id;
	indirect->msg_id = msg_id;
	indirect->vol_number = phd->g.vol_num;
	indirect->seq_number = -13;
	indirect->elev_angle = 0;

	mhb = (Pd_prod_header *)buf;
	mhb->dest_id = -1;

	HP_set_to_use_db_read (phd, indirect);
	HWQ_put_in_queue (usr, HWQ_TYPE_INDIRECT | HWQ_TYPE_ALERT_PRIO, buf);
     }
}

/**************************************************************************

    Description: This function processes a alert request message.

    Inputs:	usr - the user involved.
		umsg - the user's alert request message.

**************************************************************************/

void PWA_alert_request (User_struct *usr, char *umsg)
{
    short *spt;
    int len, ret, area_num;
    unsigned int tm;

    spt = (short *)umsg;
    len = spt[10] + 18;		/* 10 is the field after block divider. 
				   18 is the size of the product header */
    area_num = spt[11];

    /* reset the time so that we can perform session time check later */
    tm = time (NULL);
    spt[2] = (tm >> 16) & 0xffff;
    spt[3] = tm & 0xffff;

    ret = ORPGDA_write (ORPGDAT_WX_ALERT_REQ_MSG, umsg, len, 
					usr->line_ind * 100 + area_num);
    if (ret != len) {
	LE_send_msg (GL_ERROR | 180,  
	    "ORPGDA_write failed (ret %d, line %d)\n", ret, usr->line_ind);
	return;
    }
    return;
}

/**************************************************************************

    Description: This function sends a 0 length message to the alert
		request data store when the line is disconnected.

    Inputs:	usr - the user involved.

**************************************************************************/

void PWA_line_disconnected (User_struct *usr)
{
    int ret;

    ret = ORPGDA_write (ORPGDAT_WX_ALERT_REQ_MSG, (char *)&ret, 0, 
					usr->line_ind * 100 + 1);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 181,  
	    "ORPGDA_write failed (ret %d, line %d)\n", ret, usr->line_ind);
	return;
    }

    ret = ORPGDA_write (ORPGDAT_WX_ALERT_REQ_MSG, (char *)&ret, 0, 
					usr->line_ind * 100 + 2);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 181,  
	    "ORPGDA_write failed (ret %d, line %d)\n", ret, usr->line_ind);
	return;
    }

    return;
}

/**************************************************************************

    Description: This function generates an alert adaptation parameter
		message.

    Inputs:	usr - the user involved.

    Return:	Pointer to the message on success or NULL on failure.

**************************************************************************/

char *PWA_alert_adaptation_parameter_msg (User_struct *usr)
{
    Pd_alert_adaptation_parameter_msg *aap;
    Pd_alert_adaptation_parameter_entry *entry;
    Pd_alert_adaptation_parameter_entry *start;
    int mlen, i;
    char *buf;
    int	cat;

    mlen = ALIGNED_SIZE (sizeof (Pd_alert_adaptation_parameter_msg)) + 
			ALERT_THRESHOLD_CATEGORIES * 
				sizeof (Pd_alert_adaptation_parameter_entry);
    buf = WAN_usr_msg_malloc (mlen);
    if (buf == NULL)
	return (NULL);

    aap = (Pd_alert_adaptation_parameter_msg *)buf;

    aap->mhb.msg_code = MSG_ALERT_PARAMETER;
    GUM_date_time (usr->time, &(aap->mhb.date), &(aap->mhb.time));
    aap->mhb.length = mlen;
    aap->mhb.n_blocks = 2;
    aap->mhb.dest_id = -1;

    aap->divider = -1;
    aap->length = ALERT_THRESHOLD_CATEGORIES * 20 + 4;

    entry = (Pd_alert_adaptation_parameter_entry *)
	(buf + ALIGNED_SIZE (sizeof (Pd_alert_adaptation_parameter_msg)));

/*  First create a zero'ed out table.			*/

    for (i = 0; i < ALERT_THRESHOLD_CATEGORIES; i++) {
	entry->alert_group      = 0;
	entry->alert_category   = 0;
	entry->max_n_thresholds = 0;
	entry->threshold1       = 0;
	entry->threshold2       = 0;
	entry->threshold3       = 0;
	entry->threshold4       = 0;
	entry->threshold5       = 0;
	entry->threshold6       = 0;
	entry->prod_code        = 0;
	entry++;
    }

    start = (Pd_alert_adaptation_parameter_entry *)
	(buf + ALIGNED_SIZE (sizeof (Pd_alert_adaptation_parameter_msg)));

/*  Next, fill in the table using data from the alert threshold LB	*/

    for (i=0;i<ORPGALT_categories (); i++) {

	cat = ORPGALT_get_category (i);
	entry = start + cat-1;
	entry->alert_group      = ORPGALT_get_group (cat);
	entry->alert_category   = cat;
	entry->max_n_thresholds = ORPGALT_get_thresholds (cat);
	entry->threshold1       = ORPGALT_get_threshold  (cat, 1);
	entry->threshold2       = ORPGALT_get_threshold  (cat, 2);
	entry->threshold3       = ORPGALT_get_threshold  (cat, 3);
	entry->threshold4       = ORPGALT_get_threshold  (cat, 4);
	entry->threshold5       = ORPGALT_get_threshold  (cat, 5);
	entry->threshold6       = ORPGALT_get_threshold  (cat, 6);
	entry->prod_code        = ORPGALT_get_prod_code (cat);

    }

    return (buf);
}
