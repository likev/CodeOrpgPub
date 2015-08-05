/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:49 $
 * $Id: hci_blockage.h,v 1.2 2009/02/27 22:25:49 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef GUI_DEF
#define	GUI_DEF

#include <hci.h>

#define	GUI_MAX_COLORS		256

enum {DATA_TYPE_BYTE=0,
      DATA_TYPE_SHORT,
      DATA_TYPE_INT,
      DATA_TYPE_FLOAT
};

/*	The following data structure is used to define the data ranges	*
 *	and associated colors used to display the data.			*/

typedef struct {

    float	min_value;	/* Minimum data value to display */
    float	max_value;	/* Maximum data value to display */
    int		filter;		/* Filter to control data outside of min
				   and max values.

					0 - Values outside range are displayed
					    in background color
					1 - Values above max value are
					    displayed in max color.
					2 - Values below min value are
					    displayed in min color.
					3 - Options 1 and 2
				 */
    int		num_colors;	/* Number if colors for display */
    Pixel	color[GUI_MAX_COLORS];
				/* Color table for display (first color
				   is background color). */

} Gui_color_t;

/*	The following data store is used to define radial properties.	*/

typedef struct {

    float	azimuth;	/* Azimuth angle (degrees) of radial
				   center. */
    float	azimuth_width;	/* Width of beam (in degrees). */
    float	range_start;	/* Range (km) of center of first bin. */
    float	range_interval;	/* Interval (km) between bins. */
    int		bins;		/* Number of bins along radial. */
    float	x_offset;	/* X offset (km) between data focal point and
				   center of display. */
    float	y_offset;	/* Y offset (km) between data focal point and
				   center of display. */
    float	scale_x;	/* X scale factor (pixels/km). */
    float	scale_y;	/* Y scale factor (scanls/km). */
    int		center_pixel;	/* Pixel coordinate for window center */
    int		center_scanl;	/* Scanl coordinate for window center */

} Gui_radial_t;

void	Gui_display_radial_data (void    *data,
				 int      data_type,
				 Display *display,
				 Drawable window,
				 GC       GC,
				 Gui_radial_t	*radial,
				 Gui_color_t    *color);

#endif
