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
 *   FILE:    pack_grid_data.c
 *
 *   AUTHOR:  Michael F Donovan
 *
 *   CREATED: Oct 15, 2011; Initial Version
 *
 *   REVISION: 7/11/12		M. Donovan
 *             -Added test when setting CIP grid data field name and units.
 *             -Added function arguments site_i_index and site_j_index 
 *              to print_param_and_component_info to allow printing of the 
 *              Freezing Height and CIP grid values at the radar location.
 *
 * 
 *=======================================================================
 *
 *   DESCRIPTION:
 *
 *   This file contains functions that will pack model data into an
 *   external data grid of type RPGP_ext_data_t.
 *
 *   FUNCTIONS:
 *
 *   int fill_rpgp_ext_data_t( int ext_grid_type, char *product_description,
 *                             RPGCS_model_attr_t *model_attrs, 
 *                             RPGCS_model_grid_data_t *grid_data, RPGP_ext_data_t *ext_data )
 *   int fill_grid_components( int comp_num, int ext_grid_type,
 *                             RPGCS_model_grid_data_t *grid_data )
 *   int fill_grid_data( int comp_num, int ext_grid_type, RPGCS_model_grid_data_t *grid_data,
 *                       RPGP_grid_t *grid )
 *   int fill_product_params( int ext_grid_type, char *product_description, 
 *                            RPGCS_model_attr_t*model_attrs )
 *   int set_int_param( RPGP_parameter_t *param, char *id, char *id_name, int value, char *units )
 *   int set_float_param( RPGP_parameter_t *param, char *id, char *id_name, float value, char *units )
 *   int set_string_param( RPGP_parameter_t *param, char *id, char *id_name, char *value, char *units )
 *   void print_param_and_component_info( int site_i_index, int site_j_index )
 *
 *   NOTES:    
 *
 *   Some of the code functionality was adopted from public domain AWIPS C++ software
 *   acquired from ESRL (author - G. Joanne Edwards).
 *
 *
 *=======================================================================
 *$)
 */

#include <pack_grid_data.h>


/******************************************************************

   Description:
      This high level function makes calls to fill the component
      parameters of an external model data structure.

   Inputs:
      ext_grid_type - index indicating type of grid data to pack
      product_description - name defining product to be packed
      model_attrs - attributes of current model
      grid_data - grid freezing height or CIP data for the model of interest

   Output:
      ext_data - filled external grid structure of type RPGP_ext_data_t

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int fill_rpgp_ext_data_t( int ext_grid_type, char *product_description, RPGCS_model_attr_t *model_attrs,
                          RPGCS_model_grid_data_t *grid_data, RPGP_ext_data_t *ext_data ) {

  int ret=0, comp_num = 0;

  grid_ext_data = ext_data;

  grid_ext_data->numof_components = grid_data->num_levels;
  grid_ext_data->components = (void **) malloc( sizeof(char *) * grid_ext_data->numof_components );

  /* fill a grid component for each product and level */
  for( comp_num = 0; comp_num < grid_ext_data->numof_components; comp_num++ )
    ret = fill_grid_components( comp_num, ext_grid_type, grid_data );

  /* fill the parameters of a RPGP_ext_data_t structure */
  ret = fill_product_params( ext_grid_type, product_description, model_attrs );

  return( 1 );

/* End of fill_rpgp_ext_data_t() */
}

/******************************************************************

   Description:
      This function fills the component parameter attributes of the 
      external model data structure.

   Inputs:
      comp_num - component index value
      ext_grid_type - index indicating type of grid data to pack
      grid_data - grid freezing height or CIP data for the model of interest

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int fill_grid_components( int comp_num, int ext_grid_type,
                          RPGCS_model_grid_data_t *grid_data ) {

  int value, ret;
  char *id = NULL, *id_name = NULL, *units = NULL;
  RPGP_grid_t *grid = NULL;

  /* fill grid component parameters */
  grid = (RPGP_grid_t *) malloc( sizeof (RPGP_grid_t) );
  grid_ext_data->components[comp_num] = (char *)grid;

  grid->numof_comp_params = 1;
  grid->comp_params = (RPGP_parameter_t *) malloc( grid->numof_comp_params * 
                                                   sizeof(RPGP_parameter_t) );

  /* further define the parameter attributes */
  id = RPGCS_MODEL_LEVEL;
  value = (int)grid_data->params[comp_num]->level_value;

  if( ext_grid_type == 1 ) {
    id_name = FREEZING_GRID;
    units = "";
  }
  else {
    id_name = RPGCS_PRESSURE_LEVEL;
    if( grid_data->params[comp_num]->level_units == 30 )
      units = RPGCS_MILLIBAR_STR;
    else if( grid_data->params[comp_num]->level_units == 31 )
      units = RPGCS_PASCAL_STR;
    else
      units = NULL;
  }

  /* set the id and attributes for an RPGP_parameter_t */
  ret = set_int_param( grid->comp_params, id, id_name, value, units );

  /* fill in grid component data structure */
  ret = fill_grid_data( comp_num, ext_grid_type, grid_data, grid );

  grid->comp_type = RPGP_GRID_COMP;
  grid->n_dimensions = 2;
  grid->dimensions = (int *) malloc( 2 * sizeof(int) );
  grid->dimensions[0] = grid_data->dimensions[0];
  grid->dimensions[1] = grid_data->dimensions[1];
  grid->grid_type = RPGP_GT_EQUALLY_SPACED;

  return( 1 );

