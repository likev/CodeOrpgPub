/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:29:59 $
 * $Id: dp_lt_accum_func_prototypes.h,v 1.7 2014/07/29 22:29:59 dberkowitz Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef DP_LT_ACCUM_FUNC_PROTOTYPES_H
#define DP_LT_ACCUM_FUNC_PROTOTYPES_H

#include <limits.h>                 /* INT_MAX                */
#include "dp_lib_func_prototypes.h" /* Convert_Grid_to_DP_Res */
#include "dp_lt_accum_Consts.h"     /* DP_LT_ACCUM_DEBUG      */
#include "dp_lt_accum_types.h"      /* LT_Accum_Supl_t        */
#include "dp_s2s_accum_types.h"     /* S2S_Accum_Supl_t       */

/* DP Long Term Accum function prototypes */

int copy_supplemental(S2S_Accum_Supl_t* s2s_supl,
                      LT_Accum_Supl_t*  lt_supl);

int compute_Top_of_Hour(S2S_Accum_Buf_t*  s2s_accum_buf,
                   LT_Accum_Buf_t*   lt_accum_buf);

int compute_hourly(S2S_Accum_Buf_t*  s2s_accum_buf,
                   LT_Accum_Buf_t*   lt_accum_buf,
                   Circular_Queue_t* hourly_circq);

int compute_storm(S2S_Accum_Buf_t*  s2s_accum_buf,
                  LT_Accum_Buf_t*   lt_accum_buf,
                  Storm_Backup_t*   storm_backup);

int compute_diff(S2S_Accum_Buf_t*  s2s_accum_buf,
                 LT_Accum_Buf_t*   lt_accum_buf,
                 Circular_Queue_t* hourly_diff_circq,
                 Storm_Backup_t*   storm_diff_backup);

int compute_hourly_diff(S2S_Accum_Buf_t*  diff_buf,
                        S2S_Accum_Buf_t*  s2s_accum_buf,
                        LT_Accum_Buf_t*   lt_accum_buf,
                        Circular_Queue_t* hourly_diff_circq,
                        short             legacy_ST_active);

int compute_storm_diff(S2S_Accum_Buf_t*  diff_buf,
                       S2S_Accum_Buf_t*  s2s_accum_buf,
                       LT_Accum_Buf_t*   lt_accum_buf,
                       Storm_Backup_t*   storm_diff_backup,
                       short             legacy_ST_active,
                       time_t            legacy_storm_begtime);

void convert_to_low_res(int grid[][MAX_BINS]);

/* Restore */

int restore_lt_accum(Circular_Queue_t* hourly_circq,
                     Storm_Backup_t*   storm_backup,
                     LT_Accum_Buf_t*   lt_accum_buf,
                     S2S_Accum_Buf_t*  s2s_accum_buf);

int restore_lt_diff(Circular_Queue_t* hourly_diff_circq,
                    Storm_Backup_t*   storm_diff_backup,
                    LT_Accum_Buf_t*   lt_accum_buf,
                    S2S_Accum_Buf_t*  s2s_accum_buf);

int restore_hourly(Circular_Queue_t* hourly_circq,
                   LT_Accum_Buf_t*   lt_accum_buf,
                   S2S_Accum_Buf_t*  s2s_accum_buf);

int restore_hourly_diff(Circular_Queue_t* hourly_diff_circq,
                        LT_Accum_Buf_t*   lt_accum_buf,
                        S2S_Accum_Buf_t*  s2s_accum_buf);


int restore_storm(Storm_Backup_t*  storm_backup,
                  LT_Accum_Buf_t*  lt_accum_buf,
                  S2S_Accum_Buf_t* s2s_accum_buf);

int restore_storm_diff(Storm_Backup_t*  storm_diff_backup,
                       LT_Accum_Buf_t*  lt_accum_buf,
                       S2S_Accum_Buf_t* s2s_accum_buf);

int open_lt_accum_buffers(void);

/* Backup */

int backup_lt_accum(Circular_Queue_t* hourly_circq,
                    Circular_Queue_t* hourly_diff_circq,
                    Storm_Backup_t*   storm_backup,
                    Storm_Backup_t*   storm_diff_backup);

int backup_hourly(Circular_Queue_t* hourly_circq);

int backup_hourly_diff(Circular_Queue_t* hourly_diff_circq);

int backup_storm(Storm_Backup_t* storm_backup);

int backup_storm_diff(Storm_Backup_t* storm_diff_backup);

/* Init */

int init_lt_accum(Circular_Queue_t* hourly_circq,
                  Storm_Backup_t*   storm_backup,
                  LT_Accum_Buf_t*   lt_accum_buf,
                  S2S_Accum_Buf_t*  s2s_accum_buf);

int init_lt_diff(Circular_Queue_t* hourly_diff_circq,
                 Storm_Backup_t*   storm_diff_backup,
                 LT_Accum_Buf_t*   lt_accum_buf,
                 S2S_Accum_Buf_t*  s2s_accum_buf);

int init_hourly(LT_Accum_Buf_t*   lt_accum_buf,
                Circular_Queue_t* hourly_circq,
                int               max_queue);

int init_hourly_diff(LT_Accum_Buf_t*   lt_accum_buf,
                     Circular_Queue_t* hourly_diff_circq,
                     int               max_queue);

int init_storm(LT_Accum_Buf_t* lt_accum_buf,
               Storm_Backup_t*  storm_backup);

int init_storm_diff(S2S_Accum_Buf_t* s2s_accum_buf,
                    LT_Accum_Buf_t*  lt_accum_buf,
                    Storm_Backup_t*  storm_diff_backup);

/* Circular Queue Operators */

int CQ_Initialize (Circular_Queue_t *circular_queue,
                   int data_id, int max_queue);

int CQ_Read_first (Circular_Queue_t* circular_queue,
                   S2S_Accum_Buf_t* s2s_accum_buf);

int CQ_Replace_first (Circular_Queue_t* circular_queue,
                      S2S_Accum_Buf_t* s2s_accum_buf);

int CQ_Delete_first (Circular_Queue_t* circular_queue);

int CQ_Get_time_span (Circular_Queue_t* circular_queue, long* time_span);

int CQ_Add_to_back (Circular_Queue_t* circular_queue,
                    S2S_Accum_Buf_t* s2s_accum_buf,
                    int Biased[][MAX_BINS], int Unbiased[][MAX_BINS]);

int CQ_First_needs_interpolation (Circular_Queue_t *circular_queue,
                                  time_t *interp_time);

int CQ_Trim_To_Hour(Circular_Queue_t *circular_queue,
                    int    Biased[][MAX_BINS],
                    int    Unbiased[][MAX_BINS],
                    int    max_grid,
                    time_t trim_time);

int CQ_Get_Missing_Period(Circular_Queue_t *circq);

int dp_lt_accum_terminate(int signal, int flag);

/* Top of Hour Operators */

int query_DB_TOH(int data_id, time_t start_time, time_t end_time,
             void **query_result, int *number_record_p);

int interpolate_grid_TOH(S2S_Accum_Buf_t *accum, time_t interp_time,
             int direction);

int compute_TOH_accum_grid(void * query_result, int num_record,
                           LT_Accum_Buf_t*   lt_accum_buf);

/* Restart callbacks */

void restart_dp_accum_cb(int fx_parm);
void restart_dp_diff_cb(int fx_parm);

#endif /* DP_LT_ACCUM_FUNC_PROTOTYPES_H */
