
/******************************************************************

    vdeal's module containing functions for data analizing.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/11 17:20:55 $
 * $Id: vdeal_analyze.c,v 1.7 2014/08/11 17:20:55 steves Exp $
 * $Revision: 1.7 $
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

/* variables shared between analyze.c and preprocess.c */
__thread unsigned char *Wbuf = NULL;

/* variables for supporting Travel_line */
static __thread int Tl_xmin, Tl_xmax, Tl_ymin, Tl_ymax;

/* variables for passing values through global variables */
static __thread int G_xz, G_yz, G_cnt, G_fppi;
static __thread unsigned char *G_map;

static void Travel_line (int x, int y, int fp);
static void Set_external_map (int x, int y);
static void Set_border_map (Region_t *reg, unsigned char *map, int fppi);


/**************************************************************************

    Travals inside a region formed by failed gates.

**************************************************************************/

/* global data used by Travel_failed_gates */
static __thread int Cfg_depth, Cfg_yz, Cfg_xz, Cfg_cnt, Cfg_maxv, Cfg_minv;
static __thread unsigned char *Cfg_map;
#define CFG_MAX_CNT 512

static void Travel_failed_gates (Point_t *trvs, int x, int y) {
    int off, v;

    if (Cfg_depth > 4000)
	return;

    off = y * Cfg_xz + x;
    v = Cfg_map[off];
    if (v <= 1)
	return;
    if (v > Cfg_maxv)
	Cfg_maxv = v;
    if (v < Cfg_minv)
	Cfg_minv = v;
    Cfg_map[off] = 1;
    if (Cfg_cnt < CFG_MAX_CNT) {
 	trvs[Cfg_cnt].x = x;
	trvs[Cfg_cnt].y = y;
	Cfg_cnt++;
    }
    
    Cfg_depth++;
    if (y > 0)
	Travel_failed_gates (trvs, x, y - 1);
    if (y < Cfg_yz - 1)
	Travel_failed_gates (trvs, x, y + 1);
    if (x > 0)
	Travel_failed_gates (trvs, x - 1, y);
    if (x < Cfg_xz - 1)
	Travel_failed_gates (trvs, x + 1, y);

    Cfg_depth--;
}

/***************************************************************************

    Returns the border map of a region. 0 - no data; 2 - gates 
    outside the region; 3 - border gate of the region; 1 otherwise.

***************************************************************************/

unsigned char *VDA_get_border_map (Region_t *reg, int fppi) {
    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
				reg->xz * reg->yz * sizeof (unsigned char));
    Set_border_map (reg, Wbuf, fppi);
    return (Wbuf);
}

/*************************************************************************

    Detects and returns gates that are likely to be failed in dealiasing.
    A failed gate has near 2Vn shear in out and is smooth in a 
    neighborhood in inp. Isolated failed gates are note taken into account.

*************************************************************************/

static void *Hsfs = NULL;	/* store HS features for output */
static int N_hsfs = 0, N_daffs = 0;
		/* numbers of HS features and dealiasing-failure features */

int VDA_check_failed_gates (Vdeal_t *vdv, Part_t *parts, int ptind,
						Point_t *rfp, int fp_bz) {
    typedef struct {
	short x, y;
	short v;
    } Hs_points;
    int bz, ays, aye, xz, yz, ys, thr1, thr2, nyq, fcnt, wd, y, x, i;
    int ocnt, flgs;
    unsigned char *map;
    Hs_points *fp;
    Part_t *part = parts + ptind;

    xz = vdv->xz;
    yz = part->yz;
    bz = fp_bz * 3;
    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
		bz * sizeof (Hs_points) + xz * yz * sizeof (unsigned char *));
    fp = (Hs_points *)Wbuf;
    map = (unsigned char *)(fp + bz);
    flgs = DMAP_FILL | DMAP_NPRCD;

    ays = part->ys;		/* y start of available data */
    aye = part->ys + part->yz;	/* y end of available data */
    if (part->depend[0] >= 0)
	ays -= parts[part->depend[0]].yz;
    if (part->depend[1] >= 0)
	aye += parts[part->depend[1]].yz;
    ys = part->ys;
    nyq = part->nyq;
    thr1 = nyq * 3 / 2;
    thr2 = nyq / 2;
    wd = 2;			/* must be > 1 and < 15 */
    fcnt = 0;
    for (y = ys; y < ys + yz; y++) {
	short *cr;
	unsigned char *in, *dmap;

	cr = vdv->out + y * xz;
	in = vdv->inp + y * xz;
	dmap = vdv->dmap + y * xz;
	for (x = 1; x < xz - 1; x++) {
	    int df;
	    if (cr[x] == SNO_DATA || cr[x - 1] == SNO_DATA ||
		(dmap[x] & flgs) || (dmap[x - 1] & flgs))
		continue;
	    df = cr[x] - cr[x - 1];
	    if (df < 0)
		df = -df;
	    if (df > thr1) {
		int k, cnt, d[32], maxd[3];
		cnt = 0;
		for (k = -wd; k <= wd; k++) {
		    int ind = x + k;
		    if (ind < 0 || ind >= xz || in[ind] == BNO_DATA)
			continue;
		    d[cnt] = in[ind];
		    cnt++;
		}
		if (cnt >= wd) {
		    VDA_search_median_value (d, cnt, 
					nyq, vdv->data_off, maxd);
		    if (maxd[1] < thr2 && fcnt < bz - 1) {
			fp[fcnt].x = x - 1;
			fp[fcnt].y = y;
			fp[fcnt].v = df;
			fcnt++;
			fp[fcnt].x = x;
			fp[fcnt].y = y;
			fp[fcnt].v = df;
			fcnt++;
		    }
		}
	    }
	}
    }

    for (y = ys; y <= ys + yz; y++) {
	short *out;
	unsigned char *in, *dmap;
	int w, th2, k, off[32], no_value;

	if (y <= ays || y >= aye)
	    continue;
	no_value = 0x7fffffff;
	w = wd;
	th2 = thr2;
	if (y == ys || y == ys + yz) {	/* across partition border */
	    w = 1;
	    th2 = nyq;			/* disable smooth inp check */
	}
	for (k = -w; k <= w; k++) {
	    int ind = y + k;
	    if (ind < ays || ind >= aye)
		off[k + w] = no_value;
	    else
		off[k + w] = ((ind + vdv->yz) % vdv->yz) * xz;
	}

	out = vdv->out;
	in = vdv->inp;
	dmap = vdv->dmap;
	for (x = 0; x < xz; x++) {
	    int df, of0, of1;
	    if (off[w] == no_value || off[w - 1] == no_value)
		continue;
	    of0 = off[w] + x;
	    of1 = off[w - 1] + x;
	    if (out[of0] == SNO_DATA || out[of1] == SNO_DATA ||
		(dmap[of0] & flgs) || (dmap[of1] & flgs))
		continue;
	    df = out[of0] - out[of1];
	    if (df < 0)
		df = -df;
	    if (df > thr1) {
		int k, cnt, d[32], maxd[3];
		cnt = 0;
		for (k = -w; k <= w; k++) {
		    int of = off[k + w];
		    if (of == no_value || in[of + x] == BNO_DATA)
			continue;
		    d[cnt] = in[of + x];
		    cnt++;
		}
		if (cnt >= w) {
		    VDA_search_median_value (d, cnt, 
					nyq, vdv->data_off, maxd);
		    if (maxd[1] < th2 && fcnt < bz - 1) {
			if (y > ys) {
			    fp[fcnt].x = x;
			    fp[fcnt].y = (y - 1 + vdv->yz) % vdv->yz;
			    fp[fcnt].v = df;
			    fcnt++;
			}
			if (y < ys + yz) {
			    fp[fcnt].x = x;
			    fp[fcnt].y = y;
			    fp[fcnt].v = df;
			    fcnt++;
			}
		    }
		}
	    }
	}
    }
    if (fcnt <= 5)
	return (0);

    memset (map, 0, xz * yz * sizeof (unsigned char));
    Cfg_yz = yz;
    Cfg_xz = xz;
    Cfg_map = map;
    for (i = 0; i < fcnt; i++) {
	int v = fp[i].v;
	if (v < 2)
	    v = 2;
	if (v > 255)
	    v = 255;
	map[(fp[i].y - ys) * xz + fp[i].x] = v;
    }
    ocnt = 0;
    for (i = 0; i < fcnt; i++) {
	Point_t trvs[CFG_MAX_CNT];
	int k, minx, maxx, miny, maxy, size, pbd;

	pbd = 1;
	minx = maxx = fp[i].x;
	miny = maxy = fp[i].y - ys;
	Cfg_depth = Cfg_cnt = Cfg_maxv = 0;
	Cfg_minv = 1000;
	Travel_failed_gates (trvs, fp[i].x, fp[i].y - ys);
	for (k = 0; k < Cfg_cnt; k++) {
	    int xx = trvs[k].x;
	    int yy = trvs[k].y;
	    if (yy < miny)
		miny = yy;
	    if (yy > maxy)
		maxy = yy;
	    if (xx < minx)
		minx = xx;
	    if (xx > maxx)
		maxx = xx;
	    if (yy > 0 && yy < part->yz - 1)
		pbd = 0;		/* not on the partition border */
	}
	size = maxx - minx;
	if (maxy - miny > size)
	    size = maxy - miny;
	if (size >= 6) {
	    for (k = 0; k < Cfg_cnt; k++) {
		if (ocnt >= fp_bz)
		    break;
		rfp[ocnt].x = trvs[k].x;
		rfp[ocnt].y = trvs[k].y + ys;
		ocnt++;
	    }
	}
	if (size >= 12 && !pbd) {		/* save for vdeal export */
	    if ((Test_mode || Hsfs != NULL) && N_daffs < 50) {
		Hs_feature_t hsf;
		hsf.x = minx;
		hsf.y = miny + ys;
		hsf.xz = maxx - minx + 1;
		hsf.yz = maxy - miny + 1;
		hsf.maxs = Cfg_maxv;
		hsf.mins = Cfg_minv;
		hsf.n = Cfg_cnt;
		hsf.type = 0;
		hsf.nyq = nyq;
		hsf.unamb_range = vdv->unamb_range;
		hsf.min_size = 12;
		hsf.threshold = thr1 * .5;
		if (Test_mode)
		    VDD_log ("DAFF: %d %d %d %d    max %d min %d  n %d\n", 
		    hsf.x, hsf.y, hsf.xz, hsf.yz, hsf.maxs, hsf.mins, hsf.n);
		Hsfs = STR_append (Hsfs, &hsf, sizeof (Hs_feature_t));
		N_daffs++;
	    }
	}
    }

    return (ocnt);
}

int VDA_get_reset_hsf (int reset, void **hsfs) {

    if (reset) {
	Hsfs = STR_reset (Hsfs, 64 * sizeof (Hs_feature_t));
	N_hsfs = N_daffs = 0;
	return (0);
    }
    *hsfs = Hsfs;
    return (N_hsfs + N_daffs);
}

/************************************************************************

    Detects high shear features on vdv->out in the area of ys and yz.

************************************************************************/

double VDA_hs_threshold = 0.;	/* adaptation data */
int VDA_hs_size = 0;		/* adaptation data */

