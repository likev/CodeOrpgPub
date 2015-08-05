
/******************************************************************

    This is the radial data formating module for the dual-pol radar 
    data preprocesing program.
	
******************************************************************/

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/06/23 19:30:24 $ */
/* $Id: dpp_format.c,v 1.27 2014/06/23 19:30:24 steves Exp $ */
/* $Revision:  */
/* $State: */

#include <rpgc.h>
#include "dpprep.h"
#include <dpprep_isdp.h>

extern float RDA_system_phidp;
extern Dpp_isdp_est_t Isdp_est;

enum {DF_REF, DF_VEL, DF_SPW, DF_ZDR, DF_PHI, DF_RHO};
				/* input data fields */
#define N_DF 6			/* number of input data fields */
static char *F_name[] = {"DREF", "DVEL", "DSPW", "DZDR", "DPHI", "DRHO"};
				/* input data field names */
static int Dop_resolution;	/* Velocity data resolution (RPG basedata) */

static Generic_moment_t *Zdr_hd, *Phi_hd, *Rho_hd;
static float Z_offset, V_offset, SW_offset, Z_scale, V_scale, SW_scale;

static void Get_rpg_adapt_data (Dpp_params_t *params, Base_data_header *bh);
static int Get_data_fields (Dpp_params_t *params, Base_data_header *bh, 
			DPP_d_t **ref, DPP_d_t **vel, DPP_d_t **spw,
			DPP_d_t **rho, DPP_d_t **phi, DPP_d_t **zdr);
static int Verify_gm_hd (Base_data_header *bh, Generic_moment_t *gm, 
						int field);
static void Icd_to_intern (int field, int n_gates, void *inp, DPP_d_t *out);
static int Format_output_radial (Base_data_header *bh, Dpp_params_t *params,
		Dpp_out_fields_t *out, char **out_radial, DPP_d_t *ref,
		DPP_d_t *vel, DPP_d_t *rho, DPP_d_t *phi, DPP_d_t *zdr);
static void Add_moment (char *buf, char *name, char f, Dpp_params_t *params, 
	int word_size, unsigned short tover, short SNR_threshold, 
	float scale, float offset, DPP_d_t *data, DPP_d_t *inp);
static void Print_unique_msg (char *msg);
static int *Create_a_index (int igates, DPP_d_t ir0, DPP_d_t ig_s, 
			int ogates, DPP_d_t or0, DPP_d_t og_s);
static void Create_index_tables (Dpp_params_t *params);
static float Get_new_scale (float scale, float offset, float max_v, 
					float max_d, float *noffset);


/******************************************************************

    Processes the input radial "input" in the RPG basedata format
    "Base_data_header". The pointer to the dual-pol preprocessed
    output data is returned with "output". The size of the output
    data is returned with "length". If there is no radial to output,
    "length" is set to 0. The function's return value is the output
    redial_status flag, if *length > 0, or 0 otherwise. If there is
    not enough dual-pol field found in the input, the radial is
    returned without processing.
	
******************************************************************/

