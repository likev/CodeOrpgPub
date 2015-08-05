/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2011/04/06 16:39:50 $ 
 * $Id: prod_cmpr_decompress.cpp,v 1.3 2011/04/06 16:39:50 steves Exp $ 
 * $Revision: 1.3 $ 
 * $State: Exp $ 
 */ 
#include <stdio.h>

extern "C"
{
#include <bzlib.h>
#include <zlib.h>
#include <orpg.h>
}

static int Malloc_dest_buf( int dest_len, char **dest_buf );
static void Process_bzip2_error( int ret );
static void Process_zlib_error( int ret );

/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed returned.
      The size of the decompressed product is stored in "size".

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data.

   Outputs:
      size - size of the decompressed product.
   
   Returns: 
      Pointer to decompressed product or NULL on error.

***************************************************************/
void* Decompress_product( void *bufptr, int *size ){

   int ret;
   unsigned int dest_len, src_len;
   unsigned short alg;
   unsigned long long_dest_len, long_src_len;

   char *prod_data = NULL;
   char *dest = NULL;
   Graphic_product *phd = NULL;

   phd = (Graphic_product *) bufptr;
   prod_data = (char *) (((char *) bufptr) + sizeof(Graphic_product));
 
   /* Find the original and compressed size of the product. */
   ORPGMISC_unpack_value_from_ushorts( &phd->msg_len, &src_len );
   ORPGMISC_unpack_value_from_ushorts( &phd->param_9, &dest_len );
   dest_len += sizeof(Graphic_product);

   /* Check the destination buffer.  If not allocated, allocate 
      the same size as the original (i.e., uncompressed) data. */
   if( (dest_len == 0) || ((ret = Malloc_dest_buf( dest_len, &dest )) < 0) )
      return( NULL );

   /* Get the algorithm used for compression so we know how to decompress. */
   alg = (unsigned short) phd->param_8;

   /* Do the decompression. */
   switch( alg ){

      case COMPRESSION_BZIP2:
      {

         /* Since the Product header and description blocks are not compressed, 
            account for them here. */
         dest_len -= sizeof(Graphic_product);
         src_len -= sizeof(Graphic_product);

         /* Do the bzip2 decompression. */
         ret = BZ2_bzBuffToBuffDecompress( dest + sizeof(Graphic_product),
                                           &dest_len, prod_data, (unsigned int) src_len,
                                           0, 0 );

         /* Process Non-Normal return. */
         if( ret != BZ_OK ){

            Process_bzip2_error( ret );

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

         /* Since the product header and description blocks are not compressed, 
            account for them here. */
         dest_len -= sizeof(Graphic_product);
         src_len -= sizeof(Graphic_product);

         long_dest_len = (unsigned long) dest_len;
         long_src_len = (unsigned long) src_len;

         /* Do the zlib decompression. */
         ret = uncompress( (Bytef *)(dest + sizeof(Graphic_product)),
                           &long_dest_len, (Bytef *)prod_data, long_src_len );

         /* Process Non-Normal return. */
         if( ret != Z_OK ){

            Process_zlib_error( ret );

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

         if( dest != NULL )
            free( dest );

         ret = Malloc_dest_buf( *size, &dest );
         if( ret < 0)
            return( NULL );

         /* Just copy the source to the destination. */
         memcpy( (void *) dest, bufptr, *size );

         /* Store the compression type in product dependent parameter 8. */ 
         phd = (Graphic_product *) dest;

         phd->param_8 = (unsigned short) COMPRESSION_NONE;
         phd->param_9 = 0;
         phd->param_10 = 0;

         return( dest );

      }

   /* End of "switch" statement. */
   }

   /* Copy the product header and description block. */ 
   memcpy( (void*) dest, (void*) phd, sizeof(Graphic_product) );

   /* Set the uncompressed length of product in the product header. */
   phd = (Graphic_product *) dest;
   
   *size = dest_len + sizeof(Graphic_product);
   ORPGMISC_pack_ushorts_with_value( (void *) &phd->msg_len, (void *) size );

   /* Store the compression type in product dependent parameter 8. */ 
   phd->param_8 = (unsigned short) COMPRESSION_NONE;
   phd->param_9 = 0;
   phd->param_10 = 0;

   return( dest );

/* Decompress_final_product() */
}

/***************************************************************

   Description:
      Writes error message to task log file.  Error message is
      based on "ret" value.
      
***************************************************************/
static void Process_bzip2_error( int ret ){

   switch( ret ){

      case BZ_CONFIG_ERROR:
         LE_send_msg( GL_ERROR, "BZIP2 Configuration Error\n" );
         break;

      case BZ_PARAM_ERROR:
         LE_send_msg( GL_ERROR, "BZIP2 Parameter Error\n" );
         break;

      case BZ_MEM_ERROR:
         LE_send_msg( GL_ERROR, "BZIP2 Memory Error\n" );
         break;

      case BZ_OUTBUFF_FULL:
         LE_send_msg( GL_ERROR, "BZIP2 Outbuf Full Error\n" );
         break;

      case BZ_DATA_ERROR:
         LE_send_msg( GL_ERROR, "BZIP2 Data Error\n" );
         break;

      case BZ_DATA_ERROR_MAGIC:
         LE_send_msg( GL_ERROR, "BZIP2 Magic Data Error\n" );
         break;

      case BZ_UNEXPECTED_EOF:
         LE_send_msg( GL_ERROR, "BZIP2 Unexpected EOF Error\n" );
         break;

      default:
         LE_send_msg( GL_ERROR, "Unknown BZIP2 Error (%d)\n", ret );
         break;

   /* End of "switch" statement. */
   }

/* End of Process_bzip2_error() */
}

/***************************************************************

   Description:
      Writes error message to task log file.  Error message is
      based on "ret" value.
      
***************************************************************/
static void Process_zlib_error( int ret ){

   switch( ret ){

      case Z_MEM_ERROR:
         LE_send_msg( GL_ERROR, "ZLIB Memory Error\n" );
         break;

      case Z_BUF_ERROR:
         LE_send_msg( GL_ERROR, "ZLIB Buffer Error\n" );
         break;

      default:
         LE_send_msg( GL_ERROR, "Unknown ZLIB Error (%d)\n", ret );
         break;

   /* End of "switch" statement. */
   }

/* End of Process_zlib_error() */
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
static int Malloc_dest_buf( int dest_len, char **dest_buf ){

   /* Allocate an output buffer the same size as the original 
      (i.e., uncompressed) data. */
   *dest_buf = (char*) malloc( dest_len );
   if( *dest_buf == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", dest_len );
      return( -1 );

   }

   return( 0 );

/* End of Malloc_dest_buf() */
}