void VDA_detect_hs_features (Vdeal_t *vdv, int ys, int yz) {
    typedef struct {
	short x, y;
	short v;
    } Hs_points;
    int xz, bz, thr1, fcnt, y, x, i;
    unsigned char *map;
    Hs_points *fp;

    xz = vdv->xz;
    bz = 4096;		/* max 4096 high shear borders */
    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
		bz * sizeof (Hs_points) + xz * yz * sizeof (unsigned char *));
    fp = (Hs_points *)Wbuf;
    map = (unsigned char *)(fp + bz);

    thr1 = (int)(VDA_hs_threshold * 2. + .5);
    if (thr1 < vdv->nyq)
	thr1 = vdv->nyq;
    if (VDA_hs_size < 10)
	VDA_hs_size = 10;
    fcnt = 0;
    for (y = 0; y < yz; y++) {
	short *cr = vdv->out + (y + ys) * xz;
	for (x = 1; x < xz; x++) {
	    int df;
	    if (cr[x] == SNO_DATA || cr[x - 1] == SNO_DATA)
		continue;
	    df = cr[x] - cr[x - 1];
	    if (df < 0)
		df = -df;
	    if (df >= thr1 && fcnt < bz - 1) {
		fp[fcnt].x = x - 1;
		fp[fcnt].y = y;
		fp[fcnt].v = df;
		fcnt++;
		fp[fcnt].x = x;
		fp[fcnt].y = y;
		fp[fcnt].v = df;
		fcnt++;
	    }
	}
    }

    for (y = 1; y < yz; y++) {
	short *out = vdv->out + (y + ys) * xz;
	for (x = 0; x < xz; x++) {
	    int df;
	    short *p0, *p1;

	    p0 = out + x;
	    p1 = p0 - xz;
	    if (*p0 == SNO_DATA || *p1 == SNO_DATA)
		continue;
	    df = *p0 - *p1;
	    if (df < 0)
		df = -df;
	    if (df >= thr1 && fcnt < bz - 1) {
		fp[fcnt].x = x;
		fp[fcnt].y = y - 1;
		fp[fcnt].v = df;
		fcnt++;
		fp[fcnt].x = x;
		fp[fcnt].y = y;
		fp[fcnt].v = df;
		fcnt++;
	    }
	}
    }
    if (fcnt <= 5)
	return;

    memset (map, 0, xz * yz * sizeof (unsigned char));
    Cfg_yz = yz;
    Cfg_xz = xz;
    Cfg_map = map;
    for (i = 0; i < fcnt; i++) {
	int v = fp[i].v;
	if (v < 2)
	    v = 2;
	if (v > 255)
	    v = 255;
	map[(fp[i].y) * xz + fp[i].x] = v;
    }
    for (i = 0; i < fcnt; i++) {
	Point_t trvs[CFG_MAX_CNT];
	int k, minx, maxx, miny, maxy, size, pbd;

	pbd = 1;
	minx = maxx = fp[i].x;
	miny = maxy = fp[i].y;
	Cfg_depth = Cfg_cnt = Cfg_maxv = 0;
	Cfg_minv = 1000;
	Travel_failed_gates (trvs, fp[i].x, fp[i].y);
	for (k = 0; k < Cfg_cnt; k++) {
	    int xx = trvs[k].x;
	    int yy = trvs[k].y;
	    if (yy < miny)
		miny = yy;
	    if (yy > maxy)
		maxy = yy;
	    if (xx < minx)
		minx = xx;
	    if (xx > maxx)
		maxx = xx;
	}
	size = maxx - minx;
	if (maxy - miny > size)
	    size = maxy - miny;
	if (size >= VDA_hs_size) {		/* save for vdeal export */
	    if ((Test_mode || Hsfs != NULL) && N_hsfs < 50) {
		Hs_feature_t hsf;
		hsf.x = minx;
		hsf.y = miny + ys;
		hsf.xz = maxx - minx + 1;
		hsf.yz = maxy - miny + 1;
		hsf.maxs = Cfg_maxv;
		hsf.mins = Cfg_minv;
		hsf.n = Cfg_cnt;
		hsf.type = 1;
		hsf.nyq = vdv->nyq;
		hsf.unamb_range = vdv->unamb_range;
		hsf.min_size = VDA_hs_size;
		hsf.threshold = thr1 * .5;
		Hsfs = STR_append (Hsfs, &hsf, sizeof (Hs_feature_t));
		N_hsfs++;
	    }
	}
    }
}

/**************************************************************************

    Returns the average difference along the region border between the
    region value and the vdv->out. conn returns the connectivity
    (percentage of border points that has external bordering vdv->out).

**************************************************************************/

int VDA_border_dealiase (Vdeal_t *vdv, Region_t *reg, Part_t *part, 
						int *conn, int *bcntp) {
    int rxz, ryz, xz, yz, y, x, bcnt, tcnt, bddiff, bddc;

    xz = vdv->xz;
    yz = vdv->yz;
    rxz = reg->xz;
    ryz = reg->yz;

    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
					rxz * ryz * sizeof (unsigned char));
    Set_border_map (reg, Wbuf, 0);

    /* find the difference between the region and out for border points */
    bcnt = tcnt = bddiff = bddc = 0;
    for (y = 0; y < ryz; y++) {
	unsigned char *map;
	short *cr, *ur1, *dr1, *data;
	int yo, xo;

	data = reg->data + y * rxz;
	map = Wbuf + y * rxz;
	yo = (y + reg->ys) % yz;
	cr = vdv->out + yo * xz;		/* current row of out */
	ur1 = dr1 = NULL;
	if (vdv->full_ppi || yo > 0)
	    ur1 = vdv->out + ((yo - 1 + yz) % yz) * xz;
	if (vdv->full_ppi || yo < yz - 1)
	    dr1 = vdv->out + ((yo + 1) % yz) * xz;
	for (x = 0; x < rxz; x++) {
	    if (map[x] != 3)	/* not a region's border gate */
		continue;
	    xo = x + reg->xs;
	    /* each reg border gate can have up to 4 neighbors on vdv->out */
	    if ((x == 0 || map[x - 1] == 2) && xo > 0) {	/* left */
		short nd = cr[xo - 1];
		if (nd != SNO_DATA) {
		    bddiff += data[x] - nd;
		    bcnt++;
		}
		tcnt++;
	    }
	    if ((x == rxz - 1 || map[x + 1] == 2) && xo < xz - 1) { /* right */
		short nd = cr[xo + 1];
		if (nd != SNO_DATA) {
		    bddiff += data[x] - nd;
		    bcnt++;
		}
		tcnt++;
	    }
	    if ((y == 0 || map[x - rxz] == 2) && ur1 != NULL) {	/* up */
		short nd = ur1[xo];
		if (nd != SNO_DATA) {
		    bddiff += data[x] - nd;
		    bcnt++;
		}
		else if (yo == part->ys)  /* neighbor outside the partition */
		    continue;		/* not inc tcnt */
		tcnt++;
	    }
	    if ((y == ryz - 1 || map[x + rxz] == 2) && dr1 != NULL) {/* down */
		short nd = dr1[xo];
		if (nd != SNO_DATA) {
		    bddiff += data[x] - nd;
		    bcnt++;
		}
		else if (yo == part->ys + part->yz - 1)
		    continue;	/* neighbor outside the part - not inc tcnt */
		tcnt++;
	    }
	}
    }

    if (conn != NULL)
	*conn = bcnt * 100 / tcnt;
    if (bcntp != NULL)
	*bcntp = bcnt;
    return (Myround ((double)-bddiff / bcnt));
}

/**************************************************************************

    Checks how the border of region "reg" connects to available data on
    "vdv->out", if use_out is true, or on "vdv->inp" otherwise. Returns
    the percentage of region border gates that have no missing neighnor
    outside the region.

**************************************************************************/

int VDA_check_border_conn (Vdeal_t *vdv, Region_t *reg, int use_out, int fppi) {
    int rxz, ryz, xz, yz, y, x, w, ccnt, tcnt;

    xz = vdv->xz;
    yz = vdv->yz;
    rxz = reg->xz;
    ryz = reg->yz;

    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
					rxz * ryz * sizeof (unsigned char));
    Set_border_map (reg, Wbuf, fppi);

    /* set map to 4 if a border gate connects to a missing external */
    if (use_out) {
	for (y = 0; y < ryz; y++) {
	    unsigned char *map;
	    short *cr, *ur, *dr, *data;
	    int yo, xo;
    
	    data = reg->data + y * rxz;
	    map = Wbuf + y * rxz;
	    yo = y + reg->ys;
	    cr = vdv->out + (yo % yz) * xz;	/* current row of out */
	    if (vdv->full_ppi) {
		ur = vdv->out + ((yo - 1 + yz) % yz) * xz;	/* row up */
		dr = vdv->out + ((yo + 1) % yz) * xz;	/* row down */
	    }
	    else {
		ur = dr = NULL;
		if (yo > 0)
		    ur = vdv->out + (yo - 1) * xz;
		if (yo < yz - 1)
		    dr = vdv->out + (yo + 1) * xz;
	    }
	    for (x = 0; x < rxz; x++) {
		if (map[x] != 3)	/* not a region's border gate */
		    continue;
		xo = x + reg->xs;
		if (x == 0 || map[x - 1] == 2) {	/* left */
		    if (xo == 0 || cr[xo - 1] == SNO_DATA)
			map[x] = 4;
		}
		if (x == rxz - 1 || map[x + 1] == 2) {	/* right */
		    if (xo == xz - 1 || cr[xo + 1] == SNO_DATA)
			map[x] = 4;
		}
		if (y == 0 || map[x - rxz] == 2) {	/* up */
		    if (ur == NULL || ur[xo] == SNO_DATA)
			map[x] = 4;
		}
		if (y == ryz - 1 || map[x + rxz] == 2) {	/* down */
		    if (dr == NULL || dr[xo] == SNO_DATA)
			map[x] = 4;
		}
	    }
	}
    }
    else {
	for (y = 0; y < ryz; y++) {
	    unsigned char *map, *cr, *ur, *dr;
	    short *data;
	    int yo, xo;
    
	    data = reg->data + y * rxz;
	    map = Wbuf + y * rxz;
	    yo = y + reg->ys;
	    cr = vdv->inp + (yo % yz) * xz;		/* current row of out */
	    if (vdv->full_ppi) {
		ur = vdv->inp + ((yo - 1 + yz) % yz) * xz;	/* row up */
		dr = vdv->inp + ((yo + 1) % yz) * xz;	/* row down */
	    }
	    else {
		ur = dr = NULL;
		if (yo > 0)
		    ur = vdv->inp + (yo - 1) * xz;
		if (yo < yz - 1)
		    dr = vdv->inp + (yo + 1) * xz;
	    }
	    for (x = 0; x < rxz; x++) {
		if (map[x] != 3)	/* not a region's border gate */
		    continue;
		xo = x + reg->xs;
		if (x == 0 || map[x - 1] == 2) {	/* left */
		    if (xo == 0 || cr[xo - 1] == BNO_DATA)
			map[x] = 4;
		}
		if (x == rxz - 1 || map[x + 1] == 2) {	/* right */
		    if (xo == xz - 1 || cr[xo + 1] == BNO_DATA)
			map[x] = 4;
		}
		if (y == 0 || map[x - rxz] == 2) {	/* up */
		    if (ur == NULL || ur[xo] == BNO_DATA)
			map[x] = 4;
		}
		if (y == ryz - 1 || map[x + rxz] == 2) {	/* down */
		    if (dr == NULL || dr[xo] == BNO_DATA)
			map[x] = 4;
		}
	    }
	}
    }
    /* expand 4 (set to 5) so it is better for noisy data */
    w = 1;
    for (y = 0; y < ryz; y++) {
	unsigned char *map;
	int yi, xi, yy, xx, off;

	map = Wbuf + y * rxz;
	for (x = 0; x < rxz; x++) {
	    if (map[x] != 4)
		continue;
	    for (yi = -w; yi <= w; yi++) {
		yy = y + yi;
		if (yy < 0 || yy >= ryz)
		    continue;
		for (xi = -w; xi <= w; xi++) {
		    xx = x + xi;
		    if (xx < 0 || xx >= rxz)
			continue;
		    off = yy * rxz + xx;
		    if (Wbuf[off] == 3)
			Wbuf[off] = 5;
		}
	    }
	}
    }

    /* count connected and and total border gates */
    ccnt = tcnt = 0;
    for (y = 0; y < ryz; y++) {
	unsigned char *map;

	map = Wbuf + y * rxz;
	for (x = 0; x < rxz; x++) {
	    if (map[x] >= 3) {
		tcnt++;
		if (map[x] == 3)
		   ccnt++;
	    }
	}
    }
    if (tcnt == 0)
	tcnt = 1;
    return (ccnt * 100 / tcnt);
}

