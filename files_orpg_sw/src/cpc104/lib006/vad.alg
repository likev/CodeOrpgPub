# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2011/06/08 19:19:00 $
# $Id: vad.alg,v 1.10 2011/06/08 19:19:00 steves Exp $
# $Revision: 1.10 $
# $State: Exp $

    alg_name		value = VAD

    thresh_velocity	value = 5.0;	
			name = RMS Threshold  [THV];	
			type = double;		
			range = [0.0, 15.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Maximum scatter around the fitted sine wave allowed for;	

    num_fit_tests	value = 2;	
			name = Number Of Passes  [FT];	
			type = int;		
			range = [1, 5];	
			description = Number of passes through data (throwing out anomalous;	

    min_samples		value = 25;	
			name = Data Points Threshold  [NPTS];	
			type = int;		
			range = [1, 360];	
			description = Minimum number of data points to allow Fourier least;	

    anal_range		value = 30.0;	
			name = VAD Analysis Slant Range  [VAD];	
			type = double;		
			range = [1.0, 230.0];	
			accuracy = [0.1];
			unit = km;		
			permission = [ROC, URC];
			description = Slant range at which the VAD analysis is performed.;	
			misc = MERGE@-Not_merge		

    start_azimuth	value = 0.0;	
			name = Beginning Azimuth Angle  [TBZ];	
			type = double;		
			range = [0.0, 359.9];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Beginning azimuth angle for VAD analysis (represents the counter clockwise limit for the defined sector).;	

    end_azimuth		value = 0.0;	
			name = Ending Azimuth Angle  [TEZ];	
			type = double;		
			range = [0.0, 359.9];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Ending azimuth angle for VAD analysis (represents the clockwise limit for the defined sector).;	

    symmetry		value = 7.0;	
			name = Symmetry Threshold  [THY];	
			type = double;		
			range = [0.0, 20.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Maximum offset of average of fitted points from;	


   enhanced_vad		value = Yes;
			range = {No, Yes};
			enum = 0, 1;
			name = EVWP - Enables Enhanced VAD logic;
			type = string;
                        description = Enable Enhanced VADs;

    min_points		value = 25.0;	
			name = EVWP - Min Points Threshold;	
			type = int;		
			range = [0, 100];	
			description = Minimum Number of Points to Accept VAD;	

    min_symmetry	value = -6.0;	
			name = EVWP - Minimum Symmetry Value;	
			type = float;		
			range = [-10.0, -1.0];	
			accuracy = [0.1];
			description = Minimum Symmetry to Accept VAD;	

    scale_rms		value = 2.0;	
			name = EVWP - RMS Scaling Factor;	
			type = float;		
			range = [1.0, 5.0];	
			accuracy = [0.1];
			description = RMS Scaling Factor to Accept VAD;	

    min_proc_range	value = 10.0;	
			name = EVWP - Minimum VAD Processing Range;	
			type = float;		
			range = [5.0, 30.0];	
			unit = km;	
                        accuracy = [0.1];
			description = Minimum VAD Processing Range;	

    max_proc_range	value = 120.0;	
			name = EVWP - Maximum VAD Processing Range;	
			type = float;		
			range = [30.0, 120.0];	
			unit = km;	
			accuracy = [0.1];
			description = Maximum VAD Processing Range;	

