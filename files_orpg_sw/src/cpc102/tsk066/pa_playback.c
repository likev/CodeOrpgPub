

/******************************************************************

    This is a tool that reads and ingests NEXRAD radar data
    from volume files or tape archive. This is the playback
    module.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/24 19:38:07 $
 * $Id: pa_playback.c,v 1.32 2014/04/24 19:38:07 steves Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bzlib.h>
#include <zlib.h>
#include <errno.h>
#include <sys/time.h>

#include <rda_status.h> 
#include <orpgumc_rda.h> 
#include <generic_basedata.h> 
#include <infr.h> 
#include <rpg_vcp.h>
#include "pa_def.h" 

static double Play_speed;		/* Playback speed */
static int Verbose;			/* verbose mode flag */
static int Lb_fd = -1;			/* output LB file descriptor */
static int Interactive_mode;		/* interactive mode */
static int Ignore_sails;		/* ignore sails cuts in playback */
static int In_speed_control_session = 0;/* in a speed control session */
static char *(*Convert_to_31) (RDA_RPG_message_header_t *, char *);
static VCP_t VCP_data;
static VCP_t VCP_message;
static float Target_elevation[ECUTMAX];
static short Rdccon[ECUTMAX];
static int VCP_n_cuts = 0;

static int Write_message (RDA_RPG_message_header_t *msg_header);
static int Read_data (Ap_vol_file_t *vf, char *buffer, int buffer_size);
static int Read_z_file (Ap_vol_file_t *vf, char *buffer, int buffer_size);
static int Read_gz_file (Ap_vol_file_t *vf, char *buffer, int buffer_size);
static int Read_ldm_file (Ap_vol_file_t *vf, char *buffer, int buffer_size);
static double Get_current_time ();
static double Get_current_systime ();
static void Speed_control (double data_time);
static int Check_volume_header (int nbytes, char *buffer, Ap_vol_file_t *vf);
static double Convert_time (int julian_date, int millisecs_past_midnight);
static void Print_filename_time (char *path, time_t vtime);
static int Is_empty_message (RDA_RPG_message_header_t *msg_header);
static RDA_RPG_message_header_t* Save_vcp_data (RDA_RPG_message_header_t *msg_header);
static void byte_swap_msg_header (RDA_RPG_message_header_t *msg_header);
static void byte_swap_data_header (ORDA_basedata_header *data_header);
static void byte_swap_generic_header (Generic_basedata_header_t *gbhd);


/******************************************************************

    Initializas this module.

******************************************************************/

int PAP_initialize (double speed, int verbose, 
                    int interactive_mode, int ignore_sails) {

    Play_speed = speed;
    Verbose = verbose;
    Interactive_mode = interactive_mode;
    Ignore_sails = ignore_sails;

    if (PAM_convert_to_31 ()) {
	Convert_to_31 = (char *(*) (RDA_RPG_message_header_t *, char *))
		MISC_get_func ("liborpg.so", "UMC_convert_to_31", 0);
	if (Convert_to_31 == NULL) {
	    fprintf (stderr, "MISC_get_func failed\n");
	    exit (1);
	}
    }

    return (0);
}

/******************************************************************

    Opens the LB for playback. Returns 0 on success or -1 on failure.

******************************************************************/

int PAP_open_playback_lb () {
    char *lb_name, dir_name[LOCAL_NAME_SIZE], *file_name, *full_name;

    if (Lb_fd >= 0)
	return (0);		/* already open */

    lb_name = PAM_get_options (PLAYBACK_LB);
    if (lb_name[0] != '/' && lb_name[0] != '.') {
	char *cpt;

	cpt = getenv ("ORPGDIR");
	if (cpt == NULL) {
	    fprintf (stderr, "ORPGDIR not defined\n");
	    exit (1);
	}
	if (strlen (cpt) + 1 >= LOCAL_NAME_SIZE) {
	    fprintf (stderr, "ORPGDIR too long\n");
	    exit (1);
	}
	strcpy (dir_name, cpt);
	if (dir_name[strlen (dir_name) - 1] != '/')
	    strcat (dir_name, "/");
    }
    else
	dir_name[0] = '\0';
    if (lb_name[0] == '\0')
	file_name = "ingest/resp.0";
    else
	file_name = lb_name;

    full_name = PAF_get_full_path (dir_name, file_name);
    Lb_fd = LB_open (full_name, LB_WRITE, NULL);
    if (Lb_fd < 0) {
	fprintf (stderr, "LB_open %s failed (%d)\n", full_name, Lb_fd);
	return (-1);
    }

    return (0);
}

/******************************************************************

    Resets the playback speed.

******************************************************************/

void PAP_set_speed (double speed) {

    Play_speed = speed;
}

/******************************************************************

    Returns the playback speed.

******************************************************************/

double PAP_get_speed () {

    return (Play_speed);
}

