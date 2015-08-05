# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2011/06/14 15:19:42 $
# $Id: avset.alg,v 1.3 2011/06/14 15:19:42 steves Exp $
# $Revision: 1.3 $
# $State: Exp $


   alg_name					value = AVSET

   enable_avset					value = Yes;
                                                name = AVSET Enable/Disable Flag;
                                                type = string;
                                                range = { No, Yes };
                                                enum = 0, 1;

   low_ref_thresh				value = 18.0;
                                                name = AVSET Low Ref Threshold;
                                                type = double;
                                                range = [0.0, 30.0];
                                                unit = dBZ;

   low_ref_area_thresh				value = 80.0;
                                                name = AVSET Low Ref Area Threshold;
                                                type = double;
                                                range = [0.0, 2000.0];
                                                unit = km^2;

   small_core_ref_thresh			value = 38.0;
                                                name = AVSET Low Ref Threshold;
                                                type = double;
                                                range = [0.0, 50.0];
                                                unit = dBZ;

   small_core_ref_area_thresh			value = 8.0;
                                                name = AVSET Low Ref Area Threshold;
                                                type = double;
                                                range = [0.0, 2000.0];
                                                unit = km^2;

   high_ref_thresh				value = 30.0;
                                                name = AVSET High Ref Threshold;
                                                type = double;
                                                range = [0.0, 50.0];
                                                unit = dBZ;

   high_ref_area_thresh				value = 30.0;
                                                name = AVSET High Ref Area Threshold;
                                                type = double;
                                                range = [0.0, 2000.0];
                                                unit = km^2;

   area_increase				value = 12.0;
                                                name = AVSET Increase in Area From Last Scan;
                                                type = double;
                                                range = [0.0, 2000.0];
                                                unit = km^2;

   low_ref_thresh_2nd_pass			value = 18.0;
                                                name = AVSET Low Ref Threshold 2nd Pass;
                                                type = double;
                                                range = [0.0, 30.0];
                                                unit = dBZ;

   low_ref_area_thresh_2nd_pass			value = 25.0;
                                                name = AVSET Low Ref Area Threshold 2nd Pass;
                                                type = double;
                                                range = [0.0, 2000.0];
                                                unit = km^2;

   high_ref_thresh_2nd_pass			value = 25.0;
                                                name = AVSET High Ref Threshold 2nd_pass;
                                                type = double;
                                                range = [0.0, 50.0];
                                                unit = dBZ;

   high_ref_area_thresh_2nd_pass		value = 10.0;
                                                name = AVSET High Ref Area Threshold 2nd_pass;
                                                type = double;
                                                range = [0.0, 2000.0];
                                                unit = km^2;

   max_time_diff				value = 15.0;
						name = AVSET Maximum Time Between Volume Scans;
                                                type = int;
                                                range = [0, 30];
                                                unit = min;

   elev_tolerance				value = 0.3;
						name = AVSET Elevation Tolerance 2nd Pass
						type = double;
						range = [0.0, 1.0];
                                                unit = deg;
