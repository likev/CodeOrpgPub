
/******************************************************************

	file: psv_read_pd_info.c

	This module contains functions accessing prod_distri_info 
	messages, which contain the product distribution adaptation 
	data, and initializing the "users" structure list.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/15 16:53:00 $
 * $Id: psv_read_pd_info.c,v 1.73 2007/02/15 16:53:00 jing Exp $
 * $Revision: 1.73 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h>
#include <infr.h>
#include <orpgdat.h>
#include <orpgerr.h>

#include "psv_def.h"

static int P_server_ind;		/* index of this p_server instance */
static int N_users =0;			/* number of users served by this
					   p_server instance */
static User_struct **Users;		/* The users (comm links) list */


static int Initialize_user_struct ();
static void Update_line_table (User_struct *usr, int tbl_ind);
static char *Read_pd_info (int *len);
static void Print_user_profile (Pd_user_entry *up);

/**************************************************************************

    Initializes the Users list and reads product_distri_info 
    messages.

    Inputs:	p_server_ind - index of this instance of p_server.
		users - the user list.

    Outputs:	users - updated user list.

    Return:	It returns the new number of user served. Note that this
		number cannot change after initialization.

**************************************************************************/

int RPI_initialize (int p_server_ind, User_struct **users)
{
    int n_users;

    P_server_ind = p_server_ind;
    Users = users;

    n_users = Initialize_user_struct ();
    if (n_users == 0)
	LE_send_msg (GL_ERROR, 
	    "0 users specified for this p_server (ind = %d)", p_server_ind);
    return (n_users);
}

/**************************************************************************

    Description: This function reads the PD_LINE_INFO_MSG_ID message and 
		updates the basic product distribution info and the line 
		configuration. If the new line configuration is not 
		acceptable, it is discarded.

    Return:	Returns 0.

**************************************************************************/

int RPI_read_pd_info ()
{
    char *buf;
    Pd_distri_info *p_tbl;
    Pd_line_entry *l_tbl;
    int cnt, bad, i, len;
		
    /* allocate the buffer and read the message */
    buf = Read_pd_info (&len);

    p_tbl = (Pd_distri_info *)buf;
    if (len < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list + 
		p_tbl->n_lines * (int)sizeof (Pd_line_entry) > len) {
	LE_send_msg (GL_ERROR | 192,  "error in PD_LINE_INFO_MSG_ID message");
	ORPGTASK_exit (1);
    }
    l_tbl = (Pd_line_entry *)(buf + p_tbl->line_list);

    /* verify that certain stuff can not change */
    cnt = bad = 0;
    for (i = 0; i < p_tbl->n_lines; i++) {

	if (l_tbl[i].p_server_ind == P_server_ind) {
	    if (Users[cnt]->line_ind != l_tbl[i].line_ind ||
		Users[cnt]->cm_ind != l_tbl[i].cm_ind)
		bad = 1;
	    cnt++;
	}
    }
    if (cnt != N_users)
	bad = 1;
    if (bad) {
	LE_send_msg (GL_INFO | 193, 
		"Inconsistent prod_distri_info msg - ignored");
	free (buf);
	return (0);
    }

    free (Users[0]->info);
    for (i = 0; i < N_users; i++) {
	int tbl_ind;

	for (tbl_ind = 0; tbl_ind < p_tbl->n_lines; tbl_ind++) {
	    if (Users[i]->line_ind == l_tbl[tbl_ind].line_ind) {

		Users[i]->info = (Pd_distri_info *)buf;
		Update_line_table (Users[i], tbl_ind);

		/* send parameters to comm_manager */
		WAN_send_set_params_request (Users[i], l_tbl + tbl_ind, -1);

		break;
	    }
	}
    }
	
    return (0);
}

/**************************************************************************

    Description: This function initializes the user list. It first reads 
		the PD_LINE_INFO_MSG_ID to get the line configuration info.

    Return:	It returns the number of users served.

**************************************************************************/

