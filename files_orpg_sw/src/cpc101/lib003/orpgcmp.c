/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2008/12/04 14:52:09 $ 
 * $Id: orpgcmp.c,v 1.6 2008/12/04 14:52:09 steves Exp $ 
 * $Revision: 1.6 $ 
 * $State: Exp $ 
 */ 
#include <stdio.h>
#include <orpg.h>

#define ORPGCMP_NOT_COMPRESSED      0
#define ORPGCMP_COMPRESSED          1

/*
  Static global variables.
*/

/* Function prototypes. */
static int CP_bzip2_compression( char *src, int src_len, char **dest, 
                                 int *dest_len, int *flag );
static int CP_zlib_compression( char *src, int src_len, char **dest, 
                                int *dest_len, int *flag );
static int No_compress( int code, char *src, int src_len, char **dest );

/***************************************************************
   Description:
      A pointer to a data buffer is passed and all data is 
      compressed using compression method specified by "code".  
      The compression algorithm used and size of original data
      is stored in ORPGCMP_hdr_t structure preceeding data buffer. 

   Inputs:
      code - compression method
      src - pointer to data buffer.
      src_len - length, in bytes, of data buffer.

   Outputs:
      dest - contains the compressed data or NULL if data is 
             not compressed.

   Returns:
      On success, returns size of compressed data including the
      ORPGCMP_hdr_t header.  On error, negative error code is 
      returned.

   Notes:
      In some cases, the data is not compressed.  In these cases,
      the uncompressed data may or may not contain ORPGCMP_hdr_t
      header.  If not, the "dest" buffer is not allocated and
      0 is returned. 

***************************************************************/
int ORPGCMP_compress( int code, char *src, int src_len, char **dest ){

   int dest_len = 0;

   /* Vector to the compression algorithm given by "code" */
   switch( code ){

      case COMPRESSION_NONE:
      {
         
         /* In the future we may just want to return error.  For 
            now, copy the "src" to "dest". */
         return ( No_compress( code, src, src_len, dest ) );

      }

      case COMPRESSION_BZIP2:
      {
         ORPGCMP_hdr_t *hdr = NULL;
         int flag, ret = 0;

         if( (ret = CP_bzip2_compression( src, src_len, dest, 
                                          &dest_len, &flag )) == 0 ){

            /* Add the ORPGCMP_hdr_t structure only if data is 
               compressed. */
            if( flag == ORPGCMP_COMPRESSED ){

               hdr = (ORPGCMP_hdr_t *) *dest;
               hdr->code = COMPRESSION_BZIP2;
               hdr->orig_len = src_len;
               hdr->comp_len = dest_len;
               hdr->magic_num = ORPGCMP_MAGIC_NUM;

            }

            return (dest_len);

         }
         else
            return (ret);

      } 

      case COMPRESSION_ZLIB:
      {
         ORPGCMP_hdr_t *hdr = NULL;
         int flag, ret = 0;

         if( (ret = CP_zlib_compression( src, src_len, dest,
                                         &dest_len, &flag )) == 0 ){

            /* Add the ORPGCMP_hdr_t structure only if data is
               compressed. */
            if( flag == ORPGCMP_COMPRESSED ){

               hdr = (ORPGCMP_hdr_t *) *dest;
               hdr->code = COMPRESSION_ZLIB;
               hdr->orig_len = src_len;
               hdr->comp_len = dest_len;
               hdr->magic_num = ORPGCMP_MAGIC_NUM;

            }

            return (dest_len);

         }
         else
            return (ret);

      }

     
      default:
         return ( ORPGCMP_BAD_COMPRESSION_CODE );

   /* End of "switch" statement. */
   }

   return 0;

/* ORPGCMP_compress() */
}

