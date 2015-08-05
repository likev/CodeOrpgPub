
/***************************************************************************

    This module contains the routines that perform the super-resolution
    radial data recombination.

***************************************************************************/

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/07/14 19:16:49 $ */
/* $Id: combine_radials.c,v 1.33 2014/07/14 19:16:49 steves Exp $ */
/* $Revision:  */
/* $State: */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <rpgc.h>
#include <rpgcs.h>
#include <basedata.h>
#include "recomb.h"

/* The following variable are per-radial. They must be set before calling 
   Combine_radials, Combine_azi or Combine_range */
static int VELRESOL;
static float VNYQ;
static float SYSCAL;
static float ATMOS;
static float NOISE1, NOISE2, NOISEc;
static float THZ;
static float THV;
static float THW;
static float R0;
static int REF_GATE_1000;

#define NOT_INIT -1.e20f	/* uninitialized float point value */
#define GP_INIT_LONG_GATE -1	/* special value for arg rind of Get_z */
#define GP_INIT_SHORT_GATE -2	/* special value for arg rind of Get_z */

static float *Range_corr_s = NULL;	/* supper res range correction table */
static float *Range_corr_l = NULL;	/* low res range correction table */

static int Index_azimuth = 1;	/* use azm_index to detemine indexed radial */
static float Index_angle = 0.f;	/* azi of the recombined first radial index,
				   NOT_INIT if non-indexed. */
static float One_over_sqrt_3;	/* 1 / sqrt (3) */

static int Azi_num = 1;		/* Current output azimuth number */

static float noise_assumption = 0.000001;
				/* We assume noise values are at 
				   noise_assumption * noise_threshold. */
static int Azi_only = 0;
static int Range_only = 0;

static void Combine_radials (int ns, int nd,
			     Moment_t *z1, Moment_t *z2,
			     Moment_t *v1, Moment_t *v2,
			     Moment_t *w1, Moment_t *w2,
			     Moment_t *dz1, Moment_t *dz2,
		Moment_t *zr, Moment_t *vr, Moment_t *wr);
static void Combine_azi (int ns, int nd,
			     Moment_t *z1, Moment_t *z2,
			     Moment_t *v1, Moment_t *v2,
			     Moment_t *w1, Moment_t *w2,
			     Moment_t *dz1, Moment_t *dz2,
		Moment_t *zr, Moment_t *vr, Moment_t *wr);
static void Combine_range (int ns, Moment_t *z1, Moment_t *zr);
static int Quantize_z (float zf);
static void Combine_dops (Moment_t v1, Moment_t v2, 
		float p1, float p2, Moment_t w1, Moment_t w2, 
		float vthr, float wthr, int rind, 
		Moment_t *v, Moment_t *w);
static float Get_z (int dbz, int rind, int which_radial);
static int Setup_and_combine (char *rad1, char *rad2, 
						char **output, int *length);
static int Range_combine_only (char *input, char **output, int *length);
static void Build_range_corr_table (int ns);
static float Get_recombined_azi (Base_data_header *hd, float azi1, float azi2);
static char *Save_radial (char *rad);
static Moment_t *Find_z_for_dopp_recomb (char *rad, char **buf);
static void Log_action (char *msg);
static void Set_start_angle (Base_data_header *rad);


/**************************************************************************

    Initializes this module.

***************************************************************************/

void CR_combine_init (int range_only, int azi_only, int index_azi) {
    Azi_only = azi_only;
    Range_only = range_only;
    Index_azimuth = index_azi;
}

/***************************************************************************

    Takes the radial input "input", generates and returns the
    recombined radial. The pointer to the recombined radial data is
    returned via "output". The size of the recombined radial is
    returned via "length". If there is no recombined radial to output,
    "length" is set to 0. The function's return value is the output
    redial_status flag, if *length > 0, or 0 otherwise. If there is
    additional radial to output, MAIN_output_radial is called to do
    it.

    Three recombination modes are implemented: Azimuth and range,
    azimuth only and range only. If the range recombination is
    involved, the output radial format conforms with the legacy header
    restrictions (fixed DOP field sizes and order) and only three
    basic fields present in the output. The output of azimuth-only
    recomb does not follow the restrictions. veldeal, ingesting recomb
    output, should support variable field size and location.

    For azimuth recombination, this function saves the first radial
    and, when the second radial is received, calls Setup_and_combine
    to perform the recombination. Azimuth recombination is not
    performed for the following cases: 1. The radial is not
    half-degree. 2. Single radial is left at the end of elevation. 3.
    Not consecutive radials. 4. Azimuths of the two radials are not
    suitable (too different, not around the target indexed azimuth).
    5. One of the radials is spot blanked. If the two radials have
    different Nyquist velocity, DOP field recombination is not
    performed.

    If azimuth recombination is involved, the radial index (azi_num)
    is reset.

***************************************************************************/

