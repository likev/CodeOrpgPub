
/******************************************************************

    This module implements the pre-processing algorithm of the 
    dual-pol radar data fields.
	
******************************************************************/

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/08/15 19:40:53 $ */
/* $Id: dpp_process_zdr.c,v 1.1 2014/08/15 19:40:53 steves Exp $ */
/* $Revision:  */
/* $State: */

#include <rpgc.h>
#include "dpprep.h"
#include "findBragg.h"

static void Compute_snr (Dpp_params_t *params, DPP_d_t *ref, DPP_d_t *snr);
static int Compute_meteo_groups (int n_gates,
	char *metflag, unsigned short *metgpbx, unsigned short *metgpex);
static void Interpolate (Dpp_params_t *params, int w, 
		int mgtotal, unsigned short *metgpbx, unsigned short *metgpex, 
		DPP_d_t *in, DPP_d_t *out);
static void Compute_kdp (Dpp_params_t *params, int w, 
					DPP_d_t *phi, DPP_d_t *kdp);
#ifdef RPG_NOISE_CORRECTION
static void Compute_noise_corr_zdr_rho (Dpp_params_t *params, DPP_d_t snr, 
			DPP_d_t zdr_smd, DPP_d_t rho_smd, 
			DPP_d_t *zdr_prcd, DPP_d_t *rho_prcd);
#endif
static void Create_corrected_fields_and_adjust_kdp (Dpp_params_t *params, 
	DPP_d_t *snr, DPP_d_t *phi, DPP_d_t *zdr_smd, 
	DPP_d_t *ref_smd, DPP_d_t *rho_smd, DPP_d_t *phi_long_gate, 
	DPP_d_t *zdr_prcd, DPP_d_t *rho_prcd, 
	DPP_d_t *z_prcd, DPP_d_t *kdp9, DPP_d_t *kdp25);
static DPP_d_t Calculate_kdp (DPP_d_t *phi, int m, DPP_d_t g_size);
static DPP_d_t Calculate_lls_kdp (int w, DPP_d_t *in, DPP_d_t g_size);
static void Unfold_PhiDP (int num_bins, DPP_d_t init_fdp, 
			DPP_d_t *RhoHV, DPP_d_t *PhiDP);
static DPP_d_t Standard_deviation (int num, DPP_d_t *data);
static void Set_meteo_flag (int n_gates, DPP_d_t *field, DPP_d_t thresh,
				DPP_d_t *phi, char *metflag);
static int Is_high_atten_radial (Dpp_params_t *params, DPP_d_t *ref, 
		DPP_d_t *vel, DPP_d_t *spw, DPP_d_t *rho);


/******************************************************************

    This implements the dual-pol data pre-processing algorithm.
    "params" contains the processing parameters. The inputs fields are 
    "ref", "vel", "spw", "rho", "phi", "zdr". The output fields are 
    returned with "out". Returns 0 on success or -1 on failure.

    kdp9 and kdp25 are specific differential phases derived respectively 
    from 9 and 25 gates of the smoothed differential phase (-6 to 6 
    degrees/km). They are merged to become the KDP output.

******************************************************************/

