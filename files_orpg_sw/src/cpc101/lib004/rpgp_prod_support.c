/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/05 19:19:52 $
 * $Id: rpgp_prod_support.c,v 1.7 2007/02/05 19:19:52 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*** Global Include Files ***/
#include <rpgc.h>
#include <rpgcs.h>
#include <orpg_product.h>
#include <orpgsite.h>
#include <rpgp.h>

/* Static Function Prototypes. */
static void Add_param_attr( int type, char *tmp_buf, char *val_str );

/*\///////////////////////////////////////////////////////////////////////////////////

   Description:
      Populates most of the RPGP_product_t structure of the generic product format.
      The number of product parameters, the product parameters, the number of components
      and components are populated using RPGP_finish_RPGP_product_t().

   Inputs:
      prod_id - Product ID.
      vol_num - volume scan number (1-80).
      name - string specifying name of product.
      description - string specifying description of product.
      prod - pointer to RPGP_product_t to be populated.

   Returns:
      Negative value on error, 0 otherwise.

///////////////////////////////////////////////////////////////////////////////////\*/
int RPGP_build_RPGP_product_t( int prod_id, int vol_num, char *name, char *description, 
                               RPGP_product_t *prod ){

   int prod_code, type, mode, currdate, currtime, elev_ind;
   static Summary_Data *summary = NULL;
   static char *null_string = "";

   /* Set the generic product header fields. */

   /* Set the product name field. */
   if( name != NULL ){

      prod->name = (char *) malloc( strlen( name ) + 1 );
      strcpy( prod->name, name );

   }
   else
      prod->name = null_string;

   /* Set the product description field. */
   if( description != NULL ){

      prod->description = (char *) malloc( strlen( description ) + 1 );
      strcpy( prod->description, description );

   }
   else
      prod->description = null_string;

   /* Set the product ID (code) field. */
   prod_code = ORPGPAT_get_code( prod_id );
   if( prod_code > 0 )
      prod->product_id = prod_code;
   
   else
      prod->product_id = prod_id;

   /* Set the product type field. */
   type = ORPGPAT_get_type( prod_id );
   switch( type ){

      case TYPE_VOLUME:
         prod->type = RPGP_VOLUME;
         break;

      case TYPE_ELEVATION:
         prod->type = RPGP_ELEVATION;
         break;

      case TYPE_TIME:
         prod->type = RPGP_TIME;
         break;

      case TYPE_ON_DEMAND:
         prod->type = RPGP_ON_DEMAND;
         break;

      case TYPE_ON_REQUEST:
         prod->type = RPGP_ON_REQUEST;
         break;

      case TYPE_RADIAL:
         prod->type = RPGP_RADIAL;
         break;

      case TYPE_EXTERNAL:
         prod->type = RPGP_EXTERNAL;
         break;

      default:
         prod->type = RPGP_VOLUME;
         break;

   }

   /* Set the product generation time field. */
   prod->gen_time = time( NULL );

   /* Set the radar name field. */
   prod->radar_name = (char *) malloc(5);
   ORPGSITE_get_string_prop( ORPGSITE_RPG_NAME, prod->radar_name, 5 );

   /* This information is found in the Product Description Block. */

   /* Set the radar latitude/longitude and height fields. */
   prod->radar_lat = ((float) ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE )) / 1000.0;
   prod->radar_lon = ((float) ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE )) / 1000.0;
   prod->radar_height = ((float) ORPGSITE_get_float_prop( ORPGSITE_RDA_ELEVATION )) * FT_TO_M;
                         

   /* Set volume scan start time, elevation time, volume number, operational mode, 
      vcp and elevation number. 
 
     Read scan summary for VCP number, volume date, volume time,
     and operational mode.

     Note: We first have to register the scan summary buffer if it is
     not already registered.  Then we must read the scan summary data. */
   if( (summary = (Summary_Data *) SS_get_summary_data()) == NULL ){
   
      summary = (Summary_Data *) malloc( sizeof( Summary_Data ) );
      if( summary == NULL ){

         LE_send_msg( GL_MEMORY, "malloc Failed For %d Bytes\n",
                      sizeof( Summary_Data ) );
         return(-1);
                   
      }
      
      SS_send_summary_array( (int *) summary );
      SS_read_scan_summary();
   
   }
   else{

      SS_read_scan_summary();
      summary = (Summary_Data *) SS_get_summary_data();
   
   }
   
   /* If for some reason scan summary data can not be read or is otherwise
      unavailable, return failure. */
   if( summary == NULL ){
      
      LE_send_msg( GL_ERROR, "Unable To Read Scan Summary Data\n" );
      return(-1);

   }

   mode = summary->scan_summary[ vol_num].weather_mode;
   switch( mode ){

      case MAINTENANCE_MODE:
         prod->operation_mode = RPGP_OP_MAINTENANCE;
         break;

      case CLEAR_AIR_MODE:
         prod->operation_mode = RPGP_OP_CLEAR_AIR;
         break;

      case PRECIPITATION_MODE:
      default:
         prod->operation_mode = RPGP_OP_WEATHER;
         break;

   }

   prod->vcp = (short) summary->scan_summary[ vol_num ].vcp_number;
   prod->volume_number = vol_num;

   currdate = summary->scan_summary[ vol_num ].volume_start_date;
   currtime = summary->scan_summary[ vol_num ].volume_start_time;
   prod->volume_time = (currdate-1)*86400 + currtime;


   elev_ind = 0;
   prod->elevation_time = (unsigned int)0;
   if( type == TYPE_ELEVATION ){

      PS_get_current_elev_index( &elev_ind );
      prod->elevation_number  = elev_ind;
      prod->elevation_angle = RPGCS_get_target_elev_ang( prod->vcp, elev_ind ) / 10.;

   }

   prod->compress_type     = (short)0;
   prod->size_decompressed = (int)0;

   return 0;

}