int CR_combine_radials (char *input, char **output, int *length) {
    static int saved = 0;		/* Is there a saved radial? */
    static int cr_elev_n = -1;		/* Current elevation number */
    Base_data_header *bh, *sbh;
    int recomb_2rads, new_elev, ret;

    *length = 0;
    bh = (Base_data_header *)input;
    if (bh->msg_len * sizeof (short) < sizeof (Base_data_header)) {
	LE_send_msg (GL_INFO, "Unexpected radial message len %d - discarded",
							bh->msg_len);
	return (bh->status);
    }

    if (Range_only)
	return (Range_combine_only (input, output, length));

    new_elev = 0;
    if (bh->azi_num == 1 || bh->elev_num != cr_elev_n) {	/* new elev */
	new_elev = 1;
	cr_elev_n = bh->elev_num;
	LE_send_msg (0, 
		"New elevation (%4.1f deg, vol %d): msg_type %x azm_reso %d",
		(float)bh->target_elev * .1f, bh->volume_scan_num, 
		bh->msg_type, bh->azm_reso);
	Log_action (NULL);
    }

    if ((bh->azm_reso != BASEDATA_HALF_DEGREE || new_elev) &&
	saved) {			/* process the saved radial */
	int len, stat;
	char *out;
	if (new_elev)
	    Log_action ("    Single radial recomb (last radial in elev)");
	else
	    Log_action ("    Single radial recomb (last half degree radial)");
	stat = Setup_and_combine (Save_radial (NULL), NULL, &out, &len);
	if (len > 0 &&
	    MAIN_output_radial (out, len) != RPGC_NORMAL)
	    LE_send_msg (GL_ERROR, 
		    "Failed in outputing end-elevation radial");
	saved = 0;
    }

    if (new_elev) {				/* new elev */
	if (!Index_azimuth || bh->azm_index == 0)
	    Index_angle = NOT_INIT;
	else if (bh->azm_index == 100)
	    Index_angle = 0.f;
	else
	    Index_angle = bh->azm_index * .01f;
	if (Index_angle > .5f)
	    Index_angle -= 0.5f;
	Index_angle += .25f;
    }

    if (bh->azm_reso != BASEDATA_HALF_DEGREE) {
	if (Azi_only || bh->surv_bin_size != 250) {	/* recomb not needed */
	    *output = input;
	    *length = bh->msg_len * sizeof (short);
	    Log_action ("    Not super res - recombination not needed");
	    return (bh->status);
	}
	else {			/* single radial recomb */
	    Log_action ("    Single radial recombination (non-half-degree radial)");	    ret = Setup_and_combine (input, NULL, output, length);
	    return (ret);
	}
    }

    if (!saved && (bh->status == GENDEL || bh->status == GENDVOL)) {
						/* single radial recomb */
	Log_action ("    Single radial recombination (end of elevation)");
	ret = Setup_and_combine (input, NULL, output, length);
	return (ret);
    }

    if (!saved) {				/* save the input radial */
	Save_radial (input);
	saved = 1;
	return (0);
    }

    /* In the following, we either recombine two radials or do single radial
       recombination of the saved radial and save the new input radial */
    sbh = (Base_data_header *)Save_radial (NULL);
    recomb_2rads = 1;
    if (bh->azi_num != sbh->azi_num + 1) {	/* not consecutive radials */
	Log_action ("    Single radial recomb (not consecutive radials)");
	recomb_2rads = 0;
    }
    else {					/* check azimuth */
	char b[128];
	float diff = bh->azimuth - sbh->azimuth;
	if (diff < -180.f)
	    diff += 360.f;
	if (diff < 0.f || diff > .75f) {
	    sprintf (b, "    Single radial recomb (azis too different %f, %f)", bh->azimuth, sbh->azimuth);
	    Log_action (b);
	    recomb_2rads = 0;
	}
	if (Index_angle != NOT_INIT) {
				/* saved radial must be in the first half of 
				   the degree. Assume sbf->azimuth is 
				   non-negative. */
	    float fazi = sbh->azimuth - (float)((int)sbh->azimuth);
	    if (Index_angle <= .5f) {
		if (fazi > Index_angle && fazi <= Index_angle + .5f) {
		    sprintf (b, "    Single radial recomb (azi 1 not suitable, %f, Index %f)", fazi, Index_angle);
		    Log_action (b);
		    recomb_2rads = 0;
		}
	    }
	    else {
		if (fazi > Index_angle || fazi <= Index_angle - .5f) {
		    sprintf (b, "    Single radial recomb (azi 1 not suitable, %f, Index %f)", fazi, Index_angle);
		    Log_action (b);
		    recomb_2rads = 0;
		}
	    }
	}
    }

    if (recomb_2rads) {
	Log_action ("    Two radial recombination");
	ret = Setup_and_combine ((char *)sbh, input, output, length);
	saved = 0;
	return (ret);
    }
    else {			/* process the saved radial and save the new */

	ret = Setup_and_combine ((char *)sbh, NULL, output, length);

	Save_radial (input);
	saved = 1;
	return (ret);
    }

    return (0);
}

/***************************************************************************

    Logs the current recombination action.

***************************************************************************/

static void Log_action (char *msg) {
    static char buf[256] = "";

    if (msg == NULL) {
	buf[0] = '\0';
	return;
    }
    if (strcmp (msg, buf) == 0)
	return;
    strncpy (buf, msg, 255);
    buf[255] = '\0';
    LE_send_msg (0, "%s", msg);
    return;
}

/***************************************************************************

    Saves radial "rad", if "rad" != NULL, and returns the pointer to the
    saved_radial.

***************************************************************************/

static char *Save_radial (char *rad) {
    static char *saved_radial = NULL;	/* The saved radial */
    static int rad_buf_len = 0;		/* Buffer size of saved_radial */

    if (rad != NULL) {
	Base_data_header *bh = (Base_data_header *)rad;
	if (bh->msg_len > rad_buf_len) {	/* extend the buffer */
	    if (saved_radial != NULL)
		free (saved_radial);
	    saved_radial = MISC_malloc (bh->msg_len * 2);
	    rad_buf_len = bh->msg_len;
	}
	memcpy (saved_radial, rad, bh->msg_len * 2);
    }
    return (saved_radial);
}

/***************************************************************************

    Calculates the azimuth for the output radial recombined from two
    radials of "azi1" and "azi2". "azi1" and "azi2" are assumed to be
    close to each other. In the case of indexed radials, the azimuth of
    the recombined radial is the average the two rounded to the nearest
    half degrees. In the case of non-indexed radials, the average radial
    is used, and if "azi2" is missing (indicated by "azi1" == "azi2"),
    the output azimuth is assumed to be .25 degrees plus "azi1".

***************************************************************************/

static float Get_recombined_azi (Base_data_header *hd, 
						float azi1, float azi2) {
    float a;
 
    if (azi1 - azi2 > 180.f)
	azi2 += 360.f;
    if (azi2 - azi1 > 180.f)
	azi1 += 360.f;

    if (Index_angle != NOT_INIT) {		/* indexed */
	float ind_a;

	a = (azi1 + azi2) * .5f;		/* average */
	if (hd->azm_reso == BASEDATA_HALF_DEGREE && azi1 == azi2) {
						/* round to Index_angle */
	    ind_a = (float)((int)a) + Index_angle;
	    if (ind_a - a > .5f)
		ind_a -= 1.f;
	    if (a - ind_a >= .5f)
		ind_a += 1.f;
	    a = ind_a;
	    if (a < 0.f)
		a += 360.f;
	}
    }
    else {					/* non-indexed */
	if (azi1 == azi2)			/* one radial */
	    a = azi1 + .25f;
	else
	    a = (azi1 + azi2) * .5f;
    }
    if (a >= 360.f)
	a -= 360.f;
    return (a);
}

/***************************************************************************

    Setups the environment for recombining two radials "rad1" and
    "rad2", calls Combine_radials or Combine_range to do the
    recombination and, finally, creates the output redial in "output".
    The pointer to the recombined radial data is returned via
    "output". The size of the combined radial is returned via
    "length". The function return value is the output radial_status
    flag. If there is no combined radial to output, "length" is set to
    0.

    If "rad2" = NULL, the range recombination is performed on "rad1".

***************************************************************************/

