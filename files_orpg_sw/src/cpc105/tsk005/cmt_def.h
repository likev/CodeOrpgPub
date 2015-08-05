
/***********************************************************************

    Description: Internal include file for comm_manager (TCP 
	version).

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2013/01/02 17:06:40 $
 * $Id: cmt_def.h,v 1.31 2013/01/02 17:06:40 jing Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 *
 * History:
 *
 * 11JUN2003 - Chris Gilbert - CCR #NA03-06201 Issue 2-150. Add "faaclient"
 *             support going through a proxy firewall.
 *
 *  02Feb2002 - C. Gilbert - CCR #NA01-34801 Issue 1-886 Part 1. Add FAA support to 
 *                           the client cm_tcp.
 *
 * 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out. 
 * 20MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Fix some minor problems
 *                           found in unit testing. 
 *
 * 06FEB2003 Chris Gilbert - CCR NA03-03502 Issue 2-129 - Add connection_procedure_
 *           time_limit to increase amount of time to go through a firewall.
 *
 * 06FEB2003 Chris Gilbert - CCR NA03-02901 Issue 2-130 - Add no_keepalive_
 *           response_disconnect_time in order to disconect in a timely matter
 *           in case of link breaks.
 *
 */  

#ifndef CMT_DEF_H

#define CMT_DEF_H

#include <comm_manager.h>
#include <cmc_common.h>
#include <netinet/in.h>		/* for IPPROTO_TCP value */
#include <infr.h>

enum {CMT_CLIENT, CMT_SERVER, CMT_FAACLIENT, CMT_DYNADIAL}; /* values for Link_struct.server field */
enum {CMT_LAN, CMT_PPP};	/* values for Link_struct.network field */
enum {DO_IDLE, DO_DIAL, DO_CONN}; /* values for Link_struct.tcp_dial_state */

#define NOT_READY	-2	/* values for Link_struct->ready_pvc field
				   - none of the sockets is data ready */
#define SERVER_READY	-1	/* values for Link_struct->ready_pvc field
				   - server_fd is data ready */
#define MIN_NO_INPUT_TIME 60    /* min amount of time before a timeout can occur (in seconds) */

#define N_RED_SERVERS 2		/* max number of redundant servers */

/*
* TCP Dialout definitions
*/
#define MAX_N_PHONERECS  500    
#define UDP_DIALOUT_PORT 5689
#define ATTEMPTLIMIT     40     /* count to wait for offhook, approx 25 seconds */
/* Cisco MIB return codes */
#define ONHOOK 2
#define OFFHOOK 3
#define CONNECTED 4

struct phone_struct {		/* dynamic dial configurations            */
    char *phone_num;            /* telephone number                       */
    int server;			/* port type: CMT_CLIENT or CMT_FAACLIENT */
    int  port_num;              /* logical port number                    */
    char *server_name;          /* primary server name                    */
    char *ch2_name;             /* secondary (channel 2) server name      */
    char *password;             /* line password                          */
    int  override_rate;         /* optional overide rate                  */
    int line_rate;		/* baud rate: e.g. 28000 */
    char *dialout_name;         /* name/IP that will start Cisco DDR dialout */
} phone_struct;
typedef struct phone_struct Phone_struct;

struct link_struct {		/* link configuration */
    char link_ind;		/* link index */
    char device;		/* device number; not used */
    char port;			/* port number; not used */
    char link_type;		/* link type DEDICATED, DIAL_IN
				   or DIAL_IN_OUT */
    char proto;			/* PROTO_PVC or PROTO_HDLC; not used */
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
    char network;		/* CMT_LAN or CMT_ppp */

    int line_rate;		/* baud rate: e.g. 28000 */
    int packet_size;		/* maximum packet size; not used */
    int n_added_bytes;		/* number of added bytes in front of 
				   incoming/outgoing messages */

    int server;			/* port type: CMT_SERVER or CMT_CLIENT */
    int dynamic_dial;           /* flag value: 1 = line uses dynamic config */
    int tcp_dial_state;		/* state: DO_IDLE, DO_DIAL, or DO_CONN */
    int tcp_dial_cnt;		/* wait counter before we flag and error */
    int port_number;		/* current TCP port number */
    int conf_port_number;	/* TCP port number from tcp.conf */
    int user_spec_ip;		/* user specified IP/port in CM_CONNECT is used
				   */
    char *server_name;		/* server host name (or the address of format
				   128.23.22.11); Can be "INADDR_ANY" for 
				   server meaning all network interfaces on 
				   the server. */
    char *ch2_name;             /* name for channel two for FAA client type */
    int  faa_init_flag;         /* flag for FAA channel two init */
    char *dialout_name;         /* name/IP that will start Cisco DDR dialout */
    char *password;		/* password for authentication */
    time_t last_conn_time;	/* time starting to connect - client only */
    char *conf_rs_name[N_RED_SERVERS];	/* configured redundant server names */

