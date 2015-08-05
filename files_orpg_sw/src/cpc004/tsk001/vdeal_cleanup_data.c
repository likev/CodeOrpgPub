
/******************************************************************

    vdeal's module containing functions for cleaning up the data.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/22 21:58:14 $
 * $Id: vdeal_cleanup_data.c,v 1.5 2014/08/22 21:58:14 steves Exp $
 * $Revision: 1.5 $
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

/* variables for saving and retrieving the GCC gates */
static int Sgcc_size = 0, Sgcc_cnt = 0;
static unsigned char *Sgcc_buf = NULL;

static int Gcc_cnt (int func, int x, int n, int max_r, char *xgcc);
static int Get_ew (Vdeal_t *vdv, int x, short *ews);
static int Find_high_shear_range (Vdeal_t *vdv, int thr,
					int density, int *hsr_st);
static int Get_adjusted_threshold (Vdeal_t *vdv, int y, int thr, int max_dec,
						int hsr_st, int hsrw);
static int Check_wind (Vdeal_t *vdv, int max_r, char *no_wind);
static void Process_gcc_likely_ranges ();
static void Save_gcc_gate (int x, int y, unsigned char inp, int type);
static void Rm_gcc_gate (Vdeal_t *vdv, int x, int y, int type);
static void Remove_isolated_gcc (Vdeal_t *vdv);


/***************************************************************************

    Applies the SPW to remove unreliable velocity gates. The work is done
    in "eew_inp". Two bit flags are set in vdv->spw to indicate high spw
    gates. "level" = 1 is for heavy cleanup while "level" = 0 for light 
    cleanup.

***************************************************************************/

