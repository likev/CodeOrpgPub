
/***************************************************************************

    This module contains the routines that perform the Dual-pol fields
    recombination.

***************************************************************************/

/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2010/02/12 20:16:48 $ */
/* $Id: recomb_dp_fields.c,v 1.13 2010/02/12 20:16:48 ccalvert Exp $ */
/* $Revision:  */
/* $State: */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <rpgc.h>
#include "recomb.h"

#define NOT_INIT -1.e20f	/* uninitialized float point value */
static float No_data = -1.234567e5f;

/* RDA parameters */
static float ATMOS;
static float SYSCAL;
static float ZDR_CAL;
static float INIT_PHI;
static float N_h;
static float N_v;

/* radial gate specifications */
static float Z_r0 = 0.f;	/* First gate range of the Z field */
static float Z_gsize = 0.f;	/* Gate size of the Z field */
static int N_zgates = 0;	/* Number of gates of the Z field */
static float Dp_r0 = 0.f;	/* First gate range of the DP fields */
static float Dp_gsize = 0.f;	/* Gate size of the DP fields */
static int N_gates = 0;		/* Number of gates of the DP fields */

static int *Dz_ind = NULL;	/* Index table for getting Z from DP index */

static int Recomb_failed;	/* Recombination failed */

/* gate data of the two radials */
static unsigned char *Ref1, *Ref2;
static float *Rho1, *Phi1, *Zdr1;
static float *Rho2, *Phi2, *Zdr2;
static unsigned char *Range_aliased;

/* recombind DP data */
static float *Rhoc, *Phic, *Zdrc = NULL;

/* DP field headers */
static Generic_moment_t Rho_hd, Phi_hd, Zdr_hd;

/* output DP field data word size */
static int Rhoo_ws = 16;
static int Phio_ws = 16;

static int Get_dp_data (int which_rad, Base_data_header *bh, 
		unsigned char *ref, float *rho, float *phi, float *zdr);
static int Get_dp_field (Base_data_header *bh, char *fname, 
				float *out, Generic_moment_t *shd);
static void Recomb_dp_data ();
static float Get_p (unsigned char z, int ind);
static void Add_moment (char *buf, Generic_moment_t *shd, float *data);
static int Change_word_size (Generic_moment_t *hd, int out_ws);
static int Is_diff (float x, float y);
static void Create_dz_index (int n_dgates, float dr0, float dg_s, 
			int n_zgates, float zr0, float zg_s);
static float Get_new_scale (float scale, float offset, float max_v, 
					float max_d, float *noffset);


/***************************************************************************

    Adds the recombined DP data to radial "rad". The buffer size of "rad" 
    is assumed to be sufficient. Returns 0 on success or -1 on failure.

***************************************************************************/

int RCDP_get_recombined_dp_fields (char *rad) {
    Base_data_header *bh;
    int offset;

    bh = (Base_data_header *)rad;
    if (Recomb_failed) {
	bh->no_moments = 0;
	return (-1);
    }

    /* change word sizes */
    if (Change_word_size (&Rho_hd, Rhoo_ws) < 0 ||
	Change_word_size (&Phi_hd, Phio_ws) < 0)
	return (-1);

    /* increased scale and offset for better precision of phi */
    if (Phi_hd.data_word_size == 16 && Phi_hd.scale != 0.f) {
	float nscale, noffset;
	nscale = Get_new_scale (Phi_hd.scale, Phi_hd.offset, 
		1080.f, (float)0xffff, &noffset);	/* max 1080 degrees */
	Phi_hd.scale = nscale;
	Phi_hd.offset = noffset;
    }

    if (bh->no_moments == 0) {
	int off;
	offset = 0;
	off = bh->ref_offset + bh->n_surv_bins * sizeof (Moment_t);
	if (off > offset)
	    offset = off;
	off = bh->vel_offset + bh->n_dop_bins * sizeof (Moment_t);
	if (off > offset)
	    offset = off;
	off = bh->spw_offset + bh->n_dop_bins * sizeof (Moment_t);
	if (off > offset)
	    offset = off;
    }
    else
	offset = bh->offsets[bh->no_moments - 1];
    offset = ALIGNED_SIZE (offset);

    bh->offsets[bh->no_moments] = offset;
    bh->no_moments++;
    Add_moment (rad + offset, &Rho_hd, Rhoc);
    offset += ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						N_gates * (Rhoo_ws / 8));

    bh->offsets[bh->no_moments] = offset;
    bh->no_moments++;
    Add_moment (rad + offset, &Phi_hd, Phic);
    offset += ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						N_gates * (Phio_ws / 8));

    bh->offsets[bh->no_moments] = offset;
    bh->no_moments++;
    Add_moment (rad + offset, &Zdr_hd, Zdrc);

    return (0);
}

