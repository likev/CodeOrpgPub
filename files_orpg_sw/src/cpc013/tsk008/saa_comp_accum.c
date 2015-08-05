/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:44 $
 * $Id: saa_comp_accum.c,v 1.6 2008/01/04 20:54:44 aamirn Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/************************************************************
File 	: saa_comp_accum.c
Details	:Computes the hourly, 3 hourly and storm total accumulations from 
    	 the snow rate.  
    	 
Change
history:	
	10/17/2004	SW CCR NA04-30810	Build8  - Remove call to 
						initialize_usr_products

*************************************************************/

#include "saa_comp_accum.h"
#include "saa.h"
#include "input_buffer_struct.h"
#include "saa_compute_products.h"
#ifndef SAA_COMPUTE_TIME_SPAN_H
#include "saa_compute_time_span.h"
#endif
#include "saa_main.h"
#include "saa_adapt.h"
#include "saa_file_io.h"
#include "saa_generate_intermed_products.h"

static int calledFirstTime   = TRUE;
static int validAccumulation = FALSE;
static int begin_acc_date;  	/* Date(end of last hybrid scan) */
static int begin_acc_time;  	/* Time(end of last hybrid scan) */
static int end_acc_date;    	/* Date(end of current hybrid scan) */
static int end_acc_time;    	/* Date(end of current hybrid scan) */
static int flag_reset_accum = FALSE; /* Reset Snow Accumulation flag */
static int flag_reset_LBs   = FALSE; /* Reset User Selectable Data flag */

int compute_scan_to_scan(float  time_span,
	float *forwardExtrapolationTime,
	float *backwardExtrapolationTime,
	float *missing_period,
	float *duration);

/************************************************************
Method	:compute_accumulations
Details	:Computes the hourly, 3 hourly and storm total accumulations from 
    	 the snow rate.  
*************************************************************/

