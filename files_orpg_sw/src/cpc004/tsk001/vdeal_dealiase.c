
/******************************************************************

    vdeal's module for dealiaing runtines.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:10:21 $
 * $Id: vdeal_dealiase.c,v 1.12 2014/10/03 16:10:21 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include "vdeal.h"
#include <pthread.h>
#include <errno.h>

enum {P_WAIT, P_PROCESS, P_DONE, P_READY, P_OUT};	/* for Part_t.state */

int VDD_in_thread = 0;
extern int Test_mode;
int No_reprocess = 0;
int Test_mode = 0;
int dbg = 0;
int VDA_b16_update1 = 1;

int Max_partition_size = 56;	/* for B16 test */

static int N_parts = 0;		/* current number of partitions */
static Part_t *Parts = NULL;	/* partition arrays - variable sized */

enum {AR_SAVE, AR_REWIND, AR_READ, AR_RESET};

static void Process_partition (Vdeal_t *vdv, Part_t *part);
static void Partition_image (Vdeal_t *vdv);
static int Remove_overlapped_data (Region_t *region, int y_st, int yz);
static void Process_unprocessed_gates (Vdeal_t *vdv, int ys, int yz);
static void Clear_area_dmap (Vdeal_t *vdv, int ys, int yz, int cflags);
static void Clear_region_dmap (Vdeal_t *vdv, Region_t *region);
static void Put_back_gcc_gates (Vdeal_t *vdv, int ys, int yz);
static void M_proc_partition (Vdeal_t *vdv, int p_ind);
static void *T_proc_partition (void *arg);
static void Parallelize_partition ();
static void Mutex_lock ();
static void Mutex_unlock ();
static int Global_dealiase (Vdeal_t *vdv, Region_t *region,
			int step, int n_gs, int small_rgz, Part_t *part);
static void Remove_fill (Vdeal_t *vdv, int ys, int yz);
static void Rm_thin_conn (unsigned char *inp, Data_filter_t *cdf,
		Params_t *param, int xz, int yz, unsigned char *dmap, 
					unsigned char m_bf);
static void Copy_dmap (Vdeal_t *vdv, Part_t *part, Region_t *reg);
static void Update_ew_data (Vdeal_t *vdv, int pys, int pyz);
static int Archive_region (int func, Region_t *reg, int ys, int yz);
static void Process_saved_regions (Vdeal_t *vdv, int ys, int yz);
static void Output_data (Vdeal_t *vdv);
static int Correct_extreme (Vdeal_t *vdv, Region_t *region, int gd, int nyq);
static void Post_process_partition (Vdeal_t *vdv, int ptind);
static void Fix_part_data (Vdeal_t *vdv, int x, int y);
static int Ev_depth, Ev_cnt, Ev_xz, Ev_yz, Ev_thrh,
			Ev_thrl, Ev_gd, Ev_ncnt, Ev_thrd, Ev_gcnt, Ev_bcnt;
static int Is_high_wind_region (Vdeal_t *vdv, Region_t *region);
static void Set_fpd_min_max (Vdeal_t *vdv);

/********************************************************************

    Processes realtime data.

*********************************************************************/

int VDD_process_realtime (Vdeal_t *vdv) {
    int i;

    if (vdv->radial_status != RS_END_ELE)
	return (0);	/* wait until the entire cut is received */

    vdv->phase = 2;
    Archive_region (AR_RESET, NULL, 0, 0);
    Partition_image (vdv);
    Parallelize_partition ();
    M_proc_partition (vdv, 1);

    vdv->rt_state = RT_COMPLETED;
    for (i = 0; i < N_parts; i++)
	free (Parts[i].inp);
    return (0);
}

/********************************************************************

    Outputs data of partitions that are completed.

*********************************************************************/

static void Output_data (Vdeal_t *vdv) {
    int i, processed, cnt;

    /* check dependent parts */
    for (i = 0; i < N_parts; i++) {
	int k;
	if (Parts[i].state != P_DONE)
	    continue;
	for (k = 0; k < 2; k++) {
	    if (Parts[i].depend[k] >= 0 && 
				Parts[Parts[i].depend[k]].state < P_DONE)
	    break;
	}
	if (k >= 2) {
	    Post_process_partition (vdv, i);
	    Parts[i].state = P_READY;
	}
    }

    processed = vdv->rt_done;
    cnt = 0;
    while (1) {
	for (i = 0; i < N_parts; i++) {
	    if (Parts[i].ys == processed + cnt)
		break;
	}
	if (i >= N_parts)
	    break;
	if (Parts[i].state != P_READY)
	    break;
	Parts[i].state = P_OUT;
	cnt += Parts[i].yz;
    }

    if (cnt > 0) {
	Mutex_lock ();
	VDE_generate_ewm (vdv);
	Process_saved_regions (vdv, processed, cnt);
	Process_unprocessed_gates (vdv, processed, cnt);
	Remove_fill (vdv, processed, cnt);
	VDA_detect_hs_features (vdv, processed, cnt);
	Put_back_gcc_gates (vdv, processed, cnt);
	vdv->rt_processed += cnt;
	VDR_output_processed_radial (vdv);
	Mutex_unlock ();
    }
}

/********************************************************************

    Processes a ppi image.

*********************************************************************/

int VDD_process_image (Vdeal_t *vdv) {
    int i, yz;

    if (VDD_init_vdv (vdv) != 0)
	return (-1);

    yz = vdv->yz;
    for (i = 0; i < yz; i++) {
	double azi;
	azi = vdv->start_azi + (double)i * vdv->gate_width;
	while (azi > 360.)
	    azi -= 360.;
	vdv->ew_aind[i] = VDE_get_azi_ind (vdv, azi);
	vdv->ew_azi[i] = Myround (azi * 10.);
    }

    PP_preprocessing_v (vdv, 0, vdv->yz);

    if (EE_estimate_ew (vdv) == 0 &&
	EE_get_eew (vdv) < 0)
	return (-1);

    Archive_region (AR_RESET, NULL, 0, 0);
    Partition_image (vdv);

    Parallelize_partition ();
    M_proc_partition (vdv, 1);

    for (i = 0; i < N_parts; i++)
	Post_process_partition (vdv, i);

    VDE_generate_ewm (vdv);
    Process_saved_regions (vdv, 0, vdv->yz);
    Process_unprocessed_gates (vdv, 0, vdv->yz);
    Remove_fill (vdv, 0, vdv->yz);
    VDA_detect_hs_features (vdv, 0, vdv->yz);
    Put_back_gcc_gates (vdv, 0, vdv->yz);

    return (0);
}

