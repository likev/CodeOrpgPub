#include <math.h>
#include <a309.h>

#define TYPE
#include <rpgcs_coordinates.h>

static int In_units = METERS;
static REAL In_scale = 1.0;

static REAL Out_scale = 1.0;
static int Out_units = METERS;

static REAL Scale_factor = 1.0;

/* Function Prototypes. */
#define LENGTH          1 /* a value for "type" */
#define ANGLE           2 /* a value for "type" */
static int Validate_units( int units, int type, char *err_str );

/**********************************************************************

   Description:
      Converts Cartesian x and y to azimuth and range.

   Inputs:
      x - X coordinate (negative is west, positive is east)
      y - Y coordinate (negative is south, positive is north)
      xy_units - units of x and y (either METERS, KFEET, or NMILES)
      range_units - units of range (either METERS, KFEET, or NMILES)

   Outputs:
      range - magnitude of x and y, in same units as x and y
      azm - angle subtended from north to (x,y), in degrees.

   Returns:
      Currently, always returns 0.

************************************************************************/
int RPGCS_xy_to_azran_u( REAL x, REAL y, int xy_units, REAL *range, 
                         int range_units, REAL *azm ){

   int ret;

   /* Save the current settings for input/output units. */
   int save_in = RPGCS_get_input_units();
   int save_out = RPGCS_get_output_units();

   /* Validate the input/output units. */
   if( (ret = Validate_units( xy_units, LENGTH, "xy_units" )) < 0 )
      return(ret);

   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 )
      return(ret);

   /* Set the input/output units. */
   RPGCS_set_input_units( xy_units );
   RPGCS_set_output_units( range_units ); 

   /* Do the conversion. */
   ret = RPGCS_xy_to_azran( x, y, range, azm );

   /* Reset the input/output units. */
   RPGCS_set_input_units( save_in );
   RPGCS_set_output_units( save_out ); 
   
   return(ret);

}

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
int RPGCS_xy_to_azran( REAL x, REAL y, REAL *range, REAL *azm ){

   /* The range is the magnitude of x and y. */
   *range = sqrt( (x*x) + (y*y) );

   /* Apply scaling factor to put in proper units. */
   *range *= Scale_factor;

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

/**********************************************************************

   Description:
      Converts azmimuth, range, and elevation to Cartesian x and y. 

   Inputs:
      range - magnitude of x and y.
      range_units - units of range (either METERS, KFEET, or NMILES)
      azm - angle subtended from north to (x,y), in degrees.
      azm_units - units of azm (either DEG or DEG10)
      elev - elevation angle, in degrees
      elev_units - units of elev (either DEG or DEG10)
      xy_units - units of x and y (either METERS, KFEET, or NMILES)

   Outputs:
      x - X coordinate (negative is west, positive is east), in
          units determined by Out_scale and In_scale.
      y - Y coordinate (negative is south, positive is north), in
          units determined by Out_scale and In_scale.

   Returns:
      If elevation is out of bounds, returns -1.  Otherwise, returns 0.

************************************************************************/
int RPGCS_azranelev_to_xy_u( REAL range, int range_units, REAL azm, 
                             int azm_units, REAL elev, int elev_units,
                             REAL *x, REAL *y, int xy_units ){
   int ret;

   /* Save the current settings for input/output units. */
   int save_in = RPGCS_get_input_units();
   int save_out = RPGCS_get_output_units();

   /* Validate the input/output units. */
   if( (ret = Validate_units( xy_units, LENGTH, "xy_units" )) < 0 )
      return(ret);

   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 )
      return(ret);

   /* Validate the azimuth and elevation units. */
   if( (ret = Validate_units( azm_units, ANGLE, "azm_units" )) < 0 )
      return(ret);

   if( azm_units == DEG10 )
      azm /= 10.0;

   if( (ret = Validate_units( elev_units, ANGLE, "elev_units" )) < 0 )
      return(ret);

   if( elev_units == DEG10 )
      elev /= 10.0;

   /* Set the input/output units. */
   RPGCS_set_input_units( range_units );
   RPGCS_set_output_units( xy_units ); 

   /* Do the conversion. */
   ret = RPGCS_azranelev_to_xy( range, azm, elev, x, y );

   /* Reset the input/output units. */
   RPGCS_set_input_units( save_in );
   RPGCS_set_output_units( save_out ); 
   
   return(ret);

}

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
int RPGCS_azranelev_to_xy( REAL range, REAL azm, REAL elev,
                           REAL *x, REAL *y ){

   REAL cosine;

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
   cosine = range*cos(elev)*Scale_factor;
   *x = sin(azm)*cosine;
   *y = cos(azm)*cosine;

   return 0;

}