int DPPF_process_radial (char *input, char **output, int *length) {
    static int prev_zgs = 0, prev_vgs = 0, prev_dgs = 0; /* sizes of out */
    static Dpp_out_fields_t out;	/* buffer for processed fields */
    Base_data_header *bh;
    char *out_radial;
    DPP_d_t *ref, *vel, *spw, *rho, *phi, *zdr;
    static Dpp_params_t params;
    int is_ha;

    bh = (Base_data_header *)input;
    *output = input;
    *length = bh->msg_len * sizeof (short);
    if (bh->n_surv_bins == 0)			/* spot blanked radial */
	return (bh->status);
    if (bh->azi_num == 1) 
	Print_unique_msg ("");

    /* get parameters from the radial header */
    params.atmos = (DPP_d_t)(0.001) * bh->atmos_atten;
    params.z_syscal = C0;
    params.dbz0 = bh->calib_const;
    params.zdr_syscal = 0.0; /* This is already being applied at ORDA. */
    params.init_fdp = (DPP_d_t)(bh->sys_diff_phase);
    params.n_h = bh->horiz_noise;
    params.n_v = bh->vert_noise;
    Dop_resolution = bh->dop_resolution;

    /* The following parameters are used in the findBragg function */
    params.vcp_num = bh->vcp_num;
    params.status = bh->status;
    params.target_elev = bh->target_elev;
    params.zdr_time = bh->begin_vol_time;
    params.zdr_date = bh->begin_vol_date;

    /* Save the RDA's value for initial system PhiDP for status log output later */
    RDA_system_phidp = params.init_fdp;

    /* get RPG adaptation data */
    Get_rpg_adapt_data (&params, bh);

    /* extract and convert input radial data fields */
    if (Get_data_fields (&params, bh, &ref, &vel, &spw, &rho, &phi, &zdr) < 0)
	return (bh->status);

    if (params.n_zgates > prev_zgs) {	/* allocate buffers for output */
	if (prev_zgs > 0) {
	    free (out.z_prcd);
	    free (out.snr);
	    free (out.sd_zh);
	}
	out.z_prcd = (DPP_d_t *)MISC_malloc
			((params.n_zgates + 1) * sizeof (DPP_d_t));
	out.snr = (DPP_d_t *)MISC_malloc 
			((params.n_zgates + 1) * sizeof (DPP_d_t));
	out.sd_zh = (DPP_d_t *)MISC_malloc 
			(params.n_zgates * sizeof (DPP_d_t));
	out.snr[params.n_zgates] = DPP_NO_DATA;
	out.z_prcd[params.n_zgates] = DPP_NO_DATA;
	prev_zgs = params.n_zgates;
    }
    if (params.n_vgates > prev_vgs) {
	if (prev_vgs > 0) {
	    free (out.vh_smd);
	}
	out.vh_smd = (DPP_d_t *)MISC_malloc 
			(params.n_vgates * sizeof (DPP_d_t));
	prev_vgs = params.n_vgates;
    }
    if (params.n_dgates > prev_dgs) {
	if (prev_dgs > 0) {
	    free (out.rho_prcd);
	    free (out.phi_long_gate);
	    free (out.zdr_prcd);
	    free (out.kdp);
	    free (out.sd_phi);
	}
	out.rho_prcd = (DPP_d_t *)MISC_malloc 
				(params.n_dgates * sizeof (DPP_d_t));
	out.phi_long_gate = (DPP_d_t *)MISC_malloc
				((params.n_dgates + 1) * sizeof (DPP_d_t));
	out.zdr_prcd = (DPP_d_t *)MISC_malloc 
				(params.n_dgates * sizeof (DPP_d_t));
	out.kdp = (DPP_d_t *)MISC_malloc 
				(params.n_dgates * sizeof (DPP_d_t));
	out.sd_phi = (DPP_d_t *)MISC_malloc 
				(params.n_dgates * sizeof (DPP_d_t));
	out.phi_long_gate[params.n_dgates] = DPP_NO_DATA;
	prev_dgs = params.n_dgates;
    }

    if (bh->status == GOODBVOL)
	LE_send_msg (GL_INFO, "Params for high atten: snr_thr %f, start_bin %d, bin_count %d, min_z %f, max_z %f, v_thr %f, rho_thr %f min_sw %f",
	params.md_snr_thresh, params.art_start_bin, params.art_count, 
	params.art_min_z, params.art_max_z, params.art_v, params.art_corr, params.art_min_sw);

    /* perform pre-processing */
    if (DPPP_process_data (&params, bh->rpg_elev_ind, ref, vel, spw, rho, phi, zdr, &out, &is_ha) < 0)
	return (bh->status);

    *length = Format_output_radial (bh, &params, &out, &out_radial,
					ref, vel, rho, phi, zdr);
    *output = out_radial;
    bh = (Base_data_header *)out_radial;
    if (is_ha)
	bh->msg_type |= HIGH_ATTENUATION_TYPE;
    return (bh->status);
}

