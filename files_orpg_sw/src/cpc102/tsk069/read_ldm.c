
/******************************************************************

    This is a tool that reads the latest LDM radar files in a 
    specified directory and puts it in an LB. Old files are deleted.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 15:09:38 $
 * $Id: read_ldm.c,v 1.20 2014/10/03 15:09:38 steves Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bzlib.h>
#include <netinet/in.h>
#include <inttypes.h>

#include <generic_basedata.h> 
#include <infr.h> 

#define LOCAL_NAME_SIZE 128		/* maximum name size */
#define MESSAGE_SIZE 2432		/* NCDC message size */
#define FILE_HEADER_SIZE 24		/* NCDC file header size */

static int Verbose;			/* verbose mode flag */
static char Dir_name[LOCAL_NAME_SIZE];	/* the directory to read from */
static char Lb_name[LOCAL_NAME_SIZE];	/* output LB name */
static int Decompress = 1;		/* need decompress */
static int Need_convert_to_31;		/* converts message 1 to message 31 */

static DIR *Dir = NULL;			/* directory struct */
static int Lb_fd;			/* LB descriptor */

static int Next_file_time = 0;		/* time of the next file to process */
static int Cr_file_time = 0;		/* time of the current file */
static int Cr_file_done = 0;		/* the current file is completed */
static int File_fd = -1;		/* current file descriptor */

static char Next_file_name[LOCAL_NAME_SIZE];
					/* name of the next file to process */
static char Cr_file_name[LOCAL_NAME_SIZE];
					/* name of the current file */

static int Bytes_read = 0;		/* number of bytes read from file */
static char *Buffer = NULL;		/* buffer for message segments */
static int Buf_size = 500000;		/* size of Buffer */

static int Wait_ms = 20;	/* wait time (ms) after each message output */

static int Discard_data_already_in_file = 1;
					/* data in the file is discarded when
					   the file is open - We only output
					   new data */
static int Delete_data;

static int Seg_size = 0;		/* size of the current data segmant */
static int Pass_all_messages = 1;	/* all ICD messages are passed  Note:  
                                           starting with RPG Build 14, VCP messages
                                           need to be passed.  This option will 
                                           default to all messages. */
static int Playback = 0;		/* read existing files and keep them */
static char Radar_id[8] = "";		/* 4-letter radar ID read from LDM */

static char *(*Convert_to_31) (RDA_RPG_message_header_t *, char *);

/* local functions */
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static char *Get_full_path (char *name);
static void Check_new_files ();
static time_t Get_file_time (char *name);
static void Process_current_file ();
static void Output_data (int size, char *buffer);
static int Process_data (char *buffer, int n_bytes);
static void Decompress_and_process_file ();
static int Is_empty_message (int n_segs, int seg_num);
static void Print_fields (int type, int size, int n_segs, int seg_num);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    char buf[128];

    if (Read_options (argc, argv) != 0)
	exit (1);

    sprintf (buf, "%d", getpid ());
    LE_set_option ("label", buf);
    LE_set_option ("LB size", 5000);
    LE_init (argc, argv);
    if (Verbose)
	LE_send_msg (0, "Read from directory %s\n", Dir_name);

    if (Need_convert_to_31) {	/* load function UMC_convert_to_31 */
	Convert_to_31 = (char *(*) (RDA_RPG_message_header_t *, char *))
		MISC_get_func ("liborpg.so", "UMC_convert_to_31", 0);
	if (Convert_to_31 == NULL) {
	    LE_send_msg (0, "MISC_get_func failed\n");
	    fprintf (stderr, "MISC_get_func failed\n");
	    exit (1);
	}
    }

    Dir = opendir (Dir_name);
    if (Dir == NULL) {
	LE_send_msg (0, "opendir (%s) failed, errno %d\n", Dir_name, errno);
	exit (1);
    }

    Lb_fd = LB_open (Lb_name, LB_WRITE, NULL);
    if (Lb_fd < 0) {
	LE_send_msg (0, "LB_open (%s) failed, returned %d\n", Lb_name, Lb_fd);
	exit (1);
    }

    if (Playback)
	Cr_file_time = 1;	/* started with the oldest file */

    if (Buffer == NULL)
	Buffer = MISC_malloc (Buf_size);

    while (1) {

	if (Cr_file_time > 1 && !Cr_file_done)
	    Process_current_file ();
	if (Next_file_time > Cr_file_time) {	/* start a new file */
	    if (Cr_file_time > 1 && !Cr_file_done)
		Process_current_file ();	/* process last part */
	    if (File_fd >= 0) {
		close (File_fd);
		File_fd = -1;
	    }
	    Cr_file_time = Next_file_time;
	    strcpy (Cr_file_name, Next_file_name);
	    Discard_data_already_in_file = 0;
	    Cr_file_done = 0;
	    continue;
	}
	sleep (2);
	Check_new_files ();
	if (Playback && Next_file_time == 0)	/* no more file */
	    exit (0);
    }

    exit (0);
}

