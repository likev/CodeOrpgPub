/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:21 $
 * $Id: coord_conv.c,v 1.2 2008/03/13 22:45:21 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************************
Module:		coord_conv.c
Description:	General support routines for coordinate conversion.

Logic taken from the rpgcs library

*****************************************************************************************/

#include <stdio.h>


/* #define FLOAT  // USE float instead of double */
#include "coord_conv.h"


/****************************************************************/
/* FROM rpgcs_latlon.c, modified for CVG
 */
/* //#include <orpgsite.h>  // DO NOT USE THIS */
#include <math.h>
/* //#include <a309.h>  // WHY DO I NEED THIS? */

/* #define FLOAT    // WHY IS THIS DEFINED FOR LATLON? */
/* //#include <rpgcs_coordinates.h> */
/* //#include <rpgcs_latlon.h> */
/******************************************************************/


/********************************************************************
 * FROM rpgcs_coordinates.c, modified for CVG
 */
/*  #define TYPE */
/* //#include <rpgcs_coordinates.h> */

static int In_units = METERS;
/* //static REAL In_scale = 1.0; */

/* //static REAL Out_scale = 1.0; */
/* //static int Out_units = METERS; */

/* static REAL Scale_factor = 1.0; */
static float Scale_factor = 1.0;

/* Function Prototypes. */
/* #define LENGTH          1 */ /* a value for "type" */
/* #define ANGLE           2 */ /* a value for "type" */
/* //static int Validate_units( int units, int type, char *err_str ); */

/*********************************************************************/


