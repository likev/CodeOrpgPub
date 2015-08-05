# RCS info
# $Author: ryans $
# $Locker:  $
# $Date: 2005/09/13 18:19:04 $
# $Id: storm_cell_track.alg,v 1.12 2005/09/13 18:19:04 ryans Exp $
# $Revision: 1.12 $
# $State: Exp $

    alg_name		value = Storm Cell Tracking

    num_past_vols	value = 10;	
			name = Number of Past Volumes;	
			type = int;		
			range = [7, 13];	
			unit = volumes;		
			description = Maximum number of volume scans used in correlation table.;	

    num_intvls		value = 4;	
			name = Number of Intervals;	
			type = int;		
			range = [1, 4];	
			description = Number of projected positions in the storm forecast.;	

    forecast_intvl	value = 15;	
			name = Forecast Interval;	
			type = int;		
			range = [5, 30];
			accuracy = [5];	
			unit = increments of 5 mins;		
			description = Time interval for which centroid positions may be projected;	

    allow_err		value = 20;	
			name = Allowable Error;	
			type = int;		
			range = [10, 60];	
			unit = km;		
			description = Maximum error in the track of a storm cell allowed for minimum;	

    err_intvl		value = 15;	
			name = Error Interval;	
			type = int;		
			range = [5, 30];	
			accuracy = [5];
			unit = increments of 5 mins;		
			description = Interval used in determining the allowable error.;	

    default_dir		value = 225;	
			name = Default (Direction);	
			type = int;		
			range = [0, 360];	
			unit = degrees;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Default direction assigned to storm cell when storm cell;	

    max_time		value = 20;	
			name = Time (Maximum);	
			type = int;		
			range = [10, 60];	
			unit = mins;		
			description = Maximum time between successive volume scans for the storm cell;	

    default_spd		value = 25.0;	
			name = Default (Speed);	
			type = double;		
			range = [0.0, 99.9];	
			accuracy = [0.1];
			unit = knots;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Default average speed assigned to storm cell when storm cell;	

    correlation_spd	value = 30.0;	
			name = Correlation Speed;	
			type = double;		
			range = [10.0, 99.9];	
			accuracy = [0.1];
			unit = m/s;		
			description = Speed used to compute the correlation distance.;	

    minimum_spd		value = 2.5;	
			name = Thresh (Minimum Speed);	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Threshold of speed below which a storm is labeled as slow and;	

