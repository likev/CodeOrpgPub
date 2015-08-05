/*$(
 *======================================================================= 
 * 
 *   (c) Copyright, 2012 Massachusetts Institute of Technology.
 *       This material may be reproduced by or for the 
 *       U.S. Government pursuant to the copyright license 
 *       under the clause at 252.227-7013 (Jun. 1995).
 * 
 *
 *=======================================================================
 *
 *
 *   FILE:    build_model_grids.c
 *
 *   AUTHOR:  Robert G Hallowell
 *
 *   CREATED: Aug 1, 2011; Initial Version
 *
 *   REVISION: 10/15/11		M. Donovan
 *             -Modified to store model data in a grid structure of 
 *              external type RPGP_ext_data_t.
 *             -Added temperature grids for ZDR and maximum and minimum
 *              temperature of the warm (>0) and cold layers (<0) found 
 *              within each model column, respectively, to the Freezing 
 *              Height data store.
 *
 *             7/11/12		M. Donovan
 *             -Reordered Freezing Height grid structure.
 *             -Added check that only allows a CIP grid to be created if
 *              there's a matching pressure level among T, RH, and H model
 *              fields.
 *             -Added geopotential height grids to the CIP linear buffer
 *              for the same matching pressure levels.
 *             -Added function arguments site_i_index and site_j_index 
 *              to allow printing of the Freezing Height and CIP grid 
 *              values at the radar location. 
 *
 * 
 *=======================================================================
 *
 *   DESCRIPTION:
 *
 *   This file contains functions that will build two data stores of 
 *   model derived grid data. Calls are made to pack the data into an RPGP_ext_data_t
 *   structure and then are made available as linear buffer files for other 
 *   algorithms to use.
 *
 *   FUNCTIONS:
 *
 *   void Open_lb_data_store()
 *   void Create_FreezingHeight_Grid( int model, char *buf, 
 *                     RPGCS_model_attr_t *model_attrs,
 *                     int site_i_index, int site_j_index )
 *   static void Create_Model_Field( RPGCS_model_grid_data_t **grid, 
 *                                   RPGCS_model_grid_data_t *grid_t_data,
 *                                   int units, int nlevels, int ext_grid_type )
 *   void Create_CIP_Grid( int model, char *buf, RPGCS_model_attr_t *model_attrs,
 *                         int site_i_index, int site_j_index )
 *   static double Find_height( double temp1, double temp2, double height1, 
 *                              double height2, double target )
 *
 *   NOTES:    
 *
 *   The method used to find the model heights associated with certain
 *   temperature values in Create_FreezingHeight_Grid() was adopted from
 *   Update_temps() in update_alg_data.c. However, the search for additional
 *   values has been added.
 *
 *=======================================================================
 *$)
 */

#include <build_model_grids.h>

RPGCS_model_grid_data_t *grid_frz_data = NULL; 
RPGCS_model_grid_data_t *grid_cip_data = NULL; 

static int h_level_index[MAXIMUM_LEVELS];
static int t_level_index[MAXIMUM_LEVELS];
static int rh_level_index[MAXIMUM_LEVELS];
static double matched_level_value[MAXIMUM_LEVELS];

static void Create_Model_Field( RPGCS_model_grid_data_t **ptr,
                                RPGCS_model_grid_data_t *grid_data,
                                int nlevels, int ext_grid_type );
static double Find_height( double temp1, double temp2, double height1, 
                           double height2, double target );


/******************************************************************

   Description:
      Open LB data stores for Freezing Height and CIP grids.

******************************************************************/
void Open_lb_data_store() {

   int ret;

   /* open freezing height grid for writing to non-product data store */
   ret = RPGC_data_access_open( MODEL_FRZ_GRID, LB_WRITE );
   if ( ret < 0 ) {
        LE_send_msg( GL_ERROR, "RPGC_data_access_open failed for MODEL_FRZ_GRID: %d", ret );
   }

   /* open cip grid for writing to non-product data store */
   ret = RPGC_data_access_open(MODEL_CIP_GRID, LB_WRITE); 
   if ( ret < 0 ) {
        LE_send_msg( GL_ERROR, "RPGC_data_access_open failed for MODEL_CIP_GRID: %d", ret );
   }

/* End of Open_lb_data_store() */
}