/******************************************************************

    Changes DP field header "hd" to use a new word size "out_ws".
    Returns 0 on success or -1 on errors.
	
******************************************************************/

static int Change_word_size (Generic_moment_t *hd, int out_ws) {

    /* change word sizes */
    if (hd->data_word_size != out_ws) {
	if (hd->data_word_size == 8 && out_ws == 16) {
	    float nscale, noffset;
	    if (hd->scale == 0.f) {
		LE_send_msg (GL_ERROR, "Unexpected zero scale");
		return (-1);
	    }
	    nscale = Get_new_scale (hd->scale, hd->offset, 
			((float)255 - hd->offset) / hd->scale, 
			(float)0xffff, &noffset);
	    hd->scale = nscale;
	    hd->offset = noffset;
	    hd->data_word_size = out_ws;
	}
	else {
	    LE_send_msg (GL_ERROR, "Unexpected output word size (%d %d)", 
					out_ws, hd->data_word_size);
	    return (-1);
	}
    }
    return (0);
}

/******************************************************************

    Calculates and returns the new scale and new offset, returned 
    with "noffset", that map the physical value "max_v" to data value
    "max_d". The physical value for the minimum data value of 2 does
    not change for the new scaling. "scale" and "offset" are the old
    scale and offset.
	
******************************************************************/

static float Get_new_scale (float scale, float offset, float max_v, 
					float max_d, float *noffset) {
    float min_v, min_d, nscale;

    *noffset = offset;
    min_d = 2.f;		/* The minimum data value for all fields */
    if (scale == 0.f)
	return (scale);
    min_v = (min_d - offset) / scale;
    if (max_v - min_v == 0.f)
	return (scale);
    nscale = (max_d - min_d) / (max_v - min_v);
    *noffset = max_d - max_v * nscale;
    return (nscale);
}

/******************************************************************

    Creates a moment field of name "name" in "buf". The data is in
    "data". Other parameters provide the additional info for the 
    field.
	
******************************************************************/

static void Add_moment (char *buf, Generic_moment_t *shd, float *data) {
    Generic_moment_t *hd;
    int i, up, low, word_size;
    float scale, offset;
    unsigned short *spt;
    unsigned char *cpt;

    shd->control_flag = 1;
    memcpy (buf, (char *)shd, sizeof (Generic_moment_t));
    hd = (Generic_moment_t *)buf;
    word_size = shd->data_word_size;
    scale = shd->scale;
    offset = shd->offset;

    if (word_size == 16) {
	up = 0xffff;
	low = 2;
    }
    else {
	up = 0xff;
	low = 2;
    }
    spt = hd->gate.u_s;
    cpt = hd->gate.b;
    for (i = 0; i < N_gates; i++) {
	int t;

	if (data[i] == No_data) {
	    if (Range_aliased[i])
		t = 1;
	    else
		t = 0;
	}
	else {
	    float f;
	    f = data[i] * scale + offset;
	    if (f >= 0.f)
		t = f + .5f;
	    else
		t = -(-f + .5f);
/*	    t = round ((float)data[i] * scale + offset); */
	    if (t > up)
		t = up;
	    if (t < low)
		t = low;
	}
	if (word_size == 16)
	    spt[i] = (unsigned short)t;
	else
	    cpt[i] = (unsigned char)t;
    }
}

/***************************************************************************

    Recombines DP fields of radials "rad1" and "rad2". If "rad2" == NULL,
    the DP fields in "rad1" is used as the output. The results is saved for
    future retrieval. Returns the size of the combined DP data in the 
    header or 0 on failure. If recombination fails, no output is generated.

    We assume that the reflectivity is always of 250m gate size.

***************************************************************************/

