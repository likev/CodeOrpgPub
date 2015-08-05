/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2008/12/04 14:54:15 $ 
 * $Id: decompress_functs.c,v 1.4 2008/12/04 14:54:15 steves Exp $ 
 * $Revision: 1.4 $ 
 * $State: Exp $ 
 */ 
#include <stdio.h>
#include <bzlib.h>
#include <zlib.h>
#include <orpg.h>
#include <legacy_prod.h>

static int Malloc_dest_buf( int dest_len, char **dest_buf );
static void Process_bzip2_error( int ret );
static void Process_zlib_error( int ret );

/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed returned.
      The size of the decompressed product is stored in "size".
      This function is called for products that DO have a 
      Product Description Block.

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data, includes msg hdr.

   Outputs:
      size - size of the decompressed product.
   
   Returns: 
      Pointer to decompressed product or NULL on error.

***************************************************************/
void* Decompress_product_w_PDB( void *bufptr, int *size ){

   int ret;
   unsigned int dest_len, src_len;
   unsigned short alg;
   unsigned short msw, lsw;
   unsigned short *msw_p, *lsw_p;
   unsigned long long_dest_len, long_src_len;

   char *prod_data = NULL;
   char *dest = NULL;
   Graphic_product *phd = NULL;

   phd = (Graphic_product *) bufptr;
   prod_data = (char *) (((char *) bufptr) + sizeof(Graphic_product));
 
   /* Find the original (compressed) size of the product. */
   msw = *((unsigned short*)bufptr + LGMSWOFF);
   MISC_swap_shorts(1, &msw);
   lsw = *((unsigned short *)bufptr + LGLSWOFF);
   MISC_swap_shorts(1, &lsw);
   src_len = (unsigned int)((0xffff0000 & (msw << 16)) | (lsw & 0xffff));

   /* Find the decompressed size of the product. */
   msw_p = (unsigned short *) &phd->param_9;
   *msw_p = SHORT_BSWAP_L( *msw_p );
   lsw_p = (unsigned short *) &phd->param_10;
   *lsw_p = SHORT_BSWAP_L( *lsw_p );
   ORPGMISC_unpack_value_from_ushorts( msw_p, &dest_len );

   dest_len += sizeof(Graphic_product);

   /* Check the destination buffer.  If not allocated, allocate 
      the same size as the original (i.e., uncompressed) data. */
   if( (dest_len == 0) || ((ret = Malloc_dest_buf( dest_len, &dest )) < 0) )
      return( NULL );

   /* Get the algorithm used for compression so we know how to decompress. */
   alg = SHORT_BSWAP_L( (unsigned short) phd->param_8 );

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
         ret = uncompress( dest + sizeof(Graphic_product),
                           &long_dest_len, prod_data, long_src_len );

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

         /* Just copy the source to the destination. */
         memcpy( (void *) dest, bufptr, dest_len );
         *size = dest_len;

         /* Store the compression type in product dependent parameter 8. */ 
         phd = (Graphic_product *) dest;

         phd->param_8 = SHORT_BSWAP_L( (unsigned short) COMPRESSION_NONE );
         phd->param_9 = 0;
         phd->param_10 = 0;

         return( dest );

      }

   /* End of "switch" statement. */
   }

   /* Copy the product header and description block. */ 
   memcpy( (void*) dest, (void*) phd, sizeof(Graphic_product) );

   /* Set the decompressed size (the function output) */
   *size = dest_len + sizeof(Graphic_product);

   /* Set the uncompressed length MSW and LSW in the product header.*/
   msw_p = (unsigned short*)dest + LGMSWOFF;
   lsw_p = (unsigned short *)dest + LGLSWOFF;
   ORPGMISC_pack_ushorts_with_value( (void *) msw_p, (void *) size );
   *msw_p = SHORT_BSWAP_L( *msw_p );
   *lsw_p = SHORT_BSWAP_L( *lsw_p );

   /* Store the compression type in product dependent parameter 8. */ 
   phd = (Graphic_product *) dest;
   phd->param_8 = SHORT_BSWAP_L( (unsigned short) COMPRESSION_NONE );
   phd->param_9 = 0;
   phd->param_10 = 0;

   return( dest );

} /* Decompress_product_w_PDB() */

/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed returned.
      The size of the decompressed product is stored in "size".
      This function is called for products that DO NOT have a 
      Product Description Block.

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data; includes msg hdr.

   Outputs:
      size - size of the decompressed product.
   
   Returns: 
      Pointer to decompressed product or NULL on error.

