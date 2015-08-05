/**************************************************************************

      Module:  orpgmgr.c

 **************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/12/03 20:26:45 $
 * $Id: orpgmgr.c,v 1.57 2012/12/03 20:26:45 jing Exp $
 * $Revision: 1.57 $
 * $State: Exp $
 */


#include <limits.h>            /* INT_MAX                                 */
#include <stdlib.h>            /* free(), malloc()                        */
#include <time.h>
#include <errno.h>
#include <string.h>
#include <debugassert.h>

#include <orpg.h>
#include <mrpg.h>
#include <misc.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Static Global Variables

 */

#define MAX_NAME_SIZE 128
#define MAX_CMD_SIZE 128
#define TMP_BUF_SIZE 80
#define IP_BUF_SIZE 32

#define SYSCFG_SEARCH_RPC_TIME 5
		/* RPC expiration time for searching sys cfg version info */
#define SYSCFG_READ_RPC_TIME 15
		/* RPC expiration time for reading sys cfg from mrpg */

static Mrpg_state_t	ORPGMGR_rpg_state;
				/* Latest RPG state data read from LB */
static int		ORPGMGR_rpg_state_updated   = 1;
				/* RPG State message needs to be reread if 1 */
static int		ORPGMGR_registered          = 0;
				/* Set to 1 if LB notification registered. */
static int		ORPGMGR_task_status_updated = 1;
				/* Tasks status message needs to be reread if 1 */

static char *Sys_cfg_ver = NULL;	/* The current sys cfg version line */
static int Qh_registered = 0;

static int Channel;			/* channel num of this application */

typedef struct {
    char ip[IP_BUF_SIZE];
    int connected;			/* Currently is connected */
    int lost_conn;			/* Connection lost counter */
} Remote_host_t;

static int N_hosts = 0;
static Remote_host_t *Hosts = NULL;

/* variables used for handling satellite connection */
static int Owr_client_stopped = 0;
static unsigned int Owr_mrpg_ip;

/*
 * Static Function Prototypes
 */
static void Sys_cfg_cb (EN_id_t event, char *msg, int msg_len, void *arg);
static int Search_for_sys_cfg (int channel);
static void Query_host_cb (EN_id_t event, char *msg, int msg_len, void *arg);
static time_t Get_sys_cfg_version_time (char *ver, int channel, char *ip);
static char *Get_next_token (char *str, int *len);
static int Copy_sys_cfg (char *cfg_ver);
static int Read_sys_cfg (char *cfg_ver, char **scfg);
static int I_am_active_node (char *cfg_ver);
static int Is_channel_match (char *ver, int channel);
static int Get_mrpg_name (char *cfg_ver, char *mrpg_hname, int buf_size);
static int Get_remote_ver (char *ip, char **ver);
static int Is_redundant_rpg (char *ver);
static int Register_sys_cfg_event (int channel, void (*cb)());
static int Create_dirs ();
static int Query_rssd ();
static int Print_node_info (char *ip, char *hname, char *site_info);
static int Get_rhosts (Remote_host_t **rhosts);
static int Handle_satellite_conn (char *mrpg_hname, 
					char *cfg_name, int channel);
static void Owr_cmd_cb (EN_id_t event, char *msg, int len, void *arg);


/**************************************************************************

     Sends command "cmd" without message to the RPG manager (mrpg).

     Return 0 on success or -1 on failure.
 
**************************************************************************/
int ORPGMGR_send_command (int cmd) {
    return (ORPGMGR_send_msg_command (cmd, ""));
}

/**************************************************************************

    Issues an "mrpg cleanup" command.

    Returns 0 on success or -1 on failure:

    Note:  This function should only be used for debugging.

**************************************************************************/
int ORPGMGR_cleanup(){

    char mrpg_cleanup_cmd[] = "mrpg cleanup &";
    int status;

   status = MISC_system_to_buffer( mrpg_cleanup_cmd, NULL, 0, NULL ); 

   if( status < 0 )
      LE_send_msg( GL_ERROR, "ORPGMGR: MISC_system_to_buffer (%s) Failed (%d)\n",
                   mrpg_cleanup_cmd, status );

   return status;

}

/**************************************************************************

     Sends command "cmd" with message "msg" to the RPG manager (mrpg). If 
     mrpg is not running, it returns immediately. If the mrpg command LB 
     is not ready, it reties up to 100 seconds.

     Return 0 on success or -1 on failure.
 
**************************************************************************/
int ORPGMGR_send_msg_command (int cmd, char *msg) {

    Mrpg_cmd_t c;
    int ret;

    ret = ORPGDA_write_permission(ORPGDAT_MRPG_CMDS);
    ret = ORPGMGR_is_mrpg_up ();
    if (ret < 0)
	return (-1);
    else if (ret == 0) {
	LE_send_msg (GL_INFO, "ORPGMGR: mrpg is not running");
	return (-1);
    }
    c.id = MRPG_CMD_ID;
    c.mrpg_cmd = cmd;
    if (ret >= 0){
	char *vb, *p;
	int size;

	vb = NULL;
	if (strlen (msg) > 0) {
	    vb = STR_append (vb, (char *)&c, sizeof (Mrpg_cmd_t));
	    vb = STR_append (vb, msg, strlen (msg) + 1);
	    p = vb;
	    size = STR_size (vb);
	}
	else {
	    p = (char *)&c;
	    size = sizeof (Mrpg_cmd_t);
	}

        ret = ORPGDA_write (ORPGDAT_MRPG_CMDS, p, size, LB_ANY);
	STR_free (vb);
        if (ret != size){

	    LE_send_msg (GL_INFO, 
                     "ORPGMGR: ORPGDA_write mrpg cmd failed (ret %d)", ret);
	    return (-1);
	}
    }
    else{

	LE_send_msg(GL_INFO, 
           "ORPGMGR: Could not open mrpg cmd buffer with write permission (ret %d)",
            ret);
	return(-1);
    }
    return (0);
}

/******************************************************************

   Description:
      Returns the RPG state file name in buffer "name" of size "size".

    Returns 0 on success or -1 on failure.
	
******************************************************************/
int ORPGMGR_state_file_name (char *name, int size) {

    char *projdir, *n;

    projdir = getenv ("ORPGDIR");
    if (projdir == NULL) {
	LE_send_msg (GL_INFO, "ORPGMGR: Environ ORPGDIR not defined");
	return (-1);
    }
    n = "/rpg_state";
    if (strlen (projdir) + strlen (n) >= size) {
	LE_send_msg (GL_INFO, 
		"ORPGMGR: ORPGMGR_state_file_name buffer too small");
	return (-1);
    }
    strcpy (name, projdir);
    strcat (name, n);
    return (0);
}

/******************************************************************

    Returns 1 if mrpg is up and running, 0 if it is not or -1 on 
    failure. It waits 10 seconds if mrpg is not running because mrpg
    restarts itself with in 5 seconds.
	
******************************************************************/
int ORPGMGR_is_mrpg_up (){

   int fd, ret;

   /* mrpg locks message id 1 whenever it is running */
   fd = ORPGDA_lbfd (ORPGDAT_MRPG_CMDS);
   if (fd >= 0){
	time_t st = MISC_systime (NULL);
	while (1) {
	    ret = LB_lock (fd, LB_TEST_SHARED_LOCK, 1);
	    if (ret == LB_HAS_BEEN_LOCKED)
		return (1);
	    else if (ret < 0) {
		LE_send_msg( GL_INFO,
			"ORPGMGR: LB_lock mrpg state failed (ret %d)", ret);
		return (-1);
	    }
	    if (MISC_systime (NULL) >= st + 10)
		return (0);
	    msleep (500);
	}
   }
   else {
	LE_send_msg (GL_INFO, 
                   "ORPGMGR: ORPGDA_lbfd mrpg state failed (ret %d)", fd);
	return (-1);
   }

   return (0);
}

/**************************************************************************

     Reads the current RPG state message.

     Return 0 on success or a negative error number on failure.
 
**************************************************************************/

void
ORPGMGR_lb_updated (
int	fd,
LB_id_t	msg_id,
int	msg_info,
void	*arg
)
{
    switch (msg_id) {

	case MRPG_RPG_STATE_MSGID :

	    ORPGMGR_rpg_state_updated   = 1;
	    break;

	case MRPG_PT_MSGID :

	    ORPGMGR_task_status_updated = 1;
	    break;

    }
}
/**************************************************************************

     Reads the current RPG state message.

     Return 0 on success or a negative error number on failure.
 
**************************************************************************/

int ORPGMGR_read_RPG_states () {

    int ret;

    if (!ORPGMGR_registered) {

	ret = LB_UN_register (ORPGDA_lbfd(ORPGDAT_TASK_STATUS),
			      LB_ANY, ORPGMGR_lb_updated);

	if (ret != LB_SUCCESS) {

	    LE_send_msg (GL_ERROR,
		"ORPGMGR: LB_UN_register (ORPGDAT_TASK_STATUS) failed (ret %d)",
		ret);
	    return (ret);

	}

	ORPGMGR_registered = 1;

    }

    ret = ORPGDA_read (ORPGDAT_TASK_STATUS, (char *)&ORPGMGR_rpg_state, 
			sizeof (Mrpg_state_t), MRPG_RPG_STATE_MSGID);
    if( ret < 0 )
       return (ret);
    if (ret < (int)sizeof (Mrpg_state_t))
	return (-1);

    return (ret);
}

/**************************************************************************

     Returns the current RPG state structure.

     Return 0 on success or a negative error number on failure.
 
**************************************************************************/

int ORPGMGR_get_RPG_states (Mrpg_state_t *rpg_state) {

    int ret;

    if (ORPGMGR_rpg_state_updated) {

	ORPGMGR_rpg_state_updated = 0;

	ret = ORPGMGR_read_RPG_states ();
	if (ret < 0) {
	    ORPGMGR_rpg_state_updated = 1;
	    return (ret);
	}
	if (ret < (int)sizeof (Mrpg_state_t)) {
	    ORPGMGR_rpg_state_updated = 1;
	    return (-1);
	}
    }

    memcpy (rpg_state, &ORPGMGR_rpg_state, sizeof (Mrpg_state_t));
    return (0);
}

/**************************************************************************

     Reports to mrpg that this process has completed the intialization and
     is ready to operate.
 
**************************************************************************/

void ORPGMGR_report_ready_for_operation () {
    char buf[64];

    sprintf (buf, "%d", (int)getpid ());
    EN_post_msgevent (ORPGEVT_PROCESS_READY, buf, strlen (buf) + 1);
}

/**************************************************************************

     waits for ORPG to become operational.
		 wait_time is time in seconds.

     Return 0 on success or -1 on failure.
 
**************************************************************************/
int ORPGMGR_wait_for_op_state(time_t wait_time){

   Mrpg_state_t rpg_state;
   time_t st;
   int	  ret;

   st = MISC_systime (NULL);
   while(1){

/*    Force a reread of the message here since events may have been	*
 *    blocked.								*/

      ret = ORPGMGR_read_RPG_states ();
      ret = ORPGMGR_get_RPG_states(&rpg_state);

      if( (ret == 0) &&
	  (rpg_state.state == MRPG_ST_OPERATING) )
         return (0);


      if (MISC_systime (NULL) > st + wait_time){

         LE_send_msg(GL_ERROR, "ORPGMGR: ORPGMGR_wait_for_op_mode, timed out");
	 return (-1);

      }
      msleep(1000);
   }
}

/**************************************************************************

     Gets the command that generated the current RPG state.

     Return 0 on success or -1 on failure.
 
**************************************************************************/

int ORPGMGR_get_rpgcmd_in_progress (int *cmd) {

    int ret;

    if (ORPGMGR_rpg_state_updated) {

	ORPGMGR_rpg_state_updated   = 0;

	ret = ORPGMGR_read_RPG_states ();

	if (ret < 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGMGR: ORPGDA_read RPG state failed (ret %d)", ret);
	    return (-1);
	}
    }

    if (ORPGMGR_rpg_state.state != MRPG_ST_TRANSITION)
	*cmd = -1;
    else if (ORPGMGR_rpg_state.cmd == MRPG_STARTUP)
	*cmd = MRPG_STARTUP;
    else if (ORPGMGR_rpg_state.cmd == MRPG_RESTART)
	*cmd = MRPG_RESTART;
    else if (ORPGMGR_rpg_state.cmd == MRPG_STANDBY)
	*cmd = MRPG_STANDBY;
    else if (ORPGMGR_rpg_state.cmd == MRPG_SHUTDOWN)
	*cmd = MRPG_SHUTDOWN;
    else
	*cmd = -1;

    return (0);

}


/**************************************************************************

    Gets status of task "task_name" and "instance".

 **************************************************************************/
int ORPGMGR_get_task_status(char *task_name, int instance, int *state)
{
    Mrpg_process_table_t *entry ;
    char *entry_p;
    int retval ;

    static char buf[ALIGNED_SIZE(sizeof(Mrpg_process_table_t)) + ORPG_TASKNAME_SIZ];

    if( task_name == NULL )
       return(-1);

    entry = (Mrpg_process_table_t *) buf ; 
    entry->instance = instance ;
    entry->name_off = ALIGNED_SIZE(sizeof(Mrpg_process_table_t));

    entry_p = (char *) buf;
    strncpy( entry_p + entry->name_off, task_name, ORPG_TASKNAME_SIZ );
    (entry_p + entry->name_off)[ORPG_TASKNAME_SIZ - 1] = '\0';
    
    retval = ORPGMGR_read_task_status(entry) ;
    if (retval < 0) {
        LE_send_msg( GL_ERROR,"ORPGMGR: ORPGMGR_read_task_status(<%s, %d>) failed: %d",
                     task_name, entry->instance, retval) ;
        return(-1) ;
    }
    *state = entry->status;
    return(0) ;
}

/**************************************************************************
   Description: 
      Read an ORPG process table entry from the LB file.

   Input: 
      pointer to the process table entry

   Output: 
      the message is read from the LB file and placed in the storage
      provided

   Returns: 
      0 upon success; -1 otherwise

   Notes:
      We "key" on the Task ID and Instance Number values in the
      entry.

 **************************************************************************/
int ORPGMGR_read_task_status(Mrpg_process_table_t *entry_p){

    static char *buf_p = NULL;
    int retval, cnt, found;
    Mrpg_process_table_t *pt;
    char *cpt, *ecpt;

    if (entry_p == NULL){

        LE_send_msg( GL_ERROR, "ORPGMGR: Must Pass Non-Null Mrpg_process_table_t Entry\n" );
        return(-1) ;
    }

    if (!ORPGMGR_registered) {

	retval = LB_UN_register (ORPGDA_lbfd(ORPGDAT_TASK_STATUS),
			      LB_ANY, ORPGMGR_lb_updated);

	if (retval != LB_SUCCESS) {

	    LE_send_msg (GL_ERROR,
		"ORPGMGR: LB_UN_register (ORPGDAT_TASK_STATUS) failed (ret %d)",
		retval);
	    return (retval);

	}

	ORPGMGR_registered = 1;

    }

    if (buf_p != NULL) {

	free (buf_p);

    }

    retval = ORPGDA_read (ORPGDAT_TASK_STATUS, 
				&buf_p, LB_ALLOC_BUF, MRPG_PT_MSGID) ;

    if (retval < 0 && retval != LB_NOT_FOUND) {
        LE_send_msg (GL_ERROR, 
	    "ORPGMGR: ORPGDA_read MRPG_PT_MSGID failed (ret %d)", retval) ;
        return(-1) ;
    }

    pt = (Mrpg_process_table_t *)buf_p;
    cpt = (char *)buf_p;
    ecpt = (char *)entry_p;
    cnt = sizeof (Mrpg_process_table_t);
    found = 0;
    while (cnt <= retval) {

	if (strcmp( (cpt + pt->name_off), (ecpt + entry_p->name_off) ) == 0 ) {
	    if (pt->instance < 0 ||
		pt->instance == entry_p->instance) {
		entry_p->status = pt->status;
		entry_p->pid = pt->pid;
		found = 1;
		break;
	    }
	}
	cnt += pt->size;
	cpt += pt->size;
        pt = (Mrpg_process_table_t *) cpt;
    }
    if (retval > 0) {

	free(buf_p);
	buf_p = NULL;

    }

    if (!found)
	entry_p->status = MRPG_PS_NOT_STARTED;

    return(0) ;
}

