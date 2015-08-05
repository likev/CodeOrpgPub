/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/09/21 16:53:01 $
 * $Id: hci_colors.c,v 1.18 2009/09/21 16:53:01 ccalvert Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_colors.c						*
 *									*
 *	Description: This module contains functions to handle color	*
 *		     data in all HCI GUI tasks.				*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_color.h>

/*  Definitions for local color data (by color).      */

char  *Color_name [] = {
  "black",
  "cyan",
  "steelblue",
  "blue",
  "lightgreen",
  "green",
  "green1",
  "green2",
  "green3",
  "green4",
  "seagreen",
  "darkgreen",
  "yellow",
  "gold",
  "orange",
  "red",
  "red1",
  "red2",
  "red3",
  "red4",
  "indianred1",
  "pink",
  "lightgray",
  "gray",
  "darkgray",
  "magenta",
  "magenta3",
  "white",
  "purple3",
  "peachpuff3",
  "brown",
  "lightsteelblue",
  "darkseagreen"
};

static int no_of_color_names = sizeof(Color_name)/sizeof(char*);
static  Pixel Color [BACKGROUND_COLOR1];

/************************************************************************
 *	Description: This function initializes the read only colors	*
 *		     used by all HCI components.  It should be called	*
 *		     after the toplevel shell is created and before	*
 *		     any widgets are created.				*
 *									*
 *	Input:  *display - pointer to Display information		*
 *		cmap     - colormap to be used in application		*
 *	Output: colors - array of pixel values                          *
 *	Return: the number of failed color allocations			*
 ************************************************************************/

int hci_initialize_read_colors (Display	*display, Colormap cmap)
{
    static int initialized = 0;

    if (initialized)
	return (0);
    initialized = 1;

	return(hci_initialize_read_colors_r(display, cmap, no_of_color_names, &Color[0]));
}

/************************************************************************
 *	Description: This function initializes the read only colors	*
 *		     used by all HCI components.  It should be called	*
 *		     after the toplevel shell is created and before	*
 *		     any widgets are created.				*
 *									*
 *	Input:  *display - pointer to Display information		*
 *		cmap     - colormap to be used in application		*
		no_of_colors - Number of values in the array 		*
 *	Output: colors - array of pixel values                          *
 *	Return: the number of failed color allocations			*
 ************************************************************************/

int hci_initialize_read_colors_r(Display* display, Colormap cmap, int no_of_colors, Pixel* colors)
{
	int	status;
	int	count;
	int i;
	XColor	color;

	/*	Initialize the failed alloc counter.			*/

	count = 0;
	HCI_LE_log("Initialize palette of %d colors", no_of_colors);

	/*	Allocate read only colors by name			*/
	if (no_of_colors > no_of_color_names)
	   no_of_colors = no_of_color_names;

	for (i=0;i<no_of_colors;i++)
	{
	   status = XParseColor (display, cmap, Color_name[i], &color);

	   if (status == 0)
	   {

    		HCI_LE_log("Unable to XParseColor (%s)",Color_name[i]);
		count++;
    		color.red   = (i*10) << 8;
		color.green = (i*10) << 8;
    		color.blue  = (i*10) << 8;
      	   }

	   status = hci_find_best_color (display, cmap, &color);

      	   colors[i] = color.pixel;
	}
	return count;	/*  Return the number of failed allocs	*/
}

/************************************************************************
 *	Description: This function maps a color macro to an HCI object	*
 *		     color property.					*
 *									*
 *	Input:  color - One of the color macros				*
 *	Output: NONE							*
 *	Return: fixed color index					*
 ************************************************************************/

int hci_get_color_index(int color)
{
/*	Return the fixed color index.					*/

	switch (color) {

	    case ALARM_COLOR1 :

		color = RED;
		break;

	    case ALARM_COLOR2 :

		color = ORANGE;
		break;

	    case BACKGROUND_COLOR1 :

		color = PEACHPUFF3;
		break;

	    case BACKGROUND_COLOR2 :

		color = GRAY;
		break;

	    case BUTTON_BACKGROUND :

		color = STEELBLUE;
		break;

	    case BUTTON_FOREGROUND :

		color = WHITE;
		break;

	    case EDIT_BACKGROUND :

		color = LIGHTSTEELBLUE;
		break;

	    case EDIT_FOREGROUND :

		color = BLACK;
		break;

	    case ICON_BACKGROUND :

		color = STEELBLUE;
		break;

	    case ICON_FOREGROUND :

		color = WHITE;
		break;

	    case LOCA_BACKGROUND :

		color = PEACHPUFF3;
		break;

	    case LOCA_FOREGROUND :

		color = WHITE;
		break;

	    case NORMAL_COLOR :

		color = GREEN;
		break;

	    case PRODUCT_BACKGROUND_COLOR :

		color = BLACK;
		break;

	    case PRODUCT_FOREGROUND_COLOR :

		color = WHITE;
		break;

	    case TEXT_BACKGROUND :

		color = PEACHPUFF3;
		break;

	    case TEXT_FOREGROUND :

		color = BLACK;
		break;

	    case WARNING_COLOR :

		color = YELLOW;
		break;

	};
	return(color);
}

