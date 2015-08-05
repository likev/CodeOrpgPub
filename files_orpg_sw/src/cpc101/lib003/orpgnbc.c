/* 
 * RCS info 
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/10/04 19:44:08 $
 * $Id: orpgnbc.c,v 1.38 2005/10/04 19:44:08 steves Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 */ 

/**********************************************************************

	file: orpgnbc.c

	This is the NBC (Narrow Band Control) module of liborpg.
	
***********************************************************************/

 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpgevt.h>
#include <infr.h>
#include <orpg.h>
#include <orpgerr.h>
#include <rpgdbm.h>

static int Unlock_return (int ret_value, char *buf);
static Pd_user_entry *Find_user_profile (int up_type, 
				int search_ind, int distri_method, int *err);
static int Get_line_info (char **msg);
static int Verify_length (int len, char *buf);

/**************************************************************************

    Description: This function sends a disable/enable link command to 
		p_server.

    Input:	cmd - the command (NBC_DISABLE_LINK or NBC_ENABLE_LINK).
		n_lines - number of lines.
		line_ind - line index array.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int ORPGNBC_enable_disable_NB_links (int command, int n_lines, int *line_ind)
{
    int len, ret, n;
    Pd_distri_info *hd;
    Pd_distri_cmd cmd;

    if (command != NBC_DISABLE_LINK &&
	command != NBC_ENABLE_LINK)
	return (-1);

    len = Get_line_info ((char **)&hd);
    if (len < 0)
	return (-1);

    /* set the command */
    for (n = 0; n < n_lines; n++) {
	Pd_line_entry *line;
	int i;

	line = (Pd_line_entry *)((char *)hd + hd->line_list);
	for (i = 0; i < hd->n_lines; i++) {
	    if (line->line_ind == line_ind[n]) {
		if (command == NBC_DISABLE_LINK)
		    line->link_state = LINK_DISABLED;
		else
		    line->link_state = LINK_ENABLED;
		break;
	    }
	    line++;
	}
    }

    /* write back */
    ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char *)hd, 
					len, PD_LINE_INFO_MSG_ID);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGNBC: ORPGDA_write PD_LINE_INFO_MSG_ID failed (ret %d)\n", 
						ret);
	return (Unlock_return (-1, (char *)hd));
    }

    /* post an event */
    cmd.command = CMD_LINK_STATE_CHANGED;
    cmd.n_lines = 0;
    EN_post (ORPGEVT_PD_LINE, &cmd, sizeof (Pd_distri_cmd), 0);

    return (Unlock_return (0, (char *)hd));
}

/**************************************************************************

    Description: This function sends a disconnect/connect link command to 
		p_server.

    Input:	cmd - the command (NBC_DISCONNECT or NBC_CONNECT).
		n_lines - number of lines.
		line_ind - line index array.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int ORPGNBC_connect_disconnect_NB_links 
			(int command, int n_lines, int *line_ind)
{
    int len, i;
    Pd_distri_cmd *cmd;

    if (command != NBC_DISCONNECT &&
	command != NBC_CONNECT)
	return (-1);

    if (n_lines <= 0)
	return (0);

    len = sizeof (Pd_distri_cmd) + (n_lines - 1) * sizeof (int);
    cmd = (Pd_distri_cmd *)malloc (len);
    if (cmd == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed");
	return (-1);
    }

    /* set the command and post an event */
    if (command == NBC_CONNECT)
	cmd->command = CMD_CONNECT;
    else
	cmd->command = CMD_DISCONNECT;
    cmd->n_lines = n_lines;
    for (i = 0; i < n_lines; i++)
	cmd->line_ind[i] = line_ind[i];
    EN_post (ORPGEVT_PD_LINE, cmd, len, 0);
    free (cmd);

    return (0);
}