/**************************************************************************

    Starts rpgdbm. If "db_name", which is the DB (LB) name, contains the host
    name, the rpgdbm is started on the host.

**************************************************************************/

void ORPGMGR_start_rpgdbm (char *db_name) {
    static char *cmd = NULL;
    int ret, fret, n, host_len, pid;
    time_t st_t;
    char buf[4], *p;

    p = db_name;
    while (*p != '\0' && *p != ':'  && *p != '/')
	p++;
    if (*p == ':')
	host_len = p - db_name;
    else
	host_len = 0;

    cmd = STR_reset (cmd, 0);
    if (host_len > 0) {
	cmd = STR_append (cmd, db_name, host_len);
	cmd = STR_append (cmd, ":", 2);
    }
    else
	cmd = STR_copy (cmd, "");
    cmd = STR_cat (cmd, "MISC_system_to_buffer");
    ret = RSS_rpc (cmd, "i-r s-i ba-4-o i-i ia-1-o", &fret, 
				"bash -l -c \"start_rssd\"", buf, 4, &n);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: RSS_rpc (%s) failed (%d)\n", cmd, ret);
	return;
    }
    ret = RSS_rpc (cmd, "i-r s-i ba-4-o i-i ia-1-o", &pid, 
				"rpgdbm -t 300 &", buf, 4, &n);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: RSS_rpc (%s, rpgdbm) failed (%d)\n", cmd, ret);
	return;
    }
    if (pid <= 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: %s rpgdbm failed (%d)\n", cmd, pid);
	return;
    }
    st_t = MISC_systime (NULL);
    while (1) {				/* wait for rpgdbm to be ready */
	char *tqr;
	sleep (1);
   	if ((ret = SDQ_select (db_name, "$&gdtT%@3_", (void *)&tqr)) != 
							CSS_SERVER_DOWN)
	    return;
	if (MISC_systime (NULL) > st_t + 20) {
	    LE_send_msg (GL_ERROR, "ORPGMGR: rpgdbm startup timed out\n");
	    return;
	}
    }
}

/**************************************************************************

    Returns the mrpg host name. Returns 0 on success or a negative error
    code on failure. This function should be called after calling
    ORPGMGR_setup_sys_cfg.

 **************************************************************************/

int ORPGMGR_get_mrpg_host_name (char *mrpg_hname, int buf_size) {
    char buf[256];
    int ret;

    mrpg_hname[0] = '\0';
    if (Sys_cfg_ver == NULL)
	return (ORPGMGR_NO_SYS_CFG_FOUND);
    
    ret = Get_mrpg_name (Sys_cfg_ver, buf, 256);
    if (ret >= buf_size)
	return (ORPGMGR_BAD_ARGUMENT);
    if (ret > 0) {
	strcpy (mrpg_hname, buf);
	if (I_am_active_node (Sys_cfg_ver))
	    return (1);
	return (0);
    }
    return (ORPGMGR_NO_SYS_CFG_FOUND);
}

/***************************************************************************

    Returns 1 if a host is found to match node name "node" and channel
    "chan". The ip address is returned in buffer "ipb" of size
    "ipb_s". The local host IP is returned as an empty string. It
    returns 0, if the host is not found, or a negative error code.

***************************************************************************/

#define TOK_BUF_SIZE 32

int ORPGMGR_discover_host_ip (char *node, int chan, char *ipb, int ipb_s) {
    char *node_name;
    int ret, i, warn, n_hosts;
    Remote_host_t *rhosts;

    warn = 0;
    if (strcasecmp (node, "rpg") == 0) {
	node_name = "rpga";
	warn = 1;
    }
    else if (strcasecmp (node, "bdds") == 0) {
	node_name = "rpgb";
	warn = 1;
    }
    else
	node_name = node;
    if (warn)
	LE_send_msg (0, 
	"ORPGMGR: ORPGMGR_discover_host_ip called with obsolete node name (%s)", node);

    ret = Query_rssd ();
    if (ret < 0)
	return (ret);
    n_hosts = Get_rhosts (&rhosts);
    for (i = 0; i <= n_hosts; i++) {
	char *ip, func[MAX_CMD_SIZE];
	char *ret_v, type[TOK_BUF_SIZE], ch[TOK_BUF_SIZE];
	int found, t_out, is_connected, cnt;

	ip = "";
	is_connected = 1;
	if (i > 0) {
	    if (!rhosts[i - 1].connected)
		continue;
	    ip = rhosts[i - 1].ip;
	    is_connected = RMT_is_connected (ip);
	}

	found = 0;
	sprintf (func, "%s:liborpg.so,ORPGMISC_get_site_name", ip);
	t_out = RMT_set_time_out (SYSCFG_SEARCH_RPC_TIME);
	type[0] = ch[0] = '\0';
	ret_v = NULL;
	cnt = 0;	/* try twice to recover from remote host reboot */
	while ((ret = RSS_rpc (func, "s-r s-i", &ret_v, "type")) < 0 &&
		cnt < 2 && ret != RMT_TIMED_OUT)
	    cnt++;
	if (ret >= 0) {
	    if (ret_v != NULL)
		strncpy (type, ret_v, TOK_BUF_SIZE);
	    ret_v = NULL;
	    ret = RSS_rpc (func, "s-r s-i", &ret_v, "channel_num");
	    if (ret >= 0 && ret_v != NULL)
		strncpy (ch, ret_v, TOK_BUF_SIZE);
	}
	type[TOK_BUF_SIZE - 1] = ch[TOK_BUF_SIZE - 1] = '\0';
	if (ret >= 0 && strcasecmp (type, node_name) == 0) {
	    if (strcmp (ch, "2") == 0) {
		if (chan == 2)
		    found = 1;
	    }
	    else {
		if (chan != 2)
		    found = 1;
	    }
	}
	if (!is_connected)
	    RMT_close_connection ();
	RMT_set_time_out (t_out);
	if (found) {
	    strncpy (ipb, ip, ipb_s);
	    ipb[ipb_s - 1] = '\0';
	    return (1);
	}
    }
    return (0);
}

/***************************************************************************

    Prints node info of all hosts that configured in rssd conf.

***************************************************************************/

int ORPGMGR_print_node_info () {
    return (ORPGMGR_each_node (Print_node_info));
}

static int Print_node_info (char *ip, char *hname, char *site_info) {
    LE_send_msg (0, "%s (%s): %s", ip, hname, site_info);
    return (0);
}

/***************************************************************************

    Calls "cb" on each host that is configured in rssd conf with site info
    on the host.

***************************************************************************/

int ORPGMGR_each_node (int (*cb) (char *, char *, char *)) {
    int ret, i, n_hosts;
    Remote_host_t *rhosts;

    ret = Query_rssd ();
    if (ret < 0)
	return (ret);
    n_hosts = Get_rhosts (&rhosts);
    for (i = 0; i <= n_hosts; i++) {
	char *ip, func[MAX_CMD_SIZE], hname[TMP_BUF_SIZE];
	char *ret_v;
	int t_out, is_connected;

	ip = "";
	hname[0] = '\0';
	is_connected = 1;
	if (i > 0) {
	    ip = rhosts[i - 1].ip;
	    NET_get_name_by_ip (NET_get_ip_by_name (ip), hname, TMP_BUF_SIZE);
	    if (rhosts[i - 1].connected == 0) {
		cb (ip, hname, "No connection");
		continue;
	    }
	    is_connected = RMT_is_connected (ip);
	}

	sprintf (func, "%s:liborpg.so,ORPGMISC_get_site_name", ip);
	if (i == 0)
	    ip = "local";
	t_out = RMT_set_time_out (SYSCFG_SEARCH_RPC_TIME);
	ret_v = NULL;
	ret = RSS_rpc (func, "s-r s-i", &ret_v, "");
	if (ret >= 0) {
	    if (ret_v != NULL)
		cb (ip, hname, ret_v);
	    else
		cb (ip, hname, "No site info");
	}
	else {
	    char b[128];
	    sprintf (b, "No RSS_rpc access (%d)", ret);
	    cb (ip, hname, b);
	}

	if (!is_connected)
	    RMT_close_connection ();
	RMT_set_time_out (t_out);
    }
    return (0);
}

