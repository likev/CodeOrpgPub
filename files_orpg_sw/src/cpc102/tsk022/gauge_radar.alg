# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/04/13 22:53:05 $
# $Id: gauge_radar.alg,v 1.3 2011/04/13 22:53:05 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

   alg_name		value = Gauge Radar

   use_only_nonzero	value = Yes;
			name = Use Only Non-zero Gauges;
			type = string;
			range = {No, Yes};
			enum = 0, 1;
			description = If 'Yes', only DP-gauge and PPS-gauge \
					pairs where at least one of the gauge, \
					DP, or PPS is non-zero will be added to \
					the products, the hourly .txt file and \
					the storm .txt file. If 'No', add all pairs, \
					even when the gauge, DP, and PPS are all zero.;

   stats_everywhere	value = Yes;
			name = Print Statistics Everywhere;
			type = string;
			range = {No, Yes};
			enum = 0, 1;
			description = 'Yes' adds the statistics set to \
					the product TAB, the hourly .txt file \
					and the storm .txt file. 'No' prints \
					statistics only to the product TAB.;

   storm_print_volume	value = 10;
			name = Storm Print Volume;
			type = double;
			range = [2, 20];
			accuracy = [1];
			description = A storm product will be generated every \
					Storm Print Volume. The default is to \
					generate a storm product only after the \
					end of a storm. If storm_print_volume is 10, \
					a storm product will be generated every 10th volume.;

   storm_print_time	value = 2010_10_11_02_55_00;
			name = Storm Print Time;
			type = string;
			description = The first volume after the Storm Print Time \
					produces a storm product. The default is \
					to generate a storm product only after the end \
					of a storm, which may be hard to determine. \
					Format: CCYY_MM_DD_HH_MM_SS (Zulu).;