/**********************************************************************

   Description:
      Converts azmimuth and range to Cartesian x and y. 

   Inputs:
      range - magnitude of x and y.
      range_units - units of range (either METERS, KFEET, or NMILES)
      azm - angle subtended from north to (x,y), in degrees.
      azm_units - units of azm (either DEG or DEG10)
      xy_units - units of x and y (either METERS, KFEET, or NMILES)

   Outputs:
      x - X coordinate (negative is west, positive is east), in
          units determined by Out_scale and In_scale.
      y - Y coordinate (negative is south, positive is north), in
          units determined by Out_scale and In_scale.

   Returns:
      If elevation is out of bounds, returns -1.  Otherwise, returns 0.

************************************************************************/
int RPGCS_azran_to_xy_u( REAL range, int range_units, REAL azm, 
                         int azm_units, REAL *x, REAL *y, int xy_units ){
   int ret;

   /* Save the current settings for input/output units. */
   int save_in = RPGCS_get_input_units();
   int save_out = RPGCS_get_output_units();

   /* Validate the input/output units. */
   if( (ret = Validate_units( xy_units, LENGTH, "xy_units" )) < 0 )
      return(ret);

   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 )
      return(ret);

   /* Validate the azimuth units. */
   if( (ret = Validate_units( azm_units, ANGLE, "azm_units" )) < 0 )
      return(ret);

   if( azm_units == DEG10 )
      azm /= 10.0;

   /* Set the input/output units. */
   RPGCS_set_input_units( range_units );
   RPGCS_set_output_units( xy_units ); 

   /* Do the conversion. */
   ret = RPGCS_azran_to_xy( range, azm, x, y );

   /* Reset the input/output units. */
   RPGCS_set_input_units( save_in );
   RPGCS_set_output_units( save_out ); 
   
   return(ret);

}

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
int RPGCS_azran_to_xy( REAL range, REAL azm, REAL *x, REAL *y ){

   /* If azmimuth is negative, make positive in range 0 to 360.0 */
   while( azm < 0.0 )
      azm += 360.0;

   /* Convert azm in degrees to radians. */
   azm = azm*DTR;

   /* Compute the Cartesian x and y. */
   range *= Scale_factor;
   *x = range*sin(azm);
   *y = range*cos(azm);

   return 0;

}

/**********************************************************************

   Description:
      Converts range and elevation to height. 

   Inputs:
      range - slant range.
      range_units - units of range (either METERS, KFEET, or NMILES)
      elev - elevation angle, in degrees.
      elev_units - units of elev (either DEG or DEG10)
      height_units - units of height (either METERS, KFEET, or NMILES)

   Outputs:
      height - height, in units of "height_units" 

   Returns:
      If units are not validate, returns -1.  Otherwise, returns 0.

************************************************************************/
int RPGCS_height_u( REAL range, int range_units, REAL elev, 
                    int elev_units, REAL *height, int height_units ){
   int ret;

   /* Save the current settings for input/output units. */
   int save_in = RPGCS_get_input_units();
   int save_out = RPGCS_get_output_units();

   /* Validate the input/output units. */
   if( (ret = Validate_units( height_units, LENGTH, "height_units" )) < 0 )
      return(ret);

   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 )
      return(ret);

   /* Validate the elevation units. */
   if( (ret = Validate_units( elev_units, ANGLE, "elev_units" )) < 0 )
      return(ret);

   if( elev_units == DEG10 )
      elev /= 10.0;

   /* Set the input/output units. */
   RPGCS_set_input_units( range_units );
   RPGCS_set_output_units( height_units ); 

   /* Do the conversion. */
   ret = RPGCS_height( range, elev, height );

   /* Reset the input/output units. */
   RPGCS_set_input_units( save_in );
   RPGCS_set_output_units( save_out ); 
   
   return(ret);

}

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
int RPGCS_height( REAL range, REAL elev, REAL *height ){

   REAL earth;

   /* Ensure elevation is between -90.0 and +90.0 degrees. */
   if( (elev < -90.0) || (elev > 90.0) )
      return(-1);

   /* Convert the elevation to radians. */
   elev = elev*DTR;

   /* Set the effective earth's radius is appropriate scale. */
   earth = EA * In_units;

   /* Compute height using the required height equation. */
   *height = range*sin(elev) + (range*range)/(2.0*earth);

   /* Put in the proper units. */
   *height *= Scale_factor;

   return 0;

}