int DPPP_process_data (Dpp_params_t *params, int elev_num, 
		DPP_d_t *ref, DPP_d_t *vel, DPP_d_t *spw,
		DPP_d_t *rho, DPP_d_t *phi, DPP_d_t *zdr,
		Dpp_out_fields_t *out, int *is_hatt) {
    static int buf_size = 0;
    static char *t_buf = NULL;
    DPP_d_t *ref_smd, *rho_smd, *zdr_smd, *phi_med, *phi_smd;
    DPP_d_t *phi_med_smd, *phi_short_gate, *phi_long_gate;
    char *metflag;
    unsigned short *metgpbx, *metgpex;
    DPP_d_t *ref_std, *phi_std, *vel_smd, *snr;
    DPP_d_t *zdr_prcd, *rho_prcd, *z_prcd, *kdp9, *kdp25;
    DPP_d_t *metsig;  /* PROTOTYPE CODE */
    int mgtotal, n_dgates, n_zgates, tsize, i;

    /* malloc temp arrays */
    n_dgates = params->n_dgates;
    n_zgates = params->n_zgates;
    tsize = n_zgates * sizeof (DPP_d_t) +
		n_dgates * (8 * sizeof (DPP_d_t) +    /* was 7, added one for metsig */
				sizeof (char) + 2 * sizeof (short));
    if (tsize > buf_size) {
	if (buf_size > 0)
	    free (t_buf);
	t_buf = (char *)MISC_malloc (tsize);
	buf_size = tsize;
    }

    ref_smd = (DPP_d_t *)t_buf;
    rho_smd = ref_smd + n_zgates;
    zdr_smd = rho_smd + n_dgates;
    phi_med = zdr_smd + n_dgates;
    phi_med_smd = phi_med + n_dgates;
    phi_short_gate = phi_med_smd + n_dgates;
    phi_smd = phi_short_gate + n_dgates;
    kdp25 = phi_smd + n_dgates;
    metsig  = kdp25 + n_dgates;  /* PROTOTYPE CODE */
    metgpbx = (unsigned short *)(metsig + n_dgates);
    metgpex = metgpbx + n_dgates;
    metflag = (char *)(metgpex + n_dgates);

    /* use output buffers */
    phi_long_gate = out->phi_long_gate;
    snr = out->snr;
    vel_smd = out->vh_smd;
    phi_std = out->sd_phi;
    ref_std = out->sd_zh;
    kdp9 = out->kdp;

    /* See if system initial PhiDP is suspect. */
    DPPC_calc_system_PhiDP(params->n_dgates, DPP_NO_DATA, elev_num, rho, phi, ref);

    /* If operator has enabled, adjust phi data by the estimate obtained from the */
    /* previous volume scan.                                                      */
    if (params->isdp_apply == 1 && params->isdp_est != -99) {

       for(i = 0; i < params->n_dgates; i++) {
          if(phi[i] <= DPP_NO_DATA) {
             continue;
          }
          else {
             /* we have a good phi */
             phi[i] -= params->isdp_est;
             phi[i] += params->init_fdp;

             /* Correct phi[i] to be between 0 and 360 */
 /*            check_phi(&(phi[i]));*/
          }
       }
    }

    Unfold_PhiDP (params->n_dgates, params->init_fdp, rho, phi);

    /* Compute SNR using unsmoothed reflectivity */
    Compute_snr (params, ref, snr);
    findBragg(params, ref, vel, spw, rho, zdr, phi, snr);

    /* dual-pol data pre-processing algorithm */
    DPPT_average_filter (params->n_zgates, params->window, ref, ref_smd);
    DPPT_std_filter (params->n_zgates, params->window, 
		(DPP_d_t)params->max_diff_dbz, ref, ref_smd, ref_std);
    DPPT_average_filter (params->n_dgates, params->short_gate, phi, phi_smd);
    DPPT_std_filter (params->n_dgates, params->short_gate, 
		(DPP_d_t)params->max_diff_phidp, phi, phi_smd, phi_std);

    DPPT_average_filter (params->n_dgates, params->window, rho, rho_smd);
    DPPT_average_filter (params->n_dgates, params->window, zdr, zdr_smd);
    DPPT_average_filter (params->n_vgates, params->window, vel, vel_smd);

    DPPT_average_filter (params->n_zgates, params->dbz_window, ref, ref_smd);

    memset (metflag, 0, params->n_dgates);
    *is_hatt = Is_high_atten_radial (params, ref, vel, spw, rho);

    /* PROTOTYPE CODE */
/*    findMetSignal (params->n_dgates, DPP_NO_DATA,
                   ref, vel, spw, rho, zdr, phi, is_hatt, metsig);*/
    /* END PROTOTYPE CODE */
 
    /* Recompute SNR, this time using smoothed reflectivity */
    Compute_snr (params, ref_smd, snr);

    if (*is_hatt) {
	int ngs = params->n_zgates;
	if (ngs > params->n_dgates)
	    ngs = params->n_dgates;
	Set_meteo_flag (ngs, snr, params->md_snr_thresh, phi, metflag);
    }
    else
	Set_meteo_flag (params->n_dgates, rho_smd, params->corr_thresh,
							phi, metflag);

    mgtotal = Compute_meteo_groups (params->n_dgates, 
					metflag, metgpbx, metgpex);

    DPPT_median_filter (params->n_dgates, params->window, phi, phi_med);

    for (i = 0; i < params->n_dgates; i++) {
	if (metflag[i] == 0)
	    phi_med[i] = DPP_NO_DATA;
    }

    DPPT_average_filter (params->n_dgates, params->short_gate, 
					phi_med, phi_med_smd);
    Interpolate (params, params->short_gate, mgtotal, metgpbx, metgpex, 
					phi_med_smd, phi_short_gate);

    DPPT_average_filter (params->n_dgates, params->long_gate, 
					phi_med, phi_med_smd);
    Interpolate (params, params->long_gate, mgtotal, metgpbx, metgpex, 
					phi_med_smd, phi_long_gate);

    Compute_kdp (params, params->short_gate, phi_short_gate, kdp9);
    Compute_kdp (params, params->long_gate, phi_long_gate, kdp25);

    /* compute zdr_prcd, rho_prcd and z_prcd. Adjust kdp9 and kdp25 */
    zdr_prcd = out->zdr_prcd;
    rho_prcd = out->rho_prcd;
    z_prcd = out->z_prcd;
    params->alpha = EXP10 (Cp1 * (params->n_h - params->n_v));
    Create_corrected_fields_and_adjust_kdp (params, snr, phi, 
		zdr_smd, ref_smd, rho_smd, phi_long_gate, 
		zdr_prcd, rho_prcd, z_prcd, kdp9, kdp25);

    return (0);
}