/**************************************************************************

    Opens and reads current file, checks file header and then reads each
    NCDC message and passes it to Process_data.

**************************************************************************/

static void Process_current_file () {
    static int hd_processed = 0;
    int ret;

    if (File_fd < 0) { 			/* open the file */
	if (Verbose)
	    LE_send_msg (0, "process file %s...\n", 
					Get_full_path (Cr_file_name));
	File_fd = open (Get_full_path (Cr_file_name), O_RDONLY);
	if (File_fd < 0) {
	    LE_send_msg (0, "open %s failed (errno %d)\n", 
				Get_full_path (Cr_file_name), errno);
	    Cr_file_done = 1;
	    return;
	}
	Bytes_read = hd_processed = 0;
    }

    if (!hd_processed) {

	if (Bytes_read < FILE_HEADER_SIZE) {	/* read the file header */
	    ret = MISC_read (File_fd, 
		  (char *)Buffer + Bytes_read, FILE_HEADER_SIZE - Bytes_read);
	    if (ret < 0) {
		LE_send_msg (0, 
			"read file header failed (file %s, errno %d)\n", 
				    Get_full_path (Cr_file_name), errno);
		Cr_file_done = 1;
		return;
	    }
	    Bytes_read += ret;
	}
    
	if (Bytes_read >= FILE_HEADER_SIZE) {	/* check file header */
	    if (strncmp ((char *)Buffer, "ARCHIVE2.", 9) != 0 &&
		strncmp ((char *)Buffer, "AR2", 3) != 0) {
		LE_send_msg (0, "file header check failed (file %s)\n", 
					    Get_full_path (Cr_file_name));
		Cr_file_done = 1;
		return;
	    }
	    if (Verbose) {
		char label[32];
		int date, time;
		date = ntohl (*((int *)((char *)Buffer + 12)));
		time = ntohl (*((int *)((char *)Buffer + 16)));
		strncpy (label, (char *)Buffer, 12);
		label[12] = '\0';
		strncpy (Radar_id, (char *)Buffer + 20, 4);
		Radar_id[4] = '\0';
		LE_send_msg (0, 
			"File header: %s, date %d, time %d, radar ID %s\n", 
			label, date, time, Radar_id);
	    }
	    Bytes_read = 0;
	    hd_processed = 1;
	    Seg_size = 0;
	}
	else
	    return;
    }

    if (Decompress) {
	Decompress_and_process_file ();
	return;
    }

    /* The following section is from Eddie for supporting uncompr Message 31 */
    while (1) {
	short size, type, *spt;
	unsigned char *cpt;
	int bytes_processed;
	int read_size = 16;  /* Read first 16 bytes of the header */
	type = 1;
	while (Bytes_read < read_size) {
	    if (read_size + 1024 > Buf_size) {
		if (Buffer != NULL)
		    free (Buffer);
		Buf_size = read_size + 1024;
		Buffer = MISC_malloc (Buf_size);
	    }
	    ret = MISC_read (File_fd, (char *)Buffer + Bytes_read,
			         		read_size - Bytes_read);
	    if (ret < 0) {
		LE_send_msg (0, "read data failed (file %s, errno %d)\n",
                           Get_full_path (Cr_file_name), errno);
		Cr_file_done = 1;
		return;
	    }
	    if (ret < read_size - Bytes_read) {
		Discard_data_already_in_file = 0;
		return;
	    }
	    spt = (short *)Buffer;
	    cpt = (unsigned char *)Buffer;
	    size = spt[6];
#ifdef LITTLE_ENDIAN_MACHINE
	    size = SHORT_BSWAP (size);
#endif
	    type = cpt[15];
	    size *= 2;
	    if (type == 31)
		read_size = size + 12;
	    else
		read_size = MESSAGE_SIZE;
	    Bytes_read += ret;
	}
	if ((type != 31) && (Bytes_read < MESSAGE_SIZE)) {
						/* patial message read */
	    Discard_data_already_in_file = 0;
	    return;
	}
   	bytes_processed = Process_data ((char *)Buffer, Bytes_read);
	if (bytes_processed < Bytes_read) {
	    off_t off;
	    off = lseek (File_fd, bytes_processed - Bytes_read, SEEK_CUR);
	    if (off == ((off_t)-1))
		fprintf (stderr, 
			"Error %d seeking %d bytes from current position\n",
			errno, bytes_processed - Bytes_read);
        }
      	Bytes_read = 0;
    }
}

