
/******************************************************************

    vdeal's module containing functions for estimating initial EW.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/16 14:43:57 $
 * $Id: vdeal_estimate_ew.c,v 1.19 2014/07/16 14:43:57 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include "vdeal.h"

extern int Test_mode;
#define PHASE_1_MAX_TIME 8

static short *Eew_img = NULL;
static int Eew_img_xn = 0, Eew_img_yn = 0;

typedef struct eew_struct {	/* for saving estimated EW */
    time_t time;		/* data time */
    float elev;			/* elevation in degrees */
    float st_range;		/* first EW grid point range in meters */
    float r_step;		/* range step size in meters */
    short n_rgs;		/* number of range steps */
    short n_azs;		/* number of azimuth steps */
    int vol_num;
    short *ews;			/* EW values */
    unsigned char *medinp;	/* median inp */
} Eew_t;

typedef struct {
    int ssxz, ssyz;		/* sub-sampled region size */
    int xr, yr;			/* sub-sample ratio */
    int n_ssgs;			/* # of valid gates after sub-sampling */
    short *ssdata;		/* data and sub-sampled data */
    unsigned char *ssdmap;	/* sub-sampled dmap */
    int full_ppi;		/* this subimage is a full ppi image */
    Region_t *rg;		/* the original region */
} Sub_img_t;	

static Eew_t *Eews = NULL;	/* array of estimate EWs */
static int N_eews = 0;		/* size of Eews */
static Params_t **Parm_array = NULL;	/* per-radial parameter array */
static Params_t Parms[MAX_N_PRF];	/* parameters for PRF sectors */

enum {SR_CLEAR, SR_READ, SR_INFO, SR_SAVE, SR_SET_LATEST,SR_RM_LATEST};

static void Save_eew (Vdeal_t *vdv);
static int Expand_sampled_region (Sub_img_t *sub_img, Vdeal_t *vdv,
						Params_t *parms);
static void Sub_sample_region (Sub_img_t *sub_img, Params_t *parms,
				Region_t *region, Vdeal_t *vdv);
static int Sub_sample (Region_t *region, Vdeal_t *vdv,
				int xr, int yr, short **out, int fp, int nyq);
static int Process_regions (Vdeal_t *vdv, int nrs, 
				int rg_limit, int *badcnt, int *bs_cnt);
static void Get_sample_ratio (Vdeal_t *vdv, Region_t *region,
				int *xr, int *yr);
static void Check_full_cut (Vdeal_t *vdv);
static void Preprocess_input (Vdeal_t *vdv, int pass);
static void Clean_out_buffers (Vdeal_t *vdv);
static int Brokenup_regions (int func, Region_t *region, 
				Sub_img_t *sub_img, Vdeal_t *vdv);
static void Reclump_region (Sub_img_t *sub_img, int save_bh, Vdeal_t *vdv,
					int min_ngs, Params_t *parms);
static int Remove_bad_gates (Vdeal_t *vdv, Region_t *region);
static void Flag_area (Vdeal_t *vdv, Region_t* region, int flag);
static int Is_extrap_vad (Vdeal_t *vdv, Region_t *region);
static void Fill_and_rm_thin (Vdeal_t *vdv, short *data, Params_t *parms,
					int xz, int yz, int fp);
static int Is_region_small (Vdeal_t *vdv, Region_t *region);
static int Check_deal_quality (Vdeal_t *vdv, Region_t *region, int n_gs,
		int step, int brr, int gd, int qerr, int miss_cnt,
		int *bs_cnt, int *badcnt, Params_t *parms);
static int Predict_nonuniform (Vdeal_t *vdv, int set);
static int Is_storm_top (Vdeal_t *vdv, Region_t *region);
static void Init_parameters (Vdeal_t *vdv);
static int Check_extreme (Vdeal_t *vdv, Region_t *region, int gd, int nyq);

/***************************************************************************

    Initializes the EW in "vdv" using the current globally estimated EW.
    Returns 0 on success or -1 on failure.

***************************************************************************/

int EE_get_eew (Vdeal_t *vdv) {
    int ind, i;

    ind = -1;
    for (i = 0; i < N_eews; i++) {
	if (Eews[i].time == 0)
	    continue;
	if (Eews[i].time == vdv->dtm) {
	    ind = i;
	    break;
	}
    }

    if (ind < 0) {		/* return empty BW */
	VDE_reset (vdv, 1);
	MISC_log ("Use empty BW - EEW failed\n");
    }
    else {
	Ew_struct_t *ew = &(vdv->ew);
	memcpy (ew->ews, Eews[ind].ews, 
			ew->n_azs * ew->n_rgs * sizeof (short));
    }

    VDE_generate_ewm (vdv);

    return (0);
}

/***************************************************************************

    Returns the ew value at range "r", in meters, and "azi", in degrees,
    of the saved previous ew of the nearest, lower elevation. Current cut
    or too old ew is not used.

***************************************************************************/

int EE_get_near_elev_ew (Vdeal_t *vdv, double r, double azi) {
    Eew_t *eew;
    double min;
    int mini, i, x, y;

    min = 100.;
    mini = 0;
    for (i = 0; i < N_eews; i++) {
	double df;
	if (Eews[i].time == 0 || vdv->dtm < Eews[i].time || 
					vdv->dtm > Eews[i].time + 450)
	    continue;
	df = vdv->elev - Eews[i].elev;
	if (df < .5)		/* must be lower than at least .5 */
	    continue;		/* Too close elev will not provide new info */
	if (df < min + .1) {
	    if (df < min)
		min = df;
	    if (mini == 0 || Eews[i].time > Eews[mini].time)
		mini = i;	/* choose the latest */
	}
    }

    if (min >= 3.)
	return (SNO_DATA);

    eew = Eews + mini;
    x = r / eew->r_step;
    y = azi * eew->n_azs / 360.;
    if (x < 0 || y < 0 || x >= eew->n_rgs || y >= eew->n_azs)
	return (SNO_DATA);
    return (eew->ews[y * eew->n_rgs + x]);
}

/***************************************************************************

    Returns the ew value at (x, y) of the saved previous ew.

***************************************************************************/

int EE_get_previous_ew (Vdeal_t *vdv, int x, int y) {
    Eew_t *eew;
    double min;
    int mini, i, xx, yy;

    min = 100.;
    mini = -1;
    for (i = 0; i < N_eews; i++) {
	double df;
	if (Eews[i].time == 0 || vdv->dtm < Eews[i].time || 
					vdv->dtm > Eews[i].time + 450)
	    continue;
	if (vdv->vol_num != Eews[i].vol_num + 1)
	    continue;
	df = Eews[i].elev - vdv->elev;
	if (df < .3)		/* elevation must be higher */
	    continue;
	if (df < min) {
	    min = df;
	    mini = i;
	}
    }

    if (mini < 0 || min > 2.)
	return (SNO_DATA);

    eew = Eews + mini;

    xx = x * vdv->ew.rz * vdv->g_size / eew->r_step;
    yy = y * vdv->ew.az * eew->n_azs / 360.;
    if (xx < 0 || yy < 0 || xx >= eew->n_rgs || yy >= eew->n_azs)
	return (SNO_DATA);
    if (eew->medinp != NULL) {	/* med inp must be close */
	unsigned char *mi = PP_get_med_v ();
	int pv = eew->medinp[yy * eew->n_rgs + xx];
	int v = mi[y * vdv->ew.n_rgs + x];
	if (v != BNO_DATA && pv != BNO_DATA) {
	    int df = pv - v;
	    int nyq = vdv->nyq;
	    while (df > vdv->nyq)
		df -= nyq * 2;
	    while (df < -nyq)
		df += nyq * 2;
	    if (df < 0) df = -df;
	    if (df > nyq / 3)
		return (SNO_DATA);
	}
    }

    return (eew->ews[yy * eew->n_rgs + xx]);
}