/********************************************************************

    Processes partition of index "pind".

*********************************************************************/

enum {		/* processing step type */
    ORIG_REGS,	/* step 0 - Original regions are processed */
    SPW_REGS,	/* step 1 - spw marsked regions processed */
    BH_REGS, 	/* _BH regions, set by _2d_dealiase, This may take 
		    several steps until there is no such ones. */
    BE_REGS		/* _BE regions, set by check_global_deal, are
		    processed in the final step. */
};

static void Process_partition (Vdeal_t *vdv, Part_t *part) {
    extern __thread char *VDC_region_buf;
    int xz, pys, pyz, exyz, step, cnt;
    Region_t region;
    Params_t parms;
    int small_rgz;

    xz = vdv->xz;
    pys = part->ys;		/* partition start y */
    pyz = part->yz;		/* partition size */
    exyz = part->eyz;		/* extended partition size */
    if (Test_mode)
	VDD_log ("Partition: ys %d, yz %d (yoff %d eyz %d)\n", 
				pys, pyz, part->yoff, exyz);

    if (vdv->low_prf) {		/*  parallel processing is not needed */
	Mutex_lock ();
	VDE_ew_deal_area (vdv, 0, pys, xz, pyz);
	Mutex_unlock ();
	return;
    }

    VDC_region_buf = STR_reset (VDC_region_buf, xz * exyz * sizeof (short));
    region.data = (short *)VDC_region_buf;

    VDD_set_parameters (vdv, &parms, part->nyq);
    parms.stride = xz;
    parms.fppi = 0;
    parms.nyq = part->nyq;

    small_rgz = .4 * vdv->g_size / vdv->gate_width;
		/* small region size. 200 for super res */

    step = ORIG_REGS;
    cnt = -1;
    while (1) {				/* go through the steps */
	Data_filter_t cdf;
	int n_regs, i;

	cdf.exc_bits = DMAP_NPRCD;
	if (step == ORIG_REGS)
	    cdf.exc_bits = DMAP_NPRCD | DMAP_HSPW;
	else if (step == SPW_REGS)
	    cdf.exc_bits = DMAP_NPRCD | DMAP_NHSPW;
	cdf.yes_bits = 0;
	if (step == BH_REGS)
	    cdf.yes_bits = DMAP_BH;
	else if (step == BE_REGS)
	    cdf.yes_bits = DMAP_BE;

	cdf.map = part->dmap;
	if (step <= SPW_REGS) {
	    if (step == ORIG_REGS)
	        Rm_thin_conn (part->inp, &cdf, &parms,
					xz, exyz, part->dmap, DMAP_NPRCD);
	    VDA_remove_single_gate_conn (part->inp, xz,
			    		exyz, &cdf, part->dmap, DMAP_NPRCD);
	    Copy_dmap (vdv, part, NULL);	/* copy NPRCD */
	}

	if (step == BE_REGS) {
	    Region_t rg;
	    rg.xs = 0;  rg.ys = pys;  rg.xz = xz;  rg.yz = pyz;
	    rg.data = vdv->out + pys * xz;
	    Mutex_lock ();
	    VDE_check_global_deal (vdv, &rg, 0);
	    Mutex_unlock ();
	}

	if (step <= SPW_REGS) {
	    cdf.map = part->dmap;
	    n_regs = VDC_identify_regions (part->inp, &cdf, xz,
		    0, 0, xz, exyz, &parms, VDC_IDR_SIZE, NULL);
	}
	else {
	    cdf.map = vdv->dmap;
	    n_regs = VDC_identify_regions (vdv->inp, &cdf, xz,
		    0, pys, xz, pyz, &parms, VDC_IDR_SIZE, NULL);
	}

	if (step == BH_REGS)
	    Clear_area_dmap (vdv, pys, pyz, DMAP_BE | DMAP_BH);

	for (i = 0; i < n_regs; i++) {
	    int n_gs, gd;

	    n_gs = VDC_get_next_region (NULL, VDC_NEXT, &region);
	    if (n_gs < 0)
		break;
	    if (step == BH_REGS && n_gs <= small_rgz)
		continue;

	    parms.r0 = region.xs;	/* set region dependent params */
	    parms.xs = region.xs;
	    parms.ys = region.ys + part->ys - part->yoff;
	    parms.use_bc = 0;
	    if (step == ORIG_REGS)
		parms.use_bc = 1;
	    if (step <= SPW_REGS)
		parms.dmap = part->dmap + region.ys * xz + region.xs;
	    else
		parms.dmap = vdv->dmap + region.ys * xz + region.xs;
	    VD2D_2d_dealiase (region.data, &parms, n_gs,
					region.xz, region.yz, NULL);

	    if (step <= SPW_REGS) {
		n_gs -= Remove_overlapped_data (&region, part->yoff, 
							part->yoff + pyz);
		if (n_gs <= 0 || region.yz <= 0)
		    continue;
		Copy_dmap (vdv, part, &region);		/* copy BH */
		region.ys = region.ys + pys - part->yoff;
	    }

	    Mutex_lock ();
	    gd = Global_dealiase (vdv, &region, step, n_gs, small_rgz, part);
	    if (gd == SNO_DATA) {
		Clear_region_dmap (vdv, &region);
		Mutex_unlock ();
		continue;	/* not re-processed */
	    }

	    Correct_extreme (vdv, &region, gd, part->nyq);

	    VDD_apply_gd_copy_to_out (vdv, &region, gd);
	    Mutex_unlock ();
	}

	if (step == ORIG_REGS) {
	    if (vdv->data_type & DT_HSPW_SET)
		step = SPW_REGS;
	    else
		step = BH_REGS;
	}
	else if (step == SPW_REGS)
	    step = BH_REGS;
	else if (step == BH_REGS) {
	    if (n_regs <= 0)
		step = BE_REGS;
	}
	else		/* step == BE_REGS */
	    break;	/* recursive _BE processing may go infinity */

	if (No_reprocess && step >= BH_REGS) break;
    }
    VDC_free (NULL);

    Mutex_lock ();
    if (!((vdv->data_type & DT_VH_VS) && vdv->elev < 3.f))
	Update_ew_data (vdv, pys, pyz);
    Mutex_unlock ();
    return;
}

/****************************************************************************

    Update ew data using vdv->out in partition pys, pyz.

****************************************************************************/