***************************************************************/
void* Decompress_product_wo_PDB( void *bufptr, int *size ){

   int ret;
   unsigned int dest_len, src_len;
   unsigned short alg;
   unsigned long long_dest_len, long_src_len;
   unsigned short msw, lsw;

   char *prod_data = NULL;
   char *dest = NULL;
   Prod_msg_header_icd *phd = NULL;

   phd = (Prod_msg_header_icd *) bufptr;

   prod_data = (char *) (((char *) bufptr) + sizeof(Prod_msg_header_icd));
 
   /* Store the compressed size of the product. */
   msw = SHORT_BSWAP_L( phd->lengthm );
   lsw = SHORT_BSWAP_L( phd->lengthl );
   src_len = (unsigned int) (0xffff0000 & ((msw) << 16)) | ((lsw) & 0xffff);

   /* Store the decompressed size of the product. Note: the decompressed size
      of the External Data Message is 8 bytes after the msg hdr. Due to 
      memory alignment problems, we have to piece the dest length together
      out of shorts. */
   msw = SHORT_BSWAP_L( *(unsigned short *)(prod_data + 8) );
   lsw = SHORT_BSWAP_L( *(unsigned short *)(prod_data + 10) );
   dest_len = (unsigned int ) (0xffff0000 & ((msw) << 16)) | ((lsw) & 0xffff);

   /* Note that for the External Data Message we must add 12 bytes to
      the destination length to account for the 12 uncompressed bytes
      after the message header. */
   dest_len += sizeof(Prod_msg_header_icd) + 12;

   /* Check the destination buffer.  If not allocated, allocate 
      the same size as the original (i.e., uncompressed) data. */
   if( (dest_len == 0) || ((ret = Malloc_dest_buf( dest_len, &dest )) < 0) )
      return( NULL );

   /* Get the algorithm used for compression so we know how to decompress.
      NOTE: for the External Data Msg the data is 6 bytes after the msg hdr. */
   alg = SHORT_BSWAP_L( *(unsigned short *)(prod_data + 6) );

   /* Do the decompression. */
   switch( alg ){

      case COMPRESSION_BZIP2:
      {

         /* Since the Product header and description blocks are not compressed, 
            account for them here. */
         dest_len -= sizeof(Prod_msg_header_icd);
         src_len -= sizeof(Prod_msg_header_icd);

        /* For the External Data Message, the first 12 bytes after the 
           msg hdr are not compressed either. */
         dest_len -= 12;
         src_len -= 12;

         /* Do the bzip2 decompression. */
         ret = BZ2_bzBuffToBuffDecompress( dest + sizeof(Prod_msg_header_icd) + 12,
                                           &dest_len, prod_data + 12, (unsigned int) src_len,
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
         dest_len -= sizeof(Prod_msg_header_icd);
         src_len -= sizeof(Prod_msg_header_icd);

         long_dest_len = (unsigned long) dest_len;
         long_src_len = (unsigned long) src_len;

         /* Do the zlib decompression. */
         ret = uncompress( dest + sizeof(Prod_msg_header_icd),
                           &long_dest_len, prod_data, long_src_len );

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

         /* Just copy the source to the destination. */
         memcpy( (void *) dest, bufptr, dest_len );
         *size = dest_len;

         /* Store the compression type.  Note: the compression type in the
            External Data Message is the 4th halfword after the msg hdr. */ 
         *(unsigned short *)(dest + sizeof(Prod_msg_header_icd) + 6) =
            SHORT_BSWAP_L( (unsigned short) COMPRESSION_NONE );

         return( dest );

      }

   /* End of "switch" statement. */
   }

   /* Copy the product header and description block. For External
      Data Message we also need to copy the first 12 bytes after 
      the msg hdr which were also not compressed. */ 
   memcpy( (void*) dest, (void*) bufptr, sizeof(Prod_msg_header_icd) + 12 );

   /* Set the uncompressed length of product in the product header. */
   phd = (Prod_msg_header_icd *) dest;
   
   *size = dest_len + sizeof(Prod_msg_header_icd) + 12;
   ORPGMISC_pack_ushorts_with_value( (void *) &phd->lengthm, (void *) size );
   phd->lengthm = SHORT_BSWAP_L( phd->lengthm );
   phd->lengthl = SHORT_BSWAP_L( phd->lengthl );

   /* Store the new compression type (none) */ 
   *(unsigned short *)(dest + sizeof(Prod_msg_header_icd) + 6) =
      SHORT_BSWAP_L( (unsigned short) COMPRESSION_NONE );

   return( dest );

/* Decompress_product_wo_PDB() */
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
   *dest_buf = malloc( dest_len );
   if( *dest_buf == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", dest_len );
      return( -1 );

   }

   return( 0 );

/* End of Malloc_dest_buf() */
}

