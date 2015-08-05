/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:41 $
 * $Id: hydromet_prep.h,v 1.10 2007/01/30 22:56:41 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#ifndef HYDROMET_PREP_H
#define	HYDROMET_PREP_H

#include <orpgctype.h>

#define HYDROMET_PREP_DEA_NAME "alg.hydromet_prep"

/*	Hydromet (Enhanced Preprocessing)	*/


typedef struct {

	freal	beam_width;	/*#  @name "Radar Half Power Beam Width [BEAMWDTH]"
				     @desc Radar half power beam width in deg.
				     @units "degrees" @default 0.9 @min 0.8 @max 1.0 @precision 1 
				     @legacy_name BEAM_WIDTH
				*/
	freal	block_thresh;   /*#  @name "Maximum Allowable Percent of Beam Blockage [BLKTHRESH]"
                                     @desc Maximum portion of beam blocked value to allow use of BASE 
					   REFLECTIVITY DATA in HYBRID SCAN (Reflectivity).
				     @units %  @default 50.0  @min 0.0  @max 100.0 @precision 1  
				     @legacy_name BLOCKAGE_THRESHOLD
				*/
	fint	clutter_thresh;	/*#  @name "Maximum Allowable Percent Likelihood of Clutter [CLUTTHRESH]"
				     @desc Maximum portion of clutter likelihood value to allow use of BASE 
				           REFLECTIVITY DATA in HYBRID SCAN. 
				     @units %  @default 50  @min 0  @max 100  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name CLUTTER_THRESHOLD
				*/
	freal	weight_thresh;	/*#  @name "Percent of Beam Required to Compute Average Power [WGTHRESH]"
				     @desc Bin weight required to compute average power.
                                     @units %  @default 50.0  @min 0.0  @max 100.0  @precision 1 
				     @legacy_name WEIGHT_THRESHOLD
				*/
	freal	full_hys_thresh;/*#  @name "Percent of Hybrid Scan Needed To Be Considered Full [FHYS]"
				     @desc Percent of Hybrid Scan needed to be considered full.
				     @units %  @min 90.0  @max 100.0 @default 99.7  @precision 1
			      	     @legacy_name FULL_HYS_THRESHOLD
				*/
	freal	low_dbz_thresh;	/*#  @name "Low Reflectivity Threshold (dBZ) for Base Data [LOWDBZ]"
				     @desc "Low threshold for converting to and from coded reflectivity data."
				     @units dBZ @min -40.0 @max -20.0 @default -32.0 @precision 1
				     @legacy_name LOW_DBZ_THRESHOLD
				*/
	freal	rain_dbz_thresh;/*#  @name "Reflectivity (dBZ) Representing Significant Rain  [RAINZ]"
				     @desc Reflectivity level used to compute area for rain detection.
				     @units dBZ @default 20.0 @min 10.0 @max 30.0 @precision 1 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name RAIN_DBZ_THRESHOLD
				*/
	fint	rain_area_thresh;/*#  @name "Area with Reflectivity Exceeding Significant Rain Threshold [RAINA]"
				     @desc Area with Reflectivity exceeding significant Rain Threshold. 
				     @units "km**2" @default 80  @min 0  @max 82800
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name RAIN_AREA_THRESHOLD
				*/
	fint	rain_time_thresh;/*#  @name "Threshold Time without Rain for Resetting STP [RAINT]"
				     @desc Time which, if no rain id detected, then reset storm total flag is set
					   to restart the Storm Total Accumulation product.
				     @units mins  @min 0  @max 1440  @default 60
				     @legacy_name RAIN_TIME_THRESHOLD
				*/
	freal	min_refl_rate;	/*#  @name "Min dBZ for Converting to Precip. Rate via table lookup [MNDBZ]"
				     @desc Used to limit max reflectivity values instead of deleting them.
				     @units dBZ  @default 0.0  @min -32.0  @max  20.0  @precision 1
				     @legacy_name MINDBZRFL
				*/
	freal	max_refl_rate;	/*#  @name "Max dBZ for Converting to Precip. Rate (via table lookup) [MXDBZ]"
				     @desc Used to cap max reflectivity values instead of deleting them.
				     @units dBZ  @default 70.0  @min 50.0  @max 90.0 @precision 1
				     @legacy_name MAXDBZRFL
				*/
	fint	num_zone;	/*#  @name "Number of Exclusion Zones [NEXZONE]"
				     @desc Number of zones to excluded from HYBRID SCAN (Reflectivity) array.
				     @min 0 @max 20 @default 0
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name NUMZONE  
				*/
	freal	Beg_azm1;
				/*#  @name "Exclusion Zone Limits # 1     - Begin Azimuth #1"
				     @desc Begin azimuths.
 	                             @precision 1 
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM1
				*/
	freal	End_azm1;
				/*#  @name "                              - End Azimuth #1"
				     @desc End azimuth.
				     @precision 1 
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM1
				*/
	fint	Beg_rng1;
				/*#  @name "                              - Begin Range #1"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124    
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG1
				*/
	fint	End_rng1;
				/*#  @name "                              - End Range #1"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG1
				*/
	freal	Elev_agl1;
				/*#  @name "                              - Elevation Angle #1"
				     @desc Elevation Angle.
				     @precision 1 
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL1
				*/
	freal	Beg_azm2;
				/*#  @name "Exclusion Zone Limits # 2     - Begin Azimuth #2"
				     @desc Begin azimuths.
 	                             @precision 1 
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM2
				*/
	freal	End_azm2;
				/*#  @name "                              - End Azimuth #2"
				     @desc End azimuth.
				     @precision 1 
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM2
				*/
	fint	Beg_rng2;
				/*#  @name "                              - Begin Range #2"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG2

				*/
	fint	End_rng2;
				/*#  @name "                              - End Range #2"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG2

				*/
	freal	Elev_agl2;
				/*#  @name "                              - Elevation Angle #2"
				     @desc Elevation Angle.
				     @precision 1 
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL2
				*/
	freal	Beg_azm3;
				/*#  @name "Exclusion Zone Limits # 3     - Begin Azimuth #3"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM3
				*/
	freal	End_azm3;
				/*#  @name "                              - End Azimuth #3"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM3
				*/
	fint	Beg_rng3;
				/*#  @name "                              - Begin Range #3"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG3
				*/
	fint	End_rng3;
				/*#  @name "                              - End Range #3"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG3
				*/
	freal	Elev_agl3;
				/*#  @name "                              - Elevation Angle #3"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL3
				*/
	freal	Beg_azm4;
				/*#  @name "Exclusion Zone Limits # 4     - Begin Azimuth #4"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM4
				*/
	freal	End_azm4;
				/*#  @name "                              - End Azimuth #4"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM4
				*/
	fint	Beg_rng4;
				/*#  @name "                              - Begin Range #4"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG4
				*/
	fint	End_rng4;
				/*#  @name "                              - End Range #4"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG4
				*/
	freal	Elev_agl4;
				/*#  @name "                              - Elevation Angle #4"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL4
				*/
	freal	Beg_azm5;
				/*#  @name "Exclusion Zone Limits # 5     - Begin Azimuth #5"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM5
				*/
	freal	End_azm5;
				/*#  @name "                              - End Azimuth #5"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM5
				*/
	fint	Beg_rng5;
				/*#  @name "                              - Begin Range #5"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG5
				*/
	fint	End_rng5;
				/*#  @name "                              - End Range #5"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG5
				*/
	freal	Elev_agl5;
				/*#  @name "                              - Elevation Angle #5"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL5
				*/
	freal	Beg_azm6;
				/*#  @name "Exclusion Zone Limits # 6     - Begin Azimuth #6"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM6
				*/
	freal	End_azm6;
				/*#  @name "                              - End Azimuth #6"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM6
				*/
	fint	Beg_rng6;
				/*#  @name "                              - Begin Range #6"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG6
				*/
	fint	End_rng6;
				/*#  @name "                              - End Range #6"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG6
				*/
	freal	Elev_agl6;
				/*#  @name "                              - Elevation Angle #6"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL6
				*/
	freal	Beg_azm7;
				/*#  @name "Exclusion Zone Limits # 7     - Begin Azimuth #7"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM7
				*/
	freal	End_azm7;
				/*#  @name "                              - End Azimuth #7"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM7
				*/
	fint	Beg_rng7;
				/*#  @name "                              - Begin Range #7"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG7
				*/
	fint	End_rng7;
				/*#  @name "                              - End Range #7"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG7
				*/
	freal	Elev_agl7;
				/*#  @name "                              - Elevation Angle #7"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL7
				*/
	freal	Beg_azm8;
				/*#  @name "Exclusion Zone Limits # 8     - Begin Azimuth #8"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM8
				*/
	freal	End_azm8;
				/*#  @name "                              - End Azimuth #8"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM8
				*/
	fint	Beg_rng8;
				/*#  @name "                              - Begin Range #8"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG8
				*/
	fint	End_rng8;
				/*#  @name "                              - End Range #8"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG8
				*/
	freal	Elev_agl8;
				/*#  @name "                              - Elevation Angle #8"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL8
				*/
	freal	Beg_azm9;
				/*#  @name "Exclusion Zone Limits # 9     - Begin Azimuth #9"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM9
				*/
	freal	End_azm9;
				/*#  @name "                              - End Azimuth #9"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM9
				*/
	fint	Beg_rng9;
				/*#  @name "                              - Begin Range #9"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG9
				*/
	fint	End_rng9;
				/*#  @name "                              - End Range #9"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG9
				*/
	freal	Elev_agl9;
				/*#  @name "                              - Elevation Angle #9"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL9
				*/
	freal	Beg_azm10;
				/*#  @name "Exclusion Zone Limits # 10    - Begin Azimuth #10"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM10
				*/
	freal	End_azm10;
				/*#  @name "                              - End Azimuth #10"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM10
				*/
	fint	Beg_rng10;
				/*#  @name "                              - Begin Range #10"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG10
				*/
	fint	End_rng10;
				/*#  @name "                              - End Range #10"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG10
				*/
	freal	Elev_agl10;
				/*#  @name "                              - Elevation Angle #10"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL10
				*/
	freal	Beg_azm11;
				/*#  @name "Exclusion Zone Limits # 11    - Begin Azimuth #11"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM11
				*/
	freal	End_azm11;
				/*#  @name "                              - End Azimuth #11"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM11
				*/
	fint	Beg_rng11;
				/*#  @name "                              - Begin Range #11"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG11
				*/
	fint	End_rng11;
				/*#  @name "                              - End Range #11"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG11
				*/
	freal	Elev_agl11;
				/*#  @name "                              - Elevation Angle #11"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL11
				*/
	freal	Beg_azm12;
				/*#  @name "Exclusion Zone Limits # 12    - Begin Azimuth #12"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM12
				*/
	freal	End_azm12;
				/*#  @name "                              - End Azimuth #12"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM12
				*/
	fint	Beg_rng12;
				/*#  @name "                              - Begin Range #12"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG12
				*/
	fint	End_rng12;
				/*#  @name "                              - End Range #12"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG12
				*/
	freal	Elev_agl12;
				/*#  @name "                              - Elevation Angle #12"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL12
				*/
	freal	Beg_azm13;
				/*#  @name "Exclusion Zone Limits # 13    - Begin Azimuth #13"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM13
				*/
	freal	End_azm13;
				/*#  @name "                              - End Azimuth #13"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM13
				*/
	fint	Beg_rng13;
				/*#  @name "                              - Begin Range #13"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG13
				*/
	fint	End_rng13;
				/*#  @name "                              - End Range #13"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG13
				*/
	freal	Elev_agl13;
				/*#  @name "                              - Elevation Angle #13"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL13
				*/
	freal	Beg_azm14;
				/*#  @name "Exclusion Zone Limits # 14    - Begin Azimuth #14"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM14
				*/
	freal	End_azm14;
				/*#  @name "                              - End Azimuth #14"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM14
				*/
	fint	Beg_rng14;
				/*#  @name "                              - Begin Range #14"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG14
				*/
	fint	End_rng14;
				/*#  @name "                              - End Range #14"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG14
				*/
	freal	Elev_agl14;
				/*#  @name "                              - Elevation Angle #14"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL14
				*/
	freal	Beg_azm15;
				/*#  @name "Exclusion Zone Limits # 15    - Begin Azimuth #15"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM15
				*/
	freal	End_azm15;
				/*#  @name "                              - End Azimuth #15"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM15
				*/
	fint	Beg_rng15;
				/*#  @name "                              - Begin Range #15"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG15
				*/
	fint	End_rng15;
				/*#  @name "                              - End Range #15"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG15
				*/
	freal	Elev_agl15;
				/*#  @name "                              - Elevation Angle #15"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL15
				*/
	freal	Beg_azm16;
				/*#  @name "Exclusion Zone Limits # 16    - Begin Azimuth #16"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM16
				*/
	freal	End_azm16;
				/*#  @name "                              - End Azimuth #16"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM16
				*/
	fint	Beg_rng16;
				/*#  @name "                              - Begin Range #16"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG16
				*/
	fint	End_rng16;
				/*#  @name "                              - End Range #16"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG16
				*/
	freal	Elev_agl16;
				/*#  @name "                              - Elevation Angle #16"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL16
				*/
	freal	Beg_azm17;
				/*#  @name "Exclusion Zone Limits # 17    - Begin Azimuth #17"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM17
				*/
	freal	End_azm17;
				/*#  @name "                              - End Azimuth #17"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM17
				*/
	fint	Beg_rng17;
				/*#  @name "                              - Begin Range #17"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG17
				*/
	fint	End_rng17;
				/*#  @name "                              - End Range #17"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG17
				*/
	freal	Elev_agl17;
				/*#  @name "                              - Elevation Angle #17"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL17
				*/
	freal	Beg_azm18;
				/*#  @name "Exclusion Zone Limits # 18    - Begin Azimuth #18"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM18
				*/
	freal	End_azm18;
				/*#  @name "                              - End Azimuth #18"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM18
				*/
	fint	Beg_rng18;
				/*#  @name "                              - Begin Range #18"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG18
				*/
	fint	End_rng18;
				/*#  @name "                              - End Range #18"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG18
				*/
	freal	Elev_agl18;
				/*#  @name "                              - Elevation Angle #18"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL18
				*/
	freal	Beg_azm19;
				/*#  @name "Exclusion Zone Limits # 19    - Begin Azimuth #19"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM19
				*/
	freal	End_azm19;
				/*#  @name "                              - End Azimuth #19"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM19
				*/
	fint	Beg_rng19;
				/*#  @name "                              - Begin Range #19"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG19
				*/
	fint	End_rng19;
				/*#  @name "                              - End Range #19"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG19
				*/
	freal	Elev_agl19;
				/*#  @name "                              - Elevation Angle #19"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL19
				*/
	freal	Beg_azm20;
				/*#  @name "Exclusion Zone Limits # 20    - Begin Azimuth #20"
				     @desc Begin azimuths.
 	                             @precision 1
				     @units "degrees"  @min 0.0 @max 360.0  
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_AZM20
				*/
	freal	End_azm20;
				/*#  @name "                              - End Azimuth #20"
				     @desc End azimuth.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 360.0 
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_AZM20
				*/
	fint	Beg_rng20;
				/*#  @name "                              - Begin Range #20"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm @min 0 @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name BEG_RNG20
				*/
	fint	End_rng20;
				/*#  @name "                              - End Range #20"
				     @desc Begin range limits for zones to be exclude from the hybrid scan.
				     @units nm  @min 0  @max 124   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name END_RNG20
				*/
	freal	Elev_agl20;
				/*#  @name "                              - Elevation Angle #20"
				     @desc Elevation Angle.
				     @precision 1
				     @units "degrees"  @min 0.0  @max 19.5   
				     @authority "READ | OSF_WRITE | URC_WRITE"
				     @legacy_name ELEV_AGL20
				*/
} hydromet_prep_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif

