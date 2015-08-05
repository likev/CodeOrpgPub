/*
 */

#ifndef DP_LIB_FUNC_PROTOTYPES_H
#define DP_LIB_FUNC_PROTOTYPES_H

/******************************************************************************
    Filename: dp_lib_func_prototypes.h

    Description:
    ============
       Declare function prototypes for the Dual Pol library.

    Long term accumulation grids have to be ints because shorts will only hold
    a max of 32767. 1 hr and storm-total accumulation will exceed this value,
    so we need a larger data type. We created the LT_Accum_Buf_t to hold
    these long term accumulations and added grid operators to work on them.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         ----------------
    10/23/2007  0000       Stein              Initial version.
******************************************************************************/

#include "dp_Consts.h"          /* MAX_BINS             */
#include "dp_lt_accum_types.h"  /* hyadjscn_large_buf_t */
#include "dp_s2s_accum_types.h" /* S2S_Accum_Buf_t      */

/* Grid operators */

int add_biased_grids ( int grid1[][MAX_BINS],  int grid2[][MAX_BINS],
                       float bias, int max_grid );

int add_biased_short_to_int ( int grid1[][MAX_BINS],  short grid2[][MAX_BINS],
                              float bias, int max_grid );

void add_unbiased_grids ( int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                          int max_grid );

void add_unbiased_short_to_int ( int grid1[][MAX_BINS], short grid2[][MAX_BINS],
                                 int max_grid );

int subtract_biased_grids ( int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                            float bias );

int subtract_biased_short_from_int ( int grid1[][MAX_BINS],
                                     short grid2[][MAX_BINS],
                                     float bias );

void subtract_unbiased_short_from_int ( int grid1[][MAX_BINS],
                                        short grid2[][MAX_BINS] );

void subtract_unbiased_short_grids ( short grid1[][MAX_BINS], short grid2[][MAX_BINS]);

void multiply_grid_by_C ( short grid1[][MAX_BINS], float multiplier );

void convert_grid_to_DP_res ( short Legacy_grid[][MAX_2KM_RESOLUTION],
                              short DP_grid[][MAX_BINS] );

int convert_int_to_256_char (int    int_grid [][MAX_BINS],
                             int*   min_val,
                             int*   max_val,
                             float* scale,
                             float* offset,
                             int    product_code,
                             unsigned char UC_grid[][MAX_BINS]);

/* 17 Aug 2010 - Dan Stein - added conversion routine for DSA. The product had
 * been rescaling frequently (almost every volume) and some values appeared to
 * decrease. We met with ROC Apps and WDTB and decided to use a different
 * encoding scheme similar to the legacy DSP. DSA will now use a multiplicative
 * scaling factor until the product max exceeds 2 times the user-defined max
 * for the 4-bit storm-total products (STA and STP). This 2 * 4-bit max will
 * serve as a cap on the maximum DSA value.
 */
int convert_DSA_int_to_256_char (int int_grid [][MAX_BINS],
                                 int*   min_val,
                                 int*   max_val,
                                 float* scale,
                                 float* offset,
                                 int    product_code,
                                 unsigned char UC_grid[][MAX_BINS],
                                 int DSA_max);

int compute_datalevel_diffprod (int    int_grid [][MAX_BINS],
                                int*   min_val,
                                int*   max_val,
                                float* scale,
                                float* offset,
                                int    product_code,
                                unsigned char UC_grid[][MAX_BINS]);

int convert_precip_4bit(int int_grid[][MAX_2KM_RESOLUTION],
                        int* min_val,
                        int* max_val,
                        int product_code,
                        short short_grid[][MAX_2KM_RESOLUTION]);

int interpolate_grid( S2S_Accum_Buf_t *accum, time_t interp_time, int direction );

int compare_grids (int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                   char* grid1_name, char* grid2_name, int print_differences);

void print_min_max (int mini, int minj, int min_val,
                    int maxi, int maxj, int max_val,
                    float scale, float offset, int product_code);

int pointer_is_NULL (void* ptr, char* func_name, char* ptr_name);

int bias_is_negative (float bias, char* func_name);

/* Scan-to-scan data stores */

