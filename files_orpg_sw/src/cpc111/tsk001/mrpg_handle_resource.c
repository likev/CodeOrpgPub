
/******************************************************************

	Resource handling in distributed RPG.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/08 20:34:07 $
 * $Id: mrpg_handle_resource.c,v 1.42 2013/07/08 20:34:07 steves Exp $
 * $Revision: 1.42 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <mrpg.h> 
#include <infr.h> 

#include "mrpg_def.h"

#define AUTO_START_WAIT_TIME 120
#define RESTARTUP_TIME 30
#define LOST_NODE_CONN_ALARM_TIME 300

static char Ra_config_name[MRPG_NAME_SIZE] = "resource_table";

static Node_attribute_t *Nodes;		/* node table */
static int N_nodes = 0;			/* number of used nodes */
static void *Node_tbl;			/* node table id */
static int Channel;			/* RPG channel number */
static int N_total_nodes = 0;		/* total number of nodes */
static char *Res_name = NULL;		/* resource name used */

static Node_attribute_t **Node_buffer;	/* shared buffer for returning nodes */

static char *Drs_buffer = NULL;		/* data replication specifications -
					   consequtive null teminat. strings */
static int N_drs = 0;			/* number of specs in Drs_buffer */

static int Node_connectivity_OK = 1;	/* node connectivity is OK */
static time_t Lost_conn_time = 0;	/* start time of the latest lost node
					   connectivity state. 0 - conn OK */
static time_t Restart_set_time = 0;	/* time set for future RPG re-startup.
					   0 - No restart is planned */
static int Restart_required = 0;	/* RPG re-startup is required */
static int Need_publish_node_info = 0;	/* Node info changed and needs to be 
					   published */
static int Lost_conn_detected = 0;	/* set to 1 if node connectivity is 
					   lost */


enum {VECTOR_INT, VECTOR_STRING};	/* for vector_t.type */

typedef struct {
    int type;
    int buf_size;
    int length;
    void *p;
} vector_t;

static int Init_node_names (char *rc_name);
static int Read_resource_config (char *rc_name);
static int Search_resource_config ();
static int Read_ra_sections (Node_attribute_t *node);
static void Append_element (vector_t *vec, void *new);
static int Set_local_node ();
static int Add_replication_spec (char *text);
static int Getenv (char *hname, char *env, char **buf);
static int Get_envs ();
static int Getdir (char *hname, char *which, char **buf);
static int Copy_resource_table (char *fname);
static int Search_for_a_node (Node_attribute_t *node);
static void Wait_for_all_nodes_up ();
static void Connectivity_cb (EN_id_t evtcd, char *msg, int msglen, void *arg);


/******************************************************************

    Initializes this module. "rc_name" is the resource configuration
    name. Returns 0 on success or -1 on failure.
	
******************************************************************/

int MHR_init (char *rc_name, int *missing_node) {
    int cnt, i, ret;

    if (MAIN_command () == MRPG_CLEANUP || MAIN_command () == MRPG_REMOVE_DATA)
	rc_name = NULL;
    ret = Init_node_names (rc_name);
    if (ret < 0)
	return (-1);

    if (MAIN_command () == MRPG_AUTO_START) /* wait until all nodes are up */
	Wait_for_all_nodes_up ();

    /* allocate work space */
    Node_buffer = (Node_attribute_t **)
		malloc (N_nodes * sizeof (Node_attribute_t *));
    if (Node_buffer == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }

    N_total_nodes = N_nodes;
    if (N_nodes == 0 || rc_name == NULL) {	/* no further processing */
	if (Get_envs () < 0)
	    return (-1);
	return (0);
    }

    if (MHR_is_distributed () &&
	Read_resource_config (rc_name) < 0)
	return (-1);

    /* move back not_used nodes */
    for (i = 0; i < N_total_nodes; i++) {
	if (Nodes[i].not_used) {
	    int k;
	    for (k = i + 1; k < N_total_nodes; k++) {
		if (!Nodes[k].not_used)
		    break;
	    }
	    if (k < N_total_nodes) {
		Node_attribute_t tmp;
		memcpy (&tmp, Nodes + i, sizeof (Node_attribute_t));
		memcpy (Nodes + i, Nodes + k, sizeof (Node_attribute_t));
		memcpy (Nodes + k, &tmp, sizeof (Node_attribute_t));
	    }
	    else
		N_nodes--;
	}
    }

    /* verify and set default nodes */
    if (MHR_is_mrpg_node () < 0)
	return (-1);

    cnt = 0;
    for (i = 0; i < N_nodes; i++) {
	if (!Nodes[i].is_connected) {
	    LE_send_msg (GL_ERROR, "Active node %s not found", Nodes[i].node);
	    return (-1);
	}
	else if (Nodes[i].default_node)
	    cnt++;
    }
    if (cnt >= 2) {
	LE_send_msg (GL_ERROR, "Too many default nodes\n");
	return (-1);
    }
    if (cnt == 0)
	Nodes[0].default_node = 1;

    if (MHR_is_distributed ()) {
	char buf[128], *p;
	p = buf;
	for (i = 0; i < N_nodes; i++) {
	    if ((p - buf) + strlen (Nodes[i].node) + 32 > 128) {
		sprintf (p, "...");
		break;
	    }
	    if (Nodes[i].default_node)
		sprintf (p, "%s (%s, default node)", 
				Nodes[i].node, Nodes[i].hname);
	    else
		sprintf (p, "%s (%s)", Nodes[i].node, Nodes[i].hname);
	    p += strlen (p);
	    if (i < N_nodes - 2)
		sprintf (p, ", ");
	    else if (i < N_nodes - 1)
		sprintf (p, " and ");
	    p += strlen (p);
	}
	LE_send_msg (GL_INFO, "Runs on nodes: %s", buf);
    }

    if (Get_envs () < 0)
	return (-1);

    if (missing_node != NULL) {
	*missing_node = 0;
	if ((rc_name == NULL || strlen (rc_name) == 0) && 
						N_nodes < N_total_nodes)
	    *missing_node = 1;
    }

    return (0);
}

/******************************************************************

    EN_LOST_EN_CONN callback.
	
******************************************************************/

