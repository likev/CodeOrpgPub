
/***********************************************************************

    Description: Internal include file for mrpg (manage rpg).

***********************************************************************/

/*
* RCS info
* $Author: steves $
* $Locker:  $
* $Date: 2015/01/05 20:35:17 $
* $Id: mrpg_def.h,v 1.79 2015/01/05 20:35:17 steves Exp $
* $Revision: 1.79 $
* $State: Exp $
*/

#ifndef MRPG_DEF_H
#define MRPG_DEF_H

#include <mrpg.h>
#include <infr.h>


#define MRPG_NAME_SIZE 128

#define MAX_PROCESS_TERM_TIME 20


typedef struct {
    int data_id ;		/* negative - not used */
    char *path;			/* data name and subdir */
    LB_attr attr ;		/* LB attributes for this datastore */
    char persistent;		/* persistence flag */
    char mrpg_init;		/* need to be created for mrpg init */
    char no_create;		/* no creation flag */
    char is_extension;		/* this is read from extension table */
    int compr_code;		/* compression code */
    int wp_size;		/* array size of wp */
    Mrpg_wp_item *wp;		/* variable sized array */
} Mrpg_dat_entry_t ;

/* bit flags for Mrpg_tat_entry_t.type - task type */
#define MRPG_TT_START_ONCE	0x1
#define MRPG_TT_RESPAWN		0x2
#define MRPG_TT_ENABLED		0x4
#define MRPG_TT_MONITOR_ONLY	0x8
#define MRPG_TT_CONTROL_TASK	0x10	/* RPG control task */
#define MRPG_TT_PRODUCT_SERVER	0x20	/* product server task */
#define MRPG_TT_COMM_MANAGER	0x40	/* comm_manager task */
#define MRPG_TT_ALLOW_DUPLICATE	0x80	/* task is allowed to fork duplicate 
					   processes */
#define MRPG_TT_NO_LAUNCH	0x100   /* not lauched by mrpg */
#define MRPG_TT_DISABLED_CFG    0x200   /* disabled owing to configuration. */
#define MRPG_TT_ALIVE_IN_STANDBY    0x400
					/* keep alive in standby state */
#define MRPG_TT_ACTIVE_CHANNEL_ONLY	0x800
					/* run this for active channel only */
#define MRPG_TT_MULTIPLE_INVOKE	0x1000	/* multiple invokations may be used */
#define MRPG_TT_REPORT_READY	0x2000	/* the task must report ready event */
#define MRPG_TT_STOPPED		0x4000	/* the task is commanded to be 
					   stopped */
#define MRPG_TT_SUPPORT_TASK	0x8000	/* task added as support task */
#define MRPG_TT_ALL_STATES	0x10000	/* task running in all states */

#define MRPG_TT_ALL_TYPES	0xffffffff


/* for Node_attribute_t.connect_state */
enum {NODE_ACCESS_UNKNOWN, NODE_ACCESS_OK, NODE_ACCESS_LOST};

typedef struct {		/* computing node attributes */
    char *node;			/* node name */
    char *hname;		/* host name */
    unsigned int ip;		/* IP address */
    int n_rass;			/* number of resource allocation sections */
    char **rass;		/* Resource allocation section names */
    char is_connected;		/* connectivity state */
    char is_local;		/* is the local host */
    char default_node;		/* node for default resource location */
    char nds_ok;		/* nds is running OK */
    char is_bigendian;		/* node endianness */
    char not_used;		/* the node is not used in the config */
    char index;			/* a small number unique for each node */
    char prev_connected;	/* previous connectivity state */
    int n_data_ids;		/* number of data stores */
    int *data_ids;		/* data stores for the node */
    int n_prod_ids;		/* number of product stores */
    int *prod_ids;		/* product stores for this node */
    int n_tasks;		/* number of tasks */
    char **tasks;		/* tasks for this node */
    int n_processes;		/* number of processes */
    char **processes;		/* processes for this node */
    int nds_fd;			/* nds LB fd */
    int nds_pid;		/* nds pid */
    short proc_table_updated;	/* process table updated flag */
    short proc_status_updated;	/* process status updated flag */
    char *nds_msg;		/* buffer for holding nds message */
    char *orpgdir;		/* project dir */
    char *workdir;		/* working dir */
    char *logdir;		/* log dir */
} Node_attribute_t;

