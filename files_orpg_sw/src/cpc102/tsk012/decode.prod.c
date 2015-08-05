/************************************************************************
 *	Module: decode.prod.c						*
*									*
 *	Description: This module contains a collection of functions to	*
 *		     decode an RPG product.  It is almost identical to	*
 *		     the module "hci_decode_product.c" in CPC001/LIB003.*
 ************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/07/12 19:28:48 $
 * $Id: decode.prod.c,v 1.60 2005/07/12 19:28:48 steves Exp $
 * $Revision: 1.60 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <math.h>
#include <rle.h>
#include <rpgc.h>
#include <str.h>
#include <a309.h>
#include <orpg.h>
#include <orpgpat.h>

#define ORPG_PROD_HEAD_SIZE (sizeof(Prod_header))

extern float radar_latitude;	/* Radar latitude (degrees) */
extern float radar_longitude;	/* Radar longitude (degrees) */

extern int debug;	/* debug flag (debug purposes only) */
extern short product_code; /* product code of currently openned file */

int	*Int;	/* unused */
static	unsigned short product_type;

/************************************************************************
 *	Description: This function is used to decode the product	*
 *		     contained in the input buffer.			*
 *									*
 *	Input:  product_data - pointer to buffer containing product	*
 *		     data.						*
 *	Output: NONE							*
 *	Return: product type on success; -1 on failure			*
 ************************************************************************/

int decode_product( short *product_data )
{
   int bytes_read, status, product_offset, full, i, j;

   int block1_size, block1_num_layers, layer1_size;

   static struct product_pertinent att;

   /* Decode product header, and return the offset to the
      symbology block. */

   product_offset = decode_header( product_data );

   if (debug)
	printf("product offset = %d", product_offset);

   /* Validate product code. */
   if( product_code < 16 || product_code > 2999 ){

      return product_code;

   }
   
   /* Calculate the size of the symbology block */
   full = (product_data[ product_offset + block1_size_offset ] << 16) |
          (product_data[ product_offset + block1_size_offset + 1 ] & 0xffff);
   block1_size = full/2;

   /* Calculate the number of layers in the symbology block */
   block1_num_layers = product_data[ product_offset + num_layers_offset ];

   if( block1_num_layers == 1 ){

      full = (product_data[ product_offset + layer1_size_offset ] << 16) |
             (product_data[ product_offset + layer1_size_offset + 1] & 0xffff);
      layer1_size = full/2; 

   }
   else if( block1_num_layers > 1 && 
            product_code != dhr_type &&
            product_code != dbv_type &&
            product_code != dr_type  &&
            product_code != dv_type  &&
            product_code != dvl_type &&
            product_code != ss_type  &&
	    product_code != ftm_type &&
	    product_code != pup_type &&
	    ORPGPAT_get_format_type (ORPGPAT_get_prod_id_from_code (product_code)) != FORMAT_TYPE_HIRES_RADIAL   &&
            product_code != spd_type ){

      fprintf( stderr, "More than 1 layer in the symbology block\n" );
      fprintf( stderr, "Tool does not support multiple layers.\n" );
      return(0);

   }
   else if( block1_num_layers == 0){

      layer1_size = 0;

   }
   if (debug)
	printf("block size = %d\n", block1_size);
   /* Extract product type, and decode accordingly */
   product_offset += image_type_offset;
   if (debug)
	printf("prod type offset = %d\n", product_offset);
   product_type = product_data[ product_offset ];
   if (debug)
	fprintf (stderr,"PRODUCT_TYPE [%d]\n", product_type);

   if( product_type == 0xAF1F ) { 

      /* Product is a radial run-length-encoded type */
      status = decode_radial_rle( &product_data[ product_offset ] ); 
      
   }

   else if( product_type == 0xBA0F || product_type == 0xBa07 ){

      /* Product is a raster run-length-encoded type */
      product_type = 0xBA0F;

      status = decode_raster_rle( &product_data[ product_offset ] );

   }
   else if (product_type == 27)
   {
      /* Product is a raster run-length-encoded type */
      product_type = generic_hires_raster_type;
      status = decode_hires_raster( &product_data[ product_offset ] );
   }
   else if( product_code == cld_type ){

      /* Product is a radial run-length-encoded type */
      status = decode_radial_rle( &product_data[ product_offset ] );

   }

   else if( product_code == clr_type ){

      /* Product is a radial run-length-encoded type */
      status = decode_radial_rle( &product_data[ product_offset ] );

    }

   else if( product_code == dbv_type ){

      /* If this is the DBV product, decode it */
      decode_dhr( &product_data[ product_offset ] );
      product_type = dbv_type;

   }

   else if( product_code == dhr_type ){

      /* If this is the DHR product, decode it */
      decode_dhr( &product_data[ product_offset ] );
      product_type = dhr_type;

   }

   else if( product_code == dv_type ){

      /* If this is the DV product, decode it */
      decode_dhr( &product_data[ product_offset ] );
      product_type = dv_type;

   } 

   else if( product_code == dvl_type ){

      /* If this is the DVL product, decode it */
      decode_dhr( &product_data[ product_offset ] );
      product_type = dvl_type;

   }

   else if( product_code == dr_type ){

      /* If this is the DR product, decode it */
      decode_dhr( &product_data[ product_offset ] );
      product_type = dr_type;

   } 

   else if( product_code == vwp_type ){

      /* If this is the VWP product, decode it */
      decode_vwp( product_data, 
                  product_offset, 
                  layer1_size );
      product_type = vwp_type;

   } 

   else if( product_code == sti_type ){

      /* If this is the STI product, decode it */
      decode_sti( product_data, 
                  product_offset,
                  layer1_size );
      product_type = sti_type;

   }

   else if( product_code == hi_type ){

      /* If this is the HI product, decode it */
      decode_hi( product_data,
                 product_offset, 
                 layer1_size );
      product_type = hi_type;

   } 

   else if( product_code == meso_type ){

      /* If this is the MESO product, decode it */
      decode_meso( product_data, 
                   product_offset,
                   layer1_size );
      product_type = meso_type;

   } 

   else if( product_code == tvs_type ){

      /* If this is the TVS product, decode it */
      decode_tvs( product_data,
                  product_offset,
                  layer1_size );
      product_type = tvs_type;

   } 

   else if( product_code == swp_type ){

      /* If this is the SWP product, decode it */
      decode_swp( product_data, 
                  product_offset, 
                  layer1_size );
      product_type = swp_type;

   } 

   else if( (product_code == ss_type)  ||
	    (product_code == spd_type) ||
	    (product_code == ftm_type) ||
	    (product_code == pup_type) ){

      /* If this is the SPD product, do nothing. */
      /* If this is the SS product, do nothing. */

      product_type = stand_alone_type;

   }

   else {

      switch (ORPGPAT_get_format_type (ORPGPAT_get_prod_id_from_code (product_code))) {

	  case FORMAT_TYPE_RADIAL :

              /* Product is a radial run-length-encoded type */
              status = decode_radial_rle( &product_data[ product_offset ] ); 
	      fprintf (stderr,"Generic radial type defined for this product\n");
	      product_type = generic_radial_type;
	      break;

          case FORMAT_TYPE_HIRES_RADIAL :

              /* Product is a radial hires type */
              decode_dhr( &product_data[ product_offset ] ); 
	      fprintf (stderr,"Generic hires radial type defined for this product\n");
    	      product_type = generic_hires_radial_type;
	      break;

          case FORMAT_TYPE_RASTER :

              /* Product is a raster run-length-encoded type */
              status = decode_raster_rle( &product_data[ product_offset ] );
	      fprintf (stderr,"Generic raster type defined for this product\n");
	      product_type = generic_raster_type;
	      break;

	  default :

              fprintf( stderr, "Unknown Product Type: Packet Code = %2x\n",
                   product_type );
              return -1;

      }
   }

   return product_type;

}

/************************************************************************
 *	Description: This function is used to read the specified	*
 *		     legacy product file (in Concurrent format).	*
 *									*
 *	Input:  file_name - pointer to full filename string		*
 *	Output:	product_data - pointer to buffer where product data	*
 *		     are to be written.					*
 *	Return: product size on success; -1 on error			*
 ************************************************************************/

int read_concurrent_product_file( char *file_name,
                                  short **product_data )
{
   short message_header[9];
   int   fd, product_size, status;
   off_t offset = 0;
   Prod_header* header;
   char* legacy_prod_data;

   /* Open file for read only */
   fd = open( file_name, O_RDONLY );
   if( fd == -1 ){

      fprintf( stderr, "Unable to Open Input File.\n" );
      return fd;

   }

   /* Read product message header */
   read( fd, message_header, 18 );

#ifdef LINUX
   MISC_swap_shorts(9, message_header);
#endif

   /* Extract the size of this product file, in bytes */
   product_size = (message_header[4] << 16 ) | (message_header[5] & 0xffff);

   /* Allocate a buffer for the product data */
   *product_data = (short*)malloc( product_size + ORPG_PROD_HEAD_SIZE );
   assert( *product_data != NULL );

   /*  Fill a dummy internal ORPG header with zeroes  */
   memset(*product_data, 0, ORPG_PROD_HEAD_SIZE);

   legacy_prod_data = ((char*)(*product_data)) + ORPG_PROD_HEAD_SIZE;

   /* Read in the product data from the product file */
   offset = lseek( fd, offset, SEEK_SET );
   status = read( fd, legacy_prod_data, product_size);

   /* Check status of read operation.  If error, return error. */
   /* Otherwise, return number of bytes read. */
   if( status == -1 ){
      fprintf( stderr, "Error On Product Read.\n" );
   }

   header = (Prod_header*)(*product_data);

   return product_size;

}

/************************************************************************
 *	Description: This function decodes the header from the input	*
 *		     product data buffer.				*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: offset to symbology block				*
 ************************************************************************/

