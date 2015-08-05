/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:46 $
 * $Id: saa_compute_products.c,v 1.9 2008/01/04 20:54:46 aamirn Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/*********************************************************************
File    : saa_compute_products.c
Created : Aug. 14,2003
Details : Computes the one-hour,storm total, and user selectable products
          for SAA.
Author  : Reji Zachariah & Khoi Dang
Modification History: Khoi Dang added functions (since October 10)
			1. add_to_usp_total
			2. get_oh_usp_total
			3. initialize_usp_buffer
			4. get stp_begin_date
			5. check_valid_ohp_duration
			6. get_ohp_duration
			7. get_stp_end_date
			8.reset_ohp_buffer_at_position
			9.get_usp_missing_periods
		       These following functions are modified by Khoi Dang and Dave Zittel
		                 compute_ohp_storm_total_products
				 removeOld_OHP_entries
				 extrapolate_ohp_accum
				 compute_usp_products
				 get_new_usp_position
				 get_ohp_end_date
				 get_ohp_begin_date
			* deleted reset_ohp_buffer_at_position (December 18) because it is no longer needed
			* modified the get_usp_position and compute_usp_products (1/6/04). To get the position
			  of the mots recently_added accumulation in reference to 'First', we created a new global 
			  variable difference_in_pos . Hence, the two boolean flags back_two_cells was nolonger needed.The
			  block of code then is more compact and faster.Instead of checking the valid hour in the 
			  get_new_usp_position function, we moved it to the function compute_usp_product.
			 
			 * created 4 new functions: kd 1/08/04
			     generate_intermediate_usp_product(int )
			     check_usp_duration(int )
			     get_usp_product (int)
			     get_usp_accumulation(int)
			 * create new variable int past_clock_hour
			 * moved several functions from saa_generate_intermed_products into this module
			 * revised the get_new_usp_position and compute_usp_product.The past_clock_hour variable is 
			   devised to inform whenever the scan time past the clock hour. Depending on the status of 
			   this flag, proper actions will be taken to generate 1-hour usp product to Linear Buffer
			   The get_new_usp_position will return the position that hold the accumulated data and also
			   set the past_clock_hour to TRUE if the time crossed the clock hour. Then,whenever
			   the get_new_usp_position is called and the data is updated, check if the past_hour_clock is set
			   If so, check the valid duration time and if it is valid, get the accumulation data at the correct position
			   and generate usp product.
			 * added function check_past_clock_hour_to_get_usp_product kd 1/9/04
			 * implemented the look-back process kd 1/13/04.Added 10 functions:
			 	1. int get_start_time_of_volume_before_last_first_volume(int current_time, float duration);
				2. float get_usp_duration_at_pos(int position);
				3. int get_usp_time_at_pos (int position);
				4. int get_ohp_index_at_pos (int time);
				5. void get_usp_data_at_pos(int position);
				6. float get_ohp_duration_at_pos (int position);
				7. int get_valid_data_from_volume_before_last_first_volume(int position);
				8. int  get_index(int position);
				9. int check_valid_time_span (int position);
				10. void get_second_chance_usp_product(int position);
			*modified the get_start_time_of_volume_before_last_first_volume function so that the 
			 fact that the date has changed is also accounted for.
			* added function void set_look_back_index () (kd 1/23/04)
			*revised get_second_chane_usp_products (kd 1/23/04)
			*revised get_new_usp_position so that SAA_USP_BUFSIZE now can be set to 4
			* Changed the structure saa_usp_t ( SAA_USP_BUFSIZE is no loger used)
			  Renewed  replace the get_new_usp_position function by get_case 2/4/04
			  Revised  some functions for the revision (those that related
			   to manipulating saa_usp )
			* added function process_case
			* commented out get_oh_usp_total/get_usp_accumulation
			* added ForwardExtrapolate flag to passed parameter list for process_case 
			  function (3/23/2004 WDZ)
			* Added changes to save each hour's starting date and time for 
			  for CCR NA06-06603
			* Inactivated 3 debug statements for Build 10 (9/25/2007  WDZ)

*********************************************************************/
#include "saa_compute_products.h"
#include "saa_compute_time_span.h"
#include "input_buffer_struct.h"
#include <math.h>
#include "saa_main.h"
#include "saa.h"
#include "saa_comp_accum.h"
#include "saa_file_io.h"
#include <math.h>
#include "saa_generate_intermed_products.h"




/*************************** VARIABLES *******************************/

float max_usp_swe_accum=0;
float max_usp_sd_accum=0;

/****************STATIC VARIABLES ************************************/
static float tolerance = 0.0001;


/***************FUNCTION DECLARATIONS**********************************/
void removeOld_OHP_Entries(int date,int time);
void extrapolate_ohp_accum(
	int date,
	int time,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration);
void compute_ohp_storm_total_products(
	int date,
	int time,
	int needsExtrapolation,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration);
void compute_usp_products(
	int date,
	int time, 
	int needsExtrapolation,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration);
	
int  time_crossed_clock_hour(int date, int time);
void initialize_usp_buffer();
void add_to_swe_sd_totals(
	float swe_equivalent,
	int azimuth,
	int range);
void add_to_usp_total(
	float swe_equivalent,
	int azimuth,
	int range);
	
/*void get_oh_usp_total(int azimuth,int range);	*/

int check_usp_duration();
/*void get_usp_accumulation(); */

void scale_usp_outputs();
void reset_usr_data();
void get_usp_product();
int check_past_clock_hour_to_get_usp_product ();
void generate_intermediate_usp_product();
float get_usp_duration();
int get_usp_time ();
void get_usp_data_at_pos(int position);
float get_ohp_duration_at_pos (int position);
int check_valid_time_span (int position);
void get_second_chance_usp_product();
void set_look_back_index();
void print_SAA_USP(void);
void reset_usp_accumulation (void);
void process_case (int case_in, int date, int time, float duration, float missing, int ForwardExtraFlg);
int  get_case(int newdate,int newtime);

/***************FUNCTION DEFINITIONS***********************************/

/**********************************************************************
Method : initialize_products
Details: initializes the structs and products
**********************************************************************/
void initialize_products (){

	int i,j;	/*loop variables */
	

	/*initializing the OHP buffer struct */
	for(i = 0;i<SAA_OHP_BUFSIZE;i++){
		saa_ohp.bufferIsEmpty[i] 		= TRUE;
		saa_ohp.swe_buffer[i].date		= 0;
		saa_ohp.swe_buffer[i].time		= 0;
		saa_ohp.swe_buffer[i].duration		= 0.0;
		saa_ohp.swe_buffer[i].missing_period	= 0.0;
	}
	saa_ohp.size 					= 0;
	saa_ohp.first 					= 0;
       
    	/*initializing the USP buffer struct */
	
	initialize_usp_buffer();
	/*old_time=saa_usp.time; */
	/*old_date=saa_usp.date; */
	
	/*initialize the matrices */
	for(i = 0;i<MAX_AZM;i++){
		for(j = 0;j<MAX_RNG;j++){
			saa_swe_oh_total[i][j] 		= 0.0;
			saa_sd_oh_total [i][j]  	= 0.0;/*11/4 */
			saa_swe_storm_total[i][j] 	= 0.0;
			saa_sd_storm_total [i][j]	= 0.0;/*11/4 */
			
		}/*end for */
	}/*end for */
	
}/*end initialize_accumulations */
/**********************************************************************
Method : initialize_products kd 10/26/03
Details: initializes the structs and products Revised 2/3/04
**********************************************************************/
void initialize_usr_products (){

	initialize_usp_buffer();
	
	
}/*end initialize_accumulations */





/**********************************************************
Method : compute_products
Details: the function that can be called from outside
	 to compute the products.  This function directs
	 calls to other functions that compute products revised 2/3/04
***********************************************************/
void compute_products(
	int date,
	int time,
	int needsExtrapolation,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration)
{

	if(SAA_DEBUG) { 
		fprintf(stderr,"USP buffer as compute_usp_product is called\n"); 
		print_SAA_USP();
	}
		
	
	/*compute the user selectable product */
	compute_usp_products(date,time,needsExtrapolation,
		forwardExtrapolationTime,backwardExtrapolationTime,missing_period,duration);
	
	/*compute OHP and storm total products */
	if(SAA_DEBUG){fprintf(stderr,"Before compute_ohp_storm_total, Missing Period = %7.4f\n",
	                       missing_period);}
	      
	compute_ohp_storm_total_products(date,time,needsExtrapolation,
		forwardExtrapolationTime,backwardExtrapolationTime,missing_period,duration);
	if(SAA_DEBUG){fprintf(stderr,"After compute_ohp_storm_total, Missing Period = %7.4f\n",
	                       missing_period);}
	
	
}/*end compute_products */




