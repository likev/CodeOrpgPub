
/******************************************************************

    vdeal's module containing functions for EW processing.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/16 14:43:58 $
 * $Id: vdeal_ew.c,v 1.5 2014/07/16 14:43:58 steves Exp $
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

typedef struct ew_update_t {	/* struct for updating EW */
    short n_gg;		/* number of dealiased good gates */
    int tgv;		/* total gate value of dealiased good gates */
} Ew_update_t;

typedef struct {	/* for storing a neighboring grid point */
    float v;
    float x;
    float y;
} grid_point_t;

extern int Test_mode;

static int Count_processed_gates (short *reg, int rstd, int xz, int yz,
	unsigned char *dmap, int dmstd, int dys, int dyz, int *nb, int *tvp);
static void Compute_average (short *region, int rstrd, int xn, int yn, 
			unsigned char *dmap, int dstrd, int dys, int dyz,
			int *np, float *ap);
static short Get_ew_value (Vdeal_t *vdv, int xi, int y);
static int Get_ew_azi_size (Vdeal_t *vdv);
static void Print_spec_flag (unsigned short *flags, int i, int n_rgs, int rz,
				unsigned short sflag, char *b);
static int Get_interp_value (Vdeal_t *vdv, int x, int y, 
			int rsteps, int asteps, short absmax, short *vadm);
static void Set_range_limit (Vdeal_t *vdv, int cx, int steps);
static void Smooth_data (float *data, int n, int max_ext_steps);
static short Lms_interpolation (int cnt, double *dxs, double *dys,
						double *dvs, double max_d);
static void Prepare_vad_for_ew_map (Vdeal_t *vdv, short *vadm);
static void Extend_second_trip_area (Ew_struct_t *ew, int rsteps);
static void Add_near_elevation_ew (Vdeal_t *vdv);
static float Get_cell_azi_range_ratio (int n_azs, int n_rgs, int rind, 
						unsigned char **st2ai);
static int Near_range_dealiase (Vdeal_t *vdv, Region_t *region, int nyq);
static int Check_hs_grid (int x, int y, int sof, Vdeal_t *vdv, short absmax);
static void Extend_bad_vad (Vdeal_t *vdv, unsigned char *bvad, int n_azs);
static void Fill_in_vadm_gaps (Vdeal_t *vdv, short *vadm);

/**********************************************************************

    Initializes vdv->ew.

***********************************************************************/

int VDE_initialize (Vdeal_t *vdv) {
    static int bsize = 0, bs = 0;
    double rs, maxr;
    int s, size, ew_azi_z;
    Ew_struct_t *ew;

    ew = &(vdv->ew);
    rs = 6000.;		/* max range step size is 6000 meters */
    maxr = vdv->xz * vdv->g_size;
    if (vdv->elev > 0.1f) {
	double sinrs = sin ((double)vdv->elev * deg2rad);
	if (rs * sinrs > 500.)	/* max altitude step size is 500 meters */
	    rs = 500. / sinrs;
	if (MAX_ALTITUDE / sinrs < maxr)
	    maxr = MAX_ALTITUDE / sinrs;
    }
    ew->rz = rs / vdv->g_size + .5;
    ew->n_rgs = maxr / vdv->g_size / ew->rz + .5;
    ew_azi_z = Get_ew_azi_size (vdv);
    ew->az = 360.f / ew_azi_z;
    ew->n_azs = ew_azi_z;
    if (ew->n_rgs > ew->n_azs)	/* reduce ew->n_rgs to <= ew->n_azs */
	ew->n_rgs = ew->n_azs;
    if (ew->n_rgs > MAX_EW_NRS)
	ew->n_rgs = MAX_EW_NRS;
    while (ew->rz * ew->n_rgs < vdv->xz)
	ew->rz++;

    s = ew->n_azs * ew->n_rgs;
    size = (s * 2 + ew->n_rgs) * sizeof (short) + 2 * s * sizeof (char);
    if (size > bsize) {
	if (ew->ews != NULL)
	    free (ew->ews);
	ew->ews = (short *)MISC_malloc (size);
	bsize = size;
    }
    if (s > bs) {
	if (ew->ups != NULL)
	    free (ew->ups);
	ew->ups = (Ew_update_t *)MISC_malloc (s * sizeof (Ew_update_t));
	bs = s;
    }
    ew->ewm = ew->ews + s;
    ew->rfs = (unsigned short *)(ew->ewm + s);
    ew->eww = (unsigned char *)(ew->rfs + ew->n_rgs);
    ew->efs = (unsigned char *)(ew->eww + s);

    VDE_reset (vdv, 1);
    return (0);
}

/**********************************************************************

    Initializes vdv->ew. Resets ews, ewm, eww and ups. If "all" is false,
    only ups (the source data for EW) is reset.

***********************************************************************/

int VDE_reset (Vdeal_t *vdv, int all) {
    int size, i;
    Ew_struct_t *ew;

    ew = &(vdv->ew);
    size = ew->n_azs * ew->n_rgs;
    memset (ew->ups, 0, size * sizeof (Ew_update_t));
    if (all) {
	short *p1, *p2;
	unsigned char *p3;
	p1 = ew->ews;
	p2 = ew->ewm;
	p3 = ew->eww;
	for (i = 0; i < size; i++) {	/* init ews, ewm and eww */
	    *p1++ = SNO_DATA;
	    *p2++ = vdv->data_off;
	    *p3++ = 1;
	}
    }

    return (0);
}

/**********************************************************************

    Returns the azimuthal index in EW for "azimuth" in degrees. "azimuth"
    cannot be negative.

***********************************************************************/

int VDE_get_azi_ind (Vdeal_t *vdv, double azimuth) {
    int ind, ew_azi_sz;

    ew_azi_sz = Get_ew_azi_size (vdv);
    ind = (int)(azimuth * (double)ew_azi_sz / 360.);
    return (ind % ew_azi_sz);
}

/**********************************************************************

    Returns the EW azimuthal grid size.

***********************************************************************/

static int Get_ew_azi_size (Vdeal_t *vdv) {
    int s = 60;
    if (vdv->gate_width >= .6f)
	s = 45;
    if (vdv->low_prf)
	s = s * 2 / 3;
    return (s);
}

/************************************************************************

    Sets ew->flags based on input data and current EW.

*************************************************************************/

void VDE_set_ew_flags (Vdeal_t *vdv) {
    Ew_struct_t *ew;
    unsigned char *efs;
    unsigned short *rfs;

    /* compute input max wind for each EW range */
    ew = &(vdv->ew);
    rfs = ew->rfs;
    efs = ew->efs;
    memset (efs, 0, ew->n_rgs * ew->n_azs);		/* reset flags */
    memset (rfs, 0, ew->n_rgs * sizeof (short));	/* reset flags */

    PP_analyze_z (vdv);
    VDV_save_storm_distance (vdv);
}

/************************************************************************

    Logs ew->flags.

*************************************************************************/

void VDE_print_ew_flags (Vdeal_t *vdv) {

    Ew_struct_t *ew;
    char buf[512], b1[128], b2[128], b3[128];
    int rz, i;
    unsigned short *rfs;

    ew = &(vdv->ew);
    rfs = ew->rfs;
    buf[0] = '\0';
    rz = ew->rz;

    if (vdv->data_type & DT_VH_VS)
	sprintf (buf + strlen (buf), "VH_VS ");
    b1[0] = b2[0] = b3[0] = '\0';
    for (i = 0; i < ew->n_rgs; i++) {
	if (i > 0 && !(rfs[i] & RF_CLEAR_AIR) && (rfs[i - 1] & RF_CLEAR_AIR))
	    sprintf (buf + strlen (buf), "CLEAR_AIR -%d ", i * rz);
	if ((rfs[i] & RF_NONUNIFORM) &&
				(i == 0 || !(rfs[i - 1] & RF_NONUNIFORM)))
	    sprintf (buf + strlen (buf), "NONUNIFORM %d- ", i * rz);
	if (strlen (b1) < 110)
	    Print_spec_flag (rfs, i, ew->n_rgs, rz, RF_HIGH_VS, b1);
	if (strlen (b2) < 110)
	    Print_spec_flag (rfs, i, ew->n_rgs, rz, RF_HIGH_WIND, b2);
	if (strlen (b3) < 110)
	    Print_spec_flag (rfs, i, ew->n_rgs, rz, 
					RF_HIGH_VS | RF_LOW_VS, b3);
    }
    if (strlen (b1) > 0)
	sprintf (buf + strlen (buf), "HIGH_VS %s", b1);
    if (strlen (b2) > 0)
	sprintf (buf + strlen (buf), "HIGH_WIND %s", b2);
    if (strlen (b3) > 0)
	sprintf (buf + strlen (buf), "VHIGH_VS %s", b3);
    VDD_log ("%s\n", buf);
}

