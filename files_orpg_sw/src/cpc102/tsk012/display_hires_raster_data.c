/************************************************************************
 *	Module: display_raster_data.c					*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to display raster product data.	*
 ************************************************************************/

/*
 * RCS info
 * $Author: eddief $
 * $Locker:  $
 * $Date: 2002/08/09 14:55:11 $
 * $Id: display_hires_raster_data.c,v 1.1 2002/08/09 14:55:11 eddief Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	Various X and window properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	XColor		dhr_color []; /* product color data */
extern	float		scale_x; /* pixel/km scale factor */
extern	float		scale_y; /* scanline/km scale factor */
extern	int		center_pixel; /* pixel coordinate at window center */
extern	int		center_scanl; /* scanline coordinate at window center */
extern	Dimension	width; /* width (pixels) of window */
extern	Dimension	height; /* height (scanlines) of window */
extern	int		Colors; /* number of product colors */

/************************************************************************
 *	Description: This function is used to display raster product	*
 *		     data in the XPDT display window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_hires_raster_data ()
{
	float	pixel_int;
	float	scanl_int;
	int	pixs;
	int	i;
	int	j;
	int	Color;
	int	color_step;
	int	pixel;
	int	scanl;

	pixel_int = ((float) (width-width/4))/raster_image->number_of_columns;
	scanl_int = pixel_int;
	pixs      = pixel_int + 1;

	printf("display hires raster %d,%d\n", raster_image->number_of_rows, raster_image->number_of_columns);

	for (i=0;i<raster_image->number_of_rows;i++)
	{

	    for (j=0;j<raster_image->number_of_columns;j++)
	    {
		Color = raster_image->raster_data [j][i];

		if (Color == 1)
		    Color = 64;
		else
		if (Color != 0)
		    Color = Color / 4 + 1;

		if (Color != 0)
		{
		   pixel = i*pixel_int;
		   scanl = width/8 + j*scanl_int;

		   XSetForeground (display, gc,
			   dhr_color [Color].pixel);
		   XFillRectangle (display,
				pixmap,
				gc,
				pixel,
				scanl,
				pixs,
				pixs);
		}
	    }

	}

}