static int Setup_and_combine (char *rad1, char *rad2,
					char **output, int *length) {
    static int out_buf_size = 0;
    static char *out_buf = NULL;
    static char *buf1 = NULL;
    static char *buf2 = NULL;
    Base_data_header *bh1, *bh2, *outrad;
    int nd, ns, out_size, use_d1, use_d2, dop_size, ref_size, dp_size, vnyq;
    int out_spot_blanked;

    bh1 = (Base_data_header *)rad1;
    use_d1 = use_d2 = 1;    /* Data in both radials are used at this point */

    out_spot_blanked = 0;
    if (bh1->spot_blank_flag & SPOT_BLANK_RADIAL) {
	if (rad2 == NULL) {
	    use_d2 = 0;			/* use single (R1) */
	    out_spot_blanked = 1;
	}
	else {
	    if (((Base_data_header *)rad2)->spot_blank_flag & SPOT_BLANK_RADIAL)
		out_spot_blanked = 1;	/* use both R1 and R2 */
	    else
		use_d1 = 0;		/* use single (R2) */
	}
    }
    else {
	if (rad2 == NULL || 
	    (((Base_data_header *)rad2)->spot_blank_flag & SPOT_BLANK_RADIAL))
	    use_d2 = 0;			/* use single (R1) */
    }

    if (rad2 != NULL)		/* two-radial recombination */
	bh2 = (Base_data_header *)rad2;
    else		/* single radial recombination; use_d2 already = 0 */
	bh2 = (Base_data_header *)rad1;
    *length = 0;

    /* assume surv_range = dop_range and identical for both radials */
    R0 = .001f * (bh1->range_beg_dop + 125.f);
	
    /* check gate sizes */
    if (bh1->surv_bin_size != bh2->surv_bin_size ||
	(bh1->n_dop_bins > 0 && bh1->dop_bin_size != 250) || 
	(bh2->n_dop_bins > 0 && bh2->dop_bin_size != 250) ||
	(bh1->surv_bin_size != 250 && bh1->surv_bin_size != 1000)) {
	LE_send_msg (GL_INFO, "Unexpected bin size - discard two radials");
	return (bh2->status);
    }

    /* assuming the following fields are identical for the two radials */
    VELRESOL = bh1->dop_resolution;
    ATMOS = 0.001f * bh1->atmos_atten;
    NOISE1 = bh1->horiz_noise;
    NOISE2 = bh2->horiz_noise;
    NOISEc = 10. * log10 ((exp10 (.1 * NOISE1) + exp10 (.1 * NOISE2)) * .5);
		/* noise power of recombined radial */
    SYSCAL = bh1->calib_const - NOISE1;	/* constant from radial to radial */
    THZ = 0.125f * bh1->surv_snr_thresh;
    THV = 0.125f * bh1->vel_snr_thresh;
    THW = 0.125f * bh1->spw_snr_thresh;

    /* save Dual_pol fields for recombination */
    dp_size = 0;
    if (Azi_only) {
	if (use_d1 && use_d2)
	    dp_size = RCDP_dp_recomb ((char *)bh1, (char *)bh2);
	else if (use_d1)
	    dp_size = RCDP_dp_recomb ((char *)bh1, NULL);
	else if (use_d2)
	    dp_size = RCDP_dp_recomb ((char *)bh2, NULL);
    }

    /* If VNYQ is different, we use only one radial with the largest VNYQ */
    vnyq = bh1->nyquist_vel;
    if (use_d1 && use_d2) {
	if (bh2->nyquist_vel > bh1->nyquist_vel) {
	    vnyq = bh2->nyquist_vel;
	    use_d1 = 0;
	    LE_send_msg (0, "Two radials have different VNYQ - Use second\n");
	}
	else if (bh2->nyquist_vel < bh1->nyquist_vel) {
	    use_d2 = 0;
	    LE_send_msg (0, "Two radials have different VNYQ - Use first\n");
	}
    }
    VNYQ = .01f * vnyq;

    if (bh1->azi_num == 1)
	LE_send_msg (0, 
	    "SYSCAL %f, ATMOS %f, NOISE %f, THZ %f, THV %f, THW %f, VNYQ %f",
			SYSCAL, ATMOS, NOISE1, THZ, THV, THW, VNYQ);

    /* set constants */
    One_over_sqrt_3 = 1.f / sqrtf (3.f);

    /* calculate number of reflectivity gates for recombination */
    if (use_d1) {
	ns = bh1->n_surv_bins;
	if (use_d2 && bh2->n_surv_bins < ns)
	    ns = bh2->n_surv_bins;
    }
    else if (use_d2)
	ns = bh2->n_surv_bins;
    else
	ns = 0;
    if (ns > BASEDATA_REF_SIZE * 4)
	ns = BASEDATA_REF_SIZE * 4;
    if (Azi_only)
	ref_size = ns;
    else
	ref_size = BASEDATA_REF_SIZE;	/* fixed size for legacy algorithms */
    if (bh1->surv_bin_size == 1000) {
	ns *= 4;
	REF_GATE_1000 = 1;
    }
    else
	REF_GATE_1000 = 0;
    ns = (ns / 4) * 4;
 
    /* calculate number of Doppler gates for recombination */
    if (use_d1) {
	nd = bh1->n_dop_bins;
	if (use_d2 && bh2->n_dop_bins < nd)
	    nd = bh2->n_dop_bins;
 	if (bh1->vel_offset == 0)
	    nd = 0;
    }
    else if (use_d2) {
	nd = bh2->n_dop_bins;
 	if (bh2->vel_offset == 0)
	    nd = 0;
    }
    else
	nd = 0;
    if (nd > BASEDATA_DOP_SIZE)
	nd = BASEDATA_DOP_SIZE;
/*
    if (Azi_only)
	dop_size = nd;
    else */
	dop_size = BASEDATA_DOP_SIZE;	/* fixed size for legacy algorithms */
    if (nd > ns)
	nd = ns;
    nd = (nd / 4) * 4;

    /* prepare the output buffer */
    out_size = sizeof (Base_data_header) + 
		(dop_size * 2 + ref_size) * sizeof (Moment_t);
    out_size = ALIGNED_SIZE (out_size);
    out_size += dp_size;
    if (out_size > out_buf_size) {	/* extend the output buffer */
	if (out_buf != NULL)
	    free (out_buf);
	out_buf = MISC_malloc (out_size);
	out_buf_size = out_size;
    }

    /* Generate the recombined radial in out_buf */
    memcpy (out_buf, (char *)bh2, sizeof (Base_data_header));
    outrad = (Base_data_header *)out_buf;
    if (outrad->vel_offset > 0) {	/* We assume, without checking, that 
		vel_offset = 0 means there is no Doppler field in the data,
		vel_offset and spw_offset are consistent (i.e. both 0 or both
		non-zero) and they are consistent for the two radials. */
	outrad->vel_offset = sizeof (Base_data_header);
	outrad->spw_offset = outrad->vel_offset + dop_size * sizeof (Moment_t);
	outrad->ref_offset = outrad->spw_offset + dop_size * sizeof (Moment_t);
    }
    else {
	outrad->vel_offset = 0;
	outrad->spw_offset = 0;
	outrad->ref_offset = BASEDATA_REF_OFF * sizeof (Moment_t);
    }
    if (use_d1 && use_d2) {		/* two radial recombination */
	void (*comb) (int, int, Moment_t *, Moment_t *, Moment_t *, 
		Moment_t *, Moment_t *, Moment_t *, Moment_t *, Moment_t *, 
		Moment_t *, Moment_t *, Moment_t *);

	if (Azi_only)
	    comb = Combine_azi;
	else
	    comb = Combine_radials;
	comb (ns, nd,	(Moment_t *)(rad1 + bh1->ref_offset), 
			(Moment_t *)(rad2 + bh2->ref_offset), 
			(Moment_t *)(rad1 + bh1->vel_offset), 
			(Moment_t *)(rad2 + bh2->vel_offset), 
			(Moment_t *)(rad1 + bh1->spw_offset), 
			(Moment_t *)(rad2 + bh2->spw_offset),
			Find_z_for_dopp_recomb (rad1, &buf1),
			Find_z_for_dopp_recomb (rad2, &buf2),
			(Moment_t *)(out_buf + outrad->ref_offset), 
			(Moment_t *)(out_buf + outrad->vel_offset), 
			(Moment_t *)(out_buf + outrad->spw_offset));
    }
    else if (use_d1 || use_d2) {	/* single radial recombination */
	char *ref, *vel, *spw;
	if (use_d1) {			/* use only data from radial 1 */
	    ref = rad1 + bh1->ref_offset;
	    vel = rad1 + bh1->vel_offset;
	    spw = rad1 + bh1->spw_offset;
	}
	else {				/* use only data from radial 2 */
	    ref = rad2 + bh2->ref_offset;
	    vel = rad2 + bh2->vel_offset;
	    spw = rad2 + bh2->spw_offset;
	}
	memcpy (out_buf + outrad->vel_offset, vel, nd * sizeof (Moment_t));
	memcpy (out_buf + outrad->spw_offset, spw, nd * sizeof (Moment_t));
	if (REF_GATE_1000)
	    memcpy (out_buf + outrad->ref_offset, ref, 
					(ns >> 2) * sizeof (Moment_t));
	else if (Azi_only)
	    memcpy (out_buf + outrad->ref_offset, ref, 
					ns * sizeof (Moment_t));
	else
	    Combine_range (ns, (Moment_t *)ref, 
			(Moment_t *)(out_buf + outrad->ref_offset));
    }

    /* reset header fields in the output radial */
    *length = out_size;
    if (!Azi_only) {
	outrad->n_surv_bins = ns / 4;
	outrad->surv_bin_size = 1000;
    }
    outrad->n_dop_bins = nd;
    outrad->azm_reso = BASEDATA_ONE_DEGREE;
    outrad->nyquist_vel = vnyq;
    outrad->azm_index = roundf (Index_angle * 100.f);
    outrad->azimuth = Get_recombined_azi (bh1, bh1->azimuth, bh2->azimuth);
    outrad->elevation = (bh1->elevation + bh2->elevation) * .5f;
    outrad->sin_azi = sinf (outrad->azimuth * (float)ONE_RADIAN);
    outrad->cos_azi = cosf (outrad->azimuth * (float)ONE_RADIAN);
    outrad->sin_ele = sinf (outrad->elevation * (float)ONE_RADIAN);
    outrad->cos_ele = cosf (outrad->elevation * (float)ONE_RADIAN);
    outrad->msg_type &= ~SUPERRES_TYPE;
    if (outrad->status == GOODINT)
	outrad->status = bh1->status;
    if (outrad->unamb_range > bh1->unamb_range)
	outrad->unamb_range = bh1->unamb_range;
    outrad->spot_blank_flag &= ~SPOT_BLANK_RADIAL;
    if (out_spot_blanked)
	outrad->spot_blank_flag |= SPOT_BLANK_RADIAL;
    if (bh1->pbd_alg_control != 0) {
	outrad->pbd_alg_control = bh1->pbd_alg_control;
	outrad->pbd_aborted_volume = bh1->pbd_aborted_volume;
    }
    if (outrad->status == GOODBEL || outrad->status == GOODBVOL)
	Azi_num = 1;
    outrad->azi_num = Azi_num++;
    Set_start_angle (outrad);
    outrad->msg_len = out_size / sizeof (short);
    outrad->msg_type |= RECOMBINED_TYPE;
    outrad->no_moments = 0;
    outrad->calib_const = SYSCAL + NOISEc;
    outrad->horiz_noise = NOISEc;

    if (Azi_only && (use_d1 || use_d2))
	RCDP_get_recombined_dp_fields ((char *)outrad);

    if (Azi_num == BASEDATA_MAX_RADIALS - 1) {
	if (outrad->elev_num == RPGCS_get_num_elevations (outrad->vcp_num ) )
	    outrad->status = GENDVOL;
	else
	    outrad->status = GENDEL;
	LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
	 "Number of Recomb Rads In Cut > %d --> Forced End of El/Vol\n",
			 BASEDATA_MAX_RADIALS);
    }
    if (Azi_num >= BASEDATA_MAX_RADIALS)
	*length = 0;		/* no output */

    *output = out_buf;

    return (outrad->status);
}

