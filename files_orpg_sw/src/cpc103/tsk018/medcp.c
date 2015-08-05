
/******************************************************************

    This is a tool that copies files to/from a CD or diskette.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/27 13:56:44 $
 * $Id: medcp.c,v 1.37 2012/08/27 13:56:44 jing Exp $
 * $Revision: 1.37 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <infr.h> 
#include <medcp.h> 

#define STR_SIZE 256
#define TRUNC_STR_SIZE 128
#define BUF_SIZE 2048
#define CMD_SIZE 512
#define LSHAL_BUF_SIZE 20000

enum {MCP_UNDEFINED, MCP_CD, MCP_FLOPPY}; /* Media_type */

typedef struct {
    char *device;		/* e.g. /dev/hda */
    char *name;			/* e.g. storage_model__NEC_DVD_/_RW_ND_3530A */
    double capacity;		/* in bytes */
    int available;		/* the media is available in the drive */
    int is_blank;
    int is_mounted;
    char *dir;			/* mount point */
    char *type;			/* e.g. cd_r */
} Media_info_t;

static char *Src = NULL;
static char *Dest = NULL;
static char *Cp_options = NULL;
static int Media_type;
static int Cp_from = -1;
static char *Log_name = NULL;
static int Verbose = 0;
static int Blanking = 0;
static int Verification;
static int Eject = 0;;
static int Print_mount_dir = 0;
static int Wait_time = 20;
static char *Tmp_mount_dir = "";
static int Cd_image_fd = -1;
static char *Device_name = NULL;
static char *Hal_root = "/org/freedesktop/Hal/devices/";
static int Mount = -1;

static Media_info_t Med_info;

static void Print_usage (char **argv);
static int Read_options (int argc, char **argv);
static int Execute_command (char *cmd, char *buf, int b_size, int log);
static int Search_device ();
static void Write_cd ();
static int Create_cd_image (char **fname);
static char *Truncated_text (char *cmd, int size);
static void Write_floppy ();
static void Read_media ();
static int Verify_write ();
static int Diff_files (char *src, char *dest_dir, int to_media);
static int Run_command (char *cmd, char *strs, int cnt);
static int Check_mount (int timeout);
static int Look_for_CD (char *dname);
static int Mount_floppy ();
static void Exit_with_eject (int code);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Log_name != NULL)
	LE_set_option ("LE name", Log_name);
    LE_init (argc, argv);
    if (Verbose)
	LE_set_option ("verbose level", 1);
    LE_send_msg (0, "medcp starts");
    LE_set_option ("also stderr", 1);

    Med_info.device = STR_copy (NULL, "");
    Med_info.name = STR_copy (NULL, "");
    Med_info.dir = STR_copy (NULL, "");
    Med_info.type = STR_copy (NULL, "");
    Med_info.capacity = 0.;
    Med_info.available = 0;
    Med_info.is_blank = 0;
    Med_info.is_mounted = 0;

    setuid (getuid ());		/* This is added on 7/16/09. Without this, 
	    executing "lshal -t" later will fail - seem to be an OS error */
    if (Search_device () < 0)
	Exit_with_eject (MEDCP_DEVICE_NOT_FOUND);

    if (Media_type == MCP_FLOPPY)
	Mount_floppy ();
    else {
	if (Mount == 1)
	    Print_mount_dir = 1;
    }

    if (Media_type == MCP_CD && !Check_mount (Wait_time)) {
	LE_send_msg (0, "media is not detected");
	Exit_with_eject (MEDCP_MEDIA_NOT_DETECTED);
    }

    if (Print_mount_dir) {
	if (Med_info.is_mounted) {
	    printf ("%s\n", Med_info.dir);
	    Exit_with_eject (MEDCP_SUCCESS);
	}
	else
	    Exit_with_eject (MEDCP_MEDIA_NOT_MOUNTED);
    }

    if (Cp_from == 0 || Blanking) {
	if (Media_type == MCP_CD)
	    Write_cd ();
	else if (Media_type == MCP_FLOPPY)
	    Write_floppy ();
    }
    else if (Cp_from == 1)
	Read_media ();

    Exit_with_eject (MEDCP_SUCCESS);
    exit (0);
}

/******************************************************************

    Ejects the media if Eject is true and exits with "code".
	
******************************************************************/

static void Exit_with_eject (int code) {

    if (Eject) {
	char buf[STR_SIZE], cmd[CMD_SIZE];
	sprintf (cmd, "eject %s", Med_info.device);
	if( Execute_command (cmd, buf, STR_SIZE, 1) < 0 &&
	    getenv ("ORPG_NONOPERATIONAL") == NULL) {
	    sprintf (cmd, "sudo eject %s", Med_info.device);
	    Execute_command (cmd, buf, STR_SIZE, 1);
	}
    }
    exit (code);
}

/******************************************************************

    Writes the source files (Src) to diekette. 
	
******************************************************************/

