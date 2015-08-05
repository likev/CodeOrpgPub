
/******************************************************************

    vdeal's module for NEXRAD realtime data access.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/05/27 19:49:39 $
 * $Id: vdeal_realtime.c,v 1.23 2015/05/27 19:49:39 steves Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include <rpgc.h>
#include <rpgcs.h>
#include "vdeal.h"

enum {AR_INIT, AR_READ, AR_SAVE}; /* for parameter "func" of Archive_radial */

extern int Test_mode;
extern int Verbose_mode;

static short *N_gates = NULL;	/* number of V gates for each radial */
static char *Output_data = "BASEDATA";
static char *Input_data = "RAWDATA";
static time_t Vol_start_tm = 0;	/* the start time of the current volume */
static int Vol_tm = 180;	/* the latest detected volume scan time */
static char Image_label[128] = "";	/* label for debugging images */
static int Vol_seq_num = 0;	/* current volume sequence number */
static int Elev_num = 0;	/* current elevation number */

static int Process_radial (char *ibuf_ptr, Vdeal_t *vdv);
static int Initialize_ele (Base_data_header *bh, Vdeal_t *vdv, int nyq);
static int Get_radial_data (Base_data_header *bh, Vdeal_t *vdv);
static int Archive_radial (int func, int ind, void *data);
static int Process_abort (Vdeal_t *vdv);
static int Output_a_radial (Base_data_header *bh);
static time_t Get_data_time (Base_data_header *bh);
static void Get_rpg_adapt_data (Vdeal_t *vdv);
static int Read_radial (Vdeal_t *vdv);
static void Output_hsf (Vdeal_t *vdv);


/******************************************************************

    Performs realtime processing of a cut.

******************************************************************/

void VD2D_realtime_processing (Vdeal_t *vdv) {
    static int test_mode_initialized = 0;

    if (!test_mode_initialized) {
	if (getenv ("VDEAL_TEST") != NULL)
	    Test_mode = 1;
	test_mode_initialized = 1;
    }

    while (1) {

	if (vdv->radial_status != RS_END_ELE) {
	    if (Read_radial (vdv) < 0) {
		vdv->radial_status = RS_NONE;
		vdv->rt_state = RT_DONE;
		return;
	    }
	}

	if (vdv->rt_state == RT_DONE) {
	    if (vdv->radial_status == RS_START_ELE)
		vdv->rt_state = RT_START_ELE;
	}

	if (vdv->rt_state == RT_START_ELE && 
			    vdv->radial_status == RS_END_ELE) {
	    EE_estimate_ew (vdv);
	    EE_get_eew (vdv);
	    vdv->rt_state = RT_PROCESS;
	}

	if (vdv->rt_state == RT_PROCESS)
	    VDD_process_realtime (vdv);

	if (vdv->rt_state == RT_COMPLETED) {
	    vdv->radial_status = RS_NONE;
	    vdv->rt_state = RT_DONE;
	    Output_hsf (vdv);
	    return;
	}
    }
}

void VDR_status_log (const char *msg) {
    RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, "VELDEAL: %s\n", msg);
}

/******************************************************************

    Returns the latest detected volume time. The start time of the
    current volume is returned with "v_st".

******************************************************************/

int VDR_get_volume_time (time_t *v_st) {
    if (v_st != NULL)
	*v_st = Vol_start_tm;
    return (Vol_tm);
}

/******************************************************************

    Returns Image_label.

******************************************************************/

char *VDR_get_image_label () {
    return (Image_label);
}

/******************************************************************

    Reads the next redial. Returns 0 on success or -1 if the scan
    is aborted.

******************************************************************/

static int Read_radial (Vdeal_t *vdv) {
    int *ibuf_ptr, in_status;

    /* Get a radial of RAW BASE DATA. */
    ibuf_ptr = RPGC_get_inbuf_by_name( Input_data, &in_status );
    if( in_status == RPGC_NORMAL ){

	/* Process this radial */
	int ret = 0;
	if (Process_radial( (char *)ibuf_ptr, vdv) < 0) {
	    Process_abort (vdv);
	    ret = -1;
	}

	/* Release the input buffer */
	RPGC_rel_inbuf( ibuf_ptr );
	return (ret);
    }
    else{

	/* RPGC_get_inbuf_by_name status not RPGC_NORMAL: ABORT. */
	Process_abort (vdv);
	return -1;
    }
}