/******************************************************************

    Creates the signal-to-noise corrected z, zdr and rho and adjusts
    the Kdps based on the corrected fields. The output parameters are
    "zdr_prcd", "rho_prcd", "z_prcd", "kdp9" and "kdp25". Other
    parameters are inputs.
	
******************************************************************/

static void Create_corrected_fields_and_adjust_kdp (Dpp_params_t *params, 
	DPP_d_t *snr, DPP_d_t *phi, DPP_d_t *zdr_smd, 
	DPP_d_t *ref_smd, DPP_d_t *rho_smd, DPP_d_t *phi_long_gate, 
	DPP_d_t *zdr_prcd, DPP_d_t *rho_prcd, 
	DPP_d_t *z_prcd, DPP_d_t *kdp9, DPP_d_t *kdp25) {
    int i;

    for (i = 0; i < params->n_zgates; i++) {
	DPP_d_t delta_z;
	int ind;

	ind = params->zd_ind[i];
	if ((phi_long_gate[ind] <= DPP_NO_DATA) ||
            (phi_long_gate[ind] <  params->init_fdp)) {
	    delta_z = C0;
        }
	else {
	    delta_z = Cp04 * (phi_long_gate[ind] - params->init_fdp);
        }

	if (ref_smd[i] <= DPP_NO_DATA)
	    z_prcd[i] = DPP_NO_DATA;
	else
	    z_prcd[i] = ref_smd[i] + delta_z + params->z_syscal;
    }

    for (i = 0; i < params->n_dgates; i++) {
	DPP_d_t delta_zdr;
	int ind;

	ind = params->dz_ind[i];
#ifdef RPG_NOISE_CORRECTION
	if (snr[ind] <= DPP_NO_DATA) {
	    zdr_prcd[i] = zdr_smd[i];
	    rho_prcd[i] = rho_smd[i];
	}
	else {
	    if (snr[ind] >= params->zdr_thr || snr[ind] >= params->cor_thr)
		Compute_noise_corr_zdr_rho (params, snr[ind], 
			zdr_smd[i], rho_smd[i], zdr_prcd + i, rho_prcd + i);
	    if (snr[ind] < params->zdr_thr)
		zdr_smd[i] = DPP_NO_DATA;  /* zdr_prcd will be set later */
	    if (snr[ind] < params->cor_thr)
		rho_prcd[i] = DPP_NO_DATA;
	}
#else
	zdr_prcd[i] = zdr_smd[i];
	rho_prcd[i] = rho_smd[i];
#endif

	if ((phi_long_gate[i] <= DPP_NO_DATA) ||
            (phi_long_gate[i] <  params->init_fdp)) {
	    delta_zdr = C0;
        }
	else {
	    delta_zdr = Cp004 * (phi_long_gate[i] - params->init_fdp);
            }

	if (zdr_smd[i] <= DPP_NO_DATA)
	    zdr_prcd[i] = DPP_NO_DATA;
	else		/* note that here zdr_prcd cannot be NO DATA */
	    zdr_prcd[i] = zdr_prcd[i] + delta_zdr + params->zdr_syscal;

	if (rho_prcd[i] <= DPP_NO_DATA || 
#ifdef DPPREP_TEST
		(DPP_d_t)Round_signif_digits ((double)rho_prcd[i], 6) < params->corr_thresh) {
#else
		rho_prcd[i] < params->corr_thresh) {
#endif
	    kdp9[i] = DPP_NO_DATA;
	    kdp25[i] = DPP_NO_DATA;
	}
	if (z_prcd[ind] <= params->dbz_thresh)
	   kdp9[i] = kdp25[i];
    }
}