/******************************************************************

    Extracts, converts and returns all data fields from radial "bh".
    The data fields are converted to type DPP_d_t in physical units. 
    This function returns 0 on success or -1 on failure (e.g. not all
    required fields are found or gate sizes do not match).
	
******************************************************************/

static int Get_data_fields (Dpp_params_t *params, Base_data_header *bh, 
			DPP_d_t **ref, DPP_d_t **vel, DPP_d_t **spw,
			DPP_d_t **rho, DPP_d_t **phi, DPP_d_t **zdr) {
    static int buf_size = 0;
    static DPP_d_t *buf = NULL;
    int n_gates, cnt, i;
    int ngates, range, bin_size;

    params->n_zgates = bh->n_surv_bins;
    params->zg_size = (DPP_d_t)(.001) * bh->surv_bin_size;
    params->zr0 = (DPP_d_t)(.001) * bh->range_beg_surv + 
					(DPP_d_t)(.5) * params->zg_size;

    params->n_vgates = bh->n_dop_bins;
    params->vg_size = (DPP_d_t)(.001) * bh->dop_bin_size;
    params->vr0 = (DPP_d_t)(.001) * bh->range_beg_dop + 
					(DPP_d_t)(.5) * params->vg_size;

    ngates = range = bin_size = 0;	/* useless - to mute gcc */
    cnt = 0;
    for (i = 0; i < bh->no_moments; i++) {	/* get DP field gate info */
	Generic_moment_t *gm;

	gm = (Generic_moment_t *)((char *)bh + bh->offsets[i]);
	if (strncmp (gm->name, F_name[DF_ZDR], 4) == 0 ||
	    strncmp (gm->name, F_name[DF_PHI], 4) == 0 ||
	    strncmp (gm->name, F_name[DF_RHO], 4) == 0) {

	    if (cnt == 0) {
		ngates = gm->no_of_gates;
		range = gm->first_gate_range;
		bin_size = gm->bin_size;
	        params->dr0 = (DPP_d_t)(.001) * range;
	        params->n_dgates = ngates;
	        params->dg_size = (DPP_d_t)(.001) * bin_size;
	    }
	    else {
		if (gm->no_of_gates != ngates ||
		    gm->first_gate_range != range ||
		    gm->bin_size != bin_size ) {
		    LE_send_msg (GL_ERROR, 
				"Gates not the same for all DP fields");
		    return (-1);
		}
	    }
	    cnt++;
	}
    }
    if (cnt == 0) {
	Print_unique_msg ("No dual-pol field found - Output without processing");
	return (-1);
    }
    if (cnt < N_DF - 3) {
	LE_send_msg (GL_ERROR, "Fewer than expected Dual-pol fields found");
	return (-1);
    }
    if (cnt > N_DF - 3) {
	LE_send_msg (GL_ERROR, "More than expected Dual-pol fields found");
	return (-1);
    }
    Print_unique_msg ("Dual-pol fields found - Processing");

    Create_index_tables (params);

    /* allocate buffers for data fields */
    n_gates = params->n_zgates + 1 + (2 * (params->n_vgates + 1)) +
	      			cnt * (params->n_dgates + 1);
    if (n_gates > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = (DPP_d_t *)MISC_malloc (n_gates * sizeof (DPP_d_t));
	buf_size = n_gates;
    }
    *ref = buf;
    *vel = *ref + (params->n_zgates + 1);
    *spw = *vel + (params->n_vgates + 1);
    *zdr = *spw + (params->n_vgates + 1);
    *phi = *zdr + (params->n_dgates + 1);
    *rho = *phi + (params->n_dgates + 1);

    /* extract and convert dual-pol data fields */
    n_gates = params->n_dgates;
    for (i = 0; i < bh->no_moments; i++) {	/* get dual-pol fields */
	Generic_moment_t *gm;

	gm = (Generic_moment_t *)((char *)bh + bh->offsets[i]);
	if (strncmp (gm->name, F_name[DF_ZDR], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, DF_ZDR) < 0)
		return (-1);
	    params->zdr_thr = (DPP_d_t)gm->SNR_threshold * (DPP_d_t)(.125);
	    Zdr_hd = gm;
	    Icd_to_intern (DF_ZDR, n_gates, &(gm->gate), *zdr);
	    (*zdr)[n_gates] = DPP_NO_DATA;
	}
	else if (strncmp (gm->name, F_name[DF_PHI], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, DF_PHI) < 0)
		return (-1);
	    Phi_hd = gm;
	    Icd_to_intern (DF_PHI, n_gates, &(gm->gate), *phi);
	    (*phi)[n_gates] = DPP_NO_DATA;
	}
	else if (strncmp (gm->name, F_name[DF_RHO], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, DF_RHO) < 0)
		return (-1);
	    params->cor_thr = (DPP_d_t)gm->SNR_threshold * (DPP_d_t)(.125);
	    Rho_hd = gm;
	    Icd_to_intern (DF_RHO, n_gates, &(gm->gate), *rho);
	    (*rho)[n_gates] = DPP_NO_DATA;
	}
    }

    /* extract and convert basic moments */
    Icd_to_intern (DF_REF, params->n_zgates, 
			(char *)bh + bh->ref_offset, *ref);
    (*ref)[params->n_zgates] = DPP_NO_DATA;
    Icd_to_intern (DF_VEL, params->n_vgates, 
			(char *)bh + bh->vel_offset, *vel);
    (*vel)[params->n_vgates] = DPP_NO_DATA;
    Icd_to_intern (DF_SPW, params->n_vgates, 
			(char *)bh + bh->spw_offset, *spw);
    (*spw)[params->n_vgates] = DPP_NO_DATA;

    return (0);
}

