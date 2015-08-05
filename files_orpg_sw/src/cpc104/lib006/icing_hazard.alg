# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2012/09/14 14:26:24 $
# $Id: icing_hazard.alg,v 1.1 2012/09/14 14:26:24 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

###########################################################
##
##	File:		icing_hazard.alg
##	Author:		Robert Hallowell
##	Date:		August 31, 2012
##	version:	1.0
##
##	Description:
##	------------
##
##	This file contains all the adaptation data used by 
##	Icing Hazard Levels Algorithm.
##
###########################################################

  alg_name		value = Icing Hazard 

  mdl_time_gap  value = 240;
                name = Allowable minutes elapsed between volume and model time;
                type = int;
                unit = minute;
                range = [90, 900];
                accuracy = 5;
                description = model time gap allowed;

  num_lookback_gates	value = 0;
                      name = Number of end gates to trim for icing;
                      type = int;
                      range = [0, 15];
                      accuracy = [1];
                      description = Remove single entry graupel at end of a radial;

  filter_choice  value = 3;
                 name = Filter flag;
                 type = int;
                 range = [0, 3];
                 accuracy = [1];
                 description = Flag for filtering NONE 0 Valid only 1 Missing only 2 All 3;

  filter_size    value = 10;
                 name = Size of the filter in km;
                 type = int;
                 range = [1, 10];
                 unit = km;
                 accuracy = [1];
                 description = filter size in km;

  filter_thresh  value = 30;
                 name = Percent good pixels for filtering;
                 type = int;
                 range = [0, 100];
                 accuracy = [1];
                 description = Percentage of good value to filter, fewer good values results in a zero value;

  fill_method    value = 2;
                 name = fill method for filtered data;
                 type = int;
                 range = [1, 2];
                 accuracy = [1];
                 description = Fill with max 1 or average 2;

  use_cip        value = 1;
                 name = CIP flag;
                 type = int;
                 range = [0, 1];
                 accuracy = [1];
                 description = Flag to enable use of CIP data;

  cip_choice     value = 1;
                 name = Use max/thresh CIP;
                 type = int;
                 range = [0, 1];
                 accuracy = [1];
                 description = Use max 0 or thresh 1 to find top CIP icing value;

  min_cip_thresh  value = 80;
                  name = CIP probability threshold;
                  type = int;
                  range = [0, 100];
                  accuracy = [1];
                  description = Minimum CIP threshold for extending icing hazard top;


