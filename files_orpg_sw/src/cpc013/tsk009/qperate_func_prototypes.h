/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:02:26 $
 * $Id: qperate_func_prototypes.h,v 1.11 2014/09/02 20:02:26 dberkowitz Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/******************************************************************************
    Filename: qperate_func_prototypes.h

    Description:
    ============
    Declare function protoypes for the QPERATE algorithm.

    Change History
    ==============
    DATE       VERSION  PROGRAMMER      NOTES
    ---------- -------  ----------      ---------------
                0001                    Initial implementation version
    20131205    0002    Dan Berkowitz	Replaced beam_center_top[MAX_AZM] with
                                        beam_edge_top[MAX_AZM] in 
                                        Add_bin_to_RR_Polar_Grid prototype and
                                        compute_IRRate 
    08/21/2014  0003	Nicholas Cooper	For CCR NA14-00252:
					Modified read_HeaderData to include
					surv start and end date/time.
******************************************************************************/

#ifndef QPERATE_FUNC_PROTOTYPES_H
#define QPERATE_FUNC_PROTOTYPES_H

#include <orpg_product.h>           /* RPGP_product_t    */

#include "dp_lib_func_prototypes.h" /* average_time      */
#include "qperate_Consts.h"         /* MX_RNG            */
#include "qperate_list_coord.h"     /* struct listitem_t */
#include "qperate_types.h"          /* QPE_Adapt_t       */


/* Declare function prototypes */

int  dp_precip_callback_fx( void* struct_address );

void get_Adapt(Rate_Buf_t*        rate_out,
               dp_precip_adapt_t* dp_adapt,
               hydromet_prep_t*   hydprep_adapt,
               hydromet_acc_t*    hydacc_adapt,
               hydromet_adj_t*    hydadj_adapt,
               A3136C3_t*         bias_info,
               float              exzone[MAX_NUM_ZONES][EXZONE_FIELDS]);

void read_HeaderData(Compact_dp_basedata_elev* inbuf,
                      int* start_elev_date,     int* start_elev_time,
		      int* end_elev_date,	int* end_elev_time,
                      int* surv_bin_size, 	int* spotblank);

void read_Moments(Compact_dp_basedata_elev* inbuf, int azm, int bin,
                  Moments_t* bin_moments, unsigned short* max_no_of_gates);

float get_moment_value(unsigned char* input,      int bin,
                       float          zero_value, float one_value,
                       unsigned short* max_no_of_gates);

time_t get_elevation_time(int elev_date, int elev_time);

void Add_bin_to_RR_Polar_Grid(Compact_dp_basedata_elev* inbuf,
                              int azm,     int rng,
                              int*         num_bins_filled,
                              float        elev_angle_deg,
                              char         Beam_Blockage[BLOCK_RAD][MX_RNG],
                              Rate_Buf_t*  rate_out,
                              HHC_Buf_t*   hhc_out,
                              float    exzone[MAX_NUM_ZONES][EXZONE_FIELDS],
                              unsigned short* max_no_of_gates,
                              struct listitem_t* *coord_list,
                              short         beam_edge_top[MAX_AZM],
                              float*        RateZ_table);

void build_RR_Polar_Grid(Compact_dp_basedata_elev* inbuf,
                         int*   new_vol,
                         int    elev_ind,        int  elev_angle_tenths,
                         int    max_ntilts,      int* num_bins_filled,
                         int    surv_bin_size,
                         Rate_Buf_t*        rate_out,
                         HHC_Buf_t*         hhc_out,
                         int*               get_next_elev,
                         float           exzone[MAX_NUM_ZONES][EXZONE_FIELDS],
                         unsigned short*    max_no_of_gates,
                         Regression_t*      rates,
                         float*             RateZ_table);

int buildDPR_SymBlk(char **ptrDPR, RPGP_product_t* ptrXDR,
                    int vol_num, int vcp_num, int* length,
                    unsigned short RateData[MAX_AZM][MAX_BINS],
                    Rate_Buf_t* rate_out, Siteadp_adpt_t* sadpt);

int buildDPR_prod(int vol_num, int vcp_num, Rate_Buf_t* rate_out,
                  Siteadp_adpt_t* sadpt);

void Build_RadialComp(RPGP_product_t* ptrXDR,
                      unsigned short RateData[MAX_AZM][MAX_BINS]);

int is_Excluded(float azm_angle, float slant_rng_m, float elev_angle_deg,
                Rate_Buf_t* rate_out,
                float exzone[MAX_NUM_ZONES][EXZONE_FIELDS]);

float compute_IRRate(int azm, int rng, int blocked_percent,
                     Rate_Buf_t* rate_out, Moments_t* bin_moments,
                     short beam_edge_top[MAX_AZM], float* RateZ_table);

void precip_accum_init(time_t first_elev_time, time_t last_elev_time,
                       Rate_Buf_t* rate_out);

void read_Blockage(int elev_angle_tenths,
                   char Beam_Blockage[BLOCK_RAD][MX_RNG]);

long double get_BinArea(int rng);

float compute_RateZ(float Z_processed, Rate_Buf_t* rate_out);

void create_RateZ_table(float** RateZ_table, Rate_Buf_t* rate_out);

float compute_RateKdp(Moments_t* bin_moments, float rate_z,
                      Rate_Buf_t* rate_out);

float compute_RateZ_Zdr(float Z_processed, float Zdr, Rate_Buf_t* rate_out);

int compute_avg_Time(time_t begtime, time_t endtime, time_t* avgtime);

/* 11 Dec 07 - Average Zdr no longer being used
 *
 * float compute_Avg_Zdr(float Zdr_array[][MAX_BINS],
 *                       short hydrClass[][MAX_BINS],
 *                       int az, int rng);
 */

int qperate_terminate(int signal, int flag);

void End_of_volume(Rate_Buf_t* *rate_out, HHC_Buf_t* *hhc_out,
                   time_t first_elev_time, time_t last_elev_time,
                   int vol_num, int vcp_num,
                   Siteadp_adpt_t* sadpt, int check_elev_angle);

int count_neighbors(Rate_Buf_t* rate_out, int azm, int rng,
                    short azm_width, short rng_width);

void print_Moment(int azm, int bin, Moments_t* bin_moments);

void restart_qpe_cb(int fx_parm);

#endif /* QPERATE_FUNC_PROTOTYPES_H */
