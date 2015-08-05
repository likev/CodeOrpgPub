# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/01/25 22:15:55 $
# $Id: prfselect.alg,v 1.4 2013/01/25 22:15:55 steves Exp $
# $Revision: 1.4 $
# $State: Exp $

    alg_name		value = PRF_SELECT

    max_num_storms	value = 3.0;	
			name = Maximum Number of Storms to Examine;
			type = int;		
			range = [1, 5];	
			unit = unitless;		
			description = Maximum number of storms to examine;	

    min_prf		value = 5.0;
                        name = Minimum PRF for Storm-based PRF Selection;
                        type = int;
                        range = [4, 8];
                        unit = unitless;
                        description = Minimum PRF for Storm-based PRF Selection;

    min_prf_legacy	value = 5.0;
                        name = Minimum PRF for legacy PRF Selection;
                        type = int;
                        range = [4, 8];
                        unit = unitless;
                        description = Minimum PRF for Legacy PRF Selection;

    track_cell		value = Yes;
                        name = Enable storm-based PRF Selection;
                        type = string;
                        range = {No, Yes};
                        enum = 0, 1;
                        unit = unitless;
                        description = Enable storm-based PRF Selection;

    min_vil		value = 20.0;
			name = Minimum VIL for storm-based PRF Selection;
			type = float;
			range = [10.0, 30.0];
			unit = kg/m^3;
			description = Minimum VIL for storm-based PRF Selection;
