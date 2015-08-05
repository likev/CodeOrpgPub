
/**************************************************************************

    The vdeal's clump module.

**************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/05/27 19:49:39 $
 * $Id: vdeal_clump.c,v 1.6 2015/05/27 19:49:39 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "vdeal.h"
#include "infr.h"

typedef struct interval {	/* spec for each interval */
    short row;		/* row index of interval */
    short begin;	/* interval begins in this column */
    short end;		/* interval ends in this column */
    short prev;		/* index of the first connected itv on prev row */
    short next;		/* index of the first connected itv on next row */
    int id;		/* clump id */
    int nind;		/* index of the next itv of the region */
} Interval;	

typedef struct reg_spec_t {	/* spec for each region */
    int st;		/* starting index in itv for the region */
    int n_itvs;		/* number of itvs for the region */
    short xs;		/* region start x in the area */
    short xz;		/* region x size */
    short ys;		/* region start y in the area */
    short yz;		/* region y size */
} Reg_spec_t;

typedef struct regions_t {	/* struct for region identification info */
    int bsz;		/* itv buffer size */
    Interval *itv;	/* intervals */
    int nitv;		/* number of intervals */
    int n_regions;	/* number of regions */
    Reg_spec_t *ris;	/* regions */
    int rind_bsz;	/* buffer size for ris */
    int cr_r_ind;	/* current region index */
    unsigned char *inp;	/* input image (the area to clump) */
    int stride;		/* stride for inp */
    int xst, yst;	/* start location of the area to clump */
    int ysize;		/* number of rows of the area to clump */
} Regions_t;

enum {STACK_POP, STACK_PUSH, STACK_INIT}; /* for argument "func" of "Stack" */

__thread char *VDC_region_buf = NULL;	/* shared work space for each region */
static __thread Regions_t *Rgs_buf = NULL;	/* shared clumping struct */

/* variables used by Check_conn */
static __thread int Nyq, Thrl, Thrh, Stride;
static __thread unsigned char *Image;

static int Set_offsets (Interval *itvs, int n_itvs);
static int Stack (int func, int *v);
static void Mark_connnected_itvs (Interval *its, int n_itvs, 
						int ind, int value);
static int Min_xs_cmp (const void *d1, const void *d2);
static int Max_size_cmp (const void *d1, const void *d2);
static int Max_xz_cmp (const void *d1, const void *d2);
static int Clump_2d (Interval *itvs, int n_itvs, void *parms, int flags);
static int Check_conn (Interval *it, Interval *nit);
static int Merge_wrapped_regions (Interval *itv, int cnt, 
					Interval *st_itv, int yz);
static void Adjust_ys_yz (Reg_spec_t *ris, Interval *st_itv, 
					Interval *itv, int ysize);


/************************************************************************

    Frees memory segments allocated for clump structure rgsp. 

************************************************************************/

void VDC_free (void *rgsp) {
    Regions_t *rgs;

    if (rgsp == NULL)		/* The shared struct */
	rgs = Rgs_buf;
    else
	rgs = (Regions_t *)rgsp;
    if (rgs == NULL)
	return;
    if (rgs->itv != NULL) {
	free (rgs->itv);
	rgs->itv = NULL;
 	rgs->bsz = 0;
    }
    if (rgs->ris != NULL) {
	free (rgs->ris);
	rgs->ris = NULL;
	rgs->rind_bsz = 0;
    }
    if (rgs != Rgs_buf)
	free (rgs);
}

/**********************************************************************

    Identifies connected regions in area at (xst, yst) of sizes (xsize,
    ysize) in image "img" of "stride". Returns the number of regions
    found or -1 on failure. "rgsp" must be intialized to NULL if not
    returned from this. If "rgsp" is static, it does not need to be
    freed and malloced segments may be reused. If "rgsp" is non-static,
    it must be freed with VDC_free. If "rgsp" is NULL, a shared internal
    buffer is used. Part of "flags" specifies how to sort the regions.
    The default is sorted on "xs". Bit VDC_IDR_WRAP of "flags" indicates
    that the wrap-around connection is considered. "parms" specifies the
    max shear thresholding - If the shear is >= spec, it is considered
    to be not-connected. If parms == NULL, the thresholding is disabled.
    "cdf" provides the data filter.

**********************************************************************/

