/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:47 $
 * $Id: saa_compute_products.h,v 1.6 2008/01/04 20:54:47 aamirn Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*******************************************************************
File    : saa_compute_products.h
Created : Aug. 14,2003
Details : Header file for saa_compute_products.c, which
          computes the SAA products.
Author  : Reji Zachariah
Modification History: Khoi Dang added fucntions( since October 10)
			1.get_ohp_missing_periods
			2.get_usp_missing_periods
			3.get_ohp_duration
			4.check_valid_ohp_duration
			5.initialize_usr_products
			
		      and added structures:
		       1. usr_data_t
		       2. usr_header_t	
			* moved the functions from saa_generate_intermed_products:
			    
			    	int create_LB()
			    	int init_LB( int lbd)
				int get_nextLB()
				int write_LB (int lbd, int msg_id)
				int write_LB_Header ( int lbd, int msg_id)
				int read_LB (int lbd, int msg_id)
				int read_LB_Header (int lbd, int msg_id)
				void scale_usp_outputs()
				void reset_usr_data()
			* added new functions:
				void generate_intermediate_usp_product(int position)
				void get_usp_product(int position)
				int check_usp_duration(int position)
			* added new variable difference_in_pos , int past_clock_hour
			* Added parameters to saa_usp and usr_header structures for
			  CCR NA06-06603
			    
*********************************************************************/
#ifndef SAA_COMPUTE_PRODUCTS_H
#define SAA_COMPUTE_PRODUCTS_H

#include <stdlib.h>
#include <stdio.h>
#include "saa.h"

/*********************CONSTANTS**************************************/

#define SAA_OHP_BUFSIZE 24  /* Size of the One-hour buffer struct.
			     * Size should be (2*thr_time_span)+2,
			     * to accommodate the extrapolated data and
			     * the original data */
			     
#define MSG_SIZE	331200 /*MAX_AZM * MAX_RNG * sizeof(short)*2 */
#define MISSING_DATE    -1
#define MISSING_TIME 	-1
#define NUM_MSGS        30 

enum saa_product_type{ 	    /* Enumeration used in debug printing */
	swe_ohp,
	sd_ohp,
	swe_usp,
	sd_usp,
	swe_total,
	sd_total
};

/***********************DATA DECLARATIONS *********************/
typedef struct {
	float buffer[MAX_AZM][MAX_RNG];
	float missing_period;
	int date;
	int time;
	float duration;
}saa_swe_buffer_t;

typedef struct {
	float buffer[MAX_AZM][MAX_RNG];
}saa_sd_buffer_t;

/*** Structure for One Hour Product ***/
typedef struct {
	saa_swe_buffer_t swe_buffer	[SAA_OHP_BUFSIZE];
	saa_sd_buffer_t  sd_buffer	[SAA_OHP_BUFSIZE];
	int 		 bufferIsEmpty	[SAA_OHP_BUFSIZE];
	int 		 size;  
	int 		 first; 
	int 		 stp_begin_time;/* mod12/3 kd */
	int 		 stp_begin_date;
	float		 stp_missing_period;  /* Added 12/11/2003   WDZ */
}saa_ohp_t;	

/*** Structure for User Selectable Product ***/
typedef struct{
	float   saa_usp_sd_oh   [MAX_AZM][MAX_RNG]; 
	float   saa_usp_swe_oh  [MAX_AZM][MAX_RNG]; 
	int 	bufferIsEmpty	;
	float   duration;
	int 	date;
	int 	time;
    	int 	start_date;   /* Added 03/09/2006  CCR NA06-06603 */
	int	start_time;   /* Added 03/09/2006  CCR NA06-06603 */
	float   missing_period;
	int 	start;
}saa_usp_t;

/*kd 10/26 */
typedef struct 
{
	short swu_data   [MAX_AZM][MAX_RNG]; 
        short sdu_data   [MAX_AZM][MAX_RNG]; 

} usr_data_t ;

/*kd 10/19 */
typedef struct 
{
	short usr_date [NUM_MSGS];
	short usr_time [NUM_MSGS];
	short usr_start_date[NUM_MSGS];      /* Added 03/09/2006  CCR NA06-06603 */
	short usr_start_time[NUM_MSGS];      /* Added 03/09/2006  CCR NA06-06603 */
    	short data_available_flag [NUM_MSGS];
    	short usr_begin_date;
    	short usr_begin_time;
    	short usr_end_date;
    	short usr_end_time;
} usr_header_t;

saa_ohp_t saa_ohp;
saa_usp_t saa_usp;
usr_data_t usr_data;
usr_header_t usr_header;


/* Storm water Equivalent Accumulations */
float saa_swe_oh_total[MAX_AZM][MAX_RNG]; 	/* One Hour Total 	*/
float saa_swe_storm_total[MAX_AZM][MAX_RNG]; 	/* Storm Total 	  	*/

/* Snow Depth  Accumulations */
float saa_sd_oh_total[MAX_AZM][MAX_RNG]; 	/* One Hour Total 	*/
float saa_sd_storm_total[MAX_AZM][MAX_RNG]; 	/* Storm Total 	  	*/

/*kd 10/9 */
float saa_usp_data_at_pos [MAX_AZM][MAX_RNG];   /*1/12/04 */

int difference_in_pos;
int past_clock_hour;/*1/8/04 */
int older_time_closer;
int marked_index;


/***********************FUNCTION DECLARATIONS *********************/
/* Initializes products */
void initialize_products ();
void initialize_usr_products();/*10/26 kd */

/* This function needs to be called for computing products 
 * Date and Time are the end date and end time of the hybrid scan
 * respectively.  If extrapolation is needed, needsExtrapolation
 * flag will be TRUE(1).  Then, the forwardExtrapolationTime and the
 * backwardExtrapolationTime will contain the time for extrapolating
 * forward and backward respectively.  If there is a missing period
 * where no accumulation has been created, that period will be given
 * in missing period. 'duration' refers to the amount of time
 * used for calculating the current accumulation */
void compute_products(
	int date,
	int time,
	int needsExtrapolation,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration
	);

/* Function for debug printing of saa product matrices */
void print_saa_product_matrix(
		enum saa_product_type type,
		int rows, int cols,int rowstart ,int colstart);
		
/*accessor methods */
void get_ohp_begin_date(int* date, int* time_in_minutes);
void get_ohp_end_date(int* date,int* time_in_minutes);
void get_stp_begin_date(int* date, int* time_in_minutes);
void get_stp_end_date(int* date,int* time_in_minutes);
void get_usp_begin_date(int* date, int* time_in_minutes);
void get_usp_end_date(int* date,int* time_in_minutes);
int check_valid_ohp_duration();
float get_ohp_duration(); 
float get_ohp_missing_period();
void get_usp_missing_periods(float* usp_missing_periods);
int create_LB();
int init_LB( int lbd);
int get_nextLB();
int write_LB (int lbd, int msg_id);
int write_LB_Header ( int lbd, int msg_id);
int read_LB (int lbd, int msg_id);
int read_LB_Header (int lbd, int msg_id);




#endif

