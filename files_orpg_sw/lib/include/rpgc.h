/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/25 17:23:54 $
 * $Id: rpgc.h,v 1.77 2014/04/25 17:23:54 steves Exp $
 * $Revision: 1.77 $
 * $State: Exp $
 */

#ifndef RPGC_H
#define RPGC_H

#include <en.h>
#include <basedata.h>
#include <malrm.h>
#include <generic_basedata.h>
#include <orpggdr.h>
#include <rpgc_globals.h>
#include <math.h>
#include <siteadp.h>

/*
#ifndef DEBUG
#define DEBUG     0
#endif
*/

#ifdef __cplusplus
extern "C"
{
#endif

/* 
  The following functions are declared in rpgc_inbuf_c.c 
*/
int RPGC_reg_inputs( int argc, char *argv[] );
int RPGC_in_data( int datatype, ... );
int RPGC_in_opt( int datatype, int block_time );
int RPGC_in_opt_by_name( char *data_name, int block_time );
int RPGC_reg_moments( int moments );
unsigned short RPGC_check_radial_type( void *radptr, unsigned short check_type );
unsigned short RPGC_check_cut_type( void *radptr, unsigned short check_type );
void* RPGC_get_inbuf( int reqdata, int *opstat );
void* RPGC_get_inbuf_by_name( char *reqname, int *opstat );
void* RPGC_get_inbuf_any( int *datatype, int *opstat );
int RPGC_rel_inbuf( void *bufptr );
int RPGC_rel_all_inbufs( );
int RPGC_get_inbuf_len( void *bufptr );
void* RPGC_get_surv_data( void* bufptr, int* first_good_bin, int* last_good_bin );
void* RPGC_get_vel_data( void* bufptr, int* first_good_bin, int* last_good_bin );
void* RPGC_get_wid_data( void* bufptr, int* first_good_bin, int* last_good_bin );
int RPGC_monitor_input_buffer_load( char *dataname );

/* Macro used for "type" in RPGC_get_radar_data. */
#define RPGC_DREF               ORPGGDR_DREF
#define RPGC_DRF2               ORPGGDR_DRF2
#define RPGC_DVEL               ORPGGDR_DVEL
#define RPGC_DSW                ORPGGDR_DSW  
#define RPGC_DPHI               ORPGGDR_DPHI 
#define RPGC_DRHO               ORPGGDR_DRHO 
#define RPGC_DZDR               ORPGGDR_DZDR
#define RPGC_DSNR               ORPGGDR_DSNR
#define RPGC_DRFR               ORPGGDR_DRFR

#define RPGC_DSMZ		ORPGGDR_DSMZ
#define RPGC_DSMV		ORPGGDR_DSMV
#define RPGC_DKDP		ORPGGDR_DKDP
#define RPGC_DSDP		ORPGGDR_DSDP
#define RPGC_DSDZ		ORPGGDR_DSDZ

/* The following type is user-defined type. */
#define RPGC_DANY 		ORPGGDR_DANY

void* RPGC_get_radar_data( void* bufptr, int type, Generic_moment_t *mom );

/* 
  The following functions are declared in rpgc_outbuf_c.c 
*/
int RPGC_reg_outputs( int argc, char *argv[] );
int RPGC_out_data( int datatype, ... );
int RPGC_out_data_wevent( int datatype, en_t event_id, ... );
int RPGC_out_data_by_name_wevent( char *dataname, en_t event_id );
void* RPGC_get_outbuf( int datatype, int bufsiz, int *opstat );
void* RPGC_get_outbuf_by_name( char *reqname, int bufsiz, int *opstat );
void* RPGC_realloc_outbuf( void *bufptr, int bufsiz, int *opstat );
void* RPGC_get_outbuf_for_req( int datatype, int bufsiz, 
                               User_array_t *user_array,
                               int *opstat );
void* RPGC_get_outbuf_by_name_for_req( char *dataname, int bufsiz, 
                                       User_array_t *user_array,
                                       int *opstat );
int RPGC_get_code_from_name( char *data_name );
int RPGC_get_id_from_name( char *data_name );
int RPGC_get_code_from_id( int prod_id );
int RPGC_check_data_by_name( char *data_name );
int RPGC_rel_outbuf( void *bufptr, int disposition, ... );
int RPGC_rel_product( void *bufptr, int size, int disposition );
int RPGC_rel_all_outbufs( int disposition );

/* 
  The following functions are declared in rpgc_wait_act_c.c 
*/
int RPGC_wait_act( int wait_for );
int RPGC_wait_for_any_data( int wait_for );

/*
  The following functions are declared in RPGC_data_access_c.c
*/
int RPGC_data_access_open( int data_id, int flags );
int RPGC_data_access_close( int data_id );
int RPGC_data_access_UN_register( int data_id, LB_id_t msg_id,
                                  void (*callback) () );
int RPGC_data_access_seek( int data_id, int offset, LB_id_t *msg_id );
int RPGC_data_access_read( int data_id, void *buf, int buflen,
                           LB_id_t msg_id );
int RPGC_data_access_write( int data_id, void *buf, int buflen,
                            LB_id_t msg_id );
LB_id_t RPGC_data_access_previous_msgid ( int data_id );
int RPGC_data_access_msg_info( int data_id, LB_id_t id, LB_info *info );
int RPGC_data_access_clear( int data_id, int msgs );
int RPGC_data_access_stat( int data_id, LB_status *status );
int RPGC_data_access_list( int data_id, LB_info *list, int nlist );
int RPGC_get_working_dir( char **path_name );
int RPGC_construct_file_name( char *file_name, char **path_name );

/* Macros defined for event services. */
#define EVT_WAIT_FOR_DATA           -1
#define EVT_NO_WAIT_FOR_DATA        -2

/*
  The following functions are declared in rpgc_event_services_c.c 
*/
int RPGC_reg_for_internal_event( int event_code, void (*service_routine)(),
                                 int queued_parameter );
int RPGC_reg_for_external_event( int event_code, void (*service_routine)(),
                                 int queued_parameter );
int RPGC_wait_for_event();

int RPGC_UN_register( int data_id, LB_id_t msg_id, void (*service_routine)() );

/* 
  The following functions are declared in rpgc_abort_processing_c.c 
*/
int RPGC_abort_remaining_volscan( );
int RPGC_abort( );
int RPGC_abort_because( int reason );
int RPGC_abort_datatype_because( int datatype, int reason );
int RPGC_abort_dataname_because( char *dataname, int reason );
int RPGC_abort_request( User_array_t *request, int reason );
int RPGC_cleanup_and_abort( int reason );
void RPGC_hari_kiri();
void RPGC_abort_task();
int RPGC_product_replay_request_response( int pid, int reason, short *dep_params );

/* 
  The following functions are declared in rpgc_wait_act_c.c 
*/
int RPGC_wait_act( int wait_for );
int RPGC_wait_for_any_data( int wait_for );
int RPGC_allow_suppl_scans();

/* 
  The following functions are declared in rpgc_init_c.c 
*/
typedef int (*RPGC_options_callback_t)( int arg, char *optarg );

int RPGC_reg_io( int argc, char *argv[] );
int RPGC_get_input_stream( int argc, char *argv[] );
int RPGC_init_log_services( int argc, char *argv[] );
void RPGC_log_msg( int code, const char *format, ... );
int RPGC_task_init( int what_based, int argc, char *argv[] );
int RPGC_reg_and_init( int what_based, int argc, char *argv[] );
int RPGC_reg_custom_options( const char *additional_options,
                             RPGC_options_callback_t callback );
void RPGC_kill_rpg();
void RPGC_pthreads_init();

/* 
  The following functions are declared in rpgc_timer_services_c.c
*/
int RPGC_set_timer( malrm_id_t id, int interval );
int RPGC_cancel_timer( malrm_id_t id, int interval );
int RPGC_reg_timer( malrm_id_t id, void (*callback)() );

/* 
  The following functions are declared in rpgc_scan_sum_c.c
*/
#define RPGC_START_TIME			1
#define RPGC_END_TIME			2
int RPGC_reg_scan_summary();
Scan_Summary* RPGC_get_scan_summary( int vol_num );

int RPGC_reg_volume_status( Vol_stat_gsm_t *vol_stat );
int RPGC_read_volume_status( );
int RPGC_get_elev_time( int flag, int vol_num, int rda_elev_num,
                        int *the_date, int *the_time );
int RPGC_avset_last_elev( int vol_num );

/* 
  The following functions are declared in rpgc_legacy_prod_c.c
*/
int RPGC_prod_hdr( void *ptr, int prod_id, int *length );
int RPGC_set_prod_block_offsets( void *ptr, int sym_offset, 
                                 int gra_offset, int tab_offset );
int RPGC_prod_desc_block( void *ptr, int prod_id, int vol_num );
int RPGC_stand_alone_prod( void *ptr, char *string, int *length );
int RPGC_set_dep_params( void *ptr, short *params );
int RPGC_set_date_time( void *atime, void *adate, int the_time, int the_date );
int RPGC_raster_run_length( int nrows, int ncols, short *inbuf, short *cltab,
                            int maxind, short *outbuf, int obuffind,
                            int *istar2s );
int RPGC_run_length_encode( int start, int delta, short *inbuf,
                            int startix, int endix, int numbufel,
                            int buffstep, short *cltab, int *nrleb,
                            int buffind, short *outbuf );
int RPGC_run_length_encode_byte( int start, int delta, unsigned char *inbuf,
                                 int startix, int endix, int numbufel,
                                 int buffstep, short *cltab, int *nrleb,
                                 int buffind, short *outbuf );

#define RPGC_BYTE_DATA                   8
#define RPGC_SHORT_DATA                  16
#define RPGC_INT_DATA                    32

int RPGC_digital_radial_data_hdr( int first_bin_idx, int num_bins,
                                  int icenter, int jcenter,
                                  int range_scale_factor, int num_radials,
                                  void *output );
int RPGC_digital_precipitation_data_hdr( int lfm_boxes_in_row, int num_rows,
                                  void *output );
int RPGC_digital_radial_data_array( void *input, int input_size, 
                                    int start_data_idx, int end_data_idx, 
                                    int start_idx, int end_idx, int binstep, 
                                    int start_angle, int delta_angle, 
                                    void *output );
int RPGC_digital_precipitation_data_array( void *input, int input_size,
                                           int lfm_boxes_in_row,
                                           int num_rows, void *output );


/* 
  The following functions are declared in rpgc_prod_support_c.c
*/
int RPGC_get_current_vol_num();
int RPGC_get_current_elev_index();
int RPGC_get_buffer_vol_num( void *bufptr );
unsigned int RPGC_get_buffer_vol_seq_num( void *bufptr );
int RPGC_get_buffer_vcp_num( void *bufptr );
int RPGC_get_buffer_elev_index( void *bufptr );
int RPGC_is_buffer_from_last_elev( void *bufptr, int *elev_index, int *last_elev_index );
void RPGC_what_moments( Base_data_header *hdr, int *ref_flag, int *vel_flag,
                        int *wid_flag );
int RPGC_set_product_int( void *loc, unsigned int value );
int RPGC_get_product_int( void *loc, void *value );
int RPGC_set_product_float( void *loc, float value );
int RPGC_get_product_float( void *loc, void *value );
int RPGC_num_rad_bins( void *bdataptr, int maxbins, int radstep,
                       int wave_type );
int RPGC_bins_to_ceiling( void *bdataptr, int bin_size );


#define RPGC_BYTE_DATA                   8
#define RPGC_SHORT_DATA                  16
#define RPGC_INT_DATA                    32

int RPGC_digital_radial_data_hdr( int first_bin_idx, int num_bins,
                                  int icenter, int jcenter,
                                  int range_scale_factor, int num_radials,
                                  void *output );
int RPGC_digital_precipitation_data_hdr( int lfm_boxes_in_row, int num_rows,
                                  void *output );
int RPGC_digital_radial_data_array( void *input, int input_size, 
                                    int start_data_idx, int end_data_idx, 
                                    int start_idx, int end_idx, int binstep, 
                                    int start_angle, int delta_angle, 
                                    void *output );
int RPGC_digital_precipitation_data_array( void *input, int input_size,
                                           int lfm_boxes_in_row,
                                           int num_rows, void *output );


#ifdef LINUX
#define RPGC_NINT roundf
#define RPGC_NINTD round
#else
float RPGC_NINT( float value );
double RPGC_NINTD( double value );
#endif

/*
  The following functions are defined in rpgc_prod_compress_c.c 
*/
int RPGC_compress_product( void *bufptr, int method );
void* RPGC_decompress_product( void *bufptr );

/* 
  The following functions are defined in rpgc_prod_request_c.c
*/
int RPGC_get_request( int elev_index, int prod_id, User_array_t *uarray,
                      int max_requests );
int RPGC_get_request_by_name( int elev_index, char *dataname, 
                              User_array_t *uarray, int max_requests );
void *RPGC_get_customizing_data( int elev_index, int *n_requests );
int RPGC_get_customizing_info( int elev_index, User_array_t *user_array,
                               int *num_requests );
int RPGC_register_req_validation_fn( char *prod_name,
                                     int (*fn)(User_array_t *user_array) );
int RPGC_check_data( int prod_id );

/*
  The following functions are defined in rpgc_itc_c.c
*/
int RPGC_itc_in( int itc_id, void *first, unsigned int size, int sync_prod, ... );
int RPGC_itc_out( int itc_id, void *first, unsigned int size, int sync_prod, ... );
int RPGC_itc_callback( int itc_id, int (*func)() );
int RPGC_itc_read( int itc_id, int *status );
int RPGC_itc_write( int itc_id, int *status );

/*
  The following functions are defined in rpgc_adpt_data_elems_c.c
*/
int RPGC_reg_ade_callback( int (*update) (), void *, char *, int timing, ...);
int RPGC_is_ade_callback_reg( int (*update) (), int *status );
int RPGC_read_ade( int *callback_id, int *status );
int RPGC_update_all_ade();
int RPGC_ade_get_values( char *alg_name, char *value_id, double *values );
int RPGC_ade_get_string_values( char *alg_name, char *value_id, char **values );
int RPGC_ade_get_number_of_values( char *alg_name, char *value_id );
int RPGC_reg_site_info( void *struct_address );
int RPGC_reg_color_table( void *buf, int timing, ... );
int RPGC_reg_RDA_control( void *buf, int timing, ... );

/*
  The following functions are defined in rpgc_adaptation.c 
*/
int RPGC_reg_adpt( int id, char *buf, int timing, ...);
int RPGC_is_adapt_block_registered( int block_id, int *status );
int RPGC_read_adapt_block( int block_id, int *status );
void RPGC_update_adaptation ();

/*
  The following functions are defined in rpgc_volume_radial_c.c
*/
int RPGC_reg_volume_data( int data_id );
int RPGC_check_volume_radial( unsigned int vol_seq_num, int elev_index, 
                              unsigned int type );
char* RPGC_read_volume_radial( unsigned int vol_seq_num, int elev_index, 
                               unsigned int *sub_type, int *size );
void RPGC_rel_volume_radial( char* bufptr );


typedef struct RPGC_prod_rec {

   int msg_id;		/* Message ID as stored in the product database LB. */

   int vol_time;	/* Volume time of product, UTC. */

   int elev;		/* Elevation angle of product, if applicable, in deg*10. */

   int elev_ind;	/* Elevation index of product, if applicable. */

   short params[6];	/* product parameters. */

} RPGC_prod_rec_t;

/*
  The following functions are defined in rpgc_prod_functions.c 
*/
int RPGC_find_pdb_product( char *name, RPGC_prod_rec_t *results, int max_results, ... );
void* RPGC_get_pdb_product( char *name, LB_id_t msg_id );
void RPGC_release_pdb_product( void *buf );


/*
  The following functions are defined in rpgc_database_query_funcs.c
*/
int RPGC_DB_select( int data_id, char *where, void **result );
int RPGC_DB_insert( int data_id, void *record, int rec_len );
int RPGC_DB_delete( int data_id, char *where );
int RPGC_DB_get_record( void *result, int ind, char **record );
int RPGC_DB_get_header( void *result, int ind, char **hd );

/* 
  The following functions are defined in rpgc_blockage_data_funcs.c.
*/
unsigned char *RPGC_blockage_data_lookup( float elev_ang, float az_ang );


#ifdef __cplusplus
}
#endif

#endif
