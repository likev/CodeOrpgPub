
/******************************************************************

    vdeal's debugging/testing routines.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/14 19:15:22 $
 * $Id: vdeal_test.c,v 1.3 2014/07/14 19:15:22 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include "vdeal.h"

char *Hd_fields = NULL;
static int Offset = 0;

static char *Create_hd (int xsize, int ysize);
static int Get_field_value (char *text, char *key, float *v);
static void Add_hd_fields (char *fields);

/*******************************************************************

     Sets data offset for dump_simage.

*******************************************************************/

int dump_offset (int offset) {
    Offset = offset;
    return (0);
}

/******************************************************************

    Dumps a two-byte image into a file.

******************************************************************/

int dump_simage (char *name, short *image, int xsize, int ysize, int stride) {
    FILE *fl;
    unsigned char *ima;
    static int cnt = 0;
    char fname[256];
    int y, size;
    char *hd;

    size = xsize * ysize;
    hd = Create_hd (xsize, ysize);
    ima = (unsigned char *)MISC_malloc (size * sizeof (char));

    for (y = 0; y < ysize; y++) {
	int tt, i;
	short *spt;
	unsigned char *cpt;

	spt = image + (stride * y);
	cpt = ima + (xsize * y);
	for (i = 0; i < xsize; i++) {
	    tt = spt[i];
	    if (tt == SNO_DATA)
		cpt[i] = BNO_DATA;
	    else {
		tt += Offset;
		if (tt > 255)
		    tt = 255;
		else if (tt < 2)
		    tt = 2;
		cpt[i] = tt;
	    }
	}
    }
    Offset = 0;

    if (strlen (VDR_get_image_label ()) == 0)
	sprintf (fname, "%s.s%d.img", name, cnt++);
    else
	sprintf (fname, "%s.%s.img", name, VDR_get_image_label ());
    if ((fl = fopen (fname, "w")) == NULL) {
	MISC_log ("Can not create test image (%s)\n", fname);
	exit (1);
    }

    /* writes the header */
    if (fwrite (hd, 1, strlen (hd) + 1, fl) != strlen (hd) + 1) {
	MISC_log ("Error writing image header (%s)\n", fname);
	exit (1);
    }

    /* writes the image */
    if (fwrite (ima, sizeof (char), size, fl) != size) {
	MISC_log ("Error writing the image file (%s)\n", fname);
	exit (1);
    }
    free (ima);
    fclose (fl);
    return (0);
}

static void Add_hd_fields (char *fields) {

    Hd_fields = STR_copy (Hd_fields, fields);
}

static char *Create_hd (int xsize, int ysize) {
    static char hd[256];

    sprintf (hd, "size= %d, ", xsize * ysize);
    sprintf (hd + strlen (hd), "n_gates= %d, ", xsize);
    sprintf (hd + strlen (hd), "n_radials= %d, ", ysize);
    if (Hd_fields != NULL && Hd_fields[0] != '\0') {
	strcat (hd, Hd_fields);
	Hd_fields[0] = '\0';
    }
    return (hd);
}

/******************************************************************

    Dumps a byte image into a file.

*******************************************************************/

int dump_bimage (char *name, unsigned char *image, 
				int xsize, int ysize, int stride) {
    FILE *fl;
    static int cnt = 0;
    unsigned char *ima;
    char fname[256];
    char *hd;
    int size, y;

    if (strlen (VDR_get_image_label ()) == 0)
	sprintf (fname, "%s.b%d.img", name, cnt++);
    else
	sprintf (fname, "%s.%s.img", name, VDR_get_image_label ());

    if ((fl = fopen (fname, "w")) == NULL) {
	MISC_log ("Can not create test image (%s)\n", fname);
	exit (1);
    }

    size = xsize * ysize;
    hd = Create_hd (xsize, ysize);
    ima = (unsigned char *)MISC_malloc (size * sizeof (char));

    for (y = 0; y < ysize; y++) {
	unsigned char *pt, *cpt;

	pt = image + (stride * y);
	cpt = ima + (xsize * y);
	memcpy (cpt, pt, xsize);
    }

    /* writes the header */
    if (fwrite (hd, 1, strlen (hd) + 1, fl) != strlen (hd) + 1) {
	MISC_log ("Error writing image header (%s)\n", fname);
	exit (1);
    }

    /* writes the image */
    if (fwrite (ima, sizeof (char), size, fl) != size) {
	MISC_log ("Error writing the image file (%s)\n", fname);
	exit (1);
    }

    free (ima);
    fclose (fl);
    return (0);
}

/************************************************************************

    Create image with new Nyq of "nnq" for testing.

************************************************************************/