/* End of fill_grid_components() */
}

/******************************************************************

   Description:
      This function fills the grid attributes and product data of the 
      external model data structure.

   Inputs:
      comp_num - component index value
      ext_grid_type - index indicating type of grid data to pack
      grid_data - grid freezing height or CIP data for the model of interest

   Output:
      grid - pointer to the product data

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int fill_grid_data( int comp_num, int ext_grid_type, RPGCS_model_grid_data_t *grid_data,
                    RPGP_grid_t *grid ) {

  char buf[MAX_ATTR_LENGTH];
  char *data_field = NULL;
  int xdim, ydim, units=0;
  int nlevels;

  xdim = grid_data->dimensions[0];
  ydim = grid_data->dimensions[1];
  nlevels = grid_data->num_levels;

  if ( ext_grid_type == 1 ) {

    /* Define field name and units for FRZ grid */
    if ( comp_num == 0 ) {
        data_field = FRZ_GRID_RANGE;
        units = RPGCS_KILOMETER_UNITS;
    }
    else if ( comp_num == 1 ) {
        data_field = FRZ_GRID_AZIMUTH;
        units = RPGCS_UNKNOWN_UNITS;
    }
    else if ( comp_num == 2 ) {
        data_field = FRZ_GRID_NUM_ZERO_X;
        units = RPGCS_UNKNOWN_UNITS;
    }
    else if ( comp_num >= 3 && comp_num <= 8 ) {
        data_field = FRZ_GRID_HEIGHT_ZERO;
        units = RPGCS_KILOMETER_UNITS;
    }
    else if ( comp_num == 9 ) {
        data_field = FRZ_GRID_HEIGHT_ZDR_T1;
        units = RPGCS_KILOMETER_UNITS;
    }
    else if ( comp_num == 10 ) {
        data_field = FRZ_GRID_HEIGHT_ZDR_T2;
        units = RPGCS_KILOMETER_UNITS;
    }
    else if ( comp_num == 11 ) {
        data_field = FRZ_GRID_HEIGHT_MINUS20;
        units = RPGCS_KILOMETER_UNITS;
    }
    else if ( comp_num >= 12 && comp_num <= 14 ) {
        data_field = FRZ_GRID_MAX_TEMP_WL;
        units = RPGCS_CELSIUS_UNITS;
    }
    else if ( comp_num >= 15 && comp_num <= 16 ) {
        data_field = FRZ_GRID_MIN_TEMP_CL;
        units = RPGCS_CELSIUS_UNITS;
    }
    else if ( comp_num >= 17 && comp_num <= 19 ) {
        data_field = FRZ_GRID_SPARE;
        units = RPGCS_UNKNOWN_UNITS;
    }

  }

  else {

    /* Define field name and units for CIP grids and geopotential height grids */
    if ( comp_num < nlevels/2 ) {
      data_field = grid_data->field;
      units = RPGCS_PERCENT_UNITS;
    }
    else {
      data_field = RPGCS_MODEL_GH;
      units = RPGCS_KILOMETER_UNITS;
    }

  }

  /* Define data attributes */
  switch ( units ) {

    case RPGCS_UNKNOWN_UNITS:
      sprintf( buf,"name=%s;type=float;unit=none;", data_field );
      break;

    case RPGCS_METER_UNITS:
      sprintf( buf,"name=%s;type=float;unit=%s;", data_field, RPGCS_METER_STR );
      break;

    case RPGCS_KILOMETER_UNITS:
      sprintf( buf,"name=%s;type=float;unit=%s;", data_field, RPGCS_KILOMETER_STR );
      break;

    case RPGCS_KELVIN_UNITS:
      sprintf( buf,"name=%s;type=float;unit=%s;", data_field, RPGCS_KELVIN_STR );
      break;

    case RPGCS_CELSIUS_UNITS:
      sprintf( buf,"name=%s;type=float;unit=%s;", data_field, RPGCS_CELSIUS_STR );
      break;

    case RPGCS_FAHRENHEIT_UNITS:
      sprintf( buf,"name=%s;type=float;unit=%s;", data_field, RPGCS_FAHRENHEIT_STR );
      break;

    case RPGCS_PERCENT_UNITS:
      sprintf( buf,"name=%s;type=float;unit=%s;", data_field, RPGCS_PERCENT_STR );
      break;

    default:
      sprintf( buf,"name=%s;type=float;", data_field );
      break;

  }

  grid->data.attrs = (char *) malloc( strlen(buf) + 1 );
  strcpy( grid->data.attrs, buf );
  grid->data.attrs[strlen(buf)] = 0;

  grid->data.data = malloc( xdim * ydim * sizeof(float) );

  int i;
  float temp_data[xdim*ydim];

  /* Store the grid data into a 1-dimensional array */
  for (i = 0; i < xdim*ydim; i++)
    temp_data[i] = grid_data->data[comp_num][i];

  memcpy( grid->data.data, (float *)temp_data, (xdim * ydim * sizeof(float)) );

  return( 1 );