/**************************************************************************

    Sets the border map "map" for region "reg". 0 - no data; 2 - gates 
    outside the region; 3 - border gate of the region; 1 otherwise.

**************************************************************************/

static void Set_border_map (Region_t *reg, unsigned char *map, int fppi) {
    int rxz, ryz, y, x;

    rxz = reg->xz;
    ryz = reg->yz;

    for (y = 0; y < ryz; y++) {
	unsigned char *map;
	short *data;

	map = Wbuf + y * rxz;
	data = reg->data + y * rxz;
	for (x = 0; x < rxz; x++) {
	    if (data[x] == SNO_DATA)
		map[x] = 0;
	    else
		map[x] = 1;
	}
    }

    /* set map to 2 for gates external to the region */
    G_xz = rxz;		/* pass vars to Set_external_map */
    G_yz = ryz;
    G_map = Wbuf;
    G_fppi = fppi;
    for (y = 0; y < ryz; y++) {
	if (!fppi && (y == 0 || y == ryz - 1)) {
	    for (x = 0; x < rxz; x++) {
		G_cnt = 0;
		Set_external_map (x, y);
	    }
	}
	else {
	    G_cnt = 0;
	    Set_external_map (0, y);
	    G_cnt = 0;
	    Set_external_map (rxz - 1, y);
	}
    }

    /* set region border gates on the map */
    for (y = 0; y < ryz; y++) {
	unsigned char *map, *mpr, *mnx;

	map = Wbuf + y * rxz;
	mpr = NULL;
	if (fppi || y > 0)
	    mpr = Wbuf + ((y - 1 + ryz) % ryz) * rxz;
	mnx = NULL;
	if (fppi || y < ryz - 1)
	    mnx = Wbuf + ((y + 1) % ryz) * rxz;
	for (x = 0; x < rxz; x++) {
	    if (map[x] != 1)
		continue;
	    if (x == 0 || map[x - 1] == 2 ||
		x == rxz - 1 || map[x + 1] == 2 ||
		(y == 0 && !fppi) || (mpr != NULL && mpr[x] == 2) ||
		(y == ryz - 1 && !fppi) || (mnx != NULL && mnx[x] == 2))
		map[x] = 3;		/* border */
	}
    }
}

/**************************************************************************

    Recusively travels through all pixels in the region and finds out the 
    external map of the region.

**************************************************************************/

static void Set_external_map (int x, int y) {

    if (G_cnt > 20000) {	/* not fatal - should not happen normally */
	return;
    }
    G_cnt++;
    if (G_map[y * G_xz + x] == 0) {
	G_map[y * G_xz + x] = 2;
	if (x > 0)
	    Set_external_map (x - 1, y);
	if (x < G_xz - 1)
	    Set_external_map (x + 1, y);
	if (G_fppi) {
	    Set_external_map (x, (y - 1 + G_yz) % G_yz);
	    Set_external_map (x, (y + 1) % G_yz);
	}
	else {
	    if (y > 0)
		Set_external_map (x, y - 1);
	    if (y < G_yz - 1)
		Set_external_map (x, y + 1);
	}
    }
    G_cnt--;
}

/***************************************************************************

    Checks if the region "reg" fits in the output with global dealiasing 
    "gd". Returns the number of border gates that does not fit in the 
    output (difference > thr).

***************************************************************************/

int VDA_check_fit_out (Vdeal_t *vdv, Region_t *reg, 
				int gd, int thr, int *bcntp) {
    int rxz, ryz, rxs, rys, xz, yz, y, x, fpo, fpr, cnt, bcnt;

    xz = vdv->xz;
    yz = vdv->yz;
    rxz = reg->xz;
    ryz = reg->yz;
    rxs = reg->xs;
    rys = reg->ys;
    fpo = fpr = 0;
    if (vdv->full_ppi) {
	fpo = 1;
	if (ryz == yz)
	    fpr = 1;
    }
    cnt = bcnt = 0;
    for (y = 0; y < ryz; y++) {
	short *cr, *out;
	int offr[16], offo[16], yy;
	unsigned char *dm;

	yy = (y + rys) % yz;
	cr = reg->data + y * rxz;
	out = vdv->out + yy * xz;
	dm = vdv->dmap + yy * xz;	/* region's dmap */
	VDA_get_neighbor_offset (4, y, rxz, ryz, fpr, offr);
	VDA_get_neighbor_offset (4, yy, xz, yz, fpo, offo);
	for (x = 0; x < rxz; x++) {
	    int c, *ofrs, *ofos, k, xx, max;

	    c = cr[x];
	    if (c == SNO_DATA)		/* current gate has no data */
		continue;
	    xx = x + rxs;
	    if (dm[xx] & DMAP_BH)	/* current gate is BH */
		continue;
	    if (x == 0)
		ofrs = offr + 4;
	    else if (x == rxz - 1)
		ofrs = offr + 8;
	    else
		ofrs = offr;
	    if (xx == 0)
		ofos = offo + 4;
	    else if (xx == xz - 1)
		ofos = offo + 8;
	    else
		ofos = offo;
	    max = -1;
	    for (k = 0; k < 4; k++) {	/* 4 neighbors */
		int ofr, ofo, diff, ot;

		ofr = ofrs[k];
		if (ofr == 0)		/* no neighbor */
		    continue;
		ofo = ofos[k];
		if (ofo == 0)		/* no output neighbor */
		    continue;
		if (cr[x + ofr] != SNO_DATA && !(dm[xx + ofo] & DMAP_BH))
		    continue;		/* neighbor has data and not BH */
		ot = out[xx + ofo];	/* output value of the neighbor */
		if (ot == SNO_DATA)
		    continue;		/* no output data */
    
		diff = c + gd - ot;
		if (diff < 0)
		    diff = -diff;
		if (diff > max)
		    max = diff;
	    }
	    if (max >= 0)
		bcnt++;
	    if (max > thr)
		cnt++;
	}
    }
    if (bcntp != NULL)
	*bcntp = bcnt;
    return (cnt);
}

/***************************************************************************

    Finds those gates that have high shear in "reg" while the data are
    smooth in their neighborhood in "vdv->inp". Returns the maximum length of 
    line formed by such points. We dialate here to remove small gaps in
    the lines. We do not need to "skeletoning" this because we only need
    to find the line lengths. Because we are interested in spacial span
    of the line, we find the size of the box containing the line instead 
    of the line length. fp is true if the image is full PPI.

***************************************************************************/

int VDA_detect_false_shear (Vdeal_t *vdv, Region_t *reg, int thr, int *maxwp) {
    int xz, yz, fp;
    int rxz, ryz, xs, ys, x, y, cnt, thr2, i, n_dialates, maxd, maxw;
    int fpo, fpr, nyq;
    unsigned char *inp;

    xz = vdv->xz;
    yz = vdv->yz;
    inp = vdv->inp;
    fp = vdv->full_ppi;
    rxz = reg->xz;
    ryz = reg->yz;
    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
					rxz * ryz * sizeof (unsigned char));
    thr2 = thr / 8;
    nyq = VDD_get_nyq (vdv, reg->ys);
    xs = reg->xs;
    ys = reg->ys;
    fpo = fpr = 0;
    if (fp) {
	fpo = 1;
	if (ryz == yz)
	    fpr = 1;
    }
    cnt = 0;
    for (y = 0; y < ryz; y++) {
	short *cr;
	int off4[16], off8[32], yy;
	unsigned char *in, *map;

	yy = (y + ys) % yz;
	cr = reg->data + y * rxz;
	in = inp + yy * xz;
	map = Wbuf + y * rxz;
	VDA_get_neighbor_offset (4, y, rxz, ryz, fpr, off4);
	VDA_get_neighbor_offset (8, yy, xz, yz, fpo, off8);
	for (x = 0; x < rxz; x++) {
	    int c, *ofs, n, maxd[3], k, d[16], xx;

	    map[x] = 0;
	    c = cr[x];
	    if (c == SNO_DATA)
		continue;
	    if (x == 0)
		ofs = off4 + 4;
	    else if (x == rxz - 1)
		ofs = off4 + 8;
	    else
		ofs = off4;
	    for (k = 0; k < 4; k += 2) {	/* left and upper neighbors */
		int n, diff;
		n = cr[x + ofs[k]];
		if (n == SNO_DATA)
		    continue;
		diff = n - c;
		if (diff < 0)
		    diff = -diff;
		if (diff > thr)
		    break;
	    }
	    if (k >= 4)			/* no high shear detected here */
		continue;
	    xx = x + xs;
	    if (xx == 0)
		ofs = off8 + 8;
	    else if (xx == xz - 1)
		ofs = off8 + 16;
	    else
		ofs = off8;
	    n = 0;
	    for (k = 0; k < 9; k++) {
		int of;
		if (k < 8) {
		    of = ofs[k];
		    if (of == 0)
			continue;
		}
		else			/* the center point */
		    of = 0;
		if (in[xx + of] == BNO_DATA)
		    continue;
		d[n] = in[xx + of];
		n++;
	    }
	    if (n >= 4 &&
		(VDA_search_median_value (d, n, nyq, 
			vdv->data_off, maxd) > 0 && maxd[0] < thr2)) {
		map[x] = 1;
		cnt++;
	    }
	}
    }
    if (cnt == 0)
	return (0);

    n_dialates = 2; 
    for (i = 0; i < n_dialates; i++) {		/* dialate to remove gaps */
	for (y = 0; y < ryz; y++) {
	    unsigned char *map;
	    int off8[32], *ofs, k;

    	    VDA_get_neighbor_offset (8, y, rxz, ryz, fpr, off8);
	    map = Wbuf + y * rxz;
	    for (x = 0; x < rxz; x++) {
		if (map[x] == 0 || map[x] != i + 1)
		    continue;
		if (x == 0)
		    ofs = off8 + 8;
		else if (x == rxz - 1)
		    ofs = off8 + 16;
		else
		    ofs = off8;
		for (k = 0; k < 8; k++) {
		     if (ofs[k] != 0 && map[x + ofs[k]] == 0)
			map[x + ofs[k]] = i + 2;
		}
	    }
	}
    }

    G_xz = rxz;
    G_yz = ryz;
    G_map = Wbuf;
    maxd = maxw = 0;
    for (y = 0; y < ryz; y++) {
	unsigned char *map;

	map = Wbuf + y * rxz;
	for (x = 0; x < rxz; x++) {
	    int dist;

	    if (map[x] == 0)
		continue;
	    Tl_xmin = Tl_xmax = x;
	    Tl_ymin = Tl_ymax = y;
	    G_cnt = 0;
	    Travel_line (x, y, fpr);
	    dist = (Tl_xmax - Tl_xmin) * (Tl_xmax - Tl_xmin);
	    dist += (Tl_ymax - Tl_ymin) * (Tl_ymax - Tl_ymin);
	    if (dist > maxd)
		maxd = dist;
	    dist = Tl_ymax - Tl_ymin;
	    if (dist > maxw)
		maxw = dist;
	}
    }
    maxd = Myround (sqrt ((double)maxd)) - 2 * n_dialates;
    if (maxd < 0)
	maxd = 0;
    if (maxwp != NULL)
	*maxwp = maxw - 2 * n_dialates;

    return (maxd);
}

