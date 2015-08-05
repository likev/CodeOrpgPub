
/******************************************************************

    vdeal's module containing functions for data preprocessing.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/16 14:43:58 $
 * $Id: vdeal_preprocess.c,v 1.10 2014/07/16 14:43:58 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include "vdeal.h"

/* variables share between analyze.c and preprocess.c */
extern __thread unsigned char *Wbuf;

#define MAXDY 16	/* max y deviation of front template */
#define MAXDX 2		/* max 4 deviation of front template */
#define MAX_TMPLS ((MAXDY * 2 + 1) * (MAXDX * 2 + 1))
			/* max # of points in the front template */
typedef struct {
    int x, y;		/* front center location */
    int av;		/* average value of shear's two sides */
} Front_t;

#define MAX_FCNT 2	/* max # of fronts */
static Front_t Fronts[MAX_FCNT];
static int N_fronts;

static short *Hz_cnt = NULL;  /* count of high-ref gates at each range gate */
static short *Second_trip_count = NULL;
static unsigned char *Med_v = NULL;

static int Get_clear_air_and_storm_range (Vdeal_t *vdv, short *lz_cnt,
						short *hzw_cnt, int *s_rp);
static void Fill_small_gaps (Vdeal_t *vdv, unsigned char *inp,
				int xz, int yz, int fp, int level, int nyq);
static int Detect_convergence_shear (Vdeal_t *vdv, unsigned char *shears,
							int nyq);
static int Get_front_template (Ew_struct_t *ew, int x,
					int *tx, int *ty, int *yw);
static void Generate_ewv (Vdeal_t *vdv);

/******************************************************************

    Converts the SPW to the internal data format: Missing data is
    0. Unit is m/s. 1, 2, ... are respecively 0, 1, ... m/s. The 
    maximum is 63 (Any value greater than 63 is set to 63).

******************************************************************/

void PP_convert_spw (unsigned char *spw, int n, int data_off) {
    int i;
    for (i = 0; i < n; i++) {
	int w = spw[i];
	if (w <= 1)
	    w = 0;
	else if (w < data_off)
	    w = 1;
	else {
	    w = w - data_off + 1;
	    if (w > 63)
		w = 63;
	}
	spw[i] = w;
    }
}

/********************************************************************

    Preprocessing the input data of "yn" radials starting at "ys". 
    1 to set to 0. Nyquest V limit is checked and corrected.

*********************************************************************/

int PP_preprocessing_v (Vdeal_t *vdv, int ys, int yn) {
    static int max_diff = 0, cnt = -1;
    short up, low;
    int xz;
    unsigned char *inp, *p, *ps, *pe;
    Ew_struct_t *ew;

    ew = &(vdv->ew);
    if (cnt < 0 || (vdv->realtime && vdv->radial_status == RS_START_ELE)) {
	max_diff = 0;
	cnt = 0;
	Second_trip_count = (short *)STR_reset (Second_trip_count,
				ew->n_rgs * ew->n_azs * sizeof (short));
	memset (Second_trip_count, 0, ew->n_rgs * ew->n_azs * sizeof (short));
    }

    xz = vdv->xz;
    inp = vdv->inp;
    ps = inp + ys * xz;
    pe = ps + xz * yn;
    p = ps;
    low = up = 0;	/* not needed */
    while (p < pe) {
	int g;
	if (((p - inp) % xz) == 0) {
	    int nyq = VDD_get_nyq (vdv, (p - inp) / xz);
	    up = vdv->data_off + nyq;
	    low = vdv->data_off - nyq;
	}
	if (*p == 1) {
	    int x, y;
	    x = ((p - ps) % xz) / ew->rz;
	    y = vdv->ew_aind[(p - ps) / xz + ys];
	    Second_trip_count[y * ew->n_rgs + x]++;
	    *p = BNO_DATA;
	}
	g = *p;
	if (g != BNO_DATA) {
	    if (g < low) {
		*p = low;
		if (low - g > max_diff)
		    max_diff = low - g;
		cnt++;
	    }
	    if (g > up) {
		*p = up;
		if (g - up > max_diff)
		    max_diff = g - up;
		cnt++;
	    }
	}
	p++;
    }
    if (cnt > 0 && 
	(!vdv->realtime || vdv->radial_status == RS_END_ELE))
	VDD_log ("%d gates out of Nyquist range (max diff %d) - changed\n",
						cnt, max_diff);
    return (0);
}

/***************************************************************************

    Sets up vdv->n_secs and vdv->secs. n elements of secs are: azi_num and
    nyq of sector starting radials except the first sector.

***************************************************************************/

void PP_setup_prf_sectors (Vdeal_t *vdv, int nyq, int n, int *secs) {
    int min, i;

    vdv->n_secs = 1;
    vdv->secs[0].azi = 0;
    vdv->secs[0].nyq = nyq;
    vdv->secs[0].size = vdv->yz;
    vdv->nyq = nyq;
    if (n == 0)
	return;
    if (n != 2 && n != 4) {
	MISC_log ("Unexpected number of PRF secs (%d)\n", n);
	return;
    }
    if (n == 2)
	MISC_log ("PRF sector at %d (nyq %d)\n", secs[0], secs[1]);
    else
	MISC_log ("PRF sectors at %d %d (nyq %d %d)\n",
				secs[0], secs[2], secs[1], secs[3]);
    min = nyq;
    for (i = 0; i < n; i += 2) {
	int ind, min, nq;
	ind = i / 2 + 1;
	nq = secs[i + 1] + .5f;
	vdv->secs[ind].azi = secs[i] - 1 + .5f;
	vdv->secs[ind].nyq = nq;
	if (ind > 0)
	    vdv->secs[ind - 1].size = vdv->secs[ind].azi - 
					    vdv->secs[ind - 1].azi;
	if (i == n - 2)
	    vdv->secs[ind].size = vdv->yz - vdv->secs[ind].azi;
	if (nq < min)
	    min = nq;
    }
    vdv->n_secs += n / 2;
    for (i = 0; i < vdv->n_secs; i++) {
	if (vdv->secs[i].azi < 0 || vdv->secs[i].azi >= vdv->yz ||
	     vdv->secs[i].size <= 0) {
	    MISC_log ("Unexpected PRF secs specification\n");
	    vdv->n_secs = 1;
	    vdv->secs[0].size = vdv->yz;
	    return;
	}
    }
    vdv->nyq = min;
}

