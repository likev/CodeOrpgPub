/************************************************************************
 *	Module: display_graphics_attributes_table.c			*
 *	Description: This module displays a products graphics attributes*
 *		     table above the product data in the NEXRAD Product	*
 *		     Display Tool (xpdt) window.			*
 ************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:40:19 $
 * $Id: display_graphics_attributes_table.c,v 1.6 2014/03/18 18:40:19 jeffs Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

#define	COLORS		16	/* number of display colors */

/*	Various X and window properties.	*/

extern	Display		*display;
extern	Window		window;
extern	Pixmap		pixmap;
extern	GC		gc;
extern	Dimension	width;	/* width (pixels) of window */
extern	Dimension	height;	/* height (scanlines) of window */
extern	int		Colors;	/* unused */
extern	int		current_row;    /* current row number */
extern	Pixel		seagreen_color; /* background color */
extern	Pixel		black_color;	/* foreground color */

/************************************************************************
 *	Description: This function displays a products graphics		*
 *		     attributes table data above product data in the	*
 *		     XPDT display window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_graphics_attributes_table ()
{
	int	i;
	int	j;
	int	Color;
	int	last_row;
	int	spacing;
	int	page;
	char	buf [10];
	XRectangle	clip_rectangle;

/*	Define a clip rectangle for the region used for the table.	*/

	clip_rectangle.x      = 0;
	clip_rectangle.y      = 0;
	clip_rectangle.width  = 3*width/4;
	clip_rectangle.height = width/8;

	XSetClipRectangles (display,
		gc,
		0,
		0,
		&clip_rectangle,
		1,
		Unsorted);

	last_row = current_row+4;

	if (last_row > gtab->number_of_lines) {

	    last_row = gtab->number_of_lines;

	}

	page = current_row/5+1;
	sprintf (buf,"Page %i", page);

	spacing = width/50;

/*	Blank out the table display region.				*/

	XSetForeground (display, gc,
		seagreen_color);

	XFillRectangle (display, pixmap, gc,
		0, 0, width-width/4, width/8);

/*	Display the current page number					*/

	XSetForeground (display, gc,
		black_color);

	XDrawString (display, pixmap, gc,
		width/2-width/8, spacing-4,
		buf,
		strlen (buf));

/*	Display the current table page.					*/

	for (i=current_row;i<=last_row;i++) {

	    XDrawString (display, pixmap, gc,
		5, (i-current_row+2)*spacing,
		gtab->text [i],
		strlen (gtab->text[i]));

	}

/*	Reset the clip rectangle.				*/

	clip_rectangle.x      = 0;
	clip_rectangle.y      = 0;
	clip_rectangle.width  = width;
	clip_rectangle.height = height;

	XSetClipRectangles (display,
		gc,
		0,
		0,
		&clip_rectangle,
		1,
		Unsorted);

}
