# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/06/23 19:37:54 $
# $Id: bragg.alg,v 1.1 2014/06/23 19:37:54 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

   alg_name			value = Bragg Scatter

   min_bragg_bin		value = 40;
				name = Minimum Bragg Bin;
				type = int;
				range = [0, 1000];
				description = Radial bin at which to start search for Bragg scatter returns;

   max_bragg_bin		value = 320;
				name = Maximum Bragg Bin;
				type = int;
				range = [0, 1000];
				description = Radial bin at which to stop search for Bragg scatter returns;

   min_bragg_z			value = -32.0;
				name = Minimum Bragg Reflectivity;
				type = double;
				range = [-32.0, 10];
				unit = dBZ;
				description = Reflectivity above which may be considered Bragg scatter;

   max_bragg_z			value = 10.0;
				name = Maximum Bragg Reflectivity;
				type = double;
				range = [0.0, 20.0];
				unit = dBZ;
				description = Reflectivity below which may be considered Bragg scatter;

   min_bragg_v			value = 2.0;
				name = Absolute value Bragg Velocity;
				type = double;
				range = [0.0, 6.0];
				unit = m/s;
				description = Absolute value of velocity above which may be considered Bragg scatter;

   min_bragg_sw			value = 0.0;
				name = Minimum Bragg Spectrum Width;
				type = double;
				range = [0.0, 30.0];
				unit = m/s;
				description = Spectrum width above which may be considered Bragg scatter;

   max_bragg_sw			value = 30.0;
				name = Maximum Bragg Spectrum Width;
				type = double;
				range = [0.0, 30.0];
				unit = m/s;
				description = Spectrum width below which may be considered Bragg scatter;

   min_bragg_rho		value = 0.98;
				name = Minimum Bragg Correlation Coefficient;
				type = double;
				range = [0.5, 1.05];
				description = Correlation coefficient above which may be considered Bragg scatter;

   max_bragg_rho		value = 1.05;
				name = Maximum Bragg Correlation Coefficient;
				type = double;
				range = [0.5, 1.05];
				description = Correlation coefficient below which may be considered Bragg scatter;

   min_bragg_snr		value = -5.0;
				name = Minimum Bragg SNR;
				type = double;
				range = [-32.0, 30.0];
				unit = dB;
				description = Signal-to-noise ratio above which may be considered Bragg scatter;

   max_bragg_snr		value = 15.0;
				name = Maximum Bragg SNR;
				type = double;
				range = [-32.0, 30.0];
				unit = dB;
				description = Signal-to-noise ratio below which may be considered Bragg scatter;

   min_bragg_phi		value = 0.0;
				name = Minimum Bragg Differential Phase;
				type = double;
				range = [0.0, 400.0];
				unit = deg;
				description = Differential phase above which may be considered Bragg scatter;

   max_bragg_phi		value = 400;
				name = Maximum Bragg Differential Phase;
				type = double;
				range = [0.0, 400.0];
				unit = deg;
				description = Differential phase below which may be considered Bragg scatter;

   min_bragg_cnt_vol		value = 600;
				name = Minimum Bragg Bins Volume;
				type = int;
				range = [400, 1000];
				description = Minimum Bragg bin count for a volume scan;

   min_bragg_cnt		value = 10000;
				name = Minimum Bragg Bins;
				type = int;
				range = [5000, 25000];
				description = Minimum Bragg bin count for all volumes considered;

   zdr_iqr_thresh		value = 0.9;
				name = Maximum ZDR IQR;
				type = double;
				range = [0.5, 3.0];
				unit = dB;
				description = Maximum differential reflectivity interquartile range (biota filter);

   filter_z_pct			value = 0.90;
				name = Reflectivity Filter Percentage;
				type = double;
				range = [0.0, 1.0];
				description = Reflectivity percentile for precipitation filter;

   filter_z_thresh		value = -3.0;
				name = Reflectivity Percentile Threshold;
				type = double;
				range = [-10.0, 10.0];
				unit = dBZ;
				description = Reflectivity threshold for precipitation filter;
