
/**********************************************************************

    vdeal's functions for 2D dealiasing.

**********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/22 21:58:14 $
 * $Id: vdeal_2d.c,v 1.8 2014/08/22 21:58:14 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <infr.h>

#include "vdeal.h"

static __thread Params_t *Parms;
static __thread int Nyq;
static __thread int Fppi = 0;

typedef struct {
    short xi, yi, xo, yo;
    short dcnt;
} Bd_point;

/* items used for region border condition setup */
#define BD_UNALIASED 0x80
#define BD_PUNALIASED 0x40
#define BD_REACHED 0x20
static __thread unsigned char *Bd_map;	/* border map of unaliased points */
static __thread int Apply_bc = 0;	/* applying border conditions */

/* vars used for Travel_border only */
#define BD_MAX_TRVS 8192
static __thread short *Tb_carea;
static __thread int Tb_depth, Tb_cnt, Tb_xz, Tb_yz, Tb_trvcnt, \
		Tb_vh_rg, Tb_h_rg, Tb_pv, Tb_gcnt, TB_failed;
static __thread Point_t *Tb_trvd;

static int Quantize_solution (int ng, Banbks_t *x, char *dvq, 
			unsigned char *bgf, unsigned char *xrcnt);
static int Form_2D_and_solve (int trans, short *carea, int stride,
		int *indx, int xz, int yz, Banbks_t **bp, int *n_gates);
static int Create_vindex (int xz, int yz, short *carea, 
					int stride, int trans, int *indx);
static int Create_vr_vindex (int xz, int yz, unsigned char *rmap, 
					int nred, int *indx, float *bw_red);
static int Set_critical_gates (int xz, int yz, unsigned char *rmap);
static int Set_hsg_map (short *carea, int xs, int ys, int trans, int stride,
				int lthr, int hthr, unsigned char *rmapp);
static Banbks_t Verify_vred (int *indx, int xz, int yz, Banbks_t *x);
static int Construct_a_and_b (int xz, int yz, int bw, int trans,
		int stride, short *carea, Sp_matrix **sap, Banbks_t **bp, 
		int ng, int *indx);
static int Need_dealiase (short *region, int xsize, int ysize);
static int Count_repeated_x (int xz, int yz, int ng, unsigned char *rmap, 
						int *indx);
static unsigned char *Get_shared_buffer (int size);
static int Increase_buf_size (int len, Spmcg_t **vp, int **ip);
static void Post_process_bh (short *carea, int xz, int yz, short *sol,
		char *qsl, Banbks_t *b, char *dvq, unsigned char *bgf);
static void Set_external_map (int x, int y, int cnt, int xz, int yz,
						unsigned char *map);
static int Search_two_peaks (int nyq, int *h, int *exi);
static int Moving_average (int nyq, int *h, int w, float *ah, float *maxp);
static int Windowed_search (int nyq, int *h);
static void Find_peak_edges (int nyq, float *ah, int maxi, float min_thr,
					int w, int *stp, int *endp);
static int Get_border (Region_t *region, unsigned char *pmp, int xz, int yz,
					Bd_point *bds, int *dbcp);
static int Re_quantize_bh (short *carea, int xz, int yz, unsigned char *pmp,
	short *sol, char *qsl, Banbks_t *b, char *dvq, unsigned char *bgf,
	void **rgs, unsigned char **d_buf, int clump_flag);

static int Quantize_solution1 (int ng, Banbks_t *x, char *dvq, 
				unsigned char *bgf, unsigned char *xrcnt);
static int Search_nearby_min (int *h, int st, int rg);
static void Travel_border (int x, int y);
static int Border_analysys (short *carea, int xsize, int ysize);

/**************************************************************************

    Applies the 2D dealiasing algorithm to region "carea" of "xsize" by 
    "ysize". The missing data is SNO_DATA. "dmap" is the dealiasing map 
    with "stride". "n_gates" is the number of data gates in the region. 
    "dmap" of "stride", if not NULL, flags BH and BD are set.

***************************************************************************/

int VD2D_2d_dealiase (short *carea, Params_t *parms, int n_gates,
	int xsize, int ysize, short *bd_dfs) {
    static __thread int *indx = NULL;
    static __thread int indx_size = 0, bgf_size = 0;
    static __thread unsigned char *bgf = NULL;
    int trans, xz, yz;
    Banbks_t *b;
    int i, j;
    int ng, vn2, bh_cnt;
    char *dvq, *qsl;
    unsigned char *xrcnt;
    short *sol;
    extern int dbg;

    Parms = parms;
    Nyq = Parms->nyq;
    Fppi = Parms->fppi;		/* the region is of full ppi */

    if (xsize * ysize > indx_size) {
	if (indx != NULL)
	    free (indx);
	indx_size = xsize * ysize;
	indx = (int *)MISC_malloc (indx_size * (sizeof (int) +
					sizeof (short) + sizeof (char)));
    }
    sol = (short *)(indx + indx_size);
    qsl = (char *)(sol + indx_size);

    /* A better way of determine trans will need to find the max numbers of 
       available gates of all rows and columns */
    if (xsize > ysize && !Fppi) {
	trans = 1;
	xz = ysize;
	yz = xsize;
    }
    else {		/* We don't do trans if is_tbc */
	xz = xsize;
	yz = ysize;
	trans = 0;
    }

    if (!Need_dealiase (carea, xsize, ysize))
	return (0);

    Apply_bc = 0;
    if (Parms->use_bc)
	Apply_bc = Border_analysys (carea, xsize, ysize);

    ng = Form_2D_and_solve (trans, carea, xsize, indx, xz, yz, &b, &n_gates);
    if (ng < 0) {
	return (-1);
    }
    if (ng == 0)		/* done - there is no aliasing */
	return (0);
    
    if (n_gates > bgf_size) {
	if (bgf != NULL)
	    free (bgf);
	bgf_size = n_gates;
	bgf = (unsigned char *)MISC_malloc (bgf_size * 2);
    }
    dvq = (char *)bgf + n_gates;

    if (n_gates > ng) {
	xrcnt = Get_shared_buffer (ng);
	Count_repeated_x (xz, yz, ng, xrcnt, indx);
    }
    else
	xrcnt = NULL;
    
    if (Apply_bc)
	bh_cnt = Quantize_solution1 (ng, b, dvq, bgf, xrcnt);
    else
	bh_cnt = Quantize_solution (ng, b, dvq, bgf, xrcnt);
    if (bh_cnt < 0)
	return (-1);

    vn2 = Nyq * 2;
    if ((bh_cnt > 0 && ((Vdeal_t *)Parms->vdv)->low_prf) || dbg) {
	short *slf = NULL;
	if (dbg) slf = MISC_malloc (xsize * ysize * sizeof (short));
	for (j = 0; j < yz; j++) {
	    int iind;
	
	    iind = j * xz - 1;		/* index of the current gate indx */
	    for (i = 0; i < xz; i++) {
		int n, dind;
	
		iind++;
		n = indx[iind];		/* x index of the current gate */
	
		if (trans)
		    dind = i * xsize + j;	/* index of the current gate */
		else
		    dind = j * xsize + i;
		if (dbg) slf[dind] = SNO_DATA;
		sol[dind] = SNO_DATA;
		if (n < 0)
		    continue;
		if (dbg) slf[dind] = b[n] + 129.5;
		sol[dind] = carea[dind] + (int)dvq[n] * vn2;
	    }
	}
	if (bh_cnt > 0) {
	    int nhs, xy[2], i;
	    nhs = VDA_detect_false_hs (sol, carea, xsize, ysize, Parms->fppi,
						3 * Parms->nyq / 2, xy);
	    if (nhs == 0 || (xy[0] * Parms->xr < 30 && 
						xy[1] * Parms->yr < 15)) {
		for (i = 0; i < ng; i++)	/* remove BH flags */
		    bgf[i] = 0;
	    }
	}
	if (dbg) {
	    dump_simage ("sol", slf, xsize, ysize, xsize);
	    free (slf);
	}
    }

    /* dealiase the image and set _BH in dmap */
    for (j = 0; j < yz; j++) {
	int iind;

	iind = j * xz - 1;		/* index of the current gate indx */
	for (i = 0; i < xz; i++) {
	    int n, dind, dmind;

	    iind++;
	    n = indx[iind];		/* x index of the current gate */
	    if (n < 0)
		continue;

	    if (trans)
		dind = i * xsize + j;	/* index of the current gate */
	    else
		dind = j * xsize + i;
	    if (bh_cnt > 0) {
		qsl[dind] = dvq[n];
		if (b[n] > 0.)
		    sol[dind] = b[n] + .5;
		else
		    sol[dind] = -((int)(-b[n] + .5));
	    }
	    else
		carea[dind] += (int)dvq[n] * vn2;
	    if (Parms->dmap != NULL && (bgf[n] & DMAP_BH)) {
					/* move DMAP_BH from bfg to dmap */
		if (trans)
		    dmind = i * Parms->stride + j;
		else
		    dmind = j * Parms->stride + i;
		Parms->dmap[dmind] |= DMAP_BH;
	    }
	}
    }

    if (bh_cnt > 0)
	Post_process_bh (carea, xsize, ysize, sol, qsl, b, dvq, bgf);

    return (0);
}

/*************************************************************************

    Sets dmap bit of BH for all gates that are not connected to the largest
    region of non-BH. If strict_bh is false, _bh regions are re-quantized
    or left as marked as BH.

*************************************************************************/

