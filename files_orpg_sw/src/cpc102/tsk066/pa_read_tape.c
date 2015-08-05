/*******************************************************************

    This is a tool that reads and ingests NEXRAD radar data
    from volume files or tape archive. This is the tape playback
    module.

*******************************************************************/

/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2006/08/04 20:45:36 $
 * $Id: pa_read_tape.c,v 1.8 2006/08/04 20:45:36 cheryls Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/uio.h>
#include <bzlib.h>
#include <errno.h>

#include <basedata.h>
#include <infr.h> 
#include "pa_def.h" 


/* global variables */
static int Signal_received;
static int Quiet = 0 ;         /* run in quiet mode */

static char Label[LOCAL_NAME_SIZE] = "";
static char Vol_file[LOCAL_NAME_SIZE + 64];
static char Vol_dir[LOCAL_NAME_SIZE] = "";

static char *Buffer = NULL;	/* to hold the tape data */
static int Buffer_size = NEX_MAX_REC_LEN;
				/* size of Buffer - AR2 record size */

enum {DATA_VAILABLE, END_OF_FILE, END_OF_TAPE};	/* Tape_status values */
static int Tape_fd = -1;	/* tape file descriptor */
static int Tape_status;		/* current tape status */
static int Tape_type;		/* tape type */

enum {TAPE_UNKNOWN, TAPE_NCDC_Z, TAPE_NCDC_U};
				/* for Tape_type */

enum {PLAY_NOW, SKIP_VOLUME, SKIP_VOL_PLAY, TIME_PLAY};
				/* for argument func of Read_tape */

static int Save_volume_mode = 0;/* generate volume files instead of playback */

enum {PZF_WRITE, PZF_READ_TITLE, PZF_READ, PZF_CLOSE};
				/* for argument func of Process_Z_file */
enum {GVD_INIT, GVD_READ_HD, GVD_READ_DATA};
				/* for argument func of Get_volume_data* */
enum {RB_INIT, RB_RETURN, RB_READ};
				/* for argument func of Read_bytes */


static int Read_tape (int func, int n_skip_volumes, time_t st_time);
static int Rewind_tape ();
static int Read_bytes (int func, char *buffer, int buf_size);
static int Write_volume_file (char *buffer, int n_bytes);
static void Set_volume_file_name (time_t vol_time);
static int Create_volume_file (NEXRAD_vol_title *vol_title, 
					time_t cr_vol_time);
static int Set_options ();
static int Determine_tape_type ();
static int Get_volume_data (int func, char *buffer, int buf_size);
static int Get_volume_data_Z (int func, char *buffer, int buf_size);
static int Is_NCDC_Z_header (char *data, int n_bytes);
static int Process_Z_file (int func, char *buffer, int buf_size);


/******************************************************************

    The main loop function.

******************************************************************/