/******************************************************************

    Processes elevation abort. If we need to output archived radials
    we can implement it later.

******************************************************************/

static int Process_abort (Vdeal_t *vdv) {
    RPGC_abort();
    vdv->radial_status = RS_NONE;
    return (0);
}

/******************************************************************

    Outputs the newly processed radials.

******************************************************************/

int VDR_output_processed_radial (Vdeal_t *vdv) {
    int i;

    if (Test_mode && vdv->rt_done < vdv->rt_processed)
	MISC_log ("Output %d radials\n", vdv->rt_processed - vdv->rt_done);

    for (i = vdv->rt_done; i < vdv->rt_processed; i++) {
	short *in, *out;
	int size, s, k;
	Base_data_header *bh;

	size = Archive_radial (AR_READ, i, (void *)&bh);
	if (size <= 0)
	    continue;

	in = vdv->out + i * vdv->xz;
	out = (short *)((char *)bh + bh->vel_offset);
	s = N_gates[i];
	for (k = 0; k < s; k++) {
	    if (out[k] > 1) {
		int t = in[k];
		if (t == SNO_DATA) {
		    continue;
		}
		if (bh->dop_resolution == 2)	/* restore original scale */
		    t = Myround ((t - vdv->data_off) * .5 + vdv->data_off);
		if (t > 255)
		    t = 255;
		else if (t < 2)
		    t = 2;
		out[k] = t;
	    }
	}
	Output_a_radial (bh);
    }
    vdv->rt_done = vdv->rt_processed;
    return (0);
}

/******************************************************************

    Outputs radial "bh".

******************************************************************/

static int Output_a_radial (Base_data_header *bh) {
    int out_status, length;
    char *obuf_ptr;

    length = bh->msg_len * sizeof (short);
    obuf_ptr = RPGC_get_outbuf_by_name(Output_data, length, &out_status );

    if (out_status != RPGC_NORMAL)
	RPGC_abort_because( out_status );
    else {
	memcpy (obuf_ptr, (char *)bh, length);

	/* Release the output buffer with FORWARD disposition */
	RPGC_rel_outbuf( obuf_ptr, FORWARD );
    }
    return (0);
}

/******************************************************************

    Processes a newly read redial "ibuf_ptr". Returns 0 on success
    or -1 on failure (unexpected data).

******************************************************************/