int VDC_identify_regions (unsigned char *img, Data_filter_t *cdf, int stride, 
			int xst, int yst, int xsize, int ysize, 
			void *parms, int flags, void **rgsp) {
    int nr, cnt, thrl, thrh, i, rgcnt, nyq, bno_data;
    Interval *itv, *st_itv, *enditv;
    Regions_t *rgs;
    unsigned char ybits, ebits, *dmap;

    if (rgsp == NULL)
	rgsp = (void **)&Rgs_buf;
    if (*rgsp == NULL) {
	*rgsp = MISC_malloc (sizeof (Regions_t));
	memset (*rgsp, 0, sizeof (Regions_t));
    }
    rgs = (Regions_t *)*rgsp;
    if (rgs->bsz == 0) {
	int bsz = 1024;
	rgs->itv = (Interval *) MISC_malloc (bsz * sizeof (Interval));
	rgs->bsz = bsz;
    }
    itv = rgs->itv;
    img = img + (yst * stride + xst);
    if (cdf != NULL) {
	dmap = cdf->map + (yst * stride + xst);
	ybits = cdf->yes_bits;
	ebits = cdf->exc_bits;
    }
    else {
	dmap = NULL;
	ybits = ebits = 0;
    }
    rgs->inp = img;
    rgs->stride = stride;
    rgs->xst = xst;
    rgs->yst = yst;
    rgs->ysize = ysize;
    bno_data = BNO_DATA;

    /* find intervals */
    cnt = 0;
    for (i = 0; i < ysize; i++) {
	int in, j;
	unsigned char *cpt, *dmapp;
	Params_t *pms;

	if (flags & VDC_PARM_ARRAY)
	    pms = ((Params_t **)parms)[i];
	else
	    pms = (Params_t *)parms;
	if (pms != NULL) {		/* threshold for range direction */
	    nyq = pms->nyq;
	    thrl = pms->max_shear;
	    thrh = nyq * 2 - pms->am_shear;
	}
	else
	    nyq = thrl = thrh = 0;

	cpt = img + (i * stride);
	if (dmap != NULL)
	    dmapp = dmap + (i * stride);
	else
	    dmapp = NULL;
	in = 0;
	for (j = 0; j < xsize; j++) {
	    int cptj, end;
	    cptj = cpt[j];
	    if (dmap != NULL) {
		unsigned char m = dmapp[j];
		if (ybits) {
		    if (!(m & ybits))
			cptj = bno_data;
		}
		if (ebits) {
		    if (m & ebits)
			cptj = bno_data;
		}
	    }
	    end = 0;
	    if (cptj != bno_data) {
		if (in == 0) {
		    Interval *t;
		    in = 1;
		    t = itv + cnt;
		    t->begin = j;
		    t->row = i;
		    t->id = 0;
		    t->prev = -1;
		    t->next = -1;
		    t->nind = 0;
		}
		else if (nyq > 0) {
		    int diff = cptj - cpt[j - 1];
		    if (diff < 0)
			diff = -diff;
		    if (diff >= thrl && diff <= thrh)
			end = 2;
		}
	    }
	    else
		end = 1;
	    if (end) {
		if (in == 1) {
		    in = 0;
		    itv[cnt++].end = j - 1;
		    if (end == 2)
			j--;
		    if (cnt >= rgs->bsz - 32) {
			char *p;
			int bsz = rgs->bsz + 1024;
			p = (char *)MISC_malloc (bsz * sizeof (Interval));
			memcpy (p, rgs->itv, rgs->bsz * sizeof (Interval));
			rgs->bsz = bsz;
			if (rgs->itv != NULL)
			    free (rgs->itv);
			rgs->itv = (Interval *)p;
			itv = rgs->itv;
		    }
		}
	    }
	}
	if (in == 1) {
	    itv[cnt++].end = xsize - 1;
	    if (cnt >= rgs->bsz - 32) {	/* check and extend after cnt++ */
		char *p;
		int bsz = rgs->bsz + 1024;
		p = (char *)MISC_malloc (bsz * sizeof (Interval));
		memcpy (p, rgs->itv, rgs->bsz * sizeof (Interval));
		rgs->bsz = bsz;
		if (rgs->itv != NULL)
		    free (rgs->itv);
		rgs->itv = (Interval *)p;
		itv = rgs->itv;
	    }
	}
    }
    rgs->nitv = cnt;

    /* find regions */
    Image = img;
    Stride = stride;
    nr = Clump_2d (itv, cnt, parms, flags);

    if (nr > rgs->rind_bsz) {
	if (rgs->ris != NULL)
	    free (rgs->ris);
	rgs->ris = (Reg_spec_t *)MISC_malloc (nr * sizeof (Reg_spec_t));
	rgs->rind_bsz = nr;
    }

    {
	Params_t *pms;
	if (flags & VDC_PARM_ARRAY)
	    pms = ((Params_t **)parms)[0];
	else
	    pms = (Params_t *)parms;
	if (pms != NULL) {	/* threshold for Merge_wrapped_regions */
	    Nyq = pms->nyq;
	    Thrl = pms->max_shear;
	    Thrh = Nyq * 2 - pms->am_shear;
	}
	else
	    Nyq = Thrl = Thrh = 0;
    }

    rgcnt = 0;
    st_itv = itv;
    enditv = itv + cnt;
    for (i = 0; i < nr; i++) {		/* for each region */
	int xmin, xmax, ymin, ymax, nitv, cid;
	Interval *it;
	Reg_spec_t *ris;

	/* find xs, xz - x location and size */
	it = st_itv;
	while (st_itv < enditv && st_itv->id != i + 1)
	    st_itv++;
	if (st_itv >= enditv) {	/* id not found - possible after merge */
	    st_itv = it;
	    continue;
	}
	if ((flags & VDC_IDR_WRAP) && 
			Merge_wrapped_regions (itv, cnt, st_itv, ysize)) {
	    i--;
	    continue;
	}

	xmin = ymin = 100000;
	xmax = ymax = -xmin;
	nitv = 0;
	cid = st_itv->id;
	ris = rgs->ris + rgcnt;
	ris->st = st_itv - itv;
	it = st_itv;
	while (1) {
	    if (it->id != cid)
		break;
	    if (it->begin < xmin)
		xmin = it->begin;
	    if (it->end > xmax)
		xmax = it->end;
	    if (it->row < ymin)
		ymin = it->row;
	    if (it->row > ymax)
		ymax = it->row;
	    nitv++;
	    if (it->nind == 0)
		break;
	    it = itv + it->nind;
	}
	if (nitv == 0) {
	    VDD_log ("Error found - 0 size clump\n");
	    exit (1);
	}
	ris->n_itvs = nitv;
	ris->xs = xmin;
	ris->xz = xmax - xmin + 1;
	ris->ys = ymin;
	ris->yz = ymax - ymin + 1;

	if ((flags & VDC_IDR_WRAP) && ymax - ymin + 1 == ysize)
	    Adjust_ys_yz (ris, st_itv, itv, ysize);
	rgcnt++;
    }
    if ((flags & VDC_IDR_SORT) == VDC_IDR_XS)
	qsort (rgs->ris, rgcnt, sizeof (Reg_spec_t), Min_xs_cmp);
    else if ((flags & VDC_IDR_SORT) == VDC_IDR_XZ)
	qsort (rgs->ris, rgcnt, sizeof (Reg_spec_t), Max_xz_cmp);
    else
	qsort (rgs->ris, rgcnt, sizeof (Reg_spec_t), Max_size_cmp);
    rgs->cr_r_ind = 0;
    rgs->n_regions = rgcnt;

    return (rgcnt);
}