static void Update_ew_data (Vdeal_t *vdv, int pys, int pyz) {
    Region_t reg;

    reg.xs = 0;
    reg.ys = pys;
    reg.xz = vdv->xz;
    reg.yz = pyz;
    reg.data = vdv->out + pys * vdv->xz;
    VDE_update_ew_data (vdv, &reg);
}

/****************************************************************************

    Breaks thin connections in "inp" of size xz by yz. cdf is the data filter.
    The break gates is flagged in dmap with bit m_bf. This function removes
    thin conns generated by identify_regions due to parms (depending on 2d
    threshold setting). This function assumes inp is not of full ppi.

****************************************************************************/

static void Rm_thin_conn (unsigned char *inp, Data_filter_t *cdf,
			Params_t *parms, int xz, int yz, unsigned char *dmap, 
					unsigned char m_bf) {
    void *rgs;
    int xyz, nrgs;
    unsigned char *buf, *bufc;
    Region_t region;

    xyz = xz * yz;
    buf = (unsigned char *)MISC_malloc (xyz * 2 * sizeof (unsigned char));

    rgs = NULL;
    nrgs = VDC_identify_regions (inp, cdf, xz, 0, 0,
				xz, yz, parms, VDC_IDR_SIZE, &rgs);
    if (nrgs <= 0) {
	VDC_free (rgs);
	free (buf);
	return;
    }

    bufc = buf + xyz * sizeof (unsigned char);
    region.data = (short *)buf;
    while (1) {
	int x, y;
	int n_gs = VDC_get_next_region (rgs, VDC_BIN | VDC_NEXT, &region);
	if (n_gs <= 8)
	    break;
	memcpy (bufc, buf, region.xz * region.yz * sizeof (unsigned char));
	VDA_find_thin_conn (buf, region.xz, region.yz, 1, 0, NULL, 
				parms->nyq, parms->data_off, NULL, 0);
    
	for (y = 0; y < region.yz; y++) {
	    unsigned char *s, *r, *map;
	    map = dmap + ((y + region.ys) * xz + region.xs);
	    r = bufc + y * region.xz;
	    s = buf + y * region.xz;
	    for (x = 0; x < region.xz; x++) {
		if (r[x] != BNO_DATA && s[x] == BNO_DATA)
		    map[x] |= m_bf;
	    }
	}
    }
    VDC_free (rgs);
    free (buf);
}

/**********************************************************************

    Returns global dealiasing value for "region". Both EW dealiasing and
    border dealiasing results are used to generate a combined global
    dealiasing value.

***********************************************************************/

static int Global_dealiase (Vdeal_t *vdv, Region_t *region,
			int step, int n_gs, int small_rgz, Part_t *part) {
    int nyq, qerr, ewdiff, bddiff, conn, q, diff, bcnt;
    double r;

    nyq = part->nyq;
    if (step <= SPW_REGS) {
	int gd = VDE_global_dealiase (vdv, region, vdv->dmap, vdv->xz, &qerr);
	if (qerr * 2 <= nyq || n_gs >= small_rgz * 2)
	    return (gd);
	/* small area, bad fitting, will redo global dealiasing */
	if (region->xz == 1 && region->yz == 1)
	    vdv->dmap[region->ys * vdv->xz + region->xs] |= DMAP_NPRCD;
	else if (Archive_region (AR_SAVE, region, 0, 0) == 0)
	    return (gd);
	return (SNO_DATA);
    }

    bddiff = VDA_border_dealiase (vdv, region, part, &conn, &bcnt);
    q = VDE_quantize_gd (nyq, bddiff, &qerr);
    if (qerr < nyq && (conn >= 85 || (conn >= 70 && n_gs < small_rgz)))
	return (SNO_DATA);		/* keep tornado like features */
    ewdiff = VDE_global_dealiase (vdv, region, vdv->dmap, vdv->xz, NULL);

    q = VDE_quantize_gd (nyq, ewdiff, &qerr);
    r = 2. * (1. - qerr / nyq);
    if (r > 1.)		/* r is small when ew deal quality is low */
	r = 1.;		/* r is 1 when gd error < half Nyq */
    if (r < 0.)		/* r is approaching 0 when gd error >= half Nyq */
	r = 0.;

    diff = Myround ((r * ewdiff * (100 - conn) + bddiff * conn) /
						(r * (100 - conn) + conn));
    q = VDE_quantize_gd (nyq, diff, &qerr);
    if (qerr * 3 > nyq * 2 && !Is_high_wind_region (vdv, region) && 
						n_gs < small_rgz * 4)
	return (SNO_DATA);

    return (q);
}

/***************************************************************************

    Returns true if part of the region is in high wind range.

***************************************************************************/

static int Is_high_wind_region (Vdeal_t *vdv, Region_t *region) {
    int st, end, x;
    Ew_struct_t *ew = &(vdv->ew);
    st = region->xs / ew->rz;
    end = (region->xs + region->xz - 1) / ew->rz;
    for (x = 0; x <= end; x++) {
	if (ew->rfs[x] & RF_HIGH_WIND)
	    return (1);
    }
    return (0);
}

/**********************************************************************

    Removes filled-in data from vdv->out in the partition of (ys, yz).

***********************************************************************/

static void Remove_fill (Vdeal_t *vdv, int ys, int yz) {
    int xz, y, x;
    xz = vdv->xz;

    for (y = ys; y < ys + yz; y++) {
	unsigned char *map;
	short *out;
	map = vdv->dmap + y * xz;
	out = vdv->out + y * xz;
	for (x = 0; x < xz; x++) {
	    if (map[x] & DMAP_FILL) {
		out[x] = SNO_DATA;
	    }		
	}
    }
}

/********************************************************************

   Clears "cflags" bits in dmap in the area of ys, yz.

*********************************************************************/

static void Clear_area_dmap (Vdeal_t *vdv, int ys, int yz, int cflags) {
    int y;
    unsigned char *map, mask;

    mask = ~cflags;
    for (y = 0; y < yz; y++) {
	int x;
	map = vdv->dmap + (y + ys) * vdv->xz;
	for (x = 0; x < vdv->xz; x++) {
	    map[x] &= mask;
	}
    }
}

/***************************************************************************

    Copies dmap from partition dmap to the global dmap.

***************************************************************************/