/**************************************************************************

    Returns the Z analysis result of "name".

**************************************************************************/

short *PP_get_hz_cnt () {
    return (Hz_cnt);
}

/**************************************************************************

    Sets ewm in the front area.

**************************************************************************/

void PP_set_front_ew (Vdeal_t *vdv) {
    Ew_struct_t *ew;
    int n_azs, n_rgs, f;

    ew = &(vdv->ew);
    n_azs = ew->n_azs;
    n_rgs = ew->n_rgs;
    for (f = 0; f < N_fronts; f++) {
	int tx[MAX_TMPLS], ty[MAX_TMPLS], n, i;
	
	n = Get_front_template (ew, Fronts[f].x, tx, ty, NULL);
	for (i = 0; i < n; i++) {
	    int of = ((Fronts[f].y + ty[i] + n_azs) % n_azs) * n_rgs + tx[i];
	    if (ew->ewm[of] == SNO_DATA)
		ew->ewm[of] = Fronts[f].av;
	}
    }
}

/**************************************************************************

    Detects fronts by detecting clustered convergence shears and sets the
    FRONT flag.

**************************************************************************/

void PP_detect_fronts (Vdeal_t *vdv) {
    Ew_struct_t *ew;
    int n_rgs, n_azs, x, y, f, yw, max_r, tx[MAX_TMPLS], ty[MAX_TMPLS];
    unsigned char *map, *shears;
    extern int Test_mode;

    N_fronts = 0;
    if (!vdv->full_ppi || vdv->elev > 2.5f || vdv->low_prf)
	return;

    ew = &(vdv->ew);
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;
    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
			2 * n_rgs * n_azs * sizeof (unsigned char));
    map = Wbuf;
    shears = map + n_rgs * n_azs;
    if (Detect_convergence_shear (vdv, shears, vdv->nyq) == 0)
	return;

    yw = 0;
    for (x = 0; x < n_rgs; x++) {
	int yon_b[MAXDY * 2 + 1];
	int *yon, n;

	if ((ew->rfs[x] & RF_HIGH_VS) && (ew->rfs[x] & RF_LOW_VS)) {
	    for (y = 0; y < n_azs; y++) {  /* very high vs - skip detection */
		map[y * n_rgs + x] = 0;
	    }
	    continue;
	}

	yon = yon_b + MAXDY;
	n = Get_front_template (ew, x, tx, ty, &yw);

	for (y = 0; y < n_azs; y++) {
	    int i, cnt, c, cmax;

	    for (i = -MAXDY; i <= MAXDY; i++)
		yon[i] = 0;
	    for (i = 0; i < n; i++) {
		int of = ((y + ty[i] + n_azs) % n_azs) * n_rgs + tx[i];
		if (shears[of] > 0)
		    yon[ty[i]] = 1;
	    }
	    c = cmax = cnt = 0;
	    for (i = -MAXDY; i <= MAXDY; i++) {
		if (yon[i]) {
		    c++;
		    cnt++;
		}
		else
		    c = 0;
		if (c > cmax)
		    cmax = c;
	    }
	    if (cmax < yw / 6)	/* must have some consecutive shear gates */
		cnt = 0;
	    map[y * n_rgs + x] = cnt;
	}
    }

    max_r = Myround (VDV_alt_to_range (vdv, 2000.) / vdv->g_size);
    N_fronts = 0;
    while (1) {		/* search for the clusters */
	int max, xm, ym, i, n, c, avs, y90, ucnt, dcnt, tsr;
	int maxx_b[MAXDY * 2 + 1], *maxx, bcnt, max_n;

	max = xm = ym = 0;
	for (y = 0; y < n_azs; y++) {
	    for (x = 0; x < n_rgs; x++) {
		if (map[y * n_rgs + x] > max) {
		    max = map[y * n_rgs + x];
		    xm = x;
		    ym = y;
		}
	    }
	}
	max_n = yw * (2 * n_rgs - xm) / (2 * n_rgs) + 1;
	if (max < max_n / 4 + 1)
	    break;

	for (i = -MAXDY; i <= MAXDY; i++)
	    maxx_b[i] = -1;
	maxx = maxx_b + MAXDY;
	n = Get_front_template (ew, xm, tx, ty, NULL);
	c = avs = ucnt = dcnt = 0;
	for (i = 0; i < n; i++) {	/* compute shear average value */
	    int of = ((ym + ty[i] + n_azs) % n_azs) * n_rgs + tx[i];
	    if (shears[of] > 0) {
		avs += shears[of];
		c++;
		if (ty[i] > 0)
		    dcnt++;		/* down side hs count */
		else if (ty[i] < 0)
		    ucnt++;		/* up side hs count */
	    }
	    if (tx[i] > maxx[ty[i]])
		maxx[ty[i]] = tx[i];
	}
	if (c > 0)
	    avs = avs / c;
	if (dcnt <= ucnt && ucnt > 0)
	    tsr = 100 * dcnt / ucnt;	/* two side hs ratio */
	else if (dcnt >= ucnt && dcnt > 0)
	    tsr = 100 * ucnt / dcnt;
	else
	    tsr = 0;

	bcnt = 0;	/* check data after front (should be smooth) */
	for (y = 0; y <= MAXDY; y++) {
	    int xmax, i, sign;
	    xmax = -1;
	    for (i = 0; i < n; i++) {
		if (ty[i] == y && tx[i] > xmax)
		    xmax = tx[i];
	    }
	    if (xmax < 0)		/* done */
		break;
	    for (sign = -1; sign <= 1; sign += 2) {
		int yof = ((ym + sign * y + n_azs) % n_azs) * n_rgs;
		for (x = xmax + 1; x < xmax + 8; x++) {
		    if (x < n_rgs && shears[yof + x] > 0) {
			bcnt++;
			break;
		    }
		}
		if (y == 0)		/* no +/- needed */
		    break;
	    }
	}

	if (bcnt <= max_n / 5 && xm * ew->rz < max_r && tsr > 30) {
	    Fronts[N_fronts].x = xm;
	    Fronts[N_fronts].y = ym;
	    Fronts[N_fronts].av = avs;
	    N_fronts++;
	    if (N_fronts >= MAX_FCNT)
		break;
	}

	/* remove local clusters to prevent from multi count */
	y90 = n_azs / 4;
	for (i = -y90; i <= y90; i++) {
	    int j;
	    y = ym + i;
	    y = (y + n_azs) % n_azs;
	    for (j = -MAXDX; j <= MAXDX; j++) {
		x = xm + j;
		if (x < 0 || x >= n_rgs)
		    continue;
		map[y * n_rgs + x] = 0;
	    }
	}
    }
    if (N_fronts == 0)
	return;

    {
	char buf[128];
	sprintf (buf, "FRONTS DETECTED:");
	for (f = 0; f < N_fronts; f++) {	/* set the front flag */
	    int tx[MAX_TMPLS], ty[MAX_TMPLS];
	    int n, i;
	    n = Get_front_template (ew, Fronts[f].x, tx, ty, NULL);
	    for (i = 0; i < n; i++) {
		int of;
		of = ((Fronts[f].y + ty[i] + n_azs) % n_azs) * n_rgs + tx[i];
		ew->efs[of] |= EF_FRONT;
	    }
	    sprintf (buf + strlen (buf), "  x %d y %d %d", 
				Fronts[f].x, Fronts[f].y, Fronts[f].av);
	}
	if (Test_mode)
	    MISC_log ("%s\n", buf);
    }
}