/***************************************************************************

    Performs range-only combination. The input is "input". The output
    redial is created and returned via "output". The size of the
    combined radial is returned via "length". The function return
    value is the output radial_status flag. If there is no combined
    radial to output, "length" is set to 0.

***************************************************************************/

static int Range_combine_only (char *input, char **output, int *length) {
    static char *out_buf = NULL;
    Base_data_header *bh, *outbh;
    int ns, out_size, ref_size, dop_size;

    *length = 0;
    bh = (Base_data_header *)input;

    /* Allocate the output buffer for supporting legacy input */
    ref_size = BASEDATA_REF_SIZE;	/* fixed size for legacy algorithms */
    dop_size = BASEDATA_DOP_SIZE;	/* fixed size for legacy algorithms */
    if (out_buf == NULL) {
	out_size = sizeof (Base_data_header) + 
		(dop_size * 2 + ref_size) * sizeof (Moment_t);
	out_buf = MISC_malloc (out_size);
    }
    *length = out_size;
    *output = out_buf;
    memcpy (out_buf, input, sizeof (Base_data_header));
    outbh = (Base_data_header *)out_buf;
    outbh->no_moments = 0;	/* do not pass any additional field */
    if (bh->vel_offset > 0) {
	outbh->vel_offset = sizeof (Base_data_header);
	outbh->spw_offset = outbh->vel_offset + dop_size * sizeof (Moment_t);
	outbh->ref_offset = outbh->spw_offset + dop_size * sizeof (Moment_t);
    }
    else {
	outbh->vel_offset = 0;
	outbh->spw_offset = 0;
	outbh->ref_offset = BASEDATA_REF_OFF * sizeof (Moment_t);
    }
    if (outbh->vel_offset > 0) {
	memcpy (out_buf + outbh->vel_offset, input + bh->vel_offset, 
				bh->n_dop_bins * sizeof (Moment_t));
	memcpy (out_buf + outbh->spw_offset, input + bh->spw_offset, 
				bh->n_dop_bins * sizeof (Moment_t));
    }
    if (bh->surv_bin_size == 1000) {
	memcpy (out_buf + outbh->ref_offset, input + bh->ref_offset, 
				bh->n_surv_bins * sizeof (Moment_t));
	Log_action ("    Not 250m gate - Range recombination not needed");
	*length = bh->msg_len * sizeof (short);
	return (bh->status);
    }    

    R0 = .001f * (bh->range_beg_surv + 125.f);
    ATMOS = 0.001f * bh->atmos_atten;
    NOISE1 = bh->horiz_noise;
    NOISEc = NOISE2 = NOISE1;
    SYSCAL = bh->calib_const - NOISE1;
    THZ = 0.125f * bh->surv_snr_thresh;

    if (bh->azi_num == 1)
	LE_send_msg (0, 
	    "Range Recomb (e %d): SYSCAL %4.2f, ATMOS %4.2f, NOISE %4.2f, THZ %4.2f",
			bh->target_elev, SYSCAL, ATMOS, NOISE1, THZ);

    /* calculate number of reflectivity gates for recombination */
    ns = bh->n_surv_bins;
    if (ns > BASEDATA_REF_SIZE * 4)
	ns = BASEDATA_REF_SIZE * 4;
    REF_GATE_1000 = 0;
    ns = (ns / 4) * 4;

    Combine_range (ns, (Moment_t *)(input + bh->ref_offset), 
			(Moment_t *)(out_buf + outbh->ref_offset));

    outbh->n_surv_bins = ns / 4;
    outbh->surv_bin_size = 1000;
    outbh->msg_type |= RECOMBINED_TYPE;
    outbh->msg_type &= ~DUALPOL_TYPE;
    *output = out_buf;
    *length = outbh->msg_len * sizeof (short);

    return (outbh->status);
}