static int Process_radial (char *ibuf_ptr, Vdeal_t *vdv) {
    static int n_secs = 0, secs[(MAX_N_PRF - 1) * 2], prv_nyq = 0, nyq1;
    Base_data_header *bh;
    int r_stat, nyq;

    bh = (Base_data_header *)ibuf_ptr;
    if (bh->msg_len * sizeof (short) < sizeof (Base_data_header)) {
	MISC_log ("Unexpected radial message len %d - aborted",
							bh->msg_len);
	return (-1);
    }

    if (bh->status == GENDEL || bh->status == GENDVOL) {
	time_t dtm = Get_data_time (bh);
	r_stat = RS_END_ELE;
	if (bh->status == GENDVOL)
	    Vol_tm = dtm - Vol_start_tm;
	else {
	    int vtm = dtm - Vol_start_tm;
	    if (vtm > Vol_tm)
		Vol_tm = vtm;
	}
    }
    else if (bh->status == GOODBEL || bh->status == GOODBVOL) {
	static int prev_v_num = -1;
	if (bh->azi_num != 1) {
	    MISC_log ("Unexpected first azimuth number %d - aborted",
							bh->azi_num);
	    return (-1);
	}
	r_stat = RS_START_ELE;
	if (bh->volume_scan_num != prev_v_num) {
		/* I cannot use GOODBVOL - It shows up twice in a volume */
	    if (!Test_mode && prev_v_num == -1)
		VDV_read_history (vdv, 1);
	    Vol_start_tm = Get_data_time (bh);
	    Vol_seq_num = bh->vol_num_quotient * MAX_VSCAN + 
						bh->volume_scan_num;
	    Elev_num = bh->elev_num;
	    vdv->vol_num++;
	    if (vdv->nonuniform_vol + 1 < vdv->vol_num)
		vdv->nonuniform_vol = 0;  /* reset non-uniform data state */
	    vdv->cut_num = 0;
	    prev_v_num = bh->volume_scan_num;

            /* Added to support 2D Velocity Dealiasing Field Test. */
            if( Verbose_mode )
                RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG,
                              "VELDEAL: 2D Velocity Dealiasing Algorithm Being Used\n" );
	}
	else
	    vdv->cut_num++;
	VDA_get_reset_hsf (1, NULL);
    }
    else
	r_stat = RS_NORMAL;

    /* non-Doppler cut */
    if (bh->vel_offset == 0) {
	Output_a_radial (bh);
	return (0);
    }

    if (bh->dop_resolution == 1)
	nyq = (int)((float)bh->nyquist_vel / 50.f + .5f);
    else
	nyq = (int)((float)bh->nyquist_vel / 100.f + .5f) * 2;
    if (r_stat == RS_START_ELE) {
	nyq1 = nyq;
	n_secs = prv_nyq = 0;
    }
    else if (nyq != prv_nyq) {
	if (n_secs < (MAX_N_PRF - 1) * 2) {
	    secs[n_secs] = bh->azi_num;
	    secs[n_secs + 1] = nyq;
	    n_secs += 2;
	}
	else
	    MISC_log ("Too many PRF sectors - ignored");
    }
    prv_nyq = nyq;

    if (r_stat == RS_START_ELE) {
	int y, mon, d, h, m, s;
	char *t;
	Initialize_ele (bh, vdv, nyq);
	vdv->radial_status = r_stat;
	unix_time (&vdv->dtm, &y, &mon, &d, &h, &m, &s);
	if (bh->dop_resolution == 2)
	    t = ", 1 m/s";
	else
	    t = "";
	MISC_log ("Start of elevation %.2f (azi %.2f%s) %d_%.2d_%.2d_%.2d_%.2d_%.2d", vdv->elev, bh->azimuth, t, y, mon, d, h, m, s);
    }
    if (vdv->radial_status == RS_NONE)	/* cut not started */
	return (0);
    if (r_stat == RS_NORMAL) {
	if (vdv->radial_status != RS_NORMAL && 
				vdv->radial_status != RS_START_ELE) {
	    MISC_log ("Unexpected radial status - aborted");
	    return (-1);
	}
    }
    if (r_stat == RS_END_ELE) {
	if (vdv->radial_status == RS_END_ELE) {
	    MISC_log ("Unexpected end of elevation - aborted");
	    return (-1);
	}
    }
    if (bh->azi_num != vdv->rt_read + 1) {
	MISC_log ("Unexpected azimuth number - aborted");
	return (-1);
    }
    vdv->radial_status = r_stat;
    if (Get_radial_data (bh, vdv) < 0)
	return (-1);

    if (r_stat == RS_END_ELE) {
	vdv->yz = vdv->rt_read;
	PP_setup_prf_sectors (vdv, nyq1, n_secs, secs);
    }
    return (0);
}

/******************************************************************

    Gets velocity data from radial "bh" and saves it in vdv. The 
    entire radial is then saved for later retrieval. Returns 0 on 
    success or -1 on failure.

******************************************************************/

