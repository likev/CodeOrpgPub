/************************************************************************
 *	Module: display_vad_data.c					*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to display VAD products in the display	*
 *		     window.						*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:35 $
 * $Id: display_vad_data.c,v 1.4 2001/05/22 18:13:35 davep Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	System include file definitions.				*/

#include <math.h>

#define COLORS	16 /* number of product colors */
#define	DEGREES_TO_RADIANS	(3.14159265/180.0) /* degrees to radians */

/*	Various X and windows properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	XColor		color []; /* product color data */
extern	float		scale_x; /* pixels/km scale factor */
extern	float		scale_y; /* scanlines/km scale factor */
extern	int		center_pixel; /* pixel coordinate at window center */
extern	int		center_scanl; /* scanline coordinate at window center */
extern	Dimension	width; /* width (pixels) of window */
extern	Dimension	height; /* height (scanlines) of window */
extern	int		Colors; /* number of product colors */
extern	int		product_start_scanl; /* start scanline for product
						data */
static	int	vad_height; /* height of VAD element in display */
static	int	vad_width; /* width of VAD element in display */
static	int	pixel1, pixel2; /* shared pixel coordinate data */
static	int	scanl1, scanl2; /* shared scanline coordinate data */
static	float	height_interval; /* display height interval (kft) */

/************************************************************************
 *	Description: This function is used by the XPDT tool to display	*
 *		     VAD products in the display window.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_vad_data ()
{
	int	pixs;
	int	i;
	int	j;
	int	Color;
	int	color_step;
	int	pixel_offset;
	int	scanl_offset;
	float	barb_length;
	XSegment	windbarb [256];
	int		segments;

	void	display_vad_template ();
	extern	void	make_windbarb (XSegment *windbarb,
				float speed, float direction,
				int pixel, int scanl, 
				float length, int *segments);

	vad_width  = width - width/4;
	vad_height = vad_width;

	display_vad_template ();

	printf ("Number of VAD times = %i\n",
		vwp_image->number_of_times);
	printf ("Number of VAD heights = %i\n",
		vwp_image->number_of_heights);

	height_interval = (vad_height-50.0)/30.5;
	barb_length = height_interval * 1.5;

	for (i=0;i<=vwp_image->number_of_times;i++) {

	    pixel1 = width/10 + (11-i)*(vad_width-width/10)/12;

	    XDrawString (display,
			 pixmap,
			 gc,
			 pixel1-8,
			 product_start_scanl + vad_height - 8,
			 vwp_image->times [i],
			 4);

	    for (j=0;j<=vwp_image->number_of_heights;j++) {

		scanl1 = product_start_scanl + 25 + (29-j)*height_interval;

		if (i==0) {

		    XSetForeground (display,
				    gc,
				    color [COLORS].pixel);

		    XDrawString (display,
				 pixmap,
				 gc,
				 10,
				 scanl1+4,
				 vwp_image->heights [j],
				 2);

		}

		if (vwp_image->barb [j][i].direction >= 0) {

		    XSetForeground (display,
				    gc,
				    color [vwp_image->barb [j][i].color].pixel);

		    if (vwp_image->barb [j][i].speed <= 2) {

			XDrawArc (display,
				  pixmap,
				  gc,
				  pixel1-3,
				  scanl1-3,
				  6,
				  6,
				  90*64,
				  -360*64);

		    } else {

		 	make_windbarb (windbarb,
				   (float) vwp_image->barb [j][i].speed,
				   (float) vwp_image->barb [j][i].direction,
				   pixel1,
				   scanl1,
				   barb_length,
				   &segments);

			XDrawSegments (display,
				   pixmap,
				   gc,
				   windbarb,
				   segments);

		    }

		} else {

		    XSetForeground (display,
				    gc,
				    color [COLORS].pixel);

		    XDrawString (display,
				 pixmap,
				 gc,
				 pixel1-8,
				 scanl1+4,
				 "ND",
				 2);

		}

	    }

	}

}

/************************************************************************
 *	Description: This function is used by the XPDT tool to display	*
 *		     the template for the VAD data plot.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_vad_template ()
{
	int	i;
	int	pixel1, pixel2;
	int	scanl1, scanl2;
	float	height_interval;

	XSetForeground (display,
			gc,
			color [COLORS].pixel);

/*	Draw a rectangle for the border around the entire product	*/

	XDrawRectangle (display,
			pixmap,
			gc,
			0,
			product_start_scanl,
			vad_width,
			vad_height);

/*	Draw the Vertical Scale	(height)				*/

	XDrawLine (display,
		   pixmap,
		   gc,
		   vad_width/10,
		   product_start_scanl,
		   vad_width/10,
		   product_start_scanl+vad_height);

/*	Draw the Horizontal Scale (time)				*/

	scanl1 = product_start_scanl + vad_height - 25;

	XDrawLine (display,
		   pixmap,
		   gc,
		   0,
		   scanl1,
		   vad_width,
		   scanl1);

/*	Draw the tic marks along the horizontal scale for the time	*/

	scanl2 = scanl1 - 10;

	for (i=1;i<=11;i++) {

	    pixel1 = vad_width/10 + i*(vad_width-vad_width/10)/12;

	    XDrawLine (display,
		       pixmap,
		       gc,
		       pixel1,
		       scanl1,
		       pixel1,
		       scanl2);

	}

/*	Draw the horizontal lines representing the 30 height levels	*/

	height_interval = (vad_height-50.0)/30.5;
	pixel1          = vad_width/10;
	pixel2          = vad_width;

	for (i=0;i<30;i++) {

	    scanl1 = product_start_scanl + 25 + i*height_interval;

	    XDrawLine (display,
		       pixmap,
		       gc,
		       pixel1,
		       scanl1,
		       pixel2,
		       scanl1);

	}

	XDrawString (display,
		     pixmap,
		     gc,
		     2,
		     product_start_scanl + 12,
		     "ALT KFT",
		     7);

	XDrawString (display,
		     pixmap,
		     gc,
		     12,
		     product_start_scanl + vad_height - 8,
		     "TIME",
		     4);

}
