/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:04 $
 * $Id: hci_display_color_bar.c,v 1.10 2009/02/27 22:26:04 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/************************************************************************
 *	Module: hci_display_color_bar.c					*
 *	Description: This function is used to create a vertical color	
 *		     bar at a user specified location.  The color bar	*
 *		     is associated with an open RPG product and assumes	*
 *		     one has already been openned.			*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_product_colors.h>

/************************************************************************
 *	Description: This function creates a vertical color color bar.	*	
 *									*
 *	Input:  display    - pointer to Display information		*
 *	 	pixmap     - Drawable					*
 *		gc         - graphics context				*
 *		left_pixel - leftmost pixel for color bar/labels	*
 *		top_scanl  - topmost scanline for color bar/labels	*
 *		height     - height (scanlines) for color bar/labels	*
 *		width      - width (pixels) for color bar/labels	*
 *		box_width  - width (pixels) of a color bar element	*
 *		flag       - flag to indicate last color range folding	*
 *			     (1).  Set to 0 for no special processing	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
hci_display_color_bar (
Display		*display,	/* The window Display	*/
Drawable	pixmap,		/* The drawable.	*/
GC		gc,		/* The graphics context */
int		left_pixel,	/* The leftmost pixel boundary for the
				   color bar and text labels.	*/
int		top_scanl,	/* The topmost scanline boundary for
				   the color bar and text labels. */
int		height,		/* The height (scanlines) for the color
				   bar and labels. */
int		width,		/* The width (pixels) for the color
				   bar and labels. */