/***************************************************************************

    Computes the histogram of the border shears of "inp" of sizes "xz", "yz".
    The result is returned with "histp". "fp" indicates that inp is full ppi.

***************************************************************************/

int VDA_Compute_shear_hist (unsigned char *inp, int xz, int yz, 
					int stride, int fp, int **histp) {
    static __thread int hist[256];
    unsigned char *cr;
    int xz1, x, y, i, k, off[16], cnt;

    for (i = 0; i < 256; i++)
	hist[i] = 0;
    *histp = hist;
    xz1 = xz - 1;
    cnt = 0;
    for (y = 0; y < yz; y++) {

	cr = inp + y * stride;
	VDA_get_neighbor_offset (4, y, stride, yz, fp, off);
	for (x = 0; x < xz; x++) {
	    int c, *ofs;

	    c = cr[x];
	    if (c == BNO_DATA)
		continue;
	    if (x == 0)
		ofs = off + 4;
	    else if (x == xz1)
		ofs = off + 8;
	    else
		ofs = off;
	    for (k = 0; k < 4; k += 2) {
		int n, diff;
		n = cr[x + ofs[k]];
		if (n == BNO_DATA)
		    continue;
		diff = n - c;
		if (diff < 0)
		    diff = -diff;
		if (diff >= 255)
		    diff = 255;
		hist[diff]++;
		cnt++;
	    }
	}
    }
    return (cnt);
}

/***************************************************************************

    Computes the histogram of the velocity of "inp" of sizes "xz", "yz".
    The result is returned with "histp".

***************************************************************************/

int VDA_compute_data_hist (unsigned char *inp, int stride, int xs, 
					int xz, int yz, int **histp) {
    static __thread int hist[256];
    int x, y, i, cnt;

    for (i = 0; i < 256; i++)
	hist[i] = 0;
    *histp = hist;
    cnt = 0;
    for (y = 0; y < yz; y++) {

	unsigned char *cr = inp + y * stride;
	for (x = 0; x < xz; x++) {
	    int c = cr[x + xs];
	    if (c == BNO_DATA)
		continue;
	    if (c < 0)
		c = 0;
	    else if (c > 255)
		c = 255;
	    hist[c]++;
	    cnt++;
	}
    }
    return (cnt);
}

/**************************************************************************

    Returns the dealiased median value of array "d" of size "n". If
    maxdp is not NULL, maxdp[0] returns the max deviation from the
    median, maxdp[1] returns the difference between the data max and
    min. maxdp[2] returns the data span after removing 8 percent of the
    outliers. This function assumes that the input values are within the
    nyquist range. If multiple data are equally good median value, the
    smaller one is returned. The algorithm of the first part is better
    in performance especially when n is small. It is also more efficient
    for small n. The second algorithm is more efficient for large n.

**************************************************************************/

int VDA_search_median_value (int *d, int n, int nyq, int d_off, int *maxdp) {
    int nyq2, min, max, hist[256], i, s, j, hs, cnt;
    int ecnt, md, w, m, v;

#define DEAL_VALUE(x,min,max) \
		if (x < min) x += nyq2; else if (x >= max) x -= nyq2

    nyq2 = 2 * nyq;
    if (n == 0)
	return (0);
    else if (n == 1) {
	if (maxdp != NULL)
	    maxdp[0] = maxdp[1] = maxdp[2] = 0;
	return (d[0]);
    }
    else if (n <= 12) {	/* the alg is better and more efficient for small n */
	int mdi = 0, df, min;
	min = 0x7fffffff;
	for (i = 0; i < n; i++) {
	    s = 0;
	    for (j = 0; j < n; j++) {
		if (j == i)
		    continue;
		df = d[j] - d[i];
		DEAL_VALUE (df, -nyq, nyq);
		if (df < 0)
		    df = -df;
		s += df;
	    }
	    if (s < min) {
		min = s;
		mdi = i;
	    }
	    else if (s == min) {
		df = d[i] - d[mdi];
		DEAL_VALUE (df, -nyq, nyq);
		if (df < 0)
		    mdi = i;
	    }
	}

	if (maxdp != NULL) {
	    int mi, ma, mi1, ma1, d95;
	    mi = mi1 = 1;
	    ma = ma1 = -1;
	    for (j = 0; j < n; j++) {
		df = d[j] - d[mdi];
		DEAL_VALUE (df, -nyq, nyq);
		if (df <= mi) {
		    mi1 = mi;
		    mi = df;
		}
		if (df >= ma) {
		    ma1 = ma;
		    ma = df;
		}
	    }
	    maxdp[1] = ma - mi;
	    d95 = maxdp[1];
	    if (n >= 9) {
		if (ma1 >= 0 && ma1 - mi < d95)
		    d95 = ma1 - mi;
		if (mi1 <= 0 && ma - mi1 < d95)
		    d95 = ma - mi1;
	    }
	    maxdp[2] = d95;
	    mi = -mi;
	    maxdp[0] = ma >= mi? ma : mi;
	}
	return (d[mdi]);
    }

    min = d_off - nyq;
    max = d_off + nyq;
    w = nyq >> 1;
    ecnt = 0;
    for (i = min; i < max; i++)
	hist[i] = 0;
    for (i = 0; i < n; i++) {
	int ind = d[i];
	if (ind >= max) {
	    ind = min;
	    ecnt++;
	}
	else if (ind <= min) {
	    ind = min;
	    ecnt--;
	}
	hist[ind]++;
    }

    m = v = cnt = s = 0;
    for (i = min; i < max; i++) {
	if (i == min) {
	    for (j = -w; j <= w; j++) {
		int ind = i + j;
		DEAL_VALUE (ind, min, max);
		s += hist[ind];
	    }
	}
	else {
	    int ind, ind1;
	    ind = i + w;
	    DEAL_VALUE (ind, min, max);
	    ind1 = i - w - 1;
	    DEAL_VALUE (ind1, min, max);
	    s = s + hist[ind] - hist[ind1];
	}
	if (s > m) {
	    m = s;
	    v = i;
	    cnt = s;
	}
    }
    s = (cnt + 1) / 2;
    hs = md = 0;
    for (j = v - w; j <= v + w; j++) {
	int i = j;
	DEAL_VALUE (i, min, max);
	hs += hist[i];
	if (hs >= s) {
	    md = i;
	    break;
	}
    }

    if (maxdp != NULL) {
	int k, mi, ma, diff, up, low, ms, c, n05, mi1, ma1, d95;

	if (cnt != n) {
	    up = md + nyq - 1;
	    low = md - nyq;
	}
	else {
	    up = md + w;
	    low = md - w;
	}

	n05 = (n * 8 + 50) / 100;
	mi = mi1 = low - 1;
	ma = ma1 = up + 1;
	ms = md - low;
	if (up - md > ms)
	    ms = up - md;
	c = 0;
	for (k = ms; k >= 0; k--) {
	    int kkl, kku;
	    kkl = md - k;
	    if (kkl >= low) {
		int i = kkl;
		DEAL_VALUE (i, min, max);
		if (hist[i] != 0) {
		    if (mi < low)
			mi = kkl;
		    c += hist[i];
		}
		else
		    kkl = low - 1;
	    }
	    kku = md + k;
	    if (kku <= up) {
		int i = kku;
		DEAL_VALUE (i, min, max);
		if (hist[i] != 0) {
		    if (ma > up)
			ma = kku;
		    c += hist[i];
		}
		else
		    kku = up + 1;
	    }
	    if (c > n05 && mi >= low && ma <= up)
		break;
	    if (kkl >= low)
		mi1 = kkl;
	    if (kku <= up)
		ma1 = kku;
	}

	diff = ma - md;
	if (md - mi > diff)
	    diff = md - mi;
	maxdp[0] = diff;
	maxdp[1] = ma - mi;
	d95 = maxdp[1];
	if (ma1 - mi < d95)
	    d95 = ma1 - mi;
	if (ma - mi1 < d95)
	    d95 = ma - mi1;
	maxdp[2] = d95;
    }


    if (md == min && ecnt >= 0)
	md = max;


    return (md);
}

/***************************************************************************

    Calculates the offsets for the 4 or 8 neighbors for row "y" in an area
    of xz by yz. Three tables are set for the x not on the boundary, x = 0
    and x = xz - 1 respectively. "off" must have suffient size for holding
    the three tables. Offset is set to 0 if a neighbor is out of the area.
    If "fp" is true, the area is of full ppi. If n is not 4 or 8, a window
    of wx = 1 (wy = n) is assumed. Since the center point is included, -1
    is used if neighbor is out of the area.

***************************************************************************/

void VDA_get_neighbor_offset (int n, int y, int xz, int yz, int fp, int *off) {
    int i;

    if (n == 4) {
	off[0] = -1;
	off[1] = 1;
	off[2] = -xz;
	off[3] = xz;
	if (y == 0) {
	    if (fp)
		off[2] = (yz - 1) * xz;
	    else
		off[2] = 0;
	}
	if (y == yz - 1) {
	    if (fp)
		off[3] = -(yz - 1) * xz;
	    else
		off[3] = 0;
	}
	for (i = 0; i < 4; i++) {
	    off[4 + i] = off[i];
	    off[8 + i] = off[i];
	}
	off[4 + 0] = 0;		/* for x = 0 */
	off[8 + 1] = 0;		/* for x = xz - 1 */
	if (xz == 1) {
	    off[4 + 1] = 0;
	    off[8 + 0] = 0;
	}
    }
    else if (n == 8) {
	int proff, nroff;

	proff = -xz;
	nroff = xz;
	if (y == 0) {
	    if (fp)
		proff = (yz - 1) * xz;
	    else
		proff = 0;
	}
	if (y == yz - 1) {
	    if (fp)
		nroff = -(yz - 1) * xz;
	    else
		nroff = 0;
	}

	off[7] = proff - 1;
	off[0] = proff;
	off[1] = proff + 1;
	if (proff == 0)
	    off[7] = off[1] = 0;
	off[2] = 1;
	off[3] = nroff + 1;
	off[4] = nroff;
	off[5] = nroff - 1;
	if (nroff == 0)
	    off[3] = off[5] = 0;
	off[6] = -1;

	for (i = 0; i < 8; i++) {
	    off[8 + i] = off[i];
	    off[16 + i] = off[i];
	}
	off[8 + 5] = off[8 + 6] = off[8 + 7] = 0;	/* for x = 0 */
	off[16 + 1] = off[16 + 2] = off[16 + 3] = 0;	/* for x = xz - 1 */
	if (xz == 1) {
	    off[8 + 1] = off[8 + 2] = off[8 + 3] = 0;
	    off[16 + 5] = off[16 + 6] = off[16 + 7] = 0;
	}
    }
    else {
	int hb, i;
	hb = n / 2;
	for (i = 0; i < n; i++) {
	    int yy = y + i - hb;
	    if (fp) {
		int ind = yy;
		if (ind < 0)
		    ind += yz;
		else if (ind >= yz)
		    ind -= yz;
		off[i] = (ind - y) * xz;
	    }
	    else {
		if (yy < 0 || yy >= yz)
		    off[i] = -1;
		else
		    off[i] = (yy - y) * xz;
	    }
	}
    }
}

/**************************************************************************

    Recusively travels through all pixels in the line and finds out the 
    size of the line. The searching direction is randomized to reduce the 
    depth of recursive calls.

**************************************************************************/

