/**********************************************************************

    Description: This file contains the macros and private function
                 prototypes for librpg.

                 Function names indicate the file where it is defined.

                 ADE_ -  rpg_adpt_data_elems.c
                 ADP_ -  rpg_adaptation.c
                 AP_  -  rpg_abort_processing.c
                 ES_  -  rpg_event_services.c
                 IB_  -  rpg_inbuf.c
                 INIT_ - rpg_init.c
                 ITC_ -  rpg_itc.c
                 OB_  -  rpg_outbuf.c
                 PRQ_ -  rpg_prod_request.c
                 PS_  -  rpg_prod_support.c
                 SS_  -  rpg_scan_sum.c
                 VS_  -  rpg_scan_sum.c
                 WA_  -  rpg_wait_act.c

**********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/11/28 15:27:41 $
 * $Id: rpgc_globals.h,v 1.14 2012/11/28 15:27:41 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

#ifndef RPG_GLOBAL_H
#define RPG_GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include <a309.h>
#include <basedata.h>
#include <basedata_elev.h>
#include <rpg_port.h>
#include <prod_gen_msg.h>
#include <prod_request.h>
#include <prod_gen_msg.h>
#include <rpg_vcp.h>
#include <orpg.h>
#include <mrpg.h>

#define NAME_LEN 128

/* used in rpg_wait_act.c and rpg_inbuf.c */
enum { DATA_NOT_NEEDED, DATA_NEEDED };

/* used in rpg_inbuf.c */
enum { REPLAY_CURRENT_VOLUME, REPLAY_PREVIOUS_VOLUME };

/* processing resumption time */
enum {NEW_DATA, NEW_VOLUME, NEW_ELEVATION, RESUME_UNDEFINED};

/* useful macros */
#ifndef RPG_ADAPTATION
#define UNKNOWN    -1
#ifndef FALSE
#define FALSE       0
#endif
#ifndef TRUE
#define TRUE        1
#endif
#endif

#define MAXN_IBUFS 3 /* max number of buffers of a specific input 
                        allowed type */

/* For event notification. */
#define EVT_INTERNAL_EVENT   -1
#define EVT_EXTERNAL_EVENT   -2

/* For timer services. */
#define RESERVED_TIMER_ID    0x7fffffff

/* data structure for input data type */
typedef struct { 

    int type;               /* data type number */
    int type_mask;          /* data type mask value */
    int timing;             /* timing, ELEVATION_DATA, VOLUME_DATA, 
                               RADIAL_DATA, or, if >= 0, time window size in 
                               seconds for time based inputs */
    int data_id;            /* Data ID for the input.  Will be same as type
                               expect in the case where type is COMBBASE or
                               REFLDATA. Data ID then has value BASEDATA. */
    char *name;             /* Data Name .... can be empty string. */
    int len;                /* length of the current product */
    time_t time;            /* current time of data in use; Note that, if the
                               input is the driving input, this is the current
                               data time */
    time_t elev_time;       /* elevation time of the data; Driving input only. */
    time_t vol_time;        /* volume time of the data; Driving input only. */
    unsigned int vol_num;   /* volume sequence number when data was generated; 
                               Driving input only. */
    int rpg_elev_index;     /* RPG elevation index within the volume scan; Driving input
                               only. */
    char *buf[MAXN_IBUFS];  /* Array of allocated input buffers. */
    int buffer_count;       /* Maximum index of input buffers of this type 
                               currently allocated, to a maximum of 
                               MAXN_IBUFS - 1 */
    int requested;          /* this product is scheduled to be generated
                               (TRUE/FALSE) */
    char wait_for_data;     /* Only valid for replay types.  Determines whether we
                               should wait for the availability of this input or
                               should wait for a product request.  If TRUE, then
                               task should wait for the input data availability;
                               otherwise, the task should wait for a product request.
                               (TRUE/FALSE) */
    unsigned char moments;  /* For radial inputs, designates which moments user 
                               requires.  If user does not register for moments, 
                               default value is UNSPECIFIED_MOMENTS. */
    short block_time;       /* time in seconds get_inbuf will wait at most - 
                               This works only for optional inputs.  For 
                               mandatory inputs, block_time is 0. */
    short must_read;        /* If >0 indicates this data must be read, otherwise
                               0 indicates data was already read or there is no data
                               to read.  Used only for Wait_for == WAIT_ANY. */

} In_data_type;

#ifdef PROD_STATISTICS_DEFINED

/* data structure for product statistics type */
typedef struct {

   int prod_id;
   int num_prods;
   int min_size;
   int max_size;
   float avg_size;

} Prod_stats_t;

#endif

