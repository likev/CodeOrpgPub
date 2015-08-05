#include <rpgcs_model_data.h>
#include <rpgcs_latlon.h>
#include <packet_29.h>

#define TYPE_UNKNOWN                 0
#define TYPE_INTEGER                 1
#define TYPE_UINTEGER                2
#define TYPE_SHORT                   3
#define TYPE_USHORT                  4
#define TYPE_BYTE                    5
#define TYPE_UBYTE                   6
#define TYPE_FLOAT                   7
#define TYPE_DOUBLE                  8

/* Static local function prototypes. */
static void* Decompress_product( char *bufptr, int *size );
static void* Deserialize_product( char *bufptr );
static int Malloc_dest_buf( int dest_len, char **dest_buf );
static time_t Decode_date( char *date_s );
static time_t Decode_time( char *time_s );
static int Check_model( int model, char *data );
static char* Find_attr( char *str, char *attr );
static int Find_type( char *str );
static int Set_units( char *field, char *units );

/* Public functions. */
/*\//////////////////////////////////////////////////////////////
   
   Description:
      Return a pointer to the model or model-derived data.  

   Inputs:
      prod_id - product id or data store id of the LB file to read
      model - model type string.  See rpgcs_model_data.h
              for supported models.

   Outputs:
      data - pointer to the model data.

   Returns:
      0 on success, negative error on failure.

   Notes:
      If model data successfully read, caller is responsible
      for freeing memory allocated to model data.   

//////////////////////////////////////////////////////////////\*/
int RPGCS_get_model_data( int prod_id, int model, char **data  ){

   int size;
   int msg_id = 0;
   char *prod = NULL;

   /* Set the message id for the product or data store to be read */
   switch( prod_id ){

      case ORPGDAT_ENVIRON_DATA_MSG:
         msg_id = ORPGDAT_RUC_DATA_MSG_ID;
         break;

      case MODEL_FRZ_GRID:
         msg_id = MODEL_FRZ_ID;
         break;

      case MODEL_CIP_GRID:
         msg_id = MODEL_CIP_ID;
         break;

   }

   size = RPGC_data_access_read( prod_id, data, LB_ALLOC_BUF, msg_id );

   if( size < 0 ){

      LE_send_msg( GL_ERROR,
                   "RPGC_data_access_read( %d ) Failed (%d)\n", prod_id, size );
      return size;

   }

   RPGC_log_msg( GL_INFO, "RPGCS_get_model_data info   prod_id: %d   msg_id: %d   size: %d\n", prod_id, msg_id, size);

   /* Decompress the product if the product is compressed. */
   prod = Decompress_product( *data, &size );

   /* If product was decompressed, free the read buffer. */
   if( prod != *data ){

      free( *data );
      *data = prod;

   }

   /* Deserialize the product data.  Currently we assume the model 
      data is in Generic Product Format and thererore the product is 
      serialized.  The following function will fail if the product
      is not serialized. */
   prod = Deserialize_product( *data );
   free( *data );

   /* Problem deserializing the data. */
   if( prod == NULL )
      return -1;

   /* Check the type of model data and return the type. */
   *data = prod;
   if( (model = Check_model( model, prod )) < 0 ){

      /* Requested model data not found. */
      RPGP_product_free( prod );
      *data = NULL;
      return -1;

   }
   
   /* Normal return. */
   return model;

/* End of RPGCS_get_model_data() */
}

