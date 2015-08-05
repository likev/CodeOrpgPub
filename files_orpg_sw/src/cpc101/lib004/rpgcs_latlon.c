#include <orpgsite.h>
#include <math.h>
#include <a309.h>

#define FLOAT
#include <rpgcs_coordinates.h>
#include <rpgcs_latlon.h>

/*********************************************************************
   Description:
      Given the x and y position relative to the radar, determine
      the latitude and longitude.

   Inputs:
      x - x distance, in kilometers.
      y - y distance, in kilometers.

   Outputs:
      lat - receives the latitude of the point 
      long - receives the longitude of the point

   Returns:
      This function always returns 0.

   Notes:

*********************************************************************/
int RPGCS_xy_to_latlon( float x, float y, float *lat, float *lon ){

   double zlat_rda, zlon_rda;
   double zlat_pos, zlon_pos;
   double zsin_s, zcos_s;
   double zp, zsin_th, zcos_th;
   double zsin_latpos, zcos_latpos, zlon_delta;
   double s_latitude, s_longitude;

   /* GET THE RADAR'S LATITUDE AND LONGITUDE. */
   s_latitude = 
      ((double) ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE )) / ZCNV2THNTHS;
   s_longitude = 
      ((double) ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE )) / ZCNV2THNTHS;

   /* COMPUTE THE RANGE TO THE OBJECT POSITION BASED ON
      THE X AND Y DISTANCES. */
   zp = sqrt( x*x + y*y );

   /* CHECK THE X AND Y DISTANCES BETWEEN THE RADAR AND
      THE OBJECT POSITIONS ARE NEARLY ZERO. */
   if( zp < 0.1){
 
      /*  THE X AND Y DISTANCE VALUES BETWEEN THE RADAR AND
          THE OBJECT POSITIONS ARE BOTH EQUAL TO ZERO.
          THE OBJECT POSITION LATITUDE AND LONGITUDE
          ARE SET TO THE RADAR LATITUDE AND LONGITUDE. */
     *lat = (float) s_latitude;
     *lon = (float) s_longitude;

   }
   else{
 
      /* THE X AND Y DISTANCE VALUES DO NOT BOTH EQUAL
         ZERO.  COMPUTE THE OBJECT POSITION LATITUDE AND
         LONGITUDE VALUES WITH THE FOLLOWING.  FIRST,
         COMPUTE THE LATITUDE OF THE RADAR LOCATION IN
         RADIANS BASED ON THE VALUE IN THOUSANDTHS OF
         DEGREES. */
      zlat_rda = ZDEG2RAD*s_latitude;
 
      /* COMPUTE THE LONGITUDE OF THE RADAR LOCATION IN
         RADIANS BASED ON THE VALUE IN THOUSANDTHS OF
         DEGREES. */
      zlon_rda = ZDEG2RAD*s_longitude;
 
 
      /* COMPUTE THE "SIN THETA" AND THE "COS THETA"
         INTERMEDIATE TERMS WITH THE FOLLOWING. */
      zsin_th = x/zp;
      zcos_th = y/zp;
 
      /* COMPUTE THE INTERMEDIATE "SIN" AND "COS" TERMS
         WITH THE FOLLOWING. */
      zsin_s = (zp/ZCONST2)*(ZCONST3-((ZCONST1*zp)/(ZCONST2*ZCONST2)));
      zcos_s = sqrt(ZCONST3-(zsin_s*zsin_s));
 
      /* COMPUTE THE "SIN" OF THE LATITUDE OF THE
         OBJECT POSITION WITH THE FOLLOWING. */
      zsin_latpos = sin( zlat_rda )*zcos_s + cos( zlat_rda )*zsin_s*zcos_th;
 
      /* COMPUTE THE "COS" OF THE LATITUDE OF THE
         OBJECT POSITION WITH THE FOLLOWING. */
      zcos_latpos = sqrt( ZCONST3-(zsin_latpos*zsin_latpos) );
 
      /* DETERMINE THE INTERMEDIATE "LONGITUDE DELTA" TERM. */
      zlon_delta = asin( zsin_s * zsin_th / zcos_latpos );
 
      /* COMPUTE THE LATITUDE OF THE OBJECT POSITION IN
         RADIANS USING THE ARCTANGENT FUNCTION. */
      zlat_pos = atan2( zsin_latpos, zcos_latpos );
 
      /* COMPUTE THE LONGITUDE OF THE OBJECT POSITION IN
         RADIANS WITH THE FOLLOWING. */
      zlon_pos = zlon_rda + zlon_delta;
 
      /* COMPUTE THE LATITUDE OF THE OBJECT IN THOUSANDTHS
         OF DEGREES (I.E., DEGREES TIMES 1000) AND ASSIGN
         THE VALUE TO THE FIRST ELEMENT OF THE ARRAY.
         THE VALUE IS ROUNDED WITH THE ANINT FUNCTION. */
 
      *lat = (float) (zlat_pos / ZDEG2RAD); 
 
      /* COMPUTE THE LONGITUDE OF THE OBJECT IN THOUSANDTHS
         OF DEGREES AND ASSIGN THE VALUE TO THE SECOND ELEMENT
         OF THE ARRAY.  THE VALUE IS ROUNDED WITH THE ANINT
         FUNCTION. */
      *lon = (float) (zlon_pos / ZDEG2RAD);

   }
 
   return 0;