/* End of fill_grid_data() */
}

/******************************************************************

   Description:
      This function fills the product parameters of the 
      external model data structure.

   Inputs:
      ext_grid_type - index indicating type of grid data to pack
      product_description - name defining product to be packed
      model_attrs - attributes of current model

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int fill_product_params( int ext_grid_type, char *product_description, RPGCS_model_attr_t*model_attrs ) {

  char *name = "Model Derived Grid Data";
  int ret, value_int = 0;
  float value_float = 0.0;
  time_t cur_time = time(NULL);

  int yy, mm, dd, hh, mn, ss;
  int ctime, jdate;
  char temp_str[15];

  grid_ext_data->name = (char *) malloc( strlen(name) + 1 );
  strcpy( grid_ext_data->name, name );
  grid_ext_data->name[strlen(name)] = 0;

  grid_ext_data->description = (char *) malloc( strlen(product_description) + 1 );
  strcpy( grid_ext_data->description, product_description );
  grid_ext_data->description[strlen(product_description)] = 0;

  if ( ext_grid_type == FRZ_GRID )
    grid_ext_data->product_id = MODEL_FRZ_GRID;
  else if ( ext_grid_type == CIP_GRID )
    grid_ext_data->product_id = MODEL_CIP_GRID;
  else
    grid_ext_data->product_id = 99;

  grid_ext_data->type = RPGP_EXTERNAL;
  grid_ext_data->spare[0] = 0;
  grid_ext_data->spare[1] = 0;
  grid_ext_data->spare[2] = 0;
  grid_ext_data->spare[3] = 0;
  grid_ext_data->spare[4] = 0;

  /* get current time, convert to unix time, and set to gen_time */
  ret = RPGCS_get_date_time( &ctime, &jdate );
  hh = (int)( ctime / SECS_IN_HOUR );
  mn = (int)( (ctime - (hh * SECS_IN_HOUR)) / 60 );
  ss = ctime - (hh * SECS_IN_HOUR) - (mn * 60);

  ret = RPGCS_julian_to_date( jdate, &yy, &mm, &dd );
  /*fprintf( stderr,"Current Time: %d%02d%02d %02d:%02d:%02d\n", yy, mm, dd, hh, mn, ss );*/

  ret = RPGCS_ymdhms_to_unix_time( &cur_time, yy, mm, dd, hh, mn, ss );
  /*fprintf( stderr,"Current Unix Time: %d\n\n", (unsigned int)cur_time );*/

  grid_ext_data->gen_time = (unsigned int)cur_time;
  grid_ext_data->compress_type = 0;
  grid_ext_data->size_decompressed = 0;
  grid_ext_data->numof_prod_params = PARAMETER_IDS;

  /* fill the parameter ids */
  RPGP_parameter_t *params = (RPGP_parameter_t *) malloc( grid_ext_data->numof_prod_params * sizeof(RPGP_parameter_t) );
  grid_ext_data->prod_params = params;

  set_string_param( params+0, "mod_name", "Model Name", STR_RUC13, "" );

  ret = RPGCS_unix_time_to_ymdhms( model_attrs->model_run_time, &yy, &mm, &dd, &hh, &mn, &ss );
  sprintf( temp_str, "%d%02d%02d", yy, mm, dd );
  set_string_param( params+1, "mod_run_date", RPGCS_MODEL_RUN_DATE, (char *) temp_str, "" );

  sprintf( temp_str, "%02d:%02d:%02d", hh, mn, ss );
  set_string_param( params+2, "mod_run_time", RPGCS_MODEL_RUN_TIME, (char *) temp_str, "" );

  ret = RPGCS_unix_time_to_ymdhms( model_attrs->valid_time, &yy, &mm, &dd, &hh, &mn, &ss );
  sprintf( temp_str, "%d%02d%02d", yy, mm, dd );
  set_string_param( params+3, "val_date", RPGCS_VALID_DATE, (char *) temp_str, "" );

  sprintf( temp_str, "%02d:%02d:%02d", hh, mn, ss );
  set_string_param( params+4, "val_time", RPGCS_VALID_TIME, (char *) temp_str, "" );

  value_int = (model_attrs->valid_time - model_attrs->model_run_time) / 3600;
  set_int_param( params+5, "forecast_hr", RPGCS_FORECAST_HOUR, value_int, "" );

  set_string_param( params+6, "coord_system", "Coordinate System", "Cartesian", "" );

  if ( model_attrs->projection == 1 )
    set_string_param( params+7, "proj", RPGCS_PROJECTION, "Lambert Conformal", "" );
  else
    set_string_param( params+7, "proj", RPGCS_PROJECTION, "Unknown Projection", "" );

  value_float = (float)model_attrs->grid_lower_left.latitude;
  set_float_param( params+8, "lat_lower_left", RPGCS_LATITUDE_LLC, value_float, "degrees" );

  value_float = (float)model_attrs->grid_lower_left.longitude;
  set_float_param( params+9, "lon_lower_left", RPGCS_LONGITUDE_LLC, value_float, "degrees" );

  value_float = (float)model_attrs->grid_upper_right.latitude;
  set_float_param( params+10,"lat_upper_right" , RPGCS_LATITUDE_URC, value_float, "degrees" );

  value_float = (float)model_attrs->grid_upper_right.longitude;
  set_float_param( params+11, "lon_upper_right", RPGCS_LONGITUDE_URC, value_float, "degrees" );

  value_float = (float)model_attrs->tangent_point.latitude;
  set_float_param( params+12, "lat_tang_pt", RPGCS_LATITUDE_TANP, value_float, "degrees" );

  value_float = (float)model_attrs->tangent_point.longitude;
  set_float_param( params+13, "lon_tang_pt", RPGCS_LONGITUDE_TANP, value_float, "degrees" );

  value_int = model_attrs->dimensions[0];
  set_int_param( params+14, "numXpts", RPGCS_X_DIMENSION, value_int, "" );

  value_int = model_attrs->dimensions[1];
  set_int_param( params+15, "numYpts", RPGCS_Y_DIMENSION, value_int, "" );

  return( 1 );

