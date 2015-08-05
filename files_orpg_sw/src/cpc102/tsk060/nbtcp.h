/********************************************************************************
 * Internal include file for nbtcp (narrowband TCP ORPG client)
 * Configuration changes made be made to arguments section
 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/01/27 22:49:05 $
 * $Id: nbtcp.h,v 1.18 2014/01/27 22:49:05 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */


#include <sys/types.h>

#include <comm_manager.h>
#include <prod_user_msg.h>
#include <legacy_prod.h>
#include <orpgdat.h>
#include <smi.h>

 
#ifndef	NBTCP_H
#define	NBTCP_H
	
#define GSM_CODE		   2
#define CNAME_SIZE               128
#define TRUE                       1
#define FALSE                      0
#define STATS_INTERVAL            10
#define MAXLEN                  4096
#define MAX_NAME_LEN             256 
#define MSG_HDR_LEN               18
#define MSG                      128
#define TCP_HDR_LEN               12
#define TCP_ID              20983610
#define MAXRPS                  2000
#define CLASS_1                    1
#define CLASS_2                    2
#define USERPASS_LEN              12
#define PORTPASS_LEN               8

#define PROD_ID_LEN                4
#define PROD_CODE_LEN              5
#define MNE_LEN                    5
#define STRNG_LEN                 51
#define NUMBER_PROD_PARMS          6
#define PROD_PARM_LEN              8
#define MIN_FINAL_PROD_CODE       16
#define MAX_FINAL_PROD_CODE      ORPGDAT_BASE  /* the max product code possible is ORPGDAT_BASE - 1 */
#define PARAM_UNUSED_STRING      "UNU"

   /* Default Site ID (ICAO) */

#define ORPGNAME "KCRI\0"

   /* Default ORPG TCP port # */

#define ORPGPORT 	4491
#define ORPGPORT_C2 	4505

   /* First PVC # - fixed to 0 by ORPG */

#define PVC0            0
#define PVC1            1

   /* Default ORPG TCP port password - must agree with ORPG password 
      for the user line connecting to */

#define PASSWORD	    "passwd"
#define REQUEST_MINS    6

   /* Compression methods supported - ICD defined */

#define GZIP_COMPRESSION    0
#define BZIP2_COMPRESSION   1

enum {CM_TCP_INTERFACE, NATIVE_INTERFACE};

enum {LOGIN, LOGIN_ACK, DATA, DATAACK, KEEPALIVE};


 
typedef struct {	/*TCP message header */
 	int	type;	      /* message type; 
                     0 = client login message
                     1 = login ack
                     2 = user message, data
                     3 = message ack
                     4 = keep alive */
 	int	id;         /* message type dependent */
 	int length;     /* message length, excluding this header */
} tcp_header;

typedef struct {      /* Product Request message header */
	short	msgcode;            /* message code */
	short	date_of_msg;        /* date */
	short	time_of_msg_msw;    /* time (MSW) */
	short	time_of_msg_lsw;    /* time (LSW) */
	short	length_of_msg_msw;  /* msg length (MSW) */
	short	length_of_msg_lsw;  /* msg length (LSW) */
	short	src_id;             /* NEXRAD id of source */
	short	dst_id;             /* NEXRAD id of dest. */
	short	no_of_blocks;       /* number of blocks in msg */
} msg_header;

typedef struct {         /* product req. block -- RPS list */
	short	divider;               /* block divider = -1 */
	short	length_of_blk;         /* block length = 020 */
	short	prod_code;             /* NEXRAD product code */
	short	flag;                  /* flag  = 0 */
	short	seq_no;                /* incremental block seq. number */
	short	no_of_prods;           /* number of products = -1 */
	short	req_intrvl;            /* request interval = 1 */
	short	vol_scan_date;         /* -1 */
	short	vol_scan_time_msw;     /* -1 (MSW) */
	short   vol_scan_time_lsw;     /* -1 (LSW) */
	short   prod_parms [6];        /* product dependent parmaters */
} RPS_block;

typedef struct {      /* GSM message header */
	short	msgcode;            /* message code */
	short	date_of_msg;        /* date */
	short	time_of_msg_msw;    /* time (MSW) */
	short	time_of_msg_lsw;    /* time (LSW) */
	short	length_of_msg_msw;  /* msg length (MSW) */
	short	length_of_msg_lsw;  /* msg length (LSW) */
	short	src_id;             /* NEXRAD id of source */
	short	dst_id;             /* NEXRAD id of dest. */
	short	no_of_blocks;       /* number of blocks in msg */
	short	divider;
	short	len_of_block;
	short	gsm_mode;
} gsm_header;

typedef struct {          /* Alert Area */
    short divider;        /* block devider = -1 */
    short length;         /* up to 820. Number of bytes to follow */
    short alert_area_num; /* 1 or 2 */
    short num_category;   /* 0 or 10 */
} alert_area_t;

typedef struct {      /* Bias Table Environmental Data Message
                         segment used for constructing ICD msg */
    char awips_id[4]; /* AWIPS site ID ... NOT NULL TERMINATED */
    char radar_id[4]; /* radar site ID ... NOT NULL TERMINATED */
    short obs_yr;     /* observation year (1970 - 2099) */
    short obs_mon;    /* observation month (1 - 12) */
    short obs_day;    /* observation day (1 - 31) */
    short obs_hr;     /* observation hour (0 - 23) */
    short obs_min;    /* observation minute (0 - 59) */
    short obs_sec;    /* observation second (0 - 59) */
    short gen_yr;     /* generation year (1970 - 2099) */
    short gen_mon;    /* generation month (1 - 12) */
    short gen_day;    /* generation day (1 - 31) */
    short gen_hr;     /* generation hour (0 - 23) */
    short gen_min;    /* generation minute (0 - 59) */
    short gen_sec;    /* generation second (0 - 59) */
    short n_rows;     /* number of bias table rows (2 - 12) */
} bias_table_msg_base_t;