/******************************************************************

    Creates tables dz_ind and zd_ind in "params".
	
******************************************************************/

static void Create_index_tables (Dpp_params_t *params) {
    static int *dz_ind = NULL;
    static int *zd_ind = NULL;
    static int prev_zgs = 0, prev_dgs = 0;

    if (params->n_zgates != prev_zgs || params->n_dgates != prev_dgs) {
	if (dz_ind != NULL)
	    free (dz_ind);
	if (zd_ind != NULL)
	    free (zd_ind);
	dz_ind = Create_a_index (params->n_dgates, params->dr0, 
	    params->dg_size, params->n_zgates, params->zr0, params->zg_size);
	zd_ind = Create_a_index (params->n_zgates, params->zr0, 
	    params->zg_size, params->n_dgates, params->dr0, params->dg_size);
	prev_zgs = params->n_zgates;
	prev_dgs = params->n_dgates;
    }
    params->dz_ind = dz_ind;
    params->zd_ind = zd_ind;
}

/******************************************************************

    Creates a index table. The input index parameters are igates, ir0
    and ig_s. The output index parameters are ogates, or0 and og_s.

******************************************************************/

static int *Create_a_index (int igates, DPP_d_t ir0, DPP_d_t ig_s, 
			int ogates, DPP_d_t or0, DPP_d_t og_s) {
    int *ind, i;

    ind = (int *)MISC_malloc (igates * sizeof (int));
    for (i = 0; i < igates; i++) {
	DPP_d_t d;
	int oi;

	d = ir0 + (DPP_d_t)i * ig_s;
	oi = (int)((d - or0) / og_s);
	if (d - (or0 + oi * og_s) > (DPP_d_t).5 * og_s)
	    oi++;
	if (oi < 0 || oi > ogates)
	    oi = ogates;
	ind[i] = oi;
    }
    return (ind);
}