/**********************************************************
Method : compute_ohp_storm_total_products
Details: Computes OHP and Storm Total Products
***********************************************************/
void compute_ohp_storm_total_products(
	int date,
	int time,
	int needsExtrapolation,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration)
{

	int i,j;
	int newLocation;
	float outdata;
	

	
	/*remove old entries if necessary so that the OHP contains */
	/*totals for only one hour and release some unnecessary cells */
	removeOld_OHP_Entries(date,time);
	
	/*if the buffer is full, this buffer
	  cannot maintain an hour's worth of data - that is an error (this has to be
	  fixed by increasing SAA_OHP_BUFSIZE). */
	if(saa_ohp.size >= SAA_OHP_BUFSIZE){
		LE_send_msg(GL_ERROR,"SAA:compute_One_Hour_Product - Buffer too small for one hour data.\n");
		return;
	}/*end if */
	
	if(needsExtrapolation){
		/*if needsExtrapolation flag is set,
		  extrapolate the data backwards and forwards. */
		extrapolate_ohp_accum(
			date,
			time,
			forwardExtrapolationTime,
			backwardExtrapolationTime,
			missing_period,
			duration);
			
	}/*end if */
	
	/*remove old entries if necessary so that the OHP contains
	  totals for only one hour */
	removeOld_OHP_Entries(date,time);
	
	if(SAA_DEBUG ){
		if(saa_ohp.size > 0){
			fprintf(stderr,"Time Difference in OHP: %3.3f\n",saa_compute_time_span(
				(saa_ohp.swe_buffer[saa_ohp.first]).date,
				(saa_ohp.swe_buffer[saa_ohp.first]).time,
				date,
				time));
		}/*end if */
		fprintf(stderr,"Number of Elements currently in OHP : %d\n",saa_ohp.size);						
		
	}/*end if */
	
	/*determine the location to place the new data  */
	newLocation = (saa_ohp.first + saa_ohp.size)%SAA_OHP_BUFSIZE;
	
	/*fill in the data to the current location and add it to one hour total */
	for(i=0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){	
			outdata = scan_to_scan_swe[i][j];
			(saa_ohp.swe_buffer[newLocation]).buffer[i][j]
						= outdata;
			
			(saa_ohp.sd_buffer[newLocation]).buffer[i][j]
						= outdata*saa_adapt.s_w_ratio;
			
			add_to_swe_sd_totals(outdata,i,j);
			
		}/*end for */
	}/*end for */
	
	/*print one element */
	if(SAA_DEBUG){
		fprintf(stderr,"(100,175) in the new buf: %3.4f\n",saa_ohp.swe_buffer[newLocation].buffer[100][175]);
	}
	/*increment the size and update other variables */
	saa_ohp.size 					= saa_ohp.size + 1;
	saa_ohp.bufferIsEmpty[newLocation] 		= FALSE;
	saa_ohp.swe_buffer[newLocation].date 		= date;
	saa_ohp.swe_buffer[newLocation].time 		= time;
	saa_ohp.swe_buffer[newLocation].duration       += duration;
	
	saa_ohp.swe_buffer[newLocation].missing_period  = missing_period;
	
	/*  Accumulate the missing times for the storm total product */
	saa_ohp.stp_missing_period += missing_period;   /* Added 12/11/2003  WDZ  */
	if(SAA_DEBUG)
	   fprintf(stderr,"STP_missing_period = %7.4f\n",saa_ohp.stp_missing_period);

	/*cut the missing period if it exceeds a lot kd 11/8/03 */
	 
	 if( (missing_period + saa_ohp.swe_buffer[newLocation].duration + forwardExtrapolationTime) >= 1.0){
		missing_period = missing_period - (saa_ohp.swe_buffer[newLocation].duration + forwardExtrapolationTime);
	} 
	
	/*if(SAA_DEBUG){
		fprintf(stderr,"After adding new element...\n");
		fprintf(stderr,"first position : %d,size : %d,new : %d\n",saa_ohp.first,saa_ohp.size,newLocation);
		
		//print_saa_product_matrix(swe_ohp,5,5,100,175);
		
	}*/
	
}/*end compute_ohp_storm_total_products */

/*********************************************************  
 Method  : removeOld_OHP_Entries
 Details : removes old entries from OHP total so that it contains
 	   only one hour data
*********************************************************/
void removeOld_OHP_Entries(int date,int time){

	int i,j;
	float swe_difference,sd_difference;
	
	/*return if there are no entries left */
	if(saa_ohp.size == 0){
		return;
	}/*end if */
	/*should change to >1 that the look-back process works fine */
	while(saa_compute_time_span(	(saa_ohp.swe_buffer[saa_ohp.first]).date,
					(saa_ohp.swe_buffer[saa_ohp.first]).time,
					date,
					time)
	      > 1.0 )
	{
		
		if(SAA_DEBUG){fprintf(stderr, "Removing element at %d from ohp.\n",saa_ohp.first);}
		
		/*print the first element */
		if(SAA_DEBUG){
			fprintf(stderr,"(100,175) element in removed buf: %3.4f\n",saa_ohp.swe_buffer[saa_ohp.first].buffer[100][175]);
		}
		
		if(saa_ohp.bufferIsEmpty[saa_ohp.first] == FALSE){
			
			/*subtract the oldest entry from the total */
			for(i=0;i<MAX_AZM;i++){
				for(j=0;j<MAX_RNG;j++){	
					swe_difference 	 = saa_swe_oh_total[i][j] -
								(saa_ohp.swe_buffer[saa_ohp.first]).buffer[i][j];
								
					sd_difference 	 = saa_sd_oh_total[i][j] -
								(saa_ohp.sd_buffer[saa_ohp.first]).buffer[i][j];
					
					/*check if the absolute value of difference is less than tolerance. */
					/*This check is put in place because One hour Total sometimes is  */
					/*producing negative values (-0.00000..)when this function is called.  It is probably */
					/*because of truncation errors.  			 */
					if(swe_difference < 0.0){
						swe_difference = swe_difference * (-1.0);
						
						/*print the values if they yield a negative value */
						if(SAA_DEBUG){
							if(swe_difference < 0.0){
								fprintf(stderr,"Negative value(OH total):%3.5f ", swe_difference);
								fprintf(stderr,"Subtracted value:%3.5f\n", 
									(saa_ohp.swe_buffer[saa_ohp.first]).buffer[i][j]);
							
							}
						}	
						
					}
					if(swe_difference < tolerance){
						saa_swe_oh_total[i][j] = 0.0;
					}
					else{
						saa_swe_oh_total[i][j] = swe_difference;
					}
					
					if(fabs(sd_difference) < tolerance){
						saa_sd_oh_total[i][j] = 0.0;
					}
					else{
						saa_sd_oh_total[i][j] = sd_difference;
					}
					
					
				}/*end for */
			}/*end for */
		
		}/*end if */
		
		else{
			if(SAA_DEBUG){
				fprintf(stderr,"Remove old entries: buffer is  empty \n");
			}
		}
		/*update the variables	 */
		saa_ohp.bufferIsEmpty[saa_ohp.first] 			= TRUE;
		saa_ohp.swe_buffer[saa_ohp.first].date 			= 0;
		saa_ohp.swe_buffer[saa_ohp.first].time       		= 0;
		saa_ohp.swe_buffer[saa_ohp.first].duration   		= 0.0;
		saa_ohp.swe_buffer[saa_ohp.first].missing_period 	= 0.0;
		saa_ohp.first 						= (saa_ohp.first + 1) % SAA_OHP_BUFSIZE;
		saa_ohp.size  						= saa_ohp.size - 1;
		
		/*return if there are no entries left */
		if(saa_ohp.size == 0){
			return;
		}/*end if */
	
	}/*end while				 */

}/*end removeOld_OHP_Entries */