/**************************************************************************

    Description: This function sends an APUP status request command to 
		p_server.

    Input:	line_ind - line index or one of
			NBC_ALL_DIAL_IN, NBC_ALL_DEDICATED, NBC_ALL_LINES.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int ORPGNBC_request_APUP_status (int line_ind)
{
    int len, i, cnt;
    Pd_distri_info *hd;
    Pd_distri_cmd *cmd;
    Pd_line_entry *line;

    len = Get_line_info ((char **)&hd);
    if (len < 0)
	return (-1);

    if (hd->n_lines <= 0)
	return (Unlock_return (0, (char *)hd));

    len = sizeof (Pd_distri_cmd) + (hd->n_lines - 1) * sizeof (int);
    cmd = (Pd_distri_cmd *)malloc (len);
    if (cmd == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed");
	return (Unlock_return (-1, (char *)hd));
    }

    line = (Pd_line_entry *)((char *)hd + hd->line_list);
    cnt = 0;
    for (i = 0; i < hd->n_lines; i++) {
	if ((line->line_ind == line_ind) ||
	    (line_ind == NBC_ALL_DIAL_IN && 
					line->line_type == DIAL_IN) ||
	    (line_ind == NBC_ALL_DEDICATED && 
					line->line_type == DEDICATED) ||
	    (line_ind == NBC_ALL_LINES)) {
	    cmd->line_ind[cnt] = line->line_ind;
	    cnt++;
	    if (line_ind >= 0)
		break;
	}
	line++;
    }
    cmd->n_lines = cnt;

    /* post an event */
    cmd->command = CMD_REQUEST_APUP_STATUS;
    if (cnt > 0)
	len = sizeof (Pd_distri_cmd) + (cnt - 1) * sizeof (int);
    else
	len = sizeof (Pd_distri_cmd);
    EN_post (ORPGEVT_PD_LINE, cmd, len, 0);
    free (cmd);

    return (Unlock_return (0, (char *)hd));
}

/**************************************************************************

    Description: This function sends a narrow band link control command 
		"command" to the p_server. 

    Input:	command - the command (CMD_*, defined in prod_distri_info.h).
		line_ind - line index involved (used by line related 
			commands only). 

    Return:	0 on success or -1 on failure.

**************************************************************************/

int ORPGNBC_send_NB_link_control_command (int command, int line_ind)
{
    Pd_distri_cmd cmd;

    if (command != CMD_SHUTDOWN &&
	command != CMD_SWITCH_TO_INACTIVE &&
	command != CMD_SWITCH_TO_ACTIVE)
	return (-1);

    /* set the command and post an event */
    cmd.command = command;
    cmd.n_lines = 1;
    cmd.line_ind[0] = line_ind;
    EN_post (ORPGEVT_PD_LINE, &cmd, sizeof (Pd_distri_cmd), 0);

    return (0);
}

/**************************************************************************

    Description: This function sends a V&V control command 
		"command" to the p_server.

    Input:	command - the command (CMD_*, defined in prod_distri_info.h).

    Return:	0 on success or -1 on failure.

**************************************************************************/

int ORPGNBC_send_NB_vv_control_command (int command)
{
    Pd_distri_cmd cmd;

    if (command != CMD_VV_ON &&
	command != CMD_VV_OFF )
	return (-1);

    /* set the command and post an event */
    cmd.command = command;
    cmd.n_lines = 0;
    EN_post (ORPGEVT_PD_LINE, &cmd, sizeof (Pd_distri_cmd), 0);

    return (0);
}

/**************************************************************************

    Description: Returns the number of listed NB lines.

    Return:	number of listed NB lines or -1 on failure.

**************************************************************************/

int ORPGNBC_n_lines ()
{
    int len, n_lines;
    char *buf;
    Pd_distri_info *hd;

    /* read the PD_LINE_INFO_MSG_ID message */
    len = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, LB_ALLOC_BUF, 
						PD_LINE_INFO_MSG_ID);
    if (len <= 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGNBC: ORPGDA_read PD_LINE_INFO_MSG_ID failed (ret %d)\n", len);
	return (Unlock_return (-1, buf));
    }
    if (Verify_length (len, buf) < 0) {
	free (buf);
	return (-1);
    }

    hd = (Pd_distri_info *)buf;
    n_lines = hd->n_lines;
    free (buf);
    return (n_lines);
}

