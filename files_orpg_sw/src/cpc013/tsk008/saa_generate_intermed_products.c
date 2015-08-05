/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:51 $
 * $Id: saa_generate_intermed_products.c,v 1.2 2008/01/04 20:54:51 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*********************************************************************
File    : saa_generate_intermed_products.c
Created : Sept. 15,2003
Details : Generates the SAA intermediate products
Author  : Reji Zachariah

Modification History: Khoi Dang added functions(since October 10) with 
                      some modifications by Dave Zittel
			1.scale_usp_outputs
			2.read_LB_Header
			3.read_LB
			4.write_LB
			5.write_LB_Header
			6.get_hourly_index
			7.init_LB
			8.get_nextLB
			9.reset_usr_data
			10.generate_intermediate_usp_product
		    *delete the function get_hourly_index because it is not needed anymore (12/18)
		    *delete the variable storm_total_products_has_missing_data and the corresponding
		     lines of code in the scale_outputs functions because the functions check_valid_ohp_duration
		     will take care of this.
		    *move the calculation NUM_MSGS+2 out of the for loop int the init_LB and get_nextLB
		     so that the code runs faster 
		    *use memset to initialize the arrays ohp_swe_total,ohp_sd_total,storm_total_swe,storm_total_sd,usr_data.swu_data
		     usr_data.sdu_data  (12/29).The set value is 0 
		    *the calculations for the max values in the arrays are ommitted  (12/29)
		    *deleted the call to get_ohp_missing_period from the method scale_outputs. This call was replaced by
		     the call to method check_valid_ohp_duration in method generate_intermediate_products (1/5/04)
		    *added the block of code which scales and write valid usp one_hour acccumulation.This block of code
		     originally was in saa_main.c (1/5/04)
		    * replaced the back_two_cells flag with the variable difference_in_pos.Consequently, the code is more compact.
		      The position of the data to be produced is located by referencing to saa_ohp.first.
		    *made the block of code for scaling and writing valid 1-hour usp product a function called 
		     generate_intermediate_usp_product (1/7/04)
		    *move several functions to compute_usp_products (1/8/04)
			
*********************************************************************/
#include "saa_main.h"
#include "saa.h"
#include "saa_comp_accum.h"
#include "saa_compute_products.h"
#include "saa_file_io.h"
#include <math.h>
#include "saa_generate_intermed_products.h"



/********************CLASS (GLOBAL) VARIABLES *************************/
short max_ohp_swe_accum 			= 0;	
short max_total_swe_accum 			= 0;
short max_ohp_sd_accum 				= 0;
short max_total_sd_accum 			= 0;
int   offset					= 0;


/**********************DATA DECLARATIONS *****************************/
short ohp_swe_total[MAX_AZM][MAX_RNG];
short ohp_sd_total[MAX_AZM][MAX_RNG];
short storm_total_swe[MAX_AZM][MAX_RNG];
short storm_total_sd[MAX_AZM][MAX_RNG];


/**********************FUNCTION DECLARATIONS ************************/
void scale_outputs();

void checkMemoryAvailable(int offset, int max_bufsize);


/**********************FUNCTION DEFINITIONS *************************/