/* data structure for output data type */
typedef struct { 

    int type;               /* data type number */
    int timing;             /* timing, ELEVATION_DATA, VOLUME_DATA, 
                               RADIAL_DATA, or, if >= 0, genaration
                               interval for time based outputs */
    char *data_name;        /* Data name for the output. */
    char *name;             /* LB_name for the output */
    int len;                /* buffer size */
    int gen_cnt;            /* generation count for elevation based output */
    unsigned long elev_cnt; /* elevation count for elevation based output */
    int requested;          /* this product is a requested output product
                               (TRUE/FALSE) */
    en_t event_id;          /* this product has an associated event ID */
    int int_or_final;       /* indicate whether data is an intermediate
                               product type (INT_PROD), or final RPG 
                               product type (RPG_PROD) */
#ifdef PROD_STATISTICS_DEFINED

    Prod_stats_t p_stats;   /* Product statistics structure. */

#endif

} Out_data_type;

#define MAXN_BUFS       100     /* maximum number of output buffers 
                                   in use. */
#define SLOTS_CAP	20	/* maximum number of request slots. */

/* data structure compatible with legacy "USER_ARRAY" */
typedef struct {

   short ua_prod_code;          /* product code */
   short ua_dep_parm_0;         /* product dependent parameter 0 */
   short ua_dep_parm_1;         /* product dependent parameter 1 */
   short ua_dep_parm_2;         /* product dependent parameter 2 */
   short ua_dep_parm_3;         /* product dependent parameter 3 */
   short ua_dep_parm_4;         /* product dependent parameter 4 */
   short ua_dep_parm_5;         /* product dependent parameter 5 */
   short ua_elev_index;         /* elevation index */
   short ua_req_number;         /* request number */
   short ua_spare;              /* for future use */

} User_array_t;

/* data structure for buffer registration */
typedef struct {

    int type;                   /* buffer type defined in rpg_port.h */
    int size;                   /* size of the buffer in bytes */
    char *bpt;                  /* pointer to buffer. NULL means freed. */
    User_array_t *req;          /* product request data. */

} Buf_regist;

#define TIME_LIST_SIZE  3 /* size of the product time list */

typedef struct {

   LB_id_t id;           /* linear buffer id */
   time_t  time;         /* product time from product header */
   unsigned int vol_num; /* volume sequence number from product header */
   int rpg_elev_index;   /* rpg_elevation index from product header */

} Product_time_t; 

#define DRIVING 0 /* index of the driving input in the input list */

#define WAIT_TIME 1000  
   /* time (in milliseconds) defined the input polling 
      frequecy */

#define MAXN_OUTS 8 /* max number of output types */
#define MAXN_INPS 8 /* max number of input types */

/* Replay request information data structure. */
typedef struct replay_req_info_t {

   int replay_vol_seq_num;              /* Volume sequence number. */
   int replay_vol_time;                 /* Volume scan time (UNIX time) */
   int replay_elev_ind;                 /* RPG elevation index. */
   int replay_elev_ang;                 /* RPG elevation angle (*10). */
   int replay_type;   /* Currently either ALERT_OT_REQUEST 
        or USER_OT_REQUEST. */

} Replay_req_info_t;

/* The following functions are defined in rpgc_inbuf_c.c */
void IB_get_id_from_name( char *data_name, int *data_id );
void* IB_get_inbuf( int reqdata, int *datatype, int *opstat );
int IB_rel_all_inbufs ();
void IB_release_input_buffer (int ind, int buffer_ind);
int IB_read_driving_input ( int *buffer_ind );
int IB_inp_list (In_data_type **ilist);
void IB_set_task_name (char *task_name);
void IB_initialize (Orpgtat_entry_t *task_table);
void IB_new_session ();
LB_id_t IB_product_database_query( int ind );
void IB_check_input_buffer_load();


/* The following functions are defined in rpgc_outbuf_c.c */
void OB_reg_outputs( int *status );
int* OB_get_outbuf ( int dattyp, int bufsiz, int *olind, int *opstat);
int OB_rel_outbuf (int *bufptr, int *datdis, ...);
int OB_rel_all_outbufs (int *datdis);
int OB_release_output_buffer (int ind, int olind, int disposition);
void OB_set_prod_hdr_info (Base_data_header *hd, char *phd, int new_vol);
int OB_out_list (Out_data_type **olist);
int OB_buffers_list (Buf_regist **buffers, int **n_bufs);
Prod_header* OB_hd_info();
int OB_hd_info_set();
void OB_get_id_from_name( char *dataname, int *data_id );
void OB_initialize (Orpgtat_entry_t *task_table);
int OB_vol_number ();
int OB_get_buffer_number( int prod_code );
int OB_get_elev_cnt( int olind );
void OB_report_cpu_stats( char *task_name, unsigned int vol_scan_num, 
                          int vol_aborted, unsigned int expected_vol_dur );
int OB_tag_outbuf_wreq( int *bufptr, User_array_t *user_array, int olind );
int OB_force_use_supplemental_scans();


/* The following functions are defined in rpgc_abort_processing_c.c */
int AP_abort_processing( int reason );
void AP_init_prod_list( unsigned int vol_seq_num );
void AP_add_output_to_prod_list( Prod_header *phd );
int AP_abort_outputs( int reason );
int AP_abort_single_output( int datatype, int reason );
int AP_alg_control( int alg_control );
int AP_abort_flag( int abort_flag );
void AP_initialize();
void AP_hari_kiri();
int AP_get_abort_reason();
void AP_set_abort_reason( int reason );
void AP_init_abort_reason( );
void AP_set_aborted_volume( unsigned int vol_num );
int AP_abort_request( Prod_request *pr, int reason );

