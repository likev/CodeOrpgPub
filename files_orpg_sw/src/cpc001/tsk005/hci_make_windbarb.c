/************************************************************************
 *									*
 *	Module:	hci_make_windbarb.c					*
 *									*
 *	Description:    This routine is used by the HCI environmental	*
 *			winds editor to display winds symbolically.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:09 $
 * $Id: hci_make_windbarb.c,v 1.5 2009/02/27 22:26:09 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Macros.								*/

#define FLAG   	50.0 
#define FLAG1  	47.75 	/* flag for 50 kt transition */
#define BARB   	10.0
#define BARB1   7.75	/* flag for 10 kt transition */
#define HBARB   2.25	/* flag for 5  kt transition */
#define	MINIMUM_SPEED	0.0
#define	MAXIMUM_SPEED	250.0

/************************************************************************
 *	Description: This function constructs a standard wind barb	*
 *		     symbol for wind speeds up to 250 kts.  The staff	*
 *		     should be made long enough to account for all	*
 *		     pips required for this speed.  If larger speeds	*
 *		     are to be supported then a longer staff must be	*
 *		     defined.						*
 *									*
 *	Input:	wind_spd - the speed of the wind (unitless)		*
 *		wind_dir - direction the wind is from (degrees)		*
 *		first_x  - pixel coordinate of base of staff		* 
 *		first_y  - scanline coordinate of base of staff		*
 *		leng	 - length of staff (in pixels)			*
 *									*
 *	Output:	windbarb [] - Array of segments defining elements of	*
 *			   windbarb symbol				*
 *		ipts	 - Number of segments making up windbarb	*
 *			   symbol					*
 ************************************************************************/

void
hci_make_windbarb (
		XSegment windbarb[],	/*  windbarb segments		*/
		float wind_spd,		/*  Wind speed (unitless)	*/
                float wind_dir,		/*  Wind direction (degrees)	*/
		int firstx,		/*  Pixel reference		*/
                int firsty,		/*  Scanline reference		*/
		float leng,		/*  Length if staff (pixels)	*/
		int *ipts		/*  Segments in symbol		*/
)
{
 
	int	ibarb;
	int	iflag;
	int	ihalf;
	int	k;
	int	i;
	float	fval1;
	float	fval2;
	float	wspd;
	float	wdir1;
	float	wdir2;
	float	cs1;
	float	cs2;
	float	sn1;
	float	sn2;

/*	Check to see if wind speed within accepted tolerance.  If not,	*
 *	set the number of segments to 0 and return.			*/

	if (wind_spd <= MINIMUM_SPEED || wind_spd >= MAXIMUM_SPEED) {

	*ipts = 0;
	return;

}

/*	Convert the direction from degrees to radians.  Also the angle	*
 *	of the pips from the staff is 60 degrees.			*/

	wdir1          = HCI_DEG_TO_RAD*wind_dir;
	wdir2          = HCI_DEG_TO_RAD*(wind_dir+60.0);
	cs1            = cos(wdir1);
	cs2            = cos(wdir2);
	sn1            = sin(wdir1);
	sn2            = sin(wdir2);

/*	Construct the staff segment.					*/

	windbarb[0].x1 = firstx;
	windbarb[0].y1 = firsty;
	windbarb[0].x2 = windbarb[0].x1 + (leng*sn1);
	windbarb[0].y2 = windbarb[0].y1 - (leng*cs1);

	i     = 1;
	iflag = ibarb = ihalf = 0;
	wspd  = wind_spd;

/*	Determine the number of 50 unit bars for given speed		*/

	for (k = 0; k < 6; k++) {

	    if (wspd > FLAG1) {

		wspd = wspd - FLAG;
		iflag++;

	    }

	}

/*	Determine the number of 10 unit bars for given speed		*/

	for (k = 0; k < 5; k++) {
 
	    if (wspd > BARB1) {

		wspd = wspd - BARB;
		ibarb++;

	   }

	}

/*	Determine if a half barb is required				*/

	if (wspd > HBARB)
		ihalf = 1;   

/*	Determine the segments for the 50 barbs				*/

	for (k = 0; k < iflag; k++) {

	    fval1          = leng - ((float)(i-1) * 5.0);
	    fval2          = fval1 - 5.0;
	    windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
	    windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
	    windbarb[i].x2 = windbarb[i].x1+(10.0*sn2);
	    windbarb[i].y2 = windbarb[i].y1-(10.0*cs2);
	    i++;

	    windbarb[i].x1 = windbarb[0].x1 + (fval2*sn1);
	    windbarb[i].y1 = windbarb[0].y1 - (fval2*cs1);
	    windbarb[i].x2 = windbarb[i-1].x2;
	    windbarb[i].y2 = windbarb[i-1].y2;
	    i++;

	}

/*	Determine the segments for the 10 barbs				*/

	fval2 = 10.0;

	for (k = 0; k < ibarb; k++) {

	    fval1          = leng - ((float)(i-1) * 5.0);
	    windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
	    windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
	    windbarb[i].x2 = windbarb[i].x1+(fval2*sn2);
	    windbarb[i].y2 = windbarb[i].y1-(fval2*cs2);
	    i++;

	}

/*	Determine the segments for the 5 barb				*/

	fval1 = leng - ((float)(i-1) * 5.0);

	if (i == 1 && wspd < (HBARB * 2.0))
		fval1      = fval1 - 2.5;

	if (ihalf) {

	    fval2          = 5.0;
	    windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
	    windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
	    windbarb[i].x2 = windbarb[i].x1+(fval2*sn2);
	    windbarb[i].y2 = windbarb[i].y1-(fval2*cs2);
	    i++;
   
	}

    *ipts = i;

} /* end of make_windbarb */

