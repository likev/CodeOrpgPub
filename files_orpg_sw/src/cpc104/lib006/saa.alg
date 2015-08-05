# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/12 20:57:07 $
# $Id: saa.alg,v 1.11 2010/05/12 20:57:07 ccalvert Exp $
# $Revision: 1.11 $
# $State: Exp $

    alg_name		value = Snow Accumulation

    g_cf_ZS_mult	value = ;
			name = Z-S Multiplicative Coefficient;
			type = double;
			range = [10.0, 1000.0];
			accuracy = [0.1];
			permission = [ROC, URC];
			default = SAA_Northeast:  		120.0,	\
			          SAA_GreatLakes: 		180.0,	\
			          SAA_NorthernPlaines: 		180.0,	\
			          SAA_HighPlaines: 		130.0,	\
			          SAA_IntermountainWest: 	40.0, 	\
			          SAA_SierraNevada: 		222.0,	\
			          Other_sites: 			120.0;
                        description = The multiplicative coefficient of a power relationship used to determine the snow water equivalent, S, to reflectance,  Z, based on empirical testing.  This parameter has been optimized by geographical regions.;

    g_cf_ZS_power	value = ;
			name = Z-S Power Coefficient;
			type = double;
			range = [1.00, 3.00];
			accuracy = [0.01];
			permission = [ROC, URC];
			default = SAA_Northeast:  		2.00,	\
			          SAA_GreatLakes: 		2.00,	\
			          SAA_NorthernPlaines: 		2.00,	\
			          SAA_HighPlaines: 		2.00,	\
			          SAA_IntermountainWest: 	2.00, 	\
			          SAA_SierraNevada:	 	2.00,	\
			          Other_sites:		 	2.00;	
                        description = The power coefficient of a power relationship used to determine the snow water equivalent (S) to reflectance (Z) based on empirical testing.  This parameter has been optimized by geographical regions.;

    g_sw_ratio		value = ;
			name = Snow - Water Ratio;
			type = double;
			range = [4.0, 100.0];
			accuracy = [0.1];
			unit = in/in;
			permission = [ROC, URC];
			default = SAA_Northeast:  		11.8,	\
			          SAA_GreatLakes: 		16.7,	\
			          SAA_NorthernPlaines: 		11.8,	\
			          SAA_HighPlaines: 		13.3,	\
			          SAA_IntermountainWest: 	14.3, 	\
			          SAA_SierraNevada: 		8.0,	\
				  Other_sites:			11.8;
                        description = This parameter, determined empirically for different geographical regions, is a multiplicative term to compute snowfall accumulation from the snow water equivalent, S.;

    g_thr_mn_hgt_corr	value = ;
			name = Minimum Height Correction;
			type = double;
			range = [0.01, 20.00];
			accuracy = [0.01];
			unit = km;
			permission = [ROC, URC];
			default = SAA_Northeast:  		0.45,	\
			          SAA_GreatLakes: 		0.45,	\
			          SAA_NorthernPlaines: 		0.01,	\
			          SAA_HighPlaines: 		0.01,	\
			          SAA_IntermountainWest: 	0.60, 	\
			          SAA_SierraNevada: 		0.60,	\
				  Other_sites:			0.45;	
                        description = The height (km) above which an upward adjustment (range correction factor) to the Z-S relationship is made to account for partial beam filling.  It is applied to all values of Z in the Hybrid Scan Reflectivity field not from the lowest elevation angle. This parameter has been optimized by geographical regions.;

    g_cf1_rng_hgt	value = ;
			name = Range Height Correction Coefficient #1;
			type = double;
			range = [-5.0000, 5.0000];
			accuracy = [0.0001];
			permission = [ROC, URC];
			default = SAA_Northeast:  		0.8414,	\
			          SAA_GreatLakes: 		0.8414,	\
			          SAA_NorthernPlaines: 		1.0970,	\
			          SAA_HighPlaines: 		1.0970,	\
			          SAA_IntermountainWest: 	0.3580,	\
			          SAA_SierraNevada: 		2.4371, \
				  Other_sites:			0.8414;	
                        description = The constant term of a second degree polynomial used to compute a range correction factor to mitigate underestimates of S due to partial beam filling.  This parameter has been optimized by geographical regions.;

    g_cf2_rng_hgt	value = ;
			name = Range Height Correction Coefficient #2;
			type = double;
			range = [-0.5000, 0.5000];
			accuracy = [0.0001];
			permission = [ROC, URC];
			default = SAA_Northeast:  		0.0040,	\
			          SAA_GreatLakes: 		0.0040,	\
			          SAA_NorthernPlaines: 		0.0069,	\
			          SAA_HighPlaines: 		0.0069,	\
			          SAA_IntermountainWest: 	-0.0020,\
			          SAA_SierraNevada: 		-0.0640,\
				  Other_sites:			0.0040;	
                        description = A multiplicative coefficient for the linear term of a second degree polynomial used to compute a range correction factor to mitigate underestimates of S due to partial beam filling.  This parameter has been optimized by geographical regions.;

    g_cf3_rng_hgt	value = ;
			name = Range Height Correction Coefficient #3;
			type = double;
			range = [-0.5000, 0.5000];
			accuracy = [0.0001];
			permission = [ROC, URC];
			default = SAA_Northeast:  		0.0000,	\
			          SAA_GreatLakes: 		0.0000,	\
			          SAA_NorthernPlaines: 		0.0000,	\
			          SAA_HighPlaines: 		0.0000,	\
			          SAA_IntermountainWest:	0.0003,	\
			          SAA_SierraNevada: 		0.0007, \
				  Other_sites:			0.0000;
                        description = A multiplicative coefficient for the quadratic term of a second degree polynomial equation used to compute a range correction factor to mitigate underestimates of S due to partial beam filling.  This parameter has been optimized by geographical regions.;

    g_use_RCA_flag	value = No;
			name = RCA Correction Flag;
			type = string;
			range = { No, Yes};
			enum =  0, 1;

    g_thr_lo_dBZ	value = 5.0;
			name = Minimum Reflectivity/Isolated Bin Threshold;
			type = double;
			range = [-10.0, 25.0];
			accuracy = [0.1];
			unit = dBZ;

    g_thr_hi_dBZ	value = 40.0;
			name = Maximum Reflectivity/Outlier Bin Threshold;
			type = double;
			range = [30.0, 55.0];
			accuracy = [0.1];
			unit = dBZ;

    g_thr_time_span	value = 11;
			name = Time Span Threshold;
			type = int;
			range = [3, 30];
			unit = mins;

    g_thr_mn_time	value = 54;
			name = Minimum Time Threshold;
			type = int;
			range = [0, 60];
			unit = mins;

    g_rhc_base_elev	value = 0.5;
			name = Base Elevation for Default Range Height Correction;
			type = double;
			range = [0.1, 2.0];
			accuracy = [0.1];
			unit = degrees;

    @site_names.SAA_Northeast: 		PABC PACG PAPD PAHG PAKC PAIH PAEC KBMX KMXX KEOX KHTX KMOB LPLA \
					KDOX KBYX KJAX KMLB KAMX KEVX KTBW KTLH KJGX KFFC KVAX PGUA PHKM \
					PHMO PHWA PHKI RODN RKSG RKJK KPOE KLCH KSHV KLIX KBOX KGYX KCBW \
					KDGX KGWX KBIX KRAX KMHX KLTX KDIX KBGM KENX KOKX KCCX TJUA KCLX \
					KGSP KCAE KDFX KBRO KCRP KHGX KFWS KGRK KDYX KEWX KSJT KFCX KLWX \
					KAKQ KCXX KRLX 

    @site_names.SAA_GreatLakes:		KIWX KAPX KGRR KMQT KDTX KBUF KTYX KCLE KILN KPBZ KGRB

    @site_names.SAA_NorthernPlaines:	KSRX KLZK KDVN KDMX KILX KLOT KIND KVWX KDDC KGLD KTWX KICT KLVX \
					KJKL KPAH KHPX KMPX KDLH KEAX KSGF KLSX KBIS KMBX KMVX KUEX KLNX \
					KOAX KVNX KFDR KINX KTLX KCRI KABR KFSD KNQA KMRX KOHX KMKX KARX
	 
    @site_names.SAA_HighPlaines: 	KAMA KLBB KMAF KCYS KRIW KUDX KBLX KGGW KFTG KPUX 

    @site_names.SAA_IntermountainWest:	KICX KMTX KOTX KHDX KEPZ KLRX KESX KPDT KTFX KMSX KABX KFDX KCBX \
					KSFX KFSX KIWA KEMX KYUX KGJX 
 
    @site_names.SAA_SierraNevada:	KATX KRGX KMAX KRTX KEYX KDAX KBHX KHNX KVTX KMUX KVBX KBBX KNKX \
					KSOX
