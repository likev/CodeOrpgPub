/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/12 19:31:01 $
 * $Id: ps_def.h,v 1.70 2014/08/12 19:31:01 steves Exp $
 * $Revision: 1.70 $
 * $State: Exp $
 */

#ifndef PS_DEF_H
#define PS_DEF_H

#include <time.h>		/* time_t				*/

#include <lb.h>			/* LB_id_t				*/

#include <orpg.h>

#include <prod_gen_msg.h>       /* Prod_gen_msg                         */
#include <prod_distri_info.h>	/* prod_id_t				*/
#include <prod_status.h>
#include <ps_process_events.h>
#include <ps_prod_task_tables.h>
#include <ps_handle_prod.h>

#define PS_DEF_BUF_SIZE_WDS     16000
#define PS_DEF_BUF_SIZE         64000

#define PS_DEF_FOUND_IT		1
#define PS_DEF_NOT_FOUND	0

#define PS_DEF_SUCCESS	        1
#define PS_DEF_FAILED	       -1
#define PS_DEF_DUP_VOL_NUM     -2

/* For weather mode changes. */
#define PS_DEF_WXMODE_UNKNOWN     -1
#define PS_DEF_WXMODE_NOT_DEFAULT -2
#define PS_DEF_WXMODE_CHANGED     -3
#define PS_DEF_WXMODE_UNCHANGED   -4

/* For act_this_time field of Prod_gen_status_pr_req. */
#define PS_DEF_PROD_ACTIVATED        0
#define PS_DEF_PROD_DEACTIVATED   9999

/* Indicate source of ORPGDAT_PROD_STATUS data ...  */
#define PS_DEF_NEW_PRODUCT	0
#define PS_DEF_FROM_TIMER	1

/* Indicate source of product request. */
#define PS_DEF_FROM_INIT    	  1
#define PS_DEF_FROM_GEN_PROD      2
#define PS_DEF_FROM_DEFAULT       4	/* from default prod gen list	  */
#define PS_DEF_FROM_ONE_TIME      8	/* from one-time request          */
#define PS_DEF_FROM_REQUEST	 16	/* routine request                */

/* Several levels of "verbosity" are provided ... at this time, we'll
   default to the second-lowest level ...
   The assigning of levels is rather arbitrary at this time. */
#define PS_DEF_MIN_VERBOSE_LEVEL 0
#define PS_DEF_REQRD_VERBOSE_LEVEL  (PS_DEF_MIN_VERBOSE_LEVEL)
#define PS_DEF_FATAL_VERBOSE_LEVEL  (PS_DEF_MIN_VERBOSE_LEVEL)
#define PS_DEF_ERR_VERBOSE_LEVEL   ((PS_DEF_MIN_VERBOSE_LEVEL) + 1)
#define PS_DEF_WARN_VERBOSE_LEVEL  ((PS_DEF_MIN_VERBOSE_LEVEL) + 2)
#define PS_DEF_INFO_VERBOSE_LEVEL  ((PS_DEF_MIN_VERBOSE_LEVEL) + 3)
#define PS_DEF_DEBUG_VERBOSE_LEVEL ((PS_DEF_MIN_VERBOSE_LEVEL) + 4)
#define PS_DEF_MAX_VERBOSE_LEVEL    (PS_DEF_DEBUG_VERBOSE_LEVEL)
#define PS_DEF_DFLT_VERBOSE_LEVEL   (PS_DEF_ERR_VERBOSE_LEVEL)

/* Task status values. */
#define PS_DEF_TASK_UNAVAILABLE         1
#define PS_DEF_TASK_NOT_RUNNING         2
#define PS_DEF_TASK_RUNNING             3
#define PS_DEF_TASK_FAILURE             4
#define PS_DEF_TASK_STATUS_UNAVAILABLE  5

/* For CPU monitoring. */
#define MALRM_CPU_MONITOR         	1
#define MALRM_CPU_MONITOR_RATE   	10 
#define CPU_OVERHEAD              	0

#define PS_DEF_CURRENT_VOLUME       	0
#define PS_DEF_PREVIOUS_VOLUME      	1

#define PS_DEF_MAX_USERS         	2  /* See From_t structure definition */

/* For VCP checking. */
#define RRS_NO_DIFFERENCES		0
#define RRS_DIFFERENT_VCPS		-1
#define RRS_DIFFERENT_N_ANGLES		-2
#define RRS_DIFFERENT_ANGLES		-3

/* Generation Control_lists. */
#define CUR_GEN_CONTROL                 0
#define BACKUP_GEN_CONTROL              1
#define ONETIME_GEN_CONTROL             2