static void Connectivity_cb (EN_id_t evtcd, char *msg, int msglen, void *arg) {
    char tok[256];
    int n_hosts, err;
    static char *cr_msg = NULL;
    int i, ln_cnt, changed;
    time_t prev_lost_conn, cr_t;

    if (N_nodes <= 1)
	return;
    if (msglen <= 0) {
	LE_send_msg (GL_ERROR, "Unexpected EN_QUERY_HOSTS evt - no message");
	return;
    }
    err = 0;
    if (MISC_get_token (msg, "", 0, tok, 256) > 0 &&
	strcmp (tok, "Remote_hosts:") == 0 &&
	MISC_get_token (msg, "Ci", 1, &n_hosts, 0) > 0 &&
	n_hosts >= 0) {
	for (i = 0; i < n_hosts; i++) {
	    int c, k;
	    unsigned int ip;

	    if (MISC_get_token (msg, "", 2 + i * 2, tok, 256) <= 0 ||
		MISC_get_token (msg, "Ci", 3 + i * 2, &c, 0) <= 0) {
		err = 1;
		break;
	    }
	    ip = NET_get_ip_by_name (tok);
	    for (k = 0; k < N_nodes; k++) {
		if (Nodes[k].ip == ip) {
		    if (c == 1)
			Nodes[k].is_connected = 1;
		    else
			Nodes[k].is_connected = 0;
		    break;
		}
	    }
	}
    }
    else
	err = 1;
    if (err) {
	if (msglen >= 150)
	    msg[150] = '\0';
	LE_send_msg (GL_ERROR, "Unexpected EN_QUERY_HOSTS evt (%s)", msg);
	return;
    }

    /* report lost conn nodes */
    ln_cnt = changed = 0;	/* lost node count */
    cr_msg = STR_copy (cr_msg, "");
    for (i = 0; i < N_nodes; i++) {
	Node_attribute_t *node = Nodes + i;
	if (!node->is_connected) {
	    ln_cnt++;
	    cr_msg = STR_cat (cr_msg, " ");
	    cr_msg = STR_cat (cr_msg, Nodes[i].node);
	}
	if (node->is_connected != node->prev_connected)
	    changed = 1;
	node->prev_connected = node->is_connected;
    }
    if (changed) {
	Need_publish_node_info = 1;
	if (ln_cnt > 0)
	    LE_send_msg (GL_STATUS, "Lost Connectivity Node(s):%s", cr_msg);
    }
    prev_lost_conn = Lost_conn_time;
    if (ln_cnt > 0)
	Lost_conn_detected = 1;
    if (ln_cnt == 0)
	Lost_conn_time = 0;
    else if (Lost_conn_time == 0)
	Lost_conn_time = MISC_systime (NULL);
    
    if (Lost_conn_time > 0 && prev_lost_conn == 0) {  /* report lost conn */
	LE_send_msg (GL_STATUS, "RPG Node Connectivity Lost");
	Node_connectivity_OK = 0;
    }
    if (Lost_conn_time == 0 && prev_lost_conn > 0) {  /* report resume conn */
	LE_send_msg (GL_STATUS, "RPG Node Connectivity Has Resumed");
	if (!Restart_required)
	    Node_connectivity_OK = 1;
    }

    if (Lost_conn_time > 0 && !Restart_required) {
	Restart_required = 1;
	LE_send_msg (GL_STATUS, "RPG Recovery Is Scheduled Due To Node Connectivity Lost");
    }

    if (Lost_conn_time > 0)
	Restart_set_time = 0;
    else if (prev_lost_conn > 0 && Restart_required && 
		(cr_t = MISC_systime (NULL)) > Restart_set_time)
	Restart_set_time = cr_t;
}

/******************************************************************

    Checks if all nodes are up. If not, it waits up to 
    AUTO_START_WAIT_TIME seconds until all nodes are up.
	
******************************************************************/

static void Wait_for_all_nodes_up () {
    int i;
    time_t st;

    st = 0;
    while (1) {
	int done = 1;
	for (i = 0; i < N_nodes; i++) {
	    Node_attribute_t *node = Nodes + i;
	    if (node->ip == 0)
		Search_for_a_node (node);
	    if (node->ip == 0)
		done = 0;
	}
	if (done)
	    break;
	if (st == 0) {
	    st = MISC_systime (NULL);
	    LE_send_msg (GL_INFO, "Wait for other nodes to start up\n");
	}
	else if (MISC_systime (NULL) >= st + AUTO_START_WAIT_TIME) {
	    LE_send_msg (GL_INFO, "Waiting timedout - We continue\n");
	    break;
	}
	msleep (5000);
    }
}

/******************************************************************

    Returns non-zero if the system is distributed (i.e. A resource
    table is found) or zero if not.
	
******************************************************************/

int MHR_is_distributed () {
    if (Nodes[0].node[0] != '\0')
	return (1);
    return (0);
}

/******************************************************************

    Returns the list of all used nodes with "nodes". Returns the 
    number of used nodes.
	
******************************************************************/

int MHR_all_hosts (Node_attribute_t **nodes) {
    if (nodes != NULL)
	*nodes = Nodes;
    return (N_nodes);
}

/******************************************************************

    Returns the total list of all nodes with "nodes". Returns the 
    total number of nodes.
	
******************************************************************/

int MHR_all_nodes (Node_attribute_t **nodes) {
    if (nodes != NULL)
	*nodes = Nodes;
    return (N_total_nodes);
}

/******************************************************************

    Returns the node structure for node named "node_name". If the
    name is not found, NULL is returned.
	
******************************************************************/

Node_attribute_t *MHR_get_node_by_name (char *node_name) {
    int i;
    for (i = 0; i < N_nodes; i++) {
	if (strcmp (Nodes[i].node, node_name) == 0)
	    return (Nodes + i);
    }
    return (NULL);
}

/******************************************************************

    Returns non-zero if the host name "host_name" is the local node
    or zero if not.
	
******************************************************************/

