/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/05/03 14:30:07 $
 * $Id: mpda_parameters.h,v 1.5 2011/05/03 14:30:07 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef MPDAALG_ADAPT_H
#define MPDAALG_ADAPT_H

#define MPDA_DEA_NAME "alg.mpda"


/*
   All parameter definitions are explained in the get_adapt_params
   routine
*/

typedef struct
{
    float gui_mpda_tover;   /*#  @name "Threshold (Range Unfold Power Difference)"
    				 @desc "Minimum power difference required between echoes ",
    				       "at ranges corresponding to the 1st, 2nd, 3rd, and ",
    				       "4th trips to unfold data from the 2nd and 3rd ",
    				       "Doppler scans."
    				 @default 5.0  @min 0.0 @max 20.0         
				 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @units "dBZ"
    				 @precision 1                                */
    					
    short gui_min_trip_fix; /*#  @name "Threshold (Fix Trip Minimum Bin)"
     			         @desc "Number of bins before the end of the first trip ",
     			                "to flag as missing.  Applied to all three ",
     			                "velocity fields."
    			   	 @default 0 @min -4 @max 0
    				 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @units "bins"                               */
    					
    short gui_max_trip_fix; /*#  @name "Threshold (Fix Trip Maximum Bin)"
    			         @desc "Number of bins after the end of the first trip ",
    			               "to flag as missing.  Applied to all three ",
    			               "velocity fields."
    			 	 @default -1 @min -1 @max 16
    				 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @units "bins"                                 */
    					
    short gui_th_overlap_size; /*#  @name "Threshold (Tight Overlap Size)"
    			         @desc "Threshold used for matching differences between ",
    			               "triplets. Must fall within +/- of value.."
    			 	 @default 2 @min 2 @max 8
    				 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @units "m/s"                                 */
    					
    short gui_th_overlap_relax; /*#  @name "Threshold (Loose Overlap Size)"
    			         @desc "Threshold used for matching differences between ",
    			               "triplets. Must fall within +/- of value.."
    			 	 @default 2 @min 2 @max 12
    				 @authority "READ | OSF_WRITE | URC_WRITE"
    				 @units "m/s"                                 */
    					
} mpda_adapt_params_t;

/**
 *      Declare a C++ wrapper class that statically contains a pointer to the meta
 *           data for this structure.  The C++ wrapper class allows the IOCProperty API
 *           to be implemented automatically for this structure
 *       
 **/

#endif

