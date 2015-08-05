/************************************************************************
 *									*
 *	Module:  hci_display_radial_data.c				*
 *									*
 *	Description:  This routine is used by the HCI to display a	*
 *		      beam of data. 					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:04 $
 * $Id: hci_display_product_radial_data.c,v 1.9 2009/02/27 22:26:04 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/************************************************************************
 *	Description: This function is used to display a single beam of	*
 *		     radial data from an RPG radial type product.	*
 *									*
 *	Input:  display       - pointer to Display info			*
 *		window        - Drawable				*
 *		gc            - graphics context			*
 *		x_offset      - X offset of radar to center pixel	*
 *		y_offset      - Y offset of radar to center scanline	*
 *		scale_x       - X scale factor (pixels/km)		*
 *		scale_y       - Y scale factor (scanlines/km)		*
 *		center_pixel  - center pixel				*
 *		center_scanl  - center scanline				*
 *		azimuth       - azimuth angle of radial (degrees)	*
 *		azimuth_interval - horizontal bean width (degrees)	*
 *		elements      - number of data elements in radial	*
 *		data          - pointer to radial data			*
 *		product_color - pointer to product color table		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_display_product_radial_data (
Display		*display,
Drawable	window,
GC		gc,
float		x_offset,
float		y_offset,
float		scale_x,
float		scale_y,
int		center_pixel,
int		center_scanl,
float		azimuth,
float		azimuth_interval,
int		elements,
unsigned char	*data,
int		*product_color
)
{
	int		i;
	float		azimuth1, azimuth2;
	int		old_color;
	float		sin1, sin2, cos1, cos2;
	float		range = 0.0;
	XPoint		x [12];
	static float	Sin [730];
	static float	Cos [730];
	int		Init_flag = 0;
	int		step;
	int		levels;
	int		pid;
	int		color;

/*	If this is first time, then initialize Sin/Cos tables.		*/

	if (!Init_flag) {

	    for (i=0;i<730;i++) {

		Sin [i] = sin ((double) (i/2.0+90.0)*HCI_DEG_TO_RAD);
		Cos [i] = cos ((double) (i/2.0-90.0)*HCI_DEG_TO_RAD);

	    }
	}

/*	Determine the step factor to apply to the color scale.		*/

	pid        = hci_product_code ();
	levels     = hci_product_data_levels ();

	if (levels <= 0) {

	    return;

	}

	if (pid != 34) {

	    step = PRODUCT_COLORS/levels;

	    if (PRODUCT_COLORS%levels)
		step = 1;

	} else {

	    step = 1;

	}

/*	Using the azimuth interval, calculate the left and right	*
 *	azimuth values for the beam.					*/

	azimuth1 = azimuth - azimuth_interval/2.0;

	if (azimuth1 < 0) {

	    azimuth1 = azimuth1+360.0;

	}

	azimuth2 = azimuth + azimuth_interval/2.0;

	sin1  = Sin [(int) (azimuth1*2)];
	sin2  = Sin [(int) (azimuth2*2)];
	cos1  = Cos [(int) (azimuth1*2)];
	cos2  = Cos [(int) (azimuth2*2)];

	old_color = -1;
	color     = 0;

/*	unpack the data at each gate along the beam.			*/

	for (i=0;i<elements;i++) {

	    range = i;

/*	    If a data value found at this point, determine if it is	*
 *	    different from the data value at the previous point.  If	*
 *	    so, we want to fill in the beam between the last point and	*
 *	    and the point were the old color was first set.  Use the	*
 *	    current point as the start point for the next polygon.	*/

	    if (data [i] > 0) {

		if (data [i] != old_color) {

		    if (old_color <= 0) {

			x[0].x = (range*cos1 + x_offset) * scale_x +
				  center_pixel;
			x[0].y = (range*sin1 + y_offset) * scale_y +
				  center_scanl;
			x[1].x = (range*cos2 + x_offset) * scale_x +
				  center_pixel;
			x[1].y = (range*sin2 + y_offset) * scale_y +
				  center_scanl;
			x[4].x = x[0].x;
			x[4].y = x[0].y;

		    } else {

			x[2].x = (range*cos2 + x_offset) * scale_x +
				  center_pixel;
			x[2].y = (range*sin2 + y_offset) * scale_y +
				  center_scanl;
			x[3].x = (range*cos1 + x_offset) * scale_x +
				  center_pixel;
			x[3].y = (range*sin1 + y_offset) * scale_y +
				  center_scanl;

			XSetForeground (display,
					gc,
					product_color [color]);

			XFillPolygon (display,
				      window,
				      gc,
				      x, 4,
				      Convex,
				      CoordModeOrigin);

			x[0].x = x[3].x;
			x[0].y = x[3].y;
			x[1].x = x[2].x;
			x[1].y = x[2].y;
			x[4].x = x[0].x;
			x[4].y = x[0].y;

		    }
		}

	    } else if (old_color > 0) {

/*	    Else, check to see if a data value existed at the previous	*
 *	    point.  If so, we want to fill in the beam between it and	*
 *	    the point where the color was first set.			*/

		x[2].x = (range*cos2 + x_offset) * scale_x +
			  center_pixel;
		x[2].y = (range*sin2 + y_offset) * scale_y +
			  center_scanl;
		x[3].x = (range*cos1 + x_offset) * scale_x +
			  center_pixel;
		x[3].y = (range*sin1 + y_offset) * scale_y +
			  center_scanl;

		XSetForeground (display,
				gc,
				product_color [color]);

		XFillPolygon (display,
			      window,
			      gc,
			      x, 4,
			      Convex,
			      CoordModeOrigin);

		x[0].x = x[3].x;
		x[0].y = x[3].y;
		x[1].x = x[2].x;
		x[1].y = x[2].y;
		x[4].x = x[0].x;
		x[4].y = x[0].y;

	    }

	    old_color = data [i];

	    color = old_color*step;

	    if (color > PRODUCT_COLORS)
		color = PRODUCT_COLORS;

	}

/*	If we finished checking all poonts in the beam, we need to	*
 *	see if we need to fill in the beam out to the last point.	*/

	if ((elements >  0) &&
	    (old_color > 0)) {

	    x[2].x = (range*cos2 + x_offset) * scale_x +
		      center_pixel;
	    x[2].y = (range*sin2 + y_offset) * scale_y +
		      center_scanl;
	    x[3].x = (range*cos1 + x_offset) * scale_x +
		      center_pixel;
	    x[3].y = (range*sin1 + y_offset) * scale_y +
		      center_scanl;

	    XSetForeground (display,
			    gc,
			    product_color [color]);

	    XFillPolygon (display,
		          window,
		          gc,
		          x, 4,
		          Convex,
		          CoordModeOrigin);

	}

}

/************************************************************************
 *	Description: This function clears an area by filling in a	*
 *		     rectangluar area (using 0,0 as the upper left	*
 *		     coordinate) by a user specified color.		*
 *									*
 *	Input:  display - pointer to Display info			*
 *		window  - Drawable					*
 *		gc      - graphics context				*
 *		width   - width (pixels) of region to clear		*
 *		height  - height (scanlines) of region to clear		*
 *		color   - color index to use to fill region		*
 *	Output:								*
 *	Return:								*
 ************************************************************************/

void
hci_basedata_display_clear (
Display		*display,
Drawable	window,
GC		gc,
int		width,
int		height,
int		color
)
{
	XSetForeground (display,
			gc,
			color);

	XFillRectangle (display,
			window,
			gc,
			0,
			0,
			width,
			height);

}