/*********************************************************  
 Method  : extrapolate_ohp_accum
 Details : extrapolates the ohp and storm total accumulation.
*********************************************************/
void extrapolate_ohp_accum(
	int date,
	int time,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration)
    	{

	int lastPosition ;	/*last entry position of the ohp struct */
	int newPosition;
	int i,j; 		/*loop variables */
	int forward_exp_endtime, forward_exp_enddate;
	int backward_exp_endtime, backward_exp_enddate;
	float swe_equivalent;
	
	/*if the size is zero, then nothing needs to be done. */
	if(saa_ohp.size == 0){
		return;
	}
	
	/*mod 11/14  */
	/*get last position and new position */
	
	lastPosition = (saa_ohp.first + saa_ohp.size - 1) % SAA_OHP_BUFSIZE;
	newPosition  = (lastPosition + 1) % SAA_OHP_BUFSIZE;
	
	/*compute the endTime (seconds from midnight) and endDate (julian) for forward extrapolation */
	forward_exp_endtime = (saa_ohp.swe_buffer[lastPosition].time + (int)(forwardExtrapolationTime*SEC_IN_HOUR)) % SEC_IN_DAY;
	forward_exp_enddate = saa_ohp.swe_buffer[lastPosition].date;
	
	/*adjust for date change */
	if(forward_exp_endtime < saa_ohp.swe_buffer[lastPosition].time){
		forward_exp_enddate = saa_ohp.swe_buffer[lastPosition].date + 1;
	}/*end if */
	
	/*compute the endTime (seconds from midnight) and endDate (julian) for backward extrapolation */
	
	if(time > (int)(backwardExtrapolationTime*SEC_IN_HOUR)){
		/*same day */
		backward_exp_endtime = (time -  (int)(backwardExtrapolationTime*SEC_IN_HOUR)) % SEC_IN_DAY;
		backward_exp_enddate = date;
	}
	else{
		backward_exp_endtime = time + SEC_IN_DAY - (int)(backwardExtrapolationTime*SEC_IN_HOUR);
		backward_exp_enddate = saa_ohp.swe_buffer[lastPosition].date - 1;
	}/*end if */
	
	if(SAA_DEBUG){
		fprintf(stderr,"Forward Ex.plated date: %d, time: %d, ",forward_exp_enddate,forward_exp_endtime);
		fprintf(stderr,"Backward Ex.plated date: %d, time: %d, ",backward_exp_enddate,backward_exp_endtime);
	}
	/*************************Forward Extrapolation********************************************** */
	
	/*if the forward extrapolated date and time results in a one hour total to have more than */
	/*one hour of data, then some of the first entries may need to be removed. */
	removeOld_OHP_Entries(forward_exp_enddate, forward_exp_endtime);
	
	/*extrapolate forward */
	for(i = 0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){
			swe_equivalent = previous_snow_rate[i][j]*forwardExtrapolationTime;
			saa_ohp.swe_buffer[newPosition].buffer[i][j] = swe_equivalent;
			saa_ohp.sd_buffer[newPosition].buffer[i][j]  = swe_equivalent * saa_adapt.s_w_ratio;
			add_to_swe_sd_totals(swe_equivalent,i,j);
			
		}/*end for */
	}/*end for */
	
	
	saa_ohp.size 					= saa_ohp.size + 1;
	saa_ohp.bufferIsEmpty[newPosition] 		= FALSE;
	saa_ohp.swe_buffer[newPosition].date 		= forward_exp_enddate;
	saa_ohp.swe_buffer[newPosition].time 		= forward_exp_endtime;
	saa_ohp.swe_buffer[newPosition].duration 	+= forwardExtrapolationTime;
	
	if(SAA_DEBUG){fprintf(stderr,"Forward extrapolation done.\n");}
	
	/*************************Backward Extrapolation********************************************** */
	
	/*compute the new position */
	newPosition  = (saa_ohp.first + saa_ohp.size) % SAA_OHP_BUFSIZE;
	
	/*if the backward extrapolated date and time results in a one hour total to have more than */
	/*one hour of data, then some of the first entries may need to be removed. */
	removeOld_OHP_Entries(backward_exp_enddate, backward_exp_endtime);
	
	/*extrapolate backward */
	for(i = 0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){
			swe_equivalent = snow_rate[i][j]*backwardExtrapolationTime;
			saa_ohp.swe_buffer[newPosition].buffer[i][j] = swe_equivalent;
			saa_ohp.sd_buffer[newPosition].buffer[i][j]  = swe_equivalent * saa_adapt.s_w_ratio;
			add_to_swe_sd_totals(swe_equivalent,i,j);
			
		}/*end for */
	}/*end for */
	
	/*todo */
	saa_ohp.size 					= saa_ohp.size + 1;
	saa_ohp.bufferIsEmpty[newPosition] 		= FALSE;
	saa_ohp.swe_buffer[newPosition].date 		= backward_exp_enddate;
	saa_ohp.swe_buffer[newPosition].time 		= backward_exp_endtime;
	saa_ohp.swe_buffer[newPosition].duration 	+= backwardExtrapolationTime;
	
	if(SAA_DEBUG){fprintf(stderr,"Backward extrapolation done.\n");}
	

}/*extrapolate_ohp_accum */
/*********************************************************  
 Method  : add_to_swe_sd_totals
 Details : adds snow water equivalent (and the corresponding snow depth)
           to swe (and sd) one hour and storm total outputs
*********************************************************/
void add_to_swe_sd_totals(float swe_equivalent,int azimuth,int range){

	int i = azimuth;
	int j = range;
	
	/*add to one hour totals */
	saa_swe_oh_total[i][j] 	= saa_swe_oh_total[i][j] + swe_equivalent;		
	if(saa_swe_oh_total[i][j] > SAA_ACCUM_CUTOFF){
		saa_swe_oh_total[i][j] = SAA_ACCUM_CUTOFF;
	}
			
	saa_sd_oh_total[i][j] 	= saa_sd_oh_total[i][j] + swe_equivalent * saa_adapt.s_w_ratio;
	if(saa_sd_oh_total[i][j] > SAA_ACCUM_CUTOFF){
		saa_sd_oh_total[i][j] = SAA_ACCUM_CUTOFF;
	}
			
	/*add to the storm total */
	saa_swe_storm_total[i][j] = saa_swe_storm_total[i][j] + swe_equivalent;
		
	if(saa_swe_storm_total[i][j] > SAA_ACCUM_CUTOFF){
		saa_swe_storm_total[i][j] = SAA_ACCUM_CUTOFF;
	}
			
	saa_sd_storm_total[i][j] = saa_sd_storm_total[i][j] + swe_equivalent * saa_adapt.s_w_ratio;	
	
	if(saa_sd_storm_total[i][j] > SAA_ACCUM_CUTOFF){
		saa_sd_storm_total[i][j] = SAA_ACCUM_CUTOFF;
	}

}/*end add_to_swe_sd_totals */
/*********************************************************  
 Method  : compute_usp_products
 Details : computes the user selectable products revised 2/3/04 kd
*********************************************************/
void compute_usp_products(
	int date,
	int time,
	int needsExtrapolation,
	float forwardExtrapolationTime,
	float backwardExtrapolationTime,
	float missing_period,
	float duration)
{

	/*int i,j; 		//loop variables */
	int forward_exp_endtime, forward_exp_enddate;
	int backward_exp_endtime, backward_exp_enddate;
	int ForwardExtrapolate; 			/* Added 11/05/2004 for Build8 */
	/*float	swe_equivalent; */
	int case_to_do;
	
	
	
	/*if extrapolation is needed, compute the new */
	/*time and date. */
	if(needsExtrapolation){
		
		if(saa_usp.start==TRUE) {
                if(SAA_DEBUG){
 		   fprintf(stderr,"USP start is true: date = %d, time = %d\n",saa_usp.date,saa_usp.time);}
		/*compute the endTime (seconds from midnight) and endDate (julian) for forward extrapolation */
		forward_exp_endtime = (saa_usp.time + 
						(int)(forwardExtrapolationTime*SEC_IN_HOUR)) % SEC_IN_DAY;
		forward_exp_enddate = saa_usp.date;
	
		/*adjust for date change */
		if(forward_exp_endtime < saa_usp.time){
				forward_exp_enddate = saa_usp.date + 1;
		}/*end if */
	
			/*compute the endTime (seconds from midnight) and endDate (julian) for backward extrapolation */
	
		if(time > (int)(backwardExtrapolationTime*SEC_IN_HOUR)){
				/*same day */
				backward_exp_endtime = (time -  (int)(backwardExtrapolationTime*SEC_IN_HOUR)) % SEC_IN_DAY;
				backward_exp_enddate = date;
		}
		else{
				/*previous day */
				backward_exp_endtime = time + SEC_IN_DAY - (int)(backwardExtrapolationTime*SEC_IN_HOUR);
				backward_exp_enddate = date - 1;
		}/*end if */
			
			
		/*************************Forward Extrapolation********************************************** */
		case_to_do = get_case(forward_exp_enddate,forward_exp_endtime);
		ForwardExtrapolate = TRUE;      	/* Added 11/05/2004 for Build8 */
		process_case (case_to_do, forward_exp_enddate,forward_exp_endtime,forwardExtrapolationTime,missing_period,
		              ForwardExtrapolate);

		if(SAA_DEBUG) { 
				fprintf(stderr,"USP buffer after forward extrapolation\n"); 
				print_SAA_USP();
		}
		
			
		/*************************Backward Extrapolation********************************************** */
		case_to_do= get_case(backward_exp_enddate,backward_exp_endtime);
		/*take actions to generate usp product if past clock hour and the duration is valid kd 1/08/04 */
		ForwardExtrapolate = FALSE;  		/* Added 11/05/2004 for Build8  */
		process_case (case_to_do, backward_exp_enddate,backward_exp_endtime,backwardExtrapolationTime,missing_period,
		              ForwardExtrapolate);
			
		if(SAA_DEBUG) { 
				fprintf(stderr,"USP buffer after backward extrapolation\n"); 
				print_SAA_USP();
		}
			
			
		}/*end if start	 */
		
	}/*end extrapolation */
		
	
	/*add the new accumulation to the new position */
	case_to_do = get_case(date,time);
	ForwardExtrapolate = FALSE;   /* Added 3/23/2004   WDZ */
	process_case (case_to_do, date,time,duration,missing_period,ForwardExtrapolate);
        /*  Following three lines added for CCR NA06-06603 by WDZ 03/09/2006  */
        if(saa_usp.start == FALSE){
           saa_usp.start = TRUE;
        }
	
	if(SAA_DEBUG) { 
		fprintf(stderr,"USP buffer after compute_usp_product is done\n"); 
		print_SAA_USP();
	}
	
}/*end compute_usp_products */
/*********************************************************  
 Method  : add_to_usp_total
 Details : adds to the usp snow water equivalent and
 	   snow depth totals
 	     Revised DONE
*********************************************************/
void add_to_usp_total(
	
	float swe_equivalent,
	int azimuth,
	int range)
{
	int i = azimuth;
	int j = range;
				
	saa_usp.saa_usp_swe_oh[i][j] +=swe_equivalent;
	saa_usp.saa_usp_sd_oh [i][j]  +=swe_equivalent * saa_adapt.s_w_ratio;
	
	if(saa_usp.saa_usp_swe_oh[i][j] > SAA_ACCUM_CUTOFF){
		saa_usp.saa_usp_swe_oh[i][j] = SAA_ACCUM_CUTOFF;
	}
	
	if(saa_usp.saa_usp_sd_oh[i][j]  > SAA_ACCUM_CUTOFF){
		saa_usp.saa_usp_sd_oh[i][j]  = SAA_ACCUM_CUTOFF;
	}
	
	
}/*end add_to_usp_total */