/**************************************************************************

    Sets up the system configuration file and the access environment for
    "channel". If "dont_update" is false (zero), the current RPG system
    configuration is searched and copied to the local host if necessary.
    Otherwise, the local version is assumed to exist and up-to-date. If
    "dont_update" is false (zero), "channel" must be either 1 or 2
    because we do not know if the RPG is single channel or channel 1 in
    the redundant RPG. If, however, "dont_update" is true, "channel"
    must be 0, 1, or 2 where 0 must be used if the local host is active.
    This function sets up: a. The CS default system configuration file
    name. b. The EN group number. c. The ORPGDA for inactive host. The
    EN group number is set to override any setup by RMTPORT. This
    function closes the RPC connections. Thus this functon should only
    be called in the very beginning of the application. If "cb" is not
    NULL, event ORPGEVT_DATA_STORE_CREATED is registered so the
    application can restart upon the event. "cb" must be a valid AN
    callback function (see man en).

    Returns 0, if the local host is an active RPG node, or the channel
    number (1 or 2) if not active. Returns a negative error code on faulure.

    The procedure of installling sys_cfg can only be performed once.
    This function cannot be called again if a failure happens after
    certain point in the procedure. Such a call will return
    ORPGMGR_FATAL_ERROR upon which the caller should terminate.

    We assume that our buffer size (MAX_NAME_SIZE) here is sufficient
    for any possible sys_cfg name. To allow this function to be called
    repeatedly, we save the original sys_cfg name so we never use the
    changed sys_cfg name.

 **************************************************************************/

int ORPGMGR_setup_sys_cfg (int channel, int dont_update, void (*cb)()) {
    static int prev_call_state = -1;
    static char org_cfg[MAX_NAME_SIZE] = "";
    int ret, need_update, chan, sat_conn;
    char cfg_name[MAX_NAME_SIZE], *version, mrpg_hname[MAX_NAME_SIZE];

    if (prev_call_state >= 0)
	return (prev_call_state);
			/* previoud call to this function succeeded */
    if (prev_call_state == -2)
	return (ORPGMGR_FATAL_ERROR);
			/* previoud call to this function fatally failed */

    sat_conn = channel & ORPGMGR_SAT_CONN;
    channel &= ORPGMGR_CHANNEL_MASK;
    if (channel < 0 || channel > 2 ||
	(!dont_update && channel == 0))
	return (ORPGMGR_BAD_ARGUMENT);
    Channel = channel;
    CS_cfg_name ("");
    if (org_cfg[0] == '\0')
	strcpy (org_cfg, MISC_basename (CS_cfg_name (NULL)));
    strcpy (cfg_name, org_cfg);	/* avoid using changed sys sfg name */
    if (channel == 2)
	strcat (cfg_name, ".2");
    else if (channel == 1)
	strcat (cfg_name, ".1");

    if (cb != NULL &&
	(ret = Register_sys_cfg_event (channel, cb)) < 0)
	return (ret);

    /* search for the current RPG system config for "channel" */
    if (Sys_cfg_ver != NULL)
	free (Sys_cfg_ver);
    Sys_cfg_ver = NULL;
    if (dont_update) {
	CS_set_sys_cfg (cfg_name);
	ret = ORPGMGR_get_syscfg_version (&version);
	if (ret < 0)
            LE_send_msg (GL_ERROR, "ORPGMGR: existing sys cfg not found");
	else {
	    Sys_cfg_ver = malloc (strlen (version) + 1);
	    if (Sys_cfg_ver == NULL) {
		LE_send_msg (GL_ERROR, "ORPGMGR: malloc failed");
		return (ORPGMGR_MALLOC_FAILED);
	    }
	    strcpy (Sys_cfg_ver, version);
	}
    }
    else {
	if ((ret = Search_for_sys_cfg (channel)) < 0)
	    return (ret);
    }
    if (Sys_cfg_ver == NULL)
	return (ORPGMGR_NO_SYS_CFG_FOUND);

    if (I_am_active_node (Sys_cfg_ver)) {
	/* reset EN group because Register_sys_cfg_event has changed it */
	if ((chan = Is_redundant_rpg (Sys_cfg_ver)))
	    EN_control (EN_SET_AN_GROUP, chan);
	prev_call_state = 0;
	return (0);
    }

    if ((ret = Create_dirs ()) < 0)
	return (ret);

    need_update = 0;
    if (!dont_update) {
	CS_set_sys_cfg (cfg_name);
	ret = ORPGMGR_get_syscfg_version (&version);
	if (ret < 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGMGR: existing sys cfg (%s) not found - will copy", 
							cfg_name);
	    need_update = 1;
	}
	else if (strcmp (Sys_cfg_ver, version) != 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGMGR: existing sys cfg (%s) needs update", cfg_name);
	    need_update = 1;
	}
	if (need_update) {
	    CS_control (CS_CLOSE);
	    CS_cfg_name ("");
	    if ((ret = Copy_sys_cfg (Sys_cfg_ver)) < 0)
		return (ret);
	}
    }

    if ((chan = Is_redundant_rpg (Sys_cfg_ver)))
	EN_control (EN_SET_AN_GROUP, chan);
    else
	EN_control (EN_SET_AN_GROUP, -1);

    /* set up ORPGDA for non-active host */
    ret = Get_mrpg_name (Sys_cfg_ver, mrpg_hname, MAX_NAME_SIZE);
    if (ret == 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: mrpg name not found in sys_cfg");
	prev_call_state = -2;
	return (ORPGMGR_MRPG_NOT_FOUND);
    }
    ret = ORPGDA_reset_sys_cfg_local_host (mrpg_hname);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: Failed in resetting ORPGDA");
	prev_call_state = -2;
	return (ret);
    }

    if (sat_conn) {
	ret = Handle_satellite_conn (mrpg_hname, cfg_name, channel);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "ORPGMGR: Failed in Handle_satellite_conn");
	    prev_call_state = -2;
	    return (ret);
	}
    }

    if (channel <= 1) {
	prev_call_state = 1;
	return (1);
    }
    else {
	prev_call_state = 2;
	return (2);
    }
}

/**************************************************************************

    Search for the mrpg host name and returns it with "mrpg_hn" on success. 
    "buf_size" is the buffer size of "mrpg_hn". If channel = 0, the 
    search is on the local host. Otherwise, it searches on the network.
    It returns the channel number found (1 or 2) on success or a negative
    error code.

 **************************************************************************/

int ORPGMGR_search_mrpg_host_name (int channel, char *mrpg_hn, int buf_size) {
    char *version;
    int ret;

    if (channel == 0) {		/* local search */
/*	CS_set_sys_cfg ("sys_cfg"); */
	ret = ORPGMGR_get_syscfg_version (&version);
	if (ret < 0)
            return (ORPGMGR_NO_SYS_CFG_FOUND);
    }
    else {
	if (Sys_cfg_ver != NULL)
	    free (Sys_cfg_ver);
	Sys_cfg_ver = NULL;
	Channel = channel;
	ret = Search_for_sys_cfg (channel);
	if (ret < 0)
	    return (ret);
	version = Sys_cfg_ver;
    }
    if (Get_mrpg_name (version, mrpg_hn, buf_size) == 0)
	return (ORPGMGR_NO_SYS_CFG_FOUND);
    if (Is_channel_match (version, 1))
	return (1);
    if (Is_channel_match (version, 2))
	return (2);
    return (ORPGMGR_NO_SYS_CFG_FOUND);
}

/**************************************************************************

    Creates the system config dir and the data dir. Returns 0 on success
    or a negative error code.

 **************************************************************************/