/***************************************************************************

    Calculates the template of a front centered at x. Returns the number of
    template points.

***************************************************************************/

static int Get_front_template (Ew_struct_t *ew, int x,
					int *tx, int *ty, int *yw) {
    int n_azs, n_rgs, inc, c;

    n_azs = ew->n_azs;
    n_rgs = ew->n_rgs;
    c = 0;
    for (inc = 0; inc <= MAXDY; inc++) {
	double alfa;
	int rinc, st, end, k;

	alfa = inc * 360. / n_azs;
	if (yw != NULL)
	    *yw = inc * 2 + 1;
	if (alfa > 70.)
	    break;
	rinc = Myround ((1. / cos (alfa * deg2rad) - 1.) * (x + .5));
	if (alfa <= 30.) {
	    st = 0;
	    end = 1;
	}
	else {
	    st = -1;
	    end = 2;
	}
	while (end - st + 1 > MAXDX * 2) {
	    st++;
	    end--;
	}
	for (k = st; k <= end; k++) {
	    int xx = rinc + k + x;
	    if (xx < 0 || xx >= n_rgs)
		continue;
	    tx[c] = xx;
	    ty[c] = inc;
	    c++;
	    if (inc > 0) {
		tx[c] = xx;
		ty[c] = -inc;
		c++;
	    }
	}
    }
    return (c);
}

/***************************************************************************

    Detects convergence shear that is larger than thr.

***************************************************************************/

static int Detect_convergence_shear (Vdeal_t *vdv, unsigned char *shears,
							int nyq) {
    Ew_struct_t *ew;
    int gxz, max_r, thr, min_v, igw, gy, ccnt;
    int *dt;
    double gw;

    ew = &(vdv->ew);
    gw = ew->rz * .5;		/* grid width in # range gates */
    gxz = Myround (vdv->xz / gw);
    igw = gw;

    max_r = Myround (VDV_alt_to_range (vdv, 4000.) / vdv->g_size);
    thr = Myround (10. * vdv->data_scale);	/* 10 m/s V difference */
    if (thr < nyq / 2)
	thr = nyq / 2;
    min_v = nyq / 20;

    memset (shears, 0, ew->n_azs * ew->n_rgs * sizeof (unsigned char));
    dt = NULL;
    ccnt = 0;
    for (gy = 0; gy < ew->n_azs; gy++) {
	int miny, maxy, y, gx, min_cnt, pr_v, cr_v;

	miny = maxy = -1;
	for (y = 0; y < vdv->yz; y++) {
	    if (vdv->ew_aind[y] == gy) {
		if (miny < 0)
		    miny = y;
		maxy = y;
	    }
	    else if (maxy >= 0)
		break;
	}

	dt = (int *)STR_reset (dt,
		2 * (igw + 2) * (maxy - miny + 1) * sizeof (int));
	min_cnt = (maxy - miny + 1) * igw / 10;
	pr_v = cr_v = BNO_DATA;
	for (gx = 1; gx < gxz; gx++) {	/* we do not care near radar data */
	    int xs, xe, cnt, df, max_d[3];

	    xs = Myround ((gx + .5) * gw);
	    xe = Myround ((gx + 1.5) * gw) + 1;
	    if (xe >= vdv->xz)
		break;
	    cnt = 0;
	    for (y = miny; y <= maxy; y++) {
		int x;
		unsigned char *inp = vdv->inp + y * vdv->xz;
		for (x = xs; x < xe; x++) {
		    if (inp[x] != BNO_DATA) {
			dt[cnt] = inp[x];
			cnt++;
		    }
		}
	    }
	    pr_v = cr_v;
	    if (cnt > min_cnt)
		cr_v = VDA_search_median_value (dt, cnt, nyq, 
						vdv->data_off, NULL);
	    else
		cr_v = BNO_DATA;

	    if (cr_v == BNO_DATA || pr_v == BNO_DATA)
		continue;
	    df = pr_v - cr_v;
	    if (pr_v < vdv->data_off + min_v || cr_v > vdv->data_off - min_v ||
		df < thr)	/* not a convergence shear */
		continue;

	    cnt = 0;		/* check data variance */
	    for (y = miny; y <= maxy; y++) {
		int x;
		unsigned char *inp = vdv->inp + y * vdv->xz;
		for (x = xs - igw; x < xe; x++) {
		    if (inp[x] != BNO_DATA) {
			dt[cnt] = inp[x];
			cnt++;
		    }
		}
	    }
	    VDA_search_median_value (dt, cnt, nyq, vdv->data_off, max_d);
	    if (max_d[2] < df)		/* aliasing */
		continue;
	    shears[gy * ew->n_rgs + xs / ew->rz] = (pr_v + cr_v) / 2;
	    ccnt++;
	}
    }
    STR_free (dt);
    return (ccnt);
}

