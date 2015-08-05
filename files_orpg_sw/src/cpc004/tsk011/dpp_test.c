
/******************************************************************

    This module contains the test routines for the dual-pol radar 
    data preprocesing program.
	
******************************************************************/

/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2007/12/05 13:42:48 $ */
/* $Id: dpp_test.c,v 1.1 2007/12/05 13:42:48 cmn Exp $ */
/* $Revision:  */
/* $State: */

#include <rpgc.h>
#include "dpprep.h"

#define MAX_N_GATES 1500

DPP_d_t *Zout, *Vout, *Zdrout, *Rhoout, *Phiout;
DPP_d_t *Snrout, *Kdpout, *Std_zout, *Std_phidp;

static int Process_section;

static void Read_data (Dpp_params_t *params, DPP_d_t *ref, DPP_d_t *vel, 
				DPP_d_t *rho, DPP_d_t *phi, DPP_d_t *zdr);
static int Get_value (char *buf, char *key, DPP_d_t *d);
static void Print_output (int size, char *f_name, DPP_d_t *d1, DPP_d_t *d2);
static int Process_radial ();
static void Create_index_tables (Dpp_params_t *params);
static int *Create_a_index (int igates, DPP_d_t ir0, DPP_d_t ig_s, 
			DPP_d_t ogates, DPP_d_t or0, DPP_d_t og_s);

/******************************************************************

    Tests DPPP_process_data. The input data is read from the file
    provided by John Krause. DPPP_process_data is then called to 
    process the data. The results is compared with that in John's
    data file again.
	
******************************************************************/

int Test_process_radial (int argc, char **argv) {

    Zout = NULL;

    if (argc <= 1)
	Process_section = 1;
    else {
	if (sscanf (argv[1], "%d", &Process_section) != 1) {
	    printf ("Section number not specified correctly\n");
	    exit (1);
	}
    }

    if (Process_section > 0) 
	Process_radial ();
    else {
	int i;
	for (i = 0; i < 1000; i++) {
	    Process_section = 1;
	    printf ("radial cnt %d\n", i);
	    Process_radial ();
	}
    }
    return (0);
}

/******************************************************************

    Tests DPPP_process_data. The input data is read from the file
    provided by John Krause. DPPP_process_data is then called to 
    process the data. The results is compared with that in John's
    data file again.
	
******************************************************************/

