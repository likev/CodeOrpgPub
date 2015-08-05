/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2009/03/03 23:11:32 $ 
 * $Id: radazvd.h,v 1.11 2009/03/03 23:11:32 steves Exp $ 
 * $Revision: 1.11 $ 
 * $State: Exp $ 
 */ 
#ifndef RADAZVD_H
#define RADAZVD_H

#include <orpgctype.h>

#define RADAZVD_DEA_NAME "alg.radazvd"


/*	Velocity Dealiasing Algorithm 		*/
typedef struct {

	fint 	num_replace_lookahead_s; 		/*# @name "Num Replace (Look Ahead)  [NRLA]"
						    @desc Short Pulse Number of sample bins to look ahead the
							  previous saved radial to find a valid velocity
							  to compare with the first rejected velocity.
							  Used to compute previous radial average.
						    @default 10  @min 5  @max 20  @units bins
						    @legacy_name AD_NUM_REP_LKAHD_S
						*/
 	fint 	num_replace_lookback_s;		/*# @name "Num Replace (Look Back)  [NRLB]"
						    @desc "Short Pulse Number of sample bins to look back on the ",
							  "current radial to locate a valid velocity for ",
							  "reunfolding."
						    @default 4  @min 4  @max 10  @units bins
						    @legacy_name AD_NUM_REP_LKBK_S
						*/
	fint	num_lookback_s;			/*# @name "Number (Look Back)  [NLB]"
						    @desc "Short Pulse Number of sample bins to look back on the ",
							  "current radial to locate a velocity when the ",
							  "nine-point average cannot be computed."
						    @default 30  @min 5  @max 45  @units bins
						    @legacy_name AD_NUM_LKBK_S
						*/
	fint	num_lookforward_s;		/*# @name "Number (Look Forward)  [NLF]"
						    @desc Short Pulse Number of sample bins to look forward on the
							  previous saved radial to locate a velocity when
							  the nine-point average cannot be computed.
						    @default 15  @min 10  @max 20  @units bins
						    @units bins  @legacy_name AD_NUM_LKFOR_S
						*/
	fint    th_consecutive_rejected_s;	/*# @name "Thresh (Consec Reject)  [TCBR]"
						    @desc "Short Pulse Maximum number of consecutive sample bins ",
							  "which can be rejected bfore replacement commences."
						    @default 5  @min 1  @max 10  @units bins
						    @legacy_name AD_TH_CONBIN_REJ_S
						*/
	fint	th_maximum_missing_s;		/*# @name "Threshold (Max Missing)  [TMM]"
						    @desc "Short Pulse Maximum number of missing sample bins allowed in ",
							  "the search for a valid velocity when replacing a ",
							  "rejected velocity."
						    @default 30  @min 10  @max 50  @units bins
						    @legacy_name AD_TH_MXMISS_S
						*/
	fint 	num_reunfold_previous_azm_s;	/*# @name "Num (Reunfold Prev Az)  [NRPA]"
						    @desc "Short Pulse Number of surrounding sample bins to search for ",
							  "a valid velocity on the previous saved radial.  ",
							  "Used for reunfolding."
						    @default 10  @min 5  @max 20  @units bins
						    @legacy_name AD_NUM_REUNF_PRAZS_S
						*/
	fint 	num_reunfold_current_azm_s;	/*# @name "Num (Reunfold Current Az)  [NRCA]"
						    @desc "Short Pulse Num (Reunfold Current Az)"
						    @default 30  @min 15  @max 50  @units bins
						    @legacy_name AD_NUM_REUNF_CAZS_S
                                                */
	freal	th_difference_unfold_s;		/*# @name "Thresh (Diff Unfold)  [TDU]"
						    @desc "Short Pulse Maximum allowed difference in velocity between",
							  " adjacent sample bins used during initial radial ",
							  "continuity-based dealiasing."
						    @default 10.0  @min 1.0  @max 15.0  @precision 2  @units "m/s"
						    @legacy_name AD_DIFF_UNFOLD_S
						*/
        fint 	th_max_bins_with_jump_s;		/*# @name "Thresh (Max Bins Jump)  [TMBJ]"
						    @desc "Short Pulse Maximum number of sample bins allowed between velocity",
							  " jumps of opposite sense on current radial."
						    @default 75  @min 10  @max 100  @units bins
						    @legacy_name AD_TH_MXBINS_JMP_S
						*/
	freal 	th_vel_jump_fraction_radial_s;	/*# @name "Thresh (Velocity Jump Fact)  [TJR]"
						    @desc "Short Pulse Multiplied times Nyquist interval to define the maximum ",
							  "radial velocity jump."
						    @default 0.75  @min 0.50  @max 1.00  @precision 2  @units factor
					 	    @legacy_name AD_TH_VEL_JMP_FRAD_S
						*/
	freal 	th_vel_jump_fraction_azm_s;	/*# @name "Thresh (Az Diff Fact)  [TJA]"
						    @desc "Short Pulse Multiplied times the Nyquist interval to define the ",
							  "maximum azimuthal velocity jump."
						    @default 0.60  @min 0.50 @max 1.0  @precision 2  @units factor
						    @legacy_name AD_TH_VEL_JMP_FAZ_S */
        freal	th_scale_standard_dev_s;		/*# @name "Thresh (Scale Std. Dev.)  [TSSD]"
						    @desc "Short Pulse Muliplied times the Nyquist interval to generate a ",
							  "scale factor used in assessing the nine-point average."
                                                    @default 0.40  @min 0.00  @max 1.00  @precision 2  @units factor
						    @legacy_name AD_TH_SCL_STDEV_S
						*/
	fint 	th_max_azm_with_jump_s;		/*# @name "Thresh [Max Cont Az Jump)  [TMCJ]"
						    @desc "Short Pulse Maximum number of azimuths with velocity jump allowed ",
							  "before the previous saved radial is set to BAD."
						    @default 5  @min 1  @max 10  @units azimuths
						    @legacy_name AD_TH_MAX_CONAZJMP_S
						*/
	freal	th_scale_diff_unfold_s;		/*# @name "Thresh (Scale Diff Unfld)  [TSDU]"
						    @desc Short Pulse Multiplied times threshold difference unfold to compute
							  the relaxed unfolding difference threshold.
						    @default 1.50  @min 1.00  @max 2.00 @units factor  @precision 2
						    @legacy_name AD_SCL_DIFF_UNFOLD_S
						*/
	fint 	th_bins_large_azm_jump_s;		/*# @name "Thresh (Num Az Jump)  [TBLA]"
						    @desc Short Pulse Maximum number of bins with large velocity jumps in
							  azimuth allowed before reunfolding is initiated.
						    @default 10  @min 3  @max 12  @units bins
						    @legacy_name AD_TH_BINS_LRG_AZJMP_S
						*/
	fint	th_bin_first_check_s;		/*# @name "Thresh (Radial)  [NBF]"
						    @desc "Short Pulse Number of sample bins to look back for initial radial ",
							  "continuity check."
						    @default 5  @min 5  @max 10  @units bins
						    @legacy_name AD_BINS_FSTCHK_S
						*/
	fint 	env_winds_timeout_s;		/*# @name "Environmental Winds Time Out"
						    @desc "Short Pulse Age limit of environmental winds table for the data used ",
							  "by the dealiasing algorithm."
						    @default 720  @min 1  @max 999  @units mins
						    @legacy_name AD_ENV_SOUND_TO_S
						*/
	flogical use_sounding_s;			/*# @name "Flag (Sounding)  [USF]"
						    @desc "Short Pulse Flag indicating the use of the environmental winds table ",
							  "by the dealiasing algorithm."
						    @enum_values "No", "Yes"
						    @default "Yes"
						    @legacy_name AD_USE_SOUNDINGS_FLAG_S
						*/
	flogical replace_rejected_vel_s;		/*# @name "Flag (Report Rejected Vel)  [ARRV]"
						    @desc "Short Pulse Default weather mode at RPG start up"
						    @enum_values "No", "Yes"  @default "Yes"
						    @legacy_name AD_REP_REJECTED_VEL_S
						*/
	fint	num_interval_checks_s;		/*# @name "Number (Interval Checks)"
						    @desc Short Pulse Number of interval checks
						    @default 4  @min 0  @max 6
						    @legacy_name AD_NUM_INTRVL_CHK_S
						*/
	fint 	num_replace_lookahead_l; 		/*# @name "Num Replace (Look Ahead)  [NRLA]"
						    @desc Long Pulse Number of sample bins to look ahead the
							  previous saved radial to find a valid velocity
							  to compare with the first rejected velocity.
							  Used to compute previous radial average.
						    @default 10  @min 5  @max 20  @units bins
						    @legacy_name AD_NUM_REP_LKAHD_L
						*/
 	fint 	num_replace_lookback_l;		/*# @name "Num Replace (Look Back)  [NRLB]"
						    @desc "Long Pulse Number of sample bins to look back on the ",
							  "current radial to locate a valid velocity for ",
							  "reunfolding."
						    @default 4  @min 4  @max 10  @units bins
						    @legacy_name AD_NUM_REP_LKBK_L
						*/
	fint	num_lookback_l;			/*# @name "Number (Look Back)  [NLB]"
						    @desc "Long Pulse Number of sample bins to look back on the ",
							  "current radial to locate a velocity when the ",
							  "nine-point average cannot be computed."
						    @default 30  @min 5  @max 45  @units bins
						    @legacy_name AD_NUM_LKBK_L
						*/
	fint	num_lookforward_l;		/*# @name "Number (Look Forward)  [NLF]"
						    @desc Long Pulse Number of sample bins to look forward on the
							  previous saved radial to locate a velocity when
							  the nine-point average cannot be computed.
						    @default 15  @min 10  @max 20  @units bins
						    @units bins  @legacy_name AD_NUM_LKFOR_L
						*/
	fint    th_consecutive_rejected_l;	/*# @name "Thresh (Consec Reject)  [TCBR]"
						    @desc "Long Pulse Maximum number of consecutive sample bins ",
							  "which can be rejected bfore replacement commences."
						    @default 5  @min 1  @max 10  @units bins
						    @legacy_name AD_TH_CONBIN_REJ_L
						*/
	fint	th_maximum_missing_l;		/*# @name "Threshold (Max Missing)  [TMM]"
						    @desc "Long Pulse Maximum number of missing sample bins allowed in ",
							  "the search for a valid velocity when replacing a ",
							  "rejected velocity."
						    @default 30  @min 10  @max 50  @units bins
						    @legacy_name AD_TH_MXMISS_L
						*/
	fint 	num_reunfold_previous_azm_l;	/*# @name "Num (Reunfold Prev Az)  [NRPA]"
						    @desc "Long Pulse Number of surrounding sample bins to search for ",
							  "a valid velocity on the previous saved radial.  ",
							  "Used for reunfolding."
						    @default 10  @min 5  @max 20  @units bins
						    @legacy_name AD_NUM_REUNF_PRAZS_L
						*/
	fint 	num_reunfold_current_azm_l;	/*# @name "Num (Reunfold Current Az)  [NRCA]"
						    @desc "Long Pulse Num (Reunfold Current Az)"
						    @default 30  @min 15  @max 50  @units bins
						    @legacy_name AD_NUM_REUNF_CAZS_L
                                                */
	freal	th_difference_unfold_l;		/*# @name "Thresh (Diff Unfold)  [TDU]"
						    @desc "Long Pulse Maximum allowed difference in velocity between",
							  " adjacent sample bins used during initial radial ",
							  "continuity-based dealiasing."
						    @default 10.0  @min 1.0  @max 15.0  @precision 2  @units "m/s"
						    @legacy_name AD_DIFF_UNFOLD_L
						*/
        fint 	th_max_bins_with_jump_l;		/*# @name "Thresh (Max Bins Jump)  [TMBJ]"
						    @desc "Long Pulse Maximum number of sample bins allowed between velocity",
							  " jumps of opposite sense on current radial."
						    @default 75  @min 10  @max 100  @units bins
						    @legacy_name AD_TH_MXBINS_JMP_L
						*/
	freal 	th_vel_jump_fraction_radial_l;	/*# @name "Thresh (Velocity Jump Fact)  [TJR]"
						    @desc "Long Pulse Multiplied times Nyquist interval to define the maximum ",
							  "radial velocity jump."
						    @default 0.75  @min 0.50  @max 1.00  @precision 2  @units factor
					 	    @legacy_name AD_TH_VEL_JMP_FRAD_L
						*/
	freal 	th_vel_jump_fraction_azm_l;	/*# @name "Thresh (Az Diff Fact)  [TJA]"
						    @desc "Long Pulse Multiplied times the Nyquist interval to define the ",
							  "maximum azimuthal velocity jump."
						    @default 0.60  @min 0.50 @max 1.0  @precision 2  @units factor
						    @legacy_name AD_TH_VEL_JMP_FAZ_L */
        freal	th_scale_standard_dev_l;		/*# @name "Thresh (Scale Std. Dev.)  [TSSD]"
						    @desc "Long Pulse Muliplied times the Nyquist interval to generate a ",
							  "scale factor used in assessing the nine-point average."
                                                    @default 0.40  @min 0.00  @max 1.00  @precision 2  @units factor
						    @legacy_name AD_TH_SCL_STDEV_L
						*/
	fint 	th_max_azm_with_jump_l;		/*# @name "Thresh [Max Cont Az Jump)  [TMCJ]"
						    @desc "Long Pulse Maximum number of azimuths with velocity jump allowed ",
							  "before the previous saved radial is set to BAD."
						    @default 5  @min 1  @max 10  @units azimuths
						    @legacy_name AD_TH_MAX_CONAZJMP_L
						*/
	freal	th_scale_diff_unfold_l;		/*# @name "Thresh (Scale Diff Unfld)  [TSDU]"
						    @desc Long Pulse Multiplied times threshold difference unfold to compute
							  the relaxed unfolding difference threshold.
						    @default 1.50  @min 1.00  @max 2.00 @units factor  @precision 2
						    @legacy_name AD_SCL_DIFF_UNFOLD_L
						*/
	fint 	th_bins_large_azm_jump_l;		/*# @name "Thresh (Num Az Jump)  [TBLA]"
						    @desc Long Pulse Maximum number of bins with large velocity jumps in
							  azimuth allowed before reunfolding is initiated.
						    @default 10  @min 3  @max 12  @units bins
						    @legacy_name AD_TH_BINS_LRG_AZJMP_L
						*/
	fint	th_bin_first_check_l;		/*# @name "Thresh (Radial)  [NBF]"
						    @desc "Long Pulse Number of sample bins to look back for initial radial ",
							  "continuity check."
						    @default 5  @min 5  @max 10  @units bins
						    @legacy_name AD_BINS_FSTCHK_L
						*/
	fint 	env_winds_timeout_l;		/*# @name "Environmental Winds Time Out"
						    @desc "Long Pulse Age limit of environmental winds table for the data used ",
							  "by the dealiasing algorithm."
						    @default 720  @min 1  @max 999  @units mins
						    @legacy_name AD_ENV_SOUND_TO_L
						*/
	flogical use_sounding_l;			/*# @name "Flag (Sounding)  [USF]"
						    @desc "Long Pulse Flag indicating the use of the environmental winds table ",
							  "by the dealiasing algorithm."
						    @enum_values "No", "Yes"
						    @default "Yes"
						    @legacy_name AD_USE_SOUNDINGS_FLAG_L
						*/
	flogical replace_rejected_vel_l;		/*# @name "Flag (Report Rejected Vel)  [ARRV]"
						    @desc "Long Pulse Default weather mode at RPG start up"
						    @enum_values "No", "Yes"  @default "Yes"
						    @legacy_name AD_REP_REJECTED_VEL_L
						*/
	fint	num_interval_checks_l;		/*# @name "Number (Interval Checks)"
						    @desc Long Pulse Number of interval checks
						    @default 4  @min 0  @max 6
						    @legacy_name AD_NUM_INTRVL_CHK_L
						*/

	flogical disable_radaz_dealiasing;	/*# @name "Disable Radial/Azimuth Dealiasing"
						    @desc Disable Dealiasing Flag
						    @enum_values "No", "Yes"  @default "No"
						*/

	flogical use_sprt_replace_rej;		/*# @name "Use SPRT Half Nyquist Replace Reject"
						    @desc "Enable use of half of extended Nyquist 
							"with replace rejected vel."
						    @enum_values "No", "Yes"
						    @default "Yes"
						*/
        fint    th_consecutive_rejected_sprt;   /*# @name "Thresh (Consec Reject)  [TCBR]"
                                                    @desc "Short Pulse Maximum number of consecutive sample bins ",
                                                          "which can be rejected bfore replacement commences."
                                                    @default 10  @min 1  @max 15  @units bins
                                                    @legacy_name AD_TH_CONBIN_REJ_S
                                                */

        fint    th_max_azm_with_jump_sprt;      /*# @name "Thresh [Max Cont Az Jump)  [TMCJ]"
                                                    @desc "Short Pulse Maximum number of azimuths with velocity jump allowed ",
                                                          "before the previous saved radial is set to BAD."
                                                    @default 1  @min 1  @max 10  @units azimuths
                                                    @legacy_name AD_TH_MAX_CONAZJMP_S
                                                */

	flogical use_2D_dealiasing;		/*# @name "Enable 2D Dealiasing"
						    @desc Enable 2D Dealiasing Flag
						    @enum_values "No", "Yes"  @default "No"
						*/
} radazvd_t;


#endif