/***************************************************************************

    Sets fields start_angle, delta_angle and status in "rad". We use the 
    same code as that used in pbd to give consistant result.

***************************************************************************/

static void Set_start_angle (Base_data_header *rad) {
    static float st_ang, end_ang;	/* angles of starting and ending 
					   edges of this radial */
    static float scan_angle = 0.;	/* accumulate azimuth scan angle 
					   in this elevation */
    float half_azimuth_spacing;
    int idelta;
    float delta;

    half_azimuth_spacing = 0.5f;
    if (rad->azi_num == 1)
	st_ang = rad->azimuth - half_azimuth_spacing;
    else
	st_ang = end_ang;

    /* Check for wrap around 360 degrees */
    if (st_ang < 0.f)
	st_ang += 360.f;
    else if (st_ang >= 360.f)
        st_ang -= 360.f;
    rad->start_angle = roundf (st_ang * 10.0f);

    end_ang = rad->azimuth + half_azimuth_spacing;
    delta = end_ang - st_ang;
    idelta = roundf (end_ang * 10.0f) - rad->start_angle;

    /* Check for warp around 360 degrees */
    if( delta < 0.f ){	
	delta += 360.f;
	idelta += 3600;
    }
    rad->delta_angle = idelta;

    /* According to the RPG to Class 1 ICD, start angle must
       be in the range [0, 3599]. */
    if( rad->start_angle >= 3600 )
       rad->start_angle -= 3600;

    /* If almost more than 360 degrees are scaned, set pseudo end of 
       elevation or volume */
    if (rad->azi_num == 1)
	scan_angle = delta;
    else {

        if (delta <= 2.f) {

	    scan_angle += delta;
    	    if (scan_angle >= 360.f &&
	        rad->status == GOODINT) {

	        if (rad->elev_num == ORPGVCP_get_num_elevations( rad->vcp_num )){

	            /* The last elevation */
	            rad->status = PGENDVOL;
	            LE_send_msg( GL_INFO, "Marking Azi#/El# %d Pseudo EOV\n",
		  	         rad->azi_num, rad->elev_num );

	        }
	        else{

	            rad->status = PGENDEL;
	            LE_send_msg( GL_INFO, "Marking Azi#/El# %d/%d Pseudo EOE\n",
		  	         rad->azi_num, rad->elev_num );

	        }
	        scan_angle = 0.;

	    }
	}
    }
}

/***************************************************************************

    Returns the pointer to the reflectivity field to be used for Doppler
    recombination. We look for a reflectivity field in the additional
    moments in the RPG basedata radial. If it is found and matches the
    reflectivity field in the basic moments, it is used as the
    reflectivity field for Doppler recombination. Otherwise, the basic
    reflectivity is used. With normal operational VCPs, we should always
    have the additional reflectivity fields for Doppler recombination.
    The data in the additional reflectivity field must be converted to the
    momen_t type. The first gate must be aligned with that of the Doppler
    fields. Buffer "buf", a STR pointer, is used for storing the converted
    reflectivity field.

***************************************************************************/