/******************************************************************

    Prints "msg" if it not the same as the previous one.
	
******************************************************************/

static void Print_unique_msg (char *msg) {
    static char *prev = NULL;

    if (prev != NULL && strcmp (msg, prev) == 0)
	return;
    if (msg[0] != '\0')
	LE_send_msg (GL_INFO, "%s", msg);
    prev = STR_copy (prev, msg);
}

/******************************************************************

    Converts data array "inp" of "field" of array size "n_gates" to the
    physical unit and type of DPP_d_t and puts them in "out".
	
******************************************************************/

static void Icd_to_intern (int field, int n_gates, void *inp, DPP_d_t *out) {
    DPP_d_t scale, offset;
    int i, word_size;

    word_size = 8;
    scale = (DPP_d_t)1.;
    switch (field) {

	case DF_REF:
	    Z_scale = 2.f;
	    Z_offset = 66.f;
	    scale = Z_scale;
	    offset = Z_offset;
	    word_size = 16;
	    break;
	case DF_VEL:
	    if (Dop_resolution == 1)
		V_scale = 2.f;
	    else
		V_scale = 1.f;
	    V_offset = 129.f;
	    scale = V_scale;
	    offset = V_offset;
	    word_size = 16;
	    break;
	case DF_SPW:
	    SW_scale = 2.f;
	    SW_offset = 129.f;
	    scale = SW_scale;
	    offset = SW_offset;
	    word_size = 16;
	    break;
	case DF_ZDR:
	    if (Zdr_hd->scale == 0.f)
		LE_send_msg (GL_ERROR, "0 scale - Field %s", F_name[DF_ZDR]);
	    else
		scale = Zdr_hd->scale;
	    offset = Zdr_hd->offset;
	    word_size = Zdr_hd->data_word_size;
	    break;
	case DF_PHI:
	    if (Phi_hd->scale == 0.f)
		LE_send_msg (GL_ERROR, "0 scale - Field %s", F_name[DF_PHI]);
	    else
		scale = Phi_hd->scale;
	    offset = Phi_hd->offset;
	    word_size = Phi_hd->data_word_size;
	    break;
	case DF_RHO:
	    if (Rho_hd->scale == 0.f)
		LE_send_msg (GL_ERROR, "0 scale - Field %s", F_name[DF_RHO]);
	    else
		scale = Rho_hd->scale;
	    offset = Rho_hd->offset;
	    word_size = Rho_hd->data_word_size;
	    break;
	default:
	    return;
    }

    scale = (DPP_d_t)(1.) / scale;
    offset = -offset;
    if (word_size == 16) {
	unsigned short *p = (unsigned short *)inp;
	for (i = 0; i < n_gates; i++) {
	    if (p[i] == 1)
		out[i] = DPP_ALIASED;
	    else if (p[i] <= 0)
		out[i] = DPP_NO_DATA;
	    else
		out[i] = ((DPP_d_t)p[i] + offset) * scale;
	}
    }
    else {
	unsigned char *p = (unsigned char *)inp;
	for (i = 0; i < n_gates; i++) {
	    if (p[i] == 1)
		out[i] = DPP_ALIASED;
	    else if (p[i] <= 0)
		out[i] = DPP_NO_DATA;
	    else
		out[i] = ((DPP_d_t)p[i] + offset) * scale;
	}
    }
}

/******************************************************************

    Verifies if the generic moment header "gm", of field type "field",
    Returns 0 if the fields are OK or -1 if not.
	
******************************************************************/