int MHR_is_local_node (char *host_name) {
    int i;
    if (host_name == NULL || host_name[0] == '\0')
	return (1);
    for (i = 0; i < N_nodes; i++) {
	if (Nodes[i].is_local &&
	    strcmp (Nodes[i].hname, host_name) == 0)
	    return (1);
    }
    return (0);
}

/******************************************************************

    Returns the channel number.
	
******************************************************************/

int MHR_get_channel_number () {
    return (Channel);
}

/******************************************************************

    Returns non-zero if the local node is the mrpg node or zero if 
    not. It checks if mrpg is assigned to multiple nodes. If it is
    it returns -1.
	
******************************************************************/

int MHR_is_mrpg_node () {
    int n, i;
    Node_attribute_t **nodes;

    n = MHR_get_task_hosts ("mrpg", &nodes);
    if (n > 1) {
	LE_send_msg (GL_ERROR, "mrpg is assigned to multiple nodes\n");
	return (-1);
    }
    for (i = 0; i < n; i++) {
	if (nodes[i]->is_local)
	    return (1);
    }
    return (0);
}

/******************************************************************

    Returns mrpg host name in "buf" of size "buf_size". Returns 0
    on success or -1 on failure.
	
******************************************************************/

int MHR_get_mrpg_node_name (char *buf, int buf_size) {
    int n;
    Node_attribute_t **nodes;

    n = MHR_get_task_hosts ("mrpg", &nodes);
    if (n > 1) {
	LE_send_msg (GL_ERROR, "mrpg is assigned to multiple nodes\n");
	return (-1);
    }
    strncpy (buf, nodes[0]->hname, buf_size);
    buf[buf_size - 1] = '\0';
    return (0);
}

/******************************************************************

    Returns the current resource name in "buf" of size "buf_size".
	
******************************************************************/

void MHR_get_resource_name (char *buf, int buf_size) {
    if (Res_name == NULL)
	buf[0] = '\0';
    else {
	strncpy (buf, Res_name, buf_size);
	buf[buf_size - 1] = '\0';
    }
    return;
}

/******************************************************************

    Returns local host name in "buf" of size "buf_size". Returns 0
    on success or -1 on failure.
	
******************************************************************/

int MHR_get_local_node_name (char *buf, int buf_size) {
    int i;
    for (i = 0; i < N_nodes; i++) {
	if (Nodes[i].is_local)
	    break;
    }
    if (i >= N_nodes) {
	LE_send_msg (GL_ERROR, "Local node not found\n");
	return (-1);
    }
    strncpy (buf, Nodes[i].hname, buf_size);
    buf[buf_size - 1] = '\0';
    return (0);
}

/******************************************************************

    Returns the data replication specifications with "spec". The
    function returns the number of specifications.
	
******************************************************************/

int MHR_data_rep_spec (char **spec) {
    *spec = Drs_buffer;
    return (N_drs);
}

/******************************************************************

    Returns nodes, with "node", that host data store or product
    "data". Returns the number of nodes found. If a data/product
    is not explicitly specified on any node, it is assumed to 
    be on the default node.
	
******************************************************************/

int MHR_get_data_hosts (int data, Node_attribute_t ***node) {
    int cnt, i, k, found;
    Node_attribute_t *n;

    cnt = 0; 
    for (i = 0; i < N_nodes; i++) {
	n = Nodes + i;
	found = 0;
	for (k = 0; k < n->n_data_ids; k++) {
	    if (n->data_ids[k] == data) {
		found = 1;
		break;
	    }
	}
	if (!found) {
	    for (k = 0; k < n->n_prod_ids; k++) {
		if (n->prod_ids[k] == data) {
		    found = 1;
		    break;
		}
	    }
	}
	if (found) {
	    Node_buffer[cnt] = n;
	    cnt++;
	}
    }
    if (cnt == 0) {		/* not specified - on default host */
	for (i = 0; i < N_nodes; i++) {
	    if (Nodes[i].default_node) {
		Node_buffer[cnt] = Nodes + i;
		cnt++;
		break;
	    }
	}
    }
    *node = Node_buffer;
    return (cnt);
}

/******************************************************************

    Returns nodes, with "node", that host task "task". Returns the 
    number of nodes found. If a task is not explicitly specified 
    on any node, it is assumed to be on the default node.
	
******************************************************************/

int MHR_get_task_hosts (char *task, Node_attribute_t ***node) {
    int cnt, i, k;
    Node_attribute_t *n;

    cnt = 0; 
    for (i = 0; i < N_nodes; i++) {
	n = Nodes + i;
	for (k = 0; k < n->n_tasks; k++) {
	    if (strcmp (n->tasks[k], task) == 0) {
		Node_buffer[cnt] = n;
		cnt++;
		break;
	    }
	}
    }
    if (cnt == 0) {		/* not specified - on default host */
	for (i = 0; i < N_nodes; i++) {
	    if (Nodes[i].default_node) {
		Node_buffer[cnt] = Nodes + i;
		cnt++;
		break;
	    }
	}
    }
    *node = Node_buffer;
    return (cnt);
}

/******************************************************************

    Publishes node info. Returns 0 on success or -1 on failure.
	
******************************************************************/