/*********************************************************  
 Method  : get_oh_usp_total  kd 10/9
 Details : get the 1 hour total of usp snow water equivalent
           get the 1-hour total of usp snow depths 
           Revised DONE 2/3/04
*********************************************************/
/*void get_oh_usp_total(
	
	int azimuth,
	int range)
{
	int i = azimuth;
	int j = range;
	
	saa_usp_swe_oh[i][j] = saa_usp.saa_usp_swe_oh[i][j];
	saa_usp_sd_oh[i][j] =  saa_usp.saa_usp_sd_oh[i][j];
	

}
*/

/*********************************************************  
 Method  : time_crossed_clock_hour
 Details : returns TRUE (1) if the new time crossed the clock hour
 	   when compared to the previous time. Revised DONE 2/3/04
*********************************************************/
int time_crossed_clock_hour(int date, int time){

	int previoustime, previousdate;
	previoustime = saa_usp.time;
	previousdate = saa_usp.date;
	
	if(saa_usp.start== FALSE){
		return FALSE;
	}/*end if */
	
	if(SAA_DEBUG){
		fprintf(stderr,"saa_time_crossed: prev_date(%d),new_date(%d),prev_time(%d),new_time(%d)\n",
				previousdate,date,previoustime,time);
	}
	
	/*if the current date is not equal to previous date,  */
	/*time has obviously crossed  */
	if(previousdate != date){
		if(SAA_DEBUG){fprintf(stderr, "Date changed. Time crossed clock hour. \n");}
		return TRUE;
	}
	
	/*check if the new time has crossed the clock hour */
	if( time - previoustime >= SEC_IN_HOUR){
		if(SAA_DEBUG){fprintf(stderr, "Time crossed clock hour. \n");}
		return TRUE;
	}/*end if	 */
	else if( ((time % SEC_IN_HOUR) - (previoustime % SEC_IN_HOUR)) < 0){
		if(SAA_DEBUG){fprintf(stderr, "Time crossed clock hour. \n");}
		return TRUE;
	}/*end else if */
	
	
	if(SAA_DEBUG){fprintf(stderr, "Time did not cross clock hour. \n");}
	return FALSE;
	
}/*end time_crossed_clock_hour */

/*********************************************************  
 Method  : get_case  created 2/3/04 kd
 Details : return the flag indicating if a newBin needs to strore data and info 
 	   for the new usp clock hour accumulation
 	
*********************************************************/
int  get_case(int newdate,int newtime){

	
	
	
	int time_crossed;
	int time_after_hour,time_till_hour;
	int old_time;					     
	
	/*kd mod 11/13 */
	if(saa_usp.start == FALSE){
	
		saa_usp.start=TRUE;
		past_clock_hour=FALSE;
		return 3;
	}
	else {	
	 	/*check if time crossed clock hour */
		time_crossed = time_crossed_clock_hour(newdate,newtime);
		
		/*time crosses clock hour this time */
		if(time_crossed){
		        /*set flag for compute_usp_products to generate usp product if valid */
			past_clock_hour=TRUE;
			/*if time crossed clock hour, then find the time which is closest to the clock hour */
			old_time=saa_usp.time;
			if(SAA_DEBUG){fprintf(stderr, "old Time after hour : %d\n",old_time);}
			time_after_hour = newtime % SEC_IN_HOUR;
			time_till_hour  = SEC_IN_HOUR - (old_time % SEC_IN_HOUR);
			
			
			if(SAA_DEBUG){fprintf(stderr, "Time after hour : %d, Time Till Hour : %d\n",time_after_hour,time_till_hour);}
			
			if( (time_after_hour >= time_till_hour) || ( newtime - old_time) >= SEC_IN_HOUR) 
			{
				/*old time closer to the clock hour */
				if(SAA_DEBUG){fprintf(stderr,"Case 1: Old time closer to the hour.New bin needed \n");}
				return 1;
			}
			else{
				
				/*This block is entered when the new time is closer to the clock hour */
				
				if(SAA_DEBUG){fprintf(stderr,"Case 2:New time closer to the hour.\n");}
				
				/*if new bin was not assigned last time (even when the time */
				/*crossed clock hour), we need to add that to the current bin. */
				
				/*time crossed and new time is closer.  put that in the last position. */
				if(SAA_DEBUG){fprintf(stderr,"Old bin used.\n");}
				
				/*this time, time crossed the clock hour, but no new bin has been assigned.  Set the flag. */
				
			
				
				return 2;
				
			}/* end else */
			
		}/*end if time crossed */
		else
		{ /*time did not cross this time  */
			past_clock_hour=FALSE;
			
			/*this time, time did not cross the clock hour. Reset the flag because */
			/*time did not cross the clock hour. */
			
			/*re_use current bin */
			if(SAA_DEBUG){fprintf(stderr,"Case 3: \n");}
			
		        return 3;
		}/*end else */
	}/*end new else */
	

	
}
/***********************************************************************************
  Method: initialize_usp_buffer
  Details: This method reset  the values of the saa_usp at a given location to initial values
  kd 11/9 revised Done
***********************************************************************************/
void initialize_usp_buffer(){
 	int i,j;       
	if (SAA_DEBUG) { fprintf(stderr,"Initialize usp_buffer\n"); }

	for(i = 0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){
			saa_usp.saa_usp_sd_oh[i][j] = 0.0;
			saa_usp.saa_usp_swe_oh[i][j]  = 0.0;
		}/*end for */
	}/*end for */
	
	saa_usp.date = hybscan_suppl.avgdate;		/* CCR NA06-06603 03/09/2006 WDZ */
	saa_usp.time = hybscan_suppl.avgtime;		/* CCR NA06-06603 03/09/2006 WDZ */
	saa_usp.start_date = hybscan_suppl.avgdate;	/* CCR NA06-06603 03/09/2006 WDZ */
	saa_usp.start_time = hybscan_suppl.avgtime;	/* CCR NA06-06603 03/09/2006 WDZ */
	saa_usp.duration = 0;    /*2/3/04 */
	saa_usp.missing_period = 0;
	saa_usp.bufferIsEmpty= TRUE;
	saa_usp.start=FALSE;
	

}/*end initialize_usp_buffer */
/***********************************************************************************
  Method: reset_usp_acccumulation
  Details: This method reset  the values of the saa_usp at a given location to initial values
  kd 11/9 revised Done
***********************************************************************************/
void reset_usp_accumulation()
{

	int i,j;       
	if (SAA_DEBUG) { fprintf(stderr,"reset usp data accumulation\n"); }

	for(i = 0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){
			saa_usp.saa_usp_sd_oh[i][j] = 0.0;
			saa_usp.saa_usp_swe_oh[i][j]  = 0.0;
		}/*end for */
	}/*end for */
	saa_usp.start_date = hybscan_suppl.avgdate;	/* CCR NA06-06603 03/09/2006 WDZ */
	saa_usp.start_time = hybscan_suppl.avgtime;	/* CCR NA06-06603 03/09/2006 WDZ */
	saa_usp.duration=0.0;
	saa_usp.missing_period = 0.0;
	saa_usp.bufferIsEmpty=TRUE;

}

