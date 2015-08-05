# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/12 20:57:05 $
# $Id: hail.alg,v 1.16 2010/05/12 20:57:05 ccalvert Exp $
# $Revision: 1.16 $
# $State: Exp $


    alg_name		value = Hail Detection

    hke_ref_wgt_low	value = 40;	
			name = Thr HKE Ref Wgt Lower Limit;	
			type = int;		
			range = [20, 60];	
			unit = dBZ;		
			description = Lower limit of reflectivity used in the reflectivity;	

    hke_ref_wgt_high	value = 50;	
			name = Thr HKE Ref Wgt Upper Limit;	
			type = int;		
			range = [30, 70];	
			unit = dBZ;		
			description = Upper limit of reflectivity used in the reflectivity;	

    poh_min_ref		value = 45;	
			name = Thr Min Reflectivity POH;	
			type = int;		
			range = [30, 60];	
			unit = dBZ;		
			description = Minimum maximum reflectivity of component used in the;	

    hke_coef1		value = 0.0005;	
			name = HKE Coefficient 1;	
			type = double;		
			range = [0.000001, 1.000000];	
			accuracy = [0.000001];
			unit = coefficient;		
			description = Multiplicative factor used in computing;	

    hke_coef2		value = 0.084;	
			name = HKE Coefficient 2;	
			type = double;		
			range = [0.005, 0.500];	
			accuracy = [0.001];
			unit = coefficient;		
			description = Multiplicative exponential factor used in computing the;	

    hke_coef3		value = 10.0;	
			name = HKE Coefficient 3;	
			type = double;		
			range = [1.0, 100.0];	
			accuracy = [0.1];
			unit = coefficient;		
			description = Operand factor used in computing hailfall kinetic energy.;	

    posh_coef		value = 29.0;	
			name = POSH Coefficient;	
			type = double;		
			range = [1.0, 100.0];	
			accuracy = [0.1];
			unit = coefficient;		
			description = Multiplicative factor used in computing the probability of severe;	

    posh_offset		value = 50;	
			name = POSH Offset;	
			type = int;		
			range = [1, 100];	
			unit = %;
			permission = [ROC, URC];
			description = Probability of Severe Hail Offset is generally set to 50%;	

    max_hail_range	value = 230;	
			name = Max Hail Processing Range;	
			type = int;		
			range = [200, 460];	
			unit = km;		
			description = Maximum range of the storm cell (centroid) that is processed by the;	

    shi_hail_size_coef	value = 0.10;	
			name = SHI Hail Size Coefficient;	
			type = double;		
			range = [0.01, 1.00];	
			accuracy = [0.01];
			unit = coefficient;		
			description = Multiplicative factor used in computing the maximum expected hail;	

    shi_hail_size_exp	value = 0.5;	
			name = SHI Hail Size Exponent;	
			type = double;		
			range = [0.1, 1.0];	
			accuracy = [0.1];
			unit = exponent;		
			description = Power to which the severe hail index is raised in computing the;	

    warn_thr_sel_mod_coef	value = 57.5;	
			name = WTSM Coefficient;	
			type = double;		
			range = [0.0, 500.0];	
			accuracy = [0.1];
			unit = 100 * Joules/meter^2/sec;		
			description = Factor applied by the Height 0 deg C in the warning threshold selection model.;	

    warn_thr_sel_mod_off	value =;	
			name = WTSM Offset;	
			type = double;		
			range = [-500.0, 500.0];	
			accuracy = [0.1];
			unit = 10^5 Joules/meter/sec;		
			description = Offset used in the warning threshold selection model.;	
			default = KABR: -96.8, KABX: -16.7, KAKQ: -117.3, KAMA: -56.7, KAMX: -119.1,\
 			KAPX: -93.6, KARX: -97.2, KATX: -110.4, KBBX: -117.1, KBGM: -91.2, KBHX: -76.9,\
			KBIS: -90.2, KBLX: -56.8, KBMX: -107.7, KBOX: -116.9, KBRO: -119.5, KBUF: -107.2,\
			KBYX: -119.4, KCAE: -115.0, KCBW: -105.9, KCBX: -65.9, KCCX: -77.4, KCLE: -105.9,\
 			KCLX: -117.3, KCRP: -118.5, KCXX: -113.5, KCYS: -12.5, KDAX: -118.5, KDDC: -74.2,\
 			KDFX: -100.1, KDGX: -110.3, KDIX: -117.0, KDLH: -94.0, KDMX: -101.8, KDOX: -118.1,\
 			KDTX: -100.2, KDVN: -106.1, KDYX: -93.3, KEAX: -101.9, KEMX: -27.8, KENX: -87.6,\
 			KEOX: -112.3, KEPZ: -47.1, KESX: -34.3, KEVX: -117.1, KEWX: -108.2, KEYX: -71.0,\
 			KFCX: -69.0, KFDR: -98.0, KFDX: -38.7, KFFC: -104.0, KFSD: -94.8, KFSX: 10.7,\
			KFTG: -22.7, KFWS: -107.6, KGGW: -79.7, KGJX: 55.2, KGLD: -55.9, KGRB: -107.3,\
			KGRK: -110.4, KGRR: -105.7, KGSP: -102.5, KGWX: -111.2, KGYX: -112.7, KHDX: -46.2,\
			KHGX: -119.0, KHNX: -115.0, KHPX: -110.1, KHTX: -88.4, KICT: -96.5, KICX: 66.8,\
			KILN: -100.5, KILX: -108.8, KIND: -105.5, KINX: -107.9, KIWA: -96.2, KIWX: -102.5,\
			KJAX: -118.7, KJGX: -110.2, KJKL: -95.4, KLBB: -62.5, KLCH: -119.4, KLIX: -118.6,\
			KLNX: -67.2, KLOT: -107.7, KLRX: -0.8, KLSX: -108.4, KLTX: -118.5, KLVX: -106.4,\
			KLWX: -114.5, KLZK: -109.6, KMAF: -69.6, KMAX: 11.5, KMBX: -93.1, KMHX: -118.5,\
			KMKX: -103.1, KMLB: -119.0, KMOB: -115.9, KMPX: -102.4, KMQT: -94.3, KMRX: -95.9,\
			KMSX: 17.5, KMTX: -5.8, KMUX: -58.8, KMVX: -102.0, KMXX: -112.0, KNKX: -102.6,\
			KNQA: -114.4, KOAX: -98.9, KOHX: -109.2, KOKX: -117.5, KOTX: -78.1, KPAH: -112.1,\
			KPBZ: -98.8, KPDT: -93.3, KPOE: -112.7, KPUX: -27.9, KRAX: -112.9, KRGX: 26.1,
			KRIW: -22.3, KRLX: -100.1, KRTX: -91.5, KSFX: -41.5, KSGF: -96.9, KSHV: -114.2,
			KSJT: -85.9, KSOX: -66.8, KSRX: -108.4, KTBW: -118.9, KTFX: -54.8, KTLH: -117.9,\
			KTLX: -98.6, KTWX: -96.2, KTYX: -86.7, KUDX: -66.2, KUEX: -85.0, KVAX: -115.9,\
			KVBX: -97.9, KVNX: -99.0, KVTX: -71.8, KVWX: -110.1, KYUX: -116.8, LPLA: -61.2,\
			PABC: -117.6, PACG: -116.2, PAEC: -119.4, PAHG: -114.8, PAIH: -118.7, PAKC: -118.5,\
			PAPD: -73.6, PGUA: -114.4, PHKI: -115.9, PHKM: -52.2, PHMO: -95.7, PHWA: -95.4,\
			RKJK: -117.6, RKSG: -118.7, RODN: -115.2, TJUA: -70.0, Other_sites: -96.3;

    poh_height_diff1	value = 1.625;	
			name = POH Height Difference #1;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 0% POH.;	

    poh_height_diff2	value = 1.875;	
			name = POH Height Difference #2;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 10% POH.;	

    poh_height_diff3	value = 2.125;	
			name = POH Height Difference #3;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 20% POH.;	

    poh_height_diff4	value = 2.375;	
			name = POH Height Difference #4;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 30% POH.;	

    poh_height_diff5	value = 2.625;	
			name = POH Height Difference #5;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 40% POH.;	

    poh_height_diff6	value = 2.925;	
			name = POH Height Difference #6;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 50% POH.;	

    poh_height_diff7	value = 3.300;	
			name = POH Height Difference #7;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 60% POH.;	

    poh_height_diff8	value = 3.750;	
			name = POH Height Difference #8;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 70% POH.;	

    poh_height_diff9	value = 4.500;	
			name = POH Height Difference #9;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 80% POH.;	

    poh_height_diff10	value = 5.500;	
			name = POH Height Difference #10;	
			type = double;		
			range = [0.000, 20.000];	
			accuracy = [0.001];
			unit = km;		
			description = Maximum height difference which correlates to a 90% POH.;	

    rcm_probable_hail	value = 30;	
			name = Thresh (RCM probable hail);	
			type = int;		
			range = [0, 100];	
			unit = %;		
			description = Threshold of probability of severe hail at which a cell;	

    rcm_positive_hail	value = 50;	
			name = RCM positive hail;	
			type = int;		
			range = [0, 100];	
			unit = %;		
			description = Threshold of probability of severe hail POSH at which a;	

    height_0		value = 10.5;	
			name = Height (0 Deg Celsius);	
			type = double;		
			range = [0.0, 70.0];	
			accuracy = [0.1];
			unit = kft;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Height(MSL) of the 0 degree Celsius environmental temperature.;	

    height_minus_20	value = 20.0;	
			name = Height (-20 Deg Celsius);	
			type = double;		
			range = [0.0, 70.0];	
			accuracy = [0.1];
			unit = kft;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Height(MSL) of the -20 degree celsius environmental temperature.;	

    hail_date_yy	value = 96;	
			name = Hail Date YY;	
			type = int;		
			range = [0, 99];	
			unit = year;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Year of the last modification to the 0 degree and -20 degree temperature data.;	

    hail_date_mm	value = 1;	
			name = Hail Date MM;	
			type = int;		
			range = [1, 12];	
			unit = month;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Month of the last modification to the 0 degree and -20 degree temperature data.;	

    hail_date_dd	value = 1;	
			name = Hail Date DD;	
			type = int;		
			range = [1, 31];	
			unit = day;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Day of the last modification to the 0 degree and -20 degree temperature data.;	

    hail_time_hr	value = 12;	
			name = Hail Time HH;	
			type = int;		
			range = [0, 23];	
			unit = hrs;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Hour of the last modification to the 0 degree and -20 degree temperature data.;	

    hail_time_min	value = 0;	
			name = Hail Time MM;	
			type = int;		
			range = [0, 59];	
			unit = mins;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Minute of the last modification to the 0 degree and -20 degree temperature data.;	

    hail_time_sec	value = 0;	
			name = Hail Time SS;	
			type = int;		
			range = [0, 59];	
			unit = secs;		
			permission = [ROC, URC];
			misc = @-Not_for_alg_edit-@;
			description = Seconds of the last modification to the 0 degree and -20 degree temperature data.;	