int MHR_publish_node_info () {
    Node_attribute_t *n;
    int size, off, i, ret, k;
    char *buf, *cp;
    Mrpg_node_t *ni;

    size = 0;
    for (i = 0; i < N_nodes; i++) {	/* estimate the buffer size needed */
	n = Nodes + i;
	size += strlen (n->node) + strlen (n->hname) + strlen (n->orpgdir) + 
			strlen (n->workdir) + strlen (n->logdir) + 8;
	for (k = 0; k < n->n_tasks; k++)
	    size += strlen (n->tasks[k]) + 1;
	size += sizeof (Mrpg_node_t) + ALIGNED_LENGTH;
    }

    buf = malloc (size);
    if (buf == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }

    size = 0;
    for (i = 0; i < N_nodes; i++) {	/* create the message */
	off = 0;
	cp = buf + size;
	n = Nodes + i;
	ni = (Mrpg_node_t *)cp;
	ni->ip = n->ip;
	ni->is_local = n->is_local;
	ni->default_node = n->default_node;
	ni->is_bigendian = n->is_bigendian;
	ni->is_connected = n->is_connected;
	off += sizeof (Mrpg_node_t);

	ni->host_off = off;
	strcpy (cp + off, n->hname);
	off += strlen (cp + off) + 1;
	ni->node_off = off;
	strcpy (cp + off, n->node);
	off += strlen (cp + off) + 1;
	ni->orpgdir_off = off;
	strcpy (cp + off, n->orpgdir);
	off += strlen (cp + off) + 1;
	ni->workdir_off = off;
	strcpy (cp + off, n->workdir);
	off += strlen (cp + off) + 1;
	ni->logdir_off = off;
	strcpy (cp + off, n->logdir);
	off += strlen (cp + off) + 1;
	if (n->n_tasks == 0)
	    ni->tasks_off = 0;
	else {
	    ni->tasks_off = off;
	    for (k = 0; k < n->n_tasks; k++) {
		sprintf (cp + off, "%s ", n->tasks[k]);
		off += strlen (cp + off);
	    }
	    cp[off - 1] = '\0';
	}
	off = ALIGNED_SIZE (off);
	ni->size = off;
	size += off;
    }

    ret = ORPGDA_write (ORPGDAT_TASK_STATUS, buf, size, MRPG_RPG_NODE_MSGID);
    if (ret != size) {
	LE_send_msg (GL_ERROR, 
		"ORPGDA_write MRPG_RPG_NODE_MSGID failed (%d)\n", ret);
	free (buf);
	return (-1);
    }
    free (buf);
    return (0);
}

/******************************************************************

    Initializes node names by reading "resource_table". Returns 0 
    on success or -1 on failure.
	
******************************************************************/

static int Init_node_names (char *rc_name) {
    int len, ret, local_in_cfg, i;
    char dir[MRPG_NAME_SIZE], tmp[MRPG_NAME_SIZE];
    char *line, *ch;

    ch = ORPGMISC_get_site_name ("channel_num");
    if (ch == NULL) {
	LE_send_msg (GL_ERROR, 
		"ORPGMISC_get_site_name failed - channel number not found");
	return (-1);
    }
    if (strcmp (ch, "2") == 0)
	Channel = 2;
    else 
	Channel = 1;

    Node_tbl = MISC_open_table (sizeof (Node_attribute_t), 16, 
			    1, &N_nodes, (char **)&Nodes);
    if (Node_tbl == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }
    if ((rc_name == NULL || rc_name[0] == '\0') && !(MAIN_is_operational ()))
	return (Set_local_node ());

    len = MISC_get_cfg_dir (dir, MRPG_NAME_SIZE);
    if (len <= 0) {
	LE_send_msg (GL_ERROR, "ORPG cfg dir not found\n");
	return (-1);
    }
    if (len + strlen (Ra_config_name) + 3 > MRPG_NAME_SIZE) {
	LE_send_msg (GL_ERROR, "ORPG cfg dir too long\n");
	return (-1);
    }
    strcpy (tmp, dir);
    strcat (tmp, "/");
    strcat (tmp, Ra_config_name);
    strcpy (Ra_config_name, tmp);
    if (MAIN_is_operational () && Copy_resource_table (Ra_config_name) < 0)
	return (-1);

    LE_send_msg (LE_VL1, "Reading resource table");
    LE_send_msg (LE_VL2, "    Reading resource table file %s", Ra_config_name);

    CS_cfg_name (Ra_config_name);
    CS_control (CS_COMMENT | '#');

    MAIN_disable_report (1);
    ret = CS_entry ("node_names", 0, MRPG_NAME_SIZE, tmp);
    MAIN_disable_report (0);
    if (ret == CS_OPEN_ERR) {
	LE_send_msg (LE_VL2, 
		"resource_table not found - single node configuration");
	CS_cfg_name ("");
	return (Set_local_node ());
    }
    else if (ret == CS_KEY_NOT_FOUND) {
	LE_send_msg (GL_ERROR, "key node_names not found");
	return (-1);
    }
    else if (ret <= 0 ||
	     CS_level (CS_DOWN_LEVEL) < 0) {
	LE_send_msg (GL_ERROR, "error found in resource_table");
	return (-1);
    }

    local_in_cfg = 0;
    line = CS_THIS_LINE;
    while (1) {
	char node_name[MRPG_NAME_SIZE];
	Node_attribute_t *node;

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry (line, 0, MRPG_NAME_SIZE, node_name) <= 0)
	    break;

	for (i = 0; i < N_nodes; i++) {
	    if (strcmp (Nodes[i].node, node_name) == 0) {
		CS_report ("duplicated node name found");
		return (-1);
	    }
	}

	CS_control (CS_KEY_REQUIRED);

	if ((node = MISC_table_new_entry (Node_tbl, NULL)) == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed");
	    return (-1);
	}
	node->hname = NULL;
	node->node = MISC_malloc (strlen (node_name) + 1);
	strcpy (node->node, node_name);
	node->n_rass = 0;		/* read later */
	node->rass = NULL;
	node->default_node = 0;
	node->not_used = 1;
	node->index = N_nodes - 1;

	line = CS_NEXT_LINE;
    }
    CS_level (CS_UP_LEVEL);
    CS_cfg_name ("");

    for (i = 0; i < N_nodes; i++) {
	int k;
	ret = Search_for_a_node (Nodes + i);
	if (ret < 0)
	    return (-1);
	if (Nodes[i].is_local)
	    local_in_cfg = 1;
	for (k = 0; k < i; k++) {
	    if (Nodes[k].ip == Nodes[i].ip) {
		LE_send_msg (GL_ERROR, 
			"Two nodes (%s %s) have the same IP address", 
					Nodes[k].node, Nodes[i].node);
		return (-1);
	    }
	}
    }

    N_total_nodes = N_nodes;

    if (!local_in_cfg) {
	LE_send_msg (GL_INFO, "The local node is not in resource config");
	return (-1);
    }

    return (0);
}

/******************************************************************

    Search in the network of channel "Channel" for node named
    "node->node" (node name). If the node is found, the following fields
    in "node" is set: hname, ip, is_connected and is_local. If the node
    is not found, These values are set to "\0", 0, 0, 0. Note that ip ==
    0 will be used for checking if the node has ever been found. In case
    of fatal error, the function returns -1. Otherwise 0 is returned.
    node->hname must be initialized to NULL when node is allocated.
	
******************************************************************/

