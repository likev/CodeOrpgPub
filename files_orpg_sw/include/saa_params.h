/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:58 $
 * $Id: saa_params.h,v 1.4 2007/01/30 22:56:58 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef SAA_ADAPT_H
#define SAA_ADAPT_H

#define SAA_DEA_NAME "alg.saa"

/*
   All parameter definitions are explained in the get_adapt_params
   routine
*/

typedef struct
{
    float g_cf_ZS_mult;       /*#  @name "Z-S Multiplicative Coefficient"
    				 @default 120.0  @min 10.0 @max 1000.0    
    			 	 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @precision 1                                        */     
    					
    float g_cf_ZS_power;      /*#  @name "Z-S Power Coefficient"
    				 @default 2.00  @min 1.00 @max 3.00         
				 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @precision 2                                */
    					
    float g_sw_ratio;        /*#  @name "Snow - Water Ratio"
    			   	 @default 11.8 @min 4.0 @max 100.0
    			   	 @precision 1
    				 @authority "READ | OSF_WRITE | URC_WRITE"  */
    				 
    					
    float g_thr_mn_hgt_corr;  /*#  @name "Minimum Height Correction Threshold"
    				 @default 0.45  @min 0.01 @max 20.00
     			 	 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @precision 2
   				 @units "km"                                 */
    					
    float g_cf1_rng_hgt;      /*#  @name "Range Height Correction Coefficient #1"
    				@default 0.84144  @min -5.0000 @max  5.0000
    			 	@authority "READ | OSF_WRITE | URC_WRITE"
    				@precision 4                                */
    					
    float g_cf2_rng_hgt;     /*#  @name "Range Height Correction Coefficient #2"
    				@default 0.0040  @min -0.5000 @max 0.5000
    			 	@authority "READ | OSF_WRITE | URC_WRITE"
    				@precision 4 		     		   */

    float g_cf3_rng_hgt;    /*#  @name "Range Height Correction Coefficient #3"
    				@default 0.0000  @min -0.0010 @max 0.0010
    			 	@authority "READ | OSF_WRITE | URC_WRITE"
    			       @precision 4                    	                   */

    int g_use_RCA_flag;        /*#  @name "RCA Correction Flag"
			        @enum_values "No", "Yes"
			        @default "No"                              */

    float g_thr_lo_dBZ;       /*#  @name "Minimum Reflectivity/Isolated Bin Threshold"
    			 	 @default 5.0 @min -10.0 @max 25.0
				 @precision 1
    				 @units "dBZ"                                 */
    					
    float g_thr_hi_dBZ;       /*#  @name "Maximum Reflectivity/Outlier Bin Threshold"
     				 @default 40.0  @min 30.0  @max 55.0
     				 @precision 1
     				 @units "dBZ"                                              */

    int g_thr_time_span;   /*#  @name "Time Span Threshold"
     				@default 11 @min 3 @max 30
    				@units "mins"   	        */

    int g_thr_mn_time;    /*#  @name "Minimum Time Threshold"
    				 @default 54  @min 0 @max 60
    				 @units "mins"                             */

    float g_rhc_base_elev;     /*#  @name "Base Elevation for Default Range Height Correction"
    				 @default 0.5 @min 0.1 @max 2.0
    				 @precision 1
    				 @units "degrees"                                 */

} saa_adapt_params_t;

/**
 *      Declare a C++ wrapper class that statically contains a pointer to the meta
 *           data for this structure.  The C++ wrapper class allows the IOCProperty API
 *           to be implemented automatically for this structure
 *       
 **/

#endif

