/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2008/12/04 14:52:09 $ 
 * $Id: rpg_prod_compress.c,v 1.11 2008/12/04 14:52:09 steves Exp $ 
 * $Revision: 1.11 $ 
 * $State: Exp $ 
 */ 
#include <stdio.h>
#include <rpg_port.h>
#include <orpg.h>
#include <rpg.h>

/*
  Static global variables.
*/

/* General compression macro definitions. */
#define MIN_BYTES_TO_COMPRESS    1000

/* Function prototypes. */
static int CP_compress_intermediate_product( void *bufptr, int method );
static int CP_compress_final_product( void *bufptr, int method );
static int CP_decompress_intermediate_product( void *bufptr, char **dest,
                                               int *size );
static int CP_decompress_final_product( void *bufptr, char **dest,
                                        int *size );
static int CP_malloc_dest_buf( int dest_len, char **dest_buf );

/***************************************************************
   Description:
      Generic function for compressing intermediate and final 
      product data.  

      Product compression is done in-place.

   Inputs:
      bufptr - pointer to algorithm product buffer
      method - compression method

   Outputs:
      status - status of the operation

   Returns:
      Returns -1 on any error and 0 on success.

   Notes:

      We check each product to see whether the product is 
      an Abort Message since the length field in the product
      header is overloaded with an error code.

***************************************************************/
void RPG_compress_product( void *bufptr, int *method, int *status ){

   int int_or_final;
   Prod_header *phd = (Prod_header *) bufptr;

   /* Initialize the status to OK. */
   *status = 0;

   /* If the product is an Abort Message, then just return without 
      compressing the product.   For Abort Messages, the length
      will be a negative value representing an error code. */
   if( phd->g.len <= 0 )
      return;

   /* Check the compression method.  If already compressed, return. */
   if( phd->compr_method != COMPRESSION_NONE ){

      LE_send_msg( GL_ERROR, "Attempting to Compress A Compressed Product\n" );
      return;

   }

   /* Determine if product to be compressed is intermediate or final
      (i.e., ICD) format. */
   if( (int_or_final = ORPGPAT_get_code( phd->g.prod_id )) < 0 ){

      *status = -1;
      LE_send_msg( GL_ERROR, "ORPGPAT_get_code(%d) Return Error in RPG_compress_product()\n",
                   phd->g.prod_id );

   }

   /* Depending on the type of product (intermediate or final),
      call the appropriate compressor. */
   if( int_or_final == INT_PROD )
      *status = CP_compress_intermediate_product( bufptr, *method );

   else if( int_or_final > INT_PROD )
      *status = CP_compress_final_product( bufptr, *method );

   return;

/* End of RPG_compress_product() */
}

/***************************************************************

   Description:
      The function controls the compression of intermediate 
      products.  

      A pointer to a product buffer is passed and all data beyond 
      the ORPG product headeris compressed using compression 
      method specified by "method".  The compression algorithm used
      and the compressed product size are stored in the ORPG 
      product header. 

      Product compression is done in-place.

   Inputs:
      bufptr - pointer to algorithm product buffer
      method - compression method

   Outputs:

   Returns:
      Returns -1 on any error and 0 on success.

   Notes:
      Products less than MIN_BYTES_TO_COMPRESS or Abort Messages
      are not compressed and returned as is.

***************************************************************/
static int CP_compress_intermediate_product( void *bufptr, int method ){

   unsigned int total_len, src_len, dest_len;

   char *dest = NULL, *src = NULL;
   Prod_header *phd = NULL;

   phd = (Prod_header *) bufptr; 
   src = (char *) (((char *) bufptr) + sizeof(Prod_header));
 
   /* Get the original (pre-compression) size of the product,
      which includes the size of the ORPG product header. */
   total_len = (int) phd->orig_size;
   src_len = dest_len = total_len - sizeof(Prod_header);

   /* Not recommended compressing small products. */
   if( (src_len < 0) || (src_len < MIN_BYTES_TO_COMPRESS) ){

      LE_send_msg( GL_INFO, 
             "Product Not Compressed (Product Length Too Small or Abort Message)\n" );
      return( 0 );

   }

   /* Vector to the compression algorithm given by "method" */
   switch( method ){

      case COMPRESSION_NONE:
         LE_send_msg( GL_INFO, "No Compression Method Specified\n" );
         break;

      case COMPRESSION_BZIP2:
         /* Allocate a compression scratch buffer the same size as the
            original data. */
         dest = malloc( src_len );
         if( dest == NULL ){

            LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", src_len );
            return( -1 );

         }

         dest_len = MISC_compress( MISC_BZIP2, src, src_len, dest, src_len );
         break;

      case COMPRESSION_ZLIB:
         /* Allocate a compression scratch buffer. */
         dest = malloc( src_len );
         if( dest == NULL ){

            LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", src_len );
            return( -1 );

         }

         dest_len = MISC_compress( MISC_GZIP, src, src_len, dest, src_len );
         break;
      
      default:
         LE_send_msg( GL_ERROR, "Compression Method %d Not Supported\n",
                      method );
         dest_len = -1;
         break;

   /* End of "switch" statement. */
   }

   /* If the compression was a success, do the following .... */
   if( dest_len >= 0 ){


      /* Copy the compressed data to the product buffer, change the size 
         of the data in the ORPG product header, and indicate in the 
         product header the compression method. */
      memcpy( src, dest, dest_len );

      phd->g.len = dest_len + sizeof(Prod_header);

      /* Store the compression type. */ 
      phd->compr_method = method;

   }
   else
      phd->compr_method = COMPRESSION_NONE;

   /* If destination buffer allocated, free it now. */
   if( dest != NULL )
      free( dest );

   return( 0 );

/* CP_compress_intermediate_product() */
}


