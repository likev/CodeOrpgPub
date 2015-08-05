# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2004/03/24 22:14:06 $
# $Id: layer_reflectivity.alg,v 1.5 2004/03/24 22:14:06 steves Exp $
# $Revision: 1.5 $
# $State: Exp $

    alg_name		value = AP Removal

    min_cltr_dbz	value = 5.0;	
			name = Min Clutter Reflectivity;	
			type = double;		
			range = [5.0, 20.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = Minimum clutter;	

    altitude_omit	value = 1;	
			name = Omit All Altitude;	
			type = int;		
			range = [0, 5];	
			unit = km;		
			description = Omit All Altitude;	

    altitude_accept	value = 3;	
			name = Accept if Altitude;	
			type = int;		
			range = [0, 10];	
			unit = km;		
			description = Accept if Altitude;	

    distance_omit	value = 45;	
			name = Omit All Distance (Outer);	
			type = int;		
			range = [0, 100];	
			unit = km;		
			description = Omit All Distance Outer;	

    distance_accept	value = 103;	
			name = Accept if Distance (Outer);	
			type = int;		
			range = [0, 300];	
			unit = km;		
			description = Accept if distance outer;	

    distance_reject	value = 230;	
			name = Reject if Distance (Outer);	
			type = int;		
			range = [0, 300];	
			unit = km;		
			description = Reject if distance outer;	

    elevation_accept	value = 0.5;	
			name = Accept if Maximum Elevation;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = degrees;		
			description = Accept if Maximum Elevation;	

    elevation_reject	value = 5.0;	
			name = Reject if Maximum Elevation;	
			type = double;		
			range = [0.0, 15.0];	
			accuracy = [0.1];
			unit = degrees;		
			description = Reject if Maximum Elevation;	

    velocity_reject	value = 2.0;	
			name = Reject if Minimum Velocity;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Reject if minimum velocity;	

    width_reject	value = 2.0;	
			name = Reject if Minimum Spectrum Width;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = km;		
			description = Reject if minimum spectrum width;	

    velocity_accept	value = 2.0;	
			name = Accept if Minimum Velocity;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Accept if minimum velocity;	

    width_accept	value = 2.0;	
			name = Accept if Minimum Spectrum Width;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Accept if minimum spectrum width;	

    cbd_phase		value = Yes;	
			name = Clutter Bloom/Dilation(CBD) Phase;	
			type = string;		
			range = { No, Yes};	
			description = Clutter Bloom/Dilation(CBD) Phase;	
			enum =  0, 1;	

    cbd_bins		value = 8;	
			name = CBD Maximum # Range Bins;	
			type = int;		
			range = [0, 20];	
			unit = bins;		
			description = CBD Maximum # Range Bins;	

    cbd_dbz		value = 30.0;	
			name = CBD Maximum Reflectivity;	
			type = double;		
			range = [0.0, 30.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = CBD Maximum Reflectivity;	

    median_phase	value = Yes;	
			name = Median Averaging(MA) Phase;	
			type = string;		
			range = { No, Yes};	
			description = Median Averaging(MA) Phase;	
			enum =  0, 1;	

    median_bins		value = 4;	
			name = MA Maximum Range Bin Difference;	
			type = int;		
			range = [0, 5];	
			unit = bins;		
			description = MA Maximum Range Bin Difference;	

    median_range	value = 2;	
			name = MA Maximum Cross Range;	
			type = int;		
			range = [0, 10];	
			unit = km;		
			description = MA Maximum Cross Range;	

    median_dbz		value = 90.0;	
			name = Median Filter Percent Good;	
			type = double;		
			range = [0.0, 100.0];	
			accuracy = [0.1];
			unit = %;		
			description = Median averaging filter percent good;	

