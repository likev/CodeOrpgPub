/*
 * RCS info
 * $Author: steves $
 * $Date: 2003/07/09 16:21:17 $
 * $Locker:  $
 * $Id: superob_path.c,v 1.4 2003/07/09 16:21:17 steves Exp $
 * $revision$
 * $state$
 * $Logs$
 */


/************************************************************************
 *      Module:  superob_path.c                                          *
 *                                                                       *
 *      Description:  This module is used by the SuperOb to  calculate   *
 *                    latitude, longitude, height and azimuth of OB      *
 *                                                                       *
 *      Input:	      rangebar: mean radial range of cell                *
 *                    azimuthbar: mean azimuth of cell			 *
 *                    tiltsbar: mean tilt angle of cell      		 *
 *                    stalat:   lat of radar site			 *
 *                    stalon:   lon of radar site                        *
 *                    stahgt:   height of radar site			 *
 *     Output:        location: structure describing lat, lon, height,   *
 *                    azimuth of each cell				 *
 *     Return:        none						 *
 ************************************************************************/

/*   System include file                                              */

#include <stdio.h>
#include <math.h>
#include <rpgc.h>

/*   local include file						     */

#include "superob_path.h"

#define	EARTH_RADIUS 		6371200.      /* radius of Earth in m*/
#define DEG2RAD 		atan(1.0)/45.0/* convert deg to radial*/ 
#define RAD2DEG 		45.0/atan(1.0)/* convert radial to deg*/ 
#define ELEV_MIN 		0.75	      /* convert radial to deg*/ 
  

void 
superob_path (double rangebar,double azimuthbar,double tiltsbar,float stalat,
              float stalon, float stahgt, position_t *location)
{
 
  /* declare local variables					       */
  float re=EARTH_RADIUS+stahgt; /* radius of Earth plus height of radar*/
  float r43=4.0*re/3.0;         /* 4/3 of Earth radius		       */
  int   nsubzero=0;

  float selev0=0;		/* sin of tiltsbar		       */
  float celev0=0;               /* cos of tiltsbar		       */
  float slat0=0;                /* sin of latitude of radar	       */
  float clat0=0;                /* cos of latitude of radar	       */
  float sinaz=0;                /* sin fo azimuthbar		       */
  float cosaz=0;		/* cos of azimuthbar		       */
  float sinphi=0;               /* sin of latitude of cell             */
  float cosphi=0;               /* cos of latitude of cell             */
  float gamma=0;                /* distance from radar to cell in radial*/
  float cosgam=0;               /* cos of gamma			       */
  float singam=0;               /* sin of gamma			       */
  float saz1=0;			/* sin of azimuth angle at cell        */
  float caz1=0;			/* cos of azimuth angle at cell        */
  float slon1;			/* sin of longitude at cell	       */ 
  float clon1;			/* cos of longitude at cell	       */ 
  float selev;			/* sin of elevation angle at cell      */
  float celev;			/* cos of elevation angle at cell      */
  float elev;                   /* elevation angle at cell	       */
  /* temperory variables					       */
  float b, c, epsh, h, ha, z, zerr, zsub;

  /*Calculate the height of ob   					*/
  selev0=sin(tiltsbar*DEG2RAD);
  celev0=cos(tiltsbar*DEG2RAD);
  b=rangebar*(rangebar+2.0*re*selev0);
  c=sqrt(re*re+b);
  ha=b/(re+c);
  epsh=(rangebar*rangebar-ha*ha)/(8.0*re);
  h=ha-epsh;
  z=stahgt+h;

  zerr=0.5*fabs(epsh);
  if(fabs(tiltsbar)<ELEV_MIN)
  zerr=fabs(h);
  zsub=z-zerr;


  /* use 4/3rds rule to get elevation of radar beam			*
   * if( local temperature available, then vertical			*
   * position can estimated with greater accuracy			*/
  if(zsub<0.0)
   {
   nsubzero=nsubzero+1;
   RPGC_log_msg(GL_INFO, "There are %d points under ground\n", nsubzero); 
   }
   else
   {
   /* calculate the elevation angle at each cell			*/
   celev=r43*celev0/(r43+h);
   selev=(rangebar*rangebar+h*h+2.0*r43*h)/(2.0*rangebar*(r43+h));
   elev=RAD2DEG*atan2(selev,celev);

  /* get lat, lon and azimuth angle at obs location using formulas for *
   * solution of spherical triangles				       *
   * calculate distance from radar to ob location in radials	       */
  gamma=0.5*rangebar*(celev0+celev)/EARTH_RADIUS;
  cosgam=cos(gamma);
  singam=sin(gamma);
  sinaz=sin(DEG2RAD*azimuthbar);
  cosaz=cos(DEG2RAD*azimuthbar);
  clat0=cos(DEG2RAD*stalat);
  slat0=sin(DEG2RAD*stalat);

  sinphi=cosgam*slat0+singam*clat0*cosaz;
  if(sinphi>1.0)
  sinphi=0.999;
  else if(sinphi<-1.0)
  sinphi=-0.999;

  /* calculate the sin and cos of lat at ob location		     */
  cosphi=cos(asin(sinphi));
  location->lat= asin(sinphi)*RAD2DEG;

  /* calculate the sin and cos of longitude difference between       *
   * station and ob location times the cosine of station latitude    */
  clon1=cosgam*clat0-singam*slat0*cosaz;
  slon1=singam*sinaz;

  /* calculate the sin and cos of the azimuth angle at ob location   *
   * times the cosine of radar latitude				     */
  caz1=cosgam*clat0*cosaz-singam*slat0;
  saz1=sinaz*clat0;
  
  /* assign the lat, lon, azimuth to struct			     */
  location->lon=stalon+RAD2DEG*atan2(slon1,clon1);
  location->azimuth=atan2(saz1,caz1)*RAD2DEG;
  location->height=z;

 }
}