int RCDP_dp_recomb (char *rad1, char *rad2) {
    static int pre_gates = 0, pre_zgates = 0;	/* sizes of out buffer */
    Base_data_header *bh;
    int size, i;

    bh = (Base_data_header *)rad1;
    Recomb_failed = 1;

    /* get parameters from the radial header. We assume these are the same for
       the two radials. */
    ATMOS = 0.001f * bh->atmos_atten;
    SYSCAL = bh->calib_const - bh->horiz_noise;
    ZDR_CAL = 0.0f;  /* Note: This is already being appied at the RDA. */;
    INIT_PHI = (float)(bh->sys_diff_phase);	/* in degrees */
    N_h = bh->horiz_noise;
    N_v = bh->vert_noise;

    if (bh->surv_bin_size <= 0 || bh->n_surv_bins <= 0 || 
						bh->ref_offset <= 0) {
	LE_send_msg (GL_ERROR, 
		"Field Z not found (%d, %d, %d)\n", 
			bh->surv_bin_size, bh->n_surv_bins, bh->ref_offset);
	return (0);
    }
    Z_gsize = .001f * bh->surv_bin_size;
    Z_r0 = .001f * bh->range_beg_surv + .5f * Z_gsize;
    N_zgates = bh->n_surv_bins;

    /* We use ZDR as reference */
    N_gates = 0;
    for (i = 0; i < bh->no_moments; i++) {
	Generic_moment_t *gm;
	gm = (Generic_moment_t *)((char *)bh + bh->offsets[i]);
	if (strncmp (gm->name, "DZDR", 4) == 0) {
	    int n_dgates;

	    n_dgates = gm->no_of_gates;
	    Dp_gsize = .001f * gm->bin_size;
	    Dp_r0 = .001f * gm->first_gate_range;
	    if (n_dgates <= 0 || gm->bin_size <= 0) {
		LE_send_msg (GL_ERROR, 
		    "Bad ZDR gate spec (%d, %d)\n", n_dgates, gm->bin_size);
		return (0);
	    }
	    Create_dz_index (n_dgates, Dp_r0, Dp_gsize,
					N_zgates, Z_r0, Z_gsize);
	    N_gates = n_dgates;
	    break;
	}
    }
    if (N_gates == 0)		/* DP field not found */
	return (0);

    if (N_gates > pre_gates || N_zgates > pre_zgates) { /* allocate buffers */
	char *buf;
	if (Zdrc != NULL) {
	    free (Zdrc);
	    free (Zdr1);
	    free (Zdr2);
	    free (Range_aliased);
	}
	buf = MISC_malloc (3 * N_gates * sizeof (float) + N_zgates);
	Zdr1 = (float *)buf;
	Phi1 = Zdr1 + N_gates;
	Rho1 = Phi1 + N_gates;
	Ref1 = (unsigned char *)(Rho1 + N_gates);
	buf = MISC_malloc (3 * N_gates * sizeof (float) + N_zgates);
	Zdr2 = (float *)buf;
	Phi2 = Zdr2 + N_gates;
	Rho2 = Phi2 + N_gates;
	Ref2 = (unsigned char *)(Rho2 + N_gates);
	buf = MISC_malloc (3 * N_gates * sizeof (float));
	Zdrc = (float *)buf;
	Phic = Zdrc + N_gates;
	Rhoc = Phic + N_gates;
	Range_aliased = MISC_malloc (N_gates * sizeof (unsigned char));
	pre_gates = N_gates;
	pre_zgates = N_zgates;
    }

    /* extract and convert data fields */
    memset (Range_aliased, 0, N_gates * sizeof (unsigned char));
    if (Get_dp_data (0, bh, Ref1, Rho1, Phi1, Zdr1) < 0)
	return (0);
    if (rad2 != NULL &&
	Get_dp_data (1, (Base_data_header *)rad2, 
				Ref2, Rho2, Phi2, Zdr2) < 0)
	return (0);

    if (rad2 == NULL) {
	memcpy (Rhoc, Rho1, N_gates * sizeof (float));
	memcpy (Phic, Phi1, N_gates * sizeof (float));
	memcpy (Zdrc, Zdr1, N_gates * sizeof (float));
    }
    else
	Recomb_dp_data ();

    size = ALIGNED_SIZE (sizeof (Generic_moment_t) + N_gates * (Rhoo_ws / 8));
    size += ALIGNED_SIZE (sizeof (Generic_moment_t) + N_gates * (Phio_ws / 8));
    size += ALIGNED_SIZE (sizeof (Generic_moment_t) + 
				N_gates * (Zdr_hd.data_word_size / 8));

    Recomb_failed = 0;
    return (size);
}