static int Get_radial_data (Base_data_header *bh, Vdeal_t *vdv) {
    int size, i;
    double azi;

    if (vdv->rt_read >= vdv->yz) {
	MISC_log ("Too many radials in an elevation - aborted");
	return (-1);
    }

    azi = bh->azimuth;
    vdv->ew_aind[vdv->rt_read] = VDE_get_azi_ind (vdv, azi);
    vdv->ew_azi[vdv->rt_read] = Myround (azi * 10.);

    {	/* copy velocity data to vdv->inp for processing */
	unsigned char *p, *w, *z;
	short *vp, *zp, *wp;
	int data_off, dr0;

	p = vdv->inp + vdv->rt_read * vdv->xz;
	w = vdv->spw + vdv->rt_read * vdv->xz;
	z = vdv->dbz + vdv->rt_read * vdv->xz;
	vp = (short *)((char *)bh + bh->vel_offset);
	wp = (short *)((char *)bh + bh->spw_offset);
	zp = (short *)((char *)bh + bh->ref_offset);
	dr0 = bh->range_beg_dop + bh->dop_bin_size - 
				(bh->range_beg_surv + bh->surv_bin_size / 2);
	size = bh->n_dop_bins;
	if (size > vdv->xz)
	    size = vdv->xz;
	data_off = vdv->data_off;
	for (i = 0; i < size; i++) {
	    int rind;

	    p[i] = vp[i];
	    if (p[i] > 1 && bh->dop_resolution == 2)	/* change scale to 2 */
		p[i] = (p[i] - data_off) * 2 + data_off;
	    w[i] = wp[i];
	    rind = (dr0 + i * bh->dop_bin_size) / bh->surv_bin_size;
	    if (rind >= 0 && rind < bh->n_surv_bins)
		z[i] = zp[rind];
	    else
		z[i] = 0;
	}
	for (i = size; i < vdv->xz; i++) {
	    p[i] = BNO_DATA;
	    w[i] = 0;
	    z[i] = 0;
	}
	PP_convert_spw (w, vdv->xz, vdv->data_off);
	PP_preprocessing_v (vdv, vdv->rt_read, 1);
    }
    N_gates[vdv->rt_read] = size;

    Archive_radial (AR_SAVE, vdv->rt_read, (void *)bh);
    vdv->rt_read++;

    return (0);
}

/******************************************************************

    Performs initialization upon a new elevation starts.

******************************************************************/

static int Initialize_ele (Base_data_header *bh, Vdeal_t *vdv, int nyq) {
    static int prev_xz = 0, prev_yz = 0;

    vdv->xz = bh->n_dop_bins;
    if (bh->azm_reso == BASEDATA_HALF_DEGREE) {
	vdv->yz = 800;
	vdv->gate_width = .5f;
    }
    else {
	vdv->yz = 400;
	vdv->gate_width = 1.f;
    }
    vdv->yz += 10;		/* adds some extra space */
    vdv->yz = (vdv->yz / 2) * 2;

    vdv->data_off = 129;
    vdv->data_scale = 2.f;	/* always use this scale internally */
    vdv->nyq = nyq;
    vdv->n_secs = 1;
    vdv->start_range = 0;
    vdv->g_size = bh->dop_bin_size;
    vdv->start_azi = bh->azimuth;
    vdv->elev = bh->target_elev * .1f;
    vdv->vcp = bh->vcp_num;
    vdv->unamb_range = bh->unamb_range;
    vdv->dtm = Get_data_time (bh);
    vdv->data_type = 0;
    vdv->realtime = 1;		/* set REALTIME */
    vdv->rt_read = 0;
    vdv->rt_processed = 0;
    vdv->rt_done = 0;

    VDD_init_vdv (vdv);

    if (vdv->xz * vdv->yz > prev_xz * prev_yz || vdv->yz > prev_yz) {
	if (vdv->inp != NULL)
	    free (vdv->inp);
	if (vdv->spw != NULL)
	    free (vdv->spw);
	if (N_gates != NULL)
	    free (N_gates);
	vdv->inp = MISC_malloc (vdv->xz * vdv->yz);
	vdv->spw = MISC_malloc (vdv->xz * vdv->yz);
	vdv->dbz = MISC_malloc (vdv->xz * vdv->yz);
	N_gates = (short *)MISC_malloc (vdv->yz * sizeof (short));
	prev_xz = vdv->xz;
	prev_yz = vdv->yz;
    }

    Archive_radial (AR_INIT, vdv->yz, NULL);
    VDR_get_ext_wind (0, -1, NULL, NULL);
    Get_rpg_adapt_data (vdv);

    sprintf (Image_label, "%.2d_%.2d_%.2d.%d", ((int)vdv->dtm / 3600) % 24, 
	((int)vdv->dtm / 60) % 60, (int)vdv->dtm % 60, bh->target_elev);

    return (0);
}