typedef struct {                       /* product msg data */
        char prod_id[PROD_ID_LEN];     /* product id */
        char mne [MNE_LEN];            /* product mnemonic */
        char descrp [STRNG_LEN];       /* product description */
/*        char wx_modes[2];          */ /* different wx modes:
                                           1 = A
                                           2 = B
                                           7 = both */
        char prod_code[PROD_CODE_LEN]; /* NEXRAD product code */
        char params[NUMBER_PROD_PARMS][PROD_PARM_LEN]; /* product 
                                              dependent paramters */
} product_msg_info_t;

#define USER_PASSWD_LEN 6	/* user passwd buffer size	     */
#define PORT_PASSWD_LEN 4	/* port passwd buffer size	     */
				   
typedef struct {		/* Sign-on message.                          */
    short divider;		/* block devider = -1                        */
    short length;		/* block length = 18: Number of bytes from -1 
				   divider to end of block.                  */
    char user_passwd[USER_PASSWD_LEN];
                  		/* 6 chraracter user password.               */
    char port_passwd[PORT_PASSWD_LEN];
                  		/* 4 chraracter user password.               */
    short disconn_override_flag;
                  		/* 0, 1 (yes). override automatic disconnect 
				   feature.*/
    short spare;            	/* spare. 				     */         
} Sign_on_msg_t;

#define WMO_HEADER_SIZE                      30

typedef struct wmo_header {

   char form_type[2];		/* Should always be "S" - surface data followed by "D" - radar reports (i.e., SD) */
   char geo_area[2];		/* Country of original 2 letter designatorr... normally "US" */
   char distribution[2];	/* Not sure what value to give .... setiting to "00" */
   char space1;
   char originator[4];		/* Use 4 character ICAO. */
   char space2;
   char date_time[6]; 		/* In YYGGgg format, where YY is day of month, GGgg is hours and minutes, UTC. */
   char crcrlf[3];		

} WMO_header_t;

typedef struct awips_header {

   char category[3];
   char product[3];		/* Use ICAO without the leading "K" or "P", i.e., last three characters */
   char crcrlf[3];

} AWIPS_header_t;


typedef struct WMO_AWIPS_hdr {

   WMO_header_t wmo;
   AWIPS_header_t awips;

} WMO_AWIPS_hdr_t;

typedef struct Cat_prod {

   int prod_code;		/* Product code. */
   int elev_based;		/* Elevation based ... if set, elev_min and elev_max are define, and next will
                                   not be NULL. */
   int elev_min;		/* If Elevation based, lower bound for elevation angle (*10). */
   int elev_max;		/* If Elevation based, upper bound for elevation angle (*10). */
   char category[4];		/* Three character category name, followed by space. */

} Cat_prod_t;

typedef struct Node {

   Cat_prod_t cat_prod;		
   struct Node *next; 		/* Used for multiple category names for a single product code. */

} Node_t;

   /* function prototypes */

int BMF_init_prod_file (char *file_dir);

int  CMT_create_lbs (int link_num);
int  CMT_lbs_created();
int  CMT_read (int link_num);
int  CMT_write (int cm_type, int link_num, void *msg_buf, int msg_size);

void MA_abort (char *msg);
int  MA_get_connection_state (void);
char *MA_get_default_rps_file ();
int  MA_get_interface ();
int  MA_get_link_number ();
char *MA_get_site_id ();
int  MA_get_user_id ();
void MA_printlog(char *msg);
void MA_rps_required (int state);
void MA_terminate(int sig);
void MA_update_connection_state (int state);

int  PROC_init_product_dir (char *dir, int delete_files);
void PROC_new_conn (void);
int  PROC_process_msg (char *buf, int msg_len);
void *PROC_get_product_dir ();
int  PROC_get_prod_save_flag ();
void PROC_publish_naming_convention (int selection);
void PROC_toggle_prod_save_flag ();

int  RPL_read_product_list (FILE *fp, product_msg_info_t *prod_record, 
                            int *eof_flag);

int   WAH_add_awips_wmo_hdr (char *icao);
int   WAH_add_country_of_origin (char *geo_area);
char* WAH_add_header (char *buf, int msg_len, int code);

int  SM_alert_request_msg ();
char *SOC_get_inet_addr();
char *SM_init_data_dir ();
int  SM_sendrps(char *rps_file);
int  SM_send_bias_table_msg ();
int  SM_send_sign_on_msg ();
int  SM_send_product_request_msg (int pvc);
void SM_set_byte_swap_flag ();

SMI_info_t *SWAp_bytes (char *type_name, void *data);

void SOC_close_sockets ();
int  SOC_initiate_sock_connections (char *server_name, u_short port_number,
                                    char *password, int user_class);
int  SOC_pollsocks();
int  SOC_sock_write(int fd, char *buffer, int size);

void TERM_check_input (int link_state, int interface, int connection);
void TERM_check_WAN_OTR_input (char *orpg_host, ushort orpg_port, char *password);
int  TERM_init_terminal ();

#endif	/* #ifndef NBTCP_H */