/******************************************************************

    Merges ragions assuming the top and the bottom of the image is 
    connected. Returns 1 if any merge has been done or 0 otherwise.

******************************************************************/

static int Merge_wrapped_regions (Interval *itv, int cnt, 
					Interval *st_itv, int yz) {
    Interval *enditv, *cit;
    int cid;

    enditv = itv + cnt;
    cid = st_itv->id;
    cit = NULL;
    while (1) {
	Interval *ittop, *itbot, *mit;
	int merge_id;

	ittop = itv;
	merge_id = 0;
	while (ittop < enditv && ittop->row == 0) {
	    itbot = itv + cnt - 1;
	    while (itbot >= itv && itbot->row == yz - 1) {
		if (ittop->id == cid && itbot->id > cid)
		    merge_id = itbot->id;
		if (itbot->id == cid && ittop->id > cid)
		    merge_id = ittop->id;
		if (merge_id > 0 && itbot->begin <= ittop->end && 
			    ittop->begin <= itbot->end && 
			    Check_conn (ittop, itbot))
		    break;
		merge_id = 0;
		itbot--;
	    }
	    if (merge_id > 0)
		break;
	    ittop++;
	}
	if (merge_id == 0)
	    break;

	mit = st_itv;
	while (mit < enditv && mit->id != merge_id)
	    mit++;
	if (mit >= enditv)	/* should never happen */
	    break;

	if (cit == NULL) {
	    Interval *it = st_itv;
	    while (it < enditv && it->id == cid) {
		cit = it;	/* the last itv of cid */
		if (it->nind == 0)
		    break;
		it = itv + it->nind;
	    }
	}

	cit->nind = mit - itv;
	while (mit < enditv && mit->id == merge_id) {
	    mit->id = cid;
	    cit = mit;
	    if (mit->nind == 0)
		break;
	    mit = itv + mit->nind;
	}
    }
    if (cit == NULL)		/* nothing merged */
	return (0);
    return (1);
}