/***************************************************************
   Description:
      A pointer to a data buffer and the size of the data buffer
      is passed. The receiving buffer "dest" contains the 
      compressed data with a ORPGCMP_hdr_t structure prepended to 
      the data.  The length of the compressed data is returned in
      "dest_len". 

   Inputs:
      src - pointer to data buffer.
      src_len - length, in bytes, of the data buffer.

   Outputs:
      dest - pointer to a pointer to character buffer to receive
             the data.  
      dest_len - length of receiving buffer.  
      flag - either ORPGCMP_COMPRESSED or ORPGCMP_NOT_COMPRESSED

   Returns:
      returns negative number on error or 0 on success.

   Notes:
      "flag" and "dest_len" are undefined if error returned.

***************************************************************/
static int CP_bzip2_compression( char *src, int src_len, char **dest, 
                                 int *dest_len, int *flag ){

   char *dest_start = NULL;

   /* Not recommended compressing small data buffers. */
   if( src_len < MIN_BYTES_TO_COMPRESS ){

      int code = COMPRESSION_BZIP2;

      *flag = ORPGCMP_NOT_COMPRESSED;
      *dest_len = No_compress( code, src, src_len, dest );
      return 0;

   }

   /* Set flag to indicate data is compressed. */
   *flag = ORPGCMP_COMPRESSED;

   /* Allocate a compression buffer the same size as the
      original data plus a ORPGCMP_hdr_t header. */
   *dest = malloc( src_len + sizeof( ORPGCMP_hdr_t ) );
   if( *dest == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", 
                   src_len + sizeof( ORPGCMP_hdr_t ) );
      return (ORPGCMP_MALLOC_FAILED );

   }

   /* Set the destination start address and set the size of the 
      destination buffer to the size of the source. */
   dest_start = *dest + sizeof( ORPGCMP_hdr_t );
   *dest_len = src_len;

   /* Do the bzip2 compression. */
   *dest_len = MISC_compress( MISC_BZIP2, src, src_len, dest_start, *dest_len );

   /* Process Non-Normal return or data which is not very compressible. */
   if( (*dest_len < 0) || ((*dest_len + sizeof(ORPGCMP_hdr_t)) >= src_len)  ){

      int code = COMPRESSION_BZIP2;

      if( *dest != NULL )
         free( *dest );

      *flag = ORPGCMP_NOT_COMPRESSED;
      *dest_len = No_compress( code, src, src_len, dest ); 
      return 0;

   }

   /* Set the size of the destination to the compressed size plus the
      size of the compression header. */
   *dest_len += sizeof(ORPGCMP_hdr_t);
   return 0;

/* CP_bzip2_compression() */
}

/***************************************************************
   Description:
      A pointer to a data buffer and the size of the data buffer
      is passed. The receiving buffer "dest" contains the 
      compressed data with a ORPGCMP_hdr_t structure prepended to 
      the data.  The length of the compressed data is returned in
      "dest_len". 

   Inputs:
      src - pointer to data buffer.
      src_len - length, in bytes, of the data buffer.

   Outputs:
      dest - pointer to a pointer to character buffer to receive
             the data.  
      dest_len - length of receiving buffer.  
      flag - either ORPGCMP_COMPRESSED or ORPGCMP_NOT_COMPRESSED

   Returns:
      returns negative number on error or 0 on success.

   Notes:
      "flag" and "dest_len" are undefined if error returned.

***************************************************************/
static int CP_zlib_compression( char *src, int src_len, char **dest, 
                                int *dest_len, int *flag ){

   char *dest_start = NULL;
   long long_dest_len;

   /* Not recommended compressing small data buffers. */
   if( src_len < MIN_BYTES_TO_COMPRESS ){

      int code = COMPRESSION_ZLIB;

      *flag = ORPGCMP_NOT_COMPRESSED;
      *dest_len = No_compress( code, src, src_len, dest );
      return 0;

   }

   /* Set flag to indicate data is compressed. */
   *flag = ORPGCMP_COMPRESSED;

   /* Allocate a compression buffer the same size as the
      original data plus a ORPGCMP_hdr_t header. */
   *dest = malloc( src_len + sizeof( ORPGCMP_hdr_t ) );
   if( *dest == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", 
                   src_len + sizeof( ORPGCMP_hdr_t ) );
      return (ORPGCMP_MALLOC_FAILED );

   }

   /* Set the destination start address and set the size of the
      destination buffer to the size of the source. */
   dest_start = *dest + sizeof( ORPGCMP_hdr_t );
   *dest_len = (unsigned int) src_len;
   long_dest_len = (unsigned long) *dest_len;

   LE_send_msg( GL_INFO, "Compressing Using ZLIB\n" );

   /* Do the zlib compression. */
   *dest_len = MISC_compress( MISC_GZIP, src, src_len, dest_start, *dest_len );

   /* Process Non-Normal return. */
   if( *dest_len < 0 ){

      int code = COMPRESSION_ZLIB;

      if( *dest != NULL )
         free( *dest );

      *flag = ORPGCMP_NOT_COMPRESSED;
      *dest_len = (unsigned int) No_compress( code, src, src_len, dest );
      return 0;

   }
   else{

      *dest_len = (unsigned int) long_dest_len + sizeof( ORPGCMP_hdr_t ); 
      return( 0 );

   }

