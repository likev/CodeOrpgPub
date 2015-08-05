
/******************************************************************

    The sync_to_adapt module that updates the shared color tables.

******************************************************************/

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <infr.h>
#include "sta_def.h"

static int Gen_v (int n_thrs, char *thr, double res, int levels, 
				unsigned short *ct, unsigned short *tt);
static int Gen_z (int n_thrs, char *thr, int levels, 
				unsigned short *ct, unsigned short *tt);
static int Gen_precip (int n_thrs, char *thr, double res, 
				unsigned short *ct, unsigned short *tt);
static int Gen_vad (int n_thrs, char *thr, 
				unsigned short *ct, unsigned short *tt);
static double Rounding (double data, double th, double e);
static int Parse_thresholds (char *thr, double scale, int st, 
						int num, int *sh);


/***********************************************************************

    Updates color table "ct" (256 entries) and threshold table "tt" 
    (16 entries) using the new threshold "thr". "ind" is the color table
    index (started with 1). "thr" is a set of "n_thrs" null-terminated 
    strings as defined in prod_params.dea. Returns 0 on success of a 
    negative error code.

***********************************************************************/

int STA_update_color_table (int ind, char *thr, int n_thrs,
				unsigned short *ct, unsigned short *tt) {
    int ret;

    ret = 0;
    switch (ind) {

	case 23:
	    ret = Gen_precip (n_thrs, thr, 10., ct, tt);
	    break;
	case 22:
	    ret = Gen_precip (n_thrs, thr, 20., ct, tt);
	    break;
	case 26:
	    ret = Gen_v (n_thrs, thr, .5, 16, ct, tt);
	    break;
	case 27:
	    ret = Gen_v (n_thrs, thr, .5, 8, ct, tt);
	    break;
	case 28:
	    ret = Gen_v (n_thrs, thr, 1., 16, ct, tt);
	    break;
	case 29:
	    ret = Gen_v (n_thrs, thr, 1., 8, ct, tt);
	    break;
	case 4:
	    ret = Gen_v (n_thrs, thr, .5, 16, ct, tt);
	    break;
	case 5:
	    ret = Gen_v (n_thrs, thr, .5, 8, ct, tt);
	    break;
	case 6:
	    ret = Gen_v (n_thrs, thr, 1., 16, ct, tt);
	    break;
	case 7:
	    ret = Gen_v (n_thrs, thr, 1., 8, ct, tt);
	    break;
	case 2:
	    ret = Gen_z (n_thrs, thr, 16, ct, tt);
	    break;
	case 1:
	    ret = Gen_z (n_thrs, thr, 16, ct, tt);
	    break;
	case 3:
	    ret = Gen_z (n_thrs, thr, 8, ct, tt);
	    break;
	case 25:
	    ret = Gen_vad (n_thrs, thr, ct, tt);
	    break;
	case 20:
	    ret = Gen_z (n_thrs, thr, 8, ct, tt);
	    break;
    }
    return (ret);
}

/*********************************************************************

    Updates velocity color table "ct" and threshold table "tt"
    according to the new threshold "thr" (list of "n_thrs" null-terminated
    strings). "res" is the data resolusion and "levels" is the number
    of data levels. Returns 0 on success or -1 on failure.

*********************************************************************/

static int Gen_v (int n_thrs, char *thr, double res, int levels,
			unsigned short *ct, unsigned short *tt) {
/*    static int sh[14] = {-64, -50, -36, -26, -20, -10, -1, 
					0, 10, 20, 26, 36, 50, 64};
    static int sh[6] = {-10, -5, -1, 0, 5, 10}; */
    int sh[16];
    int i, ind;
    double re;

    i = 0;
    if (n_thrs != levels || 
	(i = Parse_thresholds (thr, 1., 1, levels - 2, sh)) != levels - 2) {
	LE_send_msg (0, 
	"Insufficient number of velocity threshold values (%d %d)", n_thrs, i);
	return (-1);
    }

    for (i = 0; i < levels - 2; i++)
	tt[i + 1] = (tt[i + 1] & 0xff00) | ((sh[i] < 0? -sh[i]: sh[i]) & 0xff);

    if (levels == 16)
	re = .04;		/* error used for rounding */
    else
	re = .03;
    ct[0] = 0;
    ct[1] = levels - 1;
    ind = 0;
    for (i = 2; i < 256; i++) {
	double d, s;

	if (res == .5)
	    d = ((double)i * .5 - 64.5) * 1.943841;	/* m/s to NM */
	else
	    d = ((double)i - 129.) * 1.943841;
	if (ind > levels - 3)
	    s = (double)sh[levels - 3];
	else
	    s = (double)sh[ind];
	d = Rounding (d, s, re);
	if (s < 0) {
	    if (d > s)
		ind++;
	    ct[i] = ind + 1;
	}
	else {
	    if (d >= s)
		ind++;
	    if (ind <= levels - 2)
	        ct[i] = ind;
	    else
	        ct[i] = levels - 2;
	}
    }
    return (0);
}

/************************************************************************

    Updates DBZ color table "ct" and threshold table "tt" according to 
    the new threshold "thr" (list of "n_thrs" null-terminated strings). 
    "levels" is the number of data levels. Returns 0 on success or -1 on 
    failure.

************************************************************************/