void CD_spw_filter (Vdeal_t *vdv, int level, unsigned char *eew_inp) {
    int xz, yz, x, y, hist[64], i, total, thr, drm_thr;
    int hsrw, hsr_st, max_dec, density, pcnt, vm, mexp;
    char buf[128];
    unsigned char *wbuf, *no_d;

    if (vdv->spw == NULL) {
	VDD_log ("SPW not available\n");
	return;
    }

    xz = vdv->xz;
    yz = vdv->yz;
    for (i = 0; i < 64; i++)
	hist[i] = 0;
    total = 0;
    for (y = 0; y < yz; y++) {	/* compute histogram and clear spw flags */
	unsigned char *spw;
	spw = vdv->spw + xz * y;
	for (x = 0; x < xz; x++) {
	    int s = spw[x] & SPW_MASK_value;
	    spw[x] = s;
	    if (s == 0)		/* no data */
		continue;
	    hist[s]++;
	    total++;
	}
    }
    if (total == 0)
	return;

    {					/* determine a spw threshold */
	int cnt, gcnt, thr1, thr2, thr3, max, peak, maxind;
	if (level)
	    gcnt = total * 85 / 100;
	else
	    gcnt = total * 95 / 100;
	cnt = thr1 = peak = max = maxind = 0;
	thr1 = -1;
	for (i = 0; i < 64; i++) {
	    int h = hist[i];
	    cnt += h;
	    if (cnt >= gcnt && thr1 < 0)
		thr1 = i;		/* total number based threshold */
	    if (h != 0)
		maxind = i;
	    if (h > max) {
		max = h;
		peak = i;
	    }
	}
	if (level)
	    thr2 = (peak + maxind) / 2;	/* histogram peak based threshold */
	else
	    thr2 = maxind - (maxind - peak) / 3;
	drm_thr = thr2;
	thr = thr1;
	if (level) {		/* take the smaller */
	    if (thr2 < thr)
		thr = thr2;
	}
	else {			/* take the larger */
	    if (thr2 > thr)
		thr = thr2;
	}
	thr3 = vdv->nyq / 3;	/* nyquist based threshold */
	if (vdv->nyq >= 30)
	    thr3 = vdv->nyq / 4;
	if (thr < thr3)
	    thr = thr3;
	if (Test_mode)
	    sprintf (buf, "(%d %d %d) peak %d max %d",
					thr1, thr2, thr3, peak, maxind);

	cnt = 0;
	for (i = 0; i <= thr; i++)
	    cnt += hist[i];
	density = (total - cnt) * 1000 / total;

	max_dec = (maxind - peak) / 3;
	if (thr - max_dec < peak + (maxind - peak) / 3)
	    max_dec = thr - peak - (maxind - peak) / 3;
    }

    hsr_st = hsrw = 0;			/* high shear range width */
	hsrw = Find_high_shear_range (vdv, thr, density, &hsr_st);
    if (Test_mode) {
	if (hsrw > 0)
	    VDD_log ("SPW thr %d %s den %d HSR %d %d\n",
		thr, buf, density, hsr_st, hsrw);
	else
	    VDD_log ("SPW thr %d %s den %d No HSR\n", thr, buf, density);
    }

    mexp = 3;			/* expansion steps */
    vm = 255 - mexp - 2;	/* min value for "true" in map */
    wbuf = NULL;
    if (level > 0)
	wbuf = MISC_malloc (vdv->xz * (vdv->yz + 1) * sizeof (unsigned char));
    for (y = 0; y < vdv->yz; y++) {
	unsigned char *spw, *eew_in, *map;
	int xz, tl, th, hsr_end, max_r;

	xz = vdv->xz;
	spw = vdv->spw + xz * y;
	map = wbuf + xz * y;
	eew_in = eew_inp + xz * y;
	tl = th = thr;
	if (hsrw > 0)
	    tl = Get_adjusted_threshold (vdv, y, thr, max_dec, hsr_st, hsrw);
	hsr_end = hsr_st + hsrw;
	max_r = 50000. / vdv->g_size;
	if (max_r < hsr_end)
	    max_r = hsr_end;
	for (x = 0; x < xz; x++) {
	    int t;

	    if (x <= max_r && spw[x] >= drm_thr)
		eew_in[x] = BNO_DATA;
	    if (level == 0)
		continue;
	    map[x] = 0;
	    if (x < hsr_st || x >= hsr_end)
		t = th;
	    else
		t = tl;
	    if (spw[x] > t && (vdv->ew.rfs[x / vdv->ew.rz] & RF_HIGH_VS))
		map[x] = vm;
	}
    }
    if (level == 0)
	return;

    no_d = wbuf + vdv->xz * vdv->yz * sizeof (unsigned char);
    for (x = 0; x < vdv->xz; x++)
	no_d[x] = 0;
    pcnt = 0;			/* no data array */
    for (i = -mexp; i < vm - 2; i++) {
	if ((i % 4) == 0)
	    pcnt = 0;
	for (y = 0; y < vdv->yz; y++) {
	    unsigned char *mc, *m[9], *em[4];
	    int xz, k;
    
	    xz = vdv->xz;
	    mc = wbuf + xz * y;		/* current center point */
	    if (vdv->full_ppi)
		m[1] = wbuf + xz * ((y + vdv->yz - 1) % vdv->yz);
	    else if (y > 0)
		m[1] = wbuf + xz * (y - 1);
	    else
		m[1] = no_d;
	    if (vdv->full_ppi)
		m[5] = wbuf + xz * ((y + 1) % vdv->yz);
	    else if (y < vdv->yz - 1)
		m[5] = wbuf + xz * (y + 1);
	    else
		m[5] = no_d;
	    m[0] = m[1] - 1;		/* 8 neighbors */
	    m[2] = m[1] + 1;
	    m[3] = mc + 1;
	    m[4] = m[5] + 1;
	    m[6] = m[5] - 1;
	    m[7] = mc - 1;
	    m[8] = m[0];

	    if (i < 0) {		/* expand the high SPW area */
		int v = mexp + i + vm + 1;
		for (x = 2; x < xz - 2; x++) {
		    if (mc[x] != 0 && mc[x] < v) {
			for (k = 0; k < 8; k++) {
			    if (m[k][x] == 0)
				m[k][x] = v;
			}
		    }
		}
		pcnt = 1;
		continue;
	    }

	    em[0] = m[1];  /* for pushing in 4 directions, one at a time */
	    em[1] = m[5];
	    em[2] = m[7];
	    em[3] = m[3];
	    /* In the following, if mc[x] < vm, it thrinks to a skeleton. If 
	       nc > 1, thin lines won't be shortened */
	    for (x = 1; x < xz - 1; x++) {	/* thrink the high SPW area */
		int nc, dc;
		if (mc[x] <= vm || em[i % 4][x] > i)
		    continue;		/* we keep all high SPW in the map */
		nc = dc = 0;
		for (k = 0; k < 8; k++) {
		    if (m[k][x] > i) {
			if (m[k + 1][x] <= i)
			    dc++;	/* gap count */
			nc++;		/* neighbor count */
		    }
		}
		if (dc == 1 && nc < 6) {
		    mc[x] = i + 1;
		    pcnt++;
		}
	    }
	}
	if ((i % 4) == 3 && pcnt == 0)
	    break;
    }

    for (y = 0; y < vdv->yz; y++) {
	unsigned char *spw, *eew_in, *map, *dmap;
	int xz, hsr_end;

	xz = vdv->xz;
	spw = vdv->spw + xz * y;
	dmap = vdv->dmap + xz * y;
	map = wbuf + xz * y;
	eew_in = eew_inp + xz * y;
	hsr_end = hsr_st + hsrw;
	for (x = 0; x < xz; x++) {
	    if (map[x] >= vm) {
		eew_in[x] = BNO_DATA;
		if (x < hsr_st || x >= hsr_end)
		    spw[x] |= SPW_MASK_hw;
		else
		    spw[x] |= SPW_MASK_hsr | SPW_MASK_hw;
		dmap[x] = (dmap[x] & ~(DMAP_NHSPW | DMAP_HSPW)) | DMAP_HSPW;
	    }
	    else
		dmap[x] = (dmap[x] & ~(DMAP_NHSPW | DMAP_HSPW)) | DMAP_NHSPW;
	}
    }
    vdv->data_type |= DT_HSPW_SET;

    free (wbuf);
}