int decode_header( short *product_data ){

   int source_id, height, day, month, year, i, j;
   int hours, degrees, minutes, seconds, fullword;
   float latitude, longitude;
   size_t length;


   /* Allocate space for product pertinent attributes */
   attribute = malloc( sizeof( struct product_pertinent) );
   assert( attribute != NULL );

   if (debug)
	fprintf(stderr, "attribute = %p, &attribute=%p\n", attribute, &attribute);

   /* Allocate space for annotations data */
   for( i = 0; i < 10; i++ ){

      attribute->text[i] = malloc( 40*sizeof(char) + 1 );
      assert( attribute->text[i] != NULL );

      /* Initialize data to blanks */
      for( j = 0; j < 19; j++ ){

         attribute->text[i][j] = ' ';

      }

   }
   attribute->number_of_lines = -1;

   /* Extract product code */
   product_code = product_data[ message_code_offset ];
   if (debug)
	fprintf (stderr,"product_code = %d\n", product_code);

   if( product_code >= 16 && product_code <= 300 ){

      char *title;

      length = strlen (ORPGPAT_get_description (
		       ORPGPAT_get_prod_id_from_code (product_code), STRIP_NOTHING));

      title = calloc (length+1,1);

      strcpy (title, ORPGPAT_get_description (
	             ORPGPAT_get_prod_id_from_code (product_code), STRIP_NOTHING));

      /* Build product ID and product resolution text string */
/*
      length = strlen( product_name[ product_code ] );
*/
      for( i = 0; i < length; i++ ){

         attribute->product_name[i] = title[i];
/*
            product_name[ product_code ][ i ];
*/
      }
      attribute->product_name[ length ] = '\0';

   }

   if (debug)
	fprintf (stderr,"attribute: product_name [%s]\n", attribute->product_name);
   /* Extract volume scan time of product */
   if (debug)
	printf("time_ms=%hd, time_ls=%hd, time offset=%d", product_data[time_offset], product_data[time_offset + 1], time_offset);
   fullword = (product_data[ time_offset ] << 16) |
              (product_data[ time_offset + 1 ] & 0xffff);
   convert_time( fullword, &hours, &minutes, &seconds );

   /* Extract volume scan date of product */
   calendar_date( product_data[ date_offset ], &day, &month,
                    &year );
   attribute->number_of_lines++;
   sprintf( attribute->text[0], "%2.2d/%2.2d/%2.2d %2.2d:%2.2d",
            month, day, year, hours, minutes );
   attribute->text[0][18] = '\0';
   if (debug)
	fprintf (stderr,"attribute: text[0] [%s]\n", attribute->text[0]);

   /* Extract the source ID of the product */
   source_id = product_data[ source_id_offset ];

   /* Extract radar latitude */
   fullword = (product_data[ latitude_offset ] << 16 ) |
              (product_data[ latitude_offset+1 ] & 0xffff);
   latitude = ( (float) fullword )/1000.0;
   radar_latitude = latitude;

   /* Convert to degrees, minutes, seconds format */
   degrees_minutes_seconds( latitude,
                            &degrees,
                            &minutes,
                            &seconds );

   /* Combine source id and latitude in one line */
   attribute->number_of_lines++;
   sprintf( attribute->text[1], "RPG: %3d %2.2d/%2.2d/%2.2dN",
            source_id, degrees, minutes, seconds );
   attribute->text[1][18] = '\0';
   if (debug)
	fprintf (stderr,"attribute: text[1] [%s]\n", attribute->text[1]);

   /* Extract the radar height MSL */
   height = product_data[ radar_height_offset ];

   /* Extract radar longitude */
   fullword = (product_data[ longitude_offset ] << 16) | 
              (product_data[ longitude_offset+1 ] & 0xffff);
   longitude = (float) fullword/1000.0;
   radar_longitude = longitude;

   /* Convert longitude to degrees, minutes, seconds format */
   degrees_minutes_seconds( longitude, 
                            &degrees, 
                            &minutes,
                            &seconds );

   /* Combine radar height and longitude in one line */
   attribute->number_of_lines++;
   if( degrees < 0 ){

      degrees = - degrees;
      sprintf( attribute->text[2], "%4d FT %3.2d/%2.2d/%2.2dW", 
               height, degrees, minutes, seconds ); 

   }
   else{

      sprintf( attribute->text[2], "%5d FT  %2.2d/%2.2d/%2.2dE", 
               height, degrees, minutes, seconds ); 

   }

   attribute->text[2][18] = '\0';
   if (debug)
	fprintf (stderr,"attribute: text[2] [%s]\n", attribute->text[2]);

   /* Extract operational mode and vcp.  Note weather mode is stored
      in ICD format (i.e., the way the PUP expects it. 
      */
   attribute->number_of_lines++;

   if( product_data[ mode_offset ] == PRECIPITATION_MODE ){

      sprintf( attribute->text[3], "MODE A/%4d", 
               product_data[ vcp_offset ] );

   }
   else if( product_data[ mode_offset ] == CLEAR_AIR_MODE ){

      sprintf( attribute->text[3], "MODE B/%4d", 
               product_data[ vcp_offset ] );
   }
   else if( product_data[ mode_offset ] == TEST_MODE ){

      sprintf( attribute->text[3], "MODE M/%4d", 
               product_data[ vcp_offset ] );
   }
   attribute->text[3][18] = '\0';
   if (debug)
	fprintf (stderr,"attribute: text[3] [%s]\n", attribute->text[3]);


   /* Extract elevation angle of product */
   attribute->number_of_lines++;
   if( product_data[ vcp_offset ] == vcp21 ){

      attribute->elevation =
         vcp21_elev[ product_data[ elevation_offset ] ];

   }
   else if( product_data[ vcp_offset ] == vcp11 ){

      attribute->elevation = 
         vcp11_elev[ product_data[ elevation_offset ] ];

   }
   else if( product_data[ vcp_offset ] == vcp31 ){

      attribute->elevation = 
         vcp31_elev[ product_data[ elevation_offset ] ];

   }
   else if( product_data[ vcp_offset ] == vcp32 ){

      attribute->elevation = 
         vcp32_elev[ product_data[ elevation_offset ] ];

   }
   else
      attribute->elevation = 0.0; 

   if ( attribute->elevation > 0.000000 ){

      sprintf( attribute->text[4], "ELEV= %.1f DEG", 
               attribute->elevation);
   }

   attribute->text[4][18] = '\0';
   if (debug)
	fprintf (stderr,"attribute: text[4] [%s]\n", attribute->text[4]);


   /* Add product dependent data */
   product_dependent( product_data );

   /* Determine the number of data levels */
   attribute->num_data_levels =
         data_level_tab[ product_data[ message_code_offset ] ];
   if (debug)
	fprintf (stderr,"attribute: num_data_levels = %d\n",
         data_level_tab[ product_data[ message_code_offset ] ]);

   if (debug)
	fprintf(stderr, "Format type for product code %d = %d\n",
	    product_code, ORPGPAT_get_format_type(ORPGPAT_get_prod_id_from_code(product_code)));

   /* Assign data levels. Ignore DHR product. */
   if(( product_code != dhr_type ) &&
      ( product_code != dbv_type ) &&
      ( product_code != dr_type  ) &&
      ( product_code != dv_type  ) &&
      ( ORPGPAT_get_format_type (ORPGPAT_get_prod_id_from_code (product_code))
		     != FORMAT_TYPE_HIRES_RADIAL) &&
      ( ORPGPAT_get_format_type (ORPGPAT_get_prod_id_from_code (product_code))
		     != FORMAT_TYPE_HIRES_RASTER) &&
      ( product_code != dvl_type )) {

      for( i = 0; i < attribute->num_data_levels; i++ ){

         attribute->data_levels[i] =
            malloc(max_level_length*sizeof(char));
         assert( attribute->data_levels[i] != NULL );

         decode_data_level( product_data[ data_level_offset + i ],
                attribute->data_levels[i] );
	 if (debug)
	    fprintf (stderr,"attribute: data_level[%d] = %s\n", i, attribute->data_levels[i]);
      }

   } else {

/*    If this is the digital hires VIL product then we need to decode	*
 *    the various coefficient and offset values from the product header	*/

      if (product_code == dvl_type) {

         double dvl_sign;
	 double dvl_exp;
	 double dvl_mant;
	 double value;

/*	 decode the linear coefficient and offset */

         dvl_sign = (product_data[ dvl_linear_coeff ] & 0x8000) >> 15;
         dvl_exp  = (product_data[ dvl_linear_coeff ] & 0x7c00) >> 10;
	 dvl_exp  = dvl_exp - 16;
         dvl_mant =  product_data[ dvl_linear_coeff ] & 0x03ff;
         value = pow ((double) -1.0,dvl_sign) *
		 pow ((double)  2.0,dvl_exp) *
		 (1.0 + dvl_mant/1024.0);
         attribute->linear_coeff = (float) value;
         dvl_sign = (product_data[ dvl_linear_offset ] & 0x8000) >> 15;
         dvl_exp  = (product_data[ dvl_linear_offset ] & 0x7c00) >> 10;
	 dvl_exp  = dvl_exp - 16;
         dvl_mant =  product_data[ dvl_linear_offset ] & 0x03ff;
         value = pow ((double) -1.0,dvl_sign) *
		 pow ((double)  2.0,dvl_exp) *
		 (1.0 + dvl_mant/1024.0);
         attribute->linear_offset = (float) value;

/*	 decode the log coefficient and offset */

         dvl_sign = (product_data[ dvl_log_coeff ] & 0x8000) >> 15;
         dvl_exp  = (product_data[ dvl_log_coeff ] & 0x7c00) >> 10;
	 dvl_exp  = dvl_exp - 16;
         dvl_mant =  product_data[ dvl_log_coeff ] & 0x03ff;
         value = pow ((double) -1.0,dvl_sign) *
		 pow ((double)  2.0,dvl_exp) *
		 (1.0 + dvl_mant/1024.0);
         attribute->log_coeff = (float) value;
         dvl_sign = (product_data[ dvl_log_offset ] & 0x8000) >> 15;
         dvl_exp  = (product_data[ dvl_log_offset ] & 0x7c00) >> 10;
	 dvl_exp  = dvl_exp - 16;
         dvl_mant =  product_data[ dvl_log_offset ] & 0x03ff;
         value = pow ((double) -1.0,dvl_sign) *
		 pow ((double)  2.0,dvl_exp) *
		 (1.0 + dvl_mant/1024.0);
         attribute->log_offset = (float) value;

         attribute->log_start  = product_data[ dvl_log_start ];

	 if (debug) {
	     fprintf (stderr,"attribute->linear_coeff [%f]\n",
			  attribute->linear_coeff);
	     fprintf (stderr,"attribute->linear_offset [%f]\n",
			  attribute->linear_offset);
	     fprintf (stderr,"attribute->log_coeff [%f]\n",
			  attribute->log_coeff);
	     fprintf (stderr,"attribute->log_offset [%f]\n",
			  attribute->log_offset);
	     fprintf (stderr,"attribute->log_start [%d]\n",
			  attribute->log_start);
	}
      } else {

         attribute->min_value  = product_data[ min_data_value ]/10.0;
         attribute->inc_value  = product_data[ data_increment ]/10.0;
         attribute->num_levels = product_data[ num_data_values ];

	 if (debug) {
	     fprintf (stderr, "attribute->min_value = %f\n", attribute->min_value);
	     fprintf (stderr, "attribute->inc_value = %f\n", attribute->inc_value);
	     fprintf (stderr, "attribute->num_levels = %d\n", attribute->num_levels);
	 }
      }
   }

   /* Set the resolutions for this radial product */
   /* NOTE: By default we get the product resolution from the internal	*
      table.  This is because the current ICD format doesn't contain	*
      full resolution information.  For new generic product types, it	*
      is expected that the product resolution will be defined in the	*
      product attributes table. */

   attribute->x_reso =
          xy_azran_reso[ product_data[ message_code_offset ] ][0];
   attribute->y_reso =
          xy_azran_reso[ product_data[ message_code_offset ] ][1];

   if (debug)
	fprintf (stderr,"attribute: resolution [%5.2f,%5.2f]\n",
      xy_azran_reso[ product_data[ message_code_offset ] ][0],
      xy_azran_reso[ product_data[ message_code_offset ] ][1]);

   /* Extract offset to product symbology block */
   symbology_block = (product_data[ offset_to_symbology ] << 16) |
                     (product_data[ offset_to_symbology+1 ] & 0xffff);

   /* Extract offset to product graphic alphanumeric block */
   grafattr_block = (product_data[ offset_to_grafattr ] << 16) |
                    (product_data[ offset_to_grafattr+1 ] & 0xffff);

   /* Extract offset to product tabular alphanumeric block */
   tabular_block = (product_data[ offset_to_tabular ] << 16) |
                   (product_data[ offset_to_tabular+1 ] & 0xffff);

   /* Treat the SS and SPD products as a special case.  The offset to
      the tabular alphanumeric block is stored in the symbology
      block. */
   if( (product_code == ss_type  || product_code == spd_type ||
	product_code == ftm_type || product_code == pup_type) &&
       tabular_block == 0 ){

       tabular_block = symbology_block;
       grafattr_block = 0;

   }

   /* Return offset to symbology block */
   return symbology_block;

}

