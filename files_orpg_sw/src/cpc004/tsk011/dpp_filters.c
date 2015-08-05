
/******************************************************************

    This dual-pol radar data preprocesing program module contains
    basic data filtes.
	
******************************************************************/

/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2012/03/12 12:56:06 $ */
/* $Id: dpp_filters.c,v 1.8 2012/03/12 12:56:06 ccalvert Exp $ */
/* $Revision:  */
/* $State: */

#include <stdio.h>
#include <rpgc.h>
#include "dpprep.h"


/**************************************************************************

    Performs LLS smoothing on array "in" of "n" elements. The window size 
    is "w". The output is put in "out".

***************************************************************************/

void DPPT_lls_filter (int n, int w, DPP_d_t *in, DPP_d_t *out) {
    int i, hw;

    if (w < 1 || (w % 2) != 1) {
	LE_send_msg (GL_ERROR, "Unexpected lls filter window size (%d)", w);
	return;
    }
    hw = w / 2;
    for (i = 0; i < n; i++) {
	int j, cnt;
	DPP_d_t sx, sy, sxx, sxy;

	cnt = 0;
	sx = sy = sxx = sxy = C0;
	for (j = -hw; j <= hw; j++) {
	    int k = i + j;
	    if (k >= 0 && k < n && in[k] > DPP_NO_DATA) {
		DPP_d_t x, y;
		x = (DPP_d_t)j;
		y = in[k];
		sx += x;
		sy += y;
		sxy += y * x;
		sxx += x * x;
		cnt++;
	    }
	}
	if (cnt < hw)
	    out[i] = DPP_NO_DATA;
	else if (cnt == 1)
	    out[i] = sy;
	else {
	    DPP_d_t nd = (DPP_d_t)cnt;
	    out[i] = (sy - sx * ((nd * sxy - sx * sy) / (nd * sxx - sx * sx))) / nd;
	} 
    }
}

/**************************************************************************

    Performs moving average filtering on array "in" of "n" elements. The
    window size is "w". The output is put in "out".

***************************************************************************/

void DPPT_average_filter (int n, int w, DPP_d_t *in, DPP_d_t *out) {
    DPP_d_t one_over[MAX_WIN + 1];
    int i, hw, cnt;
    DPP_d_t *p1, *p2, *end, sum;

    if (w > MAX_WIN || w < 1 || (w % 2) != 1) {
	LE_send_msg (GL_ERROR, "Unexpected average filter window size (%d)", w);
	return;
    }
    if (w == 1) {
	memcpy (out, in, n * sizeof (DPP_d_t));
	return;
    }

    for (i = 1; i <= w; i++)
	one_over[i] = C1 / (DPP_d_t)i;
    hw = w / 2;
    p1 = in - hw - 1;
    p2 = in + hw;
    end = in + n - 1;
    cnt = 0;
    sum = C0;
    for (i = 0; i < n; i++) {

	/* code for verification */
/*
	{
	    DPP_d_t s;
	    int c, j;
	    c = 0;
	    s = C0;
	    for (j = -hw; j <= hw; j++) {
		int k = i + j;
		if (k >= 0 && k < n && in[k] > DPP_NO_DATA) {
		    s += in[k];
		    c++;
		}
	    }
	    if (c > 0)
		s = s / (DPP_d_t)c;
	    else
		s = DPP_NO_DATA;
	    printf ("Verify: i = %d average = %f\n", i, s);
	}
*/

	if ((i % 128) == 0) {	/* reinitialize sum and cnt for precision */
	    int j, k;
	    sum = C0;
	    cnt = 0;
	    for (j = -hw - 1; j < hw; j++) {
		k = i + j;
		if (k >= 0 && k < n && in[k] > DPP_NO_DATA) {
		    sum += in[k];
		    cnt++;
		}
	    }
	}

	if (p1 >= in && *p1 > DPP_NO_DATA) {
	    sum -= *p1;
	    cnt--;
	}
	if (p2 <= end && *p2 > DPP_NO_DATA) {
	    sum += *p2;
	    cnt++;
	}
	if (cnt > 0)
	    out[i] = sum * one_over[cnt];
	else
	    out[i] = DPP_NO_DATA;
/*	printf ("                       : %d %f\n", i, out[i]); */
	p1++;
	p2++;
    }
}