/***************************************************************************

    Determines the high shear range in the input image vdv->spw using
    threshold "thr". Returns the width of the high shear range. The
    starting range is returned with "hsr_st".

***************************************************************************/

static int Find_high_shear_range (Vdeal_t *vdv, int thr, 
					int density, int *hsr_st) {
    short hscnt[1024], phscnt[1024];
    int xz, x, y, hst_h, hw, st_ind, end_ind;
    int st_r, end_r, max, min, min_high_spw_cnt;

    hst_h = VDV_alt_to_range (vdv, 1000.) / vdv->g_size;
				/* high shear layer depth is 1000 meters */
    st_r = hst_h / 5;	/* start beyond groud clutter range (200 m altitude) */
    end_r = hst_h * 2;	/* end at range of 2000 m altitude */
    xz = 1024;
    if (xz > vdv->xz)
	xz = vdv->xz;
    if (xz > end_r)
	xz = end_r;
    for (x = 0; x < xz; x++)
	hscnt[x] = 0;
    for (y = 0; y < vdv->yz; y++) {
	unsigned char *spw = vdv->spw + vdv->xz * y;
	for (x = 0; x < xz; x++) {
	    if (spw[x] > thr)
		hscnt[x]++;
	}
    }
    hw = hst_h / 8;		/* half width of the smoothing window */
    if (hw < 1)
	hw = 1;
    min_high_spw_cnt = vdv->yz * density / 400;
					/* min high spw count for HSR */
    if (min_high_spw_cnt > vdv->yz / 4)		/* at most 25% of yz */
	min_high_spw_cnt = vdv->yz / 4;
    if (min_high_spw_cnt < vdv->yz * 5 / 100)	/* at least 5% of yz */
	min_high_spw_cnt = vdv->yz * 5 / 100;
    max = 0;
    min = 256;
    end_ind = 0;
    for (x = st_r; x < xz; x++) {
	int cnt, sum, k, phs;

	cnt = sum = 0;
	for (k = -hw; k <= hw; k++) {
	    int xx = x + k;
	    if (xx < 0 || xx >= xz)
		continue;
	    sum += hscnt[xx];
	    cnt++;
	}
	if (cnt > 0)
	    phs = sum / cnt;		/* smoothing */
	else
	    phs = 0;
	phscnt[x] = phs;
	if (phs > max)
	    max = phs;
	if (phs < min)
	    min = phs;
	if (max > min_high_spw_cnt && phs < min + (max - min) / 3 && 
					x >= st_r + hst_h / 5) {
	    end_ind = x;
	    break;
	}
    }
    if (end_ind == 0)
	return (0);
    if (end_ind > hst_h * 2)
	return (0);

    st_ind = st_r;
    for (x = end_ind - 1; x >= st_r; x--) {	/* look for starting range */
	if (phscnt[x] < min + (max - min) / 3) {
	    st_ind = x;
	    break;
	}
    }
    *hsr_st = st_ind;
    return (end_ind - st_ind + 1);
}