/***************************************************************************

    Returns the EEW image value at "x", "y".

***************************************************************************/

short EE_get_eew_value (int x, int y) {

    if (Eew_img == NULL)
	return (SNO_DATA);
    if (x >= Eew_img_xn || y >= Eew_img_yn)
	return (SNO_DATA);
    return (Eew_img[y * Eew_img_xn + x]);
}

/***************************************************************************

    Estimates the global EW using data in "vdv". Returns 0 on success or 
    -1 on failure.

***************************************************************************/

int EE_estimate_ew (Vdeal_t *vdv) {
    unsigned char *org_inp;
    int clump_flag, pass, max_pass, failed, org_yz, rg_limit;

    if (vdv->data_type & DT_SUB_IMAGE)
	return (-1);

    MISC_log (
	"Estim. BW (ele %g, st azi%4.0f), nyq %d\n", 
				vdv->elev, vdv->start_azi, vdv->nyq);
    vdv->phase = 1;

    if (Test_mode && vdv->realtime)
	VDV_write_history (vdv, 0);
    if (!vdv->realtime)
	VDV_read_history (vdv, 0);

    Check_full_cut (vdv);
    clump_flag = VDC_IDR_SIZE | VDC_PARM_ARRAY;
    if (vdv->full_ppi)
	clump_flag |= VDC_IDR_WRAP;
    Init_parameters (vdv);

    CD_remove_ground_clutter (vdv);
    PP_fill_in_gaps (vdv, NULL, vdv->xz, vdv->yz, 4, 
					vdv->full_ppi, 8, vdv->nyq);

    VDE_set_ew_flags (vdv);
    VDV_vad_analysis (vdv, 1);		/* set flags using existing vad */
    PP_detect_fronts (vdv);

    /* limit for near radar regions */
    rg_limit = VDV_alt_to_range (vdv, 500.) / vdv->g_size;

    Predict_nonuniform (vdv, 1);
    max_pass = 2;
    if (vdv->spw == NULL)	/* second pass will not help at this time */
	max_pass = 1;
    VDB_check_timeout (PHASE_1_MAX_TIME);
    failed = 0;
    org_inp = vdv->inp;
    org_yz = vdv->yz;
    for (pass = 0; pass < max_pass; pass++) {
	int stride, nregions, gcnt, gc, bcnt, bs_cnt;

	Clean_out_buffers (vdv);  /* required when EEW after cut processing */
	VDE_reset (vdv, 1);
        VDE_generate_ewm (vdv);	/* start with EW */

	Preprocess_input (vdv, pass);
	stride = vdv->xz;
	bcnt = bs_cnt = 0;	/* bad shear gates count */
	nregions = VDC_identify_regions (vdv->inp, NULL, stride, 0, 0,
			    vdv->xz, vdv->yz, Parm_array, clump_flag, NULL);
	gcnt = Process_regions (vdv, nregions, -rg_limit, &bcnt, &bs_cnt);
					/* process near radar regions */
	if (gcnt >= 0) {
	    VDC_reset_next_region (NULL);
	    gc = Process_regions (vdv, nregions, rg_limit, &bcnt, &bs_cnt);
				/* process all other regions - phase 2 */
	}

	if (gcnt < 0 || gc < 0) {
	    MISC_log ("EEW failed (time-out)\n");
	    VDR_status_log ("Insufficient CPU (Phase 1 Timed-out)\n");
	    failed = 1;
	    break;
	}

	gcnt += gc;
	if (Test_mode)
	    MISC_log ("    pass %d: succeeded %d, discarded %d, bs_cnt %d\n",
						pass, gcnt, bcnt, bs_cnt);
/*	if ((bs_cnt > 4 && bcnt * 4 >= gcnt) || bs_cnt >= 40) {  */
						/* consider as failed */
	if (0) {	/* one pass only */
	    if (pass < max_pass - 1) {
		vdv->yz = org_yz;
		vdv->inp = org_inp;
		MISC_log ("First pass failed (good %d, bad %d, bs_cnt %d)\n",
						gcnt, bcnt, bs_cnt);
		continue;
	    }
	    if (bcnt >= gcnt || bs_cnt > 200) {
		MISC_log ("EEW failed (good %d, bad %d)\n", gcnt, bcnt);
		failed = 1;
	    }
	}
	if (!failed) {
	    VDV_vad_analysis (vdv, -1);
	    if (pass == 0 && Predict_nonuniform (vdv, 0)) {
		MISC_log ("NONUNIFORM changed - Redo estimate_ew\n");
		VDV_vad_analysis (vdv, 0);
		pass--;			/* redo estimating ew */
		vdv->yz = org_yz;
		vdv->inp = org_inp;
		continue;
	    }
	    VDV_vad_analysis (vdv, 2);
	    if (!Test_mode && vdv->realtime)
		VDV_write_history (vdv, 1);
	}
	break;
    }
    VDC_free (NULL);
    VDB_check_timeout (0);

    if (Test_mode) {
	dump_simage ("ew_out", vdv->out, vdv->xz, vdv->yz, vdv->xz);
    } 

    if (failed)
	VDE_reset (vdv, 1);	/* remove the EW */

    Save_eew (vdv);

    {		/* cleanup/restore vdv - needed if EEW before cut processing */
	vdv->yz = org_yz;
	Clean_out_buffers (vdv);
	vdv->inp = org_inp;
	vdv->phase = 2;
	VDE_reset (vdv, 0);
	VDE_generate_ewm (vdv);	/* regenerate ewm for phase 2 */
    }
    if (Test_mode) {
	VDT_dump_ew ("EW", vdv, "ewm");
	VDE_print_ew_flags (vdv);
    }

    return (0);
}

/******************************************************************

    Removes any data in vdv->out and vdv->dmap.

******************************************************************/

static void Clean_out_buffers (Vdeal_t *vdv) {
    int i, size;
    short *sp;
    unsigned char *bp;

    size = vdv->xz * vdv->yz;
    sp = vdv->out;
    bp = vdv->dmap;
    for (i = 0; i < size; i++) {
	*sp++ = SNO_DATA;
	*bp &= (DMAP_HSPW | DMAP_NHSPW | DMAP_FILL);
	bp++;
    }
}