typedef struct {

   unsigned int source;		/* Bit map indicating the source of a
                                   request.  See macros defined above. */ 
   unsigned int line_ind_list[ PS_DEF_MAX_USERS ];
				/* Bit map indicating which users 
                                   requested product.  The mapping of
                                   line id to line index is as follows:

                                      line ID 0  -> Bit 0, word 0 
                                      line ID 1  -> Bit 1, word 0
                                                  .
                                                  .
                                      line ID 32 -> Bit 0, word 1
                                                 etc
                                                                       */

} From_t;

/* Real_time generation status of a product */
typedef struct GEN_STATUS { 

    prod_id_t prod_id; 		/* product buffer id */
    short elev_index;           /* elevation index associated with elevation
                                   parameter, if defined. */
    short params[NUM_PROD_DEPENDENT_PARAMS];
                       		/* product dependent parameter */
    From_t from;       		/* FROM_ONE_TIME, FROM_DEFAULT, for routine
                       		   request, it will be line_ind */
    short gen_pr;      		/* generation period in number of volumes */
    short req_num;             	/* used by prod_gen_control, coming from one
                               	   time scheduler. by default is 0. */
    unsigned int  schedule;   	/* PGS_SCHEDULED, PGS_NOT_SCHEDULED
                               	   or PGS_TASK_NOT_RUNNING */
    time_t vol_time;           	/* volume scan starting time. */
    unsigned int vol_num;      	/* vol number replacing vol time */

    LB_id_t   msg_ids;		/* list of the latest product message IDs;
                               	   Each entry corresponds to a volume with
                               	   the first entry conresponding to the latest
                               	   (the current) volume. If a product is not
                               	   available, a special value (PGS_UNKNOWN...
                               	   as defined above) is used to indicate the
                               	   possible cause. */
} Gen_status_t;


/* Real-time generation status of a product node. */
typedef struct PROD_GEN_STATUS_PR {  

    Gen_status_t gen_status;	/* Product Generation status. */
    struct PROD_GEN_STATUS_PR *next;

} Prod_gen_status_pr;

/* Real-time generation status of a product request. */
typedef struct PROD_GEN_STATUS_PR_REQ {

    Gen_status_t gen_status; 	/* Generation status of product. */
    short num_products;	      	/* How many of product requested. */
    short act_this_time;       	/* In this vol, this request is active
                                   or not. */
    int   cnt;		       	/* act once, ++. */
    
    struct PROD_GEN_STATUS_PR_REQ *next;				   
} Prod_gen_status_pr_req;

/*
 * Function Prototypes:
 *
 *      MCPU_   Monitor CPU functions
 *      PD_     Handle Product functions
 *	PSCV_	Convert Parameters functions
 *      PSPE_ 	Process-Event functions
 *      PSPM_ 	Process-Message functions
 *      PSPTT_ 	Product/Task Tables Generation/Maintenance functions
 *      PSTS_ 	Task Status (TS) functions
 *      PSVPL_ 	Volume Product List (VPL) functions
 *      RRS_ 	RDA/RPG Status functions
 *
 */

  /* The following public functions are defined in file ps_monitor_cpu.c */
  int MCPU_monitor_cpu();
  void MCPU_initialize();
  int MCPU_report_cpu_utilization();

  /* The following public functions are defined in file ps_handle_prod.c */
  void PD_add_back_one_time_list_to_cur_vol( void );
   int PD_add_output_gen_control_list( Prod_gen_status_pr_req *req, 
                                       int call_from );
   int PD_backup_one_time_list( Prod_gen_status_pr_req *one_time_list,
                                int vol_num );
  void PD_backup_output_gen_control_list( void );
   int PD_change_act_each_line( void );
   int PD_check_scheduling_info ( prod_id_t prod_id );
   int PD_rps_update( void );
   int PD_form_default_prod_gen_list( int *default_pgt_num_prods );
  void PD_free_gen_control_list( int list_type );
  void PD_gen_output_gen_control_list( int option );