/* values for pmp - porcessing map */
#define PMP_NON_BH 50		/* not _bh flagged gates */
#define PMP_PH 100		/* _bh flagged gates */
#define PMP_EXTN 150		/* gates external to the entire region */
#define MAX_BDS 2048

static void Post_process_bh (short *carea, int xz, int yz, short *sol,
		char *qsl, Banbks_t *b, char *dvq, unsigned char *bgf) {
    static __thread void *rgs = NULL;
    static __thread unsigned char *pmp = NULL, *d_buf = NULL;
    int stride, clump_flag, n_regions, i, x, y, nyq2;
    unsigned char *dmap;
    Region_t region;

    if (Parms->dmap == NULL)
	return;
    dmap = Parms->dmap;
    stride = Parms->stride;

    /* set pmp (temporary processing map) */
    pmp = (unsigned char *)STR_reset ((char *)pmp, xz * yz);
    for (y = 0; y < yz; y++) {
	unsigned char *dm, *mp;
	short *sd = carea + (y * xz);
	mp = pmp + (y * xz);
	dm = dmap + (y * stride);
	for (x = 0; x < xz; x++) {
	    if (sd[x] == SNO_DATA || (dm[x] & DMAP_BH))
		mp[x] = BNO_DATA;
	    else
		mp[x] = PMP_NON_BH;	/* for valid data excluding _BH */
	    dm[x] &= ~DMAP_LOCAL;	/* clean up this bit to be used */
	}
    }

    clump_flag = VDC_IDR_SIZE;
    if (Fppi)
	clump_flag |= VDC_IDR_WRAP;
    n_regions = VDC_identify_regions (pmp, NULL, xz, 0, 0,
				xz, yz, NULL, clump_flag, &rgs);

    for (i = 1; i < n_regions; i++) {	/* set BH all but the first */
	int n_gs;

	region.data = NULL;
	n_gs = VDC_get_next_region (rgs, i, &region);
	if (n_gs <= 0)
	    break;
	d_buf = (unsigned char *)STR_reset ((char *)d_buf,
				region.xz * region.yz * sizeof (short));
	region.data = (short *)d_buf;
	n_gs = VDC_get_next_region (rgs, i, &region);
	for (y = 0; y < region.yz; y++) {
	    short *d = region.data + y * region.xz;
	    unsigned char *dm = dmap + ((y + region.ys) % yz) * stride + 
								region.xs;
	    for (x = 0; x < region.xz; x++) {
		if (d[x] != SNO_DATA)
		    dm[x] |= DMAP_BH;
	    }
	}
    }

    if (Parms->strict_bh == 0) {
	while (Re_quantize_bh (carea, xz, yz, pmp, sol, qsl, b, dvq, bgf, &rgs,
				&d_buf, clump_flag) >= 10);
    }

    nyq2 = Parms->nyq * 2;
    for (y = 0; y < yz; y++) {
	unsigned char *dm = dmap + (y * stride);
	short *sd = carea + (y * xz);
	char *qs = qsl + (y * xz);
	for (x = 0; x < xz; x++) {
	    if (sd[x] == SNO_DATA)
		continue;
	    sd[x] += qs[x] * nyq2;
	    if (dm[x] & DMAP_LOCAL)
		dm[x] = (dm[x] & (~DMAP_LOCAL)) | DMAP_BH;
	}
    }
}

/*************************************************************************

    Processes BH in carea. Returns the _BH count after the re-quantization.

*************************************************************************/

static int Re_quantize_bh (short *carea, int xz, int yz, unsigned char *pmp,
	short *sol, char *qsl, Banbks_t *b, char *dvq, unsigned char *bgf,
	void **rgs, unsigned char **d_buf, int clump_flag) {
    int stride, n_regions, i, x, y, nyq, nyq2, data_off, bh_cnt;
    unsigned char *dmap;
    Region_t region;

    /* set pmp (temporary processing map) */
    dmap = Parms->dmap;
    stride = Parms->stride;
    for (y = 0; y < yz; y++) {
	unsigned char *dm, *mp;
	short *sd = carea + (y * xz);
	mp = pmp + (y * xz);
	dm = dmap + (y * stride);
	for (x = 0; x < xz; x++) {
	    if (sd[x] == SNO_DATA || !(dm[x] & DMAP_BH))
		mp[x] = BNO_DATA;
	    else
		mp[x] = PMP_PH;		/* _BH gates */
	}
    }

    n_regions = VDC_identify_regions (pmp, NULL, xz, 0, 0,
				xz, yz, NULL, clump_flag, rgs);

    /* set good data (non-_BH) to pmp = PMP_NON_BH */
    for (y = 0; y < yz; y++) {
	unsigned char *dm, *mp;
	short *sd = carea + (y * xz);
	mp = pmp + (y * xz);
	dm = dmap + (y * stride);
	for (x = 0; x < xz; x++) {
	    if (sd[x] != SNO_DATA && !(dm[x] & (DMAP_BH | DMAP_LOCAL)))
		mp[x] = PMP_NON_BH;
	}
    }

    /* set pmp = PMP_EXTN for external gates */
    for (y = 0; y < yz; y++) {
	if (!Fppi && (y == 0 || y == yz - 1)) {
	    for (x = 0; x < xz; x++)
		Set_external_map (x, y, 0, xz, yz, pmp);
	}
	else {
	    Set_external_map (0, y, 0, xz, yz, pmp);
	    Set_external_map (xz - 1, y, 0, xz, yz, pmp);
	}
    }

    nyq = Parms->nyq;
    nyq2 = nyq * 2;
    data_off = Parms->data_off;
    bh_cnt = 0;
    for (i = 0; i < n_regions; i++) {	/* for each _BH region */
	int n_gs, cbc, dbc, cnt, gd, idf, wc, k, tpq, not_smooth;
	Bd_point bds[MAX_BDS];
	double df, sw;

	region.data = NULL;
	n_gs = VDC_get_next_region (*rgs, i, &region);
	if (n_gs <= 0)
	    break;
	*d_buf = (unsigned char *)STR_reset ((char *)*d_buf,
				region.xz * region.yz * sizeof (short));
	region.data = (short *)*d_buf;
	n_gs = VDC_get_next_region (*rgs, i, &region);

	cbc = Get_border (&region, pmp, xz, yz, bds, &dbc);

	wc = 1;				/* well connected to the good area */
	if (n_gs < 10 && cbc > dbc * 2)
	    wc = 0;			/* small, well-connected area */
	if (cbc < 4 || (cbc < 40 && cbc < dbc))	/* not well connected */
	    wc = 0;			/* keep this region as _BH */

	cnt = 0;
	for (y = 0; y < region.yz; y++) {
	    short *d = region.data + y * region.xz;
	    short *sl = sol + ((y + region.ys) % yz) * xz + region.xs;
	    unsigned char *dm = dmap + ((y + region.ys) % yz) * stride + 
							region.xs;
	    for (x = 0; x < region.xz; x++) {
		if (d[x] == SNO_DATA)
		    continue;
		if (wc) {
		    b[cnt] = sl[x];
		    cnt++;
		    dm[x] &= ~DMAP_BH;
		}
		else
		    dm[x] = (dm[x] & (~DMAP_BH)) | DMAP_LOCAL;
	    }
	}
	if (!wc)
	    continue;

	tpq = Parms->tp_quant;
	if (cbc > dbc * 2)	/* turn off TPQ since mostly non-aliase */
	    Parms->tp_quant = 0;	/* not be true */
	Quantize_solution (cnt, b, dvq, bgf, NULL);
	Parms->tp_quant = tpq;

	/* calculate border difference */
	df = sw = 0.;
	for (k = 0; k < cbc; k++) {
	    double w;
	    int offi, offo, v;
	    Bd_point *bd = bds + k;
	    offi = bd->yi * xz + bd->xi;
	    offo = bd->yo * xz + bd->xo;
	    v = carea[offo] - data_off;
	    if (v < 0)
		v = -v;
	    if (v * 2 > nyq)
		w = .3;
	    else
		w = 1.;

	    df += (carea[offi] + dvq[bd->dcnt] * nyq2 - 
				carea[offo] - qsl[offo] * nyq2) * w;
	    sw += w;
	}

	gd = 0;
	idf = Myround (df / sw);
	while (idf < -nyq) {
	    idf += nyq2;
	    gd++;
	}
	while (idf > nyq) {
	    idf -= nyq2;
	    gd--;
	}
	if (idf < 0)
	    idf = -idf;
	if (idf > 2 * nyq / 3) {
	    not_smooth = 1;
	}
	else {
	    not_smooth = 0;
	}

	cnt = 0;
	for (y = 0; y < region.yz; y++) {
	    short *d = region.data + y * region.xz;
	    char *qs = qsl + ((y + region.ys) % yz) * xz + region.xs;
	    unsigned char *dm = dmap + ((y + region.ys) % yz) * stride +
								region.xs;
	    for (x = 0; x < region.xz; x++) {
		if (d[x] == SNO_DATA)
		    continue;
		if (not_smooth)
		    dm[x] |= DMAP_LOCAL;
		else {
		    qs[x] = dvq[cnt] + gd;
		    if (bgf[cnt] & DMAP_BH) {
			dm[x] |= DMAP_BH;
			bh_cnt++;
		    }
		}		
		cnt++;
	    }
	}
    }

    return (bh_cnt);
}

/****************************************************************************

    Identifies border of "region" and returns the number of connected border
    points. bds returns all connected border and dbcp returns the number of
    disconnected border points.

****************************************************************************/