/************************************************************************
 *	Description: This function returns the display color index	*
 *		     associated with one of the HCI object color 	*
 *		     properties.					*
 *									*
 *	Input:  color - One of the color macros				*
 *	Output: NONE							*
 *	Return: display color index					*
 ************************************************************************/

Pixel
hci_get_read_color (
int	color
)
{
	Pixel	cell;
	
	color = hci_get_color_index(color);

  if ((color >= BLACK) &&
      (color < BACKGROUND_COLOR1)) {

      cell = Color [color];

  } else {

      cell = Color [BLACK];

  }

	return cell;
}

int
hci_find_best_color (
Display   *display,
Colormap  cmap,
XColor    *color
)
{
  int i;
  int r, g, b;
  int q_r, q_g, q_b;
  int minind;
  int mindist;
  unsigned long pixel_not_available;
static  XColor  query_color [256];
  int public_colors;
  unsigned long pixels [256];
static  int first_time = 1;

  if( XAllocColor( display, cmap, color ) != 0 )
  {
    return 0;
  }

  if (first_time)
  {
    HCI_LE_log("Desired color not found: finding best match");
  }

/*  Lets rank the color guns by order of decreasing intensity.  */

  if (color->red > color->green) {
      if (color->red > color->blue) {
    r = 1;
    if (color->green > color->blue) {
        g = 2;
        b = 3;
    } else {
        b = 2;
        g = 3;
    }
      } else {
    b = 1;
    r = 2;
    g = 3;
      }
  } else {
      if (color->green > color->blue) {
    g = 1;
    if (color->red > color->blue) {
        r = 2;
        b = 3;
    } else {
        b = 2;
        r = 3;
    }
      } else {
    b = 1;
    g = 2;
    r = 3;
      }
  }
/*  Lets first get the colors that are defined in the specified *
 *  colormap.  After we get them we need to determine which ones  *
 *  are available (i.e., read only).        */

  pixel_not_available = 0xffffffff;
  public_colors = 256;

  if (first_time) {

      first_time    = 0;
      public_colors = 0;

      for (i=0;i<256;i++)
    query_color [i].pixel = i;

      XQueryColors (display, cmap, query_color, 256);

      for (i=0;i<256;i++) {
    if (XAllocColor (display, cmap, &query_color[i]) == 0) {
        query_color[i].pixel = pixel_not_available;
    } else {
        pixels [public_colors] = query_color[i].pixel;
        public_colors++;
    }
      }

      XFreeColors (display, cmap, pixels, public_colors,
       (unsigned long) 0);

  }

  mindist = 0xfffffff;
  minind  = -1;

  for (i=0;i<256;i++) {

      int d, dist;

      if (query_color[i].pixel == pixel_not_available)
    continue;
/*      First, we want to make sure the intensities of the color  *
 *      guns are in the same order as the original color.   */

      if (query_color[i].red > query_color[i].green) {
    if (query_color[i].red > query_color[i].blue) {
        q_r = 1;
        if (query_color[i].green > query_color[i].blue) {
      q_g = 2;
      q_b = 3;
        } else {
      q_b = 2;
      q_g = 3;
        }
    } else {
        q_b = 1;
        q_r = 2;
        q_g = 3;
    }
      } else {
    if (query_color[i].green > query_color[i].blue) {
        q_g = 1;
        if (query_color[i].red > query_color[i].blue) {
      q_r = 2;
      q_b = 3;
        } else {
      q_b = 2;
      q_r = 3;
        }
    } else {
        q_b = 1;
        q_g = 2;
        q_r = 3;
    }
      }

      if ((r == q_r) && (g == q_g) && (b == q_b)) {

    d = (color->red - query_color[i].red) >> 8;
    dist = d*d;
    d = (color->green - query_color[i].green) >> 8;
    dist += d*d;
    d = (color->blue - query_color[i].blue) >> 8;
    dist += d*d;


    if (dist < mindist) {

        mindist = dist;
        minind  = i;

          }

      } else {

    continue;

      }
  }

  if (minind < 0) {

      mindist = 0xfffffff;

      for (i=0;i<256;i++) {

    int d, dist;

    if (query_color[i].pixel == pixel_not_available)
        continue;

    d = (color->red - query_color[i].red) >> 8;
    dist = d*d;
    d = (color->green - query_color[i].green) >> 8;
    dist += d*d;
    d = (color->blue - query_color[i].blue) >> 8;
    dist += d*d;

    if (dist < mindist) {

        mindist = dist;
        minind  = i;

          }
      }
  }

  color->red   = query_color[minind].red;
  color->green = query_color[minind].green;
  color->blue  = query_color[minind].blue;
  color->pixel = query_color[minind].pixel;

  if( XAllocColor( display, cmap, color ) == 0 )
  {
    HCI_LE_error("Unexpected XAllocColor failure (%d %d %d)",
                  color->red, color->green, color->blue);
    return -1;
  }

  return 0;
}

