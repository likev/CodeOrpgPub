# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/09/26 21:35:40 $
# $Id: superob.alg,v 1.5 2011/09/26 21:35:40 ccalvert Exp $
# $Revision: 1.5 $
# $State: Exp $

    alg_name  		value = Superob Adaptation

    rangemax		value = 100;	\
			name = Maximum Range;	\
			type = int;		\
			range = [60, 230];	\
			unit = km;		\
			description = max radar range;	

    deltr		value = 5;	\
			name = Cell Range Size;	\
			type = int;		\
			range = [1, 10];	\
			unit = km;		\
			description = radial dimention of averaging cell;	

    deltaz		value = 6;	\
			name = Cell Azimuth Size;	\
			type = int;		\
			range = [2, 12];	\
			unit = degrees;		\
			description = azimuthal dimention of cell;	

    deltt		value = 30;	\
			name = Time Radius;	\
			type = int;		\
			range = [5, 90];	\
			unit = mins;		\
			description = radius of time window;	

    offset_min		value = 15;	\
			name = Data collection offset;	\
			type = int;		\
			range = [0, 59];	\
			unit = mins;		\
			description = minutes to offset data collection time;	

    min_sample_size	value = 50;	\
			name = Minimum Number of Points;	\
			type = int;		\
			range = [20, 200];	\
			description = minumum sample size;	