/*********************************************************  
 Method  : print_saa_product_matrix
 Details : prints a product matrix to stderr.  The matrix type
 	   is determined by the parameter type. Size is specified by the
           rows/cols parameters.
           rowstart and colstart determines where to start
           the printing from.
*********************************************************/
void print_saa_product_matrix(
enum saa_product_type type,
int rows, int cols,int rowstart ,int colstart) 
{

	int i,j;
	float usp_total [rows][cols];
	
	if( (rowstart + rows >= MAX_AZM) ||
	    (colstart + cols >= MAX_RNG) ){
	     
	     fprintf(stderr,"Invalid ranges for printing saa product matrix.\n");
	     return;
	}
	
	switch(type){
		case swe_ohp:
			fprintf(stderr,"SWE OHP from %d,%d: \n",rowstart,colstart);
			break;
			
		case swe_total:
			fprintf(stderr,"SWE Storm Total from %d,%d: \n",rowstart,colstart);
			break;
		
		case swe_usp:
			fprintf(stderr,"SWE User Selectable Total from %d,%d: \n",rowstart,colstart);
			break;
		
		case sd_usp:
			fprintf(stderr,"SD User Selectable Total from %d,%d: \n",rowstart,colstart);
			break;
		
		default:
			fprintf(stderr,"Invalid type specified for printing. \n");
			return;
	}/*end switch */
	
	if((type != swe_usp) &&
	   (type != sd_usp)
	  ){
		for(i = 0;i<rows;i++){
			for(j = 0;j<cols;j++){
				switch(type){
					case swe_ohp:
						fprintf(stderr,"%3.4f  ",saa_swe_oh_total[i+rowstart][j+colstart]);
						continue;
					
					case swe_total:
						fprintf(stderr,"%3.4f  ",saa_swe_storm_total[i+rowstart][j+colstart]);
						continue;
		
					default:
						break;
				}/*end switch */
			}/*end for */
			fprintf(stderr,"\n");
		}/*end for */
	}/*end if */
	else{/*printing user selectable total is special. */
		
		
		
		
		/*initialize the usp_total matrix */
		for(i = 0;i<rows;i++){
			for(j = 0;j<cols;j++){
				usp_total[i][j] = 0.0;
			}
		}
		
		for(i = 0;i<rows;i++){
				for(j = 0;j<cols;j++){
					if(type == swe_usp){
						usp_total[i][j] = usp_total[i][j]+ 
							 saa_usp.saa_usp_swe_oh[i+rowstart][j+colstart];
						fprintf(stderr,"%3.4f  ",saa_usp.saa_usp_swe_oh[i+rowstart][j+colstart]);
					}
					else if(type == sd_usp){
						usp_total[i][j] = usp_total[i][j]+ 
							 saa_usp.saa_usp_sd_oh[i+rowstart][j+colstart];
						fprintf(stderr,"%3.4f  ",saa_usp.saa_usp_sd_oh[i+rowstart][j+colstart]);
					}
				}/*end for */
				fprintf(stderr,"\n");
			}/*end for */
			fprintf(stderr,"\n\n");
		
		
		fprintf(stderr,"USP Total \n");
		for(i = 0;i<rows;i++){
			for(j = 0;j<cols;j++){
				fprintf(stderr,"%3.4f  ",usp_total[i][j]);
			}/*end for */
			fprintf(stderr,"\n");
		}/*end for */
	}/*end else */
		
}/*end function print_saa_product_matrix */

/*********************************************************  
 Method  : get_ohp_begin_date kd
 Details : returns OHP begin date (Julian) and time (minutes after midnight)
*********************************************************/
void get_ohp_begin_date(int* date, int* time_in_minutes)
{
	int duration = 0;
   	int time;
   	
   	if((date == NULL) ||
	   (time_in_minutes == NULL)){
	   LE_send_msg(GL_ERROR,"SAA:get_ohp_begin_date - NULL pointers passed in as arguments.\n");
	}
	/*kd mod */
	if(saa_ohp.size == 0){
		*date 		= hybscan_suppl.avgdate;
		*time_in_minutes = hybscan_suppl.avgtime/SEC_IN_MIN;

		return;
	}
	*date = saa_ohp.swe_buffer[saa_ohp.first].date;
	time = saa_ohp.swe_buffer[saa_ohp.first].time;
	duration = (int)(saa_ohp.swe_buffer[saa_ohp.first].duration * SEC_IN_HOUR);
	
	if((time - duration) < 0){
		/*started on previous date */
		date -= 1;
		time  = SEC_IN_DAY + time - duration;
	}
	else{
		time = time - duration;
	}
        if(SAA_DEBUG){fprintf(stderr,"get_ohp_begin_date: date: %d, time in seconds: %d, size: %d .\n",*date,time,saa_ohp.size);}
   	/*convert time to time in minutes after midnight */
   	*time_in_minutes = (int)time/SEC_IN_MIN;
   	
}/*end get_ohp_begin_date */

/*********************************************************  
 Method  : get_ohp_end_date kd
 Details : returns OHP end date (Julian) and time (minutes after midnight)
*********************************************************/
void get_ohp_end_date(int* date, int* time_in_minutes)
{
	
	int time;
	if((date == NULL) ||
	   (time_in_minutes == NULL)){
	   LE_send_msg(GL_ERROR,"SAA:get_ohp_end_date - NULL pointers passed in as arguments.\n");
	}
	/*kd mod */
	if(saa_ohp.size == 0){
		*date 		= hybscan_suppl.avgdate;
		*time_in_minutes = hybscan_suppl.avgtime/SEC_IN_MIN;

		return;
	}
	
	*date 		= saa_ohp.swe_buffer[(saa_ohp.size + saa_ohp.first-1)% SAA_OHP_BUFSIZE].date;
	time 		= saa_ohp.swe_buffer[(saa_ohp.size + saa_ohp.first-1)% SAA_OHP_BUFSIZE].time;
	if(SAA_DEBUG){fprintf(stderr,"get_ohp_end_date: date: %d, time in seconds: %d, size: %d .\n",*date,time,saa_ohp.size);}
	*time_in_minutes = (int) time/SEC_IN_MIN;

}/*end get_ohp_end_date */

/*********************************************************  
 Method  : get_stp_begin_date (kd 10/9)
 Details : returns STP begin date (Julian) and time (minutes after midnight)
*********************************************************/
void get_stp_begin_date(int* date, int* time_in_minutes)
{
	/*int duration = 0; */
   	int time;
   	
   	if((date == NULL) ||
	   (time_in_minutes == NULL)){
	   LE_send_msg(GL_ERROR,"SAA:get_ohp_begin_date - NULL pointers passed in as arguments.\n");
	}
	
	*date = saa_ohp.stp_begin_date;
	time = saa_ohp.stp_begin_time;
	
	
	
        if(SAA_DEBUG){fprintf(stderr,"get_stp_begin_date: date: %d, time in seconds: %d,\n",*date,time);}
   	/*convert time to time in minutes after midnight */
   	*time_in_minutes = (int)time;
	
}/*end get_stp_begin_date */

/*********************************************************  
 Method  : get_stp_end_date (kd 10/9)
 Details : returns STP end date (Julian) and time (minutes after midnight)
*********************************************************/
void get_stp_end_date(int* date, int* time_in_minutes)
{
	int time;
	if((date == NULL) ||
	   (time_in_minutes == NULL)){
	   LE_send_msg(GL_ERROR,"SAA:get_ohp_end_date - NULL pointers passed in as arguments.\n");
	}
	if(saa_ohp.size == 0){
		*date 		= hybscan_suppl.avgdate;
		*time_in_minutes = hybscan_suppl.avgtime/SEC_IN_MIN;/*kd mod 12/4 */
		return;
	}
	
	*date 		= saa_ohp.swe_buffer[(saa_ohp.size + saa_ohp.first-1)% SAA_OHP_BUFSIZE].date;
	time 		= saa_ohp.swe_buffer[(saa_ohp.size + saa_ohp.first-1)% SAA_OHP_BUFSIZE].time;
	if(SAA_DEBUG){fprintf(stderr,"get_stp_end_date: date: %d, time in seconds: %d, size: %d .\n",*date,time,saa_ohp.size);}
	*time_in_minutes = (int) (time / MIN_IN_HOUR);
	
}/*end get_stp_end_date */



