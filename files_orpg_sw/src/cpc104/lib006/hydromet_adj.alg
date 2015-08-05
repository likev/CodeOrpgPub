# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/18 20:14:58 $
# $Id: hydromet_adj.alg,v 1.9 2010/05/18 20:14:58 ccalvert Exp $
# $Revision: 1.9 $
# $State: Exp $

    alg_name 		value = Hydromet Adjustment

    time_bias		value = 50;	
			name = Minutes After Clock Hour When Bias is Updated  [TBIES];	
			type = int;		
			range = [50, 59];	
			unit = mins;		
			description = Number of minutes after the top of each hour to begin bias \
                                      computation process. This value should be at least 25, \
                                      because AWIPS normally re-computes bias values 24 minutes \
                                      after the top of the hour.;	

    num_grpairs		value = 10;	
			name = Threshold # of Gage/Radar Pairs Needed to Select Bias from Table [NGRPS];	
			type = int;		
			range = [6, 30];
			permission = [ROC, URC];	
			description = The minimum # of locations having both radar and gage data when selecting a bias value from the table provided by a local MPE or by an RFC.;

    reset_bias		value = 1.0;	
			name = Reset Value of Gage/Radar Bias Estimate  [RESBI];	
			type = double;		
			range = [0.5, 2.0];	
			accuracy = [0.1];
			description = A default bias value used when an AWIPS-generated bias \
                                      value is not available.;

    longst_lag		value = 168;	
			name = Longest Time Lag for Using Bias Value from Table [LGLAG];	
			type = int;		
			range = [100, 1000];	
			unit = hrs;		
			description = Longest amount of time since receipt of a valid bias table \
                                      that the table can be used to compute a bias value;

    bias_flag		value = False;	
			name = Bias Flag;	
			type = string;		
			range = { False, True};	
			permission = [ROC, URC];
			description = If False, do not use the bias provided by local MPE or by RFC; if True, the bias specified in the local MPE software will be applied.;	
                        enum =  0, 1;