static void Write_floppy () {
    char tk[STR_SIZE], buf[STR_SIZE], cmd[CMD_SIZE];
    int cnt, err;

    if (Blanking) {
	LE_send_msg (LE_VL1, "** Blanking the floppy\n");
	sprintf (cmd, "sh -c \"rm %s/*\"", Med_info.dir);
	if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0 &&
	    strstr (buf, "No such file") == NULL) {
	    LE_send_msg (0, "Blanking floppy failed\n");
	    Exit_with_eject (MEDCP_BLANKING_MEDIA_FAILED);
	}
	LE_send_msg (LE_VL1, "** Blanking the floppy done\n");
	return;
    }

    err = 0;
    cnt = 0;
    while (1) {
	if (MISC_get_token (Src, "", cnt, tk, STR_SIZE) <= 0)
	    break;
	cnt++;
	sprintf (cmd, "sh -c \"test -f %s\"", tk);
	if (Execute_command (cmd, buf, STR_SIZE, 1) == 0) { /* a file */
	    sprintf (cmd, "sh -c \"cp %s %s %s\"", 
					Cp_options, tk, Med_info.dir);
	    LE_send_msg (LE_VL1, "** cp %s %s", tk, Med_info.dir);
	}
	else {
	    sprintf (cmd, "sh -c \"cp %s %s/* %s\"", 
					Cp_options, tk, Med_info.dir);
	    LE_send_msg (LE_VL1, "** cp %s/* %s", tk, Med_info.dir);
	}
	buf[0] = '\0';
	if (Execute_command (cmd, buf, STR_SIZE, 1) != 0) {
	    LE_send_msg (0, "cp %s failed", tk);
	    if (strstr (buf, "No such file") != NULL)
		err = 2;
	    else {
		err = 1;
		break;
	    }
	    continue;
	}
	if (Verification && Diff_files (tk, Med_info.dir, 1) != 0) {
	    err = 1;
	    break;
	}
    }
    if (err == 0) {
	if (Verification)
	    LE_send_msg (LE_VL1, "Files copied to %s and verified\n", 
						Med_info.dir);
	else
	    LE_send_msg (LE_VL1, "Files copied to %s\n", Med_info.dir);
    }
    else if (err == 1) {
	LE_send_msg (0, "Failed in copying files to floppy");
	Exit_with_eject (MEDCP_MEDIA_WRITE_FAILED);
    }
    return;
}

/******************************************************************

    If "src" is a file, it is differed with the file of the same name in
    dir "dest_dir". If "src" is a dir and to_media is 1, each of the
    files in src is deffered with the file of the same name in dest_dir.
    If "src" is a dir and to_media is 0, every file in src is differed
    with the file of the same name in dest_dir/dir.
	
******************************************************************/

static int Diff_files (char *src, char *dest_dir, int to_media) {
    char buf[BUF_SIZE], cmd[CMD_SIZE], sub_string[STR_SIZE];

    LE_set_option ("also stderr", 0);
    LE_send_msg (LE_VL1, "** diff each file in %s with that in %s", 
						src, dest_dir);
    LE_set_option ("also stderr", 1);
    if (to_media)
	sprintf (sub_string, "sub=s,$s,%s,", dest_dir);
    else
	sprintf (sub_string, "sub=s,`dirname $s`,%s,", dest_dir);
    sprintf (cmd, 
	"sh -c \"s=%s ; if [ -z `expr $s : '\\([.|/]*\\)'` ] ; then s=./$s ; fi ; %s ; set -e ; for file in `find $s -type f` ; do des=`echo $file | sed $sub` ; echo diff $file $des; diff $file $des > /dev/null ; done\"", src, sub_string);
    if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
	LE_send_msg (0, "diff %s %s failed", src, dest_dir);
	return (-1);
    }
    return (0);
}


/******************************************************************

    Reads the files (Src) on the media. 
	
******************************************************************/

static void Read_media () {
    int cnt, err;

    if (Src == NULL || Dest == NULL)
	return;

    if (!Med_info.is_mounted) {
	LE_send_msg (0, "The media is not mounted");
	Exit_with_eject (MEDCP_MEDIA_NOT_MOUNTED);
    }

    err = 0;
    cnt = 0;
    while (1) {
	static char *path = NULL;
	char tk[STR_SIZE], buf[BUF_SIZE], cmd[CMD_SIZE];

	if (MISC_get_token (Src, "", cnt, tk, STR_SIZE) <= 0)
	    break;
	cnt++;
	path = STR_copy (path, Med_info.dir);
	path = STR_cat (path, "/");
	path = STR_cat (path, tk);
	sprintf (cmd, "sh -c \"cp %s %s %s\"", Cp_options, path, Dest);
	LE_send_msg (LE_VL1, "** cp %s %s", path, Dest);
	buf[0] = '\0';
	if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
	    LE_send_msg (0, "cp %s failed", path);
	    if (strstr (buf, "No such file") != NULL)
		err = 2;
	    else if (err == 0)
		err = 1;
	    continue;
	}
	if (Verification && Diff_files (path, Dest, 0) != 0) {
	    err = 1;
	    break;
	}
    }
    if (err == 0) {
	if (Verification)
	    LE_send_msg (LE_VL1, "Files copied from %s and verified\n", 
						Med_info.dir);
	else
	    LE_send_msg (LE_VL1, "Files copied from %s\n", Med_info.dir);
    }
    return;
}

/******************************************************************

    Searches for media and gets the media info using "lshal". Returns
    0 if the media is not detected within "timeout", 1 if detected.
	
******************************************************************/