/***************************************************************************

    Dealiases "nrs" regions in the default VDC buffer. If "rg_limit" <
    0, only regions, that are <= -rg_limit, are processed. Otherwise,
    all regions, that are > rg_limit, are processed. Two processing
    steps: The first step skips all regions that cannot get good global
    dealasing. The second step redoes the missed regions. The regions
    that are not satisfactory in the second step are discarded. Returns
    the number of gates successfully processed. badcnt and bs_cnt are
    incremented for numbers of discarded and bad high shear gates
    respectively.

***************************************************************************/

static int Process_regions (Vdeal_t *vdv, int nrs, int rg_limit,
						int *badcnt, int *bs_cnt) {
    extern __thread char *VDC_region_buf;
    Region_t region;
    int xz, i, miss_cnt, miss_ind, step, min_ngs, succeeded;
    int miss_indexes[256];

    VDC_region_buf = STR_reset (VDC_region_buf, 
				vdv->xz * vdv->yz * sizeof (short));
    xz = vdv->xz;
    miss_cnt = miss_ind = 0;
    step = 0;		/* step 1 does all regions failed in step 0 */
    Brokenup_regions (SR_CLEAR, NULL, NULL, NULL);
    min_ngs = 150;	/* minimum number of gates of a region to be used */
    succeeded = 0;
    for (i = 0; i <= nrs; i++) {
	int n_gs, brn_gs, gd, fppi, qerr, brr, bgcnt, ret;
	Sub_img_t sub_img;
	Params_t parms;

	region.data = NULL;	/* for efficiency, don't read data here */
	brn_gs = Brokenup_regions (SR_INFO, &region, NULL, NULL);
	brr = 0;	/* a break-up regions */
	n_gs = 0;
	if (step == 0 || (miss_ind < miss_cnt && i == miss_indexes[miss_ind]))
	    n_gs = VDC_get_next_region (NULL, i, &region);

	region.data = (short *)VDC_region_buf;
	if (brn_gs >= min_ngs && n_gs <= brn_gs) {
	    n_gs = Brokenup_regions (SR_READ, &region, NULL, NULL);
	    brr = 1;
	    i--;
	}
	else if (step == 1) {
	    if (miss_ind >= miss_cnt)
		break;
	    if (n_gs > 0)
		miss_ind++;
	}

	if (n_gs < min_ngs) {
	    if (step == 0) {
		Brokenup_regions (SR_CLEAR, NULL, NULL, NULL);
		step = 1;
		i = -1;
		miss_ind = 0;
	    }
	    continue;
	}

	if (!brr) {
	    if (rg_limit < 0 && region.xs > -rg_limit)
		continue;
	    if (rg_limit >= 0 && region.xs <= rg_limit) 
		continue;
	    VDC_get_next_region (NULL, i, &region);	/* get region data */
	}
	if (Is_region_small (vdv, &region))
	    continue;


	parms = *Parm_array[region.ys % vdv->yz];
	fppi = 0;
	if (vdv->full_ppi && region.yz == vdv->yz)
	    fppi = 1;
	Sub_sample_region (&sub_img, &parms, &region, vdv);
	Fill_and_rm_thin (vdv, sub_img.ssdata, &parms,
			sub_img.ssxz, sub_img.ssyz, sub_img.full_ppi);
	Reclump_region (&sub_img, 0, vdv, min_ngs, &parms);

	parms.stride = sub_img.ssxz;
	parms.dmap = sub_img.ssdmap;
	parms.fppi = fppi;
	parms.xs = region.xs;
	parms.ys = region.ys;
	parms.xr = sub_img.xr;
	parms.yr = sub_img.yr;
	if (vdv->data_type & DT_NONUNIFORM || 
			((vdv->data_type & DT_VH_VS) && vdv->elev > 2.5f))
	    parms.use_bc = 0;

	if (VD2D_2d_dealiase (sub_img.ssdata, &parms, 
		sub_img.n_ssgs, sub_img.ssxz, sub_img.ssyz, NULL) < 0) {
	    MISC_log ("Ew estimate: 2D timed-out\n");
	    *badcnt += n_gs;
	    succeeded = -1;
	    break;
	}
	Brokenup_regions (SR_SET_LATEST, NULL, NULL, NULL);
	Reclump_region (&sub_img, 1, vdv, min_ngs, &parms);
				/* save BH gates for later processing */
	n_gs = Expand_sampled_region (&sub_img, vdv, &parms);
				/* n_gs may change after subsample/expand */

	gd = VDE_global_dealiase (vdv, &region, vdv->dmap, vdv->xz, &qerr);

	ret = Check_deal_quality (vdv, &region, n_gs, step, brr, gd, qerr,
				miss_cnt, bs_cnt, badcnt, &parms);
	if (ret) {
	    if (ret == 2) {	/* region to be processed later */
		miss_indexes[miss_cnt] = i;
		miss_cnt++;
		Brokenup_regions (SR_RM_LATEST, NULL, NULL, NULL);
	    }
	    continue;
	}

	VDE_check_global_deal (vdv, &region, gd);
	bgcnt = Remove_bad_gates (vdv, &region);
	*badcnt += bgcnt;

	VDD_apply_gd_copy_to_out (vdv, &region, gd);
	VDE_update_ew (vdv, &region);
	succeeded += n_gs - bgcnt;
    }
    return (succeeded);
}

/***************************************************************************

    Checks dealiasing quality of "region". Returns 1 if the region is
    discared, 2 if the region is to be processed later or 0 if it is OK.

***************************************************************************/

static int Check_deal_quality (Vdeal_t *vdv, Region_t *region, int n_gs,
		int step, int brr, int gd, int qerr, int miss_cnt,
		int *bs_cnt, int *badcnt, Params_t *parms) {
    int small_rgz, l_qerr, m_qerr, l_of_thr, v_qerr, m_of_thr, hs_thr;
    int boutfit, is_st, extr, nyq;
    char mbuf[128];

    nyq = parms->nyq;
    l_of_thr = nyq * 3 / 2;  /* threshold for large output fit error */
    m_of_thr = nyq;	/* threshold for medium output fit error */
    hs_thr = nyq * 3 / 2;	/* threshold for high shear check */
    if (vdv->low_prf) {
	v_qerr = nyq;
	l_qerr = .9 * nyq + .5;
	m_qerr = .6 * nyq + .5;
    }
    else {
	v_qerr = nyq * .9 + .5;	/* vary large qerr limit */
	l_qerr = .6 * nyq + .5;	/* large qerr limit */
	m_qerr = .5 * nyq + .5;	/* medium qerr limit */
    }
    small_rgz = 1. * vdv->g_size / vdv->gate_width;	/* small region */

    if ((is_st = Is_storm_top (vdv, region)))
	l_qerr = .8 * nyq + .5;
    else if (!(vdv->data_type & DT_NONUNIFORM) && 
					Is_extrap_vad (vdv, region)) {
	m_qerr = l_qerr;
	l_qerr = v_qerr;
    }

    if (step == 0 && !brr) {
	boutfit = VDA_check_fit_out (vdv, region, gd, m_of_thr, NULL);
	if ((qerr > m_qerr || boutfit > 8) &&
		    	miss_cnt < 255) {	/* try this region later */
	    return (2);
	}
    }

    boutfit = VDA_check_fit_out (vdv, region, gd, l_of_thr, NULL);
    extr = 0;
    if (!vdv->low_prf)
	extr = Check_extreme (vdv, region, gd, nyq);

    sprintf (mbuf, "%d gates: %d %d %d %d", n_gs, region->xs, region->ys, 
						region->xz, region->yz);
    if (boutfit > 8 || extr || qerr >= l_qerr || 
				(n_gs <= small_rgz && qerr > m_qerr)) {
	char b[256];

	int rflag = vdv->ew.rfs[(region->xs + region->xz / 2) / vdv->ew.rz];
	if (boutfit > 8)
	    Flag_area (vdv, region, EF_HIGH_SHEAR);
	else if (is_st)
	    Flag_area (vdv, region, EF_LOW_ELE_EW);

	sprintf (b, "brr %d qerr %d ", brr, qerr);
	if (extr) sprintf (b + strlen (b), "extr %d ", extr);
	if (boutfit > 0) sprintf (b + strlen (b), "boutfit %d ", boutfit);

	if (((rflag & RF_HIGH_VS) && (vdv->data_type & DT_VH_VS)) ||
	    VDA_check_border_conn (vdv, region, 0, parms->fppi) < 60) {
	    if (Test_mode)
		MISC_log ("Region discarded - %s (%s)\n", b, mbuf);
	    *bs_cnt += boutfit;
	    *badcnt += n_gs;
	    return (1);
	}
	else {
	    if (Test_mode)
		MISC_log ("Region unused - %s (%s)\n", b, mbuf);
	    return (1);
	}
    }

    if ((vdv->data_type & DT_VH_VS) && n_gs > small_rgz) {
	int hsll, hsw;
	hsll = VDA_detect_false_shear (vdv, region, hs_thr, &hsw);
	*bs_cnt += hsll;
	if (hsll > 30) {
	    *badcnt += n_gs;
	    if (Test_mode)
		MISC_log ("Region discarded (hsll %d) (%s)\n", hsll, mbuf);
	    return (1);
	}
    }

    return (0);
}

