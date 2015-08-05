# RCS info
# $Author: ryans $
# $Locker:  $
# $Date: 2005/12/06 21:31:39 $
# $Id: vil_echo_tops.alg,v 1.8 2005/12/06 21:31:39 ryans Exp $
# $Revision: 1.8 $
# $State: Exp $

    alg_name 		value = Vil/Echo Tops

    beam_width		value = 1.00;	
			name = Beam Width  [BW];	
			type = double;		
			range = [0.50, 2.00];	
			accuracy = [0.01];
			unit = degrees;		
			description = Angular width of the radar beam between the half-power points.;	

    min_refl		value = 18.3;	
			name = Min Ref Threshold  [MRT];	
			type = double;		
			range = [-33.0, 95.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = Minimum reflectivity used in computing vertically;	

    max_vil		value = ;	
			name = Max VIL Threshold  [MVT];	
			type = int;		
			range = [1, 200];	
			unit = kg/m^2;		
			description = Maximum allowable VIL product value.  All computed VIL values above this;	
			default = KDDC KILX KSGF: 100, KCLX KINX KRLX KSRX: 120, KICT: 200, Other_sites: 80;


