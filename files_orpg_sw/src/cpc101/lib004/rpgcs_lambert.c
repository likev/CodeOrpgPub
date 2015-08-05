#include <a309.h>
#include <math.h>
#include <rpgcs_latlon.h>

/*
  This code is based on code_LatLonLambert.cc developed at NSSL. 
*/

#define MISSING_LATLON      -999.0

static double E, F, N, T1, M1, R1;      /* These are derived quantities. */
static double Tangent_lat = -999.0;	/* This is the latitude of the Tangent point,
      	                                   in radians. */
static double Tangent_lon = -999.0;     /* This is the longitude of the meridian which
                                           aligns to the Cartesian X axis. */
static double Origin_lat = -999.0;      /* Grid origin latitude, in radians. */
static double Origin_lon = -999.0;      /* Grid origin longitude, in radians. */
static double LLC_x = 0.0;              /* Lower Left Corner X coordinate, in kilometers. */
static double LLC_y = 0.0;              /* Lower Left Corner Y coordinate, in kilometers. */
static double URC_x = 0.0;              /* Upper Right Corner X coordinate, in kilometers. */
static double URC_y = 0.0;              /* Upper Right Corner Y coordinate, in kilometers. */
static int Num_dimensions = 0;          /* Number of grid dimensions; */
static int Dimensions[4] = {0};         /* Grid dimensions. */
static double Scale = 1.0;

/* Function Prototypes. */
static double Compute_t( double e, double lat );
static double Compute_m( double e, double lat );