/********************************************************************

    Applies the global dealiasing value "gd" to "region" and checks
    against thr. Extreme date are discarded. Returns 1 if there is
    significant number of high extream data or 0 otherwise.

********************************************************************/

static int Check_extreme (Vdeal_t *vdv, Region_t *region, int gd, int nyq) {
    int x, y, thrh, thrl, cnt, tcnt;

    thrh = vdv->data_off + nyq * 2;
    thrl = vdv->data_off - nyq * 2;
    cnt = tcnt = 0;
    for (y = 0; y < region->yz; y++) {
	short *dt = region->data + y * region->xz;
	for (x = 0; x < region->xz; x++) {
	    if (dt[x] != SNO_DATA) {
		int v = dt[x] + gd;
		tcnt++;
		if (v > thrh || v < thrl) {
		    int ew = VDE_get_ew_value (vdv, x + region->xs,
							y + region->ys);
		    if (ew != SNO_DATA && (v - ew < -nyq || v - ew > nyq)) {
			dt[x] = SNO_DATA;
			cnt++;
		    }
		}
	    }
	}
    }
    if (cnt * 8 > tcnt)
	return (1);
    return (0);
}

/***************************************************************************

    Returns if the region is fully in storm top (above 7000 meters).

***************************************************************************/

static int Is_storm_top (Vdeal_t *vdv, Region_t *region) {
    double r;

    r = region->xs * vdv->g_size;
    if (r < VDV_alt_to_range (vdv, 7000.))
	return (0);
    return (1);
}

/***************************************************************************

    Returns true if region is small for phase 1 processing.

***************************************************************************/

static int Is_region_small (Vdeal_t *vdv, Region_t *region) {
    int xmin, ymin;

    xmin = 1500 / vdv->g_size + 2;
    if (region->xz < xmin)
	return (1);
    ymin = Myround (2. / vdv->gate_width) + 2;
    if (vdv->data_type & DT_NONUNIFORM)
	ymin *= 3;
    if (region->yz < ymin)
	return (1);
    return (0);
}

/****************************************************************************

    Fills in gaps and breaks thin connections in "data".

****************************************************************************/

static void Fill_and_rm_thin (Vdeal_t *vdv, short *data, Params_t *parms,
						int xz, int yz, int fp) {
    static void *rgs = NULL;
    static unsigned char *buf = NULL;
    int clump_flag, xyz, x, y, nrgs, n_gs;
    unsigned char *bds, *bdsc;
    Region_t region;

    xyz = xz * yz;
    buf = (unsigned char *)STR_reset (buf, xyz * 3 * sizeof (unsigned char));
    for (x = 0; x < xyz; x++) {
	if (data[x] == SNO_DATA)
	    buf[x] = BNO_DATA;
	else
	    buf[x] = data[x];
    }

    clump_flag = VDC_IDR_SIZE;
    if (fp)
	clump_flag |= VDC_IDR_WRAP;
    nrgs = VDC_identify_regions (buf, NULL, xz, 0, 0,
			    xz, yz, parms, clump_flag, &rgs);
    if (nrgs <= 0)
	return;

    bds = buf + xyz * sizeof (unsigned char);
    bdsc = bds + xyz * sizeof (unsigned char);
    region.data = (short *)bds;
    while (1) {
	n_gs = VDC_get_next_region (rgs, VDC_BIN | VDC_NEXT, &region);
	if (n_gs <= 4)
	    break;
	memcpy (bdsc, bds, region.xz * region.yz * sizeof (unsigned char));
	VDA_find_thin_conn (bds, region.xz, region.yz, 1, fp, NULL,
					parms->nyq, vdv->data_off, NULL, 0);
    
	for (y = 0; y < region.yz; y++) {
	    unsigned char *s, *r;
	    short *d = data + (((y + region.ys) % yz) * xz + region.xs);
	    r = bdsc + y * region.xz;
	    s = bds + y * region.xz;
	    for (x = 0; x < region.xz; x++) {
		if (r[x] == BNO_DATA)
		    continue;
		if (s[x] == BNO_DATA)
		    d[x] = SNO_DATA;	/* cp break */
		else
		    d[x] = s[x];	/* cp filled data */
	    }
	}
    }
}

/**************************************************************************

    Returns true if region is in the extrapolated VAD range, or false 
    otherwise.

**************************************************************************/

static int Is_extrap_vad (Vdeal_t *vdv, Region_t *region) {
    Ew_struct_t *ew;
    int rz, xs, x;
    unsigned short *rfs;

    ew = &(vdv->ew);
    rz = ew->rz;
    rfs = ew->rfs;
    xs = (region->xs + rz / 2) / rz;
    for (x = xs - 1; x >= 0; x--) {
	if (rfs[x] & RF_2SIDE_VAD)
	    break;
    }
    if (x < xs)
	return (1);
    return (0);
}

/***************************************************************************

    Record failed area of region "region".

***************************************************************************/

static void Flag_area (Vdeal_t *vdv, Region_t* region, int flag) {

    Ew_struct_t *ew;
    int stx, sty, ey, x, y;

    ew = &(vdv->ew);
    stx = region->xs / ew->rz;
    sty = vdv->ew_aind[region->ys % vdv->yz];
    ey = vdv->ew_aind[(region->ys + region->yz) % vdv->yz];
    if (ey < sty)
	ey += ew->n_azs;
    for (x = stx; x < ew->n_rgs; x++) {

	if (x * ew->rz > region->xs + region->xz)
	    break;
	for (y = sty; y <= ey; y++)
	    ew->efs[(y % ew->n_azs) * ew->n_rgs + x] |= flag;
    }
}