/*********************************************************  
 Method  : get_ohp_missing_period kd
 Details : returns the total ohp missing period in fractional hours
*********************************************************/
float get_ohp_missing_period()
{

	int i;
	float missing_period = 0.0;
	
	for(i = 0; i< SAA_OHP_BUFSIZE; i++){
		if(SAA_DEBUG)
		   fprintf(stderr,"OHP missing period[%2d] = %7.4f\n",i,
		           saa_ohp.swe_buffer[i].missing_period);
		missing_period += saa_ohp.swe_buffer[i % SAA_OHP_BUFSIZE].missing_period;
	}
	
	return missing_period;
	
}/*get_ohp_missing_period */

/*********************************************************  
 Method  : get_ohp_duration (kd 12/8)
 Details : returns the total ohp duration fractional hours
*********************************************************/
float get_ohp_duration()
{

	int i;
	float duration = 0.0;
	
	/*kd mod 12/5 */
	for(i = 0; i< SAA_OHP_BUFSIZE;++i){

		duration += saa_ohp.swe_buffer[i].duration;	
	}
	
	return duration;
	
}/*get_ohp_duration */
/*********************************************************  
 Method  : check_valid_ohp_duration (kd 12/8)
 Details : returns TRUE if the total duration of the OHP is >=the threshold time
*********************************************************/
int  check_valid_ohp_duration()
{

	float duration;
	
	duration=get_ohp_duration();
	if(SAA_DEBUG) { fprintf(stderr,"Check valid_ohp_duration  - duration: %f\n",duration);}
	if ( duration>=saa_adapt.thr_mn_pct_time)
	    return TRUE;
	else 
	    return FALSE; 
	
	
}/*get_ohp_missing_period */
/**********************************************************************************************
 	Method:  reset_usr_data
 	Use: reset the imtermediate array kd 11/7/04
**********************************************************************************************/
void reset_usr_data ()
{ 	
	/*set the initial values of all elements of usr_data.swu_data and usr_data.sdu  to 0 12/29 */
	memset ( usr_data.swu_data,0,MAX_AZM*MAX_RNG*sizeof(short));
        memset ( usr_data.sdu_data,0,MAX_AZM*MAX_RNG*sizeof(short)); 
}

/*********************************************************************
 Method : get the msg_id to write to the LB
 Details: 

*****************************************************************/
int get_nextLB(void)
{
	int i;
 	int temp_time;
        int oldest_time;
	int msg_id = -1;
	int max_messages;
	oldest_time =65000000;
	
	max_messages= NUM_MSGS+2;
	for ( i=2 ; i < max_messages; i++)  /* Changed +1 to +2 11/03/03  WDZ  */
	{
		if ( usr_header.usr_date [i-2 ] ==MISSING_DATE)
		{
			if (SAA_DEBUG) {	fprintf(stderr," msg_id= %d \n",i);	}
			return i;
		}
	}

	for ( i=2 ; i < max_messages; i++)  /* Changed +1 to +2 11/03/03  WDZ  */
	{
		temp_time = usr_header.usr_date[i-2]* 1440 + usr_header.usr_time[i-2];
		if ( temp_time < oldest_time)
		{
			oldest_time = temp_time;
			msg_id=i;
		}
		if ( SAA_DEBUG) { fprintf(stderr, "Oldest Time: % d , msg_id %d\n", oldest_time, i);}
      
	}
   	return msg_id;

}
/*********************************************************************
 Method Create_LB
 Details: Create LB for writing. Return the LB descriptor

*****************************************************************/
int create_LB(){

   int ret;
	
   /* try to open LB to write */
   ret = RPGC_data_access_open( SAAUSERSEL, LB_WRITE|LB_READ );
   if(ret <0){

      if (SAA_DEBUG) 
         fprintf(stderr,"RPGC_data_access_open Failed (%d)\n", ret );

      return -1;
               
   }
   else
      if(SAA_DEBUG) 
         fprintf(stderr,"Suceeded in Opening  SAAUSERSEL for writing.\n" );

   return SAAUSERSEL;

}

/*********************************************************************
 Method Write_LB
 Details: Write the usr_data to LB. Returns 0 on success, -1 on failure.

*****************************************************************/

int write_LB (int lbd, int msg_id){

   int ret;

   ret = RPGC_data_access_write( lbd, (void *) &usr_data, sizeof(usr_data_t), msg_id );

   if( ret == sizeof(usr_data_t) ){

      if (SAA_DEBUG) 
         fprintf(stderr,"Write successfully to LB. \n");
		
   }
   else{ 

      if (SAA_DEBUG) 
         fprintf(stderr, "Failed to write to LB. (%d)\n", ret);

      return -1;

   }

   return 0;
}	 	 

/*********************************************************************
 Method Write_LB_Header
 Details: Write the usr_header to LB. Returns 0 on success, -l on failure.

*****************************************************************/
int write_LB_Header ( int lbd, int msg_id){

   int ret;

   ret= RPGC_data_access_write( lbd, (void *) &usr_header, sizeof(usr_header_t), msg_id );
   if( ret == sizeof(usr_header_t) ){

      if (SAA_DEBUG)
         fprintf(stderr, "Write header successfully to LB.\n");  
		
   }
   else{

      if (SAA_DEBUG) 
         fprintf(stderr, "Failed to write header to LB (%d)\n", ret); 
      return -1;
   }

   return 0;
}

/*********************************************************************
 Method read_LB
 Details: read the usr_data from the existing  LB. Return the LB descriptor

*****************************************************************/
int read_LB (int lbd, int msg_id){

   char *buffer = NULL;
   int len;

   len = RPGC_data_access_read( lbd, &buffer, LB_ALLOC_BUF, msg_id);
   if( (len > 0) && (len <= sizeof(usr_data_t)) ){

      memcpy( &usr_data, buffer, len );
      if (SAA_DEBUG) 
         { fprintf(stderr," lbd= %d , Msg length= %d,msg_id= %d\n",lbd, len, msg_id) ; }

       free( buffer );

   }
   else{

      if (SAA_DEBUG) 
         { fprintf(stderr,"Failed to read LB. The return number= %d \n", len) ; }
      return -1; 

   }

   return 0;
}

/*********************************************************************
 Method read_LB_Header
 Details: read  the usr_header from the exisiting LB. Return the LB descriptor

*****************************************************************/
int read_LB_Header (int lbd, int msg_id){

   char *buffer = NULL;
   int len;

   len = RPGC_data_access_read(lbd, &buffer, LB_ALLOC_BUF, msg_id);
   if( (len > 0) && (len <= sizeof(usr_header_t)) ){

      memcpy( &usr_header, buffer, len );
      if (SAA_DEBUG) 
         fprintf(stderr,"lbd=%d , msg len =%d, msg_id= %d\n",lbd,len,msg_id) ; 
		
      free( buffer );
   }
   else{

      if (SAA_DEBUG) 
         fprintf(stderr,"Failed to read  LB header. The return number= %d \n",len) ; 
      return -1;
   }

   return lbd;

}

/*********************************************************************
 	Method: init_LB
  	Use : Initialize the structures usr_header anf usr_data
 

*****************************************************************/

int init_LB( int lbd)
{
	int i;
	int wr_status;
	int max_messages;
        max_messages= NUM_MSGS+2;
	if (SAA_DEBUG) { fprintf (stderr, " Initialize the LB\n"); }
	/*set the initial date,time and flag as MISSING_DATE<MISSING_TIME AND FALSE */
	for ( i=0; i< NUM_MSGS ; i++ )
	{
		usr_header.usr_date[i]  = 	MISSING_DATE;
		usr_header.usr_time [i] = 	MISSING_TIME;
		usr_header.usr_start_date[i] =	MISSING_DATE; /* CCR NA06-06603 03/09/2006 WDZ */
		usr_header.usr_start_time[i] =	MISSING_TIME; /* CCR NA06-06603 03/09/2006 WDZ */
		usr_header.data_available_flag [i] = FALSE;
	}	
      	usr_header.usr_begin_date = 	MISSING_DATE;
	usr_header.usr_begin_time = 	MISSING_TIME;
	usr_header.usr_end_date = 	MISSING_DATE;
	usr_header.usr_end_time	=  	MISSING_TIME;
        /*write the intital usr_header to LB */
	wr_status= write_LB_Header ( lbd,1);
        /*set the initial values of all elements of usr_data.swu_data and usr_data.sdu  to 0 */
	memset ( usr_data.swu_data,0,MAX_AZM*MAX_RNG*sizeof(short));
        memset ( usr_data.sdu_data,0,MAX_AZM*MAX_RNG*sizeof(short));
        /*write the initial usr_data to LB */
        for ( i=2;i< max_messages;i++)
	{
		wr_status= write_LB(lbd,i);
		if (SAA_DEBUG) { fprintf(stderr," Hour= %d,wr_status= %d\n",i,wr_status);}		
		if (wr_status < 0 ) 
		{
			return (wr_status);
		}
	}
	return (wr_status);
}