static int Search_for_a_node (Node_attribute_t *node) {
    char ipb[MRPG_NAME_SIZE], hname[MRPG_NAME_SIZE];
    int is_connected, is_local, ret;
    unsigned int ip;

    is_connected = 1;
    is_local = 0;
    LE_set_option ("LE disable", 1);
    ret = ORPGMGR_discover_host_ip (node->node, Channel, 
					    ipb, MRPG_NAME_SIZE);
    LE_set_option ("LE disable", 0);
    if (ret == 0) {
	LE_send_msg (GL_INFO, "RPG node %s, channel %d, not found", 
					node->node, Channel);
	is_connected = 0;
	ip = 0;
	hname[0] = '\0';
    }
    else if (ret < 0) {
	LE_send_msg (GL_ERROR, 
	    "ORPGMGR_discover_host_ip (node %s, channel %d) failed (%d)", 
					    node->node, Channel, ret);
	return (-1);
    }
    else {
	if (ipb[0] == '\0')
	    is_local = 1;
	ip = NET_get_ip_by_name (ipb);
	if (ip == INADDR_NONE) {
	    LE_send_msg (GL_ERROR, "IP address of host %s not found", ipb);
	    return (-1);
	}
	NET_get_name_by_ip (ip, hname, MRPG_NAME_SIZE);
    }
    if (node->hname != NULL)
	free (node->hname);
    node->hname = MISC_malloc (strlen (hname) + 1);
    strcpy (node->hname, hname);
    node->ip = ip;
    node->is_connected = is_connected;
    node->prev_connected = is_connected;
    node->is_local = is_local;
    return (0);
}

static int N_copy_ips = 0;
static char *Copy_ips = NULL;

/******************************************************************

    The callback function passed to ORPGMGR_each_node for finding 
    all rpg nodes for channel Channel. It saves the ips of found
    rpg nodes in Copy_ips of size N_copy_ips.
	
******************************************************************/

static int Each_node_cb (char *ip, char *hname, char *node_config) {
    int ind, chan_yes, type_yes;
    char tok[128];

    if (strcmp (ip, "local") == 0)
	return (0);
    ind = 0;
    type_yes = 0;
    chan_yes = 0;
    N_copy_ips = 0;
    Copy_ips = STR_reset (Copy_ips, 128);
    while (MISC_get_token (node_config, "", ind, tok, 128) > 0) {
	int ch;
	if (strcmp (tok, "TYPE:") == 0 &&
	    MISC_get_token (node_config, "", ind + 1, tok, 128) > 0 &&
	    strncmp (tok, "rpg", 3) == 0)
	    type_yes = 1;
	if (strcmp (tok, "CHAN_NUM:") == 0 &&
	    MISC_get_token (node_config, "", ind + 1, tok, 128) > 0 &&
	    sscanf (tok, "%d", &ch) == 1 &&
	    ch == Channel)
	    chan_yes = 1;
	ind++;
    }
    if (type_yes && chan_yes) {
	Copy_ips = STR_append (Copy_ips, ip, strlen (ip) + 1);
	N_copy_ips++;
    }

    return (0);
}

/******************************************************************

    Copy the resource table from another rpg node in this RPG channel.
    Assume that the dir struct is the same on all rpg nodes. Returns
    0 on success or a negative error code on failure.
	
******************************************************************/

static int Copy_resource_table (char *fname) {
    struct stat buf;
    int ret, i;
    char *p, dir[256], *src;

    if (stat (fname, &buf) == 0)	/* local resource table exists */
	return (0);

    /* try to copy over a resource file */
    if ((ret = ORPGMGR_each_node (Each_node_cb)) < 0) {
	LE_send_msg (GL_ERROR, 
		"Copy_resource_table failed (ORPGMGR_each_node ret %d)", ret);
        return (ret);
    }

    if ((ret = MISC_mkdir (MISC_dirname (fname, dir, 256))) < 0) {
	LE_send_msg (GL_ERROR, "MISC_dirname %s failed (%d)", fname, ret);
	return (ret);
    }
    p = Copy_ips;
    src = NULL;
    for (i = 0; i < N_copy_ips; i++) {
	src = STR_gen (src, p, ":", fname, NULL);
	if (RSS_copy (src, fname) < 0)
	    LE_send_msg (LE_VL2, "Copying %s to %s failed", src, fname);
	else {
	    LE_send_msg (GL_INFO, "Resource table copied from %s", p);
	    STR_free (src);
	    return (0);
	}
	p += strlen (p) + 1;
    }
    STR_free (src);
    LE_send_msg (GL_ERROR, "Resource table not found on any of the RPG nodes");

    return (0);
}