/**************************************************************************

    Description: Verifies the PD_LINE_INFO_MSG_ID message length.

    Input:	len - the message length.
		buf - the message.

    Return:	0 if the length is fine or -1 on failure.

**************************************************************************/

static int Verify_length (int len, char *buf)
{
    Pd_distri_info *hd;

    /* verify length */
    hd = (Pd_distri_info *)buf;
    if (len < (int)sizeof (Pd_distri_info) ||
	len != hd->line_list + hd->n_lines * sizeof (Pd_line_entry)) {
	LE_send_msg (GL_ERROR, 
	    "ORPGNBC: unexpected msg (PD_LINE_INFO_MSG_ID) length (%d)\n", 
								len);
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Description: This function opens ORPGDAT_PROD_INFO and reads the 
		PD_LINE_INFO_MSG_ID message for update. It also locks
		for massage.

    Output:	pointer to the message.

    Return:	length of the message or -1 on failure.

**************************************************************************/

static int Get_line_info (char **msg)
{
    int ret, len;
    char *buf;

    /* open the LB for writing (we must use an invalid argument) */
    ret = ORPGDA_write (ORPGDAT_PROD_INFO, NULL, -1, PD_LINE_INFO_MSG_ID);
    if (ret < 0 && ret != LB_BAD_ARGUMENT) {
	LE_send_msg (GL_ERROR, 
	    "ORPGNBC: ORPGDA_write ORPGDAT_PROD_INFO failed (ret %d)", ret);
	return (-1);
    }

    /* lock the message */
    ret = LB_lock (ORPGDA_lbfd (ORPGDAT_PROD_INFO), 
			LB_EXCLUSIVE_LOCK | LB_BLOCK, PD_LINE_INFO_MSG_ID);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGNBC: LB_lock ORPGDAT_PROD_INFO failed (ret %d)", ret);
	return (-1);
    }

    /* read the PD_LINE_INFO_MSG_ID message */
    buf = NULL;
    len = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, LB_ALLOC_BUF, 
						PD_LINE_INFO_MSG_ID);
    if (len <= 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGNBC: ORPGDA_read PD_LINE_INFO_MSG_ID failed (ret %d)\n", len);
	return (Unlock_return (-1, buf));
    }

    /* verify length */
    if (Verify_length (len, buf) < 0)
	return (Unlock_return (-1, buf));

    *msg = buf;
    return (len);
}

/**************************************************************************

    Description: This function releases the lock on message 
		PD_LINE_INFO_MSG_ID in data store ORPGDAT_PROD_INFO. It
		also frees a temporary buffer.

    Input:	ret_value - the return value.
		buf - buf to be freed.

    Return:	the argument "ret_value".

**************************************************************************/

static int Unlock_return (int ret_value, char *buf)
{

    if (buf != NULL)
	free (buf);
    LB_lock (ORPGDA_lbfd (ORPGDAT_PROD_INFO), 
			LB_UNLOCK, PD_LINE_INFO_MSG_ID);
    return (ret_value);
}

/**************************************************************************

    Description: This function reads a user profile completed with
		class info.

    Input:	line_type - DEDICATED, DIAL_IN, DIAL_OUT.
		uid - user id. line_type != DEDICATED only.
		line_ind - line_type = DEDICATED only.

    Output:	up - returns the pointer to the user profile. NULL on failure.

    Return:	0 on success or a negative error number on failure.

**************************************************************************/

#define COPY_A_BIT(a,b,mask) (((a) & (~mask)) | ((b) & mask))

