
/******************************************************************

    This is a tool that reads NEXRAD radar data from an LB and 
    generates volume files.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/11/19 21:19:14 $
 * $Id: save_volume_file.c,v 1.19 2013/11/19 21:19:14 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <bzlib.h>
#include <generic_basedata.h>
#include <rda_status.h>
#include <comm_manager.h>

#include <infr.h> 

enum {VOL_BADDATA, VOL_DATA, VOL_START, VOL_END};
enum {ST_VOL_STARTED, ST_RADIAL_STARTED, ST_VOL_END}; /* for cr_state */

static char *Vol_dir = NULL;
static char *Input_lb_name = NULL;
static char *Radar_name = NULL;
static char *Vol_file = NULL;

static int Print_header = 0;
static int Terse = 0;
static int Dave_requested = 0;	/* a temp option requested by Dave Zittel */
static int D_first_gate = 0, D_n_gates = 0; /* for data display */
static char D_field[256] = "", D_field1[256] = "";

/* local functions */
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Write_volume_file (char *buffer, int n_bytes, int cr_state);
static void Set_volume_file_name (time_t vol_time);
static void Get_vol_title (char *buf, NEXRAD_vol_title *cr_title);
static int Get_vol_status (char *buf, int len, time_t *msg_time, 
	char **icd_data, int *icd_len, int cr_state, int *is_radial);
static void Sig_handle (int sig);
static void Close_file ();
static void Set_radar_name (char *radar_id);
static void Print_generic_basedata_hd (Generic_basedata_header_t *ghd);
static void Print_abbrev_generic_basedata_hd (Generic_basedata_header_t *ghd);
static void Print_basedata_hd (ORDA_basedata_header *hd);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    int fd, cr_state;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Radar_name == NULL)
	Radar_name = STR_copy (Radar_name, "UNKNOWN");
    if (Vol_dir == NULL)
	Vol_dir = STR_copy (Vol_dir, ".");
    if (Input_lb_name == NULL) {
	char *cpt = getenv ("ORPGDIR");
	if (cpt == NULL) {
	    MISC_log ("ORPGDIR not defined\n");
	    exit (1);
	}
	Input_lb_name = STR_copy (Input_lb_name, cpt);
	Input_lb_name = STR_cat (Input_lb_name, "/ingest/resp.0");
    }

    fd = LB_open (Input_lb_name, LB_READ, NULL);
    if (fd < 0) {
	MISC_log ("LB_open %s failed (%d)\n", Input_lb_name, fd);
	exit (1);
    }

    MISC_sig_sigset (SIGINT, Sig_handle);
    MISC_sig_sigset (SIGTERM, Sig_handle);
    MISC_sig_sigset (SIGHUP, Sig_handle);

    cr_state = ST_VOL_END;
    while (1) {
	static char *buf = NULL;
	int len, status, icd_len, is_radial;
	time_t msg_time;
	char *icd_data;

	if (buf == NULL)
	    buf = MISC_malloc (NEX_MAX_PACKET_SIZE);

	len = LB_read (fd, buf + sizeof (NEXRAD_ctm_info), 
		NEX_MAX_PACKET_SIZE - sizeof (NEXRAD_ctm_info), LB_NEXT);
	if (len == LB_TO_COME) {
	    msleep (500);
	    continue;
	}
	if (len == LB_EXPIRED) {
	    MISC_log ("Cannot catch up with the input data\n");
	    continue;
	}
	if (len < 0) {
	    MISC_log ("LB_read %s failed (%d)\n", Input_lb_name, len);
	    exit (1);
	}
	if (len < NEX_PACKET_SIZE)
	    memset (buf + sizeof (NEXRAD_ctm_info) + len, 
					0, NEX_PACKET_SIZE - len);
	status = Get_vol_status (buf + sizeof (NEXRAD_ctm_info), 
		len, &msg_time, &icd_data, &icd_len, cr_state, &is_radial);
	if (status == VOL_BADDATA)
	    continue;
	if (status == VOL_END) {
	    if (cr_state == ST_VOL_END)
		Close_file ();
	    Write_volume_file (icd_data, icd_len, cr_state);
	    cr_state = ST_VOL_END;
	    continue;
	}
	if (cr_state == ST_VOL_END || 
	    (cr_state == ST_RADIAL_STARTED && status == VOL_START)) {
	    Close_file ();
	    cr_state = ST_VOL_STARTED;
	}
	if (cr_state == ST_VOL_STARTED && is_radial) {
	    cr_state = ST_RADIAL_STARTED;
	    Set_volume_file_name (msg_time);
	}
	Write_volume_file (icd_data, icd_len, cr_state);
    }

    exit (0);
}