int open_s2s_accum_data_store(int data_id);
int read_s2s_accum_data_store(int data_id, int rec_id, S2S_Accum_Buf_t* accum);
int write_s2s_accum_data_store(int data_id, int rec_id, S2S_Accum_Buf_t* accum);
int rpg_err_to_msg(int rpg_err, char* msg);

int open_s2s_rate_data_store(int data_id);
int read_s2s_rate_data_store(int data_id, int rec_id, Rate_Buf_t* rate);
int write_s2s_rate_data_store(int data_id, int rec_id, Rate_Buf_t* rate);

/* Output check functions, called before product is built */

float check_max_accum(float max_accum);
float check_dua_max_accum(float dua_max_accum);
float check_mean_field_bias(float mean_field_bias);
float check_sample_size(float sample_size);
short check_date(short date);
short check_time(short time);
short check_time_span(short time_span);
float check_min_accum_diff(float min_accum_diff);
float check_max_accum_diff(float max_accum_diff);
unsigned short check_max_inst_precip(unsigned short max_inst_precip);
float check_hybr_rate_filled(float hybr_rate_filled);
float check_highest_elev_used(float highest_elev_used);
short check_mode_filter_size(short mode_filter_size);
int   check_true_false_flag(int flag, char* name);
int   check_null_product(int null_indicator, char* name);

/* Debug prototypes */

void print_generic_moment ( Generic_moment_t* gm );
void print_rate ( Rate_Buf_t* rate );
void print_s2s_accum ( S2S_Accum_Buf_t* accum );
void print_table ( char array[], int size );
void print_hhc ( HHC_Buf_t* hhc );
void print_nonzero (void* array, char* name, int type );
void print_graphic_product ( Graphic_product* hdr );
void print_symbology_block ( Symbology_block* sym );
void print_legacy ( hyadjscn_large_buf_t* legacy );
void test_mode_filter( void );
void print_filter_stats(int num_changed, int num_bins);

void write_buf_to_file ( void* buf, char* filename, long num_bytes );
void read_buf_from_file ( void* buf, char* filename, long num_bytes );

long compare_levels(char* prodbuf1,   char* prodbuf2,
                    char* prod1_name, char* prod2_name);
long compare_Graphic_headers(char* prodbuf1,   char* prodbuf2,
                             char* prod1_name, char* prod2_name);
long compare_Symbology_blocks(char* prodbuf1,   char* prodbuf2,
                              char* prod1_name, char* prod2_name);
long compare_packet_16_headers(char* prodbuf1,   char* prodbuf2,
                               char* prod1_name, char* prod2_name);
long compare_packet_16_data(char* prodbuf1,   char* prodbuf2,
                            char* prod1_name, char* prod2_name,
                            int print_differences);
void compare_8bit_products(char* prodbuf1,   char* prodbuf2,
                           char* prod1_name, char* prod2_name);
void compare_4bit_and_8bit_products(char*  prodbuf1,  char*  prodbuf2,
                                    char* prod1_name, char* prod2_name);

void print_packet16_data_value(char* prodbuf, int azm, int bin);

void get_product_min_max(char* prodbuf, char* prod_name);

void print_mode_array( char* input, int num_bins, char* text );

/* Time Function prototypes */

int UNIX_time_to_julian_mins (time_t time, int* julian_date,
                              int* mins_since_midnight);
time_t average_time (time_t time1, time_t time2);

/* Mode 9 filter prototypes */

void mode_filter(char* input, int num_bins, int filter_size);

/* Regression testing */

void read_RegressionData(Moments_t** reg_moments, Regression_t* rates);
void compare_QPE_Prototype(int azm, int bin, char* type,
                           Stats_t* stats, Regression_t* rates);
void compareRates(Stats_t* stats, Regression_t* rates);
void print_stats(Stats_t* stats);
void Hca_beamMLintersection(float elev, short aznum, float bin_size);

/* Add packets to output buffer */

int make_null_symbology_block(char* outbuf, int null_indicator, char* prodname,
                              int restart_time, time_t last_time_prcp,
                              int rain_time_thresh);

int add_packet_1(char* buffer, short i_start, short j_start, char* text);

#endif /* DP_LIB_FUNC_PROTOTYPES_H */