/******************************************************************

    Computes the signal-to-noise ratio based on reflectivity "ref" and
    the parameters in "params". The snr is put in "snr" for output.
    A range correction table is built to improve the performance.
	
******************************************************************/

static void Compute_snr (Dpp_params_t *params, DPP_d_t *ref, DPP_d_t *snr) {
    static int tbl_size = 0;
    static DPP_d_t *range_corr = NULL, atmos = C0;
    DPP_d_t dbz0;
    int n_gates, i;

    /* update the table if necessary */
    n_gates = params->n_zgates;
    if (params->atmos != atmos || n_gates > tbl_size) {
	if (n_gates > tbl_size) {
	    if (tbl_size > 0)
		free (range_corr);
	    range_corr = (DPP_d_t *)MISC_malloc (n_gates * sizeof (DPP_d_t));
	    tbl_size = n_gates;
	}
	for (i = 0; i < n_gates; i++) {
	    DPP_d_t r = params->zr0 + (DPP_d_t)i * params->zg_size;
	    if (r <= C0)
		r = Csmall;
	    range_corr[i] = (DPP_d_t)(-20) * LOG10 (r) + params->atmos * r;
	}
	atmos = params->atmos;
    }

    dbz0 = params->dbz0;
    for (i = 0; i < n_gates; i++) {
	if (ref[i] <= DPP_NO_DATA)
	    snr[i] = DPP_NO_DATA;
	else
	    snr[i] = ref[i] + range_corr[i] - dbz0;
    }
}

/******************************************************************

    Returns true if the radial is a high_attenuation-present-radial,
    or 0 otherwise.
	
******************************************************************/

static int Is_high_atten_radial (Dpp_params_t *params, DPP_d_t *ref, 
			DPP_d_t *vel, DPP_d_t *spw, DPP_d_t *rho) {
    int n, count, i;

    n = params->n_dgates;
    if (params->n_zgates < n)
	n = params->n_zgates;
    if (params->n_vgates < n)
	n = params->n_vgates;
    count = 0;
    for (i = params->art_start_bin; i < n; i++) {
	if (ref[i] > DPP_NO_DATA && ref[i] >= params->art_min_z && 
		ref[i] <= params->art_max_z) {	/* data has strong signal */
	    if (vel[i] > DPP_NO_DATA && fabs (vel[i]) >= params->art_v &&
					/* data is in an area without AP */
		rho[i] > DPP_NO_DATA && rho[i] <= params->art_corr &&
		spw[i] > params->art_min_sw)  /* SW check added so sea clutter is not tagged as NBF */
			/* probably an error with such a strong signal */
		count++;
	}
    }
    if (count > params->art_count)
	return (1);
    return (0);
}

