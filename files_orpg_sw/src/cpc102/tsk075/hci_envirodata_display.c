/************************************************************************
 *									*
 *	Module:  hci_envirodata_display.c				*
 *									*
 *	Description: This function is used by the RPG Environmental	*
 *		     Model Viewer task to display environmental model   *
 *		     data.						*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/30 19:27:26 $
 * $Id: hci_envirodata_display.c,v 1.1 2013/05/30 19:27:26 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*	Include file definitions.					*/

#include <hci.h>
#include <hci_envirodata.h>

/************************************************************************
 *	Description: This function is used to display environmental	*
 *		     model data in the specified window.  The model data*
 *		     is passed directly to this function.	        *
 *									*
 *	Input:  data          - pointer to data.                	*
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
 *              rows            number of rows in the model grid        *
 *              columns         number of columns in the model grid     *
 *              pixel           size of grid in x-direction             *
 *              scanl           size of grid in y-direction             *
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
hci_envirodata_display (
float		data[],
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
int		filter,
int             rows,
int             columns,
int             pixel,
int             scanl
)
{
	int		i, j, index;
	int		color = -1;
	float		value;
	float		color_scale;
	float		range = -1.0;
	float		adjust = -1.0;
	XPoint		x [12];
	int		max_color_adjust = 1;

/*      Loop for each row of model data in the grid */
        for (j=0; j<rows; j++) {

/*	  Determine the color scale.  One color is reserved for the	*
 *	  background so subtract 1.                                       */
	  max_color_adjust = 1;

	  color_scale = (max_value - min_value)/(PRODUCT_COLORS-max_color_adjust);

/*	  Unpack the data at each grid box along the row.  If there is data	*
 *	  above the lower threshold, display it.  */

/*        HCI_LE_log("****Drawing row %d  color_scale = %f, PRODUCT_COLORS = %d max=%f, min=%f cols %d",
                                j, color_scale, PRODUCT_COLORS, max_value, min_value, columns);*/
	  for (i=0;i<columns;i++) {

/*          Set the index into the grid */
            index = j*columns+i;

            switch (moment) {

	    	case REL_HUMIDITY :

		   value = data[index];
          /*       HCI_LE_log("value[%d] = %f", i,value);*/
		   range = i/4.0 + adjust;
		   break;

		case SFC_PRESSURE :

		   value  = data[index];
             /*    HCI_LE_log("value[%d] = %f", i,value);*/
		   range = i/4.0 + adjust;
		   break;

		case TEMPERATURE :
		default :

		   value = data[index];
               /*  HCI_LE_log("value[%d] = %f", i,value);*/
		   break;
		} /*end switch */

	    if (filter == 0) {
		if (value < min_value) {
		    color = 0;
		} else if (value > max_value) {
			color = 0;
		} else {
		    color = (int) ((value-min_value)/color_scale + 0.9);
		}

	    } else {
		if (value < min_value) {
		    if (moment == TEMPERATURE) {

			if (value < TEMPERATURE_MIN) {
			    color = 0;
			} else {
			    color = 1;
			}
		    } else if (moment == REL_HUMIDITY) {

			if (value < REL_HUMIDITY_MIN) {
			    color = 0;
			} else {
			    color = 1;
			}
		    } else if (moment == SFC_PRESSURE) {

			if (value < SFC_PRESSURE_MIN) {
			    color = 0;
			} else {
			    color = 1;
			}
		    }
		} else if (value > max_value) {
			color = PRODUCT_COLORS - max_color_adjust;

		} else {
		    color = (int) ((value-min_value)/color_scale + 0.9);
		}
	    }
	    if (color > 0) {
		x[0].x = (i*pixel + x_offset);
		x[0].y = (j*scanl + y_offset);
		XSetForeground (display,
				gc,
				product_color [color]);

  /*             HCI_LE_log("XFillRectangle, x[0] = %d,%d pxl %d scnl %d Color[%d]: %d",
                            x[0].x, x[0].y, pixel, scanl, color, product_color[color]);*/
                XFillRectangle (display,
                                window,
                                gc,
                                x[0].x,
                                x[0].y,
                                pixel,
                                scanl);

		x[0].x = x[3].x;
		x[0].y = x[3].y;
		x[1].x = x[2].x;
		x[1].y = x[2].y;
		x[4].x = x[0].x;
		x[4].y = x[0].y;

	    }
          }/* end for all columns */
	}/* end for all rows */
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
hci_envirodata_display_clear (
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
