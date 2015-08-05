/****************************************************************
 *								*
 *	hci_nb_main.c - The main module for hci_nb (Narrow Band *
 *	status and control).					*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/06/01 20:28:55 $
 * $Id: hci_nb_main.c,v 1.77 2011/06/01 20:28:55 jing Exp $
 * $Revision: 1.77 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_nb_def.h>

#define SV_ADDR_SIZE 256

static Hci_pd_block_t *Pd_block = NULL;	/* block of pd/line info */
static int N_lines = 0;			/* number of lines */
static int Line_info_updated = 1;	/* line info updated */
static int Pd_msg_len = 0;		/* length of the line info msg */
static int WB_line_ind = 0;		/* Line index of RDA/RPG WB */

static void Pu_event_callback (en_t evtcd, void *msg, size_t msglen);
static void Pl_callback (int fd, LB_id_t msg_id, int msg_info, void *arg);
static void Pl_event_callback (en_t evtcd, void *msg, size_t msglen);
static int Read_nb_line_info ();


/**********************************************************************
    Description: The main function.

    Input:  argc - number of commandline arguments
	    argv - pointer to commandline argument data
    Output: NONE
    Return: exit code
***********************************************************************/

int main (int argc, char *argv[])
{
    int ret;

    /*  Initialize HCI */

    HCI_init( argc, argv, HCI_NB_TASK );

    ORPGDA_write_permission( ORPGDAT_PROD_INFO );

    RSS_set_SMI_func_name ("liborpg.so,ORPG_smi_info");

    /* register update events */

    if ((ret = EN_register (ORPGEVT_PROD_USER_STATUS, (void *) Pu_event_callback)) < 0) {

	HCI_LE_error("EN_register failed (%d)", ret);
	HCI_task_exit (HCI_EXIT_FAIL);

    }

    if ((ret = EN_register (ORPGEVT_PD_LINE, (void *) Pl_event_callback)) < 0) {

	HCI_LE_error("EN_register failed (%d)", ret);
	HCI_task_exit (HCI_EXIT_FAIL);

    }

    if (ORPGDA_UN_register (ORPGDAT_HCI_DATA, HCI_PROD_INFO_STATUS_MSG_ID,
                            Pl_callback) != LB_SUCCESS) {
	HCI_LE_error("ORPGDA_UN_register (ORPGDAT_HCI_DATA) failed");
	HCI_task_exit (HCI_EXIT_FAIL);
    }

    GUI_main (argc, argv);

    return 0;
}

/**********************************************************************

    Description: The callback function called from the GUI main-loop
		 periodically. It updates the status display.

    Input:  NONE
    Output: NONE
    Return: NONE

***********************************************************************/

void MAIN_housekeeping ()
{
    int i, updated;
    static int first_time = 1;

    if (Line_info_updated) {
	HCI_LE_status("Line info updated");
        if ((Read_nb_line_info () < 0) && (first_time)) {

            HCI_task_exit (HCI_EXIT_FAIL);

	} else {

	    first_time = 1;

	}
    }

/*  The first time through we need to get the line index for the	*
 *  RDA/RPG wideband so we can exclude it from the lines list.		*/

    if (first_time) {

	WB_line_ind = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

    }

/*  Check each line to see if it has been updated.  If so, update the	*
 *  local data.								*/

    updated = 0;
    for (i = 0; i < N_lines; i++) {
	int line_ind;
	Line_status ls;

	if (Pd_block->pd_line_info[i].not_used == 0) {
	    continue;
	}
	Pd_block->pd_line_info[i].not_used = 0;

	/* read status info */
	line_ind = Pd_block->pd_line_info[i].line_ind;
	if (line_ind < 0)
	    break;
	if (line_ind == WB_line_ind)
	    continue;

	ls.line_num = line_ind;
	ls.type = Pd_block->pd_line_info[i].line_type;
	ls.protocol = Pd_block->pd_line_info[i].protocol;
	ls.state = (short) Pd_block->pd_user_status[i].line_stat;
	if (Pd_block->pd_user_status[i].link == US_DISCONNECTED)
		ls.status = US_DISCONNECTED;
	else if (Pd_block->pd_user_status[i].link == US_CONNECTED)
		ls.status = US_CONNECTED;
	else
		ls.status = US_CONNECT_PENDING;

	if (ls.status == US_CONNECTED) {
	    int rate;

	    ls.user_id = Pd_block->pd_user_status[i].uid;
	    ls.util = Pd_block->pd_user_status[i].util;
	    rate = Pd_block->pd_user_status[i].rate;
	    if (rate > 1000000000)
		rate = 1000000000;
	    if (rate == -1)
		ls.rate = 0xffff;
	    else if (rate >= 10000000)
		ls.rate = (rate + 500000) / 1000000;	/* in MB */
	    else if (rate >= 10000)
		ls.rate = ((rate + 500) / 1000) | 0x8000;	/* in KB */
	    else 
		ls.rate = rate | 0xc000;		/* in byte */
	    ls.uclass = Pd_block->pd_user_status[i].uclass;
	}
	else {
	    ls.user_id = -1;
	    ls.util = 0;
	    ls.rate = 0xffff;
	    ls.uclass = -1;
	}

	if (Pd_block->pd_line_info[i].link_state == LINK_ENABLED)
	    ls.enable = 1;
	else 
	    ls.enable = 0;

	if (first_time)
	    strcpy (ls.str," ");

	updated = 1;
	GUI_update_line_info (&ls);
    }

    first_time = 0;

/*  If any line was updated, then update the display list.		*/

    if (updated)
	GUI_update_line_list ();
}