/**********************************************************************
Method : generate_intermediate_products
Details: generates intermediate product
**********************************************************************/
void generate_intermediate_products(char* outbuf,int max_bufsize){

	int time, date;
	int valid_ohp_duration;
	float ohp_missing_period;
	short shorttime,shortdate;
	short shortvcp = (short)currentvcpnumber;
	offset = 0;

	
	/*copy the data to output buffer*/
	scale_outputs();
	/*hybrid scan begin date */

	checkMemoryAvailable(offset + sizeof(int),max_bufsize);
	memcpy(outbuf + offset,&hybscan_begin_date,sizeof(int));
	offset += sizeof(int);
	
	/*hybrid scan begin time */

	checkMemoryAvailable(offset + sizeof(int),max_bufsize);
	memcpy(outbuf + offset,&hybscan_begin_time,sizeof(int));
	offset += sizeof(int);
	
	/*current hybrid scan date */

	checkMemoryAvailable(offset + sizeof(int),max_bufsize);
	memcpy(outbuf + offset,&current_hybscan_date,sizeof(int));
	offset += sizeof(int);
	
	/*current hybrid scan time */

	checkMemoryAvailable(offset + sizeof(int),max_bufsize);
	memcpy(outbuf + offset,&current_hybscan_time,sizeof(int));
	offset += sizeof(int);
	
	/*adaptation data */
	
	checkMemoryAvailable(offset + sizeof(saa_adapt_t),max_bufsize);
	memcpy(outbuf + offset,&saa_adapt,sizeof(saa_adapt_t));
	offset += sizeof(saa_adapt_t);
	
	/*current vcp number */
	
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shortvcp,sizeof(short));
	offset += sizeof(short);
	
	/*check the minimum time threshold */
	
	valid_ohp_duration=check_valid_ohp_duration();/*kd mod12/8 */
	/*if(SAA_DEBUG){fprintf(stderr,"valid_ohp_duration: %d \n",valid_ohp_duration);} */
	/*set data_valid_flag depending on the value of valid_ohp_duration, that is either TRUE or FALSE */
	data_valid_flag=valid_ohp_duration;
	if(SAA_DEBUG){fprintf(stderr,"valid_ohp_duration= %d \n",data_valid_flag);}
	
	
	/*if(SAA_DEBUG){fprintf(stderr,"Offset: %d \n",offset + sizeof(short));} */
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&data_valid_flag,sizeof(short));
	offset += sizeof(short);
	
	/*OHP SWE data */

	checkMemoryAvailable(offset + MAX_AZM * MAX_RNG * sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&ohp_swe_total,MAX_AZM * MAX_RNG * sizeof(short));
	offset += MAX_AZM * MAX_RNG * sizeof(short);
	
	/*MAX OHP SWE  */

	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&max_ohp_swe_accum , sizeof(short));
	offset += sizeof(short);
	
	/*OHP SD data */
	/*if(SAA_DEBUG){fprintf(stderr,"Offset: %d \n",offset + MAX_AZM * MAX_RNG* sizeof(short));} */
	checkMemoryAvailable(offset + MAX_AZM * MAX_RNG * sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&ohp_sd_total,MAX_AZM * MAX_RNG * sizeof(short));
	offset += MAX_AZM * MAX_RNG * sizeof(short);
	
	/*MAX OHP SD  */

	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&max_ohp_sd_accum , sizeof(short));
	offset += sizeof(short);
	
	/*OHP begin date */
	get_ohp_begin_date(&date,&time);
	shortdate = (short)date;
	shorttime = (short)time;


	if(SAA_DEBUG)
	   fprintf(stderr,"OHP Begin Date: %d, Begin Time: %d.\n",shortdate,shorttime);
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shortdate , sizeof(short));
	offset += sizeof(short);
	
	/*OHP begin time - minutes after midnight */

	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shorttime , sizeof(short));
	offset += sizeof(short);
	
	/*end date - same for OHP, & STP */
	get_ohp_end_date(&date,&time);
	shortdate = (short)date;
	shorttime = (short)time;
	
	if(SAA_DEBUG)
	   fprintf(stderr,"OHP End Date: %d, End Time: %d.\n",shortdate,shorttime);
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shortdate , sizeof(short));
	offset += sizeof(short);
	
	/*end time - same for OHP, & STP */

	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shorttime , sizeof(short));
	offset += sizeof(short);
	
	/*STP SWE data */

	checkMemoryAvailable(offset + MAX_AZM * MAX_RNG * sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&storm_total_swe,MAX_AZM * MAX_RNG * sizeof(short));
	offset += MAX_AZM * MAX_RNG * sizeof(short);
	
	/*MAX STP SWE  */
	
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&max_total_swe_accum  , sizeof(short));
	offset += sizeof(short);
	
	/*STP SD data */

	checkMemoryAvailable(offset + MAX_AZM * MAX_RNG * sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&storm_total_sd,MAX_AZM * MAX_RNG * sizeof(short));
	offset += MAX_AZM * MAX_RNG * sizeof(short);
	
	/*MAX STP SD */
	
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&max_total_sd_accum  , sizeof(short));
	offset += sizeof(short);
	
	/*start date - same for OHP, & STP mod 11/26 kd */
	get_stp_begin_date(&date,&time);
	shortdate = (short)date;
	shorttime = (short)time;
	 /*start date for STP products */

	if(SAA_DEBUG){fprintf(stderr,"STP Start Date: %d, Start Time: %d.\n",shortdate,time);}
	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shortdate , sizeof(short));
	offset += sizeof(short);
	
	/*start time for STP products */

	checkMemoryAvailable(offset + sizeof(short),max_bufsize);
	memcpy(outbuf + offset,&shorttime , sizeof(short));
	offset += sizeof(short);
	/*stp_data_valid_flag */

	checkMemoryAvailable(offset + sizeof(int),max_bufsize);
	memcpy(outbuf + offset,&stp_data_valid_flag, sizeof(int));
	offset += sizeof(int);
	
	/*saa_ohp.stp_missing_period */

	checkMemoryAvailable(offset + sizeof(float),max_bufsize);
	memcpy(outbuf + offset,&saa_ohp.stp_missing_period, sizeof(float));
	offset += sizeof(float);
	
	if(SAA_DEBUG)
		fprintf(stderr,"STP missing periods = %7.4f\n",saa_ohp.stp_missing_period);

	ohp_missing_period = 1.0 - get_ohp_duration();
	if(ohp_missing_period < 0.0 )
	   ohp_missing_period = 0.0;            /*  Added 12/15/2003   WDZ  */
	checkMemoryAvailable(offset + sizeof(float),max_bufsize);
	memcpy(outbuf + offset,&ohp_missing_period, sizeof(float));
	offset += sizeof(float);
	
	if(SAA_DEBUG)
		fprintf(stderr,"OHP missing period = %7.4f\n",ohp_missing_period);
	
        /*reset the maximum values */
	max_ohp_swe_accum 		= 0;	
	max_total_swe_accum 		= 0;
	max_ohp_sd_accum 		= 0;
	max_total_sd_accum 		= 0;
	
	
}/*end generate_intermediate_products */