static int Verify_gm_hd (Base_data_header *bh, Generic_moment_t *gm, 
						int field) {
    short range_beg;

    range_beg = gm->first_gate_range - gm->bin_size / 2;
    if (/*gm->no_of_gates != bh->n_dop_bins ||
	range_beg != bh->range_beg_dop || */
	gm->bin_size != 250 ||
	(gm->data_word_size != 8 && gm->data_word_size != 16)) {
	LE_send_msg (GL_ERROR, 
	  "Unexpected parameters of field %s", F_name[field]);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Fills out the RPG adaptation data in params. If the adaptation 
    data is not available, the default values are returned. The
    adaptation database is read at the beginning of each elevation.
	
******************************************************************/

static void Get_rpg_adapt_data (Dpp_params_t *params, Base_data_header *bh) {
    static int initialized = 0;

    if (!initialized || bh->azi_num == 1) {	/* get RPG adaptation data */
	int ret;
	double dbz_thresh, corr_thresh, max_diff_phidp, max_diff_dbz;
	double dbz_window, window, short_gate, long_gate;
	double md_snr_thresh, art_min_sw;
	double art_start_bin, art_count, art_min_z, art_max_z, art_v, art_corr;
        double isdp_apply;
        int isdp_est;
        char*  str;

	if ((ret = DEAU_get_values ("alg.dpprep.dbz_thresh", 
						&dbz_thresh, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.corr_thresh", 
						&corr_thresh, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.max_diff_phidp", 
						&max_diff_phidp, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.max_diff_dbz", 
						&max_diff_dbz, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.dbz_window", 
						&dbz_window, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.window", 
						&window, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.short_gate", 
						&short_gate, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.dpprep.long_gate", 
						&long_gate, 1)) <= 0) {
	    LE_send_msg (GL_INFO, 
		    "Adaptation data not available (DEAU_get_values %d)", ret);
	    LE_send_msg (GL_INFO, "Using default Adaptation data values");
	    dbz_thresh = 40.;
	    corr_thresh = .9;
	    max_diff_phidp = 100.;
	    max_diff_dbz = 50.;
	    dbz_window = 3;
	    window = 5;
	    short_gate = 9;
	    long_gate = 25;
	}
	if (DEAU_get_values ("alg.dpprep.md_snr_thresh", 
						&md_snr_thresh, 1) <= 0)
	    md_snr_thresh = 5.0;
	if (DEAU_get_values ("alg.dpprep.art_start_bin", 
						&art_start_bin, 1) <= 0)
	    art_start_bin = 180;
	if (DEAU_get_values ("alg.dpprep.art_count", &art_count, 1) <= 0)
	    art_count = 10;
	if (DEAU_get_values ("alg.dpprep.art_min_z", &art_min_z, 1) <= 0)
	    art_min_z = 30.;
	if (DEAU_get_values ("alg.dpprep.art_max_z", &art_max_z, 1) <= 0)
	    art_max_z = 50.;
	if (DEAU_get_values ("alg.dpprep.art_v", &art_v, 1) <= 0)
	    art_v = 1.0;
	if (DEAU_get_values ("alg.dpprep.art_corr", &art_corr, 1) <= 0)
	    art_corr = .8;
	if (DEAU_get_values ("alg.dpprep.art_min_sw", &art_min_sw, 1) <= 0)
	    art_min_sw = 2.0;
        if (ORPGDA_read( DP_ISDP_EST, (char *) &Isdp_est.isdp_est, 
                         sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID) <=0)
            isdp_est = -99; 
        else
	    isdp_est = Isdp_est.isdp_est;
        if (DEAU_get_string_values ("alg.dpprep.isdp_apply", &str) == 1 &&
            strcasecmp (str, "yes") == 0)
            isdp_apply = 1;
        else
            isdp_apply = 0;

	params->dbz_thresh = dbz_thresh;
	params->corr_thresh = corr_thresh;
	params->md_snr_thresh = md_snr_thresh;
	params->max_diff_phidp = max_diff_phidp;
	params->max_diff_dbz = max_diff_dbz;
	params->dbz_window = (int)dbz_window;
	params->window = (int)window;
	params->short_gate = (int)short_gate;
	params->long_gate = (int)long_gate;
	params->art_start_bin = (int)art_start_bin;
	params->art_count = (int)art_count;
	params->art_min_z = art_min_z;
	params->art_max_z = art_max_z;
	params->art_v = art_v;
	params->art_corr = art_corr;
        params->art_min_sw = art_min_sw;
	params->isdp_est = isdp_est;
        params->isdp_apply = isdp_apply;
	initialized = 1;
    }
    return;
}

/******************************************************************

    Create the output radial of the pre-processed data. The input 
    radial is "bh", the processing parameters are "params", the
    processed fields are in "out" and the pointer to the output
    radial is returned with "out_radial". The function returns the 
    size of the output radial.
	
******************************************************************/

static int Format_output_radial (Base_data_header *bh, Dpp_params_t *params,
		Dpp_out_fields_t *out, char **out_radial, DPP_d_t *ref,
		DPP_d_t *vel, DPP_d_t *rho, DPP_d_t *phi, DPP_d_t *zdr) {
    static int buf_size = 0;
    static char *buf = NULL;
    int size1, out_size, f8z_size, f8d_size, f8v_size, f16_size;
    int phi_size, zdr_size;
    Base_data_header *ohd;
    float nscale, noffset;

    /* size of the first part - basic 3 moments */
    size1 = ALIGNED_SIZE (bh->offsets[0]);

    /* the following field sizes are slightly larger than needed. But it is 
       necessary if n_gates = 0 */
    f8z_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						params->n_zgates);
    f8d_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						params->n_dgates);
    f8v_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						params->n_vgates);
    f16_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						2 * params->n_dgates);
    phi_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Phi_hd->data_word_size / 8) * params->n_dgates);
    zdr_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Zdr_hd->data_word_size / 8) * params->n_dgates);

    /* 6 8-bit fields and 3 16-bit field */
    out_size = size1 + 3 * f8z_size + f8v_size + f8d_size + 
			2 * f16_size + phi_size + zdr_size;
					/* total output radial size */
    if (out_size > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = (char *)MISC_malloc (out_size);
	buf_size = out_size;
    }

    memcpy (buf, (char *)bh, bh->offsets[0]);
    ohd = (Base_data_header *)buf;
    ohd->msg_type |= PREPROCESSED_DUALPOL_TYPE;
    ohd->no_moments = 9;

    ohd->offsets[0] = size1;
    if (Rho_hd->data_word_size == 8 && Rho_hd->scale != 0.f)
	nscale = Get_new_scale (Rho_hd->scale, Rho_hd->offset, 
			((float)255 - Rho_hd->offset) / Rho_hd->scale, 
			(float)0xffff, &noffset);
    else {
	nscale = Rho_hd->scale;
	noffset = Rho_hd->offset;
    }
    Add_moment (buf + size1, "DRHO", 'd', params, 16,
			Rho_hd->tover, Rho_hd->SNR_threshold, nscale,
			noffset, out->rho_prcd, rho);
    size1 += f16_size;
    ohd->offsets[1] = size1;
    /* increased scale and offset for better precision of phi */
    if (Phi_hd->data_word_size == 16 && Phi_hd->scale != 0.f)
	nscale = Get_new_scale (Phi_hd->scale, Phi_hd->offset, 
		1080.f, (float)0xffff, &noffset);	/* max 1080 degrees */
    else {
	nscale = Phi_hd->scale;
	noffset = Phi_hd->offset;
    }
    Add_moment (buf + size1, "DPHI", 'd', params, Phi_hd->data_word_size,
			Phi_hd->tover, Phi_hd->SNR_threshold, nscale, 
			noffset, out->phi_long_gate, phi);
    size1 += phi_size;
    ohd->offsets[2] = size1;
    Add_moment (buf + size1, "DZDR", 'd', params, Zdr_hd->data_word_size,
			Zdr_hd->tover, Zdr_hd->SNR_threshold, Zdr_hd->scale, 
			Zdr_hd->offset, out->zdr_prcd, zdr);
    size1 += zdr_size;
    ohd->offsets[3] = size1;
    Add_moment (buf + size1, "DSMZ", 'z', params, 8,
		0, bh->surv_snr_thresh, Z_scale, Z_offset, out->z_prcd, ref);
    size1 += f8z_size;
    ohd->offsets[4] = size1;
    Add_moment (buf + size1, "DSNR", 'z', params, 8,
				0, 0, 2.f, 26.f, out->snr, ref);
    size1 += f8z_size;
    ohd->offsets[5] = size1;
    Add_moment (buf + size1, "DSMV", 'v', params, 8,
	bh->vel_tover, bh->vel_snr_thresh, V_scale, V_offset, 
							out->vh_smd, vel);

    size1 += f8v_size;
    ohd->offsets[6] = size1;
    nscale = Get_new_scale (20.f, 43.f, ((float)255 - 43.f) / 20.f, 
					(float)0xffff, &noffset);
    Add_moment (buf + size1, "DKDP", 'd', params, 16,
				0, 0, nscale, noffset, out->kdp, phi);
    size1 += f16_size;
    ohd->offsets[7] = size1;
    Add_moment (buf + size1, "DSDP", 'd', params, 8,
				0, 0, 2.5f, 2.f, out->sd_phi, phi);
    size1 += f8d_size;
    ohd->offsets[8] = size1;
    Add_moment (buf + size1, "DSDZ", 'z', params, 8,
				0, 0, 8.33f, 2.f, out->sd_zh, ref);
    size1 += f8z_size;
    ohd->msg_len = size1 / sizeof (short);

    *out_radial = buf;
    return (size1);
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
    field. "inp" is the corresponding original data field.
	