/******************************************************************

    Move ys for wrapped regions.

******************************************************************/

static void Adjust_ys_yz (Reg_spec_t *ris, Interval *st_itv, 
					Interval *itv, int ysize) {
    static __thread char *ry = NULL;
    int i, cid, cnt, ys;
    Interval *it;

    ry = STR_reset (ry, ysize * sizeof (char));

    for (i = 0; i < ysize; i++)
	ry[i] = 0;
    cid = st_itv->id;
    it = st_itv;
    while (1) {
	int r;
	if (it->id != cid)
	    break;
	r = it->row;
	if (r < 0 || r >= ysize) {
	    VDD_log ("Programming error - Unexpected it->row (%d)\n", r);
	    exit (1);
	}
	ry[r] = 1;
	if (it->nind == 0)
	    break;
	it = itv + it->nind;
    }
    cnt = 0;
    ys = -1;
    for (i = ysize - 1; i >= 0; i--) {
	if (ry[i] != 0)
	    cnt++;
	else if (ys < 0)
	    ys = i + 1;
    }
    if (ys >= 0) {
	ris->ys = ys;
	ris->yz = cnt;
    }
}

/******************************************************************

    Comparison function for sorting regions based on xs.

******************************************************************/

static int Min_xs_cmp (const void *d1, const void *d2) {
    Reg_spec_t *r1, *r2;

    r1 = (Reg_spec_t *)d1;
    r2 = (Reg_spec_t *)d2;
    return (r1->xs - r2->xs);
}

/******************************************************************

    Comparison function for sorting regions based on size.

******************************************************************/

static int Max_size_cmp (const void *d1, const void *d2) {
    Reg_spec_t *r1, *r2;

    r1 = (Reg_spec_t *)d1;
    r2 = (Reg_spec_t *)d2;
    return (r2->xz * r2->yz - r1->xz * r1->yz);
}

/******************************************************************

    Comparison function for sorting regions based on X size.

******************************************************************/

static int Max_xz_cmp (const void *d1, const void *d2) {
    Reg_spec_t *r1, *r2;

    r1 = (Reg_spec_t *)d1;
    r2 = (Reg_spec_t *)d2;
    return (r2->xz - r1->xz);
}

/******************************************************************

    Copies the ind-th or the next, if ind < 0, region to "out". 
    The region location and size are returned in "out". Returns 0 
    on success or -1 is there is no next region.

******************************************************************/

