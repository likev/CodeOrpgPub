/************************************************************************
 *	Module: display_meso_data.c					*
 *	Description: This function is used by the NEXRAD Product	*
 *		     Display Tool (XPDT) to display meso product data.	*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:14 $
 * $Id: display_meso_data.c,v 1.3 2001/05/22 18:13:14 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	System include file definitons.					*/

#include <math.h>

#define	COLORS	16	/* number of display colors */

/*	Various X and window properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	XColor		color [];	/* meso product colors */
extern	float		scale_x; /* pixel/km scale factor */
extern	float		scale_y; /* scanline/km scale factor */
extern	int		center_pixel; /* pixel coordinate at window center */
extern	int		center_scanl; /* scanline coordinate at window center */
extern	Dimension	width; /* width (pixels) of window */
extern	Dimension	height; /* height (scanlines) of window */
extern	int		Colors; /* number of product colors */
extern	int		product_start_scanl; /* start scanline for product
						data */

/************************************************************************
 *	Description: This function dispay a Mesoscale product in the	*
 *		     XPDT display window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
display_meso_data ()
{
	int	i;
	int	j;
	int	pixel;
	int	scanl;
	int	size;

/*	Check first to see if meso structure defined.			*
 *	If not, get the heck out of here.				*/

	if (meso_image == (struct meso *) NULL) {

	    printf ("Meso type detected but no data.\n");
	    return;

	}

	printf ("The number of mesocyclones is %d\n",
		meso_image->number_mesos + 1);

	printf ("The number of 3D shear centers is %d\n",
		meso_image->number_3Dshears + 1);

	XSetForeground (display,
			gc,
			color [COLORS+4].pixel);

/*	The ICD says that meso's are an open circle at at least 7	*
 *	pixels in diameter and having a thickness of 4 pixels.	If	*
 *	the diameter is > 7, then the diameter is increased to that	*
 *	value.								*/

	XSetLineAttributes (display,
		       gc,
		       4,
		       LineSolid,
		       CapButt,
		       JoinMiter);

	for (i=0;i<=meso_image->number_mesos;i++) {

	    pixel = meso_image->meso_xpos [i]*scale_x + center_pixel;
	    scanl = meso_image->meso_ypos [i]*scale_y + center_scanl;

	    if (meso_image->meso_size [i] < 7) {

		size = 7;

	    } else {

		size = meso_image->meso_size [i];

	    }

	    XDrawString (display,
			 pixmap,
			 gc,
			 pixel+size,
			 scanl+size,
			 meso_image->equiv.storm_id [i],
			 2);

	    XDrawArc (display,
		      pixmap,
		      gc,
		      pixel-size/2,
		      scanl-size/2,
		      size,
		      size,
		      0,
		      -(360*64));

	}

/*	The ICD says that 3D shear's are an open circle at at least 7	*
 *	pixels in diameter and having a thickness of x1 pixel.	If	*
 *	the diameter is > 7, then the diameter is increased to that	*
 *	value.								*/

	XSetLineAttributes (display,
		       gc,
		       1,
		       LineSolid,
		       CapButt,
		       JoinMiter);

	for (i=0;i<=meso_image->number_3Dshears;i++) {

	    pixel = meso_image->a3D_xpos [i]*scale_x + center_pixel;
	    scanl = meso_image->a3D_ypos [i]*scale_y + center_scanl;

	    if (meso_image->a3D_size [i] < 7) {

		size = 7;

	    } else {

		size = meso_image->a3D_size [i];

	    }

	    XDrawArc (display,
		      pixmap,
		      gc,
		      pixel-size/2,
		      scanl-size/2,
		      size,
		      size,
		      0,
		      -(360*64));

	}
}