/**************************************************************************

    Performs analysis on the Z field to get the clear air range, storm
    range. Sets flags: RF_CLEAER_AIR, EF_CLEAR_AIR, EF_SECOND_TRIP, EF_STORM.
    Returns the storm range.

**************************************************************************/

int PP_analyze_z (Vdeal_t *vdv) {
    static int hz_c_bz = 0;
    int xz, n_azs, n_rgs, rz, *zw_cnt, bsize, *ewz;
    short *hzw_cnt, *az_cnt, *lz_cnt, *ewc;
    Ew_struct_t *ew;
    int x, y, hz_thr, ca_r, s_r, minr;
    unsigned char *efs;

    Generate_ewv (vdv);

    xz = vdv->xz;
    ew = &(vdv->ew);
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;
    rz = ew->rz;
    efs = ew->efs;

    if (xz > hz_c_bz) {
	if (Hz_cnt != NULL)
	    free (Hz_cnt);
	Hz_cnt = (short *)MISC_malloc (xz * sizeof (short));
	hz_c_bz = xz;
    }

    bsize = xz * (sizeof (short) * 2 + sizeof (int))
			+ n_rgs * n_azs * (sizeof (short) + sizeof (int));
    ewz = (int *)MISC_malloc (bsize);
    zw_cnt = ewz + n_rgs * n_azs;
    hzw_cnt = (short *)zw_cnt;
    az_cnt = (short *)(zw_cnt + xz);
    lz_cnt = az_cnt + xz;
    ewc = lz_cnt + xz;

    memset (ewz, 0, bsize);
    memset (Hz_cnt, 0, xz * sizeof (short));

    hz_thr = 15;
    for (y = 0; y < vdv->yz; y++) {
	unsigned char *vp, *zp;
	int *cewz;
	short *cewc;

	vp = vdv->inp + y * xz;
	zp = vdv->dbz + y * xz;
	cewz = ewz + (vdv->ew_aind[y] * n_rgs);
	cewc = ewc + (vdv->ew_aind[y] * n_rgs);
	for (x = 0; x < xz; x++) {
	    if (zp[x] >= 2) {
		int ind;
		int z = Myround ((zp[x] - 66.) * .5);
		az_cnt[x] += 1;
		if (z > hz_thr)
		    zw_cnt[x] += z;
		if (z > 8)
		    Hz_cnt[x] += 1;
		if (z < 5)
		    lz_cnt[x] += 1;
		ind = x / rz;
		cewz[ind] += z;
		cewc[ind] += 1;
	    }
	}
    }
    for (x = 0; x < xz; x++) {
	hzw_cnt[x] = zw_cnt[x] / hz_thr;
    }

    ca_r = Get_clear_air_and_storm_range (vdv, lz_cnt, hzw_cnt, &s_r);

    minr = VDV_alt_to_range (vdv, 300.) / vdv->g_size;
			/* ground clutter likely */
    for (x = 0; x < n_rgs; x++) {	/* set _STORM and other flags */
	int thr, min, rg, camin;
	double st_r, dec_rate;
	dec_rate = 1. - (double)x * 1.1 / (n_rgs + 1);
	if (dec_rate > .8)
	    dec_rate = .8;
	if (x == 0) {
	    int i;	/* exluding first no data ranges */
	    for (i = 0; i < rz - 5; i++) {
		if (az_cnt[i] > 0)
		    break;
	    }
	    min = (rz - i) * ew->az * dec_rate;
	}
	else
	    min = rz * ew->az * dec_rate;
	camin = min;
	rg = x * rz;
	thr = 10;
	if (rg < minr)
	    thr += 10;
	st_r = ca_r;
	if (st_r == 0)
	    st_r = minr;
	if (rg > st_r) {
	    int dec = 30. * (rg - st_r) / st_r;
	    if (dec > 10)
		dec = 10;
	    thr -= dec;
	    if (rg <= 2 * st_r)
		min = (double)min / (1. + (double)(rg - st_r) / st_r);
	    else
		min = min / 2;
	}
	if (min < 4)
	    min = 4;
	if (camin < 20)
	    camin = 20;
	for (y = 0; y < n_azs; y++) {
	    int off = y * n_rgs + x;
	    if (x >= ca_r * 2)
		efs[off] |= EF_STORM;
	    else if (ewc[off] >= min && ewz[off] / ewc[off] > thr)
		efs[off] |= EF_STORM;
	    else if (ewc[off] >= camin && x * rz < 2 * ca_r)
		efs[off] |= EF_CLEAR_AIR;
	    if (Second_trip_count[off] >= camin)
		efs[off] |= EF_SECOND_TRIP;
	    if (!(efs[off] & EF_STORM))
		efs[off] &= ~EF_HS_GRID;
	}
    }

    for (x = 0; x < ew->n_rgs; x++) {	/* set CLEAR_AIR */
	if (x * rz >= ca_r)
	    break;
	ew->rfs[x] |= RF_CLEAR_AIR;
    }

    free (ewz);

    return (s_r);
}

unsigned char *PP_get_med_v () {
    return (Med_v);
}

/***************************************************************************

***************************************************************************/

