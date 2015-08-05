/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/06/06 20:18:29 $ */
/* $Id: rpgcs_misc_prod_funcs.c,v 1.1 2007/06/06 20:18:29 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <math.h>
#include <a309.h>
#include <rpgcs.h>

/* Function prototypes. */
static float Mini( float *array );
static float Maxi( float *array );
static void Theta( float *x, float *y, float *theta, float *max_theta, 
                   float *min_theta, short flag );

/* The following functions were ported from librpgcm to support
   algorithms within the RPG. */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Module receives polar coordinates of window center and size of
      window.  Module calculates location of window corners and 
      outputs the maximum and minimum radius and the maximum and 
      minimum azimuth required to enclose window.

   Inputs:
      radius_center - center range of the window.
      azimuth_center - center azimuth of the window, in deg. 
      length - length of the window.

   Outputs:
      min_rad - minimum range of window.
      max_rad - maximum range of window.
      min_theta - minimum angle of window.
      max_theta - maximum angle of window.

////////////////////////////////////////////////////////////////////\*/
void RPGCS_window_extraction( float radius_center, float azimuth_center, 
                              float length, float *max_rad, float *min_rad,
                              float *max_theta, float *min_theta ){

    /* Local variables */
    double d;
    float rad[5], theta[5], x[5], y[5], half_dim;
    short i, dim, neg_x, neg_y, flag;

    rad[0] = radius_center;
    theta[0] = azimuth_center;
    dim = length;

    half_dim = (float) (dim / 2);

    /* Conversion to Cartesian. */

    /* Center point of window converted to Cartesion.  Coordinates
       by standard conversion equation adapted to the condition
       theta is increasing clockwise.   Window corner coordinates
       are calculated using center point and dimension of window. */
    theta[0] *= DEGTORAD;
    x[0] = rad[0] * sin(theta[0]);
    y[0] = rad[0] * cos(theta[0]);
    x[1] = x[0] - half_dim;
    y[1] = y[0] + half_dim;
    x[2] = x[0] + half_dim;
    y[2] = y[0] + half_dim;
    x[3] = x[0] + half_dim;
    y[3] = y[0] - half_dim;
    x[4] = x[0] - half_dim;
    y[4] = y[0] - half_dim;


    /* Counts negative signs. */

    /* Count of the negative signs of the corners used to determine
       the quadrant location of window. */
    neg_x = 0;
    neg_y = 0;
    for( i = 1; i < 5; ++i ){

	if( x[i] < 0.f ) 
	    neg_x = (short) (neg_x + 1);
	
	if( y[i] < 0.f ) 
	    neg_y = (short) (neg_y + 1);
	
    }

    /* Conversion to polar. */

    /* Corner Cartesian coordinates converted to polar to obtain
       radius. */
    for( i = 1; i < 5; ++i ){

	d = (double) ( (x[i] * x[i]) + (y[i] * y[i]) );
	rad[i] = (float) sqrt(d);

    }

    *max_rad = Maxi( rad );

    /* Set flag to have maximum and minumum theta determined. */
    flag = 2;

    /* Determine quadrant location of window, minumum radius,
       maximum theta, minumum theta. */

    /* Case 1: If all x's same sign and all y's same sign.
               (window solely in one quadrant). */
    if( ((neg_x == 0) || (neg_x == 4)) 
                      && 
        ((neg_y == 0) || (neg_y == 4)) ){

	*min_rad = Mini( rad );
	Theta( x, y, theta, max_theta, min_theta, flag );

    /* Case 2A: If all x's negative (Window crosses negative x axis). */
    } 
    else if( neg_x == 4 ){

	*min_rad = 0.f - x[2];
	Theta( x, y, theta, max_theta, min_theta, flag );

    /* Case 2B: If all x's positive (Window crosses positive x axis). */
    } 
    else if( neg_x == 0 ){

	*min_rad = x[1];
	Theta( x, y, theta, max_theta, min_theta, flag );

    /* Case 2C: If all x's negative (Window crosses negative y axis). */
    } 
    else if( neg_y == 4 ){

	*min_rad = 0.f - y[1];
	Theta( x, y, theta, max_theta, min_theta, flag );

    /* Case 2D: If all x's positive (Window crosses positive y axis). */
    } 
    else if( neg_y == 0 ){

	*min_rad = y[3];
	flag = 3;
	Theta( x, y, theta, max_theta, min_theta, flag );
	*max_theta = theta[3];
	*min_theta = theta[4];

    /* Case 3: (Window incloses origin). */
    } 
    else {

	*min_rad = 0.f;
	*min_theta = 0.f;
	*max_theta = 360.f;
    }

    
/* End of RPGCS_window_extraction(). */
} 

/*\////////////////////////////////////////////////////////////////

   Description:
      Determine and return maximum value.  

   Inputs:
      array - array of values for which to determine the maximum
              value.

   Returns:
      Maximum value.

////////////////////////////////////////////////////////////////\*/
static float Maxi( float *array ){

    /* Local variables */
    short sub;
    float max;

    /* Initialize "max" = 2nd element of input array. */
    max = array[1];

    /* Do Until specified elements of the array have been tested. */
    for( sub = 2; sub < 5; ++sub ){

        /* If "max" is less than current element, then .. */
	if( max < array[sub] ){

            /* Reset "max" = value of current element. */
	    max = array[sub];

	}

    }

    return max;

/* End of Maxi(). */
} 

/*\//////////////////////////////////////////////////////////////

   Description:
      Determine and return minimum value.  

   Inputs:
      array - array of values for which to determine the minimum
              value.

   Returns:
      Minimum value.

//////////////////////////////////////////////////////////////\*/
static float Mini( float *array ){

    /* Local variables */
    short sub;
    float min;

    /* Initialize "min" = 2nd element of input array. */
    min = array[1];

    /* Do Until specified elements of the array have been tested. */
    for( sub = 2; sub < 5; ++sub ){

        /* If "min" is greater than the current element, then .. */
	if( min > array[sub] ){

            /* Reset "min" = value of the current element. */
	    min = array[sub];

	}

    }

    return min;

/* End of Mini(). */
} 

/*\////////////////////////////////////////////////////////////////

   Description:
      Convert corner Cartesian coordinates to polar.

   Inputs:
      x - X coordinates.
      y - Y corrdinates.
      flag - If maximum and minimum theta are requested.      

   Outputs:
      theta - corner angles.
      max_theta - maximum angle, in deg.
      min_theta - minimum angle, in deg. 

////////////////////////////////////////////////////////////////\*/
static void Theta( float *x, float *y, float *theta, float *max_theta, 
                   float *min_theta, short flag ){

    /* Local variables */
    static short i;

    for( i = 1; i < 5; ++i ){

        /* Convert corner Cartesian coordinates to polar angle. */ 
	theta[i] = atan2(x[i], y[i]) / DEGTORAD;

        /* If "THETA" is less than 0.0, add 360.0. */
	if( theta[i] < 0.f ) 
	    theta[i] += 360.f;

    }

    /* If maximum and minimum theta requested, then ... */
    if( flag == 2 ){

        /* Get maximum theta. */
	*max_theta = Maxi( theta );

        /* Get minimum theta. */
	*min_theta = Mini( theta );

    }

/* End of Theta(). */
} 

