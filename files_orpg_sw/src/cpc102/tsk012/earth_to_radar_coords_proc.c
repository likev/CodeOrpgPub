/************************************************************************
 *	Module: earth_to_radar_coords_proc.c				*
 *	Description: This module is used to convert a earth coordinate	*
 *		     to azimuth/range coordiantes from a reference	*
 *		     earth coordinate.					*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:43 $
 * $Id: earth_to_radar_coords_proc.c,v 1.3 2001/05/22 18:13:43 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	System include file definitions.				*/

#include "math.h"

/*----------------------------------------------------------------------*
 *  This routine is used by the XPDT tool to calculate the distance     *
 *  between two earth coordinates in polar coordinates.  The input      *
 *  data are:								*
 *									*
 *	latitude		-  Target latitude (deg)		*
 *	longitude		-  Target longitude (deg)		*
 *	reference_latitude	-  Reference latitude (deg)		*
 *	reference_longitude	-  Reference longitude (deg)		*
 *									*
 *  The output from this routine are:					*
 *									*
 *	azimuth			-  Azimuthal angle of target from	*
 *				   reference (deg).			*
 *	range			-  Azimuthal distance of target from	*
 *				   reference (km).			*
 *----------------------------------------------------------------------*/

void
earth_to_radar_coord (latitude, longitude, reference_latitude,
		      reference_longitude, azimuth, range)

float	latitude;
float	longitude;
float	reference_latitude;
float	reference_longitude;
float	*azimuth;
float	*range;
{
	double	angle, temp1, temp2;
	double	a, b, c, a1, a2, alpha;

/*  Begin calculation now....						*/

	temp1 = (reference_latitude-latitude) * 3.14159265 / 180.0;
	a = fabs (temp1);
	temp1 = latitude * 3.14159265 / 180.0;
	temp2 = (reference_longitude-longitude) * 3.14159265 / 180.0;
	b = cos (temp1) * fabs (temp2);
	temp1 = cos (a) * cos (b);
	c = acos (temp1);

	a1 = sin (c);

	if (a1 <= 0.0) {

		*azimuth = 0.0;
		*range   = 0.0;

	} else {

		a2 = sin (b) / a1;

		if (a2 > 0.9999999) 

			a2 = 0.9999999;

		temp1 = cos (a) * a2;
		alpha = acos (temp1) / (3.14159265/180.0);
		*range = c * 111.12 / (3.14159265/180.0);

/*  Put azimuth angle in proper quadrants.....				*/

		if (latitude < reference_latitude) {

			if (longitude > reference_longitude) {

				*azimuth = 90.0 + alpha;

			} else {

				*azimuth = 270.0 - alpha;

			}

		} else {

			if (longitude > reference_longitude) {

				*azimuth = 90.0 - alpha;

			} else {

				*azimuth = 270.0 + alpha;

			}

		}

	}

}
