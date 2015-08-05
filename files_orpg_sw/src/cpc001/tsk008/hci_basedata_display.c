/************************************************************************
 *									*
 *	Module:  hci_display_base_data.c				*
 *									*
 *	Description: This function is used by the RPG Base Data		*
 *		     Display task to display base data.  The module	*
 *		     hci_open_base_data_window	should have been called	*
 *		     first.						*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:49 $
 * $Id: hci_basedata_display.c,v 1.17 2009/02/27 22:25:49 ccalvert Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_basedata.h>

/************************************************************************
 *	Description: This function is used to display a base data	*
 *		     radial in the specified window.  The base data	*
 *		     can be either passed directly when reading from	*
 *		     the replay database or indirectly using the	*
 *		     basedata functions in libhci.			*
 *									*
 *	Input:  data          - pointer to radial data.  If NULL, data	*
 *				are read using basedata functions in	*
 *				libhci					*
 *		*display      - Display information for X windows	*
 *		window        - Drawable				*
 *		gc            - Graphics context to use			*
 *		x_offset      - X offset of radar to drawable center	*
 *		y_offset      - Y offset of radar to drawable center	*
 *		scale_x       - pixels/km scale factor			*
 *		scale_y       - scanlines/km scale factor		*
 *		center_pixel  - center pixel in drawable		*
 *		center_scanl  - center scanline in drawable		*
 *		min_value     - minimum data value to display		*
 *		max_value     - maximum data value to display		*
 *		product_color - pointer to color table for data display	*
 *		moment        - moment to display			*
 *		filter        - flag to determine whether above/below	*
 *				threshold data are assigned min/max	*
 *				color (!0) or just background (0).	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
hci_basedata_display (
char		data[],
Display		*display,
Drawable	window,
GC		gc,
float		x_offset,
float		y_offset,
float		scale_x,
float		scale_y,
int		center_pixel,
int		center_scanl,
float		min_value,
float		max_value,
int		*product_color,
int		moment,
int		filter
)
{
	int		i;
	float		azimuth;
	float		azimuth1, azimuth2;
	int		old_color;
	int		color = -1;
	int		gates;
	int		msg_type;
	int		flag;
	float		value;
	float		sin1, sin2, cos1, cos2;
	float		color_scale;
	float		range = -1.0;
	float		adjust = -1.0;
	XPoint		x [12];
	static float	azint = 1.0;
	static float	Sin [730];
	static float	Cos [730];
	int		Init_flag = 0;
	int		max_color_adjust = 1;
	short		*bin;

/*	If this is first time, then initialize Sin/Cos tables.  This	*
 *	saves processing time.						*/

	if (!Init_flag) {

	    for (i=0;i<730;i++) {

		Sin [i] = sin ((double) (i/2.0+90.0)*HCI_DEG_TO_RAD);
		Cos [i] = cos ((double) (i/2.0-90.0)*HCI_DEG_TO_RAD);

	    }
	}

	flag = 0;

/*	If no radial data passed to function, get the data using the	*
 *	basedata functions in libhci.					*/

	if (data == NULL) {

/*	    Extract various header information.				*/

	    azimuth  = hci_basedata_azimuth ();
	    gates    = hci_basedata_number_bins (moment);
	    msg_type = hci_basedata_msg_type ();

/*	    Determine if the active moment is enabled.  If it isn't,	*
 *	    then we don't want to try and display data.			*/

	    switch (moment) {

		case REFLECTIVITY :

		    if (msg_type & REF_ENABLED_BIT)
			flag = 1;

		    break;

		case VELOCITY :

		    if (msg_type & VEL_ENABLED_BIT)
			flag = 1;

		    break;

		case SPECTRUM_WIDTH :

		    if (msg_type & WID_ENABLED_BIT)
			flag = 1;

		    break;

	    }

	} else {

/*	Else, radial data have been passed directly into this function	*/

	    Base_data_header	*hdr;
	    hdr = (Base_data_header *) data;

/*	    Extract various header information.				*/

	    azimuth  = hdr->azimuth;
	    msg_type = hdr->msg_type;

	    gates   = 0;

/*	    Determine if the active moment is enabled.  If it isn't,	*
 *	    then we don't want to try and display data.			*/

	    switch (moment) {

		case REFLECTIVITY :

		    if (msg_type & REF_ENABLED_BIT)
			flag = 1;

		    gates = hdr->n_surv_bins;
		    adjust  = hdr->surv_range/HCI_METERS_PER_KM;

		    break;

		case VELOCITY :

		    if (msg_type & VEL_ENABLED_BIT)
			flag = 1;

		    gates  = hdr->n_dop_bins;
		    adjust = hdr->dop_range/HCI_METERS_PER_KM;

		    break;

		case SPECTRUM_WIDTH :

		    if (msg_type & WID_ENABLED_BIT)
			flag = 1;

		    gates  = hdr->n_dop_bins;
		    adjust = hdr->dop_range/HCI_METERS_PER_KM;

		    break;

	    }
	}