/***************************************************************************

    Returns an adjusted spw threshold for azi "y". "thr" is the original 
    threshold. hsr_st and hsrw are respectively the starting range index 
    and width of the high shear range.    

***************************************************************************/

static int Get_adjusted_threshold (Vdeal_t *vdv, int y, int thr, int max_dec,
						int hsr_st, int hsrw) {
    unsigned char *spw;
    int dec, mincnt, maxcnt;

    mincnt = hsrw / 4;
    maxcnt = hsrw / 2;
    spw = vdv->spw + vdv->xz * y;
    for (dec = 0; dec <= max_dec; dec++) {
	int t, cnt, x;

	t = thr - dec;
	cnt = 0;
	for (x = hsr_st; x < hsr_st + hsrw; x++) {
	    if (spw[x] > t)
		cnt++;
	}
	if (cnt > maxcnt) {
	    if (dec > 0)
		dec--;
	    break;
	}
	if (cnt >= mincnt)
	    break;
	if (dec == max_dec)
	    break;
    }

    return (thr - dec);
}

/**************************************************************************

    Removes ground clutter contaminated data (white data). The data is
    removed from vdv->inp and saved in Saved gcc gates. Since this
    heavily relies on full cut data, a new algorithm will be needed in
    short delay mode. E.g. we remove white gates if the BW is non-0.

**************************************************************************/

#define CD_MAX_R 512
#define CD_MAX_AZI 800
#define CD_EWY_SCALE 2
#define MAX_N_RADS 800
static char Gc_likely_r[CD_MAX_R];
static int Gc_range = 0;