    /* for testing conn to redundant servers - CMT_FAACLIENT client only */
    int tfd[N_RED_SERVERS];	/* fds */
    char *rs_name[N_RED_SERVERS];	/* names of redundant servers */
    void *rs_addrs[N_RED_SERVERS];	/* addresses of redundant servers */
    int n_rss;				/* number of redundant servers */

    struct snmp_str {           /* SNMP structure */
       int  snmp_index;
       char *snmp_type;
       char *snmp_host;
       char *snmp_community;
       char *snmp_interface; 
       struct enable {
          char *oid_cmd;
          char *oid_type;
          char *oid_value;
       } enable;
       struct disable {
          char *oid_cmd;
          char *oid_type;
          char *oid_value;
       } disable;
       struct drop_dtr {
          char *oid_cmd;
          char *oid_type;
          char *oid_value;
       } drop_dtr;
       struct failure_cnt {
          /* read-only type=integer*/
          char *oid_cmd;
          int   oid_value;
       } failure_cnt;
       struct conn_cnt {
          /* read-only type=integer*/
          char *oid_cmd;
          int   oid_value;
       } conn_cnt;
       struct discon_code {
          /* read-only type=integer*/
          char *oid_cmd;
          int   oid_value;
       } discon_code;
       struct modem_state {
          /* read-only type=integer */
          char *oid_cmd;
          char *oid_value;
       } modem_state;
       struct modem_bsy_set {
          char *oid_cmd;
          char *oid_type;
          char *oid_value;
       } modem_bsy_set;
       struct modem_bsy_clr {
          char *oid_cmd;
          char *oid_type;
          char *oid_value;
       } modem_bsy_clr;
    } snmp_str;

    unsigned int client_ip;	/* client address used for verification */

    int server_fd;		/* server parent socket fd */
    int ready_pvc;		/* data ready pvc. SERVER_READY for data ready 
				   on server_fd. NOT_READY for none. */
    void *address;		/* remote site internet address. We only set 
				   this once for each link. We don't reset 
				   after disconnect. */
    void *ch2_address;          /* channel 2 address for faa systems */
    time_t rep_time;		/* previous statistics report time */
    time_t dial_wait_time;	/* dial connection status check time */
    time_t connect_st_time;	/* start time of connection procedure */
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

    int pvc_fd[MAX_N_STATIONS];
				/* socket fds for all PVCs */
    int ch2_fd[MAX_N_STATIONS];
				/* socket fds for ch2 PVCs */
    struct link_struct *ch2_link;  /* ch2 struct to connect to a FAA RPG*/
    int    ch_num;
    struct link_struct *ch1_link;  /* ch2 struct to connect to a FAA RPG*/

    char login_state[MAX_N_STATIONS];
				/* PVC login state */
    char connect_state[MAX_N_STATIONS];
				/* PVC link connect state */
    int log_cnt[MAX_N_STATIONS];/* log count down for log control */

    char *r_buf[MAX_N_STATIONS];/* buffer for incoming data; NULL means not 
				   used */
    int r_cnt[MAX_N_STATIONS];	/* bytes read */
    int r_msg_size[MAX_N_STATIONS];
				/* the current message size */
    int r_buf_size[MAX_N_STATIONS];
				/* read buffer size */
    unsigned int r_seq_num;	/* user data sequence number */
    time_t read_start_time[MAX_N_STATIONS];
				/* read start time */
    short r_hd_cnt[MAX_N_STATIONS];
				/* number of header bytes read */
    short w_hd_cnt[MAX_N_STATIONS];
				/* number of header bytes to write */
    char *r_tcp_hd[MAX_N_STATIONS];
				/* tcp header read buffer */
    char *w_tcp_hd[MAX_N_STATIONS];
				/* tcp header write buffer */

    char *w_buf[MAX_N_STATIONS];/* buffer for outgoing data; NULL means not 
				   used */
    int w_size[MAX_N_STATIONS];	/* outgoing message sizes */
    int w_cnt[MAX_N_STATIONS];	/* total bytes written including TCP header */
    int w_ack[MAX_N_STATIONS];	/* bytes acknowledged excluding TCP header*/
    char w_req_ind[MAX_N_STATIONS];
				/* the request index in "req" array */
    char w_time_out[MAX_N_STATIONS];
				/* write timed-out event sent */
    char w_started[MAX_N_STATIONS];
				/* The current user msg write started */

    time_t alive_time[MAX_N_STATIONS];
				/* time of the latest message received */
    unsigned int sent_seq_num[MAX_N_STATIONS];
				/* sequence number of the message sent */
    unsigned char w_blocked[MAX_N_STATIONS];
				/* flag: the socket has write buffer full */
    char read_ready[MAX_N_STATIONS];
				/* flag: the socket is ready for read */

