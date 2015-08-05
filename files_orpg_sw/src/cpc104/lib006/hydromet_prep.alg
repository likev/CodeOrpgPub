# RCS info
# $Author: dberkowitz $
# $Locker:  $
# $Date: 2014/09/02 19:56:49 $
# $Id: hydromet_prep.alg,v 1.14 2014/09/02 19:56:49 dberkowitz Exp $
# $Revision: 1.14 $
# $State: Exp $

    alg_name		value = Hydromet Preprocessing

    beam_width		value = 0.9;	
			name = Radar Half Power Beam Width [BEAMWDTH];	
			type = double;		
			range = [0.8, 1.0];	
			accuracy = [0.1];
			unit = degrees;		
			description = Radar half power beam width in deg.;	

    block_thresh	value = 50.0;	
			name = Maximum Allowable Percent of Beam Blockage [BLKTHRESH];	
			type = double;		
			range = [0.0, 100.0];	
			accuracy = [0.1];
			unit = %;		
			description = Maximum portion of beam blocked value to allow use of BASE;	

    clutter_thresh	value = 50;	
			name = Maximum Allowable Percent Likelihood of Clutter [CLUTTHRESH];	
			type = int;		
			range = [50, 100];	
			unit = %;		
			permission = [ROC, URC];
			description = The maximum likelihood allowed before a bin is rejected as being contaminated by ground clutter, forcing use of a bin at that location from a higher elevation.;

    weight_thresh	value = 50.0;	
			name = Percent of Beam Required to Compute Average Power [WGTHRESH];	
			type = double;		
			range = [0.0, 100.0];	
			accuracy = [0.1];
			unit = %;		
			description = Bin weight required to compute average power.;	

    full_hys_thresh	value = 99.9;	
			name = Percent of Hybrid Scan Needed To Be Considered Full [FHYS];	
			type = double;		
			range = [90.0, 100.0];	
			accuracy = [0.1];
			unit = %;		
			description = Percent of Hybrid Scan needed to be considered full.;	

    low_dbz_thresh	value = -32.0;	
			name = Low Reflectivity Threshold (dBZ) for Base Data [LOWDBZ];	
			type = double;		
			range = [-40.0, -20.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = Low threshold for converting to and from coded reflectivity data.;	

    rain_dbz_thresh	value = 20.0;	
			name = Reflectivity (dBZ) Representing Significant Rain  [RAINZ];	
			type = double;		
			range = [10.0, 30.0];	
			accuracy = [0.1];
			unit = dBZ;		
			permission = [ROC, URC];
			description = Minimum reflectivity to be considered for initiation or continuation of rainfall accumulation.;

    rain_area_thresh	value = 80;	
			name = Area with Reflectivity Exceeding Significant Rain Threshold [RAINA];	
			type = int;		
			range = [0, 82800];	
			unit = km^2;		
			permission = [ROC, URC];
			description = Minimum area of reflectivity greater than RAINZ threshold in order to initiate or continue rainfall accumulation.;

    rain_time_thresh	value = 60;	
			name = Threshold Time without Rain for Resetting STP [RAINT];	
			type = int;		
			range = [0, 1440];	
			unit = mins;		
			description = Time which, if no rain id detected, then reset storm total flag is set;	

    min_refl_rate	value = 0.0;	
			name = Min dBZ for Converting to Precip. Rate (via table lookup) [MNDBZ];	
			type = double;		
			range = [-32.0, 20.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = Used to limit max reflectivity values instead of deleting them.;	

    max_refl_rate	value = 70.0;	
			name = Max dBZ for Converting to Precip. Rate (via table lookup) [MXDBZ];	
			type = double;		
			range = [50.0, 90.0];	
			accuracy = [0.1];
			unit = dBZ;		
			description = Used to cap max reflectivity values instead of deleting them.;	

    num_zone		value = 0;	
			name = Number of Exclusion Zones [NEXZONE];	
			type = int;		
			range = [0, 20];	
			permission = [ROC, URC];
			description = The sequential number of zones (starting with Zone #1) that are to be utilized.;

    Beg_azm1		value = 0.0;	
			name = Exclusion Zone Limits # 1     - Begin Azimuth #1;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges begin closest to the radar.  Elevation is for the top of the selected exclusion zone.;

    End_azm1		value = 0.0;	
			name =                               - End Azimuth #1;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng1		value = 0;	
			name =                               - Begin Range #1;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng1		value = 0;	
			name =                               - End Range #1;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl1		value = 0.0;	
			name =                               - Max Elevation Angle #1;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm2		value = 0.0;	
			name = Exclusion Zone Limits # 2     - Begin Azimuth #2;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm2		value = 0.0;	
			name =                               - End Azimuth #2;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng2		value = 0;	
			name =                               - Begin Range #2;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng2		value = 0;	
			name =                               - End Range #2;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl2		value = 0.0;	
			name =                               - Max Elevation Angle #2;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm3		value = 0.0;	
			name = Exclusion Zone Limits # 3     - Begin Azimuth #3;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm3		value = 0.0;	
			name =                               - End Azimuth #3;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng3		value = 0;	
			name =                               - Begin Range #3;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng3		value = 0;	
			name =                               - End Range #3;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl3		value = 0.0;	
			name =                               - Max Elevation Angle #3;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm4		value = 0.0;	
			name = Exclusion Zone Limits # 4     - Begin Azimuth #4;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm4		value = 0.0;	
			name =                               - End Azimuth #4;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng4		value = 0;	
			name =                               - Begin Range #4;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng4		value = 0;	
			name =                               - End Range #4;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl4		value = 0.0;	
			name =                               - Max Elevation Angle #4;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm5		value = 0.0;	
			name = Exclusion Zone Limits # 5     - Begin Azimuth #5;	
			type = double;		
			accuracy = [0.1];
			range = [0.0, 360.0];	
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm5		value = 0.0;	
			name =                               - End Azimuth #5;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng5		value = 0;	
			name =                               - Begin Range #5;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng5		value = 0;	
			name =                               - End Range #5;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl5		value = 0.0;	
			name =                               - Max Elevation Angle #5;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm6		value = 0.0;	
			name = Exclusion Zone Limits # 6     - Begin Azimuth #6;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm6		value = 0.0;	
			name =                               - End Azimuth #6;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng6		value = 0;	
			name =                               - Begin Range #6;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng6		value = 0;	
			name =                               - End Range #6;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl6		value = 0.0;	
			name =                               - Max Elevation Angle #6;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm7		value = 0.0;	
			name = Exclusion Zone Limits # 7     - Begin Azimuth #7;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm7		value = 0.0;	
			name =                               - End Azimuth #7;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng7		value = 0;	
			name =                               - Begin Range #7;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng7		value = 0;	
			name =                               - End Range #7;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl7		value = 0.0;	
			name =                               - Max Elevation Angle #7;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm8		value = 0.0;	
			name = Exclusion Zone Limits # 8     - Begin Azimuth #8;	
			type = double;		
			accuracy = [0.1];
			range = [0.0, 360.0];	
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm8		value = 0.0;	
			name =                               - End Azimuth #8;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng8		value = 0;	
			name =                               - Begin Range #8;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng8		value = 0;	
			name =                               - End Range #8;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl8		value = 0.0;	
			name =                               - Max Elevation Angle #8;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm9		value = 0.0;	
			name = Exclusion Zone Limits # 9     - Begin Azimuth #9;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm9		value = 0.0;	
			name =                               - End Azimuth #9;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng9		value = 0;	
			name =                               - Begin Range #9;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng9		value = 0;	
			name =                               - End Range #9;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl9		value = 0.0;	
			name =                               - Max Elevation Angle #9;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm10		value = 0.0;	
			name = Exclusion Zone Limits # 10    - Begin Azimuth #10;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm10		value = 0.0;	
			name =                               - End Azimuth #10;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng10		value = 0;	
			name =                               - Begin Range #10;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng10		value = 0;	
			name =                               - End Range #10;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl10		value = 0.0;	
			name =                               - Max Elevation Angle #10;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm11		value = 0.0;	
			name = Exclusion Zone Limits # 11    - Begin Azimuth #11;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm11		value = 0.0;	
			name =                               - End Azimuth #11;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng11		value = 0;	
			name =                               - Begin Range #11;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng11		value = 0;	
			name =                               - End Range #11;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl11		value = 0.0;	
			name =                               - Max Elevation Angle #11;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm12		value = 0.0;	
			name = Exclusion Zone Limits # 12    - Begin Azimuth #12;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm12		value = 0.0;	
			name =                               - End Azimuth #12;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng12		value = 0;	
			name =                               - Begin Range #12;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng12		value = 0;	
			name =                               - End Range #12;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl12		value = 0.0;	
			name =                               - Max Elevation Angle #12;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm13		value = 0.0;	
			name = Exclusion Zone Limits # 13    - Begin Azimuth #13;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm13		value = 0.0;	
			name =                               - End Azimuth #13;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng13		value = 0;	
			name =                               - Begin Range #13;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng13		value = 0;	
			name =                               - End Range #13;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl13		value = 0.0;	
			name =                               - Max Elevation Angle #13;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm14		value = 0.0;	
			name = Exclusion Zone Limits # 14    - Begin Azimuth #14;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm14		value = 0.0;	
			name =                               - End Azimuth #14;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng14		value = 0;	
			name =                               - Begin Range #14;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng14		value = 0;	
			name =                               - End Range #14;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl14		value = 0.0;	
			name =                               - Max Elevation Angle #14;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm15		value = 0.0;	
			name = Exclusion Zone Limits # 15    - Begin Azimuth #15;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm15		value = 0.0;	
			name =                               - End Azimuth #15;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng15		value = 0;	
			name =                               - Begin Range #15;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng15		value = 0;	
			name =                               - End Range #15;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl15		value = 0.0;	
			name =                               - Max Elevation Angle #15;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm16		value = 0.0;	
			name = Exclusion Zone Limits # 16    - Begin Azimuth #16;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm16		value = 0.0;	
			name =                               - End Azimuth #16;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng16		value = 0;	
			name =                               - Begin Range #16;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng16		value = 0;	
			name =                               - End Range #16;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl16		value = 0.0;	
			name =                               - Max Elevation Angle #16;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm17		value = 0.0;	
			name = Exclusion Zone Limits # 17    - Begin Azimuth #17;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm17		value = 0.0;	
			name =                               - End Azimuth #17;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng17		value = 0;	
			name =                               - Begin Range #17;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng17		value = 0;	
			name =                               - End Range #17;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl17		value = 0.0;	
			name =                               - Max Elevation Angle #17;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm18		value = 0.0;	
			name = Exclusion Zone Limits # 18    - Begin Azimuth #18;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm18		value = 0.0;	
			name =                               - End Azimuth #18;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng18		value = 0;	
			name =                               - Begin Range #18;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng18		value = 0;	
			name =                               - End Range #18;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl18		value = 0.0;	
			name =                               - Max Elevation Angle #18;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm19		value = 0.0;	
			name = Exclusion Zone Limits # 19    - Begin Azimuth #19;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm19		value = 0.0;	
			name =                               - End Azimuth #19;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng19		value = 0;	
			name =                               - Begin Range #19;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng19		value = 0;	
			name =                               - End Range #19;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl19		value = 0.0;	
			name =                               - Max Elevation Angle #19;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

    Beg_azm20		value = 0.0;	
			name = Exclusion Zone Limits # 20    - Begin Azimuth #20;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Begin azimuths.;	

    End_azm20		value = 0.0;	
			name =                               - End Azimuth #20;	
			type = double;		
			range = [0.0, 360.0];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = End azimuth.;	

    Beg_rng20		value = 0;	
			name =                               - Begin Range #20;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    End_rng20		value = 0;	
			name =                               - End Range #20;	
			type = int;		
			range = [0, 124];	
			unit = nm;		
			permission = [ROC, URC];
			description = Begin range limits for zones to be exclude from the hybrid scan.;	

    Elev_agl20		value = 0.0;	
			name =                               - Max Elevation Angle #20;	
			type = double;		
			range = [0.0, 19.5];	
			accuracy = [0.1];
			unit = degrees;		
			permission = [ROC, URC];
			description = Max Elevation Angle.;	