/**********************************************************************
Method : checkMemoryAvailable
Details: checks for available memory, and flags an error if
         enough memory is not available
**********************************************************************/
void checkMemoryAvailable(int offset, int max_bufsize){

	if(offset >= max_bufsize){
		RPGC_log_msg(GL_ERROR,"Offset exceeds intermediate buffer allocation size.\n");
	}

}
/**********************************************************************
Method : scale_outputs
Details: scales the floating point outputs to short
**********************************************************************/
void scale_outputs(){

	int i, j;

	
	data_valid_flag = saa_Is_Accum_Valid() ? 1 : 0;
	
	if(data_valid_flag){
		
		
		for(i = 0; i< MAX_AZM;i++){
			for(j=0;j<MAX_RNG;j++){
				ohp_swe_total[i][j] = 
					(short)(saa_swe_oh_total[i][j]*OHP_SWE_SCALING_FACTOR);
			
				
				/*cap the value with the maximum value */
				if(ohp_swe_total[i][j] < 0){
					ohp_swe_total[i][j] = MAX_ACCUM_CAP;
				}
				
				ohp_sd_total[i][j] = 
					(short)(saa_sd_oh_total[i][j]*OHP_SD_SCALING_FACTOR);
				
				
				/*cap the value with the maximum value */
				if(ohp_sd_total[i][j] < 0){
					ohp_sd_total[i][j] = MAX_ACCUM_CAP;
				}
			    
				storm_total_swe[i][j] = 
					(saa_swe_storm_total[i][j]*TOTAL_SWE_SCALING_FACTOR);
			
				/*cap the value with the maximum value */
				if(storm_total_swe[i][j] < 0){
					storm_total_swe[i][j] = MAX_ACCUM_CAP;
				} 	
			
				storm_total_sd[i][j] = 
					(saa_sd_storm_total[i][j]*TOTAL_SD_SCALING_FACTOR);
				
				
				/*cap the value with the maximum value */
				if(storm_total_sd[i][j] < 0){
					storm_total_sd[i][j] = MAX_ACCUM_CAP;
				} 
			}/*end for */
		}/*end for */
	}/*end if */
	else{
	
		/*use memset to initialize  values of all elements of the arrays to  0 12/29 */
		memset ( ohp_swe_total,0,MAX_AZM*MAX_RNG*sizeof(short));
        	memset ( ohp_sd_total,0,MAX_AZM*MAX_RNG*sizeof(short));
        	memset ( storm_total_swe,0,MAX_AZM*MAX_RNG*sizeof(short));
        	memset ( storm_total_sd,0,MAX_AZM*MAX_RNG*sizeof(short));
		
		/*set max values to 0 */
		max_ohp_swe_accum 			= 0;	
		max_ohp_sd_accum 			= 0;
		max_total_swe_accum 			= 0;
		max_total_sd_accum 			= 0;
	}
}/*end function scale_outputs */
/*********************************************************************/


