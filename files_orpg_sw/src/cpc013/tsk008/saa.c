/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:40 $
 * $Id: saa.c,v 1.3 2008/01/04 20:54:40 aamirn Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/****************************************************************************************
    File	: saa.c
    Description	:The Snow Accumulation Algorithm (SAA) processes Hybrid Scan reflectivity and elevation angle 
    		 files generated by the EPRE algorithm and computes snow and liquid water equivalent (LWE)
    		 accumulations.  The logic is similar to the Precipitation Processing System (PPS).  
		
    Created 	:By Tim O'Bannon, June 2003
    
    Modification
    History	:
 ***************************************************************************************/

/************** Include files **********************************************************/
#include "saa.h"
#include "saa_main.h"
#include "biased_dbz_to_rate.h"	/*  Converts biased reflectivity data to rate	*/
#ifndef DEG_TO_RAD_H
	#include "deg_to_rad.h"	/*  Converts degrees to radians			*/
#endif
#include "radar_rng.h"		/*  Computes radar range (km) from height (km) 
				    and sine of elevation angle			*/
#include "radar_hgt.h"		/*  Computes radar height (km) from range (km) 
				    and sine of elevation angle			*/

#ifndef SAA_VCP_SETUP_H
	#include "saa_vcp_setup.h"
#endif


#include "saa_comp_accum.h"

/*enumeration for determining the matrix for debug printing */
enum print_Matrix_Type {
	saa_snow_rate,
	saa_scan_to_scan_swe,
	saa_scan_to_scan_sd,
	saa_hyscanE
};

/*****************************  Internal modules**************************************/
void read_saa_adaptation(void);	
void compute_snow_rate(void);
/*prints the matrix determined by type to stderr for debugging purposes*/
void print_saa_matrix(enum print_Matrix_Type type,
			   int rows, int cols,int rowstart ,int colstart); 
void range_correction(void);
void compute_snow_depth(void);
void build_products(void);
float rhc_factor(int, int);
int  neighbor_test(int, int);
float average_rate(int, int);
float rhc_fctr(int, int);
void get_RCA_correction_factors(void);
void build_saa_rhc_array(void);
void copy_to_previousrate_matrix(void);
/*************************************************************************************/

static float saa_rhc_array[MAX_TILTS][MAX_RNG],
	     previous_cf1,
	     previous_cf2,
	     previous_cf3;
	     
static int previous_vcp = 0;
float ZS_lookup_table[256];
      	
int min_biased_dBZ,
    max_biased_dBZ;
    
int  vcpstatus		= 0;  /*status for vcpsetup */
   