    char n_reqs;		/* number of pending requests to be processed */
    char st_ind;		/* index of the first request in "req" */
    char phone_no[32];		/* phone no for dialout request */
    Phone_struct **phone_nums;  /* pointer the phone list of all possible sites */
    int  n_phone;               /* number of phone list records */
    Req_struct req[MAX_N_REQS];	/* stores unfinished and pending requests */

    int n_bytes_in_window;
    time_t last_update_time;
    int tnb_sent;		/* total number of byte sent */
    int tnb_received;		/* total number of byte received */

    char compress_method;	/* compression method. -1 no compression. */
    char w_compressed;		/* message to write is compressed */
    unsigned short max_pack;	/* maximum # of msgs in each pack */
    int pack_cnt;		/* # msgs saved for packing */
    int pack_time;		/* pack time in ms. 0 - no packing */
    int n_saved_bytes;		/* number of bytes in saved_msg */
    char *saved_msgs;		/* point to saved msgs for packing */
    char *pack_buf;		/* buffer for packing. STR type. */
    double pack_st_time;	/* time of first message to pack in s */
    double comp_bytes;		/* number of bytes compressed */
    double org_bytes;		/* number of bytes before compression */

} link_struct;
typedef struct link_struct Link_struct;

#define FLOW_CNTL_WINDOW 4

/* message ID used by cm_tcp messages */
#define TCM_ID 	20983610
/* message types (Tcp_msg_header.type) */
enum {TCM_LOGIN, TCM_LOGIN_ACK, TCM_DATA, TCM_DATA_ACK, TCM_KEEP_ALIVE};
#define TCM_MSG_BUF_SIZE 128	/* max message size, except TCM_DATA. */

typedef struct {		/* TCP message header */
    int type;			/* message type */
    int param;			/* type dependent parameter */
    int length;			/* message length excluding this header */
} Tcp_msg_header;

/* If thr first bit of Tcp_msg_header.length is set, the incoming data is 
   compressed. */

#define TCM_LENGTH_MASK 0x7fffffff
#define TCM_COMPRESS_FLAG 0x80000000

/* return values of SH_read_message (), LOGIN_receive_ack () and 
   LOGIN_accept_login () */
#define TCM_MSG_INCOMPLETE -1
#define TCM_BAD_MSG -2

#define CONNECTION_PROCEDURE_TIME_LIMIT 30
			/* time limit for completing the connect procedure */

/* return values from SOCK_connect () and SOCK_accept_client () */
#define SOCK_NOT_CONNECTED -1
#define SOCK_CONNECT_ERROR -2

#define CLIENT_ADDR_UNDEFINED 0xffffffff


/* private global functions */
void CM_terminate ();
int CM_compress_method ();
int CM_pack_ms ();
int CM_max_pack ();

int TCP_initialize (int n_links, Link_struct **links);
void TCP_client_connect (Link_struct *link);
void TCP_process_input (Link_struct *link, int fd);
void TCP_write_data (struct link_struct *link, int pvc);
void TCP_house_keeping (Link_struct *link, time_t cr_time);
void TCP_set_share_bandwidth ();
void TCP_process_flow_control ();
int TCP_flow_cntl_len (Link_struct *link, int len);

int SH_initialize (int n_links, Link_struct **links);
int SH_poll (int ms);
int SH_read_message (Link_struct *link, int pvc);
int SH_is_test_fd (Link_struct *link, int fd);

int CC_read_link_config (Link_struct **links);

int SOCK_open_server (Link_struct *link) ;
int SOCK_accept_client (Link_struct *link, unsigned int *cadd);
int SOCK_open_client (void **address, char *server_name, int port);
int SOCK_connect (int *fd, void *address);
int SOCK_read (Link_struct *link, int fd, char *buf, int buf_size);
int SOCK_write (Link_struct *link, int pvc, char *buf, int n_bytes);
int SOCK_init ();
int SOCK_set_address (struct sockaddr_in *addr, char *host);

int LOGIN_accept_login (Link_struct *link, int pvc);
int LOGIN_send_server_ack (Link_struct *link);
int LOGIN_send_login (Link_struct *link, int pvc);
int LOGIN_receive_ack (Link_struct *link);

int DO_search_dialout_table (Link_struct *link);
void DO_dialout_procedure (Link_struct *link);
int DO_init (Link_struct *link);
int DO_chk (Link_struct *link);

int SNMP_set (char *host_name, char *community_name, char *object_id,
              char *datatype, char *value);

char *SNMP_get (char *host_name, char *community_name, char *object_id);

int SNMP_find_index (char *interface, char *snmp_host, 
                     char *community, int curval);

int SNMP_disable (Link_struct *link); 
int SNMP_enable (Link_struct *link); 

void CMPR_set_additional_processing (void (*ap) (Link_struct *link));
	/* This should be in cmc_common.h. I put it here for convenience - It 
	   will be removed later */

#endif		/* #ifndef CMT_DEF_H */