static int Process_radial () {
    static int prev_zgs = 0, prev_vgs = 0, prev_dgs = 0; /* sizes of out */
    static Dpp_out_fields_t out;	/* buffer for processed fields */
    Dpp_params_t params;
    DPP_d_t ref[MAX_N_GATES], vel[MAX_N_GATES], rho[MAX_N_GATES];
    DPP_d_t phi[MAX_N_GATES], zdr[MAX_N_GATES];

    Read_data (&params, ref, vel, rho, phi, zdr);

    if (params.n_zgates > prev_zgs) {	/* allocate buffers for output */
	if (prev_zgs > 0) {
	    free (out.z_prcd);
	    free (out.snr);
	    free (out.sd_zh);
	}
	out.z_prcd = MISC_malloc ((params.n_zgates + 1) * sizeof (DPP_d_t));
	out.snr = MISC_malloc ((params.n_zgates + 1) * sizeof (DPP_d_t));
	out.sd_zh = MISC_malloc (params.n_zgates * sizeof (DPP_d_t));
	out.snr[params.n_zgates] = DPP_NO_DATA;
	out.z_prcd[params.n_zgates] = DPP_NO_DATA;
	prev_zgs = params.n_zgates;
    }
    if (params.n_vgates > prev_vgs) {
	if (prev_vgs > 0) {
	    free (out.vh_smd);
	}
	out.vh_smd = MISC_malloc (params.n_vgates * sizeof (DPP_d_t));
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
	out.rho_prcd = MISC_malloc (params.n_dgates * sizeof (DPP_d_t));
	out.phi_long_gate = MISC_malloc (
			(params.n_dgates + 1) * sizeof (DPP_d_t));
	out.zdr_prcd = MISC_malloc (params.n_dgates * sizeof (DPP_d_t));
	out.kdp = MISC_malloc (params.n_dgates * sizeof (DPP_d_t));
	out.sd_phi = MISC_malloc (params.n_dgates * sizeof (DPP_d_t));
	out.phi_long_gate[params.n_dgates] = DPP_NO_DATA;
	prev_dgs = params.n_dgates;
    }

    printf ("    atmos %f dbz0 %f z_syscal %f zdr_syscal %f init_fdp %f\n", 
	params.atmos, params.dbz0, params.z_syscal, params.zdr_syscal, params.init_fdp);
    printf ("    zdr_thr %f cor_thr %f\n", params.zdr_thr, params.cor_thr);
    printf ("    n_h %f n_v %f ref_thr %f rho_thr %f\n", 
	params.n_h, params.n_v, params.dbz_thresh, params.corr_thresh);
    printf ("    r0 %f n_gates %d g_size %f\n", 
	params.zr0, params.n_zgates, params.zg_size);

/*
    for (i = 0; i < params.n_gates; i++) {
	printf ("%d  %f %f  1  %f %f %f\n", i, ref[i], vel[i], zdr[i], rho[i], phi[i]);
    }
*/

    /* perform pre-processing */
    if (DPPP_process_data (&params, ref, vel, rho, phi, zdr, &out) < 0)
	return (0);

    Print_output (params.n_zgates, "SNR", out.snr, Snrout);
    Print_output (params.n_zgates, "STD Z", out.sd_zh, Std_zout);
    Print_output (params.n_vgates, "VEL", out.vh_smd, Vout);
    Print_output (params.n_dgates, "RHO", out.rho_prcd, Rhoout);
    Print_output (params.n_dgates, "STD PHI", out.sd_phi, Std_phidp);
    Print_output (params.n_dgates, "KDP", out.kdp, Kdpout);
    Print_output (params.n_dgates, "ZDR", out.zdr_prcd, Zdrout);
    Print_output (params.n_dgates, "PHI", out.phi_long_gate, Phiout);
    Print_output (params.n_zgates, "DBZ", out.z_prcd, Zout);

    return (0);
}

/******************************************************************

    Tests DPPP_process_data. The input data is read from the file
    provided by John Krause. DPPP_process_data is then called to 
    process the data. The results is compared with that in John's
    data file again.
	
******************************************************************/

#define BUF_SIZE 256