int saa_compute_accumulations (){

	float time_span;
	int needsExtrapolation;
	float forwardExtrapolationTime 	= 0.0;
	float backwardExtrapolationTime = 0.0;
	float missing_period		= 0.0;
	float duration			= 0.0;
	int date;
	int time;
	int wr_status;
	
	/*get the current hybscan date and time */
	/*this information is required for the intermediate */
	/*product */
	current_hybscan_date	= hybscan_suppl.avgdate;
	current_hybscan_time 	= hybscan_suppl.avgtime;
	
	/*If the function is called the first time or the accumulation
	 *flag is reset, we do not
	 *have enough date to compute the time span.  So initialize
	 *the output structures and return **********************/
	 if(SAA_DEBUG)
	 {
	  	fprintf(stderr,"-->calledFirstTime= %d\n -->flag_reset_accum= %d\n",
	  	calledFirstTime,flag_reset_accum);
	 }
	if(calledFirstTime || flag_reset_accum){
		if(SAA_DEBUG)
	 	{
	  		fprintf(stderr,"-->data_read_from_files = %d\n -->data_read_from_total_files= %d\n",
	  		data_read_from_files,data_read_from_total_files);
	 	}
	  	/*kd 11/6 */
		if(data_read_from_files == 0 && data_read_from_total_files==0){
			initialize_products();
			if(flag_reset_LBs == 1){
			/*added 02/06/2004 WDZd  */
      				if (lb_descriptor<0)
					{
					lb_descriptor =create_LB();
     				}

				/* initialize data elements of the LB to default values */
      				wr_status=init_LB(lb_descriptor);
				if (wr_status==0)
					{
					usr_data_available=TRUE;
					}
				else
					usr_data_available= FALSE;

				initialize_usr_products();
				flag_reset_LBs = FALSE;
			}  /*End reset LBs test */

			begin_acc_time  	= hybscan_suppl.avgtime;
			begin_acc_date		= hybscan_suppl.avgdate;
		 	hybscan_begin_date	= (short)hybscan_suppl.avgdate;
			hybscan_begin_time	= (short)(hybscan_suppl.avgtime/SEC_IN_MIN); 
			stp_begin_date		= hybscan_begin_date;
			stp_begin_time		= hybscan_begin_time;
			saa_ohp.stp_begin_date	= stp_begin_date;
			saa_ohp.stp_begin_time	= stp_begin_time;
			saa_ohp.stp_missing_period = 0.0;
			stp_data_valid_flag	= FALSE;  /*  Added 12/8/2003 WDZ  */
			calledFirstTime		= FALSE;  /*  Added WDZ 12/09/2003  */
			flag_reset_accum = FALSE;  /*  Added WDZ 12/12/2003  */
			return 0;
		} 
		
		/*if the data recovered by reading from the file,the begin_acc_date and 
      acc_time should be the end time of the first element of the saa_ohp 
      structure that we read back from the file kd 11/8 */
		
        	get_ohp_end_date(&date,&time);
		flag_reset_accum = FALSE;  /*  Added WDZ 12/11/2003  */
/*
	The following line is not compiled for Build8.  The usp products should
	not be initialized when retrieving data from temporary files		
		initialize_usr_products();   
*/
                validAccumulation 	= FALSE;
		begin_acc_date=date;
		begin_acc_time=time*SEC_IN_MIN;;
		hybscan_begin_date	= (short)date;
		hybscan_begin_time	= (short)time;
		/*get back the stp_start_date and stp_start_time from the structure ohp */
		stp_begin_date = saa_ohp.stp_begin_date;
		stp_begin_time = saa_ohp.stp_begin_time;
	 	stp_data_valid_flag     = TRUE;  /*  Added WDZ 12/09/2003  */
		calledFirstTime		= FALSE; /*  Added WDZ 12/09/2003  */
		if(SAA_DEBUG){fprintf(stderr," Stp_begin_date/time = %d/%d\n",stp_begin_date,stp_begin_time);}
		

	}/*end if */
	if(SAA_DEBUG) {
		fprintf(stderr," Past check for reset in saa_compute_accum\n");
	}
	validAccumulation 	= TRUE;
	stp_data_valid_flag     = TRUE;
	end_acc_time 		= hybscan_suppl.avgtime;
	end_acc_date 		= hybscan_suppl.avgdate;
	
	/*compute the time span */
	time_span = saa_compute_time_span(begin_acc_date,begin_acc_time,
					  end_acc_date,end_acc_time);
	
	if(SAA_DEBUG){
		fprintf(stderr,"Begin Time: %d, End Time: %d, TSPAN: %3.3f\n",
				begin_acc_time,end_acc_time,time_span);}

	if(time_span <= 0.0 || time_span > 30.0){
		LE_send_msg(GL_ERROR,"Time span is negative or > 30 hrs, reinitializing arrays.");
		flag_reset_accum = TRUE;   /*  Added WDZ 12/11/2003   */
		data_read_from_files = FALSE;        /*  Added 12/12/2003  WDZ  */
		data_read_from_total_files = FALSE;  /*  Added 12/12/2003  WDZ  */
		
		/*added 12/18 kd  */
		if (lb_descriptor<0)
		{
			
			lb_descriptor =create_LB();
			
		}
		
		/* initialize data elements of the LB to default values */
		wr_status=init_LB(lb_descriptor);
		if (wr_status==0)
		{
			usr_data_available=TRUE;
		}
		else
			usr_data_available= FALSE;
		initialize_usr_products();	     /*  Added 12/22/2003  WDZ  */
		return saa_compute_accumulations (); /*  Added 12/12/2003  WDZ  */
	}
				  
	
	/*compute scan_to_scan Snow water equivalent and Snow Depth */
	needsExtrapolation = compute_scan_to_scan(time_span,&forwardExtrapolationTime,&backwardExtrapolationTime,&missing_period,
						  &duration);
	
	compute_products(end_acc_date,
			end_acc_time,
			needsExtrapolation,
			forwardExtrapolationTime,
			backwardExtrapolationTime,
			missing_period,
			duration);
	
	/*reset begin accumulation time and date */
	begin_acc_date = end_acc_date;
	begin_acc_time = end_acc_time;
	return 0;
			
}/*end compute_accumulations */
/*********************************************************  
Method : saa_acc_calledFirstTime
Details: Returns true if the compute_accumulations has not been
	 called at all before.	 
*********************************************************/
int saa_acc_calledFirstTime(){
	return calledFirstTime;
}/*end saa_acc_calledFirstTime */