int PAR_main_loop () {
    char *dev;

    /* catch the signals */
    PAM_get_signal (&Signal_received);

    /* buffer space */
    if (Buffer == NULL &&
	(Buffer = (char *) malloc (Buffer_size)) == NULL) {
	printf ("Failed in allocating work buffer (size = %d)\n", Buffer_size);
	exit (1);
    }

    /* open the tape drive */
    dev = PAM_get_options (TAPE_DEVICE);
    printf ("Opening the tape drive (%s) ...\n", dev);
    if (Tape_fd < 0) {
	Tape_type = TAPE_UNKNOWN;
	Read_bytes (RB_INIT, NULL, 0);	/* cancel any returned data */
	Get_volume_data (GVD_INIT, NULL, 0);
	Get_volume_data_Z (GVD_INIT, NULL, 0);
	if ((Tape_fd = open (dev, O_RDONLY)) < 0) {
	    printf ("Error opening tape (%s)\n", dev);
	    perror ("");
	    exit (1);
	}
    }

    /* The main loop */
    while (1) {
	int func, cnt;

	Signal_received = 0;

	/* the main menu */
	printf ("\n\n");
	if (Save_volume_mode) {
	    printf ("  Save volume mode: Lebel %s, DIR %s\n\n", Label, Vol_dir);	    printf ("     0: Rewind tape\n");
	    printf ("     1: Read tape to save volume\n");
	    printf ("     2: Search a volume and read tape to save volume\n");
	    printf ("     3: Skip volumes\n");
	}
	else {
	    printf ("     0: Rewind tape\n");
	    printf ("     1: Playback tape\n");
	    printf ("     2: Search a volume and playback\n");
	    printf ("     3: Skip volumes\n");
	}
	printf ("     4: Options\n");
	printf ("     5: Exit\n");
	printf ("Enter a selection (0 to 5): ");

	while (sscanf (PAI_gets (), "%d", &func) != 1 || 
						func < 0 || func > 5) {
	    printf ("Bad input - not accepted - Enter a selection: ");
	}

	switch (func) {
	    time_t tm;
	    int args_read;

	    case 0:
		printf ("Rewinding the tape ...\n");
		Rewind_tape ();
		Get_volume_data (GVD_INIT, NULL, 0);
		break;

	    case 1:
		printf ("Reading the tape ... Use ctrl-c to terminate\n");
		Read_tape (PLAY_NOW, 0, 0);
		break;

	    case 2:
		while (1) {
		    printf ("Enter: start time (yy:mm:dd:hh:mn or hh:mn) or volume #: ");
		    args_read = PMI_parse_input_time (PAI_gets (), &tm);
		    if (Signal_received)
			break;
		    if (args_read == 5 || args_read == 2) {
			printf ("Searching for specified time and reading the tape ...\n");
			printf ("    Use ctrl-c to terminate\n");
			Read_tape (TIME_PLAY, 0, tm);
			break;
		    }
		    if (args_read == 1) {
			printf ("Searching for specified volume and reading the tape ...\n");
			printf ("    Use ctrl-c to terminate\n");
			Read_tape (SKIP_VOL_PLAY, tm, 0);
			break;
		    }
		    printf ("Bad input - not accepted\n");
		}
		break;

	    case 3:
		while (1) {
		    printf ("How many volumes to skip? ");
		    args_read = sscanf (PAI_gets (), "%d", &cnt);
		    if (Signal_received)
			break;
		    if (args_read != 1)
			printf ("Bad input - not accepted\n");
		    else {
			printf ("Skipping %d volumes ...\n", cnt);
			Read_tape (SKIP_VOLUME, cnt, 0);
			printf ("Done\n");
			break;
		    }
		}
		break;

	    case 4:
		if (Set_options () != 0)
		    return (0);
		break;

	    case 5:
		exit (0);

	    default:
		break;
	}
    }
}

/*******************************************************************

    Processes the save volumes menu.

*******************************************************************/