/* CP_zlib_compression() */
}

/***************************************************************
   Description:
      The buffer pointer to by "bufptr" is decompressed and placed
      in buffer "dest".  The size of the decompressed data is
      stored in "size".

   Inputs:
      code - Compression method.
      src - pointer to data buffer containing compressed 
            data.
      src_len - size of the source buffer. 

   Outputs:
      dest - receiving buffer holding decompressed data.
   
   Returns: 
      Returns the size of the decompressed product or negative
      number on error.

   Notes:
      If the data is not compressed, then "dest" is returned
      as NULL and 0 size if returned.

***************************************************************/
int ORPGCMP_decompress( int code, char *src, int src_len,
                        char **dest ){

   int dest_len, internal_code;
   ORPGCMP_hdr_t *hdr = (ORPGCMP_hdr_t *) src;

   /* Check the magic number and compressed size in the compression 
      header.  This is used to determine whether or not the data was 
      compressed.  Note:  The compression header might have been generated
      on a box of different endianness.  If the header is not the correct
      endianness, byte-swap the header. */
   if( hdr->magic_num != ORPGCMP_MAGIC_NUM ){
         
      int temp, num_ints;
      ORPGCMP_hdr_t tmp_hdr;

      temp = INT_BSWAP( hdr->magic_num );
      if( temp == ORPGCMP_MAGIC_NUM ){
            
         num_ints = sizeof( ORPGCMP_hdr_t ) / sizeof( int );
         MISC_bswap( sizeof(int), hdr, num_ints, &tmp_hdr );
         memcpy( hdr, &tmp_hdr, sizeof( ORPGCMP_hdr_t ) );

      }

   }

   if( (hdr->magic_num != ORPGCMP_MAGIC_NUM)
                      &&
       (hdr->comp_len != src_len) ){

      /* Assume the data is not compressed.  Just return. */
      *dest = NULL;
      return 0;

   }

   /* Find the original size of the data. */
   dest_len = hdr->orig_len;
 
   /* Allocate an output buffer the same size as the original 
      (i.e., uncompressed) data. */
   *dest = malloc( dest_len );
   if( *dest == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", dest_len );
      return (ORPGCMP_MALLOC_FAILED);

   }

   /* Get the algorithm used for compression so we know how to decompress. */
   internal_code = hdr->code;

   /* Plan for the possibility the code value passed in needs byte-swapping. */
   if( internal_code != code )
      code = INT_BSWAP( code );
 
   if( (internal_code != code)
                   && 
       (internal_code != COMPRESSION_NONE) ){
   
      LE_send_msg( GL_ERROR, "Internal Code (%d) Mis-Match (%d)\n",
                   internal_code, code );

      if( *dest != NULL )
         free( *dest );

      *dest = NULL;

      return (ORPGCMP_CODE_MISMATCH);

   }

   /* Do the decompression. */
   switch( internal_code ){

      case COMPRESSION_BZIP2:
      {

         char *input_start = src + sizeof(ORPGCMP_hdr_t);
         int input_len = src_len - sizeof(ORPGCMP_hdr_t);

         /* Do the bzip2 decompression. */
         dest_len = MISC_decompress( MISC_BZIP2, input_start, input_len, *dest, dest_len );

         /* Process Non-Normal return. */
         if( dest_len < 0 ){

            /* Free the destination buffer if allocated within this module. */
            if( *dest != NULL )
               free(*dest);

            *dest = NULL;

            return (ORPGCMP_BZIP2_INTERNAL_ERROR);

         }

         return( dest_len );

      }

      case COMPRESSION_ZLIB:
      {

         char *input_start = src + sizeof(ORPGCMP_hdr_t);
         int input_len = src_len - sizeof(ORPGCMP_hdr_t);

         /* Do the zlib decompression. */
         dest_len = MISC_decompress( MISC_GZIP, input_start, input_len, *dest, dest_len );

         /* Process Non-Normal return. */
         if( dest_len < 0 ){

            /* Free the destination buffer if allocated within this module. */
            if( *dest != NULL )
               free(*dest);

            *dest = NULL;

            return (ORPGCMP_ZLIB_INTERNAL_ERROR);

         }

         return( dest_len );

      }

      case COMPRESSION_NONE:
      default:
      {

         char *input_start = src + sizeof(ORPGCMP_hdr_t);
         unsigned int input_len = src_len - sizeof(ORPGCMP_hdr_t);
         
         /* Copy source data to destination buffer. */
         memcpy( *dest, input_start, input_len );
         return (input_len);

      }

   /* End of "switch" statement. */
   }

   return 0;

/* ORPGCMP_decompress() */
}