/************************************************************************

    Prints flag "sflag" to "b".

*************************************************************************/

static void Print_spec_flag (unsigned short *flags, int i, int n_rgs, int rz,
				unsigned short sflag, char *b) {

    if ((flags[i] & sflag) == sflag) {
	if (i == 0 || (flags[i - 1] & sflag) != sflag)
	    sprintf (b + strlen (b), "%d-", i * rz);
	if (i == n_rgs - 1)
	    sprintf (b + strlen (b), "%d ", (i + 1) * rz);
    }
    if ((flags[i] & sflag) != sflag && i > 0 && 
			(flags[i - 1] & sflag) == sflag)
	sprintf (b + strlen (b), "%d ", i * rz);
}

/********************************************************************

    Updates the environmental wind data affected by "region".

*********************************************************************/

int VDE_update_ew_data (Vdeal_t *vdv, Region_t *region) {
    int xs, ys, xe, ye, yz, rstd, stride, y, rz, n_rgs, min_n_gates;
    Ew_struct_t *ew;
    short *ews;
    unsigned char *ew_aind;

    ew = &(vdv->ew);
    stride = vdv->xz;
    rstd = region->xz;
    xs = region->xs;
    ys = region->ys;
    ye = ys + region->yz;
    xe = xs + region->xz;
    rz = ew->rz;
    n_rgs = ew->n_rgs;
    ews = ew->ews;
    ew_aind = vdv->ew_aind;
    min_n_gates = .1 * ew->rz * ew->az / vdv->gate_width;
    if (vdv->low_prf)
	min_n_gates *= 2;
    yz = vdv->yz;
    y = ys;
    while (y < ye) {
	int yi, yn, x;

	yi = ew_aind[y % yz];
	yn = y + 1;
	while (yn < ye && ew_aind[yn % yz] == yi)
	    yn++;
	yn = yn - y;
	x = xs;
	while (x < xe) {
	    int xi, xn, n_gg, ng, nb, tv;
	    unsigned char *dmap;
	    Ew_update_t *ewups;

	    xi = x / rz;
	    if (xi >= n_rgs)
		break;
	    xn = rz - (x % rz);
	    if (x + xn > xe)
		xn = xe - x;
	    dmap = vdv->dmap + x;
	    ewups = (Ew_update_t *)(ew->ups) + (yi * n_rgs + xi);
	    ng = Count_processed_gates (
			region->data + ((y - ys) * rstd + (x - xs)), 
			rstd, xn, yn, dmap, stride, y, yz, &nb, &tv);
	    ewups->n_gg += ng;
	    ewups->tgv += tv;

	    n_gg = ewups->n_gg;
	    if (n_gg >= min_n_gates) {
		ews[yi * ew->n_rgs + xi] = (ewups->tgv + (n_gg >> 1))/ n_gg;
	    }
	    x += xn;
	}
	y += yn;
    }

    return (0);
}

/********************************************************************

    Updates the environmental wind map (ewm) affected by "region".

*********************************************************************/

int VDE_update_ew (Vdeal_t *vdv, Region_t *region) {

    VDE_update_ew_data (vdv, region);

    if (vdv->phase == 1)
	VDV_vad_analysis (vdv, 1);
    VDE_generate_ewm (vdv);

    return (0);
}

/***************************************************************************

    Returns ew value at x, y (in vdv->inp coordinate).

***************************************************************************/

short VDE_get_ew_value (Vdeal_t *vdv, int x, int y) {

    return (Get_ew_value (vdv, x / vdv->ew.rz, y));
}

/********************************************************************

    Counts the number of processed gates in "reg" of stride "rstd" in
    the area of sizes "xz" and "yz". "dmap" is the dealiasing map of
    stride "dmstd". The number of good processed gates is returned and
    the number of bad processed gates is returned with "nb". The sum of
    good processed gates are returned with "tvp".

*********************************************************************/

static int Count_processed_gates (short *reg, int rstd, int xz, int yz, 
	unsigned char *dmap, int dmstd, int dys, int dyz, int *nb, int *tvp) {
    int df, ngg, nbg, tv, y;

    df = DMAP_BH | DMAP_BE | DMAP_FILL;
    ngg = nbg = tv = 0;
    for (y = 0; y < yz; y++) {
	int x;
	unsigned char *dm;
	short *sp;

	sp = reg + (y * rstd);
	dm = dmap + ((y + dys) % dyz) * dmstd;
	for (x = 0; x < xz; x++) {
	    if (sp[x] != SNO_DATA) {
		if (dm[x] & df)
		    nbg++;
		else {
		    ngg++;
		    tv += sp[x];
		}
	    }
	}
    }
    *nb = nbg;
    *tvp = tv;
    return (ngg);
}

/********************************************************************

    Performs global dealiasing of an area.

*********************************************************************/

void VDE_ew_deal_area (Vdeal_t *vdv, int xs, int ys, int xz, int yz) {
    Ew_struct_t *ew;
    unsigned char *ew_aind;
    int rz, n_rgs, nyq, y;
    int min, max;

    ew = &(vdv->ew);
    ew_aind = vdv->ew_aind;
    rz = ew->rz;
    n_rgs = ew->n_rgs;
    nyq = VDD_get_nyq (vdv, ys);
    min = 0xffff;
    max = -min;
    for (y = ys; y < ys + yz; y++) {
	unsigned char *in;
	short *out, v;
	int x;

	in = vdv->inp + y * vdv->xz;
	out = vdv->out + y * vdv->xz;
	for (x = xs; x < xs + xz; x++) {
	    int xi, q;
	    float ewdiff, ewv, t;

	    if (in[x] == BNO_DATA) {
		out[x] = SNO_DATA;
		continue;
	    }
	    v = EE_get_eew_value (x, y);
	    if (v == SNO_DATA) {
		xi = x / rz;
		v = Get_ew_value (vdv, xi, y);
		if (v == SNO_DATA)
		    v = vdv->data_off;
	    }
	    if (v < min)
		min = v;
	    if (v > max)
		max = v;
	    ewv = v;
	    ewdiff = ewv - in[x];
	    if (ewdiff >= 0.)
		t = ewdiff;
	    else
		t = -ewdiff;
	    q = (t + nyq) / (2 * nyq);
	    if (ewdiff < 0.)
		q = -q;
	    q *= 2 * nyq;
	    out[x] = in[x] + q;
	}
    }

    if (vdv->low_prf || vdv->phase == 1)
	return;

    max += nyq / 2;
    min -= nyq / 2;
    for (y = ys; y < ys + yz; y++) {
	short *out;
	int x;

	out = vdv->out + y * vdv->xz;
	for (x = xs; x < xs + xz; x++) {
	    if (out[x] == SNO_DATA)
		continue;
	    if (out[x] > max)
		out[x] -= 2 * nyq;
	    else if (out[x] < min)
		out[x] += 2 * nyq;
	}
    }
}

/********************************************************************

    Compare globally dealiased gate with EW and sets the DMAP_BE flag
    if the difference is too large. Returns the number of gates that
    have the bit set.

*********************************************************************/

int VDE_check_global_deal (Vdeal_t *vdv, Region_t *region, int gd) {
    int stride, xsr, yr, rz, n_rgs, cnt, thr, vsthr, vvsthr, hthr, hrg, nyq;
    Ew_struct_t *ew;

    stride = vdv->xz;
    ew = &(vdv->ew);
    rz = ew->rz;
    n_rgs = ew->n_rgs;
    xsr = region->xs;
    nyq = VDD_get_nyq (vdv, region->ys);
    hrg = VDV_alt_to_range (vdv, 8000.) / vdv->g_size;

    if (vdv->phase == 2) {
	vvsthr = vsthr = thr = nyq;
	if (vdv->elev < 5.f) {
	    vvsthr = nyq * 4 / 5;
	    if (vdv->data_type & DT_VH_VS)
		thr = nyq * 4 / 5;
	}
    }
    else {
	vvsthr = vsthr = thr = nyq * 4 / 3;
	if (vdv->elev < 5.f) {
	    thr = nyq * 4 / 3;
	    vsthr = nyq * 3 / 4;
	    vvsthr = nyq / 2;
	    if (vdv->data_type & DT_VH_VS)
		thr = nyq / 2;
	}
    }
    hthr = thr * 3 / 2;

    cnt = 0;
    for (yr = 0; yr < region->yz; yr++) {
	int y, xr, yi;
	short *in;
	unsigned char *map;
	unsigned short *rfs;

	y = (yr + region->ys) % vdv->yz;
	in = region->data + yr * region->xz;
	map = vdv->dmap + y * stride + xsr;
	yi = vdv->ew_aind[y];
	rfs = ew->rfs;
	for (xr = 0; xr < region->xz; xr++) {
	    short rf, t;
	    t = in[xr];
	    if (t != SNO_DATA) {
		int diff, xi, off, th;
		short ewv;

		xi = (xr + xsr) / rz;
		off = yi * n_rgs + xi;
		ewv = ew->ewm[off];
		if (ewv == SNO_DATA)
		    continue;
		rf = rfs[xi];
		if (rf & RF_EW_UNAVAILABLE)
		    continue;
		diff = t + gd - ewv;
		if (diff < 0)
		   diff = -diff;
		if (rf & RF_HIGH_VS) {
		    th = vsthr;
		    if (rf & RF_LOW_VS)
			th = vvsthr;
		}
		else if (xr + xsr > hrg)
		    th = hthr;
		else
		    th = thr;
		if (diff >= th) {
		    map[xr] |= DMAP_BE;
		    cnt++;
		}
	    }
	}
    }
    return (cnt);
}