/**************************************************************************

    Performs moving standard deviation filtering on array "in" and
    smoothed data "in_smd" of "n" elements. The window size is "w". Data
    deviated from in_smd more than "std_thresholds" are not used. The output
    is put in "out".

***************************************************************************/

void DPPT_std_filter (int n, int w, DPP_d_t std_thresholds, 
			DPP_d_t *in, DPP_d_t *in_smd, DPP_d_t *out) {
    DPP_d_t one_over[MAX_WIN + 1];
    int i, hw, cnt;
    DPP_d_t sum, sq, *p1, *p2, *p3, *p4, *end;

    if (w > MAX_WIN || w < 1 || (w % 2) != 1) {
	LE_send_msg (GL_ERROR, "Unexpected std filter window size (%d)", w);
	return;
    }

    if (w == 1) {
	for (i = 0; i < n; i++)
	    out[i] = C0;
	return;
    }

    for (i = 1; i <= w; i++)
	one_over[i] = C1 / (DPP_d_t)i;
    hw = w / 2;
    p1 = in - hw - 1;
    p2 = in + hw;
    p3 = in_smd - hw - 1;
    p4 = in_smd + hw;
    end = in + n - 1;
    cnt = 0;
    sum = sq = C0;
    for (i = 0; i < n; i++) {
	DPP_d_t d, v1, v2;

	/* For verification of STD calculation */
/*
	{
	    DPP_d_t s, d, v1, v2, av;
	    int j, c;

	    s = C0;
	    c = 0;
	    for (j = -hw; j <= hw; j++) {
		int k = i + j;
		if (k >= 0 && k < n) {
		    v1 = in[k];
		    v2 = in_smd[k];
		    if (v1 > DPP_NO_DATA && v2 > DPP_NO_DATA) {
			d = v1 - v2;
			s += d;
			c++;
		    }
		}
	    }
	    av = s / c;
	    s = C0;
	    for (j = -hw; j <= hw; j++) {
		int k = i + j;
		if (k >= 0 && k < n) {
		    v1 = in[k];
		    v2 = in_smd[k];
		    if (v1 > DPP_NO_DATA && v2 > DPP_NO_DATA) {
			d = v1 - v2 - av;
			d = d * d;
			s += d;
		    }
		}
	    }
	    if (c > 1)
		s = SQRT (s / (c - 1));
	    else
		s = DPP_NO_DATA;
	    printf ("Verify: i = %d, std = %f\n", i, s);
	}
*/

	if ((i % 64) == 0) {	/* reinitialize sum and sq for precision */
	    int j, k;
	    sum = sq = C0;
	    cnt = 0;
	    for (j = -hw - 1; j < hw; j++) {
		k = i + j;
		if (k >= 0 && k < n) {
		    v1 = in[k];
		    v2 = in_smd[k];
		    if (v1 > DPP_NO_DATA && v2 > DPP_NO_DATA && 
			v1 - v2 <= std_thresholds && 
			v2 - v1 <= std_thresholds) {
			d = v1 - v2;
			sum += d;
			sq += d * d;
			cnt++;
		    }
		}
	    }
	}

	if (p1 >= in && *p1 > DPP_NO_DATA && *p3 > DPP_NO_DATA) {
	    d = *p1 - *p3;
	    if (d <= std_thresholds && d >= -std_thresholds) {
		sum -= d;
		sq -= d * d;
		cnt--;
	    }
	}
	if (p2 <= end && *p2 > DPP_NO_DATA && *p4 > DPP_NO_DATA) {
	    d = *p2 - *p4;
	    if (d <= std_thresholds && d >= -std_thresholds) {
		sum += d;
		sq += d * d;
		cnt++;
	    }
	}

	if (cnt < hw || cnt < 2)
	    out[i] = DPP_NO_DATA;
	else {
	    
	    d = sum * one_over[cnt];
/*	    d = sq * one_over[cnt] - d * d;	*/	/* Biased STD */
	    d = (sq - d * d * (DPP_d_t)cnt) * one_over[cnt - 1];
							/* Non-biased STD */
	    if (d < C0)
		d = C0;
	    out[i] = SQRT (d);
	}
	p1++;
	p2++;
	p3++;
	p4++;
    }
}