/*\//////////////////////////////////////////////////////////////////

   Description:
      Extract model attributes for "model".  

   Inputs:
      model - model type.
      data - pointer to model data.

   Returns:
      Pointer to attributes on success, NULL on error.  The 
      attributes data structure depends on the model. 

   Notes:
      For RUC 80, 40 km and 13 km, the returned pointer is a pointer 
      to a RPGCS_model_attr_t data structure.  

      The caller is resonsible for freeing memory associated
      with the attribute data structure.
      
//////////////////////////////////////////////////////////////////\*/
void* RPGCS_get_model_attrs( int model, char *data ){

   int i;
   RPGP_ext_data_t *ext = (RPGP_ext_data_t *) data;

   static RPGCS_model_attr_t *attr = NULL; 

   /* Validate the arguments. */
   if( (data == NULL) 
             ||
       ((model = Check_model( model, data )) < 0) ){

      LE_send_msg( GL_INFO, "Invalid Argument(s) to RPGCS_get_model_attrs()\n" );
      return NULL;

   }

   /* Allocate buffer to hold model attributes. */
   attr = (RPGCS_model_attr_t *) calloc( 1, sizeof(RPGCS_model_attr_t) );
   if( attr == NULL ){

      LE_send_msg( GL_INFO, "RPGCS_get_model_attrs: calloc Failed\n" );
      return NULL;

   }

   /* Initialize the model attribute. */
   attr->model = model;

   /* Initialize the latitude/longitude values to missing. */
   attr->grid_lower_left.latitude = MISSING_LATLON;
   attr->grid_lower_left.longitude = MISSING_LATLON;
   attr->grid_upper_right.latitude = MISSING_LATLON;
   attr->grid_upper_right.latitude = MISSING_LATLON; 
   
   /* Populate the attributes data structure.  If any attribute retrieval
      fails, return NULL (failure). */
   for( i = 0; i < ext->numof_prod_params; i++ ){

      /* Model Run Date. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_MODEL_RUN_DATE ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->model_run_time += Decode_date( value );
         continue;

      }

      /* Model Run Time. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_MODEL_RUN_TIME ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->model_run_time += Decode_time( value );
         continue;

      }

      /* Date for which model is valid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_VALID_DATE ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;
 
         }

         attr->valid_time += Decode_date( value );
         continue;

      }

      /* Time at which model is valid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_VALID_TIME ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->valid_time += Decode_time( value );
         continue;

      }

      /* Forecast hour for the model. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_FORECAST_HOUR ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->forecast_period = Decode_time( value );
         continue;

      }

      /* Model map projection.  Currently we only support Lambert Conformal.
         As other projections are needed, support will have to be added. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_PROJECTION ) != NULL ){

         char *value = strstr( ext->prod_params[i].attrs, "Lambert Conformal" );

         if( value != NULL )
            attr->projection = RPGCS_LAMBERT_CONFORMAL;

         else
            attr->projection = RPGCS_UNKNOWN_PROJECTION;
         continue;

      }

      /* Latitude of Lower Left Corner of grid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_LATITUDE_LLC ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }
         attr->grid_lower_left.latitude = strtod( value, (char **) NULL );
         continue;

      }

      /* Longitude of Lower Left Corner of grid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_LONGITUDE_LLC ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->grid_lower_left.longitude = strtod( value, (char **) NULL );
         continue;

      }

      /* Latitude of Upper Right Corner of grid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_LATITUDE_URC ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->grid_upper_right.latitude = strtod( value, (char **) NULL );
         continue;

      }

      /* Longitude of Upper Right Corner of grid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_LONGITUDE_URC ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->grid_upper_right.longitude = strtod( value, (char **) NULL );
         continue;

      }

      /* Latitude of Tangent Point of grid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_LATITUDE_TANP ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->tangent_point.latitude = strtod( value, (char **) NULL );

         /* The latitude of the tangent point should always be positive (north of
            equator). */
         if( attr->tangent_point.latitude < 0.0 ){

            LE_send_msg( GL_INFO, "Lambert Projection Tangent Point Latitude < 0.0\n" );
            attr->tangent_point.latitude = fabs( attr->tangent_point.latitude );

         }
         continue;

      }

      /* Longitude of Tangent Point of grid. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_LONGITUDE_TANP ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->tangent_point.longitude = strtod( value, (char **) NULL );
         continue;

      }

      /* Number of points in X Dimension. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_X_DIMENSION ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->dimensions[0] = strtol( value, (char **) NULL, 10 );
         if( attr->dimensions[0] != 0 )
            attr->num_dimensions++;
         continue;

      }

      /* Number of points in Y Dimension. */
      if( strstr( ext->prod_params[i].attrs, RPGCS_Y_DIMENSION ) != NULL ){

         char *value = Find_attr( ext->prod_params[i].attrs, "value" );
         if( value == NULL ){

            free( attr );
            return NULL;

         }

         attr->dimensions[1] = strtol( value, (char **) NULL, 10 );
         if( attr->dimensions[1] != 0 )
            attr->num_dimensions++;
         continue;

      }

   }

   /* So for, so good.  Return pointer to attrbutes. */
   return attr;

/* End of RPGCS_get_model_attrs() */
}