static void Generate_ewv (Vdeal_t *vdv) {
    Ew_struct_t *ew;
    int n_rgs, rz, ey;
    int *dt = NULL;

    ew = &(vdv->ew);
    rz = ew->rz;
    n_rgs = ew->n_rgs;

    Med_v = (unsigned char *)STR_reset (Med_v,
					n_rgs * ew->n_azs * sizeof (char));
    for (ey = 0; ey < ew->n_azs; ey++) {
	int miny, maxy, y, ex, min_cnt, nyq;

	miny = maxy = -1;
	for (y = 0; y < vdv->yz; y++) {
	    if (vdv->ew_aind[y] == ey) {
		if (miny < 0)
		    miny = y;
		maxy = y;
	    }
	    else if (maxy >= 0)
		break;
	}
	nyq = VDD_get_nyq (vdv, (miny + maxy) / 2);

	dt = (int *)STR_reset (dt, rz * (maxy - miny + 1) * sizeof (int));
	min_cnt = (maxy - miny + 1) * rz / 10;
	for (ex = 0; ex < n_rgs; ex++) {
	    int xs, xe, cnt, max_d[3];

	    xs = ex * rz;
	    xe = xs + rz;
	    if (xe >= vdv->xz)
		xe = vdv->xz;
	    cnt = 0;
	    for (y = miny; y <= maxy; y++) {
		int x;
		unsigned char *inp = vdv->inp + y * vdv->xz;
		for (x = xs; x < xe; x++) {
		    if (inp[x] != BNO_DATA) {
			dt[cnt] = inp[x];
			cnt++;
		    }
		}
	    }
	    if (cnt > 0 && ex > 0) {
		Med_v[ey * n_rgs + ex] = VDA_search_median_value 
			(dt, cnt, nyq, vdv->data_off, max_d);
		if (max_d[2] > 3 * nyq / 2)
		    ew->efs[ey * n_rgs + ex] |= EF_HS_GRID;
	    }
	    if (cnt <= min_cnt || ex == 0) /* we discard the near radar data */
		Med_v[ey * n_rgs + ex] = BNO_DATA;
	}
    }
    STR_free (dt);
}

/***************************************************************************

    Computes and returns the maxmum range of clear air return and nearest
    range of storm.

***************************************************************************/

static int Get_clear_air_and_storm_range (Vdeal_t *vdv,
				short *lz_cnt, short *hzw_cnt, int *s_rp) {
    int max, maxi, xz, yz, i;
    int ca_r, s_r, minr, maxr;
    double elev;

    max = 0;
    maxi = 0;
    xz = vdv->xz;
    yz = vdv->yz;
    elev = vdv->elev;
    if (elev < .5)
	elev = .5;
    minr = VDV_alt_to_range (vdv, 400.) / vdv->g_size;
    maxr = VDV_alt_to_range (vdv, 5000.) / vdv->g_size;
    if (maxr > yz)
	maxr = yz;
    for (i = minr; i < maxr; i++) {
	if (lz_cnt[i] > max) {
	    max = lz_cnt[i];
	    maxi = i;
	}
    }

    if (lz_cnt[maxi] * 10 < yz) {	/* too few low z - no ca found */
	ca_r = 0;
    }
    else {
	for (i = maxi; i < xz; i++) {
	    if (lz_cnt[i] * 3 < max)
		break;
	}
	ca_r = i;
	if (ca_r >= maxr)
	    ca_r = 0;
    }

    s_r = xz - 1;
    for (i = minr; i < xz; i++) {
	int thr = 10000. * rad2deg / (i * vdv->g_size * vdv->gate_width);
				/* significant storm size is 10000 meters */
	if (i < ca_r)
	    thr *= 2;
	if (thr < 4)
	    thr = 4;
	if (thr > yz / 2)
	    thr = yz / 2;
	if (hzw_cnt[i] >= thr) {
	    s_r = i;
	    break;
	}
    }
    if (s_r == minr)
	s_r = 0;

    *s_rp = s_r;
    return (ca_r);
}

/***************************************************************************

    Removes high shear gates in "inp" of sizes "xz" and "yz". "thr" is
    the threshold for identifying the high shear borders. "ethr", must
    be smaller than "thr", is used for edge pointes. "fp" indicates that 
    inp is full ppi.

***************************************************************************/

void PP_remove_high_shear_gates (unsigned char *inp, int xz, int yz, 
			int thr, int ethr, int fp, int nyq) {
    unsigned char *cr, *map, emask;
    int max_cnt, thrl, thrh, ethrl, ethrh, edge_width;
    int xz1, x, y, i, k, off[16], *ofs, inc;

    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
					xz * yz * sizeof (unsigned char));
    memset (Wbuf, 0, xz * yz * sizeof (unsigned char));
    xz1 = xz - 1;
    max_cnt = 0;
    thrl = thr;
    ethrl = ethr;		/* must <= thrl */
    thrh = 2 * nyq - thrl;
    ethrh = 2 * nyq - ethrl;

    edge_width = 2;	/* edge depth */
    for (i = 0; i < edge_width; i++) {	/* mark edge points (lower 4 bits) */
	for (y = 0; y < yz; y++) {

	    cr = inp + y * xz;
	    map = Wbuf + y * xz;
	    VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	    for (x = 0; x < xz; x++) {
		int m = map[x];
		if (cr[x] == BNO_DATA || (m > 0 && m <= i))
		    continue;
		if (x == 0)
		    ofs = off + 4;
		else if (x == xz1)
		    ofs = off + 8;
		else
		    ofs = off;
		for (k = 0; k < 4; k++) {
		    int of = x + ofs[k];
		    m = map[of];
		    if (cr[of] == BNO_DATA || (m > 0 && m <= i) ||
							ofs[k] == 0) {
			map[x] = i + 1;		/* i-th level border gates */
			break;
		    }
		}
	    }
	}
    }

    inc = 1 << 4;
    emask = 0xf;
    for (y = 0; y < yz; y++) {	/* upper 4 bits for high shear border cnt */

	cr = inp + y * xz;
	map = Wbuf + y * xz;
	VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	for (x = 0; x < xz; x++) {
	    unsigned int e;
	    int ct, c;

	    c = cr[x];
	    if (c == BNO_DATA)
		continue;
	    if (x == 0)
		ofs = off + 4;
	    else if (x == xz1)
		ofs = off + 8;
	    else
		ofs = off;
	    e = map[x] & emask;
	    ct = 0;
	    for (k = 0; k < 4; k++) {
		int n, diff, of;
		of = x + ofs[k];
		n = cr[of];
		if (n == BNO_DATA)
		    continue;
		diff = n - c;
		if (diff < 0)
		    diff = -diff;
		if (diff < ethrl || diff > ethrh)
		    continue;
		if (!e && !(map[of] & emask) &&
		    			(diff < thrl || diff > thrh))
		    continue;
		map[x] += inc;
		ct++;
		if (ct > max_cnt)
		    max_cnt = ct;
	    }
	}
    }

    for (i = max_cnt; i >= 1; i--) {	/* remove high shear data */
	for (y = 0; y < yz; y++) {

	    cr = inp + y * xz;
	    map = Wbuf + y * xz;
	    VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	    for (x = 0; x < xz; x++) {

		if ((map[x] >> 4) == i) {
		    map[x] = 0;
		    if (x == 0)
			ofs = off + 4;
		    else if (x == xz1)
			ofs = off + 8;
		    else
			ofs = off;
		    for (k = 0; k < 4; k++) {
			int of, scnt;
			of = x + ofs[k];
			scnt = map[of] >> 4;
			if (scnt > 0)
			    map[of] = (scnt - 1) << 4;
		    }
		    cr[x] = BNO_DATA;
		}
	    }
	}
    }
}