int		box_width,	/* The width of a color bar box element. */
int		flag		/* Flag to indicate last color RF (1).
				   Set to 0 for no special processing. */
)
{
	int	i;
	int	ii;
	int	scanl;
	float	xheight;
	int	box_height;
	int	start_index;
	int	stop_index;
	int	color_step;
	int	fg_color;
	int	product_colors [PRODUCT_COLORS];
	int	product_code;
	int	num_levels;
	int	RDA_config_code = -1;

/*	Get the product code of the currently open product.	*/

	product_code = hci_product_code();

/*	If the product has not been openned or the product code	*
 *	does not at least match the first product, do nothing.	*/

	if (product_code < 16) {

	    return;

	}

/*	Clear the area where the color bar is to be displayed.	*/

	XSetForeground (display, gc,
		hci_get_read_color (BACKGROUND_COLOR1));
	XFillRectangle (display,
		pixmap,
		gc,
		left_pixel,
		top_scanl,
		width,
		height);

/*	Get the product colors associated with the current product.	*/

	hci_get_product_colors (product_code,
				product_colors);

/*	Define the color step and box intervals.			*/

	if (hci_product_attribute_num_data_levels() > PRODUCT_COLORS)
	    num_levels = PRODUCT_COLORS;
	else
	    num_levels = hci_product_attribute_num_data_levels();

	xheight    = height/(1.5*num_levels);
	box_height = (xheight+1) * num_levels /
			num_levels;
	color_step = 1;

	if (hci_product_attribute_num_data_levels() == 8)
	    color_step = 2;

	start_index = 0;

        /*
         If this is the CFC product (product_code == 34),
         then we need to display the color bar differently.
        */

	if( product_code == 34 )
        {
          /*
           Check RDA configuration. Legend display will differ
           depending on whether RDA is legacy or ORDA.
          */

          RDA_config_code = ORPGRDA_get_rda_config ( NULL );

          if( RDA_config_code == ORPGRDA_LEGACY_CONFIG )
          {
            /* If RDA configuration is legacy... */

	    xheight    = height/(1.5*16);
	    box_height = (xheight+1) * 16 /
			16;

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));

	    scanl = top_scanl+20 + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "DISABLE FILTER",
			 14);

	    scanl = scanl + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "(OP SEL CODE 0)",
			 15);

	    scanl = scanl + xheight;

	    XSetForeground (display, gc,
		product_colors [0]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (WHITE));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));

	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+8+box_width,
			 scanl+15,
			 "FILTER OFF",
			 10);

	    scanl = scanl + 3*xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "BYP MAP IN CTRL",
			 15);

	    scanl = scanl + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "(OP SEL CODE 1)",
			 15);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [1]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "NO CLUTTER",
		 10);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [2]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "LOW     (1)",
		 11);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [3]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "MEDIUM  (2)",
		 11);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [4]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "HIGH    (3)",
		 11);

	    scanl = scanl + 3*xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "FORCE FILTER",
			 12);

	    scanl = scanl + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "(OP SEL CODE 2)",
			 15);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [5]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "LOW     (1)",
		 11);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [6]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "MEDIUM  (2)",
		 11);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [7]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "HIGH    (3)",
		 11);
          }
          else
          {
            /* If RDA configuration is ORDA... */

	    xheight    = height/(1.5*16);
	    box_height = (xheight+1) * 16 /
			16;

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));

	    scanl = top_scanl+20 + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "DISABLE FILTER",
			 14);

	    scanl = scanl + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "(OP SEL CODE 0)",
			 15);

	    scanl = scanl + xheight;

	    XSetForeground (display, gc,
		product_colors [0]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (WHITE));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));

	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+8+box_width,
			 scanl+15,
			 "FILTER OFF",
			 10);

	    scanl = scanl + 3*xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "BYP MAP IN CTRL",
			 15);

	    scanl = scanl + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "(OP SEL CODE 1)",
			 15);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [1]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "NO CLUTTER",
		 10);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [4]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "CLUTTER",
		 7);

	    scanl = scanl + 3*xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "FORCE FILTER",
			 12);

	    scanl = scanl + xheight;
	    XDrawString (display,
			 pixmap,
			 gc,
			 left_pixel+5,
			 scanl+5,
			 "(OP SEL CODE 2)",
			 15);

	    scanl = scanl + xheight;
	    XSetForeground (display, gc,
		product_colors [7]);
	    XFillRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);

	    XSetForeground (display, gc,
		hci_get_read_color (BLACK));
	    XDrawRectangle (display,
		pixmap,
		gc,
		left_pixel+5,
		scanl,
		box_width,
		box_height);
	    XDrawString (display,
		 pixmap,
		 gc,
		 left_pixel+8+box_width,
		 scanl+15,
		 "FORCE FILTER",
		 12);
          }
	}
        else
        {

/*	    If the input flag is set, we want to add an extra color and	*
 *	    set it purple.  It is intended to be used to denote RF but	*
 *	    can be used otherwise.					*/

	    stop_index  = num_levels*color_step;

/*	    Define the foreground color to be used for text labels.	*/

	    fg_color = hci_get_read_color (BLACK);

	    ii = start_index;

/*	    for each color index, display a color box and label.	*/

	    for (i=start_index;i<stop_index;i=i+color_step) {

	        scanl = top_scanl+20 + xheight*(i-start_index)/color_step;

	        if ((i == (stop_index-1)) && (flag != 0)) {

		    XSetForeground (display, gc,
			hci_get_read_color (PURPLE));
		    XFillRectangle (display,
			pixmap,
			gc,
			(left_pixel+40),
			scanl,
			box_width,
			box_height);

		    if (hci_product_attribute_data_level (ii) != NULL) {

		        XSetForeground (display, gc, fg_color);

		        XDrawString (display,
				pixmap,
				gc,
				left_pixel,
				(scanl+5),
				"    RF",
				6);

		    }

	        } else {

		    XSetForeground (display, gc,
			product_colors [i]);
		    XFillRectangle (display,
			pixmap,
			gc,
			(left_pixel+40),
			scanl,
			box_width,
			box_height);

		    if (hci_product_attribute_data_level (ii) != NULL) {

		        XSetForeground (display, gc, fg_color);

		        XDrawString (display,
				pixmap,
				gc,
				left_pixel,
				(scanl+5),
				(char *) hci_product_attribute_data_level (ii),
				strlen ((char *) hci_product_attribute_data_level (ii)));

		    }

		    ii++;

	        }
	    }

/*	    Draw a rectangle around the perimeter of the color bar.	*/

	    XSetForeground (display, gc, fg_color);

	    XDrawRectangle (display,
		pixmap,
		gc,
		(left_pixel+40),
		top_scanl+20,
		box_width,
		(int) (height/1.5));

/*	    Display the units text to the top right of the color bar	*/

	    if (hci_product_attribute_units () != NULL) {

	        XDrawString (display,
			 pixmap,
			 gc,
			 (left_pixel + 55),
			 (top_scanl+25),
			 hci_product_attribute_units (),
			 strlen (hci_product_attribute_units ()));

	    }
	}
}