/*********************************************************  
 Method : saa
 Details:
*********************************************************/	
int saa() {
	
	int biased_dBZ;
	
	int rowstart = 100; /*for debug printing */
	int colstart = 175; /*for debug printing */
	
		
	if(SAA_DEBUG){fprintf(stderr, "Entering saa function...\n");}
	min_biased_dBZ 	 = (saa_adapt.thr_lo_dBZ + 33.) * 2;
	max_biased_dBZ 	 = (saa_adapt.thr_hi_dBZ + 33.) * 2;
	
	saa_vcp_setup(currentvcpnumber,&vcpstatus);
	
	if(vcpstatus < 0){
		LE_send_msg(GL_ERROR,"saa: vcp setup failed. \n");
		return -1;
	}
	
	/* check if cf_ZS_mult or cf_ZS_power has changed, if so then build the Z/s look up table 
	 the initilized value for previous_ZS_mmult and previous_ZS_power are -1, then surely this should be done
	 at least once when the program starts kd 12/29 */
	 
	if( previous_ZS_mult !=saa_adapt.cf_ZS_mult || previous_ZS_power !=saa_adapt.cf_ZS_power)
	{
		/*** Build Z/S lookup table for snow reflectivities ***/	
		for (biased_dBZ = min_biased_dBZ; biased_dBZ < max_biased_dBZ; biased_dBZ++){
		ZS_lookup_table[biased_dBZ] = 
			biased_dBZ_to_rate(biased_dBZ, saa_adapt.cf_ZS_mult, saa_adapt.cf_ZS_power);
		}/*end for*/	
		if(SAA_DEBUG){fprintf(stderr, "Built ZS Lookup Table.\n");}
	}
	
	/*** Compute the snowfall rate. ***/
	compute_snow_rate();
	if(SAA_DEBUG){fprintf(stderr, "Computed Snow Rate.\n");}

	

	/*** Correct for range height biases. ***/			
	range_correction();
	if(SAA_DEBUG){fprintf(stderr, "Applied Range Correction.\n");}
	
	
	if(SAA_DEBUG){
		/* print a small portion of the snow_rate matrix for debugging purposes */
		fprintf(stderr,"\nRate Matrix after Range Correction...\n");
		print_saa_matrix(saa_snow_rate,5,5,rowstart,colstart);
		fprintf(stderr, "\n\n");
	}/*end if */
	
	
	/***  Compute snow and liquid water equivalent accumulations for all the SAA products.  ***/
	if(saa_compute_accumulations() == -1)   
	    return -1;                         /* Modified 12/11/2003   WDZ  */
	if(SAA_DEBUG){fprintf(stderr,"Computed Accumulations.\n");}	
	
	/*debug printing for accumulations */
	if(SAA_DEBUG){
		/* print a small portion of the scan-to-scan swe and sd matrix for debugging purposes */
		print_saa_matrix(saa_scan_to_scan_swe,5,5,rowstart,colstart);
		fprintf(stderr, "\n");
		
	}/*end if */
	
	
	/***  Build displayable products ***/
	
	
	/*copy the current snow rate to previous snow rate matrix */
	copy_to_previousrate_matrix();
	
	
	
	if(SAA_DEBUG){fprintf(stderr, "...Leaving saa function.\n");}
	return 0; 
	
}/*end function saa*/	

/*********************************************************  
 Method : read_saa_adaptation
 Details: Reads the adaptation data.  
*********************************************************/
void read_saa_adaptation() {
	
	/*Temporarily calling initializeAdaptData to initialize
	  Adaptation Data */
	initializeAdaptData(&saa_adapt);

}/* end function read_saa_adaptation */

/*********************************************************  
  Method: compute_snow_rate
  Details:Generates snow rate array from Hybrid Scan reflectivity data.  Includes data quality  
  	  steps to reject isolated data points and replace anomalously high values with the 
  	  average of surrounding bins.
*********************************************************/
void compute_snow_rate() {

	int  range,
	     radial,
	     biased_dBZ;
	
	for (radial = 0; radial < MAX_AZM; radial++){
		for (range = 0; range < MAX_RNG; range++) {
		
			snow_rate[radial][range] = 0;	/*  Initialize bin to zero*/
			biased_dBZ = hybscan_buf.HyScanZ[radial][range];
			
			if (biased_dBZ >= min_biased_dBZ){	/* else below threshold, snow rate = zero	*/
			
				if (neighbor_test(radial, range)){	/*  else no neighbors, snow rate = zero	*/
				
					if (biased_dBZ < max_biased_dBZ){
						snow_rate[radial][range] = ZS_lookup_table[biased_dBZ];
					}
					else{
						snow_rate[radial][range] = average_rate(radial, range);
					}
				}/*end if */
			}/*end if */
		}/*end for*/
	}/*end for*/
	
}/*end function snow_rate */

