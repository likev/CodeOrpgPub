#ifndef CLUTPROD_H
#define CLUTPROD_H

/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2008/08/28 17:35:41 $ */
/* $Id: clutprod.h,v 1.4 2008/08/28 17:35:41 cmn Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#include <rpgc.h>
#include <itc.h>
#include <coldat.h>

/* Macro Definitions */

/* For Reading Map Data. */
#define READ_CLUTTER_MAP		0
#define READ_BYPASS_MAP			1

/* Event IDs. */
#define BM_RECEIVED			1000
#define NW_RECEIVED			2000

/* Timer IDs and Time Interval. */
#define NW_MAP_NOT_COMING		100
#define BYPASS_MAP_NOT_COMING		200
#define WAIT_FOR_MAP			660

/* Product Request Information. */
#define MIN_REQUEST			2
#define MAX_REQUEST			32
#define REQUEST_BIAS			3000
#define MAX_PRODUCTS			4
#define MAX_ORDA_PRODUCTS		5

/* Structure Definitions. */
typedef struct {

    int nw_map_request_pending; 

    int bypass_map_request_pending;

    int product_request_pending; 

    int generate_all_products;

    int unsolicited_nw_received;

} Clutinfo_t;


/* Storage location for local copies of map generation times. */
typedef struct{

   int loc_nwm_gentime; 

   int loc_nwm_gendate; 

   int loc_clm_gentime; 

   int loc_clm_gendate; 

   int loc_bpm_gentime; 

   int loc_bpm_gendate; 

   int loc_bpm_gentime_orda; 

   int loc_bpm_gendate_orda;

   int loc_bpm_cmd_generated;

} Loc_hdr_data_t;


/* Definition for local copy of Notchwidth Map ... Legacy only. */
typedef struct{

   int nwm_avail_legacy;

   RDA_notch_map_t nwm_map_legacy;

} Notchwidth_map_t;

/* Definition for local copy of Clutter Map ... ORDA only. */
typedef struct{

    int clm_avail_orda;

    ORDA_clutter_map_t clm_map_orda;

} Clutter_map_t;

/* Definition for local copy of Bypass Map ... Legacy only. */
typedef struct{

    int bpm_avail_legacy;

    RDA_bypass_map_t clby_map_legacy;

} Cbpm_legacy_t;

/* Definition for local copy of Bypass Map ... ORDA only. */
typedef struct{

    int  bpm_avail_orda;

    unsigned int cmd_generated;

    ORDA_bypass_map_t clby_map_orda;

} Cbpm_orda_t;



/* Global Variables. */
Clutinfo_t Clutinfo;
short Request_list[MAX_ORDA_PRODUCTS];
int Rda_config;
Vol_stat_gsm_t Vol_stat;
Loc_hdr_data_t Local_data;
Coldat_t Coldat;
int CFCprod_id;

#ifdef CLUTPROD_MAIN
short Preset_list_legacy[MAX_PRODUCTS] = { 2,4,3,5 };
short Preset_list_orda[MAX_ORDA_PRODUCTS] = { 2,4,8,16,32 };
#else
short Preset_list_legacy[MAX_PRODUCTS];
short Preset_list_orda[MAX_ORDA_PRODUCTS];
#endif


/* Used for storing various information concerning the maps and
   map availability. */
Cbpm_legacy_t Cbpm_legacy;
Cbpm_orda_t Cbpm_orda;
Notchwidth_map_t Cnwm;
Clutter_map_t Clm;

/* ITC information. */
A304c2 Map_flags;

/* Function Prototypes. */
int Clutprod_get_rda_config( );
int Clutprod_is_wb_connected( int *status_value );
int Clutprod_nw_gen_date_time( int *gen_time, int *gen_date );
int Clutprod_bp_gen_date_time( int *gen_time, int *gen_date );
int Clutprod_read_map( int type );
int Clutprod_handle_volstart( int num_products );
int Clutprod_request_map( int requested_map, char *error_phrase );
int Clutprod_generation_control();
void Clutprod_request_response( int reason_code, short request_bit_map );
int Clutprod_process_request( int queued_parameter );
int Clutprod_process_legacy_request( int queued_parameter );
#ifdef CMD_TEST
int Clutprod_gen_clutter_map();
#endif


#endif
