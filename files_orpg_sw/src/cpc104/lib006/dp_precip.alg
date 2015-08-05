# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/10/03 18:23:33 $
# $Id: dp_precip.alg,v 1.19 2014/10/03 18:23:33 steves Exp $
# $Revision: 1.19 $
# $State: Exp $

#******************************************************************************
#   Filename: dp_precip.alg
#
#   Description:
#   ============
#   Adaptation data for the Dual-Pol Precipitation algorithm.
#
#   Inputs: none
#
#   Outputs: none
#
#   Change History
#   ==============
#   DATE       VERSION    PROGRAMMERS        NOTES
#   --------   -------    -----------------  --------------------------------
#   20070514   0000       Pham, Liu, Ward    Initial implementation
#   20080124   0001       Ward               Combined several files into 1
#   20080821   0002       Ward               Changed names and reordered data
#                                            like AEL spreadsheet rows.
#   20081202   0003       Ward               Added 3 adaptable parameters.
#   20090712   0004       Ward               Changed "RhoHV" to
#                                            "Correlation Coefficient".
#   20091104   0005       Ward               Removed ROC from permissions
#   20100310   0005       Ward               Deleted RhoHV_min_Kdp_rate;
#                                            to be replaced with dpprep.alg
#                                            corr_thresh
#   20101001   0006       Ward               Added max rate
#   20111031   0007       Ward               For CCR NA11-00372:
#                                            Removed  Use_pbb1
#                                            Removed  Z_max_beam_blk (pbb1)
#                                            Added    Min_blockage
#                                            Added    Kdp_min_beam_blk
#                                            Replaced RhoHV_min_rate
#                                            with     art_corr
#                                            Refl_min range = [-32.0, 5.0];
#   20140214   0008       Schultz,           Correct "name" and "description"
#                         Berkowitz,         attributes for specific adaptable
#                         Murnan             parameters.
#   20140812   0009       Berkowitz,         CCR NA13-00260 and 00261;
#                         Murnan             Added Hr_HighZThresh
#                                            Added Ds_BelowMLTop_mult
#*****************************************************************************/

    alg_name		value = Dual-Pol Precip

    Mode_filter_len	value = 9;
			name = Mode Filter Length;
			type = short;
			range = [3, 99];
			accuracy = [1];
			description = Mode filter length.;

    Kdp_mult		value = 44;
			name = Kdp Coefficient;
			type = double;
			range = [10, 1000];
			accuracy = [1];
			description = Kdp rate relationship coefficient.;

    Kdp_power		value = 0.822;
			name = Kdp Exponent;
			type = double;
			range = [0.010, 1.000];
			accuracy = [0.001];
			description = Kdp rate relationship exponent.;

    Z_mult		value = 300;
			name = Z-R Coefficient;
			type = double;
			range = [30, 3000];
			accuracy = [1];
			description = Z rate relationship coefficient.;

    Z_power		value = 1.4;
			name = Z-R Exponent;
			type = double;
			range = [-10.0, 10.0];
			accuracy = [0.1];
			description = Z rate relationship exponent.;

    Zdr_z_mult		value = 0.0067;
			name = Zdr/Z Coefficient for Z;
			type = double;
			range = {0.0067, 0.0142};
			permission = [ROC, URC];
			accuracy = [0.0001];
			description = Zdr/Z rate relationship Z coefficient.  \
                                      The first value is intended for tropical or warm rain \
                                      regimes, while the second is intended for continental regimes.;
			unit = (Tropical, Continental);

    Zdr_z_power		value = 0.927;
			name = Zdr/Z Exponent for Z;
			type = double;
			range = {0.927, 0.770};
			permission = [ROC, URC];
			accuracy = [0.001];
			description = Zdr/Z rate relationship Z exponent.  \
                                      The first value is intended for tropical or warm rain \
                                      regimes, while the second is intended for continental regimes.;
			unit = (Tropical, Continental);

    Zdr_zdr_power	value = -3.43;
			name = Zdr/Z Exponent for Zdr;
			type = double;
			range = {-3.43, -1.67};
			permission = [ROC, URC];
			accuracy = [0.01];
			description = Zdr/Z rate relationship Zdr Exponent.  \
                                      The first value is intended for tropical or warm rain \
                                      regimes, while the second is intended for continental regimes.;
			unit = (Tropical, Continental);

    Gr_mult		value = 0.8;
                        name = GR Multiplier;
			type = double;
			range = [0.1, 2.0];
			permission = [ROC, URC];
			accuracy = [0.1];
			description = Graupel multiplier.;

    Rh_mult		value = 0.8;
			name = HA Multiplier;
			type = double;
			range = [0.1, 2.0];
			permission = [ROC, URC];
			accuracy = [0.1];
			description = Rain/Hail multiplier.;

    Ds_mult		value = 2.8;
			name = DS Multiplier;
			type = double;
			range = [1.0, 2.8];
			permission = [ROC, URC];
			accuracy = [0.1];
			description = Dry snow multiplier.;

    Ds_BelowMLTop_mult  value = 1.0;
                        name = DS Multiplier Below ML Top;
                        type = double;
                        range = [0.8, 1.2];
                        permission = [ROC, URC];
                        accuracy = [0.1];
                        description = Dry snow multiplier in or below the melting layer.;

    Ic_mult		value = 2.8;
			name = IC Multiplier;
			type = double;
			range = [1.0, 2.8];
			permission = [ROC, URC];
			accuracy = [0.1];
			description = Ice crystals multiplier.;

    Ws_mult		value = 0.6;
			name = WS Multiplier;
			type = double;
			range = [0.1, 2.0];
			permission = [ROC, URC];
			accuracy = [0.1];
			description = Wet snow multiplier.;

    Grid_is_full	value = 99.9;
			name = Percent of the Rainfall Rate Grid Filled;
			type = double;
			range = [90.0, 100.0];
			accuracy = [0.1];
			unit = %;
			description = Minimum percentage threshold required before the hybrid rate \
                                      grid is considered to be filled.  The hybrid Digital Precip \
                                      Rate (DPR) product is generated when either 1) this percentage \
                                      of bins are filled, 2) the next to last elevation is completed \
                                      during normal (non-AVSET) operation, or 3) the last elevation \
                                      is completed during AVSET operation.;

    Min_blockage        value = 5;
			name = Minimum Blockage to augment Z;
			type = int;
			range = [0, 20];
			accuracy = [1];
			unit = %;
			description = Minimum beam blockage to augment Z.;

    Kdp_min_beam_blk    value = 20;
			name = Minimum Beam Blockage Using KDP;
			type = int;
			range = [0, 100];
			accuracy = [1];
			unit = %;
			description = Minimum beam blockage to use Kdp to compute rate.;

    Hr_HighZThresh      value = 45.0;
                        name = Reflectivity Threshold for use of R(Kdp) in Heavy Rain;
                        type = double;
                        range = [40.0, 50.0];
                        accuracy = [0.1];
                        unit = dBZ;
                        permission = [ROC, URC];
                        description = Maximum reflectivity with which a rainfall rate \
                                      will be computed using the R(Z,Zdr) relationship \
                                      when Heavy Rain is classified.  Heavy Rain having \
                                      values at or above this value will use the R(Kdp) \
                                      relationship.;    

    Kdp_max_beam_blk	value = 70;
			name = Maximum Beam Blockage Using KDP;
			type = int;
			range = [0, 100];
			accuracy = [1];
			unit = %;
			description = Maximum beam blockage to use Kdp to compute rate.;

    Kdp_min_usage_rate  value = 10.0;
                        name = Minimum Kdp Usage Rate;
                        type = double;
                        range = [0.0, 50.0];
                        accuracy = [0.1];
                        unit = mm/hr;
                        description = Minimum Z rate to use Kdp to compute rate.;

    Refl_min            value = -32.0;
                        name = Minimum Reflectivity;
                        type = double;
                        range = [-32.0, 5.0];
                        accuracy = [0.1];
                        unit = dBZ;
                        description = Minimum reflectivity necessary to compute rate.;

    Refl_max		value = 53.0;
			name = Maximum Reflectivity;
			type = double;
			range = [45.0, 60.0];
			accuracy = [0.1];
			unit = dBZ;
			permission = [URC];
			description = Maximum reflectivity with which a rainfall rate will be \
                                      computed. All reflectivity values are capped at this \
                                      value.;

    Paif_area		value = 80;
			name = PAIF Area Threshold;
			type = int;
			range = [0, 82800];
			accuracy = [1];
			unit = km^2;
			permission = [URC];
			description = Minimum area over which precip must be detected before \
                                      accumulation is initiated with the Precip Accumulation \
                                      Initiation Function (PAIF). Precip is detected if it \
                                      exceeds the rate threshold.;

    Paif_rate		value = 0.5;
			name = PAIF Rate Threshold;
			type = double;
			range = [0.0, 50.0];
			accuracy = [0.1];
			unit = mm/hr;
			permission = [URC];
			description = Minimum rate for which precip is detected within a bin. \
                                      Once there are enough bins with precip to cover the \
                                      PAIF area, then the Precip. Accumulation Initiation \
                                      Function (PAIF) initiates accumulation.;

    Max_vols_per_hour	value = 30;
			name = Maximum Volumes per Hour;
			type = int;
			range = [15, 30];
			accuracy = [1];
			description = Maximum volumes per hour.;

    Min_early_term_ang	value = 5;
			name = Minimum Early Termination Angle;
			type = int;
			range = [5, 5];
			accuracy = [1];
			unit = degrees;
			description = Minimum angle for AVSET early termination.;

    Max_precip_rate	value = 200.0;
			name = DP Max Precip Rate;
			type = double;
			range = [50.0, 500.0];
			permission = [ROC, URC];
			accuracy = [0.1];
			unit = mm/hr;
			description = If a rate > DP Max Precip Rate, \
				      replace it with DP Max Precip Rate.;

    Num_zones		value = 0;
			name = Number of Exclusion Zones;
			type = int;
			range = [0, 20];
			permission = [URC];
			description = The sequential number of zones (starting with Zone 1) \
                                      that are to be utilized.;

    Beg_azm1		value = 0.0;
			name = Exclusion Zone Limits # 1     - Begin Azimuth #1;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm1		value = 0.0;
			name =                               - End Azimuth #1;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng1		value = 0;
			name =                               - Begin Range #1;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng1		value = 0;
			name =                               - End Range #1;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang1		value = 0.0;
			name =                               - Elevation angle #1;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm2		value = 0.0;
			name = Exclusion Zone Limits # 2     - Begin Azimuth #2;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm2		value = 0.0;
			name =                               - End Azimuth #2;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng2		value = 0;
			name =                               - Begin Range #2;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng2		value = 0;
			name =                               - End Range #2;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang2		value = 0.0;
			name =                               - Elevation angle #2;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm3		value = 0.0;
			name = Exclusion Zone Limits # 3     - Begin Azimuth #3;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm3		value = 0.0;
			name =                               - End Azimuth #3;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng3		value = 0;
			name =                               - Begin Range #3;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng3		value = 0;
			name =                               - End Range #3;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang3		value = 0.0;
			name =                               - Elevation angle #3;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm4		value = 0.0;
			name = Exclusion Zone Limits # 4     - Begin Azimuth #4;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm4		value = 0.0;
			name =                               - End Azimuth #4;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng4		value = 0;
			name =                               - Begin Range #4;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng4		value = 0;
			name =                               - End Range #4;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang4		value = 0.0;
			name =                               - Elevation angle #4;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm5		value = 0.0;
			name = Exclusion Zone Limits # 5     - Begin Azimuth #5;
			type = float;
			accuracy = [0.1];
			range = [0.0, 360.0];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm5		value = 0.0;
			name =                               - End Azimuth #5;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng5		value = 0;
			name =                               - Begin Range #5;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng5		value = 0;
			name =                               - End Range #5;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang5		value = 0.0;
			name =                               - Elevation angle #5;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm6		value = 0.0;
			name = Exclusion Zone Limits # 6     - Begin Azimuth #6;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm6		value = 0.0;
			name =                               - End Azimuth #6;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng6		value = 0;
			name =                               - Begin Range #6;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng6		value = 0;
			name =                               - End Range #6;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang6		value = 0.0;
			name =                               - Elevation angle #6;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm7		value = 0.0;
			name = Exclusion Zone Limits # 7     - Begin Azimuth #7;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm7		value = 0.0;
			name =                               - End Azimuth #7;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng7		value = 0;
			name =                               - Begin Range #7;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng7		value = 0;
			name =                               - End Range #7;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang7		value = 0.0;
			name =                               - Elevation angle #7;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm8		value = 0.0;
			name = Exclusion Zone Limits # 8     - Begin Azimuth #8;
			type = float;
			accuracy = [0.1];
			range = [0.0, 360.0];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm8		value = 0.0;
			name =                               - End Azimuth #8;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng8		value = 0;
			name =                               - Begin Range #8;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng8		value = 0;
			name =                               - End Range #8;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang8		value = 0.0;
			name =                               - Elevation angle #8;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm9		value = 0.0;
			name = Exclusion Zone Limits # 9     - Begin Azimuth #9;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm9		value = 0.0;
			name =                               - End Azimuth #9;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng9		value = 0;
			name =                               - Begin Range #9;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng9		value = 0;
			name =                               - End Range #9;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang9		value = 0.0;
			name =                               - Elevation angle #9;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm10		value = 0.0;
			name = Exclusion Zone Limits # 10    - Begin Azimuth #10;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm10		value = 0.0;
			name =                               - End Azimuth #10;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng10		value = 0;
			name =                               - Begin Range #10;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng10		value = 0;
			name =                               - End Range #10;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang10		value = 0.0;
			name =                               - Elevation angle #10;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm11		value = 0.0;
			name = Exclusion Zone Limits # 11    - Begin Azimuth #11;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm11		value = 0.0;
			name =                               - End Azimuth #11;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng11		value = 0;
			name =                               - Begin Range #11;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng11		value = 0;
			name =                               - End Range #11;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang11		value = 0.0;
			name =                               - Elevation angle #11;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm12		value = 0.0;
			name = Exclusion Zone Limits # 12    - Begin Azimuth #12;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm12		value = 0.0;
			name =                               - End Azimuth #12;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng12		value = 0;
			name =                               - Begin Range #12;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng12		value = 0;
			name =                               - End Range #12;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang12		value = 0.0;
			name =                               - Elevation angle #12;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm13		value = 0.0;
			name = Exclusion Zone Limits # 13    - Begin Azimuth #13;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm13		value = 0.0;
			name =                               - End Azimuth #13;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng13		value = 0;
			name =                               - Begin Range #13;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng13		value = 0;
			name =                               - End Range #13;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang13		value = 0.0;
			name =                               - Elevation angle #13;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm14		value = 0.0;
			name = Exclusion Zone Limits # 14    - Begin Azimuth #14;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm14		value = 0.0;
			name =                               - End Azimuth #14;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng14		value = 0;
			name =                               - Begin Range #14;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng14		value = 0;
			name =                               - End Range #14;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang14		value = 0.0;
			name =                               - Elevation angle #14;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm15		value = 0.0;
			name = Exclusion Zone Limits # 15    - Begin Azimuth #15;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm15		value = 0.0;
			name =                               - End Azimuth #15;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng15		value = 0;
			name =                               - Begin Range #15;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng15		value = 0;
			name =                               - End Range #15;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang15		value = 0.0;
			name =                               - Elevation angle #15;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm16		value = 0.0;
			name = Exclusion Zone Limits # 16    - Begin Azimuth #16;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm16		value = 0.0;
			name =                               - End Azimuth #16;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng16		value = 0;
			name =                               - Begin Range #16;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng16		value = 0;
			name =                               - End Range #16;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang16		value = 0.0;
			name =                               - Elevation angle #16;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm17		value = 0.0;
			name = Exclusion Zone Limits # 17    - Begin Azimuth #17;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm17		value = 0.0;
			name =                               - End Azimuth #17;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng17		value = 0;
			name =                               - Begin Range #17;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng17		value = 0;
			name =                               - End Range #17;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang17		value = 0.0;
			name =                               - Elevation angle #17;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm18		value = 0.0;
			name = Exclusion Zone Limits # 18    - Begin Azimuth #18;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm18		value = 0.0;
			name =                               - End Azimuth #18;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng18		value = 0;
			name =                               - Begin Range #18;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng18		value = 0;
			name =                               - End Range #18;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang18		value = 0.0;
			name =                               - Elevation angle #18;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm19		value = 0.0;
			name = Exclusion Zone Limits # 19    - Begin Azimuth #19;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm19		value = 0.0;
			name =                               - End Azimuth #19;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng19		value = 0;
			name =                               - Begin Range #19;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng19		value = 0;
			name =                               - End Range #19;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang19		value = 0.0;
			name =                               - Elevation angle #19;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;

    Beg_azm20		value = 0.0;
			name = Exclusion Zone Limits # 20    - Begin Azimuth #20;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Azimuth beginning and ending values are clockwise.  Ranges \
                                      begin closest to the radar.  Elevation is for the top of \
                                      the selected exclusion zone.;

    End_azm20		value = 0.0;
			name =                               - End Azimuth #20;
			type = float;
			range = [0.0, 360.0];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Ending azimuth.;

    Beg_rng20		value = 0;
			name =                               - Begin Range #20;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Beginning range.;

    End_rng20		value = 0;
			name =                               - End Range #20;
			type = int;
			range = [0, 124];
			unit = nm;
			permission = [URC];
			description = Ending range.;

    Elev_ang20		value = 0.0;
			name =                               - Elevation angle #20;
			type = float;
			range = [0.0, 19.5];
			accuracy = [0.1];
			unit = degrees;
			permission = [URC];
			description = Elevation angle.;