static int Create_dirs () {
    char buf[256];
    int ret;

    ret = MISC_get_cfg_dir (buf, 256);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: Config dir not defined");
	return (ret);
    }
    ret = MISC_mkdir (buf);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: Cannot create dir %s", buf);
	return (ORPGMGR_MKDIR_FAILED);
    }

    ret = LE_dirpath (buf, 256);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: LE dir not defined");
	return (ret);
    }
    ret = MISC_mkdir (buf);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: Cannot create dir %s", buf);
	return (ORPGMGR_MKDIR_FAILED);
    }
    return (0);
}

/**************************************************************************

    Registers AN callback function "cb" for the RPG sys cfg update event.
    "channel" (1 or 2) is the RPG channel to connect to.

 **************************************************************************/

static int Register_sys_cfg_event (int channel, void (*cb)()) {
    int ret;

    if (channel == 2) {
	EN_control (EN_SET_AN_GROUP, channel);
	ret = EN_register (ORPGEVT_DATA_STORE_CREATED, cb);
    }
    else {	/* We don't know redundant type, so we register for both */
	EN_control (EN_SET_AN_GROUP, 1);
	ret = EN_register (ORPGEVT_DATA_STORE_CREATED, cb);
	if (ret < 0)
	    return (ret);
	EN_control (EN_SET_AN_GROUP, -1);	/* for single channel */
	ret = EN_register (ORPGEVT_DATA_STORE_CREATED, cb);
    }
    return (ret);
}

/**************************************************************************

    Determines if the local host is an active node according to the system
    config version line "cfg_ver". Returns 1 if it is or 0 if not. The
    channel number is also verified.

 **************************************************************************/

static int I_am_active_node (char *cfg_ver) {
    char *p, ver_tmp[TMP_BUF_SIZE];
    int n_nodes, i;
    unsigned int ip, hip;

    strncpy (ver_tmp, cfg_ver, TMP_BUF_SIZE);
    ver_tmp[TMP_BUF_SIZE - 1] = '\0';	/* for error printing */

    if ((p = strstr (cfg_ver, "n_nodes")) == NULL ||
	(p = Get_next_token (p, NULL)) == NULL ||
	sscanf (p, "%d", &n_nodes) != 1 ||
	n_nodes < 1) {
        LE_send_msg (GL_INFO, 
		"ORPGMGR: Bad sys cfg version (%s) - ignored", ver_tmp);
	return (0);
    }

    hip = NET_get_ip_by_name ("");		/* local host address */
    if (hip == INADDR_NONE) {
        LE_send_msg (GL_INFO, 
	    "ORPGMGR: NET_get_ip_by_name (finding local IP) failed - ignored");
	return (0);
    }

    for (i = 0; i < n_nodes; i++) {
	int len;
	char h[MAX_NAME_SIZE];

	if ((p = Get_next_token (p, &len)) == NULL ||
	    len <= 0 || len >= MAX_NAME_SIZE) {
            LE_send_msg (GL_INFO, 
		"ORPGMGR: Unexpected sys cfg version (%s) - ignored", ver_tmp);
	    return (0);
	}
	memcpy (h, p, len);
	h[len] = '\0';
	ip = NET_get_ip_by_name (h);
	if (ip == INADDR_NONE)
	    continue;
	if (ip == hip)
	    return (1);
    }
    return (0);
}

/**************************************************************************

    Copies the sys cfg from the mrpg host to the local host. Returns 1 
    on success or a negative error code.

 **************************************************************************/

static int Copy_sys_cfg (char *cfg_ver) {
    int len, fd, ret;
    char *cfg_name, *tmp_file, *scfg, mrpg_hname[128];

    scfg = NULL;
    len = Read_sys_cfg (cfg_ver, &scfg);
    if (len < 0)
	return (len);
    cfg_name = CS_cfg_name (NULL);
    tmp_file = malloc (strlen (cfg_name) + 8);
    if (tmp_file == NULL) {
        LE_send_msg (GL_ERROR, 
		"ORPGMGR: malloc failed in ORPGMGR_update_sys_cfg");
	return (ORPGMGR_MALLOC_FAILED);
    }
    sprintf (tmp_file, "%s.tmp", cfg_name);
    fd = open (tmp_file, O_WRONLY | O_CREAT | O_TRUNC, 0660);
    if (fd < 0) {
        LE_send_msg (GL_ERROR, 
	    "ORPGMGR: open (create) %s failed (errno %d)", tmp_file, errno);
	free (tmp_file);
	return (ORPGMGR_CREATE_FILE_FAILED);
    }
    ret = write (fd, scfg, len);
    close (fd);
    if (ret != len) {
        LE_send_msg (GL_ERROR, 
	    "ORPGMGR: write sys cfg failed (%d, errno %d)", ret, errno);
	free (tmp_file);
	return (ORPGMGR_CREATE_FILE_FAILED);
    }
    ret = rename (tmp_file, cfg_name);
    free (tmp_file);
    if (ret < 0) {
        LE_send_msg (GL_ERROR, 
	    	"ORPGMGR: rename sys cfg failed (errno %d)", errno);
	return (ORPGMGR_CREATE_FILE_FAILED);
    }
    Get_mrpg_name (cfg_ver, mrpg_hname, 128);
    LE_send_msg (GL_INFO, 
	"ORPGMGR: sys cfg (channel %d) copied from %s", Channel, mrpg_hname);
    return (1);
}

/**************************************************************************

    Reads the sys cfg file from the mrpg host as in sys cfg version line
    "cfg_ver". On success, the contents is returned with "scfg" and the
    number of bytes is returned with the function return value. On failure,
    a negative error code is returned. The caller needs to free the pointer
    returned by "scfg" on success.

 **************************************************************************/

static int Read_sys_cfg (char *cfg_ver, char **scfg) {
    char *p, mrpg_hname[MAX_NAME_SIZE], func[MAX_NAME_SIZE + 64];
    int t_out, ret, ret_value, comp_on;

    if (Get_mrpg_name (cfg_ver, mrpg_hname, MAX_NAME_SIZE) == 0)
	return (ORPGMGR_MRPG_NAME_NOT_FOUND);	/* mrpg node name not found */

    sprintf (func, "%s:liborpg.so,ORPGMGR_read_sys_cfg", mrpg_hname);
    t_out = RMT_set_time_out (SYSCFG_READ_RPC_TIME);
    comp_on = RMT_set_compression (RMT_COMPRESSION_ON);
    ret = RSS_rpc (func, "i-r s-o", &ret_value, &p);
    RMT_close_connection ();
    RMT_set_time_out (t_out);
    RMT_set_compression (comp_on);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGMGR: RSS_rpc (%s) failed (%d)", func, ret);
	return (ret);
    }
    if (ret_value < 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGMGR: Remote (%s) ORPGMGR_read_sys_cfg failed (%d)", 
			    mrpg_hname, ret_value);
	return (ret_value);
    }

    *scfg = p;
    return (ret_value);
}

/**************************************************************************

    Reads and returns the local system config file with "scfg" as a 
    null-terminated string. The size of the file is returned. On failure,
    a negative error code is returned. Because this function is called 
    remotely, we do not print LE messages. This function is user internally.

 **************************************************************************/

int ORPGMGR_read_sys_cfg (char **scfg) {
    static char *buf = NULL;
    int fd, size, ret;

    CS_cfg_name ("");
    fd = open (CS_cfg_name (NULL), O_RDONLY);
    if (fd < 0)
	return (ORPGMGR_OPEN_FAILED);
    size = lseek (fd, 0, SEEK_END);
    if (size <= 0) {
	close (fd);
	return (ORPGMGR_LSEEK_FAILED);
    }
    lseek (fd, 0, SEEK_SET);
    if (buf != NULL)
	free (buf);
    buf = malloc (size + 1);
    if (buf == NULL) {
	close (fd);
	return (ORPGMGR_MALLOC_FAILED);
    }
    ret = read (fd, buf, size);
    close (fd);
    if (ret != size)
	return (ORPGMGR_READ_FAILED);
    buf[size] = '\0';
    *scfg = buf;
    return (size);
}

/**************************************************************************

    Deregister EN_QUERY_HOSTS registration.

 **************************************************************************/

int ORPGMGR_deregister_query_hosts () {
    EN_control (EN_DEREGISTER);
    EN_register (EN_QUERY_HOSTS, Query_host_cb);
    Qh_registered = 0;
    return (0);
}

/**************************************************************************

    Checks connectivity of "n_ips" IP addresses "ips" (In net work byte
    order). The status is returned in "stat". The LSB of "stat" is 1 for
    connected, or 0 for not connected. The other bits of "stat" is the 
    number of lost connection (to "ip") events since this function is 
    first called. "stat" is set to -1 if the connectivity info for "ip" is
    not available. Returns 1 on success or a negative error code on
    failure.

 **************************************************************************/

int ORPGMGR_check_connectivity (int n_ips, unsigned int *ips, int *stat) {
    int ret, i;

    ret = Query_rssd ();
    if (ret < 0)
	return (ret);

    EN_control (EN_BLOCK);
    for (i = 0; i < n_ips; i++)
	stat[i] = -1;
    for (i = 0; i < N_hosts; i++) {
	int k;
	unsigned int ip;

	ip = NET_get_ip_by_name (Hosts[i].ip);
	for (k = 0; k < n_ips; k++) {
	    if (ips[k] == ip) {
		stat[k] = (Hosts[i].lost_conn << 1) | Hosts[i].connected;
	    }
	}
    }
    if (Owr_client_stopped) {
	int k;
	for (k = 0; k < n_ips; k++) {
	    if (ips[k] == Owr_mrpg_ip)
		stat[k] &= (~1);
	}
    }
    EN_control (EN_UNBLOCK);
    return (1);
}

/**************************************************************************

    Searches the current system configuration file for RPG channel "channel".
    First, mrpg is queried. If it does not respond, all remote hosts are
    searched for the latest time stamp. "Sys_cfg_ver" is updated on success.
    "Sys_cfg_ver" must be set to NULL before calling this. Returns 0 on 
    success or a negative error code.

 **************************************************************************/

static int Search_for_sys_cfg (int channel) {
    Remote_host_t *rhosts;
    int n_hosts, ret, i;
    time_t max_ver_time;
    time_t st_t;
    char *max_ver;

    /* query mrpg to get the sys cfg version info */
    ret = EN_register (ORPGEVT_SYSTEM_CONFIG_INFO, Sys_cfg_cb);
    if (ret < 0) {
        LE_send_msg (GL_ERROR, 
	    "ORPGMGR: EN_register SYSTEM_CONFIG_INFO failed (%d)", ret);
	return (ret);
    }
    EN_post_event (ORPGEVT_SYSTEM_CONFIG_INFO);
    st_t = MISC_systime (NULL);
    while (Sys_cfg_ver == NULL) {	/* wait 2 seconds for mrpg response */
	sleep (1);
	if (MISC_systime (NULL) >= st_t + 2)
	    break;
    }
    if (Sys_cfg_ver != NULL)	/* mrpg response came */
	return (0);

    if ((ret = Query_rssd ()) < 0)
	return (ret);

    /* RPC all remote hosts - search the max time stamp */
    max_ver = NULL;
    max_ver_time = 0;
    n_hosts = Get_rhosts (&rhosts);
    for (i = 0; i <= n_hosts; i++) {
	char *ver, *ip;
	time_t ver_time;

	if (Sys_cfg_ver != NULL) {	/* mrpg response came */
	    if (max_ver != NULL)
		free (max_ver);
	    return (0);
	}

	if (i == 0) {			/* local host */
	    ret = ORPGMGR_get_syscfg_version (&ver);
	    if (ret < 0)
		continue;
	    ip = "";			/* local host name */
	}
	else {				/* remote host */
	   if (!rhosts[i - 1].connected)
		continue;
	    ip = rhosts[i - 1].ip;
	    ret = Get_remote_ver (ip, &ver);
	    if (ret == 0)
		continue;
	}

	ver_time = Get_sys_cfg_version_time (ver, channel, ip);
	if (ver_time > max_ver_time) {
	    max_ver_time = ver_time;
	    if (max_ver != NULL)
		free (max_ver);
	    max_ver = malloc (strlen (ver) + 1);
	    if (max_ver == NULL) {
        	LE_send_msg (GL_ERROR, 
			"ORPGMGR: malloc in Search_for_sys_cfg failed");
		return (ORPGMGR_MALLOC_FAILED);
	    }
	    strcpy (max_ver, ver);
	}
    }

    EN_control (EN_BLOCK);
    if (Sys_cfg_ver == NULL && max_ver_time > 0) {
	char mrpg_hname[128];
	Sys_cfg_ver = max_ver;
        Get_mrpg_name (Sys_cfg_ver, mrpg_hname, 128);
	LE_send_msg (GL_INFO, 
	    "ORPGMGR: sys cfg version (channel %d) got from host %s", 
						Channel, mrpg_hname);
    }
    else if (max_ver != NULL)
	free (max_ver);
    EN_control (EN_UNBLOCK);

    if (Sys_cfg_ver == NULL)
	return (ORPGMGR_NO_SYS_CFG_FOUND);
    return (0);
}

/**************************************************************************

    Register EN_QUERY_HOSTS for getting host names and connectivity from 
    rssd. returns 0 on success or a negative error on faulure.

 **************************************************************************/

static int Query_rssd () {

    /* query rssd get the list of all remote hosts */
    if (!Qh_registered) {
	int ret = EN_register (EN_QUERY_HOSTS, Query_host_cb);
	if (ret < 0) {
            LE_send_msg (GL_ERROR, 
	    "ORPGMGR: EN_register EN_QUERY_HOSTS failed (%d)", ret);
	    return (ret);
	}
	Qh_registered = 1;
    }

    return (0);
}

/**************************************************************************

    Takes a snapshot of N_hosts and Hosts.

***************************************************************************/

static int Get_rhosts (Remote_host_t **rhosts) {
    static char *buf = NULL;
    int n_hosts;

    EN_control (EN_BLOCK);
    n_hosts = N_hosts;
    buf = STR_reset (buf, n_hosts * sizeof (Remote_host_t));
    memcpy (buf, Hosts, n_hosts * sizeof (Remote_host_t));
    EN_control (EN_UNBLOCK);
    *rhosts = (Remote_host_t *)buf;
    return (n_hosts);
}

/**************************************************************************

    Reads the sys cfg version line on host "ip". The version line is 
    returned with "ver". Returns 1 on success, 0 if the remote host is not
    connected, cannot be accessed, or sys cfg not found on that host, -1
    on failure.

***************************************************************************/

static int Get_remote_ver (char *ip, char **ver) {
    int t_out, ret, ret_value;
    char func[MAX_CMD_SIZE];

    sprintf (func, "%s:liborpg.so,ORPGMGR_get_syscfg_version", ip);
    t_out = RMT_set_time_out (SYSCFG_SEARCH_RPC_TIME);
    ret = RSS_rpc (func, "i-r s-o", &ret_value, ver);
    RMT_close_connection ();
    RMT_set_time_out (t_out);
    if (ret < 0)		/* RSS_rpc failed */
	return (0);
    if (ret_value < 0)		/* remote ORPGMGR_get_syscfg_version failed */
	return (0);
    return (1);
}

/**************************************************************************

    Checks if "channel" matches the sys cfg version line "ver" and the 
    "ip" is mrpg node in terms of "ver". If both are true, returns the time 
    stamp of "ver". Otherwise 0 is returned.

 **************************************************************************/

static time_t Get_sys_cfg_version_time (char *ver, int channel, char *ip) {
    char *p, mrpg_hname[MAX_NAME_SIZE];
    unsigned int ip1, ip2;
    time_t tm;

    if (!Is_channel_match (ver, channel))
	return (0);

    if (Get_mrpg_name (ver, mrpg_hname, MAX_NAME_SIZE) == 0)
	return (0);		/* mrpg node name not found */
    ip1 = NET_get_ip_by_name (ip);
    ip2 = NET_get_ip_by_name (mrpg_hname);
    if (ip1 == INADDR_NONE || ip2 == INADDR_NONE || ip1 != ip2)
	return (0);

    /* get time stamp */
    if ((p = strstr (ver, "Version")) == NULL ||
	(p = Get_next_token (p, NULL)) == NULL ||
	sscanf (p, "%lu", &tm) != 1)
	return (0);
    return (tm);
}