/********************************************************************

    Performs global dealiasing of "region" using the EW map. Returns
    the global dealiasing value. "qerr", if not NULL, returns the
    quantization error. If qerr == NULL, the difference between ewm 
    and region is returned.

*********************************************************************/

int VDE_global_dealiase (Vdeal_t *vdv, Region_t *region,
			unsigned char *dmap, int stride, int *qerr) {
    int xs, ys, xe, ye, yz, rstd, x, rz, n_rgs;
    unsigned char *ew_aind;
    double ewdiff, tw, ewdiffz, twz;
    Ew_struct_t *ew;

    ew = &(vdv->ew);
    ew_aind = vdv->ew_aind;
    rz = ew->rz;
    n_rgs = ew->n_rgs;
    if (qerr != NULL)
	*qerr = 0;
    if (region->xz == 1 && region->yz == 1) {
			/* a frequent case - processed efficiently */
	int xi;
	short ewv, d;

	xi = region->xs / rz;
	ewv = Get_ew_value (vdv, xi, region->ys);
	if (ewv == SNO_DATA)
	    ewv = vdv->data_off;
	d = region->data[0];
	if (d == SNO_DATA)	/* This is possible because removing overlap */
	    return (0);
	ewdiff = (int)ewv - d;
	tw = 1.f;
	goto done;
    }

    rstd = region->xz;
    xs = region->xs;
    ys = region->ys;
    ye = ys + region->yz;
    xe = xs + region->xz;
    yz = vdv->yz;

    x = xs;				/* the left most column */
    tw = ewdiff = twz = ewdiffz = 0.f;	/* total weight and diff from ewm */
    while (x < xe) {
	int xn, xi, y;

	xi = x / rz;
	xn = rz - (x % rz);
	if (x + xn > xe)
	    xn = xe - x;

	y = ys;
	while (y < ye) {			/* do one column */
	    int yi, yn, n;
	    unsigned char *dm;
	    float a;

	    yi = ew_aind[y % yz];
	    yn = y + 1;
	    while (yn < ye && ew_aind[yn % yz] == yi)
		yn++;
	    yn = yn - y;

	    if (dmap != NULL)
		dm = dmap + x;
	    else
		dm = NULL;
	    Compute_average (region->data + ((y - ys) * rstd + (x - xs)),
			rstd, xn, yn, dm, stride, y, yz, &n, &a);
	    if (n > 0) {
		short ewv;
		double w;
		int ewind = yi * n_rgs + xi;
		ewv = ew->ewm[ewind];
		w = ew->eww[ewind] * n;
		if (ewv == SNO_DATA) {
		    ewdiffz += ((float)vdv->data_off - a) * w;
		    twz += w;
		}
		else {
		    ewdiff += ((float)ewv - a) * w;
		    tw += w;
		}
	    }
	    y += yn;
	}
	x += xn;			/* next column */
    }

    if (tw == 0.) {
	tw = twz;
	ewdiff = ewdiffz;
    }

    if (tw == 0.)		/* no data or EW available */
	return (0);

done:
    {
	int nyq, q, qe, diff;
	diff = Myround (ewdiff / tw);
	nyq = VDD_get_nyq (vdv, region->ys);
	q = VDE_quantize_gd (nyq, diff, &qe);
	if (qe > nyq * 2 / 3) {
	    int rdf, rq, rqe;
	    rdf = Near_range_dealiase (vdv, region, nyq);
	    if (rdf != SNO_DATA) {
		rq = VDE_quantize_gd (nyq, rdf, &rqe);
		if (rqe <= nyq / 2) {
		    diff = rdf;
		    q = rq;
		    qe = rqe;
		}
	    }
	}
	if (qerr == NULL)
	    return (diff);
	*qerr = qe;
	return (q);
    }
}

/***************************************************************************

    Returns the quantized value, in terms of nyq * 2, of diff.

***************************************************************************/

int VDE_quantize_gd (int nyq, int diff, int *qerr) {
    int abs, q;

    abs = diff >= 0. ? diff : -diff;
    q = (abs + nyq) / (2 * nyq);
    if (diff < 0.)
	q = -q;
    q *= 2 * nyq;
    if (qerr != NULL) {
	int qe = Myround (diff - q);
	if (qe < 0)
	    qe = -qe;
	*qerr = qe;
    }
    return (q);
}

/**************************************************************************

    Returns EW at location of EW range grid index xi and radial y in the
    image.

**************************************************************************/

static short Get_ew_value (Vdeal_t *vdv, int xi, int y) {
    Ew_struct_t *ew;
    int yi;
    short *ewm, v;

    yi = vdv->ew_aind[y % vdv->yz];
    if (xi >= vdv->ew.n_rgs)
	xi = vdv->ew.n_rgs - 1;

    ew = &(vdv->ew);
    ewm = ew->ewm;
    v = ewm[yi * ew->n_rgs + xi];
    return (v);
}

/**********************************************************************

    Computes gate average in "region" at (0, 0) of size ("xn", "yn").
    "dmap" is applied to exclude bad gates. "np" and "ap" return the
    gate average and the number of all good gates. "rstrd" and "dstrd"
    are strides for "region" and "dmap". "dys" is the starting y index
    in dmap and "dyz" is the total number of rows of dmap.

***********************************************************************/

static void Compute_average (short *region, int rstrd, int xn, int yn, 
			unsigned char *dmap, int dstrd, int dys, int dyz,
			int *np, float *ap) {
    int n, a;
    int x, y;

    n = a = 0;
    for (y = 0; y < yn; y++) {
	short *r;
	unsigned char *dm;

	r = region + (rstrd * y);
	if (dmap != NULL)
	    dm = dmap + (dstrd * ((y + dys) % dyz));
	else
	    dm = 0;
	for (x = 0; x < xn; x++) {
	    int v;

	    v = r[x];
	    if (v == SNO_DATA)
		continue;
	    if (dmap != NULL && (dm[x] & (DMAP_BH || DMAP_FILL)))
		continue;
	    a += v;		/* overflow should not happen */
	    n++;
	}
    }
    if (n > 0)
	*ap = (float)a / n;
    else
	*ap = 0.f;
    *np = n;
}

/**************************************************************************

    Generates the environmental wind map, ewm, by filling in missing EW grid
    with VAD and interpolation. The weighting factor grid, eww, is alao
    generated which represents the quality map of ewm. The possible weighting
    values are 1, 2, 4, 8, and 16.

**************************************************************************/

typedef struct {
    float u, v;		/* EW wind compoments smoothed and extended linearly */
    float s;		/* vad speed */
    int rup, rlow;	/* upper and lower bounds for range interp */
} Ew_wind;		/* EW wind for generating ewm */
static Ew_wind *Ews = NULL;
static float Fno_data = 1.e20f;