/******************************************************************

    This function is called with non-zero "true" when a playback 
    session starts. It is called with "tue" = 0 when a playback 
    session ends.

******************************************************************/

void PAP_session_start (int true) {

    if (!PAM_full_speed_simulation ())
	return;
    if (true) {
	Speed_control (0.);
	In_speed_control_session = 1;
    }
    else
	In_speed_control_session = 0;
}

/******************************************************************

    Plays back the volume file "vf" and puts the radials in the 
    output LB. Returns 0 on success, -1 if an error is detected.

******************************************************************/

int PAP_playback_volume (Ap_vol_file_t *vf) {
    static char *buffer = NULL;
    int nbytes;		/* available bytes in buffer */

    if (buffer == NULL)
	buffer = MISC_malloc (NEX_MAX_PACKET_SIZE);

    Read_data (NULL, NULL, 0);
    nbytes = Read_data (vf, buffer, NEX_PACKET_SIZE);
    if (nbytes <= 0)
	return (nbytes);
    if (vf->content_type != AP_TAPE_TYPE && vf->content_type != AP_LDM_TYPE
		&& (nbytes = Check_volume_header (nbytes, buffer, vf)) <= 0)
	return (-1);

    if (!In_speed_control_session)
	Speed_control (0.);
    while (1) {
	RDA_RPG_message_header_t *msg_header;
	int type, n, hd_s, rec_s;

	hd_s = sizeof (RDA_RPG_message_header_t) + sizeof (NEXRAD_ctm_info);
	if (nbytes < hd_s) {
	    if ((n = Read_data (vf, buffer + nbytes, hd_s - nbytes)) >= 0)
		nbytes += n;
	    if (nbytes == 0)		/* no more data - done */
		return (0);
	    if (nbytes < hd_s) {
		if (Verbose)
		    fprintf (stderr, "Unused trailing bytes (%d) in file\n", nbytes);
		return (-1);
	    }
	}

	msg_header = (RDA_RPG_message_header_t *)(buffer + sizeof (NEXRAD_ctm_info));
	byte_swap_msg_header (msg_header);

	/* reads additional data to complete this record if necessary */
	/* NEX_PACKET_SIZE - The legacy message has msg_header->size = 1208
	   which is 2416 bytes plus 12 byte ctm header and 4 byte trailer. 
	   message 31 does not have the trailer. */
	type = msg_header->type;
	if (type == GENERIC_DIGITAL_RADAR_DATA)
	    rec_s = msg_header->size * sizeof (short) + 
						sizeof (NEXRAD_ctm_info);
	else
	    rec_s = NEX_PACKET_SIZE;
	if (rec_s > NEX_MAX_PACKET_SIZE) {
	    fprintf (stderr, "Bad data - record size %d\n", rec_s);
	    return (-1);
	}
	if (nbytes < rec_s) {
	    if ((n = Read_data (vf, buffer + nbytes, rec_s - nbytes)) >= 0)
		nbytes += n;
	    if (nbytes < rec_s) {
		if (Verbose)
		    fprintf (stderr, "Truncated record (type %d, size %d, expect %d)\n", 
				    			type, nbytes, rec_s);
		return (-1);
	    }
	}

	if (type == DIGITAL_RADAR_DATA) {
	    ORDA_basedata_header *data_header;
	    time_t t;
	    int ret;

	    data_header = (ORDA_basedata_header *)msg_header;
	    byte_swap_data_header (data_header);

	    if (data_header->elev_num > 1 && data_header->status == 3)
		data_header->status = 0;	/* correct RIDDS */
	    t = PAP_convert_time (data_header->date, data_header->time);
	    if (t < 1054735929) {	/* correct VCP number (for data before
					   6/4/03 09:12:46) */
		if (data_header->vcp_num == 77)
		    data_header->vcp_num = 12;
		if (data_header->vcp_num == 44)
		    data_header->vcp_num = 121;
	    }
	    if (vf->content_type == AP_TAPE_TYPE &&
		PAR_pause ((int)(data_header->status), t) != 0)
		return (0);
	    if (vf->content_type != AP_TAPE_TYPE) {
		ret = PAI_pause ((int)(data_header->status), t);
		if (ret == 1)
		    return (0);
		if (ret == 2)
		    Speed_control (0.);
	    }
	    Speed_control (Convert_time (msg_header->julian_date, 
			    msg_header->milliseconds));
/*
{
short *p, *end;
p = (short *)((char *)data_header + sizeof (ORDA_basedata_header));
end = (short *)(buffer + rec_s - 2);
while (p < end) {
    *p = rand ();
    p++;
}
}
*/
	    if (PAM_convert_to_31 ()) {
		char *m31;
		byte_swap_msg_header (msg_header);
		byte_swap_data_header (data_header);
		m31 = Convert_to_31 (msg_header, vf->prefix);
		byte_swap_msg_header ((RDA_RPG_message_header_t *)m31);
		byte_swap_generic_header ((Generic_basedata_header_t *)
				(m31 + sizeof (RDA_RPG_message_header_t)));
		ret = Write_message ((RDA_RPG_message_header_t *)m31);
	    }
	    else 
		ret = Write_message (msg_header);
	    if (ret != 0)
		return (-1);
	}
	else if (type == GENERIC_DIGITAL_RADAR_DATA) {
	    time_t t;
	    Generic_basedata_header_t *gbhd = (Generic_basedata_header_t *)
		((char *)msg_header + sizeof (RDA_RPG_message_header_t));

	    byte_swap_generic_header (gbhd);
	    t = PAP_convert_time (gbhd->date, gbhd->time);
	    if (vf->content_type == AP_TAPE_TYPE &&
		PAR_pause ((int)(gbhd->status), t) != 0)
		return (0);
	    if (vf->content_type != AP_TAPE_TYPE) {
		int ret = PAI_pause ((int)(gbhd->status), t);
		if (ret == 1)
		    return (0);
		if (ret == 2)
		    Speed_control (0.);
	    }
	    Speed_control (Convert_time (msg_header->julian_date, 
			    msg_header->milliseconds));
	    if (Write_message (msg_header) != 0)
		return (-1);
	}
	else if (type == RDA_STATUS_DATA) {
	    RDA_status_msg_t *st;
	    time_t t;

	    st = (RDA_status_msg_t *)msg_header;
	    t = PAP_convert_time (msg_header->julian_date, 
			    msg_header->milliseconds);
	    if (t < 1054735929) {	/* correct VCP number (for data before
					   6/4/03 09:12:46) */
		int vcp, sign;
		vcp = SHORT_BSWAP_L (st->vcp_num);
		sign = 1;
		if (vcp < 0) {
		    vcp = -vcp;
		    sign = -1;
		}
		if (vcp == 77)
		    vcp = 12;
		if (vcp == 44)
		    vcp = 121;
		vcp = sign * vcp;
		st->vcp_num = SHORT_BSWAP_L (vcp);
	    }
	    if (Write_message (msg_header) != 0)
		return (-1);
	}
	else if (type > 0 && type < 50) {	/* all ICD messages */
            if( (Ignore_sails) && (type == RDA_RPG_VCP) )
               msg_header = Save_vcp_data (msg_header);
	    if (!Is_empty_message (msg_header) &&
		Write_message (msg_header) != 0)
		return (-1);
	}
	else if (type == 0) {
	    if (msg_header->size == 0)		/* B12 LDM sets this to 0 */
		msg_header->size = 2416 / sizeof (short);
	    if (msg_header->size * sizeof (short) != 2416) {
		if (Verbose)
		    printf ("Bad data: Message type %d, size %d - discarded\n",
						type, msg_header->size);
	    }
	    else if (Write_message (msg_header) != 0)
		return (-1);
	}
	else {
	    if (Verbose)
		printf ("Bad data: Message type %d - discarded\n", type);
	}

	/* discard consumed bytes */
	if (nbytes > rec_s)
	    memmove (buffer, buffer + rec_s, nbytes - rec_s);
	nbytes -= rec_s;

	/* read next record - for efficiency */
	if (type != GENERIC_DIGITAL_RADAR_DATA) {
	    if ((n = Read_data (vf, buffer + nbytes, NEX_PACKET_SIZE - nbytes)) >= 0)
		nbytes += n;
	    if (nbytes <= 0)
		return (0);
	}
    }
    return (0);
}