static int Set_options () {
    char *menu = "\
\n\
        1: Select a new playback speed (NOW %4.2f)\n\
        2: %s\n\
        3: Select a new radar label (NOW %s)\n\
        4: Select a new dir to save volume (NOW %s)\n\
        5: Save volume files to CD\n\
        6: Go to volume file playback mode\n\
        7: Done\n\
Enter a selection (1 to 7): ";

    if (Vol_dir[0] == '\0')
	strcpy (Vol_dir, PAP_get_work_dir ());

    while (1) {				/* The save volume menu loop */
	int func;
	char *mode_text, *label;

	Signal_received = 0;
	if (Save_volume_mode)
	    mode_text = "Select playback mode (NOW save volume mode)";
	else
	    mode_text = "Select save volume mode (NOW playback mode)";
	if (Label[0] == '\0')
	    label = "Undefined";
	else
	    label = Label;
	printf (menu, PAP_get_speed (), mode_text, label, Vol_dir);

	while (sscanf (PAI_gets (), "%d", &func) != 1 || 
					func <= 0 || func > 7) {
	    if (Signal_received)
		return (0);
	    printf ("Bad input - not accepted - Enter a selection: ");
	}

	switch (func) {
	    char tmp[STR_SIZE];
	    Ap_vol_file_t *vol_files;
	    int n_vol_files, ret;
	    double d;

	    case 1:
		while (1) {
		    printf ("Enter a new playback speed (> 0.): ");
		    ret = sscanf (PAI_gets (), "%lf", &d);
		    if (Signal_received)
			break;
		    if (ret != 1 || d <= 0.)
			printf ("Bad input - not accepted\n");
		    else {
			PAP_set_speed (d);
			break;
		    }
		}
		break;

	    case 2:
		if (Save_volume_mode)
		    Save_volume_mode = 0;
		else {
		    if (strlen (Label) == 0)
			printf ("Not ready: Radar label must be specified\n");
		    else
			Save_volume_mode = 1;
		}
		break;

	    case 3:
		while (1) {
		    printf ("Enter a new radar label (such as KTLX, at most 8 chars): ");
		    ret = sscanf (PAI_gets (), "%s", tmp);
		    if (Signal_received)
			break;
		    if (ret != 1 || strlen (tmp) > 8)
			printf ("Bad input - not accepted\n");
		    else {
			strcpy (Label, tmp);
			break;
		    }
		}
		break;

	    case 4:
		while (1) {
		    printf ("Enter a new directory (full path required): ");
		    ret = sscanf (PAI_gets (), "%s", tmp);
		    if (Signal_received)
			break;
		    if (ret != 1 || strlen (tmp) + 1 >= LOCAL_NAME_SIZE ||
				    			tmp[0] != '/')
			printf ("Bad input - not accepted\n");
		    else {
			strcpy (Vol_dir, tmp);
			if (Vol_dir[strlen (Vol_dir) - 1] != '/')
			    strcat (Vol_dir, "/");
			break;
		    }
		}
		break;

	    case 5:
		n_vol_files = PAF_search_volume_files (Vol_dir, &vol_files);
		PAW_write_cd (vol_files, n_vol_files);
		PAM_get_signal (&Signal_received);
		return (0);

	    case 6:
		if (Tape_fd >= 0)
		    close (Tape_fd);
		Tape_fd = -1;
		return (1);

	    case 7:
		return (0);

	    default:
		break;
	}
    }
}

/*******************************************************************

    Reads the tape and processes the data. Returns 0 on success or
    -1 on failure.

*******************************************************************/

static int Read_tape (int func, int n_skip_volumes, time_t st_time) {
    int vol_cnt;

    if (!Save_volume_mode && PAP_open_playback_lb () != 0)
	return (-1);

    PAP_session_start (1);
    vol_cnt = 0;
    while (1) {
	Ap_vol_file_t vf;
	NEXRAD_vol_title cr_title;
	time_t cr_vol_time;	/* current volume time */
	int cr_volume_number, searching;

	if (Signal_received)
	    break;

	if (Tape_type == TAPE_UNKNOWN &&
	    (Tape_type = Determine_tape_type ()) == TAPE_UNKNOWN)
	    break;

	if (Get_volume_data (GVD_READ_HD, (char *)&cr_title, 
					sizeof (NEXRAD_vol_title)) == 0)
	    break;
	else {			/* volume header read */

 	    cr_volume_number = atoi (cr_title.extension);
	    cr_vol_time = PAP_convert_time (cr_title.julian_date, 
				cr_title.millisecs_past_midnight);
	    vol_cnt++;
	    searching = 0;
	    if (func == SKIP_VOLUME) {
		if (vol_cnt <= n_skip_volumes)
		    searching = 1;
		else
		    break;
	    }
	    else if (func == SKIP_VOL_PLAY) {
		if (cr_volume_number < n_skip_volumes)
		    searching = 1;
	    }
	    else if (func == TIME_PLAY) {
		if (st_time <= 86400)		/* hour and minute only */
		    st_time = PAM_get_time_by_seconds_in_a_day 
						(cr_vol_time, st_time);
		if (cr_vol_time < st_time)
		    searching = 1;
	    }
	    if (!Quiet) {
		printf ("\r          Volume time: %s  scan #: %d",
			 PAI_ascii_time (cr_vol_time), cr_volume_number);
		if (searching)
		    printf ("   searching");
		else if (Save_volume_mode)
		    printf (" Generating vol files");
		else
		    printf (" playing back");
		fflush (stdout);
	    }
	}

	if (searching)
	    continue;

	if (Save_volume_mode) 
	    Create_volume_file (&cr_title, cr_vol_time);
	else {
	    vf.time = cr_vol_time;
	    vf.content_type = AP_TAPE_TYPE;
	    vf.session = -1;
	    PAP_playback_volume (&vf);
	}
    }
    PAP_session_start (0);
    return (0);
}