/*********************************************************  
  Method : neighbor_test
  Details: Checks to see if bin has a neighbor.
  
  Change:	8/12/04		Correct negative index for radial pointer 
*********************************************************/
int neighbor_test(int center_radial, int center_range) {

	int rad,bin,i;
	
	for (i = - 1; i <= 1; i++) {
	
		/*  Handle zero degree azimuth crossing	*/
	        rad = (i + center_radial) % MAX_AZM;
	        if(rad < 0)
	             rad = MAX_AZM - 1;  /* Added 8/12/2004 WDZ */
		
		switch (center_range) {
		
			case 0:			/*  First bin, just check outward for neighbor	*/
				for (bin = center_range; bin <= center_range + 1; bin++){
					if ( ( (rad != center_radial) || (bin != center_range) ) 
					    && (hybscan_buf.HyScanZ[rad][bin] >= min_biased_dBZ) )
					    	return 1;
				}/*end for */
				break;
					
			case MAX_RNG - 1:	/*  Last bin, just check inward for neighbor 	*/
				for (bin = center_range - 1; bin <= center_range; bin++){
					if ( ( (rad != center_radial) || (bin != center_range) )
					    && (hybscan_buf.HyScanZ[rad][bin] >= min_biased_dBZ) )
					    return 1;
				}/*end for */
				break;
					    
			default:
				for (bin = center_range - 1; bin <= center_range + 1; bin++){
				 	if ( ( (rad != center_radial) || (bin != center_range) )
					    && (hybscan_buf.HyScanZ[rad][bin] >= min_biased_dBZ) )
					    	return 1;
				}/*end for */
				break;
		}/*end switch */
	}/*end for */

	return 0;
	
}/*end function neighbor_test */

/*********************************************************  
 Method : average_rate
 Details: Computes the average snow rate for the surrounding bins that 
          do not exceed the max threshold.  
          The range boundaries are handled specially. 

  Change:	8/12/04		Correct negative index for radial pointer 
*********************************************************/
float average_rate(int center_radial, int center_range) {

	int rad,
	    bin,
	    count,
	    i;
	
	float sum_rate,
	      average;
	
	count 		= 0;
	sum_rate 	= 0.;
	
	for (i = - 1; i <= 1; i++) {
	
		/*  Handle zero degree azimuth crossing	*/
	        rad = (i + center_radial) % MAX_AZM;
	        if (rad < 0 )
	           rad = MAX_AZM - 1;   /* Added 8/12/2004  WDZ  */
		
		switch (center_range) {
			case 0:	/*  First bin, just average outward	*/
				for (bin = center_range; bin <= center_range + 1; bin++){
				    
					if ( (rad != center_radial) || (bin != center_range) ) 
					{   
					    /*mod 12/4 */
					    if  (hybscan_buf.HyScanZ[rad][bin] < max_biased_dBZ) 
					    {
					    	sum_rate += ZS_lookup_table[hybscan_buf.HyScanZ[rad][bin]];
					    }
					    else
					    {
					      
					    	sum_rate += ZS_lookup_table[max_biased_dBZ-1];
					    }
					    count++;
					    	
					} /*end if */
				}/*end for */
				break;
			case MAX_RNG - 1: /*  Last bin, just average inward 	*/
				for (bin = center_range - 1; bin <= center_range; bin++){
					if ( (rad != center_radial) || (bin != center_range) ) 
					{
					    if (hybscan_buf.HyScanZ[rad][bin] < max_biased_dBZ) 
					    {
					    	sum_rate += ZS_lookup_table[hybscan_buf.HyScanZ[rad][bin]];
					    }
					    else
					    {
					      
					    	sum_rate += ZS_lookup_table[max_biased_dBZ-1];
					    }
					    count++;
					    	
					} /*end if */
				}/*end for */
				break;
			default:
				for (bin = center_range - 1; bin <= center_range + 1; bin++){
					if ( (rad != center_radial) || (bin != center_range) ) 
					{
					    if (hybscan_buf.HyScanZ[rad][bin] < max_biased_dBZ) 
					    {
					    	sum_rate += ZS_lookup_table[hybscan_buf.HyScanZ[rad][bin]];
					    }
					    else
					    {
					      
					    	sum_rate += ZS_lookup_table[max_biased_dBZ-1];
					    }
					    count++;
					    	
					} /*end if */
				}/*end for */
				break;
		}/*end switch*/
	}

	/*  Compute the average, checking for zero divide error.  	*/		
	if (count > 0)
		average = sum_rate / count;
	else
		average = 0.0;
		
	return average;
	
}/*end function average_rate */
/*********************************************************/
/*********************************************************  
 Method  : print_saa_matrix
 Details : prints a matrix to stderr.  The matrix type
 	   is determined by the parameter type. Size is specified by the
           rows/cols parameters.
           rowstart and colstart determines where to start
           the printing from.
*********************************************************/
void print_saa_matrix(
enum print_Matrix_Type type,
int rows, int cols,int rowstart ,int colstart) 
{

	int i,j;

	if( (rowstart + rows >= MAX_AZM) ||
	    (colstart + cols >= MAX_RNG) ){
	     
	     fprintf(stderr,"Invalid ranges for printing saa matrix.\n");
	     return;
	}
	
	switch(type){
		case saa_snow_rate:
			fprintf(stderr,"Snow Rate Matrix from %d,%d: \n",rowstart,colstart);
			break;
		
		case saa_scan_to_scan_swe:
			fprintf(stderr,"Scan-to-Scan SWE from %d,%d: \n",rowstart,colstart);
			break;
		
		case saa_scan_to_scan_sd:
			fprintf(stderr,"Scan-to-Scan Snow Depth from %d,%d: \n",rowstart,colstart);
			break;
		
		case saa_hyscanE:
			fprintf(stderr,"HyScanE Matrix from %d,%d: \n",rowstart,colstart);
			break;
		
		default:
			fprintf(stderr,"Invalid type specified for printing. \n");
			return;
	}/*end switch */
	
	for(i = 0;i<rows;i++){
		for(j = 0;j<cols;j++){
			switch(type){
				case saa_snow_rate:
					fprintf(stderr,"%3.4f  ",snow_rate[i+rowstart][j+colstart]);
					continue;
		
				case saa_scan_to_scan_swe:
					fprintf(stderr,"%3.4f  ",scan_to_scan_swe[i+rowstart][j+colstart]);
					continue;
				
				case saa_scan_to_scan_sd:
					fprintf(stderr,"%3.4f  ",scan_to_scan_sd[i+rowstart][j+colstart]);
					continue;	
		
				case saa_hyscanE:
					fprintf(stderr,"%d  ",hybscan_buf.HyScanE[i+rowstart][j+colstart]);
					continue;
		
				default:
					break;
			}/*end switch */
		}/*end for */
		fprintf(stderr,"\n");
	}/*end for */
		
}/*end function print_saa_matrix */