/***************************************************************************

    Removes BH | BE marked data from region. Returns the number of gates
    removed.

***************************************************************************/

static int Remove_bad_gates (Vdeal_t *vdv, Region_t *region) {
    int y, x, cnt;

    cnt = 0;
    for (y = 0; y < region->yz; y++) {
	short *dt;
	unsigned char *map = vdv->dmap + 
			(((y + region->ys) % vdv->yz) * vdv->xz + region->xs);
	dt = region->data + y * region->xz;
	for (x = 0; x < region->xz; x++) {
	    if ((map[x] & (DMAP_BH | DMAP_BE)) && dt[x] != SNO_DATA) {
		dt[x] = SNO_DATA;
		cnt++;
	    }
	}
    }
    return (cnt);
}

/***************************************************************************

    Sub-sample region "region". "dmap" buffer is also prepared. More
    sophistcated non-uniform subsampling method can be added later by 
    adding necessary fields in Sub_img_t for storing info for later region
    expansion.

***************************************************************************/

static void Sub_sample_region (Sub_img_t *sub_img, Params_t *parms,
					Region_t *region, Vdeal_t *vdv) {
    static char *dmap_buf = NULL;
    int xr, yr, nb;

    sub_img->rg = region;
    if (region->yz == vdv->yz && vdv->full_ppi)
	sub_img->full_ppi = 1;
    else
	sub_img->full_ppi = 0;

    Get_sample_ratio (vdv, region, &xr, &yr);
    sub_img->xr = xr;
    sub_img->yr = yr;
    parms->r_w_ratio *= yr;
    parms->r0 = region->xs / xr;
    parms->g_size *= xr;

    sub_img->n_ssgs = Sub_sample (region, vdv, xr, yr,
			&(sub_img->ssdata), sub_img->full_ppi, parms->nyq);
    sub_img->ssxz = region->xz / xr;
    sub_img->ssyz = region->yz / yr;
    nb = sub_img->ssxz * sub_img->ssyz * sizeof (char);
    dmap_buf = STR_reset (dmap_buf, nb);
    memset (dmap_buf, 0, nb);
    sub_img->ssdmap = (unsigned char *)dmap_buf;
}

/***************************************************************************

    Determines and returns the required subsampling ratio.

***************************************************************************/

static void Get_sample_ratio (Vdeal_t *vdv, Region_t *region, 
						int *xrp, int *yrp) {
    int xr, yr, max_n;

    max_n = 20000;	/* max number of subsampled gates */
    xr = yr = 1;
    while (1) {
	if (region->n_gs / (xr * yr) <= max_n)
	    break;
	if (region->yz / yr > region->xz / xr) {
	    yr += 1;
	    if (region->n_gs / (xr * yr) <= max_n)
		break;
	    xr += 1;
	}
	else {
	    xr += 1;
	    if (region->n_gs / (xr * yr) <= max_n)
		break;
	    yr += 1;
	}
    }
    if (vdv->low_prf) {		/* less subsampling because variance across */
	*xrp = xr;		/* gates is high relative to Nyq */
	*yrp = yr;
	if (vdv->gate_width < .8f) {	/* super res */
	    *xrp = xr * 2;	/* save cpu */
	    *yrp = yr * 2;
	}
    }
    else {
	*xrp = xr * 2;	/* save cpu without significant performance impact */
	*yrp = yr * 2;
	if (*xrp < 3) *xrp = 3;	/* to smooth the data */
	if (*yrp < 3) *yrp = 3;
    }
}

/***************************************************************************

    Expands the sub-sampled region of "sub_img" and put it in the original
    region and dmap.

***************************************************************************/

static int Expand_sampled_region (Sub_img_t *sub_img, Vdeal_t *vdv,
						Params_t *parms) {
    int xr, yr, y, xz, ssxz, cnt;
    Region_t *rg;

    xr = sub_img->xr;
    yr = sub_img->yr;
    parms->r_w_ratio /= yr;
    parms->r0 *= xr;
    parms->g_size /= xr;
    rg = sub_img->rg;
    xz = rg->xz;
    ssxz = sub_img->ssxz;
    cnt = 0;
    for (y = 0; y < rg->yz; y++) {
	int sy, sx, x, off;
	short *pd, *psd, d;
	unsigned char *pm, *psm, m;

	sy = y / yr;
	if (sy >= sub_img->ssyz)
	    sy = sub_img->ssyz - 1;
	pd = rg->data + (y * xz);
	psd = sub_img->ssdata + (sy * ssxz);
	off = ((y + rg->ys) % vdv->yz) * vdv->xz + rg->xs;
	pm = vdv->dmap + off;
	psm = sub_img->ssdmap + (sy * sub_img->ssxz);
	sx = -1;
	m = d = 0;		/* to eliminate compiler warning */
	for (x = 0; x < xz; x++) {
	    if ((x % xr) == 0) {
		sx++;
		if (sx >= ssxz)
		    sx = ssxz - 1;
		d = psd[sx];
		m = psm[sx];
	    }
	    if (*pd != SNO_DATA) {
		*pd = d;
		if (d != SNO_DATA) {
		    cnt++;
		    pm[x] |= m;
		}
	    }
	    pd++;
	}
    }
    rg->n_gs = cnt;
    return (cnt);
}

/**************************************************************************

    Saves the newly estimated EW in table Eews.

***************************************************************************/

static void Save_eew (Vdeal_t *vdv) {
    int ind, size;
    Eew_t *eew;
    Ew_struct_t *ew;

    /* save the EEW image */
    ew = &(vdv->ew);
    Eew_img_xn = Eew_img_yn = 0;
    Eew_img = (short *)STR_reset ((char *)Eew_img, 
				vdv->xz * vdv->yz * sizeof (short));
    memcpy (Eew_img, vdv->out, vdv->xz * vdv->yz * sizeof (short));
    Eew_img_xn = vdv->xz;
    Eew_img_yn = vdv->yz;

    /* save the EEW */
    for (ind = 0; ind < N_eews; ind++) {
	if (Eews[ind].time != 0 && (vdv->dtm < Eews[ind].time ||
	    			vdv->vol_num > Eews[ind].vol_num + 1)) {
	    Eews[ind].time = 0;	/* delete expired record */
	}
    }
    for (ind = 0; ind < N_eews; ind++) {	/* where to put? */
	if (Eews[ind].time == 0)
	    break;
    }
    if (ind >= N_eews) {	/* extend array */
	Eew_t eew;
	int i;
  	memset ((char *)&eew, 0, sizeof (Eew_t));
	if (N_eews == 0) {
	    Eews = (Eew_t *)STR_reset ((char *)Eews, 30 * sizeof (Eew_t));
	}
	N_eews++;
	for (i = ind; i < N_eews; i++)
	    Eews = (Eew_t *)STR_append ((char *)Eews, 
					(char *)&eew, sizeof (Eew_t));
    }

    eew = Eews + ind;
    eew->time = vdv->dtm;
    eew->vol_num = vdv->vol_num;
    eew->elev = vdv->elev;
    eew->r_step = vdv->g_size * ew->rz;
    eew->st_range = .5f * eew->r_step;
    eew->n_rgs = ew->n_rgs;
    eew->n_azs = ew->n_azs;
    size = ew->n_rgs * ew->n_azs;
    if (eew->ews != NULL)
	free (eew->ews);
    eew->ews = (short *)MISC_malloc (size * (sizeof (short) + 
						sizeof (unsigned char)));
    memcpy (eew->ews, ew->ews, size * sizeof (short));
    eew->medinp = (unsigned char *)(eew->ews + size);
    memcpy (eew->medinp, PP_get_med_v (), size * sizeof (unsigned char));

    return;    
}