/**********************************************************************

    Description: Returns the detailed info about line "ls".

    Input:  ls - pointer to line status data
    Output: NONE
    Return: Pointer to the Line_details structure.

***********************************************************************/

Line_details *MAIN_get_line_details (Line_status *ls)
{
	int	lind;
	Line_details	*tmp_ld;
static	Line_details	ld;
	char	*buf;
	int	ret;

/*	Find line index	*/

	for (lind = 0; lind < N_lines; lind++)
	    if (Pd_block->pd_line_info[lind].line_ind == ls->line_num)
		break;
	if (lind >= N_lines) {
	    HCI_LE_error("unexpected line number");
	    lind = -1;

	}

/*	If running locally invoke the function to create the line	*
 *	details data directly.  If not, use the RSSD RPC mechanism to	*
 *	invoke the function on the RPG host.				*/

	if (strlen (ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES)) <= 0) {

	    tmp_ld = hci_up_nb_get_line_details (ls);

	    memcpy (&ld, tmp_ld, sizeof (Line_details));

	} else {

	    buf = (char *) calloc (128,1);

	    if (strlen (ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES))) {

		sprintf (buf,"%s:libhci.so,hci_up_nb_get_line_details",
			  ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES));

	    } else {

		sprintf (buf,"libhci.so,hci_up_nb_get_line_details");

	    }

	    ret = RSS_rpc (buf, "Line_details-r Line_status-i", &tmp_ld, ls);

	    if (ret != 0) {

		HCI_LE_error("RSS_rpc (%s) failed (%d)", buf, ret);
		lind = -1;

	    } else {

		memcpy (&ld, tmp_ld, sizeof (Line_details));

	    }

	    free (buf);

	}

	if (lind <= 0) {

	    strcpy (ld.port_pswd,"");
	    ld.baud_rate     =  0;
	    ld.pserver_num   = -1;
	    ld.comms_mgr_num = -1;
	    ld.max_conn_time =  1;
	    ld.packet_size   =  0;

	} else {

	    strcpy (ld.port_pswd,Pd_block->pd_line_info[lind].port_password);
	    ld.baud_rate     = Pd_block->pd_line_info[lind].baud_rate;
	    ld.pserver_num   = Pd_block->pd_line_info[lind].p_server_ind;
	    ld.comms_mgr_num = Pd_block->pd_line_info[lind].cm_ind;
	    ld.max_conn_time = Pd_block->pd_line_info[lind].conn_time_limit;
	    ld.packet_size   = Pd_block->pd_line_info[lind].packet_size;

	}

	ld.retries = Pd_block->pd_info.nb_retries;
	ld.timeout = Pd_block->pd_info.nb_timeout;

	return (&ld);
}

/**********************************************************************

    Description: Reads NB line info (all lines).

    Input:  NONE
    Output: NONE
    Return: 0 on success; negative on error

***********************************************************************/