/************************************************************************
 *	Description: This function is used to decode radial image data.	*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: 0							*
 ************************************************************************/

int decode_radial_rle( short *product_data ) { 

   int num_halfwords_radial, start_index, bin, i, j, k;
   float total_width;
   unsigned short halfword;

   unsigned char run, color;
 
   /* Allocate memory for the radial_rle struture */
   radial_image = malloc( sizeof(struct radial_rle) );
   assert( radial_image != NULL );
 

   /* Extract the number of data elements, range interval, and number 
      of radials */
   radial_image->data_elements = 
      product_data[ number_of_bins_offset ];
   radial_image->range_interval = 
      product_data[ range_interval_offset ]/1000.0;
   radial_image->number_of_radials = 
      product_data[ number_of_radials_offset ];

   /* Allocate memory for the radial data */
   for( i = 0; i < radial_image->number_of_radials; i++ ){
 
      radial_image->radial_data[i] = malloc( 920*sizeof(short) );
      assert( radial_image->radial_data[i] != NULL );
 
   }

   /* Allocate memory for the azimuth data */
   radial_image->azimuth_angle = 
       malloc( radial_image->number_of_radials*sizeof(float) );
   assert( radial_image->azimuth_angle != NULL );

   /* Allocate memory for the azimuth width data */
   radial_image->azimuth_width = 
        malloc( radial_image->number_of_radials*sizeof(float) );
   assert( radial_image->azimuth_width != NULL );

   start_index = radial_rle_offset;
   total_width = 0.0;

   for( i = 0; i < radial_image->number_of_radials; i++ ){
 
     /* Get the number of halfwords this radial */
      num_halfwords_radial = product_data[ start_index ];
      
      /* Get the radial azimuth and azimuth delta */
      radial_image->azimuth_angle[i] = 
             product_data[ start_index + azimuth_angle_offset ]/10.0;
      radial_image->azimuth_width[i] = 
             product_data[ start_index + azimuth_delta_offset ]/10.0;

      total_width = radial_image->azimuth_width[i] + total_width;

      /* initialize start_index and bin */
      start_index += radial_rle_header_offset;
      bin = 0;
      for( j = 0; j <  num_halfwords_radial; j++, start_index++ ){
          
         halfword = (unsigned short) product_data[ start_index ];

         /* Extract first nibble of RLE data */
         run =  (halfword >> 8)/16;
         color = (halfword >> 8) & 0x0F;
         for( k = 0; k < run; k++, bin++ ){

            radial_image->radial_data[i][bin] = color; 

         } 

         /* Extract second nibble of RLE data */
         run = (halfword & 0xff)/16;
         color = (halfword & 0xff) & 0x0F;
         for( k = 0; k < run; k++, bin++ ){

            radial_image->radial_data[i][bin] = color;

         }  

      } 

   }

   if (ORPGPAT_get_format_type (
		ORPGPAT_get_prod_id_from_code (product_code)) >= 0) {

/*	For radial type products, the azimuth and range resolution	*
 *	are defined in the product header (see ICD).			*/

 	attribute->x_reso = radial_image->range_interval;
	attribute->y_reso = total_width/radial_image->number_of_radials;

   }

   return 0;

}

/************************************************************************
 *	Description: This function is used to decode raster image data.	*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: 0							*
 ************************************************************************/

int decode_raster_rle( short *product_data ){


   int num_halfwords_row, start_index, cell, i, j, k;
   unsigned short halfword;
   unsigned char run, color;


   /* Allocate memory for the raster_rle struture */
   raster_image = malloc( sizeof(struct raster_rle) );
   assert( raster_image != NULL );

   /* Extract the i,j start coordinates, the x,y scale factors, and i
      number of rows */
   raster_image->x_start = product_data[ i_start_offset ];
   raster_image->y_start = product_data[ j_start_offset ];
   raster_image->x_scale = product_data[ x_scale_offset ];
   raster_image->y_scale = product_data[ y_scale_offset ];
   raster_image->number_of_rows =
      product_data[ number_of_rows_offset ];
   raster_image->number_of_columns = raster_image->number_of_rows;

   /* Allocate memory for the raster data */
   for( i = 0; i < raster_image->number_of_rows; i++ ){

      raster_image->raster_data[i] =
             malloc( raster_image->number_of_columns*sizeof(short) );
      assert( raster_image->raster_data[i] != NULL );

   }

   start_index = raster_rle_offset;
   for( i = 0; i < raster_image->number_of_rows; i++ ){

      /* Get the number of halfwords this row */
      num_halfwords_row = product_data[ start_index ]/2;

      /* initialize start_index and bin */
      start_index += raster_rle_header_offset;
      cell = 0;
      for( j = 0; j <  num_halfwords_row; j++, start_index++ ){

         halfword = (unsigned short) product_data[ start_index ];

         /* Extract first nibble of RLE data */
         run =  (halfword >> 8)/16;
         color = (halfword >> 8) & 0x0F;
         for( k = 0; k < run; k++, cell++ ){

            raster_image->raster_data[i][cell] = color;

         }

         /* Extract second nibble of RLE data */
         run = (halfword & 0xff)/16;
         color = (halfword & 0xff) & 0x0F;
         for( k = 0; k < run; k++, cell++ ){

            raster_image->raster_data[i][cell] = color;

         }

      }

   }

   return 0;
}

/************************************************************************
 *	Description: This function is used to decode raster image data.	*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: 0							*
 ************************************************************************/

int decode_hires_raster( short *product_data )
{
   int i, j, left_byte;
   short* raster_data;


   /* Allocate memory for the raster_rle struture */
   raster_image = malloc( sizeof(struct raster_rle) );
   assert( raster_image != NULL );

   /* Extract the i,j start coordinates, the x,y scale factors, and i
      number of rows */
   raster_image->x_start = product_data[ i_start_offset ];
   raster_image->y_start = product_data[ j_start_offset ];
   raster_image->x_scale = product_data[ x_scale_offset ];
   raster_image->y_scale = product_data[ y_scale_offset ];
   raster_image->number_of_rows =
      product_data[ number_of_rows_offset ];
   raster_image->number_of_columns = product_data[raster_rle_offset];

   /* Allocate memory for the raster data */
   for( i = 0; i < raster_image->number_of_rows; i++ )
   {
      raster_image->raster_data[i] =
             malloc( raster_image->number_of_columns*sizeof(short) );
      assert( raster_image->raster_data[i] != NULL );

   }
   left_byte = 1;
   raster_data = &product_data[raster_rle_offset + 1];
   for( i = 0; i < raster_image->number_of_rows; i++ )
   {
      for( j = 0; j <  raster_image->number_of_columns; j++)
      {
	if (left_byte)
	    raster_image->raster_data[i][j] = (*raster_data >> 8) & 0xff;
	else
	    raster_image->raster_data[i][j] = (*raster_data & 0xff);

	if (left_byte == 0)
	   raster_data++;

	if (raster_image->raster_data[i][j] > 255)
	   printf("[%d][%d]=%d\n",i, j, raster_image->raster_data[i][j]);

	left_byte = 1 - left_byte;
      }
   }
   return 0;
}