/* End of fill_product_params() */
}

/******************************************************************

   Description:
      This function sets the id and attributes for an RPGP_parameter_t
      structure.

   Inputs:
      param - pointer to an RPGP_parameter_t
      id - pointer to parameter id
      id_name - pointer to parameter id long name
      value - pointer to integer value of grid level
      units - pointer to name of data units

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int set_int_param( RPGP_parameter_t *param, char *id, char *id_name, int value, char *units ) {

  char buf[MAX_ATTR_LENGTH];

  param->id = (char *)malloc( strlen(id) + 1 );
  strcpy( param->id, id );
  param->id[strlen(id)] = 0;

  if ( units != NULL )
    sprintf( buf,"name=%s;type=int;value=%d;unit=%s;", id_name, value, units );
  else
    sprintf( buf,"name=%s;type=int;value=%d;", id_name, value );

  param->attrs = (char *) malloc( strlen(buf) + 1 );
  strcpy( param->attrs, buf );
  param->attrs[strlen(buf)] = 0;

  return( 1 );

/* End of set_int_param() */
}

/******************************************************************

   Description:
      This function sets the id and attributes for an RPGP_parameter_t
      structure.

   Inputs:
      param - pointer to an RPGP_parameter_t
      id - pointer to parameter id
      id_name - pointer to parameter id long name
      value - pointer to float value of grid level
      units - pointer to name of data units

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int set_float_param( RPGP_parameter_t *param, char *id, char *id_name, float value, char *units ) {

  char buf[MAX_ATTR_LENGTH];

  param->id = (char *)malloc( strlen(id) + 1 );
  strcpy( param->id, id );
  param->id[strlen(id)] = 0;

  if ( units != NULL )
    sprintf( buf,"name=%s;type=float;value=%f;unit=%s;", id_name, value, units );
  else
    sprintf( buf,"name=%s;type=float;value=%f;", id_name, value );

  param->attrs = (char *) malloc( strlen(buf) + 1 );
  strcpy( param->attrs, buf );
  param->attrs[strlen(buf)] = 0;

  return(1);

/* End of set_float_param() */
}

