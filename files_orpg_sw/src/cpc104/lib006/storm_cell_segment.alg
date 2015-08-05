# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/12 20:57:07 $
# $Id: storm_cell_segment.alg,v 1.9 2010/05/12 20:57:07 ccalvert Exp $
# $Revision: 1.9 $
# $State: Exp $

    alg_name 		value = Storm Cell Segments

    refl_threshold	value = 60, 55, 50, 45, 40, 35, 30;	
			name = Thresh - Reflectivity #;	
			type = int;		
			range = [0, 80];	
			unit = dBZ;		
			description = Minimum effective reflectivity which the sample;	

    seg_length		value = 1.9, 1.9, 1.9, 1.9, 1.9, 1.9, 1.9;	
			name = Thresh - Segment Length #;	
			type = double;		
			range = [1.0, 5.0];	
			accuracy = [0.1];
			unit = km;		
			description = Minimum length of consecutive sample bins with;	

    dropout_refl_diff	value = 5;	
			name = Thresh (Dropout Ref Diff);	
			type = int;		
			range = [0, 10];	
			unit = dBZ;		
			description = The difference in effective reflectivity of sample volumes;	

    dropout_count	value = 2;	
			name = Thresh (Dropout Count);	
			type = int;		
			range = [0, 5];	
			unit = volumes;		
			description = Maximum number of contiguous sample volumes with a;	

    num_refl_levels	value = 7;	
			name = Nbr Reflectivity Levels;	
			type = int;		
			range = [0, 7];	
			description = Number of reflectivity thresholds used to search for segments.;	

    max_segment_range	value = 460;	
			name = Thresh (Max Segment Range);	
			type = int;		
			range = [230, 460];	
			unit = km;		
			description = Maximum slant range for processing or identifying segments.;	

    max_segs_per_radial	value = 15;	
			name = Max # of Segments/Radial;	
			type = int;		
			range = [10, 50];	
			unit = storm segments;		
			description = Maximum number of segments to be processed per radial.;	

    max_segs_per_elev	value = 6000;	
			name = Max # of Segments/Elevation;	
			type = int;		
			range = [4000, 6000];	
			unit = storm segments;		
			description = Maximum number of segments to be processed per elevation.;	

    refl_ave_factor	value = 3;	
			name = Reflectivity Avg Factor;	
			type = int;		
			range = [1, 5];	
			unit = bins;		
			description = Number of sample volumes used for determining the maximum;	

    mass_weight_factor	value = 53000.0;	
			name = Mass Weighted Factor;	
			type = double;		
			range = [50000, 60000];	
			accuracy = [0.1];
			unit = hr/kg/km^4;		
			description = Factor for converting precipitation intensity to mass of;	

    mass_mult_factor	value = 486.0;	
			name = Mass Multiplicative Factor;	
			type = double;		
			range = [450, 550];	
			accuracy = [0.1];
			unit = (mmi^6/m^3)*(hr/mm)^n;		
			description = Multiplicative factor used in determining precipitation;	

    mass_coef_factor	value = 1.37;	
			name = Mass Coefficient Factor;	
			type = double;		
			range = [1.20, 1.50];	
			accuracy = [0.01];
			unit = exponent;		
			description = Power coefficient in the reflectivity to rainfall rate;	

    filter_kernel_size  value = 7.0;
                        name = Filter Kernel Size;
                        type = double; 
                        range = [1.0, 10.0];
                        accuracy = [0.1];
                        unit = km;
                        description = Filter kernel size used in filtering reflectivity;

    filter_fract_req    value = 0.5;
 			name = Thresh - Filter Fraction;
                        type = double;
			range = [0.0, 1.0];
			accuracy = [0.01];
			description = Unmissed-data fraction required for filtering;
   			
    filter_on		value = Yes;
			name = Reflectivity Filtered;
			type = string;
			range = {No, Yes};
			permission = [ROC, URC];
  			enum = 0, 1; 
                        description = Toggles on or off a reflectivity smoothing function.  This function is used to improve time continuity of storm cell tracks That is, it lengthens the average track duration and reduces number of cells that are tracked.  Other cell attributes such as max reflectivity and the cell-based VIL are not affected by the smoothing function.;

