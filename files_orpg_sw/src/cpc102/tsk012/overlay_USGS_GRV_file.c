/************************************************************************
 *	Module: overlay_USGS_GRV_file.c					*
 *	Description: This routine displays a USGS Digital Line Graph	*
 *		     format map overlay file on a product.		*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:14:02 $
 * $Id: overlay_USGS_GRV_file.c,v 1.4 2001/05/22 18:14:02 davep Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*	System include file definitions.				*/

#include <math.h>

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>

/*	Local include file definitions.					*/

#include "rle.h"

#define DEGRAD	(3.14159265/180.0) /* degrees to radians conversion */
#define COLORS	16 /* number of color levels */

FILE	*fd;	/* USGS Digital Line Graph file descriptor */
char	inbuf [128000]; /* line graph dat buffer */

/*	Various X and windows properties	*/

extern	Display	*display;
extern	Window	window;
extern	Pixmap	pixmap;
extern	GC	gc;
extern	float	scale_x; /* pixel/km scale factor */
extern	float	scale_y; /* scanline/km scale factor */
extern	int	center_pixel; /* pixel coordinate at window center */
extern	int	center_scanl; /* scanline coordinate at window center */
extern	Dimension	width; /* width (pixels) of window */
extern	Dimension	height; /* height (scanlines) of window */
extern	int	product_start_scanl; /* start scanline for product data */
extern	int	pixel_offset; /* pixel offset for start of product data */
extern	int	scanl_offset; /* scanline offset for start of product data */
extern	float	radar_latitude; /* radar latitude (degrees) */
extern	float	radar_longitude; /* radar longitude (degrees) */
extern	XColor	color []; /* product color data */
extern	char	xpdt_map_filename []; /* map filename */

/************************************************************************
 *	Description: This function is used by the NEXRAD Product	*
 *		     Display (XPDT) tool to display USGS Digital Line	*
 *		     Graph data in the display window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
overlay_USGS_GRV_file ()
{
	double	angle;
	float	latitude1, longitude1, latitude2, longitude2, azimuth, range;
	int	pixel1, scanl1, pixel2, scanl2,
		points;
	int	number_of_points;
	float	first_latitude;
	float	first_longitude;
	float	second_latitude;
	float	second_longitude;
	int	i;
	int	i1, i2, i3, i4;
	XRectangle	clip_rectangle;
	char	*radar_override;

	extern void	earth_to_radar_coord ();

/*	If the radar override latitude environment variable is set then	*
 *	we want to use it to define the radar latitude.			*/

	radar_override = getenv ("XPDT_RADAR_LATITUDE");

	if (radar_override != NULL) {

	    sscanf (radar_override,"%f",&radar_latitude);

	}

/*	If the radar override longitude environment variable is set	*
 *	then we want to use it to define the radar latitude.		*/

	radar_override = getenv ("XPDT_RADAR_LONGITUDE");

	if (radar_override != NULL) {

	    sscanf (radar_override,"%f",&radar_longitude);

	}

/*	Open the USGS data file.					*/

	fd = fopen (xpdt_map_filename, "r");

	if (fd == NULL) {

	    return;

	}

/*	Define the clip region for the data.				*/

	points = 0;

	clip_rectangle.x      = 0;
	clip_rectangle.y      = product_start_scanl;
	clip_rectangle.width  = 3*width/4;
	clip_rectangle.height = 3*width/4;

	XSetClipRectangles (display,
			    gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XSetForeground (display,
		gc, color [COLORS].pixel);

/*	Read and process all of the data in the file.			*/

	fread (inbuf, 1, 24, fd);

	while (feof (fd) == 0) {

/*		Extract the lat/lon coordinate of each point, convert	*
 *		it to azimuth/range from the radar, convert it to a	*
 *		window coordinate and then display, one segment at a	*
 *		time.							*/

		i1 = (int) inbuf [1];
		i2 = (int) inbuf [2];

		if (i1 < 0)
			i1 = i1+256;
		if (i2 < 0)
			i2 = i2+256;

		number_of_points = 100*i1 + i2;

		i1 = (int) inbuf [17];
		if (i1>127)
			i1 = i1-256;
		i2 = (int) inbuf [18];
		if (i2>127)
			i2 = i2-256;
		i3 = (int) inbuf [19];
		if (i3>127)
			i3 = i3-256;
		i4 = (int) inbuf [20];
		if (i4>127)
			i4 = i4-256;

		first_longitude = 2.0*i1 + i2 +i3/60.0 + i4/3600.0;

		i1 = (int) inbuf [21];
		if (i1>127)
			i1 = i1-256;
		i2 = (int) inbuf [22];
		if (i2>127)
			i2 = i2-256;
		i3 = (int) inbuf [23];
		if (i3>127)
			i3 = i3-256;

		first_latitude = i1 +i2/60.0 + i3/3600.0;

		earth_to_radar_coord (first_latitude, first_longitude,
			radar_latitude, radar_longitude,
			&azimuth, &range);

		angle  = (azimuth-90.0) * DEGRAD;
		pixel1 = (range * cos (angle) ) * scale_x +
			pixel_offset + center_pixel;
		angle  = (azimuth+90.0) * DEGRAD;
		scanl1 = (range * sin (angle) ) * scale_y +
			scanl_offset + center_scanl;

		fread (inbuf,1,2*(number_of_points-1),fd);

		second_latitude  = first_latitude;
		second_longitude = first_longitude;

		for (i=1;i<number_of_points;i++) {

			i1 = (int) inbuf [i*2];
			if (i1 > 127)
				i1 = i1 - 256;
			i2 = (int) inbuf [i*2+1];
			if (i2 > 127)
				i2 = i2 - 256;

			second_latitude  = second_latitude  + i2/3600.0;
			second_longitude = second_longitude + i1/3600.0;

			earth_to_radar_coord (second_latitude, second_longitude,
				 radar_latitude, radar_longitude,
				&azimuth, &range);

			angle  = (azimuth-90.0) * DEGRAD;
			pixel2 = (range * cos (angle) ) * scale_x +
				pixel_offset + center_pixel;
			angle  = (azimuth+90.0) * DEGRAD;
			scanl2 = (range * sin (angle) ) * scale_y +
				scanl_offset + center_scanl;

			XDrawLine (display, pixmap, gc,
				   pixel1, scanl1,
				   pixel2, scanl2);


			pixel1 = pixel2;
			scanl1 = scanl2;
			points++;

		}

		fread (inbuf, 1, 24, fd);

	}

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

	fclose (fd);

}