int Aliase_image (unsigned char *image, Vdeal_t *vdv, char *fname) {
    int x, y;
    char out_fl_name[256];
    int stride, nnq, xsize, ysize, size;
    FILE *fl;
    char buf[256];

    stride = vdv->xz;
    nnq = vdv->nyq;
    xsize = vdv->xz;
    ysize = vdv->yz;
    size = xsize * ysize;

    for (y = 0; y < ysize; y++) {
	for (x = 0; x < xsize; x++) {
	    int v = image[y * stride + x];
	    if (v > 1) {
		v = v - vdv->data_off;
		while (v > nnq)
		    v -= 2 * nnq;
		while (v < -nnq)
		    v += 2 * nnq;
		v += 129;
	    }
	    image[y * stride + x] = v;
	}
    }
    sprintf (out_fl_name, "%s.%d", fname, nnq);

    if ((fl = fopen (out_fl_name, "w")) == NULL) {
	MISC_log ("Can not create aliased image (%s)\n", out_fl_name);
	return (-1);
    }

    sprintf (buf, "size = %d, ", size);
    sprintf (buf + strlen (buf), "n_gates = %d, ", vdv->xz);
    sprintf (buf + strlen (buf), "n_radials = %d, ", vdv->yz);
    sprintf (buf + strlen (buf), "elevation = %g, ", vdv->elev);
    sprintf (buf + strlen (buf), "Nyquist = %d, ", nnq);

    sprintf (buf + strlen (buf), "azimuth = %g, ", vdv->start_azi);
    sprintf (buf + strlen (buf), "start_range = %g, ", vdv->start_range);
    sprintf (buf + strlen (buf), "gate_width = %g, ", vdv->gate_width);
    sprintf (buf + strlen (buf), "gate_size = %d, ", vdv->g_size);
    sprintf (buf + strlen (buf), "data_scale = %g, ", vdv->data_scale);
    sprintf (buf + strlen (buf), "data_off = %d, ", vdv->data_off);
    sprintf (buf + strlen (buf), "data_time = %d, ", (int)vdv->dtm);

    /* writes the header */
    if (fwrite (buf, 1, strlen (buf) + 1, fl) != strlen (buf) + 1) {
	MISC_log ("Error writing image header (%s)\n", out_fl_name);
	return (-1);
    }

    /* writes the image */
    if (fwrite (image, sizeof (char), size, fl) != size) {
	MISC_log ("Error writing the aliased image file (%s)\n", out_fl_name);
	return (-1);
    }

    fclose (fl);
    return (0);
}

/**************************************************************************

    Dumps dmap for testing and debugging.

**************************************************************************/

int VDT_dump_dmap (char *name, Vdeal_t *vdv) {
    int x, y;
    unsigned char *dmap, *inp;
    
    dmap = vdv->dmap;
    inp = vdv->inp;
    for (y = 0; y < vdv->yz; y++) {
	for (x = 0; x < vdv->xz; x++) {
	    int t, v;
	    unsigned char *m;
	    m = dmap + (y * vdv->xz + x);
	    v = inp[y * vdv->xz + x];
	    t = *m;
	    if (v != BNO_DATA)
		*m = 129;
	    if (t & DMAP_BE)
		*m = 180;
	    if (t & DMAP_BH)
		*m = 200;
	}
    }
    dump_bimage (name, vdv->dmap, vdv->xz, vdv->yz, vdv->xz);
    return (0);
}

/****************************************************************************

    Dumps the EW map (in both text and image) for debugging purpose.

****************************************************************************/

int VDT_dump_efs (char *name, Vdeal_t *vdv, unsigned char bit) {
    Ew_struct_t *ew;
    int x, y;
    unsigned char *img;

    ew = &(vdv->ew);
    img = MISC_malloc (vdv->yz * vdv->xz);
    for (y = 0; y < vdv->yz; y++) {
	int yi, xi, e;

	yi = vdv->ew_aind[y];
	for (x = 0; x < vdv->xz; x++) {

	    xi = x / ew->rz;
	    e = ew->efs[yi * ew->n_rgs + xi];
	    if (!(e & bit))
		img[y * vdv->xz + x] = BNO_DATA;
	    else
		img[y * vdv->xz + x] = 200;
	}
    }

    dump_bimage (name, img, vdv->xz, vdv->yz, vdv->xz);
    free (img);
    return (0);
}

/****************************************************************************

    Dumps the env flag map for debugging purpose.

****************************************************************************/

