# RCS info
# $Author: cheryls $
# $Locker:  $
# $Date: 2004/04/02 21:43:20 $
# $Id: hydromet_acc.alg,v 1.5 2004/04/02 21:43:20 cheryls Exp $
# $Revision: 1.5 $
# $State: Exp $

    alg_name		value = Hydromet Accumulation

    restart_time	value = 60;	
			name = Re-initialization Time Lapse Threshold (For Accum Process)  [TIMRS];	
			type = int;		
			range = [45, 60];	
			unit = mins;		
			description = Elapsed time to restart.;	

    max_interp_time	value = 30;	
			name = Max Time Difference Between Scans For Interpolation  [MXTIN];	
			type = int;		
			range = [15, 60];	
			unit = mins;		
			description = Maximum period over which accumulation can be computed using two;	

    min_time_period	value = 54;	
			name = Min Time Needed to Accumulate Hourly Totals  [MNTIP];	
			type = int;		
			range = [0, 60];	
			unit = mins;		
			description = Minimum period of accumulation scan data withing an hourly;	

    hourly_outlier	value = 400;	
			name = Threshold for Hourly Outlier Accumulation  [THRLI];	
			type = int;		
			range = [50, 800];	
			unit = mm;		
			description = Maximum hourly rainfall amount allowed.;	

    end_gage_time	value = 0;	
			name = Hourly Gage Accumulation Scan Ending Time  [ENGAG];	
			type = int;		
			range = [0, 59];	
			unit = mins;		
			description = Time within each hour when gage and radar accumlations are required for adjustment.;	

    max_period_acc	value = 400;	
			name = Max Accumulation per Scan-to-Scan Period  [MXPAC];	
			type = int;		
			range = [50, 400];	
			unit = mm;		
			description = Max Accumulation per scan;	

    max_hourly_acc	value = 800;	
			name = Max Accumulation per Hourly Period  [MXHAC];	
			type = int;		
			range = [50, 1600];	
			unit = mm;		
			description = Max Accumulation per Hour;	