static int Get_border (Region_t *region, unsigned char *pmp, int xz, int yz,
					Bd_point *bds, int *dbcp) {
    int dbc, cbc, dcnt, y, x;
    Bd_point *bd;

    dbc = cbc = 0;	/* border counts of disconn and to conn gates */
    dcnt = 0;		/* data count */
    bd = bds;
    for (y = 0; y < region->yz; y++) {
	int rxz, xo, yo, uy, dy;
	unsigned char *cr, *ur, *dr;

	short *d = region->data + y * region->xz;
	rxz = region->xz;
	yo = uy = dy = y + region->ys;
	cr = pmp + (yo % yz) * xz;	/* current row */
	ur = dr = NULL;		/* up and down row */
	if (Fppi) {
	    uy = (yo - 1 + yz) % yz;
	    ur = pmp + uy * xz;
	    dy = (yo + 1) % yz;
	    dr = pmp + dy * xz;
	}
	else {
	    if (yo > 0) {
		uy = yo - 1;
		ur = pmp + uy * xz;
	    }
	    if (yo < yz - 1) {
		dy = yo + 1;
		dr = pmp + dy * xz;
	    }
	}
	for (x = 0; x < region->xz; x++) {
	    if (d[x] == SNO_DATA)
		continue;
	    xo = x + region->xs;
	    if ((x == 0 || d[x - 1] == SNO_DATA) && xo > 0) {
		int t = cr[xo - 1];
		if (t == PMP_NON_BH) {
		    if (cbc < MAX_BDS) {
			bd->xi = xo;
			bd->yi = yo;
			bd->xo = xo - 1;
			bd->yo = yo;
			bd->dcnt = dcnt;
			bd++;
			cbc++;
		    }
		}
		else if (t == PMP_EXTN)
		    dbc++;
	    }
	    if ((x == region->xz - 1 || d[x + 1] == SNO_DATA) && 
					    xo < xz - 1) {
		int t = cr[xo + 1];
		if (t == PMP_NON_BH) {
		    if (cbc < MAX_BDS) {
			bd->xi = xo;
			bd->yi = yo;
			bd->xo = xo + 1;
			bd->yo = yo;
			bd->dcnt = dcnt;
			bd++;
			cbc++;
		    }
		}
		else if (t == PMP_EXTN)
		    dbc++;
	    }
	    if ((y == 0 || d[x - rxz] == SNO_DATA) && ur != NULL) {
		int t = ur[xo];
		if (t == PMP_NON_BH) {
		    if (cbc < MAX_BDS) {
			bd->xi = xo;
			bd->yi = yo;
			bd->xo = xo;
			bd->yo = uy;
			bd->dcnt = dcnt;
			bd++;
			cbc++;
		    }
		}
		else if (t == PMP_EXTN)
		    dbc++;
	    }
	    if ((y == region->yz - 1 || d[x + rxz] == SNO_DATA) && 
					    dr != NULL) {
		int t = dr[xo];
		if (t == PMP_NON_BH) {
		    if (cbc < MAX_BDS) {
			bd->xi = xo;
			bd->yi = yo;
			bd->xo = xo;
			bd->yo = dy;
			bd->dcnt = dcnt;
			bd++;
			cbc++;
		    }
		}
		else if (t == PMP_EXTN)
		    dbc++;
	    }
	    dcnt++;
	}
    }
    if (dbcp != NULL)
	*dbcp = dbc;
    return (cbc);
}

/**************************************************************************

    Recusively travels through all pixels in the region and set all
    external map gates to PMP_EXTN.

**************************************************************************/

static void Set_external_map (int x, int y, int cnt, int xz, int yz,
						unsigned char *map) {

    if (cnt > 20000)
	return;
    cnt++;
    if (map[y * xz + x] == BNO_DATA) {
	map[y * xz + x] = PMP_EXTN;
	if (x > 0)
	    Set_external_map (x - 1, y, cnt, xz, yz, map);
	if (y > 0)
	    Set_external_map (x, y - 1, cnt, xz, yz, map);
	if (x < xz - 1)
	    Set_external_map (x + 1, y, cnt, xz, yz, map);
	if (y < yz - 1)
	    Set_external_map (x, y + 1, cnt, xz, yz, map);
    }
    cnt--;
}

/**************************************************************************

    Quantizes the solution of x to be integer representing multiple of 
    2 * nyq. The algorithm is to minimize the gates that are not near the
    2kNyq.

**************************************************************************/

#define UA_HIST_MAX 300

static int Quantize_solution1 (int ng, Banbks_t *x, char *dvq, 
				unsigned char *bgf, unsigned char *xrcnt) {
    int h[UA_HIST_MAX], center, k, up, low, up2, low2;

    if (Nyq * 3 >= UA_HIST_MAX) {
	VDD_log ("Nyquist too high\n");
	return (-1);
    }

    memset (h, 0, sizeof (int) * UA_HIST_MAX);
    center = UA_HIST_MAX / 2;
    for (k = 0; k < ng; k++) {	/* generate histogram */
	int xx;

	xx = (int)(x[k] + 1000.5) - 1000;	/* rounding */
	xx += center;
	if (xx >= UA_HIST_MAX)
	    xx = UA_HIST_MAX - 1;
	if (xx < 0)
	    xx = 0;
	if (xrcnt == NULL)
	    h[xx]++;
	else
	    h[xx] += xrcnt[k];
    }

    /* find where to quantize */
    up = Search_nearby_min (h, center + Nyq, Nyq / 3);
    low = Search_nearby_min (h, center - Nyq, Nyq / 3);
    up2 = center + Nyq * 3;
    low2 = center - Nyq * 3;

    for (k = 0; k < ng; k++) {	/* quantize */
	int xx;
	xx = (int)(x[k] + 1000.5) - 1000;	/* rounding */
	xx += center;
	bgf[k] = 0;
	if (xx < low) {
	    if (xx < low2)
		dvq[k] = -2;
	    else {
		dvq[k] = -1;
		if (xx > low - Nyq / 4)
		    bgf[k] |= DMAP_BH;
	    }
	}
	else if (xx > up) {
	    if (xx > up2)
		dvq[k] = 2;
	    else {
		dvq[k] = 1;
		if (xx < up + Nyq / 4)
		    bgf[k] |= DMAP_BH;
	    }
	}
	else
	    dvq[k] = 0;
    }
    return (0);
}

static int Search_nearby_min (int *h, int st, int rg) {
    int mini, min, i;

    mini = st;
    min = h[st];
    for (i = 0; i < 2; i++) {
	int step, k;

	if (i == 0)
	    step = 1;
	else
	    step = -1;
	for (k = 1; k < rg; k++) {
	    int nx = st + step * k;
	    if (nx >= UA_HIST_MAX || nx < 0)
		break;
	    if (h[nx] >= min)
		break;
	    mini = nx;
	    min = h[nx];
	}
    }
    return (mini);
}

#define HIST_MAX 150
#define MAX_AW 40