/***************************************************************
   Description:
      A pointer to a product buffer is passed and all data beyond 
      the product description block is compressed using compression 
      method specified by "method".  The compression algorithm used
      is stored in product dependent parameter (PDP) param_8 and the
      compressed size of the product (excluding the header and 
      description blocks) is stored in PDP param_9 and parm_10 (see 
      product.h for details on product header and description block).

      Product compression is done in-place.

   Inputs:
      bufptr - pointer to algorithm product buffer
      method - compression method

   Outputs:

   Returns:
      Returns -1 on any error and 0 on success.

   Notes:
      Products (excluding product header and product description
      block) less than MIN_BYTES_TO_COMPRESS are not compressed
      and returned as is.
***************************************************************/
static int CP_compress_final_product( void *bufptr, int method ){

   unsigned int length, total_len, src_len, dest_len;

   char *dest = NULL, *src = NULL;
   Graphic_product *phd = NULL;
   Prod_header *orpg_phd = NULL;

   orpg_phd = (Prod_header *) bufptr;

   phd = (Graphic_product *) (((char *) bufptr) + sizeof(Prod_header));

#ifdef LITTLE_ENDIAN_MACHINE
   /* For Little-Endian machine, this must be done since the product has
      already been converted to network byte order. */
   UMC_msg_hdr_desc_blk_swap( phd );
#endif

   src = (char *) (((char *) bufptr) + sizeof(Graphic_product) + sizeof(Prod_header));
 
   /* Get the original (pre-compression) size of the product. */
   total_len = (int) orpg_phd->orig_size;
   src_len = dest_len = total_len - sizeof(Graphic_product) - sizeof(Prod_header);

   /* Not recommended compressing small products. */
   if( (src_len < 0) || (src_len < MIN_BYTES_TO_COMPRESS) ){

      LE_send_msg( GL_INFO, 
             "Product Not Compressed (Product Length Too Small)\n" );

      /* Initialize product dependent parameters associated with 
         compression in the product description block. */
      phd->param_8 = COMPRESSION_NONE;
      phd->param_9 = phd->param_10 = 0;

#ifdef LITTLE_ENDIAN_MACHINE
      /* Must convert back to network byte order. */
      UMC_msg_hdr_desc_blk_swap( phd );
#endif

      return( 0 );

   }

   /* Vector to the compression algorithm given by "method" */
   switch( method ){

      case COMPRESSION_NONE:
         LE_send_msg( GL_INFO, "No Compression Method Specified\n" );
         break;

      case COMPRESSION_BZIP2:
         /* Allocate a compression scratch buffer. */
         dest = malloc( src_len );
         if( dest == NULL ){

            LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", src_len );
            return( -1 );

         }

         dest_len = MISC_compress( MISC_BZIP2, src, src_len, dest, src_len );
         break;

      case COMPRESSION_ZLIB:
         /* Allocate a compression scratch buffer. */
         dest = malloc( src_len );
         if( dest == NULL ){

            LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", src_len );
            return( -1 );

         }

         dest_len = MISC_compress( MISC_GZIP, src, src_len, dest, src_len );
         break;
      
      default:
         LE_send_msg( GL_ERROR, "Compression Method %d Not Supported\n",
                      method );
         dest_len = -1;
         break;

   /* End of "switch" statement. */
   }

   /* If the compression was a success, do the following .... */
   if( dest_len >= 0 ){

      /* Copy the compressed data to the product buffer, change the size 
         of the data in the product header, indicate the compression 
         method in the product header, and set the method and size
         in the product description block. */
      memcpy( src, dest, dest_len );

      /* Initialize method and size in ORPG product header. */
      orpg_phd->compr_method = method;
      orpg_phd->g.len = dest_len + sizeof(Graphic_product) + sizeof(Prod_header);

      /* Initialize size in the ICD product header. */
      length = dest_len + sizeof(Graphic_product);
      RPG_set_product_int( (void *) &phd->msg_len, (void *) &length ); 

      /* Store the compression type and original size of the product
         in product dependent parameters 8, 9 and 10, respectively. */ 
      phd->param_8 = (unsigned short) method;
      phd->param_9 = (unsigned short) ((src_len >> 16) & 0xffff);
      phd->param_10 = (unsigned short) (src_len & 0xffff);

   }
   else{

      /* If the compression failed, then indicate the product was not 
         compressed. */
      phd->param_8 = COMPRESSION_NONE;
      phd->param_9 = phd->param_10 = 0;

   }

   /* If destination buffer allocated, free it now. */
   if( dest != NULL )
      free( dest );

#ifdef LITTLE_ENDIAN_MACHINE
      /* Must convert back to network byte order. */
      UMC_msg_hdr_desc_blk_swap( phd );
#endif

   return( 0 );

/* End of CP_compress_final_product() */
}