/*\/////////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Finishes up with initializing the RPGP_product_t structure.
//
//   Inputs:
//      numof_params - number of product parameters.
//      prod_params - pointer to the product parameters.
//      numof_components - number of product components.
//      components - pointer to array of pointers to components.
//
//   Returns:
//      -1 on error, or 0 otherwise.
//
/////////////////////////////////////////////////////////////////////////////////\*/
int RPGP_finish_RPGP_product_t( RPGP_product_t *prod, int numof_prod_params, 
                                RPGP_parameter_t *prod_params, int numof_components, 
                                void **components ){

   /* Set the number of product parameters and the parameters. */
   if( numof_prod_params == 0 ){

      prod->numof_prod_params = 0;
      prod->prod_params = NULL;

   }
   else if( (numof_prod_params > 0) && (prod_params != NULL) ){

      prod->numof_prod_params = numof_prod_params;
      prod->prod_params = prod_params;

   }
   else{

      RPGC_log_msg( GL_ERROR, "Invalid Argument in RPGP_finish_RPGP_product_t\n" );
      return(-1);

   }

   /* Set the number of component parameters and the components. */
   if( numof_components == 0 ){

      prod->numof_components = 0;
      prod->components = NULL;

   }
   else if( (numof_components > 0) && (components != NULL) ){

      prod->numof_components = numof_components;
      prod->components = components;

   }
   else{

      RPGC_log_msg( GL_ERROR, "Invalid Argument in RPGP_finish_RPGP_product_t\n" );
      return(-1);

   }

   return 0;
   
} /* End of RPGP_finish_RPGP_product_t() */

#define MAX_ATTR_LENGTH           512