/* End of RPGCS_xy_to_latlon() */
}

/*******************************************************************
   Description:
      Given the azran of a point relative to the radar, convert
      to latitude and longitude. 

   Inputs:
      rng - slant range, in kilometers.
      azm - azimuth angle, measured clockwise from north, in degrees

   Outputs:
      lat - latitude of point, in degrees.
      lon - longitude of point, in degrees.

   Returns:
      This function always returns 0.

   Notes:

*******************************************************************/
int RPGCS_azran_to_latlon( float rng, float azm, float *lat, float *lon ){

   float x, y;

   /* First convert azran to x/y. */
   RPGCS_azran_to_xy( rng, azm, &x, &y );

   /* Now convert x/y to latlon. */
   RPGCS_xy_to_latlon( x, y, lat, lon );

   return 0;

/* End of RPGCS_azran_to_latlon() */ 
}

/*******************************************************************
   Description:
      Converts latitude/longitude to x/y relative to the radar.  
      The x/y coordinates are in kilometers.

   Input:
      lat - latitude of point, in degrees.
      lon - longitude of point, in degrees.

   Output:
      x - receives the x position of the point, relative to the 
          radar, in kilometers.
      y - receives the y position of the point, relative to the 
          radar, in kilometers.

   Returns:
      This function always returns 0.

   Notes:

*******************************************************************/
int RPGCS_latlon_to_xy( float lat, float lon, float *x, float *y ){

   double zlat_rda, zlon_rda;
   double zlat_pos, zlon_pos;
   double zterm_a, zterm_b;
   double zsin_s, zcos_s;
   double zd;
   double s_latitude, s_longitude;

   s_latitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE )*ZCNV2DEG;
   s_longitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE )*ZCNV2DEG;
 
   /* COMPUTE THE LATITUDE OF THE RADAR SITE IN RADIANS (BASED
      ON THE RADAR SITE LATITUDE IN THOUSANDTHS OF DEGREES). */
   zlat_rda = ZDEG2RAD*s_latitude;
 
   /* COMPUTE THE LONGITUDE OF THE RADAR SITE IN RADIANS (BASED
      ON THE RADAR SITE LONGITUDE IN THOUSANDTHS OF DEGREES). */
   zlon_rda = ZDEG2RAD*s_longitude;
 
   /* COMPUTE THE LATITUDE OF THE OBJECT IN RADIANS (BASED
      ON THE POSITION LATITUDE IN THOUSANDTHS OF DEGREES). */
   zlat_pos = ZDEG2RAD*lat;
 
   /* COMPUTE THE LONGITUDE OF THE OBJECT IN RADIANS (BASED
      ON THE POSITION LONGITUDE IN THOUSANDTHS OF DEGREES). */
   zlon_pos = ZDEG2RAD*lon;
 
   /* COMPUTE THE INTERMEDIATE TERM "A" OF THE EQUATION. */
   zterm_a = cos( zlat_pos )*sin( zlon_pos - zlon_rda );
 
   /* COMPUTE THE INTERMEDIATE TERM "B" OF THE EQUATION. */
   zterm_b = (cos( zlat_rda )*sin( zlat_pos ))-
             (sin( zlat_rda )*cos( zlat_pos )*
             (cos( zlon_pos - zlon_rda )));
 
   /* COMPUTE THE FOLLOWING INTERMEDIATE "SIN" AND "COS" TERMS. */
   zsin_s = sqrt( zterm_a*zterm_a + zterm_b*zterm_b );
   zcos_s = sqrt( 1.0 - (zsin_s*zsin_s));
 
   /* COMPUTE THE INTERMEDIATE "ZD" TERM WITH THE FOLLOWING. */
   zd = ZCONST1*zsin_s+ZCONST2;
 
   /* COMPUTE THE DISTANCE IN THE X DIRECTION AND ASSIGN TO
      THE REAL VARIABLE.  THE DISTANCE IS IN KILOMETERS. */
   *x = (float) zd*cos( zlat_pos )*sin( zlon_pos - zlon_rda );

   /* COMPUTE THE DISTANCE IN THE Y DIRECTION AND ASSIGN TO
      A REAL VARIABLE.  THE DISTANCE IS KILOMETERS. */
   *y = (float) zd*(sin( zlat_pos )-(sin( zlat_rda )*zcos_s))/cos( zlat_rda );

   return 0;

/* End of RPGCS_latlon_to_xy() */
}

/*************************************************************************
   Description:
      Given the latitude and longitude, in degrees, returns the azimuth
      and range, in degrees and kilometers, respectively.

   Inputs:
      lat - latitude, in degrees.
      lon - longitude, in degrees.

   Outputs:
      rng - receives the slant range, in kilometers.
      azm - receives the azimuth, in degrees.

   Returns:
      This function always returns 0.

   Notes:

*************************************************************************/
int RPGCS_latlon_to_azran( float lat, float lon, float *rng, float *azm ){

   float x, y;

   /* First convert the lat/lon to x/y. */
   RPGCS_latlon_to_xy( lat, lon, &x, &y );

   /* Then convert x/y to azran. */
   RPGCS_xy_to_azran( x, y, rng, azm );

   return 0;

/* End of RPGCS_latlon_to_azran() */
}
