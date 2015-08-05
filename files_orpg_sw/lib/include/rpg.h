
/**************************************************************************

    Description: This file defines the public interface for librpg library.
                 All public functions are preceeded by RPG_ with the 
                 exception of the FORTRAN support functions which were 
                 rewritten to support legacy.  These functions were 
                 originally Concurrent system support functions which are
                 an extension of the FORTRAN ANSI standard.

                 A description of the public functions can be found in the
                 rpg library man page.

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/25 17:22:31 $
 * $Id: rpg.h,v 1.37 2014/04/25 17:22:31 steves Exp $
 * $Revision: 1.37 $
 * $State: Exp $
 */

#ifndef RPG_H
#define RPG_H

#ifndef DEVELOP_VERSION
#include <rss_replace.h>
#endif

#include <rpg_globals.h>
#include <siteadp.h>

/* Macros defined for event services. */
#define EVT_WAIT_FOR_DATA           -1
#define EVT_NO_WAIT_FOR_DATA        -2

/*
#define DEBUG    0
*/

#ifndef C_NO_UNDERSCORE

#ifdef LINUX
/*
 * Note single trailing underscore ...
 */
#define ilbyte ilbyte_
#define isbyte isbyte_
#define date(a) date_(a)
#define iclock iclock_
#define btest conc_btest__
#define btest_short btest_short__
#define os32bclr os32bclr_
#define os32sbclr os32sbclr_
#define os32bset os32bset_
#define os32sbset os32sbset_
#define sndmsg sndmsg_
#define deflst deflst_
#define atl atl_
#define abl abl_
#define rtl rtl_
#define rbl rbl_
#define lstfun lstfun_
#define queue queue_
#define ioerr ioerr_
#define s4cmad s4cmad_
#define a3cm40 a3cm40_
#define A3CM40 a3cm40_
#define a3cm41 a3cm41_
#define A3CM41 a3cm41_

/*
 * Note double trailing underscores ...
 */
#define ITOC_OS32 itoc_os32__
#define itoc_os32 itoc_os32__
#define t41194__gettime t41194__gettime__