Prod_gen_status_pr_req* PD_get_output_gen_control_list( void );
   int PD_get_output_gen_control_list_len( void );
  void PD_initialize( void );
  void PD_keep_one_time_req_vol_list( void );
  void PD_keep_routine_req_vol_list( void );
  void PD_merge_cur_backup_output_gen_control_list( void );
   int PD_process_request( int length, char *buf, int call_from );
   int PD_init_prod_list_to_gen_control( int prod_id );
   int PD_schedule_this_prod( prod_id_t prod_id, unsigned int source,
                              unsigned int line_id, void *aux_data );
   int PD_tell_same_prod( Gen_status_t *prod, Gen_status_t *prod1 );
   void PD_set_line_from_line_ind_list( From_t *from, unsigned int line_id );
   void PD_merge_from_line_ind_list( From_t *dest, From_t *src );
   int PD_test_line_from_line_ind_list( From_t *dest, unsigned int line_id );
   void PD_clear_line_from_line_ind_list( From_t *dest, unsigned int line_id );
   void PD_clear_from_line_ind_list( From_t *from );
   void PD_write_prod( short pid, short *params, short elev_index );
   Pd_prod_gen_tbl_entry* PD_get_default_prod_gen_list( void );
   int PD_validate_request_message( char *buf, int len, int call_from );
   int PD_build_request_list( int len, char *buf, int call_from, int line_ind,
                              Prod_gen_status_pr_req **requests );
   void PD_special_request_processing( short prod_id, short *req_elev_ind );

  /* The following public functions are defined in file ps_convert_params.c */
   int PSCV_convert_p6( Prod_gen_msg *gen_msg,
                        Prod_gen_status_pr *prod_status );


  /* The following public functions are defined in file ps_process_events.c */
   int PSPE_handle_one_time_req_list( char *buf, int len );
   int PSPE_init_gen_control_lb( void );
  void PSPE_initialize( void );
  void PSPE_proc_ot_schedule_list_event( void );
  void PSPE_proc_prod_list_event( void );
  void PSPE_proc_rt_request_event( void );
  void PSPE_proc_start_of_volume_event( int prod_list_updated, int force_update_all );
   int PSPE_prod_list_to_prod_request( Prod_gen_status_pr_req *list,
                                       int num_req );


  /* The following public functions are defined in file ps_process_msg.c */
  void PSPM_initialize( void );
   int PSPM_chk_new_prods( void );

  /* The following public functions are defined in file ps_task_status_list.c */
  void PSTS_initialize( );
   int PSTS_gen_task_status_list( Task_prod_chain *task_prod_list );
   int PSTS_update_task_status( void );
Task_prod_chain* PSTS_is_gen_task_running( short prod_id, int *status );
   Task_prod_chain*  PSTS_what_is_task_status( char *task_name, int *status );
   int PSTS_set_task_prod_list_status( char *task_name, int status );
Task_prod_chain* PSTS_find_task_in_task_prod_list( char  *task_name );


  /* The following public functions are defined in file ps_vol_prod_list.c */
   int PSVPL_init_vol_list_for_cur_vol( void );
  void PSVPL_initialize( void );
   int PSVPL_add_prod_gen_status( Prod_gen_status_pr *prod_status, int vol_num );
   int PSVPL_delete_line_ind_vol_list_latest( int index );
   int PSVPL_free_prod_list_according_to_ind( int ind );
  void PSVPL_init_vpl_curvol_wxmode_voltime( void );
  void PSVPL_mark_one_time_req_last_vol_not_used_cur_vol( void );
   int PSVPL_newly_gen_prod_too_old( Prod_gen_status_pr *prod_status );
   int PSVPL_output_product_status( int from );
   int PSVPL_put_failed_ones( void );
   int PSVPL_update_vol_list( void );
   int PSVPL_shift_one_upper_vol_prod_list( void );
   int PSVPL_get_vol_list_index( Prod_gen_status_pr *prod_status );

  /* The following public functions are defined in file ps_rda_rpg_status.c */
time_t RRS_get_volume_time( time_t *clock );
   int RRS_get_current_weather_mode( void );
   int RRS_get_previous_weather_mode( void );
   int RRS_get_current_vcp_num( void );
   int RRS_get_previous_vcp_num( void );
   int RRS_initial_volume( void );
unsigned int RRS_initial_volume_number( void );
  void RRS_init_for_ps( void );
  void RRS_init_read( void );
  int  RRS_update_vol_status( void );
  int  RRS_get_elevation_index( int elev, int *wx_mode );
  int  RRS_get_elevation_angle( int elev_ind );
  int  RRS_current_previous_vcps_differ();
unsigned int RRS_get_volume_num( time_t *clock );   

  /* The following public functions are defined in file ps_prod_task_tables.c */
  void PSPTT_initialize( void );
  Prod_depend_chain* PSPTT_get_prod_depend_list( int *size );
  Depend_prod_chain* PSPTT_get_depend_prod_list( int *size );
  Task_prod_chain* PSPTT_get_task_prod_list( int *size );
  Task_prod_wth_only_id_next* PSPTT_find_entry_in_depend_prod_list( char  *task_name );
  void PSPTT_dump_task_prod_list( void );
  void PSPTT_dump_prod_depend_list( void );
  void PSPTT_dump_depend_prod_list( void );
  void PSPTT_schedule_basic_products();
   int PSPTT_through_dep_list_of_this_prod( prod_id_t prod_id, unsigned int source,
                                            unsigned int line_id, void *aux_data );
  void PSPTT_build_prod_task_tables( void );

#endif		/* #ifndef PS_DEF_H */