/*\//////////////////////////////////////////////////////////////////

   Description:
      Retrieves "model" "field" given model data "data".

   Inputs:
      model - model string associated with data
      data - pointer to model data
      field - field of interest string.

   Returns:
      Pointer to structure holding the model field data.

   Notes:
      See rpgcs_model_data.h for valid "model" and "field".

      The user is responsible for freeing the field data.
      
//////////////////////////////////////////////////////////////////\*/
void* RPGCS_get_model_field( int model, char *data, char *field ){

   RPGP_ext_data_t *ext = (RPGP_ext_data_t *) data;
   RPGCS_model_grid_data_t *grid_data = NULL;
   int levels, grid_size = 0, type, i, k;
   
   /* Verify the model. */
   if( (ext->type != RPGP_EXTERNAL)
                  ||
       ((model != RUC40) && (model != RUC13) 
                         && 
        (model != RUC80) && (model != RUC_ANY_TYPE)) )

      return NULL;

   /* Look for the field of interest. */
   levels = 0;
   for( i = 0; i < ext->numof_components; i++ ){

      RPGP_grid_t *grid = (RPGP_grid_t *) ext->components[i];
      RPGP_data_t *data = (RPGP_data_t *) &grid->data; 
      int j;

      /* Currently we only look at grid components. */
      if( grid->comp_type != RPGP_GRID_COMP )
         continue;

      /* Find the particular field in the component parameters. */
      if( strstr( data->attrs, field ) != NULL ){

         levels++;
         if( levels == 1 ){

            char *units = NULL;

            grid_data = (RPGCS_model_grid_data_t *) 
                               calloc( 1, sizeof( RPGCS_model_grid_data_t) );
            if( grid_data == NULL ){

               LE_send_msg( GL_INFO, "calloc Failed\n" );
               return NULL;

            }

            /* Set data grid parameters. */
            grid_data->field = malloc( strlen(field) + 1 );
            if( grid_data->field != NULL )
               strcpy( grid_data->field, field );

            grid_data->num_dimensions = grid->n_dimensions; 
            grid_data->dimensions = 
                      (int *) calloc( grid_data->num_dimensions, sizeof(int) );
            grid_size = 1;
            for( j = 0; j < grid_data->num_dimensions; j++ ){

               grid_data->dimensions[j] = grid->dimensions[j];
               grid_size *= grid_data->dimensions[j];

            }

            /* Find the units of measure for this field. */
            units = Find_attr( data->attrs, "units" );
            grid_data->units = Set_units( field, units );

         }
           
         /* Copy the data. */
         grid_data->data[levels-1] = (double *) malloc( grid_size*sizeof(double) );
         if( grid_data->data[levels-1] == NULL ){

            LE_send_msg( GL_INFO, "malloc Failed for %d Bytes\n", 
                         grid_size*sizeof(double) );
            return NULL;

         }

         type = Find_type( data->attrs );
         switch( type ){

            case TYPE_INTEGER:
            {

               int *int_p = (int *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) int_p[k];
               
               break;

            }

            case TYPE_UINTEGER:
            {

               unsigned int *uint_p = (unsigned int *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) uint_p[k];
               
               break;

            }

            case TYPE_SHORT:
            {

               short *short_p = (short *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) short_p[k];
               
               break;

            }
            case TYPE_USHORT:
            {

               unsigned short *ushort_p = (unsigned short *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) ushort_p[k];
               
               break;

            }

            case TYPE_BYTE:
            {

               char *char_p = (char *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) char_p[k];
               
               break;

            }
            case TYPE_UBYTE:
            {

               unsigned char *uchar_p = (unsigned char *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) uchar_p[k];
               
               break;

            }

            case TYPE_FLOAT:
            {

               float *float_p = (float *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) float_p[k];
               
               break;

            }

            case TYPE_DOUBLE:
            {

               double *double_p = (double *) grid->data.data;

               for( k = 0; k < grid_size; k++ )
                  grid_data->data[levels-1][k] = (double) double_p[k];
               
               break;

            }

            default:
            case TYPE_UNKNOWN:
            {

               LE_send_msg( GL_INFO, "Unknown Data Type ....\n" );
               return NULL;

            }

         }

         /* Extract the grid component parameters. */
         grid_data->params[levels-1] = 
                    (RPGCS_model_grid_params_t *) malloc( sizeof(RPGCS_model_grid_params_t) );
         if( grid_data->params[levels-1] == NULL ){

            LE_send_msg( GL_INFO, "malloc Failed\n" );
            return NULL;

         }

         for( j = 0; j < grid->numof_comp_params; j++ ){

            /* Get the "level type" .... */
            if( strstr( grid->comp_params[j].id, RPGCS_MODEL_LEVEL ) != NULL ){

               /* Get the level type. */
               char *value = Find_attr( grid->comp_params[j].attrs, "name" );

               if( strstr( value, RPGCS_PRESSURE_LEVEL ) != NULL )
                  grid_data->params[levels-1]->level_type = RPGCS_CONST_PRESSURE_LEVEL;

               else
                  grid_data->params[levels-1]->level_type = RPGCS_UNDEFINED_LEVEL_TYPE;

               /* Get the level value. */
               value = Find_attr( grid->comp_params[j].attrs, "value" );
               grid_data->params[levels-1]->level_value = strtod( value, (char **) NULL );

               /* Get the level units. */
               value = Find_attr( grid->comp_params[j].attrs, "units" );
               if( strstr( value, RPGCS_PRESSURE_LEVEL_UNITS ) != NULL )
                  grid_data->params[levels-1]->level_units = RPGCS_MILLIBAR_UNITS;

               else
                  grid_data->params[levels-1]->level_units = RPGCS_UNKNOWN_UNITS;

               continue;

            }

         }

      }

   }

   if( grid_data != NULL ){

      grid_data->num_levels = levels;
      return grid_data;

   }

   /* If here, field was not found. */
   return NULL;

/* End of RPGCS_get_model_field() */
}