#define RPG_reg_adpt rpg_reg_adpt__
#define RPG_is_adapt_block_registered rpg_is_adapt_block_registered__
#define RPG_DEAU_callback_fx rpg_deau_callback_fx__
#define RPG_read_adapt_block rpg_read_adapt_block__
#define RPG_update_adaptation rpg_update_adaptation__
#define RPG_reg_ade_callback rpg_reg_ade_callback__
#define RPG_is_ade_callback_reg rpg_is_ade_callback_reg__
#define RPG_read_ade rpg_read_ade__
#define RPG_update_all_ade rpg_update_all_ade__
#define RPG_reg_site_info rpg_reg_site_info__
#define RPG_data_access_group rpg_data_access_group__
#define RPG_data_access_update rpg_data_access_update__
#define RPG_data_access_read rpg_data_access_read__
#define RPG_data_access_write rpg_data_access_write__
#define RPG_reg_timer rpg_reg_timer__
#define RPG_read_scan_summary rpg_read_scan_summary__
#define RPG_read_volume_status rpg_read_volume_status__
#define RPG_get_current_target_elev rpg_get_current_target_elev__
#define RPG_elev_angle_BAMS_to_deg rpg_elev_angle_bams_to_deg__
#define RPG_get_buffer_vol_seq_num rpg_get_buffer_vol_seq_num__
#define RPG_get_buffer_vol_num rpg_get_buffer_vol_num__
#define RPG_get_buffer_vcp_num rpg_get_buffer_vcp_num__
#define RPG_get_buffer_elev_index rpg_get_buffer_elev_index__
#define RPG_is_buffer_from_last_elev rpg_is_buffer_from_last_elev__
#define RPG_get_request rpg_get_request__
#define RPG_get_request rpg_get_request__
#define RPG_task_init_c rpg_task_init_c__
#define RPG_init_log_services_c rpg_init_log_services_c__
#define RPG_itc_in rpg_itc_in__
#define RPG_itc_out rpg_itc_out__
#define RPG_itc_read rpg_itc_read__
#define RPG_itc_write rpg_itc_write__
#define RPG_itc_callback rpg_itc_callback__
#define RPG_set_veldeal_bit rpg_set_veldeal_bit__
#define RPG_reg_for_internal_event rpg_reg_for_internal_event__
#define RPG_reg_for_external_event rpg_reg_for_external_event__
#define RPG_UN_register rpg_un_register__
#define RPG_wait_for_event rpg_wait_for_event__
#define RPG_wait_for_any_data rpg_wait_for_any_data__
#define RPG_in_data rpg_in_data__
#define RPG_in_opt rpg_in_opt__
#define RPG_in_opt_by_name rpg_in_opt_by_name__
#define RPG_reg_moments rpg_reg_moments__
#define RPG_get_inbuf_len rpg_get_inbuf_len__
#define RPG_what_moments rpg_what_moments__
#define RPG_monitor_input_buffer_load rpg_monitor_input_buffer_load__
#define RPG_monitor_input_buffer_load rpg_monitor_input_buffer_load__
#define RPG_out_data rpg_out_data__
#define RPG_out_data_wevent rpg_out_data_wevent__
#define RPG_out_data_by_name_wevent rpg_out_data_by_name_wevent__
#define RPG_get_customizing_info rpg_get_customizing_info__
#define RPG_wait_act rpg_wait_act__
#define RPG_allow_suppl_scans rpg_allow_suppl_scans__
#define RPG_remap_rpg_elev_index rpg_remap_rpg_elev_index__
#define RPG_wait_for_task rpg_wait_for_task__
#define RPG_abort_processing rpg_abort_processing__
#define RPG_abort_datatype_processing rpg_abort_datatype_processing__
#define RPG_abort_request rpg_abort_request__
#define RPG_cleanup_and_abort rpg_cleanup_and_abort__
#define RPG_get_abort_reason rpg_get_abort_reason__
#define RPG_abort_task rpg_abort_task__
#define RPG_hari_kiri rpg_hari_kiri__
#define RPG_clear_msg rpg_clear_msg__
#define RPG_send_msg rpg_send_msg__
#define RPG_compress_product rpg_compress_product__
#define RPG_set_product_int rpg_set_product_int__
#define RPG_get_product_int rpg_get_product_int__
#define RPG_set_product_float rpg_set_product_float__
#define RPG_get_product_float rpg_get_product_float__
#define RPG_set_mssw_to_uint rpg_set_mssw_to_uint__
#define RPG_NINT rpg_nint__
#define RPG_site_info_callback_fx rpg_site_info_callback_fx__
#define RPG_get_code_from_name rpg_get_code_from_name__
#define RPG_get_code_from_id rpg_get_code_from_id__
#define RPG_get_elev_time rpg_get_elev_time__
#define RPG_avset_last_elev rpg_avset_last_elev__