/******************************************************************

    Sets metio flag "metflag", for the meteorological gates, based 
    on data field "field" and threshold "thresh". "metflag" must have
    at least n_gates elements.
	
******************************************************************/

static void Set_meteo_flag (int n_gates, DPP_d_t *field, DPP_d_t thresh,
				DPP_d_t *phi, char *metflag) {
    int i;

    for (i = 0; i < n_gates; i++) {
#ifdef DPPREP_TEST
	DPP_d_t fldv = Round_signif_digits ((double)field[i], 6);
#else
	DPP_d_t fldv = field[i];
#endif

	if (fldv <= DPP_NO_DATA || fldv < thresh || phi[i] <= DPP_NO_DATA)
	    metflag[i] = 0;
	else
	    metflag[i] = 1;
    }
}

/******************************************************************

    Computes, based on meteo flag "metflag", metgpbx and metgpex which
    identify the meteorological gate groups. These arrays must have at 
    least n_gates elements. Returns the number of meteorological gate
    groups identified.
	
******************************************************************/

static int Compute_meteo_groups (int n_gates, 
	char *metflag, unsigned short *metgpbx, unsigned short *metgpex) {
    int i, mgt, to_finish;

    mgt = 0;
    to_finish = 0;
    if (metflag[0] == 1) {
	metgpbx[mgt] = 0;
	to_finish = 1;
    }
    for (i = 1; i < n_gates; i++) {
	if (metflag[i - 1] == 0 && metflag[i] == 1) {
	    metgpbx[mgt] = i;
	    to_finish = 1;
	}
	else if (metflag[i - 1] == 1 && metflag[i] == 0) {
	    metgpex[mgt] = i - 1;
	    to_finish = 0;
	    mgt++;
	}
    }
    if (to_finish) {
	metgpex[mgt] = n_gates - 1;
	mgt++;
    }

    return (mgt);
}

/**************************************************************************

    Fills in gaps between valid MGs with interpolated data on field
    "in". The output is returned with "out". "params" is the parameter
    set. The window size if "w". "mgtotal", "metgpbx" and "metgpex" are
    meteorological gate groups.

***************************************************************************/