/******************************************************************

    Creates the index table for locating the Z gate given the gate 
    index of a DP field.

******************************************************************/

static void Create_dz_index (int n_dgates, float dr0, float dg_s, 
			int n_zgates, float zr0, float zg_s) {
    static int prev_n_dgates = 0, prev_n_zgates = 0;
    static float prev_dr0 = 0.f, prev_dg_s = 0.f, prev_zr0 = 0.f, prev_zg_s = 0.f;
    int i;

    if (n_dgates == prev_n_dgates && dr0 == prev_dr0 && dg_s == prev_dg_s && 
	n_zgates == prev_n_zgates && zr0 == prev_zr0 && zg_s == prev_zg_s)
	return;

    if (Dz_ind != NULL)
	free (Dz_ind);

    Dz_ind = (int *)MISC_malloc (n_dgates * sizeof (int));
    for (i = 0; i < n_dgates; i++) {
	float d;
	int oi;

	d = dr0 + (float)i * dg_s;
	oi = (d - zr0) / zg_s;
	if (d - (zr0 + oi * zg_s) > .5f * zg_s)
	    oi++;
	if (oi < 0 || oi > n_zgates)
	    oi = n_zgates;
	Dz_ind[i] = oi;
    }
    prev_n_dgates = n_dgates;
    prev_dr0 = dr0;
    prev_dg_s = dg_s;
    prev_n_zgates = n_zgates;
    prev_zr0 = zr0;
    prev_zg_s = zg_s;
    return;
}

/**********************************************************************

    Performs the DP recombination. The source are Ref?, Rho?, Phi?, Zdr?
    and the results are put in Rhoc, Phic, Zdrc.

    If I use unsigned char Zdr, a lookup table can be used to calculate
    1 / exp10f (.1f * Zdr1[i]). This however relies on the assumption 
    that ZDR is always 8 bit. I don't use lookup table for the moment.
	
**********************************************************************/

static void Recomb_dp_data () {
    int i;
    float degree_to_radian, radian_to_degree;

    degree_to_radian = ONE_RADIAN;
    radian_to_degree = 1.f / degree_to_radian;
    for (i = 0; i < N_gates; i++) {
	float ph1, ph2, pv1, pv2;
	float re1, im1, re2, im2;
	float phc, pvc, imc, rec;
	float t, f;
	int ind;

	ind = Dz_ind[i];
	ph1 = Get_p (Ref1[ind], ind);
	ph2 = Get_p (Ref2[ind], ind);

	if (ph1 != No_data && Zdr1[i] != No_data)
	    pv1 = ph1 / (exp10f (.1f * (Zdr1[i] + ZDR_CAL)));
	else
	    pv1 = No_data;
	if (ph2 != No_data && Zdr2[i] != No_data)
	    pv2 = ph2 / (exp10f (.1f * (Zdr2[i] + ZDR_CAL)));
	else
	    pv2 = No_data;

	if (ph1 != No_data && pv1 != No_data && 
		Rho1[i] != No_data && Phi1[i] != No_data) {
	    t = Rho1[i] * sqrtf (ph1 * pv1);
	    f = Phi1[i] * degree_to_radian;
	    re1 = t * cosf (-f);
	    im1 = t * sinf (-f);
	}
	else {
	    re1 = No_data;
	    im1 = No_data;
	}

	if (ph2 != No_data && pv2 != No_data && 
		Rho2[i] != No_data && Phi2[i] != No_data) {
	    t = Rho2[i] * sqrtf (ph2 * pv2);
	    f = Phi2[i] * degree_to_radian;
	    re2 = t * cosf (-f);
	    im2 = t * sinf (-f);
	}
	else {
	    re2 = No_data;
	    im2 = No_data;
	}

	if (ph1 != No_data) {
	    if (ph2 != No_data)
		phc = .5f * (ph1 + ph2);
	    else
		phc = ph1;
	}
	else if (ph2 != No_data) {
	    phc = ph2;
	}
	else
	    phc = No_data;

	if (pv1 != No_data) {
	    if (pv2 != No_data)
		pvc = .5f * (pv1 + pv2);
	    else
		pvc = pv1;
	}
	else if (pv2 != No_data) {
	    pvc = pv2;
	}
	else
	    pvc = No_data;

	if (re1 != No_data) {
	    if (re2 != No_data)
		rec = .5f * (re1 + re2);
	    else
		rec = re1;
	}
	else if (re2 != No_data) {
	    rec = re2;
	}
	else
	    rec = No_data;

	if (im1 != No_data) {
	    if (im2 != No_data)
		imc = .5f * (im1 + im2);
	    else
		imc = im1;
	}
	else if (im2 != No_data) {
	    imc = im2;
	}
	else
	    imc = No_data;

	if (phc != No_data && pvc != No_data)
	    Zdrc[i] = 10.f * log10f (phc / pvc) - ZDR_CAL;
	else
	    Zdrc[i] = No_data;

	if (rec != No_data && imc != No_data && 
			phc != No_data && pvc != No_data)
	    Rhoc[i] = sqrtf ((rec * rec + imc * imc) / (phc * pvc));
	else
	    Rhoc[i] = No_data;

	if (rec != No_data && imc != No_data) {
	    Phic[i] = - atan2f (imc, rec) * radian_to_degree;
	    if (Phic[i] < 0.f)
		Phic[i] += 360.f;
	}
	else
	    Phic[i] = No_data;
    }
}