/**************************************************************************

    Reads file, decompress the data and processes a complete file.

**************************************************************************/

static void Decompress_and_process_file () {
    int ret;

    while (1) {

	if (Seg_size <= 0) { 			/* read the segment size */
	    if (Bytes_read < 4) {
		ret = MISC_read (File_fd, 
				(char *)Buffer + Bytes_read, 4 - Bytes_read);
		if (ret < 0) {
		    LE_send_msg (0, 
			    "read segment size failed (file %s, errno %d)\n", 
			    Get_full_path (Cr_file_name), errno);
		    Cr_file_done = 1;
		    return;
		}
		Bytes_read += ret;
	    }
	    if (Bytes_read >= 4) {
		Seg_size = *((int *)((char *)Buffer));
#ifdef LITTLE_ENDIAN_MACHINE
		Seg_size = INT_BSWAP (Seg_size);
#endif
		Bytes_read = 0;
		if (Seg_size < 0)
		    Seg_size = -Seg_size;
		if (Seg_size <= 0 || Seg_size >= 10000000) {
		    LE_send_msg (0, "Bad segment size (file %s, size %d)\n",
				Get_full_path (Cr_file_name), Seg_size);
		    Cr_file_done = 1;
		    return;
		}
	    }
	    else {
		Discard_data_already_in_file = 0;
		return;
	    }
	}

	if (Seg_size > 0) {
	    static char *dest = NULL;
	    static int dest_bsize = 500000;
	    int dest_len, bytes_processed;

	    if (Bytes_read < Seg_size) {
		if (Seg_size + 10000 > Buf_size) {
		    if (Buffer != NULL)
			free (Buffer);
		    Buf_size = Seg_size + 10000;
		    Buffer = MISC_malloc (Buf_size);
		}
		ret = MISC_read (File_fd, (char *)Buffer + Bytes_read, 
						Seg_size - Bytes_read);
		if (ret < 0) {
		    LE_send_msg (0, 
			    "read data failed (file %s, errno %d)\n", 
			    Get_full_path (Cr_file_name), errno);
		    Cr_file_done = 1;
		    return;
		}
		Bytes_read += ret;
	    }
	    if (Bytes_read < Seg_size) {
		Discard_data_already_in_file = 0;
		return;
	    }

	    while (1) {
		if (dest == NULL)
		    dest = MISC_malloc (dest_bsize);
		dest_len = dest_bsize;
		ret = BZ2_bzBuffToBuffDecompress (dest, 
		    (unsigned int *)&dest_len, (char *)Buffer, Seg_size, 0, 0);
		if (ret == BZ_OUTBUFF_FULL) {
		    if (dest != NULL)
			free (dest);
		    dest_bsize *= 2;
		    dest = MISC_malloc (dest_bsize);
		    continue;
		}
		if (ret < 0) {
		    LE_send_msg (0, 
				"BZ2 decompress failed (file %s), returns %d\n", 
				Get_full_path (Cr_file_name), ret);
		    Cr_file_done = 1;
		    return;
		}
		break;
	    }

	    Bytes_read = dest_len;
	    bytes_processed = 0;
	    while (Bytes_read > bytes_processed) {
		bytes_processed += 
			Process_data ((char *)dest + bytes_processed, 
					Bytes_read - bytes_processed);
	    }

	    Bytes_read = 0;
	    Seg_size = 0;
	}
    }
}

/**************************************************************************

    Processes a complete NCDC message in "buffer".

**************************************************************************/