/*********************************************************  
 Method : range_correction
 Details: Adjusts the snow rate values to compensate for range/height biases.  Note, this module should
 be able to use range/height corrections from the Range Correction Algorithm (RCA) when that 
 algorithm is fielded (currently scheduled for ORPG build 7), but in the interim will 
 generate a climatologically derived range height correction array whenever the VCP changes.
*********************************************************/
void range_correction()	{

	int radial,
	    range,
	    el_angle,
	    hybridscan_el_index;
	
	if (saa_adapt.use_RCA_flag){
		/*get_RCA_correction_factors();*/
	}
	else{
		/*  Rebuild the vcp info and the Range Height table if the VCP changes or range height coefficients change  */
		
		if(currentvcpnumber		!= previous_vcp){
			
			saa_vcp_setup(currentvcpnumber,&vcpstatus);
			if(vcpstatus < 0){
				LE_send_msg(GL_ERROR,"saa: vcp setup failed. \n");
				return;
			}/*end if */
			
		}/*end if */
		
		if((currentvcpnumber 		!= previous_vcp ) ||
		   (saa_adapt.cf1_rng_hgt 	!= previous_cf1 ) ||
		   (saa_adapt.cf2_rng_hgt 	!= previous_cf2 ) ||
		   (saa_adapt.cf3_rng_hgt 	!= previous_cf3 ) ){

			build_saa_rhc_array();
			
		}/*end if */
		
	}/*end else */

	/*  Apply range height correction */
	for (radial = 0; radial < MAX_AZM; radial++){
		for (range = 0; range < MAX_RNG; range++) {
			
			hybridscan_el_index = hybscan_buf.HyScanE[radial][range];
			
			if(hybridscan_el_index < 0){
				/*missing data.  set the snow_rate[radial][range] to zero */
				snow_rate[radial][range] = 0.0;
			}
			else{
				/*The HyScanE matrix contains the elevation indices.  The
			 	*elevation indices are not 0-based - they start from 1.  So,
			 	*an elevation index of 1 corresponds to the base angle.  To 
			 	*obtain the elevation angle, we need to subtract the elevation 
			 	*index by one and then look up in the vcp.el_ang array */
			 
				el_angle = vcp.el_angle[hybridscan_el_index  - 1];
				snow_rate[radial][range] = snow_rate[radial][range] * 
						   	rhc_factor(el_angle, range);
			}/*end else */
		}/*end for */
	}/*end for */
	
}/*end function range_correction */