/**************************************************************************

    Performs moving median filtering on array "in" of "n" elements. The
    window size is "w". The output is put in "out".

***************************************************************************/

void DPPT_median_filter (int n, int w, DPP_d_t *in, DPP_d_t *out) {
    int i, hw, b1, b2;

    if (w > MAX_WIN || w <= 0) {
	LE_send_msg (GL_ERROR, "Unexpected median filter window size (%d)", w);
	return;
    }
    hw = w / 2;
    b1 = hw;
    b2 = n - hw;
    for (i = 0; i < n; i++) {
	int cnt, j;
	DPP_d_t buf[MAX_WIN];

	cnt = 0;
	if (i > b1 && i < b2) {		/* efficient in the middle of array */
	    for (j = -hw; j <= hw; j++) {
		DPP_d_t v = in[i + j];
		if (v > DPP_NO_DATA) {
		    buf[cnt] = v;
		    cnt++;
		}
	    }
	}
	else {				/* near the boundaries of array */
	    for (j = -hw; j <= hw; j++) {
		int k = i + j;
		if (k >= 0 && k < n) {
		    if (in[k] > DPP_NO_DATA) {
			buf[cnt] = in[k];
			cnt++;
		    }
		}
	    }
	}
	if (cnt > 0)
/*	if (cnt > 1)*/	/* John */
	    out[i] = DPPT_med_filter (buf, cnt);
	else
	    out[i] = DPP_NO_DATA;
    }
}

/**************************************************************************

    Performs median filtering on array "arr" of size "n". The medial
    value is returned. If n <= 0, 0 is returned. This routine is adapted
    from scit_filter_quickselect.c (Yukuan Song) and based on the
    algorithm described in "numerical recipies in C", Second edition,
    Cambridge University Press, 1992, section 8.5, ISBN 0-521-43108-5.
    ((low + high + 1) / 2) selects the upper element in case of n is
    even while ((low + high) / 2) selects the lower.

***************************************************************************/

#define ELEM_SWAP(a,b) {DPP_d_t t = (a); (a) = (b); (b) = t;}

DPP_d_t DPPT_med_filter (DPP_d_t *arr, int n) {
    int low, high, median;
    int middle, ll, hh;

    if (n <= 0)
	return (C0);
    low = 0;
    high = n - 1;
    median = (low + high + 1) / 2;
    for (;;) {

        if (high <= low)		/* One element only */
            return arr[median] ;

        if (high == low + 1) {		/* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP (arr[low], arr[high]) ;
            return arr[median];
        }

	/* Find median of low, middle and high items; swap into position low */
	middle = (low + high + 1) / 2;
	if (arr[middle] > arr[high])
	    ELEM_SWAP (arr[middle], arr[high]);
	if (arr[low] > arr[high])
	    ELEM_SWAP (arr[low], arr[high]);
	if (arr[middle] > arr[low])
	    ELEM_SWAP (arr[middle], arr[low]);
    
	/* Swap low item (now in position middle) into position (low + 1) */
	ELEM_SWAP (arr[middle], arr[low+1]);
    
	/* Nibble from each end towards middle, swapping items when stuck */
	ll = low + 1;
	hh = high;
	for (;;) {

	    do ll++;
	    while (arr[low] > arr[ll]);
	    do hh--;
	    while (arr[hh] > arr[low]);
    
	    if (hh < ll)
		break;
    
	    ELEM_SWAP (arr[ll], arr[hh]);
	}
    
	/* Swap middle item (in position low) back into correct position */
	ELEM_SWAP (arr[low], arr[hh]);
    
	/* Re-set active partition */
	if (hh <= median)
	    low = ll;
	if (hh >= median)
	    high = hh - 1;
    }
}