/******************************************************************

    Checks and removed the first part of the volume file, "nbytes" 
    bytes in "buffer". Return data bytes left in buffer if to go 
    ahead or -1 if not.

******************************************************************/

static int Check_volume_header (int nbytes, char *buffer, Ap_vol_file_t *vf) {
    NEXRAD_vol_title *vol_title;

    if (nbytes < sizeof (NEXRAD_vol_title)) {
	if (nbytes > 0 && Verbose)
	    fprintf (stderr, "Unexpected file size (%d) - too small\n", nbytes);
	return (-1);
    }

    if (nbytes > 35 && buffer[21] == '-' && buffer[25] == '-' &&
	buffer[31] == ':' && buffer[34] == ':') { /* NCDC Header file */
	if (Verbose)
	    fprintf (stderr, "NCDC header file - not processed\n");
	return (-1);
    }

    vol_title = (NEXRAD_vol_title *)buffer;
    PAP_byte_swap_vol_title (vol_title);

    if (strncmp (vol_title->filename, "ARCHIVE2.", 9) != 0 &&
	strncmp (vol_title->filename, "AR2V", 4) != 0) {
	if (Verbose)
	    fprintf (stderr, "Volume header not found in file %s\n", vf->name);
	return (-1);
    }

    if (Verbose && Interactive_mode) {
	if (vf->time == 0) {
	    printf ("\r          %s (Volume time: %s)...", vf->name,
		    PAI_ascii_time (PAP_convert_time (vol_title->julian_date, 
			    vol_title->millisecs_past_midnight)));
	    fflush (stdout);
	}
	else {
	    printf ("\r          %s...", vf->name);
	    fflush (stdout);
	}
    }
    if (Verbose && !Interactive_mode) {
	Print_filename_time (vf->path, 
		PAP_convert_time (vol_title->julian_date, 
			    vol_title->millisecs_past_midnight));
    }

    /* discard NEXRAD_vol_title in front of the file */
    memmove (buffer, buffer + sizeof (NEXRAD_vol_title), 
				nbytes - sizeof (NEXRAD_vol_title));
    nbytes -= sizeof (NEXRAD_vol_title);

    return (nbytes);
}