static int Check_mount (int timeout) {
    char buf[LSHAL_BUF_SIZE], *p, line[STR_SIZE], *dname, *cmd;
    time_t st;
    int off;

    LE_send_msg (LE_VL1, "** Checking media\n");
    st = 0;
    if (timeout > 0)
	st = MISC_systime (NULL);
    while (1) {
	int lead_space;

	if (Execute_command ("lshal -t", buf, LSHAL_BUF_SIZE, 0) != 0) {
	    LE_send_msg (0, "lshal -t (Check_mount) failed");
	    Exit_with_eject (MEDCP_CHECK_MOUNT_FAILED);
	}
	dname = NULL;
	p = buf;
	lead_space = -1;
	while ((off = MISC_get_token (p, "S\n", 0, line, STR_SIZE)) > 0) {
	    int l_space = MISC_char_cnt (p, " \t");
	    p += off;
	    if (strcmp (line + MISC_char_cnt (line, " \t"), 
						Med_info.name) == 0) {
		lead_space = l_space;
		continue;
	    }
	    if (lead_space < 0)
		continue;

	    if (l_space <= lead_space)
		break;

	    if (strncmp (line + MISC_char_cnt (line, " \t"), 
							"volume_", 7) == 0) {
		dname = STR_gen (dname, Hal_root, 
				line + MISC_char_cnt (line, " \t"), NULL);
		break;
	    }
	}
	if (lead_space < 0) {
	    LE_send_msg (0, "Device %s not found (Check_mount)", 
							Med_info.name);
	    Exit_with_eject (MEDCP_DEVICE_NOT_FOUND);
	}
	if (dname != NULL)
	    break;
	if (timeout <= 0 || MISC_systime (NULL) >= st + timeout) {
	    return (0);
	}
	sleep (2);
    }

    cmd = STR_gen (NULL, "lshal -l -u ", dname, NULL);
    STR_free (dname);

    st = MISC_systime (NULL);
    while (1) {
	char label[STR_SIZE];
	if (Execute_command (cmd, buf, LSHAL_BUF_SIZE, 0) != 0) {
	    LE_send_msg (0, "Command %s failed (Check_mount)", cmd);
	    Exit_with_eject (MEDCP_CHECK_MOUNT_FAILED);
	}
	p = buf;
	while ((off = MISC_get_token (p, "S\n", 0, line, STR_SIZE)) > 0) {
	    char tok[STR_SIZE], v[STR_SIZE];
	    double s;
    
	    if (MISC_get_token (line, "", 1, tok, STR_SIZE) > 0 &&
		strcmp (tok, "=") == 0) {
		MISC_get_token (line, "S=", 0, tok, STR_SIZE);
		if (strcmp (tok, "volume.disc.capacity") == 0 &&
		    MISC_get_token (line, "Cd", 2, &s, 0) > 0) {
		    Med_info.capacity = s;
		}
		if (strcmp (tok, "volume.disc.is_blank") == 0 &&
		    MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		    strcmp (v, "true") == 0) {
		    Med_info.is_blank = 1;
		}
		if (strcmp (tok, "volume.is_mounted") == 0 &&
		    MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		    strcmp (v, "true") == 0) {
		    Med_info.is_mounted = 1;
		}
		if (strcmp (tok, "volume.mount_point") == 0 &&
		    MISC_get_token (line, "Q'", 2, v, STR_SIZE) > 0) {
		    Med_info.dir = STR_copy (Med_info.dir, v);
		}
		if (strcmp (tok, "volume.disc.type") == 0 &&
		    MISC_get_token (line, "Q'", 2, v, STR_SIZE) > 0) {
		    Med_info.type = STR_copy (Med_info.type, v);
		}
		if (strcmp (tok, "volume.label") == 0 &&
		    MISC_get_token (line, "Q'", 2, v, STR_SIZE) > 0) {
		    strcpy (label, v);
		}
		if (strcmp (tok, "block.device") == 0 &&
		    MISC_get_token (line, "Q'", 2, v, STR_SIZE) > 0 &&
		    strcmp (v, Med_info.device) != 0) {
		    LE_send_msg (0, "Device %s does not match %s", 
						    v, Med_info.device);
		    Exit_with_eject (MEDCP_DEVICE_MISMATCH);
		}
	    }
	    p += off;
	}
	if (strlen (label) > 0 && !Med_info.is_mounted) {
	    if (MISC_systime (NULL) >= st + 4)
		break;
	    LE_set_option ("also stderr", 0);
	    LE_send_msg (0, "Non-empty label but not mounted");
	    LE_set_option ("also stderr", 1);
	    sleep (1);
	}
	else
	    break;
    }
    STR_free (cmd);
    return (1);
}

/*
static int Check_mount (char *device, int timeout) {
    char buf[LSHAL_BUF_SIZE], *p, line[STR_SIZE];
    int off;
    time_t st;

    if (timeout > 0)
	st = MISC_systime (NULL);
    while (1) {
	if (Execute_command ("mount", buf, LSHAL_BUF_SIZE, 1) != 0) {
	    LE_send_msg (0, "command mount failed");
	    Exit_with_eject (MEDCP_MOUNT_FAILED);
	}
	p = buf;
	while ((off = MISC_get_token (p, "S\n", 0, line, STR_SIZE)) > 0) {
	    char tok[STR_SIZE];
    
	    if (MISC_get_token (line, "", 0, tok, STR_SIZE) > 0 &&
		strcmp (tok, device) == 0 &&
		MISC_get_token (line, "", 1, tok, STR_SIZE) > 0 &&
		strcmp (tok, "on") == 0 &&
		MISC_get_token (line, "", 2, tok, STR_SIZE) > 0) {
		Med_info.dir = STR_copy (Med_info.dir, tok);
		return (1);
	    }
	    p += off;
	}
	sleep (1);
	if (timeout <= 0 || MISC_systime (NULL) >= st + timeout)
	    break;
    }
    LE_send_msg (0, "Med_info.device %s does not exist or cannot be mounted\n", device);
    return (0);
}
*/