/************************************************************************
 *	Description: This function is used to decode digital hybrid	*
 *		     reflectivity data.					*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_dhr( short *product_data ) {

   int num_halfwords_radial, start_index, bin, i, j, k;
   unsigned char run, color;
   unsigned short halfword;
   float total_width;


   /* Allocate memory for the radial_rle struture */
   dhr_image = malloc( sizeof(struct dhr_rle) );
   assert( dhr_image != NULL );


   /* Extract number of data elements, range interval, and number of
      radials */
   dhr_image->data_elements = product_data[ number_of_bins_offset ];
   dhr_image->range_interval = attribute->x_reso;
   dhr_image->number_of_radials = product_data[ number_of_radials_offset ];

   if (debug) {
	fprintf (stderr,"dhr_type: data_elements     [%d]\n", dhr_image->data_elements);
	fprintf (stderr,"dhr_type: range_interval    [%5.2f]\n", dhr_image->range_interval);
	fprintf (stderr,"dhr_type: number_of_radials [%d]\n", dhr_image->number_of_radials);
	fprintf (stderr,"dhr_type: min_value         [%5.1f]\n", attribute->min_value);
	fprintf (stderr,"dhr_type: inc_value         [%5.1f]\n", attribute->inc_value);
	fprintf (stderr,"dhr_type: num_levels        [%d]\n", attribute->num_levels);
   }

   /* Allocate memory for the radial data */
   for( i = 0; i < dhr_image->number_of_radials; i++ ){

     dhr_image->radial_data[i] = malloc( dhr_image->data_elements*sizeof(short) );
      assert( dhr_image->radial_data[i] != NULL );

   }

   /* Allocate memory for the azimuth data */
   dhr_image->azimuth_angle = 
       malloc( dhr_image->number_of_radials*sizeof(float) );
   assert( dhr_image->azimuth_angle != NULL );

   /* Allocate memory for the azimuth width data */
   dhr_image->azimuth_width = 
        malloc( dhr_image->number_of_radials*sizeof(float) );
   assert( dhr_image->azimuth_width != NULL );

   start_index = radial_rle_offset;
   total_width = 0.0;

   for( i = 0; i < dhr_image->number_of_radials; i++ ){
 
     /* Get the number of halfwords this radial */
      num_halfwords_radial = product_data[ start_index ]/2;
      
      /* Get the radial azimuth and azimuth delta */
      dhr_image->azimuth_angle[i] = 
             product_data[ start_index + azimuth_angle_offset ]/10.0;
      dhr_image->azimuth_width[i] = 
             product_data[ start_index + azimuth_delta_offset ]/10.0;

      total_width = dhr_image->azimuth_width[i] + total_width;

      /* initialize start_index and bin */
      start_index += radial_rle_header_offset;
      bin = 0;

      for( j = 0; j <  num_halfwords_radial; j++, start_index++ ){
          
         halfword = (unsigned short) product_data[ start_index ];
         /* Extract the colors */
         dhr_image->radial_data[i][bin++] = (halfword >> 8); 
         dhr_image->radial_data[i][bin++] = (halfword & 0xff);
      } 
   }
  
/* If this is a new product whos format is defined in the product	*
 * attributes table then get it's resolution from there (if the		*
 * fields are defined.							*/

   if (ORPGPAT_get_format_type (
		ORPGPAT_get_prod_id_from_code (product_code)) > 0) {
  
      if (ORPGPAT_get_resolution (
		ORPGPAT_get_prod_id_from_code (product_code),
		ORPGPAT_X_AZI_RES) > 0) {

         attribute->y_reso = (ORPGPAT_get_resolution (
		ORPGPAT_get_prod_id_from_code (product_code),
		ORPGPAT_X_AZI_RES)) / 10.0;
	
      } else {

	 attribute->y_reso = total_width/dhr_image->number_of_radials;

      }
  
      if (ORPGPAT_get_resolution (
		ORPGPAT_get_prod_id_from_code (product_code),
		ORPGPAT_Y_RAN_RES) > 0) {

         attribute->x_reso = (ORPGPAT_get_resolution (
		ORPGPAT_get_prod_id_from_code (product_code),
		ORPGPAT_Y_RAN_RES)) / 1000.0;
	
      } else {

	 attribute->x_reso = 230/dhr_image->data_elements;

      }
   }

   return;
}