/**********************************************************************

   Description:
      Computes the range given height and elev.

   Inputs:
      range - slant range.
      range_units - units of range (either METERS, KFEET, or NMILES)
      elev - elevation angle, in degrees.
      elev_units - units of elev (either DEG or DEG10)
      height_units - units of height (either METERS, KFEET, or NMILES)

   Outputs:
      height - height, in units of "height_units"

   Returns:
      If units are not validate, returns -1.  Otherwise, returns 0.

************************************************************************/
int RPGCS_range_u( REAL height, int height_units, REAL elev,
                    int elev_units, REAL *range, int range_units ){
   int ret;

   /* Save the current settings for input/output units. */
   int save_in = RPGCS_get_input_units();
   int save_out = RPGCS_get_output_units();

   /* Validate the input/output units. */
   if( (ret = Validate_units( height_units, LENGTH, "height_units" )) < 0 )
      return(ret);

   if( (ret = Validate_units( range_units, LENGTH, "range_units" )) < 0 )
      return(ret);

   /* Validate the elevation units. */
   if( (ret = Validate_units( elev_units, ANGLE, "elev_units" )) < 0 )
      return(ret);

   if( elev_units == DEG10 )
      elev /= 10.0;

   /* Set the input/output units. */
   RPGCS_set_input_units( range_units );
   RPGCS_set_output_units( height_units );

   /* Do the conversion. */
   ret = RPGCS_range( height, elev, range );

   /* Reset the input/output units. */
   RPGCS_set_input_units( save_in );
   RPGCS_set_output_units( save_out );

   return(ret);

}

/************************************************************************

   Description:
      Computes the range given height and elev.

   Inputs:
      height - height at elevation and range.
      elev - elevation angle, in degrees.

   Outputs:
      range - slant range, in "units".

   Returns:
      Returns -1
         if elev is out of bounds.

************************************************************************/
int RPGCS_range( REAL height, REAL elev, REAL *range ){

   double earth;
   double range_d;
   double sin_elev;
   double elev_d;

   /* Ensure elevation is between -90.0 and +90.0 degrees. */
   if( (elev < -90.0) || (elev > 90.0) )
      return(-1);

   /* Convert the elevation to radians. */
   elev_d = ((double) elev)*DTR;

   /* Set the effective earth's radius is appropriate scale. */
   earth = EA * In_units;

   /* Compute sine of elevation . */
   sin_elev = sin( elev_d );

   /* Compute height using the required height equation. */
   range_d = earth*(sqrt(sin_elev*sin_elev + (2.0*((double) height))/earth) - sin_elev);

   /* Put in the proper units. */
   *range =  (REAL) (range_d * Scale_factor);

   return 0;

}

/***********************************************************************

   Description:
      Sets the input units for coordinate transformations.

   Inputs:
      units - either METERS, KFEET or NMILES

   Notes:
      If units is neither METERS, KFEET, or NMILES, function call
      has no effect.

************************************************************************/
void RPGCS_set_input_units( int units ){

   if( units == METERS ){

      In_units = METERS;
      In_scale = 1.0;

   }
   else if( units == KFEET ){

      In_units = KFEET;
      In_scale = M_TO_KFT;

   }
   else if( units == NMILES ){

      In_units = NMILES;
      In_scale = M_TO_NMI;

   }

   /* Set the scale factor. */
   Scale_factor = Out_scale/In_scale;

}

/***********************************************************************

   Description:
      Gets the current value of input units.

   Returns:
      either METERS, KFEET, or NMILES

************************************************************************/
int RPGCS_get_input_units( ){

   return( In_units );

}

/***********************************************************************

   Description:
      Sets the output units for coordinate transformations.

   Inputs:
      units - either METERS, KFEET, or NMILES

   Notes:
      If units is neither METERS, KFEET, or NMILES, function call
      has no effect.

************************************************************************/
void RPGCS_set_output_units( int units ){

   if( units == METERS ){

      Out_units = METERS;
      Out_scale = 1.0;

   }
   else if( units == KFEET ){

      Out_units = KFEET;
      Out_scale = M_TO_KFT;

   }
   else if( units == NMILES ){

      Out_units = NMILES;
      Out_scale = M_TO_NMI;

   }

   /* Set the scale factor. */
   Scale_factor = Out_scale/In_scale;

}

/***********************************************************************

   Description:
      Gets the current value of output units.

   Returns:
      either METERS, KFEET, or NMILES

************************************************************************/
int RPGCS_get_output_units( ){

   return( Out_units );

}

/***********************************************************************

   Description:
      Performs validation of "units" given "type".

   Input:
      units - the units to validate
      type - type of units (either LENGTH or ANGLE)
      err_str - string to be included in error text.

   Returns:
      -1 on error, or 0 otherwise.

***********************************************************************/
static int Validate_units( int units, int type, char *err_str ){

   if( type == LENGTH ){

      /* Validate the length units. */
      if( (units != METERS)
                &&
          (units != KFEET)
                &&
          (units != NMILES)){

         LE_send_msg( GL_ERROR, "%s units Value (%d) Not Recognized\n",
                   units, err_str );
         return(-1);

      }

      return 0;

   }

   else if( type == ANGLE ){

      if( (units != DEG10 )
                 &&
          (units != DEG)){

         LE_send_msg( GL_ERROR, "%s units Value (%d) Not Recognized\n",
                   units );
         return(-1);

      }

      return 0;

   }

   else
      return 0;

}