#elif SUNOS
#define RPG_reg_adpt rpg_reg_adpt_
#define RPG_is_adapt_block_registered rpg_is_adapt_block_registered_
#define RPG_DEAU_callback_fx rpg_deau_callback_fx_
#define RPG_read_adapt_block rpg_read_adapt_block_
#define RPG_update_adaptation rpg_update_adaptation_
#define RPG_reg_ade_callback rpg_reg_ade_callback_
#define RPG_is_ade_callback_reg rpg_is_ade_callback_reg_
#define RPG_read_ade rpg_read_ade_
#define RPG_update_all_ade rpg_update_all_ade_
#define RPG_reg_site_info rpg_reg_site_info_
#define RPG_data_access_group rpg_data_access_group_
#define RPG_data_access_update rpg_data_access_update_
#define RPG_data_access_read rpg_data_access_read_
#define RPG_data_access_write rpg_data_access_write_
#define RPG_reg_timer rpg_reg_timer_
#define RPG_read_scan_summary rpg_read_scan_summary_
#define RPG_read_volume_status rpg_read_volume_status_
#define RPG_get_current_target_elev rpg_get_current_target_elev_
#define RPG_elev_angle_BAMS_to_deg rpg_elev_angle_bams_to_deg_
#define RPG_get_buffer_vol_seq_num rpg_get_buffer_vol_seq_num_
#define RPG_get_buffer_vol_num rpg_get_buffer_vol_num_
#define RPG_get_buffer_vcp_num rpg_get_buffer_vcp_num_
#define RPG_get_buffer_elev_index rpg_get_buffer_elev_index_
#define RPG_is_buffer_from_last_elev rpg_is_buffer_from_last_elev_
#define RPG_get_request rpg_get_request_
#define RPG_get_request rpg_get_request_
#define RPG_task_init_c rpg_task_init_c_
#define RPG_init_log_services_c rpg_init_log_services_c_
#define RPG_itc_in rpg_itc_in_
#define RPG_itc_out rpg_itc_out_
#define RPG_itc_read rpg_itc_read_
#define RPG_itc_write rpg_itc_write_
#define RPG_itc_callback rpg_itc_callback_
#define RPG_set_veldeal_bit rpg_set_veldeal_bit_
#define RPG_reg_for_internal_event rpg_reg_for_internal_event_
#define RPG_reg_for_external_event rpg_reg_for_external_event_
#define RPG_UN_register rpg_un_register_
#define RPG_wait_for_event rpg_wait_for_event_
#define RPG_wait_for_any_data rpg_wait_for_any_data_
#define RPG_in_data rpg_in_data_
#define RPG_monitor_input_buffer_load rpg_monitor_input_buffer_load_
#define RPG_in_opt rpg_in_opt_
#define RPG_in_opt_by_name rpg_in_opt_by_name_
#define RPG_reg_moments rpg_reg_moments_
#define RPG_get_inbuf_len rpg_get_inbuf_len_
#define RPG_what_moments rpg_what_moments_
#define RPG_out_data rpg_out_data_
#define RPG_out_data_wevent rpg_out_data_wevent_
#define RPG_out_data_by_name_wevent rpg_out_data_by_name_wevent_
#define RPG_get_customizing_info rpg_get_customizing_info_
#define RPG_wait_act rpg_wait_act_
#define RPG_allow_suppl_scans rpg_allow_suppl_scans_
#define RPG_remap_rpg_elev_index rpg_remap_rpg_elev_index_
#define RPG_wait_for_task rpg_wait_for_task_
#define RPG_abort_processing rpg_abort_processing_
#define RPG_abort_datatype_processing rpg_abort_datatype_processing_
#define RPG_abort_request rpg_abort_request_
#define RPG_cleanup_and_abort rpg_cleanup_and_abort_
#define RPG_get_abort_reason rpg_get_abort_reason_
#define RPG_abort_task rpg_abort_task_
#define RPG_hari_kiri rpg_hari_kiri_
#define RPG_clear_msg rpg_clear_msg_
#define RPG_send_msg rpg_send_msg_
#define RPG_compress_product rpg_compress_product_
#define RPG_set_product_int rpg_set_product_int_
#define RPG_get_product_int rpg_get_product_int_
#define RPG_set_product_float rpg_set_product_float_
#define RPG_get_product_float rpg_get_product_float_
#define RPG_set_mssw_to_uint rpg_set_mssw_to_uint_
#define RPG_NINT rpg_nint_
#define RPG_site_info_callback_fx rpg_site_info_callback_fx_
#define RPG_get_code_from_name rpg_get_code_from_name_
#define RPG_get_code_from_id rpg_get_code_from_id_
#define RPG_get_elev_time rpg_get_elev_time_
#define RPG_avset_last_elev rpg_avset_last_elev_

#define ilbyte ilbyte_
#define isbyte isbyte_
#define date(a) date_(a)
#define iclock iclock_
#define t41194__gettime t41194__gettime_
#define btest conc_btest_
#define btest_short btest_short_
#define os32bclr os32bclr_
#define os32sbclr os32sbclr_
#define os32bset os32bset_
#define os32sbset os32sbset_
#define sndmsg sndmsg_
#define deflst deflst_
#define atl atl_
#define abl abl_
#define rtl rtl_
#define rbl rbl_
#define lstfun lstfun_
#define wait_c wait_c_
#define queue queue_
#define ioerr ioerr_
#define s4cmad s4cmad_
#define a3cm40 a3cm40_
#define A3CM40 a3cm40_
#define a3cm41 a3cm41_
#define A3CM41 a3cm41_
#define ITOC_OS32 itoc_os32_
#define itoc_os32 itoc_os32_


#else