/******************************************************************

   Description:
      Create model derived grids of freezing height, hail height, and other
      height levels of temperature data along with max and min temperature
      data within any warm or cold layer found within each model grid
      point column. Calls are made to pack the data into an RPGP_ext_data_t
      structure and write it to a linear buffer file.

   Inputs:
      model - model of interest
      buf - buffer holding the model of interest
      model_attrs - attributes of current mode
      site_i_index - model grid index value at the radar
      site_j_index - model grid index value at the radar

   Returns:
      None

******************************************************************/
void Create_FreezingHeight_Grid( int model, char *buf, 
                                 RPGCS_model_attr_t *model_attrs,
                                 int site_i_index, int site_j_index ) {

   RPGCS_model_grid_data_t *grid_h_data = NULL;
   RPGCS_model_grid_data_t *grid_t_data = NULL;

   int zerodeg_set, minus20_set;
   int zdr_temp1_set, zdr_temp2_set;
   double zero_height = MISSING_HGT, minus20_height = MISSING_HGT;
   double zdr_temp1_height = MISSING_HGT, zdr_temp2_height = MISSING_HGT;
 
   int x, y, z;
   int i, j;
   int xy_index;
   double range, azimuth;
   int ret, ext_grid_type = FRZ_GRID;
	 	 
   /* Get the Height and Temperature fields. */
   grid_h_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_GH );
   grid_t_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_TEMP );

   /* Are height and temperature data available? */
   if( grid_h_data != NULL && grid_t_data != NULL ){

     LE_send_msg( GL_INFO, "H grids     xdim: %d  ydim: %d nlvls: %d\n", 
                  grid_h_data->dimensions[0], grid_h_data->dimensions[1], grid_h_data->num_levels );
     LE_send_msg( GL_INFO, "T grids     xdim: %d  ydim: %d nlvls: %d\n", 
                  grid_t_data->dimensions[0], grid_t_data->dimensions[1], grid_t_data->num_levels );

     /* Don't create a FRZ grid if Height and Temperature dimensions don't match */
     if( (grid_h_data->dimensions[0] != grid_t_data->dimensions[0]) ||
         (grid_h_data->dimensions[1] != grid_t_data->dimensions[1]) ){
       LE_send_msg( GL_ERROR, "FRZ Grid NOT Updated due to grid dimension mismatch between Hgt & Tmp.\n");
       RPGCS_free_model_field( model, (char *) grid_h_data );
       RPGCS_free_model_field( model, (char *) grid_t_data );
       return;
     }

     /* Create grid structure to hold freezing height grid data */
     Create_Model_Field( &grid_frz_data, grid_t_data, NUM_COMPONENTS, ext_grid_type );

     if (grid_frz_data == NULL) {
	LE_send_msg( GL_ERROR, "Memory could not be allocated for grid_frz_data.\n"); 
	RPGCS_free_model_field( model, (char *) grid_h_data );
	RPGCS_free_model_field( model, (char *) grid_t_data );
	return;
     }

     LE_send_msg( GL_INFO, "FRZ grids   xdim: %d  ydim: %d nlvls: %d\n", 
                  grid_frz_data->dimensions[0], grid_frz_data->dimensions[1], grid_frz_data->num_levels );

     /* Find the highest freezing level first (top-down) */
     for( x = 0; x < grid_frz_data->dimensions[0]; x++ ){
       for( y = 0; y < grid_frz_data->dimensions[1]; y++ ){
	 minus20_set = 0;
	 zdr_temp1_set = 0;
	 zdr_temp2_set = 0;
         zerodeg_set = 0;

         xy_index = ( (y*grid_frz_data->dimensions[0]) + x );

	 RPGCS_lambert_grid_point_azran( x, y, &range, &azimuth );
	 grid_frz_data->data[MDL_RANGE][xy_index] = range;
	 grid_frz_data->data[MDL_AZ][xy_index] = azimuth;

         /* start the search from the highest level (lowest p-level) and progress downward */
	 for( z = grid_t_data->num_levels - 2; z >= 0; z-- ){		/* need to start one lvl down from top because
									   temp2 gets starting value at highest lvl */

           int t_units;
           int warm_layer = 0;
           int cold_layer = 0;
           double tmax = -99.9;
           double tmin = 99.9;
           double temp1 = RPGCS_get_data_value( grid_t_data, z, x, y, &t_units );
           double temp2 = RPGCS_get_data_value( grid_t_data, z+1, x, y, &t_units );

           /* We want the units of temperature in Deg C. */
           if( t_units == RPGCS_KELVIN_UNITS ) {
             temp1 += KELVIN_TO_C;
             temp2 += KELVIN_TO_C;
           }
					 
           /* Find first drop below -20 for hail algorithm */
           if ( minus20_set == 0 && temp1 > -20.0 ) {
             double t_level_value1 = grid_t_data->params[z]->level_value;
             double t_level_value2 = grid_t_data->params[z+1]->level_value;
             double height1 = MISSING_HGT, height2 = MISSING_HGT;

             /* Make sure geopotential height levels match temperature levels. */
             for( i = grid_h_data->num_levels - 1; i >= 0; i-- ){
               double h_level_value = grid_h_data->params[i]->level_value;
               int h_units;

               if( h_level_value == t_level_value1 ){
                 height1 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height1 *= M_TO_KM;
               }
               if( h_level_value == t_level_value2 ){
                 height2 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height2 *= M_TO_KM;
               }
               /* Are both heights found? */
               if( (height1 != MISSING_HGT) && (height2 != MISSING_HGT) ){

                 minus20_set = 1;

                 /* Do the interpolation .... */
                 minus20_height = Find_height( temp1, temp2, height1, height2, -20.0 );
                 grid_frz_data->data[MDL_MINUS_20][xy_index] = minus20_height;

                 break;
               }
             }
           }

           /* Find first drop below ZDR_TEMP2 */
           if ( zdr_temp2_set == 0 && temp1 > ZDR_TEMP2 ) {
             double t_level_value1 = grid_t_data->params[z]->level_value;
             double t_level_value2 = grid_t_data->params[z+1]->level_value;
             double height1 = MISSING_HGT, height2 = MISSING_HGT;

             /* Make sure geopotential height levels match temperature levels. */
             for( i = grid_h_data->num_levels - 1; i >= 0; i-- ){
               double h_level_value = grid_h_data->params[i]->level_value;
               int h_units;

               if( h_level_value == t_level_value1 ){
                 height1 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height1 *= M_TO_KM;
               }
               if( h_level_value == t_level_value2 ){
                 height2 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height2 *= M_TO_KM;
               }
               /* Are both heights found? */
               if( (height1 != MISSING_HGT) && (height2 != MISSING_HGT) ){

                 zdr_temp2_set = 1;

                 /* Do the interpolation .... */
                 zdr_temp2_height = Find_height( temp1, temp2, height1, height2, ZDR_TEMP2 );
                 grid_frz_data->data[MDL_ZDR_TEMP2][xy_index] = zdr_temp2_height;

                 break;
               }
             }
           }

           /* Find first drop below ZDR_TEMP1 */
           if ( zdr_temp1_set == 0 && temp1 > ZDR_TEMP1 ) {
             double t_level_value1 = grid_t_data->params[z]->level_value;
             double t_level_value2 = grid_t_data->params[z+1]->level_value;
             double height1 = MISSING_HGT, height2 = MISSING_HGT;

             /* Make sure geopotential height levels match temperature levels. */
             for( i = grid_h_data->num_levels - 1; i >= 0; i-- ){
               double h_level_value = grid_h_data->params[i]->level_value;
               int h_units;

               if( h_level_value == t_level_value1 ){
                 height1 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height1 *= M_TO_KM;
               }
               if( h_level_value == t_level_value2 ){
                 height2 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height2 *= M_TO_KM;
               }
               /* Are both heights found? */
               if( (height1 != MISSING_HGT) && (height2 != MISSING_HGT) ){

                 zdr_temp1_set = 1;

                 /* Do the interpolation .... */
                 zdr_temp1_height = Find_height( temp1, temp2, height1, height2, ZDR_TEMP1 );
                 grid_frz_data->data[MDL_ZDR_TEMP1][xy_index] = zdr_temp1_height;

                 break;
               }
             }
           }

           /* If the temperature toggles above/below freezing, then interpolate to find freezing height */ 
           if ( (((zerodeg_set % 2)==0) && (temp1 > 0.0 )) || (((zerodeg_set % 2)==1) && (temp1 < 0.0 )) ) {
             if ( temp1 > 0.0 ){
               warm_layer = 1;
               tmax = temp1;
             }
  
             if ( temp1 < 0.0 ){
               cold_layer = 1;
               tmin = temp1;
             }

             double t_level_value1 = grid_t_data->params[z]->level_value;
             double t_level_value2 = grid_t_data->params[z+1]->level_value;
             double height1 = MISSING_HGT, height2 = MISSING_HGT;

             /* Make sure geopotential height levels match temperature levels. */
             for( i = grid_h_data->num_levels - 1; i >= 0; i-- ){
               double h_level_value = grid_h_data->params[i]->level_value;
               int h_units;

               if( h_level_value == t_level_value1 ){
                 height1 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height1 *= M_TO_KM;
               }
               if( h_level_value == t_level_value2 ){
                 height2 = RPGCS_get_data_value( grid_h_data, i, x, y, &h_units );

                 if( h_units == RPGCS_METER_UNITS )
                   height2 *= M_TO_KM;
               }
               /* Are both heights found? */
               if( (height1 != MISSING_HGT) && (height2 != MISSING_HGT) ){

                 zerodeg_set += 1;

                 /* Do the interpolation .... */
                 zero_height = Find_height( temp1, temp2, height1, height2, 0.0 );
                 /* Add 2 to zerodeg indexing to account for range [0] and azimuth [1] grid components */
                 grid_frz_data->data[zerodeg_set+2][xy_index] = zero_height;

                 /* search all levels below current level for another zero degree crosspoint
                    and save max or min temperature depending on which layer we're in(no interpolation performed) */
                 for( j = i; j >= 0; j-- ){
                   double temp = RPGCS_get_data_value( grid_t_data, j, x, y, &t_units );

                   if( t_units == RPGCS_KELVIN_UNITS )
                     temp += KELVIN_TO_C;

                   if( warm_layer == 1 && temp > 0.0 && temp > tmax )
                     tmax = temp;

                   if( cold_layer == 1 && temp < 0.0 && temp < tmin )
                     tmin = temp;

                   if( (warm_layer == 1 && temp < 0.0) || (cold_layer == 1 && temp > 0.0) )
                     break;
                 }

                 /* set max/min temperature values in grids; dependent on layer type and 
                    number of zero degree crossings (profile type) */
                 if( warm_layer == 1 ){
                   if( zerodeg_set == 1 || zerodeg_set == 2 )
                     grid_frz_data->data[MDL_MAX_TEMP_WL][xy_index] = tmax;
                   else if( zerodeg_set == 3 || zerodeg_set == 4 )
                     grid_frz_data->data[MDL_MAX_TEMP_WL+1][xy_index] = tmax;
                   else if( zerodeg_set == 5 )
                     grid_frz_data->data[MDL_MAX_TEMP_WL+2][xy_index] = tmax;
                 }

                 if( cold_layer == 1 ){
                   if( zerodeg_set == 2 || zerodeg_set == 3 )
                     grid_frz_data->data[MDL_MIN_TEMP_CL][xy_index] = tmin;
                   else if( zerodeg_set == 4 || zerodeg_set == 5 )
                     grid_frz_data->data[MDL_MIN_TEMP_CL+1][xy_index] = tmin;
                 }

                 break;
               }
             }
           }

         } /* end for z */

         grid_frz_data->data[MDL_ZERO_X][xy_index] = (double) zerodeg_set;

       }  /* end for y */
     }  /* end for x */

     /* Print the grid component values for the grid bin closest to the radar */
     int ijk, site_grid_index;
     site_grid_index = ( site_j_index * grid_frz_data->dimensions[0] + site_i_index );
     LE_send_msg( GL_INFO, "FRZ Grid Data:\n" );
     for ( ijk = 0; ijk < NUM_COMPONENTS; ijk++ ) {
	float val = grid_frz_data->data[ijk][site_grid_index];
        if( ijk == 0 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,         Range (km): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk == 1 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,       Azimuth (dg): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk == 2 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,           Num0degX: %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk >= 3 && ijk <= 8 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,  Hgt0degX (km MSL): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk == 9 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,  HgtZdrT1 (km MSL): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk == 10 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,  HgtZdrT2 (km MSL): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk == 11 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,    Hgt-20 (km MSL): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk >= 12 && ijk <= 14 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,        MaxT-WL (C): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk >=15 && ijk <= 16 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,        MinT-CL (C): %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
        else if( ijk >= 17 && ijk <= 19 )
	  LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Component: %2.0f,              Spare: %8.2f \n", 
                       site_i_index, site_j_index, (float)ijk, val );
     }

     /* pack an RPGP_ext_data_t struct for FRZ grid data */
     char *product_description = "FRZ Gridded Data";
     void *frz_ext_data = malloc( sizeof(RPGP_ext_data_t) );
     ret = fill_rpgp_ext_data_t( ext_grid_type, product_description, model_attrs, 
                                 grid_frz_data, (RPGP_ext_data_t *) frz_ext_data );

     /* for debugging, print all parameter and components stored in structure */
     /*print_param_and_component_info( site_i_index, site_j_index );*/

     /* send the RPGP_ext_data_t struct to a linear buffer file */
     ret = send_grid_to_lb( ext_grid_type, (RPGP_ext_data_t *) frz_ext_data );
     if ( ret == 1 ) {
   	LE_send_msg( GL_INFO, "Successfully sent FRZ grid to a linear buffer file\n" );
   	LE_send_msg( GL_INFO, "--------------------------------------------------\n\n" );
     }
     else
   	LE_send_msg( GL_INFO, "Failed to send FRZ to a linear buffer file\n\n" );

     /* Free the data */
     ret = RPGP_product_free( (void *)frz_ext_data );

     if( grid_frz_data != NULL )
	RPGCS_free_model_field( model, (char *) grid_frz_data );

     RPGCS_free_model_field( model, (char *) grid_h_data );
     RPGCS_free_model_field( model, (char *) grid_t_data );

   }  /* end if grid_h_data and grid_t_data */

   else
     LE_send_msg(GL_INFO, "model error: fields are NULL ... FRZ Grid NOT Updated\n");