/*	If the active moment isn't enabled, then display a message	*
 *	at the window center indicating that the data for the moment	*
 *	is not available.						*/

	if (!flag) {

	    XSetForeground (display,
			    gc,
			    hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

	    XDrawImageString (display,
			      window,
			      gc,
			      center_pixel-100,
			      center_scanl+5,
			      "Data Not Available for this field",
			      33);

	    return;

	}

/*	Define the width of the beam.					*/

	azimuth1 = azimuth - azint;

	if (azimuth1 < 0) {

	    azimuth1 = azimuth1+360.0;

	}

	azimuth2 = azimuth + 3*azint/2;

	sin1  = Sin [(int) (azimuth1*2)];
	sin2  = Sin [(int) (azimuth2*2)];
	cos1  = Cos [(int) (azimuth1*2)];
	cos2  = Cos [(int) (azimuth2*2)];

/*	Determine the color scale.  One color is reserved for the	*
 *	background so subtract 1.  For Doppler moments, subtract	*
 *	another 1 for range folding.					*/

	if (moment == REFLECTIVITY) {

	    max_color_adjust = 1;

	} else {

	    max_color_adjust = 2;

	}

	color_scale = (max_value - min_value)/(PRODUCT_COLORS-max_color_adjust);

	old_color = -1;

/*	Unpack the data at each gate along the beam.  If there is data	*
 *	above the lower threshold, display it.  To reduce the number of	*
 *	XPolygonFill operations, only paint gate(s) when a color change	*
 *	occurs.								*/

	for (i=0;i<gates;i++) {

	    if (data == NULL) {

	        value = hci_basedata_value (i, moment);
	        range = hci_basedata_range (i, moment);

	    } else {

		switch (moment) {

		    case VELOCITY :

			bin   = (short *) &(data [START_OF_VELOCITY_DATA]);
			value = hci_basedata_dopl_value (bin[i]);
			range = i/4.0 + adjust;
			break;

		    case SPECTRUM_WIDTH :

			bin   = (short *) &(data [START_OF_SPECTRUM_WIDTH_DATA]);
			value = hci_basedata_width_value (bin[i]);
			range = i/4.0 + adjust;
			break;

		    case REFLECTIVITY :
		    default :

			bin   = (short *) &(data [START_OF_REFLECTIVITY_DATA]);
			value = hci_basedata_refl_value (bin[i]);
                        if( ((Base_data_header *)data)->surv_bin_size  == REFL_QUARTERKM )
			   range = i/4.0 + adjust;
                        else
			   range = i + adjust;
			break;

		}
	    }

	    if (filter == 0) {

		if (value < min_value) {

		    color = 0;

		} else if (value > max_value) {

		    if (moment == REFLECTIVITY) {

			color = 0;

		    } else {

			if (value > 999) {

			    color = PRODUCT_COLORS - max_color_adjust+1;

			} else {

			    color = 0;

			}
		    }


		} else {

		    color = (int) ((value-min_value)/color_scale + 1);

		}

	    } else {

		if (value < min_value) {

		    if (moment == REFLECTIVITY) {

			if (value < REFLECTIVITY_MIN) {

			    color = 0;

			} else {

			    color = 1;

			}

		    } else if (moment == VELOCITY) {

			if (value < VELOCITY_MIN) {

			    color = 0;

			} else {

			    color = 1;

			}

		    } else if (moment == SPECTRUM_WIDTH) {

			if (value < SPECTRUM_WIDTH_MIN) {

			    color = 0;

			} else {

			    color = 1;

			}
		    }

		} else if (value > max_value) {

		    if (moment == REFLECTIVITY) {

			color = PRODUCT_COLORS - max_color_adjust;

		    } else {

			if (value > 999) {

			    color = PRODUCT_COLORS - max_color_adjust+1;

			} else {

			    color = PRODUCT_COLORS - max_color_adjust;

			}
		    }

		} else {

		    color = (int) ((value-min_value)/color_scale + 1);

		}
	    }

	    if (color > 0) {

		if (color != old_color) {

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
					product_color [old_color]);

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
				product_color [old_color]);

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

	    old_color = color;

	}

	if ((gates >  0) &&
	    (color > 0)) {

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
			    product_color [old_color]);

	    XFillPolygon (display,
		          window,
		          gc,
		          x, 4,
		          Convex,
		          CoordModeOrigin);

	}

}

/************************************************************************
 *	Description: This function clears the contents of the specified	*
 *	window by filling in region with specified color.		*
 *									*
 *	Input:	*display      - Display information for X windows	*
 *		window        - Drawable				*
 *		gc            - Graphics context to use			*
 *		width         - Width of region (pixels) to clear	*
 *		height        - Height of region (scanlines) to clear	*
 *		color         - Color index to fill with		*
 *	Output: NONE							*
 *	Return: NONE							*
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