static int Initialize_user_struct ()
{
    char *buf;
    Pd_distri_info *p_tbl;
    Pd_line_entry *l_tbl;
    int cnt, i, len;
    int min_ind, rda_line;

    /* allocate the buffer and read the message */
    buf = Read_pd_info (&len);

    p_tbl = (Pd_distri_info *)buf;
    if (len < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list +
		p_tbl->n_lines * (int)sizeof (Pd_line_entry) > len) {
	LE_send_msg (GL_ERROR | 194,  "error in PD_LINE_INFO_MSG_ID message");
	ORPGTASK_exit (1);
    }
    l_tbl = (Pd_line_entry *)(buf + p_tbl->line_list);
    rda_line = ORPGCMI_rda_response () - ORPGDAT_CM_RESPONSE;

    /* find user lines to be served by this p_server */
    cnt = 0;
    min_ind = 0x7fffffff;
    for (i = 0; i < p_tbl->n_lines; i++) {
	if (l_tbl[i].line_ind != rda_line && l_tbl[i].p_server_ind < min_ind)
	    min_ind = l_tbl[i].p_server_ind;	/* min p_server index */

	if (l_tbl[i].p_server_ind == P_server_ind) {

	    if (cnt >= MAX_N_USERS) {
	        LE_send_msg (GL_ERROR | 195,  "Too many users configured");
	        ORPGTASK_exit (1);
	    }

	    Users[cnt] = malloc (sizeof (User_struct));
	    if (Users[cnt] == NULL) {
	        LE_send_msg (GL_ERROR | 196,  "malloc failed");
	        ORPGTASK_exit (1);
	    }

	    /* initialize the new user structure */
	    Users[cnt]->info = (Pd_distri_info *)buf;
	    Update_line_table (Users[cnt], i);
	    Users[cnt]->first_psv = 0;
	    Users[cnt]->status_msg_cnt = 0;

	    Users[cnt]->up = NULL;
	    RPI_get_empty_up (Users[cnt]);
	    Users[cnt]->rrs = NULL;
	    Users[cnt]->wan = NULL;

	    Users[cnt]->link_state = l_tbl[i].link_state;
	    if (Users[cnt]->link_state == LINK_ENABLED)
		Users[cnt]->psv_state = ST_DISCONNECTING;
	    else
		Users[cnt]->psv_state = ST_DISCONNECTED;
	    Users[cnt]->next_state = 1;

	    Users[cnt]->time = MISC_systime (NULL);
	    Users[cnt]->session_time = 0;
	    Users[cnt]->bad_msg_cnt = 0;
	    Users[cnt]->user_msg_block = 0;
	    Users[cnt]->discon_reason = 0;
	    Users[cnt]->status_received = 0;

	    RPI_update_config_on_disconnect (Users[cnt]);

	    cnt++;
	}
    }
    N_users = cnt;
    if (min_ind == P_server_ind)
	Users[0]->first_psv = 1;

    return (cnt);
}


/**************************************************************************

    Description: This function allocates an empty user profile.

    Inputs:	usr - the involved user.

**************************************************************************/

void RPI_get_empty_up (User_struct *usr)
{
    Pd_user_entry *up;

    up = (Pd_user_entry *)malloc (sizeof (Pd_user_entry));
    if (up == NULL) {
	LE_send_msg (GL_ERROR | 197,  "malloc failed");
	ORPGTASK_exit (1);
    }

    memset ((char *)up, 0, sizeof (Pd_user_entry));
    if (usr->up != NULL)
	free ((char *)usr->up);
    usr->up = up;

    return;
}

/**************************************************************************

    Description: This function updates the line configuration table. Those 
		items that can not take effect immediately are kept in the 
		usr->line_tbl. 

    Inputs:	usr - the involved user (link).
		tbl_ind - index of the line table entry for this user.

**************************************************************************/

static void Update_line_table (User_struct *usr, int tbl_ind)
{
    Pd_distri_info *p_tbl;
    Pd_line_entry *l_tbl;

    p_tbl = usr->info;
    l_tbl = (Pd_line_entry *)((char *)usr->info + sizeof (Pd_distri_info));

    usr->line_ind = l_tbl[tbl_ind].line_ind;
    usr->cm_ind = l_tbl[tbl_ind].cm_ind;

    usr->line_tbl = l_tbl + tbl_ind;

    return;
}