int VDE_generate_ewm (Vdeal_t *vdv) {
    static short *vadm = NULL;
    Ew_struct_t *ew;
    short *ewm, absmax;
    unsigned char *eww;
    int n_rgs, n_azs, y, x, ccnt, max_rsteps;

    ew = &(vdv->ew);
    ewm = ew->ewm;
    eww = ew->eww;
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;

    /* copy to bw map and remove grids that are isolate */
    memcpy (ewm, ew->ews, n_azs * n_rgs * sizeof (short));

    /* add ewm from near elevation */
    if (!(vdv->data_type & DT_VH_VS))
	Add_near_elevation_ew (vdv);

    if (!(vdv->data_type & DT_VH_VS))
	PP_set_front_ew (vdv);		/* set ewm in front area */

    vadm = (short *)STR_reset (vadm, n_rgs * n_azs * sizeof (short));
    Prepare_vad_for_ew_map (vdv, vadm);

    max_rsteps = Myround (75000. / (vdv->g_size * ew->rz));	/* 75 km */
    if (!(vdv->data_type & DT_NONUNIFORM))  /* set ewm in second trip area */
	Extend_second_trip_area (ew, max_rsteps * 3 / 2);

    for (x = 0; x < n_rgs; x++) {	/* set ewm in FAIL_P1 area */
	for (y = 0; y < n_azs; y++) {
	    int off = y * n_rgs + x;
	    if (ew->efs[off] & EF_HIGH_SHEAR)
		ewm[off] = vadm[off];
	}
    }

    /* fill in storm areas with linear interpolation */
    absmax = 5000;	/* value limit of ews - must > |SNO_DATA| */
    ccnt = 0;
    for (x = 0; x < n_rgs; x++) {
	unsigned short *rfs;
	int rsteps, asteps;
	double maxa;

	rfs = ew->rfs;
	maxa = 45.;		/* max interp distance in azi */
	if (Ews[x].s != Fno_data && !(rfs[x] & RF_HIGH_VS)) {
	    double t = 20. * vdv->nyq / Ews[x].s;
	    if (t < maxa)
		maxa = t;
	}
	asteps = Myround (maxa * n_azs / 360.);

	rsteps = max_rsteps;
	if (x * 2 < n_rgs) {
	    if (rfs[x] & RF_HIGH_VS)
		rsteps /= 2;
	    else
		rsteps = rsteps * 2 / 3;
	}
	if ((vdv->data_type & DT_NONUNIFORM) && x > n_rgs / 2) {
	    asteps = 0;
	    rsteps = 50;
	}

	Set_range_limit (vdv, x, rsteps);

	for (y = 0; y < n_azs; y++) {
	    short v;

	    int off = y * n_rgs + x;
	    if (ewm[off] != SNO_DATA)
		continue;

	    v = Get_interp_value (vdv, x, y, rsteps, asteps, absmax, vadm);
	    if (v != SNO_DATA)
		ewm[off] = v;
	    else {
		ewm[off] = vadm[off];
		eww[off] = 2;
	    }
	    ewm[off] += 2 * absmax;
	    ccnt++;
	}
    }
	
    if (ccnt > 0) {
	for (y = 0; y < n_azs; y++) {
	    short *e = ewm + (y * n_rgs);
	    for (x = 0; x < n_rgs; x++) {
		if (e[x] > absmax)
		    e[x] -= 2 * absmax;
	    }
	}
    }

    return (0);
}

/***************************************************************************

    For missing ews grid, where the ew is known to be not good, we take the 
    ews from the previous cut that is the closest, in elevation, to the 
    current cut.

***************************************************************************/

static void Add_near_elevation_ew (Vdeal_t *vdv) {
    Ew_struct_t *ew;
    int n_rgs, n_azs, y, x;
    unsigned char *efs;
    short *ewm;
    double min_r;

    ew = &(vdv->ew);
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;
    ewm = ew->ewm;
    efs = ew->efs;
    min_r = VDV_alt_to_range (vdv, 8000.);
    for (x = 0; x < n_rgs; x++) {
	double r;
	r = (x * ew->rz + ew->rz / 2) * vdv->g_size;
	if (r < min_r)
	    continue;
	for (y = 0; y < n_azs; y++) {
	    int off = y * n_rgs + x;
	    if ((efs[off] & EF_LOW_ELE_EW) && ewm[off] == SNO_DATA) {
		ewm[off] = EE_get_near_elev_ew (vdv, r, 
					(y + .5) * 360. / n_azs);
	    }
	}
    }
}

/****************************************************************************

    Reads and processes environmental (VAD) wind for generating ewm. Ews and
    vadm are populated here. vadm provides an EW based estimate of the
    background wind.

****************************************************************************/

static void Prepare_vad_for_ew_map (Vdeal_t *vdv, short *vadm) {
    Ew_struct_t *ew;
    int n_rgs, n_azs, x, rz, cnt;
    unsigned short *rfs;
    float u[MAX_EW_NRS], v[MAX_EW_NRS], uep[MAX_EW_NRS], vep[MAX_EW_NRS];
    unsigned char *bvad;

    ew = &(vdv->ew);
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;
    rfs = ew->rfs;
    rz = ew->rz;

    Ews = (Ew_wind *)STR_reset (Ews, n_rgs * sizeof (Ew_wind));
    cnt = 0;
    for (x = 0; x < n_rgs; x++) {
	double spd, azi;
	Ew_wind *w;

	w = Ews + x;
	if (VDV_get_wind (vdv, x * rz + rz / 2, &spd, &azi) < 0) {
	    u[x] = v[x] = w->s = Fno_data;
	    rfs[x] |= RF_EW_UNAVAILABLE;
	    continue;
	}
	rfs[x] &= ~RF_EW_UNAVAILABLE;
	w->s = spd;
	if (rfs[x] & RF_2SIDE_VAD) {
	    double alfa = (90. - azi) * deg2rad;
	    u[x] = spd * cos (alfa);
	    v[x] = spd * sin (alfa);
	    cnt++;
	}
	else {
	    u[x] = v[x] = Fno_data;
	}
    }
    if (cnt == 0) {	/* vad not available - use zero vad map */
	for (x = 0; x < n_rgs; x++)
	    u[x] = v[x] = 0.f;
    }

    for (x = 0; x < n_rgs; x++) {
	uep[x] = u[x];
	vep[x] = v[x];
    }
    Smooth_data (u, n_rgs, 0);
    Smooth_data (v, n_rgs, 0);
    Smooth_data (uep, n_rgs, 4);
    Smooth_data (vep, n_rgs, 4);
    bvad = MISC_malloc (n_azs * sizeof (unsigned char));
    for (x = 0; x < n_rgs; x++) {
	double dir, spd, ux, vx;
	int check_bad_vad, y;
	unsigned char *med_v;
	Ew_wind *w = Ews + x;		/* set Ews */
	w->u = uep[x];
	w->v = vep[x];

	ux = u[x];
	vx = v[x];
	if (vx == 0. && ux == 0.)	/* set vadm */
	    dir = 0.;
	else
	    dir = 90. - atan2 (vx, ux) * rad2deg;
	if (dir < 0.)
	    dir += 360.;
	if (dir > 360.)
	    dir -= 360.;
	spd = sqrt (ux * ux + vx * vx);
	med_v = PP_get_med_v ();
	check_bad_vad = 1;
	if (vdv->low_prf)
	    check_bad_vad = 0;

	for (y = 0; y < n_azs; y++) {	/* set bad VAD grid */
	    int off = y * n_rgs + x;
	    double azi = ((double)y + .5) * 360. / n_azs;
	    vadm[off] = Myround (
		    ( -spd * cos ((dir - azi) * deg2rad)) + vdv->data_off)
			+ VDV_get_bw_vs_correction (vdv, x, y);
	    bvad[y] = 0;
	    if (check_bad_vad && med_v[off] != BNO_DATA && med_v[off] != 255) {
		int df, adf, nyq;
		df = vadm[off] - med_v[off];
		nyq = vdv->nyq;
		adf = df;
		if (adf < 0)
		    adf = -adf;
		if (adf <= nyq / 3) {
		    bvad[y] = 1;
		    continue;
		}
		while (df > nyq)
		    df -= nyq * 2;
		while (df < -nyq)
		    df += nyq * 2;
		if (df < 0) df = -df;
		if (df >= nyq / 3)
		    bvad[y] = 2;
	    }
	}
	Extend_bad_vad (vdv, bvad, n_azs);

	for (y = 0; y < n_azs; y++) {	/* set vadm */
	    int off = y * n_rgs + x;
	    if (bvad[y] == 2)
		vadm[off] = EE_get_previous_ew (vdv, x, y);
	}
    }
    free (bvad);

    Fill_in_vadm_gaps (vdv, vadm);
}

/****************************************************************************

    Extends the detected bad VAD grid in azi direction. Here bvad values of
    2, 1, 0 are respectively bad VAD, good VAD, not determined.

****************************************************************************/

