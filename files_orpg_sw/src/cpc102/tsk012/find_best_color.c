/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:31:51 $
 * $Id: find_best_color.c,v 1.2 2005/06/02 19:31:51 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
*/


/************************************************************************
 *									*
 ************************************************************************/

/*	System include file definitions.				*/

#include <stdio.h>
#include <stdlib.h>

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

int
find_best_color (
Display		*display,
Colormap	cmap,
XColor		*color
)
{
	int	i;
	int	r, g, b;
	int	q_r, q_g, q_b;
	int	minind;
	int	mindist;
	unsigned long pixel_not_available;
static	XColor	query_color [256];
	int	public_colors;
	unsigned long pixels [256];
static	int	first_time = 1;

/*	Lets first get the colors that are defined in the specified	*
 *	colormap.  After we get them we need to determine which ones	*
 *	are available (i.e., read only).				*/

	pixel_not_available = 0xffffffff;
	public_colors = 256;

	if (first_time) {

	    first_time    = 0;
	    public_colors = 0;

	    for (i=0;i<256;i++)
		query_color [i].pixel = i;

	    XQueryColors (display, cmap, query_color, 256);

	    for (i=0;i<256;i++) {
		if (XAllocColor (display, cmap, &query_color[i]) == 0) {
		    query_color[i].pixel = pixel_not_available;
		} else {
		    pixels [public_colors] = query_color[i].pixel;
		    public_colors++;
		}
	    }

	    XFreeColors (display, cmap, pixels, public_colors,
		 (unsigned long) 0);
	}

	if (XAllocColor (display, cmap, color) != 0) {

	    return (0);

	}

/*	Lets rank the color guns by order of decreasing intensity.	*/

	if (color->red > color->green) {
	    if (color->red > color->blue) {
		r = 1;
		if (color->green > color->blue) {
		    g = 2;
		    b = 3;
		} else {
		    b = 2;
		    g = 3;
		}
	    } else {
		b = 1;
		r = 2;
		g = 3;
	    }
	} else {
	    if (color->green > color->blue) {
		g = 1;
		if (color->red > color->blue) {
		    r = 2;
		    b = 3;
		} else {
		    b = 2;
		    r = 3;
		}
	    } else {
		b = 1;
		g = 2;
		r = 3;
	    }
	}

	mindist = 0xfffffff;
	minind  = -1;

	for (i=0;i<256;i++) {

	    int d, dist;

	    if (query_color[i].pixel == pixel_not_available)
		continue;

/*	    First, we want to make sure the intensities of the color	*
 *	    guns are in the same order as the original color.		*/

	    if (query_color[i].red > query_color[i].green) {
		if (query_color[i].red > query_color[i].blue) {
		    q_r = 1;
		    if (query_color[i].green > query_color[i].blue) {
			q_g = 2;
			q_b = 3;
		    } else {
			q_b = 2;
			q_g = 3;
		    }
		} else {
		    q_b = 1;
		    q_r = 2;
		    q_g = 3;
		}
	    } else {
		if (query_color[i].green > query_color[i].blue) {
		    q_g = 1;
		    if (query_color[i].red > query_color[i].blue) {
			q_r = 2;
			q_b = 3;
		    } else {
			q_b = 2;
			q_r = 3;
		    }
		} else {
		    q_b = 1;
		    q_g = 2;
		    q_r = 3;
		}
	    }

	    if ((r == q_r) && (g == q_g) && (b == q_b)) {

		d = (color->red - query_color[i].red) >> 8;
		dist = d*d;
		d = (color->green - query_color[i].green) >> 8;
		dist += d*d;
		d = (color->blue - query_color[i].blue) >> 8;
		dist += d*d;

		if (dist < mindist) {

		    mindist = dist;
		    minind  = i;

	        }

	    } else {

		continue;

	    }
	}

	if (minind < 0) {

	    fprintf (stderr,"could not find closest color\n");
	    mindist = 0xfffffff;

	    for (i=0;i<256;i++) {

		int d, dist;

		if (query_color[i].pixel == pixel_not_available)
		    continue;

		d = (color->red - query_color[i].red) >> 8;
		dist = d*d;
		d = (color->green - query_color[i].green) >> 8;
		dist += d*d;
		d = (color->blue - query_color[i].blue) >> 8;
		dist += d*d;

		if (dist < mindist) {

		    mindist = dist;
		    minind  = i;

	        }
	    }
	}

	color->red   = query_color[minind].red;
	color->green = query_color[minind].green;
	color->blue  = query_color[minind].blue;
	color->pixel = query_color[minind].pixel;
	color->flags = query_color[minind].flags;

	if (XAllocColor (display, cmap, color) == 0) {

	    fprintf (stderr,"closest color found but could not be allocated\n");
	    return (-1);

	}

	return (0);
}