int VDC_get_next_region (void *rgsp, int ind, Region_t *out) {
    Interval *itv;
    Reg_spec_t *ritv;
    int cnt, index, bin;
    Regions_t *rgs;

    if (rgsp == NULL)
	rgsp = Rgs_buf;
    rgs = (Regions_t *)rgsp;
    bin = 0;
    if (ind & VDC_BIN) {
	bin = 1;
	ind &= ~VDC_BIN;
    }
    if (ind == VDC_NEXT)
	index = rgs->cr_r_ind;
    else
	index = ind;
    if (index >= rgs->n_regions)
	return (-1);

    cnt = 0;
    itv = rgs->itv;
    ritv = rgs->ris + index;
    if (out != NULL) {
	int i, n, ostride, total, xss, yss;
	unsigned char *im;
	short *spt;
	Interval *it;

	ostride = ritv->xz;
	total = ritv->yz * ostride;
	if (out->data != NULL) {
	    if (bin) {
		unsigned char *cpt = (unsigned char *)out->data;
		for (i = 0; i < total; i++)
		    cpt[i] = BNO_DATA;
	    }
	    else {
		spt = out->data;
		for (i = 0; i < total; i++)
		    spt[i] = SNO_DATA;
	    }
	}

	n = ritv->n_itvs;
	yss = ritv->ys;
	xss = ritv->xs;
	it = itv + ritv->st;
	for (i = 0; i < n; i++) {
	    int r, st, sz, x, ro;

	    r = it->row;
	    st = it->begin;
	    sz = it->end - st + 1;
	    if (out->data != NULL) {
		im = rgs->inp + (rgs->stride * r + st);
		ro = r - yss;
		if (ro < 0)
		    ro += rgs->ysize;
		if (bin) {
		    unsigned char *cpt = (unsigned char *)out->data + 
						(ostride * ro + st - xss);
		    for (x = 0; x < sz; x++) {
			unsigned char tt = im[x];
			if (tt != BNO_DATA)
			    cpt[x] = tt;
			else
			    cpt[x] = BNO_DATA;
		    }
		}
		else {
		    spt = out->data + (ostride * ro + st - xss);
		    for (x = 0; x < sz; x++) {
			unsigned char tt = im[x];
			if (tt != BNO_DATA)
			    spt[x] = tt;
			else
			    spt[x] = SNO_DATA;
		    }
		}
	    }
	    cnt += sz;
	    it = itv + it->nind;
	}
    }
    if (out != NULL) {
	out->xs = ritv->xs + rgs->xst;
	out->ys = ritv->ys + rgs->yst;
	out->xz = ritv->xz;
	out->yz = ritv->yz;
	out->n_gs = cnt;
    }
    if (ind == VDC_NEXT)
	rgs->cr_r_ind++;

    return (cnt);
}

/******************************************************************

    Resets the current region to the first.

******************************************************************/

void VDC_reset_next_region (void *rgsp) {
    Regions_t *rgs;
    if (rgsp == NULL)
	rgsp = Rgs_buf;
    rgs = (Regions_t *)rgsp;
    rgs->cr_r_ind = 0;
}

/************************************************************************

    Clumps "n_itvs" intervals "itvs". The intervals must be sorted in
    the order of row and, in each row, in begin. id, prev and next must 
    initialized.

************************************************************************/

static int Clump_2d (Interval *itvs, int n_itvs, void *parms, int flags) {
    int cnt, i;

    Set_offsets (itvs, n_itvs);

    Stack (STACK_INIT, NULL);
    cnt = 0;
    for (i = 0; i < n_itvs; i++) {
	Params_t *pms;
	if (itvs[i].id != 0)
	    continue;

	if (flags & VDC_PARM_ARRAY)
	    pms = ((Params_t **)parms)[itvs[i].row];
	else
	    pms = (Params_t *)parms;
	if (pms != NULL) {		/* threshold for azimuth direction */
	    Nyq = pms->nyq;
	    Thrl = pms->max_shear;
	    Thrh = Nyq * 2 - pms->am_shear;
	}
	else
	    Nyq = Thrl = Thrh = 0;

	cnt++;
	Mark_connnected_itvs (itvs, n_itvs, i, cnt);
    }
    return (cnt);
}

/*************************************************************************

    Marks all connected intervals connected to "ind" with "value". Field 
    "nind" is set for convenient access to the intervals in a clump.

*************************************************************************/