/**************************************************************************

    Generates sub-sample of the region image "region". The output image is 
    returned with "out". The sizes of the the output image are xz / xr and 
    yz / yr. Note that the output image's origin is offset by half of the
    window sizes. Returns the number of non-NO_DATA gates in the sub-sampled
    region. vdv->spw is used in subsampling.

***************************************************************************/

static int Sub_sample (Region_t *region, Vdeal_t *vdv,
			int xr, int yr, short **out, int fp, int nyq) {
    static short *img = NULL;
    int xz, yz, xs, ys, wxz, wyz, xn, yn, y, x, sscnt, spw_ok, min_cnt;
    unsigned char *spw;

    xz = region->xz;
    yz = region->yz;
    xn = xz / xr;
    yn = yz / yr;
    img = (short *)STR_reset ((char *)img, xn * yn * sizeof (short));
    *out = img;
    if (xr == 1 && yr == 1) {
	memcpy (img, region->data, xn * yn * sizeof (short));
	return (region->n_gs);
    }

    xs = region->xs;
    ys = region->ys;
    wxz = vdv->xz;
    wyz = vdv->yz;
    spw = vdv->spw;
    spw_ok = 1;
    min_cnt = xr * yr / 4;
    if (min_cnt < 1 || vdv->low_prf)
	min_cnt = 1;
    if (spw == NULL)
	spw_ok = 0;
    sscnt = 0;
    for (y = 0; y < yn; y++) {
	short *p, *pc, *imgp;
	int yy;

	p = region->data + y * yr * xz;
	imgp = img + y * xn;
	yy = y * yr + ys;
	for (x = 0; x < xn; x++) {
	    int d[512], cnt, hwcnt, hsrcnt, i, j, maxd[3];
	    unsigned char *spwx;

	    spwx = spw + x * xr + xs;
	    cnt = hwcnt = hsrcnt = 0;
	    pc = p;
	    for (i = 0; i < yr; i++) {
		unsigned char *w = spwx + ((yy + i) % wyz) * wxz;
		for (j = 0; j < xr; j++) {
		    int v = pc[j];
		    if (v != SNO_DATA) {
			d[cnt] = v;
			cnt++;
		    }

		    if (spw_ok) {
			if (w[j] & SPW_MASK_hsr)
			    hsrcnt++;
			if (w[j] & SPW_MASK_hw)
			    hwcnt++;
		    }
		}
		pc += xz;
	    }

	    if (cnt < min_cnt || cnt < hsrcnt * 2 || cnt < hwcnt)
		*imgp = SNO_DATA;
	    else {
		*imgp = VDA_search_median_value (d, cnt, 
					nyq, vdv->data_off, maxd);
		if (!vdv->low_prf && maxd[2] > nyq) 
		    *imgp = SNO_DATA;
		else
		    sscnt++;
	    }
	    imgp++;
	    p += xr;
	}
    }

    return (sscnt);
}

/**************************************************************************

    Checks if the data is of full 360 degrees. Discard extra radials and
    sets "vdv->full_ppi" and "Saved_yz". Full_ppi cannot be set if vdv->yz is 
    less than the max vertical window size used in all processing routines.

***************************************************************************/

static void Check_full_cut (Vdeal_t *vdv) {
    unsigned short *azi, a0;
    int i;

    azi = vdv->ew_azi;
    a0 = azi[0];
    vdv->full_ppi = 0;
    for (i = 100; i < vdv->yz; i++) {
	int diff = azi[i] - a0;
	if (diff < 0)
	    diff += 3600;
	if (diff >= 3585)
	    vdv->full_ppi = 1;
	if (diff >= 3600) {
	    if (vdv->yz != i + 1)
		MISC_log ("Extra radials discarded for 360 degree EEW\n");
	    vdv->yz = i + 1;
	    break;
	}
    }
    if (Test_mode && vdv->full_ppi)
	MISC_log ("The data for EEW is of full 360 degrees\n");
    if (vdv->n_secs > 1 && vdv->secs[vdv->n_secs - 1].nyq != vdv->secs[0].nyq)
	vdv->full_ppi = 0;
}

/****************************************************************************

    Sets DT_NONUNIFORM, if set is true, to the value of the same cut of
    the previous volume to reduce the possibility of reprocessing. If
    set is false, saves DT_NONUNIFORM for the cut and returns either
    true or false depending on whether a reprocessing pass is required
    (the nonuniform flag changes).

****************************************************************************/

#define MAXN_CUTS 20
static int Predict_nonuniform (Vdeal_t *vdv, int set) {
    static int last_nu[MAXN_CUTS], guess = 0; 
    int cut_num;

    cut_num = vdv->cut_num;
    if (set) {
	guess = 0;
	if (vdv->nonuniform_vol == 0)
	    memset (last_nu, 0, MAXN_CUTS * sizeof (int));
	if (cut_num >= 0 && cut_num < MAXN_CUTS && last_nu[cut_num]) {
	    guess = 1;
	    vdv->data_type |= DT_NONUNIFORM;
	}
	return (0);
    }
    else {
	if (cut_num >= 0 && cut_num < MAXN_CUTS && 
					vdv->data_type & DT_NONUNIFORM)
	    last_nu[cut_num] = 1;
	else
	    last_nu[cut_num] = 0;
	
	if ((guess == 0 && (vdv->data_type & DT_NONUNIFORM)) ||
	    (guess == 1 && !(vdv->data_type & DT_NONUNIFORM))) {
	    guess = -1;		/* true can only return once */
	    return (1);
	}
	return (0);
    }
}

/**************************************************************************

    Initializes Parm_array and Parms;

**************************************************************************/

static void Init_parameters (Vdeal_t *vdv) {
    int i, y;

    Parm_array = (Params_t **)STR_reset (Parm_array,
					vdv->yz * sizeof (Params_t *));
    for (i = 0; i < vdv->n_secs; i++)
	VDD_set_parameters (vdv, Parms + i, vdv->secs[i].nyq);
    for (y = 0; y < vdv->yz; y++) {
	for (i = 0; i < vdv->n_secs; i++) {
	    Prf_sec_t *s = vdv->secs + i;
	    if (y < s->azi + s->size) {
		Parm_array[y] = Parms + i;
		break;
	    }
	}
    }
}

