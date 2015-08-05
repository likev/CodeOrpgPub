/************************************************************************
 *	Module: display_product_attributes.c				*
 *	Description: This module dispays product attibutes data in the	*
 *		     NEXRAD Product Display Tool (XPDT) window.		*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:18 $
 * $Id: display_product_attributes.c,v 1.4 2001/05/22 18:13:18 davep Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

#define	COLORS		16	/* number of product colors */

/*	Various X and window properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	Dimension	width;	/* width (pixels) of window. */
extern	Dimension	height;	/* height (scanlines) of window. */
extern	Pixel		black_color; /* foreground color of text */
extern	Pixel		green_color; /* foreground color of text */
extern	Pixel		yellow_color; /* foreground color of text */

/************************************************************************
 *	Description: This function display product pertinent data in	*
 *		     the XPDT display window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_product_attributes ()
{
	int	i;
	int	spacing;
	char	buf [10];

	spacing = width/50;

/*	For each line, decode and display text data */

	for (i=0;i<=attribute->number_of_lines;i++) {

	    switch (i) {

		case 0 :

		    XSetForeground (display, gc,
				black_color);
		    break;

		case 1 :

		    XSetForeground (display, gc,
				yellow_color);
		    break;

		case 4 :

		    XSetForeground (display, gc,
				green_color);
		    break;

	    }

	    XDrawString (display, pixmap, gc,
		    3*width/4+10, (i+2)*spacing-4,
		    attribute->text [i],
		    strlen (attribute->text [i]));

	}

	if (attribute->units != NULL) {

	    XSetForeground (display, gc,
		    black_color);

	    XDrawString (display, pixmap, gc,
		    width-width/8+15, height/4+5,
		    attribute->units,
		    strlen (attribute->units));

	}

}