static int Quantize_solution (int ng, Banbks_t *x, char *dvq, 
				unsigned char *bgf, unsigned char *xrcnt) {
    static __thread int init = 0, h[HIST_MAX * 2];
    int k, minh, maxh, nyq, nyq2, bh_cnt;
    Banbks_t d, scale;
    int kmax, exi[4], l_wid, u_wid;

    if (!init) {
	memset (h, 0, HIST_MAX * 2 * sizeof (int));
	init = 1;
    }

    bh_cnt = 0;
    minh = HIST_MAX - 1;
    maxh = 0;
    nyq = Nyq;
    nyq2 = nyq * 2;
    if (nyq > HIST_MAX) {
	VDD_log ("Nyquist too high\n");
	return (-1);
    }
    for (k = 0; k < ng; k++) {	/* generate histogram */
	int xx, ac;

	xx = (int)(x[k] + 1000.5) - 1000;	/* rounding */
	if (xx > 1000 || xx < -1000) {
	    VDD_log ("Bad 2D solution (xx %d)\n", xx);
	    return (-1);
	}
	ac = 0;
	while (xx < 0) {
	    xx += nyq2;
	    ac--;
	}
	while (xx >= nyq2) {
	    xx -= nyq2;
	    ac++;
	}
	dvq[k] = ac;
	bgf[k] = xx;
	if (xx < minh)
	    minh = xx;
	if (xx > maxh)
	    maxh = xx;
	if (xrcnt == NULL)
	    h[xx]++;
	else
	    h[xx] += xrcnt[k];
    }


    u_wid = l_wid = nyq / 2;
    kmax = Windowed_search (nyq, h);	/* window center quantization */
		/* if kmax >= 0, almost all data in a window of width nyq */

    if (Parms->tp_quant && kmax < 0 && Search_two_peaks (nyq, h, exi) == 2) {
	int st, end, peak1, peak2, cnt1, cnt2, gcnt[10], max, maxac1, maxac2;

	/* two peaks found - try two peak dealiasing */
	if (exi[1] < exi[3])
	    st = 1;
	else
	    st = 3;
	end = exi[(st + 2) % 4];
	peak1 = exi[(st + 1) % 4];
	peak2 = exi[(st + 3) % 4];
	st = exi[st];
	cnt1 = cnt2 = 0;
	for (k = 0; k < 10; k++)
	    gcnt[k] = 0;
	for (k = 0; k < ng; k++) {
	    int dv, center, ac;

	    if (bgf[k] >= st && bgf[k] < end)
		center = 1;
	    else
		center = 0;
	    dv = bgf[k] + dvq[k] * nyq2;
	    if (center)
		dv -= peak1;
	    else
		dv -= peak2;
	    if (dv >= 0) 
		ac = (dv + nyq) / nyq2;
	    else
		ac = -((-dv + nyq) / nyq2);
	    if (ac < -2 || ac > 2)	/* not significant */
		continue;
	    if (center) {
		if (xrcnt == NULL)
		    gcnt[2 + ac] += 1;
		else
		    gcnt[2 + ac] += xrcnt[k];
		cnt1++;
	    }
	    else {
		if (xrcnt == NULL)
		    gcnt[7 + ac] += 1;
		else
		    gcnt[7 + ac] += xrcnt[k];
		cnt2++;
	    }
	}
	max = maxac1 = -1;
	for (k = 0; k < 5; k++) {
	   if (gcnt[k] > max) {
		max = gcnt[k];
		maxac1 = k - 2;
	   }
	}
	max = maxac2 = -1;
	for (k = 5; k < 10; k++) {
	   if (gcnt[k] > max) {
		max = gcnt[k];
		maxac2 = k - 2 - 5;
	   }
	}
	if (cnt1 >= 5 * Parms->small_gc && cnt2 >= 5 * Parms->small_gc) {
	    int da1, da2, df;
	    if (exi[1] < exi[3]) {
		da1 = exi[2] + maxac1 * nyq2;
		da2 = exi[0] + maxac2 * nyq2;
	    }
	    else {
		da1 = exi[0] + maxac1 * nyq2;
		da2 = exi[2] + maxac2 * nyq2;
	    }
	    df = da1 - da2;
	    if (df < 0)
		df = -df;
	    if (df < nyq2 * 4 / 5) {
		int dfs[2];
		kmax = (((da1 + da2) / 2) + 3 * nyq2) % nyq2;
		for (k = 0; k < 2; k++) {
		    dfs[k] = kmax - exi[k * 2 + 1];
		    if (dfs[k] < 0)
			dfs[k] = -dfs[k];
		    if (dfs[k] > nyq)
			dfs[k] = nyq2 - dfs[k];
		}
		if (dfs[0] < dfs[1])
		    kmax = (exi[3] + nyq) % nyq2;
		else
		    kmax = (exi[1] + nyq) % nyq2;
		l_wid = u_wid = nyq2;	/* no limit */
	    }
	}
    }

    if (kmax < 0) {	/* single peak dealiasing */
	int st, end, thr;
	float ah[HIST_MAX * 2];
	kmax = Moving_average (nyq, h, nyq / 5, ah, NULL);
	Moving_average (nyq, h, 2, ah, NULL);
	Find_peak_edges (nyq, ah, kmax, 1.2f, nyq * 2 / 3, &st, &end);
	l_wid = kmax - st;
	u_wid = end - kmax;
	thr = Parms->bh_thr * nyq;
	if (l_wid < thr)
	    l_wid = thr;
	if (u_wid < thr)
	    u_wid = thr;
    }

    d = kmax;
    scale = .5 / (Banbks_t)nyq;
    for (k = 0; k < ng; k++) {
	int t;
	Banbks_t dt;
	dt = x[k] - d;
	if (dt < 0.) {
	    t = (-dt) * scale + .5;
	    dvq[k] = -t;
	    t = -((int)(-dt + .5));
	}
	else {
	    t = dt * scale + .5;
	    dvq[k] = t;
	    t = dt + .5;
	}
	while (t < -nyq)
	    t += nyq2;
	while (t > nyq)
	    t -= nyq2;
	if ((t < 0 && t < -l_wid) || (t >= 0 && t > u_wid)) {
	    bgf[k] = DMAP_BH;
	    if (xrcnt == NULL)
		bh_cnt++;
	    else
		bh_cnt += xrcnt[k];
	}
	else
	    bgf[k] = 0;
    }

    for (k = minh; k <= maxh; k++)
	h[k] = 0;

    return (bh_cnt);
}

/**************************************************************************

    Searches for two peaks on histogram "h". Returns the number of peaks
    found.

**************************************************************************/

static int Search_two_peaks (int nyq, int *h, int *exi) {
    int k, nyq2, i, st1, end1, st2, end2, st, end, cnt;
    int max1i, max2i, min1i, min2i;
    float maxmin, minmax;
    float ah[HIST_MAX * 2], max1, max2, min1, min2;

    /* compute moving avarage and find maximum */
    max1i = Moving_average (nyq, h, nyq / 10, ah, &max1);
    Find_peak_edges (nyq, ah, max1i, 2.f, nyq / 2, &st1, &end1);
    if (end1 - st1 > nyq * 3 / 2)
	return (1);

    /* find second maximum excluding the first peak */
    st = st1;
    end = end1;
    nyq2 = nyq * 2;
    if (st < 0) {
	st += nyq2;
	end += nyq2;
    }
    else if (end >= nyq2) {
	st -= nyq2;
	end -= nyq2;
    }
    max2 = -1.f;
    max2i = 0;
    for (k = 0; k < nyq2; k++) {
	if (k >= st && k <= end)
	    continue;
	if (k >= st1 && k <= end1)
	    continue;
	if (ah[k] > max2) {
	    max2 = ah[k];
	    max2i = k;
	}
    }
    if (max2 < 0.f)
	return (1);
    Find_peak_edges (nyq, ah, max2i, 2.f, nyq / 2, &st2, &end2);
    /* adjust st2 and end2 to remove overlap */
    if (max2i >= max1i) {
	if (st2 < end1)
	    st2 = end1;
	if (end2 - st1 >= nyq2)
	    end2 = st1 + nyq2;
    }
    else {
	if (end2 > st1)
	    end2 = st1;
	if (end1 - st2 >= nyq2)
	    st2 = end1 - nyq2;
    }

    /* check size of peak2 */
    cnt = 0;
    for (k = st2; k <= end2; k++)
	cnt += h[(k + nyq2) % nyq2];
    if (cnt < 4 * Parms->small_gc) {
	return (1);		/* second peak too small */
    }

    {
	/* check if the second preak is too close to the first peak's edges*/
	int min, df;
	min = nyq2;
	df = max2i > st ? max2i - st : st - max2i;
	if (df < min)
	    min = df;
	df = max2i > end ? max2i - end : end - max2i;
	if (df < min)
	    min = df;
	df = max2i > st1 ? max2i - st1 : st1 - max2i;
	if (df < min)
	    min = df;
	df = max2i > end1 ? max2i - end1 : end1 - max2i;
	if (df < min)
	    min = df;
	if (min * 10 < nyq) {
	    return (1);
	}

	/* check if the two peaks are too close */
	df = max1i - max2i;
	if (df < 0)
	    df = -df;
	if (df < nyq / 3 || df > nyq2 - nyq / 3) {
	    return (1);
	}
    }

    /* find the two minimums */
    Moving_average (nyq, h, nyq / 20, ah, NULL);
    minmax = max1 > max2 ? max2 : max1;
    min1 = min2 = 0.f;
    min1i = min2i = 0;
    for (i = 0; i < 2; i++) {
	int stm, endm, ind, vally, vally_maxi, maxc, cnt, si, msi;
	float min, vally_max;

	if (i == 0) {
	    st = max1i;
	    end = max2i;
	    stm = (end1 + nyq2) % nyq2;
	    endm = (st2 + nyq2) % nyq2;
	}
	else {
	    st = max2i;
	    end = max1i;
	    stm = (end2 + nyq2) % nyq2;
	    endm = (st1 + nyq2) % nyq2;
	}
	min = 10000000.f;
	vally = 0;
	vally_maxi = -1;
	vally_max = 0.f;
	ind = st;
	while (ind != end) {
	    if (ah[ind] < min)
		min = ah[ind];
	    if (ind == stm)
		vally++;
	    if (vally == 1 && ah[ind] > vally_max) {
		vally_max = ah[ind];
		vally_maxi = ind;
	    }
	    if (ind == endm)
		vally--;
	    ind = (ind + 1) % nyq2;
	}
	if (vally_maxi >= 0 && vally_max * 3.f > minmax &&
				vally_maxi != stm && vally_maxi != endm) {
	    return (3);
	}
	min = min * 1.1;
	maxc = -1;
	cnt = si = msi = 0;
	ind = st;
	while (ind != end) {
	    if (ah[ind] <= min) {
		if (cnt == 0)
		    si = ind;
		cnt++;
	    }
	    else
		cnt = 0;
	    if (cnt > maxc) {
		msi = si;
		maxc = cnt;
	    }
	    ind = (ind + 1) % nyq2;
	}

	if (i == 0) {
	    min1i = (msi + maxc / 2) % nyq2;
	    min1 = min / 1.1f;
	}
	else {
	    min2i = (msi + maxc / 2) % nyq2;
	    min2 = min / 1.1f;
	}
    }
    maxmin = min1 > min2 ? min1 : min2;
    if (maxmin * 3.f > minmax) {
	return (0);
    }
    exi[0] = max1i;
    exi[1] = min1i;
    exi[2] = max2i;
    exi[3] = min2i;
    return (2);
}

/****************************************************************************

    If the histogram is almost within nyq width, the center of the
    window that covers the most of data is returned. If the location is
    non-unique, the one with the max center value is returned. Returns
    -1 if the data is not within nyq width.

****************************************************************************/

#define DEAL_INDEX(x,min,max) \
		if (x < min) x += nyq2; else if (x >= max) x -= nyq2

