/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:04 $
 * $Id: hci_display_radial_product.c,v 1.12 2009/02/27 22:26:04 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module: hci_display_radial_data.c				*
 *	Description: This module is used to display a polar product in	*
 *		     the display window used by the NEXRAD Product	*
 *		     Display Tool (xpdt).				*
 ************************************************************************/

/*	Local include file definitons.c					*/

#include <hci.h>

float	Range [2000];	/* Range data lookup table */

/************************************************************************
 *	Description: This function displays a radial product in the	*
 *		     user specified window using the input parameters	*
 *		     to control positionand magnification.		*
 *									*
 *	Input:  display   - pointer to Display data			*
 *		window    - Drawable					*
 *		gc        - graphics context				*
 *		pixel     - left pixel of draw area			*
 *		scanl     - top scanline of draw area			*
 *		width     - width (pixels) of draw area			*
 *		height    - height (scanlines) of draw area		*
 *		max_range - range (km) of last data element to display	*
 *		x_offset  - X offset (km) of radar to draw area center  *
 *		y_offset  - Y offset (km) of radar to draw area center  *
 *		zoom      - Magnification factor			*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
hci_display_radial_product (
Display		*display,
Drawable	window,
GC		gc,
int		pixel,
int		scanl,
int		width,
int		height,
int		max_range,
float		x_offset,
float		y_offset,
int		zoom
)
{
	int	beam;
	int	i;
	float	azimuth1, azimuth2;
	int	old_Color;
	int	Color = 0;
	XPoint	X [12];
	float	sin1, sin2;
	float	cos1, cos2;
	float	scale_x;
	float	scale_y;
	int	center_pixel;
	int	center_scanl;
	int	product_color [PRODUCT_COLORS];


	float	resolution;
	int	data_elements;
	float	*azimuth_data;
	float	*azimuth_width;
	unsigned short	**radial_data;
	int	radials;
	int	pid;
	int	levels;
	float	step;

/*	Clear the display area first.					*/

        XSetForeground (display,
			gc,
			hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XFillRectangle (display,
			window,
			gc,
			0,
			0,
			width,
			height);

/*	Get the neccessary product parameters.				*/

	resolution    = hci_product_resolution ();
	data_elements = hci_product_data_elements ();
	azimuth_width = hci_product_azimuth_width ();
	azimuth_data  = hci_product_azimuth_data ();
	radial_data   = hci_product_radial_data ();
	levels        = hci_product_data_levels ();
	pid           = hci_product_code ();
	radials       = hci_product_radials ();

/*	Detemine the step factor to apply to the color scale.  A 16	*
*	color product uses a step of 1.  An 8 color level product uses	*
*	a step of 2.							*/

	if (pid != 34) {

	    if (levels == 8)

		step = 2;

	    else if (levels > PRODUCT_COLORS)

		step = PRODUCT_COLORS/((float) (levels));

	    else

		step = 1;
/*
	    if (PRODUCT_COLORS%levels)
		step = 1;
*/
	} else {

	    step = 1;

	}

/*	Compute scale factors and displa center coordinate.		*/

	scale_x = zoom*(width/(2.0*max_range));
	scale_y = -scale_x;
	center_pixel = width/2;
	center_scanl = height/2;
	
/*	Initialize the color database for the product		*/

	hci_get_product_colors (pid, product_color);

	for (i=0;i<data_elements;i++) {

	    Range [i] = i*resolution;

	}

/*	For each beam in the product, display it using the product	*
 *	value as a unique color in the display.				*/

	for (beam=0;beam<radials;beam++) {

	    if ((azimuth_data [beam] > 360.0) ||
		(azimuth_data [beam] <   0.0)) {

		HCI_LE_error("Error displaying radial data [bad azimuth: %f]",
		    azimuth_data [beam]);
		return;

	    }

	    azimuth1 = azimuth_data [beam] - 0.5;
	    if( beam < radials - 1 )
	    {
	      azimuth2 = azimuth_data [beam] + azimuth_width [beam] + 0.5;
	    }
	    else
	    {
	      azimuth2 = azimuth_data [beam] + azimuth_width [beam];
	    }

	    sin1 = sin ((double) (azimuth1+90)*HCI_DEG_TO_RAD);
	    sin2 = sin ((double) (azimuth2+90)*HCI_DEG_TO_RAD);
	    cos1 = cos ((double) (azimuth1-90)*HCI_DEG_TO_RAD);
	    cos2 = cos ((double) (azimuth2-90)*HCI_DEG_TO_RAD);

	    old_Color = -1;

/*	unpack the data at each gate along the beam.  If there is data	*
 *	above the lower threshold, display it.  To reduce the number of	*
 *	XPolygonFill operations, only paint gate(s) when a color change	*
 *	occurs.								*/

	    for (i=0;i<data_elements;i++) { 

	 	Color = (int) (radial_data[beam][i]*step);

		if ((Color < 0) ||
		    (Color >= PRODUCT_COLORS)) {

		    HCI_LE_error("Error displaying radial data [bad data: %d]",
			Color);
		    return;

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

			    X[0].x = (Range[i] * cos1 + x_offset) * scale_x +
				     center_pixel;
			    X[0].y = (Range[i] * sin1 + y_offset) * scale_y +
				     center_scanl;
			    X[1].x = (Range[i] * cos2 + x_offset) * scale_x +
				     center_pixel;
			    X[1].y = (Range[i] * sin2 + y_offset) * scale_y +
				     center_scanl;
			    X[4].x = X[0].x;
			    X[4].y = X[0].y;

			} else {

/*			The last point was not background so find the	*
 *			coordinates of the last two points defining the	*
 *			sector and display it.				*/

			    X[2].x = (Range[i] * cos2 + x_offset) * scale_x +
				     center_pixel;
			    X[2].y = (Range[i] * sin2 + y_offset) * scale_y +
				     center_scanl;
			    X[3].x = (Range[i] * cos1 + x_offset) * scale_x +
				     center_pixel;
			    X[3].y = (Range[i] * sin1 + y_offset) * scale_y +
				     center_scanl;

			    XSetForeground (display,
					    gc,
					    product_color [old_Color]);
			    XFillPolygon (display,
				          window,
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

		    X[2].x = (Range[i] * cos2 + x_offset) * scale_x +
		  	     center_pixel;
		    X[2].y = (Range[i] * sin2 + y_offset) * scale_y +
			     center_scanl;
		    X[3].x = (Range[i] * cos1 + x_offset) * scale_x +
		     	     center_pixel;
		    X[3].y = (Range[i] * sin1 + y_offset) * scale_y +
			     center_scanl;

		    XSetForeground (display,
				    gc,
				    product_color [old_Color]);
		    XFillPolygon (display,
			          window,
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

	        i = data_elements; 

/*	    There are no more gates in the beam but check to see if	*
 *	    the previous gate had data which needs to be displayed.  If	*
 *	    so, find the last two sector coordinates and display it.	*/

		X[2].x = (Range [i-1] * cos2 + x_offset) * scale_x +
			 center_pixel;
		X[2].y = (Range [i-1] * sin2 + y_offset) * scale_y +
			 center_scanl;
		X[3].x = (Range [i-1] * cos1 + x_offset) * scale_x +
			 center_pixel;
		X[3].y = (Range [i-1] * sin1 + y_offset) * scale_y +
			 center_scanl;

		XSetForeground (display,
				gc,
				product_color [Color]);
		XFillPolygon (display,
			      window,
			      gc,
			      X, 4,
			      Convex,
			      CoordModeOrigin);

	    }
	}
}