/******************************************************************

    Writes the source files (Src) to CD. 
	
******************************************************************/

static void Write_cd () {
    char buf[BUF_SIZE], cmd[CMD_SIZE], *fname;
    int size, ret, dvd;

    dvd = 0;
    if (Med_info.capacity > 800000000.)
	dvd = 1;

    if (Blanking) {
	if (dvd) {
	    LE_send_msg (0, "DVD blanking is not implemented\n");
	    Exit_with_eject (MEDCP_BLANKING_MEDIA_FAILED);
	}
	if (strcmp (Med_info.type, "cd_rw") != 0) {
	    LE_send_msg (0, "Media is not rewritable\n");
	    Exit_with_eject (MEDCP_MEDIA_NOT_REWRITABLE);
	}
	LE_send_msg (LE_VL1, "** Blanking the CD\n");
	if (Med_info.is_mounted) {
	    sprintf (cmd, "umount %s", Med_info.device);
	    if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
		LE_send_msg (0, "umount %s failed\n", Med_info.device);
		Exit_with_eject (MEDCP_DEVICE_UNMOUNT_FAILED);
	    }
	}
	sprintf (cmd, "cdrecord dev=%s blank=fast", Med_info.device);
	if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
	    LE_send_msg (0, "Blanking CD failed\n");
	    Exit_with_eject (MEDCP_BLANKING_MEDIA_FAILED);
	}
	LE_send_msg (LE_VL1, "** Blanking the CD done\n");
	return;
    }

    if (Src == NULL)
	return;

/*    if (strcmp (Med_info.type, "cd_r") != 0 && 
			strcmp (Med_info.type, "cd_rw") != 0) {
	LE_send_msg (0, "Media is not writable\n");
	Exit_with_eject (MEDCP_MEDIA_NOT_WRITABLE);
    }
*/
    if (!Med_info.is_blank) {
	LE_send_msg (0, "Media is not blank\n");
	Exit_with_eject (MEDCP_MEDIA_NOT_BLANK);
    }

    if (dvd) {
	char *cmd_buf;
	LE_send_msg (LE_VL1, "** Writing to the DVD\n");
	cmd_buf = MISC_malloc (128 + strlen (Src));
	sprintf (cmd_buf, "sh -c (growisofs -quiet -Z /dev/dvd -R -J %s)", Src);
	ret = Run_command (cmd_buf, "not recognized as recordable DVD: 0\0not recognized as recordable DVD: 9\0No space left on device", 3);
	free (cmd_buf);
	if (ret == 3) {
	    LE_send_msg (0, "Media is too small to hold the data\n");
	    Exit_with_eject (MEDCP_MEDIA_TOO_SMALL);
	}
    }
    else {

	LE_send_msg (LE_VL1, "** Creating the ISO image\n");
	fname = NULL;
	size = Create_cd_image (&fname);
	if (size <= 0) {
	    ftruncate (Cd_image_fd, 0);
	    if (size == -2) {
		LE_send_msg (0, "Media is too small to hold the data\n");
		Exit_with_eject (MEDCP_MEDIA_TOO_SMALL);
	    }
	    Exit_with_eject (MEDCP_CREATE_CD_IMAGE_FAILED);
	}
    
	if (size > (int)Med_info.capacity) {
	    LE_send_msg (0, "Media is too small to hold the data\n");
	    ftruncate (Cd_image_fd, 0);
	    Exit_with_eject (MEDCP_MEDIA_TOO_SMALL);
	}

	LE_send_msg (LE_VL1, "** Writing to the CD\n");
	sprintf (cmd, "sh -c (cdrecord dev=%s %s)", Med_info.device, fname);
	ret = Run_command (cmd, "load media by hand\0media cannot be written\0No disk", 3);
	ftruncate (Cd_image_fd, 0);
	STR_free (fname);
    }
    if (ret == 3 || ret == 1) {
	LE_send_msg (0, "Writing CD failed - No disk?\n");
	Exit_with_eject (MEDCP_MEDIA_NOT_DETECTED);
    }
    if (ret == 2) {
	LE_send_msg (0, "Writing CD failed - Media cannot be written?\n");
	Exit_with_eject (MEDCP_MEDIA_NOT_WRITABLE);
    }
    if (ret != 0) {
	LE_send_msg (0, "Writing CD failed\n");
	Exit_with_eject (MEDCP_MEDIA_WRITE_FAILED);
    }

    if (Verification) {
	LE_send_msg (LE_VL1, "** Verifying the data on CD\n");
	if (Verify_write () < 0) {
	    LE_send_msg (0, "Data verification failed");
	    Exit_with_eject (MEDCP_MEDIA_VERIFY_FAILED);
	}
	else
	    LE_send_msg (LE_VL1, 
			"** Files \"%s\" copied to CD and verified\n", 
					Truncated_text (Src, 80));
    }
    else
	LE_send_msg (LE_VL1, "** Files \"%s\" copied to CD\n", 
					Truncated_text (Src, 80));
}