/**********************************************************************************************
 	Method: scale the valid 1-hour usp product and write it to the Linear Buffer kd 11/7/04
 	Details: get the 1-hour usp data ready for writing to Linear Buffer
**********************************************************************************************/
void scale_usp_outputs(){

	int i, j;
	

	data_valid_flag = saa_Is_Accum_Valid() ? 1 : 0;
	
	if(data_valid_flag){
		
		for(i = 0; i< MAX_AZM;i++){
			for(j=0;j<MAX_RNG;j++){
				usr_data.swu_data[i][j] = 
					(short)(saa_usp.saa_usp_swe_oh[i][j]*OHP_SWE_SCALING_FACTOR);
			
				
				/*cap the value with the maximum value */
				if(usr_data.swu_data[i][j] < 0){
					usr_data.swu_data[i][j] = MAX_ACCUM_CAP;
				}
			      
			        /* 12/29 */
				usr_data.sdu_data[i][j] = 
					(short)(saa_usp.saa_usp_sd_oh[i][j]*OHP_SD_SCALING_FACTOR);
				
				
				/*cap the value with the maximum value */
				
				if(usr_data.sdu_data[i][j] < 0){
					usr_data.sdu_data[i][j] = MAX_ACCUM_CAP;
				}
				
			}/*end for */
		}/*end for */
	}/*end if */
	/*if the data is not valid , set all values to 0 12/29 */
	else{
		
		/*use memset to set the initial values of all elements of usr_data.swu_data and usr_data.sdu  to 0 */
		memset ( usr_data.swu_data,0,MAX_AZM*MAX_RNG*sizeof(short));
        	memset ( usr_data.sdu_data,0,MAX_AZM*MAX_RNG*sizeof(short));
		
		/*set max values to 0  kd 12/29 */
		max_usp_swe_accum 			= 0;	
		max_usp_sd_accum 			= 0;
		
	}

}


/*********************************************************  
 Method  : get_usp_missing_periods  kd 10/9/03
 Details : returns the usp missing periods for the whole 
           array (of size SAA_USP_BUFSIZE) 
*********************************************************/
void get_usp_missing_periods(float* usp_missing_periods)
{

	
	if(usp_missing_periods == NULL){
	   LE_send_msg(GL_ERROR,"SAA:get_usp_missing_periods - NULL pointer passed in as argument.\n");
	}
	
		
	*usp_missing_periods = saa_usp.missing_period;
			
		

     
	
}/*get_usp_missing_periods */
/*****************************************************************************
	Method:    Check_usp_duration kd 1/08/04
	Details: check if the usp duration at a given location is >=the minimum threshold time
		 Revised DONE
*********************************************************************/
int check_usp_duration ()
{

     float usp_duration=0;
     usp_duration=saa_usp.duration  ;
     if (usp_duration<saa_adapt.thr_mn_pct_time )
     {  
       if(SAA_DEBUG) { fprintf(stderr,"Duration < saa_adapt.thr_mn_pct_time; duration = %f\n", usp_duration);}
     	return FALSE;
     }
     if(SAA_DEBUG) { fprintf(stderr,"Valid USP duration to write to LB %f\n", usp_duration);}
     return TRUE;


}

/************************************************************
	Method get_usp_accumulation kd 1/08/04
	Details: get 1-hour usp data REVISED DONE
*************************************************************/
/*void get_usp_accumulation()
{

	int i;
	int j;
		
	if (SAA_DEBUG) { fprintf(stderr,"Get 1_hour usp accumulation\n"); } 	
	for(i = 0;i<MAX_AZM;i++){
		for(j = 0; j < MAX_RNG; j++)
		{
			get_oh_usp_total(i,j);							
		}//end for
	}//end for
			
}
*/
/**********************************************************************************************
 	Method: generate the intermediate usp product kd 1/08/04
 	Details: scale the usp data for writing to LB. REVISED DONE
**********************************************************************************************/
void generate_intermediate_usp_product()
{
	int count_msg_id=0;
	int time=0;
	int date=0;
	
	if(SAA_DEBUG) { fprintf(stderr,"Scale usr_data to get ready to write intermediate array to LB\n");} 
        scale_usp_outputs();
  	count_msg_id= get_nextLB();
  
	if(count_msg_id>0)
	{
		/*get the date/time   12/2/03 kd */
		
		date=saa_usp.date;
		time=saa_usp.time;
		/*assign date/time to the corresponding structure elements */
		usr_header.usr_date[count_msg_id-2]=date;
		usr_header.usr_time[count_msg_id-2]=time/MIN_IN_HOUR;
		usr_header.usr_start_date[count_msg_id-2]=saa_usp.start_date;
		usr_header.usr_start_time[count_msg_id-2]=saa_usp.start_time/MIN_IN_HOUR;
		usr_header.data_available_flag[count_msg_id-2]=TRUE;
		/*write data out to the Linear Buffer */
		wr_status=write_LB_Header(lb_descriptor,1);
		wr_status=write_LB(lb_descriptor,count_msg_id);
		if (SAA_DEBUG) { fprintf(stderr,"1 HOUR ACCUMULATION DONE; successfully write to LB \n");}
		if (SAA_DEBUG) { fprintf(stderr,"Date: %d;Time:%d \n",date,time);}
	} 
						
	/*Reset flags after writing the 1-hour data to LB */
	reset_usr_data();
	if (SAA_DEBUG) { fprintf(stderr,"Reset data writing \n");}
		

}
/*********************************************************************************
  Method get_usp_product kd 1/08/04
  Details: generate the 1-hour usp product at a given location
*********************************************************************************/
void  get_usp_product()
{
	/*get_usp_accumulation(); */
    	generate_intermediate_usp_product();
   
    
}

/**********************************************************************************
    Method check_past_clock_to_get_usp_product kd 1/08/04
    Details: check if the clock hour is passed and then generate 1-hour usp product if the
         duration is valid.If not, then try the look-back process to generate the product.
***********************************************************************************/
int  check_past_clock_hour_to_get_usp_product()
{
   
   int valid_duration=FALSE;
   
   
   
   if(past_clock_hour==TRUE)
   
   {    
   	set_look_back_index();/*kd 1/20/04 */
        if (SAA_DEBUG) { fprintf(stderr,"Marked_index: %d \n",marked_index);}
       
   	valid_duration=check_usp_duration();
   	if(valid_duration==TRUE)
   	{
	  	get_usp_product();
	}			
	else if (valid_duration==FALSE)
	{
		get_second_chance_usp_product();
	}		
        else 
        {
        	if (SAA_DEBUG) { fprintf(stderr,"The retuned value from check_usp_duration is not correct.\n");}
        }
       
     	
        	
        	
       
   }
   return FALSE;
}



/**********************************************************************************
    Method get_usp_duration_at_pos kd 1/13/04
    Details: Return the duration at a given usp location of the saa_usp REVISED DONE
***********************************************************************************/
float get_usp_duration ()
{

   	return saa_usp.duration;
}

/**********************************************************************************
    Method get_usp_time_at_pos(int position) kd 1/13/04
    Details; Return the time at a given location of the saa_usp REVISED DONE
***********************************************************************************/
int get_usp_time ()
{
  	return saa_usp.time;
}

/**********************************************************************************
    Method get_ohp_duration_at_pos(int position) kd 1/13/04
    Details: return the duration of a given location of the saa_ohp structure
***********************************************************************************/
float get_ohp_duration_at_pos (int position)
{
	return saa_ohp.swe_buffer[position].duration;
}



/**********************************************************************************
    Method get_usp_data_at_pos(int position) kd 1/13/04
    Details: copy the data at specific usp buffer cell to the array
***********************************************************************************/
void get_usp_data_at_pos( int position)
{

        int i,j;       
	if (SAA_DEBUG) { fprintf(stderr,"Get data for usp at location %d\n",position); }

	for(i = 0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){
			saa_usp_data_at_pos[i][j]=((saa_ohp.swe_buffer[position]).buffer[i][j]) ;
			
		}/*end for */
	}/*end for */
}


