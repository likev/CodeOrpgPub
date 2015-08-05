/************************************************************************
 *	Module: display_tvs_data.c					*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to dispay a Tornado (TVS) product in	*
 *		     the display window.				*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:33 $
 * $Id: display_tvs_data.c,v 1.3 2001/05/22 18:13:33 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	System include file definitions.				*/

#include <math.h>

#define	COLORS	16	/* number of product colors */

/*	Various X and windows properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	XColor		color []; /* product color data */
extern	float		scale_x; /* pixel/km scale factor */
extern	float		scale_y; /* scanline/km scale factor */
extern	int		center_pixel; /* pixel coordinate at window center */
extern	int		center_scanl; /* scanline coordinate at window center */
extern	Dimension	width; /* width (pixels) of window */
extern	Dimension	height; /* height (scanlines) of window */
extern	int		Colors; /* number of product colors */
extern	int		product_start_scanl; /* start scanline for product
						data */

/************************************************************************
 *	Description: This function is used to display TVS product data	*
 *		     in the XPDT display window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_tvs_data ()
{
	int	i;
	int	j;
	int	pixel;
	int	scanl;
	int	size;
	XPoint	X [5];

/*	Check first to see if tvs structure defined.			*
 *	If not, get the heck out of here.				*/

	if (tvs_image == (struct tvs *) NULL) {

	    printf ("TVS type detected but no data.\n");
	    return;

	}

	fprintf (stderr,"The number of TVS is %d\n",
		tvs_image->number_tvs + 1);

	XSetForeground (display,
			gc,
			color [COLORS+2].pixel);

/*	The ICD says that tvs's are inverted isosceles triangles	*/

	for (i=0;i<=tvs_image->number_tvs;i++) {

	    pixel = tvs_image->tvs_xpos [i]*scale_x + center_pixel;
	    scanl = tvs_image->tvs_ypos [i]*scale_y + center_scanl;

	    XSetForeground (display,
			    gc,
			    color [COLORS].pixel);

	    fprintf (stderr,"TVS ID: %s\n", tvs_image->equiv.storm_id [i]);

	    XDrawString (display,
			 pixmap,
			 gc,
			 pixel+-4,
			 scanl+10,
			 tvs_image->equiv.storm_id [i],
			 2);

	    XSetForeground (display,
			    gc,
			    color [COLORS+2].pixel);

	    X [0].x = pixel;
	    X [0].y = scanl;
	    X [1].x = pixel-6;
	    X [1].y = scanl-10;
	    X [2].x = pixel+6;
	    X [2].y = scanl-10;
	    X [3].x = pixel;
	    X [3].y = scanl;

	    XFillPolygon (display,
		 	  pixmap,
			  gc,
			  X, 4,
			  Convex,
			  CoordModeOrigin);

	}

}