static int Process_data (char *buffer, int n_bytes) {
    short size, type, *spt, n_segs, seg_num;
    unsigned char *cpt;

    spt = (short *)buffer;
    cpt = (unsigned char *)buffer;
    size = spt[6];
    n_segs = spt[12];
    seg_num = spt[13];
#ifdef LITTLE_ENDIAN_MACHINE
    size = SHORT_BSWAP (size);
    n_segs = SHORT_BSWAP (n_segs);
    seg_num = SHORT_BSWAP (seg_num);
#endif
    type = cpt[15];
    size *= 2;

    if (type != 31) {
	if (size > MESSAGE_SIZE) {
	    LE_send_msg (0,
		"Message size larger than expected (%d)\n", MESSAGE_SIZE);
	    Print_fields (type, size, n_segs, seg_num);
	    return (MESSAGE_SIZE);
	}
	if (n_bytes < MESSAGE_SIZE) {
	    LE_send_msg (0, "Data bytes (%d) less than expected (%d)\n", 
						n_bytes, MESSAGE_SIZE);
	    Print_fields (type, size, n_segs, seg_num);
	    return (n_bytes);
	}
    }
    else {
	if (n_bytes < size) {
	    LE_send_msg (0, "Data bytes (%d) less than expected (%d)\n", 
						n_bytes, size);
	    Print_fields (type, size, n_segs, seg_num);
	    return (n_bytes);
	}
    }

    if (type == 0)			/* unused message - discarded */
	return (MESSAGE_SIZE);

    if (type <= 0 || type > 50)	{	/* not a NCDC message */
	LE_send_msg (0, "Unexpected data type (%d)\n", type);
	Print_fields (type, size, n_segs, seg_num);
	return (MESSAGE_SIZE);
    }

    if (type == 1) {	
	if (!Need_convert_to_31) {
	    size = 2416;	/* msg size may be less than 1208 from ORDA, 
				   but LDM data size for message 1 is 1208 */
	    Output_data (size, buffer);
	}
	else {
	    char *m31 = Convert_to_31 
			((RDA_RPG_message_header_t *)(spt + 6), Radar_id);
	    Output_data ((*((unsigned short *)m31)) * 2, m31 - 12);
	}
	return (MESSAGE_SIZE);
    }

    if (type == 2) {			/* RDA status data */
	if (size != 96) {
	    LE_send_msg (0, "unexpected RDA status size (%d)\n", size);
	    return (MESSAGE_SIZE);
	}
	Output_data (size, buffer);
	return (MESSAGE_SIZE);
    }

    if (type == 31) {
	Output_data (size, buffer);
	return (size + 12);
    }

    if (Pass_all_messages) {		/* for types 2 - 50 */
	if (!Is_empty_message (n_segs, seg_num))
	    Output_data (MESSAGE_SIZE, buffer);
    }
    else if (Verbose)
	LE_send_msg (0, "data type (%d) discarded\n", type);
    return (MESSAGE_SIZE);
}

static void Print_fields (int type, int size, int n_segs, int seg_num) {
    LE_send_msg (0, "    type = %d  size = %d  n_segs = %d  seg_num = %d\n",
					type, size, n_segs, seg_num);
}

/******************************************************************

    Checks if message of "seg_num" is an empty message. The LDM ICD assumes
    maximum sizes for certain messages. It a message is smaller than
    its maximum size. 

******************************************************************/

static int Is_empty_message (int n_segs, int seg_num) {
    static int cr_num_segs = 0;

    if (seg_num == 1)
	cr_num_segs = n_segs;

    if (cr_num_segs > 0 && seg_num > cr_num_segs)
	return (1);
    return (0);
}

/**************************************************************************

    Writes data of size "size" to the output LB. The 12 byte CTM header
    is removed.

**************************************************************************/

#include <basedata.h>

static void Output_data (int size, char *buffer) {
    static int cnt = 0;
    int ret;

    if (Discard_data_already_in_file)
	return;

    ret = LB_write (Lb_fd, (char *)buffer + 12, size, LB_NEXT);
    if (ret < 0) {
	LE_send_msg (0, "LB_write data failed (%d)\n", ret);
	return;
    }
    cnt++;
    if (Verbose && (cnt % 300) == 1)
	LE_send_msg (0, "%d messages written to LB\n", cnt);
    if (Wait_ms > 0)
	msleep (Wait_ms);
    return;
}

/**************************************************************************

    Goes through the directory and finds any new file. Any file that is
    older than the current file is removed.

**************************************************************************/

static void Check_new_files () {
    struct dirent *dp;

    rewinddir (Dir);	/* rewind and refresh */
    Next_file_time = 0;
    while ((dp = readdir (Dir)) != NULL) {
	time_t ft;

	ft = Get_file_time (dp->d_name);
	if (ft == 0)		/* not a data file */
	    continue;
	if (Cr_file_time > 0 && ft < Cr_file_time) {
	    if (Delete_data) {
		unlink (Get_full_path (dp->d_name));
		if (Verbose)
		    LE_send_msg (0, "File %s removed\n", 
					Get_full_path (dp->d_name));
	    }
	    continue;
	}
	if (ft > Cr_file_time) {
	    if ((Cr_file_time == 0 && ft > Next_file_time) ||
		(Cr_file_time > 0 && 
			(Next_file_time == 0 || ft < Next_file_time))) {
		Next_file_time = ft;
		strcpy (Next_file_name, dp->d_name);
	    }
	}
    }
}