/**********************************************************************************
    Method set_look_back_index kd 1/13/04
    Details: Set the index of the volume scan just before the first volume scan of the
             top-of-the-clock-hour
***********************************************************************************/
void set_look_back_index()
{
 	marked_index=saa_ohp.first;


}
/**********************************************************************************
    Method get_index(int position) kd 1/13/04
   Details: To check if the time span at a specific location is <= the threshold time span( 0.18333)
             REVISED DONE
***********************************************************************************/
int check_valid_time_span (int position)
{
  float time_span=0;

  time_span=get_ohp_duration_at_pos(position);
  if(time_span<=saa_adapt.thr_time_span&&time_span>0)
  	return TRUE;
  return FALSE;
  
}

/**********************************************************************************
    Method get_second_chance_usp_product(int position) kd 1/13/04
    Details: This method is used if the duration is <54. Then we will give another try to
         look back in time up to the time span threshold before the initial start time
         of the current volume scan  The durationn at the found buffer cell is added to 
         that of the current cell. If the new duration is valid > the minimum time threshold, 
         then we can generate the 1-hour usp product at the top-of-the-clock-hour
    	 REVISED 
   
***********************************************************************************/
void get_second_chance_usp_product ()
{
	int index=-1;
	int i;
	int j;
	
	
	int duration_flag=FALSE;
	float duration=0;
	if (SAA_DEBUG) { fprintf(stderr,"Try to get the 1-hour usp product for the second chance\n"); } 
	
	index=marked_index;
	/*if index is valid then check duration to see if duration at the ohp cell <=11 minutes */
	if(index>=0 && index < SAA_OHP_BUFSIZE)
	{
	   
	   if ( saa_ohp.bufferIsEmpty[index] == TRUE)
	   {
	    		if (SAA_DEBUG) { fprintf(stderr,"The saa_ohp buffer cell at index %d was removed\n",index); } 
	    		return;
	    	
	   }
	  if (SAA_DEBUG) { fprintf(stderr,"The loock_back buffer cell at time %d \n",saa_ohp.swe_buffer[index].time); } 	
	  if (SAA_DEBUG) { fprintf(stderr,"The loock_back buffer cell at date %d \n",saa_ohp.swe_buffer[index].date); } 
	
	  duration=get_ohp_duration_at_pos(index);
	  
	   /*if duration is valid (<=threshold time span)then get data at the buffer cell,get duration and add data to the cell */
	   /*containing the curent 1-hour product */
	   
	 
	   if(duration<=saa_adapt.thr_time_span&&duration>0)
	   {
	   	
	   	saa_usp.duration +=duration;
	   	
		/*generate the 1-hour product is after adding the duration of the volume before the last */
		/*'first' volume the duration is >= threshold time */
		/*recheck the duration at the usp buffer cell */
		
		duration_flag=check_usp_duration();
		if(duration_flag==TRUE)
		{
	   		get_usp_data_at_pos(index);
	   		if (SAA_DEBUG) { fprintf(stderr,"Add data to saa_usp buffer cell\n"); } 	
			for(i = 0;i<MAX_AZM;i++){
				for(j = 0; j < MAX_RNG; j++)
				{
				
					add_to_usp_total(saa_usp_data_at_pos[i][j],i,j);							
							
				}/*end for */
			}/*end for */
	   		get_usp_product();
	   	}
	   	else if (duration_flag==FALSE)
	   	{
	   		saa_usp.duration -=duration;
	   		if (SAA_DEBUG) { fprintf(stderr,"Not enough duration to generate the usp product for the second chance.\n");}
	   	}
	   	else if (duration_flag==-1)
	   	{	
	   		saa_usp.duration -=duration;;
	   		if (SAA_DEBUG) { fprintf(stderr,"The retuned value from check_usp_duration is not correct.\n");}
	   	}
	   }
	   else
	   {
	  	if (SAA_DEBUG) { fprintf(stderr,"Invalid additional duration to generate usp. duration =  %f\n",duration); }
	   }
	  
	 
	 }/*end outer if */
	 
	 else
	 {
	  	   if (SAA_DEBUG) { fprintf(stderr,"Invalid index . index =  %d\n",index); }
	  	   
	 }/*end else */
	 
}
/**********************************************************************************
    Method print_SAA_USP kd 1/13/04
    Details: print the information about the saa_usp buffer
        
***********************************************************************************/
void print_SAA_USP()
{

	/*2/3/04 */
	
	
	fprintf(stderr," buffEmpty = %d, missPer = %7.4f, date = %6d, time= %6d, duration = %7.4f\n",
			saa_usp.bufferIsEmpty,
			saa_usp.missing_period,
			saa_usp.date,
			saa_usp.time,
			saa_usp.duration
	);	
		
	
	
}
/**********************************************************************************
    Method process_case kd 2/4/04
    Details: 

Change
history:	27 Oct 04	David Zittel	Check for forward extrapolation 
						added to Build8
***********************************************************************************/
void process_case (int case_to_do ,int date,int time,float duration,float missing_period, int ForwardExtrapolate)
{
    float swe_equivalent;
    int i,j;
    
    int SAA_DEBUG1 = 0;
 
    if(SAA_DEBUG){fprintf(stderr,"Process Case: time = %d\n",time);}

    switch(case_to_do) {
	case 1: 	 
			if (SAA_DEBUG1) { fprintf(stderr,"Case1: Old time closer, ForExtrap = %d \n",ForwardExtrapolate); }       
		        /*generate product if possible then initialize buffer and then add data and update info */
		         past_clock_hour=check_past_clock_hour_to_get_usp_product();
		         reset_usp_accumulation();
		         for(i = 0;i<MAX_AZM;i++){
			       for(j = 0; j < MAX_RNG; j++){
				       /* Forward Extrapolation test added for Build8  */   
		       			if(ForwardExtrapolate==TRUE)			
		       				swe_equivalent = previous_snow_rate[i][j]*duration;
		       			else
		       				swe_equivalent = snow_rate[i][j]*duration;
					add_to_usp_total(swe_equivalent,i,j);
				}/*end for */
		        }/*end for	 */
			saa_usp.date 		= date;
			saa_usp.time 		= time;
			saa_usp.bufferIsEmpty  	= FALSE;
			saa_usp.duration  	+= duration;
			/*saa_usp.missing_period  	+= missing_period; */
			
			break;	
				
         case 2:
 		 	if (SAA_DEBUG1) { fprintf(stderr,"Case 2: New time closer, ForExtrap = %d\n",ForwardExtrapolate); }  
			 /*update info, add data then generate product if possible then reset the saa_usp_accumulation */
			
			/*add the data to the marix */
			for(i = 0;i<MAX_AZM;i++){
				for(j = 0; j < MAX_RNG; j++){
				       /* Forward Extrapolation test added for Build8  */   
					if(ForwardExtrapolate==TRUE)
						swe_equivalent = previous_snow_rate[i][j]*duration;
					else
						swe_equivalent = snow_rate[i][j]*duration;

					add_to_usp_total(swe_equivalent,i,j);

				}/*end for */
			}/*end for	 */
			saa_usp.date 		= date;
			saa_usp.time 		= time;
			saa_usp.bufferIsEmpty  	= FALSE;
			saa_usp.duration  	+= duration;
			/*saa_usp.missing_period  	+= missing_period; */
			past_clock_hour=check_past_clock_hour_to_get_usp_product();
			reset_usp_accumulation();
		        break;		
  
  	case 3:
			if (SAA_DEBUG1) { fprintf(stderr,"Case 3,ForExtrap = %d\n",ForwardExtrapolate); }  
			/*extrapolate and add the data to the appropriate location */
			for(i = 0;i<MAX_AZM;i++){
				for(j = 0; j < MAX_RNG; j++){
				       /* Forward Extrapolation test added for Build8  */   
					if(ForwardExtrapolate==TRUE)
						swe_equivalent = previous_snow_rate[i][j]*duration;
					else
						swe_equivalent = snow_rate[i][j]*duration;

					add_to_usp_total(swe_equivalent,i,j);

				}/*end for */
			}/*end for	 */
			saa_usp.date 		= date;
			saa_usp.time 		= time;
			saa_usp.bufferIsEmpty  	= FALSE;
			saa_usp.duration  	+= duration;
			/*saa_usp.missing_period  	+= missing_period; */
			break;
	default:
	         	if (SAA_DEBUG) { fprintf(stderr,"invalid case :=  %d\n",case_to_do); }  					
			break;
  }/*end switch */



}/*end function */
