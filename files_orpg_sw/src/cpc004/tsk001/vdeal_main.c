
/******************************************************************

    vdeal's main module.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:10:21 $
 * $Id: vdeal_main.c,v 1.7 2014/10/03 16:10:21 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include "vdeal.h"

static char *Ppi_fname = NULL;	/* input file name */
int dbg;
int Test_mode;
int Verbose_mode = 0;

static int Read_ppi_file (char *name, Vdeal_t *vdv);
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int put_image (char *name, Vdeal_t *vdv);
static int Process_image (Vdeal_t *vdv);
static int Get_field_value (char *text, char *key, void *vp);

/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char *argv[]) {
    Vdeal_t vdv;

    dbg = 0;
    Test_mode = 0;
    memset (&vdv, 0, sizeof (Vdeal_t));

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (getenv ("VDEAL_TEST") != NULL)
	Test_mode = 1;

    if (Ppi_fname != NULL) {	/* process an image */
	if (Process_image (&vdv) != 0)
	    exit (1);
    }
    else {		/* real time processing */
#ifdef VDEAL_FIO
	MISC_log ("Realtime WSR88D processing not compiled\n");
#else
	VDR_realtime_process (argc, argv, &vdv);
#endif
    }

    exit (0);
}


/******************************************************************

    Dealiases an ppi image.

******************************************************************/

static int Process_image (Vdeal_t *vdv) {
    char out_fl_name[256];

    if (Read_ppi_file (Ppi_fname, vdv) != 0)
	return (-1);

    if (VDD_process_image (vdv) != 0) {
	MISC_log ("Process_image done - deali exits\n");
	return (-1);
    }

    /* save the output image */
    sprintf (out_fl_name, "%s.deal", Ppi_fname);
    if (put_image (out_fl_name, vdv) != 0)
	return (-1);

    return (0);
}

/******************************************************************

    Reads ppi image file "name". The image header is a null-terminated
    string. The following fields are specified in the header with
    for format of "key = value". Each field is separated by "," or
    " ".

    Required fields are:
	size - Total number of gates
	n_gates - Number of gates in each radial
	n_radials - Number of radials in the ppi
	Nyquist - Nyquist velocity in data levels
	elevation - Elevation angle in degrees.

    Optional fields are:
	azimuth - Starting azimuth, in degrees, of the ppi
	start_range - Starting range in meters
	gate_width - Gate width in degrees
	gate_size - Gate size in meters
	data_scale - Data scale factor
	data_off - Data offset
	    (Physical value = (data - data_off) / data_scale)
	data_time - Unix time of the data.

******************************************************************/

static int Read_ppi_file (char *name, Vdeal_t *vdv) {
    FILE *fl;
    float fsize, n_gates, n_radials, nyq, elev, g_size, fdata_off;
    float data_time;
    char buf[256];
    unsigned char *vel, *spw, *dbz;
    int size, data_off, n, secs[4];

    if ((fl = fopen (name, "r")) == NULL) {
	MISC_log ("fopen ppi file %s failed\n", name);
	return (-1);
    }

    if (fgets (buf, 256, fl) == NULL)
	return (0);
    buf[255] = '\0';
    if (!Get_field_value (buf, "size", &fsize) ||
	!Get_field_value (buf, "n_gates", &n_gates) ||
	!Get_field_value (buf, "n_radials", &n_radials) ||
	!Get_field_value (buf, "elevation", &elev)) {
	MISC_log ("Unexpected file header (%s)\n", buf);
	return (-1);
    }
    if (!Get_field_value (buf, "Nyquist", &nyq)) {
	MISC_log ("Nyquist velocity not found - dealiased?\n");
	return (-1);
    }
    if (fsize != n_gates * n_radials) {
	MISC_log ("Data size incorrect (%d, expect %d)\n", 
				(int)fsize, (int)(n_gates * n_radials));
	return (-1);
    }
    vdv->xz = n_gates;
    vdv->yz = n_radials;
    vdv->nyq = nyq;
    vdv->elev = elev;

    if (!Get_field_value (buf, "azimuth", &(vdv->start_azi)))
	vdv->start_azi = 0;
    if (!Get_field_value (buf, "start_range", &(vdv->start_range)))
	vdv->start_range = 0;
    if (!Get_field_value (buf, "gate_width", &(vdv->gate_width)))
	vdv->gate_width = .5;	/* assume super res */
    if (!Get_field_value (buf, "data_scale", &(vdv->data_scale)))
	vdv->data_scale = 2;
    if (!Get_field_value (buf, "gate_size", &g_size))
	g_size = 250;
    if (!Get_field_value (buf, "data_off", &fdata_off))
	data_off = 129;
    else
	data_off = (int)fdata_off;
    if (!Get_field_value (buf, "data_time", &data_time))
	data_time = time (NULL);
    vdv->dtm = data_time;
    vdv->data_off = data_off;
    vdv->g_size = g_size;
    if (vdv->start_range != 0)
	vdv->data_type |= DT_SUB_IMAGE;
    vdv->vol_num = 1;	/* must start with non-zero */

    n = Get_field_value (buf, "secs", secs);
    PP_setup_prf_sectors (vdv, nyq, n, secs);

    size = (int)fsize;

    /* read the velocity */
    fseek (fl, strlen (buf) + 1, SEEK_SET);
    vel = (unsigned char *) MISC_malloc (size);
    if (fread ((char *)vel, sizeof (char), size, fl) != size) {
	MISC_log ("Error reading the vel field (%s)\n", name);
	return (-1);
    }

    /* read the spw */
    spw = (unsigned char *) MISC_malloc (size);
    if (fread ((char *)spw, sizeof (char), size, fl) != size) {
	MISC_log ("SPW not found\n");
	free (spw);
    }
    else {
	PP_convert_spw (spw, size, data_off);
	vdv->spw = spw;
    }

    /* read the dbz */
    dbz = (unsigned char *) MISC_malloc (size);
    if (fread ((char *)dbz, sizeof (char), size, fl) != size) {
	MISC_log ("DBZ not found\n");
	free (dbz);
    }
    else
	vdv->dbz = dbz;

    fclose (fl);

    {	/* remove second trip data and change data scale */
	int x, y, rescale;
	double s = 2. / vdv->data_scale;
	rescale = 0;
	if (vdv->data_scale != 2.f)
	    rescale = 1;
	for (y = 0; y < vdv->yz; y++) {
	    unsigned char *v = vel + y * vdv->xz;
	    for (x = 0; x < vdv->xz; x++) {
		if (v[x] > 1) {
		    if (rescale)	/* change data scale */
			v[x] = Myround ((v[x] - data_off) * s) + data_off;
		}
	    }
	}
    }
    vdv->inp = vel;

    return (0);
}

