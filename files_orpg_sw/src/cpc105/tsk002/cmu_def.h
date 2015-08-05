
/***********************************************************************

    Description: Internal include file for comm_manager (UCONX/MPS300 
	version).

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2013/01/03 21:34:51 $
 * $Id: cmu_def.h,v 1.19 2013/01/03 21:34:51 jing Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */  

#ifndef CMU_DEF_H

#define CMU_DEF_H

#include <comm_manager.h>
#include <cmc_common.h>
#include <infr.h>

#include <xstopts.h>

struct link_struct {		/* link configuration */
    char link_ind;		/* link index */
    char device;		/* device number */
    char port;			/* port number */
    char link_type;		/* link type DEDICATED, DIAL_IN
				   or DIAL_IN_OUT */
    char proto;			/* PROTO_PVC or PROTO_HDLC */
    char n_pvc;			/* number of PVCs */
    char link_state;		/* link connection state */
    char data_en;		/* incoming data event notification: 
				   non-zero - yes, zero - no */

    char conn_activity;		/* current connection activity: CONNECTING,
				   DISCONNECTING or NO_ACTIVITY; There can 
				   be only one connect/disconnect request being
				   processed at any time */
    char dial_activity;		/* valid for dialout lines only. current 
				   connection activity: CONNECTING,
				   DISCONNECTING ,FAIL_DISCONNECTING, or
				   NO_ACTIVITY; There can be only one 
				   connect/disconnect request being 
				   processed at any time */
    char dial_state;		/* Dialout procedure state NORMAL or X25 or 
				   HDLC. Default is NORMAL  */ 
    char conn_req_ind;		/* req index in "req" of the current 
				   connect/disconnect request being processed */

    char enable_state;		/* link enable state (for PVC) */
    char lapb_enable_state;	/* lapb enable state (for PVC) */
    char dlpi_state;		/* DLPI state as received from MPS (for PVC) */
    char conn_wait_state;	/* connection wait state (for PVC) */
    char clean_tdx;		/* cleaning up TXD state is needed (for PVC) */
    char connect_state;		/* link connect state (for HDLC) */
    char wait_before_discon_state;
				/* in the state of waiting for all packets to
				   be transmitted before disconnecting (for 
				   PVC) */
    unsigned char retry_enable_cnt;
				/* max number of enable retry times before
				   disabling a link */

    time_t conn_wait_st_time;	/* start time of current conn_wait state (for 
				   PVC) */
    time_t w_ack_time;		/* latest msg ACK time (for PVC) */
    time_t disable_st_time;	/* time starting to disable WAN */

    int line_rate;		/* baud rate: e.g. 28000 */
    int packet_size;		/* maximum packet size */
    int n_added_bytes;		/* number of added bytes in front of 
				   incoming/outgoing messages */

    time_t rep_time;		/* previous statistics report time */
    time_t dial_wait_time;	/* dial connection statue check time */
    int rep_period;		/* statistics report period in seconds; 0
				   indicates that the report is turned off */

    int rw_time;		/* read/write expiration time; 0 means 
				   infinity */

    int reqfd;			/* LB file descriptor for requests; negative
				   value means sharing with another link of
				   -reqfd. (We assume that LB fd is always
				   positive) */
    int must_read;		/* reqfd is a must_read LB */
    int respfd;			/* LB file descriptor for responses */

    int lapb_fd;		/* for LAPB level statistics */
    int client_id[MAX_N_STATIONS];
				/* client ID returned from MPSopen */
    char bind_state[MAX_N_STATIONS];
				/* PVC bind state */
    char attach_state[MAX_N_STATIONS];
				/* PVC attach state */
    void *oreq;			/* MPSopen request data struct, HDLC only */
    void *p_bind;		/* link bind request data struct, HDLC only */
    char *wan_conf;		/* WAN config file name, X25 only */
    char *lapb_conf;		/* LAPB config file name, X25 only */
    char *x25_conf;		/* X25 config file name, X25 only */
    char snid[4];		/* subnet ID, X25 only */