/* Public functions follow. */

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the grid indices i_ind, j_ind, find the grid point x
      and y values, in meters.

   Inputs:
      i_ind - i index (assumes 0 indexed)
      j_ind - j index (assumes 0 indexed)

   Outputs:
      x - holds Lambert X coordinate, in kilometers.
      y - holds Lambert Y coordinate, in kilometers.

   Returns:
      Negative number on failure, non-negative on success.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_grid_point_xy( int i_ind, int j_ind, double *x, 
                                 double *y ){

   /* Check to see if the Lambert grid parameters have been initialized. */
   if( (Dimensions[0] == 0.0) || (Dimensions[1] == 0.0) )
      return RPGCS_LAMBERT_ERROR;

   /* Do interpolation. */
   *x = LLC_x + (((double) i_ind * (URC_x - LLC_x)) / (((double) Dimensions[0]) - 1.0));
   *y = LLC_y + (((double) j_ind * (URC_y - LLC_y)) / (((double) Dimensions[1]) - 1.0));

   return 0;

/* End of RPGCS_lambert_grid_point_xy() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the grid indices i_ind, j_ind, find the grid point lat
      and lon values, in degrees.

   Inputs:
      i_ind - i index (assumes 0 indexed)
      j_ind - j index (assumes 0 indexed)

   Outputs:
      lat - holds Latitude, in degrees.
      lon - holds Longitude, in degrees.

   Returns:
      Negative number on failure, non-negative on success.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_grid_point_latlon( int i_ind, int j_ind, double *lat,
                                     double *lon ){

   int ret;
   double x, y;

   if( (ret = RPGCS_lambert_grid_point_xy( i_ind, j_ind, &x, &y )) < 0 )
      return ret;

   return( RPGCS_lambert_xy_to_latlon( x, y, lat, lon ) );

/* End of RPGCS_lambert_grid_point_latlon() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the grid indices i_ind, j_ind, find the grid point azm
      and ran values.

   Inputs:
      i_ind - i index (assumes 0 indexed)
      j_ind - j index (assumes 0 indexed)

   Outputs:
      azm - holds Latitude, in degrees.
      ran - holds Longitude, in kilometers.

   Returns:
      Negative number on failure, non-negative on success.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_grid_point_azran( int i_ind, int j_ind, double *ran,
                                    double *azm ){

   int ret;
   double lat, lon;
   float azmf, ranf;

   /* To determine az/ran, must convert the coordinates to common
      reference frame (lat/lon), then do coordinate transformation
      (lat/lon) to (az/ran). */
   if( (ret = RPGCS_lambert_grid_point_latlon( i_ind, j_ind, 
                                               &lat, &lon )) < 0 )
      return ret;

   if( (ret = RPGCS_latlon_to_azran( (float) lat, (float) lon, 
                                     &ranf, &azmf)) < 0 )
      return ret;

   *ran = (double) ranf;
   *azm = (double) azmf;

   return 0;

/* End of RPGCS_lambert_grid_point_latlon() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the lambert grid x and y values, in kilometers, find
      the closest grid indices i_ind and j_ind.

   Inputs:
      x - Lambert X coordinate, in kilometers.
      y - Lambert Y coordinate, in kilometers.

   Outputs:
      i_ind - holds i index (assumes 0 indexed)
      j_ind - holds j index (assumes 0 indexed)

   Returns:
      Negative number on error, non-negative on success.

      NOTE:  If the x/y value is outside of the grid, the 
             closest grid point will be returned along with
             error RPGCS_DATA_POINT_NOT_IN_GRID

   20120126 Ward CCR NA12-00024 Added checks for i, j < 0.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_xy_to_grid_point( double x, double y, int *i_ind, 
                                    int *j_ind ){

   double temp_i, temp_j;
   int i, j, retval;

   /* Check to see if the Lambert grid parameters have been initialized. */
   if( (Dimensions[0] == 0.0) || (Dimensions[1] == 0.0) )
      return RPGCS_LAMBERT_ERROR;

   /* Do interpolation. */
   temp_i = ((x - LLC_x) * (((double) Dimensions[0]) - 1.0)) / (URC_x - LLC_x);
   temp_j = ((y - LLC_y) * (((double) Dimensions[1]) - 1.0)) / (URC_y - LLC_y);

   /* Make sure the i,j coordinates do not exceed the grid dimensions. */
   retval = 0;
   i = (int) RPGC_NINTD( temp_i );
   if( i >= Dimensions[0] ){

      i = Dimensions[0] - 1;
      retval = RPGCS_DATA_POINT_NOT_IN_GRID;

   }
   else if( i < 0 ){

      i = 0;
      retval = RPGCS_DATA_POINT_NOT_IN_GRID;

   }

   j = (int) RPGC_NINTD( temp_j );
   if( j >= Dimensions[1] ){

      j = Dimensions[1] - 1;
      retval = RPGCS_DATA_POINT_NOT_IN_GRID;

   }
   else if( j < 0 ){

      j = 0;
      retval = RPGCS_DATA_POINT_NOT_IN_GRID;

   }

   *i_ind = i;
   *j_ind = j;

   return retval;

/* End of RPGCS_lambert_xy_to_grid_point() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the latitude and longitude values, in degrees, find
      the closest grid indices i_ind and j_ind.

   Inputs:
      lat - Latitude of point, in degrees
      lon - Longitude of point, in degrees

   Outputs:
      i_ind - holds i index (assumes 0 indexed)
      j_ind - holds j index (assumes 0 indexed)

   Returns:
      Negative number on error, non-negative on success.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_latlon_to_grid_point( double lat, double lon, 
                                        int *i_ind, int *j_ind ){

   double x, y;
   int ret;

   /* First convert lat/lon to Lambert grid x, y coordinates. */
   if( (ret = RPGCS_lambert_latlon_to_xy( lat, lon, &x, &y )) < 0 )
      return ret;

   /* Convert the x/y coordinates to grid indices. */
   if( (ret = RPGCS_lambert_xy_to_grid_point( x, y, i_ind, j_ind )) < 0 )
      return ret;

   return 0;
   
/* End of RPGCS_latlon_to_grid_point() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the Latitude/Longitude of a data point on the Lambert
      grid, compute the x/y location on that same grid.

   Inputs:
      lat - latitude of data value, in degrees.
      lon - longitude of data value, in degrees.

   Outputs:
      x - x position, in kilometers
      y - y position, in kilometers

   Returns:
      RPGCS_LAMBERT_ERROR if Lambert_init hasn't been called to 
      initialize Lambert projection map parameters, and 0 otherwise.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_latlon_to_xy( double lat, double lon, double *x, double *y ){

   double phi, t, r, lambda, theta;

   /* Check to see if the Lambert grid parameters have been initialized. */
   if( (Origin_lon == MISSING_LATLON) 
                    || 
       (Origin_lat == MISSING_LATLON) 
                    ||
       (Tangent_lon == MISSING_LATLON) 
                    || 
       (Tangent_lat == MISSING_LATLON) )
      return RPGCS_LAMBERT_ERROR;

   /* Compute x/y. */
   phi = lat * DEGTORAD;
   t = Compute_t( E, phi );
   r = 6378137.0 * F * pow( t, N ) * Scale;

   lambda = lon * DEGTORAD;
   theta = N * (lambda - Origin_lon);

   *x = (r * sin( theta ))/1000.0;
   *y = (R1 - (r*cos( theta )))/1000.0;

   /* Check to see if the x/y values are valid. */
   if( isnan(*x) || isnan(*y) )
      return RPGCS_LAMBERT_ERROR;

   return 0;

/* End of RPGCS_lambert_latlon_to_xy() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Given the /y of a data point on the Lambert grid, compute
      the Latitude/Longitude location on that same grid.

   Inputs:
      x - x position, in kilometers
      y - y position, in kilometers

   Outputs:
      lat - latitude of data value, in degrees.
      lon - longitude of data value, in degrees.

   Returns:
      RPGCS_LAMBERT_ERROR if Lambert_init hasn't been called to 
      initialize Lambert projection map parameters, and 0 otherwise.

/////////////////////////////////////////////////////////////////\*/
int RPGCS_lambert_xy_to_latlon( double x, double y, double *lat, double *lon ){

   double r, r1my, t, theta, phi, old_phi;
   int iter = 0;

   /* Check to see if the Lambert grid parameters have been initialized. */
   if( (Origin_lon == MISSING_LATLON) 
                   || 
       (Origin_lat == MISSING_LATLON)
                   ||
       (Tangent_lon == MISSING_LATLON) 
                   || 
       (Tangent_lat == MISSING_LATLON) )
      return RPGCS_LAMBERT_ERROR;

   /* Convert x and y in kilometers, to meters. */
   x *= 1000.0;
   y *= 1000.0;

   /* Compute the latitude and longitude. */
   r1my = R1 - y;
   r = sqrt( x*x + r1my*r1my );
   if( N < 0.0 )
      r = -r;

   t = pow( r / (Scale * 6378137.0 * F), 1.0/N );
   theta = atan( x/r1my );
 
   /* Convert the longitude value to degrees. */
   *lon = (theta/N + Origin_lon) / DEGTORAD;

   /* Iterate to find phi. */
   phi = PI_CONST/2.0 - 2.0*atan(t);
   do {

      old_phi = phi;
      iter++;
      phi = PI_CONST/2.0 - 
            2.0*atan( t*pow( (1.0 - E*sin( phi ) ) / ( 1.0 + E*sin( phi )), E/2.0 ) );

   } while( (fabs(phi-old_phi) > 0.00001) && (iter < 5) );

   /* Convert the latitude value to degrees. */ 
   *lat = phi / DEGTORAD;

   /* Check to see if the lat/lon values are numbers. */
   if( isnan(*lat) || isnan(*lon) )
      return RPGCS_LAMBERT_ERROR;
   
   return 0;

/* End of RPGCS_lambert_xy_to_latlon() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Precomputes quantities based on the tangent point of the 
      Lambert conformal grid.

   Inputs:
      model_attr - point to RUC_model_attr_t data structure.

   Notes:
      The constants 0.00669438 and 6378137.0 are from WGS-84.

/////////////////////////////////////////////////////////////////\*/
void RPGCS_lambert_init( RPGCS_model_attr_t *model_attr ){

   int ret, i;

   double tangent_lat = model_attr->tangent_point.latitude * DEGTORAD;
   double tangent_lon = model_attr->tangent_point.longitude * DEGTORAD;
   double grid_orig_lat = model_attr->grid_lower_left.latitude * DEGTORAD;
   double grid_orig_lon = model_attr->grid_lower_left.longitude * DEGTORAD;

   /* If we have already precomputed what's possible, no need to do
      it again. */
   if( (tangent_lat != Tangent_lat)
                    ||
       (tangent_lon != Tangent_lon)
                    ||
       (grid_orig_lat != Origin_lat)
                    ||
       (grid_orig_lon != Origin_lon) ){

      /* Save the tangent latitude, longitude and grid origin.  */ 
      Tangent_lat = tangent_lat;
      Tangent_lon = tangent_lon;
      Origin_lat = grid_orig_lat;
      Origin_lon = grid_orig_lon;

      LE_send_msg( GL_INFO, "RPGCS_lambert_init: \n" );
      LE_send_msg( GL_INFO, "--->model_attr->tangent_point.latitude: %f\n",
                   model_attr->tangent_point.latitude );
      LE_send_msg( GL_INFO, "--->model_attr->tangent_point.longitude: %f\n",
                   model_attr->tangent_point.longitude );
      LE_send_msg( GL_INFO, "--->model_attr->grid_lower_left.latitude: %f\n",
                   model_attr->grid_lower_left.latitude );
      LE_send_msg( GL_INFO, "--->model_attr->grid_lower_left.longitude: %f\n",
                   model_attr->grid_lower_left.longitude );
      LE_send_msg( GL_INFO, "--->model_attr->grid_upper_right.latitude: %f\n",
                   model_attr->grid_upper_right.latitude );
      LE_send_msg( GL_INFO, "--->model_attr->grid_upper_right.longitude: %f\n",
                   model_attr->grid_upper_right.longitude );

      /* Assumes Earth is not an sphere, but an ellipsoid. */
      E = sqrt( (double) 0.00669438 );

      /* Precompute common terms. */
      T1 = Compute_t( E, Tangent_lat );
      M1 = Compute_m( E, Tangent_lat );
      N = sin( Tangent_lat );
      F = M1 / (N * pow( T1, N )); 
      R1 = 6378137.0 * F * pow( T1, N ) * Scale;

      /* Find the Lambert x/y coordinates of the grid corners. */
      if( ((ret = RPGCS_lambert_latlon_to_xy( model_attr->grid_lower_left.latitude,
                                              model_attr->grid_lower_left.longitude,
                                              &LLC_x, &LLC_y )) == RPGCS_LAMBERT_ERROR)
                                    ||
          ((ret = RPGCS_lambert_latlon_to_xy( model_attr->grid_upper_right.latitude,
                                              model_attr->grid_upper_right.longitude,
                                              &URC_x, &URC_y )) == RPGCS_LAMBERT_ERROR) )
         LE_send_msg( GL_INFO, "RPGCS_lambert_latlon_to_xy Error!\n" );

      LE_send_msg( GL_INFO, "--->LLC_x: %f, LLC_y: %f, URC_x: %f, URC_y: %f\n",
                   LLC_x, LLC_y, URC_x, URC_y );

   }

   /* Save the grid dimensions. */
   Num_dimensions = model_attr->num_dimensions;
   LE_send_msg( GL_INFO, "--->Number of Dimensions: %d\n", 
                Num_dimensions );
   
   for( i = 0; i < Num_dimensions; i++ ){

      Dimensions[i] = model_attr->dimensions[i];
      LE_send_msg( GL_INFO, "------>Dimension %d: %d\n", i, Dimensions[i] );

   }

/* End of RPGCS_lambert_init() */
}

/*\///////////////////////////////////////////////////////////////

   Description:  
      Converts map u/v components (ur, vr) at longitude
      lon to meteorological u/v components (u, v).

   Inputs:
      ur - map u component
      vr - map v component
      lon - longitude of u/v component, in degrees

   Outputs:
      u - meteorlogical u component
      v - meteorlogical v component

   Returns:
      RPGCS_LAMBERT_ERROR if Lambert_init hasn't been called to 
      initialize Lambert projection map parameters, and 0 otherwise.

///////////////////////////////////////////////////////////////\*/
int RPGCS_lambertuv_to_uv( double ur, double vr, double lon,
                           double *u, double *v ){

   double sinx2, cosx2, angle2;

   /* Check to see if the Lambert grid parameters have been initialized. */
   if( (Origin_lon == 0.0) && (Origin_lat == 0.0) )
      return RPGCS_LAMBERT_ERROR;

   angle2 = sin( Tangent_lat) * ((lon * DEGTORAD) - Tangent_lon);
   sinx2 = sin( angle2 );
   cosx2 = cos( angle2 );

   *u = cosx2*ur + sinx2*vr;
   *v = -sinx2*ur + cosx2*vr;

   /* Check to make sure u and v are valid numbers. */
   if( isnan(*u) || isnan(*v) )
      return RPGCS_LAMBERT_ERROR;

   return 0;

/* End of RPGCS_lambertuv_to_uv() */
}

/* Private functions follow. */

/*\/////////////////////////////////////////////////////////////////

   Description:
      Routine for computing the quantity t.

   Input: 
      E - from WGS-84
      phi - latitude of point, in radians.

   Returns: 
      Quantity t.

/////////////////////////////////////////////////////////////////\*/
static double Compute_t( double e, double phi ){

   double t = 
      tan(PI_CONST/4.0 - phi/2.0) / pow( (1.0-e*sin(phi))/(1.0+e*sin(phi)), e/2.0);

   return t;

/* End of Compute_t() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Routine for computing the quantity t.

   Input: 
      E - from WGS-84
      phi - latitude of point, in radians.

   Returns: 
      Quantity m.

/////////////////////////////////////////////////////////////////\*/
static double Compute_m( double e, double phi ){

   double esinphi = e * sin( phi );
   double m = cos( phi )/sqrt( 1.0 - esinphi*esinphi );

   return m;

/* End of Compute_m() */
}