static void Mark_connnected_itvs (Interval *its, int n_itvs, 
						int ind, int value) {
    Interval *itend, *prev_it;
    int nind;

    its[ind].id = value;
    Stack (STACK_PUSH, &ind);
    prev_it = its + ind;

    itend = its + n_itvs;
    while (Stack (STACK_POP, &nind)) {
	Interval *nit, *it;
	int end, row, i;

	nit = its + nind;
	end = nit->end;
	if (nit->prev > 0) {
	    it = nit - nit->prev;
	    row = it->row;
	    while (it->row == row && it->begin <= end) {
		if (it->id == 0 && Check_conn (it, nit)) {
		    it->id = value;
		    i = it - its;
		    Stack (STACK_PUSH, &i);
		    prev_it->nind = i;
		    prev_it = it;
		}
		it++;
	    }
	}
	if (nit->next > 0) {
	    it = nit + nit->next;
	    row = it->row;
	    while (it < itend && it->row == row && it->begin <= end) {
		if (it->id == 0 && Check_conn (it, nit)) {
		    it->id = value;
		    i = it - its;
		    Stack (STACK_PUSH, &i);
		    prev_it->nind = i;
		    prev_it = it;
		}
		it++;
	    }
	}
    }
    return;
}

/*************************************************************************

    Checks if the two connecting intervals are seperated due to large 
    shears. Returns 1 if connected or 0 otherwise.

*************************************************************************/

static int Check_conn (Interval *it, Interval *nit) {
    int b, e, i;
    unsigned char *p1, *p2;

    if (Nyq == 0)
	return (1);
    b = it->begin;
    if (nit->begin > b)
	b = nit->begin;
    e = it->end;
    if (nit->end < e)
	e = nit->end;
    p1 = Image + it->row * Stride;
    p2 = Image + nit->row * Stride;
    for (i = b; i <= e; i++) {
	int diff = p1[i] - p2[i];
	if (diff < 0)
	    diff = -diff;
	if (diff < Thrl || diff > Thrh)
	    return (1);
    }
    return (0);
}

/*************************************************************************

    Initializes the fields of "prev" and "next" of "itvs".

*************************************************************************/

static int Set_offsets (Interval *itvs, int n_itvs) {
    Interval *cr_itv, *next_itv;

    if (n_itvs <= 0)
	return (0);
    cr_itv = itvs;
    next_itv = itvs;

    while (1) {
	Interval *itv_end;
	int cr_row, next_row;

	itv_end = itvs + n_itvs;
	cr_row = cr_itv->row;
	while (next_itv < itv_end && next_itv->row <= cr_row)
	    next_itv++;
	if (next_itv == itv_end)
	    break;
	next_row = next_itv->row;
	
	while (1) {

	    if (cr_itv->row != cr_row) {
		while (next_itv < itv_end && next_itv->row == next_row) {
		    next_itv++;
		}
		break;
	    }

	    if (next_itv >= itv_end || next_itv->row != next_row) {
		cr_itv++;
		while (cr_itv->row == cr_row) {
		    cr_itv++;
		}
		break;
	    }
    
	    if (next_row - cr_row > 1 || cr_itv->end < next_itv->begin) {
		cr_itv++;
		continue;
	    }
	    else if (cr_itv->begin > next_itv->end) {
		next_itv++;
		continue;
	    }
	    else {		/* overlap */
		int diff = next_itv - cr_itv;
		if (cr_itv->next < 0)
		    cr_itv->next = diff;
		if (next_itv->prev < 0)
		    next_itv->prev = diff;
		if (cr_itv->end <= next_itv->end) {
		    cr_itv++;
		}
		else {
		    next_itv++;
		}
		continue;
	    }
	}
	if (next_itv >= itv_end) {
	    break;
	}
    }
    return (0);
}

/*************************************************************************

    The stack for saving the current connected intervals. The size is 
    extended upon necessary.

*************************************************************************/

static int Stack (int func, int *v) {
    static __thread int stack_ind = 0, stack_bz = 0;
    static __thread unsigned short *stack;

    if (func == STACK_POP) {
	if (stack_ind <= 0)
	    return (0);
	stack_ind--;
	*v = stack[stack_ind];
	return (1);
    }

    if (func == STACK_INIT) {
	stack_ind = 0;
	return (0);
    }

    if (stack_ind >= stack_bz) {
	int s;
	char *p;

	if (stack_bz == 0)
	    s = 1024;
	else
	    s = stack_bz * 2;
	p = MISC_malloc (s * sizeof (short));
	if (stack_bz > 0) {
	    memcpy (p, stack, stack_bz * sizeof (short));
	    free (stack);
	}
	stack = (unsigned short *)p;
	stack_bz = s;
    }
    stack[stack_ind] = *v;
    stack_ind++;
    return (0);
}