/**************************************************************************

    Returns true (1) if the channel number in sys cfg version line "ver"
    matches channel, or false (0) otherwise.

 **************************************************************************/

static int Is_channel_match (char *ver, int channel) {
    char *p;
    int chan;

    if ((p = strstr (ver, "channel")) == NULL ||
	(p = Get_next_token (p, NULL)) == NULL ||
	sscanf (p, "%d", &chan) != 1)
	return (0);
    if (chan == 0)
	chan = 1;
    if (chan != channel)	/* wrong channel */
	return (0);
    return (1);
}

/**************************************************************************

    Returns FAA channel number (1 or 2) if the RPG is redundant RPG in 
    terms of sys cfg version line "ver", or 0 otherwise.

 **************************************************************************/

static int Is_redundant_rpg (char *ver) {
    char *p;
    int chan;

    if ((p = strstr (ver, "channel")) == NULL ||
	(p = Get_next_token (p, NULL)) == NULL ||
	sscanf (p, "%d", &chan) != 1 ||
	chan == 0)
	return (0);
    return (chan);
}

/**************************************************************************

    Returns the pointer to the next token after the current token pointed
    to by "str". The length of the next token is returned by "len". If
    the next token does not exist, NULL is returned.

 **************************************************************************/

static char *Get_next_token (char *str, int *len) {
    char *p, *t;

    p = str;
    while (*p != '\0' && *p != ' ')
	p++;
    while (*p == ' ')
	p++;
    t = p;
    while (*p != '\0' && *p != ' ')
	p++;
    if (len != NULL)
	*len = p - t;
    if (p - t > 0)
	return (t);
    return (NULL);  
}

/**************************************************************************

    Returns the version line of the local RPG system configuration file
    with "version". It returns 0 on success or a negative error code.
    Because this function is called remotely, we do not send LE messages.
    This function is used internally.

 **************************************************************************/

#define ORPGMGR_SYSCFG_VER_BUF 256

int ORPGMGR_get_syscfg_version (char **version) {
    static char *ver = NULL;
    char buf[ORPGMGR_SYSCFG_VER_BUF];
    int ret;

    if (ver != NULL)
	free (ver);
    ver = NULL;

    CS_cfg_name ("");
    ret = CS_entry ("Version", CS_FULL_LINE, ORPGMGR_SYSCFG_VER_BUF, buf);
    if (ret < 0)
	return (ret);
    if (ret == 0)
	return (ORPGMGR_EMPTY_VERSION_LINE);
    ver = malloc (strlen (buf) + 1);
    if (ver == NULL)
	return (ORPGMGR_MALLOC_FAILED);
    strcpy (ver, buf);
    *version = ver;
    return (0);
}

/**************************************************************************

    Retrieves the mrpg host name from sys cfg version line "cfg_ver". The
    name is copied to "mrpg_hname" of size at least "buf_size". Returns 
    the length of the mrpg host name on success or 0 on failure.

 **************************************************************************/

static int Get_mrpg_name (char *cfg_ver, char *mrpg_hname, int buf_size) {
    char *p;
    int n_nodes, len;

    mrpg_hname[0] = '\0';
    if ((p = strstr (cfg_ver, "n_nodes")) == NULL ||
	(p = Get_next_token (p, NULL)) == NULL ||
	sscanf (p, "%d", &n_nodes) != 1 ||
	n_nodes < 1 ||
	(p = Get_next_token (p, &len)) == NULL ||
	len <= 0 || len >= buf_size) {
        LE_send_msg (GL_ERROR, "ORPGMGR: mrpg name not found");
	return (0);		/* mrpg node name not found */
    }
    memcpy (mrpg_hname, p, len);
    mrpg_hname[len] = '\0';
    return (len);
}

/**************************************************************************

    EN_QUERY_HOSTS event callback - Receives the remote host info.

 **************************************************************************/

static void Query_host_cb (EN_id_t event, char *msg, int msg_len, void *arg) {
    int err, n_hosts, i;
    char tok[256];

    if (msg_len <= 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: Unexpected EN_QUERY_HOSTS evt - no message");
	return;
    }
    err = 0;
    Hosts = (Remote_host_t *)STR_reset ((char *)Hosts, 128);
    if (MISC_get_token (msg, "", 0, tok, 256) > 0 &&
	strcmp (tok, "Remote_hosts:") == 0 &&
	MISC_get_token (msg, "Ci", 1, &n_hosts, 0) > 0 &&
	n_hosts >= 0) {
	for (i = 0; i < n_hosts; i++) {
	    Remote_host_t host;
	    int c, k;

	    if (MISC_get_token (msg, "", 2 + i * 2, tok, 256) <= 0 ||
		MISC_get_token (msg, "Ci", 3 + i * 2, &c, 0) <= 0) {
		err = 1;
		break;
	    }
	    for (k = 0; k < N_hosts; k++) {
		if (strcmp (Hosts[k].ip, tok) == 0) {
		    if (Hosts[k].connected && c == 0)
			Hosts[k].lost_conn++;
		    Hosts[k].connected = c;
		    break;
		}
	    }
	    if (k >= N_hosts) {
		strncpy (host.ip, tok, IP_BUF_SIZE);
		host.ip[IP_BUF_SIZE - 1] = '\0';
		host.connected = c;
		host.lost_conn = 0;
		Hosts = (Remote_host_t *)STR_append ((char *)Hosts, 
				    (char *)&host, sizeof (Remote_host_t));
		N_hosts++;
	    }
	}
    }
    else
	err = 1;
    if (err) {
	if (msg_len >= 150)
	    msg[150] = '\0';
	LE_send_msg (GL_ERROR, "ORPGMGR: Unexpected EN_QUERY_HOSTS evt (%s)", msg);
	return;
    }
}

/**************************************************************************

    SYSTEM_CONFIG_INFO event callback - Receives the sys cfg version info.

 **************************************************************************/

static void Sys_cfg_cb (EN_id_t event, char *msg, int msg_len, void *arg) {

    if (Sys_cfg_ver == NULL && msg_len > 0 &&
	Is_channel_match (msg, Channel)) {
	Sys_cfg_ver = malloc (msg_len);
	if (Sys_cfg_ver == NULL)
            LE_send_msg (GL_ERROR, "ORPGMGR: malloc (%d) failed", msg_len);
	else {
	    char mrpg_hname[128];
	    memcpy (Sys_cfg_ver, msg, msg_len);
	    Sys_cfg_ver[msg_len - 1] = '\0';
	    Get_mrpg_name (Sys_cfg_ver, mrpg_hname, 128);
            LE_send_msg (GL_INFO, 
		"ORPGMGR: sys cfg version (channel %d) got from mrpg (at %s)", 
						Channel, mrpg_hname);
	}
    }
}

/**************************************************************************

    Handles a satellite connection: owr_client is started; owr-modified
    system configuration file is generated and installed.

 **************************************************************************/

#define OWR_CMD_EVENT 23
enum {OWR_NOT_READY, OWR_READY, OWR_SYS_CFG_READY, OWR_FAILD};
/* variables, local to here, used for handling satellite connection */
static time_t Owr_recv_time = 0;
static int Owr_ready, Owr_chan;
static char *Owr_cfg;