#define RPG_task_init_c rpg_task_init_c
#define RPG_init_log_services_c rpg_init_log_services_c
#define RPG_reg_adpt rpg_reg_adpt
#define RPG_is_adapt_block_registered rpg_is_adapt_block_registered
#define RPG_DEAU_callback_fx rpg_deau_callback_fx
#define RPG_read_adapt_block rpg_read_adapt_block
#define RPG_update_adaptation rpg_update_adaptation
#define RPG_reg_ade_callback rpg_reg_ade_callback
#define RPG_is_ade_callback_reg rpg_is_ade_callback_reg
#define RPG_read_ade rpg_read_ade
#define RPG_update_all_ade rpg_update_all_ade
#define RPG_reg_site_info rpg_reg_site_info
#define RPG_data_access_group rpg_data_access_group
#define RPG_data_access_update rpg_data_access_update
#define RPG_data_access_read rpg_data_access_read
#define RPG_data_access_write rpg_data_access_write
#define RPG_itc_in rpg_itc_in
#define RPG_itc_out rpg_itc_out
#define RPG_itc_read rpg_itc_read
#define RPG_itc_write rpg_itc_write
#define RPG_itc_callback rpg_itc_callback
#define RPG_read_scan_summary rpg_read_scan_summary
#define RPG_read_volume_status rpg_read_volume_status
#define RPG_reg_timer rpg_reg_timer
#define A3CM40 a3cm40
#define A3CM41 a3cm41
#define RPG_get_request rpg_get_request
#define RPG_set_veldeal_bit rpg_set_veldeal_bit
#define RPG_reg_for_internal_event rpg_reg_for_internal_event
#define RPG_reg_for_external_event rpg_reg_for_external_event
#define RPG_UN_register rpg_un_register
#define RPG_elev_angle_BAMS_to_deg rpg_elev_angle_bams_to_deg
#define RPG_get_buffer_vol_seq_num rpg_get_buffer_vol_seq_num
#define RPG_get_buffer_vol_num rpg_get_buffer_vol_num
#define RPG_get_buffer_vcp_num rpg_get_buffer_vcp_num
#define RPG_get_buffer_elev_index rpg_get_buffer_elev_index
#define RPG_is_buffer_from_last_elev rpg_is_buffer_from_last_elev
#define RPG_wait_for_event rpg_wait_for_event
#define RPG_wait_for_any_data rpg_wait_for_any_data
#define RPG_in_data rpg_in_data
#define RPG_monitor_input_buffer_load rpg_monitor_input_buffer_load
#define RPG_in_opt rpg_in_opt
#define RPG_in_opt_by_name rpg_in_opt_by_name
#define RPG_reg_moments rpg_reg_moments
#define RPG_get_inbuf_len rpg_get_inbuf_len
#define RPG_what_moments rpg_what_moments
#define RPG_out_data rpg_out_data
#define RPG_out_data_wevent rpg_out_data_wevent
#define RPG_out_data_by_name_wevent rpg_out_data_by_name_wevent
#define RPG_get_customizing_info rpg_get_customizing_info
#define RPG_wait_act rpg_wait_act
#define RPG_allow_suppl_scans rpg_allow_suppl_scans
#define RPG_remap_rpg_elev_index rpg_remap_rpg_elev_index
#define RPG_wait_for_task rpg_wait_for_task
#define RPG_abort_processing rpg_abort_processing
#define RPG_cleanup_and_abort rpg_cleanup_and_abort
#define RPG_abort_datatype_processing rpg_abort_datatype_processing
#define RPG_abort_request rpg_abort_request
#define RPG_get_abort_reason rpg_get_abort_reason
#define RPG_abort_task rpg_abort_task
#define RPG_hari_kiri rpg_hari_kiri
#define RPG_clear_msg rpg_clear_msg
#define RPG_send_msg rpg_send_msg
#define RPG_compress_product rpg_compress_product
#define RPG_set_product_int rpg_set_product_int
#define RPG_get_product_int rpg_get_product_int
#define RPG_set_product_float rpg_set_product_float
#define RPG_get_product_float rpg_get_product_float
#define RPG_set_mssw_to_uint rpg_set_mssw_to_uint
#define RPG_NINT rpg_nint
#define RPG_site_info_callback_fx rpg_site_info_callback_fx
#define RPG_get_code_from_name rpg_get_code_from_name
#define RPG_get_code_from_id rpg_get_code_from_id
#define RPG_get_elev_time rpg_get_elev_time
#define RPG_avset_last_elev rpg_avset_last_elev