int VDT_dump_ew (char *name, Vdeal_t *vdv, char *field) {
    Ew_struct_t *ew;
    char buf[256];

    ew = &(vdv->ew);
    sprintf (buf, "start_y = %d, sub_sample_x = %d, sub_sample_y = %d, azimuth = %.1f, start_range = 0, gate_width = %.1f, gate_size = %d, elevation = %.2f, ",
		Myround (vdv->start_azi * vdv->yz / 360.), ew->rz, 
		Myround (ew->az * vdv->yz / 360.),
		vdv->start_azi, vdv->gate_width, vdv->g_size, vdv->elev);

    if (strcmp ("ews", field) == 0) {
	Add_hd_fields (buf);
	dump_simage (name, ew->ews, ew->n_rgs, ew->n_azs, ew->n_rgs);
    }
    else if (strcmp ("ewm", field) == 0) {
	Add_hd_fields (buf);
	dump_simage (name, ew->ewm, ew->n_rgs, ew->n_azs, ew->n_rgs);
    }
    else if (strcmp ("efs", field) == 0) {
	Add_hd_fields (buf);
	dump_bimage (name, ew->efs, ew->n_rgs, ew->n_azs, ew->n_rgs);
    }
    else if (strcmp ("eww", field) == 0) {
	int i;
	unsigned char *p = ew->eww;
	Add_hd_fields (buf);
	for (i = 0; i < ew->n_rgs * ew->n_azs; i++) {
	    if (*p == 1)
		*p = 80;
	    else if (*p == 2)
		*p = 100;
	    else if (*p == 4)
		*p = 120;
	    else if (*p == 8)
		*p = 140;
	    else if (*p == 16)
		*p = 160;
	    p++;
	}
	dump_bimage (name, ew->eww, ew->n_rgs, ew->n_azs, ew->n_rgs);
	p = ew->eww;
	for (i = 0; i < ew->n_rgs * ew->n_azs; i++) {
	    if (*p == 80)
		*p = 1;
	    else if (*p == 100)
		*p = 2;
	    else if (*p == 120)
		*p = 4;
	    else if (*p == 140)
		*p = 8;
	    else if (*p == 160)
		*p = 16;
	    p++;
	}
    }

    return (0);
}

/****************************************************************************

    Reads the EW map file "name" to "ew".

****************************************************************************/

int VDT_read_ew (char *name, Ew_struct_t *ew) {
    FILE *fl;
    char buf[256];
    float xdim, ydim, size;
    int s, xz, yz, x, y;
    unsigned char sb[256];

    if ((fl = fopen (name, "r")) == NULL) {	/* open image file */
	MISC_log ("Error open (%s) - not read\n", name);
	return (0);
    }

    /* read the text header */
    if (fgets (buf, 256, fl) == NULL)
	return (0);
    buf[255] = '\0';
    if (!Get_field_value (buf, "size", &size) ||
	!Get_field_value (buf, "n_gates", &xdim) ||
	!Get_field_value (buf, "n_radials", &ydim) ||
	size != xdim * ydim ||
	xdim > 256.f) {
	MISC_log ("Bad header in file: %s\n", name);
	exit (1);
    }

    fseek (fl, strlen (buf) + 1, SEEK_SET);
    xz = xdim;
    yz = ydim;
    for (y = 0; y < ew->n_azs; y++) {
	if (y >= yz) {
	    for (x = 0; x < ew->n_rgs; x++)
		ew->ews[y * ew->n_rgs + x] = SNO_DATA;
	    continue;
	}
	if (fread (sb, sizeof (char), xz, fl) != xz) {
	    MISC_log ("Reading data from file (%s) failed\n", name);
	    exit (1);
	}
	s = xz;
	if (ew->n_rgs < s)
	    s = ew->n_rgs;
	for (x = 0; x < s; x++) {
	    if (sb[x] == 0)
		ew->ews[y * ew->n_rgs + x] = SNO_DATA;
	    else
		ew->ews[y * ew->n_rgs + x] = sb[x];
	}
 	for (x = s; x < ew->n_rgs; x++)
	    ew->ews[y * ew->n_rgs + x] = SNO_DATA;
    }
    return (0);
}

#define LOC_CTR_SIZE 32

static int Get_field_value (char *text, char *key, float *v) {
    int i;

    for (i = 0; i < 2; i++) {
	char *p, k[LOC_CTR_SIZE + 4];
	strncpy (k, key, LOC_CTR_SIZE);
	k[LOC_CTR_SIZE - 1] = '\0';
	if (i == 0)
	    strcat (k, "=");
	else
	    strcat (k, " =");
	if ((p = strstr (text, k)) != NULL &&
	    sscanf (p + strlen (k), "%f", v) == 1)
	    return (1);
    }
    return (0);
}