int ORPGNBC_get_user_profile (int line_type, 
			int uid, int line_ind, Pd_user_entry **up)
{
    int size0, err;
    Pd_user_entry *up0, *up1, *uup;

    if (line_type == DEDICATED)			/* dedicated line */
	up0 = Find_user_profile (UP_LINE_USER, line_ind, 0, &err);
    else					/* dial line */
	up0 = Find_user_profile (UP_DIAL_USER, uid, 0, &err);
    if (up0 == NULL)
	return (err);

    if (up0->defined & UP_DEFINED_CLASS) {	/* read incorporated class */
	if (up0->defined & UP_DEFINED_DISTRI_METHOD)
	    up1 = Find_user_profile (UP_CLASS, (int)up0->class,
					(int)up0->distri_method, &err);
	else
	    up1 = Find_user_profile (UP_CLASS, (int)up0->class, -1, &err);
	if (up1 == NULL) {
	    free (up0);
	    return (err);
	}
    }
    else
	up1 = NULL;

    if (up1 == NULL)
	uup = (Pd_user_entry *)malloc (up0->entry_size);
    else
	uup = (Pd_user_entry *)malloc (up0->entry_size + up1->entry_size);
    if (uup == NULL) {
	LE_send_msg (GL_ERROR, "NBC: malloc failed");
	free (up0);
	free (up1);
	return (-1);
    }

    memcpy ((char *)uup, (char *)up0, up0->entry_size);

    size0 = up0->entry_size;
    if (up1 != NULL) {
	memcpy ((char *)uup + size0, (char *)up1, up1->entry_size);
	if (!(uup->defined & UP_CD_OVERRIDE))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_OVERRIDE);
	if (!(uup->defined & UP_CD_APUP_STATUS))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_APUP_STATUS);
	if (!(uup->defined & UP_CD_RPGOP))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_RPGOP);
	if (!(uup->defined & UP_CD_ALERTS))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_ALERTS);
	if (!(uup->defined & UP_CD_COMM_LOAD_SHED))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, 
							UP_CD_COMM_LOAD_SHED);
	if (!(uup->defined & UP_CD_STATUS))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_STATUS);
	if (!(uup->defined & UP_CD_MAPS))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_MAPS);
	if (!(uup->defined & UP_CD_PROD_GEN_LIST))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_PROD_GEN_LIST);
	if (!(uup->defined & UP_CD_PROD_DISTRI_LIST))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, 
						UP_CD_PROD_DISTRI_LIST);
	if (!(uup->defined & UP_CD_RCM))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_RCM);
	if (!(uup->defined & UP_CD_DAC))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_DAC);
	if (!(uup->defined & UP_CD_MULTI_SRC))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_MULTI_SRC);
	if (!(uup->defined & UP_CD_AAPM))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_AAPM);
	if (!(uup->defined & UP_CD_IMM_DISCON))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_IMM_DISCON);
	if (!(uup->defined & UP_CD_NO_SCHEDULE))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_NO_SCHEDULE);
	if (!(uup->defined & UP_CD_FREE_TEXTS))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, UP_CD_FREE_TEXTS);
	if (!(uup->defined & UP_CD_RESTRICTED_RCM))
	    uup->cntl = COPY_A_BIT (uup->cntl, up1->cntl, 
							UP_CD_RESTRICTED_RCM);

	if (!(uup->defined & UP_DEFINED_MAX_CONNECT_TIME))
	    uup->max_connect_time = up1->max_connect_time;
	if (!(uup->defined & UP_DEFINED_N_REQ_PRODS))
	    uup->n_req_prods = up1->n_req_prods;
	if (!(uup->defined & UP_DEFINED_WAIT_TIME_FOR_RPS))
	    uup->wait_time_for_rps = up1->wait_time_for_rps;
	if (!(uup->defined & UP_DEFINED_DISTRI_METHOD))
	    uup->distri_method = up1->distri_method;

	if (!(uup->defined & UP_DEFINED_PMS)) {
	    uup->pms_len = up1->pms_len;
	    uup->pms_list = up1->pms_list + size0;
	}
	if (!(uup->defined & UP_DEFINED_DD)) {
	    uup->dd_len = up1->dd_len;
	    uup->dd_list = up1->dd_list + size0;
	}
	if (!(uup->defined & UP_DEFINED_MAP)) {
	    uup->map_len = up1->map_len;
	    uup->map_list = up1->map_list + size0;
	}

	uup->defined = up0->defined | up1->defined;
	uup->entry_size = up0->entry_size + up1->entry_size;
	free (up1);
    }
    free (up0);

    /* set default values */
    if (!(uup->defined & UP_DEFINED_MAX_CONNECT_TIME))
	uup->max_connect_time = 1440;
    if (!(uup->defined & UP_DEFINED_PMS))
	uup->pms_len = 0;
    if (!(uup->defined & UP_DEFINED_DD))
	uup->dd_len = 0;
    if (!(uup->defined & UP_DEFINED_MAP))
	uup->map_len = 0;
    if (!(uup->defined & UP_DEFINED_N_REQ_PRODS))
	uup->n_req_prods = 31;
    if (!(uup->defined & UP_DEFINED_WAIT_TIME_FOR_RPS))
	uup->wait_time_for_rps = 0;
			/* we use 0 for optional rps */
    if (!(uup->defined & UP_DEFINED_DISTRI_METHOD))
	uup->distri_method = 0;
    *up = uup;
    return (0);
}