static int Read_nb_line_info ()
{
    int ret, i;

    if (Pd_block != NULL){ free( Pd_block ); }

    Pd_block = NULL;
    Pd_msg_len = 0;

    ret = ORPGDA_read (ORPGDAT_HCI_DATA, 
			&Pd_block, LB_ALLOC_BUF, HCI_PROD_INFO_STATUS_MSG_ID);
    if (ret < 0)
    {
        HCI_LE_error("ORPGDA_read (ORPGDAT_HCI_DATA) failed (%d)", ret);
        if( Pd_block != NULL ){ free( Pd_block ); }
	return (ret);
    }
    else if( ret < sizeof( Pd_block ) )
    {
        HCI_LE_error("bad Pd_block msg size (%d)", ret);
        if( Pd_block != NULL ){ free( Pd_block ); }
	return (-1);
    }

    HCI_LE_log("read HCI_PROD_INFO_STATUS_MSG_ID");

    N_lines = Pd_block->pd_info.n_lines;
    Pd_msg_len = sizeof(Pd_distri_info) + (N_lines*sizeof(Pd_line_entry));
    for (i = 0; i < N_lines; i++) {
	Pd_block->pd_line_info[i].not_used = 1;	/* The line info is updated */
    }

    Line_info_updated = 0;

    return (0);
}

/**********************************************************************

    Description: ORPGEVT_PROD_USER_STATUS event callback function.

    Input:  evtcd - event code
	    msg - pointer to event message data
	    msglen - length of event message data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Pu_event_callback (en_t evtcd, void *msg, size_t msglen)
{
    int ln;

    ln = *((char *)msg);
    if (ln >= 0 && ln < N_lines)
	Pd_block->pd_line_info[ln].not_used = 1; /* The line info is updated */
}

/**********************************************************************

    Description: ORPGEVT_PD_LINE event callback function.

    Input:  evtcd - event code
	    msg - pointer to event message data
	    msglen - length of event message data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Pl_event_callback (en_t evtcd, void *msg, size_t msglen)
{
    Line_info_updated = 1;
}

/**********************************************************************

    Description: ORPGDAT_PD_LINE update callback function.

    Input:  fd - ORPGDAT_PD_LINE file descriptor
	    msg_id - ID of message which was updated
	    msg_info - length of updated message
	    arg - unused
    Output: NONE
    Return: NONE

***********************************************************************/

static void Pl_callback (int fd, LB_id_t msg_id, int msg_info, void *arg)
{
    Line_info_updated = 1;
}

/**********************************************************************

    Description: Sends commands to p_server for product distribution 
		control.

    Input:  cmd - the command to send: CMD_DISCONNECT, CMD_DISABLE
		  or CMD_ENABLE.
	    n_list_lines - total number of NB lines.
	    list_lines - line info for all lines.
    Output: NONE
    Return: NONE

***********************************************************************/

void MAIN_nb_control (int cmd, int n_list_lines, Line_status *list_lines)
{
    int buf[128], *p;
    int i, ret, cnt;

    if (n_list_lines <= 128)
	p = buf;
    else {
	p = (int *)malloc (n_list_lines * sizeof (int));
	if (p == NULL) {
	    HCI_LE_error("Malloc (p) failed");
	    return;
	}
    }

    cnt = 0;
    for (i = 0; i < n_list_lines; i++) {
	if (list_lines[i].selected) {
	    p[cnt] = list_lines[i].line_num;
	    cnt++;
	}
    }

    ret = 0;
    switch (cmd) {
	case HCI_NB_DISCONNECT:
	    ret = ORPGNBC_connect_disconnect_NB_links (NBC_DISCONNECT, cnt, p);
	    break;
	case HCI_NB_DISABLE:
	    ret = ORPGNBC_enable_disable_NB_links (NBC_DISABLE_LINK, cnt, p);
	    break;
	case HCI_NB_ENABLE:
	    ret = ORPGNBC_enable_disable_NB_links (NBC_ENABLE_LINK, cnt, p);
	    break;
    }
    if (ret < 0) 
	HCI_LE_error("ORPGNBC send command failed");

    if (p != buf)
	free (p);
}

/******************************************************************

    Description: This function reads the user profile database and
		 builds a table of all dial-in users.  The argument
		 "dial" contains the table.  The function returns
		 the number of table items

    Input:  dial - pointer to dial details data
    Output: NONE
    Return: number of dialin users

******************************************************************/