/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed and placed
      in buffer "dest".  The size of the decompressed product is
      stored in "size".

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data.
      dest - pointer to receiving buffer.  If NULL, the buffer
             is allocated on behalf of the user.

      size - size of the receiving buffer.  If 0, the receiving
             buffer is allocated on behalf of the user.

   Outputs:
      dest - receiving buffer holding decompressed product.
      size - size of the decompressed product.
      status - -1 on error, or 0 on success.
   
   Returns: 
      There is no return value defined for this function.

***************************************************************/
void RPG_decompress_product( void *bufptr, char **dest,
                             int *size, int *status ){

   int int_or_final;
   Prod_header *phd = (Prod_header *) bufptr;

   /* If product is an Abort Message, just return.  For Abort 
      Messages, the length will be a negative value representing
      an error code.   Abort messages will never be compressed. */
   if( phd->g.len <= 0 ){

      *status = 0;
      *size = phd->orig_size;

      phd->compr_method = COMPRESSION_NONE;
      *dest = bufptr;

      return;

   }

   /* Determine if product to be decompressed is intermediate or final
      (i.e., ICD) format. */
   if( (int_or_final = ORPGPAT_get_code( phd->g.prod_id )) < 0 ){

      *status = -1;
      LE_send_msg( GL_ERROR, "ORPGPAT_get_code(%d) Return Error in RPG_decompress_product()\n",
                   phd->g.prod_id );

   }

   /* Depending on the type of product (intermediate or final),
      call the appropriate decompressor. */
   if( int_or_final == INT_PROD )
      *status = CP_decompress_intermediate_product( bufptr, dest, size );

   else
      *status = CP_decompress_final_product( bufptr, dest, size );

   return;
   
/* End of RPG_decompress_product() */
}


/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed and placed
      in buffer "dest".  The size of the decompressed product is
      stored in "size".

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data.
      dest - pointer to pointer to receiving buffer.  

      size - size of the receiving buffer.  If 0, the receiving
             buffer is allocated on behalf of the user.

   Outputs:
      dest - receiving buffer holding decompressed product.
      size - size of the decompressed product.
      status - -1 on error, or 0 on success.
   
   Returns: 
      There is no return value defined for this function.