/************************************************************************

    Returns the full path of file name "name".

************************************************************************/

static char *Get_full_path (char *name) {
    static char fname[LOCAL_NAME_SIZE * 2 + 4];
    sprintf (fname, "%s/%s", Dir_name, name);
    return (fname);
}

/************************************************************************

    Extracts and returns the time part of file name "name". It returns 
    0 if the file name is not in the format of "yyyymmddhhmmss.raw".

************************************************************************/

static time_t Get_file_time (char *name) {
    struct stat st;
    int ret;
    time_t t;
    int y, mon, d, h, m, s;

    ret = stat (Get_full_path (name), &st);
    if (ret < 0) {
	LE_send_msg (0, "stat (%s) failed, errno %d\n", name, errno);
	return (0);
    }
    if (!(st.st_mode & S_IFREG))	/* not a regular file */
	return (0);

    if (strlen (name) == 25 && strcmp (name + 21, ".raw") == 0) {
	if (sscanf (name + 4, "%4d%2d%2d%2d%2d%2d", 
					&y, &mon, &d, &h, &m, &s) != 6)
	    return (0);
    }
    else if (strlen (name) == 18 && strcmp (name + 14, ".raw") == 0) {
	if (sscanf (name, "%4d%2d%2d%2d%2d%2d", &y, &mon, &d, &h, &m, &s) != 6)
	    return (0);
    }
    else 
	return (0);

    t = 0;
    unix_time (&t, &y, &mon, &d, &h, &m, &s);
    return (t);
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
    sprintf (Dir_name, ".");
    Lb_name[0] = '\0';
    Verbose = 1;
    Pass_all_messages = 1;
    Playback = 0;
    Need_convert_to_31 = 0;
    Delete_data = 1;
    while ((c = getopt (argc, argv, "d:w:rqusaph?")) != EOF) {
	switch (c) {

            case 'd':
		strncpy (Dir_name, optarg, LOCAL_NAME_SIZE);
		Dir_name[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 'w':
		if (sscanf (optarg, "%d", &Wait_ms) != 1) {
		    LE_send_msg (0, "unexpected -w specification\n");
		    err = -1;
		}
                break;

	    case 'u':
		Decompress = 0;
		break;

	    case 's':
		Discard_data_already_in_file = 0;
		break;

	    case 'r':
		Discard_data_already_in_file = 0;
		Pass_all_messages = 1;
		Delete_data = 0;
		break;

	    case 'a':
		Pass_all_messages = 1;
		break;

	    case 'p':
		Playback = 1;
		Discard_data_already_in_file = 0;
		Delete_data = 0;
		break;

	    case 'C':
		Need_convert_to_31 = 1;
		break;

	    case 'q':
		Verbose = 0;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1) {      /* get the LB name  */
	strncpy (Lb_name, argv[optind], LOCAL_NAME_SIZE);
	Lb_name[LOCAL_NAME_SIZE - 1] = '\0';
    }

    if (err == 0 && strlen (Lb_name) == 0) {
	LE_send_msg (0, "LB name not specified\n");
	err = -1;
    }

    return (err);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    printf ("Usage: %s (options) lb_name\n", argv[0]);
    printf ("       Dynamically reads new LDM radar files and puts WSR88D\n");
    printf ("       messages in LB \"lb_name\".\n");
    printf ("       The LDM radar files must have name of \"yyyymmddhhmmss.raw\"\n");
    printf ("       Options:\n");
    printf ("       -d dir (Specifies an alternative directory to read from\n");
    printf ("               instead of the current directory)\n");
    printf ("       -w wait_ms (Specifies wait time (in ms) after each message output.\n");
    printf ("               The default is 20 ms.)\n");
    printf ("       -C (Convert message 1 to message 31)\n");
    printf ("       -u (Process uncompressed data file)\n");
    printf ("       -s (Process data already in the latest files when started)\n");
    printf ("       -r (Process realtime data starting with the existing latest file. All\n");
    printf ("           messages are passed and no data file is deleted.)\n");
    printf ("       -a (Pass all messages (other than radial and status only))\n");
    printf ("       -p (Playback mode - read existing files and keep them)\n");
    printf ("       -q (Turns off the verbose mode - no activity report.)\n");
    printf ("       Examples:\n");
    printf ("       read_ldm -d /export/home/ldm/data/KTLX /export/home/ldm/radars/KTLX.lb\n");
    printf ("       Reads .raw data in /export/home/ldm/data/KTLX and puts in\n");
    printf ("       LB /export/home/ldm/radars/KTLX.lb\n");
    exit (0);
}