static void Interpolate (Dpp_params_t *params, int w, 
		int mgtotal, unsigned short *metgpbx, unsigned short *metgpex, 
		DPP_d_t *in, DPP_d_t *out) {
    int j, n, hw, cnt, pre_vmgind, mgsize;

    n = params->n_dgates;
    hw = w / 2;
    memcpy (out, in, n * sizeof (DPP_d_t));
    cnt = 0;			/* valid MG count */
    pre_vmgind = 0;		/* index of the previous valid MG */
    for (j = 0; j <= mgtotal; j++) {
	int begbin, endbin;
	DPP_d_t begphidp, endphidp, slope;

	if (j < mgtotal) {	/* process segment before cnt-th valid MG */
	    mgsize = metgpex[j] - metgpbx[j] + 1;	/* group size */
	    if (mgsize < w)			/* not a valid meteo group */
		continue;

	    if (cnt == 0) {		/* first meteo group */
		if (metgpbx[j] == 0) {	/* radial starts with MG, do nothing */
		    begbin = 0;
		    endbin = 0;
		    begphidp = endphidp = 0;	/* useless - to mute gcc */
		}
		else {			/* first part of radial is not MG */
		    begbin = 0;
		    endbin = metgpbx[j] + hw;
		    begphidp = params->init_fdp;
		    endphidp = in[endbin];
		}
	    }
	    else {			/* meteo groups other than the first */
		begbin = metgpex[pre_vmgind] - hw;
		endbin = metgpbx[j] + hw;
		begphidp = in[begbin];
		endphidp = in[endbin];
	    }
	}
	else {			/* process segment after the last valid MG */
	    if (cnt == 0)
		break;
	    begbin = metgpex[pre_vmgind] - hw;
	    endbin = n - 1;
	    /* Set to in[begbin] or out[begbin]? Should be no difference. If,
	       however, out[begbin] is not generated by interp. or modified 
	       after interp., setting to "out" is more robust. */
	    begphidp = endphidp = out[begbin];
	}
	if (endbin - begbin > 0) {
	    int i;
	    slope = (endphidp - begphidp) / (DPP_d_t)(endbin - begbin);
/*        if (slope < 0 && w == 25) {
          fprintf(stderr,"Negative Slope. beg phi= %f end phi= %f beg bin= %d end bin= %d\n",
                                  begphidp, endphidp, begbin, endbin);
         }*/

	    for (i = begbin; i <= endbin; i++)
		out[i] = slope * (DPP_d_t)(i - begbin) + begphidp;
	}
	pre_vmgind = j;
	cnt++;
    }

    if (cnt == 0) {			/* no valid meteo group */
	int i;
	for (i = 0; i < n; i++)
	    out[i] = params->init_fdp;
    }
}

/**************************************************************************

    Computes Kdp from Phi (the differetial phase). "w" is the window size.
    "params" is the parameter set. "w" must be an odd number and >= 5.

***************************************************************************/

static void Compute_kdp (Dpp_params_t *params, int w, 
					DPP_d_t *phi, DPP_d_t *kdp) {
    int hw, n, b1, b2, i;

    if (w < 5 || (w % 2) == 0) {
	LE_send_msg (0, "Error - Bad window (%d) for Compute_kdp\n", w);
	return;
    }

    n = params->n_dgates;
    hw = w / 2;
    b1 = hw;
    b2 = n - hw;
    for (i = 0; i < n; i++) {

	if (i > b1 && i < b2) {		/* efficient in the middle of array */
	    kdp[i] = Calculate_kdp (phi + (i - hw), w, params->dg_size);
	}
	else {				/* near the boundaries of array */
	    int cnt, j;
	    DPP_d_t buf[MAX_WIN];

	    cnt = 0;
	    for (j = -hw; j <= hw; j++) {
		int k = i + j;
		if (k >= 0 && k < n) {
		    buf[cnt] = phi[k];
		    cnt++;
		}
	    }
	    if ((cnt % 2) == 0)
		kdp[i] = Calculate_lls_kdp (cnt, buf, params->dg_size);
	    else 		/* cnt is an odd number */
		kdp[i] = Calculate_kdp (buf, cnt, params->dg_size);
	}
    }
}

#ifdef RPG_NOISE_CORRECTION

/**************************************************************************

    Calculate noise corrected zdr and rho. "snr" must not be DPP_NO_DATA.

***************************************************************************/

static void Compute_noise_corr_zdr_rho (Dpp_params_t *params, DPP_d_t snr, 
			DPP_d_t zdr_smd, DPP_d_t rho_smd, 
			DPP_d_t *zdr_prcd, DPP_d_t *rho_prcd) {
    DPP_d_t alpha, zdr_temp1, snr_linear, denom, zdr_temp2;

    if (zdr_smd <= DPP_NO_DATA) {
	*zdr_prcd = DPP_NO_DATA;
	*rho_prcd = DPP_NO_DATA;
	return;
    }
    alpha = params->alpha;
    zdr_temp1 = EXP10 (Cp1 * zdr_smd);
    snr_linear = EXP10 (Cp1 * snr);
    denom = alpha * snr_linear + alpha - zdr_temp1;
    if (denom < Cp01)
	denom = Cp01;
    zdr_temp2 = alpha * snr_linear * zdr_temp1 / denom;
    if (zdr_temp2 <= C0)
	zdr_temp2 = Csmall;
    *zdr_prcd = C10 * LOG10 (zdr_temp2);
    if (rho_smd <= DPP_NO_DATA)
	*rho_prcd = DPP_NO_DATA;
    else
	*rho_prcd = rho_smd * SQRT ((C1 + C1 / snr_linear) * 
			(C1 + zdr_temp2 / (snr_linear * alpha)));
}
#endif

