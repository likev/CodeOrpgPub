/************************************************************************
 *									*
 *	Module:  gui_display_radial_data.c				*
 *									*
 *	Description: This function is used to display radial type	*
 *		     data in a user defined window.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:31:10 $
 * $Id: trd_display_radial_data.c,v 1.3 2009/02/27 22:31:10 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <trd.h>


/************************************************************************
 *	Description: This function is used to display a terrain data	*
 *		     radial in the specified window.			*
 *		     BASEDATA LB using libhci functions.		*
 *									*
 *	Input:  data          - pointer to radial data.			*
 *		data_type     - DATA_TYPE_BYTE				*
 *				DATA_TYPE_SHORT				*
 *				DATA_TYPE_INT				*
 *				DATA_TYPE_FLOAT				*
 *		*display      - Display information for X windows	*
 *		window        - Drawable				*
 *		gc            - Graphics context to use			*
 *		radial        - structure containing radial type data	*
 *		color         - structure containing color type data	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
Gui_display_radial_data (
void		*data,
int		data_type,
Display		*display,
Drawable	window,
GC		gc,
Gui_radial_t	*radial,
Gui_color_t	*color
)
{
	unsigned char	*byte_data = NULL;
	short		*short_data = NULL;
	int		*int_data = NULL;
	float		*float_data = NULL;
	int		i;
	float		azimuth1, azimuth2;
	int		old_color_index;
	int		color_index = -1;
	float		value = -1;
	float		range;
	float		sin1, sin2, cos1, cos2;
	float		color_scale;
	XPoint		x [12];
	static float	Sin [3600];
	static float	Cos [3600];
	static int	Init_flag = 0;

	switch (data_type) {

	    case DATA_TYPE_BYTE :

		byte_data = (unsigned char *) data;
		break;

	    case DATA_TYPE_SHORT :

		short_data = (short *) data;
		break;

	    case DATA_TYPE_INT :

		int_data = (int *) data;
		break;

	    case DATA_TYPE_FLOAT :

		float_data = (float *) data;
		break;

	    default :

		return;

	}

/*	If this is first time, then initialize Sin/Cos tables.  This	*
 *	saves processing time.						*/

	if (!Init_flag) {

	    for (i=0;i<3600;i++) {

		Sin [i] = sin ((double) (i/10.0+90.0)*HCI_DEG_TO_RAD);
		Cos [i] = cos ((double) (i/10.0-90.0)*HCI_DEG_TO_RAD);

	    }

	    Init_flag = 1;

	}

/*	Define the width of the beam.					*/

	azimuth1 = radial->azimuth - radial->azimuth_width/2;

	if (azimuth1 < 0) {

	    azimuth1 = azimuth1+360.0;

	}

	azimuth2 = radial->azimuth + 3*radial->azimuth_width/2;

	if (azimuth2 >= 360.0) {

	    azimuth2 = azimuth2-360.0;

	}

	sin1  = Sin [(int) (azimuth1*10)];
	sin2  = Sin [(int) (azimuth2*10)];
	cos1  = Cos [(int) (azimuth1*10)];
	cos2  = Cos [(int) (azimuth2*10)];

	color_scale = (color->max_value - color->min_value)/
		      (color->num_colors-1);

	old_color_index = -1;