/* bits for Mrpg_tat_entry_t.misc */
#define MRPG_MISC_FAIL_REPORTED 0x1	
				/* process failure/recovery reported */
#define MRPG_MISC_WAIT_FOR_READY 0x2	
				/* waiting for process's ready event */
#define MRPG_MISC_SHARE_CMD 0x4	
				/* the task shares command with others */

struct tat_entry {		/* TAT entry */
    int type;			/* type bit flags */
    short cmd_index;		/* sorted index by cmd_name */
    short first_cmd_index;	/* index of the first entry of the same cmd */
    char *name;			/* task name */
    char *cmd_name;		/* command name (never free this) */
    Node_attribute_t *node;	/* node the task runs on */
    char *cmd;			/* command line (never free this) */
    char *acmd;			/* actual cmd as read from OS. Valid when
				   pid > 0. (never free this) */
    char *site_str;		/* string describing site specific info */
    short mem_limit;		/* memory limit */
    short cpu_limit;		/* CPU limit */
    int instance;		/* instance number (negative - none) */
    int pid;
    int cpu;
    int mem;
    int swap;
    time_t info_t;		/* nds sys time the ps info retrieved */
    short status;		/* process running status: MRPG_PS_* */
    short misc;			/* miscellaneous control bits */
    int life;			/* process life time in seconds */
    time_t st_time;		/* process start or failure sys time. It is set
				   to sys time when status changes to 
				   MRPG_PS_STARTED, MRPG_PS_FAILED. Is is set
				   based on the process info if available */
    short n_dupps;		/* number of duplicated instances */
    short is_extension;		/* this is read from an extended task table */
    struct tat_entry *next;	/* next duplicated instances */
    struct tat_entry *next_node;/* next node for this task */
};

typedef struct tat_entry Mrpg_tat_entry_t;

/* bit flags for Mrpg_cmd_list_t.type */
#define MRPG_STOP_ON_ERROR	0x1

typedef struct {		/* init command entry */
    int type;			/* type bit flags */
    char *name;			/* process name */
    char *cmd;			/* command line (never free this) */
} Mrpg_cmd_list_t;

typedef struct {		/* comms config info */
    int link;			/* link number */
    int user;			/* comms link user number */
    int cm;			/* comm_manager number */
    char cm_mgr[MRPG_NAME_SIZE];
				/* comm_manager name */
} Mrpg_comms_link_t;

#define UNEXP_CMD_SIZE 256

typedef struct {		/* unexpected command */
    int pid;			/* pid */
    char cmd[UNEXP_CMD_SIZE];	/* command line */
    Node_attribute_t *node;	/* node on which the command runs */
    time_t t;
} Unexp_process_t;

#define MRPG_STATE_MSGID 1	/* msg ID of Mrpg_state_t as stored in the
				   RPG state LB */
#define MRPG_STATE_RES_NAME 2	/* msg ID of resource name as stored in the
				   RPG state LB */

#define MRPG_LOCKED 1		/* an return value of MPC_init */


time_t MAIN_start_time ();
void MAIN_disable_report (int yes);
int MAIN_wait_time ();
char *MAIN_init_section_name ();
int MAIN_is_operational ();
int MAIN_clean_up_processes (int all, int q_cmd);
int MAIN_command ();
int MAIN_switch_to_deau_db ();
void MAIN_exit (int status);

int MRD_init ();
int MRD_read_comms_config ();
int MRD_get_DAT (Mrpg_dat_entry_t **datp);
int MRD_get_PAT (Mrpg_dat_entry_t **patp);
int MRD_get_CMT (Mrpg_comms_link_t **cmsp);
int MRD_get_RDA_link ();
int MRD_get_next_file_name (char *dir_name, char *basename, 
					char *buf, int buf_size);
int MRD_get_table_file_name (char *name, char *dir, char *basename, 
					char *tmp, int size);