void CD_remove_ground_clutter (Vdeal_t *vdv) {
    int xz, yz, x, near_0, near_0c, data_off, gccr;
    int vst_tgsp, vst_tgc, vst_x, vst_thr, vst_stop;
    extern int Test_mode;
    char no_wind[CD_MAX_R], gc_detect[CD_MAX_R], *cgcgs, cgcgs_b[CD_MAX_AZI];

    /* ground clutter up to 100 KM range */
    Gc_range = 100000. / vdv->g_size;	/* in number of gates */
    if (Gc_range > CD_MAX_R)
	Gc_range = CD_MAX_R;
    if (Check_wind (vdv, Gc_range, no_wind) <= 0 && vdv->elev <= 3.f)
	return;
    Sgcc_cnt = 0;		/* discard previous saved GCC gates */

    gccr = -1;			/* current range where gc is likely */
    near_0 = vdv->data_scale + .5;
    near_0c = 2. * vdv->data_scale + .5;  /* threshold for near 0 check */
    if (vdv->low_prf)
	near_0c--;
    data_off = vdv->data_off;	/* threshold for near 0 check for cc */
    xz = vdv->xz;
    yz = vdv->yz;

    /* variables for detecting transitional area of vertical shear */
    vst_tgsp = vst_tgc = vst_x = vst_stop = 0;
    vst_thr = yz * 7 / 10;

    cgcgs = cgcgs_b;
    if (yz > CD_MAX_AZI)
	cgcgs = MISC_malloc (yz);
    for (x = 0; x < Gc_range; x++) {
	int y, d, cc, c1c, c0max, pass, max_pas, near_gc, mxmn0, mncn0, minclc;
	int f0, l0, fstt, done, rng, ngc, nc0, tngsp, tngc, tngs_t_ew, tngs_t;
	unsigned char *inp, uc;

	gc_detect[x] = 0;

	near_gc = 0;
	{
	    int k, cnt;
	    cnt = 0;
	    for (k = 1; k < 5; k++) {
		if (x - k >= 0 && gc_detect[x - k])
		    cnt++;
	    }
	    if (cnt >= 2)
		near_gc = 1;	/* gc detected in nearby */
	}

	cc = 0;		/* gate count in current section of 0s and no data */
	c1c = 0;		/* latest number of non-0s */
	f0 = -1;		/* first 0 gate in section */
	l0 = 0;			/* last 0 gate in section */
	rng = 0;		/* number non 0 gates to remove in section */
	ngc = 0;		/* non 0 gates between two near 0 gates */
	fstt = -1;		/* start y of the first processed section */
	nc0 = 0;

	c0max = 60. / vdv->gate_width + .5;
	if (no_wind[x])
	    c0max = c0max * 2;
	if (near_gc)
	    c0max = c0max * 3 / 4;
	mxmn0 = (10. / vdv->gate_width + .5) + 2;
				/* max # of effective missing around near 0 */
	mncn0 = (20. / vdv->gate_width + .5);
				/* min # of consecutive near 0 for non gc */
	if (vdv->elev < 2.f)
	    minclc = 3. / vdv->gate_width + .5;
	else
	    minclc = 5. / vdv->gate_width + .5;
	pass = 0;		/* second pass needed for full 360 ppi */
	tngsp = 0;		/* total effective near 0 span */
	tngc = 0;		/* total non 0 data count */
	max_pas = 1;
	if (vdv->full_ppi)
	    max_pas = 2;
	done = 0;
	for (y = 0; y < yz; y++) {
	    inp = vdv->inp + y * xz + x;
	    uc = *inp;
	    cc++;
	    if (uc == BNO_DATA)
		ngc++;
	    else {
		d = (int)uc - data_off;		/* distance to zero */
		if (d < 0)
		    d = -d;    
		if (d <= near_0c) {
		    if (c1c > 0)
			c1c--;
		    if (f0 < 0)
			f0 = y;
		    else if (ngc > mxmn0)
			rng += ngc - mxmn0;
				/* rm excessive consecutive non-near-0 */
		    l0 = y;
		    ngc = 0;
		    nc0++;
		}
		else {
		    c1c++;
		    ngc++;
		    tngc++;
		}
	    }
	    if (ngc != 0) {		/* missing or non-0 */
		if (nc0 > mncn0)
		    rng += nc0;	/* rm consecutive near-0s that are not gc */
		nc0 = 0;
	    }
	    if (pass == 0)
		cgcgs[y] = 0;	/* current gc gates */
	    if (y == yz - 1 && (max_pas == 1 || fstt < 0))
		done = 1;
	    if (pass > 0 && y >= fstt - 1)
		done = 1;
	    if (c1c >= minclc || done) {
				/* minclc non-0 gates stops the secton */
		if (fstt >= 0 || max_pas == 1 || done) {
		    int w;			/* effective section width */
		    if (f0 < 0)
			w = 0;
		    else if (l0 < f0)
			w = l0 + yz - f0 + 1;
		    else
			w = l0 - f0 + 1;
		    w -= rng;
		    if (w > 0) {	/* add missing outside [f0, l0] */
			int k, mxmn02;
			unsigned char *ip = vdv->inp + x;
			mxmn02 = mxmn0 / 2;
			for (k = f0 - 1; k >= f0 - mxmn02; k--) {
			    if (max_pas == 1 && k < 0)
				break;
			    if (ip[((k + yz) % yz) * xz] != BNO_DATA)
				break;
			}
			w += f0 - 1 - k;
			for (k = l0 + 1; k <= l0 + mxmn02; k++) {
			    if (max_pas == 1 && k >= yz)
				break;
			    if (ip[(k % yz) * xz] != BNO_DATA)
				break;
			}
			w += k - l0 - 1;
		    }
		    tngsp += w;

		    if (w >= c0max) {	/* remove consecutive near 0 gates */
			int k;
			for (k = 0; k < cc; k++) {
			    int yy = (y + yz - k) % yz;
			    unsigned char *ip = inp - (y - yy) * xz;
			    if (*ip != BNO_DATA) {
				d = (int)*ip - data_off;  /* dist to zero */
				if (d < 0)
				    d = -d;
				if (d <= near_0c)
				    cgcgs[yy] = 1;
			    }
			}
			gc_detect[x] = 1;
		    }
		    if (fstt < 0)
			fstt = y - cc + 1;
		}
		else if (fstt < 0)
		    fstt = y + 1;
		if (done)
		    break;
		cc = c1c = l0 = rng = ngc = nc0 = 0;
		f0 = -1;
	    }

	    if (y == yz - 1) {
		pass++;
		y = -1;
		if (pass > max_pas)
		    break;
	    }
	}
	if (vst_tgsp >= vst_thr && tngsp + tngc < yz / 2)
	    vst_stop = 1;	/* restoring gcc stop here */
	if (!vst_stop) {
	    if (tngsp > vst_tgsp) {
		vst_tgsp = tngsp;	/* max tngsp */
		vst_tgc = tngc;
	    }
	    if (tngc > vst_tgc) {
		vst_tgc = tngc;		/* max tngc after max tngsp */
		vst_x = x;
	    }
	}


	if (tngsp >= c0max || tngsp > tngc * 8) {
	    for (y = 0; y < yz; y++) {
		if (cgcgs[y]) {
		    unsigned char *inp = vdv->inp + y * xz + x;
		    if (*inp != BNO_DATA)
			Rm_gcc_gate (vdv, x, y, GATE_OTHER);
		}
	    }
	}

	if (no_wind[x])
	    tngs_t = 180. / vdv->gate_width + .5;
	else
	    tngs_t = 120. / vdv->gate_width + .5;
	if (near_gc)
	    tngs_t = tngs_t * 3 / 4;
	if (vdv->low_prf)
	    tngs_t_ew = tngs_t;
	else
	    tngs_t_ew = tngs_t * 3 / 4;
	if (tngsp > tngc * 8)
	    tngs_t = tngs_t_ew = 0;
	if (tngsp >= tngs_t_ew) {	/* sufficient near-zeros for GCC */
	    short ewsb[MAX_N_RADS / CD_EWY_SCALE], *ews;
	    int ew_yes, thr;
	    ews = ewsb;
	    ew_yes = 0;
	    if (!no_wind[x]) {
		if (vdv->yz > MAX_N_RADS)
		    ews = MISC_malloc ((vdv->yz / CD_EWY_SCALE) * sizeof (short));
		ew_yes = Get_ew (vdv, x, ews);
	    }
	    thr = 4.f * vdv->data_scale + .5f;
	    for (y = 0; y < yz; y++) {	
		inp = vdv->inp + (y * xz + x);
		uc = *inp;
		if (uc == BNO_DATA)
		    continue;
		d = (int)uc - data_off;
		if (d < 0)
		    d = -d;
		if (d > near_0)
		    continue;
		if (no_wind[x] || !ew_yes) {	/* no wind or no ew */
		    if (tngsp >= tngs_t && *inp != BNO_DATA) {
			Rm_gcc_gate (vdv, x, y, GATE_OTHER);
			gc_detect[x] = 1;
		    }
		}
		else {		/* check against the EW map */
		    int aew, nyq;
		    aew = ews[y / CD_EWY_SCALE];
		    if (aew < 0) aew = -aew;
		    nyq = VDD_get_nyq (vdv, y);
		    while (aew > nyq * 2)
			aew -= nyq * 2;
		    if (aew > nyq)
			aew = nyq * 2 - aew;
		    if (vdv->low_prf)
			thr = nyq / 2;
		    if (aew > thr) {	/* ew is far from 0 */
			Rm_gcc_gate (vdv, x, y, GATE_OTHER);
			gc_detect[x] = 1;
		    }
		}
	    }
	    if (ews != ewsb)
		free (ews);
	}

	if (tngsp > (int)(45. / vdv->gate_width + .5) || gc_detect[x])
	    Gc_likely_r[x] = 1;		/* marks the data at x as GCC likely */
	if (gc_detect[x])
	    gccr = x;

	if (x > Gc_range / 2 && x - gccr > Gc_range / 8)
	    break;		/* max GCC range is unlikely reached */
    }
    Gc_range = gccr + 1;
    if (cgcgs != cgcgs_b)
	free (cgcgs);

    /* restore near 0 gates in transitional area of vertical shear */
    if (vdv->elev < 5.f && vst_tgsp >= vst_thr && vst_tgc >= yz / 2) {
	Gate_t *gccin, *gccout;
	int cnt, i;
	for (x = 0; x < vst_x; x++)
	    Gc_likely_r[x] = 0;
	gccin = gccout = (Gate_t *)Sgcc_buf;
	cnt = 0;
	for (i = 0; i < Sgcc_cnt; i++) {
	    if (gccin->x < vst_x)
		vdv->inp[gccin->y * xz + gccin->x] = gccin->v;
	    else {
		gccout[cnt] = *gccin;
		cnt++;
	    }
	    gccin++;
	}
	Sgcc_cnt = cnt;
    }

    Process_gcc_likely_ranges ();

    Remove_isolated_gcc (vdv);

    return;
}