***************************************************************/
static int CP_decompress_intermediate_product( void *bufptr, char **dest,
                                               int *size ){

   int ret, alg;
   unsigned int dest_len, src_len;

   char *prod_data = NULL;
   Prod_header *phd = NULL;

   phd = (Prod_header *) bufptr;
   prod_data = (char *) (((char *) bufptr) + sizeof(Prod_header));

   /* Find the original and compressed size of the product. */
   dest_len = (unsigned int) phd->orig_size;
   src_len = (unsigned int) phd->g.len;

   /* Allocate a destination buffer the same size as the original
      (i.e., uncompressed) data. */
   if( (ret = CP_malloc_dest_buf( dest_len, dest )) < 0 )
      return( ret );


   /* Get the algorithm used for compression so we know how to decompress. */
   alg = phd->compr_method;

   /* Do the decompression. */
   switch( alg ){

      case COMPRESSION_BZIP2:
      {

         /* Subtract the size of the Prod_header from the source and destination lengths
            since the Prod_header is not compressed. */
         dest_len -= sizeof(Prod_header);
         src_len -= sizeof(Prod_header);

         /* Do the bzip2 decompression. */
         dest_len = MISC_decompress( MISC_BZIP2, prod_data, src_len,  
                                     *dest + sizeof(Prod_header), dest_len ); 

         /* Process Non-Normal return. */
         if( dest_len < 0 ){

            /* Free the destination buffer. */
            free(*dest);
            *dest = NULL;
            *size = 0;

            return( -1 );

         }
         else
            break;

      }

      case COMPRESSION_ZLIB:
      {

         /* Subtract the size of the Prod_header from the source and destination lengths
            since the Prod_header is not compressed. */
         dest_len -= sizeof(Prod_header);
         src_len -= sizeof(Prod_header);

         /* Do the zlib decompression. */
         dest_len = MISC_decompress( MISC_GZIP, prod_data, src_len, *dest + sizeof(Prod_header), 
                                     dest_len );

         /* Process error condition. */
         if( dest_len < 0 ){

            /* Free the destination buffer. */
            free(*dest);
            *dest = NULL;
            *size = 0;

            return( -1 );

         }
         else
            break;

      }

      default:
      {
         LE_send_msg( GL_ERROR, "Compression Not Method Not Supported (%x)\n", alg );
      }

      case COMPRESSION_NONE:
      {
         /* Just copy the source to the destination. */
         memcpy( (void *) *dest, bufptr, dest_len );
         *size = dest_len;
         return( 0 );

      }

   /* End of "switch" statement. */
   }

   /* Copy the ORPG product header. */ 
   memcpy( (void*) *dest, (void*) phd, sizeof(Prod_header) );

   /* Set the compression method in the ORPG product header. */
   phd->compr_method = COMPRESSION_NONE;

   /* Set the uncompressed length of product in ORPG product header. */
   phd->g.len = phd->orig_size; 

   /* Return the size of the destination buffer. */
   *size = phd->orig_size;

   return( 0 );

/* CP_decompress_intermediate_product() */
}

/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed and placed
      in buffer "dest".  The size of the decompressed product is
      stored in "size".

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data.
      dest - pointer pointer to receiving buffer.  

      size - size of the receiving buffer.  If 0, the receiving
             buffer is allocated on behalf of the user.

   Outputs:
      dest - receiving buffer holding decompressed product.
      size - size of the decompressed product.
      status - -1 on error, or 0 on success.
   
   Returns: 
      There is no return value defined for this function.