/*******************************************************************

    Opens a new volume file.

*******************************************************************/

static void Close_file () {

    Write_volume_file (NULL, 0, 0);		/* start a new file */
}

/*******************************************************************

    Signal handler.

*******************************************************************/

static void Sig_handle (int sig) {

    Write_volume_file (NULL, 0, 0);
    exit (0);
}

/*******************************************************************

    Verifies the radial data in "buf' of "len" bytes. Returns the 
    volume status. The message time and pointer to the ICD message
    header are returned with "msg_time" and "icd_data" (starting
    at the CTM header). The number of bytes of the ICD data is returned
    via "icd_len".

*******************************************************************/

static int Get_vol_status (char *buf, int len, time_t *msg_time, 
	char **icd_data, int *icd_len, int cr_state, int *is_radial) {
    RDA_RPG_message_header_t *msg_header;
    unsigned short type, msg_len, num_segs, seg_num;
    int date, ms;

    *is_radial = 0;
    if (len < 4) {
	MISC_log ("Data size is too small (%d)\n", len);
	return (VOL_BADDATA);
    }
    *icd_len = NEX_PACKET_SIZE;

#ifdef LITTLE_ENDIAN_MACHINE
    if (buf[2] == '\0' && buf[3] == '\0') {
#else
    if (buf[0] == '\0' && buf[1] == '\0') {
#endif
	int cm_type;

	if (len < sizeof (CM_resp_struct))	/* comms manager header size */
	    return (VOL_BADDATA);
	cm_type = ((int *)buf)[0];
	if (cm_type != CM_DATA)
	    return (VOL_BADDATA);
	if (len < sizeof (RDA_RPG_message_header_t) + 24 + 12)
	    return (VOL_BADDATA);
	msg_header = (RDA_RPG_message_header_t *)(buf + 24 + 12); 
			/* comm_manager header and CTM header removed */
    }
    else {
	if (len < sizeof (RDA_RPG_message_header_t))
	    return (VOL_BADDATA);
	msg_header = (RDA_RPG_message_header_t *)buf;
    }
    msg_len = SHORT_BSWAP_L (msg_header->size);
    type = msg_header->type;
    num_segs = SHORT_BSWAP_L (msg_header->num_segs);
    seg_num = SHORT_BSWAP_L (msg_header->seg_num);
    date = SHORT_BSWAP_L (msg_header->julian_date);
    ms = INT_BSWAP_L (msg_header->milliseconds);
    if (Print_header && !Terse) {
	printf ("type %d, size %d, n_segs %d, seg %d, date %d, ms %d, plen %d\n",
		type, msg_len, num_segs, seg_num, date, ms, len);
    }
    if (type <= 0 || type > 31 ||
	msg_len * 2 > len || msg_len < 8 ||
	num_segs == 0 || 
	seg_num == 0 || seg_num > num_segs) {
	MISC_log ("Data is not in NEXRAD level II format (type %d, msg_len %d, n_segs %d, seg_n %d\n", 
				type, msg_len, num_segs, seg_num);
	return (VOL_BADDATA);
    }
    *msg_time = (time_t)((date - 1) * 86400 + ms / 1000);
    *icd_data = (char *)msg_header - sizeof (NEXRAD_ctm_info);
    memset (*icd_data, 0, sizeof (NEXRAD_ctm_info));

    if (type == LOOPBACK_TEST_RDA_RPG || type == LOOPBACK_TEST_RPG_RDA)
	return (VOL_BADDATA);
    if (type == DIGITAL_RADAR_DATA) {
	ORDA_basedata_header *data_header;
	short radial_status;

	if (len > 2500) {
	    MISC_log ("Data size is too large (type %d, size %d)\n", 
							type, len);
	    return (VOL_BADDATA);
	}
	if (Dave_requested) {
	    short rlen = 1208;
	    msg_header->size = SHORT_BSWAP_L (rlen);
	}
	*is_radial = 1;
	data_header = (ORDA_basedata_header *)msg_header;
	if (Print_header)
	    Print_basedata_hd (data_header);
	radial_status = SHORT_BSWAP_L (data_header->status);
 	radial_status &= 0xff;
	if (radial_status == 3)
	    return (VOL_START);
	if (radial_status == 4)
	    return (VOL_END);
	return (VOL_DATA);
    }
    else if (type == GENERIC_DIGITAL_RADAR_DATA) {
	static int radar_id_set = 0;
	Generic_basedata_header_t *gbhd;
	short radial_status;

	gbhd = (Generic_basedata_header_t *)
		((char *)msg_header + sizeof (RDA_RPG_message_header_t));
	if (Print_header)
	    Print_generic_basedata_hd (gbhd);
	if (!radar_id_set) {
	    Set_radar_name (gbhd->radar_id);
	    radar_id_set = 1;
	}
	radial_status = gbhd->status;
	*icd_len = msg_len * sizeof (short) + sizeof (NEXRAD_ctm_info);

	*is_radial = 1;
	if (radial_status == 3)
	    return (VOL_START);
	if (radial_status == 4)
	    return (VOL_END);
	return (VOL_DATA);
    }
    else if (cr_state == ST_RADIAL_STARTED) {
	if (type == RDA_STATUS_DATA) {
	    int rda_status, chan;
	    chan = msg_header->rda_channel;
	    if (!(chan & RDA_RPG_MSG_HDR_ORDA_CFG)) {		/* Legacy */
		RDA_status_msg_t *status_l = (RDA_status_msg_t *)msg_header;
		rda_status = SHORT_BSWAP_L (status_l->rda_status);
	    }
	    else {						/* ORDA */
		ORDA_status_msg_t *status_o = (ORDA_status_msg_t *)msg_header;
		rda_status = SHORT_BSWAP_L (status_o->rda_status);
	    }
	    if (rda_status != RS_OPERATE)
		return (VOL_END);
	}
	else
	    return (VOL_START);
    }

    return (VOL_DATA);
}

/*******************************************************************

    Sets Radar_name to "radar_id" if it is "UNKNOWN".

*******************************************************************/

static void Set_radar_name (char *radar_id) {

    if (strcmp (Radar_name, "UNKNOWN") == 0) {
	Radar_name = STR_reset (Radar_name, 0);
	Radar_name = STR_append (Radar_name, radar_id, 4);
	Radar_name = STR_append (Radar_name, "", 1);
    }
}

/*******************************************************************

    Creates and returns the volume title of radial data in "buf".

*******************************************************************/

static void Get_vol_title (char *buf, NEXRAD_vol_title *cr_title) {
    static int vol_num = 0;
    RDA_RPG_message_header_t *msg_header;

    msg_header = (RDA_RPG_message_header_t *)(buf + sizeof (NEXRAD_ctm_info));
    vol_num++;
    vol_num = vol_num % 1000;
    sprintf (cr_title->filename, "ARCHIVE2.%.3d", vol_num);
    cr_title->julian_date = SHORT_BSWAP_L (msg_header->julian_date);
    cr_title->millisecs_past_midnight = msg_header->milliseconds;
    cr_title->julian_date = INT_BSWAP_L (cr_title->julian_date);
    cr_title->filler1 = 0;
}

/*******************************************************************

    Write to the current volume file "Vol_file" "n_bytes" data
    in "buffer". If "buffer" is NULL, the current file is closed.
    return 0 on success or -1 on failure.

*******************************************************************/

static int Write_volume_file (char *buffer, int n_bytes, int cr_state) {
    static FILE *fl = NULL;
    static BZFILE *bf;
    static char *saved_bytes = NULL;
    static int n_saved_bytes = 0;
    int berr;

    if (Print_header)
	return (0);

    if (buffer == NULL) {
	if (fl != NULL) {
	    if (n_saved_bytes > 0)
		BZ2_bzWrite (&berr, bf, saved_bytes, n_saved_bytes);
	    BZ2_bzWriteClose (&berr, bf, 0, NULL, NULL);
	    if (berr != BZ_OK)
		fprintf (stderr, "BZ2_bzWriteClose failed (bzerr %d)\n", berr);
	    fclose (fl);
	    fl = NULL;
	    if (Vol_file != NULL)
		Vol_file[0] = '\0';
	}
	if (n_saved_bytes > 0) {
	    n_saved_bytes = 0;
	    saved_bytes = STR_reset (saved_bytes, 0);
	}
	return (0);
    }

    if (n_bytes > 0 && cr_state >= 0 && cr_state != ST_RADIAL_STARTED) {
	saved_bytes = STR_append (saved_bytes, buffer, n_bytes);
	n_saved_bytes += n_bytes;
	return (0);
    }

    if (fl == NULL) {		/* open the file */
	NEXRAD_vol_title title;

	if (Vol_file == NULL || strlen (Vol_file) == 0) {
	    fprintf (stderr, "Volume file name not set - unexpected\n");
	    return (-1);
	}
	printf ("New volume file %s\n", Vol_file);
 	fl = fopen (Vol_file, "w");
	if (fl == NULL) {
	    fprintf (stderr, "fopen %s for writing failed (errno %d)\n", 
						Vol_file, errno);
	    return (-1);
	}
	bf = BZ2_bzWriteOpen (&berr, fl, 9, 0, 0);
	if (berr != BZ_OK) {
	    fprintf (stderr, "BZ2_bzWriteOpen failed (bzerr %d)\n", berr);
	    fclose (fl);
	    fl = NULL;
	    return (-1);
	}
	Get_vol_title (buffer, &title);
	Write_volume_file ((char *)&title, sizeof (NEXRAD_vol_title), -1);
    }
    if (n_bytes == 0)
	return (0);

    berr = BZ_OK;
    if (n_saved_bytes > 0 && cr_state >= 0) {
	BZ2_bzWrite (&berr, bf, saved_bytes, n_saved_bytes);
	n_saved_bytes = 0;
	saved_bytes = STR_reset (saved_bytes, 0);
    }
    if (berr == BZ_OK)
	BZ2_bzWrite (&berr, bf, buffer, n_bytes);
    if (berr != BZ_OK) {
	fprintf (stderr, "BZ2_bzWrite failed (bzerr %d)\n", berr);
	BZ2_bzWriteClose (&berr, bf, 0, NULL, NULL);
	if (berr != BZ_OK)
	    fprintf (stderr, "BZ2_bzWriteClose failed (bzerr %d)\n", berr);
	fclose (fl);
	fl = NULL;
	return (-1);
    }
    return (0);
}

/******************************************************************

    Sets up the volume file name "Vol_file" in terms of volume time 
    "vol_time" and label "Label". If "vol_time" is 0, the name is 
    not set.

******************************************************************/

static void Set_volume_file_name (time_t vol_time) {
    int y, mon, d, h, m, s;

    if (vol_time == 0)
	return;

    unix_time (&vol_time, &y, &mon, &d, &h, &m, &s);
    Vol_file = STR_reset (Vol_file, strlen (Vol_dir) + 
					strlen (Radar_name) + 128);
    sprintf (Vol_file, 
		"%s/%s_%.4d_%.2d_%.2d_%.2d_%.2d_%.2d.bz2", 
			Vol_dir, Radar_name, y, mon, d, h, m, s);
    return;
}

/******************************************************************

    Prints selected fields in ORDA_basedata_header.

******************************************************************/

static void Print_basedata_hd (ORDA_basedata_header *hd) {

    printf ("    status %d, elev_n %d, elev %d, azi_n %d, azi %d\n", 
		SHORT_BSWAP_L (hd->status), SHORT_BSWAP_L (hd->elev_num), 
		SHORT_BSWAP_L (hd->elevation), SHORT_BSWAP_L (hd->azi_num), 
		SHORT_BSWAP_L (hd->azimuth));
    printf ("    vcp %d, n_surv %d, n_dop %d, date %d, ms %ld\n", 
		SHORT_BSWAP_L (hd->vcp_num), SHORT_BSWAP_L (hd->n_surv_bins), 
		SHORT_BSWAP_L (hd->n_dop_bins), SHORT_BSWAP_L (hd->date), 
		INT_BSWAP_L (hd->time)); 
}

/******************************************************************

    Prints selected fields in Generic_basedata_header_t.

******************************************************************/

static void Print_generic_basedata_hd (Generic_basedata_header_t *ghd) {
    float azi, elev;
    int n_d, i, major_version = 1;
    char rid[8];

    azi = ghd->azimuth;
    elev = ghd->elevation;
    FLOAT_BSWAP_L (azi);
    FLOAT_BSWAP_L (elev);
    memcpy (rid, ghd->radar_id, 4);
    rid[4] = '\0';

    if( Terse )
       return( Print_abbrev_generic_basedata_hd( ghd ) );

    printf ("    status %d, elev_n %d, elev %f, azi_n %d, azi %f\n", 
		ghd->status, ghd->elev_num, 
		elev, SHORT_BSWAP_L (ghd->azi_num), 
		azi);
    n_d = SHORT_BSWAP_L (ghd->no_of_datum);
    printf ("    radar \"%s\", date %d, ms %ld, no_of_datum %d\n", rid,
		SHORT_BSWAP_L (ghd->date), INT_BSWAP_L (ghd->time), n_d); 
    if( ghd->spot_blank_flag & 0x1 )
        printf("   Radial is Spot Blanked\n" );
    else if( ghd->spot_blank_flag & 0x2 )
        printf("   Elevation is Spot Blanked\n" );
    else if( ghd->spot_blank_flag & 0x4 )
        printf("   Volume is Spot Blanked\n" );
    for (i = 0; i < n_d; i++) {
	int off;
	char type[8], *p;

	off = ghd->data[i];
	off = INT_BSWAP_L (off);
	p = (char *)ghd + off;
	strncpy (type, p, 4);
	type[4] = '\0';
	if (strcmp (type, "RVOL") == 0) {
	    float calib_const, lat, lon, horiz_shv_tx_power;
	    Generic_vol_t *hd = (Generic_vol_t *)p;

	    major_version = hd->major_version; 
	    calib_const = hd->calib_const;
	    FLOAT_BSWAP_L (calib_const);
	    lat = hd->lat;
	    lon = hd->lon;
	    FLOAT_BSWAP_L (lat);
	    FLOAT_BSWAP_L (lon);
	    horiz_shv_tx_power = hd->horiz_shv_tx_power;
	    FLOAT_BSWAP_L (horiz_shv_tx_power);
	    printf ("        %s: len %d, major version: %d, vcp %d, calib_const %8.4f\n", type,
		SHORT_BSWAP_L (hd->len), major_version, SHORT_BSWAP_L (hd->vcp_num), calib_const);
	    printf ("            lat %8.4f, lon %8.4f, height %d, h_tx_p %8.4f\n",
		lat, lon, SHORT_BSWAP_L (hd->height), horiz_shv_tx_power);
	}
	else if (strcmp (type, "RELV") == 0) {
	    Generic_elev_t *hd = (Generic_elev_t *)p;

	    printf ("        %s: len %d, atmos %d\n", type,
		SHORT_BSWAP_L (hd->len), (short)(SHORT_BSWAP_L (hd->atmos)));
	}
	else if (strcmp (type, "RRAD") == 0) {
	    float horiz_noise, vert_noise;
	    Generic_rad_t *hd = (Generic_rad_t *)p;

	    horiz_noise = hd->horiz_noise;
	    FLOAT_BSWAP_L (horiz_noise);
	    vert_noise = hd->vert_noise;
	    FLOAT_BSWAP_L (vert_noise);
	    printf ("        %s: len %d, unamb_r %d, nyq_v %d, h_noise %8.4f, v_noise %8.4f\n", type,
		SHORT_BSWAP_L (hd->len), SHORT_BSWAP_L (hd->unamb_range), 
		SHORT_BSWAP_L (hd->nyquist_vel), horiz_noise, vert_noise);
            if (major_version == GENERIC_RAD_DBZ0_MAJOR) {
               Generic_rad_dBZ0_t *dbz0 = (Generic_rad_dBZ0_t *) ((char *) hd + sizeof(Generic_rad_t));
               float h_dBZ0 = dbz0->h_dBZ0;
               float v_dBZ0 = dbz0->v_dBZ0;
               FLOAT_BSWAP_L(h_dBZ0);
               FLOAT_BSWAP_L(v_dBZ0);
	       printf ("            h_dBZ0 %8.4f, v_dBZ0 %8.4f\n", h_dBZ0, v_dBZ0);
	    }
	}
	else if (strcmp (type, "DREF") == 0 ||
		 strcmp (type, "DSW ") == 0 ||
		 strcmp (type, "DSW") == 0 ||
		 strcmp (type, "DVEL") == 0 ||
		 strcmp (type, "DPHI") == 0 ||
		 strcmp (type, "DRHO") == 0 ||
		 strcmp (type, "DSNR") == 0 ||
		 strcmp (type, "DZDR") == 0) {
	    float scale, offset;
	    Generic_moment_t *hd = (Generic_moment_t *)p;

	    scale = hd->scale;
	    FLOAT_BSWAP_L (scale);
	    offset = hd->offset;
	    FLOAT_BSWAP_L (offset);
	    printf ("        %s: n_gates %d, R0 %d, bin_s %d, tover %d, thr %d, res %d\n",
		type,
		SHORT_BSWAP_L (hd->no_of_gates), 
		(short)(SHORT_BSWAP_L (hd->first_gate_range)), 
		SHORT_BSWAP_L (hd->bin_size), SHORT_BSWAP_L (hd->tover), 
		SHORT_BSWAP_L (hd->SNR_threshold), 
		hd->data_word_size);
	    printf ("              scale %8.4f, offset %8.4f\n", scale, offset);
	    if (D_n_gates > 0 && 
		(strcmp (D_field, type) == 0 || 
					strcmp (D_field1, type) == 0)) {
		if (hd->data_word_size == 8) {
		    unsigned char *v;
		    int i;
		    v = hd->gate.b + D_first_gate;
		    for (i = 0; i < D_n_gates; i++) {
			printf ("%d ", v[i]);
		    }
		    printf ("\n");
		}
		if (hd->data_word_size == 16) {
		    unsigned short *v;
		    int i;
		    v = hd->gate.u_s + D_first_gate;
		    for (i = 0; i < D_n_gates; i++) {
			printf ("%d ", v[i]);
		    }
		    printf ("\n");
		}
	    }
	}
	else {
	    printf ("        Unexpected type %s\n", type);
	}
    }
}


/******************************************************************

    Prints selected fields in Generic_basedata_header_t.

******************************************************************/

static void Print_abbrev_generic_basedata_hd (Generic_basedata_header_t *ghd) {

    float azi, elev, horiz_noise, vert_noise;
    int n_d, i, rdate, rtime, mills;
    int yr = 0, mon = 0, day = 0, hr = 0, min = 0, sec = 0;
    time_t ttime = 0;
    char cdate[16], ctime[16];

    azi = ghd->azimuth;
    elev = ghd->elevation;
    FLOAT_BSWAP_L (azi);
    FLOAT_BSWAP_L (elev);
    n_d = SHORT_BSWAP_L (ghd->no_of_datum);
    rdate = SHORT_BSWAP_L (ghd->date);
    rtime = INT_BSWAP_L (ghd->time);

    horiz_noise = -99.9999;
    vert_noise = -99.9999;
    for (i = 0; i < n_d; i++) {
        int off;
        char type[8], *p;

        off = ghd->data[i];
        off = INT_BSWAP_L (off);
        p = (char *)ghd + off;
        strncpy (type, p, 4);
        type[4] = '\0';
         
        if (strcmp (type, "RRAD") == 0) {
            Generic_rad_t *hd = (Generic_rad_t *)p;

            horiz_noise = hd->horiz_noise;
            FLOAT_BSWAP_L (horiz_noise);
            vert_noise = hd->vert_noise;
            FLOAT_BSWAP_L (vert_noise);
            break;
        }

    }

    mills = rtime - (rtime/1000)*1000;
    ttime = (rdate-1)*86400 + rtime/1000;
    unix_time( &ttime, &yr, &mon, &day, &hr, &min, &sec );
    if( yr >= 2000 )
       yr -= 2000;
    else
       yr -= 1900;
    sprintf( cdate, "%2.2d/%2.2d/%2.2d", mon, day, yr );
    sprintf( ctime, "%2.2d:%2.2d:%2.2d.%3.3d", hr, min, sec, mills );
    printf( "%s  %s  %2d  %4.1f  %2d  %5.1f  %8.4f  %8.4f\n", 
            cdate, ctime, ghd->elev_num, elev, SHORT_BSWAP_L (ghd->azi_num), 
            azi, horiz_noise, vert_noise );
}


/**************************************************************************

    Reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "ad:i:r:D:tph?")) != EOF) {
	switch (c) {

            case 'a':
               Terse = 1;
               Print_header = 1;
               break;

            case 'd':
		Vol_dir = STR_copy (Vol_dir, optarg);
                break;

            case 'i':
		Input_lb_name = STR_copy (Input_lb_name, optarg);
                break;

            case 'r':
		Radar_name = STR_copy (Radar_name, optarg);
                break;

            case 'p':
		Print_header = 1;
                break;

            case 't':
		Dave_requested = 1;
                break;

            case 'D':
		if (sscanf (optarg, "%d%*c%d", &D_first_gate, &D_n_gates) != 2 ||
		    MISC_get_token (optarg, "S,", 2, D_field, 256) <= 0) {
		    fprintf (stderr, "Unexpected -D option (%s)\n", optarg);
		    exit (1);
		}
		if (MISC_get_token (optarg, "S,", 3, D_field1, 256) <= 0)
		    D_field1[0] = '\0';
                break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    return (err);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Reads NEXRAD level II data from an LB and creates volume files\n\
        which can then be read by play_a2 for playback. Use control-c\n\
	to terminate.\n\
        Options:\n\
          -a (Prints level II data header info only (abbreviated) - File not created)\n\
          -d dir (\"dir\" is the directory for the volume files.\n\
                  The default is the current directory.)\n\
          -i LB_name (\"LB_name\" is the name of input LB.\n\
                      The default is $ORPGDIR/ingest/resp.0)\n\
          -r radar_name (\"radar_name\" is the radar name (e.g. KTLX).\n\
                      The default is from the radial data or UNKNOWN.)\n\
          -p (Prints level II data header info only - File not created)\n\
          -D start_gate,n_gates,field (Prints level II data)\n\
          -h (Prints usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}