int hci_update_dialup_user_table( Dial_details *dial )
{
  int	users;
  int	ret;
  char	*tmp = NULL;
  char	*buf = NULL;
  Dial_details_tbl_t *tmp_dial = NULL;

  /* If running locally invoke the function to create the dial
     users table directly.  If not, use the RSSD RPC mechanism to
     invoke the function on the RPG host. */

  if( strlen( ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) ) <= 0 )
  {
    users = hci_up_nb_update_dial_user_table( &tmp );
    tmp_dial = (Dial_details_tbl_t*) tmp;
  }
  else
  {
    buf = (char *) calloc( 128, 1 );

    if( strlen( ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) ) )
    {
      sprintf( buf,"%s:libhci.so,hci_up_nb_update_dial_user_table",
               ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) );
    }
    else
    {
      sprintf( buf,"libhci.so,hci_up_nb_update_dial_user_table" );
    }

    ret = RSS_rpc( buf, "i-r Dial_details_tbl_t-o", &users, &tmp_dial );

    if( ret != 0 )
    {
      HCI_LE_error( "RSS_rpc (%s) failed (%d)", buf, ret );
      users = 0;
    }

    free( buf );
  }

  if( users > 0 )
  {
    memcpy( dial, tmp_dial->data, users*sizeof( Dial_details ) );
  }

  return users;

}

/******************************************************************

    Description: This function reads the user profile database and
		 builds a table of all dedicated users.  The argument
		 "user" contains the table.  The function returns
		 the number of table items.

    Input:  dial - pointer to dial details data
    Output: NONE
    Return: number of dedicated users

******************************************************************/

int hci_update_dedicated_user_table( Dial_details *user )
{
  int	users;
  int	ret;
  char	*buf = NULL;
  char	*tmp = NULL;
  Dial_details_tbl_t *tmp_user = NULL;

  /* If running locally invoke the function to create the dedicated
     users table directly.  If not, use the RSSD RPC mechanism to
     invoke the function on the RPG host. */

  if( strlen( ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) ) <= 0 )
  {
    users = hci_up_nb_update_dedicated_user_table( &tmp );
    tmp_user = (Dial_details_tbl_t *) tmp;
  }
  else
  {
    buf = (char *) calloc( 128, 1 );

    if( strlen( ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) ) )
    {
      sprintf( buf,"%s:libhci.so,hci_up_nb_update_dedicated_user_table",
               ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) );
    }
    else
    {
      sprintf( buf,"libhci.so,hci_up_nb_update_dedicated_user_table" );
    }

    ret = RSS_rpc( buf, "i-r Dial_details_tbl_t-o", &users, &tmp_user );

    if( ret != 0 )
    {
      HCI_LE_error( "RSS_rpc (%s) failed (%d)", buf, ret );
      users = 0;
    }

    free( buf );
  }

  if( users > 0 )
  {
    memcpy( user, tmp_user->data, users*sizeof( Dial_details) );
  }

  return users;
}

/******************************************************************

    Description: This function queries the user_profiles database
		 and returns a table of all defined classes.  A class
		 may be defined more than once for multiple
		 distribution methods.  The table is returned using
		 the "class" argument.  The function returns the number
		 of class entries found.

    Input:  dial - pointer to class details data
    Output: NONE
    Return: number of classes

******************************************************************/

int hci_update_class_table( Class_details *class )
{
  int classes;
  int ret;
  char *buf = NULL;
  char *tmp = NULL;
  Class_details_tbl_t *tmp_class = NULL;

  /* If running locally invoke the function to create the dedicated
     users table directly.  If not, use the RSSD RPC mechanism to
     invoke the function on the RPG host. */

  if( strlen( ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) ) <= 0 )
  {
    classes = hci_up_nb_update_class_table( &tmp );
    tmp_class = (Class_details_tbl_t *) tmp;
  }
  else
  {
    buf = (char *) calloc( 128, 1 );

    if( strlen( ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) ) )
    {
      sprintf( buf,"%s:libhci.so,hci_up_nb_update_class_table",
               ORPGDA_get_data_hostname(ORPGDAT_USER_PROFILES) );
    }
    else
    {
      sprintf( buf,"libhci.so,hci_up_nb_update_class_table" );
    }

    ret = RSS_rpc( buf, "i-r Class_details_tbl_t-o", &classes, &tmp_class );

    if( ret != 0 )
    {
      HCI_LE_error( "RSS_rpc (%s) failed (%d)", buf, ret );
      classes = 0;
    }

    free( buf );
  }

  if( classes > 0 )
  {
    memcpy( class, tmp_class->data, classes*sizeof( Class_details ) );
  }

  return classes;
}
