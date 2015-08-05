/************************************************************************
 *	Module: display_swp_data.c					*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to display severe weather propbability	*
 *		     products.						*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:28 $
 * $Id: display_swp_data.c,v 1.3 2001/05/22 18:13:28 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	System include file definitions.				*/

#include <math.h>

#define	COLORS	16	/* number of display colors */

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
extern	Pixel		yellow_color; /* data level 2 color */
extern	Pixel		red_color;    /* data level 3 color */
extern	Pixel		cyan_color;   /* data level 1 color */

/************************************************************************
 *	Description: This function is used to display a Severe Weather	*
 *		     Probability (SWP) product in the XPDT display	*
 *		     window.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_swp_data ()
{
	int	i;
	int	j;
	int	pixel;
	int	scanl;
	int	size;
	Pixel	color;

/*	Check first to see if swp structure defined.			*
 *	If not, get the heck out of here.				*/

	if (swp_image == (struct swp *) NULL) {

	    printf ("SWP type detected but no data.\n");
	    return;

	}

	printf ("The number of storms is %d\n",
		swp_image->number_storms + 1);

	for (i=0;i<=swp_image->number_storms;i++) {

	    switch (swp_image->text_value [i]) {

		case 1 :

		    color = cyan_color;
		    break;

		case 2 :

		    color = yellow_color;
		    break;

	 	case 3 :

		    color = red_color;
		    break;

		default :

		    color = 0;
		    break;

	    }

	    XSetForeground (display,
			    gc,
			    color);

	    pixel = swp_image->xpos [i]*scale_x + center_pixel;
	    scanl = swp_image->ypos [i]*scale_y + center_scanl;

	    XDrawString (display,
			 pixmap,
			 gc,
			 pixel-5,
			 scanl+5,
			 swp_image->equiv.swp [i],
			 2);

	}

}