/**************************************************************************

    Description: This function reads the user profile.

    Input:	usr - the user involved.
		uid - user id. Used only if the line is not DEDICATED.

    Return:	0 on success or a negative error number if the user profile 
		is not found.

**************************************************************************/

int RPI_read_user_table (User_struct *usr, int uid)
{
    int ret;

    if (usr->up != NULL)
	free ((char *)usr->up);
    usr->up = NULL;
    if ((ret = ORPGNBC_get_user_profile (usr->line_type, 
				uid, usr->line_ind, &(usr->up))) < 0)
	return (ret);

    LE_send_msg (LE_VL1 | 198,  "user (%d) profile retrieved, L%d",
					usr->up->user_id, usr->line_ind);

    if (LE_local_vl (-1) >= 2)
	    Print_user_profile (usr->up);

    return (0);
}

/**************************************************************************

    Description: This function reads the product distribution info
		message (pd_table). The caller must free the buffer. 

    Output:	len - number of bytes of the message.

    Return:	The buffer holding the message.

**************************************************************************/

static char *Read_pd_info (int *len)
{
    char *buf;
    int ret;

    ret = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, 
				LB_ALLOC_BUF, PD_LINE_INFO_MSG_ID);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR,  
		"ORPGDA_read (data store %d, msg %d) failed (ret %d)", 
			ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID, ret);
	ORPGTASK_exit (1);
    }
    *len = ret;

    return (buf);
}

/**************************************************************************

    Description: This function is called when the a link is disconnected and
		a new connection is to be made. This function copies the
		new configuration items, that can only be updated when a
		link is disconnected and reused, to the current values.

    Inputs:	usr - the involved user (link).

**************************************************************************/

void RPI_update_config_on_disconnect (User_struct *usr)
{

    usr->line_type = usr->line_tbl->line_type;

    return;
}

/**************************************************************************

    Description: This function prints user profile "up".

    Input:	up - the user profiles to print.

**************************************************************************/

static void Print_user_profile (Pd_user_entry *up)
{
    int i;

    LE_send_msg (0, 
	"    size %d, u_id %d, u_type %d, line_ind %d, class %d, u_name %s\n",
			up->entry_size, up->user_id, up->up_type, 
			up->line_ind, up->class, up->user_name);
    LE_send_msg (0, 
	"    cntl %x, defined %x, max_con_time %d, max_n_reqs %d, wait_time_for_rps %d, pswd %s\n", 
			up->cntl, up->defined, up->max_connect_time, 
			up->n_req_prods, up->wait_time_for_rps, 
			up->user_password);

    LE_send_msg (0, "    PMS list: len %d\n", up->pms_len);
    if (up->pms_len > 0) {
	Pd_pms_entry *entry;

	entry = (Pd_pms_entry *)((char *)up + up->pms_list);
	for (i = 0; i < up->pms_len; i++) {
	    if (i < 2 || i == up->pms_len - 1) {
		LE_send_msg (0, 
	"        prod_id %d, wx_modes %2x, types %2x\n",
			entry->prod_id, entry->wx_modes, entry->types);
	    }
	    if (i == 2 && up->pms_len != 3)
		LE_send_msg (0, "        ......\n");
	    entry++;
	}
    }

    LE_send_msg (0, "    Default Distri list: len %d\n", up->dd_len);
    if (up->dd_len > 0) {
	Pd_prod_item *entry;

	entry = (Pd_prod_item *)((char *)up + up->dd_list);
	for (i = 0; i < up->dd_len; i++) {
	    if (i < 2 || i == up->dd_len - 1) {
		LE_send_msg (0, 
	"        pid %d, wx %2x, prd %d, num %d, map %d, prio %d, prms %s\n",
			entry->prod_id, entry->wx_modes, 
			entry->period, entry->number, entry->map_requested, 
			entry->priority, 
			HP_print_parameters (entry->params, entry->prod_id));
	    }
	    if (i == 2 && up->dd_len != 3)
		LE_send_msg (0, "        ......\n");
	    entry++;
	}
    }

    LE_send_msg (0, "    Map list: len %d\n", up->map_len);
    if (up->map_len > 0) {
	short *entry;
	char buf[128];

	entry = (short *)((char *)up + up->map_list);
	sprintf (buf, "        maps: ");
	for (i = 0; i < up->map_len; i++) {
	    if (i > 6) 
		break;
	    if (i == 0)
		sprintf (buf + strlen (buf), "%d", *entry);
	    else
		sprintf (buf + strlen (buf), ", %d", *entry);
	    entry++;
	}
	if (i < up->map_len)
	    strcat (buf, " ... ");
	LE_send_msg (0, buf);
   }
    return;
}