/*********************************************************  
Method :build_saa_rhc_array
Details:Build the climatological Range Height Correction Factor array.  
*********************************************************/
void build_saa_rhc_array() {

	int tilt,
	    range,
	    rng_prime,
	    min_rng;
	    
	float el_ang,
	      sin_el,
	      sin_base = sin(deg_to_rad(saa_adapt.rhc_base_elev)),
	      height,
	      base_cf[MAX_RNG];

	/*  Compute minimum range for SAA range height correction	*/
	min_rng = (int)(radar_range(saa_adapt.thr_mn_hgt_corr, sin_base));

	for (range = 0; range < MAX_RNG; range++) 
	{
		if (range <= min_rng){
			base_cf[range] = 1.;
		}
		else{
			base_cf[range] = saa_adapt.cf1_rng_hgt 		+
					 saa_adapt.cf2_rng_hgt * range 	+
					 saa_adapt.cf3_rng_hgt * range * range;
		}/*end else */
	}
	
	for (tilt = 0; tilt < MAX_TILTS; tilt++) {
	
		/*  Convert from integer elevation angles to radians 	*/
		el_ang = vcp.el_angle[tilt] / 10.0;

		/*  Build the table					*/
		if (el_ang == saa_adapt.rhc_base_elev)
			for (range = 0; range < MAX_RNG; range++)
				saa_rhc_array[tilt][range] = base_cf[range];
		else
		{
			sin_el = sin(deg_to_rad(el_ang));
			for (range = 0; range < MAX_RNG; range++)
			{
				/*  Calculate the range of the base data with the same height as the current range  */
				height = radar_height(range, sin_el);
				rng_prime = (int)(radar_range(height, sin_base));
				if (rng_prime >= MAX_RNG){
					 rng_prime = MAX_RNG - 1;
				}
				saa_rhc_array[tilt][range] = base_cf[rng_prime];
			}/*end for */
		}/*end else */
	}/*end for */	

	/*  Define new values for SAA RHC rebuilding	*/    
	previous_vcp = currentvcpnumber;
	previous_cf1 = saa_adapt.cf1_rng_hgt;
	previous_cf2 = saa_adapt.cf2_rng_hgt;
	previous_cf3 = saa_adapt.cf3_rng_hgt;	
	
}/* end function build_saa_rhc_array */

/*********************************************************  
Method : rhc_factor
Details: Select the range height correction factor	 
*********************************************************/
 
float rhc_factor(int el_angle, int range) {

	float rhcf;	
	int tilt;

	for (tilt = 0; tilt < MAX_TILTS; tilt++){
	
		if (el_angle == vcp.el_angle[tilt]) {
			rhcf = saa_rhc_array[tilt][range];
			return(rhcf);
		}
	}/* end for */
	
	/*  If couldn't find tilt so something's wrong, complain and return.  ???Note, I need to check
    	with Steve to see how to keep this sane.  	*/
	LE_send_msg(GL_ERROR,"SAA: rhc_fctr -- Couldn't find elevation angle: %d\n", &el_angle);
	
	return(0.0);

}/*end function rhc_factor*/

/*********************************************************  
Method : copy_to_previousrate_matrix
Details: Copy the current snow rate to previous snow rate array	 
*********************************************************/
void copy_to_previousrate_matrix(){

	memcpy(&previous_snow_rate,&snow_rate,MAX_AZM*MAX_RNG*sizeof(float));
	
}/*end function copy_to_previousrate_matrix */

/*************************************************************/