static void Extend_bad_vad (Vdeal_t *vdv, unsigned char *bvad, int n_azs) {
    int st, i, fppi;

    fppi = vdv->full_ppi;
    st = 0;
    while (st < n_azs) {
	int end, gf;
	if (bvad[st] != 2) {
	    st++;
	    continue;
	}
	end = -1;
	gf = 0;
	for (i = 1; i < n_azs; i++) {
	    int ind = st + i;
	    if (!fppi && ind >= n_azs)
		break;
	    ind = ind % n_azs;
	    if (bvad[ind] == 1)
		gf = 1;
	    if (bvad[ind] == 2) {
		end = st + i;
		break;
	    }
	}
	if (end < 0)
	    break;
	if (!gf && end - st < n_azs / 4) {
	    for (i = st; i <= end; i++)
		bvad[i % n_azs] = 2;
	}
	st = end;
    }
}

/****************************************************************************

    Fill in no data in vadm by interpolaton and radial extension.

****************************************************************************/

static void Fill_in_vadm_gaps (Vdeal_t *vdv, short *vadm) {
    Ew_struct_t *ew;
    int n_rgs, n_azs, x, y;

    ew = &(vdv->ew);
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;

    /* fill in small gaps in radial direction */
    for (y = 0; y < n_azs; y++) {
	int s, e, i;
	short *v = vadm + y * n_rgs;
	for (s = 0; s < n_rgs; s++) {
	    if (v[s] != SNO_DATA)
		continue;
	    for (e = s + 1; e < n_rgs; e++) {
		if (v[e] != SNO_DATA)
		    break;
	    }
	    if (e - s <= 3 && s > 0 && e < n_rgs) {
		int df = v[e - 1] - v[s];
		if (df < 0)
		    df = -df;
		if (df <= vdv->nyq / 4) {
		    for (i = s; i < e; i++) {
			v[i] = v[s - 1] + (v[e] - v[s - 1]) * 
						(i - s + 1) / (e - s + 1);
		    }
		}
	    }
	    s = e - 1;
	}
    }

    /* fill in gaps in azi direction */
    for (x = 0; x < n_rgs; x++) {
	int s, e, i;
	short *v = vadm + x;
	for (s = 0; s < n_azs; s++) {
	    if (v[s * n_rgs] != SNO_DATA)
		continue;
	    for (e = s + 1; e < s + n_azs; e++) {
		if (v[(e % n_azs) * n_rgs] != SNO_DATA)
		    break;
	    }
	    if (e - s < n_azs / 3 && (s > 0 || 
				v[(n_azs - 1) * n_rgs] != SNO_DATA)) {
		int vs = v[((s - 1 + n_azs) % n_azs) * n_rgs];
		int ve = v[(e % n_azs) * n_rgs];
		for (i = s; i < e; i++) {
		    v[(i % n_azs) * n_rgs] = vs + (ve - vs) * 
					(i - s + 1) / (e - s + 1);
		}
	    }
	    s = e - 1;
	}
    }

    {	/* radial extension of VAD beyond available data which is better than
	   no data */
	unsigned char *efs = ew->efs;
	for (y = 0; y < n_azs; y++) {
	    int xs, v;
	    for (x = n_rgs - 1; x >= 0; x--) {
		int off = y * n_rgs + x;
		if (efs[off] & EF_CLEAR_AIR) {
		    x++;
		    break;
		}
		if (ew->ewm[off] != SNO_DATA)
		    break;
	    }
	    xs = x;
	    if (xs < 0)
		xs = 0;
	    v = SNO_DATA;
	    for (x = xs; x < n_rgs; x++) {
		int off = y * n_rgs + x;
		if (vadm[off] != SNO_DATA)
		    v = vadm[off];
		else if (vadm[off] == SNO_DATA)
		    vadm[off] = v;
	    }
	}
    }
}

/****************************************************************************

    Returns the interpolated value for ew grid [cx, cy] using neighboring
    grids. rsteps and asteps are the maximum steps in x (range) and y 
    (azimuth).

****************************************************************************/

static int Get_interp_value (Vdeal_t *vdv, int cx, int cy, 
			int rsteps, int asteps, short absmax, short *vadm) {
    int quarter, n_rgs, n_azs, cr, cnt, rl_b, ru_b, dmin, lsteps, i, cr_front;
    short *ewm;
    unsigned short *rfs;
    unsigned char *eww, *efs;
    double dxs[4], dys[4], dvs[4], unavailable, vs;
    Ew_struct_t *ew;
    float arr;

    ew = &(vdv->ew);
    ewm = ew->ewm;
    eww = ew->eww;
    efs = ew->efs;
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;
    rfs = ew->rfs;
    arr = Get_cell_azi_range_ratio (n_azs, n_rgs, cx, NULL);

    cr = cx + cy * n_rgs;			/* current offset */
    rl_b = -1;
    ru_b = n_rgs;
    if (efs[cr] & EF_FRONT) {
	rl_b = cx - 1;
	ru_b = cx + 1;
    }
    else {
	for (i = 1; i <= rsteps; i++) {
	    if (cx - i >= 0 && efs[cr - i] & EF_FRONT) {
		rl_b = cx - i;
		break;
	    }
	}
	for (i = 1; i <= rsteps; i++) {
	    if (cx + i < n_rgs && efs[cr + i] & EF_FRONT) {
		ru_b = cx + i;
		break;
	    }
	}
    }
    cr_front = efs[cr] & EF_FRONT;

    cnt = 0;
    vs = 0.;
    dmin = 0x7fffffff;		/* a large number */
    unavailable = 1.e10;
    /* Note that the azi direction is pointed to down (instead of up).
       quarter 1 right, quarter 2 down and so on. */
    for (quarter = 1; quarter <= 4; quarter++) {
	int rsign, asign, search_steps, firststep;
	int dx, dy, dv, c, hsg_yes;

	dxs[quarter - 1] = unavailable;
	asign = rsign = 1;
	if (quarter == 3 || quarter == 4)
	    asign = -1;
	if (quarter == 2 || quarter == 3)
	    rsign = -1;
	if (quarter == 1 || quarter == 3)
	    search_steps = rsteps;
	else
	    search_steps = asteps;
	firststep = search_steps;
	c = 0;
	dx = dy = dv = 0;
	hsg_yes = 0;
	if (vdv->low_prf)	/* disalbe hsg processing */
	    hsg_yes = -1;
	Check_hs_grid (-1, 0, 0, NULL, 0);
	for (i = 1; i <= search_steps; i++) {
	    int nextj;
	    short ev;
	    if (i > firststep + 2)
		break;
	    nextj = 0;
	    while (1) {
		int j, dr, da, ri, ai, sof, d_front, hsg;

		j = nextj;
		if (j < 0 && c > 0)
		    break;
		if (j >= i)
		    break;
		if (nextj >= 0) {
		    nextj = -nextj - 1;
		}
		else 
		    nextj = -nextj;
		if (quarter == 1 || quarter == 3) {
		    da = j * asign;
		    dr = i * rsign;
		}
		else {
		    dr = j * rsign;
		    da = i * asign;
		}

		if ((da >= 0 ? da : -da) > asteps ||
					(dr >= 0 ? dr : -dr) > rsteps)
		    continue;
		ri = cx + dr;
		if (ri < 0 || ri >= n_rgs)
		    continue;
		if (dr < 0) {
		    if (ri <= Ews[cx].rlow || ri <= rl_b)
			continue;
		}
		else if (dr > 0) {
		    if (ri >= Ews[cx].rup || ri >= ru_b)
			continue;
		}
		ai = cy + da;
		ai = (ai + n_azs) % n_azs;

		sof = ai * n_rgs + ri;
		d_front = efs[sof] & EF_FRONT;
		if ((cr_front && !d_front) || (!cr_front && d_front)) {
					/* data of this quarter not used */
		    i = search_steps + 1;
		    c = 0;
		    break;
		}
		hsg = efs[sof] & EF_HS_GRID;
		if (hsg && hsg_yes == 0)
		    hsg_yes = 1;
		if (hsg_yes == 1 && Check_hs_grid (i, -j, sof, vdv, absmax))
		    continue;		/* data behind HS grid - not used */
		ev = ewm[sof];
		if (ev == SNO_DATA || ev > absmax)
		    continue;

		dx += dr;
		dy += da;
		dv += ev;
		if (c == 0)
		    firststep = i;
		c++;
		if (i < dmin)
		    dmin = i;
	    }
	}
	if (c > 0) {	/* found at least one for this quarter */
	    int ind = quarter - 1;
	    dxs[ind] = (double)dx / c;
	    dys[ind] = (double)dy * arr / c;
	    dvs[ind] = (double)dv / c;
	    vs += dvs[ind];	/* sum of the data values */
	    cnt++;
	}
    }

    if (cnt == 0)
	return (SNO_DATA);

    {			/* discard outliers in dxs, dys, dvs */
	int maxd;
	vs = vs / cnt;
	maxd = 0;
	for (i = 0; i < 4; i++) {
	    int df;
	    if (dxs[i] == unavailable)
		continue;
	    df = dvs[i] - vs;
	    if (df < 0)
		df = -df;
	    if (df > maxd)
		maxd = df;
	}
	if (maxd * 3 > vdv->nyq) {	/* data variation is large */
		/* for each quarter, we remove any distant data */
	    for (quarter = 1; quarter <= 4; quarter++) {
		int c, mind, ind[4], d2[4];
		c = 0;
		for (i = 0; i < 4; i++) {
		    int ok = 0;
		    if (dxs[i] == unavailable)
			continue;
		    if (quarter == 1 && dxs[i] >= 0 && dys[i] >= 0)
			ok = 1;
		    if (quarter == 2 && dxs[i] >= 0 && dys[i] <= 0)
			ok = 1;
		    if (quarter == 3 && dxs[i] <= 0 && dys[i] <= 0)
			ok = 1;
		    if (quarter == 4 && dxs[i] <= 0 && dys[i] >= 0)
			ok = 1;
		    if (ok) {
			ind[c] = i;
			c++;
		    }
		}
		if (c <= 1)
		    continue;
		mind = 0x7fffffff;	/* min distance */
		for (i = 0; i < c; i++) {
		    int dx, dy;
		    dx = dxs[ind[i]];
		    dy = dys[ind[i]];
		    d2[i] = dx * dx + dy * dy;
		    if (d2[i] < mind)
			mind = d2[i];
		}
		for (i = c - 1; i >= 0; i--) {
		    if (d2[i] <= mind * 2)	/* distant ok */
			continue;
		    dxs[ind[i]] = unavailable;	/* rm ind[i] */
		}
	    }
	}
    }

    lsteps = rsteps >= asteps ? rsteps : asteps;
    {			/* add vadm to the data set */
	double v, xmax, xmin, ymax, ymin;
	xmax = ymax = -1000.;
	xmin = ymin = 1000.;
	for (i = 0; i < 4; i++) {
	    if (dxs[i] == unavailable)
		continue;
	    if (dxs[i] > xmax)
		xmax = dxs[i];
	    if (dxs[i] < xmin)
		xmin = dxs[i];
	    if (dys[i] > ymax)
		ymax = dys[i];
	    if (dys[i] < ymin)
		ymin = dys[i];
	}
	/* Here I add VAD at the current location to the data for interp. Using
	   VAD at lsteps away will need can-it-be-used control and may not 
	   improve interp result significantly */
	v = vadm[cr];
	if (v != SNO_DATA) {
	    if (xmax <= 0.2 && dxs[0] == unavailable) {
		dxs[0] = lsteps;
		dys[0] = 0.;
		dvs[0] = v;
	    }
	    if (xmin >= -0.2 && dxs[2] == unavailable) {
		dxs[2] = -lsteps;
		dys[2] = 0.;
		dvs[2] = v;
	    }
	    if (ymax < -0.2 && dxs[1] == unavailable) {
		dxs[1] = 0.;
		dys[1] = lsteps * arr;
		dvs[1] = v;
	    }
	    if (ymin > 0.2 && dxs[3] == unavailable) {
		dxs[3] = 0.;
		dys[3] = -lsteps * arr;
		dvs[3] = v;
	    }
	}
    }

    if (lsteps >= dmin * 2)		/* set eww */
	eww[cr] = 16;
    else {
	if (rfs[cx] & RF_2SIDE_VAD)
	    eww[cr] = 8;
	else				/* vad is extrapolated */
	    eww[cr] = 4;
    }

    cnt = 0;
    for (i = 0; i < 4; i++) {		/* remove unavailable data */
	if (dxs[i] == unavailable)
	    continue;
	if (cnt != i) {
	    dxs[cnt] = dxs[i];
	    dys[cnt] = dys[i];
	    dvs[cnt] = dvs[i];
	}
	cnt++;
    }

    return (Lms_interpolation (cnt, dxs, dys, dvs, lsteps * .5));
}

