/************************************************************************
 *	Module: display_storm_track_data.c				*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to display storm track product data.	*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:26 $
 * $Id: display_storm_track_data.c,v 1.3 2001/05/22 18:13:26 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	System include file definitions.				*/

#include <math.h>

#define	COLORS	16 /* number of product colors */

/*	Various X and Window properties.	*/

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
extern	int		first_storm; /* index of first storm to display */
extern	int		last_storm; /* index of last storm to display */

/************************************************************************
 *	Description: This function is used to display storm track	*
 *		     product data in the XPDT display window.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_storm_track_data ()
{
	int	i;
	int	j;
	int	last;
	int	pixel;
	int	scanl;
	int	current_pixel;
	int	current_scanl;
	int	last_pixel;
	int	last_scanl;

/*	Check first to see if storm track structure defined.		*
 *	If not, get the heck out of here.				*/

	if (sti_image == (struct sti *) NULL) {

	    printf ("Storm Track type detected but no data.\n");
	    return;

	}

	printf ("The number of storms is %d\n",
		sti_image->number_storms + 1);

	XSetForeground (display,
			gc,
			color [COLORS].pixel);

	if (sti_image->number_storms < first_storm-1) {

	    return;

	}

	if (sti_image->number_storms < last_storm-1) {

	    last = sti_image->number_storms;

	} else {

	    last = last_storm-1;

	}

	for (i=first_storm-1;i<=last;i++) {

	    current_pixel = sti_image->curr_xpos [i]*scale_x + center_pixel;
	    current_scanl = sti_image->curr_ypos [i]*scale_y + center_scanl;

	    XDrawString (display,
			    pixmap,
			    gc,
			    current_pixel+5,
			    current_scanl+15,
			    sti_image->equiv.storm_id [i],
			    2);

	    XDrawArc (display,
		      pixmap,
		      gc,
		      current_pixel-4,
		      current_scanl-4,
		      8,
		      8,
		      0,
		      -(360*64));

/*	    Check for a slow mover (like I feel today).  If true	*
 *	    draw a larger circle around the first circle.		*/

	    if (sti_image->number_past_pos [i] == slow_mover) {

		XDrawArc (display,
			  pixmap,
			  gc,
			  current_pixel-7,
			  current_scanl-7,
			  14,
			  14,
			  0,
			  -(360*64));

	    }

	    last_pixel = -999;

	    for (j=0;j<sti_image->number_past_pos [i];j++) {

	        pixel = sti_image->past_xpos [i][j]*scale_x + center_pixel;
	        scanl = sti_image->past_ypos [i][j]*scale_y + center_scanl;

	        XFillArc (display,
		      pixmap,
		      gc,
		      pixel-4,
		      scanl-4,
		      8,
		      8,
		      0,
		      -(360*64));

		if (j>0) {

		    XDrawLine (display,
			       pixmap,
			       gc,
			       pixel,
			       scanl,
			       last_pixel,
			       last_scanl);

		}

		last_pixel = pixel;
		last_scanl = scanl;

	    }

	    if (last_pixel > -999) {

		XDrawLine (display,
			   pixmap,
			   gc,
			   current_pixel,
			   current_scanl,
			   last_pixel,
			   last_scanl);

	    }

	    last_pixel = current_pixel;
	    last_scanl = current_scanl;

	    for (j=0;j<sti_image->number_forcst_pos [i];j++) {

	        pixel = sti_image->forcst_xpos [i][j]*scale_x + center_pixel;
	        scanl = sti_image->forcst_ypos [i][j]*scale_y + center_scanl;

		XDrawLine (display,
			   pixmap,
			   gc,
			   pixel-3,
			   scanl-3,
			   pixel+3,
			   scanl+3);

		XDrawLine (display,
			   pixmap,
			   gc,
			   pixel-3,
			   scanl+3,
			   pixel+3,
			   scanl-3);

		XDrawLine (display,
			   pixmap,
			   gc,
			   pixel,
			   scanl,
			   last_pixel,
			   last_scanl);

		last_pixel = pixel;
		last_scanl = scanl;

	    }

	}
}