***************************************************************/
static int CP_decompress_final_product( void *bufptr, char **dest,
                                        int *size ){

   int ret;
   unsigned int length, dest_len, src_len;
   unsigned short alg;

   char *prod_data = NULL;
   Prod_header *orpg_phd = NULL;
   Graphic_product *phd = NULL;

   orpg_phd = (Prod_header *) bufptr;
   phd = (Graphic_product *) (((char *) bufptr) + sizeof(Prod_header));
   prod_data = (char *) (((char *) bufptr) + sizeof(Graphic_product) + sizeof(Prod_header));
 
   /* Find the original and compressed size of the product. */
   src_len = (unsigned int) orpg_phd->g.len;
   dest_len = (unsigned int) orpg_phd->orig_size;

   /* Check the destination buffer.  If not allocated, allocate 
      the same size as the original (i.e., uncompressed) data. */
   if( (ret = CP_malloc_dest_buf( dest_len, dest )) < 0 )
      return( ret );

   /* Get the algorithm used for compression so we know how to decompress. */
   alg = (unsigned short) orpg_phd->compr_method;

   /* Do the decompression. */
   switch( alg ){

      case COMPRESSION_BZIP2:
      {

         /* Since the product header and description blocks are not compressed, 
            account for them here. */
         dest_len -= (sizeof(Graphic_product) + sizeof(Prod_header));
         src_len -= (sizeof(Graphic_product) + sizeof(Prod_header));

         /* Do the bzip2 decompression. */
         dest_len = MISC_decompress( MISC_BZIP2, prod_data, src_len, 
                                     *dest + sizeof(Graphic_product) + sizeof(Prod_header),
                                     dest_len );

         /* Process error condition. */
         if( dest_len < 0 ){

            /* Free the destination buffer. */
            free(*dest);
            *dest = NULL;
            *size = 0;

            return( -1 );

         }
         else
            break;

      }

      case COMPRESSION_ZLIB:
      {

         /* Since the product header and description blocks are not compressed, 
            account for them here. */
         dest_len -= (sizeof(Graphic_product) + sizeof(Prod_header));
         src_len -= (sizeof(Graphic_product) + sizeof(Prod_header));

         /* Do the zlib decompression. */
         dest_len = MISC_decompress( MISC_GZIP, prod_data, src_len,
                                     *dest + sizeof(Graphic_product) + sizeof(Prod_header),
                                     dest_len );

         /* Process error condition. */
         if( dest_len < 0 ){

            /* Free the destination buffer. */
            free(*dest);
            *dest = NULL;
            *size = 0;

            return( -1 );

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

         /* Just copy the source to the destination. */
         memcpy( (void *) *dest, bufptr, dest_len );
         *size = dest_len;

         /* Store the compression type in product dependent parameter 8. */ 
         phd = (Graphic_product *) (((char *) (*dest)) + sizeof(Prod_header));
#ifdef LITTLE_ENDIAN_MACHINE
         /* Product in network byte order ... convert to native format. */
         UMC_msg_hdr_desc_blk_swap( phd );
#endif
         phd->param_8 = (unsigned short) COMPRESSION_NONE;
         phd->param_9 = 0;
         phd->param_10 = 0;

#ifdef LITTLE_ENDIAN_MACHINE
         /* Convert back to network byte order. */
         UMC_msg_hdr_desc_blk_swap( phd );
#endif
         return( 0 );

      }

   /* End of "switch" statement. */
   }

   /* Copy the ORPG product header. */
   memcpy( (void*) *dest, (void*) orpg_phd, sizeof(Prod_header) );
 
   /* Copy the product header and description block. */ 
   memcpy( (void*) (((char *)(*dest)) + sizeof(Prod_header)), (void*) phd,
           sizeof(Graphic_product) );

   /* Set the uncompressed length of product in the product header. */
   phd = (Graphic_product *) (((char *) (*dest)) + sizeof(Prod_header));
#ifdef LITTLE_ENDIAN_MACHINE
   /* Product in network byte order ... convert to native format. */
   UMC_msg_hdr_desc_blk_swap( phd );
#endif
   length = dest_len + sizeof(Graphic_product);
   RPG_set_product_int( (void *) &phd->msg_len, (void *) &length );

   /* Set the compression method and length in the ORPG product header. */
   orpg_phd->compr_method = COMPRESSION_NONE;
   orpg_phd->g.len = orpg_phd->orig_size;

   *size = orpg_phd->orig_size;

   /* Store the compression type in product dependent parameter 8. */ 
   phd->param_8 = (unsigned short) COMPRESSION_NONE;
   phd->param_9 = 0;
   phd->param_10 = 0;

#ifdef LITTLE_ENDIAN_MACHINE
   /* Convert back to network byte order. */
   UMC_msg_hdr_desc_blk_swap( phd );
#endif

   return( 0 );

/* CP_decompress_final_product() */
}

/***********************************************************************

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

************************************************************************/
static int CP_malloc_dest_buf( int dest_len, char **dest_buf ){

   /* Allocate an output buffer the same size as the original 
      (i.e., uncompressed) data. */
   *dest_buf = malloc( dest_len );
   if( *dest_buf == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", dest_len );
      return( -1 );

   }

   return( 0 );

/* End of CP_check_dest_buf() */
}