/***************************************************************************

    Removes near-zero v data surrounded by only high v data or near-zero
    data. Such data are most likely bad data.

***************************************************************************/

static void Remove_isolated_gcc (Vdeal_t *vdv) {
    unsigned char *inp;
    int xz, yz, y, x, thr0, thr1, thr2, d_off;

    return;	/* this function is disabled because it affects performance 
		   near tornados. We can use this in the future when we have
		   a tornado detection function built in. */

    inp = vdv->inp;
    xz = vdv->xz;
    yz = vdv->yz;
    thr0 = 5;
    if (vdv->data_scale <= 1.5f)
	thr0 = 3;
    thr1 = vdv->nyq / 6;
    thr2 = vdv->nyq / 2;
    d_off = vdv->data_off;
    for (y = 0; y < yz; y++) {
	int ofs[16];

	unsigned char *in = inp + y * xz;
	VDA_get_neighbor_offset (4, y, xz, yz, vdv->full_ppi, ofs);
	for (x = 1; x < xz - 1; x++) {
	    int df, lcnt, hcnt, i;

	    if (in[x] == BNO_DATA)
		continue;
	    df = in[x] - d_off;
	    if (df < 0)
		df = -df;
	    if (df >= thr0)
		continue;
	    hcnt = lcnt = 0;
	    for (i = 0; i < 4; i++) {
		int of = ofs[i];
		if (of != 0) {
		    int v = in[x + of];
		    if (v != BNO_DATA) {
			df = v - d_off;
			if (df < 0)
			    df = -df;
			if (df >= thr2)
			    hcnt++;
			else if (df >= thr1)
			    lcnt++;
		    }
		}
	    }
	    if (hcnt < 1 || lcnt > 1)
		continue;
	    Rm_gcc_gate (vdv, x, y, GATE_ISOLATE);
	}
    }
}