/******************************************************************

    Verifies files that have been written to the media. Returns 0
    on success or -1 if the verification fails.
	
******************************************************************/

static int Verify_write () {
    int err, cnt, mount;
    char buf[BUF_SIZE], cmd[CMD_SIZE];

    mount = 0;
    if (Media_type == MCP_CD) {
	if (setuid (0) == 0) {
	    sprintf (cmd, "mount %s %s", Med_info.device, Tmp_mount_dir);
	    if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
		LE_send_msg (0, "Failed in %s", cmd);
		Exit_with_eject (MEDCP_DEVICE_MOUNT_FAILED);
	    }
	    mount = 1;
	}
	else {
	    sprintf (cmd, "eject %s", Med_info.device);
	    Execute_command (cmd, buf, BUF_SIZE, 1);
	    sprintf (cmd, "eject -t %s", Med_info.device);
	    if( Execute_command (cmd, buf, BUF_SIZE, 1) != 0 )
	    {
	      Exit_with_eject (MEDCP_EJECT_T_FAILED);
	    }
    	}

	if (!Check_mount (Wait_time) || !Med_info.is_mounted) {
	    LE_send_msg (0, "Mounting media failed");
/*
printf ("    available %d device %s name %s capacity %f is_blank %d is_mounted %d dir %s type %s\n", Med_info.available, Med_info.device, Med_info.name, Med_info.capacity, Med_info.is_blank, Med_info.is_mounted, Med_info.dir, Med_info.type);
*/
	    Exit_with_eject (MEDCP_DEVICE_MOUNT_FAILED);
	}
    }

    err = 0;
    cnt = 0;
    while (1) {
	char tk[STR_SIZE];

	if (MISC_get_token (Src, "", cnt, tk, STR_SIZE) <= 0)
	    break;
	cnt++;
	if (Diff_files (tk, Med_info.dir, 1) != 0) {
	    err = -1;
	    break;
	}
    }
    if (Media_type == MCP_CD && mount) {
	sprintf (cmd, "umount %s", Med_info.device);
	Execute_command (cmd, buf, BUF_SIZE, 1);
    }
    return (err);
}

/******************************************************************

    Creates a CD image of all selected files. The image is 
    work_dir/cd_image. It returns the total size of all volume files 
    on the CD.
	
******************************************************************/

static int Create_cd_image (char **fname) {
    char buf[BUF_SIZE];
    char *cmd, name[STR_SIZE];
    int size;
    struct stat fst;

    Tmp_mount_dir = "/tmp/medcp/medcp_mnt";
    MISC_mkdir (Tmp_mount_dir);
    chmod ("/tmp/medcp", 0777);
    chmod (Tmp_mount_dir, 0777);
    strcpy (name, "/tmp/medcp/cd_image");
    chmod (name, 0777);
    Cd_image_fd = open (name, O_CREAT | O_RDWR, 0777);
    if (Cd_image_fd < 0) {
	LE_send_msg (0, "open %s failed (%d)", name, errno);
	return (-1);
    }
    chmod (name, 0777);
    while (1) {
	int ret;
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	ret = fcntl (Cd_image_fd, F_SETLK, &lock);
	if (ret == 0)
	    break;
	if (errno == EACCES || errno == EAGAIN) {
	    LE_send_msg (0, "Waiting for another medcp to finish...");
	    sleep (2);
	}
	else {
	    close (Cd_image_fd);
	    LE_send_msg (0, "fcntl F_SETLK on %s failed", name);
	    return (-1);
	}
    }

    cmd = MISC_malloc (STR_SIZE + strlen (Src));    
    sprintf (cmd, "sh -c \"mkisofs -quiet -l -R -o %s %s\"", name, Src);
    if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
	LE_send_msg (0, "mkisofs failed");
	return (-1);
    }

    if (stat (name, &fst) < 0) {
	if (errno == EOVERFLOW)
	    return (-2);
	LE_send_msg (0, "stat (%s) failed (%d)", name, errno);
	return (-1);
    }
    size = fst.st_size;

    *fname = STR_copy (*fname, name);
    free (cmd);

    return (size);
}

/******************************************************************

    Searches for "Med_info.device" and gets the device info using 
    "lshal". 
	
******************************************************************/

