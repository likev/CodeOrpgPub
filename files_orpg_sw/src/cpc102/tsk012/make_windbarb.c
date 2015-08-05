/************************************************************************
 *	Module: make_windbarb.c						*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to build windbarb display data from	*
 *		     a wind direction and speed.			*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:13:53 $
 * $Id: make_windbarb.c,v 1.3 2001/05/22 18:13:53 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Motif & X windows include file definitions.			*/

#include <Xm/Xm.h>

/*	System include file definitions.				*/

#include <math.h>

#define	RAD	(3.14159265/180.0) /* degrees to radians conversion */

/************************************************************************
 *	Description: This functio is used to create the data for a	*
 *		     windbarb from an input wind direction and speed.	*
 *									*
 *	Input:  wind_spd - wind speed (kts)				*
 *		wind_dir - wind direction (degrees)			*
 *		firstx   - x position of first poiint			*
 *		firsty   - y position of first point			*
 *		leng     - length (pixels) if windbarb staff		*
 *	Output: windbarb - pointer to windbarb data.			*
 *		ipts     - number of points for windbarb data		*
 *	Return: NONE							*
 ************************************************************************/

void
make_windbarb (XSegment windbarb[], float wind_spd,
                float wind_dir, int firstx,
                int firsty, float leng, int *ipts)
{
    #define MFLAG   25.0       /*   metric  messurement for  */
    #define MFLAG1  23.75      /*   metric  messurement for  */
    #define MBARB    5.0       /*   metric  messurement for  */
    #define MBARB1   3.75      /*   metric  messurement for  */
    #define MHBARB   1.25      /*   metric  messurement for  */
    #define EFLAG   50.0       /*   english messurement for  */
    #define EFLAG1  47.75      /*   english messurement for  */
    #define EBARB   10.0       /*   english messurement for  */
    #define EBARB1   7.75      /*   english messurement for  */
    #define EHBARB   2.25      /*   english messurement for  */
 
    int    ibarb, iflag, ihalf, k, i;
    float  fval1, fval2, wspd, wdir1, wdir2, cs1, cs2, sn1, sn2;
    float  FLAG,FLAG1,BARB,BARB1,HBARB;

static    int	   english_units = 1;

    if (english_units)
    {
      FLAG  =  EFLAG;
      FLAG1 =  EFLAG1;
      BARB  =  EBARB;
      BARB1 =  EBARB1;
      HBARB =  EHBARB;
    }
    else
    {
      FLAG  =  MFLAG;
      FLAG1 =  MFLAG1;
      BARB  =  MBARB;
      BARB1 =  MBARB1;
      HBARB =  MHBARB;
    }

    if (wind_spd <= 0.0 || wind_spd >= 100.0)
    {
      *ipts = 0;
      return;
    }

    wdir1          = RAD*wind_dir;
    wdir2          = RAD*(wind_dir+60.0);
    cs1            = cos(wdir1);
    cs2            = cos(wdir2);
    sn1            = sin(wdir1);
    sn2            = sin(wdir2);
/*
    windbarb[0].x1 = firstx - (leng*sn1*0.5);
    windbarb[0].y1 = firsty + (leng*cs1*0.5);
    windbarb[0].y1 = firsty;
*/
    windbarb[0].x1 = firstx;
    windbarb[0].y1 = firsty;
    windbarb[0].x2 = windbarb[0].x1 + (leng*sn1);
    windbarb[0].y2 = windbarb[0].y1 - (leng*cs1);
    i              = 1;
    iflag = ibarb = ihalf = 0;

    wspd = wind_spd;

   for (k = 0; k < 5; k++)
   {
       if (wspd > FLAG1)
       {
          wspd = wspd - FLAG;
          iflag++;
       }
    }
    for (k = 0; k < 5; k++)
    {
       if (wspd > BARB1)
       {
          wspd = wspd - BARB;
          ibarb++;
       }
    }
    if (wspd > HBARB)
        ihalf = 1;   

    for (k = 0; k < iflag; k++)
    {
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
    fval2 = 10.0;
    for (k = 0; k < ibarb; k++)
    {
       fval1          = leng - ((float)(i-1) * 5.0);
       windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
       windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
       windbarb[i].x2 = windbarb[i].x1+(fval2*sn2);
       windbarb[i].y2 = windbarb[i].y1-(fval2*cs2);
       i++;
    }
    for (k = 0; k < ihalf; k++)
    {
       fval1          = leng - ((float)(i-1) * 5.0);
       if (i == 1 && wspd < (HBARB * 2.0))
           fval1      = fval1 - 2.5;
       fval2          = 5.0;
       windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
       windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
       windbarb[i].x2 = windbarb[i].x1+(fval2*sn2);
       windbarb[i].y2 = windbarb[i].y1-(fval2*cs2);
       i++;
    }
    *ipts = i;
} /* end of make_windbarb */