static int Handle_satellite_conn (char *mrpg_hname, 
					char *cfg_name, int channel) {
    char cmd[MAX_NAME_SIZE];
    int ret;
    time_t st_t, send_t, t;

    Owr_client_stopped = 0;
    Owr_mrpg_ip = NET_get_ip_by_name (mrpg_hname);
    if (Owr_mrpg_ip == INADDR_NONE) {
	LE_send_msg (GL_ERROR, 
		"ORPGMGR: IP of mrpg host (%s) not found\n", mrpg_hname);
	return (-1);
    }

    if (channel == 0)
	channel = 1;
    sprintf (cmd, "owr_client -c %d -l %s &", channel, mrpg_hname);
    ret = MISC_system_to_buffer (cmd, NULL, 0, NULL);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
			"ORPGMGR: Failed in invoking owr_client (%d)", ret);
	return (-1);
    }

    LE_send_msg (0, "ORPGMGR:    Register event %d\n", OWR_CMD_EVENT + 1);
    ret = EN_register (OWR_CMD_EVENT + 1, Owr_cmd_cb);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR: Failed in EN_register (%d)\n", ret);
	return (-1);
    }
    Owr_chan = channel;
    Owr_ready = OWR_NOT_READY;
    send_t = 0;
    st_t = MISC_systime (NULL);
    Owr_recv_time = st_t;
    sprintf (cmd, "Is owr ready for channel %d - %d", channel, getpid ());
    while (Owr_ready != OWR_READY) {
	time_t t = MISC_systime (NULL);
	if (Owr_ready == OWR_FAILD || t > Owr_recv_time + 5) {
		/* not received response from owr_client for 5 seconds */
	    LE_send_msg (GL_ERROR, "ORPGMGR: owr_client not ready");
	    return (-1);
	}
	if (t != send_t) {	/* we post one each second */
	    send_t = t;
	    LE_send_msg (0, "ORPGMGR:    Post %d - %s\n", OWR_CMD_EVENT, cmd);
	    EN_post_msgevent (OWR_CMD_EVENT, cmd, strlen (cmd) + 1);
	}
	sleep (1);
    }
    send_t = 0;
    st_t = 0;
    sprintf (cmd, "Generate sys_cfg for channel %d from %s - %d", 
					channel, cfg_name, getpid ());
    Owr_cfg = cfg_name;
    while (Owr_ready != OWR_SYS_CFG_READY) {
	t = MISC_systime (NULL);
	if (st_t == 0)
	    st_t = t;
	if (t > st_t + 8) {  /* wait up to 8 seconds for preparing sys cfg */
	    LE_send_msg (GL_ERROR, 
			"ORPGMGR: owr_client failed in generating sys cfg");
	    return (-1);
	}
	if (t != send_t) {	/* we post one each second */
	    send_t = t;
	    LE_send_msg (0, "ORPGMGR:    Post %d - %s\n", OWR_CMD_EVENT, cmd);
	    EN_post_msgevent (OWR_CMD_EVENT, cmd, strlen (cmd) + 1);
	}
	sleep (1);
    }

    CS_control (CS_CLOSE);
    CS_set_sys_cfg (Owr_cfg);
    CS_cfg_name ("");
    LE_send_msg (GL_INFO, "ORPGMGR: sys cfg set to %s", Owr_cfg);

    return (0);
}

/**************************************************************************

    Callback function for owr commands.

 **************************************************************************/

static void Owr_cmd_cb (EN_id_t event, char *msg, int len, void *arg) {
    static char *buf = NULL;
    char resp1[MAX_NAME_SIZE], resp2[MAX_NAME_SIZE];
    char resp3[MAX_NAME_SIZE], resp4[MAX_NAME_SIZE];
    int pid, msgpid;

    if (len <= 0 || msg[len - 1] != '\0')
	return;

    if (len > 0)
	LE_send_msg (0, "ORPGMGR:    Recvd ev %d - %s\n", event, msg);
    else
	LE_send_msg (0, "ORPGMGR:    Recvd ev %d\n", event);
    sprintf (resp1, "owr_client for channel %d restarting", Owr_chan);
    if (strcmp (msg, resp1) == 0) {
	Owr_client_stopped = 1;
	LE_send_msg (GL_INFO, "ORPGMGR: owr_client stopped");
	return;
    }
    if (Owr_client_stopped) {
	sprintf (resp1, "owr_client for channel %d restarted", Owr_chan);
	if (strcmp (msg, resp1) == 0) {
	    Owr_client_stopped = 0;
	    LE_send_msg (GL_INFO, "ORPGMGR: owr_client restarted");
	    return;
	}
    }

    pid = getpid ();
    if (MISC_get_token (msg, "Ci", 0, &msgpid, 0) <= 0 ||
	msgpid != pid)
	return;
    Owr_recv_time = MISC_systime (NULL);
    sprintf (resp1, "%d - owr is ready for channel %d", pid, Owr_chan);
    sprintf (resp2, "%d - sys_cfg for channel %d generated: ", pid, Owr_chan);
    sprintf (resp3, "%d - owr is initializing for channel %d", pid, Owr_chan);
    sprintf (resp4, "%d - owr is not ready for channel %d", pid, Owr_chan);
    if (strcmp (msg, resp4) == 0) {
	LE_send_msg (GL_INFO, "ORPGMGR: owr_client reps: owr is not ready for channel %d", Owr_chan);
	Owr_ready = OWR_FAILD;
	return;
    }
    if (Owr_ready == OWR_NOT_READY && strcmp (msg, resp1) == 0) {
	Owr_ready = OWR_READY;
	return;
    }
    else if (Owr_ready == OWR_NOT_READY && strcmp (msg, resp3) == 0)
	return;
    else if (Owr_ready == OWR_READY && 
				strncmp (msg, resp2, strlen (resp2)) == 0) {
	Owr_ready = OWR_SYS_CFG_READY;
	buf = STR_copy (buf, msg + strlen (resp2));
	Owr_cfg = buf;
	return;
    }
    else 
	LE_send_msg (GL_INFO, "ORPGMGR: owr_client response ignored: %s", msg);
}

#define ETC_BOOT_TIME "/etc/boot_time"

/******************************************************************

    Sets the ETC_BOOT_TIME file to record the fact that the OS crash
    has been processed. This is called after the RPG data stores are 
    cleaned up to remove any possible corruption due to OS crash.
    Returns 1 on failure or 0 on success.
	
******************************************************************/

int ORPGMGR_set_os_crash_fixed () {
    FILE *fl;
    char *text;

    if (ORPGMGR_was_os_crashed () == 0) 
	return (0);
    fl = fopen (ETC_BOOT_TIME, "a+");
    if (fl == NULL) {
	LE_send_msg (GL_ERROR,
		"ORPGMGR: Can not open %s for write", ETC_BOOT_TIME);
	return (1);
    }
    text = "\nfixed\n";
    if (fwrite (text, sizeof (char), strlen (text), fl) != strlen (text))
	LE_send_msg (GL_ERROR, "ORPGMGR: Can not write %s", ETC_BOOT_TIME);
    else
	LE_send_msg (0, "ORPGMGR: %s fixed", ETC_BOOT_TIME);
    fclose (fl);
    return (0);
}

/******************************************************************

    Returns true if the OS is detected to be crashed before booting.
	
******************************************************************/

int ORPGMGR_was_os_crashed () {
    enum {BOOT_MSG, SHUTDOWN_MSG, FIXED_MSG, NO_MSG};
    FILE *fl;
    char buf[128];
    int prev_line, last_line, os_crashed;

    fl = fopen (ETC_BOOT_TIME, "r");
    if (fl == NULL) {
	LE_send_msg (GL_ERROR, 
	"ORPGMGR: File %s not found - OS crash assumed", ETC_BOOT_TIME);
	return (1);
    }
    prev_line = last_line = NO_MSG;
    while ((fgets (buf, 128, fl)) != NULL) {
	int new_line;

	buf[127] = '\0';
	if (strncmp (buf, "boot", 4) == 0)
	    new_line = BOOT_MSG;
	else if (strncmp (buf, "shutdown", 8) == 0)
	    new_line = SHUTDOWN_MSG;
	else if (strncmp (buf, "fixed", 5) == 0)
	    new_line = FIXED_MSG;
	else
	    continue;
	prev_line = last_line;
	last_line = new_line;
    }
    if (last_line == FIXED_MSG)
	os_crashed = 0;
    else if (last_line != BOOT_MSG)
	os_crashed = 1;
    else {
	if (prev_line == SHUTDOWN_MSG || prev_line == NO_MSG)
	    os_crashed = 0;
	else
	    os_crashed = 1;
    }
    if (os_crashed)
	LE_send_msg (GL_ERROR, 
	    "ORPGMGR: OS crash assumed in terms of %s", ETC_BOOT_TIME);

    fclose (fl);
    return (os_crashed);
}