/*\//////////////////////////////////////////////////////////////////

   Description:
      Retrieves data value.

   Inputs:
      grid - Pointer to model field data RPGCS_model_grid_data_t
             structure.
      level - Data level of interest.  This is in effect, the k index.
      i_ind - I index (assumed 0-indexed)
      j_ind - J index (assumed 0-indexed)

   Outputs:
     units - units of measure for the data of interest.

   Returns:
      The data value at i_ind, j_ind on success, or MODEL_BAD_VALUE
      on error.

//////////////////////////////////////////////////////////////////\*/
double RPGCS_get_data_value( RPGCS_model_grid_data_t *grid, int level, 
                             int i_ind, int j_ind, int *units ){

   double *value;

   /* Make sure index values are within array boundaries. */
   if( (i_ind >= grid->dimensions[0]) 
                  ||
       (j_ind >= grid->dimensions[1])
                  ||
       (level >= grid->num_levels) )
      return MODEL_BAD_VALUE;

   value = grid->data[level] + ((j_ind*grid->dimensions[0]) + i_ind);

   /* Set the units. */
   *units = grid->units;

   return( *value );

/* End of RPGCS_get_data_value() */
}

/*\//////////////////////////////////////////////////////////////////

   Description:
      Convenience function for freeing model field data.

   Inputs:
      model - model associated with "data".
      data - pointer to the model data.

   Returns:
      There is no return value for this function.

//////////////////////////////////////////////////////////////////\*/
void RPGCS_free_model_field( int model, char *data ){

   int i;

   /* Model data associated with RUC model. */
   if( (model == RUC40) || (model == RUC13) || (model == RUC80) ){

      RPGCS_model_grid_data_t *grid_data = (RPGCS_model_grid_data_t *) data;

      if( grid_data == NULL )
         return;
   
      if( grid_data->field != NULL )
         free( grid_data->field );

      if( grid_data->dimensions != NULL )
         free( grid_data->dimensions );

      for( i = 0; i < grid_data->num_levels; i++ ){

         if( grid_data->params[i] != NULL )
            free( grid_data->params[i] );

         if( grid_data->data[i] != NULL )
            free( grid_data->data[i] );

      }

      free( grid_data );

   }

   return;

/* End of RPGCS_free_model_field() */
}

/* Private functions follow. */