static void Copy_dmap (Vdeal_t *vdv, Part_t *part, Region_t *reg) {
    int y, x, xz;
    unsigned char flags, *s, *d;

    xz = vdv->xz;
    flags = DMAP_BH | DMAP_NPRCD;
    if (reg == NULL) {
	for (y = 0; y < part->yz; y++) {
	    s = part->dmap + (y + part->yoff) * xz;
	    d = vdv->dmap + (y + part->ys) * xz;
	    for (x = 0; x < xz; x++) {
		if (s[x] & flags)
		    d[x] |= s[x] & flags;
	    }
	}
    }
    else {
	for (y = 0; y < reg->yz; y++) {    
	    s = part->dmap + (y + reg->ys) * xz + reg->xs;
	    d = vdv->dmap + ((y + reg->ys + part->ys - part->yoff) % vdv->yz) *
							xz + reg->xs;
	    for (x = 0; x < reg->xz; x++) {
		if (s[x] & flags)
		    d[x] |= s[x] & flags;
	    }
	}
    }
}

/********************************************************************

    Clears dmap in region area.

********************************************************************/

static void Clear_region_dmap (Vdeal_t *vdv, Region_t *region) {
    int x, y;
    unsigned char *dmap, mask;

    mask = ~(DMAP_BH | DMAP_BE);
    for (y = 0; y < region->yz; y++) {
	short *in = region->data + y * region->xz;
	dmap = vdv->dmap + ((y + region->ys) % vdv->yz) * vdv->xz + region->xs;
	for (x = 0; x < region->xz; x++) {
	    if (in[x] != SNO_DATA)
		dmap[x] &= mask;
	}
    }
}

/********************************************************************

    Removes data points in "region" that are in the overlapped area 
    (outside of area of yz lines started at y_st). Returns the number 
    of points removed.

********************************************************************/

static int Remove_overlapped_data (Region_t *region, int y_st, int yz) {
    int cnt, y;

    cnt = 0;
    for (y = 0; y < region->yz; y++) {
	short *dp;
	int yo, x;

	yo = region->ys + y;
	if (yo >= y_st && yo < y_st + yz)
	    continue;

	dp = region->data + y * region->xz;
	for (x = 0; x < region->xz; x++) {
	    if (dp[x] != SNO_DATA)
		cnt++;
	}
    }
    if (region->ys < y_st) {
	int t = y_st - region->ys;
	region->ys += t;
	region->yz -= t;
	region->data += t * region->xz;
    }
    if (region->ys + region->yz > yz) {
	region->yz = yz - region->ys;
    }
    if (region->yz < 0)
	region->yz = 0;
    region->n_gs -= cnt;
    return (cnt);
}

/********************************************************************

    Performs global dealiasing for all saved regions in partition
    ys, yz.

********************************************************************/

static void Process_saved_regions (Vdeal_t *vdv, int ys, int yz) {
    Region_t reg;

    Archive_region (AR_REWIND, NULL, 0, 0);
    while (Archive_region (AR_READ, &reg, ys, yz)) {
	int gd, qerr, nyq;
	gd = VDE_global_dealiase (vdv, &reg, vdv->dmap, vdv->xz, &qerr);
	nyq = VDD_get_nyq (vdv, reg.ys);
	Correct_extreme (vdv, &reg, gd, nyq);
	VDD_apply_gd_copy_to_out (vdv, &reg, gd);
    }
}

/********************************************************************

    Applies the global dealiasing value "gd" to "region" and copies 
    it to the output buffer.

********************************************************************/

int VDD_apply_gd_copy_to_out (Vdeal_t *vdv, Region_t *region, int gd) {
    int x, y, yz;

    yz = vdv->yz;
    for (y = 0; y < region->yz; y++) {
	short *in, *out;
	in = region->data + y * region->xz;
	out = vdv->out + ((y + region->ys) % yz) * vdv->xz + region->xs;
	for (x = 0; x < region->xz; x++) {
	    short t = in[x];
	    if (t != SNO_DATA) {
		out[x] = t + gd;
		in[x] = t + gd;
	    }
	}
    }
    return (0);
}

/* global data used by Fix_part_data - no need to be threaded because of 
   locked use */
static int FPD_depth, FPD_nyq, FPD_ys, FPD_yz, FPD_rz;
static short FPD_min[MAX_EW_NRS], FPD_max[MAX_EW_NRS];

static void Fix_part_data (Vdeal_t *vdv, int x, int y) {
    short e, v;
    int thr;

    if (FPD_depth > 2000 || No_reprocess)
	return;
    FPD_depth++;

    thr = FPD_nyq;
    e = EE_get_eew_value (x, y);
    if (e == SNO_DATA)
	e = VDE_get_ew_value (vdv, x, y);
    if (e != SNO_DATA) {
	int df;
	short *out = vdv->out + (y * vdv->xz + x);

	v = *out;
	if (v == SNO_DATA)
	    return;
	df = v - e;
	if (df < -thr && *out + FPD_nyq * 2 < FPD_max[x / FPD_rz]) {
	    *out += FPD_nyq * 2;
	}
	else if (df > thr && *out - FPD_nyq * 2 > FPD_min[x / FPD_rz]) {
	    *out -= FPD_nyq * 2;
	}
	else {
	    FPD_depth--;
	    return;
	}
	if (y > FPD_ys)
	    Fix_part_data (vdv, x, y - 1);
	if (y < FPD_ys + FPD_yz - 1)
	    Fix_part_data (vdv, x, y + 1);
	if (x > 0)
	    Fix_part_data (vdv, x - 1, y);
	if (x < vdv->xz - 1)
	    Fix_part_data (vdv, x + 1, y);
    }  
    FPD_depth--;
}

/***************************************************************************

    We checks the partition border against the neighboring partition. If there
    is very high shear along the border, it is likely one of the partions is
    not dealiased correctly. We then apply a ew dealiasing in the area.

***************************************************************************/

#define MAX_FCNT 1024