/**************************************************************************

    Description: This function reads a user profile entry.

    Input:	up_type - user profile type to search.
		search_ind - user profile index to search. class for 
			UP_CLASS, line_ind for UP_LINE_USER, and 
			user_id for UP_DIAL_USER.
		distri_method - distribution method (UP_CLASS only).

    Output:	err - error number on failure.

    Return:	The pointer to the user profile on success or NULL on failure.

**************************************************************************/

static Pd_user_entry *Find_user_profile (int up_type, 
				int search_ind, int distri_method, int *err)
{
    void *qr;			/* query results */
    int ret;
    char buf[256], *lb_name;

    sprintf (buf, "up_type = %d", up_type);

    if (up_type == UP_LINE_USER)
	sprintf (buf + strlen (buf), " and line_ind = %d", search_ind);
    else if (up_type == UP_DIAL_USER)
	sprintf (buf + strlen (buf), " and user_id = %d", search_ind);
    else if (up_type == UP_CLASS) {
	sprintf (buf + strlen (buf), " and class_num = %d", search_ind);
	if (distri_method >= 0)
	    sprintf (buf + strlen (buf), 
				" and distri_method = %d", distri_method);
    }
    else {
	LE_send_msg (GL_ERROR, "Unexpected up_type %d\n", up_type);
	return (NULL);
    }

    lb_name = ORPGDA_lbname (ORPGDAT_USER_PROFILES);
    if (lb_name == NULL) {
	LE_send_msg (GL_ERROR,
		"ORPGNBC: User profile DB LB name not found\n");
	return (NULL);
    }
    ret = SDQ_select (lb_name, buf, (void **)&qr);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "SDQ_select (%s) failed, ret %d\n", buf, ret);
	*err = ret;
	return (NULL);
    }

    if (SDQ_get_n_records_returned (qr) > 0) {
	RPG_up_rec_t *rec;
	int len;
	char *buf;

	SDQ_get_query_record (qr, 0, (void **)&rec);
				/* user the first record */
	len = ORPGDA_read (ORPGDAT_USER_PROFILES, 
				&buf, LB_ALLOC_BUF, rec->msg_id);
	free (qr);
	if (len <= 0) {		/* This should not happen normally */
	    LE_send_msg (GL_ERROR, 
		"ORPGDA_read (ORPGDAT_USER_PROFILES %d) failed (ret %d)\n", 
							rec->msg_id, len);
	    *err = len;
	    return (NULL);
	}
	return ((Pd_user_entry *)buf);
    }
    else {			/* not found in the data base */
	LE_send_msg (GL_ERROR, 
		"Not found in UP data base: up_type %d, search_ind %d, distri_method %d",
		up_type, search_ind, distri_method);
	free (qr);
	*err = -1;
	return (NULL);
    }
}