/**************************************************************************

    Calculate Kdp from Phi. Phi, being interpolated, has no NO DATA in it.
    "m" is the size of "phi" and "g_size" is the gate size in Km. It returns
    Kdp estimated with the least mean square method.

***************************************************************************/

static DPP_d_t Calculate_kdp (DPP_d_t *phi, int m, DPP_d_t g_size) {
    static int prev_m = 0;
    static DPP_d_t factor = C0, st_r = C0;
    DPP_d_t j, sum, kdp;
    int i;

    /* The following is correct only if m is odd and >= 3 */
    if (m != prev_m) {
	factor = (DPP_d_t)6 / (g_size * (DPP_d_t)(m * (m - 1) * (m + 1)));
	st_r = -((m - 1) / 2);
	prev_m = m;
    }
    sum = C0;
    j = st_r;
    for (i = 0; i < m; i++) {
	sum += j * phi[i];
	j += C1;
    }
    kdp = factor * sum;

    return (kdp);
}

/**************************************************************************

    Calculate Kdp from Phi. Phi, being interpolated, has no NO DATA in it.
    "m" is the size of "phi" and "g_size" is the gate size in Km. It returns
    Kdp estimated with the least mean square method. This returns the same
    result as Calculate_kdp, but without the limitation that w must be odd.

***************************************************************************/

static DPP_d_t Calculate_lls_kdp (int w, DPP_d_t *in, DPP_d_t g_size) {
    int j;
    DPP_d_t sx, sy, sxx, sxy, x, y, hw, nd;

    if (w <= 1)
	return DPP_NO_DATA;
    sx = sy = sxx = sxy = C0;
    hw = (w - 1) * (DPP_d_t).5;
    for (j = 0; j < w; j++) {
	x = ((DPP_d_t)j - hw) * g_size;
	y = in[j];
	sx += x;
	sy += y;
	sxy += y * x;
	sxx += x * x;
    }
    nd = (DPP_d_t)w;
    return (DPP_d_t).5 * (nd * sxy - sx * sy) / (nd * sxx - sx * sx);
}

/*************************************************************************

    Unfolds PHI. The unfolded PHI replaces the input "PhiDP". The input 
    array size of "RhoHV" and "PhiDP" is "num_bins". "init_fdp" is the
    system PhiDP at range 0. This routine does work if the data is
    folded more than once.

*************************************************************************/

