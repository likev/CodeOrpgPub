# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/11/10 15:10:47 $
# $Id: radazvd.alg,v 1.16 2014/11/10 15:10:47 steves Exp $
# $Revision: 1.16 $
# $State: Exp $

    alg_name		value = Velocity Dealiasing

    num_replace_lookahead_s		value = 10;	
			name = Short Pulse Num Replace (Look Ahead)  [NRLA];	
			type = int;		
			range = [5, 20];	
			unit = bins;		
			description = Short Pulse Number of sample bins to look ahead the;	

    num_replace_lookback_s		value = 4;	
			name = Short Pulse Num Replace (Look Back)  [NRLB];	
			type = int;		
			range = [4, 10];	
			unit = bins;		
			description = Short Pulse Number of sample bins to look back on the;	

    num_lookback_s		value = 30;	
			name = Short Pulse Number (Look Back)  [NLB];	
			type = int;		
			range = [5, 45];	
			unit = bins;		
			description = Short Pulse Number of sample bins to look back on the;	

    num_lookforward_s		value = 15;	
			name = Short Pulse Number (Look Forward)  [NLF];	
			type = int;		
			range = [10, 20];	
			unit = bins;		
			description = Short Pulse Number of sample bins to look forward on the;	

    th_consecutive_rejected_s		value = 5;	
			name = Short Pulse Thresh (Consec Reject)  [TCBR];	
			type = int;		
			range = [1, 10];	
			unit = bins;		
			description = Short Pulse Maximum number of consecutive sample bins;	

    th_maximum_missing_s		value = 30;	
			name = Short Pulse Threshold (Max Missing)  [TMM];	
			type = int;		
			range = [10, 50];	
			unit = bins;		
			description = Short Pulse Maximum number of missing sample bins allowed in;	

    num_reunfold_previous_azm_s		value = 10;	
			name = Short Pulse Num (Reunfold Prev Az)  [NRPA];	
			type = int;		
			range = [5, 20];	
			unit = bins;		
			description = Short Pulse Number of surrounding sample bins to search for;	

    num_reunfold_current_azm_s		value = 30;	
			name = Short Pulse Num (Reunfold Current Az)  [NRCA];	
			type = int;		
			range = [15, 50];	
			unit = bins;		
			description = Short Pulse Num (Reunfold Current Az);	

    th_difference_unfold_s		value = 10.0;	
			name = Short Pulse Thresh (Diff Unfold)  [TDU];	
			type = double;		
			range = [1.0, 15.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Short Pulse Maximum allowed difference in velocity between;	

    th_max_bins_with_jump_s		value = 75;	
			name = Short Pulse Thresh (Max Bins Jump)  [TMBJ];	
			type = int;		
			range = [10, 100];	
			unit = bins;		
			description = Short Pulse Maximum number of sample bins allowed between velocity;	

    th_vel_jump_fraction_radial_s		value = 0.75;	
			name = Short Pulse Thresh (Velocity Jump Fact)  [TJR];	
			type = double;		
			range = [0.50, 1.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Short Pulse Multiplied times Nyquist interval to define the maximum;	

    th_vel_jump_fraction_azm_s		value = 0.60;	
			name = Short Pulse Thresh (Az Diff Fact)  [TJA];	
			type = double;		
			range = [0.50, 1.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Short Pulse Multiplied times the Nyquist interval to define the;	

    th_scale_standard_dev_s		value = 0.40;	
			name = Short Pulse Thresh (Scale Std. Dev.)  [TSSD];	
			type = double;		
			range = [0.00, 1.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Short Pulse Muliplied times the Nyquist interval to generate a;	

    th_max_azm_with_jump_s		value = 5;	
			name = Short Pulse Thresh [Max Cont Az Jump)  [TMCJ];	
			type = int;		
			range = [1, 10];	
			unit = azimuths;		
			description = Short Pulse Maximum number of azimuths with velocity jump allowed;	

    th_scale_diff_unfold_s		value = 1.50;	
			name = Short Pulse Thresh (Scale Diff Unfld)  [TSDU];	
			type = double;		
			range = [1.00, 2.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Short Pulse Multiplied times threshold difference unfold to compute;	

    th_bins_large_azm_jump_s		value = 10;	
			name = Short Pulse Thresh (Num Az Jump)  [TBLA];	
			type = int;		
			range = [3, 12];	
			unit = bins;		
			description = Short Pulse Maximum number of bins with large velocity jumps in;	

    th_bin_first_check_s		value = 5;	
			name = Short Pulse Thresh (Radial)  [NBF];	
			type = int;		
			range = [5, 10];	
			unit = bins;		
			description = Short Pulse Number of sample bins to look back for initial radial;	

    env_winds_timeout_s		value = 720;	
			name = Short Pulse Environmental Winds Time Out;	
			type = int;		
			range = [1, 999];	
			unit = mins;		
			description = Short Pulse Age limit of environmental winds table for the data used;	

    use_sounding_s		value = Yes;	
			name = Short Pulse Flag (Sounding)  [USF];	
			type = string;		
			range = { No, Yes};	
			description = Short Pulse Flag indicating the use of the environmental winds table;	
			enum =  0, 1;	

    replace_rejected_vel_s		value = Yes;	
			name = Short Pulse Flag (Report Rejected Vel)  [ARRV];	
			type = string;		
			range = { No, Yes};	
			description = Short Pulse Default weather mode at RPG start up;	
			enum =  0, 1;	

    num_interval_checks_s		value = 4;	
			name = Short Pulse Number (Interval Checks);	
			type = int;		
			range = [0, 6];	
			description = Short Pulse Number of interval checks;	

    num_replace_lookahead_l		value = 10;	
			name = Long Pulse Num Replace (Look Ahead)  [NRLA];	
			type = int;		
			range = [5, 20];	
			unit = bins;		
			description = Long Pulse Number of sample bins to look ahead the;	

    num_replace_lookback_l		value = 4;	
			name = Long Pulse Num Replace (Look Back)  [NRLB];	
			type = int;		
			range = [4, 10];	
			unit = bins;		
			description = Long Pulse Number of sample bins to look back on the;	

    num_lookback_l		value = 30;	
			name = Long Pulse Number (Look Back)  [NLB];	
			type = int;		
			range = [5, 45];	
			unit = bins;		
			description = Long Pulse Number of sample bins to look back on the;	

    num_lookforward_l		value = 15;	
			name = Long Pulse Number (Look Forward)  [NLF];	
			type = int;		
			range = [10, 20];	
			unit = bins;		
			description = Long Pulse Number of sample bins to look forward on the;	

    th_consecutive_rejected_l		value = 5;	
			name = Long Pulse Thresh (Consec Reject)  [TCBR];	
			type = int;		
			range = [1, 10];	
			unit = bins;		
			description = Long Pulse Maximum number of consecutive sample bins;	

    th_maximum_missing_l		value = 30;	
			name = Long Pulse Threshold (Max Missing)  [TMM];	
			type = int;		
			range = [10, 50];	
			unit = bins;		
			description = Long Pulse Maximum number of missing sample bins allowed in;	

    num_reunfold_previous_azm_l		value = 10;	
			name = Long Pulse Num (Reunfold Prev Az)  [NRPA];	
			type = int;		
			range = [5, 20];	
			unit = bins;		
			description = Long Pulse Number of surrounding sample bins to search for;	

    num_reunfold_current_azm_l		value = 30;	
			name = Long Pulse Num (Reunfold Current Az)  [NRCA];	
			type = int;		
			range = [15, 50];	
			unit = bins;		
			description = Long Pulse Num (Reunfold Current Az);	

    th_difference_unfold_l		value = 3.0;	
			name = Long Pulse Thresh (Diff Unfold)  [TDU];	
			type = double;		
			range = [1.0, 15.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Long Pulse Maximum allowed difference in velocity between;	

    th_max_bins_with_jump_l		value = 40;	
			name = Long Pulse Thresh (Max Bins Jump)  [TMBJ];	
			type = int;		
			range = [10, 100];	
			unit = bins;		
			description = Long Pulse Maximum number of sample bins allowed between velocity;	

    th_vel_jump_fraction_radial_l		value = 0.75;	
			name = Long Pulse Thresh (Velocity Jump Fact)  [TJR];	
			type = double;		
			range = [0.50, 1.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Long Pulse Multiplied times Nyquist interval to define the maximum;	

    th_vel_jump_fraction_azm_l		value = 0.60;	
			name = Long Pulse Thresh (Az Diff Fact)  [TJA];	
			type = double;		
			range = [0.50, 1.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Long Pulse Multiplied times the Nyquist interval to define the;	

    th_scale_standard_dev_l		value = 0.80;	
			name = Long Pulse Thresh (Scale Std. Dev.)  [TSSD];	
			type = double;		
			range = [0.00, 1.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Long Pulse Muliplied times the Nyquist interval to generate a;	

    th_max_azm_with_jump_l		value = 2;	
			name = Long Pulse Thresh [Max Cont Az Jump)  [TMCJ];	
			type = int;		
			range = [1, 10];	
			unit = azimuths;		
			description = Long Pulse Maximum number of azimuths with velocity jump allowed;	

    th_scale_diff_unfold_l		value = 1.20;	
			name = Long Pulse Thresh (Scale Diff Unfld)  [TSDU];	
			type = double;		
			range = [1.00, 2.00];	
			accuracy = [0.01];
			unit = factor;		
			description = Long Pulse Multiplied times threshold difference unfold to compute;	

    th_bins_large_azm_jump_l		value = 10;	
			name = Long Pulse Thresh (Num Az Jump)  [TBLA];	
			type = int;		
			range = [3, 12];	
			unit = bins;		
			description = Long Pulse Maximum number of bins with large velocity jumps in;	

    th_bin_first_check_l		value = 10;	
			name = Long Pulse Thresh (Radial)  [NBF];	
			type = int;		
			range = [5, 10];	
			unit = bins;		
			description = Long Pulse Number of sample bins to look back for initial radial;	

    env_winds_timeout_l		value = 720;	
			name = Long Pulse Environmental Winds Time Out;	
			type = int;		
			range = [1, 999];	
			unit = mins;		
			description = Long Pulse Age limit of environmental winds table for the data used;	

    use_sounding_l		value = Yes;	
			name = Long Pulse Flag (Sounding)  [USF];	
			type = string;		
			range = { No, Yes};	
			description = Long Pulse Flag indicating the use of the environmental winds table;	
			enum =  0, 1;	

    replace_rejected_vel_l		value = Yes;	
			name = Long Pulse Flag (Report Rejected Vel)  [ARRV];	
			type = string;		
			range = { No, Yes};	
			description = Long Pulse Default weather mode at RPG start up;	
			enum =  0, 1;	

    num_interval_checks_l		value = 6;	
			name = Long Pulse Number (Interval Checks);	
			type = int;		
			range = [0, 6];	
			description = Long Pulse Number of interval checks;	

    disable_radaz_dealiasing		value = No;	
			name = Disable Rad/Azm Velocity Dealiasing;	
			type = string;		
			range = { No, Yes };	
			description = Disable Rad/Azm Velocity Dealiasing;	
			enum =  0, 1;	

    use_sprt_replace_rej                value = Yes;
                        name = Use Sprt Half Ext Nyquist Velocity;
                        type = string;
                        range = { No, Yes };
                        description = Use SPRT Half Extended Nyquist for replace rej;
                        enum =  0, 1;

    th_consecutive_rejected_sprt        value = 9;
                        name = Staggered PRT Thresh (Consec Reject)  [TCBR];
                        type = int;
                        range = [1, 15];
                        unit = bins;
                        description = Staggered PRT Maximum number of consecutive sample bins;

    th_max_azm_with_jump_sprt           value = 1;
                        name = Staggered PRT Thresh [Max Cont Az Jump)  [TMCJ];
                        type = int;
                        range = [1, 10];
                        unit = azimuths;
                        description = Staggered PRT Maximum number of azimuths with velocity jump allowed;

    use_2D_dealiasing	value = Yes;	
			name = Use 2D Velocity Dealiasing [2DVDA];	
			type = string;		
			range = { No, Yes };	
			enum =  0, 1;	
                        permission = [ROC, URC];
                        description = Yes sets 2DVDA as the default dealiasing algorithm unless a user selects a Velocity Increment of 1.94 kts or uses sectorized PRFs.  Then the legacy VDA is used.  VCP 121 still uses the Multiple PRF Dealiasing Algorithm.  No totally disables 2DVDA.;