static int Search_device () {
    char buf[LSHAL_BUF_SIZE], *p, line[STR_SIZE], *dname, *cmd;
    int off, found, cdt;

    if (Device_name == NULL && getenv ("MEDCP_DEFAULT_DEVICE") != NULL)
	Device_name = STR_copy (Device_name, getenv ("MEDCP_DEFAULT_DEVICE"));

    LE_send_msg (LE_VL1, "** Searching device\n");
    if (Execute_command ("lshal -t", buf, LSHAL_BUF_SIZE, 0) != 0) {
	LE_send_msg (0, "lshal -t (finding CD-R device) failed");
	return (-1);
    }
    p = buf;
    found = 0;
    cdt = 0;
    while ((off = MISC_get_token (p, "S\n", 0, line, STR_SIZE)) > 0) {
	if (Device_name != NULL) {
	    if (strstr (line, Device_name) != NULL)
		found = 1;
	}
	else {
	    switch (Media_type) {

		case MCP_CD:
		if (strstr (line, "storage") != NULL) {
		    int ret = Look_for_CD (line);
		    if (ret > cdt) {
			cdt = ret;
			Med_info.name = STR_copy (Med_info.name, 
				line + MISC_char_cnt (line, " \t"));
		    }
		}
		break;
    
		case MCP_FLOPPY:
		if (strstr (line, "floppy") != NULL &&
		    strstr (line, "storage") != NULL)
		    found = 1;
		break;
    
		default:
		break;
	    }
	}
	if (found) {
	    Med_info.name = STR_copy (Med_info.name, 
				line + MISC_char_cnt (line, " \t"));
	    break;
	}
	p += off;
    }
    if (strlen (Med_info.name))
	dname = STR_gen (NULL, Hal_root, Med_info.name, NULL);
    else {
	LE_send_msg (0, "Cannot find the required device");
	return (-1);
    }

    cmd = STR_gen (NULL, "lshal -l -u ", dname, NULL);
    STR_free (dname);
    if (Execute_command (cmd, buf, LSHAL_BUF_SIZE, 0) != 0) {
	LE_send_msg (0, "(finding CD-R device) Command %s failed", cmd);
	return (-1);
    }
    STR_free (cmd);
    p = buf;
    while ((off = MISC_get_token (p, "S\n", 0, line, STR_SIZE)) > 0) {
	char tok[STR_SIZE], v[STR_SIZE];

	if (MISC_get_token (line, "", 1, tok, STR_SIZE) > 0 &&
	    strcmp (tok, "=") == 0) {
	    MISC_get_token (line, "S=", 0, tok, STR_SIZE);
	    if (strcmp (tok, "block.device") == 0 &&
		MISC_get_token (line, "Q'", 2, v, STR_SIZE) > 0) {
		Med_info.device = STR_copy (Med_info.device, v);
	    }
	    if (strcmp (tok, "storage.removable.media_available") == 0 &&
		MISC_get_token (line, "Q'", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0) {
		Med_info.available = 1;
	    }
	}
	p += off;
    }
    if (Med_info.device == NULL) {
	LE_send_msg (0, "Device not found");
	return (-1);
    }

    return (0);
}

/******************************************************************

    Looks for whether device "dname" is a CD or DVD. It returns 0
    if it is not. Otherwise it returns a number defined in the enum.
	
******************************************************************/

static int Look_for_CD (char *dname) {
    enum {NONE, DVD_R, CD_R, DVDRW, CDRW, DVDPLUSRW, DVDPLUSR, DVD};
    char *cmd, *p, buf[LSHAL_BUF_SIZE], line[STR_SIZE];
    int off, dev;

    cmd = STR_gen (NULL, "lshal -l -u ", Hal_root, dname, NULL);
    if (Execute_command (cmd, buf, LSHAL_BUF_SIZE, 0) != 0) {
	LE_send_msg (0, "In Look_for_CD %s failed", cmd);
	return (-1);
    }
    STR_free (cmd);
    dev = NONE;
    p = buf;
    while ((off = MISC_get_token (p, "S\n", 0, line, STR_SIZE)) > 0) {
	char tok[STR_SIZE], v[STR_SIZE];

	if (MISC_get_token (line, "", 1, tok, STR_SIZE) > 0 &&
	    strcmp (tok, "=") == 0) {
	    MISC_get_token (line, "S=", 0, tok, STR_SIZE);
	    if (strcmp (tok, "storage.cdrom.cdrw") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < CDRW) {
		dev = CDRW;
	    }
	    if (strcmp (tok, "storage.cdrom.dvdrw") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < DVDRW) {
		dev = DVDRW;
	    }
	    if (strcmp (tok, "storage.cdrom.dvdr") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < DVD_R) {
		dev = DVD_R;
	    }
	    if (strcmp (tok, "storage.cdrom.cdr") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < CD_R) {
		dev = CD_R;
	    }
	    if (strcmp (tok, "storage.cdrom.dvdplusrw") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < DVDPLUSRW) {
		dev = DVDPLUSRW;
	    }
	    if (strcmp (tok, "storage.cdrom.dvdplusr") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < DVDPLUSR) {
		dev = DVDPLUSR;
	    }
	    if (strcmp (tok, "storage.cdrom.dvd") == 0 &&
		MISC_get_token (line, "", 2, v, STR_SIZE) > 0 &&
		strcmp (v, "true") == 0 &&
		dev < DVD) {
		dev = DVD;
	    }
	}
	p += off;
    }
    return (dev);
}

/******************************************************************

    Mount/umount the floppy which is not managed by auto-mounting.
	
******************************************************************/

static int Mount_floppy () {
    char buf[BUF_SIZE], cmd[CMD_SIZE];
    int uid;

    Med_info.dir = STR_copy (Med_info.dir, "/media/floppy");
    uid = setuid (0);
    if (Mount == 0) {
	if (uid == 0)
	    sprintf (cmd, "umount %s", Med_info.device);
	else
	    sprintf (cmd, "sudo umount %s", Med_info.device);
	if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0) {
	    LE_send_msg (0, "Failed in %s", cmd);
	    Exit_with_eject (MEDCP_DEVICE_UNMOUNT_FAILED);
	}
	Med_info.is_mounted = 0;
    }
    else if (Mount == 1 || Blanking || Cp_from >= 0) {
	MISC_mkdir ("/media/floppy");
	if (uid == 0)
	    sprintf (cmd, "mount -t vfat %s %s", Med_info.device, "/media/floppy");
	else
	    sprintf (cmd, "sudo mount -t vfat %s %s", Med_info.device, "/media/floppy");
	if (Execute_command (cmd, buf, BUF_SIZE, 1) != 0 &&
	    strstr (buf, "mounted") == NULL) {
	    LE_send_msg (0, "Failed in \"%s\"", cmd);
	    LE_send_msg (0, "  - %s", buf);
	    Exit_with_eject (MEDCP_DEVICE_MOUNT_FAILED);
	}
	Med_info.is_mounted = 1;
    }
    return (0);
}