/*	Unpack the data at each gate along the beam.  If there is data	*
 *	above the lower threshold, display it.  To reduce the number of	*
 *	XPolygonFill operations, only paint gate(s) when a color change	*
 *	occurs.								*/

	for (i=0;i<radial->bins;i++) {

	    range = radial->range_start + i*radial->range_interval;

	    switch (data_type) {

		case DATA_TYPE_BYTE :

		    value   = (float) byte_data [i];
		    break;

		case DATA_TYPE_SHORT :

		    value   = (float) short_data [i];
		    break;

		case DATA_TYPE_INT :

		    value   = (float) int_data [i];
		    break;

		case DATA_TYPE_FLOAT :

		    value   = float_data [i];
		    break;

	    }

	    switch (color->filter) {

		case 0 :

		    if ((value < color->min_value) ||
			(value > color->max_value)) {

			color_index = 0;

		    } else {

			color_index = (int) ((value-color->min_value)/
					     color_scale + 1);

		    }

		    break;

		case 1 :

		    if (value < color->min_value) {

			color_index = 1;

		    } else if (value > color->max_value) {

			color_index = 0;

		    } else {

			color_index = (int) ((value-color->min_value)/
					     color_scale + 1);

		    }

		    break;

		case 2 :

		    if (value < color->min_value) {

			color_index = 0;

		    } else if (value > color->max_value) {

			color_index = color->num_colors-1;

		    } else {

			color_index = (int) ((value-color->min_value)/
					     color_scale + 1);

		    }

		    break;

		case 3 :

		    if (value < color->min_value) {

			color_index = 1;

		    } else if (value > color->max_value) {

			color_index = color->num_colors-1;

		    } else {

			color_index = (int) ((value-color->min_value)/
					     color_scale + 1);

		    }

		    break;

	    }

	    if (color_index > 0) {

		if (color_index >= color->num_colors)
		    color_index = color->num_colors-1;

		if (color_index != old_color_index) {

		    if (old_color_index <= 0) {

			x[0].x = (range*cos1 + radial->x_offset) *
				  radial->scale_x +
				  radial->center_pixel;
			x[0].y = (range*sin1 + radial->y_offset) *
				  radial->scale_y +
				  radial->center_scanl;
			x[1].x = (range*cos2 + radial->x_offset) *
				  radial->scale_x +
				  radial->center_pixel;
			x[1].y = (range*sin2 + radial->y_offset) *
				  radial->scale_y +
				  radial->center_scanl;
			x[4].x = x[0].x;
			x[4].y = x[0].y;

		    } else {

			x[2].x = (range*cos2 + radial->x_offset) *
				  radial->scale_x +
				  radial->center_pixel;
			x[2].y = (range*sin2 + radial->y_offset) *
				  radial->scale_y +
				  radial->center_scanl;
			x[3].x = (range*cos1 + radial->x_offset) *
				  radial->scale_x +
				  radial->center_pixel;
			x[3].y = (range*sin1 + radial->y_offset) *
				  radial->scale_y +
				  radial->center_scanl;

			XSetForeground (display,
					gc,
					color->color [old_color_index]);

			if ((x[0].x == x[1].x) &&
			    (x[0].x == x[2].x) &&
			    (x[0].x == x[3].x) &&
			    (x[0].x == x[4].x) &&
			    (x[0].y == x[1].y) &&
			    (x[0].y == x[2].y) &&
			    (x[0].y == x[3].y) &&
			    (x[0].y == x[4].y)) {

			    XDrawPoint (display,
					window,
					gc,
					x[0].x,
					x[0].y);

			} else {

			    XFillPolygon (display,
				      window,
				      gc,
				      x, 4,
				      Convex,
				      CoordModeOrigin);

			}

			x[0].x = x[3].x;
			x[0].y = x[3].y;
			x[1].x = x[2].x;
			x[1].y = x[2].y;
			x[4].x = x[0].x;
			x[4].y = x[0].y;

		    }
		}

	    } else if (old_color_index > 0) {

		x[2].x = (range*cos2 + radial->x_offset) * radial->scale_x +
			  radial->center_pixel;
		x[2].y = (range*sin2 + radial->y_offset) * radial->scale_y +
			  radial->center_scanl;
		x[3].x = (range*cos1 + radial->x_offset) * radial->scale_x +
			  radial->center_pixel;
		x[3].y = (range*sin1 + radial->y_offset) * radial->scale_y +
			  radial->center_scanl;

		XSetForeground (display,
				gc,
				color->color [old_color_index]);

		if ((x[0].x == x[1].x) &&
		    (x[0].x == x[2].x) &&
		    (x[0].x == x[3].x) &&
		    (x[0].x == x[4].x) &&
		    (x[0].y == x[1].y) &&
		    (x[0].y == x[2].y) &&
		    (x[0].y == x[3].y) &&
		    (x[0].y == x[4].y)) {

		    XDrawPoint (display,
				window,
				gc,
				x[0].x,
				x[0].y);

		} else {

		    XFillPolygon (display,
			      window,
			      gc,
			      x, 4,
			      Convex,
			      CoordModeOrigin);

		}

		x[0].x = x[3].x;
		x[0].y = x[3].y;
		x[1].x = x[2].x;
		x[1].y = x[2].y;
		x[4].x = x[0].x;
		x[4].y = x[0].y;

	    }

	    old_color_index = color_index;

	}

	if ((i >  0) &&
	    (color_index > 0)) {

	    x[2].x = (range*cos2 + radial->x_offset) * radial->scale_x +
		      radial->center_pixel;
	    x[2].y = (range*sin2 + radial->y_offset) * radial->scale_y +
		      radial->center_scanl;
	    x[3].x = (range*cos1 + radial->x_offset) * radial->scale_x +
		      radial->center_pixel;
	    x[3].y = (range*sin1 + radial->y_offset) * radial->scale_y +
		      radial->center_scanl;

	    XSetForeground (display,
			    gc,
			    color->color [old_color_index]);

	    if ((x[0].x == x[1].x) &&
		(x[0].x == x[2].x) &&
		(x[0].x == x[3].x) &&
		(x[0].x == x[4].x) &&
		(x[0].y == x[1].y) &&
		(x[0].y == x[2].y) &&
		(x[0].y == x[3].y) &&
		(x[0].y == x[4].y)) {

		XDrawPoint (display,
			window,
			gc,
			x[0].x,
			x[0].y);

	    } else {

		XFillPolygon (display,
			      window,
			      gc,
			      x, 4,
			      Convex,
			      CoordModeOrigin);

	    }
	}
}