/***************************************************************************

    Preprocesses the input image for estimating the EW. vdv->input is 
    replaced by the preprocessed input.

***************************************************************************/

static void Preprocess_input (Vdeal_t *vdv, int pass) {
    static char *buf = NULL;
    int xz, yz, i;
    unsigned char *pp_inp;

    xz = vdv->xz;
    yz = vdv->yz;
    buf = STR_reset (buf, xz * yz * sizeof (unsigned char));
    pp_inp = (unsigned char *)buf;
    memcpy (pp_inp, vdv->inp, xz * yz * sizeof (unsigned char));

    if (pass == 1) {	/* remove gap fill */
	unsigned char *dmap = vdv->dmap;
	for (i = 0; i < xz * yz; i++) {
	    if (dmap[i] & DMAP_FILL) {
		pp_inp[i] = BNO_DATA;
		dmap[i] &= ~DMAP_FILL;
	    }
	}
    }

    CD_spw_filter (vdv, pass, pp_inp);

    VDA_find_thin_conn (pp_inp, xz, yz, 2, vdv->full_ppi, NULL,
				vdv->nyq, vdv->data_off, NULL, 0);

    /* put separation lines between PRF sectors */
    for (i = 0; i < vdv->n_secs - 1; i++) {
	int x;
	Prf_sec_t *s = vdv->secs + i;
	unsigned char *in = pp_inp + (s->azi + s->size - 1) * xz;
	for (x = 0; x < xz; x++)
	    in[x] = BNO_DATA;
    }

    if (Test_mode)
	dump_bimage ("preprocessed", pp_inp, xz, yz, xz);

    vdv->inp = pp_inp;
}

/************************************************************************

    Two functions are performed here. 1. save_bh == 0: Re-clumps the
    region of sub_img and saves broken-up regions (small ones, <
    min_ngs, are discarded). 2. save_bh == 1: All gates in sub_img
    marked as BH are reclumped and saved (small ones discarded). BH is
    cleared for those gates.

    Saved or discarded gates are removed from sub_img. org_rg_data is
    the region data before subsampling. org_rg_data is not modified.

*************************************************************************/

static void Reclump_region (Sub_img_t *sub_img, int save_bh, Vdeal_t *vdv, 
					int min_ngs, Params_t *parms) {
    static void *rgs = NULL;
    static char *buf = NULL, *d_buf = NULL;
    int y, x, ssxz, ssyz, n_regions, cnt, clump_flag, st, bh_cnt;
    Region_t region;
    unsigned char *img;
    int i;

    /* copy region data to img (temporary buffer) */
    ssxz = sub_img->ssxz;
    ssyz = sub_img->ssyz;
    buf = STR_reset (buf, ssxz * ssyz);
    img = (unsigned char *)buf;		/* supress gcc warning */
    bh_cnt = 0;
    for (y = 0; y < ssyz; y++) {
	unsigned char *dm;
	short *sd = sub_img->ssdata + (y * ssxz);
	img = (unsigned char *)buf + (y * ssxz);
	dm = sub_img->ssdmap + (y * sub_img->ssxz);
	for (x = 0; x < ssxz; x++) {
	    if (sd[x] == SNO_DATA)
		img[x] = BNO_DATA;
	    else {
		if (save_bh)
		    img[x] = 100;  /* a valid data value - sd can overflow */
		else
		    img[x] = sd[x];
	    }
	    if (save_bh) {
		if (!(dm[x] & DMAP_BH))
		    img[x] = BNO_DATA;
		else {
		    dm[x] &= ~DMAP_BH;
		    bh_cnt++;
		}
	    }
	}
    }
    img = (unsigned char *)buf;

    clump_flag = VDC_IDR_SIZE;
    if (sub_img->full_ppi && !save_bh)
	clump_flag |= VDC_IDR_WRAP;
    n_regions = VDC_identify_regions (img, NULL, ssxz, 0, 0,
				ssxz, ssyz, parms, clump_flag, &rgs);

    min_ngs = min_ngs / (sub_img->xr * sub_img->yr);
    cnt = 0;
    st = 1;		/* check all but the first */
    if (save_bh)
	st = 0;		/* check all regions */
    for (i = st; i < n_regions; i++) {
	int x, y, n_gs;

	region.data = NULL;
	n_gs = VDC_get_next_region (rgs, i, &region);
	if (n_gs <= 0)
	    break;
	d_buf = STR_reset (d_buf, region.xz * region.yz * sizeof (short));
	region.data = (short *)d_buf;
	n_gs = VDC_get_next_region (rgs, i, &region);
	if (n_gs >= min_ngs && (!save_bh || (n_gs >= 10 * min_ngs &&
		n_gs * 10 < sub_img->n_ssgs * 9 /* to avoid infinite loop */)))
	    Brokenup_regions (SR_SAVE, &region, sub_img, vdv);
	cnt += n_gs;
	for (y = 0; y < region.yz; y++) {
	    short *d = region.data + y * region.xz;
	    short *ssd = sub_img->ssdata + 
			((region.ys + y) % ssyz) * ssxz + region.xs;
	    for (x = 0; x < region.xz; x++) {
		if (d[x] != SNO_DATA)
		    ssd[x] = SNO_DATA;
	    }
	}
    }
    sub_img->n_ssgs -= cnt;
    return;
}

/********************************************************************

    Saves and retrieves regions broken up by subsampling and median
    filtering. The regions saved may not be a connected region.

*********************************************************************/

#define MAX_REGIONS_SAVED 64