static Moment_t *Find_z_for_dopp_recomb (char *rad, char **buf) {
    Base_data_header *bh;
    int i;

    bh = (Base_data_header *)rad;
    for (i = 0; i < bh->no_moments; i++) {
	Generic_moment_t *gm;

	gm = (Generic_moment_t *)(rad + bh->offsets[i]);
	if (strncmp (gm->name, "DRF2", 4) == 0) {
	    Moment_t *p;
	    unsigned char *pin;
	    int n_missing_gates, n_data, k;

	    if (gm->bin_size <= 0 ||
		gm->bin_size != bh->surv_bin_size) {
		LE_send_msg (0, 
		    "Unexpected DRF2 bin size (%d, %d) - not used", 
					gm->bin_size, bh->surv_bin_size);
		break;
	    }
	    n_missing_gates = 
		(gm->first_gate_range - bh->range_beg_surv) / gm->bin_size;
	    if (n_missing_gates < 0) {
		LE_send_msg (0, 
		    "Unexpected DRF2 first_gate_range (%d, %d) - not used", 
			gm->first_gate_range, bh->range_beg_surv);
		break;
	    }
	    if (gm->no_of_gates + n_missing_gates < bh->n_dop_bins) {
		LE_send_msg (0, 
		    "Unexpected DRF2 number of gates (%d, %d, %d) - not used", 
			gm->no_of_gates, n_missing_gates, bh->n_dop_bins);
		break;
	    }
	    if (gm->data_word_size != 8) {	/* must be byte data */
		LE_send_msg (0, 
		    "DRF2 data_word_size (%d) is not 8 - not used", 
			gm->data_word_size);
		break;
	    }
	    if (bh->azi_num == 1)
		LE_send_msg (0, "Use reflectivity in DOP scan\n");

	    *buf = STR_reset (*buf, sizeof (Moment_t) * bh->n_dop_bins);
	    p = (Moment_t *)(*buf);
	    for (k = 0; k < n_missing_gates; k++)
		p[k] = 0;
	    p += n_missing_gates;
	    n_data = bh->n_dop_bins - n_missing_gates;
	    pin = (unsigned char *)(&(gm->gate));
	    for (k = 0; k < n_data; k++)
		p[k] = pin[k];
	    return ((Moment_t *)(*buf));
	}
    }
    return ((Moment_t *)(rad + bh->ref_offset));
}

/***************************************************************************

    Combines two radials z1, v1, w1, z2, v2, and w2. dz1 and dz2 are the
    reflectivity field of the two for Doppler recombination. The array
    sizes of z1, z2, dz1 and dz2 are ns. z and dz must have the same ns
    and gate size. The array sizes of the Doppler fields are nd. nd must
    be <= ns. ns and nd must be divisible by 4. The results are put in
    zr, vr and wr. The sizes of the output buffers must be at least ns /
    4, nd and nd respectively. Global R0, VELRESOL, SYSCAL, ATMOS,
    NOISE, VNYQ, REF_GATE_1000, THZ, THV and THW are used. These
    variables must be set before calling this function. These variables
    must be the the same for the two radials. Doppler gate size must be
    .25 KM. z gate size must be .25 KM if REF_GATE_1000 = 0, or 1 KM if
    REF_GATE_1000 = 1.

    If REF_GATE_1000, z is assumed to be already in 1000 m gate. Range
    recombination for z is not preformed. ns is assumed to be 4 * number
    of gates.

***************************************************************************/

static void Combine_radials (int ns, int nd,
			     Moment_t *z1, Moment_t *z2,
			     Moment_t *v1, Moment_t *v2,
			     Moment_t *w1, Moment_t *w2,
			     Moment_t *dz1, Moment_t *dz2,
		Moment_t *zr, Moment_t *vr, Moment_t *wr) {
    int i;
    float TZ, TV, TW, n_avr;

    if (REF_GATE_1000)		/* z is already in 1000 gate size */
	n_avr = 2.f;		/* number of gates to average */
    else
	n_avr = 8.f;

    /* initialize Get_z and update tables Range_corr_? */
    if (REF_GATE_1000)
	Get_z (ns, GP_INIT_LONG_GATE, 0);
    else
	Get_z (ns, GP_INIT_SHORT_GATE, 0);

    TZ = exp10f ((NOISEc + THZ) * .1f);
    TV = exp10f ((NOISEc + THV) * .1f);
    TW = exp10f ((NOISEc + THW) * .1f);

    for (i = 0; i < ns; i += 4) {
	int j, m;
	float ZNUM;

	/* Initialize recombined power accumulator */
	ZNUM = 0;

	/* output index */
	m = i / 4;

	/* For each 250-m subgate */
	for (j = 0; j < 4; j++) {
	    float Z1NUM, Z2NUM;
	    int k, s;

	    /* Actual index */
	    k = i + j;

	    /* Get reflectivity for each radial */
	    if (REF_GATE_1000)
		s = m;				/* index for z and dz */
	    else
		s = k;
	    if (!REF_GATE_1000 || j == 0) {
		ZNUM += Get_z (z1[s], s, 1);		/* Accumulate z */
		if (z2 != NULL) {
		    Z1NUM = Get_z (dz1[s], s, 1);
		    Z2NUM = Get_z (dz2[s], s, 2);
		    ZNUM += Get_z (z2[s], s, 2);	/* Accumulate z */
		}
	    }

	    if (z2 != NULL && i < nd)
		Combine_dops (v1[k], v2[k], Z1NUM, Z2NUM, w1[k], w2[k], 
				TV, TW, k, vr + k, wr + k);
	}

	/* Average accumulated powers */
	ZNUM = ZNUM / n_avr;

	/* Censor recombined reflectivity */
	if (ZNUM * Range_corr_l[m] < TZ)	/* Below the SNR threshold */
	    zr[m] = 0;
	else			/* Re-quantize recombined reflectivity */
	    zr[m] = Quantize_z (ZNUM);
    }			/* end of for i */
}

/***************************************************************************

    Combines two radials z1, v1, w1, z2, v2, and w2. dz1 and dz2 are the
    reflectivity field of the two for Doppler recombination. The array
    sizes of z1, z2, dz1 and dz2 are ns. z and dz must have the same ns
    and gate size. The array sizes of the Doppler fields are nd. nd must
    be <= ns. ns and nd must be divisible by 4. The results are put in
    zr, vr and wr. The sizes of the output buffers must be at least ns /
    4, nd and nd respectively. Global R0, VELRESOL, SYSCAL, ATMOS,
    NOISE, VNYQ, REF_GATE_1000, THZ, THV and THW are used. These
    variables must be set before calling this function. These variables
    must be the the same for the two radials. Doppler gate size must be
    .25 KM. z gate size must be .25 KM if REF_GATE_1000 = 0, or 1 KM if
    REF_GATE_1000 = 1.

    If REF_GATE_1000, z is assumed to be already in 1000 m gate. ns is 
    assumed to be 4 * number of gates.

***************************************************************************/

