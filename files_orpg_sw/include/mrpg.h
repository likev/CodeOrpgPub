/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2012/12/03 20:25:24 $
* $Id: mrpg.h,v 1.48 2012/12/03 20:25:24 jing Exp $
* $Revision: 1.48 $
* $State: Exp $
*/

/***********************************************************************

    Description: global include file for mrpg (manage rpg).

***********************************************************************/


#ifndef MRPG_H
#define MRPG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include <infr.h>

/* mrpg command code */
enum {MRPG_STARTUP, MRPG_RESTART, MRPG_STANDBY, MRPG_SHUTDOWN, MRPG_NO_MANAGE,
	MRPG_MANAGE, MRPG_STATUS, MRPG_ENTER_TM, MRPG_EXIT_TM, MRPG_ACTIVE, 
	MRPG_INACTIVE, MRPG_POWERFAIL, MRPG_AUTO_START, MRPG_CLEANUP, 
	MRPG_STANDBY_RESTART, MRPG_INIT, MRPG_TASK_CONTROL, MRPG_REMOVE_DATA,
	MRPG_RESUME};

/* The following commands cannot be sent to the command queue: MRPG_INIT,
   MRPG_REMOVE_DATA, MRPG_RESUME and MRPG_CLEANUP */

/* values for process status used for Mrpg_process_table_t.status and others */
#define MRPG_PS_NOT_STARTED 1
#define MRPG_PS_STARTED     2
#define MRPG_PS_ACTIVE      3
#define MRPG_PS_FAILED      4

/* msg IDs in ORPGDAT_TASK_STATUS */
#define MRPG_PT_MSGID 4
#define MRPG_PS_MSGID 5
#define MRPG_RPG_STATE_MSGID 3	/* the RPG state of Mrpg_state_t */
#define MRPG_RPG_NODE_MSGID 2	/* the RPG node info (Mrpg_node_t) */
#define MRPG_RPG_DATA_MSGID 6	/* the RPG data info (Mrpg_data_t) */

typedef struct {		/* process table struct (short form) */
    int size;			/* SMI_vss_size this->size;
				   size of this item */
    int name_off;		/* task name offset */
    int instance;		/* instance number (negative - none) */
    int pid;
    int status;			/* process running status */
} Mrpg_process_table_t;

/* process table is stored in ORPGDAT_TASK_STATUS with msgid MRPG_PT_MSGID.
   The message is a list of Mrpg_process_table_t. All processes of status 
   MRPG_PS_ACTIVE and MRPG_PS_FAILED are listed. The message is updated when
   the info is changed. */

typedef struct {		/* process status struct (long form) */
    int size;			/* SMI_vss_size this->size;
				   size of this item */
    int name_off;		/* task name offset */
    int node_off;		/* node name offset */
    int cmd_off;		/* command line offset */
    int instance;		/* instance number (negative - none) */
    int pid;
    int cpu;			/* accumulated CPU usage in milli-seconds
				   (% 0x7fffffff) */
    int mem;			/* stack and heap memory usage in K (1024) 
				   bytes */
    int status;			/* process running status */
    int life;			/* process life time in seconds */
    time_t info_t;		/* time the info is retrieved */
} Mrpg_process_status_t;

/* process status is stored in ORPGDAT_TASK_STATUS with msgid MRPG_PS_MSGID.
   The message is a list of Mrpg_process_status_t. All processes of status 
   MRPG_PS_ACTIVE and MRPG_PS_FAILED are listed. The message is updated upon
   request (sending mrpg MRPG_STATUS). */

enum {MRPG_ST_SHUTDOWN, MRPG_ST_STANDBY, MRPG_ST_OPERATING, 
		MRPG_ST_TRANSITION, MRPG_ST_FAILED, MRPG_ST_POWERFAIL};
				/* for Mrpg_state_t.state */
enum {MRPG_TM_NONE, MRPG_TM_RPG, MRPG_TM_RDA};
				/* for Mrpg_state_t.test_mode */
enum {MRPG_ST_INACTIVE, MRPG_ST_ACTIVE};
				/* for Mrpg_state_t.active */

typedef struct {		/* RPG state struct */
    int state;			/* RPG state */
    int test_mode;		/* test mode */
    time_t st_time;		/* start time of the current state */
    time_t alive_time;		/* the last time this is updated */
    int cmd;			/* mrpg cmd that leads to the current state */
    int active;			/* in active or inactive state */
    short fail_count;		/* number of consecutive failures */
    short pf_state;		/* state when power failure occurs */
    time_t fail_time;		/* latest failure time */
    unsigned int check_sum;
} Mrpg_state_t;

/* RPG node info is stored in ORPGDAT_TASK_STATUS with msgid 
   MRPG_RPG_NODE_MSGID. The message is a list of Mrpg_node_t. */

