# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/18 20:14:59 $
# $Id: hydromet_rate.alg,v 1.10 2010/05/18 20:14:59 ccalvert Exp $
# $Revision: 1.10 $
# $State: Exp $


    alg_name 		value = Hydromet Rate


    range_cutoff	value = 230;	
			name = Range Beyond Which to Apply Range Effect Correction  [RNCUT];	
			type = int;		
			range = [0, 230];	
			unit = km;		
			description = Range beyond which a range effect correction must be applied.;	

    range_coef1		value = 0.0;	
			name = 1st Coefficient of Range Effect Function  [COER1];	
			type = double;		
			range = [0.0, 3.0];	
			accuracy = [0.1];
			unit = dBR;		
			description = First range effect correction coefficient.;	

    range_coef2		value = 1.0;	
			name = 2nd Coefficient of Range Effect Function  [COER2];	
			type = double;		
			range = [1.0, 10.0];	
			accuracy = [0.1];
			unit = dBR;		
			description = Second range effect correction coefficient.;	

    range_coef3		value = 0.0;	
			name = 3rd Coefficient of Range Effect Function  [COER3];	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			unit = dBR;		
			description = Third range effect correction coefficient.;	

    min_precip_rate	value = 0.0;	
			name = Min Rate Signifying Precipitation  [MNPRA];	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = mm/hr;		
			description = Minimum precipitation rate allowed.;	

    max_precip_rate	value = 103.8;	
			name = Max Precipitation Rate  [MXPRA];	
			type = double;		
			range = [50.0, 1600.0];	
			accuracy = [0.1];
			unit = mm/hr;		
			permission = [AGENCY, ROC, URC];
			description = Maximum instantaneous rainfall rate allowed when reflectivity is converted to a rate, used as a hail cap in the PPS.;

    zr_mult		value = 300.0;	
			name = Z-R Multiplier Coef. [CZM];	
			type = double;		
			range = [30.0, 3000.0];	
			accuracy = [0.1];
			unit = coefficient;		
			permission = [AGENCY, ROC, URC];
			description = The value to be multiplied by the rainfall rate in the reflectivity-rainfall rate relationship.;

    zr_exp		value = 1.4;	
			name = Z-R Exponent Coef. [CZP];	
			type = double;		
			range = [1.0, 2.5];	
			accuracy = [0.1];
			unit = factor;		
			permission = [AGENCY, ROC, URC];
			description = The exponent of the rainfall rate in the reflectivity-rainfall rate relationship.;