/******************************************************************

    Initializes the RPGC library.

******************************************************************/

int VDR_realtime_process (int argc, char *argv[], Vdeal_t *vdv) {

    /* Initialize the log error services. */
    RPGC_init_log_services( argc, argv );

    /* Specify inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Tell system we are radial-based. */
    RPGC_task_init( RADIAL_BASED, argc, argv );

    while (1) {
	RPGC_wait_act( WAIT_DRIVING_INPUT );
	VD2D_realtime_processing (vdv);
    }

    return (0);
}

/******************************************************************

    Returns the UNIX time of radial "bh".

******************************************************************/

static time_t Get_data_time (Base_data_header *bh) {
    return ((time_t)((bh->date - 1) * 86400 + bh->time / 1000));
}

/******************************************************************

    Archives radial "bh" for later retieval. Returns the radial size
    on success or -1 on failure. This function terminates the process
    if file access fails. I choose to use this file version for 
    saving memory.

******************************************************************/

static int Archive_radial (int func, int ind, void *data) {
    static int fl = -1;
    static int *offsets = NULL;
    static int b_size = 0;

    if (func == AR_INIT) {
	if (ind > b_size) {
	    if (offsets != NULL)
		free (offsets);
	    offsets = (int *)MISC_malloc (ind * sizeof (int));
	    b_size = ind;
	}
	if (fl < 0) {
	    char name[256], task_name[64];
	    if (ORPGTAT_get_my_task_name (task_name, 64) != 0) {
		MISC_log ("ORPGTAT_get_my_task_name failed\n");
		exit (1);
	    }
	    if (MISC_get_work_dir (name, 256 - 64 - 8) < 0) {
		MISC_log ("MISC_get_work_dir failed\n");
		exit (1);
	    }
	    sprintf (name + strlen (name), "/%s.tmp", task_name);
	    if ((fl = open (name, O_RDWR | O_CREAT, 0666)) < 0) {
		MISC_log ("open radial file (%s) failed\n", name);
		exit (1);
	    }
	    MISC_log ("Radial archive file (%s) opened (fd %d)\n", name, fl);
	}
	return (0);
    }

    else if (func == AR_SAVE) {
	int off, size;

	Base_data_header *bh = (Base_data_header *)data;
	size = bh->msg_len * sizeof (short);
	if (ind > 0)
	    off = offsets[ind - 1];
	else
	    off = 0;
	lseek (fl, off, SEEK_SET);

	if (write (fl, (char *)data, size) != size) {
	    MISC_log ("Error writing radial file\n");
	    exit (1);
	}
	offsets[ind] = off + size;	
	return (size);
    }

    else if (func == AR_READ) {
	static char *buf = NULL;
	int off, size;
	if (ind > 0)
	    off = offsets[ind - 1];
	else
	    off = 0;
	size = offsets[ind] - off;
	lseek (fl, off, SEEK_SET);
	buf = STR_reset (buf, size);
	if (read (fl, buf, size) != size) {
	    MISC_log ("Error reading radial file (off %d, size %d)\n", 
							off, size);
	    exit (1);
	}
	*((char **)data) = buf;
	return (size);
    }

    else
	return (-1);
}

/**************************************************************************

    Returns the EW read from the external EW source (RUC). Searches for the
    wind at altitude <=, if "up" is 0 or > otherwise, "alt" in meters, and
    returns its speed and direction. The return value is the altitude of 
    the EW in meters on success or -1 on failure. When "up" < 0, the
    data store is read. 

***************************************************************************/

#include <itc.h>