/***************************************************************************

    Removes noisy data in unreliable areas - in thin connecting areas,
    near missing gates and on region edges. The image is thinned by
    removing any of the gate that is not smooth to its neighbors. A gate
    is subject to be thinned if it has a missing 4-neighbor, there are
    at least min_md n8-neighbor missing gates and the maximim deviation
    from the median value of this and its 8-neighbors is less than a
    threshold. The thining is performed "level" times. "fp" indicates 
    that inp is full ppi.

***************************************************************************/

int PP_remove_noisy_data (unsigned char *inp, int xz, int yz, int level,
	int min_md, int thr, int fp, Data_filter_t *dft,
	unsigned char *outmap, unsigned char mapv, int nyq, int d_off) {
    int v, xz1, i, x, y, g_cnt;
    unsigned char *map, *cr, *out;
    unsigned char ybits, ebits, *dmap, *dmapp;

    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
					xz * yz * sizeof (unsigned char));
    if (dft != NULL) {
	dmap = dft->map;
	ybits = dft->yes_bits;
	ebits = dft->exc_bits;
    }
    else {
	dmap = NULL;
	ybits = ebits = 0;
    }
    dmapp = NULL;
    xz1 = xz - 1;
    v = 200;				/* map value for data available */
    for (y = 0; y < yz; y++) {		/* init the map */
	cr = inp + y * xz;
	map = Wbuf + y * xz;
	if (dmap != NULL)
	    dmapp = dmap + y * xz;
	for (x = 0; x < xz; x++) {
	    unsigned char d = cr[x];

	    if (dmap != NULL) {
		unsigned char m = dmapp[x];
		if (ybits) {
		    if (!(m & ybits))
			d = BNO_DATA;
		}
		if (ebits) {
		    if (m & ebits)
			d = BNO_DATA;
		}
	    }
	    if (d == BNO_DATA)
		map[x] = 0;		/* no data */
	    else
		map[x] = v;		/* data */
	}
    }

    for (i = 0; i < level; i++) {	/* thin the regions */
	for (y = 0; y < yz; y++) {
	    int off4[16], off8[32], d[16], cnt, max_diff[3];

	    map = Wbuf + y * xz;
	    cr = inp + y * xz;
	    VDA_get_neighbor_offset (4, y, xz, yz, fp, off4);
	    VDA_get_neighbor_offset (8, y, xz, yz, fp, off8);
	    for (x = 0; x < xz; x++) {
		int k, *ofs;
		if (map[x] <= i)
		    continue;
		if (x == 0)
		    ofs = off4 + 4;
		else if (x == xz1)
		    ofs = off4 + 8;
		else
		    ofs = off4;
		for (k = 0; k < 4; k++) {
		    if (ofs[k] == 0 || map[x + ofs[k]] <= i)
			break;
		}
		if (k >= 4)			/* not close to no-data */
		    continue;

		if (x == 0)
		    ofs = off8 + 8;
		else if (x == xz1)
		    ofs = off8 + 16;
		else
		    ofs = off8;
		cnt = 0;
		for (k = 0; k < 8; k++) {
		    int off;
		    if (ofs[k] == 0)
			continue;
		    off = x + ofs[k];
		    if (map[off] >= i && cr[off] != BNO_DATA) {
			d[cnt] = cr[off];
			cnt++;
		    }
		}
		if (cnt <= 0 || cnt >= 8 - min_md)
		    continue;
		if (cr[x] != BNO_DATA) {
		    d[cnt] = cr[x];
		    cnt++;
		}
		VDA_search_median_value (d, cnt, nyq, d_off, max_diff);
		if (max_diff[0] < thr)
			continue;
		map[x] = i + 1;
	    }
	}
    }

    g_cnt = 0;
    for (y = 0; y < yz; y++) {		/* output the found gates */
	out = cr = NULL;
	if (outmap != NULL)
	    out = outmap + y * xz;
	else
	    cr = inp + y * xz;
	map = Wbuf + y * xz;
	for (x = 0; x < xz; x++) {
	    int m = map[x];
	    if (m <= level && m > 0) {
		if (outmap != NULL)
		    out[x] = out[x] | mapv;
		else
		    cr[x] = BNO_DATA;
		g_cnt++;
	    }
	}
    }
    return (g_cnt);
}