/*******************************************************************

    Returns the next part of "buf_size" bytes of the volume data in 
    "buffer" if "func" = GVD_READ_DATA. If "func" = GVD_READ_HD, it 
    searches the next volume header and returns the volume title in 
    "buffer". If "func" = GVD_INIT, it initializes the function. 
    Returns the number of bytes retured in "buffer" on success or 0 
    if no more data. This the non-compressed NCDC tape version.

*******************************************************************/

static int Get_volume_data (int func, char *buffer, int buf_size) {
    static int data_offset = 0;	/* offset of unused data in Buffer */
    static int data_size = 0;	/* total data in Buffer */

    if (Tape_type == TAPE_NCDC_Z)
	return (Get_volume_data_Z (func, buffer, buf_size));

    if (func == GVD_INIT) {
	data_offset = data_size = 0;
	return (0);
    }
    else if (func == GVD_READ_HD) {
	NEXRAD_vol_title *title;

	while (1) {
    
	    data_size = Read_bytes (RB_READ, Buffer, Buffer_size);
	    if (Signal_received || Tape_status == END_OF_TAPE)
		return (0);
	    if (Tape_status == END_OF_FILE || 
			    data_size != sizeof (NEXRAD_vol_title))
		continue;
	    data_offset = 0;
	    data_size = 0;

	    title = (NEXRAD_vol_title *)Buffer;
	    if (strncmp (title->filename, "ARCHIVE2", 8) == 0) {
	
		PAP_byte_swap_vol_title (title);
		memcpy (buffer, Buffer, sizeof (NEXRAD_vol_title));
		return (sizeof (NEXRAD_vol_title));
	    }
	    continue;
	}
    }
    else {				/* read data */
	int size;

	while (data_size - data_offset < buf_size) {	/* reads more data */
	    int nread;

/*
	    move remaining bytes to the beginning
	    if (data_offset > 0 && data_size > 0) {	
		memmove (Buffer, Buffer + data_offset, 
					data_size - data_offset);
		data_size = data_size - data_offset;
		data_offset = 0;
	    }
*/
	    data_offset = data_size = 0;	/* discard remaining bytes */

	    nread = Read_bytes (RB_READ, Buffer + data_size, 
					Buffer_size - data_size);
	    if (Tape_status == END_OF_TAPE || Tape_status == END_OF_FILE)
		break;
	    data_size += nread;
	}

	if (data_size - data_offset < buf_size)
	    size = data_size - data_offset;
	else
	    size = buf_size;
	memcpy (buffer, Buffer + data_offset, size);
	data_offset += size;
	return (size);
    }
}

/*******************************************************************

    Returns the next part of "buf_size" bytes of the volume data in 
    "buffer" if "func" = GVD_READ_DATA. If "func" = GVD_READ_HD, it 
    searches the next volume header and returns the volume title in 
    "buffer". If "func" = GVD_INIT, it initializes the function. 
    Returns the number of bytes retured in "buffer" on success or 0 
    if no more data. This the compressed NCDC tape version.

*******************************************************************/