/***************************************************************

   Description:
      Handles the case of no compression.

   Inputs:
      code - compression code
      src - source data
      src_len - length of the source buffer
      dest - malloc buffer containing destination data
 
   Returns:
      The size of the destination buffer or negative number on
      error.
      
***************************************************************/
static int No_compress( int code, char *src, int src_len, char **dest){

   /* If we were supposed to compress the data but for some 
      reason (either data size too small or data not compressible) 
      we are not compressing the data, must check original data
      for magic number. */
   if( code != COMPRESSION_NONE ){

      ORPGCMP_hdr_t hdr;
      
      /* Copy the ORPGCMP_hdr_t worth of bytes.  We do this to 
         ensure the data is word-aligned. */
      memcpy( &hdr, src, sizeof(ORPGCMP_hdr_t) );

      /* Check the original data to ensure the data does not contain
         the magic number.  If it does we must add an ORPGCM_hdr_t
         header.  This prevents any confusion during the decompression
         of this data. */
      if( hdr.magic_num == ORPGCMP_MAGIC_NUM ){

         ORPGCMP_hdr_t *dest_hdr = NULL;

         *dest = (char *) malloc( src_len + sizeof(ORPGCMP_hdr_t) );
         if( *dest == NULL ){

            LE_send_msg( GL_ERROR, "malloc Failed For %d Bytes\n", src_len );
            return (ORPGCMP_MALLOC_FAILED);

         }

         /* Copy the source data to the destination buffer. */
         memcpy( *dest + sizeof(ORPGCMP_hdr_t), src, src_len ); 

         /* Fill in the ORPGCMP_hdr_t header. */
         dest_hdr = (ORPGCMP_hdr_t *) *dest;
         dest_hdr->code = COMPRESSION_NONE;
         dest_hdr->orig_len = src_len;
         dest_hdr->comp_len = src_len + sizeof(ORPGCMP_hdr_t);
         dest_hdr->magic_num = ORPGCMP_MAGIC_NUM;
  
         return (src_len + sizeof(ORPGCMP_hdr_t));

      }

   }

   /* If we are not compressing the data and the original data does
      not contain the magic number, just return. */
   *dest = NULL;
   return 0;

}