static void Post_process_partition (Vdeal_t *vdv, int ptind) {
    Point_t fp[MAX_FCNT];
    int fcnt;

    if (vdv->low_prf)
	return;
    fcnt = VDA_check_failed_gates (vdv, Parts, ptind, fp, MAX_FCNT);

    if (fcnt > 0 && (vdv->data_type & DT_VH_VS) && vdv->elev < 3.f) {
	Part_t *part = Parts + ptind;	/* redo phase 2 */
	int l_prf = vdv->low_prf;
	Mutex_lock ();
	vdv->low_prf = 1;	/* to generate a full ewm map */
	VDE_generate_ewm (vdv);
	VDE_ew_deal_area (vdv, 0, part->ys, vdv->xz, part->yz);
	vdv->low_prf = l_prf;	/* restore the ewm */
	VDE_generate_ewm (vdv);
	Mutex_unlock ();
	return;
    }

    if (fcnt > 0) {
	int i;
	Part_t *part = Parts + ptind;
	Ew_struct_t *ew = &(vdv->ew);
	Mutex_lock ();
	FPD_depth = 0;
	FPD_nyq = part->nyq;
	FPD_ys = part->ys;
	FPD_yz = part->yz;
	Set_fpd_min_max (vdv);
	for (i = 0; i < fcnt; i++) {
	    int rfs, rf_vhvs;
	    rfs = ew->rfs[fp[i].x / ew->rz];
	    rf_vhvs = RF_LOW_VS | RF_HIGH_VS;	/* very high vertical shear */
	    if ((rfs & RF_HIGH_WIND) || ((rfs & rf_vhvs) == rf_vhvs))
		Fix_part_data (vdv, fp[i].x , fp[i].y);
	}
	if (Test_mode)
	    VDD_log ("VHS detected (ys %d) %d points\n", part->ys, fcnt);
	Mutex_unlock ();
    }
}

/**************************************************************************

    Sets FPD_min and FPD_max.

**************************************************************************/

static void Set_fpd_min_max (Vdeal_t *vdv) {
    int x;

    Ew_struct_t *ew = &(vdv->ew);
    FPD_rz = ew->rz;
    for (x = 0; x < ew->n_rgs; x++) {
	short *ev, min, max;
	int y;
	ev = ew->ewm + x;
	min = max = *ev;
	for (y = 0; y < ew->n_azs; y++) {
	    if (*ev > max)
		max = *ev;
	    else if (*ev < min)
		min = *ev;
	    ev += ew->n_rgs;
	}
	FPD_min[x] = min - FPD_nyq / 2;
	FPD_max[x] = max + FPD_nyq / 2;
    }
}

/**************************************************************************

    Recusively travels through all pixels in the region and finds out the 
    extreme values.

**************************************************************************/

static int Ev_depth, Ev_cnt, Ev_xz, Ev_yz, Ev_thrh,
			Ev_thrl, Ev_gd, Ev_ncnt, Ev_thrd, Ev_gcnt, Ev_bcnt;
static short *Ev_data;
static unsigned char *Ev_map;
#define EV_MAX_CNT 256
static Point_t Evs[EV_MAX_CNT];

static void Travel_ev_data (int x, int y, short pv) {
    short dt;

    if (Ev_depth > 20000 || Ev_cnt >= EV_MAX_CNT)
	return;
    Ev_depth++;
    dt = Ev_data[y * Ev_xz + x];
    if (dt == SNO_DATA)
	Ev_ncnt++;
    else if (!(Ev_map[y * Ev_xz + x] & DMAP_LOCAL)) {
	if ((dt + Ev_gd > Ev_thrh || dt + Ev_gd < Ev_thrl)) {

	    Ev_map[y * Ev_xz + x] |= DMAP_LOCAL;
	    Evs[Ev_cnt].x = x;
	    Evs[Ev_cnt].y = y;
	    Ev_cnt++;
	    if (x > 0)
		Travel_ev_data (x - 1, y, dt);
	    if (y > 0)
		Travel_ev_data (x, y - 1, dt);
	    if (x < Ev_xz - 1)
		Travel_ev_data (x + 1, y, dt);
	    if (y < Ev_yz - 1)
		Travel_ev_data (x, y + 1, dt);
	}
	else {
	    int df = dt - pv;
	    if (df < 0)
		df = -df;
	    if (df >= Ev_thrd) {
		Ev_bcnt++;
	    }
	    else
		Ev_gcnt++;
	}
    }
    Ev_depth--;
}

/********************************************************************

    Applies the global dealiasing value "gd" to "region" and checks
    against thr. Return 1 if there are high extream values, -1 if there
    are low extream values or 0 otherwise.

********************************************************************/

static int Correct_extreme (Vdeal_t *vdv, Region_t *region, int gd, int nyq) {
    int x, y, tcnt, nyq2, thr;

	return (0);	/* Thus function is disabled because it affects 				   tornado area. We will put this back when we have
			   a tornado detection function. */

    Ev_thrh = vdv->data_off + 2 * nyq;
    Ev_thrl = vdv->data_off - 2 * nyq;
    Ev_xz = region->xz;
    Ev_yz = region->yz;
    Ev_map = vdv->dmap;		/* use this locally */
    Ev_data = region->data;
    Ev_thrd = nyq / 2;
    Ev_gd = gd;
    nyq2 = nyq * 2;
    thr = nyq / 2;
    tcnt = 0;
    for (y = 0; y < region->yz; y++) {
	short *dt = region->data + y * region->xz;
	unsigned char *dm = vdv->dmap + y * region->xz;
	for (x = 0; x < region->xz; x++) {
	    if (dt[x] != SNO_DATA && !(dm[x] & DMAP_LOCAL)) {
		if (dt[x] + gd > Ev_thrh || dt[x] + gd < Ev_thrl) {
		    Ev_cnt = Ev_depth = Ev_ncnt = Ev_gcnt = Ev_bcnt = 0;
		    Travel_ev_data (x, y, dt[x]);
		    tcnt += Ev_cnt;
		    if (Ev_bcnt > Ev_gcnt ||
					Ev_ncnt + Ev_bcnt > 3 * Ev_gcnt / 2) {
			int k;
			for (k = 0; k < Ev_cnt; k++) {
			    short *dp = region->data + 
					(Evs[k].y * region->xz + Evs[k].x);
			    int ew = VDE_get_ew_value (vdv, x + region->xs,
							y + region->ys);
			    int v = *dp + gd;
			    if (ew == SNO_DATA)
				continue;
			    if (v > Ev_thrh && v - ew > thr)
				*dp -= nyq2;
			    if (v < Ev_thrh && v - ew < -thr)
				*dp += nyq2;
			}
		    }
		}
	    }
	}
    }
    if (tcnt > 0) {
	for (y = 0; y < region->yz; y++) {
	    unsigned char *dm = vdv->dmap + y * region->xz;
	    for (x = 0; x < region->xz; x++) {
		if (dm[x] & DMAP_LOCAL) {
		    dm[x] &= ~DMAP_LOCAL;
		}
	    }
	}
    }
    return (0);
}

/********************************************************************

    Processes gates that are not yet processed (marked by DMAP_NPRCD).
    The flag is then cleared.

*********************************************************************/