******************************************************************/

static void Add_moment (char *buf, char *name, char f, Dpp_params_t *params, 
	int word_size, unsigned short tover, short SNR_threshold, 
	float scale, float offset, DPP_d_t *data, DPP_d_t *inp) {
    Generic_moment_t *hd;
    int i, up, low, n_gates;
    unsigned short *spt;
    unsigned char *cpt;
    DPP_d_t dscale, doffset;

    hd = (Generic_moment_t *)buf;
    strcpy (hd->name, name);
    hd->info = 0;
    if (f == 'z') {
	n_gates = params->n_zgates;
	hd->first_gate_range = (short)roundf ((float)(params->zr0) * 1000.f);
	hd->bin_size = (short)roundf ((float)(params->zg_size) * 1000.f);
    }
    else if (f == 'd') {
	n_gates = params->n_dgates;
	hd->first_gate_range = (short)roundf ((float)(params->dr0) * 1000.f);
	hd->bin_size = (short)roundf ((float)(params->dg_size) * 1000.f);
    }
    else {
	n_gates = params->n_vgates;
	hd->first_gate_range = (short)roundf ((float)(params->vr0) * 1000.f);
	hd->bin_size = (short)roundf ((float)(params->vg_size) * 1000.f);
    }
    hd->no_of_gates = n_gates;

    hd->tover = tover;
    hd->SNR_threshold = SNR_threshold;
    hd->control_flag = 0;
    hd->data_word_size = word_size;
    hd->scale = scale;
    hd->offset = offset;

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
    dscale = scale;
    doffset = offset;
    for (i = 0; i < n_gates; i++) {
	int t;

	if (inp[i] == DPP_ALIASED)
	    t = 1;
	else if (inp[i] <= DPP_NO_DATA)
	    t = 0;
	else if (data[i] <= DPP_NO_DATA)
	    t = 0;
	else {
	    DPP_d_t f;
	    f = data[i] * dscale + doffset;
	    if (f >= C0)
		t = (int)(f + Cp5);
	    else
		t = (int)(-(-f + Cp5));
/*	    t = round ((double)data[i] * dscale + doffset); */
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