static void Travel_line (int x, int y, int fp) {
    int i;

    if (G_cnt < 0)
	return;
    if (G_cnt > 20000) {
	VDD_log ("Too many recursive calls (%d) to Travel_line\n", G_cnt);
	G_cnt = -G_cnt;
	return;
    }
    G_cnt++;
    G_map[((y + G_yz) % G_yz)* G_xz + x] = 0;
    for (i = 0; i < 4; i++) {
	int xx, yy, ii;
	ii = (i + y) % 4;
	if (ii == 0) {
	    if (x <= 0)
		continue;
	    xx = x - 1;
	    yy = y;
	}
	else if (ii == 2) {
	    if (x >= G_xz - 1)
		continue;
	    xx = x + 1;
	    yy = y;
	}
	else if (ii == 1) {
	    yy = y - 1;
	    if (!fp && yy < 0)
		continue;
	    xx = x;
	}
	else {
	    yy = y + 1;
	    if (!fp && yy >= G_yz)
		continue;
	    xx = x;
	}
	if (G_map[((yy + G_yz) % G_yz)* G_xz + xx] > 0) {
	    if (yy > Tl_ymax)
		Tl_ymax = yy;
	    if (yy < Tl_ymin)
		Tl_ymin = yy;
	    if (xx > Tl_xmax)
		Tl_xmax = xx;
	    if (xx < Tl_xmin)
		Tl_xmin = xx;
	    Travel_line (xx, yy, fp);
	}
    }
    G_cnt--;
}


/***************************************************************************

    Eliminates, by removing some gates, thin connections in each region
    of "inp" of sizes "xz" by "yz". dft, if not NULL, is the data
    filter. Returns the number of removed gates. "level" is the level of
    thinning (each level removes connection of two gates). It must be
    less than 8. If outmap == NULL, the thinning gates in inp are set to
    no-data. Otherwise, mapv is ORed to outmap. mapv can be any value
    regardless of existing values in dft. "fp" indicates that inp is
    full ppi. Small internal data holes surrounded by good data are
    excluded when searching the thin connections.

***************************************************************************/

typedef struct {	/* gate separating two regions */
    unsigned short x, y;	/* location of the gate */
    unsigned short id1, id2;	/* id1 < id2; ids of the regions separate */
} Thin_gate_t;

typedef struct {	/* borders between two regions */
    unsigned short n;		/* # break gates of the border */
    unsigned short off;		/* location (offset in xys) of border points */
    unsigned short id1, id2;	/* ids of the regions separate */
} Border_t;		/* this must have same binary fields as Thin_gate_t */

typedef struct {	/* coordinate list of border points */
    unsigned short x, y;
    unsigned short next;
} Coord_t;

static int Tps_cmp (const void *tp1p, const void *tp2p);
static int Merge_cmp (const void *tp1p, const void *tp2p);
static int Bd_cmp (const void *bd1p, const void *bd2p);
static int Fill_in_holes (unsigned char *inp, unsigned char *wbuf, 
			int xz, int yz, int fp, int v, int nyq, int d_off);
static int Add_reflection (Border_t *bds, int b_cnt);
static int Search_section (Border_t *bds, int b_cnt, 
				unsigned short id, int gi, int *stp);
static int Min_border_size (int a_size);
static int Join_two_regions (Border_t *bds, int b_cnt, Border_t*nbds,
		int st, int n, int st2, int n2, 
				unsigned char *flags, int id_for_rm);
static void Break_thined_region (unsigned char *mapbuf, int xz, int yz,
					Region_t *region, int maxs);
static int Put_back_joined (Border_t *bds, int b_cnt,
			Border_t*nbds, int nb_cnt, Coord_t *xys,
				unsigned char *flags, int id_for_rm);
static void Generate_break_map (unsigned char *mapbuf, int xz, int yz,
				int level, int fp, unsigned char *rd_buf);
static int Generate_idmap (unsigned char *mapbuf, int xz, int yz,
		unsigned short *idmap, int fp, unsigned char *rd_buf);
static void Remove_break (Border_t *bd, Coord_t *xys, 
				unsigned char *map, int xz, int yz);
static int Rm_break_between_well_conn_regs (unsigned char *mapbuf,
	unsigned short *idmap, int xz, int yz, int fp, Thin_gate_t *tps,
	Coord_t *xys, unsigned char *flags, unsigned char *rd_buf);


#define BD_BUF_SZ 50000		/* max number of border gates in prosesing */
#define MAX_ID 0xfffe		/* max region id - ffff reserved */
#define RG_CHANGED 0x1		/* the range has been changed */
#define BD_REMOVED 0x2		/* the border has been removed */

static __thread unsigned short *Rg_size;	/* region sizes array */

int VDA_find_thin_conn (unsigned char *inp, int xz, int yz, int level, 
			int fp, Data_filter_t *dft, int nyq, int d_off,
			unsigned char *outmap, unsigned char mapv) {
    int v, x, y;
    int out_cnt, i;
    unsigned char *map, *mapbuf, *flags, *rd_buf;
    unsigned short *idmap;
    unsigned char ybits, ebits, *dmap, *dmapp;
    Thin_gate_t *tps;
    Coord_t *xys;


    {		/* malloc buffers */
	int s1, s2;
	unsigned char *b;
	s1 = xz * yz * sizeof (short);
	s2 = BD_BUF_SZ * sizeof (Thin_gate_t);
	s1 = s1 >= s2 ? s1 : s2;
	Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, s1 + 
		2 * xz * yz * sizeof (unsigned char) + 
		BD_BUF_SZ * (sizeof (Thin_gate_t) + sizeof (Coord_t) + 
					sizeof (unsigned char)) + 
		(MAX_ID + 2) * sizeof (short));
	b = Wbuf;
	tps = (Thin_gate_t *)b;
	b += BD_BUF_SZ * sizeof (Thin_gate_t);
	xys = (Coord_t *)b;
	b += BD_BUF_SZ * sizeof (Coord_t);
	idmap = (unsigned short *)b;
	b += xz * yz * sizeof (unsigned short);
	Rg_size = (unsigned short *)b;
	b += (MAX_ID + 2)  * sizeof (short);
	flags = (unsigned char *)b;
	b += BD_BUF_SZ * sizeof (unsigned char);
	mapbuf = (unsigned char *)b;
	rd_buf = mapbuf + xz * yz * sizeof (unsigned char);
    }

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
    v = 31;				/* map value for data available */
    for (y = 0; y < yz; y++) {		/* init the map */

	unsigned char *cr = inp + y * xz;
	map = mapbuf + y * xz;
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

    Fill_in_holes (inp, mapbuf, xz, yz, fp, v, nyq, d_off);
    VDA_thin_and_dialate (mapbuf, xz, yz, fp, v, level);

    Generate_break_map (mapbuf, xz, yz, level, fp, rd_buf);

    for (i = 0; i < 10; i++) {
	if (Rm_break_between_well_conn_regs (mapbuf, idmap,
			xz, yz, fp, tps, xys, flags, rd_buf) == 0)
	    break;
    }

    out_cnt = 0;
    for (y = 0; y < yz; y++) {
	map = mapbuf + y * xz;
	for (x = 0; x < xz; x++) {
	    if (map[x] == 1) {
		int of = y * xz + x;
		if (outmap != NULL)
		    outmap[of] |= mapv;
		else
		    inp[of] = BNO_DATA;
		out_cnt++;
	    }
	}
    }

    return (out_cnt);
}

/***************************************************************************

    Removes break gates separating two well connected regions. The break
    gates are removed from mapbuf. Returns true if any break is removed.

***************************************************************************/

static int Rm_break_between_well_conn_regs (unsigned char *mapbuf,
	unsigned short *idmap, int xz, int yz, int fp, Thin_gate_t *tps,
	Coord_t *xys, unsigned char *flags, unsigned char *rd_buf) {
    int xz1, i, x, y, g_cnt, b_cnt, rm_cnt, joined;
    int id_for_rm, next_id;
    unsigned char *map;
    unsigned short *smap;
    Border_t *bds, *nbds;

    xz1 = xz - 1;
    next_id = Generate_idmap (mapbuf, xz, yz, idmap, fp, rd_buf);
    id_for_rm = MAX_ID + 1;		/* record to be removed */

    /* Rremoves fake breaking gates and builds the breaking gate list with 
       region ids they separate. */
    for (i = 0; i < 10; i++) {
	int changed = 0;
	g_cnt = 0;
	for (y = 0; y < yz; y++) {
	    int off[32]; 
	    VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	    smap = idmap + y * xz;
	    map = mapbuf + y * xz;
	    for (x = 0; x < xz; x++) {
		int cnt, minid, maxid, nm, *ofs, k;
		if (map[x] != 1)
		    continue;
		if (x == 0)
		    ofs = off + 4;
		else if (x == xz1)
		    ofs = off + 8;
		else
		    ofs = off;
		cnt = 0;
		minid = MAX_ID + 1;
		maxid = 0;
		for (k = 0; k < 4; k++) {
		    if (ofs[k] == 0)
			continue;
		    nm = smap[x + ofs[k]];
		    if (nm >= 1) {
			if (nm == minid || nm == maxid)
			    continue;
			cnt++;
			if (nm < minid)
			    minid = nm;
			if (nm > maxid)
			    maxid = nm;
		    }
		}
		if (cnt <= 1) {	/* not separating two regions - removed */
		    if (cnt == 1)	
			smap[x] = minid; /* necessary to avoid over removing */
		    map[x] = 2;
		    changed = 1;
		    continue;
		}
		if (cnt >= 3)	/* border of more than 2 regions */
		    continue;
		if (changed)
		    continue;
		if (g_cnt >= BD_BUF_SZ)  /* leave remaining break unprocess */
		    break;
		if (minid == 1 || maxid == 1)  /* border of non-id small rgs */
		    continue;		/* break not processed */
		tps[g_cnt].x = x;
		tps[g_cnt].y = y;
		tps[g_cnt].id1 = minid;
		tps[g_cnt].id2 = maxid;
		g_cnt++;
	    }
	}
	if (!changed)
	    break;
    }

    /* generate bds and xys tables */
    qsort (tps, g_cnt, sizeof (Thin_gate_t), Tps_cmp);
    bds = (Border_t *)tps;
    {
	unsigned short id1, id2;
	Border_t *bd;

	b_cnt = 0;
	id1 = id2 = 0;
	bd = bds;
	for (i = 0; i < g_cnt; i++) {
	    Thin_gate_t *tp = tps + i;
	    xys[i].x = tp->x;
	    xys[i].y = tp->y;
	    xys[i].next = i + 1;
	    if (tp->id1 != id1 || tp->id2 != id2) {
		bd = bds + b_cnt;
		b_cnt++;
		if (b_cnt * 2 >= BD_BUF_SZ)
		    break;	/* remianing break point left not processed */
		bd->id1 = tp->id1;
		bd->id2 = tp->id2;
		bd->off = i;
		bd->n = 1;
	    }
	    else
		bd->n = bd->n + 1;
	    id1 = tp->id1;
	    id2 = tp->id2;
	}
	b_cnt = Add_reflection (bds, b_cnt);
    }

    /* Join well-connected regions */
    nbds = (Border_t *)idmap;		/* reuse space */
    joined = 0;
    rm_cnt = 0;
    while (1) {
	int nb_cnt;

	qsort (bds, b_cnt, sizeof (Border_t), Bd_cmp);
	b_cnt -= rm_cnt;
	memset (flags, 0, b_cnt);	
	nb_cnt = 0;
	for (i = 0; i < b_cnt; i++) {	/* go though all regions */
	    int n, n2, st, st2;
	    Border_t *bd = bds + i;

	    n = Search_section (bds, b_cnt, bd->id1, i, &st);
	    if (n <= 0 || st != i)
		continue;
	    i += n - 1;

	    if (bd->n < Min_border_size (Rg_size[bd->id1]))
		continue;	/* border too small to join */

	    n2 = Search_section (bds, b_cnt, bd->id2, -1, &st2);
	    if (n2 <= 0)
		continue;
	    if ((flags[st] & RG_CHANGED) || (flags[st2] & RG_CHANGED))
		continue;	/* regions to join must not be changed */

	    nb_cnt += Join_two_regions (bds, b_cnt, nbds + nb_cnt,
					st, n, st2, n2, flags, id_for_rm);
	    Remove_break (bd, xys, mapbuf, xz, yz);
	    joined = 1;
	}
	if (nb_cnt == 0)
	    break;
	rm_cnt = Put_back_joined (bds, b_cnt, nbds, nb_cnt, xys,
							flags, id_for_rm);
    }
    return (joined);
}