/* End of Create_FreezingHeight_Grid() */
}

/******************************************************************

   Description:
      Create model grid structure for either freezing height
      or CIP data. Data array is initialized to a parameter value.

   Inputs:
      grid_t_data - grid temperature data for the model of interest.
      nlevels - number of grid levels being stored.
      ext_grid_type - grid type being stored.

   Output:
      grid - filled grid structure of type RPGCS_model_grid_data_t.

   Returns:
      None

******************************************************************/
static void Create_Model_Field( RPGCS_model_grid_data_t **grid, RPGCS_model_grid_data_t *grid_t_data,
                                int nlevels, int ext_grid_type ) {
   int i, j;
   int xdim = 0;
   int ydim = 0;
   char *field = NULL;

   /* initialize the product field for each component*/
   if ( ext_grid_type == 1 )
   	field = FREEZING_GRID;
   else
   	field = CIP_GRID_PROD;
        
   xdim = grid_t_data->dimensions[0];
   ydim = grid_t_data->dimensions[1];

   /* Should check to see if the ptr is NULL? */
   *grid = NULL;

   *grid = (RPGCS_model_grid_data_t *) calloc( sizeof(RPGCS_model_grid_data_t), 1 );
	
   /* Set the name of the derived product */
   (*grid)->field = malloc( strlen(field) + 1 );
   memcpy( (*grid)->field, field, strlen(field) );
   (*grid)->field[strlen(field)] = 0;

   /* Set the number of dimensions */
   (*grid)->num_dimensions = 2;
   (*grid)->dimensions = (int *) calloc( sizeof(int), 2 );
   (*grid)->dimensions[0] = xdim;
   (*grid)->dimensions[1] = ydim;

   /* Initialize the type of units to unknown */
   (*grid)->units = RPGCS_UNKNOWN_UNITS;

   /* Set the number of vertical levels */
   (*grid)->num_levels = nlevels;

   /* For each level allocate the space needed to store the product data and initialize to bad value */
   for ( i = 0; i < (*grid)->num_levels; i++ ) {
   	(*grid)->data[i] = (double *) calloc( sizeof(double), xdim*ydim );

   	for ( j = 0; j < (*grid)->dimensions[0]*(*grid)->dimensions[1]; j++ )
		(*grid)->data[i][j] = MODEL_BAD_VALUE;
   }

   /* For each level allocate the space needed to store the product parameters */
   for ( i = 0; i < nlevels; i++ ) {

	/* Set level_type to first level_type in grid_t_data */
	(*grid)->params[i] = (RPGCS_model_grid_params_t *) malloc( sizeof(RPGCS_model_grid_params_t) );
	(*grid)->params[i]->level_type = grid_t_data->params[0]->level_type;

        if ( ext_grid_type == 1 ) {
        	(*grid)->params[i]->level_value = (double)i;
		(*grid)->params[i]->level_units = 0;

	}
	else {
		/* First half of levels will be the CIP product field,
		second half of levels will be the geopotential height field */
		if ( i < nlevels/2 )
			(*grid)->params[i]->level_value = matched_level_value[i];
		else
			(*grid)->params[i]->level_value = matched_level_value[i-nlevels/2];
		(*grid)->params[i]->level_units = grid_t_data->params[0]->level_units;
	}

   }

/* End of Create_Model_Field() */
}