/*\/////////////////////////////////////////////////////////////////////////////////

   Description:
      Convenience function for setting Integer type parameters.

   Inputs:
      id - parameter ID 
      name - parameter name
      type - type of parameter
      value - pointer to parameter value 
      size = number of values

   Outputs:
      param - RPGP_parameter_t structure

   Notes:
      Additional arguments are specified using tuples <item, value> where
      the supported items are defined in rpgp_prod_support.h and 
      value are strings.  The format of the value strings depends on the item.
      See orpg_product.h for information concerning what is expected.

/////////////////////////////////////////////////////////////////////////////////\*/
int RPGP_set_int_param( RPGP_parameter_t *param, /* (IN/OUT) Parameter to be set */
                        char *id, /* (IN) Single token parameter ID */
                        char *name, /* (IN) Parameter name */
                        int type, /* (IN) Type of argument */
                        void *value, /* (IN) Value of this parameter */
                        int size, /* (IN) Number of values */
                        int scale, /* (INT) Scaling factor */
                        ... ){

   char buf[MAX_ATTR_LENGTH], tmp_buf[MAX_ATTR_LENGTH], *val_str;
   va_list ap;
   int i;
   
   /* Set the parameter ID. */
   param->id = (char *) malloc( strlen(id) + 1 );
   strcpy( param->id, id );
   
   /* Begin building the attr field.   "Type" and "Value" are required
      fields. */
   memset( buf, 0, MAX_ATTR_LENGTH );

   switch( type ){

      case RPGP_TYPE_INT:
      {
         int *v = (int *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=int; Value=", name );
 
         else
            sprintf( buf, "Type=int; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;

      }

      case RPGP_TYPE_UINT:
      {

         unsigned int *v = (unsigned int *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=uint; Value=", name );

         else
            sprintf( buf, "Type=uint; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;

      }
      case RPGP_TYPE_SHORT:
      {

         short *v = (short *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=short; Value=", name );

         else
            sprintf( buf, "Type=short; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;

      }

      case RPGP_TYPE_USHORT:
      {

         unsigned short *v = (unsigned short *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=ushort; Value=", name );

         else
            sprintf( buf, "Type=ushort; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;

      }

      case RPGP_TYPE_BYTE:
      {

         char *v = (char *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=byte; Value=", name );

         else
            sprintf( buf, "Type=byte; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;

      }

      case RPGP_TYPE_UBYTE:
      {

         unsigned char *v = (unsigned char *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=ushort; Value=", name );

         else
            sprintf( buf, "Type=ushort; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;
   
      }

      case RPGP_TYPE_BIT:
      {

         unsigned int *v = (unsigned int *) value;

         if( (name != NULL) && (strlen(name) > 0) )
            sprintf( buf, "Name=%s; Type=bit; Value=", name );

         else
            sprintf( buf, "Type=bit; Value=" );

         for( i = 0; i < size; i++ )
            sprintf( &buf[strlen(buf)], "%d,", scale*(*(v+i)) );

         break;

      }

      default:
         return(-1);
   
   }

   /* Replace last comma */
   if (size > 0)
      strcpy( &buf[strlen(buf) - 1], ";" );

   else
      strcpy( &buf[strlen(buf)], ";" );

         

   /* Check for any optionally defined attributes. */
   va_start( ap, scale );
   while(1){

      type = va_arg( ap, int );
      if( type == 0 )
         break;

      val_str = va_arg( ap, char* );
      if( val_str != NULL )
         Add_param_attr( type, tmp_buf, val_str );

      /* If there is something to add and we aren't going to overflow the buffer,
         add the attribute. */
      if( (tmp_buf[0] != '\0') 
                   &&  
          ((strlen(tmp_buf) + strlen(buf)) < MAX_ATTR_LENGTH) )
         strcat( buf, tmp_buf );

      else
         break;

   }
   va_end( ap );

   /* Copy additional attributes to the parameter. */
   param->attrs = (char *) malloc( strlen(buf) + 1  );
   strcpy( param->attrs, buf );

   return 0;

} /* End of RPGP_set_int_param() */

/*\/////////////////////////////////////////////////////////////////////////////////

   Description:
      Convenience function for setting Integer type parameters.

   Inputs:
      id - parameter ID 
      type - type of parameter
      value - pointer to parameter value 
      size = number of values

   Outputs:
      param - RPGP_parameter_t structure

   Notes:
      Additional arguments are specified using tuples <item, value> where
      the supported items are defined in rpgp_prod_support.h and 
      value are strings.  The format of the value strings depends on the item.
      See orpg_product.h for information concerning what is expected.

/////////////////////////////////////////////////////////////////////////////////\*/
int RPGP_set_string_param( RPGP_parameter_t *param, /* (IN/OUT) Parameter to be set */
                           char *id, /* (IN) Single token parameter ID */
                           char *name, /* (IN) Parameter name */
                           void *value, /* (IN) Value of this parameter */
                           int size, /* (IN) Number of values */
                           ... ){

   char buf[MAX_ATTR_LENGTH], tmp_buf[MAX_ATTR_LENGTH], *val_str;
   va_list ap;
   int type, i;
   
   /* Set the parameter ID. */
   param->id = (char *) malloc( strlen(id) + 1 );
   strcpy( param->id, id );
   
   /* Begin building the attr field.   "Type" and "Value" are required
      fields. */
   memset( buf, 0, MAX_ATTR_LENGTH );

   if( size > 1 ){

      char **v = (char **) value;

      if( (name != NULL) && (strlen(name) > 0) )
         sprintf( buf, "Name=%s; Type=string; Value=", name );

      else
         sprintf( buf, "Type=string; Value=" );

      for( i = 0; i < size; i++ )
         sprintf( &buf[strlen(buf)], "%s,", v[i] );

   }
   else{

      char *v = (char *) value;

      if( (name != NULL) && (strlen(name) > 0) )
         sprintf( buf, "Name=%s; Type=string; Value=", name );

      else
         sprintf( buf, "Type=string; Value=" );

      for( i = 0; i < size; i++ )
         sprintf( &buf[strlen(buf)], "%s,", v );

   }

   /* Replace last comma */
   if (size > 0)
      strcpy( &buf[strlen(buf) - 1], ";" );

   else
      strcpy( &buf[strlen(buf)], ";" );

         

   /* Check for any optionally defined attributes. */
   va_start( ap, size );
   while(1){

      type = va_arg( ap, int );
      if( type == 0 )
         break;

      val_str = va_arg( ap, char* );
      if( val_str != NULL )
         Add_param_attr( type, tmp_buf, val_str );

      /* If there is something to add and we aren't going to overflow the buffer,
         add the attribute. */
      if( (tmp_buf[0] != '\0') 
                   &&  
          ((strlen(tmp_buf) + strlen(buf)) < MAX_ATTR_LENGTH) )
         strcat( buf, tmp_buf );

      else
         break;

   }
   va_end( ap );

   /* Copy additional attributes to the parameter. */
   param->attrs = (char *) malloc( strlen(buf) + 1  );
   strcpy( param->attrs, buf );

   return 0;

} /* End of RPGP_set_string_param() */

/*\/////////////////////////////////////////////////////////////////////////////////

   Description:
      Convenience function for setting float type parameters.

   Inputs:
      id - parameter ID 
      type - type of parameter
      name - parameter name
      value - pointer to parameter value 
      fld_width - maximum number of characters in float
      precision - number of digits right of decimal point 
      scale - scaling factor 

   Outputs:
      param - RPGP_parameter_t structure

   Notes:
      Additional arguments are specified using tuples <item, value> where
      the supported items are defined in rpgp_prod_support.h and 
      value are strings.  The format of the value strings depends on the item.
      See orpg_product.h for information concerning what is expected.

/////////////////////////////////////////////////////////////////////////////////\*/
int RPGP_set_float_param( RPGP_parameter_t *param, /* (IN/OUT) Parameter to be set */
                          char *id, /* (IN) Single token parameter ID */
                          char *name, /* (IN) Parameter name */
                          int type, /* (IN) Type of argument */
                          void *value, /* (IN) Value of this parameter */
                          int size, /* (IN) Number of elements (1 for scalar) */
                          const int fld_width,/* (IN) Max number of characters in float
                                                 field. (0 = no limit) */
                          const int precision, /* (IN) Max digits right of decimal point. */ 
                          const double scale, /* (IN) Scaling parameter */
                          ... ){

   char buf[MAX_ATTR_LENGTH], tmp_buf[MAX_ATTR_LENGTH], *val_str;
   va_list ap;
   int i;
   
   /* Set the parameter ID. */
   param->id = (char *) malloc( strlen(id) + 1 );
   strcpy( param->id, id );
   
   /* Begin building the attr field.   "Type" and "Value" are required
      fields. */
   memset( buf, 0, MAX_ATTR_LENGTH );

   if( type == RPGP_TYPE_FLOAT ){

      float *v = (float *) value;

      if( (name != NULL) && (strlen(name) > 0) )
         sprintf( buf, "Name=%s; Type=float; Value=", name );

      else
         sprintf( buf, "Type=float; Value=" );

      for( i = 0; i < size; i++ ){

         if( fld_width > 0 )
            sprintf( &buf[strlen(buf)], "%*.*f,", fld_width, precision, scale*(*(v+i)) );

         else
            sprintf( buf, "%f,", scale*(*(v+i)) );

      }

   }
   else if( type == RPGP_TYPE_DOUBLE ){

      double *v = (double *) value;

      if( (name != NULL) && (strlen(name) > 0) )
         sprintf( buf, "Name=%s; Type=double; Value=", name );

      else
         sprintf( buf, "Type=double; Value=" );

      for( i = 0; i < size; i++ ){

         if( fld_width > 0 )
            sprintf( buf, "%*.*f;", fld_width, precision, scale*(*(v+i)) );      
    
         else 
            sprintf( buf, "%f;", scale*(*(v+i)) );

      }

   }
   else
      return(-1);

   /* Replace last comma */
   if (size > 0)
      strcpy( &buf[strlen(buf) - 1], ";" );

   else
      strcpy( &buf[strlen(buf)], ";" );

   /* Check for any optionally defined attributes. */
   va_start( ap, scale );
   while(1){

      type = va_arg( ap, int );
      if( type == 0 )
         break;

      val_str = va_arg( ap, char* );
      if( val_str != NULL )
         Add_param_attr( type, tmp_buf, val_str );

      /* If there is something to add and we aren't going to overflow the buffer,
         add the attribute. */
      if( (tmp_buf[0] != '\0') 
                   &&  
          ((strlen(tmp_buf) + strlen(buf)) < MAX_ATTR_LENGTH) )
         strcat( buf, tmp_buf );

      else
         break;

   }
   va_end( ap );

   /* Copy additional attributes to the parameter. */
   param->attrs = (char *) malloc( strlen(buf) + 1  );
   strcpy( param->attrs, buf );

   return 0;

} /* End of RPGP_set_float_param() */

/*\/////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//     Convenience function for adding parameter attributes.
//
//  Inputs:
//     type - parameter attribute type (see rpgp.h)
//     tmp_buf - buffer to hold the added parameter.
//     val_str - value for the parameter.
//
//  Outputs:
//     tmp_buf - holds the added parameter attributes.
//
/////////////////////////////////////////////////////////////////////////////////\*/
static void Add_param_attr( int type, char *tmp_buf, char *val_str ){

   switch( type ){

      case RPGP_ATTR_NAME:
         sprintf( tmp_buf, "Name=%s;", val_str );
         break;

      case RPGP_ATTR_DESCRIPTION:
         sprintf( tmp_buf, "Description=%s;", val_str );
         break;

      case RPGP_ATTR_UNITS:
         sprintf( tmp_buf, "Units=%s;", val_str );
         break;

      case RPGP_ATTR_RANGE:
         sprintf( tmp_buf, "Range=%s;", val_str );
         break;
      
      case RPGP_ATTR_DEFAULT:
         sprintf( tmp_buf, "Default=%s;", val_str );
         break;

      case RPGP_ATTR_ACCURACY:
         sprintf( tmp_buf, "Accuracy=%s;", val_str );
         break;

      case RPGP_ATTR_CONVERSION:
         sprintf( tmp_buf, "Conversion=%s;", val_str );
         break;

      case RPGP_ATTR_EXCEPTION:
         sprintf( tmp_buf, "Exception=%s;", val_str );
         break;

      default:
         tmp_buf[0] = '\0';
         break;

   }

} /* End of Add_param_attr() */