typedef struct {		/* RPG node info struct */
    int size;			/* SMI_vss_size this->size;
				   size of this message */
    unsigned int ip;		/* host IP address */
    int host_off;		/* host name offset */
    int node_off;		/* node name offset */
    char is_local;		/* is the local host */
    char default_node;		/* is the default node */
    char is_bigendian;		/* the node is of big endian */
    char is_connected;		/* this node is currently connected */
    int orpgdir_off;		/* project dir name offset */
    int workdir_off;		/* working dir name offset */
    int logdir_off;		/* log dir name offset */
    int tasks_off;		/* offset of node task names. Tasks on default
				   node are not listed. 0 - no task. */
} Mrpg_node_t;

/* RPG data info is stored in ORPGDAT_TASK_STATUS with msgid 
   MRPG_RPG_DATA_MSGID. The message is a list of Mrpg_data_t. */

#define MRPG_WP_NAME_SIZE 32

typedef struct {		/* RPG data write permission */
    LB_id_t msg_id;		/* message ID */
    char name[MRPG_WP_NAME_SIZE];
				/* task name */
} Mrpg_wp_item;

typedef struct {		/* RPG node info struct */
    int size;			/* SMI_vss_size this->size;
				   size of this message */
    int data_id;		/* data ID */
    int compr_code;		/* compression code */
    int wp_size;		/* array size of wp */
    Mrpg_wp_item wp[1];		/* SMI_vss_field [this->wp_size];
				   variable sized array */
} Mrpg_data_t;

/* RPG state is stored in ORPGDAT_TASK_STATUS with msgid MRPG_RPG_STATE_MSGID.
   The message is Mrpg_state_t. Field life_time may not be updated. */

#define MRPG_CMD_ID 68390375	/* for Mrpg_cmd_t.id */

typedef struct {		/* mrpg commands struct */
    int id;			/* a magic number of protect RPG */
    int mrpg_cmd;		/* mrpg command code: MRPG_STARTUP ... */
    int sender;			/* send ID - not used for now. */
    int time;			/* command time */
} Mrpg_cmd_t;

/* for argument "channel" of ORPGMGR_setup_sys_cfg */
#define ORPGMGR_SAT_CONN 0x10
#define ORPGMGR_CHANNEL_MASK 0xf

int ORPGMGR_send_command (int cmd);
int ORPGMGR_cleanup ();
int ORPGMGR_send_msg_command (int cmd, char *msg);
int ORPGMGR_get_RPG_states (Mrpg_state_t *rpg_state);
int ORPGMGR_get_rpgcmd_in_progress (int *cmd);
int ORPGMGR_state_file_name (char *name, int size);
int ORPGMGR_is_mrpg_up ();
int ORPGMGR_wait_for_op_state(time_t wait_time); /* wait time in seconds */
void ORPGMGR_report_ready_for_operation ();
int ORPGMGR_setup_sys_cfg (int channel, int dont_update, void (*cb)());
int ORPGMGR_get_syscfg_version (char **version);
int ORPGMGR_read_sys_cfg (char **scfg);
int ORPGMGR_get_mrpg_host_name (char *mrpg_hname, int buf_size);
int ORPGMGR_search_mrpg_host_name (int channel, char *mrpg_hn, int buf_size);
void ORPGMGR_start_rpgdbm (char *db_name);
int ORPGMGR_get_task_status (char *task_name, int instance, int *state);
int ORPGMGR_discover_host_ip (char *node, int chan, char *ipb, int ipb_s);
int ORPGMGR_print_node_info ();
int ORPGMGR_each_node (int (*cb) (char *, char *, char *));
int ORPGMGR_check_connectivity (int n_ips, unsigned int *ips, int *stat);
int ORPGMGR_deregister_query_hosts ();
int ORPGMGR_was_os_crashed ();
int ORPGMGR_set_os_crash_fixed ();

#define ORPGMGR_MALLOC_FAILED -10001
#define ORPGMGR_EMPTY_VERSION_LINE -10002
#define ORPGMGR_NO_SYS_CFG_FOUND -10003
#define ORPGMGR_CREATE_FILE_FAILED -10004
#define ORPGMGR_MRPG_NAME_NOT_FOUND -10005
#define ORPGMGR_OPEN_FAILED -10006
#define ORPGMGR_LSEEK_FAILED -10007
#define ORPGMGR_READ_FAILED -10008
#define ORPGMGR_MRPG_NOT_FOUND -10009
#define ORPGMGR_BAD_ARGUMENT -10010
#define ORPGMGR_BAD_RSSD_HOST_INFO -10011
#define ORPGMGR_MKDIR_FAILED -10012
#define ORPGMGR_FATAL_ERROR -10013

#ifdef __cplusplus
}
#endif	
#endif		/* #ifndef MRPG_H */


