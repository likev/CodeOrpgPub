# RCS info
# $Author: cheryls $
# $Locker:  $
# $Date: 2004/04/02 21:41:34 $
# $Id: recclalg.alg,v 1.6 2004/04/02 21:41:34 cheryls Exp $
# $Revision: 1.6 $
# $State: Exp $

    alg_name		value = REC AP/Clutter Target

    Ztxtr0		value = 0.0;	
			name = Texture of Reflectivity 0% Value;	
			type = double;		
			range = [0.0, 80.0];	
			accuracy = [0.1];
			description = Texture value that represents 0% likelihood of a ground clutter target.;	

    Ztxtr1		value = 45.0;	
			name = Texture of Reflectivity 100% Value;	
			type = double;		
			range = [0.0, 80.0];	
			accuracy = [0.1];
			description = Texture value that represents 100% likelihood of a ground clutter target.;	

    Zsign0		value = 0.6;	
			name = Sign of Reflectivity Change 0% Value;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = Absolute value of ZSIGN that represents 0% likelihood of a ground clutter target.;	

    Zsign1		value = 0.0;	
			name = Sign of Reflectivity Change 100% Value;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = Absolute value of ZSIGN that represents 100% likelihood of a ground clutter target.;	

    Zspin0		value = 50.0;	
			name = Reflectivity Spin Change 0% Value;	
			type = double;		
			range = [0.0, 100.0];	
			accuracy = [0.1];
			description = (Spin Change-50) value that represents 0% likelihood of a ground clutter target.;	

    Zspin1		value = 0.0;	
			name = Reflectivity Spin Change 100% Value;	
			type = double;		
			range = [0.0, 100.0];	
			accuracy = [0.1];
			description = (Spin Change-50) value that represents 100% likelihood of a ground clutter target.;	

    ZspinThr		value = 2.0;	
			name = Spin Change Threshold;	
			type = double;		
			range = [0.0, 20.0];	
			accuracy = [0.1];
			description = Threshold of spin change needed to be met in order to be included in the ZSPIN calclulation.;	

    ZThr		value = 5.0;	
			name = Spin Reflectivity Threshold;	
			type = double;		
			range = [0.0, 20.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = Threshold of reflectivity needed to be met in order to be included in the ZSPIN calculation.;	

    Vmean0		value = 2.3;	
			name = Mean Velocity 0% Value;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Absolute value of Doppler velocity that represents 0% likelihood of a ground clutter target.;	

    Vmean1		value = 0.0;	
			name = Mean Velocity 100% Value;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Absolute value of Doppler velocity that represents 100% likelihood of a ground clutter target.;	

    Vstdv0		value = 0.7;	
			name = Standard Deviation of Velocity 0% Value;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Standard deviation of velocity value that represents 0% likelihood of a ground clutter target.;	

    Vstdv1		value = 0.0;	
			name = Standard Deviation of Velocity 100% Value;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Standard deviation of velocity value that represents 100% likelihood of a ground clutter target.;	

    Wmean0		value = 3.2;	
			name = Mean Spectrum Width 0% Value;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Spectrum width value that represents 0% likelihood of a ground clutter target.;	

    Wmean1		value = 0.0;	
			name = Mean Spectrum Width 100% Value;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = m/s;		
			description = Spectrum width value that represents 100% likelihood of a ground clutter target.;	

    ZtxtrWt		value = 1.0;	
			name = Texture of Reflectivity Weight;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = The relative contribution of texture in the output likelihood of an AP/clutter target.;	

    ZsignWt		value = 1.0;	
			name = Sign of Reflectivity Weight;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = The relative contribution of Sign of Refl. in the output likelihood of a ground clutter target.;	

    ZspinWt		value = 1.0;	
			name = Reflectivity Spin Change Weight;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = The relative contribution of Refl. Spin Change in the output likelihood of a ground clutter target.;	

    VmeanWt		value = 1.0;	
			name = Mean Velocity Weight;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = The relative contribution of Doppler velocity in the output likelihood of a ground clutter target.;	

    VstdvWt		value = 1.0;	
			name = Standard Deviation of Velocity Weight;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = The relative contribution of the standard deviation of velocity in the output likelihood of a ground clutter target.;	

    WmeanWt		value = 1.0;	
			name = Mean Spectrum Width Weight;	
			type = double;		
			range = [0.0, 1.0];	
			accuracy = [0.1];
			description = The relative contribution of spectrum width in the output likelihood of a ground clutter target.;	

    deltaAz		value = 1;	
			name = Azimuthal Extent (in radials);	
			type = int;		
			range = [1, 4];	
			unit = azimuths;
			description = Number of radials (+/-) used to define the azumithal dimension of the region over which the pattern characteristics of texture, mean sign delta reflectivity and standard deviation of velocity are computed.;	

    deltaRng		value = 2;	
			name = Reflectivity Range Extent;	
			type = int;		
			range = [1, 4];	
			unit = bins;		
			description = Number of reflectivity bins (+/-) used to define the range dimension of the region over which the pattern characteristics of texture and mean sign delta reflectivity are computed.;	

    deltaBin		value = 4;	
			name = Doppler Range Extent;	
			type = int;		
			range = [1, 8];	
			unit = bins;		
			description = Number of Doppler bins (+/-) used to define the range dimension of the region over which the pattern characteristic of standard deviation of velocity is computed.;	