#endif
#endif

/* function prototypes */

/* The following functions are defined in rpg_inbuf.c */
int RPG_in_data (int *data_type, int *flags);
int RPG_monitor_input_buffer_load( int *data_id );
int RPG_in_opt (int *data_type, int *block_time);
int RPG_in_opt_by_name (char *data_name, int *block_time);
int RPG_reg_moments (int *moments);
int RPG_get_inbuf_len ( int *bufptr, int *len );


/* The following functions are defined in rpg_outbuf.c */
int RPG_out_data (int *data_type, int *timing,
                  int *product_code);
int RPG_out_data_wevent (int *data_type, int *timing,
                         EN_id_t *event_id, int *product_code );
int RPG_out_data_by_name_wevent (char *data_name, EN_id_t *event_id, 
                                 int *product_id );
void RPG_get_customizing_info( int *elev_index,
                               short *user_array[][10],
                               int *num_requests,
                               int *status );
void RPG_get_code_from_name( char *data_name, int *pcode );
void RPG_get_code_from_id( int *prod_id, int *pcode );


/* The following functions are defined in rpg_abort_processing.c */
int RPG_abort_processing ( int *reason );
int RPG_abort_datatype_processing ( int *datatype, int *reason );
int RPG_abort_dataname_processing ( char *dataname, int *reason );
int RPG_product_replay_request_response( int pid, int reason, short *dep_params );
int RPG_get_abort_reason( int *reason );
int RPG_abort_request( void *request, int *reason );
int RPG_cleanup_and_abort( int *reason );
void RPG_hari_kiri();
void RPG_abort_task();


/* The following functions are defined in rpg_wait_act.c */
int RPG_wait_act (int *wait_for);
int RPG_wait_for_task (char *task_name);
int RPG_wait_for_any_data (int *wait_for, int *status );
int RPG_allow_suppl_scans( fint *allow_suppl_scans );


/* The following functions are defined in rpg_event_services.c */
void RPG_wait_for_event ();
int RPG_reg_for_internal_event ( int *event_code,
                                 void (*service_routine)(),
                                 int *queued_parameter );
int RPG_reg_for_external_event ( int *event_code,
                                 void (*service_routine)(),
                                 int *queued_parameter );
int RPG_UN_register( int *data_id, LB_id_t *msg_id, 
                     void (*service_routine)() );
   
/* The following functions are defined in rpg_legacy_prod.c */
void RPG_prod_hdr( void *ptr, int *prod_id, int *length, int *status );
void RPG_prod_desc_block( void *ptr, int *prod_id, int *vol_scan_num, int *status );
void RPG_stand_alone_prod( void *ptr, char *string, int *length, int *status );
void RPG_set_dep_params( void *ptr, short *params );


/* The following functions are defined in rpg_product_support.c */
int RPG_get_current_target_elev (int *elev);
int RPG_elev_angle_BAMS_to_deg( unsigned short *elev_bams, float *elev_deg );
void RPG_get_buffer_vol_seq_num( int *bufptr,
                                 unsigned int *ui_vol_num );
void RPG_get_buffer_vol_num( int *bufptr, int *vol_num );
void RPG_get_buffer_vcp_num( int *bufptr, int *vcp_num );
void RPG_get_buffer_elev_index( int *bufptr, int *elev_index );
void RPG_is_buffer_from_last_elev( int *bufptr, int *elev_index,
                                   int *last_elev_index );
void RPG_what_moments ( Base_data_header *hdr, int *refflag, int *velflag, 
                        int *widflag );
int RPG_set_veldeal_bit( short *flag_value );
int RPG_set_product_int( void *loc, void *value ); 
int RPG_get_product_int( void *loc, void *value ); 
int RPG_get_product_float( void *loc, void *value );
int RPG_set_product_float( void *loc, void *value );
int RPG_set_mssw_to_uint( void *loc, unsigned int *value );
void RPG_NINT( float *value, int *nearest_integer );



/* The following functions are defined in rpg_init.c */
int RPG_task_init_c (int *what_based);	
int RPG_init_log_services_c ();	