/* The following functions are defined in rpgc_wait_act_c.c */
void WA_initialize ( Orpgtat_entry_t *task_entry );
int WA_check_data_prod ( int buffer_ind );
int WA_check_data ( int buffer_ind );
int WA_radial_status ();
int WA_wait_driving ();
int WA_data_filtering ( In_data_type *Inp_list, int buffer_ind );
int WA_set_resume_time();
Replay_req_info_t* WA_get_query_info();
int WA_get_next_avail_input();
int WA_wait_for_any_data (int wait_for, int *status, int wait_for_data );
int WA_waiting_for_activation();
int WA_check_replay_elev_vol_availability( int data_store_id, int sub_type, 
                                           int vol_seq_num, int elev_ind );
int WA_set_output_info( char *outbuf, int olind );
int WA_supplemental_scans_allowed();

/* The following functions are defined in rpgc_prod_compress_c.c */
void* CP_decompress_product( void* src, int *size );

/* The following functions are defined in rpgc_event_services_c.c */
int ES_event_registered ( int event_code );
void ES_initialize();

/* The following functions are defined in rpgc_prod_request_c.c */
void PRQ_initialize ();
int PRQ_update_requests (int new_vol, int elev_ind, int time);
int PRQ_check_data(int datatype, User_array_t *user_array, int *status);
int PRQ_check_req(int datatype, short *user_array, int *status);
Prod_request* PRQ_replay_product_requests( Prod_request *req,
                                           int length, int *num_req );
Prod_request* PRQ_check_for_replay_requests( int *num_reqs );
Prod_request* PRQ_get_prod_request( int data_type, int *n_requests );
int PRQ_populate_user_array( int elev_index, int prod_id, char *buf,
                             int num_reqs, User_array_t *uarray,
                             int max_requests );
void PRQ_set_vol_seq_num( unsigned int vol_seq );



/* The following functions are defined in rpgc_itc_c.c */
void ITC_initialize();
void ITC_read_all (int inp_prod);
void ITC_write_all (int out_prod);
void ITC_update (int new_vol);
void ITC_update_by_event (en_t event_id);
void* ITC_get_data(int itc_id);

/* The following functions are defined in rpgc_product_support_c.c */
void PS_register_bd_hd (Base_data_header *bd_hd);
void PS_register_prod_hdr (Prod_header *phd);
int PS_get_current_elev_index (int *index);
unsigned int  PS_get_current_vol_num (int *vol_num);
int  PS_get_vol_stat_vol_num (unsigned int *vol_deq_num);
void PS_task_abort (char *format, ... );
time_t PS_convert_to_unix_time (short date, short time_ms, short time_ls);
time_t PS_get_volume_time (Base_data_header *bhd);
int PS_get_elev_angle( int *vcpnum, int *rpg_elev_ind, int *elev_angle,
                       int *found ); 


/* The following functions are defined in rpgc_init_c.c */
int INIT_task_type();
char* INIT_task_name();
void INIT_register_sc_status (int *sc_changed);
int INIT_process_argv (char *argv, int *len);
int INIT_task_terminating();
int INIT_get_task_input_stream();
int INIT_process_eov_event();
int INIT_file_line( char *file, int line );
int CO_process_custom_options( int c );
void CO_initialize();


/* The following functions are defined in rpgc_scan_sum_c.c */
void SS_initialize ();
void SS_update_summary (Base_data_header *bd_hd);
int  SS_send_summary_array (int *summary);
void* SS_get_summary_data();
time_t SS_get_volume_time( unsigned int vol_num );
int  SS_get_wx_mode( unsigned int vol_num, int *vcp_num );
void SS_read_scan_summary();

/* The following functions are defined in rpgc_scan_sum_c.c */
int SS_register ();

void VS_initialize ();
void VS_update_volume_status (Base_data_header *bd_hd);
int  VS_send_volume_status ( char *vol_stat);
void* VS_get_volume_status();
void VS_read_volume_status();

/* The following functions are defined in rpgc_adaptation_c.c */
void ADP_update_adaptation (int new_vol);
void ADP_update_adaptation_by_event( en_t event_id );
int register_adapt (int *id, char *buf);
void ADP_initialize ();

/* The following functions are define in rpgc_adpt_data_elems_c.c */
void ADE_initialize();
int ADE_update_ade(int new_vol);
int ADE_update_ade_by_event(en_t event_id);

/* The following functions are defined in rpgc_timer_services_c.c */
void TS_reg_timer( int timer_id, void (*callback) () );
void a3cm40( int parameter, int count, int flag, int *ier );
void a3cm41( int parameter, int count, int flag, int *ier );

/* The following function is defined is rpgcs_vcp_info.c */
void VI_initialize();
void VI_set_vol_seq_num( unsigned int vol_seq );
int VI_is_supplemental_scan( int vcp_num, int vol_num, int elev_index );

#endif   /* #ifndef RPG_GLOBALS_H */