static int Gen_z (int n_thrs, char *thr, int levels,
			unsigned short *ct, unsigned short *tt) {
/*    static int sh[15] = {-28, -24, -20, -16, -12, -8, -4, 
					0, 4, 8, 12, 16, 20, 24, 28}; */
/*    static int sh[15] = {5, 10, 15, 20, 25, 30, 35, 
					40, 45, 50, 55, 60, 65, 70, 75}; */
/*    static int sh[15] = {5, 18, 30, 41, 46, 50, 57}; */
/*    static int sh[15] = {15, 30, 40, 45, 50, 55, 200};  RCM */
    int sh[16];
    int i, ind, nvs;

    nvs = 0;
    if (n_thrs != levels || 
	(nvs = Parse_thresholds (thr, 1., 1, levels - 1, sh)) < levels - 2) {
	LE_send_msg (0, 
	"Insufficient number of DBZ threshold values (%d %d)", n_thrs, nvs);
	return (-1);
    }
    if (levels == 8 && nvs == 6)
	sh[6] = 200;			/* for RCM */

    for (i = 0; i < nvs; i++)
	tt[i + 1] = (tt[i + 1] & 0xff00) | ((sh[i] < 0? -sh[i]: sh[i]) & 0xff);

    ind = 0;
    for (i = 0; i < 256; i++) {
	double d, s;

	d = (double)i * .5 - 33.;
	if (ind > levels - 2)
	    s = (double)sh[levels - 2];
	else
	    s = (double)sh[ind];
	if (d >= s)
	    ind++;
	if (ind <= levels - 1)
	    ct[i] = ind;
	else
	    ct[i] = levels - 1;
    }
    return (0);
}

/************************************************************************

    Updates precip color table "ct" and threshold table "tt" according to 
    the new threshold "thr" (list of "n_thrs" null-terminated strings). 
    "res" is the threshold scaling factor. Returns 0 on success or -1 on 
    failure.

************************************************************************/

static int Gen_precip (int n_thrs, char *thr, double res,
			unsigned short *ct, unsigned short *tt) {
/*    static double sh[15] = {0., .1, .25, .5, .75, 1., 1.25, 1.5, 1.75,
				2., 2.5, 3., 4., 6., 8. };  OHP/THP */
/*    static double sh[15] = {0., .3, .6, 1., 1.5, 2., 2.5, 3., 4.,
				5., 6., 8., 10., 12., 15.}; STP */
    int sh[16];
    int levels, i, ind;

    levels = 16;
    i = 0;
    if (n_thrs != levels || 
	(i = Parse_thresholds (thr, res, 1, levels - 1, sh)) != levels - 1) {
	LE_send_msg (0, 
	"Insufficient number of precip threshold values (%d %d)", n_thrs, i);
	return (-1);
    }

    for (i = 0; i < levels - 1; i++)
	tt[i + 1] = (tt[i + 1] & 0xff00) | ((sh[i] < 0? -sh[i]: sh[i]) & 0xff);

    ct[0] = 0;
    ind = 0;
    for (i = 1; i < 256; i++) {
	double d, s;

	d = (double)i; /*.05 for OHP/THP; .1; for STP */
	if (ind > levels - 2)
	    s = (double)sh[levels - 2];
	else
	    s = (double)sh[ind];
	if (d > s)
	    ind++;
	if (ind <= levels - 1)
	    ct[i] = ind;
	else
	    ct[i] = levels - 1;
    }
    return (0);
}

/************************************************************************

    Updates VAD color table "ct" and threshold table "tt" according to 
    the new threshold "thr" (list of "n_thrs" null-terminated strings). 
    Returns 0 on success or -1 on failure.

************************************************************************/

static int Gen_vad (int n_thrs, char *thr, 
			unsigned short *ct, unsigned short *tt) {
/*    static int sh[15] = {5, 5, 18, 30, 41, 46, 50}; */
    int sh[16];
    int levels, i, ind;

    levels = 8;
    i = 0;
    if (n_thrs != levels - 1 || 
	(i = Parse_thresholds (thr, 1., 0, levels - 1, sh)) != levels - 1) {
	LE_send_msg (0, 
	"Insufficient number of VAD threshold values (%d %d)", n_thrs, i);
	return (-1);
    }

    for (i = 0; i < levels - 1; i++)
	tt[i + 1] = (tt[i + 1] & 0xff00) | ((sh[i] < 0? -sh[i]: sh[i]) & 0xff);

    ind = 1;
    for (i = 0; i < 256; i++) {
	double d, s;

	d = (double)i * .5 - 33.;
	if (ind > levels - 2)
	    s = (double)sh[levels - 2];
	else
	    s = (double)sh[ind];
	if (d >= s)
	    ind++;
	if (ind <= levels - 1)
	    ct[i] = ind;
	else
	    ct[i] = levels - 1;
    }
    return (0);
}

/************************************************************************

    Compares "data" with threshold "th". Returns "th" if "data" is 
    sufficiently close to "th" in terms of error "e", or "data" otherwise.

************************************************************************/

static double Rounding (double data, double th, double e) {
    double diff;

    if (e == 0.)
	return (data);
    diff = data - th;
    if (diff < 0.)
	diff = -diff;
    if (diff < e)
	return (th);
    return (data);
}

/*************************************************************************

    Parses thresholds, in array of null-treminated strings format, "thr"
    and returns them in "sh". "scale" is the scaling factor. The first 
    "st" strings in "thr" are discarded. At most "num" values are parsed.
    Returns the number of values parsed.

*************************************************************************/

static int Parse_thresholds (char *thr, double scale, int st, 
						int num, int *sh) {
    char *p;
    int i, cnt;

    p = thr;
    cnt = 0;
    for (i = 0; i < st + num; i++) {
	if (i >= st) {
	    double d;
	    int n;
	    if (sscanf (p, "%lf", &d) != 1)
		break;
	    cnt++;
	    d = d * scale;
	    n = (int)(fabs (d) + .5);
	    if (d < 0)
		n = -n;
	    sh[i - st] = n;
	}
	p += strlen (p) + 1;
    }
    return (cnt);
}






