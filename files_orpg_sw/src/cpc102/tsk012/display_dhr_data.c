/************************************************************************
 *	Module: display_dhr_data.c					*
 *	Description: This module displays a dhr product in the display	*
 *		     window used by the NEXRAD Product Display Tool	*
 *		     (xpdt).						*
 ************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 19:00:42 $
 * $Id: display_dhr_data.c,v 1.6 2014/03/18 19:00:42 jeffs Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*	System include file definitions.				*/

#include <math.h>

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

#define	DEGRAD	(3.14159265/180.0)	/* degrees to radians conversion */
#define	COLORS	64			/* number of display colors */

/*	Various X and window properties.	*/

extern	Display	*display;
extern	Window	window;
extern	Pixmap	pixmap;
extern	GC	gc;
extern	XColor	dhr_color [];
extern	float	scale_x; /* pixel/km scale factor */
extern	float	scale_y; /* scanline/km scale factor */
extern	int	center_pixel; /* pixel coordinate at window center */
extern	int	center_scanl; /* scanline coordinate at window center */
extern	Dimension	width;	/* width (pixels) of window */
extern	Dimension	height;	/* height (scanlines) of window */
extern	float	value_min; /* minimum displayable data value */
extern	float	value_max; /* maximum displayable data value */
extern	int	product_start_scanl; /* start scanline for product data */
extern	int	pixel_offset;	/* pixel offset for start of product data */
extern	int	scanl_offset;	/* scanline offset for start of product data */

float	Range [2000]; /* bin range lookup table */

/************************************************************************
 *	Description: This function displays a DHR (Digital Hybrid	*
 *		     Reflectivity) product in the XPDT display window.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void display_dhr_data (
)
{
	int	beam;
	int	i;
	float	azimuth1, azimuth2;
	int	old_Color;
	int	color;
	int	Color;
	float	value;
	XPoint	X [12];
	float	sin1, sin2;
	float	cos1, cos2;
	int	color_step;
	XRectangle	clip_rectangle;

/*	Set the clip window.					*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = product_start_scanl;
	clip_rectangle.width   = 3*width/4;
	clip_rectangle.height  = 3*width/4;

	XSetClipRectangles (display,
			    gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	color_step = 1;

	for (i=0;i<dhr_image->data_elements;i++) {

	    Range [i] = i*attribute->x_reso;

	}

/*	For each beam in the product, display it using the product	*
 *	value as a unique color in the display.				*/

	for (beam=0;beam<dhr_image->number_of_radials;beam++) {

	    azimuth1 = dhr_image->azimuth_angle [beam] - 0.5;
	    azimuth2 = azimuth1 + dhr_image->azimuth_width [beam] + 0.5;

	    sin1 = sin ((double) (azimuth1+90)*DEGRAD);
	    sin2 = sin ((double) (azimuth2+90)*DEGRAD);
	    cos1 = cos ((double) (azimuth1-90)*DEGRAD);
	    cos2 = cos ((double) (azimuth2-90)*DEGRAD);

	    old_Color = -1;

/*	unpack the data at each gate along the beam.  If there is data	*
 *	above the lower threshold, display it.  To reduce the number of	*
 *	XPolygonFill operations, only paint gate(s) when a color change	*
 *	occurs.								*/

	    for (i=0;i<dhr_image->data_elements;i++) { 

		color = dhr_image->radial_data[beam][i];

		if (color == 0) {

		    Color = 0;

		} else if (color == 1) {	/* RF */

		    Color = 64;

		} else {

		    Color = color/4 + 1;

		}

		if (Color > 0) {

/*		The value at this gate is not background so process it	*/

		    if (Color != old_Color) {

/*		    If the current value is different from the last	*
 *		    gate processed, either the last value was back-	*
 *		    ground or a real value.				*/

			if (old_Color <= 0) {

/*			If the last gate was background, then find the	*
 *			coordinates of the first two points making up	*
 *			the sector.					*/

			    X[0].x = Range[i] * cos1 * scale_x +
				     center_pixel + pixel_offset;
			    X[0].y = Range[i] * sin1 * scale_y +
				     center_scanl + scanl_offset;
			    X[1].x = Range[i] * cos2 * scale_x +
				     center_pixel + pixel_offset;
			    X[1].y = Range[i] * sin2 * scale_y +
				     center_scanl + scanl_offset;
			    X[4].x = X[0].x;
			    X[4].y = X[0].y;

			} else {

/*			The last point was not background so find the	*
 *			coordinates of the last two points defining the	*
 *			sector and display it.				*/

			    X[2].x = Range[i] * cos2 * scale_x +
				     center_pixel + pixel_offset;
			    X[2].y = Range[i] * sin2 * scale_y +
				     center_scanl + scanl_offset;
			    X[3].x = Range[i] * cos1 * scale_x +
				     center_pixel + pixel_offset;
			    X[3].y = Range[i] * sin1 * scale_y +
				     center_scanl + scanl_offset;

			    XSetForeground (display,
					    gc,
					    dhr_color [old_Color].pixel);
			    XFillPolygon (display,
				          pixmap,
				          gc,
				          X, 4,
				          Convex,
				          CoordModeOrigin);

			    X[0].x = X[3].x;
			    X[0].y = X[3].y;
			    X[1].x = X[2].x;
			    X[1].y = X[2].y;
			    X[4].x = X[0].x;
			    X[4].y = X[0].y;

			}

	 	    }

		} else if (old_Color > 0) {

/*		The last gate had a real value so determine the last	*
 *		pair of points defining the sector and display it.	*/

		    X[2].x = Range[i] * cos2 * scale_x +
		  	     center_pixel + pixel_offset;
		    X[2].y = Range[i] * sin2 * scale_y +
			     center_scanl + scanl_offset;
		    X[3].x = Range[i] * cos1 * scale_x +
			     center_pixel + pixel_offset;
		    X[3].y = Range[i] * sin1 * scale_y +
			     center_scanl + scanl_offset;

		    XSetForeground (display,
				    gc,
				    dhr_color [old_Color].pixel);
		    XFillPolygon (display,
			          pixmap,
			          gc,
			          X, 4,
			          Convex,
			          CoordModeOrigin);

		    X[0].x = X[3].x;
		    X[0].y = X[3].y;
		    X[1].x = X[2].x;
		    X[1].y = X[2].y;
		    X[4].x = X[0].x;
		    X[4].y = X[0].y;

		}

		old_Color = Color;

	    }

	    if (Color     > 0) {

/*	    There are no more gates in the beam but check to see if	*
 *	    the previous gate had data which needs to be displayed.  If	*
 *	    so, find the last two sector coordinates and display it.	*/

		X[2].x = Range [i-1] * cos2 * scale_x +
			 center_pixel + pixel_offset;
		X[2].y = Range [i-1] * sin2 * scale_y +
			 center_scanl + pixel_offset;
		X[3].x = Range [i-1] * cos1 * scale_x +
			 center_pixel + pixel_offset;
		X[3].y = Range [i-1] * sin1 * scale_y +
			 center_scanl + pixel_offset;

		XSetForeground (display,
				gc,
				dhr_color [old_Color].pixel);
		XFillPolygon (display,
			      pixmap,
			      gc,
			      X, 4,
			      Convex,
			      CoordModeOrigin);

	    }

	}

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = width;
	clip_rectangle.height  = height;

	XSetClipRectangles (display,
			    gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

}