int MRD_publish_data_info ();

int MGC_init ();
int MGC_gen_system_config ();
int MGC_system (char *hname, char *cmd, char **output);
int MGC_complete_sys_cfg ();

int MCD_init ();
int MCD_create_ds (int sw);
int MCD_create_dir (char *name);
void MCD_recreate_lbs ();
void MCD_recreate_all_lbs (int yes);
int MCD_is_lb_created ();
void MCD_verify_permanent_file (char *fname);

int MRT_init ();
int MRT_get_ops (Mrpg_tat_entry_t **prcsp);
int MRT_get_startup_cmds (Mrpg_cmd_list_t **startup_cmdsp);
int MRT_get_restart_cmds (Mrpg_cmd_list_t **restart_cmdsp);
char *MRT_get_last_token (char *str, int *len);
char *MRT_get_first_token (char *str, int *len);
int MRT_get_shutdown_cmds (Mrpg_cmd_list_t **shutdown_cmdsp);
int MRT_get_init_cmds (Mrpg_cmd_list_t **init_cmdsp);
int MRT_process_install_cmds ();
void MRT_get_t_option (char *cmd, char *buf, int buf_size);
Mrpg_tat_entry_t *MRT_get_next_task (int first);
int MRT_set_site_processes ();
int MRT_get_perm_file_names (char **pf_names);

int MPC_init ();
int MPC_process_command (int command);
int MPC_process_queued_cmds ();
void MPC_mrpg_cmds_lb_removed ();
int MPC_execute_op_process (Mrpg_tat_entry_t *pr);
void MPC_shutdown_done (Mrpg_state_t *rpg_state, int n_processes_to_kill);
int MPC_execute_commands (int n_cmds, Mrpg_cmd_list_t *cmds);
int MPC_open_mrpg_cmds_lb ();
void MPC_set_os_crash_start (int set);

int MPI_init ();
int MPI_get_process_info ();
void MPI_housekeep ();
void MPI_clear_task_failure_alarm ();
int MPI_get_unexpected_processes (Unexp_process_t **up_p);

int MMR_open_state (int wait);
int MMR_init ();
char *MMR_state_file_name ();
int MMR_create_state_file ();
int MMR_get_rpg_state (Mrpg_state_t **Rpg_state);
void MMR_housekeep ();
void MMR_set_manage_off_flag (int manage_off);
int MMR_is_rpg_state_new ();
int MMR_set_rpg_state (int new_state, int new_cmd);
void MMR_set_active (int active);
int MMR_opp_count ();
void MMR_set_limits (int cpu_limit, int mem_limit, int cpu_window);
void MMR_set_restart_bit (int set);
int MMR_in_standby_state ();
void MMR_set_save_log_cmd (char *cmd);
void MMR_close_state_file ();
int MMR_add_cron_job (int hour, int minute, char *cmd);
int MMR_read_resource_name (char *buf, int buf_size);
int MMR_save_resource_name ();
int MMR_is_faa_redundant ();
void MMR_nds_started ();

int MHR_init (char *res_name, int *missing_nodep);
int MHR_get_data_hosts (int data, Node_attribute_t ***node);
int MHR_all_hosts (Node_attribute_t **nodes);
int MHR_all_nodes (Node_attribute_t **nodes);
int MHR_get_task_hosts (char *task, Node_attribute_t ***node);
int MHR_is_distributed ();
int MHR_data_rep_spec (char **spec);
Node_attribute_t *MHR_get_node_by_name (char *node_name);
int MHR_is_local_node (char *host_name);
int MHR_is_mrpg_node ();
int MHR_publish_node_info ();
int MHR_get_mrpg_node_name (char *buf, int buf_size);
int MHR_get_local_node_name (char *buf, int buf_size);
void MHR_get_resource_name (char *buf, int buf_size);
int MHR_get_channel_number ();
void MHR_check_node_connectivity ();
int MHR_id_node_connectivity_OK ();
int MHR_register_lost_en_conn_event ();


#endif		/* #ifndef MRPG_DEF_H */