static int Brokenup_regions (int func, Region_t *region, 
				Sub_img_t *sub_img, Vdeal_t *vdv) {
    typedef struct {
	unsigned char *data;	/* the regions data. The stride is xz */
	int xs, ys, xz, yz;	/* location and size */
	int n_gs;		/* number of valid gates */
    } saved_regs_t;
    static saved_regs_t saved_regs[MAX_REGIONS_SAVED];
    static unsigned char latest_saved[MAX_REGIONS_SAVED];
    static int n_regs = 0, n_latest_saved = 0;
    int i;

    if (func == SR_RM_LATEST) {
	for (i = 0; i < n_latest_saved; i++)
	    saved_regs[latest_saved[i]].n_gs = 0;
	n_latest_saved = 0;
	return (0);
    }

    if (func == SR_SET_LATEST) {
	n_latest_saved = 0;
	return (0);
    }

    if (func == SR_CLEAR) {
	for (i = 0; i < n_regs; i++)
	    saved_regs[i].n_gs = 0;
	return (0);
    }

    if (func == SR_READ || func == SR_INFO) {
	saved_regs_t *sr;
	int max, ind, x, y;
	max = 0;
	ind = -1;
	for (i = 0; i < n_regs; i++) {
	    sr = saved_regs + i;
	    if (sr->n_gs > max) {
		max = sr->n_gs;
		ind = i;
	    }
	}
	if (ind < 0)
	    return (-1);
	sr = saved_regs + ind;
	region->xs = sr->xs;
	region->ys = sr->ys;
	region->xz = sr->xz;
	region->yz = sr->yz;
	region->n_gs = sr->n_gs;
	if (func == SR_READ && region->data != NULL) {
	    for (y = 0; y < sr->yz; y++) {
		unsigned char *src;
		short *dest;
		src = sr->data + y * sr->xz;
		dest = region->data + y * sr->xz;
		for (x = 0; x < sr->xz; x++) {
		    if (src[x] == BNO_DATA)
			dest[x] = SNO_DATA;
		    else
			dest[x] = src[x];
		}
	    }
	}
	if (func == SR_READ)
	    sr->n_gs = 0;
	return (region->n_gs);
    }

    if (func == SR_SAVE) {
	saved_regs_t *sr;
	int min, ind, xr, yr, cnt, y;
	Region_t *rg = sub_img->rg;

	min = 1000000;		/* a sufficiently large number */
	ind = 0;
	for (i = 0; i < n_regs; i++) {	/* find the one of min gates */
	    sr = saved_regs + i;
	    if (sr->n_gs < min) {
		min = sr->n_gs;
		ind = i;
	    }
	}
	if (n_regs < MAX_REGIONS_SAVED && min > 0) {
	    ind = n_regs;
	    memset (saved_regs + ind, 0, sizeof (saved_regs_t));
	    n_regs++;
	}

	xr = sub_img->xr;
	yr = sub_img->yr;
	sr = saved_regs + ind;
	sr->xs = rg->xs + region->xs * xr;
	sr->ys = rg->ys + region->ys * yr;
	sr->xz = region->xz * xr;
	sr->yz = region->yz * yr;

	if (sr->data != NULL)
	    free (sr->data);
	sr->data = MISC_malloc (region->xz * region->yz * xr * yr);
	cnt = 0;
	for (y = 0; y < region->yz; y++) {
	    short *rd = region->data + y * region->xz;
	    for (i = 0; i < yr; i++) {
		int yy, yyi, x;
		unsigned char *out, *inp;
		short *rin;

		yy = (region->ys + y) * yr + i;
		rin = rg->data + (yy % rg->yz) * rg->xz;
		yyi = (yy + rg->ys) % vdv->yz;
		inp = vdv->inp + yyi * vdv->xz + rg->xs;
		out = sr->data + (y * yr + i) * sr->xz;
		for (x = 0; x < region->xz; x++) {
		    int k;
		    for (k = 0; k < xr; k++) {
			int xx = (region->xs + x) * xr + k;
			if (rin[xx] != SNO_DATA && rd[x] != SNO_DATA && 
						inp[xx] != BNO_DATA) {
			    out[x * xr + k] = inp[xx];
			    cnt++;
			}
			else
			    out[x * xr + k] = BNO_DATA;
		    }
		}
	    }
	}
	sr->n_gs = cnt;
	if (n_latest_saved < MAX_REGIONS_SAVED) {
	    latest_saved[n_latest_saved] = ind;
	    n_latest_saved++;
	}
	return (0);
    }
    return (0);
}

int EE_read_data (Vdeal_t *vdv, FILE *fl, char *fname, int new_ver, int ops) {
    int yz;

    if (!ops) {
	if (fread (&(vdv->start_azi), sizeof (float), 1, fl) != 1 ||
	    fread (&yz, sizeof (int), 1, fl) != 1 ||
	    yz != vdv->yz ||
	    fread (vdv->ew_aind, sizeof (char), yz, fl) != yz ||
	    fread (vdv->ew_azi, sizeof (short), yz, fl) != yz) {
	    if (yz != vdv->yz)
		MISC_log ("Read VDV data bad yz from %s\n", fname);
	    else
		MISC_log ("Read VDV data failed from %s\n", fname);
	    exit (1);
	}
    }

    N_eews = 0;
    if (new_ver) {
	int n_eews;
	if (fread (&n_eews, sizeof (int), 1, fl) == 1) {
	    int i, s, err;
	    for (i = 0; i < n_eews; i++) {
		Eew_t eew;
		if (fread (&eew, sizeof (Eew_t), 1, fl) != 1)
		    break;
		s = eew.n_rgs * eew.n_azs;
		eew.ews = (short *)MISC_malloc (s * 
				(sizeof (short) + sizeof (unsigned char)));
		eew.medinp = (unsigned char *)(eew.ews + s);
		err = 0;
		if (fread (eew.ews, sizeof (short), s, fl) != s ||
		    fread (eew.medinp, sizeof (unsigned char), s, fl) != s)
		    err = 1;
		Eews = (Eew_t *)STR_append ((char *)Eews, 
					    (char *)&eew, sizeof (Eew_t));
		N_eews++;
	 	if (err)
		    break;
	    }
	    if (i >= n_eews)		/* done */
		return (0);
	}
    }
    else {
	typedef struct eew_struct {	/* old Eew_t */
	    time_t time; float elev; float st_range; float r_step;
	    short n_rgs; short n_azs; int vol_num; int b_size; short *ews;
	} Eew_t_old;

	if (fread (&N_eews, sizeof (int), 1, fl) == 1) {
	    int i, s;
	    for (i = 0; i < N_eews; i++) {
		Eew_t_old eewo;
		Eew_t eew;
		if (fread (&eewo, sizeof (Eew_t_old), 1, fl) != 1)
		    break;
		memcpy (&eew, &eewo, sizeof (Eew_t_old));
		s = eewo.n_rgs * eewo.n_azs;
		eew.ews = (short *)MISC_malloc (s * sizeof (short));
		if (fread (eew.ews, sizeof (short), s, fl) != s)
		    break;
		eew.medinp = NULL;	/* not available */
		Eews = (Eew_t *)STR_append ((char *)Eews, 
					    (char *)&eew, sizeof (Eew_t));
	    }
	    MISC_log ("    Median inp missing in historical data\n");
	    if (i >= N_eews)
		return (0);
	}
    }
    MISC_log ("Error reading EW from %s\n", fname);
    if (!ops)
	exit (1);
    else {
	int i;
	for (i = 0; i < N_eews; i++)
	    free (Eews[i].ews);
	N_eews = 0;
    }

    return (0);
}

int EE_save_data (Vdeal_t *vdv, FILE *fl, char *fname, int ops) {
    int yz = vdv->yz;
    if (!ops) {
	if (fwrite (&(vdv->start_azi), sizeof (float), 1, fl) != 1 ||
	    fwrite (&yz, sizeof (int), 1, fl) != 1 ||
	    fwrite (vdv->ew_aind, sizeof (char), yz, fl) != yz ||
	    fwrite (vdv->ew_azi, sizeof (short), yz, fl) != yz) {
	    MISC_log ("Save VDV data failed from %s\n", fname);
	    return (-1);
	}
    }

    if (fwrite (&N_eews, sizeof (int), 1, fl) == 1) {
	int i, s;
	for (i = 0; i < N_eews; i++) {
	    Eew_t *eew = Eews + i;
	    s = eew->n_rgs * eew->n_azs;
	    if (fwrite (eew, sizeof (Eew_t), 1, fl) != 1)
		break;
	    if (fwrite (eew->ews, sizeof (short), s, fl) != s)
		break;
	    if (fwrite (eew->medinp, sizeof (unsigned char), s, fl) != s)
		break;
	}
	if (i >= N_eews)
	    return (0);
    }
    MISC_log ("Error saving EW to %s\n", fname);
    return (-1);
}