static void Read_data (Dpp_params_t *params, DPP_d_t *ref, DPP_d_t *vel, 
				DPP_d_t *rho, DPP_d_t *phi, DPP_d_t *zdr) {
    static FILE *fd = NULL;
    char buf[BUF_SIZE];
    enum {R_NONE, R_INPUT, R_OUTPUT};
    int R_state, in_cnt, out_cnt, param_cnt, started;

    if (fd == NULL) {
	char *fname = "test.output";
	fd = fopen (fname, "r");
	if (fd == NULL) {
	    printf ("file %s failed\n", fname);
	    exit (1);
	}
    }
//    else
//	fseek (fd, 0, SEEK_SET);

    if (Zout == NULL) {
	Zout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Vout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Zdrout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Rhoout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Phiout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Snrout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Kdpout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Std_zout = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
	Std_phidp = (DPP_d_t *)MISC_malloc (sizeof (DPP_d_t) * MAX_N_GATES);
    }

    started = 0;
    params->atmos = (DPP_d_t)(0.);
    R_state = R_NONE;
    in_cnt = out_cnt = param_cnt = 0;
    while (fgets (buf, BUF_SIZE, fd) != NULL) {
	int i;
	DPP_d_t d;

	if (strncmp (buf, "Alignment", 9) == 0 && started == Process_section)
	    break;
	if (strncmp (buf, "Az: ", 3) == 0)
	    started++;
	if (started < Process_section)
	    continue;
	if (started > Process_section)
	    break;

	if (buf[0] <= 57 && buf[0] >= 48) {
	    double z, v, sw, zd, rh, ph;
	    if (R_state == R_INPUT) {
		if (in_cnt >= MAX_N_GATES) {
		    printf ("Too many input gates\n");
		    exit (1);
		}
		if (sscanf (buf, "%d %lf %lf %lf %lf %lf %lf", &i, 
					&z, &v, &sw, &zd, &rh, &ph) != 7) {
		    printf ("Unexpected number of fields in input data\n");
		    exit (1);
		}
		if (i != in_cnt) {
		    printf ("Unexpected input data index\n");
		    exit (1);
		}
		if (z == -32768.)
		    z = DPP_NO_DATA;
		if (v == -32768.)
		    v = DPP_NO_DATA;
		if (sw == -32768.)
		    sw = DPP_NO_DATA;
		if (rh == -32768.)
		    rh = DPP_NO_DATA;
		if (ph == -32768.)
		    ph = DPP_NO_DATA;
		if (zd == -32768.)
		    zd = DPP_NO_DATA;
		ref[i] = z;
		vel[i] = v;
		rho[i] = rh;
		phi[i] = ph;
		zdr[i] = zd;
		in_cnt++;
	    }
	    else if (R_state == R_OUTPUT) {
		double z, v, sw, zdr, rho, phi, snr, kdp, std_z, std_phidp;
		if (out_cnt >= MAX_N_GATES) {
		    printf ("Too many output gates\n");
		    exit (1);
		}
		if (sscanf (buf, "%d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
				&i, 
				&z, &v, &sw, &zdr, &rho, &phi, &snr, 
				&kdp, &std_z, &std_phidp) != 11) {
		    printf ("Unexpected number of fields in output data\n");
		    exit (1);
		}
		if (i != out_cnt) {
		    printf ("Unexpected output data index\n");
		    exit (1);
		}
		Zout[i] = z;
		Vout[i] = v;
		Zdrout[i] = zdr;
		Rhoout[i] = rho;
		Phiout[i] = phi;
		Snrout[i] = snr;
		Kdpout[i] = kdp;
		Std_zout[i] = std_z;
		Std_phidp[i] = std_phidp;
		out_cnt++;
	    }
	    continue;
	}

	params->dbz_thresh = 40.;
	params->corr_thresh = .9;
	params->max_diff_phidp = 100.;
	params->max_diff_dbz = 50.;
	params->dbz_window = 3;
	params->window = 5;
	params->short_gate = 9;
	params->long_gate = 25;

	param_cnt++;
	if (Get_value (buf, "min_PhiDP:", &d))
	    params->init_fdp = d;
	else if (Get_value (buf, "Nh-Nv:", &d)) {
	    params->n_h = (DPP_d_t)(-50.);
	    params->n_v = params->n_h - d;
	}
	else if (Get_value (buf, "RhoHV Noise Correction SNR threshold:", &d))
	    params->cor_thr = d;
	else if (Get_value (buf, "Zdr Noise Correction SNR threshold:", &d))
	    params->zdr_thr = d;
	else if (Get_value (buf, "KDP RhoHV threshold:", &d))
	    params->corr_thresh = d;
	else if (Get_value (buf, "KDP Z Threshold:", &d))
	    params->dbz_thresh = d;
	else if (strncmp (buf, "Calibration check:", 18) == 0) {
	    char *p;
	    double d1, d2, d3;
	    if ((p = strstr (buf, "Z:")) == NULL ||
		sscanf (p + 2, "%lf", &d1) != 1 ||
	        (p = strstr (buf, "Zdr:")) == NULL ||
		sscanf (p + 4, "%lf", &d2) != 1 ||
	        (p = strstr (buf, "C:")) == NULL ||
		sscanf (p + 2, "%lf", &d3) != 1) {
		printf ("unexpected \"Calibration check:\"\n");
		exit (1);
	    }
	    params->z_syscal = d1;
	    params->zdr_syscal = d2;
	    params->dbz0 = -d3;
	    param_cnt += 2;
	}
	else if (strncmp (buf, "input i:", 8) == 0)
	    R_state = R_INPUT;
	else if (strstr (buf, "output i:") != NULL)
	    R_state = R_OUTPUT;
	else
	    param_cnt--;
    }
    param_cnt -= 2;

    if (param_cnt != 9) {
	if (param_cnt == -2) {
	    printf ("done\n");
	    exit (0);
	}
	printf ("Bad number of parameters (%d, expected 9)\n", param_cnt);
	exit (1);
    }

    if (in_cnt != out_cnt) {
	printf ("output data count (%d) differs from input (%d)\n", 
					out_cnt, in_cnt);
	exit (1);
    }
    printf ("%d bins read\n", in_cnt);
    params->zr0 = (DPP_d_t)(.125);
    params->dr0 = params->vr0 = params->zr0;
    params->n_zgates = in_cnt;
    params->n_dgates = params->n_vgates = params->n_zgates;
    params->zg_size = (DPP_d_t)(.25);
    params->dg_size = params->vg_size = params->zg_size;

    Create_index_tables (params);
}