/***************************************************************************

    Fills in small gaps in "vdv->inp" with the median value of the
    surrounding data. The gap must be small (<= max_gap) and the
    surounding gates must be close (thr). Bit DMAP_FILL is set in
    vdv->dmap for filled gates. We here assume BNO_DATA = 0. We first
    identify the areas where the data is dense and gap-filling will be
    applied. Vertical gap bars are filled first and horizontal gap bars
    are filled after.

***************************************************************************/

static void Vertical_fill (unsigned char *inp, int xz, int yz, 
		unsigned char *buf, int x, int y, int cnt, int *yof, 
				int thr, int no_v, int nyq, int d_off);
static void Horizontal_fill (unsigned char *inp, int xz, int yz, 
		unsigned char *buf, int x, int y, int cnt, int *yof,
			int thr, int no_v, int nyq, int d_off);
static void Set_filling_area (Vdeal_t *vdv, int level, int fp,
				unsigned char *inp, unsigned char *mapbuf);

#define MAX_F_GAP 10

void PP_fill_in_gaps (Vdeal_t *vdv, unsigned char *inp, int xz, int yz,
				int level, int fp, int max_gap, int nyq) {
    int xyz, x, y, thr, no_v;
    unsigned char *dmap, *mapbuf, *buf;

    xyz = xz * yz;
    Wbuf = (unsigned char *)STR_reset (Wbuf, 2 * xyz * sizeof (unsigned char));
    mapbuf = Wbuf;
    buf = Wbuf + xyz;

    thr = nyq / 4;
    if (max_gap > MAX_F_GAP - 2)
	max_gap = MAX_F_GAP - 2;
    if (vdv->gate_width >= .7f)
	max_gap = max_gap * 5 / 8;
    dmap = NULL;
    if (inp == NULL) {
	inp = vdv->inp;
	dmap = vdv->dmap;
    }

    if (level > 0)
	Set_filling_area (vdv, level, fp, inp, mapbuf);
    else {
	for (x = 0; x < xyz; x++) {
	    if (inp[x] == BNO_DATA)
		mapbuf[x] = 1;	/* no data - subject to fill */
	    else
		mapbuf[x] = 2;	/* data */
	}
    }

    memset (buf, 0, xyz);
    no_v = 0xffffff;
    for (y = 0; y < yz; y++) {	/* vertical filling */
	int i, yof[MAX_F_GAP];
	for (i = 0; i < max_gap + 2; i++) {
	    int yy = y + i;
	    if (fp)
		yy = yy % yz;
	    else if (yy >= yz)
		yy = no_v;
	    if (yy == no_v)
		yof[i] = no_v;
	    else
		yof[i] = yy * xz;
	}
	for (x = 2; x < xz - 2; x++) {
	    int cnt, m;
	    unsigned char *map = mapbuf + x;
	    if (map[yof[0]] != 2 || yof[1] == no_v || map[yof[1]] != 1)
		continue;
	    cnt = 1;
	    for (i = 2; i < max_gap + 2; i++) {
		if (yof[i] == no_v)
		    break;
		m = map[yof[i]];
		if (m == 1) {
		    cnt++;
		    continue;
		}
		else if (m == 0)
		    break;
		Vertical_fill (inp, xz, yz, buf, x, y, cnt, yof, thr,
						no_v, nyq, vdv->data_off);
		break;
	    }
	}
    }
    for (x = 0; x < xyz; x++) {
	if (buf[x] != 0) {
	    inp[x] = buf[x];
	    if (dmap != NULL)
		dmap[x] |= DMAP_FILL;
	    mapbuf[x] = 2;
	    buf[x] = 0;
	}
    }

    for (y = 0; y < yz; y++) {	/* horizontal filling */
	int i, yof[MAX_F_GAP];
	for (i = 0; i <= 2; i++) {
	    int yy = y + i - 1;
	    if (fp)
		yy = (yy + yz) % yz;
	    else if (yy < 0 || yy >= yz)
		yy = no_v;
	    if (yy == no_v)
		yof[i] = no_v;
	    else
		yof[i] = yy * xz;
	}
	for (x = 0; x < xz - 1; x++) {
	    int cnt, m;
	    unsigned char *map = mapbuf + yof[1] + x;
	    if (map[0] != 2 || map[1] != 1)
		continue;
	    cnt = 1;
	    for (i = 2; i < max_gap + 2; i++) {
		if (x + i >= xz)
		    break;
		m = map[i];
		if (m == 1) {
		    cnt++;
		    continue;
		}
		else if (m == 0)
		    break;
		Horizontal_fill (inp, xz, yz, buf, x, y, cnt, yof, thr, 
						no_v, nyq, vdv->data_off);
		break;
	    }
	}
    }
    for (x = 0; x < xyz; x++) {
	if (buf[x] != 0) {
	    inp[x] = buf[x];
	    if (dmap != NULL)
		dmap[x] |= DMAP_FILL;
	}
    }

    if (level == 0)
	Fill_small_gaps (vdv, inp, xz, yz, fp, 1, nyq);
    else
	Fill_small_gaps (vdv, NULL, xz, yz, fp, 2, nyq);

    /* remove filled GCC */
    {
	int cnt, i;
	Gate_t *gccgs;
	cnt = CD_get_saved_gcc_gate (&gccgs);
	for (i = 0; i < cnt; i++) {
	    vdv->inp[gccgs->y * xz + gccgs->x] = BNO_DATA;
	    gccgs++;
	}
    }
}

/****************************************************************************

    Identify areas where filling will be applied.

****************************************************************************/