    char *r_buf[MAX_N_STATIONS];/* buffer for incoming data; NULL means not 
				   used */
    int r_cnt[MAX_N_STATIONS];	/* bytes read */
    int r_buf_size[MAX_N_STATIONS];
				/* read buffer size */
    unsigned int r_seq_num;	/* user data sequence number */
    time_t read_start_time[MAX_N_STATIONS];
				/* read start time */

    char *w_buf[MAX_N_STATIONS];/* buffer for outgoing data; NULL means not 
				   used */
    int w_size[MAX_N_STATIONS];	/* outgoing message sizes */
    int w_cnt[MAX_N_STATIONS];	/* bytes written */
    int w_ack[MAX_N_STATIONS];	/* bytes acknowledged */
    char w_req_ind[MAX_N_STATIONS];
				/* the request index in "req" array */
    char w_time_out[MAX_N_STATIONS];
				/* write timed-out event sent */
    char w_blocked[MAX_N_STATIONS];
				/* write has been blocked */

    char n_reqs;		/* number of pending requests to be processed */
    char st_ind;		/* index of the first request in "req" */
    unsigned int sequence[MAX_N_STATIONS];
				/* write message sequence number */
    Req_struct req[MAX_N_REQS];
				/* stores unfinished and pending requests */

    char compress_method;	/* compression method. -1 no compression. */
    char w_compressed;		/* message to write is compressed */
    unsigned short max_pack;	/* maximum # of msgs in each pack */
    int pack_cnt;		/* # msgs to seek back for unprocessed msgs */
    int pack_time;		/* pack time in ms. 0 - no packing */
    int n_saved_bytes;		/* number of bytes in saved_msg */
    char *saved_msgs;		/* point to saved msgs for packing */
    char *pack_buf;		/* buffer for packing. STR type. */
    double pack_st_time;	/* time of first message to pack in s */
    double comp_bytes;		/* number of bytes compressed */
    double org_bytes;		/* number of bytes before compression */
};
typedef struct link_struct Link_struct;

enum {CCC_SEND_RESTART, CCC_ENABLE_TDX, CCC_DISABLE_TDX};
				/* for argument "func" of function 
				   CCC_control_restart */

enum {				/* values for conn_wait_state */
    CONN_WAIT_INACTIVE, CONN_WAIT_FOR_ENABLE, CONN_WAIT_FOR_RESTART,
    CONN_WAIT_FOR_TDX_ON, CONN_WAIT_REST_CONFIRM
};


/* private global functions */
void CM_terminate ();
int CM_boot_time ();
int CM_block_signals (int block);
int CM_max_reenable();

int HA_initialize (int n_links, Link_struct **links,
		struct xstrbuf *control_buf, struct xstrbuf *data_buf);
int HA_config_protocol ();
int HA_process_packets (Link_struct *link);
void HA_clean_up ();

int XP_initialize (int n_links, Link_struct **links,
		struct xstrbuf *control_buf, struct xstrbuf *data_buf);
void XP_set_restart_wait_time (int t);
void XP_clean_up ();
int XP_process_packets (Link_struct *link, int pvc);
int XP_config_protocol ();
void XP_house_keeping ();
void XP_process_DLPI_status (int dlpifd);

int SH_initialize (int n_links, Link_struct **links, int cmsv_addr);
int SH_poll ();
void SH_shared_bufs (struct xstrbuf **control, struct xstrbuf **data);
void SH_house_keeping (Link_struct *link, time_t cr_time);
void SH_process_fatal_error ();
int SH_config_OK ();
void SH_set_dlpi_fd (int dlpifd);

int CC_read_link_config (Link_struct **links, 
				int *device_number, int *cmsv_addr);
char *CC_get_server_name ();
char *CC_get_X25_service_name ();
char *CC_uconx_conf_file ();
char *CC_get_reboot_command ();

int RATE_get_rate (Link_struct *link);

int CCC_x25_config (int n_links, Link_struct **links);
int CCC_wanCommand (char *pSnid, unsigned int command);
int CCC_lapbCommand (char *pSnid, unsigned int command);
int CCC_remove_labels (int device);
int CCC_control_restart (int func, char *pSnid);

#endif		/* #ifndef CMU_DEF_H */