static int Get_volume_data_Z (int func, char *buffer, int buf_size) {
    static int data_offset = 0;	/* offset of unused data in Buffer */
    static int data_size = 0;	/* total data in Buffer */

    if (func == GVD_INIT) {
	data_offset = data_size = 0;
	Process_Z_file (PZF_CLOSE, NULL, 0);
	return (0);
    }
    else if (func == GVD_READ_HD) {
	int hd_found;

	Process_Z_file (PZF_CLOSE, NULL, 0);
	hd_found = 0;
	while (1) {

	    if (data_size - data_offset < 512) {
		data_size = Read_bytes (RB_READ, Buffer, Buffer_size);
		if (Signal_received || Tape_status == END_OF_TAPE)
		    return (0);
		if (Tape_status == END_OF_FILE)
		    continue;
		data_offset = 0;
	    }

	    while (data_size - data_offset >= 512) {
		if (Is_NCDC_Z_header (Buffer + data_offset, 512)) {
		    hd_found++;
		    if (hd_found == 2)
			return (Process_Z_file (PZF_READ_TITLE, 
							buffer, buf_size));
		}
		else if (hd_found)
		    Process_Z_file (PZF_WRITE, Buffer + data_offset, 512);
		data_offset += 512;
	    }
	    if (data_size - data_offset != 0)
		printf ("Unexpected %d bytes left in the buffer", 
						data_size - data_offset);
	    data_size = data_offset = 0;
	}
    }
    else {				/* read data */
	return (Process_Z_file (PZF_READ, buffer, buf_size));
    }
}

/*******************************************************************

    Reads the first part of the tape data to determine the data type
    of the tape. Returns the tape type. The data is left in Buffer.

*******************************************************************/

static int Determine_tape_type () {
    int n_bytes;
    char label[9];

    /* read the first part of a tape file */
    while (1) {
	n_bytes = Read_bytes (RB_READ, Buffer, Buffer_size);
	if (Tape_status == END_OF_TAPE)
	    return (TAPE_UNKNOWN);
	if (Tape_status == END_OF_FILE)
	    continue;
	if (n_bytes == sizeof (NEXRAD_id_rec))
	    continue;
	break;
    }

    Read_bytes (RB_RETURN, Buffer, n_bytes);	/* return the data */
    if (n_bytes == 32768 && Is_NCDC_Z_header (Buffer, n_bytes)) {
	memcpy (label, Buffer, 4);
	label[4] = '\0';
	printf ("%s - compressed data\n", label);
	strcpy (Label, label);
	return (TAPE_NCDC_Z);
    }
    memcpy (label, Buffer + 8, 4);
    label[4] = '\0';
    if (strlen (label) == 4 && label[0] != '.') {
	printf ("%s - uncompressed data\n", label);
	strcpy (Label, label);
    }
    else
	printf ("Radar label not found - uncompressed data\n");
    return (TAPE_NCDC_U);
}

/*******************************************************************

    Returns 1 if the data is a NCDC Z volume header or 0 otherwise.

*******************************************************************/

static int Is_NCDC_Z_header (char *data, int n_bytes) {

    if (n_bytes >= 512 && 
		data[12] == '_' && data[19] == '.' && data[20] == 'Z') {
	int i;
	for (i = 0; i < 8; i++) {
	    if (data[21 + i] != '\0')
		break;
	}
	if (i < 8)
	    return (0);
	return (1);
    }
    return (0);
}

/*******************************************************************

    Reads tape and creates a volume file.

*******************************************************************/