int VDR_get_ext_wind (int alt, int up, double *speed, double *dir) {
    static char *b = NULL;
    A3cd97 *md;
    int sind, altfeet, ind;
    float bdata;

    if (up < 0) {		/* read the data */
	int ret, bad;

	if (b != NULL)
	    free (b);
	bad = 0;
	ret = ORPGDA_read ((ENVVAD / ITC_IDRANGE) * ITC_IDRANGE, 
					    &b, LB_ALLOC_BUF, LBID_MODEL_EWT);
	if (ret < sizeof (A3cd97)) {
	    MISC_log ("Read external EW failed (%d) - Not used\n", ret);
	    bad = 1;
	}
	else {
	    int t, d;
	    md = (A3cd97 *)b;
	    d = md->sound_time - (int)(Vol_start_tm / 60);
	    t = d;
	    if (t < 0)
		t = -t;
	    if (!Test_mode && t > 150) {	/* two and half hours */
		MISC_log ("External EW not usable (%d minutes off)\n", d);
		bad = 1;
	    }
	}
	if (bad) {
	    if (b != NULL)
		free (b);
	    b = NULL;
	}
	return (-1);
    }

    if (b == NULL)
	return (-1);

    md = (A3cd97 *)b;
    altfeet = Myround (alt * M_TO_FT);
    sind = (altfeet - BASEHGT) / HGTINC;
    bdata = MTTABLE;
    if (up) {
	ind = sind + 1;
	while (ind < LEN_EWTAB) {
	    if (ind >= 0 && md->ewtab[WNDSPD][ind] != bdata)
		break;
	    ind++;
	}
	if (ind >= LEN_EWTAB)
	    return (-1);
    }
    else {
	ind = sind;
	while (ind >= 0) {
	    if (ind < LEN_EWTAB && md->ewtab[WNDSPD][ind] != bdata)
		break;
	    ind--;
	}
	if (ind < 0)
	    return (-1);
    }

    *speed = (double)(md->ewtab[WNDSPD][ind]);
    *dir = (double)(md->ewtab[WNDDIR][ind]);
    return (Myround (FT_TO_M * (BASEHGT + ind * HGTINC)));
}

/******************************************************************

    Fills out the RPG adaptation data in params. If the adaptation 
    data is not available, the default values are returned. The
    adaptation database is read at the beginning of each elevation.
	
******************************************************************/

static void Get_rpg_adapt_data (Vdeal_t *vdv) {
    static int init = 0;
    extern int VDB_threads;
    extern int VDD_threads;
    extern double VDA_hs_threshold;
    extern int VDA_hs_size;
    extern int VDA_b16_update1;
    extern int Max_partition_size;

    if (!init) {
	double n;
	char *str;

	/* We set the default behavior of the exception signals to avoid
	   not terminating upon the signals */
	MISC_sig_sigset (SIGSEGV, SIG_DFL);
	MISC_sig_sigset (SIGFPE, SIG_DFL);
	MISC_sig_sigset (SIGABRT, SIG_DFL);
	MISC_log ("Exception signals handle is set to default\n");

	VDB_threads = VDD_threads = 4;
	VDA_hs_threshold = 0.f;
	VDA_hs_size = 10;
	init = 1;
	if (DEAU_get_values ("alg.vdeal.solver_threads", &n, 1) > 0 &&
							n > 0.)
	    VDB_threads = n;
	if (DEAU_get_values ("alg.vdeal.dealiase_threads", &n, 1) > 0 &&
							n > 0.)
	    VDD_threads = n;

	if (DEAU_get_values ("alg.vdeal.max_partition_size", &n, 1) > 0 &&
							n > 30.)
	    Max_partition_size = n;

	if (DEAU_get_values ("alg.vdeal.hs_threshold", &n, 1) > 0)
	    VDA_hs_threshold = n;

	if (DEAU_get_values ("alg.vdeal.hs_size", &n, 1) > 0)
	    VDA_hs_size = n;

	if (DEAU_get_string_values ("alg.vdeal.slow_processor", &str) >= 1 &&
	    strcmp (str, "YES") == 0) {
	    Max_partition_size = 56;
	    VDB_threads = VDD_threads = 2;
	    MISC_log ("Set slow processor mode\n");
	}

	if (DEAU_get_values ("alg.vdeal.b16_update1", &n, 1) > 0)
	    VDA_b16_update1 = n;

	MISC_log ("Use adapt data: VDB_threads %d; VDD_threads %d;\n", 
					VDB_threads, VDD_threads);
	MISC_log ("                max_partition_size %d; b16_update1 %d;\n", 
					Max_partition_size, VDA_b16_update1);
    }
    return;
}