/******************************************************************

    Reads the resource configuration named "rc_name" from 
    "resource_table". If "rc_name" is NULL, we search for the
    first resource configuration based on the node connectivity.
    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_resource_config (char *rc_name) {
    char tmp[MRPG_NAME_SIZE], node_name[MRPG_NAME_SIZE];
    vector_t ras;
    char *line;
    int i;

    CS_cfg_name (Ra_config_name);
    CS_control (CS_COMMENT | '#');

    if (rc_name == NULL || strlen (rc_name) == 0) {
	if (Search_resource_config () < 0) {
	    LE_send_msg (GL_ERROR, "no appropriate resource config found");
	    return (-1);
	}
    }
    else {
	Res_name = STR_copy (Res_name, rc_name);
	LE_send_msg (LE_VL2, "    Using resource config %s", rc_name);
	if (CS_entry (rc_name, 0, MRPG_NAME_SIZE, tmp) <= 0) {
	    LE_send_msg (GL_ERROR, "Resource config %s not found", rc_name);
	    return (-1);
	}
	if (CS_level (CS_DOWN_LEVEL) < 0) {
	    LE_send_msg (GL_INFO, "Empty resource config %s", rc_name);
	    return (0);
	}
    }

    ras.type = VECTOR_STRING;
    ras.length = ras.buf_size = 0;
    line = CS_THIS_LINE;
    while (1) {
	Node_attribute_t *node;
	int ind;

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry (line, 0, MRPG_NAME_SIZE, node_name) <= 0)
	    break;

	if (strcmp (node_name, "replicate") == 0 ||
	    strcmp (node_name, "multicast") == 0) {
	    char buf[256];
	    CS_entry (CS_THIS_LINE, CS_FULL_LINE, 256, buf);
	    if (Add_replication_spec (buf) < 0)
		return (-1);
	    line = CS_NEXT_LINE;
	    continue;
	}

	for (i = 0; i < N_nodes; i++) {
	    if (strcmp (node_name, Nodes[i].node) == 0)
		break;
	}
	if (i >= N_nodes) {
	    LE_send_msg (GL_ERROR, "node name %s not defined", node_name);
	    return (-1);
	}
	node = Nodes + i;
	node->not_used = 0;

	ras.length = ras.buf_size = node->n_rass;
	ras.p = node->rass;
	ind = 1;
	while (1) {
	    if (CS_entry (CS_THIS_LINE, ind, MRPG_NAME_SIZE, tmp) <= 0)
		break;
	    if (strcmp (tmp, "ras_default") == 0)
		node->default_node = 1;
	    else 
		Append_element (&ras, tmp);
	    ind++;
	}
	node->n_rass = ras.length;
	node->rass = ras.p;

	line = CS_NEXT_LINE;
    }
    CS_level (CS_TOP_LEVEL);

    for (i = 0; i < N_nodes; i++) {
	if (Read_ra_sections (Nodes + i) < 0)
	    return (-1);
    }

    CS_cfg_name ("");
    return (0);
}

/******************************************************************

    Searches for the first resource configuration with which all
    nodes are accessible. Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Search_resource_config () {
    char *line, *l;

    line = CS_THIS_LINE;
    while (1) {
	int cnt, i;
	char conf_name[MRPG_NAME_SIZE], name[MRPG_NAME_SIZE];

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry (line, 0, MRPG_NAME_SIZE, conf_name) <= 0)
	    break;
	if (strcmp (conf_name, "node_names") == 0 ||
	    strncmp (conf_name, "ras_", 4) == 0 ||
	    CS_level (CS_DOWN_LEVEL) < 0) {
	    line = CS_NEXT_LINE;
	    continue;
	}

	cnt = 0;
	l = CS_THIS_LINE;
	while (1) {
	    if (CS_entry (l, 0, MRPG_NAME_SIZE, name) <= 0) {
		if (cnt > 0) {	/* found the config */
		    CS_level (CS_UP_LEVEL);
		    CS_entry (conf_name, 0, MRPG_NAME_SIZE, name);
		    CS_level (CS_DOWN_LEVEL);
		    LE_send_msg (GL_INFO, 
			"Resource config %s selected", name);
		    Res_name = STR_copy (Res_name, name);
		    return (0);
		}
		break;
	    }
	    for (i = 0; i < N_nodes; i++) {
		if (strcmp (name, Nodes[i].node) == 0)
		    break;
	    }
	    if (i >= N_nodes) {
		if (strcmp (name, "replicate") != 0 &&
		    strcmp (name, "multicast") != 0)
		break;
	    }
	    else if (!Nodes[i].is_connected)
		break;
	    else
		cnt++;
	    l = CS_NEXT_LINE;
	}
	CS_level (CS_UP_LEVEL);
	line = CS_NEXT_LINE;
    }
    return (-1);
}

/******************************************************************

    Reads all resource allocation sections for node "node". Returns
    0 on success or -1 on failure.
	
******************************************************************/

static int Read_ra_sections (Node_attribute_t *node) {
    int ret, i;
    vector_t data, product, task, process;

    data.type = product.type = VECTOR_INT;
    task.type = process.type = VECTOR_STRING;
    data.length = product.length = task.length = process.length = 0;
    data.buf_size = product.buf_size = 
				task.buf_size = process.buf_size = 0;
    ret = 0;
    for (i = 0; i < node->n_rass; i++) {
	char tmp[MRPG_NAME_SIZE], *line;

	if (CS_entry (node->rass[i], 0, MRPG_NAME_SIZE, tmp) <= 0) {
	    LE_send_msg (GL_ERROR, 
		"resource allocation section %s not found in %s", 
					node->rass[i], Ra_config_name);
	    ret = -1;
	    break;
	}
	if (CS_level (CS_DOWN_LEVEL) < 0)
	    continue;

	line = CS_THIS_LINE;
	while (1) {
	    char name[MRPG_NAME_SIZE];
	    int ind, id;

	    CS_control (CS_KEY_OPTIONAL);
	    if (CS_entry (line, 0, MRPG_NAME_SIZE, name) <= 0)
		break;

	    if (strcmp (name, "data") == 0) {
		ind = 1;
		while (1) {
		    if (CS_entry (CS_THIS_LINE, ind, 
					MRPG_NAME_SIZE, tmp) <= 0 ||
			tmp[0] == '#')
			break;
		    if (CS_entry (CS_THIS_LINE, ind | CS_INT, 
						0, (char *)&id) <= 0)
			break;
		    Append_element (&data, &id);
		    ind++;
		}
	    }
	    else if (strcmp (name, "product") == 0) {
		ind = 1;
		while (1) {
		    if (CS_entry (CS_THIS_LINE, ind, 
					MRPG_NAME_SIZE, tmp) <= 0 ||
			tmp[0] == '#')
			break;
		    if (CS_entry (CS_THIS_LINE, ind | CS_INT, 
						0, (char *)&id) <= 0)
			break;
		    Append_element (&product, &id);
		    ind++;
		}
	    }
	    else if (strcmp (name, "task") == 0) {
		ind = 1;
		while (1) {
		    if (CS_entry (CS_THIS_LINE, ind, MRPG_NAME_SIZE, 
					tmp) <= 0 || tmp[0] == '#')
			break;
		    Append_element (&task, &tmp);
		    ind++;
		}
	    }
	    else if (strcmp (name, "process") == 0) {
		ind = 1;
		while (1) {
		    if (CS_entry (CS_THIS_LINE, ind, MRPG_NAME_SIZE, 
					tmp) <= 0 || tmp[0] == '#')
			break;
		    if (ind >= 2) {
			CS_report ("more than one item on process line");
			ret = -1;
			break;
		    }
		    Append_element (&process, &tmp);
		    ind++;
		}
	    }
	    else {
		CS_report ("unknown key word");
		ret = -1;
		break;
	    }
	    if (ret < 0)
		break;
	    line = CS_NEXT_LINE;
	}
	if (ret < 0)
	    break;
	CS_level (CS_UP_LEVEL);
    }
    node->n_data_ids = data.length;
    node->data_ids = (int *)data.p;
    node->n_prod_ids = product.length;
    node->prod_ids = (int *)product.p;
    node->n_tasks = task.length;
    node->tasks = (char **)task.p;
    node->n_processes = process.length;
    node->processes = (char **)process.p;

    return (ret);
}