/**************************************************************************

    Removes break gates on border "db".

***************************************************************************/

static void Remove_break (Border_t *bd, Coord_t *xys, 
				unsigned char *map, int xz, int yz) {
    int k;
    Coord_t *xy = xys + bd->off;
    for (k = 0; k < bd->n; k++) {
	int of = xy->y * xz + xy->x;
	map[of] = 2;
	xy = xys + xy->next;
    }
    return;
}

/**************************************************************************

    Merges borders of jointed regions and put back to bds.

**************************************************************************/

static int Put_back_joined (Border_t *bds, int b_cnt,
			Border_t*nbds, int nb_cnt, Coord_t *xys,
				unsigned char *flags, int id_for_rm) {
    int i, c, rm_cnt, scnt;

    qsort (nbds, nb_cnt, sizeof (Border_t), Merge_cmp);
    for (i = 1; i < nb_cnt; i++) {		/* merge border entries */
	Border_t *pbd, *bd = nbds + i;
	if (bd->id1 == id_for_rm)
	    break;
	pbd = bd - 1;
	if (bd->id1 == pbd->id1 && bd->id2 == pbd->id2) {	/* merge */
	    int of, k;
	    of = bd->off;
	    for (k = 1; k < bd->n; k++)
		of = xys[of].next;
	    xys[of].next = pbd->off;
	    bd->n += pbd->n;
	    pbd->id1 = id_for_rm;	/* discard this */
	}
    }

    c = 0;
    for (i = 0; i < nb_cnt; i++) {		/* compress nbds */
	if (nbds[i].id1 == id_for_rm)
	    continue;
	if (c != i)
	    nbds[c] = nbds[i];
	c++;
    }
    c = Add_reflection (nbds, c);

    rm_cnt = scnt = 0;
    for (i = 0; i < b_cnt; i++) {	/* copy processed borders back */
	if (flags[i] & BD_REMOVED) {
	    if (scnt < c) {
		bds[i] = nbds[scnt];
		scnt++;
	    }
	    else {
		bds[i].id1 = id_for_rm;	/* to remove */
		rm_cnt++;
	    }
	}
    }
    return (rm_cnt);
}

/**************************************************************************

    Joins a border of region bds[st].id1 to bds[st].id2. n is the region
    border section size and st is the first border of the region. The
    borders of the joined section is stored in nbds. The size of the 
    section is returned.

**************************************************************************/

static int Join_two_regions (Border_t *bds, int b_cnt, Border_t*nbds,
		int st, int n, int st2, int n2, 
				unsigned char *flags, int id_for_rm) {
    unsigned short ido, idn;
    int i, c2;

    /* copy the section of the region to remove to nbds and mark as changed;
       remove and mark borders of its neighbor regions */
    ido = bds[st].id1;		/* region id to remove */
    idn = bds[st].id2;		/* region id to join */
    for (i = 0; i < n; i++) {
	nbds[i] = bds[st + i];
	flags[st + i] |= (RG_CHANGED | BD_REMOVED);
    }
    for (i = 1; i < n; i++) {
	int nn, sti, k;
	nn = Search_section (bds, b_cnt, bds[i + st].id2, -1, &sti);
	if (nn < 0)
	    continue;
	for (k = sti; k < sti + nn; k++) {
	    if (bds[k].id2 == ido || bds[k].id2 == idn)
		flags[k] |= BD_REMOVED;
	    flags[k] |= RG_CHANGED;
	}
    }

    /* copy the section of the region to join to nbds and mark as joined;
       remove and mark borders of its neighbor regions */
    for (i = 0; i < n2; i++) {
	nbds[n + i] = bds[st2 + i];
	flags[st2 + i] |= (RG_CHANGED | BD_REMOVED);
    }
    c2 = n2;
    for (i = 0; i < n2; i++) {
	int nn, sti, k;
	if (bds[i + st2].id2 == ido)
	    continue;
	nn = Search_section (bds, b_cnt, bds[i + st2].id2, -1, &sti);
	if (nn < 0)
	    continue;
	for (k = sti; k < sti + nn; k++) {
	    if (bds[k].id2 == idn || bds[k].id2 == ido)
		flags[k] |= BD_REMOVED;
	    flags[k] |= RG_CHANGED;
	}
    }

    /* perform the join in nbds */
    n += c2;
    for (i = 0; i < n; i++) {
	Border_t *bd = nbds + i;
	if (bd->id1 == ido)
	    bd->id1 = idn;
	if (bd->id2 == ido)
	    bd->id2 = idn;
	if (bd->id1 == bd->id2)
	    bd->id1 = id_for_rm;	/* discard this */
    }
    Rg_size[idn] += Rg_size[ido];		/* region size after join */
    return (n);
}

/**************************************************************************

    Adds reflection border entries to border table "bds" so binary search
    can be applied.

**************************************************************************/

static int Add_reflection (Border_t *bds, int b_cnt) {
    int c, i;

    c = 0;
    for (i = 0; i < b_cnt; i++) {
	Border_t *d, *s;
	s = bds + i;
	if (s->id1 == s->id2)
	    continue;
	d = bds + b_cnt + c;
	d->id1 = s->id2;
	d->id2 = s->id1;
	d->n = s->n;
	d->off = s->off;
	c++;
    }
    return (b_cnt + c);
}

/**************************************************************************

    Returns the min border size an region of size "a_size" can be joined to
    a neighbor.

**************************************************************************/

static int Min_border_size (int a_size) {
    #define MBDS_SIZE 40
    static unsigned char mbds[MBDS_SIZE] = {1, 1, 1, 2, 2, 2, 2, 
	3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 
	5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 8};

    if (a_size >= MBDS_SIZE)
	a_size = MBDS_SIZE - 1;
    return (mbds[a_size]);
}

/**************************************************************************

    Searches for the section of "bds" that has id1 = id. gi is the guess
    index and "stp" returns the starting index. It returns the section size
    or -1 if not found.

**************************************************************************/

static int Search_section (Border_t *bds, int b_cnt, 
				unsigned short id, int gi, int *stp) {
    int g;

    if (gi < 0 || bds[gi].id1 != id) {	/* binary search */
	int st, end;
	st = 0;
	end = b_cnt - 1;
	while (1) {
	    int i = (st + end) / 2;
	    if (i == st) {
		if (bds[st].id1 != id) {
		    if (bds[end].id1 != id) {
			VDD_log ("Search_section id %d not found\n", id);
			if (Test_mode)
			    exit (1);
			return (-1);
		    }
		    st = end;
		}
		break;
	    }
	    if (bds[i].id1 < id)
		st = i;
	    else if (bds[i].id1 > id)
		end = i;
	    else {
		st = i;
		break;
	    }
	}
	gi = st;
    }
    g = gi;
    while (gi >= 0 && bds[gi].id1 == id)
	gi--;
    while (g < b_cnt && bds[g].id1 == id)
	g++;
    if (stp != NULL)
	*stp = gi + 1;

    return (g - gi - 1);
}

/**************************************************************************

    Compare function for sorting breaking gate list in decreasing id. This
    is for generating border table.

**************************************************************************/

static int Tps_cmp (const void *tp1p, const void *tp2p) {
    Thin_gate_t *tp1, *tp2;
    tp1 = (Thin_gate_t *)tp1p;
    tp2 = (Thin_gate_t *)tp2p;
    if (tp1->id1 == tp2->id1)
	return (tp2->id2 - tp2->id2);
    return (tp2->id1 - tp1->id1);
}

/**************************************************************************

    Compare function for sorting border list in increasing id. This
    is for merging records.

**************************************************************************/

static int Merge_cmp (const void *tp1p, const void *tp2p) {
    Border_t *tp1, *tp2;
    tp1 = (Border_t *)tp1p;
    tp2 = (Border_t *)tp2p;
    if (tp1->id1 == tp2->id1)
	return (tp1->id2 - tp2->id2);
    return (tp1->id1 - tp2->id1);
}

/**************************************************************************

    Compare function for sorting border list in increasing id. This is for
    joining regions.

**************************************************************************/

static int Bd_cmp (const void *bd1p, const void *bd2p) {
    Border_t *b1, *b2;
    b1 = (Border_t *)bd1p;
    b2 = (Border_t *)bd2p;
    if (b1->id1 == b2->id1) {
	if (b1->n == b2->n)
	    return (Rg_size[b1->id2] - Rg_size[b2->id2]);
	return (b2->n - b1->n);
    }
    return (b1->id1 - b2->id1);
}

/**************************************************************************

    Thining and conditionally dialating mapbuf to identify all thin 
    connections.

**************************************************************************/

void VDA_thin_and_dialate (unsigned char *mapbuf, int xz, int yz,
					int fp, int v, int level) {
    int xz1, i, x, y, k, off[32], *ofs;
    unsigned char *map;

    xz1 = xz - 1;
    for (i = 0; i < level; i++) {	/* thin the regions */
	for (y = 0; y < yz; y++) {

	    map = mapbuf + y * xz;
	    VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	    for (x = 0; x < xz; x++) {
		if (map[x] <= i)
		    continue;
		if (x == 0)
		    ofs = off + 4;
		else if (x == xz1)
		    ofs = off + 8;
		else
		    ofs = off;
		for (k = 0; k < 4; k++) {
		    if (ofs[k] != 0 && map[x + ofs[k]] <= i) {
			map[x] = i + 1;		/* i-th border gates */
			break;
		    }
		}
	    }
	}
    }

    /* restore thined gates without causing connection */
    for (i = 0; i < level * 2; i++) {
	int v_i, v_i_1;

	v_i = v - i;		/* >= v_i - current data gate */
	v_i_1 = v - i - 1;	/* to be data gate in next step */
	for (y = 0; y < yz; y++) {

	    VDA_get_neighbor_offset (8, y, xz, yz, fp, off);
	    map = mapbuf + y * xz;
	    for (x = 0; x < xz; x++) {
		int gg, dn, cnt;

		if (map[x] >= v_i || map[x] == 0)
		    continue;
		if (x == 0)
		    ofs = off + 8;
		else if (x == xz1)
		    ofs = off + 16;
		else
		    ofs = off;
		gg = 0;	/* is current gate good - init for cycling neighbors */
		if (map[x + ofs[7]] >= v_i_1)
		    gg = 1;
		cnt = 0;		/* bad to good gate count */
		dn = 0;			/* a data neighbor found */
		for (k = 0; k < 8; k++) {
		    int m = map[x + ofs[k]];
		    if (m < v_i_1)
			gg = 0;
		    else {
			if (!gg)
			    cnt++;
			gg = 1;
			if (!dn && (k % 2) == 0 && m >= v_i)
			    dn = 1;
		    }
		}
		if (cnt <= 1 && dn)
		    map[x] = v_i_1;
	    }
	}
    }
}