/**************************************************************************

    Exports extreme high shear features detected by the current cut.

**************************************************************************/

#define MAXN_HSF_IN_MSG 128
#define MAXN_HSFS 256

static ORPG_hsf_t Rhsfs[MAXN_HSFS];	/* stores the latest hsf records */
static int N_rhsfs = 0;
static int Free_loc = 0;		/* next put location of Rhsfs */

static void Output_hsf (Vdeal_t *vdv) {
    Hs_feature_t *new_fs;		/* new features */
    ORPG_hsf_t hsf_msg[MAXN_HSF_IN_MSG];
    int n, cnt, i;

    /* store the new high shear features */
    n = VDA_get_reset_hsf (0, (void **)&new_fs);
    for (i = 0; i < n; i++) {
	ORPG_hsf_t *rf = Rhsfs + Free_loc;
	rf->vol_seq = Vol_seq_num;
	rf->time = vdv->dtm;
	rf->elev_num = Elev_num;
	rf->range = new_fs[i].x + 1;
	rf->azi = new_fs[i].y + 1;
	rf->n_bins = new_fs[i].xz;
	rf->n_rs = new_fs[i].yz;
	rf->n = new_fs[i].n;
	rf->maxs = new_fs[i].maxs;
	rf->mins = new_fs[i].mins;
	rf->nyq = new_fs[i].nyq;
	rf->unamb_range = new_fs[i].unamb_range;
	rf->type = new_fs[i].type;
	rf->min_size = new_fs[i].min_size;
	rf->threshold = new_fs[i].threshold;
	Free_loc = (Free_loc + 1) % MAXN_HSFS;
	if (N_rhsfs < MAXN_HSFS)
	    N_rhsfs++;
    }
    if (n == 0)
	return;

    /* export the hsfs in the latest two volumes */
    cnt = 0;
    for (i = 0; i < N_rhsfs; i++) {
	ORPG_hsf_t *rf = Rhsfs + ((Free_loc - 1 - i + MAXN_HSFS) % MAXN_HSFS);
	if (rf->vol_seq < Vol_seq_num - 1)	/* too old */
	    break;
	hsf_msg[i] = *rf;
	cnt++;
	if (cnt >= MAXN_HSF_IN_MSG)
	    break;
    }
    if (cnt > 0) {
	int ret = ORPGDA_write (ORPGDAT_ENVIRON_DATA_MSG, (char *)hsf_msg, 
		cnt * sizeof (ORPG_hsf_t), ORPGDAT_VELDEAL_HSF_MSG_ID);
	if (ret < 0)
	    MISC_log ("Write high shear features failed (%d)\n", ret);
    }
}


#ifdef MEM_ARCHIVE

/******************************************************************

    Archives radial "bh" for later retieval. Returns the radial size
    on success or -1 on failure. This is the memory version. I choose
    to use tje file version for saving memory.

******************************************************************/

#define RAD_SIZE 12000

static int Archive_radial (int func, int ind, void *data) {
    static char *rbuf = NULL;
    static int *sizes = NULL;
    static int b_size = 0;

    if (func == AR_INIT) {
	if (ind > b_size) {
	    if (sizes != NULL)
		free (sizes);
	    sizes = (int *)MISC_malloc (ind * sizeof (int));
	    if (rbuf != NULL)
		free (rbuf);
	    rbuf = (char *)MISC_malloc (ind * RAD_SIZE);
	    b_size = ind;
	}
	return (0);
    }

    else if (func == AR_SAVE) {
	int off, size;

	Base_data_header *bh = (Base_data_header *)data;
	size = bh->msg_len * sizeof (short);
	off = ind * RAD_SIZE;
	memcpy (rbuf + off, data, size);
	sizes[ind] = size;	
	return (size);
    }

    else if (func == AR_READ) {
	static char *buf = NULL;
	int off, size;

	size = sizes[ind];
	off = ind * RAD_SIZE;
	buf = rbuf + off;
	*((char **)data) = buf;
	return (size);
    }

    else
	return (-1);
}

#endif