/******************************************************************

    Execute command "cmd". The output of the command is returned with
    "buf" of size "b_size". Returns 0 on success or -1 on failure.
    The command output is also sent to LE.
	
******************************************************************/

static int Execute_command (char *cmd, char *buf, int b_size, int log) {
    int ret, n_bytes, len;
    char *p, l[CMD_SIZE];

    LE_set_option ("also stderr", 0);
    LE_send_msg (0, "** RUN %s", Truncated_text (cmd, TRUNC_STR_SIZE));
    n_bytes = 0;
    ret = MISC_system_to_buffer (cmd, buf, b_size, &n_bytes);
    if (n_bytes > 0 && log) {
	p = buf;
	while ((len = MISC_get_token (p, "S\n", 0, l, CMD_SIZE)) > 0) {
	    if (strncmp (l, "Note:", 5) != 0)
		LE_send_msg (0, "    %s", Truncated_text (l, TRUNC_STR_SIZE));
	    p += len;
	}
    }
    if (ret != 0) {
	p = buf;
	while ((len = MISC_get_token (p, "S\n", 0, l, CMD_SIZE)) > 0) {
	    if (strstr (l, "Invalid node") != NULL)
		LE_send_msg (0, "    %s", l);
	    p += len;
	}
	LE_send_msg (0, "Ret %d in executing \"%s\"\n", 
				ret, Truncated_text (cmd, TRUNC_STR_SIZE));
	ret = -1;
    }
    LE_set_option ("also stderr", 1);
    return (ret);
}

/******************************************************************

    Execute command "cmd" as co-processor. Each line of the output
    of the command is matched with "cnt" strings in "strs" for
    detecting error conditions. Returns index + 1 if the string
    matched, 0 on success or -1 on failure. The command output is
    also sent to LE.
	
******************************************************************/

static int Run_command (char *cmd, char *strs, int cnt) {
    int ret, i;
    char buf[CMD_SIZE];
    void *cp;

    LE_set_option ("also stderr", 0);
    LE_send_msg (0, "** RUN %s", Truncated_text (cmd, TRUNC_STR_SIZE));
    LE_set_option ("also stderr", 1);
    ret = MISC_cp_open (cmd, MISC_CP_MANAGE, &cp);
    if (ret < 0) {
	LE_send_msg (0, "MISC_cp_open (%s) failed (%d)", 
			Truncated_text (cmd, TRUNC_STR_SIZE), ret);
	return (-1);
    }
    while (1) {
	ret = MISC_cp_read_from_cp (cp, buf, CMD_SIZE);
	if (ret == 0)
	    sleep (1);
	else if (ret < 0) {
	    int stat = MISC_cp_get_status (cp);
	    if (stat != 0) {
		LE_set_option ("also stderr", 0);
		LE_send_msg (0, "Cmd (%s) failed (%d)", 
			Truncated_text (cmd, TRUNC_STR_SIZE), stat);
		LE_set_option ("also stderr", 1);
		return (-1);
	    }
	    return (0);
	}
	else {
	    char *p = strs;
	    LE_set_option ("also stderr", 0);
	    LE_send_msg (0, "    %s", Truncated_text (buf, TRUNC_STR_SIZE));
	    LE_set_option ("also stderr", 1);
	    for (i = 0; i < cnt; i++) {
		if (strstr (buf, p) != NULL) {
		    MISC_cp_close (cp);
		    return (i + 1);
		}
		p += strlen (p) + 1;
	    }
	}
    }
}

/******************************************************************

    Returns a truncated string of "cmd" to "size" bytes.
	
******************************************************************/