static void Unfold_PhiDP (int num_bins, DPP_d_t init_fdp, 
					DPP_d_t *RhoHV, DPP_d_t *PhiDP) {
    int max_hist_median_size, hist_median_thresh;
    int min_valid_data, min_bin_unfold_start, count;
    DPP_d_t min_rhv_thresh, max_stddev, half_fold_in_degrees;
    DPP_d_t historical_median, fold_in_degrees;
    static DPP_d_t *unfolded_PhiDP = NULL;
    static DPP_d_t *historical_median_vector = NULL;
    static int buf_size = 0;
    int valid_data, i;

    /* init constants */
    fold_in_degrees = 360.;
    min_rhv_thresh = 0.85;
    max_hist_median_size = 30;
    hist_median_thresh = 25;
    max_stddev = fold_in_degrees / 3.0;
    min_valid_data = 15;
    half_fold_in_degrees = fold_in_degrees / 2.0;
    min_bin_unfold_start = 240; /* 60 km assuming 0.25 km bin size */

    /* prepare buffers */
    if (num_bins > buf_size) {
	if (unfolded_PhiDP != NULL)
	    free (unfolded_PhiDP);
	unfolded_PhiDP = (DPP_d_t *)MISC_malloc (num_bins * sizeof (DPP_d_t));
	if (historical_median_vector != NULL)
	    free (historical_median_vector);
	historical_median_vector = (DPP_d_t *)MISC_malloc 
					(num_bins * sizeof (DPP_d_t));
	buf_size = num_bins;
    }

    /* init variables */
    valid_data = 0;
    count = 0;
    historical_median = init_fdp;

    for (i = 0; i < num_bins; i++) {	/* for each bin */
	int unfold_flag;
	DPP_d_t phi_diff, phi_single_fold, phi_double_fold;

	unfold_flag = 0;
	if (PhiDP[i] <= DPP_NO_DATA) {
	    unfolded_PhiDP[i] = PhiDP[i]; 
	    continue;
	}

	if (RhoHV[i] > DPP_NO_DATA && RhoHV[i] >= min_rhv_thresh) {
	    valid_data++;
	}

	if (i >= max_hist_median_size) {
	    int v_size, j;
	    DPP_d_t stddev;

	    count = 0;
	    v_size = 0;
	    for (j = 1; j <= max_hist_median_size; j++) {
	        if (RhoHV[i - j] >= min_rhv_thresh && 
		    unfolded_PhiDP[i - j] > DPP_NO_DATA) {
		    historical_median_vector[v_size] = unfolded_PhiDP[i - j];
		    v_size++;
		    count++;
	        }
	    }
	   
	    if (count > hist_median_thresh) {
		stddev = Standard_deviation (v_size, historical_median_vector);
		if (stddev < max_stddev)
		    historical_median = 
			DPPT_med_filter (historical_median_vector, v_size);
	    }
	}

	phi_diff = PhiDP[i] - historical_median;
	if (phi_diff < C0)
	    phi_diff = -phi_diff;

	phi_double_fold = C0;	/* useless - for disabling gcc warning */
	if (phi_diff >= half_fold_in_degrees &&
	    valid_data > min_valid_data &&
	    i > min_bin_unfold_start) {
	    DPP_d_t single_fold_diff, double_fold_diff;

	    phi_single_fold = PhiDP[i] + fold_in_degrees;
	    phi_double_fold = PhiDP[i] + 2 * fold_in_degrees;
	
	    single_fold_diff = historical_median - phi_single_fold;
	    if (single_fold_diff < C0)
		single_fold_diff = -single_fold_diff;
	    double_fold_diff = historical_median - phi_double_fold;
	    if (double_fold_diff < C0)
		double_fold_diff = -double_fold_diff;
	
	    if (phi_diff > single_fold_diff) {
		unfold_flag = 1;
	    }
	    if (single_fold_diff > double_fold_diff) {
		unfold_flag = 2;
	    }
	}
      
	if (unfold_flag == 1) {
	    unfolded_PhiDP[i] = phi_single_fold;
	}
	else if (unfold_flag == 2) {
	    unfolded_PhiDP[i] = phi_double_fold;
	}
	else {
	    unfolded_PhiDP[i] = PhiDP[i];
	}
    }

    /* put result in PhiDP for output */
    memcpy (PhiDP, unfolded_PhiDP, num_bins * sizeof (DPP_d_t));

}

/*************************************************************************

    Returns the STD of array "data" of "num" elements.

*************************************************************************/

static DPP_d_t Standard_deviation (int num, DPP_d_t *data) {
    DPP_d_t sum, sq, d;
    int i;

    if (num <= 1)
	return (C0);
    sum = sq = C0;
    for (i = 0; i < num; i++) {
	DPP_d_t d = data[i];
	sum += d;
	sq += d * d;
    }
    d = sum / num;
    d = (sq - d * d * num) / (num - 1);		/* Non-biased STD */
    if (d < C0)
	d = C0;
    return (SQRT (d));
}