/******************************************************************

   Description:
      This function sets the id and attributes for an RPGP_parameter_t
      structure.

   Inputs:
      param - pointer to an RPGP_parameter_t
      id - pointer to parameter id
      id_name - pointer to parameter id long name
      value - pointer to character value of grid level
      units - pointer to name of data units

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int set_string_param( RPGP_parameter_t *param, char *id, char *id_name, char *value, char *units ) {

  char buf[MAX_ATTR_LENGTH];

  param->id = (char *)malloc( strlen(id) + 1 );
  strcpy( param->id, id );
  param->id[strlen(id)] = 0;

  if ( units != NULL )
    sprintf( buf,"name=%s;type=string;value=%s;unit=%s;", id_name, value, units );
  else
    sprintf( buf,"name=%s;type=string;value=%s;", id_name, value );

  param->attrs = (char *) malloc( strlen(buf) + 1 );
  strcpy( param->attrs, buf );
  param->attrs[strlen(buf)] = 0;

  return( 1 );

/* End of set_string_param() */
}

/******************************************************************

   Description:
      This function prints the parameter values and the
      data values for the grid bin closest to the radar for each
      component defined in the RPGP_ext_data_t structure.

   Output:
      none

   Returns:
      none

******************************************************************/
void print_param_and_component_info( int site_i_index, int site_j_index ) {

  int i = 0;

  fprintf( stderr,"Name: %s\n", grid_ext_data->name );
  fprintf( stderr,"Description: %s\n", grid_ext_data->description );
  fprintf( stderr,"Product id: %d\n", grid_ext_data->product_id );
  fprintf( stderr,"type: %d\n", grid_ext_data->type );
  fprintf( stderr,"Spare: %d\n", grid_ext_data->spare[0] );
  fprintf( stderr,"Spare: %d\n", grid_ext_data->spare[1] );
  fprintf( stderr,"Spare: %d\n", grid_ext_data->spare[2] );
  fprintf( stderr,"Spare: %d\n", grid_ext_data->spare[3] );
  fprintf( stderr,"Spare: %d\n", grid_ext_data->spare[4] );
  fprintf( stderr,"Gen time: %d\n", grid_ext_data->gen_time );
  fprintf( stderr,"Compress type: %d\n", grid_ext_data->compress_type );
  fprintf( stderr,"Size decompressed: %d\n", grid_ext_data->size_decompressed );
  fprintf( stderr,"Num of params: %d\n", grid_ext_data->numof_prod_params );
  for ( i = 0; i < grid_ext_data->numof_prod_params; i++ )
  {
    fprintf( stderr, "Params[%d] id = %s   attrs = %s\n", i, 
             grid_ext_data->prod_params[i].id, grid_ext_data->prod_params[i].attrs );
  }
  fprintf( stderr, "Num of comps: %d\n", grid_ext_data->numof_components );

  RPGP_grid_t *grid;
  RPGP_parameter_t *gridParam;
  float *dataVals;
  int siteGridIndex;

  for ( i = 0; i < grid_ext_data->numof_components; i++ )
  {
    grid = (RPGP_grid_t *)grid_ext_data->components[i];
    fprintf( stderr, "Component[%d] ---------------\n", i );
    fprintf( stderr, "comp type: %d\n", grid->comp_type );
    fprintf( stderr, "n dimensions: %d\n", grid->n_dimensions );
    fprintf( stderr, "dimension[0]: %d\n", grid->dimensions[0] );
    fprintf( stderr, "dimensions[1]: %d\n", grid->dimensions[1] );
    fprintf( stderr, "grid type: %d\n", grid->grid_type );
    fprintf( stderr, "num of comp params: %d\n", grid->numof_comp_params );
    gridParam = (RPGP_parameter_t *)grid->comp_params;
    fprintf( stderr, "param id = %s   attrs = %s\n", gridParam->id, gridParam->attrs );
    fprintf( stderr, "data attrs: %s\n", grid->data.attrs );
    dataVals = (float *)grid->data.data;
    siteGridIndex = ( site_j_index * grid->dimensions[1] + site_i_index );
    fprintf( stderr, "data val at grid bin closest to radar: %.2f\n", dataVals[siteGridIndex] );
  }
  fprintf( stderr, "\n" );

/* End of print_param_and_component_info() */
}
