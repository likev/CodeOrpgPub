/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:07 $
 * $Id: gauge_radar_proto.h,v 1.3 2011/04/13 22:53:07 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef GAUGE_RADAR_PROTO_H
#define GAUGE_RADAR_PROTO_H

#include "gauge_radar_common.h"
#include "gauge_radar_types.h"

#include <dp_Consts.h>            /* MAX_BINS */
#include <dp_precip_8bit_types.h> /* packet16_hdr */

void get_adapt(void);

int task_handler(void);

int get_user_requests(User_array_t * * request_list);

int user_time_to_unix_time(User_array_t * user_request,
              time_t *start_time, time_t *end_time, prod_dep_para_t * prod_dep,
              int year, int month, int day, int hour, int minute, int second);

int write_to_output_product(int type,
                            dua_accum_buf_metadata_t* metadata,
                            User_array_t* request_list,
                            prod_dep_para_t* prod_dep,
                            LT_Accum_Buf_t* lt_accum_buf,
                            int dp[MAX_AZM][MAX_BINS],
                            int pps[MAX_AZM][MAX_2KM_RESOLUTION],
                            int vol_num,
                            int total_vols);

int write_to_files(Gauges_Buf_t* gauges_buf,
                   int dp[MAX_AZM][MAX_BINS],
                   int pps[MAX_AZM][MAX_2KM_RESOLUTION],
                   dua_accum_buf_metadata_t* metadata,
                   int type,
                   char header[HEADER_LEN],
                   char messages[NUM_MSGS][MSG_LEN],
                   int vol_num,
                   int total_vols,
                   int* num_packet1,
                   short packet1_index[MAX_GAUGES],
                   char packet1_txt[MAX_GAUGES][MAX_PACKET1]);

void reset_prod_dep(prod_dep_para_t * prod_dep);

void calculate_statistics(double   gauge_d[MAX_GAUGES],
                          double   radar_d[MAX_GAUGES],
                          stats_t* stats);

void calculate_pps_dp_stats(int do_stats,
                            Gauges_Buf_t* gauges_buf,
                            int pps[MAX_AZM][MAX_2KM_RESOLUTION],
                            int dp[MAX_AZM][MAX_BINS],
                            char messages[NUM_MSGS][MSG_LEN]);

int gauges_tab(char* tab_start,
               char header[HEADER_LEN],
               char messages[NUM_MSGS][MSG_LEN]);

int generate_product_hourly(int year, int month, int day, int hour);

void generate_product_for_a_storm(LT_Accum_Buf_t*       lt_accum_buf,
                                  hyadjscn_large_buf_t* legacy,
                                  User_array_t*         request,
                                  prod_dep_para_t*      prod_dep,
                                  int                   vol_num,
                                  int                   total_vols);

void check_gauge_lat_lon(void);

void add_gauges_block(char* outbuf,
                      int num_packet1,
                      short packet1_index[MAX_GAUGES],
                      char packet1_txt[MAX_GAUGES][MAX_PACKET1]);

void add_features(char* outbuf);

int read_new_gauges(char* gauges_start_day, char* gauges_start_time,
                    char* gauges_end_day,   char* gauges_end_time,
                    Gauge gauges[MAX_GAUGES],
                    short num_gauges,
                    Gauges_Buf_t* gauges_buf);

int query_gauges_web(time_t start_time, time_t end_time,
                     Gauge gauges[MAX_GAUGES],
                     short num_gauges,
                     Gauges_Buf_t* gauges_buf);

void read_gauges(void);

void print_gauges(void);

int add_packet_25(char* buffer, short i_start, short j_start, short radius);

#endif /* GAUGE_RADAR_PROTO_H */
