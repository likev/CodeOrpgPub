
/******************************************************************

	file: mrpg_read_tats.c

	Reads in task tables: TAT. operational processes and init 
	commands.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/10/05 21:03:40 $
 * $Id: mrpg_read_tats.c,v 1.52 2011/10/05 21:03:40 jing Exp $
 * $Revision: 1.52 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 
#include <mrpg.h>
#include "mrpg_def.h"


#define TMP_BUF_SIZE 128
#define INIT_CMD_SIZE 256

static void *Prc_tbl = NULL;		/* process table id */
static Mrpg_tat_entry_t *Prcs;		/* process table. Sorted by name. */
static int N_prcs = 0;			/* size of process table */

static Mrpg_comms_link_t *Cms;		/* comms links table */
static int N_cms = 0;			/* size of comms links table */

static char *Task_attr_table_basename = "task_attr_table";
static char *Task_table_basename = "task_tables";
static char Task_attr_table_name[MRPG_NAME_SIZE];
static char Task_table_name[MRPG_NAME_SIZE];
static char Cfg_extensions[MRPG_NAME_SIZE] = "extensions";
static int Task_table_extensions = 0;

static Mrpg_cmd_list_t *Startup_cmds;	/* startup init commands */
static int N_startup_cmds = 0;		/* number of startup init commands */
static void *Startup_cmds_tbl = NULL;	/* startup init commands table id */

static Mrpg_cmd_list_t *Restart_cmds;	/* restart init commands */
static int N_restart_cmds = 0;		/* number of restart init commands */
static void *Restart_cmds_tbl = NULL;	/* restart init commands table id */

static Mrpg_cmd_list_t *Shutdown_cmds;	/* shutdown commands */
static int N_shutdown_cmds = 0;		/* number of shutdown commands */
static void *Shutdown_cmds_tbl = NULL;	/* shutdown commands table id */
static Mrpg_cmd_list_t *Init_cmds;	/* init commands */
static int N_init_cmds = 0;		/* number of init commands */
static void *Init_cmds_tbl = NULL;	/* init commands table id */

static char *Ops_prcs_name = NULL;	/* operational task names */
static int N_ops_prcs = 0;		/* number of operational tasks */

static char *Perm_files = NULL;
static int N_perm_files = 0;

static int Read_a_task_file (char *fname);
static int Add_new_tat (char *task_name, char *path, int type, 
			char *args, char *site, int cpu, int mem);
static int Insert_cmp (void *e1, void *e2);
static int Search_cmp (void *e1, void *e2);
static int Read_operational_prcs ();
static void Disable_unused_processes ();
static int Read_cmd_section (char *key, void *tbl_id);
static int Read_common_attributes ();
static void Cleanup_trailing_spaces (char *str);
static int Init_task_table_name ();
static int Set_task_node ();
static int Add_distributed_support_processes ();
static int Add_a_process (char *cmd, Node_attribute_t *node, int type);
static int Add_data_replicate_processes ();
static char *Find_data_path (int data);
static int Read_cron_jobs ();
static int Read_permanent_data ();
static int Read_task_table_files ();
static void Free_task_entry (int ind);
static int Post_process_tat ();
static int File_name_cmp (const void *e1, const void *e2);
static void Remove_t_opt (char *cmd);
static void Apply_operational_process_list ();
static int Read_task_attr_file (char *fname);
static int Add_infr_processes_and_rm_duplication ();
static void Add_a_node (Mrpg_tat_entry_t *prc, Node_attribute_t *node);
static int Proc_args_line (char *line, char *task_name, char *path, int type, 
				char *site, int cpu, int mem);
static int Inst_number (Mrpg_tat_entry_t *prc);
static int Close_cs_file (int ret);


/******************************************************************

    Initializes this module. This may be called twice in which case 
    the first call is invoked when adaptation and comms info are not
    available (N_cms == 0).

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MRT_init () {

    if (Prc_tbl == NULL &&
	Read_task_table_files () < 0)
	return (-1);

    N_cms = MRD_get_CMT (&Cms);
    if (N_cms <= 0)		/* Other tables not read yet */
	return (0);

    if (Add_infr_processes_and_rm_duplication () < 0)
	return (-1);
    Disable_unused_processes ();
    if (Post_process_tat () < 0)
	return (-1);

    if (Set_task_node () < 0)
	return (-1);

    return (0);
}

/***************************************************************************

    Reads task table files. Returns 0 on success or -1 on failure.

***************************************************************************/

static int Read_task_table_files () {
    char ext_name[MRPG_NAME_SIZE], *call_name;

    if (Init_task_table_name () < 0)
	return (-1);

    if ((Prc_tbl = MISC_open_table (sizeof (Mrpg_tat_entry_t), 
			64, 1, &N_prcs, (char **)&Prcs)) == NULL ||
	(Startup_cmds_tbl = MISC_open_table (sizeof (Mrpg_cmd_list_t),
		16, 0, &N_startup_cmds, (char **)&Startup_cmds)) == NULL ||
	(Restart_cmds_tbl = MISC_open_table (sizeof (Mrpg_cmd_list_t),
		16, 0, &N_restart_cmds, (char **)&Restart_cmds)) == NULL ||
	(Shutdown_cmds_tbl = MISC_open_table (sizeof (Mrpg_cmd_list_t),
		16, 0, &N_shutdown_cmds, (char **)&Shutdown_cmds)) == NULL ||
	(Init_cmds_tbl = MISC_open_table (sizeof (Mrpg_cmd_list_t),
		16, 0, &N_init_cmds, (char **)&Init_cmds)) == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }

    LE_send_msg (LE_VL1, "Reading task tables");

    /* read baseline task table */
    Task_table_extensions = 0;
    LE_send_msg (LE_VL2, 
		"    Reading task attr table file %s", Task_attr_table_name);
    if (Read_task_attr_file (Task_attr_table_name) < 0)
	return (-1);
    if (N_prcs <= 0) {
	LE_send_msg (GL_ERROR, 
		"Base task attribute file %s not found or empty", 
						Task_attr_table_name);
	return (-1);
    }
    LE_send_msg (LE_VL2, "    Reading task table file %s", Task_table_name);
    if (Read_a_task_file (Task_table_name) < 0) {
	LE_send_msg (GL_ERROR, "Failed in reading base task file %s", 
						Task_table_name);
	return (-1);
    }

    /* read extended task tables */
    Task_table_extensions = 1;
    call_name = Cfg_extensions;
    while (MRD_get_next_file_name (call_name, 
		Task_attr_table_basename, ext_name, MRPG_NAME_SIZE) == 0) {

	LE_send_msg (LE_VL2, "    Reading task attr table file %s", ext_name);
	if (Read_task_attr_file (ext_name) < 0)
	    return (-1);
	call_name = NULL;
    }
    call_name = Cfg_extensions;
    while (MRD_get_next_file_name (call_name, 
		Task_table_basename, ext_name, MRPG_NAME_SIZE) == 0) {

	LE_send_msg (LE_VL2, "    Reading task table file %s", ext_name);
	if (Read_a_task_file (ext_name) < 0)
	    return (-1);
	call_name = NULL;
    }
    CS_cfg_name ("");

    Apply_operational_process_list ();

    if (N_startup_cmds == 0)
	LE_send_msg (LE_VL2, "Empty startup init commands table");
    if (N_restart_cmds == 0)
	LE_send_msg (LE_VL2, "Empty restart init commands table");
    if (N_shutdown_cmds == 0)
	LE_send_msg (LE_VL2, "Empty shutdown commands table");
    if (N_init_cmds == 0)
	LE_send_msg (LE_VL2, "Empty init commands table");

    return (0);
}