/**************************************************************************

    Generates the breaking map: 0 for no data; bit 128 for breaking data;
    bit 32 for thined data; 64 is for other data.

**************************************************************************/

static void Generate_break_map (unsigned char *mapbuf, int xz, int yz,
				int level, int fp, unsigned char *rd_buf) {
    void *rgs;
    int xz1, x, y, nregions, i;
    int clump_flag;
    unsigned char *map;
    Data_filter_t cdf;
    Region_t region;

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

    /* break large regions formed by "thined gates" */
    cdf.map = mapbuf;
    cdf.yes_bits = 64;
    cdf.exc_bits = 0;
    clump_flag = VDC_IDR_SIZE;
    if (fp)
	clump_flag |= VDC_IDR_WRAP;
    rgs = NULL;
    nregions = VDC_identify_regions (mapbuf, &cdf, xz,
				0, 0, xz, yz, NULL, clump_flag, &rgs);
    region.data = (short *)rd_buf;
    for (i = 0; i < nregions; i++) {
	int n_gs, maxs;
	maxs = 6;
	n_gs = VDC_get_next_region (rgs, VDC_BIN | VDC_NEXT, &region);	    
	if (region.xz <= maxs && region.yz <= maxs)
	    break;
	Break_thined_region (mapbuf, xz, yz, &region, maxs);
    }
    VDC_free (rgs);

    for (y = 0; y < yz; y++) {	/* set map value */
	map = mapbuf + y * xz;
	for (x = 0; x < xz; x++) {
	    if (map[x] == 0)	/* no data */
		continue;
	    if (map[x] & 128)	/* breaking data gates */
		map[x] = 1;
	    else
		map[x] = 2;	/* other data gates */
	}
    }
}

/**************************************************************************

    Generates the region id map "idmap" from breaking map "mapbuf". Returns
    the next unused id.

**************************************************************************/

static int Generate_idmap (unsigned char *mapbuf, int xz, int yz,
		unsigned short *idmap, int fp, unsigned char *rd_buf) {
    int v, x, y, nregions, i;
    int clump_flag;
    unsigned short *smap;
    Data_filter_t cdf;
    Region_t region;
    void *rgs;

    memset (idmap, 0, xz * yz * sizeof (unsigned short));
    /* assign region id */
    cdf.map = mapbuf;
    cdf.yes_bits = 2;
    cdf.exc_bits = 0;
    clump_flag = VDC_IDR_SIZE;
    if (fp)
	clump_flag |= VDC_IDR_WRAP;
    rgs = NULL;
    nregions = VDC_identify_regions (mapbuf, &cdf, xz,
				0, 0, xz, yz, NULL, clump_flag, &rgs);
    region.data = (short *)rd_buf;
    v = MAX_ID;
    for (i = 0; i < nregions; i++) {	/* generate the region id map */
	int n_gs, id;

	n_gs = VDC_get_next_region (rgs, VDC_BIN | VDC_NEXT, &region);
	if (n_gs < 0)
	    break;

	if (region.xz <= 2 && region.yz <= 2) {	/* eliminate small regions */
	    for (y = 0; y < region.yz; y++) {
		int yy = (y + region.ys) % yz;
		for (x = 0; x < region.xz; x++) {
		    int of = yy * xz + x + region.xs;
		    if (mapbuf[of] != 0)
			mapbuf[of] = 1;
		}
	    }
	    continue;
	}

	id = 1;
	if (v > 1) {	/* 1 used for remaining small regs no id available */
	    id = v;
	    Rg_size[id] = region.xz >= region.yz? region.xz : region.yz;
	    v--;
	}
	for (y = 0; y < region.yz; y++) {
	    unsigned char *cp;
	    int yy = (region.ys + y) % yz;
	    smap = idmap + yy * xz + region.xs;
	    cp = rd_buf + y * region.xz;
	    for (x = 0; x < region.xz; x++) {
		if (cp[x] != BNO_DATA)
		    smap[x] = id;
	    }
	}
    }
    VDC_free (rgs);
    return (v);
}

/**************************************************************************

    Breaks large region by turning some 64 (region formed by thined gate)
    to 128 (breaking gate).

**************************************************************************/

static void Break_thined_region (unsigned char *mapbuf, int xz, int yz,
					Region_t *region, int maxs) {
    int rxz, ryz, step, x, y, of;
    unsigned char *data;

    rxz = region->xz;
    ryz = region->yz;
    data = (unsigned char *)region->data;
    if (rxz > maxs) {
	step = rxz / (rxz / maxs + 1);
	for (x = step; x < rxz; x += step) {
	    int xo = x + region->xs;
	    for (y = 0; y < ryz; y++) {
		if (data[y * rxz + x] == BNO_DATA)
		    continue;
		of = ((y + region->ys) % yz) * xz + xo;
		mapbuf[of] = (mapbuf[of] & (~64)) | 128;
	    }
	}
    }
    if (ryz > maxs) {
	step = ryz / (ryz / maxs + 1);
	for (y = step; y < ryz; y += step) {
	    int yo = (y + region->ys) % yz;
	    for (x = 0; x < rxz; x++) {
		if (data[y * rxz + x] == BNO_DATA)
		    continue;
		of = yo * xz + x + region->xs;
		mapbuf[of] = (mapbuf[of] & (~64)) | 128;
	    }
	}
    }
}

/*************************************************************************

    Fills in data holes if their neighboring data is smooth.

*************************************************************************/

typedef struct {
    unsigned char *map;
    int xz, yz;
    Point_t *tcps;
    int n_tcps;
    int depth;
} Rm_dialated_area_t;

#define CHECK_FA_SIZE 10	/* the max distance for checking neighborhood 
				   data of a filled hole */
#define CHECK_FA_NBS (((CHECK_FA_SIZE + 1) * 2 + 1) * ((CHECK_FA_SIZE + 1) * 2 + 1))
#define CHECK_FA_HGS ((CHECK_FA_SIZE * 2 + 1) * (CHECK_FA_SIZE * 2 + 1))

typedef struct {
    unsigned char *map;
    int xz, yz;
    int x0, y0;
    int n_nbs;			/* number of neighbor gates found */
    int nbs_off[CHECK_FA_NBS];	/* offsets of neighbor gates */
    int n_hgs;			/* number of hole gates checked */
    int hgs_off[CHECK_FA_HGS];	/* offsets of hole gates */
} Get_fa_neighbors_t;

static void Rm_dilated_area (int x, int y, int fp, Rm_dialated_area_t *rda);
static void Get_fa_neighbors (int x, int y, int fp, Get_fa_neighbors_t *gfan);

static int Fill_in_holes (unsigned char *inp, unsigned char *wbuf, 
			int xz, int yz, int fp, int v, int nyq, int d_off) {
    int x, y, xz1, off[32], *ofs, i, k;
    unsigned char *map;
    Rm_dialated_area_t rda;
    Get_fa_neighbors_t gfan;

    xz1 = xz - 1;
    /* dilate the image to fill in the holes */
    for (y = 0; y < yz; y++) {

	map = wbuf + y * xz;
	VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	for (x = 0; x < xz; x++) {
	    if (map[x] > 0)
		continue;
	    if (x == 0)
		ofs = off + 4;
	    else if (x == xz1)
		ofs = off + 8;
	    else
		ofs = off;
	    for (k = 0; k < 4; k++) {
		if (ofs[k] != 0 && map[x + ofs[k]] > 1) {
		    map[x] = 1;		/* marked as dilated gate */
		    break;
		}
	    }
	}
    }

    /* remove open dilated areas */
    rda.n_tcps = 0;
    rda.tcps = NULL;
    rda.xz = xz;
    rda.yz = yz;
    rda.map = wbuf;
    rda.depth = 0;
    for (y = 0; y < yz; y++) {

	map = wbuf + y * xz;
	VDA_get_neighbor_offset (4, y, xz, yz, fp, off);
	for (x = 0; x < xz; x++) {
	    if (map[x] != 1)
		continue;
	    if (x == 0)
		ofs = off + 4;
	    else if (x == xz1)
		ofs = off + 8;
	    else
		ofs = off;
	    for (k = 0; k < 4; k++) {
		if (ofs[k] != 0 && map[x + ofs[k]] == 0) {
		    Rm_dilated_area (x, y, fp, &rda);
		}
	    }
	}
    }
    i = 0;
    while (i < rda.n_tcps) {  /* finish left-overs due to too deep recursion */
	Rm_dilated_area (rda.tcps[i].x, rda.tcps[i].y, fp, &rda);
	i++;
    }
    STR_free (rda.tcps);

    /* Check neighbors of filled-in areas  */
    gfan.map = wbuf;
    gfan.xz = xz;
    gfan.yz = yz;
    for (y = 0; y < yz; y++) {
	map = wbuf + y * xz;
	for (x = 0; x < xz; x++) {
	    if (map[x] == 1) {
		int *d, maxd[3], dbuf[256], med, c;

		gfan.x0 = x;
		gfan.y0 = y;
		gfan.n_nbs = 0;
		gfan.n_hgs = 0;
		Get_fa_neighbors (x, y, fp, &gfan);
		if (gfan.n_nbs == 0) {
		    for (i = 0; i < gfan.n_hgs; i++)
			wbuf[gfan.hgs_off[i]] = 0;	/* do not fill */
		    continue;
		}
		if (gfan.n_nbs < 256)
		    d = dbuf;
		else
		    d = MISC_malloc (gfan.n_nbs * sizeof (int));
		c = 0;
		for (i = 0; i < gfan.n_nbs; i++) {
		    d[i] = v = inp[gfan.nbs_off[i]];
		    if (v != BNO_DATA) {
			d[c] = v;
			c++;
		    }
		}
		if (c <= 1)
		    continue;
		med = VDA_search_median_value (d, c, nyq, d_off, maxd);
		if (d != dbuf)
		    free (d);

		if (maxd[0] * 4  > nyq) {		/* bad neighbor data */
		    for (i = 0; i < gfan.n_hgs; i++)
			wbuf[gfan.hgs_off[i]] = 0;	/* do not fill */
		}
		else {
		    for (i = 0; i < gfan.n_hgs; i++)
			wbuf[gfan.hgs_off[i]] = v;	/* fill */
		}

		for (i = 0; i < gfan.n_nbs; i++)
		    wbuf[gfan.nbs_off[i]] = v;	/* remove data taken flag */
	    }
	}
    }

    return (0);
}

/***************************************************************************

    Remove the dilated gate (map = 1) at (x, y) (set map to 0) and expands
    this action to the neighborhood of this location. To prevent from stack
    overflow, the recursive call stops at certain depth. The stopping 
    location is saved for later processing.

***************************************************************************/

static void Rm_dilated_area (int x, int y, int fp, Rm_dialated_area_t *rda) {
    int off[16], *ofs, k, cr, xz;
    unsigned char *map;

    xz = rda->xz;
    map = rda->map;
    cr = y * xz + x;
    if (map[cr] != 1)
	return;
    if (rda->depth > 500) {	/* to avoid stack overflow, process later */
	Point_t tcp;
	tcp.x = x;
	tcp.y = y;
	rda->tcps= (Point_t *)STR_append (rda->tcps, &tcp, sizeof (tcp));
	rda->n_tcps++;
	return;
    }
    rda->depth++;
    map[cr] = 0;
    VDA_get_neighbor_offset (4, y, xz, rda->yz, fp, off);
    if (x == 0)
	ofs = off + 4;
    else if (x == xz - 1)
	ofs = off + 8;
    else
	ofs = off;
    for (k = 0; k < 4; k++) {
	if (ofs[k] != 0) {
	    int of = cr + ofs[k];
	    if (map[of] == 1)
		Rm_dilated_area (of % xz, of / xz, fp, rda);
	}
    }
    rda->depth--;
}