#define LOC_CTR_SIZE 32

static int Get_field_value (char *text, char *key, void *vp) {
    int i;

    for (i = 0; i < 2; i++) {
	float *v = (float *)vp;
	char *p, k[LOC_CTR_SIZE + 4];

	strncpy (k, key, LOC_CTR_SIZE);
	k[LOC_CTR_SIZE - 1] = '\0';
	if (i == 0)
	    strcat (k, "=");
	else
	    strcat (k, " =");
	if ((p = strstr (text, k)) != NULL) {
	    if (strcmp (key, "secs") == 0) {
		int n, *vi;
		vi = (int *)vp;
		n = sscanf (p + strlen (k), "%d %d %d %d",
						vi, vi + 1, vi + 2, vi + 3);
		if (n > 0)
		    return (n);
	    }
	    else if (sscanf (p + strlen (k), "%f", v) == 1)
		return (1);
	}
    }
    return (0);
}

/******************************************************************

    Outputs the dealiased image file. The output image must be
    converted to 8 bit. Scale down can added later is required.

*******************************************************************/

static int put_image (char *name, Vdeal_t *vdv) {
    FILE *fl;
    unsigned char *p, *pe, *image;
    short *sp;
    int size, err, data_off, rescale;
    char buf[256];
    double s;

    size = vdv->xz * vdv->yz;
    image = (unsigned char *)MISC_malloc (size * sizeof (char));
    p = image;
    sp = vdv->out;
    pe = p + size;
    s = vdv->data_scale / 2.;
    rescale = 0;
    if (vdv->data_scale != 2.f)
	rescale = 1;
    data_off = vdv->data_off;
    while (p < pe) {
	short t;
	t = *sp;
	if (t == SNO_DATA)
	    *p = BNO_DATA;
	else {
	    if (rescale)
		t = Myround ((t - data_off) * s) + data_off;
	    if (t > 255)
		t = 255;
	    else if (t < 2)
		t = 2;
	    *p = t;
	}
	p++;
	sp++;
    }

    err = 0;
    if ((fl = fopen (name, "w")) == NULL) {
	MISC_log ("fopen output image (%s) failed\n", name);
	err = 1;
    }

    sprintf (buf, "size = %d, ", size);
    sprintf (buf + strlen (buf), "n_gates = %d, ", vdv->xz);
    sprintf (buf + strlen (buf), "n_radials = %d, ", vdv->yz);
    sprintf (buf + strlen (buf), "elevation = %g, ", vdv->elev);

    sprintf (buf + strlen (buf), "azimuth = %g, ", vdv->start_azi);
    sprintf (buf + strlen (buf), "start_range = %g, ", vdv->start_range);
    sprintf (buf + strlen (buf), "gate_width = %g, ", vdv->gate_width);
    sprintf (buf + strlen (buf), "gate_size = %d, ", vdv->g_size);
    sprintf (buf + strlen (buf), "data_scale = %g, ", vdv->data_scale);
    sprintf (buf + strlen (buf), "data_off = %d, ", vdv->data_off);
    sprintf (buf + strlen (buf), "data_time = %d, ", (int)vdv->dtm);

    /* writes the header */
    if (fwrite (buf, 1, strlen (buf) + 1, fl) != strlen (buf) + 1) {
	MISC_log ("Error writing image header (%s)\n", name);
	err = 1;
    }

    /* writes the image */
    if (!err && fwrite (image, sizeof (char), size, fl) != size) {
	MISC_log ("Error writing the image file (%s)\n", name);
	err = 1;
    }
    free (image);
    if (fl != NULL)
	fclose (fl);
    if (err)
	return (-1);
    return (0);
}