static void Process_unprocessed_gates (Vdeal_t *vdv, int ys, int yz) {
    int y, x, xz;

    xz = vdv->xz;
    for (y = ys; y < ys + yz; y++) {
	unsigned char *cr, *map;
	short *out, v;
	Region_t reg;

	/* process NPRCD gates */
	cr = vdv->inp + y * xz;
	out = vdv->out + y * xz;
	map = vdv->dmap + y * xz;
	reg.ys = y;
	reg.xz = 1;
	reg.yz = 1;
	reg.n_gs = 1;
	reg.data = &v;
	for (x = 0; x < xz; x++) {
	    if (map[x] & DMAP_NPRCD) {
		if (cr[x] != BNO_DATA) {
		    int qerr;
		    reg.xs = x;
		    v = cr[x];
		    v += VDE_global_dealiase (vdv, &reg, NULL, 0, &qerr);
		    out[x] = v;
		}
	    }
	}
    }
}

/*************************************************************************

    Returns the nyquist velovity for azi ys.

*************************************************************************/

int VDD_get_nyq (Vdeal_t *vdv, int ys) {
    int y, i;
    if (vdv->n_secs <= 1)
	return (vdv->nyq);
    y = ys % vdv->yz;
    for (i = 0; i < vdv->n_secs; i++) {
	Prf_sec_t *s = vdv->secs + i;
	if (y < s->azi + s->size)
	    return (s->nyq);
    }
    return (vdv->nyq);	/* should never happen */
}

/********************************************************************

    Puts GCC gates in the partition of (ys, yz) back to the output.

*********************************************************************/

static void Put_back_gcc_gates (Vdeal_t *vdv, int ys, int yz) {
    int cnt, i, xz, data_off;
    Gate_t *gccgs;

    cnt = CD_get_saved_gcc_gate (&gccgs);
    xz = vdv->xz;
    data_off = vdv->data_off;
    for (i = 0; i < cnt; i++) {
	short v;
	Region_t reg;
	int x, y, qerr, nyq, gd, off, type;

	x = gccgs->x;
	y = gccgs->y;
	v = gccgs->v;
	type = gccgs->type;
	gccgs++;
	if (x < 0 || x >= xz)
	    continue;
	if (y < ys || y >= ys + yz)
	    continue;

	if (VDA_b16_update1 && type == GATE_ISOLATE) {
	    int c, yy, cv, sum;
	    c = sum = 0;
	    yy = y - ys;	/* get neighbor values */
	    if (yy > 0 && 
		(cv = vdv->out[(yy - 1) * xz + x]) != SNO_DATA) {
		sum += cv;
		c++;
	    }
	    if (yy < yz - 1 &&
		(cv = vdv->out[(yy + 1) * xz + x]) != SNO_DATA) {
		sum += cv;
		c++;
	    }
	    if (x > 0 &&
		(cv = vdv->out[yy * xz + x - 1]) != SNO_DATA) {
		sum += cv;
		c++;
	    }
	    if (x < xz - 1 &&
		(cv = vdv->out[yy * xz + x + 1]) != SNO_DATA) {
		sum += cv;
		c++;
	    }
	    if (c > 0) {
		sum = sum / c;	/* average neighbor value */
		nyq = VDD_get_nyq (vdv, y);
		while (v > sum + nyq)
		    v -= nyq * 2;
		while (v < sum - nyq)
		    v += nyq * 2;
		vdv->out[y * xz + x] = v;
		continue;
	    }
	}

	reg.xs = x;
	reg.ys = y;
	reg.xz = 1;
	reg.yz = 1;
	reg.data = &v;
	gd = VDE_global_dealiase (vdv, &reg, NULL, 0, &qerr);
	off = y * xz + x;
	if (qerr * 4 < VDD_get_nyq (vdv, y))
	    vdv->out[off] = v + gd;
	else
	    vdv->out[off] = v;
    }
}

/******************************************************************

    Initializes vdeal struct "vdv". This must be called each 
    elevation. "vdv" must be initialized to 0 before the first
    time calling this.

******************************************************************/

int VDD_init_vdv (Vdeal_t *vdv) {
    int i, size;
    short *sp;

    size = vdv->xz * vdv->yz;
    if (vdv->out != NULL)
	free (vdv->out);
    vdv->out = (short *)MISC_malloc (size * sizeof (short));
    sp = vdv->out;
    for (i = 0; i < size; i++)
	*sp++ = SNO_DATA;
    if (vdv->dmap != NULL)
	free (vdv->dmap);
    vdv->dmap = (unsigned char *)MISC_malloc (size * sizeof (unsigned char));
    memset (vdv->dmap, 0, size * sizeof (unsigned char));

    if (vdv->ew_azi != NULL)
	free (vdv->ew_azi);
    vdv->ew_azi = (unsigned short *)MISC_malloc 
	    (vdv->yz * (sizeof (unsigned short) + sizeof (unsigned char)));
    vdv->ew_aind = (unsigned char *)(vdv->ew_azi + vdv->yz);
    memset (vdv->ew_aind, 0, vdv->yz * sizeof (unsigned char));

    vdv->low_prf = 0;
    if (vdv->nyq < Myround (15 * vdv->data_scale))
	vdv->low_prf = 1;

    N_parts = 0;
    Parts = (Part_t *)STR_reset ((char *)Parts, 12 * sizeof (Part_t));

    if (VDE_initialize (vdv) != 0)
	return (-1);

    return (0);
}

/******************************************************************

    Sets algorithm parameters.

******************************************************************/

void VDD_set_parameters (Vdeal_t *vdv, Params_t *parms, int nyq) {

    memset (parms, 0, sizeof (Params_t));
    if (vdv->low_prf) {
	parms->max_shear = .4 * nyq;
	parms->am_shear = .4 * nyq;
    }
    else if (vdv->phase == 1) {
	parms->max_shear = .5 * nyq;
	parms->am_shear = .4 * nyq;
    }
    else {
	parms->max_shear = .8 * nyq;
	parms->am_shear = .6 * nyq;
    }

    if (vdv->phase == 1) {
	parms->weight_factor = .4f;
	if (nyq <= 45)
	    parms->weight_factor = .3f;
	if (nyq <= 30)
	    parms->weight_factor = .2f;
    }
    else {
	parms->weight_factor = .1f;
    }

    parms->has_weight = 1.f;	/* disable shear-based azi weighting */
    if (VDA_b16_update1 && vdv->phase == 2 && !vdv->low_prf)
	parms->has_weight = .1f;
			/* turn on shear-based azi weight for tornado */

    parms->use_bc = 0;
    parms->tp_quant = 0;
    parms->strict_bh = 0;
    if (vdv->phase == 1)
	parms->strict_bh = 1;
    if (vdv->low_prf) {
	if (vdv->phase == 1)
	    parms->bh_thr = .5f;
	else
	    parms->bh_thr = .5f;
    }
    else {
	parms->use_bc = 1;
	if (vdv->phase == 1)
	    parms->bh_thr = .3f;
	else
	    parms->bh_thr = .2f;
    }

    parms->small_gc = 10;
    if (vdv->gate_width > .6f)
	parms->small_gc = 7;
    if (vdv->phase == 1)
	parms->small_gc /= 3;

    parms->data_off = vdv->data_off;
    parms->nyq = nyq;
    parms->r_w_ratio = 2. * sin (.5 * vdv->gate_width * deg2rad) * 4;
		/* The above last value adds a higher weight on range over
		   azimuth */
    parms->r0 = 0;
    parms->g_size = vdv->g_size;
    parms->data_scale = vdv->data_scale;
    parms->xr = parms->yr = 1;
    parms->vdv = vdv;
}