static int Windowed_search (int nyq, int *h) {
    int nyq2, max, maxi, s, w, cp, cnt, k;

    nyq2 = nyq * 2;
    w = nyq >> 1;
    max = maxi = s = cp = cnt = 0;
    for (k = 0; k < nyq2; k++) {
	cnt += h[k];
	if (k == 0) {
	    int j;
	    for (j = -w; j <= w; j++) {
		int ind = k + j;
		DEAL_INDEX (ind, 0, nyq2);
		s += h[ind];
	    }
	}
	else {
	    int ind, ind1;
	    ind = k + w;
	    DEAL_INDEX (ind, 0, nyq2);
	    ind1 = k - w - 1;
	    DEAL_INDEX (ind1, 0, nyq2);
	    s = s + h[ind] - h[ind1];
	}
	if (s > max) {
	    max = s;
	    maxi = k;
	    cp = h[k];
	}
	else if (s == max) {
	    if (h[k] > cp) {
		cp = h[k];
		maxi = k;	/* highest hist of all max windowed h */
	    }
	}
    }
    if (cnt - max <= Parms->small_gc)
	return (maxi);
    return (-1);
}

/***************************************************************************

    Computes moving average of window width "w" on "h" and returns it in 
    "ah". Returns the location of the peak. "stp" and "endp" return the edges
    of the peak. min_w is the minimum width of the peak.

***************************************************************************/

static int Moving_average (int nyq, int *h, int w, float *ah, float *maxp) {
    float wt[MAX_AW * 2 + 1], tw, max;
    int nyq2, k, maxi;

    if (w > MAX_AW)
	w = MAX_AW;
    wt[w] = 1.f;
    tw = 1.f;				/* normalize factor */
    for (k = 1; k <= w; k++) {		/* square weighting */
	float t = (float)k / w;
	wt[k + w] = wt[w] * (1.f - t * t * .8f);
	wt[w - k] = wt[k + w];
	tw += 2.f * wt[k + w];
    }

    /* compute moving avarage and find maximum */
    nyq2 = nyq * 2;
    tw = 1.f / tw;
    max = -1.f;
    maxi = 0;
    for (k = w; k < nyq2 + w; k++) {
	int j, ind;
	float sum;

	sum = 0.f;
	for (j = -w; j <= w; j++) {
	    int v = h[(j + k) % nyq2];
	    sum += v * wt[j + w];
	}
	ind = k % nyq2;
	ah[ind] = sum * tw;
	if (sum > max) {
	    max = sum;
	    maxi = ind;
	}
    }
    if (maxp != NULL)
	*maxp = max;
    return (maxi);
}

/**************************************************************************

    Finds the two edges of peak maxi on histogram ah.

**************************************************************************/

static void Find_peak_edges (int nyq, float *ah, int maxi, float min_thr, 
					int w, int *stp, int *endp) {
    int nyq2, i;

    /* find the edges of the peak */
    nyq2 = nyq * 2;
    for (i = 0; i < 2; i++) {
	float min, thr;
	int st, end, mini, cnt;

	st = maxi;
	if (i == 0)
	    end = maxi - w;
	else
	    end = maxi + w;
	min = ah[maxi];
	mini = st;
	cnt = 0;
	while (st != end) {
	    float v = ah[(st + nyq2) % nyq2];
	    if (v < min) {
		min = v;
		mini = st;
	    }
	    else if (v > min * 2. && cnt * 2 >= w)
		break;
	    if (i == 0)
		st--;
	    else
		st++;
	    cnt++;
	}
	thr = (ah[maxi] - min) * .3f + min;
	if (thr > min_thr * min)
	    thr = min_thr * min;
	st = maxi;
	while (st != end) {
	    float v = ah[(st + nyq2) % nyq2];
	    if (v <= thr)
		break;
	    if (i == 0)
		st--;
	    else
		st++;
	}
	if (i == 0)
	    *stp = st;
	else
	    *endp = st;
    }
}

/*************************************************************************

    Forms the 2D dealiasing problem and calls the solver to solve it. If
    the size of the problem is large, we try to apply the variable 
    reduction technique. If the technique fails, we redo it without
    variable reduction. 

*************************************************************************/

static int Form_2D_and_solve (int trans, short *carea, int stride, 
		int *indx, int xz, int yz, Banbks_t **bp, int *n_g) {
    int nr, ng, n, n_gates;
    double saving;

    nr = 1;		/* number of variable reduction */
    if (xz * yz > 1000 && !Fppi)
	nr = 3;		/* we do not try nr if Fppi for simplicity */

    ng = n_gates = 0;		/* rm compiler warning */
    for (n = 0; n < 2; n++) {
	Sp_matrix *a;
	Banbks_t *b;
	unsigned char *rmap;
	int bw, ret;
	float bw_red;

	bw_red = 1.;
	bw = xz;
	if (nr > 1) {
	    rmap = Get_shared_buffer (xz * yz);
	    n_gates = Set_hsg_map (carea, xz, yz, trans, stride, 
					Nyq / 3, 3 * Nyq, rmap);
	    Set_critical_gates (xz, yz, rmap);
	    ng = Create_vr_vindex (xz, yz, rmap, nr, indx, &bw_red);
	    bw += nr - 1;
	}
	else {
	    ng = Create_vindex (xz, yz, carea, stride, trans, indx);
	    n_gates = ng;
	}

	ret = Construct_a_and_b (xz, yz, bw, trans, stride, carea, 
						    &a, &b, ng, indx);
	*bp = b;
	if (ret == 0) {		/* nothing to be done */
	    return (0);
	}

	saving = 1. - (double)bw_red * bw_red * ng / n_gates;
	if (nr > 1 && saving < .35) {	/* savings not large enough */
	    nr = 1;
	    continue;
	}
	ret = VDB_solve (ng, a, b);
	if (ret < 0) {
	    return (ret);
	}
    
	if (nr > 1) {
	    if (Verify_vred (indx, xz, yz, b) <= .1 * Nyq)
		break;
	    nr = 1;		/* retry without variable reduction */
	}
	else
	    break;
    }
    *n_g = n_gates;
    return (ng);
}

/*************************************************************************

    Sets up the high shear gate map of "hmap". 0 for bad data; 1 for
    normal gate; 2 for high shear gates. "lthr" and "hthr" are
    respectively the lower and upper thresholds for high shear gates.
    Returns the number of data gates.

*************************************************************************/

static int Set_hsg_map (short *carea, int xz, int yz, int trans, int stride,
				int lthr, int hthr, unsigned char *hmap) {
    int dofa[4];
    int ng, i, j;

    if (trans) {
	dofa[0] = -stride;	/* data offsets of neighboring gates */
	dofa[1] = stride;
	dofa[2] = -1;
	dofa[3] = 1;
    }
    else {
	dofa[0] = -1;
	dofa[1] = 1;
	dofa[2] = -stride;
	dofa[3] = stride;
    }

    /* determine the critical gates */
    ng = 0;
    for (j = 0; j < yz; j++) {
	int iofa[4], hmapind;

	if (j == 0)
	    iofa[2] = 0;
	else
	    iofa[2] = -xz;
	if (j == yz - 1)
	    iofa[3] = 0;
	else
	    iofa[3] = xz;

	hmapind = j * xz - 1;
	for (i = 0; i < xz; i++) {
	    int v, dind, k;

	    hmapind++;
	    if (trans)
		dind = i * stride + j;	/* data index of the current gate */
	    else
		dind = j * stride + i;
	    v = carea[dind];		/* value of the current gate */
	    if (v == SNO_DATA) {
		hmap[hmapind] = 0;
		continue;
	    }
	    ng++;

	    /* index offsets of neighboring gates */
	    if (i == 0)
		iofa[0] = 0;
	    else
		iofa[0] = -1;
	    if (i == xz - 1)
		iofa[1] = 0;
	    else
		iofa[1] = 1;

	    hmap[hmapind] = 1;
	    for (k = 0; k < 4; k++) {
		int iof, diff, nv;

		iof = iofa[k];
		if (iof == 0)
		    continue;

		nv = carea[dind + dofa[k]];
		if (nv == SNO_DATA)
		    continue;
		diff = v - nv;
		if (diff < 0)
		    diff = -diff;
		if (diff > lthr && diff < hthr)
		    hmap[hmapind] = 2;
	    }
	}
    }
    return (ng);
}

/*************************************************************************

    Sets the critical gates based on the high shear gates in the high
    shear gate map "rmap". The critical gates are marked by 3.

*************************************************************************/

static int Set_critical_gates (int xz, int yz, unsigned char *rmap) {
    int i, j;
    int xw, yw;

    /* create the critical gates based on high shear gates */
    yw = 2;
    xw = 2;
    for (j = 0; j < yz; j++) {
	unsigned char *cp, *np;

	cp = rmap + j * xz - 1;
	for (i = 0; i < xz; i++) {
	    int m;

	    cp++;
	    if (*cp != 2)
		continue;
	    for (m = -yw; m <= yw; m++) {
		int t, n;
		t = j + m;
		if (t < 0 || t >= yz)
		    continue;
		np = cp + m * xz;
		for (n = -xw; n <= xw; n++) {
		    t = i + n;
		    if (t < 0 || t >= xz)
			continue;
		    if (np[n] == 1)
			np[n] = 3;
		}
	    }
	}
    }
    return (0);
}

/*************************************************************************

    Create the variable (x) index table using the variable reduction
    map. Returns the number of variables (dimension of x).

*************************************************************************/