#ifdef TEMP_DEBUG

/**************************************************************************

    Description: This function prints out the user structure for debugging
		purpose.

    Inputs:	usr - the involved user (link).

**************************************************************************/

void RPI_print_user (User_struct *usr)
{
    Pd_distri_info *info;
    Pd_line_entry *l_tbl;
    Pd_user_entry *u_tbl;
    Pd_prod_entry *p_tbl;
    Pd_attr_entry *a_tbl;
    int i;

    info = usr->info;
    printf ("\n");
    printf ("The User_struct:\n");
    printf ("    line_ind = %d\n", usr->line_ind);
    printf ("    line_type = %d\n", usr->line_type);
    printf ("    cm_ind = %d\n", usr->cm_ind);

    printf ("    psv_state = %d\n", usr->psv_state);
    printf ("    next_state = %d\n", usr->next_state);
    printf ("    link_state = %d\n", usr->link_state);

    info = usr->info;
    printf ("    info:\n");
    printf ("        nb_retries = %d\n", info->nb_retries);
    printf ("        nb_timeout = %d\n", info->nb_timeout);
    printf ("        connect_time_limit  = %d\n", info->connect_time_limit);
    printf ("        rcm_ed_timeout = %d\n", info->rcm_ed_timeout);
    printf ("        rcm_ed_decision_time = %d\n", info->rcm_ed_decision_time);
    printf ("        rcm_ed_name = %s\n", info->rcm_ed_name);

    if (usr->up != NULL) {
	printf ("    user profile:\n");
	printf ("        size %d, user_id %d, rt_len %d, ot_len %d, map_len %d, rt_list %d\n", 
			usr->up->entry_size, usr->up->user_id, usr->up->rt_len, 
			usr->up->ot_len, usr->up->map_len, usr->up->rt_list);

	printf ("        max_connect_time %d, type %d, override %d, apup_status %d,  rcm %d\n", 
			usr->up->max_connect_time, usr->up->line, 
			usr->up->override ,usr->up->apup_status, usr->up->rcm);
	printf ("        user_password %s, user_name %s\n", 
			usr->up->user_password, usr->up->user_name);

        if (usr->up->rt_len > 0) {
	    Pd_prod_type_item *pt;
	    pt = (Pd_prod_type_item *)((char *)usr->up + usr->up->rt_list);
	    printf ("        	RT p0: prod_id = %d elev_ind = %d\n", 
						pt->prod_id, pt->elev_ind);
        }
        if (usr->up->ot_len > 0) {
    	    Pd_prod_type_item *pt;
	    pt = (Pd_prod_type_item *)((char *)usr->up + usr->up->ot_list);
	    printf ("        	OT p0: prod_id = %d elev_ind = %d\n", 
						pt->prod_id, pt->elev_ind);
        }
        if (usr->up->drt_len > 0) {
    	    Pd_prod_item *pt;
	    pt = (Pd_prod_item *)((char *)usr->up + usr->up->drt_list);
	    printf ("        	Default RT p0: prod_id = %d\n", pt->prod_id);

        }
        if (usr->up->map_len > 0) {
	    prod_id_t *pt;
	    pt = (prod_id_t *)((char *)usr->up + usr->up->map_list);
	    printf ("        	RT p0 = %d\n", *pt);
	}
    }

    l_tbl = usr->line_tbl;
    printf ("    line table:\n");
    printf ("        %d  %d  %d  %d  %d  %d  %d %s\n", 
		l_tbl->line_ind, l_tbl->cm_ind, l_tbl->p_server_ind, 
		l_tbl->line_type, l_tbl->link_state, l_tbl->status_req, 
		l_tbl->baud_rate, l_tbl->port_password);

    /* print prod attribute table - not printed */

}

#endif