static void Combine_azi (int ns, int nd,
			     Moment_t *z1, Moment_t *z2,
			     Moment_t *v1, Moment_t *v2,
			     Moment_t *w1, Moment_t *w2,
			     Moment_t *dz1, Moment_t *dz2,
		Moment_t *zr, Moment_t *vr, Moment_t *wr) {
    int i;
    float TZ, TV, TW, *range_corr;

    TZ = exp10f ((NOISEc + THZ) * .1f);
    TV = exp10f ((NOISEc + THV) * .1f);
    TW = exp10f ((NOISEc + THW) * .1f);

    /* initialize Get_z, ns, range_corr and update tables Range_corr_? */
    if (REF_GATE_1000) {
	Get_z (ns, GP_INIT_LONG_GATE, 0);
	ns = ns >> 2;
	range_corr = Range_corr_l;
    }
    else {
	Get_z (ns, GP_INIT_SHORT_GATE, 0);
	range_corr = Range_corr_s;
    }

    /* combine reflectivity field */
    for (i = 0; i < ns; i++) {
	float ZNUM;

	ZNUM = (Get_z (z1[i], i, 1) + Get_z (z2[i], i, 2)) * .5f;

	/* Censor recombined reflectivity */
	if (ZNUM * range_corr[i] < TZ)	/* Below the SNR threshold */
	    zr[i] = 0;
	else			/* Re-quantize recombined reflectivity */
	    zr[i] = Quantize_z (ZNUM);
    }

    /* combine DOP fields */
    for (i = 0; i < nd; i++) {
	int s;
	float Z1NUM, Z2NUM;

	if (REF_GATE_1000)
	    s = i >> 2;				/* index in Get_z table */
	else
	    s = i;
	Z1NUM = Get_z (dz1[s], s, 1);
	Z2NUM = Get_z (dz2[s], s, 2);
	Combine_dops (v1[i], v2[i], Z1NUM, Z2NUM, w1[i], w2[i], 
				TV, TW, i, vr + i, wr + i);
    }
}

/***************************************************************************

    Combines the reflectivity field of radial z1. The array sizes of
    z1 is ns. ns must be divisible by 4. The results are put in zr.
    The sizes of the output buffers must be at least ns / 4. Global
    R0, VELRESOL, SYSCAL, ATMOS, NOISE, VNYQ, REF_GATE_1000, THZ, THV
    and THW are used. These variables must be set before calling this
    function. z gate size must be .25 KM.

***************************************************************************/

static void Combine_range (int ns, Moment_t *z1, Moment_t *zr) {
    int i;
    float TZ, ZNUM;

    Get_z (ns, GP_INIT_SHORT_GATE, 0);
			/* initialize Get_z and update tables Range_corr_? */

    TZ = exp10f ((NOISEc + THZ) * .1f);

    /* combine reflectivity field */
    ZNUM = 0.f;
    for (i = 0; i < ns; i++) {
	int m;

	ZNUM += Get_z (z1[i], i, 1);		/* Accumulate z */
	if ((i % 4) < 3)
	    continue;

	/* Censor recombined reflectivity */
	m = i >> 2;
	if (ZNUM * Range_corr_l[m] < TZ)	/* Below the SNR threshold */
	    zr[m] = 0;
	else			/* Re-quantize recombined reflectivity */
	    zr[m] = Quantize_z (ZNUM * .25f);
	ZNUM = 0.f;
    }
}

/**************************************************************************

    Quantizes reflectivity "zf" and returns the result.

**************************************************************************/

static int Quantize_z (float zf) {
    int z;

    if (zf <= 0.0001f)
	return (0);

    z = roundf (2.f * 10.f * log10f (zf) + 64.f) + 2;

    /* Make sure re-quantized reflectivity is in valid range */
    if (z < 2)
	z = 0;
    if (z > 255)
	z = 255;
    return (z);
}

/***********************************************************************

    Returns the reflectivity (z) of "dbz" (in ICD value) at range gate
    index of "rind":

	z = 10 ^ ( .1 * ( ICD_dbz * .5 - 33 ) )

    If "dbz" is not available (under threshold or range aliased), the
    reflectivity at the gate is assumed to be that corresponding to the
    received power of .7 of the threshold plus the system noise:

	z = .7 * exp10f ( (NOISE + THZ) * .1 ) / range_correction

    The function uses a lookup table to do the conversion. Global
    variables R0, SYSCAL, ATMOS, NOISE and THZ are used. If rind =
    GP_INITIALIZE (< 0), this function initializes/rebuilds, if
    necessary, the static variable and table. In the latter case, "dbz"
    is the number of reflectivity gates.

    which_radial = 1 is the first radial and 2 is the second radial.

***********************************************************************/

static float Get_z (int dbz, int rind, int which_radial) {
    static float *z_table = NULL;
    static float guessed_power[2], *range_cor;
    int ns, i;

    if (rind >= 0 && which_radial >= 1) {
	if (dbz <= 0)	/* Fill in censored gate with artificial value */
	    return (guessed_power[which_radial - 1] / range_cor[rind]);
	else {
	    if (dbz > 255)
		dbz = 255;
	    return (z_table[dbz]);
	}
    }

    /* initialize static variables and tables - called each radial */
    ns = dbz;
    Build_range_corr_table (ns);

    guessed_power[0] = noise_assumption * exp10f ((NOISE1 + THZ) * .1f);
    guessed_power[1] = noise_assumption * exp10f ((NOISE2 + THZ) * .1f);
    if (rind == GP_INIT_LONG_GATE)
	range_cor = Range_corr_l;
    else
	range_cor = Range_corr_s;

    if (z_table == NULL) {
	z_table = MISC_malloc (256 * sizeof (float));
	for (i = 0; i < 256; i++)
	    z_table[i] = exp10f (i * .05f - 3.3f);
    }
    return (0.f);
}

/***********************************************************************

    Builds range correction tables for both low (Range_corr_l) and super
    (Range_corr_s) resolution radials (power = z * Range_corr_?). The
    table value is

	10 ^ ( 0.1 * ( -syscal + R * atmos - 20 * log10 (R) ) ) 

    where R = R0 + 0.25 * i for low res and R = R0 + 1. * i + 0.375
    for super res. The tables depends on ns, R0, syscal and atmos. If
    non of the parameters has changed, the tables are not updated.

***********************************************************************/