/***************************************************************************

    Saves V at (x, y) and remove it from V, Z and spw.

***************************************************************************/

static void Rm_gcc_gate (Vdeal_t *vdv, int x, int y, int type) {
    unsigned char *inp;
    int off = y * vdv->xz + x;
    inp = vdv->inp + off;
    Save_gcc_gate (x, y, *inp, type);
    *inp = BNO_DATA;
    if (vdv->dbz != NULL)
	vdv->dbz[off] = BNO_DATA;
    vdv->spw[off] = BNO_DATA;
}

/***************************************************************************

    Returns the pointer to the GCC gates.

***************************************************************************/

int CD_get_saved_gcc_gate (Gate_t **saved_gates) {
    if (saved_gates != NULL)
	*saved_gates = (Gate_t *)Sgcc_buf;
    return (Sgcc_cnt);
}

/***************************************************************************

    Saves the GCC gate at (x, y) of value inp.

***************************************************************************/

static void Save_gcc_gate (int x, int y, unsigned char inp, int type) {
    Gate_t *g;

    if (Sgcc_cnt >= Sgcc_size) {
	unsigned char *nb;
	int nz;
	if (Sgcc_size > 0)
	    nz = Sgcc_size * 2;
	else
	    nz = 20000;
	nb = MISC_malloc (nz * sizeof (Gate_t));
	memcpy (nb, Sgcc_buf, Sgcc_size * sizeof (Gate_t));
	if (Sgcc_buf != NULL)
	    free (Sgcc_buf);
	Sgcc_buf = nb;
	Sgcc_size = nz;
    }
    g = (Gate_t *)Sgcc_buf + Sgcc_cnt;
    g->x = x;
    g->y = y;
    g->v = inp;
    g->type = type;
    Sgcc_cnt++;
}

/***************************************************************************

    Checks the if there is significant wind for each radial in
    "vdv->inp" up to range "max_r". no_wind is set if there is no
    significant wind at this range. Return 0 if there is no significant
    wind in all ranges, or 1 otherwise.

***************************************************************************/