static int Create_volume_file (NEXRAD_vol_title *vol_title, 
						time_t cr_vol_time) {

    Set_volume_file_name (cr_vol_time);
    Write_volume_file (NULL, 0);		/* start a new file */
    if (Write_volume_file ((char *)vol_title, sizeof (NEXRAD_vol_title)) < 0)
	return (-1);
    while (1) {
	int nread;
	char buffer[NEX_PACKET_SIZE];

	nread = Get_volume_data (GVD_READ_DATA, buffer, NEX_PACKET_SIZE);
	if (Signal_received) {
	    Write_volume_file (NULL, 0);
	    unlink (Vol_file);
	    return (-1);
	}
	if (Tape_status == END_OF_FILE || nread < NEX_PACKET_SIZE) {
	    Write_volume_file (NULL, 0);
	    return (0);
	}
	if (nread > 0 &&
	    Write_volume_file (buffer, nread) < 0) {
	    unlink (Vol_file);
	    return (-1);
	}
    }
}

/*******************************************************************

    Reads "buffer_size" bytes from tape buffer and puts them in
    "buffer". It returns the number of bytes read or -1 on error.
    If "vf" = NULL, it does nothing.

*******************************************************************/

int PAR_read_tape_data (Ap_vol_file_t *vf, char *buffer, int buf_size) {

    if (vf == NULL)
	return (0);

    return (Get_volume_data (GVD_READ_DATA, buffer, buf_size));
}

/*******************************************************************

    Checks and processes pause mode. This function is called every
    time before a radial data is sent to output LB. Return 0 for
    continuing playback or 1 for terminate playback.

*******************************************************************/

int PAR_pause (int radial_status, time_t data_time) {
    static int cnt = 0;

    cnt++;
    if ((cnt % 50) == 0) {
	printf ("\r%s", PAI_ascii_time (data_time) + 11);
	fflush (stdout);
    }
    if (Signal_received)
	return (1);
    return (0);
}

/*******************************************************************

    Reads "buf_size" bytes to "buffer" from Tape_fd. Returns the 
    number of bytes read. It sets Tape_status. If "func" = RB_RETURN,
    the data is returned to this function and the next call will 
    return the returned data if "buffer" is the same. If "func" = 
    RB_INIT, any returned data are discarded.

*******************************************************************/

static int Read_bytes (int func, char *buffer, int buf_size) {
    static int eof_flag = 0, returned_num = 0;
    static char *returned_pt;
    int nread;

    if (func == RB_INIT) {
	returned_num = 0;
	returned_pt = NULL;
	return (0);
    }
    else if (func == RB_RETURN) {
	returned_num = buf_size;
	returned_pt = buffer;
	return (buf_size);
    }
    if (returned_num > 0 &&
		buffer == returned_pt && buf_size >= returned_num) {
	nread = returned_num;
	returned_num = 0;
	return (nread);
    }
    returned_num = 0;

    Tape_status = DATA_VAILABLE;
    nread = read (Tape_fd, buffer, buf_size);
    if (nread <= 0) {
					/* no bytes returned from read */
	if (eof_flag >= 5) {		/* end of tape */
	    printf ("\r                       End of tape encountered \n");
	    fflush (stdout);
	    Tape_status = END_OF_TAPE;
	    return (0);
	}
	else {		/* end of file */
	    eof_flag++;
#ifdef SUNX86
	    Skip_files (Tape_fd, 1);
#endif
	    Tape_status = END_OF_FILE;
	    return (0);
	}
    }
    eof_flag = 0;
    return (nread);
}

/********************************************************************

    Rewinds tape "Tape_fd".

********************************************************************/

static int Rewind_tape () {

    struct mtop mt_command;

    mt_command.mt_op = MTREW;
    return ioctl (Tape_fd, MTIOCTOP, &mt_command);
}

/*******************************************************************

    Write to the current volume file "Vol_file" "n_bytes" data
    in "buffer". If "buffer" is NULL, the current file is closed.
    return 0 on success or -1 on failure.

*******************************************************************/