/******************************************************************

    Writes a radial or a status message to the output LB file.
    Returns 0 on success or -1 on failure.

******************************************************************/

static int Write_message (RDA_RPG_message_header_t *msg_header) {
    int ret, len, type;

    if (Lb_fd < 0 && PAP_open_playback_lb () != 0)
	return (-1);

    len = msg_header->size * 2;
    if (len <= 0) {
	fprintf (stderr, "Bad msg_header->size (%d)\n", msg_header->size);
	return (-1);
    }
    type = msg_header->type;
    if (PAM_reset_data_time ()) {
	int d, julian_date, millisecs_past_midnight;
	double t;

	t = Get_current_time ();
	d = (int)(t / (86400000.0));
	julian_date = d + 1;
	millisecs_past_midnight = (int)(t - d * 86400000.0);
	msg_header->julian_date = julian_date;
	msg_header->milliseconds = millisecs_past_midnight;

	if (type == DIGITAL_RADAR_DATA) {
	    ORDA_basedata_header *data_header = 
				(ORDA_basedata_header *)msg_header;
	    data_header->date = julian_date;
	    data_header->time = millisecs_past_midnight;
	}
	else if (type == GENERIC_DIGITAL_RADAR_DATA) {
	    Generic_basedata_header_t *gbhd = (Generic_basedata_header_t *)
		((char *)msg_header + sizeof (RDA_RPG_message_header_t));
	    gbhd->date = julian_date;
	    gbhd->time = millisecs_past_midnight;
	}
    }

    if (type == DIGITAL_RADAR_DATA)
	byte_swap_data_header ((ORDA_basedata_header *)msg_header);
    else if (type == GENERIC_DIGITAL_RADAR_DATA){

	Generic_basedata_header_t *bhd = (Generic_basedata_header_t *)
		((char *)msg_header + sizeof (RDA_RPG_message_header_t));

        /* Determine if this is a SAILS cut? */
        if( Ignore_sails ) {

           int ind = bhd->elev_num - 1;

           if( (ind < 0) || (Rdccon[ind] < 0) ) {
              /* Ignore this cut. */
              return (0);
           }
  
           if( bhd->elev_num != Rdccon[ind] )
              bhd->elev_num = Rdccon[ind] ;

        }
        
	byte_swap_generic_header ((Generic_basedata_header_t *)
		((char *)msg_header + sizeof (RDA_RPG_message_header_t)));
    }
    byte_swap_msg_header (msg_header);

    ret = LB_write (Lb_fd, (char *)msg_header, len, LB_NEXT);
    if (ret < 0) {
	fprintf (stderr, "LB_write failed (%d)\n", ret);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Checks if "msg_header" is an empty message. The LDM ICD assumes
    maximum sizes for certain messages.

******************************************************************/

static int Is_empty_message (RDA_RPG_message_header_t *msg_header) {
    static int cr_num_segs = 0;

    if (msg_header->seg_num == 1)
	cr_num_segs = msg_header->num_segs;

    if (cr_num_segs > 0 && msg_header->seg_num > cr_num_segs)
	return (1);
    return (0);
}

/******************************************************************

    Reads "buffer_size" bytes from volume data "vf" and puts them in
    "buffer". This is the implementation for reading bz2 file. It
    calls other routines for different media and format. It returns
    the number of bytes read or -1 on error. The function saves the
    pointer of "vf" so the next call will continue to read if
    pointer "vf" is not changed. If "vf" = NULL, the current volume 
    file will be closed.

******************************************************************/

static int Read_data (Ap_vol_file_t *vf, char *buffer, int buffer_size) {
    static FILE *file = NULL;
    static BZFILE *handle = NULL;
    static char *cur_name = NULL;
    static int end_of_file = 0;
    int error, nbytes;

    if (vf == NULL) {
	cur_name = NULL;
	Read_z_file (vf, buffer, buffer_size);
	Read_gz_file (vf, buffer, buffer_size);
	Read_ldm_file (vf, buffer, buffer_size);
	PAR_read_tape_data (vf, buffer, buffer_size);
	return (0);
    }

    if (vf->content_type == AP_TAPE_TYPE)
	return (PAR_read_tape_data (vf, buffer, buffer_size));
    else if (vf->content_type == AP_LDM_TYPE)
	return (Read_ldm_file (vf, buffer, buffer_size));

    if (vf->compress_type == AP_COMP_Z)
	return (Read_z_file (vf, buffer, buffer_size));
    else if (vf->compress_type == AP_COMP_GZ)
	return (Read_gz_file (vf, buffer, buffer_size));

    if (handle == NULL || cur_name != vf->path) {	/* open the file */
	if (handle != NULL) {
	    BZ2_bzReadClose (&error, handle);
	    fclose (file);
	    handle = NULL;
	}
	if ((file = fopen (vf->path, "r")) == NULL) {
	    fprintf (stderr, "fopen %s failed (errno %d)\n", vf->path, errno);
	    return (-1);
	}
	handle = (BZFILE *) BZ2_bzReadOpen (&error, file, 0, 0, NULL, 0);
	if (error != BZ_OK) {
	    fprintf (stderr, "BZ2_bzReadOpen %s failed (error %d)\n", vf->path, error);
	    fclose (file);
	    handle = NULL;
	    return (-1);
	}
	cur_name = vf->path;
	end_of_file = 0;
    }

    if (end_of_file)
	return (0);
    nbytes = BZ2_bzRead (&error, handle, buffer, buffer_size);
    if (error == BZ_STREAM_END) {
	end_of_file = 1;
	return (nbytes);
    }
    if (error != BZ_OK) {
	fprintf (stderr, "BZ2_bzRead failed (error %d)\n", error);
	BZ2_bzReadClose (&error, handle);
	fclose (file);
	handle = NULL;
	return (-1);
    }
    return (nbytes);
}

/******************************************************************

    Reads "buffer_size" bytes from volume file "vf" and puts them in
    "buffer". It returns the number of bytes read or -1 on error.
    The function saves the pointer of "vf" so the next call will
    continue to read if pointer "vf" is not changed. This is the
    Z (compress) version. If "vf" = NULL, the current volume file 
    will be closed.

******************************************************************/

static int Read_z_file (Ap_vol_file_t *vf, char *buffer, int buffer_size) {
    static FILE *file = NULL;
    static char *cur_name = NULL;
    int nbytes;

    if (vf == NULL) {
	cur_name = NULL;
	return (0);
    }

    if (file == NULL || cur_name != vf->path) {	/* open the file */
	char cmd[LOCAL_NAME_SIZE * 2 + 64];
	char work_file[LOCAL_NAME_SIZE * 2], *tmp_file;

	if (file != NULL) {
	    fclose (file);
	    file = NULL;
	}

	strcpy (work_file, 
		PAF_get_full_path (PAP_get_work_dir (), "play_a2.tmp"));
	tmp_file = PAF_get_full_path (work_file, ".Z");
	unlink (tmp_file);
	unlink (work_file);
	sprintf (cmd, "cp -f %s %s", vf->path, tmp_file);
	if (MISC_system_to_buffer (cmd, NULL, 0, NULL) != 0) {
	    fprintf (stderr, "Failed in executing %s\n", cmd);
	    return (-1);
	}
	sprintf (cmd, "uncompress %s", tmp_file);
	if (MISC_system_to_buffer (cmd, NULL, 0, NULL) != 0) {
	    fprintf (stderr, "Failed in executing %s\n", cmd);
	    return (-1);
	}
	unlink (tmp_file);

	if ((file = fopen (work_file, "r")) == NULL) {
	    fprintf (stderr, "fopen %s failed (errno %d)\n", work_file, errno);
	    return (-1);
	}
	unlink (work_file);
	cur_name = vf->path;
    }

    errno = 0;
    nbytes = fread (buffer, 1, buffer_size, file);
    if (nbytes == 0 && errno != 0) {
	fprintf (stderr, "fread failed (errno %d)\n", errno);
	return (-1);
    }
    return (nbytes);
}

/******************************************************************

    Reads "buffer_size" bytes from volume file "vf" and puts them in
    "buffer". It returns the number of bytes read or -1 on error.
    This is the gz version. If "vf" = NULL, the current volume file 
    will be closed.

******************************************************************/

static int Read_gz_file (Ap_vol_file_t *vf, char *buffer, int buffer_size) {
    static gzFile *file = NULL;
    static char *cur_name = NULL;
    static int end_of_file = 0;
    int nbytes;

    if (vf == NULL) {
	cur_name = NULL;
	return (0);
    }

    if (file == NULL || cur_name != vf->path) {	/* open the file */
	if (file != NULL) {
	    gzclose (file);
	    file = NULL;
	}
	if ((file = gzopen (vf->path, "r")) == NULL) {
	    fprintf (stderr, "gzopen %s failed (errno %d)\n", vf->path, errno);
	    return (-1);
	}
	cur_name = vf->path;
	end_of_file = 0;
    }

    if (end_of_file)
	return (0);
    nbytes = gzread (file, buffer, buffer_size);
    if (nbytes < 0) {
	int zerr = 0;
	gzerror (file, &zerr);
	fprintf (stderr, "gzread failed (error %d %d)\n", zerr, errno);
	gzclose (file);
	file = NULL;
	return (-1);
    }
    else if (nbytes == 0) {
	end_of_file = 1;
	return (nbytes);
    }
    return (nbytes);
}

/******************************************************************

    Reads "buffer_size" bytes from volume file "vf" and puts them in
    "buffer". It returns the number of bytes read or -1 on error.
    The function saves the pointer of "vf" so the next call will
    continue to read if pointer "vf" is not changed. This is the
    LDM file version. If "vf" = NULL, the current volume file 
    will be closed.

******************************************************************/

#define LDM_FILE_HEADER_SIZE 24		/* LDM file header size */

static int Read_ldm_file (Ap_vol_file_t *vf, char *buffer, int buffer_size) {
    static int fd = -1;
    static int data_len = 0, end_of_file = 0;
    static char *data, *buf = NULL, *decomp_buf = NULL;
    static int buf_size = 0, decomp_bsize = 500000;
    static char *cur_name = NULL;
    int ret;

    if (vf == NULL) {
	cur_name = NULL;
	return (0);
    }

    if (cur_name != vf->path) {		/* close the file */
	if (fd >= 0)
	    close (fd);
	fd = -1;
    }

    if (fd < 0) { 			/* open the file */
	char hd[LDM_FILE_HEADER_SIZE];

	cur_name = vf->path;
	fd = open (vf->path, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "open %s failed\n", vf->path);
	    return (-1);
	}
	/* read the file header */
	ret = MISC_read (fd, hd, LDM_FILE_HEADER_SIZE);
	if (ret < LDM_FILE_HEADER_SIZE) {
	    fprintf (stderr, "read file header failed (%d)\n", ret);
	    close (fd);
	    fd = -1;
	    return (-1);
	}
	/* check file header */
	if (strncmp (hd, "ARCHIVE2.", 9) != 0 &&
	    strncmp (hd, "AR2", 3) != 0) {
	    fprintf (stderr, "file header check failed\n");
	    close (fd);
	    fd = -1;
	    return (-1);
	}
	end_of_file = 0;
	data_len = 0;
    }

    if (!end_of_file && data_len <= 0) {	/* read next segment */
	int seg_size;

	ret = MISC_read (fd, (char *)&seg_size, 4);
	if (ret == 0) {		/* end of file */
	    data_len = 0;
	    end_of_file = 1;
	    goto done;
	}
	else if (ret < 4) {
	    fprintf (stderr, "read segment size failed (%d)\n", ret);
	    goto failed;
	}
#ifdef LITTLE_ENDIAN_MACHINE
	seg_size = INT_BSWAP (seg_size);
#endif
	if (seg_size < 0)
	    seg_size = -seg_size;
	if (seg_size <= 0) {
	    fprintf (stderr, "Bad segment size (size %d)\n", seg_size);
	    goto failed;
	}

	if (seg_size > buf_size) {
	    if (buf != NULL)
		free (buf);
	    buf_size = seg_size * 2;
	    buf = MISC_malloc (buf_size);
	    if (decomp_buf == NULL)
		decomp_buf = MISC_malloc (decomp_bsize);
	}

	ret = MISC_read (fd, buf, seg_size);
	if (ret < seg_size) {
	    fprintf (stderr, "read data (%d bytes) failed\n", seg_size);
	    goto failed;
	}

	while (1) {
	    data = decomp_buf;
	    data_len = decomp_bsize;
	    ret = BZ2_bzBuffToBuffDecompress ((char *)data, 
		(unsigned int *)&data_len, (char *)buf, seg_size, 0, 0);
	    if (ret == BZ_OUTBUFF_FULL) {
		if (decomp_buf != NULL)
		    free (decomp_buf);
		decomp_bsize *= 2;
		decomp_buf = MISC_malloc (decomp_bsize);
		continue;
	    }
	    if (ret < 0) {
		fprintf (stderr, "BZ2 decompress failed (returns %d)\n", ret);
		goto failed;
	    }
	    break;
	}
    }

done:
    if (data_len == 0)		/* end of file */
	return (0);

    if (buffer_size <= data_len) {
	memcpy (buffer, data, buffer_size);
	data_len -= buffer_size;
	data += buffer_size;
	return (buffer_size);
    }
    fprintf (stderr, "Insufficient data (request %d, available %d)\n", 
				buffer_size, data_len);

failed:
    close (fd);
    fd = -1;
    return (-1);
}

/******************************************************************

    Returns the work directory.

******************************************************************/

char *PAP_get_work_dir () {
    static char work_dir[LOCAL_NAME_SIZE] = "";

    if (work_dir[0] == '\0') {
	char *cpt;
	cpt = getenv ("WORK_DIR");
	if (cpt == NULL)
	    cpt = "/tmp";
	if (strlen (cpt) + 1 >= LOCAL_NAME_SIZE) {
	    fprintf (stderr, "Work dir path (%s) too long\n", cpt);
	    exit (1);
	}
	strcpy (work_dir, cpt);
	if (work_dir[strlen (work_dir) - 1] != '/')
	    strcat (work_dir, "/");
    }
    return (work_dir);
}

/******************************************************************

    Returns the Unix time in ms.

******************************************************************/

static double Convert_time (int julian_date, int millisecs_past_midnight) {

    return ((double)(julian_date - 1) * 86400 * 1000 + 
					millisecs_past_midnight);
}

/******************************************************************

    Returns the Unix time in seconds.

******************************************************************/

time_t PAP_convert_time (int julian_date, int millisecs_past_midnight) {

    return ((time_t)(julian_date - 1) * 86400 + 
					millisecs_past_midnight / 1000);
}

/******************************************************************

    Returns the current clock time in ms.

******************************************************************/

static double Get_current_time () {
    struct timeval cur_time;

    gettimeofday (&cur_time, NULL);
    return ((double)cur_time.tv_sec * 1000 + cur_time.tv_usec * .001);
}

/******************************************************************

    Returns the current system time in ms.

******************************************************************/

static double Get_current_systime () {
    time_t t;
    int ms;

    t = MISC_systime (&ms);
    return ((double)t * 1000. + ms);
}

/******************************************************************

    Sleeps for a while to adjust the play back speed. "Play_speed" 
    of value 1.0 indicates the real time, 2.0 indicates double
    speed playback and so on. "data_time" is the current data time.
    "data_time" = 0 resets the speed control.

******************************************************************/

static void Speed_control (double data_time) {
    static int cnt = 0;
    static double st_data_time, st_clock;
    double cur_clock, wait;
    int wait_ms;

    if (data_time == 0.) {
	cnt = 0;
	return;
    }

    if (Play_speed <= 0.) {
	msleep (25);
	return;
    }

    cnt++;
    if (cnt == 1) {
	st_data_time = data_time;
	st_clock = Get_current_systime ();
	return;
    }

    cur_clock = Get_current_systime ();
    wait = (data_time - st_data_time) - (cur_clock - st_clock) * Play_speed;
    if (wait >= 0.)
	wait_ms = (int)wait;
    else
	wait_ms = 0;
    if (wait_ms >= 10)
	msleep (wait_ms);
}

/******************************************************************

    Prints file name and volume time in the "play_ar2" format.

******************************************************************/

static void Print_filename_time (char *path, time_t vtime) {
    int y, mon, d, h, m, s;

    unix_time (&vtime, &y, &mon, &d, &h, &m, &s);
    printf ("Playing file: %s\n", path);
    printf ("Volume date [yyyy-mm-dd] %.4d-%.2d-%.2d    Volume time [hh:mm:ss]: %.2d:%.2d:%.2d\n\n", y, mon, d, h, m, s);
    return;
}

/******************************************************************

    Byte-swaps the message header.

******************************************************************/

static void byte_swap_msg_header (RDA_RPG_message_header_t *msg_header) {

    msg_header->milliseconds = INT_BSWAP_L (msg_header->milliseconds);
    msg_header->size = SHORT_BSWAP_L (msg_header->size);
    msg_header->sequence_num = SHORT_BSWAP_L (msg_header->sequence_num);
    msg_header->julian_date = SHORT_BSWAP_L (msg_header->julian_date);
    msg_header->num_segs = SHORT_BSWAP_L (msg_header->num_segs);
    msg_header->seg_num = SHORT_BSWAP_L (msg_header->seg_num);
}
  
/******************************************************************

    Byte-swaps some of the fields of ORDA_basedata_header.

******************************************************************/

static void byte_swap_data_header (ORDA_basedata_header *data_header) {

    data_header->time = INT_BSWAP_L (data_header->time);
    data_header->date = SHORT_BSWAP_L (data_header->date);
    data_header->elev_num = SHORT_BSWAP_L (data_header->elev_num);
    data_header->status = SHORT_BSWAP_L (data_header->status);
    data_header->vcp_num = SHORT_BSWAP_L (data_header->vcp_num);
}
  
/******************************************************************

    Byte-swaps some of the fields of Generic_basedata_header_t.

******************************************************************/

static void byte_swap_generic_header (Generic_basedata_header_t *gbhd) {

    gbhd->time = INT_BSWAP_L (gbhd->time);
    gbhd->date = SHORT_BSWAP_L (gbhd->date);
}

/******************************************************************

    Byte-swaps the volume title.

******************************************************************/

void PAP_byte_swap_vol_title (void *vol_title) {
    NEXRAD_vol_title *v_title = (NEXRAD_vol_title *)vol_title;

    v_title->julian_date = INT_BSWAP_L (v_title->julian_date); 
    v_title->millisecs_past_midnight = 
			INT_BSWAP_L (v_title->millisecs_past_midnight); 
}

/******************************************************************

    Save the RDA/RPG VCP message from the metadata.

******************************************************************/

RDA_RPG_message_header_t* Save_vcp_data( RDA_RPG_message_header_t *data ){

   int angle_deg_int, i, ind, size = 0;
   short n_cuts = 0, start_angle = 0, msg_size = 0;
   double angle_deg = 0;

   VCP_elevation_cut_header_t *vcp_hd, *vcp_hd_ws;

   /* Clear out the Rdccon data. */
   for( i = 0; i < ECUTMAX; i++ )
      Rdccon[i] = -1;

   /* Make a local copy of this VCP data. */
   size = data->size*sizeof(short);
   memcpy( (void *) &VCP_data, data, size );
   vcp_hd = &VCP_data.vcp_data.vcp_elev_data;

   /* Make a local copy without the SAILS cuts. */
   memcpy( (void *) &VCP_message.msg_hdr, data, sizeof(RDA_RPG_message_header_t) );
   memcpy( (void *) &VCP_message.vcp_data, (char *) data + sizeof(RDA_RPG_message_header_t),
           sizeof(VCP_message_header_t) );
   memcpy( (void *) &VCP_message.vcp_data.vcp_elev_data, vcp_hd, 16 );
   vcp_hd_ws = (VCP_elevation_cut_header_t *) ((char*) data + sizeof(RDA_RPG_message_header_t) +
                                                              sizeof(VCP_message_header_t));

   /* Convert the message to internal format. */
   MISC_swap_shorts (sizeof (VCP_message_header_t) / sizeof (short), 
                     (short *) &VCP_data.vcp_data.vcp_msg_hdr);
   MISC_swap_shorts (8, (short *)vcp_hd);
   MISC_swap_shorts (1, (short *)vcp_hd + 2);
   VCP_n_cuts = vcp_hd->number_cuts;

   for (i = 0; i < VCP_n_cuts; i++) {

       VCP_elevation_cut_data_t *h;

       h = &(vcp_hd->data[i]);
       MISC_swap_shorts (sizeof (VCP_elevation_cut_data_t) / sizeof (short), (short *)h);
       MISC_swap_shorts (2, (short *)h + 1);

   }

   /* Do For All elevation cuts in the VCP ... */
   start_angle = vcp_hd->data[0].angle;
   n_cuts = 0;
   for( ind = 0; ind < VCP_n_cuts; ind++ ){

      angle_deg = (double) (((double) (vcp_hd->data[ind].angle & 0xfff8)) * ORPGVCP_ELVAZM_BAMS2DEG);
      if( angle_deg > 180.0 )
         angle_deg -= 360.0;
      if( angle_deg > 0 )
         angle_deg_int = (int) ((angle_deg*10.0) + 0.5);
      else
         angle_deg_int = (int) ((angle_deg*10.0) - 0.5);

      Target_elevation[ind] = (angle_deg_int/10.0);
      
      /* Do not include SAILS cuts. */
      if( ind <= 1 ) {
         memcpy( (void *) &VCP_message.vcp_data.vcp_elev_data.data[n_cuts], 
                 &vcp_hd_ws->data[ind], 
                 sizeof (VCP_elevation_cut_data_t) / sizeof (short) );
         n_cuts++;
         Rdccon[ind] = n_cuts;
      }
      else {
         Rdccon[ind] = -1;
         if( vcp_hd->data[ind].angle != start_angle ){
            memcpy( (void *) &VCP_message.vcp_data.vcp_elev_data.data[n_cuts], 
                    &vcp_hd_ws->data[ind], 
                    sizeof (VCP_elevation_cut_data_t) / sizeof (short) );
            n_cuts++;
            Rdccon[ind] = n_cuts;
         }

      }

   }
 
   /* Set the message size and number of cuts. */
   msg_size = sizeof(RDA_RPG_message_header_t) + sizeof(VCP_message_header_t) + 
              16 + n_cuts*sizeof(VCP_elevation_cut_data_t);
   msg_size /= sizeof(short);

   /* The message header element does not need byte-swapping ... it will be
      done in the calling routine. */
   VCP_message.msg_hdr.size = msg_size;

   /* Save the number of cuts. */
   if( Ignore_sails )
      VCP_n_cuts = n_cuts;

   /* The VCP message element does need byte-swapping. */
   MISC_swap_shorts( 1, &n_cuts );
   VCP_message.vcp_data.vcp_elev_data.number_cuts = n_cuts; 


   if( Ignore_sails )
      return ((void *) &VCP_message );

   return (data);
}