static int Create_vr_vindex (int xz, int yz, unsigned char *rmap, 
					int nred, int *indx, float *bw_red) {
    int ng, max_lcnt, max_rcnt, cnt, i, j;

    ng = 0;		/* number of gates to dealiase - x array size */
    max_lcnt = max_rcnt = 0;
    for (j = 0; j < yz; j++) {
	int *indp, lcnt, st_ng;;
	unsigned char *rmapp;

	indp = indx + j * xz - 1;
	rmapp = rmap + j * xz - 1;
	cnt = 0;
	st_ng = ng;
	lcnt = 0;
	for (i = 0; i < xz; i++) {

	    indp++;
	    rmapp++;
	    if (*rmapp == 0) {
		*indp = -1;
		if (cnt > 0) {
		    cnt = 0;
		    ng++;
		}
	    }
	    else if (*rmapp == 1) {
		*indp = ng;
		cnt++;
		if (cnt >= nred || ng == 0) {	/* first gate must be 
						   processed differently */
		    cnt = 0;
		    ng++;
		}
		lcnt++;
	    }
	    else {
		*indp = ng;
		ng++;
		cnt = 0;
		lcnt++;
	    }
	}
	if (cnt > 0)
	    ng++;
	if (lcnt > max_lcnt)
	    max_lcnt = lcnt;
	if (ng - st_ng > max_rcnt)
	    max_rcnt = ng - st_ng;
    }
    if (max_lcnt > 0)
	*bw_red = (float)max_rcnt / max_lcnt;
    else
	*bw_red = 1.;
    return (ng);
}

/*************************************************************************

    .

*************************************************************************/

static int Count_repeated_x (int xz, int yz, int ng, unsigned char *rmap, 
						int *indx) {
    int j, i;

    memset (rmap, 0, ng);
    for (j = 0; j < yz; j++) {
	int *indp;

	indp = indx + j * xz - 1;
	for (i = 0; i < xz; i++) {
	    int v;

	    indp++;
	    v = *indp;
	    if (v >= 0)
		rmap[v]++;
	}
    }
    return (0);
}

/*************************************************************************

    Create the variable (x) index table without the variable reduction
    map. Returns the number of variables (dimension of x).

*************************************************************************/

static int Create_vindex (int xz, int yz, short *carea, 
					int stride, int trans, int *indx) {
    int ng, i, j;

    ng = 0;		/* number of gates to dealiase - x array size */
    for (j = 0; j < yz; j++) {
	int *indp;

	indp = indx + j * xz;
	for (i = 0; i < xz; i++) {
	    int ind;

	    if (trans)
		ind = i * stride + j;
	    else
		ind = j * stride + i;
	    if (carea[ind] != SNO_DATA) {
		*indp = ng;
		ng++;
	    }
	    else
		*indp = -1;
	    indp++;
	}
    }
    return (ng);
}

/*************************************************************************

    Creates and returns the azimuthal high shear map.

*************************************************************************/

static unsigned char *Set_azi_hsg_map (short *carea, int xz, int yz, 
			int trans, int stride, int lthr, int hthr) {
    static __thread int b_size = 0;
    static __thread unsigned char *map = NULL;
    int dofa;
    int xsize, ysize, i, j;

    if (trans) {
	xsize = yz;
	ysize = xz;
    }
    else {
	xsize = xz;
	ysize = yz;
    }
    if (ysize * stride > b_size) {
	if (map != NULL)
	    free (map);
	b_size = ysize * stride;
	map = (unsigned char *)MISC_malloc (b_size * sizeof (unsigned char));
    }

    /* determine the azimuthal high shear gates */
    dofa = stride;
    for (j = 0; j < ysize; j++) {
	for (i = 0; i < xsize; i++) {
	    int v, dind, diff, nv;

	    dind = j * stride + i;
	    v = carea[dind];		/* value of the current gate */
	    if (v == SNO_DATA) {
		map[dind] = 0;
		continue;
	    }

	    map[dind] = 1;
	    if (j == ysize - 1)
		continue;
	    nv = carea[dind + dofa];
	    if (nv == SNO_DATA)
		continue;
	    diff = v - nv;
	    if (diff < 0)
		diff = -diff;
	    if (diff > lthr && diff < hthr)
		map[dind] = 2;
	}
    }

    for (j = 0; j < ysize - 1; j++) {
	unsigned char *st, *cr, *end, *next, *p;

	st = map + j * stride;
	end = st + xsize - 1;
	cr = st;
	while (cr <= end) {
	    if (*cr & 2) {
		if (cr > st) {
		    cr[-1] |= 4;
		    cr[stride - 1] |= 8;
		}
		if (cr < end) {
		    cr[1] |= 4;
		    cr[stride + 1] |= 8;
		}
		cr[0] |= 4;
		cr[stride] |= 8;
		next = cr + 1;
		while (next <= end && next - cr <= 4) {
		    if (*next & 2) {
			p = cr + 1;
			while (p < next) {
			    if (*p & 1) {
				*p |= 4;
				p[stride] |= 8;
			    }
			    p++;
			}
			break;
		    }
		    next++;
		}
		cr = next - 1;
	    }
	    cr++;
	}
    }

    return (map);
}

/*************************************************************************

    Create the A and B for the 2D dealiasing problem. Return 1 on success,
    or 0 there is no aliasing found.

*************************************************************************/

static int Construct_a_and_b (int xz, int yz, int bw, int trans,
		int stride, short *carea, Sp_matrix **sap, Banbks_t **bp, 
		int ng, int *indx) {
    static __thread Spmcg_t *sa_ev = NULL;
    static __thread int *sa_cind = NULL, *sa_ne = NULL;
    static __thread int sa_size = 0;
    static __thread Sp_matrix sa;
    static __thread Banbks_t *b = NULL;
    static __thread int b_size = 0;
    int rs, al_found, i, j, sa_off, sa_cnt, pr_n, db_size, rgz, bcset;
    int dofa[4];
    int tl2, th2, vn2;
    float rwbuf[1200], *rw, wf, cal, cbl, cah, cbh;
    unsigned char *ahsh_map;

    rs = bw * 2  + 1;
    if (ng > b_size) {
	if (b != NULL)
	    free (b);
	if (sa_ne != NULL)
	    free (sa_ne);
	if (sa_cind != NULL)
	    free (sa_cind);
	if (sa_ev != NULL)
	    free (sa_ev);
	b_size = ng;
	b = (Banbks_t *)MISC_malloc (b_size * sizeof (Banbks_t));
	sa_ne = (int *)MISC_malloc (b_size * sizeof (int));
	sa_size = 5 * b_size + 256;
	sa_cind = (int *)MISC_malloc (sa_size * sizeof (int));
	sa_ev = (Spmcg_t *)MISC_malloc (sa_size * sizeof (Spmcg_t));
    }
    *bp = b;
    *sap = &sa;

    if (trans) {
	dofa[0] = -stride;	/* data offsets of neighboring gates */
	dofa[1] = stride;
	dofa[2] = -1;
	dofa[3] = 1;
	db_size = xz * stride;
	rgz = yz;
    }
    else {
	dofa[0] = -1;
	dofa[1] = 1;
	dofa[2] = -stride;
	dofa[3] = stride;
	db_size = yz * stride;		/* data buffer size */
	rgz = xz;
    }

    /* rw, range bordar weight, for taking into accout the gate's shape */
    if (rgz <= 1200)
	rw = rwbuf;
    else
	rw = (float *)MISC_malloc (rgz * sizeof (float));
    for (i = 0; i < rgz; i++) {
	float rwt = (Parms->r0 + i) * Parms->r_w_ratio;
	if (rwt < .1f)
	    rwt = .1f;
	rw[i] = rwt;
    }

    for (i = 0; i < ng; i++)
	b[i] = 0.;

    vn2 = Nyq * 2;
    tl2 = Parms->max_shear;
    th2 = vn2 - Parms->am_shear;

    wf = Parms->weight_factor;
    cal = (wf - 1.f) / tl2;		/* linear weiting coefficients */
    cbl = 1.f;
    cah = (1.f - wf) / (vn2 - th2);
    cbh = 1.f - cah * vn2;

    ahsh_map = NULL;
    if (Parms->has_weight < .99f && ng > 20)
	ahsh_map = Set_azi_hsg_map (carea, xz, yz, trans, stride, 
					Nyq / 4, 2 * Nyq - Nyq / 4);

    al_found = 0;
    sa_cnt = 0;
    pr_n = -1;
    bcset = 0;				/* boundary condition not set */
    for (j = 0; j < yz; j++) {
	int iind, iofa[4], pscnt;
	double diag;

	if (j == 0) {
	    if (Fppi)
		iofa[2] = (yz - 1) * xz;
	    else
		iofa[2] = 0;
	}
	else
	    iofa[2] = -xz;
	if (j == yz - 1) {
	    if (Fppi)
		iofa[3] = -(yz - 1) * xz;
	    else
		iofa[3] = 0;
	}
	else
	    iofa[3] = xz;

	diag = 0.;		/* init these two to calm gcc false warning */
	pscnt = 0;
	iind = j * xz - 1;		/* index of the current gate indx */
	for (i = 0; i < xz; i++) {
	    int n, v, dind, k, same_n, rg;

	    iind++;
	    n = indx[iind];		/* x index of the current gate */
	    if (n < 0)
		continue;
	    if (trans) {
		dind = i * stride + j;	/* data index of the current gate */
		rg = j;
	    }
	    else {
		dind = j * stride + i;
		rg = i;
	    }
	    v = carea[dind];		/* value of the current gate */

	    if (n == pr_n) {
		sa_cnt--;
		same_n = 1;
	    }
	    else {
		pscnt = sa_cnt;
		diag = 0.;
		same_n = 0;
	    }
	    pr_n = n;

	    /* index offsets of neighboring gates */
	    if (i == 0)
		iofa[0] = 0;
	    else
		iofa[0] = -1;
	    if (i == xz - 1)
		iofa[1] = 0;
	    else
		iofa[1] = 1;

	    for (k = 0; k < 4; k++) {
		int iof, nn;
		Banbks_t bb, w;

		iof = iofa[k];
		if (iof == 0 || (nn = indx[iof + iind]) < 0) {
					/* nn is x index of the neighbor */
		    if (Apply_bc && (Bd_map[dind] & BD_UNALIASED)) {
			diag += 1.;
			bcset = 1;
		    }
		    continue;
		}
		if (nn != n) {
		    int diff, adiff, ci, m, dof;

		    bb = 0.;
		    dof = dind + dofa[k];
		    if (Fppi) {
			if (dof < 0)
			    dof += db_size;
			else if (dof >= db_size)
			    dof -= db_size;
		    }			
		    diff = v - carea[dof];
		    if (diff < 0)
			adiff = -diff;
		    else
			adiff = diff;
		    w = 1.;
		    if ((k <= 1 && trans) || (k > 1 && !trans)) {
			if (adiff > tl2 && adiff < th2)
			    continue;
			if (diff >= th2)
			    bb = -1.;
			else if (diff <= -th2)
			    bb = 1.;
			else
			    bb = 0.;
			if (adiff <= tl2)
			    w *= cal * adiff + cbl;
			else
			    w *= cah * adiff + cbh;
		    }
		    else {
			if (adiff > tl2 && adiff < th2)
			    continue;
			if (diff >= th2)
			    bb = -1.;
			else if (diff <= -th2)
			    bb = 1.;
			else
			    bb = 0.;
			if (dofa[k] == -1)
			    w *= rw[rg];
			else
			    w *= rw[rg + 1];
			if (adiff <= tl2)
			    w *= cal * adiff + cbl;
			else
			    w *= cah * adiff + cbh;
		    }


		    if (ahsh_map != NULL) {
			int ahshm = ahsh_map[dind];
			if (trans) {
			    if (((ahshm & 4) && k == 1) || ((ahshm & 8) && k == 0))
				w *= Parms->has_weight;
			}
			else {
			    if (((ahshm & 4) && k == 3) || ((ahshm & 8) && k == 2))
				w *= Parms->has_weight;
			}
		    }



		    ci = nn;
		    if (same_n) {
			for (m = pscnt; m < sa_cnt; m++) {
			    if (ci == sa_cind[m]) {
				sa_ev[m] += -w;
				break;
			    }
			}
		    }
		    else
			m = sa_cnt;
		    if (m >= sa_cnt) {
			sa_cind[sa_cnt] = ci;
			sa_ev[sa_cnt] = -w;
			sa_cnt++;
		    }
		    diag += w;
		    b[n] += bb * vn2 * w;
		    if (bb != 0.)
			al_found = 1;
		}
	    }
	    sa_cind[sa_cnt] = n;
	    sa_ev[sa_cnt] = diag;
	    sa_cnt++;
	    sa_ne[n] = sa_cnt - pscnt;
	    if (sa_cnt + 256 > sa_size)
		sa_size = Increase_buf_size (sa_cnt, &sa_ev, &sa_cind);
	}
    }

    /* modify the first row to let x[0] = 0 */
    if (!bcset) {
	sa_off = sa_ne[0] - 1;
	sa_cind[sa_off] = 0;
	sa_ne[0] = 1;
	sa_ev[sa_off] = 1.;
	b[0] = 0.;
    }
    else			/* no need of setting */
	sa_off = 0;

    sa.ne = sa_ne;
    sa.ev = sa_ev + sa_off;
    sa.c_ind = sa_cind + sa_off;
    if (rw != rwbuf)
	free (rw);

    return (al_found);
}