/******************************************************************

   Description:
      Create model grid of CIP calculations, store the data into an
      RPGP_ext_data_t structure, and send it to a linear buffer file.

   Inputs:
      model - model of interest.
      buf - buffer holding the model of interest.
      model_attrs - attributes of current mode.
      site_i_index - model grid index value at the radar.
      site_j_index - model grid index value at the radar.

   Returns:
      None

******************************************************************/
void Create_CIP_Grid( int model, char *buf, RPGCS_model_attr_t *model_attrs,
                      int site_i_index, int site_j_index ) {

   RPGCS_model_grid_data_t *grid_h_data = NULL;
   RPGCS_model_grid_data_t *grid_t_data = NULL;
   RPGCS_model_grid_data_t *grid_rh_data = NULL;
   int i, j, k;
   int num_matching_levels = 0;
   int t_index, rh_index;
   double h_level_value, t_level_value, rh_level_value;

   int xy_index;
   float cip_t, cip_rh;
   int x, y, z; 
   int ret, ext_grid_type = CIP_GRID;

   /* Create CIP lookup table for processing model data */
   create_cipLookup(); 
	 
   /* Get the Height, Temperature and Relative Humidity fields */
   grid_h_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_GH );
   grid_t_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_TEMP );
   grid_rh_data = RPGCS_get_model_field( model, buf, RPGCS_MODEL_RH );

   if( grid_h_data != NULL && grid_t_data != NULL && grid_rh_data != NULL ){

     LE_send_msg( GL_INFO, "H grids     xdim: %d  ydim: %d nlvls: %d\n", 
                  grid_h_data->dimensions[0], grid_h_data->dimensions[1], grid_h_data->num_levels );
     LE_send_msg( GL_INFO, "T grids     xdim: %d  ydim: %d nlvls: %d\n", 
                  grid_t_data->dimensions[0], grid_t_data->dimensions[1], grid_t_data->num_levels );
     LE_send_msg( GL_INFO, "RH grids    xdim: %d  ydim: %d nlvls: %d\n", 
                  grid_rh_data->dimensions[0], grid_rh_data->dimensions[1], grid_rh_data->num_levels );

     /* Don't create a CIP grid if Height, Temperature, and Relative Humidity dimensions don't match */
     if( (grid_h_data->dimensions[0] != grid_t_data->dimensions[0]) || 
         (grid_h_data->dimensions[0] != grid_rh_data->dimensions[0]) ||
         (grid_h_data->dimensions[1] != grid_t_data->dimensions[1]) ||
         (grid_h_data->dimensions[1] != grid_rh_data->dimensions[1]) ){
       LE_send_msg( GL_ERROR, "CIP Grid NOT Updated due to grid dimension mismatch between Hgt, Tmp, & RH.\n");
       RPGCS_free_model_field( model, (char *) grid_h_data );
       RPGCS_free_model_field( model, (char *) grid_t_data );
       RPGCS_free_model_field( model, (char *) grid_rh_data );
       return;
     }

     /* To guard against missing pressure levels among the model fields of interest,
        find all matching pressure levels. Compute CIP for these levels only.
        An unequal number of levels regularly exists in the RUC40 data. */
     for( i = 0; i < grid_h_data->num_levels; i++ ){
       h_level_value = grid_h_data->params[i]->level_value;
       t_index = -1;
       rh_index = -1;

       for( j = 0; j < grid_t_data->num_levels; j++ ){
         t_level_value = grid_t_data->params[j]->level_value;
         if( t_level_value == h_level_value ){
           t_index = j;
           break;
         }
       }
       for( k = 0; k < grid_rh_data->num_levels; k++ ){
         rh_level_value = grid_rh_data->params[k]->level_value;
         if( rh_level_value == h_level_value ){
           rh_index = k;
           break;
         }
       }

       if( t_index != -1 && rh_index != -1 ){
         matched_level_value[num_matching_levels] = h_level_value;
         h_level_index[num_matching_levels] = i;
         t_level_index[num_matching_levels] = t_index;
         rh_level_index[num_matching_levels] = rh_index;
         num_matching_levels++;
       }
     }

     if( num_matching_levels != 0 ){

       /* Create model field to hold CIP calculations.
          Double the number of levels to hold the model height field */
       Create_Model_Field( &grid_cip_data, grid_t_data, num_matching_levels*2, ext_grid_type );

       if ( grid_cip_data == NULL ) {
   	    LE_send_msg( GL_ERROR, "Memory could not be allocated for grid_cip_data.\n" );
 	    RPGCS_free_model_field( model, (char *) grid_h_data );
	    RPGCS_free_model_field( model, (char *) grid_t_data );
            RPGCS_free_model_field( model, (char *) grid_rh_data );
   	    return;
       }

       /* Check the model field units and adjust to desired units */
       int testU;
       RPGCS_get_data_value( grid_t_data, 0, 0, 0, &testU );
       float tadj = 0;
       if( testU == RPGCS_KELVIN_UNITS ) {
         tadj = KELVIN_TO_C;
       }
       int h_units;
       RPGCS_get_data_value( grid_h_data, 0, 0, 0, &h_units );
       float hadj = 1;
       if( h_units == RPGCS_METER_UNITS ) {
         hadj = M_TO_KM;
       }

       LE_send_msg( GL_INFO, "CIP grids   xdim: %d  ydim: %d nlvls: %d\n", 
                    grid_cip_data->dimensions[0], grid_cip_data->dimensions[1], grid_cip_data->num_levels );

       /* Compute and fill the CIP grid bins and then fill the model geopotential height 
          grid bins for each matched pressure level. */
       int t_idx, rh_idx, h_idx;

       for( x = 0; x < grid_cip_data->dimensions[0]; x++ ){
         for( y = 0; y < grid_cip_data->dimensions[1]; y++ ){

           xy_index = ( (y*grid_cip_data->dimensions[0]) + x );

           /* Fill grid bins with CIP product values */
           for( z = 0; z < num_matching_levels; z++ ){
             t_idx = t_level_index[z];
             rh_idx = rh_level_index[z];
             cip_t = T_lookup( grid_t_data->data[t_idx][xy_index] + tadj );
             cip_rh = RH_lookup( grid_rh_data->data[rh_idx][xy_index] );
             grid_cip_data->data[z][xy_index] = 100 * (cip_t * cip_rh);
           }

           /* Fill grid bins with geopotential height values */
           for( z = num_matching_levels; z < grid_cip_data->num_levels; z++ ){
             h_idx = h_level_index[z-num_matching_levels];
             grid_cip_data->data[z][xy_index] = (grid_h_data->data[h_idx][xy_index] * hadj);
           }

         }
       }

       /* Print the grid component values for the grid bin closest to the radar */
       int ijk, site_grid_index;
       float cip_val, hgt_val;

       site_grid_index = ( site_j_index * grid_cip_data->dimensions[0] + site_i_index );
       LE_send_msg( GL_INFO, "CIP Grid Data:\n" );
       for ( ijk = 0; ijk < num_matching_levels; ijk++ ) {
         cip_val = grid_cip_data->data[ijk][site_grid_index];
         hgt_val = grid_cip_data->data[ijk+num_matching_levels][site_grid_index];
	 LE_send_msg( GL_INFO, "--->Gridpoint [%d][%d], Level (mb): %4.0f (%2.0f), CipInt (%s): %6.2f, Height (km MSL): %5.2f\n", 
                      site_i_index, site_j_index, matched_level_value[ijk], (float)ijk, RPGCS_PERCENT_STR, cip_val, hgt_val );
       }

       /* pack an RPGP_ext_data_t struct for CIP grid data */
       char *product_description = "CIP Gridded Data";
       void *cip_ext_data = malloc( sizeof(RPGP_ext_data_t) );
       ret = fill_rpgp_ext_data_t( ext_grid_type, product_description, model_attrs, 
                                   grid_cip_data, (RPGP_ext_data_t *) cip_ext_data );

       /* for debugging, print all parameter and components stored in structure */
       /*print_param_and_component_info( site_i_index, site_j_index );*/

       /* send the RPGP_ext_data_t struct to a linear buffer file */
       ret = send_grid_to_lb( ext_grid_type, (RPGP_ext_data_t *) cip_ext_data );
       if (ret == 1) {
         LE_send_msg( GL_INFO, "Successfully sent CIP grid to a linear buffer file\n" );
         LE_send_msg( GL_INFO, "--------------------------------------------------\n\n" );
       }
       else
         LE_send_msg( GL_INFO, "Failed to send CIP to a linear buffer file\n\n" );

       /* Free the data */
       ret = RPGP_product_free( (void *)cip_ext_data );

       if( grid_cip_data != NULL )
          RPGCS_free_model_field( model, (char *) grid_cip_data );

     }

     else
       LE_send_msg(GL_ERROR, "model error: field level values do not match ... CIP Grid NOT Updated\n");

   }

   else 
     LE_send_msg(GL_INFO, "model error: fields are NULL ... CIP Grid NOT Updated\n");

   if( grid_h_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_h_data );

   if( grid_t_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_t_data );

   if( grid_rh_data != NULL )
      RPGCS_free_model_field( model, (char *) grid_rh_data );


/* End of Create_CIP_Grid() */
}

/******************************************************************
   
   Description:
      Interpolate to find the height of the "target" temperature.

   Inputs:
      temp1 - temperature at height "height1"
      temp2 - temperature at height "height2"
      height1 and height2 - heights of temp1 and temp2, respectively
      target - target temperature.

   Returns:
      The height of the "target" temperature.

   Notes:
      Copied from update_alg_data.c

******************************************************************/
static double Find_height( double temp1, double temp2, double height1, 
                           double height2, double target ){

   double height = height1 + 
                   ((height2-height1)*(target-temp1))/(temp2-temp1);

   return( height );

/* End of Find_height() */
}
