# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/12/10 17:45:48 $
# $Id: dpprep.alg,v 1.8 2014/12/10 17:45:48 steves Exp $
# $Revision: 1.8 $
# $State: Exp $

   alg_name			value = DP Preprocessor

   max_diff_phidp		value = 100.0;
				name = Max Phidp Diff Thresh;
				description = Maximum Phidp difference for acceptable data;
				type = double;
				unit = degree;

   max_diff_dbz			value = 50.0;
				name = Max DBZ Diff Thresh;
				description = Maximum DBZ difference for acceptable data;
				type = double;
				unit = Dbz;

   corr_thresh			value = 0.9;
				name = Corr Thresh for Kdp Calc;
				description = Correlation threshold for Kdp calculation;
				type = double;
				range = [.5, 1.];

   dbz_thresh			value = 40.;
				name = Min DBZ for Short Gate Kdp;
				description = Minimum DBZ for accepting short gate Kdp;
				type = double;

   dbz_window			value = 3;
				name = DBZ Window;
				description = Window size for DBZ smoothing;
				type = int;
				range = {1, 3, 5};	

   window			value = 5;
				name = Window Size;
				description = Window size for smoothing other fields;
				type = int;
				range = {3, 5, 7};

   short_gate			value = 9;
				name = Short Gate Window Size;
				description = Window size for short gate Kdp calculation;
				type = int;

   long_gate			value = 25;
				type = int;
				name = Long Gate Window Size;
				description = Window size for long gate Kdp calculation;

   md_snr_thresh		value = 5.0;
				name = SNR Thresh for meteo data;
				description = SNR threshold for identifying meteo data;
				type = double;
				range = [3., 10.];

   art_start_bin		value = 180;
				name = start bin for HAPR;
				description = start bin for high-attenuation-presence-radial test;
				type = int;
				range = [60, 1200];

   art_count			value = 10;
				name = minimum number of high-attenuation bins for HAPR;
				description = minimum number of high-attenuation bins for high-attenuation-presence-radial test;
				type = int;
				range = [5, 50];

   art_min_z			value = 30.0;
				name = Minimum Z for high-attenuation bin;
				description = Minimum Z for high-attenuation bin testing;
				type = double;
				range = [20., 40.];

   art_max_z			value = 50.0;
				name = Maximum Z for high-attenuation bin;
				description = Maximum Z for high-attenuation bin testing;
				type = double;
				range = [30., 70.];

   art_v			value = 1.0;
				name = Minimum V for high-attenuation bin;
				description = Minimum V for high-attenuation bin testing;
				type = double;
				range = [0.5, 5.];

   art_corr			value = 0.8;
				name = Correlation threshold for high-attenuation bin;
				description = Correlation threshold for high-attenuation bin testing;
				type = double;
				range = [0.5, 0.9];

   art_min_sw			value = 2.0;
				name = Minimum SW threshold for high-attenuation bin;
				description = Minimum SW threshold for high-attenuation bin testing;
				type = double;
				range = [0.0, 6.0];

   isdp_apply                   value = NO;
                                name = Apply RPG estimated ISDP?;
                                description = If YES, PhiDP data in RPG is adjusted using RPG estimated ISDP;
                                type = string;
                                range = {YES, NO};
                                permission = [ROC, URC]