/* The following functions are defined in rpg_scan_sum.c */
#define RPG_START_TIME			1
#define RPG_END_TIME			2
void RPG_read_scan_summary();
time_t RPG_scan_summary_last_updated();
void RPG_read_volume_status();
time_t RPG_volume_status_last_updated();
void RPG_get_elev_time( int *flag, int *vol_num, int *rda_elev_num,
                        int *the_time, int *the_date );


/* The following functions are defined in rpg_adaptation.c */
int RPG_reg_adpt (int *id, char *buf, int *timing, ...);
int RPG_is_adapt_block_registered (int *block_id, int *status );
int RPG_read_adapt_block( int *block_id, int *status );

/* The following functions are defined in rpg_prod_request.c */
int RPG_get_request (int elev_index, int buf_type, int p_code, 
         	     int *index, short *uarray);	


/* The following function are defined in rpg_timer_services.c */
void RPG_reg_timer( int *parameter, void (*callback)() );
void a3cm40( int *parameter, int *count, int *flag, int *ier );
void a3cm41( int *parameter, int *count, int *flag, int *ier );


/* The following functions are defined in rpg_os_32.c */
int ilbyte (int *i, short j[3000], int *k);
int isbyte (int *i, short *j, int *k);
int btest (unsigned char *data, int *off, int *result);
int btest_short (unsigned char *data, int *off, int *result);
int bclr (unsigned char *data, int *off);	
int bset (unsigned char *data, int *off);	
int date (int *yr);
int iclock (int *sw, int *buf);
int t41194__gettime (double *comp, double *ref, int *ms, short *date);
void sndmsg (char *rcv_name, int *msg, int *status);
void ioerr( fint *pblk, fint *status );
int deflst (int *list, int *size);
int atl (int *value, int *list, int *status);
int abl (int *value, int *list, int *status);
int rtl (int *value, int *list, int *status);
int rbl (int *value, int *list, int *status);
int lstfun (int *fun, int *value, int *list, int *status);
int wait_c (int *delay, int *unit, int *status);
int queue (char *rcv_name, int *parm, int *status);
int itoc_os32( int *value, int *num_chars, char string[12]);
int RPG_clear_msg( int *size, char *message, int *status );
int RPG_send_msg( char *message );


/* The following functions are defined in rpg_itc.c */
int RPG_itc_in (int *itc_id, void *first, void *last, int *sync_prd, ...);
int RPG_itc_out (int *itc_id, void *first, void *last, int *sync_prd, ...);
int RPG_itc_read (int *itc_id, int *status);
int RPG_itc_write (int *itc_id, int *status);
int RPG_itc_callback (int *itc_id, int (*func)());


/* The following functions are defined in RPG_data_access.c */
void RPG_data_access_group( int *group, int *data_id, int *msg_id,
                            void *msg, int *msg_size, int *status );
void RPG_data_access_update( int *group, int *status );
void RPG_data_access_read( int *data_id, void *buf, int *buflen,
                           int *msg_id, int *status );
void RPG_data_access_write( int *data_id, void *msg, int *length,
                            int *msg_id, int *status );

/* The following functions are defined in RPG_prod_compress.c */
void RPG_compress_product( void *bufptr, int *method, int *status );
void RPG_decompress_product( void *bufptr, char **out_bufptr, int *size,
                             int *status );

/* The following functions are defined in RPG_adpt_data_elems.c */
int RPG_reg_ade_callback( int (*update) (), void *, char *, fint *timing, ...);
int RPG_is_ade_callback_reg( int (*update) (), fint *status );
int RPG_DEAU_callback_fx( int lb_fd, LB_id_t msgid, int msglen, char *gp_name );
int RPG_read_ade( fint *callback_id, fint *status );
int RPG_update_all_ade();
int RPG_ade_get_values( char *alg_name, char *value_id, double *values );
int RPG_ade_get_string_values( char *alg_name, char *value_id, char **values );
int RPG_ade_get_number_of_values( char *alg_name, char *value_id );
int RPG_reg_site_info( void * );

/* The following functions are defined in RPG_site_info_callback_fx.c */
int RPG_site_info_callback_fx( void * );

/* The following functions are defined in RPG_vcp_info.c */
int RPG_remap_rpg_elev_index( int *vcp_num, int *elev_index );

#endif 		/* #ifndef RPG_H */
