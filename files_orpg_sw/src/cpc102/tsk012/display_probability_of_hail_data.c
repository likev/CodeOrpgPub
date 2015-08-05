/************************************************************************
 *	Module: display_probability_of_hail_data.c			*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to display hail product data in the	*
 *		     XPDT window.					*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:16 $
 * $Id: display_probability_of_hail_data.c,v 1.3 2001/05/22 18:13:16 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

/*	System include file definitions.				*/

#include <math.h>

#define	COLORS	16	/* number of product colors	*/

/*	Various X and window properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	float		scale_x; /* pixel/km scale factor */
extern	float		scale_y; /* scanline/km scale factor */
extern	int		center_pixel; /* pixel coordinate at window center */
extern	int		center_scanl; /* scanline coordinate at window center */
extern	Dimension	width; /* width (pixels) of window */
extern	Dimension	height; /* height (scanlines) of window */
extern	int		Colors; /* number of product colors */
extern	int		product_start_scanl; /* start scanline for product
						data */
extern	int		hail_threshold1; /* lower hail threshold value */
extern	int		hail_threshold2; /* upper hail threshold value */
extern	int		svr_hail_threshold1; /* lower severe hail threshold */
extern	int		svr_hail_threshold2; /* upper severe hail threshold */
extern	Pixel		green_color; /* hail symbol color */
extern	Pixel		seagreen_color; /* background color */
extern	Pixel		black_color; /* foreground color */
extern	XColor		color []; /* hail color data */
extern	XFontStruct	*font_info; /* label font information */

/************************************************************************
 *	Description: This function displays a Hail product in the XPDT	*
 *		     display window.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_probability_of_hail_data ()
{
	int	i;
	int	pixel;
	int	scanl;
	XPoint	X[8];
	char	buf [2];

/*	Check first to see if hail probability structure defined.	*
 *	If not, get the heck out of here.				*/

	if (hi_image == (struct hi *) NULL) {

	    printf ("Probability of Hail type detected but no data.\n");
	    return;

	}

	printf ("The number of storms is %d\n", hi_image->number_storms);

	for (i=0;i<=hi_image->number_storms;i++) {

	    printf ("Storm [%d] = %s\n", i, hi_image->equiv.storm_id [i]);
	    printf ("		Position (%d,%d)\n",
		hi_image->curr_xpos [i], hi_image->curr_ypos [i]);
	    printf ("		Probability Hail: %d\n",
		hi_image->prob_hail [i]);
	    printf ("		Probability Severe Hail: %d\n",
		hi_image->prob_svr [i]);
	    printf ("		Hail Size: %d\n",
		hi_image->hail_size [i]);

	    pixel = hi_image->curr_xpos [i]*scale_x + center_pixel;
	    scanl = hi_image->curr_ypos [i]*scale_y + center_scanl;

	    XSetForeground (display,
			    gc,
			    green_color);

	    if (hi_image->prob_svr [i] >= svr_hail_threshold1) {

		X [0].x = pixel;
		X [0].y = scanl - 10;
		X [1].x = pixel - 10;
		X [1].y = scanl + 10;
		X [2].x = pixel + 10;
		X [2].y = scanl + 10;
		X [3].x = pixel;
		X [3].y = scanl - 10;

		if (hi_image->prob_svr [i] >= svr_hail_threshold2) {

		    XFillPolygon (display,
				  pixmap,
				  gc,
				  X, 4,
				  Convex,
				  CoordModeOrigin);

		} else {

		    XDrawLines (display,
				pixmap,
				gc,
				X, 4,
				CoordModeOrigin);

		}

		XSetForeground (display,
				gc,
				seagreen_color);
			
		if (hi_image->hail_size [i] > 0) {

		    sprintf (buf,"%1i", hi_image->hail_size [i]);

		} else {

		    sprintf (buf,"*");

		}

			 /*XTextWidth (font_info, buf, 1)/2,*/
		XDrawString (display,
			 pixmap,
			 gc,
			 pixel - 4,
			 scanl+7,
			 buf,
			 1);

		XSetForeground (display,
				gc,
				color [COLORS].pixel);

		strncpy (buf,hi_image->equiv.storm_id [i],2);

			     	/*XTextWidth (font_info, buf, 2),*/
		XDrawString (display,
			     pixmap,
			     gc,
			     pixel - 8 - 16,
			     scanl,
			     buf,
			     2);

	    } else if (hi_image->prob_hail [i] >= hail_threshold1) {

		X [0].x = pixel;
		X [0].y = scanl - 5;
		X [1].x = pixel - 5;
		X [1].y = scanl + 5;
		X [2].x = pixel + 5;
		X [2].y = scanl + 5;
		X [3].x = pixel;
		X [3].y = scanl - 5;

		if (hi_image->prob_hail [i] >= hail_threshold2) {

		    XFillPolygon (display,
				  pixmap,
				  gc,
				  X, 4,
				  Convex,
				  CoordModeOrigin);

		} else {

		    XDrawLines (display,
				pixmap,
				gc,
				X, 4,
				CoordModeOrigin);

		}

		XSetForeground (display,
				gc,
				color [COLORS].pixel);

		strncpy (buf,hi_image->equiv.storm_id [i],2);

			     	/*XTextWidth (font_info, buf, 2),*/
		XDrawString (display,
			     pixmap,
			     gc,
			     pixel - 4 - 16,
			     scanl,
			     buf,
			     2);

	    }

	}

}