/**********************************************************************

    Returnd power of reflectivity "z" of gate index "ind".
	
**********************************************************************/

static float Get_p (unsigned char z, int ind) {
    static float z_table[256], *r_table = NULL;
    static float r0 = NOT_INIT, atmos = NOT_INIT;
    static float syscal = NOT_INIT, gsize = NOT_INIT;
    static int n_gates = 0;

    if (N_zgates > n_gates || Z_r0 != r0 || 
				ATMOS != atmos || Z_gsize != gsize) {
	int i;
	if (r_table != NULL)
	    free (r_table);
	r_table = MISC_malloc (N_zgates * sizeof (float));
	for (i = 0; i < N_zgates; i++) {
	    float R;
	    R = Z_r0 + Z_gsize * i;
	    r_table[i] = exp10f (0.1f * (R * ATMOS - 20.f * log10f (R)));
	}
	r0 = Z_r0;
	atmos = ATMOS;
	gsize = Z_gsize;
	n_gates = N_zgates;
    }

    if (SYSCAL != syscal) {
	int i;
	for (i = 0; i < 256; i++) {
	    float zf;
	    zf = i * .5f - 33.f;
	    z_table[i] = exp10f (0.1f * (zf - SYSCAL));
	}
	syscal = SYSCAL;
    }
    if (z == 0 || ind < 0 || ind >= N_zgates)
	return (No_data);
    return (z_table[z] * r_table[ind]);
}

/**********************************************************************

    Extracts, converts and returns data fields, ref, rho, phi and zdr
    from radial "bh". The DP data fields are converted to type float in
    physical units. "which_rad" = 0 or 1 for the first and the second
    redial respectively. This function returns 0 on success or -1 on
    failure.
	
**********************************************************************/

static int Get_dp_data (int which_rad, Base_data_header *bh, 
		unsigned char *ref, float *rho, float *phi, float *zdr) {
    int retz, retp, retr;

    if (which_rad != 0) {  /* the two radials must have the same gate spec */
	int ngates;
	float r0, gsize;

	ngates = bh->n_surv_bins;
	gsize = .001f * bh->surv_bin_size;
	r0 = .001f * bh->range_beg_surv + .5f * gsize;
	if (ngates != N_zgates) {
	    LE_send_msg (GL_ERROR, 
		"Z gate numbers differ (%d, %d)\n", ngates, N_zgates);
	    return (-1);
	}
	if (Is_diff (Z_gsize, gsize)) {
	    LE_send_msg (GL_ERROR, 
		"Z gate sizes differ (%f, %f)\n", gsize, Z_gsize);
	    return (-1);
	}
	if (Is_diff (Z_r0, r0)) {
	    LE_send_msg (GL_ERROR, 
		"Z first gate ranges differ (%f, %f)\n", r0, Z_r0);
	    return (-1);
	}
	if (bh->ref_offset <= 0) {
	    LE_send_msg (GL_ERROR, 
		"Unexpected second redial ref_offset (%d)\n", bh->ref_offset);
	    return (-1);
	}
    }

    if ((retz = Get_dp_field (bh, "DZDR", zdr, &Zdr_hd)) < 0 ||
	(retp = Get_dp_field (bh, "DPHI", phi, &Phi_hd)) < 0 ||
	(retr = Get_dp_field (bh, "DRHO", rho, &Rho_hd)) < 0)
	return (-1);

    if (retz + retp + retr == 0)	/* no DP data */
	return (-1);

    if (retz + retp + retr < 3) {
	if (retz == 0)
	    LE_send_msg (GL_ERROR, "zdr field missing\n");
	if (retp == 0)
	    LE_send_msg (GL_ERROR, "phi field missing\n");
	if (retr == 0)
	    LE_send_msg (GL_ERROR, "rho field missing\n");
	return (-1);
    }

    {		/* get reflectivity data */
	int i;
	Moment_t *d = (Moment_t *)((char *)bh + bh->ref_offset);
	for (i = 0; i < N_zgates; i++)
	    ref[i] = d[i];
    }
    return (0);
}