/********************************************************************

    Partitions the image.

*********************************************************************/

static void Partition_image (Vdeal_t *vdv) {
    int y, cnt, xz, yz, fp, overlap_width, i;

    xz = vdv->xz;
    yz = vdv->yz;
    fp = vdv->full_ppi;
    cnt = 0;
    y = 0;
    for (i = 0; i < vdv->n_secs; i++) {
	int ps, pz, w, n;
	Prf_sec_t *secs = vdv->secs + i;

	ps = secs->azi;
	pz = secs->azi + secs->size;

	w = Max_partition_size;
	n = (pz - ps - 1) / w + 1;
	while ((pz - ps - 1) / w + 1 == n && w > 1)
	    w--;
	w++;
	overlap_width = OVERLAP_SIZE;

	while (y < pz) {
	    Part_t p;
    
	    p.depend[0] = -1;
	    p.depend[1] = -1;
	    p.ys = y;
	    p.yz = w;
	    if (p.ys + p.yz > pz)
		p.yz = pz - p.ys;
	    p.state = P_WAIT;
	    p.nyq = secs->nyq;
    
	    p.yoff = overlap_width;
	    p.eyz = p.yz + 2 * overlap_width;
	    if ((cnt == 0 && !fp) || (cnt > 0 && p.ys == ps)) {
		p.yoff = 0;
		p.eyz -= overlap_width;
	    }
	    if ((p.ys + p.yz == pz && p.ys + p.yz < yz) || 
				(!fp && p.ys + p.yz == yz))
		p.eyz -= overlap_width;
   
	    y += p.yz;
	    Parts = (Part_t *)STR_append ((char *)Parts, (char *)&p, 
						    sizeof (Part_t));
	    cnt++;
	}
    }

    /* copy data to partitions */
    for (i = 0; i < cnt; i++) {
	int size, sy, ycnt, dy, n;

	Part_t *p = Parts + i;
	size = p->eyz * xz * sizeof (unsigned char);
	p->inp = (unsigned char *)MISC_malloc (2 * size);
	p->dmap = p->inp + size;

	sy = p->ys - p->yoff;
	ycnt = p->eyz;
	dy = 0;
	if (sy < 0) {
	    memcpy (p->inp, vdv->inp + (sy + yz) * xz, 
				-sy * xz * sizeof (unsigned char));
	    memcpy (p->dmap, vdv->dmap + (sy + yz) * xz, 
				-sy * xz * sizeof (unsigned char));
	    ycnt -= -sy;
	    dy += -sy;
	    sy = 0;
	}
	n = ycnt;
	if (sy + n > yz)
	    n = yz - sy;
	memcpy (p->inp + dy * xz, vdv->inp + sy * xz, 
				n * xz * sizeof (unsigned char));
	memcpy (p->dmap + dy * xz, vdv->dmap + sy * xz, 
				n * xz * sizeof (unsigned char));
	dy += n;
	sy = (sy + n + yz) % yz;
	ycnt -= n;
	if (ycnt > 0) {
	    memcpy (p->inp + dy * xz, vdv->inp + sy * xz, 
				ycnt * xz * sizeof (unsigned char));
	    memcpy (p->dmap + dy * xz, vdv->dmap + sy * xz, 
				ycnt * xz * sizeof (unsigned char));
	}
    }

    if (vdv->full_ppi) {	/* make the last the first */
	Part_t tmp = Parts[cnt - 1];
	for (i = cnt - 1; i > 0; i--)
	    Parts[i] = Parts[i - 1];
	Parts[0] = tmp;
    }

    /* set dependent parts */
    for (i = 0; i < cnt; i++) {
	int ind = i - 1;
	if (ind < 0 && vdv->full_ppi)
	    ind += cnt;
	Parts[i].depend[0] = ind;
	ind = i + 1;
	if (ind >= cnt) {
	    if (vdv->full_ppi)
		ind -= cnt;
	    else
		ind = -1;
	}
	Parts[i].depend[1] = ind;
    }

    N_parts = cnt;
}

/********************************************************************

    Saves and retrieves regions for post processing. We use a fixed-
    sized buffer here for simplicity. In case of buffer full, extra
    regions are simply not saved and reprocessed later.

********************************************************************/

static int Archive_region (int func, Region_t *reg, int ys, int yz) {
    typedef struct {
	short xs, ys, xz, yz;
    } Saved_reg_t;
    static int sr_buf_size = 0, save_off = 0, read_off = 0;
    static char *sr_buf = NULL;
    Saved_reg_t *sr;

    if (func == AR_REWIND) {	/* reset read point to the first saved */
	read_off = 0;
	return (0);
    }
    if (func == AR_RESET) {	/* discard all saved */
	read_off = save_off = 0;
	return (0);
    }

    if (func == AR_SAVE) {
	int sz, tsz;
	if (sr_buf_size == 0) {
	    sr_buf_size = 360 * 600 * sizeof (short);
	    sr_buf = MISC_malloc (sr_buf_size);
	}
	sz = reg->xz * reg->yz * sizeof (short);
	tsz = sizeof (Saved_reg_t) + sz;
	if (save_off + tsz > sr_buf_size)
	    return (0);
	sr = (Saved_reg_t *)(sr_buf + save_off);
	sr->ys = reg->ys;
	sr->xs = reg->xs;
	sr->yz = reg->yz;
	sr->xz = reg->xz;
	memcpy ((char *)sr + sizeof (Saved_reg_t), reg->data, sz);
	save_off += tsz;
	return (1);
    }

    if (AR_READ) {	/* read next region */
	while (read_off < save_off) {
	    int sz;
	    sr = (Saved_reg_t *)(sr_buf + read_off);
	    sz = sizeof (Saved_reg_t) + sr->xz * sr->yz * sizeof (short);
	    if (read_off + sz > save_off)
		return (0);	/* savety check - should never happen */
	    if (sr->ys >= ys && sr->ys < ys + yz) {
		reg->ys = sr->ys;
		reg->xs = sr->xs;
		reg->yz = sr->yz;
		reg->xz = sr->xz;
		reg->data = (short *)((char *)sr + sizeof (Saved_reg_t));
		read_off += sz;
		return (1);
	    }
	    read_off += sz;
	}
	return (0);
    }
}