/************************************************************************
 *	Description: This function is used to decode storm tracking	*
 *		     data.						*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_sti( short *product_data, short product_offset, 
                 int layer1_size ){

   int last_index, number_storms;
   short block_size, packet_type, packet_size, i, j, k;
   static short four_km = 4;
  
   /* Allocate space for sti stucture */
   sti_image = malloc( sizeof( struct sti ) );
   assert( sti_image != NULL ); 

   /* Initialize the number of storms and number of past and
      forcast positions. */
   sti_image->number_storms = -1;
   for( i = 0; i < 100; i++ ){
      sti_image->number_past_pos[i] = -1;
      sti_image->number_forcst_pos[i] = -1;
   }

   /* Process the Symbology Block. */
   last_index = product_offset + layer1_size;
   while( product_offset < last_index ){

      /* Extract the packet type. */
      packet_type = product_data[ product_offset ];

      if( packet_type == 15 ){

         /* This is a Storm ID packet. Increment # of storms. */
         sti_image->number_storms++;
         i = sti_image->number_storms;
   
         /* Set the the Storm ID */
         sti_image->equiv.storm_id[sti_image->number_storms][2] =
            '\0';
         sti_image->equiv.storm_id_hw[sti_image->number_storms][0] = 
            product_data[ product_offset + packet15_stormid ];

         /* Set the x and y positions. */
         sti_image->curr_xpos[sti_image->number_storms] = 
            product_data[ product_offset + packet15_xpos ]/four_km;
         sti_image->curr_ypos[sti_image->number_storms] = 
            product_data[ product_offset + packet15_ypos ]/four_km;

         /* Update product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
      
      else if( packet_type == 23 ){
     
         /* Extract packets within this packet. */
         packet_size = product_data[ product_offset + 1 ]/2;
         block_size = product_offset + packet_size + 2;
    
         /* Move offset to start of first packet */
         product_offset += 2;

         /* Decode all packets within this block of packets. */
         while( product_offset < block_size ){ 
        
            packet_type = product_data[ product_offset ];

            /* If special symbol packet, get x and y positions */
            if( packet_type == special_symbol_packet ){
               sti_image->number_past_pos[i]++;
               j = sti_image->number_past_pos[i];
               sti_image->past_xpos[i][j] = 
                  product_data[product_offset + packet2_xpos]/four_km;
               sti_image->past_ypos[i][j] = 
                  product_data[product_offset + packet2_ypos]/four_km;
            }

            /* Update product offset. */
            packet_size = product_data[ product_offset + 1 ]/2;
            product_offset = product_offset + packet_size + 2;

         }
      }
      
      else if( packet_type == 24 ){
     
         /* Extract packets within this packet. */
         packet_size = product_data[ product_offset + 1 ]/2;
         block_size = product_offset + packet_size + 2;

         /* Move offset to start of first packet */
         product_offset += 2;

         /* Decode all packets within this block of packets. */
         while( product_offset < block_size ){ 
        
            packet_type = product_data[ product_offset ];

            /* If special symbol packet, get x and y positions */
            if( packet_type == special_symbol_packet ){
               sti_image->number_forcst_pos[i]++;
               k = sti_image->number_forcst_pos[i];
               sti_image->forcst_xpos[i][k] = 
                  product_data[product_offset + packet2_xpos]/four_km;
               sti_image->forcst_ypos[i][k] = 
                  product_data[product_offset + packet2_ypos]/four_km;
            }

            /* Update product offset. */
            packet_size = product_data[ product_offset + 1 ]/2;
            product_offset = product_offset + packet_size + 2;
         }
      }
      
      else if( packet_type == 25 ){
         /* This is a slow mover. */
         
         /* Set number of past and forecast positions to flag value. */
         sti_image->number_past_pos[i] = slow_mover; 
         sti_image->number_forcst_pos[i] = slow_mover; 
      }

      else{
         /* This is a don't care packet type. */

         /* Update the product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
   }       
}

/************************************************************************
 *	Description: This function is used to decode hail index		*
 *		     data.						*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_hi( short *product_data, short product_offset, 
                 int layer1_size ){

   int last_index, number_storms;
   short block_size, packet_type, packet_size;
   short prob_hail, prob_svr;
   static short four_km = 4;
  
   /* Allocate space for hi stucture */
   hi_image = malloc( sizeof( struct hi ) );
   assert( hi_image != NULL ); 

   /* Initialize the number of storms. */
   hi_image->number_storms = -1;

   /* Process the Symbology Block. */
   last_index = product_offset + layer1_size;
   while( product_offset < last_index ){

      /* Extract the packet type. */
      packet_type = product_data[ product_offset ];

      if( packet_type == 15 ){

         /* Set the the Storm ID */
         hi_image->equiv.storm_id[hi_image->number_storms][2] = '\0';
         hi_image->equiv.storm_id_hw[hi_image->number_storms][0] = 
            product_data[ product_offset + packet15_stormid ];

         /* Set the x and y positions. */
         hi_image->curr_xpos[hi_image->number_storms] = 
            product_data[ product_offset + packet15_xpos ]/four_km;
         hi_image->curr_ypos[hi_image->number_storms] = 
            product_data[ product_offset + packet15_ypos ]/four_km;

         /* Update product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;

      }
      
      else if( packet_type == 19 ){
     
         /* Extract packets within this packet. */

         /* This is a Storm ID packet. Increment # of storms. */
         hi_image->number_storms++;

         /* Extract the probabilities and maximum hail size. */
         prob_hail = product_data[ product_offset + packet19_poh ];
         prob_svr = product_data[ product_offset + packet19_psh ];
         if( prob_hail > 0 || prob_svr > 0 ){
         
            /* Both probabilities > 0. */
               hi_image->prob_hail[hi_image->number_storms] = prob_hail;
               hi_image->prob_svr[hi_image->number_storms] = prob_svr;
               hi_image->hail_size[hi_image->number_storms] = 
                  product_data[ product_offset + packet19_mhs ];
         }
         else{

            /* Ignore this storm. */
            hi_image->number_storms--;
         }

         /* Update product offset. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;

      }
      
      else{
         /* This is a don't care packet type. */

         /* Update the product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
   }       
}

/************************************************************************
 *	Description: This function is used to decode mesocyclone	*
 *		     data.						*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_meso( short *product_data, short product_offset, 
                  int layer1_size ){

   int last_index, number_storms, number_symbols, cnt, i;
   short block_size, packet_type, packet_size;
   static short four_km = 4;
  
   /* Allocate space for meso stucture */
   meso_image = malloc( sizeof( struct meso ) );
   assert( meso_image != NULL ); 

   /* Initialize the number of mesocyclones and 3D shears. */
   meso_image->number_mesos = -1;
   meso_image->number_3Dshears = -1;

   /* Process the Symbology Block. */
   last_index = product_offset + layer1_size;
   while( product_offset < last_index ){

      /* Extract the packet type. */
      packet_type = product_data[ product_offset ];

      if( packet_type == 15 ){

         /* Set the the Storm ID */
         meso_image->equiv.storm_id[meso_image->number_mesos][2] 
            = '\0';
         meso_image->equiv.storm_id_hw[meso_image->number_mesos][0]  
            = product_data[ product_offset + packet15_stormid ];

         /* Update product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
      
      else if( packet_type == 3 ){
     
         /* This is a mesocyclone packet. */

         /* Increment # of mesocyclones. */
         meso_image->number_mesos++;

         /* Extract the mesocyclone position. */
         meso_image->meso_xpos[meso_image->number_mesos] = 
            product_data[ product_offset + packet3_xpos ]/four_km;
         meso_image->meso_ypos[meso_image->number_mesos] = 
            product_data[ product_offset + packet3_ypos ]/four_km;
   
         /* Extract the mesocyclone size. */
         meso_image->meso_size[meso_image->number_mesos] =
            product_data[ product_offset + packet3_size ]/four_km;

         /* Update product offset. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;

      }

      else if( packet_type == 11 ){
     
         /* This is a 3D shear packet. */

         /* Extract the packet size.  If there are multiple symbols
            in this packet, each symbol requires 3 halfwords to 
            define.  The packet has two halfwords of overhead. */
         packet_size = product_data[ product_offset + 1 ]/2;

         /* Calculate the number of symbols. */
         number_symbols = packet_size / HW_PER_MESO_SYMBOL;

         for( cnt = 0; cnt < number_symbols; cnt++ ){

            /* Increment # of 3D shears. */
            meso_image->number_3Dshears++;

            /* Define packet offset for this symbol. */
            i = cnt*HW_PER_MESO_SYMBOL;

            /* Extract the 3D shear position. */
            meso_image->a3D_xpos[meso_image->number_3Dshears] = 
               product_data[ product_offset+packet3_xpos+i ]/four_km;
            meso_image->a3D_ypos[meso_image->number_3Dshears] = 
               product_data[ product_offset+packet3_ypos+i ]/four_km;

            /* Extract the 3D shear size. */
            meso_image->a3D_size[meso_image->number_3Dshears] =
               product_data[ product_offset+packet3_size+i ]/four_km;

         }

         /* Update product offset. */
         product_offset = product_offset + packet_size + 2;

      }
      
      else{
         /* This is a don't care packet type. */

         /* Update the product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
   }       
}

/************************************************************************
 *	Description: This function is used to decode tornado detection	*
 *		     data.						*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_tvs( short *product_data, short product_offset, 
                  int layer1_size ){

   int last_index, number_storms;
   short block_size, packet_type, packet_size;
   static short four_km = 4;
  
   /* Allocate space for tvs stucture */
   tvs_image = malloc( sizeof( struct tvs ) );
   assert( tvs_image != NULL ); 

   /* Initialize the number of TVSs. */
   tvs_image->number_tvs = -1;

   /* Process the Symbology Block. */
   last_index = product_offset + layer1_size;
   while( product_offset < last_index ){

      /* Extract the packet type. */
      packet_type = product_data[ product_offset ];

      if( packet_type == 15 ){

         /* Set the the Storm ID */
         tvs_image->equiv.storm_id[tvs_image->number_tvs][2] = '\0';
         tvs_image->equiv.storm_id_hw[tvs_image->number_tvs][0] = 
            product_data[ product_offset + packet15_stormid ];

         /* Update product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
      
      else if( packet_type == 12 ){
     
         /* This is a TVS packet. */

         /* Increment # of TVSs. */
         tvs_image->number_tvs++;

         /* Extract the TVS position. */
         tvs_image->tvs_xpos[tvs_image->number_tvs] = 
            product_data[ product_offset + packet12_xpos ]/four_km;
         tvs_image->tvs_ypos[tvs_image->number_tvs] = 
            product_data[ product_offset + packet12_ypos ]/four_km;
   
         /* Update product offset. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;

      }

      else{
         /* This is a don't care packet type. */

         /* Update the product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
   }       

}

/************************************************************************
 *	Description: This function is used to decode velocity wind	*
 *		     profile data.					*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_vwp( short *product_data, short product_offset, 
                 int layer1_size ){

   /* Constants extracted from VWP product generator for 
      calculating time */
   float time_const1 = 90.33;
   float time_const2 = 38.334;
   float max_volume_scans = 10.0;
   float time;

   /* Constants extracted from VWP product generator for 
      calculating height */
   float height_const1 = 10.0;
   float height_const2 = 460.0;

   int dummy_arg, last_index, done, i, j;
   size_t length;
   short packet_type, block4_length, height;
   short height_counter = -1, time_counter = -1;
   char *packet_text;

   /* Allocate memory for vwp product data */
   vwp_image = malloc( sizeof( struct vwp ) );
   assert( vwp_image != NULL );

   /* Initialize the time and height strings */
   for( i = 0; i < 30; i++ ){

      vwp_image->heights[i][0] = '\0';

   }

   for( i = 0; i < 11; i++ ){

      vwp_image->times[i][0] = '\0';

   }

   /* Initialize the number of times and number of heights to 
      missing */
   vwp_image->number_of_times = -1;
   vwp_image->number_of_heights = -1;

   /* Initialize the color for all possible times and heights to 0 */
   /* Initialize the heights and times to all possible values */
   /* Initialize the speed and direction to missing value */

   for( i = 0; i < 30; i++ ){ 

      for( j = 0; j < 11; j++ ){ 

         vwp_image->barb[i][j].color = 0;
         vwp_image->barb[i][j].time = j;
         vwp_image->barb[i][j].height = i;
         vwp_image->barb[i][j].direction = -666;
         vwp_image->barb[i][j].speed = -666;

      }

   } 

   /* Decode all pertinent data packets in this product */
   done = 0;
   last_index = product_offset + layer1_size;
   packet_type = product_data[ product_offset ];

   while( !done && product_offset < last_index ){

      if( packet_type == 0x8 ){

         decode_text_packet_8( &product_data[ product_offset ], 
                               &packet_text,
                               &dummy_arg,
                               &dummy_arg );

         length = strlen( packet_text );

         if( length == 4 && strcmp("TIME", packet_text) ){

            /* This must be a time field */
            vwp_image->number_of_times++; 
            strncpy( vwp_image->times[vwp_image->number_of_times],
                    packet_text, 4 );
            vwp_image->times[vwp_image->number_of_times][4] = '\0';

         } else if( length == 2 && strcmp("ND", packet_text) ){
            /* This must be a height field */
            vwp_image->number_of_heights++; 
            strncpy( vwp_image->heights[vwp_image->number_of_heights],
                    packet_text, 2 );
            vwp_image->heights[vwp_image->number_of_heights][2] = '\0';

         }

         /* Update the product_offset */
         product_offset = product_offset +
                 product_data[ product_offset+packet_size_offset]/2 + 2;

         /* Extract next packet type for next pass */   
         packet_type = product_data[ product_offset ]; 

         free( packet_text );
         
      } 

      else if( packet_type == 0xA ){

         /* Find the length of this packet and add to product_offset */
         /* Currently, we are ignoring this packet code */
         product_offset = product_offset + 
                  product_data[product_offset+packet_size_offset]/2 + 2; 

         /* Extract next packet type for next pass */   
         packet_type = product_data[ product_offset ]; 

      }

      else if( packet_type == 0x4 ){

         /* This is wind barb data */
    
         /* Find length of data block n number of shorts*/
         block4_length = 
            product_data[ product_offset + 1 ]/2; 

         product_offset += block4_overhead; 
         while( block4_length > 0 ){

            /* Calculate time_counter and height_counter from pixel 
               location data */ 
            time = (float) product_data[product_offset+packet4_time_offset]; 
            time_counter = (int) ((time - time_const1)/time_const2 + 0.5); 
            time_counter = max_volume_scans - time_counter;
            if( time_counter < 0 || time_counter > 10 ) {

               fprintf( stderr, "time_counter out of range!" );
               return;

            }

            height = product_data[product_offset + packet4_height_offset]; 
            height_counter = ( vwp_image->number_of_heights + 1 ) - 
                             (((float)height-height_const1)/height_const2 *
                             (float)(vwp_image->number_of_heights+2));
            if( height_counter < 0 || height_counter > 29 ) {

               fprintf( stderr, "height_counter out of range!" );
               return;

            }
       
            /* Store color, time, height, direction, and speed */
            vwp_image->barb[height_counter][time_counter].color = 
                product_data[ product_offset + packet4_color_offset ]; 
 
            vwp_image->barb[height_counter][time_counter].time = 
                time_counter; 

            vwp_image->barb[height_counter][time_counter].height = 
                height_counter; 

            vwp_image->barb[height_counter][time_counter].direction = 
                product_data[ product_offset + packet4_direction_offset ]; 

            vwp_image->barb[height_counter][time_counter].speed = 
                product_data[ product_offset + packet4_speed_offset ]; 

            /* Update the product_offset */
            product_offset = product_offset + packet4_size;

            /* Update the number of shorts remaining in this block */
            block4_length -= packet4_size;
 
         }  

         /* Extract next packet type for next pass */   
         packet_type = product_data[ product_offset ]; 

      }

      else{

         done = 1;
      }
   }

   return;
}

/************************************************************************
 *	Description: This function is used to decode severe weather	*
 *		     probability data.					*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_swp( short *product_data, short product_offset, 
                 int layer1_size ){

   int last_index, number_storms;
   static short four_km = 4;
   short block_size, packet_type, packet_size;
  
   /* Allocate space for swp stucture */
   swp_image = malloc( sizeof( struct swp ) );
   assert( swp_image != NULL ); 

   /* Initialize the number of storms. */
   swp_image->number_storms = -1;

   /* Process the Symbology Block. */
   last_index = product_offset + layer1_size;
   packet_size = product_data[ product_offset + 1 ];
   while( product_offset < last_index ){

      /* Extract the packet type. */
      packet_type = product_data[ product_offset ];

      if( packet_type == 8 ){

         /* Increment the number of storms. */
         swp_image->number_storms++;
         assert( swp_image->number_storms <= 99 );

         /* Set the value of the text string. */
         swp_image->text_value[swp_image->number_storms] = 
            product_data[ product_offset + packet8_text_value ];

         /* Set the x and y positions. */
         swp_image->xpos[swp_image->number_storms] = 
            product_data[ product_offset + packet8_xpos ]/four_km;
         swp_image->ypos[swp_image->number_storms] = 
            product_data[ product_offset + packet8_ypos ]/four_km;

         /* Set the SWP value. */
         swp_image->equiv.swp_hw[swp_image->number_storms][0] = 
            product_data[ product_offset + packet8_text_start ];
         
         swp_image->equiv.swp[swp_image->number_storms][2] = '\0';

         /* Update product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ];
         product_offset = product_offset + packet_size/2 + 2;

      }
      
      else{
         /* This is a don't care packet type. */
 
         fprintf( stderr, "Unknown packet type %d\n",packet_type);
         /* Update the product offset for next pass. */
         packet_size = product_data[ product_offset + 1 ]/2;
         product_offset = product_offset + packet_size + 2;
      }
   }       
}

/************************************************************************
 *	Description: This function is used to decode data level data.	*
 *									*
 *	Input:  level - data level to decode				*
 *	Output: decoded_level - pointer to decoded level string		*
 *	Return: NONE							*
 ************************************************************************/

void decode_data_level( short level, char *decoded_level ){

   float float_level;
   static float scale_factor = 1.0;
   int i;
   char qualifier;

   scale_factor = 1.0;

   /* Initialize string to blanks and null terminate */
   for( i = 0; i < max_level_length; i++ ){
      decoded_level[i] = ' ';
   }
   decoded_level[6] = '\0';

   /* If level is less than zero, then the least significant byte has special 
      meaning */
   if ( level < 0 ){

      switch( (level & 0xff) ){

         case 1:
            decoded_level[4] = 'T';
            decoded_level[5] = 'H';
            break;

         case 2:
            decoded_level[4] = 'N';
            decoded_level[5] = 'D';
            break;
   
         case 3:
            decoded_level[4] = 'R';
            decoded_level[5] = 'F';
            break;

         default:
            break;
      }
 
   } 
   
   /* Most significant bit is not set, so low order byte is numeric value */
   else{

      /* Add qualifier if required */ 
      qualifier = ' ';
      if( (level >> 8) & 0x08 )
         qualifier = '>';
      
      else if( (level >> 8) & 0x04 )
         qualifier = '<';

      else if( (level >> 8) & 0x02 )
         qualifier = '+';

      else if( (level >> 8) & 0x01 )
         qualifier = '-';  

      /* Determine if a scale factor bit is set */
      if( (level >> 8) & 0x20 )
         scale_factor = 100.0;
      
      else if( (level >> 8) & 0x10 )
         scale_factor = 10.0;

      level_to_ASCII( scale_factor, (level & 0xff), decoded_level ); 
     
      /* Add qualifier to data level, if required */
      for( i = 0; i < 6; i++ ){
         if( decoded_level[ i ] != ' ' ){
            decoded_level[ i-1 ] = qualifier;
            break;
         }
      } 
      
   }

}

/************************************************************************
 *	Description: This function is used to convert a scaled data	*
 *		     level to an ASCII string.				*
 *									*
 *	Input:  scale_factor - scale factor value			*
 *		level - data level value				*
 *	Output: string - data level string				*
 *	Return: NONE							*
 ************************************************************************/

void level_to_ASCII( float scale_factor, unsigned char level, char *string ){

   unsigned char digit = 0;
   short number_of_digits, i = 5;
   short check_for_blank;

   /* Convert data level to ASCII text string */   
     check_for_blank = 0; 
     do{

      /* Get next digit to be converted */
      digit = (level % 10);

      /* If check for blank flag is set, clear it */
      if( check_for_blank ) check_for_blank = 0;

      /* Convert next digit to ASCII and update data level */
      string[i--] = digit + 0x30;
      level /= 10;

      /* Add decimal point if number is scaled */
      if( scale_factor == 10.0 && i == 4 ){
         string[i] = '.';
         i--;
         check_for_blank = 1;
      }
      else if( scale_factor == 100.0 && i >= 3 ){
         if( i == 3 ){
            string[i] = '.';
            i--;
         }
         check_for_blank = 1;
      }
   }

   /* Continue until digit is converted */
   while( level != 0 || check_for_blank );

}

/************************************************************************
 *	Description: This function is used to convert a composite time	*
 *		     value (in hhmmss) into hours, minutes, and seconds.*
 *									*
 *	Input:  timevalue - time (in hhmmss)				*
 *	Output: hours - hour						*
 *		minutes - minute					*
 *		seconds - second					*
 *	Return: NONE							*
 ************************************************************************/

void convert_time( int timevalue, int *hours, int *minutes, int *seconds )
{

   /* Extract the number of hours */
   *hours = timevalue/3600;

   /* Extract the number of minutes */
   timevalue = timevalue - (*hours)*3600;
   *minutes = timevalue/60;

   /* Extract the number of seconds */
   *seconds = timevalue - *minutes*60;

   if (debug)
	printf("volume time = %d=%d:%d:%d\n", timevalue, *hours, *minutes, *seconds);

}

/************************************************************************
 *	Description: This function is used to deallocate memory used	*
 *		     for a previously openned product.			*
 *									*
 *	Input:  product_code - product code of previously openned	*
 *			product						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void free_memory( short product_code ){

   int i, j;

   /* Free memory for product annotations data */
   if( attribute != NULL ){
     
      if(( product_code != dhr_type ) &&
	 ( product_code != dbv_type ) &&
	 ( product_code != dr_type  ) &&
	 ( product_code != dv_type  ) &&
	 ( product_type != generic_hires_radial_type ) &&
	 ( product_code != dvl_type )) {

         for( i = 0; i < attribute->num_data_levels; i++ ){

            free( attribute->data_levels[i] );

         }
        
       }

      for( i = 0; i < 10; i++ ){

         free( attribute->text[i] );

      }

      free( attribute );
      attribute = NULL;
   }


   /* Free memory if product had graphic attributes */
   if( gtab != NULL ){
     
      for( i = 0; i <= gtab->number_of_lines; i++ ){

         free( gtab->text[i] );

      }

      free( gtab );
      gtab = NULL;

   }

   /* Free memory if product had tabular alphanumeric */
   if( ttab != NULL ){
     
      for( i = 0; i < ttab->number_of_pages; i++ ){

         for( j = 0; j <= ttab->number_of_lines[i]; j++ ){

            /* Free memory for text this page and line. */
            free( ttab->text[ i ][ j ] );

         }
         
         /* Free memory for line this page. */
         free( ttab->text[i] );

      }

      free( ttab->number_of_lines );

      /* Free memory for text. */
      free( ttab->text );
      free( ttab );
      ttab = NULL;
      
   }


   /* Free memory for radial image data */
   if( radial_image != NULL ){

      free( radial_image->azimuth_angle );
      free( radial_image->azimuth_width );

      for( i = 0; i < radial_image->number_of_radials; i++ ){

         free( radial_image->radial_data[i] );

      } 

      free( radial_image );
      radial_image = NULL;

   }

   /* Free memory for dhr image data */
   if( dhr_image != NULL ){

      free( dhr_image->azimuth_angle );
      free( dhr_image->azimuth_width );

      for( i = 0; i < dhr_image->number_of_radials; i++ ){

         free( dhr_image->radial_data[i] );

      } 

      free( dhr_image );
      dhr_image = NULL;

   }


   /* Free memory for raster image */
   if( raster_image != NULL ){

      for( i = 0; i < raster_image->number_of_rows; i++ ){

         free( raster_image->raster_data[i] );

      }

      free( raster_image );
      raster_image = NULL;

   }

   /* Free memory for vwp image */
   if( vwp_image != NULL ){
   
      free( vwp_image );
      vwp_image = NULL;
   
   }

   /* Free memory for sti image */
   if( sti_image != NULL ){
   
      free( sti_image );
      sti_image = NULL;
   
   }

   /* Free memory for hi image */
   if( hi_image != NULL ){
   
      free( hi_image );
      hi_image = NULL;
   
   }

   /* Free memory for meso image */
   if( meso_image != NULL ){
   
      free( meso_image );
      meso_image = NULL;
   
   }

   /* Free memory for tvs image */
   if( tvs_image != NULL ){
   
      free( tvs_image );
      tvs_image = NULL;
   
   }

   /* Free memory for swp image */
   if( swp_image != NULL ){
   
      free( swp_image );
      swp_image = NULL;
   
   }

}

/************************************************************************
 *	Description: This function is used to decode the graphics	*
 *		     attributes block from the product data buffer.	*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_grafattr_block( short *product_data ){

   int length_of_block;

   char *packet_text, *old_string, *new_string;

   /* Initialize variables to NULL */
   int number_of_pages = 0;   
   int page_number = 0;
   int page_length = 0;
   int page_offset = 0;
   int length_offset = 0;

   int line_number;
   int col_number;
   int packet_code;
   int packet_length;
   int packet_code_offset;
   int packet_length_offset;
   int bytes_remaining;
   int new_string_length;
   int i, j;

   /* Verify this block is ID 2 */
   if( product_data[ block_id_offset ] != 2 ) return;

   /* If graphic alphanumeric block, allocate storage for text data */
   gtab = malloc( sizeof( struct graphic_attr ) );
   assert( gtab != NULL );

   /* Initialize the number of lines of table to -1 */
   gtab->number_of_lines = -1;
   line_number = -1;

   /* Extract the number of pages and page length in bytes */
   number_of_pages = product_data[ num_pages_offset ];

   length_of_block = (product_data[ block_length_offset ] << 16 ) |
                     (product_data[ block_length_offset+1 ] & 0xffff);

   /* Initialize offsets */
   page_offset = first_page_offset;
   length_offset = first_page_length_offset;
   packet_code_offset = first_packet_offset;
   packet_length_offset = first_packet_length_offset;

   for( i = 0; i < number_of_pages; i++ ){
  
      /* Set the page number and page length in bytes */
      page_number = product_data[ page_offset ];
      page_length = product_data[ length_offset ];
     
      /* Set the number of bytes remaining to the page length */
      bytes_remaining = page_length;

      while( bytes_remaining > 0 ){
      
         /* Extract packets code and packet length. */
         packet_code = product_data[ packet_code_offset ];
         packet_length = product_data[ packet_length_offset ];
         
         /* Currently only care about text packets (packet code 8) */
         if( packet_code == 8 ){
    
            decode_text_packet_8( &product_data[ packet_code_offset], 
                                  &packet_text,
                                  &line_number,
                                  &col_number );
            
            /* Calculate the line number this page. */
            line_number = (i*LINES_PER_PAGE) + line_number; 
     
            if( gtab->number_of_lines < line_number ) {

               /* Increment the number of lines of text data */
               gtab->number_of_lines = gtab->number_of_lines + 1;
            
               /* Assign text pointer to structure */
               gtab->text[ gtab->number_of_lines ] = packet_text;

            }

            else {
  
               /* Extract the pointer to the original text string. */ 
               old_string = gtab->text[ line_number ];

               /* Calculate the length of the new string. */
               new_string_length = strlen( old_string ) + 
                                   strlen( packet_text );

               /* Allocate storage for the new string. */
               new_string = malloc( new_string_length*sizeof(char)+1 );
               assert( new_string != NULL );

               /* Concatenate the strings, and NULL terminate the string. */
               new_string[0] = '\0';
               strcat( new_string,
                       old_string );

               new_string[col_number] = '\0';
               strcat( new_string, 
                       packet_text );

               for( j = new_string_length - 1; j > 0; j-- ){

                  if( new_string[j] >= '0' && new_string[j] <= 'z' ) {

                     new_string[ j+1 ] = '\0';
                     break;

                  }

               }

               /* Assign new string to gtab structure and free old
                  strings. */
               gtab->text[ line_number ] = new_string;
               
               free( old_string );
               free( packet_text );

            }
               
         }

         /* Update bytes remaining in this data block */
         bytes_remaining -= ( packet_length + packet_overhead_bytes );

         /* Update word offsets to packet code and packet length */
         packet_code_offset = packet_code_offset + 
                              (packet_length+1)/2 + 
                              2;
         packet_length_offset = packet_code_offset + 
                                1;

      }

      /* Update word offsets for page number and page length */
      page_offset = page_offset + 
                    (page_length+1)/2 + 
                    page_header_data_size;
      length_offset = page_offset + 
                      1;

      /* Update word offsets for packet code and packet length */
      packet_code_offset = length_offset + 
                           1;
      packet_length_offset = packet_code_offset + 
                             1;

   }

   /* Return */
}

/************************************************************************
 *	Description: This function is used to decode the tabular	*
 *		     data block from the product data buffer.		*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void decode_tabular_block( short *product_data,
                           unsigned short product_type ){

   int length_of_block;

   int block_offset, char_this_line, i, j;
   int lines_this_page;

   /* Verify this block is ID 3 */
   if( product_data[ block_id_offset ] != 3 &&
       product_type != stand_alone_type ){
      
      if( debug )
         fprintf( stderr, "Block ID is not 3. ID = %d\n",
                  product_data[ block_id_offset ] );
      return;
   }

   /* If tabular alphanumeric block, allocate storage for text data */
   ttab = malloc( sizeof( struct tabular_attr ) );
   assert( ttab != NULL );

   /* Initialize the number of pages. */
   if( product_type != stand_alone_type ){

      ttab->number_of_pages = product_data[ tabular_page_offset ];

   }
   else{

      ttab->number_of_pages = product_data[ sa_tabular_page_offset ];

   }
 
   if( debug )
      fprintf( stderr, "number of tabular pages: %d\n", 
               ttab->number_of_pages );

   /* Allocate storage for lines per page. */
   ttab->number_of_lines = 
      malloc( ttab->number_of_pages * sizeof( int ) );
   assert( ttab->number_of_lines != NULL );

   /* Allocate storage for text data. */
   ttab->text = malloc( sizeof(char ***) * ttab->number_of_pages );
   assert( ttab->text != NULL );

   if( product_type != stand_alone_type )

      block_offset = tabular_page_offset + 1;

   else

      block_offset = sa_tabular_page_offset + 1;

   for( i = 0; i < ttab->number_of_pages; i++ ){
  
      /* Allocate storage for up to 17 lines per page. */
      ttab->text[ i ] = malloc( 17 * sizeof( char ** ) );
      assert( ttab->text[i] != NULL );

      /* Initialize the number of lines this page to -1. */
      ttab->number_of_lines[ i ] = lines_this_page =  -1;

      while( product_data[ block_offset ] != -1 ){
        
         /* Allocate storage for char_this_line. */ 
         lines_this_page++; 
         ttab->text[ i ][ lines_this_page ] = 
            malloc( 81 * sizeof( char ) );
         assert( ttab->text[i][ lines_this_page ] != NULL );

         /* Extract the number of characters this line. */
         char_this_line = product_data[ block_offset ];

         /* Update block offset. */
         block_offset++;

         /* Copy line out of product data. */
         strncpy( ttab->text[ i ][ lines_this_page ],
                  (char *) &product_data[ block_offset ],
                  char_this_line ); 

         /* NULL terminate the line. */
         ttab->text[ i ][ lines_this_page ][ char_this_line ] = '\0';

         /* Update block offset, then go to next line. */
         block_offset += (char_this_line + 1)/2;

      }

      /* Update the number of lines this page. */
      ttab->number_of_lines[ i ] = lines_this_page;

      /* Increment block offset to start at next page. */
      block_offset++;
   }  
}

/************************************************************************
 *	Description: This function is used to decode a text packet	*
 *		     code 8 block from the product data buffer.		*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: packet_text - text data					*
 *		line_number - text line number				*
 *		col_number  - text column number			*
 *	Return: NONE							*
 ************************************************************************/


void decode_text_packet_8( short *product_data, 
                           char **packet_text,
                           int *line_number,
                           int *col_number ) {

   static short j_pix_start = 0;
   static short i_pix_start = 0;
   short j_pix, i_pix, string_length;
   unsigned short halfword;

   int number_of_chars, i, j;
   

   /* Get the length of the text data only. */
   string_length = product_data[ packet8_length ] - 
                   packet8_header_length;
   
   /* Allocate storage for text data. */ 
   *packet_text = malloc( string_length * sizeof( char )+1 );
   assert( *packet_text != NULL );

   /* If this is the first time called for this product, perform 
      some initialization. */
   if( *line_number == -1 ){

      *line_number = 0;

      /* Extract the pixel start positions. */
      j_pix_start = product_data[ packet8_ypos ];
      i_pix_start = product_data[ packet8_xpos ];

   }

   /* Extract the pixel locations for row and column computation. */
   j_pix = product_data[ packet8_ypos ];
   i_pix = product_data[ packet8_xpos ];
   
   /* Calculate which row this belongs to. */
   *line_number = (int) ( (float) (j_pix - j_pix_start)/10.0 + 0.5 );
   *col_number = (int) ( (float) (i_pix - i_pix_start)/7.0 + 0.5 );

   /* Extract the text data. */
   for( i = 0, j = 0; i < (string_length+1)/2; i++, j += 2 ){

      halfword = product_data[ packet8_text_start + i ];  
#ifndef LITTLE_ENDIAN_MACHINE
      (*packet_text)[ j+1 ] = (halfword & 0xff);
      (*packet_text)[ j ] = (halfword >> 8);
#else
      (*packet_text)[ j+1 ] = (halfword >> 8); 
      (*packet_text)[ j ] = (halfword & 0xff); 
#endif

   }

   /* Pad the text data with a NULL terminator. */
   (*packet_text)[ string_length ] = '\0';
 
   /* Return. */

}

/************************************************************************
 *	Description: This function is used to decode product dependent	*
 *		     data from the product data buffer.			*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void product_dependent( short *product_data ){

   int product_code, i, int_range, int_azimuth;
   unsigned char date_string[9];
   int day, month, year, hours, minutes;
   float obscuration;

   /* Set the azimuth/range center to 0/0 */
   attribute->center_azimuth = 0.0;
   attribute->center_range = 0.0;

   /* Set the full screen flag */
   attribute->full_screen = 1;

   /* NULL terminate the units array */
   attribute->units[0] = '\0';
   attribute->units[5] = '\0';

   /* Get product code */
   product_code = product_data[ message_code_offset ];
   attribute->product_code = product_code;
  
   if( product_code >= 16 && product_code <= 21 ){
   /* Process Base Reflectivity */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX= %3d dBZ", product_data[ 46 ] );
  
      /* Assign units for color bar */
      sprintf( attribute->units, "  dBZ" );
   }
  
   else if( product_code == dhr_type || product_code == dr_type ){
   /* Process Reflectivity */

      /* Assign units for color bar */
      sprintf( attribute->units, "  dBZ" );
   }
  
   else if( product_code == dbv_type || product_code == dv_type ){
   /* Process Velocity */

      /* Assign units for color bar */
      sprintf( attribute->units, "  m/s" );
   }
  
   else if( product_code == dvl_type ){
   /* High Resolution VIL */

      /* Assign units for color bar */
      sprintf( attribute->units, "kg/m2" );
   }
  
   else if( product_code >= 22 && product_code <= 27 ){
   /* Process Base Velocity */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX= %4d KT %3d KT", product_data[ 46 ],
               product_data[ 47 ] );
  
      /* Assign units for color bar */
      sprintf( attribute->units, "  KTS" );
   }
  
   else if( product_code >= 28 && product_code <= 30 ){
   /* Process Base Spectrum Width */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX= %3d KT", product_data[ 46 ] );
  
      /* Assign units for color bar */
      sprintf( attribute->units, "  KTS" );
   }

   else if( product_code == 34 ){
   /* Process Clutter Filter Control */
      
      attribute->number_of_lines++;
      if ( (0x02 & product_data[ 26 ]) ){
         sprintf( attribute->text[ attribute->number_of_lines ],
                  "ELEV SEG NUMBER 1" );  
      }

      else if ( (0x04 & product_data[ 26 ]) ){
         sprintf( attribute->text[ attribute->number_of_lines ],
                  "ELEV SEG NUMBER 2" );  
      }

      attribute->number_of_lines++;
      if( (0x01 & product_data[ 26 ]) ){
         sprintf( attribute->text[ attribute->number_of_lines ],
                  "CHANNEL D" );  
      }
      else{
         sprintf( attribute->text[ attribute->number_of_lines ],
                  "CHANNEL S" );  
      }

      attribute->number_of_lines++;
      calendar_date( product_data[ 47 ], &day, &month, &year );
      hours = product_data[ 48 ] / 60;
      minutes = product_data[ 48 ] - hours*60;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "BM %2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
               hours, minutes );

      attribute->number_of_lines++;
      calendar_date( product_data[ 49 ], &day, &month, &year );
      hours = product_data[ 50 ] / 60;
      minutes = product_data[ 50 ] - hours*60;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "NW %2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
               hours, minutes );

   }

   else if ( product_code >= 35 && product_code <= 38 ){
   /* Process Composite Reflectivity and contour */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX= %3d dBZ", product_data[ 46 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "  dBZ" );
   }

   else if ( product_code == 41 || product_code == 42 ){
   /* Process echo tops and echo tops contour */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "MAX=  %2d KFT", product_data[ 46 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "  KFT" );
   }

   else if ( product_code == 43 ){
   /* Process severe weather reflectivity */ 

      /* Clear the full screen flag */
      attribute->full_screen = 0;
      
      attribute->number_of_lines++;
      attribute->center_azimuth = (float) product_data[ 26 ]/10.0;
      attribute->center_range = (float) product_data[ 27 ]/10.0;
      int_azimuth = (int) (attribute->center_azimuth + 0.5 );
      int_range = (int) (attribute->center_range + 0.5 );
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "MAX=  %2d dBZ", product_data[ 46 ] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "HEIGHT:  %2d KFT AGL", product_data[ 48 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "  dBZ" );
   }

   else if ( product_code == 44 ){
   /* Process severe weather velocity */ 
 
      /* Clear the full screen flag */
      attribute->full_screen = 0;

      attribute->number_of_lines++;
      attribute->center_azimuth = (float) product_data[ 26 ]/10.0;
      attribute->center_range = (float) product_data[ 27 ]/10.0;
      int_azimuth = (int) (attribute->center_azimuth + 0.5 );
      int_range = (int) (attribute->center_range + 0.5 );
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "MAX=  %3d KTS", product_data[ 46 ] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "HEIGHT:  %2d KFT AGL", product_data[ 48 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "  KTS" );
   }

   else if ( product_code == 45 ){
   /* Process severe weather width */ 

      /* Clear the full screen flag */
      attribute->full_screen = 0;

      attribute->number_of_lines++;
      attribute->center_azimuth = (float) product_data[ 26 ]/10.0;
      attribute->center_range = (float) product_data[ 27 ]/10.0;
      int_azimuth = (int) (attribute->center_azimuth + 0.5 );
      int_range = (int) (attribute->center_range + 0.5 );
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "MAX=  %2d KTS", product_data[ 46 ] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "HEIGHT:  %2d KFT AGL", product_data[ 48 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "  KTS" );
   }

   else if ( product_code == 46 ){
   /* Process severe weather shear */ 

      /* Clear the full screen flag */
      attribute->full_screen = 0;

      attribute->number_of_lines++;
      attribute->center_azimuth = (float) product_data[ 26 ]/10.0;
      attribute->center_range = (float) product_data[ 27 ]/10.0;
      int_azimuth = (int) (attribute->center_azimuth + 0.5 );
      int_range = (int) (attribute->center_range + 0.5 );
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               " %3d E-3/S  %2dE-3/S", product_data[ 46 ],
               product_data[ 47 ] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "HEIGHT:  %2d KFT AGL", product_data[ 48 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "E-3/S" );
   }

   else if ( product_code == 48 ){
   /* Process vertical  wind profile */ 

      /* Clear the full screen flag */
      attribute->full_screen = 0;

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "MAX=%3d DEG  %3d KT", product_data[47], 
               product_data[46] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "ALT: %5d FT", product_data[ 48 ]*10 );

      /* Assign units for color bar */
      sprintf( attribute->units, "RMS" );
   }

   else if ( product_code == 55 ){
   /* Process storm relative velocity region */ 

      /* Clear the full screen flag */
      attribute->full_screen = 0;

      attribute->number_of_lines++;
      attribute->center_azimuth = (float) product_data[ 26 ]/10.0;
      attribute->center_range = (float) product_data[ 27 ]/10.0;
      int_azimuth = (int) (attribute->center_azimuth + 0.5 );
      int_range = (int) (attribute->center_range + 0.5 );
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "CNTR= %3d DEG  %3d NM", int_azimuth, int_range );
 
      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX= %4d KT %3d KT", product_data[ 46 ],
               product_data[ 47 ] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "SRM:%3dDEG %3d KT", product_data[51]/10,
               product_data[50]/10 );

      /* Assign units for color bar */
      sprintf( attribute->units, "  KTS" );
   }

   else if ( product_code == 56 ){
   /* Process storm relative velocity map */ 
 
      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX= %4d KT %3d KT", product_data[ 46 ],
               product_data[ 47 ] );

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "SRM:%3dDEG %3d KT", product_data[51]/10,
               product_data[50]/10 );

      /* Assign units for color bar */
      sprintf( attribute->units, "  KTS" );
   }

   else if ( product_code == 57 ){
   /* Process VIL */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX=  %2d KG/M2", product_data[ 46 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "KG/M2" );
   }

   else if ( product_code == 58 ){
   /* Process STI */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "%3d IDENTIFIED STMS", product_data[ 46 ] );
   }

   else if ( product_code == 59 ){
   /* Process HI */
   
      /* Nothing to do! */
   }

   else if ( product_code == 63 || product_code == 65 ||
             product_code == 67 ){
   /* Process Layer 1 Composite Reflectivity Max or Ave, or
      Layer 1 Composite Reflectivity AP removed */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX=  %2d dBZ", product_data[ 46 ] );
      
      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "ALT= %d - %2d KFT", product_data[ 47 ],
	       product_data[ 48 ] );

      /* Assign units for color bar */
      sprintf( attribute->units, "  KFT" );
   }

   else if ( product_code == 64 || product_code == 66 ){
      /* Process Layer 2 Composite Reflectivity Max or Ave */
    
      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX=  %2d dBZ", product_data[ 46 ] );
 
      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "ALT=  %2d- %2d KFT", product_data[ 47],
               product_data[ 48 ] );
   
      /* Assign units for color bar */
      sprintf( attribute->units, "  KFT" );
   }
  
   else if ( product_code == 89 || product_code == 90 ){
      /* Process Layer 3 Composite Reflectivity Max or Ave */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX=  %2d dBZ", product_data[ 46 ] );
  
      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "ALT=  %2d- %2d KFT", product_data[ 47],
               product_data[ 48 ] );
 
      /* Assign units for color bar */
      sprintf( attribute->units, "  KFT" );
   }

   else if ( product_code == 78 || product_code == 79 ){
   /* Process storm total precipitation */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX=  %.2f IN", product_data[ 46 ]/10.0 );
      
      attribute->number_of_lines++;
      calendar_date( product_data[ 49 ], &day, &month, &year );
      hours = product_data[ 50 ] / 60;
      minutes = product_data[ 50 ] - hours*60;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "END=%2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
               hours, minutes );

      /* Assign units for color bar */
      sprintf( attribute->units, "   IN" );
   }

   else if ( product_code == 80 ){
   /* Process storm total precipitation */

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "MAX=  %.2f IN", product_data[ 46 ]/10.0 );
      
      attribute->number_of_lines++;
      calendar_date( product_data[ 47 ], &day, &month, &year );
      hours = product_data[ 48 ] / 60;
      minutes = product_data[ 48 ] - hours*60;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "BEG=%2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
               hours, minutes );
      
      attribute->number_of_lines++;
      calendar_date( product_data[ 49 ], &day, &month, &year );
      hours = product_data[ 50 ] / 60;
      minutes = product_data[ 50 ] - hours*60;
      sprintf( attribute->text[ attribute->number_of_lines ],
               "END=%2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
               hours, minutes );

      /* Assign units for color bar */
      sprintf( attribute->units, "   IN" );
   }

   else if ( product_code == 87 ){
   /* Process combined shear */ 

      attribute->number_of_lines++;
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "MAX= %3d E-4/S", product_data[ 46 ] );

      attribute->number_of_lines++;
      attribute->center_azimuth = (float) product_data[ 47 ]/10.0;
      attribute->center_range = (float) product_data[ 48 ]/10.0;
      int_azimuth = (int) (attribute->center_azimuth + 0.5 );
      int_range = (int) (attribute->center_range + 0.5 );
      sprintf( attribute->text[ attribute->number_of_lines ], 
               "POS= %3d DEG %3dNM", int_azimuth, int_range );

      /* Assign units for color bar */
      sprintf( attribute->units, "E-3/S" );

   }
   
   else {

      sprintf( attribute->units, " --- " );

   }
}