/******************************************************************

    Appends a new element "new" to the vector "vec" if the new 
    element is not already in the vector.
	
******************************************************************/

static void Append_element (vector_t *vec, void *new) {
    int new_int, size, k;
    char *new_str, *p;

    new_int = 0;	/* this and the following line are useless - */
    new_str = "";	/* assigned for avoiding compiler warning */
    if (vec->type == VECTOR_INT) {
	new_int = *((int *)new);
	size = sizeof (int);
    }
    else if (vec->type == VECTOR_STRING) {
	new_str = (char *)new;
	size = sizeof (char *);
    }
    else
	return;

    for (k = 0; k < vec->length; k++) {
	if (vec->type == VECTOR_INT) {
	    if (new_int == ((int *)(vec->p))[k])
		break;
	}
	else if (vec->type == VECTOR_STRING) {
	    if (strcmp (new_str, ((char **)(vec->p))[k]) == 0)
		break;
	}
    }
    if (k >= vec->length) {
	if (vec->length >= vec->buf_size) {
	    p = MISC_malloc ((vec->length + 64) * size);
	    memcpy (p, (char *)vec->p, vec->length * size);
	    if (vec->buf_size > 0)
		free (vec->p);
	    vec->p = p;
	    vec->buf_size = vec->length + 64;
	}
	if (vec->type == VECTOR_INT) {
	    ((int *)(vec->p))[k] = new_int;
	}
	else if (vec->type == VECTOR_STRING) {
	    p = MISC_malloc (strlen (new_str) + 1);
	    strcpy (p, new_str);
	    ((char **)(vec->p))[k] = p;
	}
	vec->length++;
    }
}

/******************************************************************

    Sets the local node in case the resource configuration is not
    found (non-distributed case). Returns 0 on success or -1 on
    failure.
	
******************************************************************/

static int Set_local_node () {
    Node_attribute_t *node;
    char buf[256];

    if ((node = MISC_table_new_entry (Node_tbl, NULL)) == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed");
	return (-1);
    }

    node->node = "";
    node->ip = NET_get_ip_by_name (NULL);
    if (NET_get_name_by_ip (node->ip, buf, 256)) {
	node->hname = malloc (strlen (buf) + 1);
	if (node->hname == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed");
	    return (-1);
	}
	strcpy (node->hname, buf);
    }
    else
	node->hname = "";
    node->n_rass = 0;
    node->rass = NULL;
    node->is_local = 1;
    node->default_node = 1;
    node->is_connected = 1;
    node->prev_connected = 1;
    node->n_data_ids = 0;
    node->n_prod_ids = 0;
    node->n_tasks = 0;
    node->n_processes = 0;
    node->not_used = 0;
    node->index = N_nodes - 1;

    return (0);
}

/******************************************************************

    Appends a new data replication specification "text" to the buffer
    "Drs_buffer". Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Add_replication_spec (char *text) {
    static int buf_size = 0, n_bytes = 0;
    int len;

    len = strlen (text) + 1;
    if (len + n_bytes > buf_size) {
	char *cpt;
	cpt = malloc (buf_size + len + 1024);
	if (cpt == NULL)
	    return (-1);
	if (Drs_buffer != NULL) {
	    memcpy (cpt, Drs_buffer, n_bytes);
	    free (Drs_buffer);
	}
	buf_size += len + 1024;
	Drs_buffer = cpt;
    }
    memcpy (Drs_buffer + n_bytes, text, len);
    n_bytes += len;
    N_drs++;
    return (0);
}

/******************************************************************

    Retrieves the environmental variables, project directories and
    node endianness for each node. Returns 0 on success or -1 on 
    failure. We assume the dirs are the same for all nodes for the
    moment. If we must get them from remote nodes, we must save entire
    node info so "mrpg" can restart without risk of not be able to
    get the info remotely. We may do that in the future.
	
******************************************************************/

static int Get_envs () {
    int i;

    for (i = 0; i < N_nodes; i++) {
	Node_attribute_t *node;

	node = Nodes + i;
	if (Getenv (node->hname, "ORPGDIR", &(node->orpgdir)) < 0 ||
	    Getdir (node->hname, "MISC_get_work_dir", &(node->workdir)) < 0 ||
	    Getdir (node->hname, "LE_dirpath", &(node->logdir)) < 0)
	    return (-1);

	if (strlen (node->orpgdir) > MRPG_NAME_SIZE ||
	    strlen (node->workdir) > MRPG_NAME_SIZE ||
	    strlen (node->logdir) > MRPG_NAME_SIZE) {
	    LE_send_msg (GL_INFO, 
		"one of the directory names (%s %s %s %s) is too long", 
			node->orpgdir, node->workdir, node->logdir);
	    return (-1);
	}

/*	if (node->is_local) */
	    node->is_bigendian = MISC_i_am_bigendian ();
/*	else {
	    char rpc_buf[MRPG_NAME_SIZE + 64];
	    int t;
	    sprintf (rpc_buf, "%s:MISC_i_am_bigendian", node->hname);
	    if ((ret = RSS_rpc (rpc_buf, "i-r", &t)) < 0) {
		LE_send_msg (GL_INFO, 
		    	"RSS_rpc %s failed (returns %d)", rpc_buf, ret);
		return (-1);
	    }
	    node->is_bigendian = t;
	}
*/
   }

    return (0);
}

