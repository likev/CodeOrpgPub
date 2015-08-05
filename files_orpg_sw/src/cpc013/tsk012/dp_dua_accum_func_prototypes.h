/*
 * RCS info.
 * $Author: ccalvert $
 * $Date: 2009/03/03 18:12:09 $
 * $Locker:  $
 * $Id: dp_dua_accum_func_prototypes.h,v 1.2 2009/03/03 18:12:09 ccalvert Exp $
 * $State: Exp $
 * $Revision: 1.2 $
 */

#ifndef DP_DUA_ACCUM_FUNC_H
#define DP_DUA_ACCUM_FUNC_H

#include <dp_precip_8bit_types.h>  /* packet16_hdr */
#include "dp_dua_accum_common.h"
#include "dp_dua_accum_types.h"

int task_handler();

int get_user_requests(User_array_t * * request_list);

int user_time_to_unix_time(User_array_t * user_request, 
              time_t *start_time, time_t *end_time, prod_dep_para_t * prod_dep,
              int year, int month, int day, int hour, int minute, int second);

int query_DB(int data_id, time_t start_time, time_t end_time, 
             void **query_result, int *number_record_p); 

int compute_dua_accum_grid(void * query_result, int num_record, 
                           int dua_accum_grid[][MAX_BINS], 
                           dua_accum_buf_metadata_t * metadata_p);

int write_to_output_product(void * dua_accum_grid,
                            dua_accum_buf_metadata_t metadata, 
                            User_array_t * request_list,
                            prod_dep_para_t * prod_dep, 
                            int vol_num,
                            S2S_Accum_Buf_t* inbuf);
        
void reset_prod_dep(prod_dep_para_t * prod_dep);  

#endif
