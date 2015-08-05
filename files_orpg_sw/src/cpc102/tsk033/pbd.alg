# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/03/17 21:40:08 $
# $Id: pbd.alg,v 1.1 2010/03/17 21:40:08 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

   alg_name					value = Sensitivity Loss

   allow_snr_thresholding			value = No;	
						name = Allow SNR Thresholding;	
						type = string;		
						range = {No, Yes};
						enum = 0, 1;		
                                                description = Allow Additional SNR Thresholding to Emulate Sensitivity Loss.;

   sensitivity_loss_qrtkm 		        value = 0.0;	
						name = Sensitivity Loss (1/4 km);
						type = double;		
						range = [0.0, 20.0];
						unit = dB;
                                                description = The Sensitivity Loss (dB) to be Applied to 1/4 km R, V and W.;

   sensitivity_loss_onekm 		        value = 0.0;	
						name = Sensitivity Loss (1 km);
						type = double;		
						range = [0.0, 20.0];
						unit = dB;
                                                description = The Sensitivity Loss (dB) to be Applied to 1 km R, V and W.;

   apply_speckle_filter                         value = Yes;	
						name = Apply Speckle Filter;
						type = string;		
						range = {No, Yes};
						enum = 0, 1;
                                                description = Apply Speckle Filter to Censored Data.;