/*************************************************************************

    Checks to find out if grid (x, y) is behind another HS grid in the 
    quarter and returns 1 if yes or 0 if no. If x < 0, the function is reset.

*************************************************************************/

#define MAX_HS_GRIDS 32

static int Check_hs_grid (int x, int y, int sof, Vdeal_t *vdv, short absmax) {
    static int n_hsgs = 0;
    static char hsx[MAX_HS_GRIDS], hsy[MAX_HS_GRIDS];
    static double hsr[MAX_HS_GRIDS];
    Ew_struct_t *ew;
    int hsg;

    if (x < 0) {	/* clear current hs grids */
	n_hsgs = 0;
	return (0);
    }

    ew = &(vdv->ew);
    hsg = ew->efs[sof] & EF_HS_GRID;
    if (hsg) {		/* This is a hs grid */
	short *ewm;
	int n_rgs, i, j, ev, off[4], max, k;
	if (n_hsgs < MAX_HS_GRIDS) {
	    hsx[n_hsgs] = x;
	    hsy[n_hsgs] = y;
	    hsr[n_hsgs] = sqrt ((double)(x * x + y * y));
	    n_hsgs++;
	}
	/* check neighbors to determine if this grid is to use */
	n_rgs = ew->n_rgs;
	i = sof % n_rgs;
	j = sof / n_rgs;
	ewm = ew->ewm;
	ev = ewm[sof];
	if (ev == SNO_DATA || ev > absmax)
	    return (1);
	off[0] = off[1] = 0;
	if (i > 0)
	    off[0] = -1;
	if (i < n_rgs - 1)
	    off[1] = 1;
	off[2] = (((j - 1 + ew->n_azs) % ew->n_azs) - j) * n_rgs;
	off[3] = (((j + 1) % ew->n_azs) - j) * n_rgs;
	max = 0;
	for (k = 0; k < 4; k++) {
	    int v;
	    v = ewm[sof + off[k]];
	    if (v == SNO_DATA || v > absmax)
		continue;
	    if ((v - vdv->data_off) * (ev - vdv->data_off) < 0)
		return (0);	/* different v sign - to use */
	}
	return (1);		/* to not use */
    }
    else {	/* check if this is behind a previous hs grid */
	int i;
	/* The algorithm: For each previous HS grid, we rotate it to the x
	   (horizontal) direction. We check if the rotated (x, y) is within a
	   fan sector of alfa (where tan alfa = .3). The sector is widened by
	   .5 to take care the discrete nature of grid location. The code here
	    is optimized for performance and thus not straightforward */
	for (i = 0; i < n_hsgs; i++) {
	    double xx, yy, r2, xcr, ycr, ybr;
	    xx = hsx[i];
	    yy = hsy[i];
	    r2 = hsr[i] * hsr[i];
	    xcr = xx * x + yy * y;
	    if (xcr <= r2)
		continue;
	    ycr = -yy * x + xx * y;
	    ybr = (xcr - r2) * .3 + .5 * hsr[i];	/* tan alfa = .3 */
	    if (ycr * ycr <= ybr * ybr)
		return (1);
	}
	return (0);
    }
}

/**************************************************************************

    Perfoms linear interpolation of "cnt" values "dvs" at ("dxs", "dys")
    and returns the estimated value at (0, 0). No extrapolated value is
    used. If the target is outside the points and the distance is >
    "maxd", SNO_DATA is returned. When cnt >= 4, the LMS interpolation
    is always fine because the way the points are determined ((0, 0) is
    always inside the span of the points.

**************************************************************************/