/*\//////////////////////////////////////////////////////////////////

   Description:
      The buffer pointer to by "bufptr" is decompressed returned.
      The size of the decompressed product is stored in "size".

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data; includes External_data_msg_hdr and may 
               include Message Header.

   Outputs:
      size - size of the decompressed product.
   
   Returns: 
      Pointer to decompressed product or NULL on error.  The 
      decompressed product does not include a Message Header even
      if "bufptr" did.

/////////////////////////////////////////////////////////////////\*/
static void* Decompress_product( char *bufptr, int *size ){

   int hdr_size, ret;
   short divider;
   unsigned int dest_len, src_len;
   unsigned short alg;
   unsigned short msw, lsw;

   char *prod_data = NULL;
   char *dest = NULL;
   External_data_msg_hdr *ext_hdr = 
         (External_data_msg_hdr *) (bufptr + sizeof(Prod_msg_header_icd));

   /* Check to see if this message has a ICD Message Header.  We handle it 
      either way. */
   if( (divider = (short) SHORT_BSWAP_L( ext_hdr->divider )) == -1 ){

      hdr_size = sizeof(Prod_msg_header_icd);
      prod_data = (char *) (((char *) bufptr) + sizeof(Prod_msg_header_icd));

   }
   else{

      hdr_size = 0;
      prod_data = bufptr;
      ext_hdr = (External_data_msg_hdr *) prod_data;

   }

   /* Get the algorithm used for compression so we know how to decompress. */
   alg = SHORT_BSWAP_L( ext_hdr->comp_type );
   if( (alg != COMPRESSION_BZIP2) && (alg != COMPRESSION_ZLIB) )
      return( (void *) bufptr );
 
   /* Get the decompressed size of the data packets.  This size is stored in 
      the External Data Message Header. */
   msw = SHORT_BSWAP_L( ext_hdr->decomp_sz_msw );
   lsw = SHORT_BSWAP_L( ext_hdr->decomp_sz_lsw );
   src_len = dest_len = (unsigned int ) (0xffff0000 & ((msw) << 16)) | ((lsw) & 0xffff);

   /* Allocate the destination buffer.  The total size of the buffer to 
      allocate must include sufficient size to hold the decompressed data 
      packets plus the External Data Message Header. */
   if( (dest_len == 0) 
              || 
       ((ret = Malloc_dest_buf( dest_len + sizeof(External_data_msg_hdr), &dest )) < 0) )
      return( NULL );

   /* Do the decompression based on the algorithm used. */
   switch( alg ){

      case COMPRESSION_BZIP2:
      {

         /* Do the bzip2 decompression. */
         dest_len = MISC_decompress( MISC_BZIP2, prod_data + sizeof(External_data_msg_hdr), src_len,
                                     dest + sizeof(External_data_msg_hdr), dest_len );

         /* Process Non-Normal return. */
         if( dest_len < 0 ){

            /* Free the destination buffer. */
            free(dest);
            *size = 0;

            return( NULL );

         }
         else
            break;

      }

      case COMPRESSION_ZLIB:
      {

         /* Do the zlib decompression. */
         dest_len = MISC_decompress( MISC_GZIP, prod_data + sizeof(External_data_msg_hdr), src_len,
                                     dest + sizeof(External_data_msg_hdr), dest_len );

         /* Process Non-Normal return. */
         if( dest_len < 0 ){

            /* Free the destination buffer. */
            free(dest);
            *size = 0;

            return( NULL );

         }
         else
            break;

      }

      default:
      {
         LE_send_msg( GL_ERROR, "Decompression Method Not Supported (%x)\n", alg );
      }
      case COMPRESSION_NONE:
      {

         External_data_msg_hdr *ext_hdr = (External_data_msg_hdr *) dest;

         /* Just copy the source to the destination. */
         memcpy( (void *) dest, prod_data, dest_len + sizeof(External_data_msg_hdr) );
         *size = dest_len;

         /* Store the compression type in the external data message header. */
         ext_hdr->comp_type = SHORT_BSWAP_L( (unsigned short) COMPRESSION_NONE );

         return( dest );

      }

   /* End of "switch" statement. */
   }

   /* Copy the header to the destination buffer and set the decompressed
      product size. */
   memcpy( (void*) dest, (void*) (bufptr + hdr_size), sizeof(External_data_msg_hdr) );
   *size = dest_len  + sizeof(External_data_msg_hdr);

   /* Store the new compression type (none) in the External Data Message
      header. */ 
   ext_hdr = (External_data_msg_hdr *) dest;
   ext_hdr->comp_type =  SHORT_BSWAP_L( (unsigned short) COMPRESSION_NONE );

   return( dest );

/* Decompress_product() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Allocates a destination buffer the same size of the original,
      uncompressed product size.

   Inputs:
      dest_len - the size of the destination buffer, in bytes.
      dest_buf - pointer to pointer to destination buffer.

   Outputs:
      dest_buf - malloc'd destination buffer.

   Returns:
      -1 on error, or 0 on success.

/////////////////////////////////////////////////////////////////////\*/
static int Malloc_dest_buf( int dest_len, char **dest_buf ){

   /* Allocate an output buffer the same size as the original 
      (i.e., uncompressed) data. */
   *dest_buf = malloc( dest_len );
   if( *dest_buf == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", dest_len );
      return( -1 );

   }

   return( 0 );

/* End of Malloc_dest_buf() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:  
      This function deserializes the serialized product.

   Input: 
      data - pointer to serialized product.

   Return:  
      NULL on error, or pointer to deserialized product.

////////////////////////////////////////////////////////////////////\*/
static void* Deserialize_product( char *data ){

   int ret, size;
   unsigned short size_msw, size_lsw;
   char *serialized_data, *deserialized_data;
   packet_29_t *packet29 = (packet_29_t *) (data + sizeof(External_data_msg_hdr));

   /* Check to make sure the data is either packet 28 or 29 */
   if ( ( SHORT_BSWAP_L( packet29->code ) != 28 ) 
                         && 
        ( SHORT_BSWAP_L( packet29->code ) != 29 ) ){

      LE_send_msg( GL_INFO, "Packet code not 28 or 29 (%d)!\n",
                   SHORT_BSWAP_L(packet29->code));
      return NULL;

   }

   /* Increment pointer to length of serialized data and store prod size */
   size_msw = SHORT_BSWAP_L( packet29->num_bytes_msw );
   size_lsw = SHORT_BSWAP_L( packet29->num_bytes_lsw );
   size = (size_msw << 16) | size_lsw;

   /* Set the serial data pointer and deserialize the data. */
   serialized_data = (char *) ((char *) packet29 + sizeof(packet_29_t));
   ret = RPGP_product_deserialize( serialized_data, size,
                                   (void **) &deserialized_data );

   if( ret < 0 )
      return NULL;

   return deserialized_data;

} /* End of Deserialize_product() */

/*\/////////////////////////////////////////////////////////////////////

   Description:  
      This function decodes a date string in YYYYMMDD format and returns
      the corresponding UNIX time.

   Input: 
      date_s - pointer to YYYYMMDD string.

   Return:  
      UNIX time on success, or 0 on error.

/////////////////////////////////////////////////////////////////////\*/
static time_t Decode_date( char *date_s ){
 

   if( date_s != NULL ){

      char cpt[5];
      time_t utime = 0;
      int year, month, day, hour = 0, min = 0, sec = 0;

      /* Extract year. */
      memset( cpt, 0, 5 );
      memcpy( cpt, date_s, 4 );
      year = strtol( cpt, (char **) NULL, 10 );
      date_s += 4;

      /* Extract month. */
      memset( cpt, 0, 5 );
      memcpy( cpt,  date_s, 2 );
      month = strtol( cpt, (char **) NULL, 10 );
      date_s += 2;

      /* Extract day. */
      memset( cpt, 0, 5 );
      memcpy( cpt, date_s, 2 );
      day = strtol( cpt, (char **) NULL, 10 );

      /* Make sure the year, month and day are valid. */
      if( (year >= 1970)
                  &&
          ((month > 0) && (month <= 12))
                  &&
          ((day > 0) && (day <= 31)) ){

         /* Get unix time for year, month and day. */
         unix_time( &utime, &year, &month, &day, &hour, &min, &sec );

         return utime;

      }

   }

   return 0;

/* End of Decode_date. */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      This function decodes a time string in HH:MM:SS format and returns
      the corresponding UNIX time.

   Input:
      date - pointer to HH:NN:SS string.

   Return:
      UNIX time on success, or 0 on error.

/////////////////////////////////////////////////////////////////////\*/
static time_t Decode_time( char *time_s ){

   if( time_s != NULL ){

      char *cpt = NULL;
      time_t utime;
      int hour = 0, min = 0, sec = 0;
      int i, len = strlen( time_s );

      /* Extract hour. */
      cpt = time_s;
      hour = strtol( cpt, (char **) NULL, 10 );

      len = strlen( cpt );
      i = 0;
      while( (*(cpt+i) != ':') && (i < len) )
         i++;

      /* Extract minute. */
      cpt += i;
      min = strtol( cpt, (char **) NULL, 10 );

      len = strlen( cpt );
      i = 0;
      while( (*(cpt+i) != ':') && (i < len) )
         i++;

      sec = strtol( cpt, (char **) NULL, 10 );

      /* Make sure the hour, minute and second are valid. */
      if( ((hour >= 0) && (hour < 24))
                     &&
          ((min >= 0) && (min < 60))
                     &&
          ((sec >= 0) && (sec < 60)) ){

         /* Get unix time for hour, minute and seconds. */
         utime = hour*3600 + min*60 + sec;

         return utime;

      }

   }

   return 0;

/* End of Decode_time() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      This function decodes a time string in HH:MM:SS format and returns
      the corresponding UNIX time.

   Input:
      date - pointer to HH:NN:SS string.

   Return:
      UNIX time on success, or 0 on error.

/////////////////////////////////////////////////////////////////////\*/
static int Check_model( int model, char *data ){

   int match, i;
   RPGP_ext_data_t *ext = (RPGP_ext_data_t *) data;

   /* Validate the arguments. */
   if( (data == NULL)
             ||
       ((model != RUC40) && (model != RUC13) 
                         && 
        (model != RUC80) && (model != RUC_ANY_TYPE)) ){

      LE_send_msg( GL_INFO, "Invalid Argument(s) ....\n" );
      return -1;

   }

   /* If number of product parameters is 0, return error.  We
      expect the model name to be part of the parameters. */
   if( ext->numof_prod_params == 0 ){

      LE_send_msg( GL_INFO, "# Prod Params == 0\n" );
      return -1;

   }

   /* Search for the "Model Name" parameter.  Make sure the the name matches
      what is expected. */
   match = 0;
   for( i = 0; i < ext->numof_prod_params; i++ ){

      char *param = ext->prod_params[i].attrs;

      if( strstr( param, "Model Name" ) != NULL ){

         /* User wants any kind of RUC model data. */
         if( model == RUC_ANY_TYPE ){

            if( strstr( param, STR_RUC80 ) != NULL ){

               model = RUC80;
               match = 1;

            }
            else if( strstr( param, STR_RUC40 ) != NULL ){

               model = RUC40;
               match = 1;

            }
            else if( strstr( param, STR_RUC13 ) != NULL ){

               model = RUC13;
               match = 1;

            }

            if( match )
               break;

         }

         /* User wants 80 km RUC model data. */
         if( (model == RUC80) && (strstr( param, STR_RUC80 ) != NULL) ){

            match = 1;
            break;

         }

         /* User wants 40 km RUC model data. */
         if( (model == RUC40) && (strstr( param, STR_RUC40 ) != NULL) ){

            match = 1;
            break;

         }

         /* User wants 13 km RUC model data. */
         if( (model == RUC13) && (strstr( param, STR_RUC13 ) != NULL) ){

            match = 1;
            break;

         }

      }

   }

   /* The model data is not what was requested. */
   if( !match ){

      LE_send_msg( GL_INFO, "Requested Model Not Found\n" );
      return -1;

   }

   return model;

/* End of Check_model() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Given a string of tokens "str", this functions tests each token
      for a match on "attr".  On match, the address of the value portion
      of the token is returned.   

   Inputs:
      str - pointer to string containing tokens
      attr - pointer to token to match

   Returns:
      Address of matching token value or NULL if no match found.

/////////////////////////////////////////////////////////////////////\*/
static char* Find_attr( char *str, char *attr ){

   char *s = NULL, *token = NULL, *equal = NULL;
   static char *cpt = NULL;
   static int size = 0;

   /* The static buffer holds the string to be parsed for tokens. */
   if( (cpt == NULL) || (strlen(cpt) < strlen(str)) ){

      size = strlen(str)+1;
      cpt = (char *) realloc( cpt, size );
      if( cpt == NULL ){

         LE_send_msg( GL_INFO, "realloc Failed for %d\n", strlen(str)+1 );
         size = 0;

         return NULL;

      }

   }

   /* Initialize the static buffer. */
   memset( cpt, 0, size );

   /* Copy the input string to the static buffer. */
   strcpy( cpt, str );
   s = cpt;

   /* Find desired token. */
   while( (token = strtok( s, ";" )) != NULL ){

      equal = strstr( token, "=" );
   
      if( equal == NULL )
         break;

      *equal = '\0';
      equal++;
      if( strstr( token, attr ) != NULL )
         break;

      /* Need to set "s" to NULL so that strtok knows we are operating
         on the previous string. */
      s = NULL;

   }

   if( equal == NULL )
      LE_send_msg( GL_INFO, "Find_attr Failed in Finding Attribute %s\n",
                   attr );
   return equal;

/* End of Find_attr() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Find the data type ... return macro corresponding to type.

/////////////////////////////////////////////////////////////////////\*/
static int Find_type( char *str ){

   char *type = Find_attr( str, "type" );

   if( type != NULL ){

      if( strstr( type, "byte" ) != NULL )
         return TYPE_BYTE;

      if( strstr( type, "ubyte" ) != NULL )
         return TYPE_UBYTE;

      if( strstr( type, "short" ) != NULL )
         return TYPE_SHORT;

      if( strstr( type, "ushort" ) != NULL )
         return TYPE_USHORT;

      if( strstr( type, "int" ) != NULL )
         return TYPE_INTEGER;

      if( strstr( type, "uint" ) != NULL )
         return TYPE_UINTEGER;

      if( strstr( type, "float" ) != NULL )
         return TYPE_FLOAT;

      if( strstr( type, "double" ) != NULL )
         return TYPE_DOUBLE;

   }

   return TYPE_UNKNOWN;

/* End of Find_type() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Set the unit type based on the field and units string. 

   Inputs:
      field - string containing the data field of interest.
      units - string containing the units string.

   Returns:
     Units type.

/////////////////////////////////////////////////////////////////////\*/
static int Set_units( char *field, char *units ){

   /* Is this a Temperature field? */
   if( strcmp( RPGCS_MODEL_TEMP, field ) == 0 ){

      if( strstr( units, RPGCS_KELVIN_STR ) != NULL )
         return RPGCS_KELVIN_UNITS;

      if( strstr( units, RPGCS_CELSIUS_STR ) != NULL )
         return RPGCS_CELSIUS_UNITS;

      if( strstr( units, RPGCS_FAHRENHEIT_STR ) != NULL )
         return RPGCS_FAHRENHEIT_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Geopotential Height field? */
   if( strcmp( RPGCS_MODEL_GH, field ) == 0 ){

      if( strstr( units, RPGCS_KILOMETER_STR ) != NULL )
         return RPGCS_KILOMETER_UNITS;

      if( strstr( units, RPGCS_METER_STR ) != NULL )
         return RPGCS_METER_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a U or V Component field? */
   if( (strcmp( RPGCS_MODEL_UCOMP, field ) == 0)
                         ||
       (strcmp( RPGCS_MODEL_VCOMP, field ) == 0) ){

      if( strstr( units, RPGCS_MPS_STR ) != NULL )
         return RPGCS_MPS_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Relative Humidity field? */
   if( strcmp( RPGCS_MODEL_RH, field ) == 0 ){

      if( strstr( units, RPGCS_PERCENT_STR ) != NULL )
         return RPGCS_PERCENT_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this Surface Pressure field? */
   if( strcmp( RPGCS_MODEL_SFCP, field ) == 0 ){

      if( strstr( units, RPGCS_MILLIBAR_STR ) != NULL )
         return RPGCS_MILLIBAR_UNITS;

      if( strstr( units, RPGCS_PASCAL_STR ) != NULL )
         return RPGCS_PASCAL_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Range field? */
   if( strcmp( FRZ_GRID_RANGE, field ) == 0 ){

      if( strstr( units, RPGCS_KILOMETER_STR ) != NULL )
         return RPGCS_KILOMETER_UNITS;

      if( strstr( units, RPGCS_METER_STR ) != NULL )
         return RPGCS_METER_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Azimuth field? */
   if( strcmp( FRZ_GRID_AZIMUTH, field ) == 0 ){

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Number Zero Degree Crossings field? */
   if( strcmp( FRZ_GRID_NUM_ZERO_X, field ) == 0 ){

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Height Zero field? */
   if( strcmp( FRZ_GRID_HEIGHT_ZERO, field ) == 0 ){

      if( strstr( units, RPGCS_KILOMETER_STR ) != NULL )
         return RPGCS_KILOMETER_UNITS;

      if( strstr( units, RPGCS_METER_STR ) != NULL )
         return RPGCS_METER_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Height ZDR Temp1 or ZDR Temp2 field? */
   if( strcmp( FRZ_GRID_HEIGHT_ZDR_T1, field ) == 0 || strcmp( FRZ_GRID_HEIGHT_ZDR_T2, field ) == 0 ){

      if( strstr( units, RPGCS_KILOMETER_STR ) != NULL )
         return RPGCS_KILOMETER_UNITS;

      if( strstr( units, RPGCS_METER_STR ) != NULL )
         return RPGCS_METER_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Height Minus 20 field? */
   if( strcmp( FRZ_GRID_HEIGHT_MINUS20, field ) == 0 ){

      if( strstr( units, RPGCS_KILOMETER_STR ) != NULL )
         return RPGCS_KILOMETER_UNITS;

      if( strstr( units, RPGCS_METER_STR ) != NULL )
         return RPGCS_METER_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Max Temp Warm Layer or Min Temp Cold Layer field? */
   if( strcmp( FRZ_GRID_MAX_TEMP_WL, field ) == 0 || strcmp( FRZ_GRID_MIN_TEMP_CL, field ) == 0 ){

      if( strstr( units, RPGCS_KELVIN_STR ) != NULL )
         return RPGCS_KELVIN_UNITS;

      if( strstr( units, RPGCS_CELSIUS_STR ) != NULL )
         return RPGCS_CELSIUS_UNITS;

      if( strstr( units, RPGCS_FAHRENHEIT_STR ) != NULL )
         return RPGCS_FAHRENHEIT_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a Freezing Grid Spare field? */
   if( strcmp( FRZ_GRID_SPARE, field ) == 0 ){

      return RPGCS_UNKNOWN_UNITS;

   }

   /* Is this a CIP Product Interest field? */
   if( strcmp( CIP_GRID_PROD, field ) == 0 ){

      if( strstr( units, RPGCS_PERCENT_STR ) != NULL )
         return RPGCS_PERCENT_UNITS;

      return RPGCS_UNKNOWN_UNITS;

   }

   return RPGCS_UNKNOWN_UNITS;

/* End of Set_units() */
}