static int Write_volume_file (char *buffer, int n_bytes) {
    static FILE *fl = NULL;
    static BZFILE *bf;
    int berr;

    if (buffer == NULL) {
	if (fl != NULL) {
	    BZ2_bzWriteClose (&berr, bf, 0, NULL, NULL);
	    fclose (fl);
	    fl = NULL;
	}
	return (0);
    }

    if (fl == NULL) {		/* open the file */

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
    }
    if (n_bytes == 0)
	return (0);

    BZ2_bzWrite (&berr, bf, buffer, n_bytes);
    if (berr == BZ_IO_ERROR) {
	fprintf (stderr, "BZ2_bzWrite failed (bzerr %d)\n", berr);
	BZ2_bzWriteClose (&berr, bf, 0, NULL, NULL);
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
    strcpy (Vol_file, Vol_dir);
    sprintf (Vol_file + strlen (Vol_file), 
	"%s_%.4d:%.2d:%.2d:%.2d:%.2d:%.2d.bz2", Label, y, mon, d, h, m, s);
    return;
}

/*******************************************************************

    Processes the compressed volume data. "func" == PZF_WRITE: Writes
    "buf_size" bytes in "buffer" to the tmp file. 
    "func" == PZF_READ_TITLE: closes the file, uncompresses it and 
    opens the uncompressed file and returns the volume title.
    "func" == PZF_READ: Reads and returns the next "buf_size" bytes in 
    the uncomressed file. "func" == PZF_CLOSE: Closes and removes 
    existing tmp files. Return the number of bytes written/read
    on success or 0 on failure.

*******************************************************************/

static int Process_Z_file (int func, char *buffer, int buf_size) {
    static int fd = -1;

    switch (func) {
	char cmd[LOCAL_NAME_SIZE * 2 + 64];
	char work_file[LOCAL_NAME_SIZE * 2], *name;
	int ret;

	case PZF_WRITE: 
	    if (fd < 0) {
		name = PAF_get_full_path (PAP_get_work_dir (), 
							"play_a2.tmp.Z");
		fd = open (name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0) {
		    printf ("open %s failed\n", name);
		    exit (1);
		}
	    }
	    ret = write (fd, buffer, buf_size);
	    if (ret != buf_size) {
		printf ("write (%d bytes) failed (%d written)\n", 
						buf_size, ret);
		exit (1);
	    }
	    return (buf_size);

	case PZF_READ:
	    if (fd < 0)
		return (0);
	    ret = read (fd, buffer, buf_size);
	    if (ret < 0) {
		printf ("read failed\n");
		return (0);
	    }
	    if (ret < buf_size) {
		close (fd);
		fd = -1;
	    }
	    return (ret);

	case PZF_CLOSE:
	    if (fd >= 0)
		close (fd);
	    fd = -1;
	    return (0);

	case PZF_READ_TITLE:
	    if (fd >= 0)
		close (fd);
	    fd = -1;
	    strcpy (work_file, 
		PAF_get_full_path (PAP_get_work_dir (), "play_a2.tmp"));
	    unlink (work_file);
	    sprintf (cmd, "uncompress %s", 
				PAF_get_full_path (work_file, ".Z"));
	    if ((ret = MISC_system_to_buffer (cmd, NULL, 0, NULL)) != 0) {
		printf ("Failed in executing %s (%d)\n", cmd, ret);
		unlink (PAF_get_full_path (work_file, ".Z"));
		return (0);
	    }
	    fd = open (work_file, O_RDONLY);
	    unlink (work_file);
	    if (fd < 0) {
		printf ("open %s failed\n", work_file);
		exit (1);
	    }
	    ret = read (fd, buffer, sizeof (NEXRAD_vol_title));
	    if (ret != sizeof (NEXRAD_vol_title)) {
		printf ("read failed\n");
		close (fd);
		fd = -1;
		return (0);
	    }
	    return (ret);

	default:
	    break;
    }
    return (0);
}