static short Lms_interpolation (int cnt, double *dxs, double *dys,
						double *dvs, double max_d) {

    if (cnt == 3) {	/* we check if the target is inside the triangle */
	int pc, pind, nind, i;

	pc = pind = nind = 0;
	for (i = 0; i < 3; i++) {
	    double x1, y1, x2, y2, xx, yy, t;
	    x1 = dxs[i];		/* Both the triangle and the coord */
	    y1 = dys[i];		/* system are clock-wise */
	    x2 = dxs[(i + 1) % 3];	/* so the following will work */
	    y2 = dys[(i + 1) % 3];
	    xx = yy = 0.;	/* target location */
	    t = (y1 - y2) * (xx - x2) - (x1 - x2) * (yy - y2);
	    if (t > -0.001) {
		pc++;
		pind = i;
	    }
	    else
		nind = i;
	}
	if (pc == 2) {		/* target outside one side of the triangle */
	    int c = 0;
	    for (i = 0; i < 3; i++) {
		if (i == ((nind + 2) % 3))	/* drop this point */
		    continue;
		dxs[c] = dxs[i];
		dys[c] = dys[i];
		dvs[c] = dvs[i];
		c++;
	    }
	    cnt = 2;
	}
	else if (pc <= 1) {  /* target outside triangle and near one point */
	    cnt = 1;
	    i = (pind + 2) % 3;
	    dxs[0] = dxs[i];
	    dys[0] = dys[i];
	    dvs[0] = dvs[i];
	}
    }

    if (cnt >= 3) {		/* compute LMS interpolation */
	double sxx, syy, sxy, sxv, syv, sx, sy, sv, d, ad, min, max;
	int i;

	sxx = syy = sxy = sxv = syv = sx = sy = sv = 0.;
	min = max = dvs[0];
	for (i = 0; i < cnt; i++) {
	    double x, y, v;

	    x = dxs[i];
	    y = dys[i];
	    v = dvs[i];
	    sxx += x * x;
	    sxy += x * y;
	    syy += y * y;
	    sxv += x * v;
	    syv += y * v;
	    sx += x;
	    sy += y;
	    sv += v;
	    if (v < min)
		min = v;
	    if (v > max)
		max = v;
	}
	d = sxx * syy * cnt + 2. * sxy * sx * sy - 
			syy * sx * sx - sxy * sxy * cnt - sy * sy * sxx;
	ad = d;
	if (ad < 0.)
	    ad = -ad;
	if (ad >= .1) {
	    double t;
	    short s;
	    t = (sxx * syy * sv + sxy * (syv * sx + sy * sxv) - 
		sxv * syy * sx - sxy * sxy * sv - sxx * sy * syv) / d;
	    if (t < min)
		t = min;
	    if (t > max)
		t = max;
	    s = Myround (t);
	    return (s);
	}
    }

    while (cnt >= 3) {	/* the points are on a line - we drop distant points */
	int i, c, ind;
	double max = -1.;
	ind = 0;
	for (i = 0; i < cnt; i++) {
	    double d = dxs[i] * dxs[i] + dys[i] * dys[i];
	    if (d > max) {
		max = d;
		ind = i;	/* most distant point */
	    }
	}
	c = 0;
	for (i = 0; i < cnt; i++) {
	    if (i == ind)
		continue;
	    dxs[c] = dxs[i];
	    dys[c] = dys[i];
	    dvs[c] = dvs[i];
	    c++;
	}
	cnt = c;
    }
    if (cnt == 2 && dxs[0] == dxs[1] && dys[0] == dys[1])
	cnt = 1;

    if (cnt == 2) {	/* two point interpolation */
	double x1, y1, x2, y2, P, d1, d2;
	x1 = 0. - dxs[0];
	y1 = 0. - dys[0];
	x2 = dxs[1] - dxs[0];
	y2 = dys[1] - dys[0];
	P = x1 * x2 + y1 * y2;
	d2 = x2 * x2 + y2 * y2;
	if (P <= 0.)		/* use first point */
	    cnt = 1;
	else if (P >= d2) {	/* use second point */
	    dxs[0] = dxs[1];
	    dys[0] = dys[1];
	    dvs[0] = dvs[1];
	    cnt = 1;
	}
	else {			/* two point interplation */
	    short s;
	    d1 = x1 * x1 + y1 * y1;
	    if (d1 - P * P / d2 > max_d * max_d)
		return (SNO_DATA);	/* distance too far */
	    s = Myround (dvs[0] + (dvs[1] - dvs[0]) * P / d2);
	    return (s);
	}
    }

    if (cnt == 1) {	/* single point interpolation */
	double x1, y1, d;
	x1 = 0. - dxs[0];
	y1 = 0. - dys[0];
	d = x1 * x1 + y1 * y1;
	if (d > max_d * max_d)
	    return (SNO_DATA);	/* distance too far */
	return (Myround (dvs[0]));
    }
    return (SNO_DATA);
}

/****************************************************************************

    Sets the range limits for linear interpolation.

****************************************************************************/

static void Set_range_limit (Vdeal_t *vdv, int cx, int steps) {
    Ew_struct_t *ew;
    int n_rgs, dir;
    double max_ewc, u, v, cdir;
    unsigned short *rfs;

    ew = &(vdv->ew);
    n_rgs = ew->n_rgs;
    rfs = ew->rfs;

    Ews[cx].rup = n_rgs + steps;
    Ews[cx].rlow = -1 - steps;
    if (Ews[cx].u == Fno_data)
	return;
    u = Ews[cx].u;
    v = Ews[cx].v;
    cdir = 0.;
    if (u != 0. && v != 0.)
	cdir = atan2 (v, u) * rad2deg;
    max_ewc = vdv->nyq * .6;	/* the max EW change (vertical shear) to alow
				   range interplation */
    max_ewc = max_ewc * max_ewc;
    for (dir = 0; dir < 2; dir++) {		/* two directions */
	int i;
	for (i = 1; i <= steps; i++) {
	    int ni;
	    double du, dv, ndir, diff;
	    if (dir == 0) {
		ni = cx - i;
		if (ni < 0)
		    break;
	    }
	    else {
		ni = cx + i;
		if (ni >= n_rgs)
		    break;
	    }
	    if (Ews[ni].u == Fno_data)
		break;
	    u = Ews[ni].u;
	    v = Ews[ni].v;
	    ndir = 0.;
	    if (u != 0. && v != 0.)
		ndir = atan2 (v, u) * rad2deg;
	    diff = ndir - cdir;
	    while (diff < 0.)
		diff += 360.;
	    while (diff >= 360.)
		diff -= 360.;
	    if (diff > 180.)
		diff = 360. - diff;
	    du = u - Ews[cx].u;
	    dv = v - Ews[cx].v;
	    if ((du * du + dv * dv >= max_ewc) || diff > 30.) {
		int inc;

		inc = i;
		if (inc <= 0)
		    inc = 1;
		if (dir == 0)
		    Ews[cx].rlow = cx - inc;
		else
		    Ews[cx].rup = cx + inc;
		break;
	    }
	}
    }

    return;
}

/***************************************************************************

    Smoothing, interpolating and extrapolating array "data" of "n" elements.
    Data in a window of 3 elements is used in smoothing.

***************************************************************************/

static void Smooth_data (float *data, int n, int max_ext_steps) {
    int i, first, last, side;

    first = last = -1;
    for (i = 0; i < n; i++) {		/* interpolate data */
	if (data[i] == Fno_data) {
	    last = i;
	    if (first < 0)
		first = i;
	}
	if (first >= 0 && (data[i] != Fno_data)) {
	    if (first > 0 && last < n - 1) {
		int st, end, k;
		st = first - 1;
		end = last + 1;
		for (k = first; k <= last; k++)
		    data[k] = data[st] + (k - st) *
				(data[end] - data[st]) / (end - st);
	    }
	    first = last = -1;
	}
    }

    for (side = 0; side < 2; side++) {
	int st, end, inc, k, i1, i2, i3;

	if (side == 0) {	/* extrapolate data near element 0 */
	    if (data[0] != Fno_data)
		continue;	/* nothing to do */
	    st = 1;
	    end = n;
	    inc = 1;
	}
	else {			/* extrapolate data near element n - 1 */
	    if (data[n - 1] != Fno_data)
		continue;	/* nothing to do */
	    st = n - 2;
	    end = -1;
	    inc = -1;
	}
	i1 = i2 = i3 = -1;
	for (k = st; k != end; k += inc) {
	    if (data[k] != Fno_data) {
		if (i1 < 0)
		    i1 = k;
		else if (i2 < 0)
		    i2 = k;
		else if (i3 < 0) {
		    i3 = k;
		    break;
		}
	    }
	}
	if (i3 > 0)		/* 3 data elements available */
	    i2 = i3;
	if (side == 0)
	    st = 0;
	else
	    st = n - 1;
	if (i2 > 0) {
	    for (k = st; k != i1; k += inc) {
		int steps = i1 - k;
		if (steps > max_ext_steps)
		    steps = max_ext_steps;
		if (steps < -max_ext_steps)
		    steps = -max_ext_steps;
		data[k] = data[i1] - steps * 
				(data[i2] - data[i1]) / (i2 - i1);
	    }
	}
	else if (i1 > 0) {
	    for (k = st; k != i1; k += inc)
		data[k] = data[i1];
	}
    }

    for (i = 0; i < n; i++) {		/* smooth data */
	float d, out[3];
	int st, ci, k, c;
	if (data[i] == Fno_data)
	    continue;
	if (i == 0) {
	    st = 0;
	    ci = 2;
	}
	else if (i == n - 1) {
	    st = (n - 2);
	    ci = 2;
	}
	else {
	    st = (i - 1);
	    ci = 3;
	}
	c = 0;
	d = 0.f;
	for (k = st; k < st + ci; k++) {
	    if (data[k] != Fno_data) {
		d += data[k];
		c++;
	    }
	}
	out[i % 3] = d / (float)c;	/* c cannot be 0 */
	if (i >= 2)
	    data[i - 2] = out[(i + 3 - 2) % 3];
	if (i == n - 1) {
	    data[n - 2] = out[(n + 1) % 3];
	    data[n - 1] = out[(n + 2) % 3];
	}
    }
}