char *VDM_get_image_label () {
    static char label[64];
    int h, m, s, ele;

    if (Ppi_fname == NULL ||
	Ppi_fname[MISC_char_cnt (Ppi_fname, "\0_")] != '_' ||
	sscanf (Ppi_fname + MISC_char_cnt (Ppi_fname, "\0_") + 1, 
		"%*d%*c%*d%*c%*d%*c%d%*c%d%*c%d%*c%d", &h, &m, &s, &ele) != 4)
	return (NULL);
    sprintf (label, "%.2d_%.2d_%.2d.%d", h, m, s, ele);
    return (label);
}

char *VDM_get_data_dir (char *buf, int buf_size) {
    char *p;
    int len;

    if (Ppi_fname == NULL)
	return (NULL);
    p = Ppi_fname + strlen (Ppi_fname) - 1;
    while (p >= Ppi_fname) {
	if (*p == '/')
	    break;
	p--;
    }
    if (p < Ppi_fname)
	len = 1;
    else
	len = p - Ppi_fname;
    if (len >= buf_size) {
	MISC_log ("VDM_get_data_dir buffer too small\n");
	exit (1);
    }
    if (p < Ppi_fname)
	buf[0] = '.';
    else
	memcpy (buf, Ppi_fname, len);
    buf[len] = '\0';
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
    extern int VDB_threads, VDD_threads;
    int c;                  /* used by getopt */
    int err, realtime, i, dopt;
    char *opt_str = "t:d:p:nh?";

    /* determine if realtime */
    if (argc == 1)
 	realtime = 1;
    else if (argc == 2 && argv[1][0] != '-')
	realtime = 0;
    else {
 	realtime = 1;
	for (i = 1; i < argc; i++) {
	    int k;
	    if (argv[i][0] == '-') {
		if (strstr (argv[i], "h") || strstr (argv[i], "?")) {
		    printf ("Image usage:\n");
		    Print_usage (argv);
		    printf ("Realtime usage:\n");
		    return (0);
		}
		for (k = 0; k < strlen (opt_str); k++) {
		    char str[2];
		    if (opt_str[k] == ':')
			continue;
		    str[0] = opt_str[k];
		    str[1] = '\0';
		    if (strstr (argv[i], str))
			realtime = 0;
		}
	    }
	}
    }
    if (realtime)
	return (0);

    err = dopt = 0;
    while ((c = getopt (argc, argv, opt_str)) != EOF) {

	switch (c) {
	    extern int No_reprocess;
	    int nt;
	    extern int Max_partition_size;

	    case 'T':
		break;

	    case 't':
		if (sscanf (optarg, "%d", &nt) != 1 || nt <= 0) {
		    fprintf (stderr, "Unexpected option -t (%s)\n", optarg);
		    exit (1);
		}
		VDB_threads = nt;
		break;

	    case 'd':
		if (sscanf (optarg, "%d", &nt) != 1 || nt <= 0) {
		    fprintf (stderr, "Unexpected option -d (%s)\n", optarg);
		    exit (1);
		}
		VDD_threads = nt;
		dopt = 1;
		break;

	    case 'p':
		if (sscanf (optarg, "%d", &Max_partition_size) != 1 ||
					Max_partition_size <= 30) {
		    fprintf (stderr, "Unexpected option -p (%s)\n", optarg);
		    exit (1);
		}
		break;

	    case 'n':
		No_reprocess = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		exit (0);
	}
    }
    if (!dopt)
	VDD_threads = VDB_threads;

    if (optind == argc - 1)		/* get the input file name  */
	Ppi_fname = argv[optind];

    return (0);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Velocity dealiases a data file or realtime data flow using\n\
	the 2D method. If data_file_name is missing, realtime data flow\n\
	is assumed.\n\
        Options:\n\
         -d threads (Number of threads used in dealiasing. Default = -t option.\n\
            Can be more than physical cores to use more CPUs)\n\
         -t threads (Number of threads used. Default = 1. Should not be\n\
            more than physical cores)\n\
         -p partition_size (partition size in phase 2. Default = 56)\n\
         -n (Re-processing is disabled - For testing)\n\
         -h (Prints usage info)\n\
";

    printf ("Usage:  %s [options] data_file_name\n", argv[0]);
    printf ("%s\n", usage);
}

#ifdef VDEAL_FIO
/* functions for the file IO version (independent of WSR88D) of vdeal */

int VDR_get_ext_wind (int alt, int up, double *speed, double *dir) {
    return -1;		/* need to implement this if external EW is used */
}

char *VDR_get_image_label () {
    return ("'");	/* not used */
}

int VDR_output_processed_radial (Vdeal_t *vdv) {
    return (0);		/* not used */
}

void VDR_status_log (const char *msg) {
}

#endif