static char *Truncated_text (char *cmd, int size) {
    static char buf[STR_SIZE];
    if (size > STR_SIZE)
	size = STR_SIZE;
    if (strlen (cmd) < size)
	return (cmd);
    strncpy (buf, cmd, size);
    buf[size - 4] = '\0';
    strcat (buf, "...");
    return (buf);
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
    int cnt;

    err = 0;
    Cp_options = STR_cat (Cp_options, "-Rf");
    Verification = 0;
    while ((c = getopt (argc, argv, "d:o:l:t:umbecpvh?")) != EOF) {
	switch (c) {

            case 'd':
		Device_name = STR_copy (Device_name, optarg);
                break;

            case 'o':
		Cp_options = STR_copy (Cp_options, optarg);
                break;

            case 'l':
		Log_name = STR_copy (Log_name, optarg);
                break;

            case 't':
		if (sscanf (optarg, "%d", &Wait_time) != 1) {
		    fprintf (stderr, "medcp: Bad -t option (%s)\n", optarg);
		    exit (1);
		}
                break;

            case 'b':
		Blanking = 1;
                break;

            case 'c':
		Verification = 1;
                break;

            case 'e':
		Eject = 1;
                break;

            case 'p':
		Print_mount_dir = 1;
                break;

            case 'm':
		Mount = 1;
                break;

            case 'u':
		Mount = 0;
                break;

            case 'v':
		Verbose = 1;
                break;

	    case 'h':
		Print_usage (argv);
		exit (0);

	    case '?':
		Print_usage (argv);
		exit (1);
	}
    }

    cnt = -1;
    Media_type = MCP_UNDEFINED;
    Src = NULL;
    Dest = NULL;
    while (optind <= argc - 1) {
	int is_cd, is_floppy;

	cnt++;
	is_cd = (strcmp (argv[optind], "cd") == 0 || 
			strcmp (argv[optind], "cdrecorder") == 0 ||
			strcmp (argv[optind], "cdrom") == 0);
	is_floppy = (strcmp (argv[optind], "floppy") == 0 ||
			strcmp (argv[optind], "diskette") == 0);
	if (is_cd || is_floppy) {
	    if (Media_type != MCP_UNDEFINED) {
		fprintf (stderr, "medcp: Device specified more than once\n");
		exit (1);
	    }
	    if (cnt == 0)
		Cp_from = 1;
	    else if (optind == argc - 1)
		Cp_from = 0;
	    else {
		fprintf (stderr, "medcp: Unexpected device name on command line\n");
		exit (1);
	    }
	    if (is_cd)
		Media_type = MCP_CD;
	    else
		Media_type = MCP_FLOPPY;
	    optind++;
	    continue;
	}

	if (optind < argc - 1) {
	    if (Src != NULL)
		Src = STR_cat (Src, " ");
	    Src = STR_cat (Src, argv[optind]);
	}
	else 
	    Dest = STR_cat (Dest, argv[optind]);
	optind++;
    }
    if (Media_type == MCP_UNDEFINED) {
	fprintf (stderr, "medcp: Device name not specified\n");
	exit (1);
    }
    if (Mount == 0)
	Print_mount_dir = 0;
    if (Print_mount_dir)
	Mount = 1;
    if (Mount >= 0) {
	Blanking = 0;
	Cp_from = -1;
    }
    if (Blanking)
	Cp_from = -1;

    if (Cp_from >= 0 && Src == NULL && !Eject) {
	fprintf (stderr, "medcp: Source not specified\n");
	exit (1);
    }
    if (Cp_from == 1 && Dest == NULL && !Eject) {
	fprintf (stderr, "medcp: Destination not specified\n");
	exit (1);
    }

    return (err);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *bname;

    bname = MISC_basename (argv[0]);

    printf ("Usage:  %s [options] cd<floppy> f_list dest_dir\n", bname);
    printf ("    or  %s [options] f_list cd<floppy>\n", bname);
    printf ("    Where f_list is a list of files and dirs.\n");
    printf ("    The first copies all listed files and dirs from the media to dest_dir\n");
    printf ("    The second copies all listed files and all files and dirs in\n");
    printf ("    the listed dirs (not the listed dirs themselves) to the media.\n");
    printf ("    Exit values: 0 - success; 5 - Data too large for the disk; 4 - Data \n");
    printf ("    verification failed; 3 - No disk; 2 - CD not writable; 1 - Other errors.\n");
    printf ("    Use \"lem %s\" to view log.\n", bname);
    printf ("    \"cdrecorder\" and \"cdrom\" are accepted as aliases of \"cd\".\n");
    printf ("    Options:\n");
    printf ("        -d dev_name (Specifies a unique part of the device name. e.g.\n");
    printf ("           \"ND_3530A\". To find this, use lshal -t. Default: Known names)\n");
    printf ("        -o cp_options (Options to be passed to cp. Default: -Rf)\n");
    printf ("        -l log_name (Log file name. Default: %s)\n", bname);
    printf ("        -b (Blanking the media (a RW disk or floppy))\n");
    printf ("        -c (Check data after copy)\n");
    printf ("        -e (eject the media after reading/writing it)\n");
    printf ("        -u (unmount the media)\n");
    printf ("        -p (prints mount directory)\n");
    printf ("        -m (mount the media)\n");
    printf ("        -t wait_time (max seconds of waiting media, Default: 20)\n");
    printf ("        -v (Verbose mode)\n");
    printf ("    Examples:\n");
    printf ("    %s d1 d2 cd\n", bname);
    printf ("        - Copies all files and dirs in directories \"d1\" and \"d2\" to CD.\n");
    printf ("    %s -c f1 f2 floppy\n", bname);
    printf ("        - Copies, with verification, files \"fi\" and \"f2\" to diskette.\n");
    printf ("    %s cd f1 d2 .\n", bname);
    printf ("        - Copies file \"f1\" and the entire directory of \"d2\"\n");
    printf ("          on CD to the current dir.\n");
    printf ("    %s -p cd\n", bname);
    printf ("        - prints the mount point if mounted.\n");
    return;
}