/****************************************************************************

    Returns the rounded integer of "x".

****************************************************************************/

int Myround (double x) {
    if (x >= 0.)
	return ((int)(x + .5));
    else
	return (-((int)(-x + .5)));
}

/**************************************************************************

    Extend available ews to second trip area.

**************************************************************************/

static void Extend_second_trip_area (Ew_struct_t *ew, int rsteps) {
    short *ewm, absmax;
    unsigned char *efs;
    int n_rgs, n_azs, y, x, changed;

    ewm = ew->ewm;
    efs = ew->efs;
    n_rgs = ew->n_rgs;
    n_azs = ew->n_azs;
    absmax = 5000;	/* value limit of ewm - must > |SNO_DATA| */
    changed = 0;
    for (x = n_rgs / 2; x < n_rgs; x++) {

	if (ew->rfs[x] & RF_HIGH_VS)
	    continue;
	for (y = 0; y < n_azs; y++) {
	    int off, dist1, dist2, step;
	    short v;

	    off = y * n_rgs + x;
	    if (!(efs[off] & EF_SECOND_TRIP) || ewm[off] != SNO_DATA)
		continue;
	    dist1 = dist2 = -1;
	    step = 1;
	    while (step <= rsteps && x - step >= 0) {
		int sof = off - step;
		if (!(efs[sof + 1] & EF_SECOND_TRIP))
		    break;
		v = ewm[sof];
		if (v != SNO_DATA && v <= absmax) {
		    dist1 = step;
		    break;
		}
		step++;
	    }
	    step = 1;
	    while (step <= rsteps && x + step < n_rgs) {
		int sof = off + step;
		if (!(efs[sof - 1] & EF_SECOND_TRIP))
		    break;
		v = ewm[sof];
		if (v != SNO_DATA && v <= absmax) {
		    dist2 = step;
		    break;
		}
		step++;
	    }
	    if (dist1 > 0 && dist2 > 0)
		v = ewm[off - dist1] + (ewm[off + dist2] - ewm[off - dist1]) *
					dist1 / (dist1 + dist2);
	    else if (dist1 > 0)
		v = ewm[off - dist1];
	    else if (dist2 > 0)
		v = ewm[off + dist2];
	    else
		continue;
	    ewm[off] = v + 2 * absmax;
	    changed = 1;
	}
    }

    if (changed) {
	for (y = 0; y < n_azs; y++) {
	    short *e = ewm + (y * n_rgs);
	    for (x = 0; x < n_rgs; x++) {
		if (e[x] > absmax)
		    e[x] -= 2 * absmax;
	    }
	}
    }
}

/**************************************************************************

    Returns the azimuth-to-range grid cell width ratio at grid of range 
    of grid index rind. The step-to-azi-grid-increment table is returned 
    by "st2ai" for this grid point. The ratio is set to 1 if it is less 
    than 1. The max ratio can be as large as 6.23 when n_rgs = n_azs. Thus
    n_rgs should probably no larger than n_azs. In order to increase azi
    search distance, we scale down "ratio" for searching indexes. I can
    either scale down "ratio" or limit it to, e.g., 2. The latter is used.
    Note that the unscaled ratio gets geo search distance while ratio = 1
    gets B-scan distance.

**************************************************************************/

static float Get_cell_azi_range_ratio (int n_azs, int n_rgs, int rind, 
						unsigned char **st2ai) {
    static unsigned char *step_to_ai = NULL;
    static float *arrs = NULL;
    static int pn_rgs = 0, pn_azs = 0;

    if (n_rgs != pn_rgs || n_azs != pn_azs) {
	int i;

	if (arrs != NULL)
	    free (arrs);
	arrs = (float *)MISC_malloc (n_rgs * sizeof (float) + 
				(n_rgs * n_rgs) * sizeof (unsigned char));
	step_to_ai = (unsigned char *)(arrs + n_rgs);
	pn_rgs = n_rgs;
	pn_azs = n_azs;

	for (i = 0; i < n_rgs; i++) {
	    double ratio, ratio2;
	    int ai, j;
	    unsigned char *p;

	    ratio = ((double)i + .5) * 2. * sin (pi / (double)n_azs);
	    if (ratio < 1.)
		ratio = 1.;
	    arrs[i] = ratio;

	    /* modify the search distant weight in azi to at most 2. */
	    ratio2 = 2. * ((double)i + .5) / (n_rgs - 1);
	    if (ratio2 < 1.)
		ratio2 = 1.;
	    if (ratio > ratio2)
		ratio = ratio2;
	    /* ratio = 1. + (ratio - 1.) * .3; scaling down v.s. use ratio2 */

	    p = step_to_ai + (i * n_rgs);
	    p[0] = 0;
	    ai = 1;
	    for (j = 1; j < n_rgs; j++) {
		double d = (double)j;
		if (d >= ai * ratio - .5) {
		    p[j] = ai;
		    ai++;
		}
		else
		    p[j] = 255;		/* not a valid value */
	    }
	}
    }

    if (rind < 0 || rind >= n_rgs) {
	MISC_log ("Code error - Invalid EW grid range index %d\n", rind);
	rind = 0;
    }
    if (st2ai != NULL)
	*st2ai = step_to_ai + (rind * n_rgs);
    return (arrs[rind]);
}

/*************************************************************************

    Returns the difference of region data from the nearest out data towards
    radar.

*************************************************************************/

static int Near_range_dealiase (Vdeal_t *vdv, Region_t *region, int nyq) {
    int cnt, rcnt, df, y, maxx, minx, cyi, dminx;

    if (vdv->phase == 1)	/* we will apply to both phase */
	return (SNO_DATA);

    maxx = 0;
    for (y = 0; y < region->yz; y++) {
	short *out;
	int x;
	out = vdv->out + ((y + region->ys + vdv->yz) % vdv->yz) * vdv->xz;
	for (x = region->xs - 1; x >= 0; x--) {
	    if (out[x] != SNO_DATA)
		break;
	}
	if (x >= 0 && x > maxx)
	    maxx = x;
    }
    dminx = maxx - 20000 / vdv->g_size;  /* data-based search range (minx) */

    cnt = rcnt = df = 0;
    cyi = -1;
    minx = 0;
    for (y = 0; y < region->yz; y++) {
	short *rdata, *out;
	int rd, yi, x;

	rdata = region->data + y * region->xz;
	rd = SNO_DATA;
	for (x = 0; x < region->xz; x++) {
	    if (rdata[x] != SNO_DATA) {
		rd = rdata[x];
		break;
	    }
	}
	if (rd == SNO_DATA)
	    continue;
	rcnt++;

	yi = vdv->ew_aind[(region->ys + y) % vdv->yz];
	if (yi != cyi) {	/* set search range (minx) */
	    int xi, low, clx;
	    Ew_struct_t *ew = &(vdv->ew);
	    unsigned char *efs = ew->efs + yi * ew->n_rgs;
	    for (xi = ew->n_rgs - 1; xi >= 1; xi--) {
		if (efs[xi] & EF_CLEAR_AIR)
		    break;
	    }
	    clx = (xi + 1) * ew->rz;
	    minx = clx;
	    if (region->xs > clx && clx < vdv->xz / 2)
		minx = region->xs - 
		    (region->xs - clx) * vdv->xz / ((vdv->xz - clx) * 3);
	    if (minx < dminx)
		minx = dminx;
	    low = (Ews[region->xs / ew->rz].rlow) * ew->rz;
	    if (minx < low)	/* limit based on VS */
		minx = low;
	    cyi = yi;
	}

	out = vdv->out + ((y + region->ys + vdv->yz) % vdv->yz) * vdv->xz;
	for (x = region->xs - 1; x >= minx; x--) {
	    if (out[x] != SNO_DATA) {
		df += out[x] - rd;
		cnt++;
		break;
	    }
	}
    }
    if ((cnt < 3 && rcnt >= 3) || cnt == 0)
	return (SNO_DATA);

    return (df / cnt);
}




