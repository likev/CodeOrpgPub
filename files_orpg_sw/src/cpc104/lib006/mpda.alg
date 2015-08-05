# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/05/03 14:31:39 $
# $Id: mpda.alg,v 1.17 2011/05/03 14:31:39 ccalvert Exp $
# $Revision: 1.17 $
# $State: Exp $

    alg_name		value = Velocity Dealiasing - Multi-PRF

    gui_mpda_tover	value = 5.0;	
			name = Threshold (Range Unfold Power Difference);	
			type = double;		
			range = [0.0, 20.0];	
			accuracy = [0.1];
			unit = dB;		
			permission = [ROC, URC];
			description = The minimum difference in surveillance scan power (dB) that is needed to reliably range unfold Doppler data.  The Doppler data are separated by multiples of their unambiguous ranges.  This parameter, although used by the MPDA in the RPG, is equivalent to TOVER used at the RDA.;

    gui_min_trip_fix	value = -5;	
			name = Threshold (Fix Trip Minimum Bin);	
			type = int;		
			range = [-16, 16];	
			unit = bins;		
			permission = [ROC, URC];
			description = This parameter and the next parameter Threshold (Fix Trip Maximum Bin), in combination, define a ring of bins around the end of the unambiguous for each Doppler cut that are omitted from processing by the MPDA; they tend to be unreliable due to signal processing issues.   Because the unambiguous range differs for each scan there is loss of data in the final output MPDA velocity field.;

    gui_max_trip_fix	value = 2;	
			name = Threshold (Fix Trip Maximum Bin);	
			type = int;		
			range = [-16, 16];	
			unit = bins;		
			permission = [ROC, URC];
			description = See Threshold (Fix Trip Minimum Bin) above.;

    gui_th_overlap_size	value = 2;	
			name = Threshold (Tight Overlap Size);	
			type = int;		
			range = [2, 8];	
			unit = m/s;		
			permission = [ROC, URC];
			description = Threshold used for matching differences between triplets. All three PRFs must fall within +/- this value.;

    gui_th_overlap_relax	value = 2;	
			name = Threshold (Loose Overlap Size);	
			type = int;		
			range = [2, 12];	
			unit = m/s;		
			permission = [ROC, URC];
			description = Same as gui_th_overlap_size except it is a looser threshold.;