/******************************************************************

    Gets data for DP fields "fname" from radial "bh", converts it to
    float in physical units and return it with "out". This function
    returns 1 on success, 0 if the field is not found or -1 on
    failure. A DP field must have the same gate spec as the ZDR field
    of the first radial.
	
******************************************************************/

static int Get_dp_field (Base_data_header *bh, char *fname, 
				float *out, Generic_moment_t *shd) {
    int i;

    for (i = 0; i < bh->no_moments; i++) {
	Generic_moment_t *gm;

	gm = (Generic_moment_t *)((char *)bh + bh->offsets[i]);
	if (strncmp (gm->name, fname, 4) == 0) {
	    int k, ngates;
	    float gsize, r0, scale, offset;

	    ngates = gm->no_of_gates;
	    gsize = .001f * gm->bin_size;
	    r0 = .001f * gm->first_gate_range;

	    if (ngates != N_gates) {
		LE_send_msg (GL_ERROR, 
			"DP n_gates (%s) differs from ZDR (%d, %d)",
					fname, ngates, N_gates);
		return (-1);
	    }
	    if (Is_diff (r0, Dp_r0)) {
		LE_send_msg (GL_ERROR, 
		"DP R0 (%s) differs from ZDR (%f, %f)", fname, r0, Dp_r0);
		return (-1);
	    }
	    if (Is_diff (gsize, Dp_gsize)) {
		LE_send_msg (GL_ERROR, 
			"DP gate size (%s) differs from ZDR (%f, %f)",
					fname, gsize, Dp_gsize);
		return (-1);
	    }

	    memcpy (shd, gm, sizeof (Generic_moment_t));

	    scale = gm->scale;
	    offset = gm->offset;
	    if (scale == 0.f) {
		LE_send_msg (GL_ERROR, "Zero scale (%s)", fname);
		return (-1);
	    }

	    scale = 1.f / scale;
	    offset = -offset;
	    if (gm->data_word_size == 16) {
		unsigned short *p = gm->gate.u_s;
		for (k = 0; k < N_gates; k++) {
		    if (p[k] <= 1) {
			if (p[k] == 1)
			    Range_aliased[k] = 1;
			out[k] = No_data;
		    }
		    else
			out[k] = ((float)p[k] + offset) * scale;
		}
	    }
	    else {
		unsigned char *p = gm->gate.b;
		for (k = 0; k < N_gates; k++) {
		    if (p[k] <= 1) {
			if (p[k] == 1)
			    Range_aliased[k] = 1;
			out[k] = No_data;
		    }
		    else
			out[k] = ((float)p[k] + offset) * scale;
		}
	    }
	    return (1);
	}
    }
    return (0);
}

/******************************************************************

    Returns 1 if the two floating point numbers "x" and "y" are 
    significantly different, or 0 otherwise.
	
******************************************************************/

static int Is_diff (float x, float y) {
    float diff;

    diff = x - y;
    if (diff < 0.f)
	diff = -diff;
    if (x < 0.f)
	x = -x;
    if (y < 0.f)
	y = -y;
    if (y > x)
	x = y;
    if (x < .000001f)
	return (0);
    if (diff / x > .00001f)
	return (1);
    return (0);
}