/***************************************************************************

    Gets the offsest of the data neighbors of the filled area containing
    (x, y) and stores them in nbs_off. The data neighbor taken is marked
    by map = 3 so we do not re-take the data gate. This point is marked
    by map = 2 so we can recursively go through all gates in the filled
    area. The offset of this point is stored in hgs_off so we can
    set/reset it later. To avoid stack overflow, the search is limited
    to a max distance.

***************************************************************************/

static void Get_fa_neighbors (int x, int y, int fp, Get_fa_neighbors_t *gfan) {
    int off[32], *ofs, k, cr, xz, yz, dx, dy;
    unsigned char *map;

    xz = gfan->xz;
    map = gfan->map;
    cr = y * xz + x;
    if (map[cr] != 1)
	return;
    dx = x - gfan->x0;
    if (dx < 0)
	dx = -dx;
    dy = y - gfan->y0;
    if (dy < 0)
	dy = -dy;
    yz = gfan->yz;
    if (fp && yz > 4 * CHECK_FA_SIZE) {
	if (dy * 2 >= yz)
	    dy = yz - dy;
    }
    if (dx > CHECK_FA_SIZE || dy > CHECK_FA_SIZE) {
			/* reaching the max distance from the start */
	return;
    }
    if (gfan->n_hgs >= CHECK_FA_HGS)	/* should never happen */
	return;
    map[cr] = 2;
    gfan->hgs_off[gfan->n_hgs] = cr;
    gfan->n_hgs++;
    VDA_get_neighbor_offset (8, y, xz, yz, fp, off);
    if (x == 0)
	ofs = off + 8;
    else if (x == xz - 1)
	ofs = off + 16;
    else
	ofs = off;
    for (k = 0; k < 8; k++) {
	if (ofs[k] != 0) {
	    int of = cr + ofs[k];
	    if (map[of] > 3 &&			/* a untaken data neighbor */
		gfan->n_nbs < CHECK_FA_NBS) {	/* should never happen */
		map[of] = 3;
		gfan->nbs_off[gfan->n_nbs] = of;
		gfan->n_nbs++;
	    }
	}
    }
    for (k = 0; k < 8; k += 2) {
	if (ofs[k] != 0) {
	    int of = cr + ofs[k];
	    if (map[of] == 1)
		Get_fa_neighbors (of % xz, of / xz, fp, gfan);
	}
    }
}

/********************************************************************

    Finds gates that cause single gate connection in "inp" of sizes
    "xz" and "yz". dft is the data filter. Returns the number of found
    gates. If outmap = NULL, the found gates in inp is set to no-data.

*********************************************************************/

#define BAD_IND 0x7fffffff

int VDA_remove_single_gate_conn (unsigned char *inp, int xz, int yz, 
	Data_filter_t *dft, unsigned char *outmap, unsigned char mapv) {
    unsigned char *wb, ybits, ebits, *dmap;
    int x, y, i, changed, max_exp, g_cnt;

    Wbuf = (unsigned char *)STR_reset ((char *)Wbuf, 
				xz * yz * sizeof (unsigned char));
    wb = Wbuf;				/* work space */
    if (dft != NULL) {
	dmap = dft->map;
	ybits = dft->yes_bits;
	ebits = dft->exc_bits;
    }
    else {
	dmap = NULL;
	ybits = ebits = 0;
    }
    g_cnt = 0;
    for (y = 0; y < yz; y++) {
	int off[4], cind;

	off[2] = -xz;
	off[3] = xz;
	if (y == 0)
	    off[2] = BAD_IND;
	if (y == yz - 1)
	    off[3] = BAD_IND;
	cind = y * xz - 1;
	for (x = 0; x < xz; x++) {
	    int cnt, k, v[4];
	    unsigned char d;

	    cind++;
	    d = inp[cind];
	    if (dmap != NULL) {
		unsigned char m = dmap[cind];
		if (ybits) {
		    if (!(m & ybits))
			d = BNO_DATA;
		}
		if (ebits) {
		    if (m & ebits)
			d = BNO_DATA;
		}
	    }
	    if (d == BNO_DATA) {
		wb[cind] = 0;
		continue;
	    }

	    if (x == 0)
		off[0] = BAD_IND;
	    else
		off[0] = -1;
	    if (x == xz - 1)
		off[1] = BAD_IND;
	    else
		off[1] = 1;
	    
	    cnt = 0;
	    for (k = 0; k < 4; k++) {
		if (off[k] != BAD_IND) {
		    unsigned char d;
		    int ind = cind + off[k];
		    d = inp[ind];
		    if (dmap != NULL) {
			unsigned char m = dmap[ind];
			if (ybits) {
			    if (!(m & ybits))
				d = BNO_DATA;
			}
			if (ebits) {
			    if (m & ebits)
				d = BNO_DATA;
			}
		    }
		    if (d != BNO_DATA)
			cnt++;
		    else
			d = 0;
		    v[k] = d;
		}
		else
		    v[k] = 0;
	    }
	    if (cnt == 2 && !((v[0] == 0 && v[1] == 0) || 
				(v[2] == 0 && v[3] == 0)))
		cnt = 3;
	    wb[cind] = cnt;
	}
    }

    max_exp = 3;
    for (i = 0; i <= max_exp; i++) {
	changed = 0;
	for (y = 0; y < yz; y++) {
	    int off[4], cind;

	    off[2] = -xz;
	    off[3] = xz;
	    if (y == 0)
		off[2] = BAD_IND;
	    if (y == yz - 1)
		off[3] = BAD_IND;
	    cind = y * xz - 1;
	    for (x = 0; x < xz; x++) {
		int cnt, k, v[4], found5;
    
		cind++;
		if (wb[cind] == 0 || wb[cind] >= 3)
		    continue;
    
		if (x == 0)
		    off[0] = BAD_IND;
		else
		    off[0] = -1;
		if (x == xz - 1)
		    off[1] = BAD_IND;
		else
		    off[1] = 1;
		
		cnt = found5 = 0;
		for (k = 0; k < 4; k++) {
		    if (off[k] != BAD_IND) {
			int vv = wb[cind + off[k]];
			v[k] = vv;
			if (vv >= 3)
			    cnt++;
			if (vv == 5)
			    found5 = 1;
		    }
		    else
			v[k] = 0;
		}
		if (i == max_exp && cnt > 1) {
		    if (outmap != NULL)
			outmap[cind] = outmap[cind] | mapv;
		    else
			inp[cind] = BNO_DATA;
		    g_cnt++;
		    wb[cind] = 0;
		    changed = 1;
		    continue;
		}
		if (cnt == 2) {
		    if (found5 || (v[0] == 0 && v[1] == 0) || 
				(v[2] == 0 && v[3] == 0)) {
			if (outmap != NULL)
			    outmap[cind] = outmap[cind] | mapv;
			else
			    inp[cind] = BNO_DATA;
			g_cnt++;
			wb[cind] = 0;
			changed = 1;
			continue;
		    }
		}
		if (cnt >= 1) {
		    wb[cind] = 5;
		    changed = 1;
		}
	    }
	}
	if (!changed)
	    break;
    }
    return (g_cnt);
}


/* variable for VDA_detect_false_hs */
#define DHS_MAX_DEPTH 2048
#define DHS_REACHED 0x1
static __thread int Dhs_depth, Dhs_cnt, Dhs_xmin, Dhs_xmax, Dhs_ymin, \
			Dhs_ymax, Dhs_fp, Dhs_xz, Dhs_yz, Dhs_thr0, Dhs_thr;
static __thread short *Dhs_data, *Dhs_refd;
static __thread unsigned char *Dhs_map;
static int Dhs_xof[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static int Dhs_yof[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

/*************************************************************************

    Traveling function for VDA_detect_false_hs.

*************************************************************************/

static void Dhs_travel (int x, int y) {
    int off, i;

    if (Dhs_depth > DHS_MAX_DEPTH)
	return;

    off = y * Dhs_xz + x;
    if (Dhs_data[off] == SNO_DATA || (Dhs_map[off] & DHS_REACHED))
	return;
    if (Dhs_cnt < 0) {
	Dhs_xmin = Dhs_xmax = x;
	Dhs_ymin = Dhs_ymax = y;
	Dhs_cnt = 0;
    }
    Dhs_map[off] |= DHS_REACHED;
    for (i = 0; i < 8; i++) {	/* four neighbors */
	int nx, ny, nof, df;
	nx = x + Dhs_xof[i];
	if (nx < 0 || nx >= Dhs_xz)
	    continue;
	ny = y + Dhs_yof[i];
	if (Dhs_fp)
	    ny = (ny + Dhs_yz) % Dhs_yz;
	else if (ny < 0 || ny >= Dhs_yz)
	    continue;

	nof = ny * Dhs_xz + nx;
	if (Dhs_data[nof] == SNO_DATA || (Dhs_map[nof] & DHS_REACHED))
	    continue;
	df = Dhs_data[nof] - Dhs_data[off];
	if (df < 0) df = -df;
	if (Dhs_depth == 0) {
	    if (df < Dhs_thr0)
		continue;
	}
	else {
	    if (df < Dhs_thr)
		continue;
	}
	df = Dhs_refd[nof] - Dhs_refd[off];
	if (df < 0) df = -df;
	if (df * 3 > Dhs_thr)
	    continue;
	if (nx < Dhs_xmin)
	    Dhs_xmin = nx;
	if (nx > Dhs_xmax)
	    Dhs_xmax = nx;
	if (ny < Dhs_ymin)
	    Dhs_ymin = ny;
	if (ny > Dhs_ymax)
	    Dhs_ymax = ny;
	Dhs_cnt++;
	Dhs_depth++;
	Dhs_travel (nx, ny);
	Dhs_depth--;
    }
}

/**************************************************************************

    Detects the maximum section of high shear in "reg". Returns the number 
    of gates in the section. maxxy, if not NULL, returns the width and height
    of the section.

**************************************************************************/

int VDA_detect_false_hs (short *data, short *refd, int xz, int yz, int fp, 
						int thr, int *maxxy) {
    static __thread unsigned char *map = NULL;
    int max, y;

    map = (unsigned char *)STR_reset ((char *)map, 
					xz * yz * sizeof (unsigned char));
    memset (map, 0, xz * yz * sizeof (unsigned char));

    Dhs_map = map;
    Dhs_data = data;
    Dhs_refd = refd;
    Dhs_fp = fp;
    Dhs_xz = xz;
    Dhs_yz = yz;
    Dhs_thr0 = thr;
    Dhs_thr = thr * 9 / 10;	/* use smaller threshold except the first */
    max = -1;
    for (y = 0; y < yz; y++) {
	int x;
	short *d = data + y * xz;
	unsigned char *m = map + y * xz;
	for (x = 0; x < xz; x++) {
	    if (d[x] == SNO_DATA || (m[x] & DHS_REACHED))
		continue;
	    Dhs_depth = 0;
	    Dhs_cnt = -1;
	    Dhs_travel (x, y);
	    if (Dhs_cnt > max) {
		max = Dhs_cnt;
		if (maxxy != NULL) {
		    maxxy[0] = Dhs_xmax - Dhs_xmin;
		    maxxy[1] = Dhs_ymax - Dhs_ymin;
		}
	    }
	}
    }

    if (xz * yz > 20000) {
	STR_free (map);
	map = NULL;
    }
    return (max / 2);
}