static void Build_range_corr_table (int ns) {
    static int cr_ns = 0, buf_z = 0;
    static float r0 = NOT_INIT, atmos = NOT_INIT, syscal = NOT_INIT;
    float R;
    int i;

    if (ns <= cr_ns && R0 == r0 && SYSCAL == syscal && ATMOS == atmos)
	return;

    if (ns > buf_z) {
	if (Range_corr_s != NULL)
	    free (Range_corr_s);
	if (Range_corr_l != NULL)
	    free (Range_corr_l);
	Range_corr_s = MISC_malloc (ns * sizeof (float));
	Range_corr_l = MISC_malloc ((ns / 4) * sizeof (float));
	buf_z = ns;
    }

    for (i = 0; i < ns; i++) {
	R = R0 + 0.25f * i;
	Range_corr_s[i] = exp10f (0.1f * 
		    (-20.f * log10f (R) - SYSCAL + R * ATMOS));
    }
    for (i = 0; i < ns / 4; i++) {
	R = R0 + 1.f * i + 0.375f;
	Range_corr_l[i] = exp10f (0.1f * 
		    (-20.f * log10f (R) - SYSCAL + R * ATMOS));
    }

    cr_ns = ns;
    r0 = R0;
    atmos = ATMOS;
    syscal = SYSCAL;
}

/***************************************************************************

    Combines two gates for v and w. The v and w for the two gates, in
    ICD units, are "v1", "v2", "w1" and "w2". The z value for the two
    gates are "z1" and "z2". The combined v and w are returned via "v"
    and "w". Global VELRESOL and VNYQ are used. z1 and z2 are assumed to
    be non-negative. Values 0 and 1 of v1, v2, w1, w2 are
    under-threshold and range-aliased respectively. "vthr" and "wthr"
    are the power thresholds for v and w respectively. "rind" is the
    range index of the two gates.

    The original NSSL algorithm converts v and w to m/s and performs
    recombination sometimes even if they are 0 or 1 which makes the
    algorithm difficult to verify. For example, there can be a potential
    numerical error in combining w when one of v is 0 and another is 1.
    The output is not well-defined when one of the gates is under
    threshold and the other is range aliased.

    This is the ROC improved version. It converts v and w to m/s and
    conducts recombination only when they are good data (> 1) which
    eliminates the weakness of the NSSL algorithm. This routine is also
    more efficient because it does not convert and process the data when
    one of the data is not available (0 or 1) which happens with high
    probability practically.

***************************************************************************/

static void Combine_dops (Moment_t v1, Moment_t v2, 
		float z1, float z2, Moment_t w1, Moment_t w2, 
		float vthr, float wthr, int rind, 
		Moment_t *v, Moment_t *w) {
    float V1NUM, V2NUM, VRNUM, V1DIFF, V2DIFF;
    float W1NUM, W2NUM, WRNUM;
    int need_comb_v, need_comb_w, icd;

    need_comb_v = 0;
    if (v1 == 0 && v2 == 0)		/* both are under threshold */
	*v = 0;
    else if (v1 != 1 && v2 != 1 &&
			(z1 + z2) * .5 * Range_corr_s[rind] < vthr)
	*v = 0;		/* The average power is under threshold */
    else if (v1 > 1 && v2 > 1)		/* both data are good */
	need_comb_v = 1;
    else if (v2 > 1)			/* only v2 is good */
	*v = v2;
    else if (v1 > 1)			/* only v1 is good */
	*v = v1;
    else		/* both are not good, at lease one is range aliased */
	*v = 1;

    /* apply the same logic to w */
    need_comb_w = 0;
    if (w1 == 0 && w2 == 0)
	*w = 0;
    else if (w1 != 1 && w2 != 1 &&
			(z1 + z2) * .5 * Range_corr_s[rind] < wthr)
	*w = 0;
    else if (w1 > 1 && w2 > 1)
	 need_comb_w = 1;
    else if (w2 > 1)
	*w = w2;
    else if (w1 > 1)
	*w = w1;
    else
	*w = 1;

    if (!need_comb_v && !need_comb_w)		/* done */
	return;

    if (need_comb_v) {		/* compute combined velocity */
	/* Depending on the velocity resolution, compute velocity in m/s. We
	   would not need this if we do not need V?DIFF. */
	if (VELRESOL == 1) {
	    V1NUM = v1 * .5f - 64.5f;
	    V2NUM = v2 * .5f - 64.5f;
	}
	else {
	    V1NUM = v1 - 129.f;
	    V2NUM = v2 - 129.f;
	}
    
	/* De-alias velocities */
/*
	if (V1NUM - V2NUM > VNYQ)
	    V2NUM = V2NUM + 2.f * VNYQ;
	if (V2NUM - V1NUM > VNYQ)
	    V1NUM = V1NUM + 2.f * VNYQ;
*/
    
	/* Compute power-weighted recombined velocity */
	VRNUM = (z1 * V1NUM + z2 * V2NUM) / (z1 + z2);
    	V1DIFF = V1NUM - VRNUM;
	V2DIFF = V2NUM - VRNUM;

	/* Re-alias recombined velocity */
/*
	if (VRNUM > VNYQ)
	    VRNUM = VRNUM - 2.f * VNYQ;
*/

	/* convert back to ICD value */
	if (VELRESOL == 1)
	    icd = roundf (2.f * VRNUM + 127.f) + 2;
	else
	    icd = roundf (VRNUM + 127.f) + 2;
	if (icd < 2)
	    icd = 2;
	if (icd > 255)
	    icd = 255;
	*v = icd;
    }
    else {			/* no differences available */
	V1DIFF = 0.f;
	V2DIFF = 0.f;
    }

    if (need_comb_w) {		/* compute combined sw */
	float MAX_SPW;

	/* Compute width in m/s */
	W1NUM = w1 * .5f - 64.5f;
	W2NUM = w2 * .5f - 64.5f;

	/* Compute power- and velocity-weighted recombined width */
	WRNUM = sqrtf ((z1 * (W1NUM * W1NUM + V1DIFF * V1DIFF) + 
		     z2 * (W2NUM * W2NUM + V2DIFF * V2DIFF)) / (z1 + z2));
	MAX_SPW = VNYQ * One_over_sqrt_3;
	if (WRNUM > MAX_SPW)
	    WRNUM = MAX_SPW;

	/* convert back to ICD value */
	icd = roundf (2.f * WRNUM + 127.f) + 2;
	if (icd < 2)
	    icd = 2;
	if (icd > 255)
	    icd = 255;
	*w = icd;
    }
}