static void Set_filling_area (Vdeal_t *vdv, int level, int fp,
				unsigned char *inp, unsigned char *mapbuf) {
    int xz, yz, xyz, xz1, v, x, y;
    unsigned char *map;

    xz = vdv->xz;
    yz = vdv->yz;
    xyz = xz * yz;

    inp = vdv->inp;
    v = 31;
    for (y = 0; y < yz; y++) {		/* init the no-data map */

	unsigned char *cr = inp + y * xz;
	map = mapbuf + y * xz;
	for (x = 0; x < xz; x++) {
	    unsigned char d = cr[x];
	    if (d == BNO_DATA)
		map[x] = v;		/* no data */
	    else
		map[x] = 0;		/* data */
	}
    }

    VDA_thin_and_dialate (mapbuf, xz, yz, fp, v, level);

    xz1 = xz - 1;
    for (y = 0; y < yz; y++) {	/* mark breaking gates */
	int off[32];
	unsigned char mask = 0x1f;
	VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	map = mapbuf + y * xz;
	for (x = 0; x < xz; x++) {
	    int m = map[x] & mask;
	    if (m > level)
		map[x] |= 32;		/* all other data gates */
	    else if (m > 0) {
		int *ofs, k;
		if (x == 0)
		    ofs = off + 4;
		else if (x == xz1)
		    ofs = off + 8;
		else
		    ofs = off;
		for (k = 0; k < 4; k++) {
		    if (ofs[k] != 0 && (map[x + ofs[k]] & mask) > level)
			break;
		}
		if (k < 4)
		    map[x] |= 128;	/* breaking gates */
		else
		    map[x] |= 64;	/* thined gates */
	    }
	}
    }
    for (x = 0; x < xyz; x++) {
	if (mapbuf[x] & 64)
	    mapbuf[x] = 1;	/* no data - subject to fill */
	else if (mapbuf[x] & (128 | 32))
	    mapbuf[x] = 0;	/* no data */
	else
	    mapbuf[x] = 2;	/* data */
    }
}

/***************************************************************************

    Checks all data surrounding a vertical to-be-filled bar area of
    cnt size and starting at (x, y). inp is the input data and buf to
    hold the filled data. yof is the offsets of the gates on the bar.

***************************************************************************/

static void Vertical_fill (unsigned char *inp, int xz, int yz, 
		unsigned char *buf, int x, int y, int cnt, int *yof,
				int thr, int no_v, int nyq, int d_off) {
    int c, yy, d[MAX_F_GAP * 3], maxd[3], med;

    c = 0;
    for (yy = 0; yy < cnt + 2; yy++) {	/* get surrounding data */
	int step, of, i;
	if (yy == 0 || yy == cnt + 1)
	    step = 1;
	else
	    step = 2;
	if (yof[yy] == no_v)
	    continue;
	of = yof[yy] + x;
	for (i = -1; i <= 1; i += step) {
	    if (inp[of + i] == BNO_DATA)
		continue;
	    d[c] = inp[of + i];
	    c++;
	}
    }
    if (c <= 4)
	return;

    med = VDA_search_median_value (d, c, nyq, d_off, maxd);
    if (maxd[1] > thr)
	return;

    for (yy = 1; yy < cnt + 1; yy++) {	/* fill in data */
	if (yof[yy] != no_v)
	    buf[yof[yy] + x] = med;
    }
}

/***************************************************************************

    Checks all data surrounding a horizontal to-be-filled bar area of
    cnt size and starting at (x, y). inp is the input data and buf to
    hold the filled data. yof is the offsets of the gates on the bar.

***************************************************************************/

static void Horizontal_fill (unsigned char *inp, int xz, int yz, 
		unsigned char *buf, int x, int y, int cnt, int *yof, 
				int thr, int no_v, int nyq, int d_off) {
    int c, yy, xx, d[MAX_F_GAP * 3], maxd[3], med;

    c = 0;
    for (yy = 0; yy < 3; yy++) {	/* get surrounding data */
	int step, of, i;
	if (yy == 1)
	    step = cnt + 1;
	else
	    step = 1;
	if (yof[yy] == no_v)
	    continue;
	of = yof[yy] + x;
	for (i = 0; i <= cnt + 1; i += step) {
	    if (inp[of + i] == BNO_DATA)
		continue;
	    d[c] = inp[of + i];
	    c++;
	}
    }
    if (c <= 4)
	return;

    med = VDA_search_median_value (d, c, nyq, d_off, maxd);
    if (maxd[1] > thr)
	return;

    for (xx = 1; xx < cnt + 1; xx++) {	/* fill in data */
	if (yof[1] != no_v)
	    buf[yof[1] + x + xx] = med;
    }
}

/***************************************************************************

    Fills in small data gaps in inp with the median value of its
    nearby data if the latter are approximately the same. The procedure
    is performed "level" times.

***************************************************************************/

static void Fill_small_gaps (Vdeal_t *vdv, unsigned char *inp,
				int xz, int yz, int fp, int level, int nyq) {
    int y, x, thr3, thr4, d_off, it;
    unsigned char *dmap;

    dmap = NULL;
    if (inp == NULL) {
	inp = vdv->inp;
	dmap = vdv->dmap;
    }
    thr3 = nyq / 3;
    thr4 = nyq / 4;
    d_off = vdv->data_off;

    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
					xz * yz * sizeof (unsigned char));
    for (it = 0; it < level; it++) {

	memcpy (Wbuf, inp, xz * yz * sizeof (unsigned char));
	for (y = 0; y < yz; y++) {
	    int ofs[32];
	    unsigned char *in;

	    in = Wbuf + y * xz;
	    VDA_get_neighbor_offset (4, y, xz, yz, fp, ofs);
	    for (x = 1; x < xz - 1; x++) {
		int cnt, i, md, max_df[3], dd[8], thr;
    
		if (in[x] != BNO_DATA)
		    continue;

		cnt = 0;
		for (i = 0; i < 4; i++) {
		    int of, v;
		    of = ofs[i];
		    if (of != 0 && (v = in[x + of]) != BNO_DATA) {
			dd[cnt] = v;
			cnt++;
		    }
		}
		if (cnt < 2)
		    continue;
		if (cnt == 2)
		    thr = thr4;
		else
		    thr = thr3;
		md = VDA_search_median_value (dd, cnt,
					nyq, vdv->data_off, max_df);
		if (max_df[1] >= thr)
		    continue;
		if (dmap != NULL)
		    dmap[y * xz + x] |= DMAP_FILL;
		inp[y * xz + x] = md;
	    }
	}
    }
}