/******************************************************************

    Added infrastructure processes for supporting distributed 
    processing and removes diplicated entries in the table table.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Add_infr_processes_and_rm_duplication () {
    int i, k;

    if (Add_distributed_support_processes () < 0 ||
	Add_data_replicate_processes () < 0)
	return (-1);

    /* remove dupllicated tasks */
    for (i = 0; i < N_prcs; i++) {
	for (k = i + 1; k < N_prcs; k++) {
	    if (strcmp (Prcs[k].name, Prcs[i].name) != 0)
		break;
	    if (!(Prcs[k].type & MRPG_TT_MULTIPLE_INVOKE)) {
		LE_send_msg (GL_INFO, 
		    "duplicate task entry %s - removed", Prcs[k].name);
		MISC_table_free_entry (Prc_tbl, k);
		i--;
		break;
	    }
	}
    }
    return (0);
}

/******************************************************************

    Reads task attribute file "fname".

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_task_attr_file (char *fname) {
    char *line;
    char tmp[TMP_BUF_SIZE];
    int ret;

    CS_cfg_name (fname);
    CS_control (CS_COMMENT | '#');

    CS_control (CS_KEY_OPTIONAL);
    ret = CS_entry ("Task_attr_table", 0, TMP_BUF_SIZE, tmp);
    CS_control (CS_KEY_REQUIRED);
    if (ret > 0 && CS_level (CS_DOWN_LEVEL) < 0)
	return (Close_cs_file (-1));

    line = CS_THIS_LINE;
    while (1) {
	int ret, type, cpu, mem, i;
	char filename[TMP_BUF_SIZE], site[TMP_BUF_SIZE];
	char task_name[TMP_BUF_SIZE], tb[TMP_BUF_SIZE];

	CS_control (CS_KEY_OPTIONAL);
	ret = CS_entry (line, 0, TMP_BUF_SIZE, tmp);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND)
	    break;
	if (ret == CS_OPEN_ERR) {
	    LE_send_msg (GL_ERROR, "Failed in opening %s", fname);
	    return (Close_cs_file (-1));
	}
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "CS_entry CS_NEXT_LINE failed (%d)", ret);
	    return (Close_cs_file (-1));
	}
	if (strcmp (tmp, "Task") != 0) {
	    LE_send_msg (GL_ERROR, "Unexpected key %d", tmp);
	    return (Close_cs_file (-1));
	}
	if (CS_entry (CS_THIS_LINE, 1, TMP_BUF_SIZE, task_name) <= 0)
	    task_name[0] = '\0';
	CS_control (CS_KEY_REQUIRED);

	if (CS_level (CS_DOWN_LEVEL) < 0) {
	    CS_report ("Empty task section");
	    return (Close_cs_file (-1));
	}

	if (CS_entry ("filename", 1, TMP_BUF_SIZE, (void *)filename) <= 0) {
	    LE_send_msg (GL_ERROR, "Field filename not found");
	    return (Close_cs_file (-1));
	}
	if (task_name[0] == '\0')
	    strcpy (task_name, MISC_basename (filename));
	i = 2;
	MAIN_disable_report (1);
	while (CS_entry ("filename", i, TMP_BUF_SIZE - strlen (filename) - 1,
							(void *)tb) > 0) {
	    strcat (filename, " ");
	    strcat (filename, tb);
	    i++;
	}
	MAIN_disable_report (0);

	for (i = 0; i < N_prcs; i++) {	/* check Duplicated task specs */
	    if (strcmp (Prcs[i].name, task_name) == 0 &&
		!(Prcs[i].type & MRPG_TT_MULTIPLE_INVOKE)) {
		if (Prcs[i].is_extension ||
		    Prcs[i].is_extension == Task_table_extensions) {
		    LE_send_msg (GL_ERROR, 
			"Duplicated task (name %s) found in %s", 
					task_name, fname);
		    return (Close_cs_file (-1));
		}
		else {
		    Free_task_entry (i);
		    break;
		}
	    }
	}

	CS_control (CS_KEY_OPTIONAL);
	type = MRPG_TT_START_ONCE;
	if (strcmp (MISC_basename (filename), "rssd") == 0 || 
	    strcmp (MISC_basename (filename), "nds") == 0)
	    type |= MRPG_TT_SUPPORT_TASK;
	if (CS_entry ("respawn", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_RESPAWN;
	if (CS_entry ("allow_duplicate", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_ALLOW_DUPLICATE;
	if (CS_entry ("monitor_only", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_MONITOR_ONLY;
	if (CS_entry ("rpg_control_task", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_CONTROL_TASK;
	if (CS_entry ("alive_in_standby", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_ALIVE_IN_STANDBY;
	if (CS_entry ("active_channel_only", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_ACTIVE_CHANNEL_ONLY;
	if (CS_entry ("multiple_invoke", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_MULTIPLE_INVOKE;
	if (CS_entry ("report_ready_to_operate", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_REPORT_READY;
	if (CS_entry ("no_launch", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_NO_LAUNCH;
	if (CS_entry ("alive_in_allstates", 0, TMP_BUF_SIZE, tmp) > 0)
	    type |= MRPG_TT_ALL_STATES;
	site[0] = '\0';
	if (CS_entry ("site_task", 0, TMP_BUF_SIZE, site) > 0) {
	    CS_control (CS_KEY_REQUIRED);
	    if (CS_entry ("site_task", CS_ALL_TOKENS | 1, 
					TMP_BUF_SIZE, site) <= 0) {
		LE_send_msg (GL_ERROR, "Missing site task type");
		return (Close_cs_file (-1));
	    }
	    if (strcmp (site, "PRODUCT_SERVER") == 0) {
		type |= MRPG_TT_PRODUCT_SERVER;
		site[0] = '\0';
	    }
	    else if (strcmp (site, "COMM_MANAGER") == 0) {
		type |= MRPG_TT_COMM_MANAGER;
		site[0] = '\0';
	    }
	    CS_control (CS_KEY_OPTIONAL);
	}

	if (CS_entry ("cpu_limit", 1 | CS_INT, 0, (char *)&cpu) <= 0)
	    cpu = 0;			/* not specified */
	else
	    LE_send_msg (LE_VL2, "    cpu_limit set to %d for %s", cpu, task_name);
	if (CS_entry ("mem_limit", 1 | CS_INT, 0, (char *)&mem) <= 0)
	    mem = 0;			/* not specified */
	else
	    LE_send_msg (LE_VL2, "    mem_limit set to %d for %s", mem, task_name);

	MAIN_disable_report (1);
	ret = CS_entry ("args", 0, TMP_BUF_SIZE, tmp);
	MAIN_disable_report (0);
	if (ret > 0) {
	    CS_entry ("args", CS_FULL_LINE, TMP_BUF_SIZE, tmp);
	    if (Proc_args_line (tmp, task_name, filename, type, 
						site, cpu, mem) < 0)
		return (Close_cs_file (-1));
	}
	else if (ret == CS_KEY_AMBIGUOUS) {
	    char *ln = CS_THIS_LINE;
	    CS_level (CS_UP_LEVEL);
	    CS_level (CS_DOWN_LEVEL);
	    while (CS_entry (ln, 0, TMP_BUF_SIZE, tmp) > 0) {
		if (strcmp (tmp, "args") == 0) {
		    CS_entry (CS_THIS_LINE, CS_FULL_LINE, TMP_BUF_SIZE, tmp);
		    if (Proc_args_line (tmp, task_name, filename, type, 
						site, cpu, mem) < 0)
			return (Close_cs_file (-1));
		}
		ln = CS_NEXT_LINE;
	    }
	}
	else {
	    if (Add_new_tat (task_name, filename, type, "", site, 
							cpu, mem) < 0)
		return (Close_cs_file (-1));
	}

	CS_level (CS_UP_LEVEL);
	line = CS_NEXT_LINE;
    }

    return (Close_cs_file (0));
}

static int Close_cs_file (int ret) {

    CS_control (CS_CLOSE);
    CS_cfg_name ("");
    return (ret);
}

/******************************************************************

    Parses a "args" line of "line" in the task attr table and
    generates a tat entry for the line. Return 0 on success or -1 on
    failure.
	
******************************************************************/

static int Proc_args_line (char *line, char *task_name, char *path, int type, 
				char *site, int cpu, int mem) {
    char tk[TMP_BUF_SIZE], tsk_n[TMP_BUF_SIZE], ops[TMP_BUF_SIZE];
    int ret;

    if (MISC_get_token (line, "Q\"", 0, tk, TMP_BUF_SIZE) < 0 ||
	strcmp (tk, "args") != 0 ||
	MISC_get_token (line, "Q\"", 1, tk, TMP_BUF_SIZE) < 0 ||
	MISC_get_token (line, "Q\"", 3, tk, TMP_BUF_SIZE) > 0) {
	CS_report ("Unexpected args statement\n");
	return (-1);
    }
    if (MISC_get_token (line, "Q\"", 2, ops, TMP_BUF_SIZE) > 0)
	MISC_get_token (line, "Q\"", 1, tsk_n, TMP_BUF_SIZE);
    else {
	MISC_get_token (line, "Q\"", 1, ops, TMP_BUF_SIZE);
	tsk_n[0] = '\0';
    }

    if (tsk_n[0] == '\0')
	ret = Add_new_tat (task_name, path, type, ops, site, cpu, mem);
    else
	ret = Add_new_tat (tsk_n, path, type, ops, site, cpu, mem);
    if (ret < 0)
	return (-1);
    return (0);
}

/******************************************************************

    Adds data replication processes to the task table. Returns 0 
    success or -1 on failure.
	
******************************************************************/

#define COMMAND_SIZE 3 * MRPG_NAME_SIZE + 64

static int Add_data_replicate_processes () {
    int n_specs, type, ip, port, i;
    char *spec, *cr_sp, saved_spec[MRPG_NAME_SIZE];
    Mrpg_dat_entry_t *dp;

    if (!MHR_is_distributed () ||
	(MRD_get_DAT (&dp) == 0 && MRD_get_PAT (&dp) == 0))
					/* MRD_init not called */
	return (0);

    type = MRPG_TT_START_ONCE | MRPG_TT_ENABLED | 
		MRPG_TT_CONTROL_TASK | MRPG_TT_MULTIPLE_INVOKE;

    ip = 22;			/* starting ip for bcast */
    port = 46821;		/* starting port for bcast */
    n_specs = MHR_data_rep_spec (&spec);
    for (i = 0; i < n_specs; i++) {
	char *tk, *src_node, *dest_node, *path, *data_type, *comp_type;
	char cmd[COMMAND_SIZE];
	int replic, data, need_bswap;
	Node_attribute_t *src_n, *dest_n;

	cr_sp = spec;
	strncpy (saved_spec, cr_sp, MRPG_NAME_SIZE);
	saved_spec[MRPG_NAME_SIZE - 1] = '\0';
	spec += strlen (spec) + 1;
	tk = strtok (cr_sp, " \t");
	if (strcmp (tk, "replicate") == 0)
	    replic = 1;
	else if (strcmp (tk, "multicast") == 0)
	    replic = 0;
	else
	    goto err;

	tk = strtok (NULL, " \t");
	if (tk == NULL || sscanf (tk, "%d", &data) != 1)
	    goto err;
	if ((path = Find_data_path (data)) == NULL)
	    goto err;
	src_node = strtok (NULL, " \t");
	dest_node = strtok (NULL, " \t");
	data_type = comp_type = NULL;
	need_bswap = 0;
	while ((tk = strtok (NULL, " \t")) != NULL) {
	    if (strncmp (tk, "T-", 2) == 0) {
		data_type = tk + 2;
		need_bswap = 1;
	    }
	    if (strncmp (tk, "C-", 2) == 0) {
		comp_type = tk + 2;
		if (strcmp (comp_type, "0") != 0 && 
					strcmp (comp_type, "1") != 0)
		    goto err;
	    }
	}
	if (dest_node == NULL || src_node == NULL)
	    goto err;
	if ((src_n = MHR_get_node_by_name (src_node)) == NULL) {
	    LE_send_msg (GL_ERROR, 
		"source node (%s) not found", src_node);
	    goto err;
	}
	if ((dest_n = MHR_get_node_by_name (dest_node)) == NULL) {
	    LE_send_msg (GL_ERROR, 
		"destination node (%s) not found", dest_node);
	    goto err;
	}
	if (replic) {
	    sprintf (cmd, "lb_rep -p 500 -r %s:%s/%s,%s/%s", 
		src_n->hname, src_n->orpgdir, path, dest_n->orpgdir, path);
	    if (data_type != NULL && strlen (data_type) > 0)
		sprintf (cmd + strlen (cmd), ",%s", data_type);
	    if (need_bswap && src_n->is_bigendian != dest_n->is_bigendian)
		sprintf (cmd + strlen (cmd), " -t liborpg.so:ORPG_smi_info");
	    if (comp_type != NULL)
		sprintf (cmd + strlen (cmd), " -C %s", comp_type);
	    if (Add_a_process (cmd, dest_n, type) < 0)
		return (-1);
	}
	else {			/* multicast */
	    sprintf (cmd, "bcast -m 225.3.41.%d -p %d %s/%s", 
					ip, port, src_n->orpgdir, path);
	    if (Add_a_process (cmd, src_n, type) < 0)
		return (-1);
	    while (1) {
		sprintf (cmd, "brecv -m 225.3.41.%d -p %d -b %s %s/%s", 
			ip, port, src_n->hname, dest_n->orpgdir, path);
		if (Add_a_process (cmd, dest_n, type) < 0)
		    return (-1);
		dest_node = strtok (NULL, " \t");
		if (dest_node == NULL)
		    break;
		if ((dest_n = MHR_get_node_by_name (dest_node)) == NULL) {
		    LE_send_msg (GL_ERROR, 
			"destination node (%s) not found", dest_node);
		    goto err;
		}
	    }
	    ip++;
	    port++;
	}
    }
    return (0);

err:
    LE_send_msg (GL_ERROR, "bad resource spec: %s", saved_spec);
    return (-1);
}

/******************************************************************

    Finds and returns the partial path of data of ID "data". If not 
    found, NULL is returned.

******************************************************************/

static char *Find_data_path (int data) {
    int n_pats, n_dats, i;
    Mrpg_dat_entry_t *patp, *datp, *datap;

    n_pats = MRD_get_PAT (&patp);
    n_dats = MRD_get_DAT (&datp);
    datap = NULL;
    for (i = 0; i < n_pats; i++) {
	if (patp[i].data_id == data) {
	    datap = patp + i;
	    break;
	}
    }
    if (datap == NULL) {
	for (i = 0; i < n_dats; i++) {
	    if (datp[i].data_id == data) {
		datap = datp + i;
		break;
	    }
	}
    }
    if (datap == NULL) {
	LE_send_msg (GL_ERROR, "data %d not found\n", data);
	return (NULL);
    }
    return (datap->path);
}

/******************************************************************

    Adds processes defined in the resource table for supporting 
    distributed operation to the task table. Returns 0 success or 
    -1 on failure.
	
******************************************************************/

static int Add_distributed_support_processes () {
    Node_attribute_t *nodes;
    int n_nodes, i;

    if (!MHR_is_distributed ())
	return (0);

    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	int k;
	for (k = 0; k < nodes[i].n_processes; k++) {
	    if (Add_a_process (nodes[i].processes[k], nodes + i,
		MRPG_TT_START_ONCE | MRPG_TT_ENABLED | 
		MRPG_TT_CONTROL_TASK | MRPG_TT_ALLOW_DUPLICATE) < 0)
		return (-1);
	}
    }
    return (0);
}

/******************************************************************

    Adds a process of "cmd" to the task table. Returns 0 success or 
    -1 on failure.
	
******************************************************************/

static int Add_a_process (char *cmd, Node_attribute_t *node, int type) {
    Mrpg_tat_entry_t ta;
    char *p, *tp;
    int i;

    for (i = 0; i < N_prcs; i++) {
	Mrpg_tat_entry_t *p = Prcs + i;
	if ((p->type & MRPG_TT_SUPPORT_TASK))
	    continue;
	if (strcmp (cmd, p->name) == 0 || strcmp (cmd, p->cmd_name) == 0) {
	    LE_send_msg (GL_ERROR, 
		"Support task name (%s) identical to another task", cmd);
	    return (-1);
	}
    }

    LE_send_msg (LE_VL3, "Add process %s on %s\n", cmd, node->node);
    p = malloc (3 * strlen (cmd) + 2);
    if (p == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }
    ta.name = p;
    strcpy (p, cmd);
    tp = p;
    while (*tp != '\0' && *tp != ' ' && *tp != '\t')
	tp++;
    *tp = '\0';
    p += strlen (p) + 1;
    ta.cmd = p;
    strcpy (p, cmd);
    Cleanup_trailing_spaces (ta.cmd);
    p += strlen (p) + 1;
    ta.cmd_name = p;
    strcpy (p, ta.name);
    ta.cmd_index = 0;
    ta.type = type | MRPG_TT_SUPPORT_TASK;
    ta.node = node;
    ta.cpu = ta.mem = ta.swap = 0;
    ta.status = MRPG_PS_NOT_STARTED;
    ta.misc = 0;
    ta.n_dupps = 0;
    ta.is_extension = 0;
    ta.site_str = NULL;
    ta.next_node = NULL;
    ta.cpu_limit = ta.mem_limit = 0;

    if (MISC_table_insert (Prc_tbl, (void *)&ta, Insert_cmp) < 0) {
	LE_send_msg (GL_ERROR, "Malloc failed\n");
	return (-1);
    }
    return (0);
}

/******************************************************************

    Sets node info for each task. Infrastructure processes rssd and 
    nds and monitor only processes have to be monitored on all nodes.
    This function add them to all nodes for monitoring. 

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Set_task_node () {
    int n_nodes, i, k;
    Node_attribute_t *nodes;
    Mrpg_tat_entry_t *p;

    /* infr processes monitored on all nodes */
    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < N_prcs; i++) {
	p = Prcs + i;
	if (strcmp (p->name, "rssd") == 0 ||
	    strcmp (p->name, "nds") == 0 ||
	    (p->type & MRPG_TT_MONITOR_ONLY)) {
	    for (k = 0; k < n_nodes; k++) {
		p->type |= MRPG_TT_MONITOR_ONLY;
		Add_a_node (p, nodes + k);
	    }
	    continue;
	}
    }

    /* set task nodes */
    for (i = 0; i < N_prcs; i++) {
	Node_attribute_t **nds;
	p = Prcs + i;
	if (p->type & (MRPG_TT_SUPPORT_TASK | MRPG_TT_MONITOR_ONLY))
	    continue;
	n_nodes = MHR_get_task_hosts (p->name, &nds);
	if (n_nodes <= 0) {
	    LE_send_msg (GL_ERROR, "Zero node for task %s\n", p->name);
	    return (-1);
	}
	for (k = 0; k < n_nodes; k++)		/* add new nodes */
	    Add_a_node (p, nds[k]);
    }
    return (0);
}

/******************************************************************

    Adds "node" to process "prc". Duplicated node is not added.
	
******************************************************************/

static void Add_a_node (Mrpg_tat_entry_t *prc, Node_attribute_t *node) {
    Mrpg_tat_entry_t *cr, *n;

    if (prc->node == NULL) {
	prc->node = node;
	return;
    }
    cr = prc;
    while (cr != NULL) {
	if (cr->node == node)
	    return;		/* already listed */
	if (cr->next_node != NULL)
	    cr = cr->next_node;
	else
	    break;
    }
    n = (Mrpg_tat_entry_t *)MISC_malloc (sizeof (Mrpg_tat_entry_t));
    memcpy (n, prc, sizeof (Mrpg_tat_entry_t));
    n->node = node;
    n->next_node = NULL;
    if (prc->next_node == NULL)
	prc->next_node = n;
    else
	cr->next_node = n;
}

/******************************************************************

    Reads the Install_cmds section in the task table and executes
    the commands.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MRT_process_install_cmds () {
    Mrpg_cmd_list_t *cmds;
    void *cmds_tbl;
    int n_cmds, i;

    LE_send_msg (LE_VL1, "Process installation commands");
    if (Init_task_table_name () < 0)
	return (-1);

    if ((cmds_tbl = MISC_open_table (sizeof (Mrpg_cmd_list_t),
			16, 0, &n_cmds, (char **)&cmds)) == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }

    CS_cfg_name (Task_table_name);
    CS_control (CS_COMMENT | '#');

    if (Read_cmd_section ("Install_cmds", cmds_tbl) < 0)
	return (Close_cs_file (-1));
    Close_cs_file (0);

    if (MPC_execute_commands (n_cmds, cmds) < 0)
	return (-1);

    for (i = 0; i < n_cmds; i++)
	free (cmds[i].name);
    MISC_free_table (cmds_tbl);
    return (0);
}

/******************************************************************

    Initializes Task_table_name.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Init_task_table_name () {
    static int initialized = 0;
    char dir[MRPG_NAME_SIZE], tmp[MRPG_NAME_SIZE];
    int len;

    if (initialized)
	return (0);
    len = MISC_get_cfg_dir (dir, MRPG_NAME_SIZE);
    if (len <= 0) {
	LE_send_msg (GL_ERROR, "ORPG cfg dir not found\n");
	return (-1);
    }
    if (len + strlen (Cfg_extensions) + 3 > MRPG_NAME_SIZE ||
	len + strlen (Task_table_basename) + 3 > MRPG_NAME_SIZE ||
	len + strlen (Task_attr_table_basename) + 3 > MRPG_NAME_SIZE) {
	LE_send_msg (GL_ERROR, "ORPG cfg dir too long\n");
	return (-1);
    }
    strcpy (tmp, dir);
    strcat (tmp, "/");
    strcat (tmp, Cfg_extensions);
    strcpy (Cfg_extensions, tmp);
    if (MRD_get_table_file_name (Task_table_name, dir, 
			Task_table_basename, tmp, MRPG_NAME_SIZE) < 0 ||
	MRD_get_table_file_name (Task_attr_table_name, dir, 
			Task_attr_table_basename, tmp, MRPG_NAME_SIZE) < 0)
	return (-1);
    initialized = 1;
    return (0);
}

/******************************************************************

    Returns operational process table in "prcsp".

    Returns the size of the table.
	
******************************************************************/

int MRT_get_ops (Mrpg_tat_entry_t **prcsp) {
    *prcsp = Prcs;
    return (N_prcs);
}

/******************************************************************

    Returns the next task-node structure. If "first" is not 0, it
    returns the first. Returns NULL if the next does not 
	
******************************************************************/

Mrpg_tat_entry_t *MRT_get_next_task (int first) {
    static int cr_ind = 0;
    static Mrpg_tat_entry_t *cr_t = NULL;
    Mrpg_tat_entry_t *ret_t;
    Mrpg_tat_entry_t *prc;

    if (first) {
	cr_ind = 0;
	cr_t = NULL;
    }
    if (cr_ind >= N_prcs)
	return (NULL);
    prc = Prcs + cr_ind;
    if (cr_t == NULL)
	cr_t = prc;
    ret_t = cr_t;
    if (cr_t->next_node != NULL)
	cr_t = cr_t->next_node;
    else {
	cr_ind++;
	cr_t = NULL;
    }
    return (ret_t);
}

/******************************************************************

    Returns startup command table in "startup_cmdsp".

    Returns the size of the table.
	
******************************************************************/

int MRT_get_startup_cmds (Mrpg_cmd_list_t **startup_cmdsp) {
    *startup_cmdsp = Startup_cmds;
    return (N_startup_cmds);
}

/******************************************************************

    Returns restart command table in "restart_cmdsp".

    Returns the size of the table.
	
******************************************************************/

int MRT_get_restart_cmds (Mrpg_cmd_list_t **restart_cmdsp) {
    *restart_cmdsp = Restart_cmds;
    return (N_restart_cmds);
}

/******************************************************************

    Returns shutdown command table in "shutdown_cmdsp".

    Returns the size of the table.
	
******************************************************************/

int MRT_get_shutdown_cmds (Mrpg_cmd_list_t **shutdown_cmdsp) {
    *shutdown_cmdsp = Shutdown_cmds;
    return (N_shutdown_cmds);
}

/******************************************************************

    Returns init command table in "init_cmdsp".

    Returns the size of the table.
	
******************************************************************/

int MRT_get_init_cmds (Mrpg_cmd_list_t **init_cmdsp) {
    *init_cmdsp = Init_cmds;
    return (N_init_cmds);
}

/******************************************************************

    Returns the names of the permanent files.

    Returns the number of permanent files.
	
******************************************************************/

int MRT_get_perm_file_names (char **pf_names) {
    *pf_names = Perm_files;
    return (N_perm_files);
}

/******************************************************************

    Reads the task tables in file "fname".

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_a_task_file (char *fname) {
    char startup_init_section[MRPG_NAME_SIZE];
    int len;

    CS_cfg_name (fname);
    CS_control (CS_COMMENT | '#');

    if (!Task_table_extensions) {
	if (Read_common_attributes () < 0)
	    return (Close_cs_file (-1));
	if (Read_cron_jobs () < 0)
	    return (Close_cs_file (-1));
	if (Read_permanent_data () < 0)
	    return (Close_cs_file (-1));

	len = strlen (MAIN_init_section_name ());
	if (len > 0 && len < MRPG_NAME_SIZE - 32)
	    sprintf (startup_init_section, "%s_%s", 
		    "Startup_cmds", MAIN_init_section_name ());
	else
	    strcpy (startup_init_section, "Startup_cmds");
    
	if (Read_cmd_section (startup_init_section, Startup_cmds_tbl) < 0 ||
	    Read_cmd_section ("Restart_init_cmds", Restart_cmds_tbl) < 0 ||
	    Read_cmd_section ("Shutdown_cmds", Shutdown_cmds_tbl) < 0 ||
	    Read_cmd_section ("Init_cmds", Init_cmds_tbl) < 0)
	    return (Close_cs_file (-1));
    }

    if (Read_operational_prcs () < 0)
	return (Close_cs_file (-1));

    return (Close_cs_file (0));
}

/******************************************************************

    Reads common process attributes. The limits are sent to the 
    mrpg_manage_rpg module.

    Returns 0 success or -1 on failure.
	
******************************************************************/

static int Read_common_attributes () {
    char tmp[TMP_BUF_SIZE];
    int cpu_limit, mem_limit, cpu_window;

    CS_level (CS_TOP_LEVEL);
    if (CS_entry ("Common_process_attributes", 0, TMP_BUF_SIZE, tmp) <= 0 ||
	CS_level (CS_DOWN_LEVEL) < 0) {
	LE_send_msg (LE_VL1, "Common_process_attribute table not found");
	return (-1);
    }
    CS_control (CS_KEY_OPTIONAL);

    if (CS_entry ("cpu_limit", 1 | CS_INT, 0, (char *)&cpu_limit) <= 0) {
	cpu_limit = 100;		/* no limit */
	LE_send_msg (LE_VL2, "    Common cpu_limit not found - No limit is applied");
    }
    else
	LE_send_msg (LE_VL2, "    Common cpu_limit set to %d", cpu_limit);
    if (CS_entry ("mem_limit", 1 | CS_INT, 0, (char *)&mem_limit) <= 0) {
	mem_limit = 0x7fffffff;		/* no limit */
	LE_send_msg (LE_VL2, "    Common mem_limit not found - No limit is applied");
    }
    else
	LE_send_msg (LE_VL2, "    Common mem_limit set to %d", mem_limit);
    if (CS_entry ("cpu_window", 1 | CS_INT, 0, (char *)&cpu_window) <= 0) {
	cpu_window = 15;		/* seconds */
	LE_send_msg (LE_VL2, "    Common cpu_window not found - Set to 15");
    }
    else
	LE_send_msg (LE_VL2, "    Common cpu_window set to %d", cpu_window);
    MMR_set_limits (cpu_limit, mem_limit, cpu_window);

    if (CS_entry ("save_log_command", 1, TMP_BUF_SIZE, tmp) <= 0)
	tmp[0] = '\0';
    MMR_set_save_log_cmd (tmp);

    CS_level (CS_UP_LEVEL);
    CS_control (CS_KEY_REQUIRED);
    return (0);
}

/******************************************************************

    Read cron jobs.

    Returns 0 success or -1 on failure.
	
******************************************************************/

static int Read_cron_jobs () {
    char tmp[TMP_BUF_SIZE];
    char *line;

    CS_level (CS_TOP_LEVEL);
    if (CS_entry ("Cron_jobs", 0, TMP_BUF_SIZE, tmp) <= 0 ||
	CS_level (CS_DOWN_LEVEL) < 0) {
	LE_send_msg (LE_VL1, "Cron_jobs table not found");
	return (-1);
    }
    CS_control (CS_KEY_OPTIONAL);

    line = CS_THIS_LINE;
    while (1) {
	int ret, h, m;
	char c, cmd[INIT_CMD_SIZE];

	ret = CS_entry (line, 0, TMP_BUF_SIZE, tmp);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "CS_entry CS_NEXT_LINE (cron_job) failed");
	    return (-1);
	}
	CS_control (CS_KEY_REQUIRED);
	if (sscanf (tmp, "%d%*c%d%c", &h, &m, &c) != 2 ||
	    h < 0 || h > 23 || m < 0 || m > 59) {
	    CS_report ("Bad cron job time");
	    return (-1);
	}
	ret = CS_entry (CS_THIS_LINE, 1, INIT_CMD_SIZE, cmd);
	if (ret <= 0) {
	    CS_report ("Cron_job command not found");
	    return (-1);
	}
	MMR_add_cron_job (h, m, cmd);
	line = CS_NEXT_LINE;
    }
    CS_level (CS_UP_LEVEL);
    CS_control (CS_KEY_REQUIRED);
    return (0);
}

/******************************************************************

    Read permanent file names.

    Returns 0 success or -1 on failure.
	
******************************************************************/

static int Read_permanent_data () {
    char tmp[TMP_BUF_SIZE];
    char *line;

    CS_level (CS_TOP_LEVEL);
    CS_control (CS_KEY_OPTIONAL);
    if (CS_entry ("Permanent_files", 0, TMP_BUF_SIZE, tmp) <= 0 ||
	CS_level (CS_DOWN_LEVEL) < 0) {
	LE_send_msg (LE_VL2, "Permanent_files table not found");
	return (0);
    }

    line = CS_THIS_LINE;
    while (1) {
	int ret;

	ret = CS_entry (line, 0, TMP_BUF_SIZE, tmp);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"CS_entry CS_NEXT_LINE (Permanent_files) failed");
	    return (-1);
	}
	Perm_files = STR_append (Perm_files, tmp, strlen (tmp) + 1);
	N_perm_files++;
	line = CS_NEXT_LINE;
    }
    CS_level (CS_UP_LEVEL);
    CS_control (CS_KEY_REQUIRED);
    return (0);
}

/******************************************************************

    Adds a new entry to the TAT.

    Input:	task_name - task name.
		path - command path.
		type - task type.
		args - process command line arguments.
		site - site specification string.
		cpu - task CPU limit
		mem - task memory limit

    Returns 0 success or -1 on failure.
	
******************************************************************/

static int Add_new_tat (char *task_name, char *path, int type, 
			char *args, char *site, int cpu, int mem) {
    Mrpg_tat_entry_t ta;
    int name_len, n;
    char tk[TMP_BUF_SIZE];

    if (strcmp (task_name, "lb_rep") == 0 ||
	strcmp (task_name, "bcast") == 0 ||
	strcmp (task_name, "brecv") == 0)
	return (0);
    name_len = strlen (task_name);
    ta.name = malloc (name_len * 2 + strlen (path) * 2 + strlen (args) + 12);
    if (ta.name == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed\n");
	return (-1);
    }

    strcpy (ta.name, task_name);
    ta.cmd_name = ta.name + name_len + 1;
    MISC_get_token (path, "", 0, tk, TMP_BUF_SIZE);
    strcpy (ta.cmd_name, MISC_basename (tk));
    if ((n = MISC_get_token (path, "", 0, NULL, 0)) > 1) {
	MISC_get_token (path, "", n - 1, tk, TMP_BUF_SIZE);
	strcat (ta.cmd_name, "@");
	strcat (ta.cmd_name, MISC_basename (tk));
    }
    ta.cmd = ta.cmd_name + strlen (ta.cmd_name) + 1;
    strcpy (ta.cmd, path);
    strcat (ta.cmd, " ");
    sprintf (ta.cmd + strlen (ta.cmd), "-T %s ", task_name);
    strcat (ta.cmd, args);
    Cleanup_trailing_spaces (ta.cmd);
    ta.type = type;
    ta.node = NULL;
    ta.cpu = ta.mem = ta.swap = 0;
    ta.pid = -1;
    ta.status = MRPG_PS_NOT_STARTED;
    ta.misc = 0;
    ta.n_dupps = 0;
    ta.next_node = NULL;
    ta.is_extension = Task_table_extensions;
    ta.cpu_limit = cpu;
    ta.mem_limit = mem;
    ta.instance = -1;		/* no longer used */

    ta.site_str = NULL;
    if (strlen (site) != 0) {
	ta.site_str = (char *)malloc (strlen (site) + 1);
	if (ta.site_str  == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed\n");
	    return (-1);
	}
	strcpy (ta.site_str, site);
    }

    if (MISC_table_insert (Prc_tbl, (void *)&ta, Insert_cmp) < 0) {
	LE_send_msg (GL_ERROR, "Malloc failed\n");
	return (-1);
    }

    return (0);
}

/******************************************************************

    Removes trailing space characters from string "str".
	
******************************************************************/

static void Cleanup_trailing_spaces (char *str) {
    char *cpt;
    cpt = str + strlen (str) - 1;
    while (cpt >= str && (*cpt == ' ' || *cpt == '\t' || *cpt == '\n')) {
	*cpt = '\0';
	cpt--;
    }
}

/******************************************************************

    Frees task table entry of index "ind".
	
******************************************************************/

static void Free_task_entry (int ind) {
    Mrpg_tat_entry_t *ta;

    ta = Prcs + ind;
    free (ta->name);
    if (ta->site_str != NULL)
	free (ta->site_str);
    MISC_table_free_entry (Prc_tbl, ind);
}

/******************************************************************

    Reads the operational process table.

    Returns 0 success or -1 on failure.
	
******************************************************************/

static int Read_operational_prcs () {
    char tmp[TMP_BUF_SIZE];
    char *line;

    CS_level (CS_TOP_LEVEL);

    if (CS_entry ("Operational_processes", 0, TMP_BUF_SIZE, tmp) <= 0 ||
	CS_level (CS_DOWN_LEVEL) < 0)
	return (0);

    CS_control (CS_KEY_OPTIONAL);

    line = CS_THIS_LINE;
    while (1) {
	int ret, remove, found, i;
	char *p, *name;

	ret = CS_entry (line, CS_FULL_LINE, TMP_BUF_SIZE, tmp);
	if (ret < 0)
	    break;
	Cleanup_trailing_spaces (tmp);

	name = tmp;
	while (*name == ' ' || *name == '\t')
	    name++;
	remove = 0;
	if (*name == '-' || *name == '+') {
	    if (*name == '-')
		remove = 1;
	    name++;
	    while (*name == ' ' || *name == '\t')
		name++;
	}

	p = Ops_prcs_name;
	found = 0;
	for (i = 0; i < N_ops_prcs; i++) {
	    if (strcmp (p, name) == 0) {
		found = 1;
		break;
	    }
	    p += strlen (p) + 1;
	}
		
	if (remove && found) {
	    for (i = 0; i < strlen (name); i++)
		p[i] = ' ';			/* remove the name */
	}
	if (!remove && !found) {
	    Ops_prcs_name = STR_append (Ops_prcs_name, 
					name, strlen (name) + 1);
	    N_ops_prcs++;
	}
	line = CS_NEXT_LINE;
    }

    return (0);
}

/************************************************************************

    Disables unused processes: unused product servers and comms managers 
    in terms of the comms link configuration and other unused site 
    dependent tasks.

************************************************************************/

static void Disable_unused_processes () {
    int rda_link, i;

    rda_link = MRD_get_RDA_link ();

    for (i = 0; i < N_prcs; i++) {
	Mrpg_tat_entry_t *prc;

	prc = Prcs + i;
	if (prc->type & MRPG_TT_PRODUCT_SERVER) {  /* product server task */
	    int k, inst;

	    inst = Inst_number (prc);
	    for (k = 0; k < N_cms; k++)
		if (Cms[k].user == inst && k != rda_link)
		    break;
	    if (k >= N_cms) {		/* not used */
		prc->type &= ~(MRPG_TT_ENABLED);
		prc->type |= MRPG_TT_DISABLED_CFG;
            }
	}
	if (prc->type & MRPG_TT_COMM_MANAGER) {	/* comm_manager task */
	    int k, inst;

	    inst = Inst_number (prc);
	    for (k = 0; k < N_cms; k++)
		if (Cms[k].cm == inst && 
				strcmp (Cms[k].cm_mgr, prc->cmd_name) == 0)
		    break;
	    if (k >= N_cms) {		/* not used */
		prc->type &= ~(MRPG_TT_ENABLED);
		prc->type |= MRPG_TT_DISABLED_CFG;
            }
	}
    }
    MRT_set_site_processes ();
}

/******************************************************************

    Returns the instance number (the last argument of command line)
    for comms manager and p_server.
	
******************************************************************/

static int Inst_number (Mrpg_tat_entry_t *prc) {
    int n_tks, inst;

    if ((n_tks = MISC_get_token (prc->cmd, "", 0, NULL, 0)) <= 0 ||
	MISC_get_token (prc->cmd, "Ci", n_tks - 1, &inst, 0) <= 0)
	return (-1);
    return (inst);
}

/************************************************************************

    Disables or enable site specific processes in terms of the current
    adaptation data.

************************************************************************/

int MRT_set_site_processes () {
    int i;

    for (i = 0; i < N_prcs; i++) {
	Mrpg_tat_entry_t *prc;

	prc = Prcs + i;
	if (prc->site_str != NULL &&	/* get and check site info */
	    (prc->type & (MRPG_TT_ENABLED | MRPG_TT_DISABLED_CFG))) {
	    int site_enabled;
	    char *p;

	    site_enabled = 0;
	    if (strcmp (prc->site_str, "BDDS") == 0) {
		if (DEAU_get_string_values ("site_info.has_bdds", &p) == 1 &&
		    strcmp (p, "Yes") == 0)
		    site_enabled = 1;
	    }
	    else if (strcmp (prc->site_str, "MLOS") == 0) {
		if (DEAU_get_string_values ("site_info.has_mlos", &p) == 1 &&
		    strcmp (p, "Yes") == 0)
		    site_enabled = 1;
	    }
	    else if (strcmp (prc->site_str, "Redundant") == 0) {
		if (DEAU_get_string_values ("Redundant_info.redundant_type", 
							&p) == 1 &&
		    strcmp (p, "FAA Redundant") == 0)
		    site_enabled = 1;
	    }
	    else if (strcmp (prc->site_str, "RMS") == 0) {
		if (DEAU_get_string_values ("site_info.has_rms", &p) == 1 &&
		    strcmp (p, "Yes") == 0)
		    site_enabled = 1;
	    }
	    else if (strcmp (prc->site_str, "LDM") == 0) {
		if (MISC_system_to_buffer 
			("/usr/bin/which ldmadmin", NULL, 0, NULL) == 0)
		    site_enabled = 1;
	    }
	    else {
		char id[TMP_BUF_SIZE], value[TMP_BUF_SIZE];
		strcpy (id, "site_info.");
		if (MISC_get_token (prc->site_str, "S=", 0, 
			id + strlen (id), TMP_BUF_SIZE - strlen (id)) > 0 &&
		    MISC_get_token (prc->site_str, "S=", 1, 
			value, TMP_BUF_SIZE) > 0) {
		    if (DEAU_get_string_values (id, &p) == 1 &&
			strcmp (p, value) == 0)
			site_enabled = 1;
		}
		else {
		    LE_send_msg (GL_ERROR, 
		    "Unexpected site_task label: %s - Ignored", prc->site_str);
		}
	    }

	    if (!site_enabled){		/* disable the task */
		prc->type &= ~(MRPG_TT_ENABLED);
		prc->type |= MRPG_TT_DISABLED_CFG;
            }
	    else {
		prc->type &= ~(MRPG_TT_DISABLED_CFG);
		prc->type |= MRPG_TT_ENABLED;
	    }
	}
    }
    return (0);
}

/************************************************************************

    Comparison function for table Prcs insertion. The table is sorted in
    terms of process name.

************************************************************************/

static int Insert_cmp (void *e1, void *e2) {

    Mrpg_tat_entry_t *prc1, *prc2;
    prc1 = (Mrpg_tat_entry_t *)e1;
    prc2 = (Mrpg_tat_entry_t *)e2;
    return (strcmp (prc1->name, prc2->name));
}

/************************************************************************

    Post-processes the task attribute table. It sorts the task table
    based on cmd_name (executable file names) and puts the results in
    field "cmd_index". It sets fields "first_cmd_index" and bit
    "MRPG_MISC_SHARE_CMD". It verifies "-T" option specified in the line
    lead by key "args" and removes unnecessary "-T" option added earlier
    in mrpg. The function return 0 on success or -1 on failure.

************************************************************************/

static int Post_process_tat () {
    char *buf;
    Mrpg_tat_entry_t *prc;
    int i;

    for (i = 0; i < N_prcs; i++)
	Prcs[i].cmd_index = i;
    buf = malloc (N_prcs * sizeof (Mrpg_tat_entry_t));
    if (buf == NULL) {
	LE_send_msg (GL_ERROR, "Malloc faile in sort_processes");
	return (-1);
    }
    memcpy (buf, Prcs, N_prcs * sizeof (Mrpg_tat_entry_t));

    qsort (buf, N_prcs, sizeof (Mrpg_tat_entry_t), File_name_cmp);
    prc = (Mrpg_tat_entry_t *)buf;
    for (i = 0; i < N_prcs; i++) {
	Prcs[i].cmd_index = prc[i].cmd_index;
	Prcs[i].first_cmd_index = -1;
    }
    free (buf);

    for (i = 1; i < N_prcs; i++) {
	int cr_ind, prev_ind;
	cr_ind = Prcs[i].cmd_index;
	prev_ind = Prcs[i - 1].cmd_index;
	if (Prcs[cr_ind].type & 
			(MRPG_TT_MONITOR_ONLY | MRPG_TT_MULTIPLE_INVOKE | 
						MRPG_TT_SUPPORT_TASK))
	    continue;
	if (strcmp (Prcs[cr_ind].cmd_name, Prcs[prev_ind].cmd_name) == 0) {
	    Prcs[cr_ind].misc |= MRPG_MISC_SHARE_CMD;
	    Prcs[prev_ind].misc |= MRPG_MISC_SHARE_CMD;
	    if (Prcs[prev_ind].first_cmd_index < 0)
		Prcs[prev_ind].first_cmd_index = i - 1;
	    Prcs[cr_ind].first_cmd_index = Prcs[prev_ind].first_cmd_index;
	}
    }

    for (i = 0; i < N_prcs; i++) {
	if (!(Prcs[i].misc & MRPG_MISC_SHARE_CMD)) {
	    if (strcmp (Prcs[i].name, Prcs[i].cmd_name) == 0)
		Remove_t_opt (Prcs[i].cmd);
	}
	else {		/* check -T in cmd argument spec */
	    char *p = strstr (Prcs[i].cmd, " -T ");
	    if (p != NULL) {
		char topt[128];
		p += 4;
		MRT_get_t_option (p, topt, 128);
		if (topt[0] != '\0') {
		    if (strcmp (topt, Prcs[i].name) != 0) {
			LE_send_msg (GL_ERROR, 
				"Task %s has wrong -T option (%s)", 
					Prcs[i].name, topt);
			return (-1);
		    }
		    else
			Remove_t_opt (Prcs[i].cmd);
		}
	    }
	}
    }

    return (0);
}

/************************************************************************

    Removes the -T option added by mrpg. The work is done in-place.

************************************************************************/

static void Remove_t_opt (char *cmd) {
    char *st, *p;
    st = strstr (cmd, " -T ");
    if (st == NULL)
	return;
    p = st + 4;
    while (*p != '\0' && *p != ' ')
	p++;
    if (*p == ' ')
	p++;
    memmove (st + 1, p, strlen (p) + 1);
}

/************************************************************************

    Returns the "-T" option in command line "cmd" and returns it in "buf"
    of "buf_size". Returns "" if the option is not found. This does not
    process style such as "-dTf something" where d, T and f are options.

************************************************************************/

#define CMD_BUF_SIZE 256

void MRT_get_t_option (char *cmd, char *buf, int buf_size) {
    char *topt, *p;

    topt = "";
    while ((p = strstr (cmd, "-T")) != NULL) {
	if (p == cmd || p[-1] == ' ' || p[-1] == '\t')
	    break;
	p += 2;
    }
    if (p != NULL) {
	char buf[256];
	strncpy (buf, p + 2, CMD_BUF_SIZE);
	buf[CMD_BUF_SIZE - 1] = 0;
	topt = buf;
	while (*topt == ' ' || *topt == '\t')
	    topt++;
	p = topt;
	while (*p != '\0' && *p != ' ' && *p != '\t')
	    p++;
	*p = '\0';
    }
    strncpy (buf, topt, buf_size);
    buf[buf_size - 1] = '\0';
}

/************************************************************************

    Comparison function for sorting executable file names. The table is 
    sorted in terms of file name.

************************************************************************/

static int File_name_cmp (const void *e1, const void *e2) {
    Mrpg_tat_entry_t *prc1, *prc2;
    prc1 = (Mrpg_tat_entry_t *)e1;
    prc2 = (Mrpg_tat_entry_t *)e2;
    return (strcmp (prc1->cmd_name, prc2->cmd_name));
}

/************************************************************************

    Comparison function for table Prcs name search.

************************************************************************/

static int Search_cmp (void *e1, void *e2) {
    Mrpg_tat_entry_t *prc1, *prc2;
    prc1 = (Mrpg_tat_entry_t *)e1;
    prc2 = (Mrpg_tat_entry_t *)e2;
    return (strcmp (prc1->name, prc2->name));
}

/******************************************************************

    Reads a section of commands.

    Input:	key - key of the section.
		tbl_id - command table id.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_cmd_section (char *key, void *tbl_id) {
    Mrpg_cmd_list_t cmds, *new;
    char *line;
    char tmp[INIT_CMD_SIZE];

    CS_level (CS_TOP_LEVEL);
    if (CS_entry (key, 0, INIT_CMD_SIZE, tmp) <= 0 ||
	CS_level (CS_DOWN_LEVEL) < 0) {
	return (0);
    }
    CS_control (CS_KEY_OPTIONAL);

    CS_level (CS_TOP_LEVEL);
    CS_entry (key, 0, INIT_CMD_SIZE, tmp);
    CS_level (CS_DOWN_LEVEL);
    line = CS_THIS_LINE;
    while (1) {
	char *cpt, *tp, *name;
	int ret, tlen;

	ret = CS_entry (line, CS_FULL_LINE, INIT_CMD_SIZE, tmp);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND)
	    break;

	cpt = MRT_get_first_token (tmp, &tlen);
	if (tlen <= 0) {
	    LE_send_msg (GL_ERROR, "cmd name not found in cmd line %s", tmp);
	    return (-1);
	}
	name = cpt;
	tp = cpt;
	while (*cpt != '\0' && cpt - tp <= tlen) {
	    if (*cpt == '/')
		name = cpt + 1;
	    cpt++;
	}
	tlen -= name - tp;
	cmds.name = (char *)malloc (tlen + strlen (tmp) + 2);
	cmds.cmd = cmds.name + tlen + 1;
	memcpy (cmds.name, name, tlen);
	cmds.name[tlen] = '\0';
	cpt = MRT_get_last_token (tmp, &tlen); /* find the last token */
	cmds.type = 0;
	if (tlen > 0 && strcmp (cpt, "STOP_ON_ERROR") == 0) {
	    cmds.type |= MRPG_STOP_ON_ERROR;
	    *cpt = '\0';
	}
	Cleanup_trailing_spaces (tmp);
	strcpy (cmds.cmd, tmp);

	new = MISC_table_new_entry (tbl_id, NULL);
	if (new == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed");
	    return (-1);
	}
	memcpy (new, &cmds, sizeof (Mrpg_cmd_list_t));

	line = CS_NEXT_LINE;
    }

    return (0);
}

/**************************************************************************

    Returns the pointer to the first char of of the last token of "str".
    The token length is returned in "len". len = 0 if there is no token.

**************************************************************************/

char *MRT_get_last_token (char *str, int *len) {
    char *cpt, *end, c;
    int in_token;

    cpt = str + strlen (str) - 1;
    end = cpt + 1;
    in_token = 0;
    while (cpt >= str) {
	if (!in_token) {
	    if ((c = *cpt) != ' ' && c != '\n' && c != '\t')
		in_token = 1;
	    else
		end = cpt;
	}
	else {
	    if ((c = *cpt) == ' ' || c == '\n' || c == '\t')
		break;
	}
	cpt--;
    }
    cpt++;
    *len = end - cpt;
    return (cpt);
}

/**************************************************************************

    Returns the pointer to the first char of of the first token of "str".
    The token length is returned in "len". len = 0 if there is no token.

**************************************************************************/

char *MRT_get_first_token (char *str, int *len) {
    char *cpt, *st, c;
    int in_token;

    cpt = str;
    st = cpt;
    in_token = 0;
    while ((c = *cpt) != '\0') {
	if (!in_token) {
	    if (c != ' ' && c != '\n' && c != '\t') {
		in_token = 1;
	    }
	    else
	 	st = cpt + 1;
	}
	else {
	    if (c == ' ' || c == '\n' || c == '\t')
		break;
	}
	cpt++;
    }
    *len = cpt - st;
    return (st);
}

/************************************************************************

    Sets bit MRPG_TT_ENABLED based on the "Operational_processes" table. 

************************************************************************/

static void Apply_operational_process_list () {
    char *opname;
    int i;

    opname = Ops_prcs_name;
    for (i = 0; i < N_ops_prcs; i++) {
	Mrpg_tat_entry_t ent;
	int ind, k, star_off;

	if (i > 0)
	    opname += strlen (opname) + 1;

	/* search for this process */
	if (opname[0] == ' ')
	    continue;
	star_off = strlen (opname) - 1;
	if (opname[star_off] != '*')
	    star_off = -1;
	else
	    opname[star_off] = '\0';
	ent.name = opname;
	if (!MISC_table_search (Prc_tbl, &ent, Search_cmp, &ind)) {
	    if (star_off < 0)
		LE_send_msg (LE_VL1, 
		"Operational process name %s not defined in TAT - ignored", 
							opname);
	    else {
		for (k = ind; k < N_prcs; k++) {
		    if (strncmp (Prcs[k].name, opname, star_off) != 0)
			break;
		    Prcs[k].type |= MRPG_TT_ENABLED;
		}
	    }	
	}
	else {
	    for (k = ind; k < N_prcs; k++) {
		if (strcmp (Prcs[k].name, opname) != 0)
		    break;
		Prcs[k].type |= MRPG_TT_ENABLED;
	    }
	}
	if (star_off >= 0)
	    opname[star_off] = '*';
    }
    STR_free (Ops_prcs_name);
    Ops_prcs_name = NULL;
    N_ops_prcs = 0;
}