/************************************************************************
 *	Description: This function is used to decode a latitude or	*
 *		     longitude into its components (deg, min, sec).	*
 *									*
 *	Input:  latlong - latitude or longitude value (degrees)		*
 *	Output: deg - degrees						*
 *		min - minutes						*
 *		sec - seconds						*
 *	Return: NONE							*
 ************************************************************************/

void degrees_minutes_seconds( float latlong, int *deg, int *min, int *sec ){

   /* Extract the number of degrees */
   *deg = (int) latlong;

   /* Extract the number of minutes */
   latlong -= (float) *deg;
   if( latlong < 0.0 ) latlong = -latlong;
   *min = (int) ( latlong*60.0 );

   /* Extract the number of seconds */
   latlong -= ( (*min)/60.0 );
   *sec = (int) ( latlong*3600.0 );

}

/************************************************************************
 *	Description: This function is used to decode a julian date	*
 *		     into a calendar date.				*
 *									*
 *	Input:  date - julian date (from 1/1/1970)			*
 *	Output: dd - calendar day					*
 *		dm - calendar month					*
 *		dy - calendar year					*
 *	Return: NONE							*
 ************************************************************************/

void calendar_date( short date, int *dd, int *dm, int *dy ){

   int l,n, julian;

   /* Convert modified julian to type integer */
   julian = date;

   /* Convert modified julian to year/month/day */
   julian += 2440587;
   l = julian + 68569;
   n = 4*l/146097;
   l = l -  (146097*n + 3)/4;
   *dy = 4000*(l+1)/1461001;
   l = l - 1461*(*dy)/4 + 31;
   *dm = 80*l/2447;
   *dd= l -2447*(*dm)/80;
   l = *dm/11;
   *dm = *dm+ 2 - 12*l;
   *dy = 100*(n - 49) + *dy + l;
   *dy = *dy - 1900;
   while (*dy > 99) {
	*dy = *dy-100;
   } 
   return;
}