/* ///////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////// */
/*  ALL latlon FUNCTIONS FROM rpgcs_latlon.c */

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
int _88D_xy_to_latlon( float x, float y, float *lat, float *lon ){

   double zlat_rda, zlon_rda;
   double zlat_pos, zlon_pos;
   double zsin_s, zcos_s;
   double zp, zsin_th, zcos_th;
   double zsin_latpos, zcos_latpos, zlon_delta;
   double s_latitude, s_longitude;

/*  CVG - NEED TO USE THE RADAR LOCATION IN THE PRODUCT HEADER INSTEAD!!!! */
   /* GET THE RADAR'S LATITUDE AND LONGITUDE. */
/* //   s_latitude =  */
/* //      ((double) ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE )) / ZCNV2THNTHS; */
/* //   s_longitude =  */
/* //      ((double) ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE )) / ZCNV2THNTHS; */
   int   offset, word;
/*  TEMPORARY use KMLB */
/*    s_latitude = 28.113; */
/*    s_longitude = -80.654; */
   
   offset = RADAR_POS_OFFSET;
   word = read_word(sd->icd_product, &offset);
   s_latitude = (float)word / 1000.0;
   word = read_word(sd->icd_product, &offset);
   s_longitude = (float)word / 1000.0;

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

/* End of _88D_xy_to_latlon() */
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
int _88D_azran_to_latlon( float rng, float azm, float *lat, float *lon ){

   float x, y;

   /* First convert azran to x/y. */
   _88D_azran_to_xy( rng, azm, &x, &y );

   /* Now convert x/y to latlon. */
   _88D_xy_to_latlon( x, y, lat, lon );

   return 0;

/* End of _88D_azran_to_latlon() */ 
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
int _88D_latlon_to_xy( float lat, float lon, float *x, float *y ){

   double zlat_rda, zlon_rda;
   double zlat_pos, zlon_pos;
   double zterm_a, zterm_b;
   double zsin_s, zcos_s;
   double zd;
   double s_latitude, s_longitude;

/*  CVG - NEED TO USE THE RADAR LOCATION IN THE PRODUCT HEADER INSTEAD!!!!   */
/*    s_latitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE )*ZCNV2DEG; */
/*    s_longitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE )*ZCNV2DEG; */
   int   offset, word;
/*  TEMPORARY use KMLB */
/*    s_latitude = 28.113; */
/*    s_longitude = -80.654; */
   
   offset = RADAR_POS_OFFSET;
   word = read_word(sd->icd_product, &offset);
   s_latitude = (float)word / 1000.0;
   word = read_word(sd->icd_product, &offset);
   s_longitude = (float)word / 1000.0;
 
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

/* End of _88D_latlon_to_xy() */
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
int _88D_latlon_to_azran( float lat, float lon, float *rng, float *azm ){

   float x, y;

   /* First convert the lat/lon to x/y. */
   _88D_latlon_to_xy( lat, lon, &x, &y );

   /* Then convert x/y to azran. */
   _88D_xy_to_azran( x, y, rng, azm );

   return 0;

/* End of _88D_latlon_to_azran() */
}



/* ///////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////// */
/*  THE FOLLOWING FUNCTIONS FROM rpgcs_coordinates.c */

/* // NOT USED BY CVG */
/* //*/ /**********************************************************************/
/* // */
/* //   Description: */
/* //      Converts Cartesian x and y to azimuth and range. */
/* // */
/* //   Inputs: */
/* //      x - X coordinate (negative is west, positive is east) */
/* //      y - Y coordinate (negative is south, positive is north) */
/* //      xy_units - units of x and y (either METERS, KFEET, or NMILES) */
/* //      range_units - units of range (either METERS, KFEET, or NMILES) */
/* // */
/* //   Outputs: */
/* //      range - magnitude of x and y, in same units as x and y */
/* //      azm - angle subtended from north to (x,y), in degrees. */
/* // */
/* //   Returns: */
/* //      Currently, always returns 0. */
/* // */
/* /*/ /************************************************************************/
/* //int _88D_xy_to_azran_u( REAL x, REAL y, int xy_units, REAL *range,  */
/* //                         int range_units, REAL *azm ){ */
/* // */
/* //   int ret; */
/* // */
/* //   */ /* Save the current settings for input/output units. */
/* //   int save_in = _88D_get_input_units(); */
/* //   int save_out = _88D_get_output_units(); */
/* // */
/* //   */ /* Validate the input/output units. */
/* //   if( (ret = Validate_units( xy_units, LENGTH, "xy_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   */ /* Set the input/output units. */
/* //   _88D_set_input_units( xy_units ); */
/* //   _88D_set_output_units( range_units );  */
/* // */
/* //   */ /* Do the conversion. */
/* //   ret = _88D_xy_to_azran( x, y, range, azm ); */
/* // */
/* //   */ /* Reset the input/output units. */
/* //   _88D_set_input_units( save_in ); */
/* //   _88D_set_output_units( save_out );  */
/* //    */
/* //   return(ret); */
/* // */
/* //} */



/**********************************************************************

   Description:
      Converts Cartesian x and y to azimuth and range.

   Inputs:
      x - X coordinate (negative is west, positive is east)
      y - Y coordinate (negative is south, positive is north)

   Outputs:
      range - magnitude of x and y, in same units as x and y
      azm - angle subtended from north to (x,y), in degrees.

   Returns:
      Currently, always returns 0.

************************************************************************/
/* int _88D_xy_to_azran( REAL x, REAL y, REAL *range, REAL *azm ){ */
int _88D_xy_to_azran( float x, float y, float *range, float *azm ){
    
   /* The range is the magnitude of x and y. */
   *range = sqrt( (x*x) + (y*y) );

   /* Apply scaling factor to put in proper units. */
   *range *= Scale_factor;  /* CVG - Scale_factor always 1.0 for units of meters */

   /* The azimuth is generated from arctan.  To avoid EDOM error,
      set azm to 0.0 if both arctan arguments are 0.0. */
   if( (x == 0.0) && (y == 0.0) )
      *azm = 0.0;

   else
      *azm = atan2( x, y );

   /* Make sure the azimuth returned is positive. */
   if( *azm < 0.0 )
      *azm = *azm + TWOPI;

   /* Convert to degrees from radians. */
   *azm = (*azm)*RTD;

   return 0;

}


/* // NOT USED BY CVG */
/* //*/ /**********************************************************************/
/* // */
/* //   Description: */
/* //      Converts azmimuth, range, and elevation to Cartesian x and y.  */
/* // */
/* //   Inputs: */
/* //      range - magnitude of x and y. */
/* //      range_units - units of range (either METERS, KFEET, or NMILES) */
/* //      azm - angle subtended from north to (x,y), in degrees. */
/* //      azm_units - units of azm (either DEG or DEG10) */
/* //      elev - elevation angle, in degrees */
/* //      elev_units - units of elev (either DEG or DEG10) */
/* //      xy_units - units of x and y (either METERS, KFEET, or NMILES) */
/* // */
/* //   Outputs: */
/* //      x - X coordinate (negative is west, positive is east), in */
/* //          units determined by Out_scale and In_scale. */
/* //      y - Y coordinate (negative is south, positive is north), in */
/* //          units determined by Out_scale and In_scale. */
/* // */
/* //   Returns: */
/* //      If elevation is out of bounds, returns -1.  Otherwise, returns 0. */
/* // */
/* /*/ /************************************************************************/
/* //int _88D_azranelev_to_xy_u( REAL range, int range_units, REAL azm,  */
/* //                             int azm_units, REAL elev, int elev_units, */
/* //                             REAL *x, REAL *y, int xy_units ){ */
/* //   int ret; */
/* // */
/* //   */ /* Save the current settings for input/output units. */
/* //   int save_in = _88D_get_input_units(); */
/* //   int save_out = _88D_get_output_units(); */
/* // */
/* //   */ /* Validate the input/output units. */
/* //   if( (ret = Validate_units( xy_units, LENGTH, "xy_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   */ /* Validate the azimuth and elevation units. */
/* //   if( (ret = Validate_units( azm_units, ANGLE, "azm_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( azm_units == DEG10 ) */
/* //      azm /= 10.0; */
/* // */
/* //   if( (ret = Validate_units( elev_units, ANGLE, "elev_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( elev_units == DEG10 ) */
/* //      elev /= 10.0; */
/* // */
/* //   */ /* Set the input/output units. */
/* //   _88D_set_input_units( range_units ); */
/* //   _88D_set_output_units( xy_units );  */
/* // */
/* //   */ /* Do the conversion. */
/* //   ret = _88D_azranelev_to_xy( range, azm, elev, x, y ); */
/* // */
/* //   */ /* Reset the input/output units. */
/* //   _88D_set_input_units( save_in ); */
/* //   _88D_set_output_units( save_out );  */
/* //    */
/* //   return(ret); */
/* // */
/* //} */



/**********************************************************************

   Description:
      Converts azmimuth, range, and elevation to Cartesian x and y. 

   Inputs:
      range - magnitude of x and y.
      azm - angle subtended from north to (x,y), in degrees.
      elev - elevation angle, in degrees

   Outputs:
      x - X coordinate (negative is west, positive is east), in
          units determined by Out_scale and In_scale.
      y - Y coordinate (negative is south, positive is north), in
          units determined by Out_scale and In_scale.

   Returns:
      If elevation is out of bounds, returns -1.  Otherwise, returns 0.

************************************************************************/
/* int _88D_azranelev_to_xy( REAL range, REAL azm, REAL elev, */
/*                            REAL *x, REAL *y ){ */
int _88D_azranelev_to_xy( float range, float azm, float elev,
                           float *x, float *y ){
                            
/*    REAL cosine; */
   float cosine;

   /* If azimuth is negative, make positive in range 0 to 360.0. */
   while( azm < 0.0 )
      azm += 360.0;

   /* Ensure elevation angle is between -90 and +90. */
   if( (elev < -90.0) || (elev > 90.0) )
      return(-1);

   /* Convert azm and elev to radians. */
   azm = azm*DTR;
   elev = elev*DTR;

   /* Compute the Cartesian x and y. */
   cosine = range*cos(elev)*Scale_factor;  /* CVG - Scale_factor always 1.0 for units of meters */
   *x = sin(azm)*cosine;
   *y = cos(azm)*cosine;

   return 0;

}


/* // NOT USED BY CVG */
/* //*/ /**********************************************************************/
/* // */
/* //   Description: */
/* //      Converts azmimuth and range to Cartesian x and y.  */
/* // */
/* //   Inputs: */
/* //      range - magnitude of x and y. */
/* //      range_units - units of range (either METERS, KFEET, or NMILES) */
/* //      azm - angle subtended from north to (x,y), in degrees. */
/* //      azm_units - units of azm (either DEG or DEG10) */
/* //      xy_units - units of x and y (either METERS, KFEET, or NMILES) */
/* // */
/* //   Outputs: */
/* //      x - X coordinate (negative is west, positive is east), in */
/* //          units determined by Out_scale and In_scale. */
/* //      y - Y coordinate (negative is south, positive is north), in */
/* //          units determined by Out_scale and In_scale. */
/* // */
/* //   Returns: */
/* //      If elevation is out of bounds, returns -1.  Otherwise, returns 0. */
/* // */
/* /*/ /************************************************************************/
/* //int _88D_azran_to_xy_u( REAL range, int range_units, REAL azm,  */
/* //                         int azm_units, REAL *x, REAL *y, int xy_units ){ */
/* //   int ret; */
/* // */
/* //   */ /* Save the current settings for input/output units. */
/* //   int save_in = _88D_get_input_units(); */
/* //   int save_out = _88D_get_output_units(); */
/* // */
/* //   */ /* Validate the input/output units. */
/* //   if( (ret = Validate_units( xy_units, LENGTH, "xy_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   */ /* Validate the azimuth units. */
/* //   if( (ret = Validate_units( azm_units, ANGLE, "azm_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( azm_units == DEG10 ) */
/* //      azm /= 10.0; */
/* // */
/* //   */ /* Set the input/output units. */
/* //   _88D_set_input_units( range_units ); */
/* //   _88D_set_output_units( xy_units );  */
/* // */
/* //   */ /* Do the conversion. */
/* //   ret = _88D_azran_to_xy( range, azm, x, y ); */
/* // */
/* //   */ /* Reset the input/output units. */
/* //   _88D_set_input_units( save_in ); */
/* //   _88D_set_output_units( save_out );  */
/* //    */
/* //   return(ret); */
/* // */
/* //} */



/**********************************************************************

   Description:
      Converts azmimuth and range to Cartesian x and y.

   Inputs:
      range - magnitude of x and y, in same units as x and y.
      azm - angle subtended from north to (x,y), in degrees.

   Outputs:
      x - X coordinate (negative is west, positive is east), in
          same units as range.
      y - Y coordinate (negative is south, positive is north), in
          same units as range.

   Returns:
      Currently, always returns 0.

************************************************************************/
/* int _88D_azran_to_xy( REAL range, REAL azm, REAL *x, REAL *y ){ */
int _88D_azran_to_xy( float range, float azm, float *x, float *y ){

   /* If azmimuth is negative, make positive in range 0 to 360.0 */
   while( azm < 0.0 )
      azm += 360.0;

   /* Convert azm in degrees to radians. */
   azm = azm*DTR;

   /* Compute the Cartesian x and y. */
   range *= Scale_factor; /* CVG - Scale_factor always 1.0 for units of meters */
   *x = range*sin(azm);
   *y = range*cos(azm);

   return 0;

}


/* // NOT USED BY CVG */
/* //*/ /**********************************************************************/
/* // */
/* //   Description: */
/* //      Converts range and elevation to height.  */
/* // */
/* //   Inputs: */
/* //      range - slant range. */
/* //      range_units - units of range (either METERS, KFEET, or NMILES) */
/* //      elev - elevation angle, in degrees. */
/* //      elev_units - units of elev (either DEG or DEG10) */
/* //      height_units - units of height (either METERS, KFEET, or NMILES) */
/* // */
/* //   Outputs: */
/* //      height - height, in units of "height_units"  */
/* // */
/* //   Returns: */
/* //      If units are not validate, returns -1.  Otherwise, returns 0. */
/* // */
/* /*/ /************************************************************************/
/* //int _88D_get_height_u( REAL range, int range_units, REAL elev,  */
/* //                    int elev_units, REAL *height, int height_units ){ */
/* //   int ret; */
/* // */
/* //   */ /* Save the current settings for input/output units. */
/* //   int save_in = _88D_get_input_units(); */
/* //   int save_out = _88D_get_output_units(); */
/* // */
/* //   */ /* Validate the input/output units. */
/* //   if( (ret = Validate_units( height_units, LENGTH, "height_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   */ /* Validate the elevation units. */
/* //   if( (ret = Validate_units( elev_units, ANGLE, "elev_units" )) < 0 ) */
/* //      return(ret); */
/* // */
/* //   if( elev_units == DEG10 ) */
/* //      elev /= 10.0; */
/* // */
/* //   */ /* Set the input/output units. */
/* //   _88D_set_input_units( range_units ); */
/* //   _88D_set_output_units( height_units );  */
/* // */
/* //   */ /* Do the conversion. */
/* //   ret = _88D_get_height( range, elev, height ); */
/* // */
/* //   */ /* Reset the input/output units. */
/* //   _88D_set_input_units( save_in ); */
/* //   _88D_set_output_units( save_out );  */
/* //    */
/* //   return(ret); */
/* // */
/* //} */



/************************************************************************

   Description:
      Computes the height given range and elev.

   Inputs:
      range - slant range.
      elev - elevation ange, in degrees.

   Outputs:
      height - height, in "units".

   Returns:
      Returns -1
         if elev is out of bounds.

************************************************************************/
/* int _88D_get_height( REAL range, REAL elev, REAL *height ){ */
int _88D_get_height( float range, float elev, float *height ){
    
/*    REAL earth; */
   float earth;

   /* Ensure elevation is between -90.0 and +90.0 degrees. */
   if( (elev < -90.0) || (elev > 90.0) )
      return(-1);

   /* Convert the elevation to radians. */
   elev = elev*DTR;

   /* Set the effective earth's radius is appropriate scale. */
   earth = EA * In_units;  /* CVG - In_units always 1.0 for meters */

   /* Compute height using the required height equation. */
   *height = range*sin(elev) + (range*range)/(2.0*earth);

   /* Put in the proper units. */
   *height *= Scale_factor;  /* CVG - Scale_factor always 1.0 for units of meters */

   return 0;

}


/* // NOT USED BY CVG */
/* //*/ /***********************************************************************/
/* // */
/* //   Description: */
/* //      Sets the input units for coordinate transformations. */
/* // */
/* //   Inputs: */
/* //      units - either METERS, KFEET or NMILES */
/* // */
/* //   Notes: */
/* //      If units is neither METERS, KFEET, or NMILES, function call */
/* //      has no effect. */
/* // */
/* /*/ /************************************************************************/
/* //void _88D_set_input_units( int units ){ */
/* // */
/* //   if( units == METERS ){ */
/* // */
/* //      In_units = METERS; */
/* //      In_scale = 1.0; */
/* // */
/* //   } */
/* //   else if( units == KFEET ){ */
/* // */
/* //      In_units = KFEET; */
/* //      In_scale = M_TO_KFT; */
/* // */
/* //   } */
/* //   else if( units == NMILES ){ */
/* // */
/* //      In_units = NMILES; */
/* //      In_scale = M_TO_NMI; */
/* // */
/* //   } */
/* // */
/* //   */ /* Set the scale factor. */
/* //   Scale_factor = Out_scale/In_scale; */
/* // */
/* //} */
/* // */
/* //*/ /***********************************************************************/
/* // */
/* //   Description: */
/* //      Gets the current value of input units. */
/* // */
/* //   Returns: */
/* //      either METERS, KFEET, or NMILES */
/* // */
/* /*/ /************************************************************************/
/* //int _88D_get_input_units( ){ */
/* // */
/* //   return( In_units ); */
/* // */
/* //} */



/* // NOT USED BY CVG */
/* //*/ /***********************************************************************/
/* // */
/* //   Description: */
/* //      Sets the output units for coordinate transformations. */
/* // */
/* //   Inputs: */
/* //      units - either METERS, KFEET, or NMILES */
/* // */
/* //   Notes: */
/* //      If units is neither METERS, KFEET, or NMILES, function call */
/* //      has no effect. */
/* // */
/* /*/ /************************************************************************/
/* //void _88D_set_output_units( int units ){ */
/* // */
/* //   if( units == METERS ){ */
/* // */
/* //      Out_units = METERS; */
/* //      Out_scale = 1.0; */
/* // */
/* //   } */
/* //   else if( units == KFEET ){ */
/* // */
/* //      Out_units = KFEET; */
/* //      Out_scale = M_TO_KFT; */
/* // */
/* //   } */
/* //   else if( units == NMILES ){ */
/* // */
/* //      Out_units = NMILES; */
/* //      Out_scale = M_TO_NMI; */
/* // */
/* //   } */
/* // */
/* //   */ /* Set the scale factor. */
/* //   Scale_factor = Out_scale/In_scale; */
/* // */
/* //} */
/* // */
/* //*/ /***********************************************************************/
/* // */
/* //   Description: */
/* //      Gets the current value of output units. */
/* // */
/* //   Returns: */
/* //      either METERS, KFEET, or NMILES */
/* // */
/* /*/ /************************************************************************/
/* //int _88D_get_output_units( ){ */
/* // */
/* //   return( Out_units ); */
/* // */
/* //} */

/* // NOT USED BY CVG */
/* //*/ /***********************************************************************/
/* // */
/* //   Description: */
/* //      Performs validation of "units" given "type". */
/* // */
/* //   Input: */
/* //      units - the units to validate */
/* //      type - type of units (either LENGTH or ANGLE) */
/* //      err_str - string to be included in error text. */
/* // */
/* //   Returns: */
/* //      -1 on error, or 0 otherwise. */
/* // */
/* /*/ /***********************************************************************/
/* //static int Validate_units( int units, int type, char *err_str ){ */
/* // */
/* //   if( type == LENGTH ){ */
/* // */
/* //      */ /* Validate the length units. */
/* //      if( (units != METERS) */
/* //                && */
/* //          (units != KFEET) */
/* //                && */
/* //          (units != NMILES)){ */
/* // */
/* //         LE_send_msg( GL_ERROR, "%s units Value (%d) Not Recognized\n", */
/* //                   units, err_str ); */
/* //         return(-1); */
/* // */
/* //      } */
/* // */
/* //      return 0; */
/* // */
/* //   } */
/* // */
/* //   else if( type == ANGLE ){ */
/* // */
/* //      if( (units != DEG10 ) */
/* //                 && */
/* //          (units != DEG)){ */
/* // */
/* //         LE_send_msg( GL_ERROR, "%s units Value (%d) Not Recognized\n", */
/* //                   units ); */
/* //         return(-1); */
/* // */
/* //      } */
/* // */
/* //      return 0; */
/* // */
/* //   } */
/* // */
/* //   else */
/* //      return 0; */
/* // */
/* //} */





/* ///////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////// */
/*  CVG TEST FUNCTION */

void test_coord_conv()
{

int ret;
/*  Test using KMLB data product; NM/degLon varies with Latitude     */
/*  60.0 NM  = 364,560 ft = 111,118 meters = 111.118 KM */
/*  at KMLB 28.113 deg N: 60.0 NM = 0.9975 deg Lat & 1.124 deg Lon */
/*  TEST NORTH */
float N_in_y_km = 111.118;  /*  KM */
float N_in_x_km = 0; 
float N_in_lat_d = 29.1105;  /*   (0.9975 deg N) */
float N_in_lon_d = -80.654; /*  same as KMLB */
float N_in_az_d = 360.0;
float N_in_ran_km = 111.118; /*  KM    */
/*  TEST WEST */
float W_in_y_km = 0;  /*  KM */
float W_in_x_km = -111.118; 
float W_in_lat_d = 28.113;  /*  same as KMLB */
float W_in_lon_d = -81.778; /*   (1.124 deg W) */
float W_in_az_d = 270.0;
float W_in_ran_km = 111.118; /*  KM  */
/*  TEST NORTH WEST */
float NW_in_y_km = 50.5;  /*  KM */
float NW_in_x_km = -50.5; 
float NW_in_az_d = 315.0;
float NW_in_ran_km = 71.4178; /*  KM  */
/*  TEST PROJECTION */
float in_elev = 45.0;

float out_x, out_y, out_lat, out_lon, out_az, out_ran, out_height;

float circum;

    circum = 12715.43 * PI;  
    fprintf(stderr, "\nEarth Polar Circumference in KM is %6.3f\n", circum);
    fprintf(stderr, "One deg Lat is %6.3f KM or %6.3f NM\n", circum/360, 
                                                     circum/360*CKM_TO_NM );

    circum = 12756.3 * PI;  
    fprintf(stderr, "\nEarth Eqatorial Circumference in KM is %6.3f\n", circum);
    fprintf(stderr, "One deg Lat is %6.3f KM or %6.3f NM\n", circum/360, 
                                                     circum/360*CKM_TO_NM );
    
          
    /*  TEST NORTH */
    fprintf(stderr, "\nTESTING NORTH POINT LAT LON FUNCTIONS\n");
    ret = _88D_latlon_to_xy( N_in_lat_d, N_in_lon_d, &out_x, &out_y );
    fprintf(stderr," in Lat = %6.3f, in Lon = %6.3f, out x is %6.3f, out y is %6.3f\n",
               N_in_lat_d, N_in_lon_d, out_x, out_y);
               
    ret = _88D_latlon_to_azran( N_in_lat_d, N_in_lon_d, &out_ran, &out_az );
    fprintf(stderr," in Lat = %6.3f, in Lon = %6.3f, out range is %6.3f, out azi is %6.3f\n",
               N_in_lat_d, N_in_lon_d, out_ran, out_az);
               
    ret = _88D_xy_to_latlon( N_in_x_km, N_in_y_km, &out_lat, &out_lon );
    fprintf(stderr," in x = %6.3f, in y = %6.3f, out Lat is %6.3f, out Lon is %6.3f\n",
               N_in_x_km, N_in_y_km, out_lat, out_lon);    
               
    ret = _88D_azran_to_latlon( N_in_ran_km, N_in_az_d, &out_lat, &out_lon );
    fprintf(stderr," in range = %6.3f, in azi = %6.3f, out Lat is %6.3f, out Lon is %6.3f\n",
               N_in_ran_km, N_in_az_d, out_lat, out_lon); 
    
    /*  TEST WEST */
    fprintf(stderr, "\nTESTING WEST POINT LAT LON FUNCTIONS\n");
    ret = _88D_latlon_to_xy( N_in_lat_d, N_in_lon_d, &out_x, &out_y );
    fprintf(stderr," in Lat = %6.3f, in Lon = %6.3f, out x is %6.3f, out y is %6.3f\n",
               W_in_lat_d, W_in_lon_d, out_x, out_y);
               
    ret = _88D_latlon_to_azran( N_in_lat_d, N_in_lon_d, &out_ran, &out_az );
    fprintf(stderr," in Lat = %6.3f, in Lon = %6.3f, out range is %6.3f, out azi is %6.3f\n",
               W_in_lat_d, W_in_lon_d, out_ran, out_az);
               
    ret = _88D_xy_to_latlon( N_in_x_km, N_in_y_km, &out_lat, &out_lon );
    fprintf(stderr," in x = %6.3f, in y = %6.3f, out Lat is %6.3f, out Lon is %6.3f\n",
               W_in_x_km, W_in_y_km, out_lat, out_lon);    
               
    ret = _88D_azran_to_latlon( N_in_ran_km, N_in_az_d, &out_lat, &out_lon );
    fprintf(stderr," in range = %6.3f, in azi = %6.3f, out Lat is %6.3f, out Lon is %6.3f\n",
               W_in_ran_km, W_in_az_d, out_lat, out_lon); 


    /*  TEST NORTH */
    fprintf(stderr, "\nTESTING NORTH POINT Polar/Rect FUNCTIONS\n");
    ret = _88D_xy_to_azran( N_in_x_km, N_in_y_km, &out_ran, &out_az );
    fprintf(stderr," in x = %6.3f, in y = %6.3f, out range is %6.3f, out azi is %6.3f\n",
               N_in_x_km, N_in_y_km, out_ran, out_az);
               
    ret = _88D_azran_to_xy( N_in_ran_km, N_in_az_d, &out_x, &out_y );
    fprintf(stderr, " in range = %6.3f, in azi = %6.3f, out x is %6.3f, out y is %6.3f\n",
               N_in_ran_km, N_in_az_d, out_x, out_y); 
        
    /*  TEST WEST    */
    fprintf(stderr, "\nTESTING WEST POINT Polar/Rect FUNCTIONS\n");
    ret = _88D_xy_to_azran( W_in_x_km, W_in_y_km, &out_ran, &out_az );
    fprintf(stderr," in x = %6.3f, in y = %6.3f, out range is %6.3f, out azi is %6.3f\n",
               W_in_x_km, W_in_y_km, out_ran, out_az);
               
    ret = _88D_azran_to_xy( W_in_ran_km, W_in_az_d, &out_x, &out_y );
    fprintf(stderr, " in range = %6.3f, in azi = %6.3f, out x is %6.3f, out y is %6.3f\n",
               W_in_ran_km, W_in_az_d, out_x, out_y);     
    
    /*  TEST NORTH WEST */
    fprintf(stderr, "\nTESTING NORTH WEST POINT Polar/Rect FUNCTIONS\n");
    ret = _88D_xy_to_azran( NW_in_x_km, NW_in_y_km, &out_ran, &out_az );
    fprintf(stderr," in x = %6.3f, in y = %6.3f, out range is %6.3f, out azi is %6.3f\n",
               NW_in_x_km, NW_in_y_km, out_ran, out_az);
               
    ret = _88D_azran_to_xy( NW_in_ran_km, NW_in_az_d, &out_x, &out_y );
    fprintf(stderr, " in range = %6.3f, in azi = %6.3f, out x is %6.3f, out y is %6.3f\n",
               NW_in_ran_km, NW_in_az_d, out_x, out_y);     

    /*  TEST PROJECTIONS     */
    ret = _88D_azranelev_to_xy( N_in_ran_km, N_in_az_d, in_elev,
                            &out_x, &out_y );                           
       
    ret = _88D_get_height( N_in_ran_km, in_elev, &out_height );    
      
    
} /*  end test_coord_conv() */