/******************************************************************

    Gets environmental var "env" on host "hname" and returns it
    with "buf". Returns 0 on success or -1 on failure. Returns
    the RSS_rpc return code if it is not possible to connect to the
    remote node.
	
******************************************************************/

static int Getenv (char *hname, char *env, char **buf) {
    char *p;

    *buf = NULL;
    if (MHR_is_distributed () && strlen (hname) > 0) {
	int ret;
	char rpc_buf[MRPG_NAME_SIZE + 64];

	sprintf (rpc_buf, "%s:getenv", hname);
	if ((ret = RSS_rpc (rpc_buf, "p-r s-i", &p, env)) < 0)
	    return (ret);
	if (p == NULL) {
	    LE_send_msg (GL_ERROR, "Environmental variable %s undefined on %s", env, hname);
	    return (-1);
	}
	if ((ret = RSS_rpc (rpc_buf, "s-r s-i", &p, env)) < 0) {
	    LE_send_msg (GL_ERROR, "RSS_rpc %s failed (ret %d)", rpc_buf, ret);
	    return (-1);
	}
    }
    else {
	p = getenv (env);
	if (p == NULL) {
	    LE_send_msg (GL_ERROR, "Environmental variable %s undefined", env);
	    return (-1);
	}
    }
    *buf = malloc (strlen (p) + 1);
    if (*buf == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed");
	return (-1);
    }
    strcpy (*buf, p);
    return (0);
}

/******************************************************************

    Gets the project directory "which" (MISC_get_work_dir, ...) on 
    host "hname" and returns it with "buf". If the dir is not
    defined, *buf is set to NULL. Returns 0 on success or -1 on
    failure. Returns the RSS_rpc return code if it is not
    possible to connect to the remote node.
	
******************************************************************/

static int Getdir (char *hname, char *which, char **buf) {
    char tbuf[128];
    int ret;

    *buf = NULL;
    if (MHR_is_distributed () && strlen (hname) > 0) {
	int rpc_ret;
	char rpc_buf[MRPG_NAME_SIZE + 64];

	sprintf (rpc_buf, "%s:%s", hname, which);
	if ((rpc_ret = RSS_rpc (rpc_buf, 
			"i-r ba-128-io i", &ret, tbuf, 128)) < 0) {
	    LE_send_msg (GL_ERROR, 
			"RSS_rpc %s failed (ret %d)", rpc_buf, rpc_ret);
	    return (rpc_ret);
	}
	if (ret <= 0 && ret != MISC_RSRC_DIR_MISSING) {
	    LE_send_msg (GL_ERROR, 
		"function %s on %s failed (returns %d)", which, hname, ret);
	    return (-1);
	}
    }
    else {
	if (strcmp (which, "MISC_get_work_dir") == 0)
	    ret = MISC_get_work_dir (tbuf, 128);
	else if (strcmp (which, "LE_dirpath") == 0)
	    ret = LE_dirpath (tbuf, 128);
	else {
	    LE_send_msg (GL_ERROR, "unexpected function %s", which);
	    return (-1);
	}
	if (ret <= 0) {
	    LE_send_msg (GL_ERROR, 
			"function %s failed (returns %d)", which, ret);
	    return (-1);
	}
    }
    *buf = malloc (strlen (tbuf) + 1);
    if (*buf == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed");
	return (-1);
    }
    strcpy (*buf, tbuf);
    return (0);
}

/**************************************************************************

    Returns Node_connectivity_OK.

 **************************************************************************/

int MHR_id_node_connectivity_OK () {
    return (Node_connectivity_OK);
}

/**************************************************************************

    Register lost en conn event.

 **************************************************************************/

int MHR_register_lost_en_conn_event () {
    if (N_nodes >= 2) {
	int ret = EN_register (EN_QUERY_HOSTS, Connectivity_cb);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"EN_register EN_QUERY_HOSTS failed (%d)\n", ret);
	    return (-1);
	}
    }
    return (0);
}

/**************************************************************************

    Checks the node connectivity status. This function must be called 
    frequently. It reports connectivity status and startup RPG when conn
    resumes.

 **************************************************************************/

void MHR_check_node_connectivity () {
    static int node_con_alarm_set = 0;
    time_t cr_t;
    unsigned char alarm_flag;

    if (Need_publish_node_info) {
	Need_publish_node_info = 0;
	MHR_publish_node_info ();
    }

    cr_t = MISC_systime (NULL);
    if (Lost_conn_time > 0 && !node_con_alarm_set &&
			cr_t >= Lost_conn_time + LOST_NODE_CONN_ALARM_TIME) {
	if (ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_NODE_CON,
			ORPGINFO_STATEFL_SET, &alarm_flag) < 0)
	    LE_send_msg (GL_ERROR, "Failed in setting NODE_CON alarm");
	else
	    node_con_alarm_set = 1;
    }

    /* startup RPG after conn resumed */
    if (Restart_set_time > 0 && cr_t >= Restart_set_time + RESTARTUP_TIME) {
	int ret;
	char buf[128];

	LE_send_msg (GL_INFO, "RPG re-startup - conn resumed...");
	MGC_system ("", "prm -9 -quiet nds", NULL);
		/* we must kill local nds so rpg will not be started by it */
	LE_send_msg (GL_INFO, "        local nds killed");
	Lost_conn_detected = 0;
	if (ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_NODE_CON,
			ORPGINFO_STATEFL_CLR, &alarm_flag) < 0)
	    LE_send_msg (GL_ERROR, "Failed in clearing NODE_CON alarm");
	sprintf (buf, "Restart RPG because connection to a disconnected node resumed");
	EN_post (ORPGEVT_HCI_COMMAND_ISSUED, buf, strlen (buf) + 1, 0);
	LE_send_msg (GL_INFO, "        starting RPG (auto_start) ...");
	ret = MISC_system_to_buffer ("mrpg -b auto_start", NULL, 0, NULL);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "Restarting mrpg failed");
	    exit (1);
	}
	exit (0);
    }
}