static int Check_wind (Vdeal_t *vdv, int max_r, char *no_wind) {
    int *hist, min0, max0, minh, maxh, cn0, tch, x;

    min0 = vdv->data_off;
    max0 = vdv->data_off;
    minh = vdv->data_off - Myround (.5 * vdv->nyq);
    maxh = vdv->data_off + Myround (.5 * vdv->nyq);
    cn0 = tch = 0;
    for (x = 0; x < max_r; x++) {
	int xs, xz, c0, ch, n, i;

	xs = x - 1;
	if (xs < 0)
	    xs = 0;
	xz = x + 1 - xs + 1;
	if (xs + xz > vdv->xz)
	    xz = vdv->xz - xs;
	if (xz < 1) {
	    int k;
	    for (k = x; k < max_r; k++)
		no_wind[k] = 1;
	    break;
	}
	n = VDA_compute_data_hist (vdv->inp, vdv->xz, xs, xz, vdv->yz, &hist);
	c0 = ch = 0;
	for (i = 0; i < 256; i++) {
	    int h = hist[i];
	    if (i >= min0 && i <= max0)
		c0 += h;
	    else if (i > maxh || i < minh)
		ch += h;
	}
	if (ch * 20 < n - c0)
	    no_wind[x] = 1;
	else
	    no_wind[x] = 0;

	cn0 += n - c0;
	tch += ch;
    }
    if (tch * 20 < cn0)		/* no high wind exists */
	return (0);

    return (1);
}

/**************************************************************************

    Returns number of 1's in "n" elements of xgcc starting with index x.
    max_r is the array size of xgcc. func = 1 for finding consecutive 1's
    while func = 2 for finding consecutive 1's in reverse direction.

**************************************************************************/

static int Gcc_cnt (int func, int x, int n, int max_r, char *xgcc) {
    int cnt, i, xx;

    cnt = 0;
    for (i = 0; i < n; i++) {
	if (func == 2)
	    xx = x - i;
	else
	    xx = x + i;
	if (xx >= 0 && xx < max_r) {
	    if (xgcc[xx] == 1)
		cnt++;
	    else if (func != 0)
		break;
	}
	else {
	    if (func != 0)
		break;
	}
    }
    return (cnt);
}

/**************************************************************************

    Processes the detected GCC-likely ranges.

**************************************************************************/

static void Process_gcc_likely_ranges () {
    int maxw, w, x;

    maxw = 5;
    for (w = 1; w <= maxw; w++) {	/* fill in Gc_likely_r gaps */
	int extw = w * 2;
	for (x = 1; x < Gc_range; x++) {
	    int xx, lcnt, rcnt, k;

	    if (Gc_likely_r[x] != 0 ||
		Gc_likely_r[x - 1] != 1 ||
		Gc_likely_r[x + w] != 1 ||
		Gcc_cnt (0, x, w, Gc_range, Gc_likely_r) > 0) {
		continue;
	    }
	    xx = x;
	    x += w - 1;
	    lcnt = Gcc_cnt (2, xx - 1, extw, Gc_range, Gc_likely_r);
	    if (lcnt < w)
		continue;
	    rcnt = Gcc_cnt (1, xx + w, extw, Gc_range, Gc_likely_r);
	    if (rcnt < w || lcnt + rcnt < w * 3)
		continue;
	    for (k = 0; k < w; k++)
		Gc_likely_r[xx + k] = 2;
	}
    }
}

/**************************************************************************

    Returns Gc_likely_r.

**************************************************************************/

int CD_get_gcc_likely (char **p) {

    *p = Gc_likely_r;
    return (Gc_range);
}

/*****************************************************************************

*****************************************************************************/

static int Get_ew (Vdeal_t *vdv, int x, short *ews) {
    double spd, azi;
    int y;

    if (VDV_get_wind (vdv, x, &spd, &azi) != 0)
	return (0);

    for (y = 0; y < vdv->yz; y += CD_EWY_SCALE) {
	double a = vdv->ew_azi[y] * .1;
	ews[y / CD_EWY_SCALE] = Myround ( -spd * cos ((azi - a) * deg2rad));
    }
    return (1);
}

int CD_read_gcc (Vdeal_t *vdv, FILE *fl, char *fname) {
    int vcp;
    if (fread (&vcp, sizeof (int), 1, fl) != 1) {
	VDD_log ("Read GCC failed from %s\n", fname);
	return (-1);
    }
    vdv->vcp = vcp;
    return (0);
}

int CD_save_gcc (Vdeal_t *vdv, FILE *fl, char *fname) {
    int vcp = vdv->vcp;
    /* vol_num and dtm have already saved and restored */
    if (fwrite (&vcp, sizeof (int), 1, fl) != 1) {
	VDD_log ("Save GCC failed from %s\n", fname);
	return (-1);
    }
    return (0);
}