/******************************************************************

    Reads the text in "buf" to match "key". If it does not match,
    returns 0. Otherwise, it reads the value after the key into "d".
	
******************************************************************/

static int Get_value (char *buf, char *key, DPP_d_t *d) {
    int len;
    double t;

    len = strlen (key);
    if (strncmp (buf, key, len) != 0)
	return (0);
    if (sscanf (buf + len, "%lf", &t) != 1) {
	printf ("Invalid text %s\n", buf);
	exit (1);
    }
    *d = (DPP_d_t)t;
    printf ("%s", buf);
    return (1);
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
			DPP_d_t ogates, DPP_d_t or0, DPP_d_t og_s) {
    int *ind, i;

    ind = (int *)MISC_malloc (igates * sizeof (int));
    for (i = 0; i < igates; i++) {
	DPP_d_t d;
	int oi;

	d = ir0 + (DPP_d_t)i * ig_s;
	oi = (d - or0) / og_s;
	if (d - (or0 + oi * og_s) > (DPP_d_t).5 * og_s)
	    oi++;
	if (oi < 0 || oi > ogates)
	    oi = ogates;
	ind[i] = oi;
    }
    return (ind);
}

/******************************************************************

    Prints output field "f_name".
	
******************************************************************/

static void Print_output (int size, char *f_name, DPP_d_t *d1, DPP_d_t *d2) {
    int i, diff;

    diff = 0;
    printf ("\n%s---\n", f_name);
    for (i = 0; i < size; i++) {
	DPP_d_t d, a1, a2;
//	if (i > 10 && i < size - 10 && (i % 10) != 0)
//	     continue;
	a1 = d1[i];
	if (a1 < C0)
	    a1 = -a1;
	a2 = d2[i];
	if (a2 < C0)
	    a2 = -a2;
	a1 = a1 + a2;
	if (a1 < C1)
	    a1 = C1;
	d = d1[i] - d2[i];
	if (d < C0)
	    d = -d;
	if (d / a1 < (DPP_d_t)(.00001))
	    continue;
	if (d1[i] == DPP_NO_DATA && d2[i] == (DPP_d_t)(-32768))
	    continue;
	printf ("%6d  %10f  %10f\n", i, d1[i], d2[i]);
	diff = 1;
    }
    if (Process_section > 1 && diff)
	exit (1);
}

/***********************************************************************

    Rounds double value "v" to a precision of "n_digits" significant 
    digits.

***********************************************************************/

double Round_signif_digits (double v, int n_digits) {
    int negative, cnt, i;
    double r;

    if (n_digits <= 0 || v == 0.)
	return (v);
    negative = 0;
    if (v < 0.) {
	v = -v;
	negative = 1;
    }
    cnt = 0;
    while (v >= 10.) {
	cnt--;
	v *= .1;
    }
    while (v < 1.) {
	cnt++;
	v *= 10.;
    }
    for (i = 0; i < n_digits - 1; i++) {
	cnt++;
	v *= 10.;
    }
    v = (double)((int)(v + .5));
    if (cnt < 0)
	r = 10.;
    else
	r = .1;
    for (i = 0; i < cnt; i++)
	v *= r;
    if (negative)
	v = -v;
    return (v);
}