/*************************************************************************

    Verifies the errors caused by the variable reduction in solving
    the linear system. 

*************************************************************************/

static Banbks_t Verify_vred (int *indx, int xz, int yz, Banbks_t *x) {
    int j;
    Banbks_t max_diff;

    max_diff = 0.;
    for (j = 0; j < yz; j++) {
	int iind, iofa[4], i;

	if (j == 0)
	    iofa[2] = 0;
	else
	    iofa[2] = -xz;
	if (j == yz - 1)
	    iofa[3] = 0;
	else
	    iofa[3] = xz;

	iind = j * xz - 1;		/* index of the current gate indx */
	for (i = 0; i < xz; i++) {
	    int n, k, nl, nr;

	    iind++;
	    n = indx[iind];		/* x index of the current gate */
	    if (n < 0)
		continue;

	    /* index offsets of neighboring gates */
	    if (i == 0) {
		iofa[0] = 0;
		nl = -1;
	    }
	    else {
		iofa[0] = -1;
		nl = indx[iind - 1];
	    }
	    if (i == xz - 1) {
		iofa[1] = 0;
		nr = -1;
	    }
	    else {
		iofa[1] = 1;
		nr = indx[iind + 1];
	    }
	    if (n != nl && n != nr)
		continue;

	    for (k = 0; k < 4; k++) {
		int iof, nn;

		iof = iofa[k];
		if (iof == 0)
		    continue;
		nn = indx[iof + iind];		/* x index of the neighbor */
		if (nn >= 0 && nn != n) {
		    Banbks_t diff;

		    diff = x[n] - x[nn];
		    if (diff < 0.)
			diff = -diff;
		    if (diff > max_diff)
			max_diff = diff;
		}
	    }
	}
    }
    return (max_diff);
}

/********************************************************************

    Does a preliminary check to see if dealiasing is needed.

********************************************************************/

static int Need_dealiase (short *region, int xsize, int ysize) {
    int size, min, max, i;

    min = 10000;
    max = -min;
    size = xsize * ysize;
    for (i = 0; i < size; i++) {
	int t = region[i];
	if (t != SNO_DATA) {
	    if (t < min)
		min = t;
	    if (t > max)
		max = t;
	}
    }
    if (max - min >= (int)(1.5 * Nyq))
	return (1);
    return (0);
}

/********************************************************************

    Returns a buffer of at least "size" bytes for shared use. The
    caller should not free it and must make sure the buffer is not used
    simultaneously for other purposes.

********************************************************************/

static unsigned char *Get_shared_buffer (int size) {
    static __thread int b_size = 0;
    static __thread unsigned char *buf = NULL;

    if (size > b_size) {
	if (buf != NULL)
	    free (buf);
	buf = (unsigned char *)MISC_malloc (size);
    }
    return (buf);
}

/*************************************************************************

    Re-allocates the buffers "vp" and "ip" to increase their size by 
    1 / 3 and at least 256. It returns the new size.

*************************************************************************/

static int Increase_buf_size (int len, Spmcg_t **vp, int **ip) {
    int new_s;
    char *p;

    new_s = len * 4 / 3;
    if (new_s < len + 256)
	new_s = len + 256;
    p = MISC_malloc (new_s * sizeof (Spmcg_t));
    memcpy (p, *vp, len * sizeof (Spmcg_t));
    free (*vp);
    *vp = (Spmcg_t *)p;
    p = MISC_malloc (new_s * sizeof (int));
    memcpy (p, *ip, len * sizeof (int));
    free (*ip);
    *ip = (int *)p;
    return (new_s);
}

/**************************************************************************

**************************************************************************/

static int Get_bc_ew (int x, int y) {
    int xo, yo, ew;

    Vdeal_t *vdv = (Vdeal_t *)Parms->vdv;
    xo = Parms->xs + x * Parms->xr;
    if (vdv->phase == 1 && xo > Tb_vh_rg)  /* no BC is used above this */
	return (SNO_DATA);
    xo = xo % vdv->xz;
    yo = Parms->ys + y * Parms->yr;
    yo = (yo + vdv->yz) % vdv->yz;
    ew = VDE_get_ew_value (vdv, xo, yo);

    return (ew);
}

/**************************************************************************

    Travels on the region border to mark the unaliased border as UNALIASED.
    We start from a group of gates with |v| < Vn/2 and extend it along the
    border until a sudden change happens.

**************************************************************************/

#define TB_MAX_DEPTH 10000

#define GO_NEXT_GATE(func) \
    {\
	int yy, xx, ny;\
	for (yy = -1; yy <= 1; yy++) {\
	    if (!Parms->fppi && (y + yy < 0 || y + yy >= Tb_yz))\
		continue;\
	    ny = (y + yy + Tb_yz) % Tb_yz;\
	    for (xx = -1; xx <= 1; xx++) {\
		if (xx == 0 && yy == 0)\
		    continue;\
		if (x + xx < 0 || x + xx >= Tb_xz)\
		    continue;\
		Tb_depth++;\
		func (x + xx, ny);\
		Tb_depth--;\
	    }\
	}\
    }\