/*********************************************************  
Method : saa_Is_Accum_Valid
Details: Returns true if the current accumulation is valid 
         (accumulation is valid if it is not the first scan,
          or if reset flag is not set)
*********************************************************/
int saa_Is_Accum_Valid(){
	return validAccumulation;
}/*end saa_acc_calledFirstTime */
/*********************************************************  
Method : compute_scan_to_scan
Details: Compute the scan_to_scan Snow Water Equivalent 
	 and the Snow Depth Equivalent matrices.
	 Returns TRUE (1) if extrapolation is needed.  Else,
	 returns FALSE (0).  If extrapolation is needed,
	 the times for forwardExtrapolation and backwardExtrapolation
	 will be returned in the arguments.  If there is a missing
	 period, then that's also returned in the corresponding argument.
*********************************************************/
int compute_scan_to_scan(
float  time_span,
float *forwardExtrapolationTime,
float *backwardExtrapolationTime,
float *missing_period,
float *duration
){

	int i,j;
	float swe_equivalent;
	int needsExtrapolation 		= FALSE;
	float difference 		= 0.0;
	
	/*Check for NULL pointers  */
	if ( (forwardExtrapolationTime  == NULL) ||
	     (backwardExtrapolationTime == NULL) ||
	     (missing_period		== NULL) ||
	     (duration			== NULL)
           ){
		LE_send_msg(GL_ERROR,"SAA:compute_scan_to_scan - NULL pointers passed in as arguments.\n");
	  	return FALSE;
	}/*end if */
	
	*forwardExtrapolationTime 	= 0.0;
	*backwardExtrapolationTime 	= 0.0;
	*missing_period 		= 0.0; 
	*duration			= time_span;

	/*extrapolation is necessary if the time_span
	  exceeds the threshold time span (saa_adapt.thr_time_span)
	  If the time_span exceeds the threshold time span,
  	then the difference is found.  Then, the current rate is
  	extrapolated backwards and the previous rate is extrapolated 
  	forwards each for upto the threshold time span.  If the difference
  	is less than twice the threshold time span, then the current rate
  	and the previous rate are extrapolated for half the difference.
  	If there is any remaining time after extrapolation, that is considered
  	as missing period. */
	
	
	if(time_span > saa_adapt.thr_time_span){
		
		difference			= time_span - saa_adapt.thr_time_span;
		
		if(difference > 2 * saa_adapt.thr_time_span){
			*forwardExtrapolationTime  	= saa_adapt.thr_time_span;
			*backwardExtrapolationTime 	= saa_adapt.thr_time_span;
			*missing_period		   	= difference - 2*saa_adapt.thr_time_span;
		}
		else{
			*forwardExtrapolationTime 	= difference/2;
			*backwardExtrapolationTime	= difference/2;
		}
		/*since extrapolation is needed, for now, compute the accumulation for 
		  only the threshold time span.  The extrapolation will be done later 
      (in saa_compute_products.c) */
		*duration		 	= saa_adapt.thr_time_span;
						 
		needsExtrapolation 		= TRUE;
		
		if(SAA_DEBUG){fprintf(stderr,"Extrapolation Required.\n");}
		if(SAA_DEBUG){fprintf(stderr,"Timespan for backward extrapolation: %3.3f\n",*backwardExtrapolationTime);}
		if(SAA_DEBUG){fprintf(stderr,"Timespan for forward extrapolation: %3.3f\n", *forwardExtrapolationTime);}
	}/*end if */
	
	/***************************************************
	Compute Scan to Scan Snow Water Equivalent and 
	Scan to Scan Snow Depth.
	Scan to Scan SWE = Snow Rate * Time Span
	Scan to Scan SD  = Scan to Scan SWE * Snow Water Ratio
	****************************************************/

	for(i = 0;i<MAX_AZM;i++){
		for(j=0;j<MAX_RNG;j++){
			swe_equivalent = snow_rate[i][j] * (*duration);
			scan_to_scan_swe[i][j] 	= swe_equivalent;
			scan_to_scan_sd[i][j] 	= swe_equivalent * saa_adapt.s_w_ratio;
		}/*end for */
	}/*end for */
	
	return needsExtrapolation;
	
}/*end compute_scan_to_scan */

/**********************************************************/

/***********************************************************
  
   Method: 
      Event_handler
 
   Details:
      Event handler for ORPGEVT_RESET_SAAACCUM event. 
***********************************************************/
void Event_handler( int queued_param ){

   if( queued_param == ORPGEVT_RESET_SAAACCUM ){

      LE_send_msg( 0, "Received event to Reset Snow Accumulation\n" );
      flag_reset_accum = 1;
      data_read_from_files 	 = FALSE;  /*  Added 02/06/2004  WDZ  */
      data_read_from_total_files = FALSE;  /*  Added 02/06/2004  WDZ  */
      flag_reset_LBs = 1;                  /*  Added 02/06/2004  WDZ  */

   }

}

