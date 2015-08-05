/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:06 $
 * $Id: hci_find_azimuth.c,v 1.3 2009/02/27 22:26:06 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_find_azimuth.c					*
 *									*
 *	Description:  This module returns the azimuth angle (from	*
 *		      north) from a pair of points.  The second point	*
 *		      is the reference.					*
 *									*
 ************************************************************************/

/*	Local include file definitions.				*/

#include <hci.h>

/************************************************************************
 *	Description: This function is used to compute the azimuth/range	*
 *		     coordinate of a screen pixel element relative to	*
 *		     another screen coordinate (radar).			*
 *									*
 *	Input:  pixel     - pixel coordiate of point			*
 *		scanl     - scanline coordinate of point		*
 *		ref_pixel - reference pixel coordinate			*
 *		ref_scanl - reference scanline coordinate		*
 *	Output: NONE							*
 *	Return: azimuth angle (degrees)					*
 ************************************************************************/

float
hci_find_azimuth (
int	pixel,
int	scanl,
int	ref_pixel,
int	ref_scanl
)
{
	float	x1, y1, xr, yr;
	float	azimuth;

	xr = (float) ref_pixel;
	yr = (float) ref_scanl;
	x1 = (float) pixel;
	y1 = (float) scanl;

	if (pixel <= ref_pixel) {

	    if (scanl >= ref_scanl) {

/*	----------NW Quadrant----------		*/

		if (pixel == ref_pixel) {

		    azimuth = 180.0;

		} else {

		    azimuth = 270.0 + atan ((double) (yr-y1)/(xr-x1))/HCI_DEG_TO_RAD;

		}

	    } else {

/*	----------SW Quadrant----------		*/

		if (pixel == ref_pixel) {

		    azimuth = 0.0;

		} else {

		    azimuth = 270.0 - atan ((double) (y1-yr)/(xr-x1))/HCI_DEG_TO_RAD;

		}

	    }

	} else {

	    if (scanl <= ref_scanl) {

/*	----------NE Quadrant----------		*/

		if (scanl == ref_scanl) {

		    azimuth = 90.0;

		} else {

		    azimuth = 90.0 - atan ((double) (yr-y1)/(x1-xr))/HCI_DEG_TO_RAD;

		}

	    } else {

/*	----------SE Quadrant----------		*/

		azimuth = 90.0 + atan ((double) (y1-yr)/(x1-xr))/HCI_DEG_TO_RAD;

	    }

	}

	return azimuth;

}