/**************************************************************************

    Prepares for threading process partition.

**************************************************************************/

static int N_threads = 1;
int VDD_threads = 1;
static int T_buf_size = 0;

enum {T_NOT_READY, T_DONE, T_PROCESS};	/* T_state values */
static int *T_state, *T_pind;
static Vdeal_t *T_vdv;

static pthread_t *T_threads = NULL;
static pthread_mutex_t Sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t Sync_st = PTHREAD_COND_INITIALIZER;
static pthread_cond_t Sync_end = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t Mutex;

static void Mutex_lock () {
    if (N_threads > 1)
	pthread_mutex_lock (&Mutex);
    return;
}

static void Mutex_unlock () {
    if (N_threads > 1)
	pthread_mutex_unlock (&Mutex);
    return;
}

static void Parallelize_partition () {
    static int inited = 0;
    int i;

    N_threads = VDD_threads;	/* for future threading control */
    if (N_threads == 1)
	return;

    if (!inited) {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init (&attr);
	pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init (&Mutex, &attr);
	pthread_mutexattr_destroy (&attr);
	inited = 1;
    }

    if (VDD_threads > T_buf_size) {
	if (T_state != NULL)
	    free (T_state);
	T_buf_size = 0;
	T_state = malloc (VDD_threads * sizeof (int) * 2);
	if (T_state == NULL) {
	    VDD_log ("malloc (%d) failed\n", VDD_threads * sizeof (int));
	    N_threads = 1;
	    return;
	}
	T_pind = T_state + VDD_threads;
	T_buf_size = VDD_threads;
    }

    if (T_threads == NULL) {

	for (i = 0; i < VDD_threads; i++) {
	    T_state[i] = T_NOT_READY;
	    T_pind[i] = -1;
	}

	T_threads = (pthread_t *)malloc (VDD_threads * sizeof (pthread_t));
	if (T_threads == NULL) {
	    VDD_log ("malloc (%d) failed\n", VDD_threads * sizeof (pthread_t));
	    N_threads = 1;
	    return;
	}
	for (i = 0; i < VDD_threads; i++) {
	    int t_ind, err;

	    t_ind = i;
	    if ((err = pthread_create 
			(T_threads + i, NULL, T_proc_partition, (void *)t_ind)) != 0) {
		VDD_log ("pthread_create 0 ret %d, errno %d\n", err, errno);
		exit (1);
	    }
	}

	pthread_mutex_lock (&Sync_mutex);  /* wait until all threads ready */
	while (1) {
	    for (i = 0; i < VDD_threads; i++) {
		if (T_state[i] != T_DONE)
		    break;
	    }
	    if (i >= VDD_threads)
		break;
	    pthread_cond_wait (&Sync_end, &Sync_mutex);
	}
	pthread_mutex_unlock (&Sync_mutex);
    }

    return;
}

/**************************************************************************

    Parallel-processing threads for process partition.

**************************************************************************/

static void *T_proc_partition (void *arg) {
    int t_ind, p_ind;

    t_ind = (int)arg;

    while (1) {

	pthread_mutex_lock (&Sync_mutex);
	T_state[t_ind] = T_DONE;
	pthread_cond_signal (&Sync_end);
	while (T_state[t_ind] != T_PROCESS)
	    pthread_cond_wait (&Sync_st, &Sync_mutex);
	p_ind = T_pind[t_ind];
	pthread_mutex_unlock (&Sync_mutex);

	Process_partition (T_vdv, Parts + p_ind);
    }
}

/**************************************************************************

    The main threads for process partition. Additional code is needed for
    supporting short delay mode.

**************************************************************************/

static void M_proc_partition (Vdeal_t *vdv, int wait) {
    int i;

    if (N_threads == 1) {	/* No threading */
	VDD_in_thread = 0;
	for (i = 0; i < N_parts; i++) {
	    if (Parts[i].state == P_WAIT) {
		Process_partition (vdv, Parts + i);
		Parts[i].state = P_DONE;
	    }
	}
	return;
    }

    pthread_mutex_lock (&Sync_mutex);
    while (1) {
	int cnt, pti;

	for (i = 0; i < N_threads; i++) {
	    if (T_state[i] == T_DONE) {
		if (T_pind[i] >= 0 && Parts[T_pind[i]].state == P_PROCESS) {
		    Parts[T_pind[i]].state = P_DONE;
		    T_pind[i] = -1;
		    if (vdv->realtime)
			Output_data (vdv);
		}
	    }
	}

	for (i = 0; i < N_threads; i++) {
	    if (T_state[i] == T_DONE)
		break;
	}

	cnt = 0;
	for (pti = 0; pti < N_parts; pti++) {
	    if (Parts[pti].state == P_WAIT)
		break;
	    else if (Parts[pti].state >= P_DONE)
		cnt++;
	}
	if (cnt == N_parts)
	    break;

	if (pti < N_parts && i < N_threads) {	/* launch a thread */
	    VDD_in_thread = 1;
	    Parts[pti].state = P_PROCESS;
	    T_state[i] = T_PROCESS;
	    T_pind[i] = pti;
	    T_vdv = vdv;
	    pthread_cond_broadcast (&Sync_st);
	}
	else {
	    if (wait)
		pthread_cond_wait (&Sync_end, &Sync_mutex);
	    else
		break;
	}
    }
    pthread_mutex_unlock (&Sync_mutex);
    VDD_in_thread = 0;
}

/*************************************************************************

    Thread safe functions.

*************************************************************************/

void VDD_log (const char *format, ...) {
    va_list args;
    char b[512];

    Mutex_lock ();
    va_start (args, format);
    vsprintf (b, format, args);
    va_end (args);

    MISC_log ("%s", b);
    Mutex_unlock ();
}