static void Travel_border (int x, int y) {
    int off, d, m, srg;

    if (Tb_trvcnt >= BD_MAX_TRVS || Tb_depth > TB_MAX_DEPTH)
	return;

    off = y * Tb_xz + x;
    m = Bd_map[off];
    if (m & BD_REACHED)
	return;

    srg = 8 / (Parms->xr + Parms->xr);	/* range centered at starting point
					   where v must be near ew */
    if ((Tb_depth < srg && (m & 0x3) == 3) || (Tb_depth >= srg && m == 3)) {
	int df, ew, nyq2;

	Bd_map[off] |= BD_REACHED;
	Tb_trvd[Tb_trvcnt].x = x;
	Tb_trvd[Tb_trvcnt].y = y;
	Tb_trvcnt++;
	d = Tb_carea[off];
	ew = Get_bc_ew (x, y);
	if (ew == SNO_DATA)
	    return;
	nyq2 = Nyq / 2;
	if (Tb_depth < srg && (d < ew - nyq2 || d > ew + nyq2)) {
	    TB_failed = 1;
	    return;
	}
	df = d - Tb_pv;
	if (df < 0) df = -df;
	if (((d < ew - nyq2 || d > ew + nyq2) && df > Nyq) ||
					(d < ew - Nyq || d > ew + Nyq))
	    return;
	if (m == 3) {	/* not already marked */
	    int thr;
	    Tb_cnt++;
	    Bd_map[off] |= BD_PUNALIASED;
	    thr = Nyq / 4;
	    if (d >= ew - thr && d <= ew + thr)
		Tb_gcnt++;
	    if (Parms->xs + x * Parms->xr <= Tb_h_rg) {
		int doff = Parms->data_off;
		if (ew <= doff + nyq2 && ew >= doff - nyq2 &&
				d >= doff - thr && d <= doff + thr)
		    Tb_gcnt++;
	    }
	}
	Tb_pv = d;
	GO_NEXT_GATE (Travel_border);
    }
}

/**************************************************************************

    Travels on the region border to count the length of segments that do
    not contain any near 2Vn change.

**************************************************************************/

static void Travel_border1 (int x, int y) {
    int off, m, thr;

    if (Tb_trvcnt >= BD_MAX_TRVS || Tb_depth > TB_MAX_DEPTH)
	return;

    off = y * Tb_xz + x;
    m = Bd_map[off];
    if (m & BD_REACHED)
	return;

    thr = 3 * Nyq / 2;
    if (x + Parms->xs > Tb_h_rg)
	thr = 5 * Nyq / 4;
    if ((m & 3) == 3) {
	int d, max, min, yy, xx, df;

	Bd_map[off] |= BD_REACHED;
	if (Parms->xr * Parms->xr < 4) {
	    min = 0xffff;
	    max = -min;
	    for (yy = -1; yy <= 1; yy++) {  /* find max and min in neighbor */
		short *dp;
		if (!Parms->fppi && (y + yy < 0 || y + yy >= Tb_yz))
		    continue;
		dp = Tb_carea + (((y + yy + Tb_yz) % Tb_yz) * Tb_xz + x);
		for (xx = -1; xx <= 1; xx++) {
		    if (x + xx < 0 || x + xx >= Tb_xz)
			continue;
		    d = dp[xx];
		    if (d == SNO_DATA)
			continue;
		    if (d < min)
			min = d;
		    if (d > max)
			max = d;
		}
	    }
	}
	else {
	    max = Tb_carea[off];
	    if (Tb_pv != SNO_DATA)
		min = Tb_pv;
	    else
		min = max;
	    Tb_pv = max;
	}
	df = max - min;
	if (df < 0) df = -df;
	if (df > thr)
	    return;

	if (m & BD_UNALIASED)
	    Tb_gcnt++;
	Tb_trvd[Tb_trvcnt].x = x;
	Tb_trvd[Tb_trvcnt].y = y;
	Tb_trvcnt++;
	GO_NEXT_GATE (Travel_border1);
    }
}

/**************************************************************************

    Travels on the region border to eliminate the border.

**************************************************************************/

static void Travel_border2 (int x, int y) {
    int off;

    if (Tb_depth >= TB_MAX_DEPTH)
	return;
    off = y * Tb_xz + x;
    if (Bd_map[off] != 3)
	return;
    Bd_map[off] = 1;
    GO_NEXT_GATE (Travel_border2);
}

/*************************************************************************

    Analyzes the border points of the carea to mark (on Bd_map) all border
    points that are likely to be unaliased. Returns true if BC is to apply
    or false otherwise.

*************************************************************************/

static int Border_analysys (short *carea, int xsize, int ysize) {
    Region_t reg;
    int x, y, tcnt, max, maxst, ratio, gp_found, tbcnt;
    Point_t *trvd;

    Vdeal_t *vdv = (Vdeal_t *)Parms->vdv;
    if ((vdv->data_type & DT_NOVAD) && vdv->phase == 1)
	return (0);

    memset (&reg, 0, sizeof (Region_t));
    reg.xz = xsize;
    reg.yz = ysize;
    reg.data = carea;
    Bd_map = VDA_get_border_map (&reg, Parms->fppi);

    Tb_xz = xsize;
    Tb_yz = ysize;
    Tb_cnt = 0;
    /* removes the left half of the border in case of two border sections */
    if (Parms->fppi) {
	for (y = 0; y < ysize; y++) {
	    unsigned char *m = Bd_map + y * xsize;
	    for (x = 0; x < xsize; x++) {
		if (m[x] == 3) {
		    Tb_depth = 0;
		    Travel_border2 (x, y);
		    break;
		}
	    }
	    if (x < xsize) break;
	}
	/* check if there is any border left */
	for (y = 0; y < ysize; y++) {
	    unsigned char *m = Bd_map + y * xsize;
	    for (x = 0; x < xsize; x++) {
		if ((m[x] & 3) == 3)
		    break;
	    }
	    if (x < xsize)
		break;
	}
	if (y >= ysize)		/* all border points gone (single border) */
	    Bd_map = VDA_get_border_map (&reg, Parms->fppi);	/* restore */
    }

    Tb_vh_rg = VDV_alt_to_range ((Vdeal_t *)Parms->vdv, 8000.) / 
			((Vdeal_t *)Parms->vdv)->g_size;
    Tb_h_rg = VDV_alt_to_range ((Vdeal_t *)Parms->vdv, 6000.) / 
			((Vdeal_t *)Parms->vdv)->g_size;
    ratio = Parms->xr * Parms->yr * 5;
			/* large xr, yr reduces chance of isolated bad data */

    /* finds border sections, in which v changes smoothly and each point is 
       relatively close to ew. Such section is marked as PUNALIASED. If there
       are significant number of border points that are very close to ew (good
       points), the section is marked as UNALIASED. */
    trvd = (Point_t *)Get_shared_buffer (BD_MAX_TRVS * sizeof (Point_t));
    Tb_carea = carea;
    Tb_trvd = trvd;
    Tb_cnt = tcnt = gp_found = 0;
    for (y = 0; y < ysize; y++) {
	int off = y * xsize;
	unsigned char *m = Bd_map + off;
	for (x = 0; x < xsize; x++) {
	    if ((m[x] & 3) == 3)
		tcnt++;
	    if (m[x] == 3) {		/* a border point */
		int v, ew;
		v = carea[off + x];
		ew = Get_bc_ew (x, y);
		if (ew == SNO_DATA)
		    continue;
		if (v <= ew + Nyq / 2 && v >= ew - Nyq / 2) {
		    int k, sig;
		    Tb_depth = Tb_trvcnt = Tb_gcnt = TB_failed = 0;
		    Tb_pv = v;
		    Travel_border (x, y);
		    sig = Tb_gcnt * ratio >= 20 || Tb_gcnt * 4 > Tb_trvcnt;
		    for (k = 0; k < Tb_trvcnt; k++) {
			int off = trvd[k].y * xsize + trvd[k].x;
			Bd_map[off] &= ~BD_REACHED;
			if (TB_failed) {
			    if (Bd_map[off] & BD_PUNALIASED) {
				Bd_map[off] &= ~BD_PUNALIASED;
				Tb_cnt--;
			    }
			}
			else if (sig && (Bd_map[off] & BD_PUNALIASED)) {
			    Bd_map[off] |= BD_UNALIASED;
			    gp_found++;
			}
		    }
		}
	    }
	}
    }
    tbcnt = Tb_cnt;

    /* joins sections find earlier if there is no aliasing signature in them.
       If any of the sections to join is "good", the joined section is marked 
       as UNALIASED. If non of the joined sections is good, the longest joined 
       section is marked as UNALIASED. */
    Tb_trvcnt = Tb_cnt = max = maxst = 0;
    for (x = 0; x < xsize; x++) {
	for (y = 0; y< ysize; y++) {
	    int m, st;
	    m = Bd_map[y * xsize + x];
	    if ((m & 3) != 3 || (m & BD_REACHED))
		continue;

	    st = Tb_trvcnt;
	    Tb_depth = Tb_gcnt = 0;
	    Tb_pv = SNO_DATA;
	    Travel_border1 (x, y);
	    if (Tb_trvcnt - st > max) {
		max = Tb_trvcnt - st;
		maxst = st;
	    }
	    if (Tb_gcnt > 0) {
		int k;
		for (k = st; k < Tb_trvcnt; k++) {
		    int off = trvd[k].y * xsize + trvd[k].x;
		    if (Bd_map[off] & BD_PUNALIASED) {
			Bd_map[off] |= BD_UNALIASED;
			Tb_cnt++;
		    }
		}
	    }
	}
    }
    if (gp_found == 0) {	/* non of the sections is UNALIASED */
	for (x = maxst; x < maxst + max; x++) {
	    int off = trvd[x].y * xsize + trvd[x].x;
	    if (Bd_map[off] & BD_PUNALIASED) {
		Bd_map[off] |= BD_UNALIASED;
		Tb_cnt++;
	    }
	}
    }

if (tcnt == 0)
VDD_log ("tcnt == 0:  x %d  y %d\n", xsize, ysize);

    if (tcnt == 0)	/* shoud never happen */
	return (0);
    if (Tb_cnt * 100 / tcnt >= 40)  /* large percent of border is UNALIASED */
	return (1);
    return (0);
}


