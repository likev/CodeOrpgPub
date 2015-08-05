#include <bzlib.h>
#include <zlib.h>
#include <stdlib.h>
#include "asp_view.h"
#include "asp_view_lib.h"
#include "interface.h"
#include "support.h"

static FILE *in_fp = NULL;
static int Read_options(int argc, char **argv);
static void Dump_components( int num_comps, RPGP_product_t *prod, char ***myPacket , int *numPackets );
static unsigned int Get_uint_value( void *addr );
static void Process_product_data( char *buf, char ***myPacket, char **radarName, int *numPackets  );
static void* DSP_decompress_product( void *src );
static int Decompress_product( void *bufptr, char **dest );
static void Process_bzip2_error( int ret );
static void Process_zlib_error( int ret );
static int Malloc_dest_buf( int dest_len, char **dest_buf );
static int Dump_WMO_header( char *buf );
static int Unpack_value_from_ushorts( void *loc, void *value );
static void Msg_hdr_desc_blk_swap (void *mhb);

/*\//////////////////////////////////////////////////////////////////

   Description:
      Displays the status log product.

//////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){
    
    Read_options(argc, argv);
    GtkWidget *outer_window;
    gtk_set_locale();
    gtk_init(&argc, &argv);
    outer_window = create_outer_window();
    gtk_signal_connect(GTK_OBJECT(outer_window), "destroy", GTK_SIGNAL_FUNC(destroy), NULL);
    gtk_widget_show(outer_window);
    gtk_main();
    return 0;
}

GtkWidget* _create_filter_window(void)
{
    return create_filter_window();
}

static int Read_options(int argc, char **argv) 
{
    int arg = 0, c;
    extern char *optarg;
    extern int optind;
    int option_num = 0;
    while ((c = getopt(argc, argv, "fdh")) != EOF) {
        switch (c) {
            case 'f':
              option_num = 1;
              break;
            case 'd':
              option_num = 2;
              break;
            case 'h':
            default:
              printf("Usage: %s [options] [file_name]\n", argv[0]);
              printf("       Options:\n");
              printf("       -d path\n");
              printf("            Set the default path to look for ASP products.\n");
              printf("       -f filename\n");
              printf("            Pass in the location of an ASP file to view.\n");
              printf("\n\nIf no options or file is specified, then the tool will start up\n");
              printf("  and will use /import/orpg/ASP_Prod as the default location to look\n");
              printf("  for ASP Products.  The user will also still have the ability to enter\n");
              printf("  in file names for specific files into the utility.  No command line input\n");
              printf("  is required for use of this tool.  See \"man asp_view\"\n");
              printf("  for more information.\n");
              exit(0);
        }
    }

    if (optind == (argc - 1)) 
    {
        if (option_num == 1)
        {
            set_init_file(argv[optind]);
            set_dir_location("/import/orpg/ASP_Prod/\0");
        }
        else if (option_num == 2)
            set_dir_location(argv[optind]);
    }
    else
        set_dir_location("/import/orpg/ASP_Prod/\0");
    return arg;
}

int generate_packet(char ***myPacket, char **radarName, int *numPackets, char* filename)
{
   int ret, size;
   int Strip_WMO_product_header = 1;
   int Product_header_stripped = 1;

   char *buf = NULL, *status_product = NULL;

   if( strlen( filename ) > 0 ){

      status_product = (char *) malloc( MAX_PROD_SIZE );
      if( status_product == NULL ){

         fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
         exit(0);

      }

      /* This assumes that the Archive III status product is given on the 
         command line. */
      if( (in_fp = fopen( filename, "r" )) == NULL ){

         fprintf( stderr, "Couldn't open %s for read\n", filename );
         return 0;

      }

      /* Read entire status product. */
      size = MAX_PROD_SIZE;
      buf = status_product;

      if( (ret = fread( status_product, 1, size, in_fp )) <= 0 ){

         fprintf( stderr, "Read Error (%d) of Status Product data %s\n", ret, filename );
         fclose(in_fp);
         return 0;

      }

      if( Strip_WMO_product_header || !Product_header_stripped ){

         char *temp = (char *) malloc( MAX_PROD_SIZE );
         if( temp == NULL ){

            fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
            exit(0);

         }

         buf = status_product;
         size = ret;

         /* Account for various headers. */
         if( Strip_WMO_product_header ){

            int hdr_size = Dump_WMO_header( buf );

            if( hdr_size < 0 ){

               fprintf( stderr, "\n!!!!!!! Error Parsing WMO/AWIPS Header !!!!!!!\n" );
               /*exit(0);*/
               fclose(in_fp);
               return 0;

            }

            buf += hdr_size;
            size -= hdr_size;

         }

         if( !Product_header_stripped ){

            buf += sizeof(Prod_header);
            size -= sizeof(Prod_header);

         }
         
         /* Copy the product data to temp ..... discarding the
            WMO header and ORPG product header if needed. */
         memcpy( temp, buf, size );
         free( status_product );

         status_product = temp;

      }

      Process_product_data( status_product, myPacket, radarName, numPackets );
      free( status_product );
      fclose(in_fp);
      return 1;

   }

   return 0;
}

/*\////////////////////////////////////////////////////////////////////

   Description:
      Dumps the component information in Packet 28. 

   Inputs:
      num_comps - number of components.
      prod - pointer to RPGP_product_t structure. 

////////////////////////////////////////////////////////////////////\*/
static void Dump_components( int num_comps, RPGP_product_t *prod, char ***myPacket, int *numPackets ){

   int type, size, i, j;
   char *loc, *loc1;
   
   *myPacket = (char **)malloc(num_comps * sizeof(char **));
   *numPackets = num_comps;

   for( i = 0; i < num_comps; i++ ){
      type = *((int *) prod->components[i]);    
      if( type == RPGP_TEXT_COMP ){

         RPGP_text_t *comp = (RPGP_text_t *) prod->components[i];

         if( comp->numof_comp_params > 0 ){

            if( 1){

               /*fprintf( stdout, "%d,  ", i );*/

               for( j = 0; j < comp->numof_comp_params; j++ ){

                  size = strlen( comp->comp_params[j].attrs );
                  if( size > 128 )
                     size = 128;

                  loc = strstr( comp->comp_params[j].attrs, "Value=" );
                  if( loc != NULL ){

                     loc += strlen( "Value=" );
                     loc1 = strstr( loc, ";" );
                     if( loc1 != NULL ){

                        *loc1 = '\0';
                        /*fprintf( stdout,"%s,  ", loc ); */

                     }

                  }

               } /* End of for loop. */
                char *tempVar = malloc(strlen(comp->text) + strlen(loc) + 3);
                strcpy(tempVar, loc);
                strcat(tempVar, "||");
                strcat(tempVar, comp->text);
                (*myPacket)[i] = g_strdup(tempVar);
                free(tempVar);
     
            }
         }
               }
      else
         fprintf( stderr, "Unsupported Component Type\n" ); 

   }

} /* End of Dump_components. */

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Takes the address of an unsigned short pair, performs the necessary
      byte-swapping, then converts to unsigned int.

   Inputs:
      addr - address that stores the unsigned short pair.

   Returns:
      Unsigned int value from unsigned short pair.

/////////////////////////////////////////////////////////////////////////\*/
static unsigned int Get_uint_value( void *addr ){

   unsigned short *temp1, *temp2;
   unsigned int value;

   temp1 = (unsigned short *) addr;
   temp2 = temp1 + 1;

   *temp1 = SHORT_BSWAP_L( *temp1 );
   *temp2 = SHORT_BSWAP_L( *temp2 );

   Unpack_value_from_ushorts( (void *) temp1, &value );

   return value;

} /* End of Get_unit_value() */

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Driver for all status product processing.

   Inputs:
      buf - address that stores the product.

/////////////////////////////////////////////////////////////////////////\*/
static void Process_product_data( char *buf, char ***myPacket, char **radarName, int *numPackets ){

   int block_length, data_length, prod_id, compr;
   Graphic_product *phd_p = NULL;
   Symbology_block *sym_p = NULL;
   packet_28_t *packet28_p = NULL;
   RPGP_product_t *prod = NULL;
   char  *cpt = NULL, *temp = NULL;

   /* Set the product code. */
   prod_id = STATUS_PROD_CODE;

   /* Set pointer to beginning of product ... the ICD Product Header Block. */
   cpt = buf;

   /* Process Message Header and Product Description Block. */
   phd_p = (Graphic_product *) cpt;

   if( STATUS_PROD_CODE != (int) SHORT_BSWAP_L( phd_p->msg_code ) ){

      fprintf( stderr, "\n !!!!!!!! Product Code %d Unexpected (%d) !!!!!!!\n",
               SHORT_BSWAP_L( phd_p->msg_code ), STATUS_PROD_CODE );
      return;

   } 

   /* Check to see whether this product is compressed or not. */
   compr = (int) SHORT_BSWAP_L( phd_p->param_8 );
   if( compr < 0 ){

      fprintf( stderr, "\n !!!!!!! Compression Type %d (%d) Not Supported. !!!!!!! \n", 
               compr, phd_p->param_8 );
      fprintf( stderr, "--->Skipping Product.\n" );
      return;

   }

   /* Product is compressed.   Need to decompress. */
   if( compr > 0 ){

      if( (compr == COMPRESSION_BZIP2) || (compr == COMPRESSION_ZLIB) ){

         /*fprintf( stdout, "\n >>>>>>> Decompressing Status Product <<<<<<< \n\n" );*/
         temp = (char *) DSP_decompress_product( buf );
         /*fprintf( stdout, "\n >>>>>> Finished decompressing status product <<<<<<\n\n");*/
         if( temp == NULL ){

            fprintf( stderr, "\n !!!!!!! Product Decompression Failed !!!!!!! \n" );
            fprintf( stderr, "--->Skipping product.\n" );
            return;

         }

         cpt = buf = temp;

      }
      else{

         fprintf( stderr, "\n !!!!!!! Compression Type %d (%d) Not Supported. !!!!!!! \n",
                  compr, phd_p->param_8 );
         fprintf( stderr, "---> Skipping Product.\n" );
         return;

      }

   }

   cpt += sizeof(Graphic_product);
   sym_p = (Symbology_block *) cpt;
   block_length = Get_uint_value(&sym_p->data_len );

   cpt += sizeof(Symbology_block);
   packet28_p = (packet_28_t *) cpt;
   data_length = Get_uint_value( &packet28_p->num_bytes );
   cpt += sizeof(packet_28_t);
   if( RPGP_product_deserialize( cpt, data_length, (void *) &prod ) < 0 ){

      fprintf( stderr, "Could Not Deserialize Serialized Data\n" );

      if( temp != NULL )
         free( temp );

      return;

   }
 
   *radarName = (char *)malloc(strlen(prod->radar_name) * sizeof(char *));
   strcpy(*radarName, prod->radar_name);
   /* Process components. */
   if( prod->numof_components > 0 )
      Dump_components( prod->numof_components, prod, myPacket, numPackets );

   if( temp != NULL )
      free( temp );
}

/*\/////////////////////////////////////////////////////////////

   Description:
      Decompress a product pointed to by src.

   Inputs:
      src - pointer to algorithm output buffer.

   Returns:

/////////////////////////////////////////////////////////////\*/
static void* DSP_decompress_product( void *src ){

   int status;
   char *dest = NULL;

   /* Call the RPG library routine for decompression. */
   status = Decompress_product( src, &dest );
   if( status < 0 )
      return(NULL);

   return( (void *) dest );

/* DSP_decompress_product() */
}
 
/*\///////////////////////////////////////////////////////////////

   Description:
      The buffer pointer to by "bufptr" is decompressed and placed
      in buffer "dest".  The size of the decompressed product is
      stored in "size".

   Inputs:
      bufptr - pointer to product buffer containing compressed 
               data.
      dest - pointer pointer to receiving buffer.  

   Outputs:
      dest - receiving buffer holding decompressed product.
      size - size of the decompressed product.
      status - -1 on error, or 0 on success.
   
   Returns: 
      There is no return value defined for this function.

/////////////////////////////////////////////////////////////\*/
static int Decompress_product( void *bufptr, char **dest ){

   int ret;
   unsigned int length, dest_len, src_len;
   unsigned short alg;
   unsigned long long_dest_len, long_src_len;

   char *prod_data = NULL;
   Graphic_product *phd = NULL;

   phd = (Graphic_product *) ((char *) bufptr);
#ifdef LITTLE_ENDIAN_MACHINE
         /* Product in network byte order ... convert to native format. */
         Msg_hdr_desc_blk_swap( phd );
#endif

   prod_data = (char *) (((char *) bufptr) + sizeof(Graphic_product));
 
   /* Find the original and compressed size of the product. */
   Unpack_value_from_ushorts( &phd->msg_len, &src_len );
   Unpack_value_from_ushorts( &phd->param_9, &dest_len );

   /* Since the product header and description blocks are not compressed, 
      account for them here. */
   dest_len += sizeof(Graphic_product);
   src_len -= sizeof(Graphic_product);

   /* Check the destination buffer.  If not allocated, allocate 
      the same size as the original (i.e., uncompressed) data. */
   if( (ret = Malloc_dest_buf( dest_len, dest )) < 0 )
      return( ret );

   /* Get the algorithm used for compression so we know how to decompress. */
   alg = (unsigned short) phd->param_8;
   /* Do the decompression. */
   switch( alg ){

      case COMPRESSION_BZIP2:
      {
         /* Do the bzip2 decompression. */
         ret = BZ2_bzBuffToBuffDecompress( *dest + sizeof(Graphic_product),
                                           &dest_len, prod_data, (unsigned int) src_len,
                                           RPG_BZIP2_NOT_SMALL, RPG_BZIP2_NOT_VERBOSE );

         /* Process Non-Normal return. */
         if( ret != BZ_OK ){

            Process_bzip2_error( ret );

            /* Free the destination buffer. */
            free(*dest);
            *dest = NULL;

            return( -1 );

         }
         else
            break;

      }

      case COMPRESSION_ZLIB:
      {

         fprintf(stdout, "** Using zlib compression\n");
         long_dest_len = (unsigned long) dest_len;
         long_src_len = (unsigned long) src_len;

         /* Do the zlib decompression. 
         ret = uncompress( *dest + sizeof(Graphic_product),*/
         ret = uncompress((Bytef *) *dest + sizeof(Graphic_product),
                           &long_dest_len, (Bytef *)prod_data, long_src_len );

         /* Process Non-Normal return. */
         if( ret != Z_OK ){

            Process_zlib_error( ret );

            /* Free the destination buffer. */
            free(*dest);
            *dest = NULL;

            return( -1 );

         }
         else
            break;

      }

      default:
      {
         fprintf( stderr, "Decompression Method Not Supported (%x)\n", alg );
      }
      case COMPRESSION_NONE:
      {
         fprintf(stdout, "** No compression\n");

         dest_len -= sizeof(Graphic_product);
         src_len += sizeof(Graphic_product);

         /* Just copy the source to the destination. */
         memcpy( (void *) *dest, bufptr, dest_len );

         /* Store the compression type in product dependent parameter 8. */ 
         phd = (Graphic_product *) ((char *) (*dest));

         phd->param_8 = (unsigned short) COMPRESSION_NONE;
         phd->param_9 = 0;
         phd->param_10 = 0;

#ifdef LITTLE_ENDIAN_MACHINE
         /* Convert back to network byte order. */
         Msg_hdr_desc_blk_swap( phd );
#endif
         return( 0 );

      }

   /* End of "switch" statement. */
   }

   /* Copy the product header and description block. */ 
   memcpy( (void*) ((char *)(*dest)), (void*) phd,
           sizeof(Graphic_product) );

   /* Set the uncompressed length of product in the product header. */
   phd = (Graphic_product *) ((char *) (*dest));
#ifdef LITTLE_ENDIAN_MACHINE
   /* Product in network byte order ... convert to native format. */
   Msg_hdr_desc_blk_swap( phd );
#endif
   length = dest_len + sizeof(Graphic_product);
   Unpack_value_from_ushorts( (void *) &phd->msg_len, (void *) &length );

   /* Store the compression type in product dependent parameter 8. */ 
   phd->param_8 = (unsigned short) COMPRESSION_NONE;
   phd->param_9 = 0;
   phd->param_10 = 0;

#ifdef LITTLE_ENDIAN_MACHINE
   /* Convert back to network byte order. */
   Msg_hdr_desc_blk_swap( phd );
#endif

   return( 0 );

/* Decompress_final_product() */
}

/*\/////////////////////////////////////////////////////////////

   Description:
      Writes error message to task log file.  Error message is
      based on "ret" value.
      
/////////////////////////////////////////////////////////////\*/
static void Process_bzip2_error( int ret ){

   switch( ret ){

      case BZ_CONFIG_ERROR:
         fprintf( stderr, "BZIP2 Configuration Error\n" );
         break;

      case BZ_PARAM_ERROR:
         fprintf( stderr, "BZIP2 Parameter Error\n" );
         break;

      case BZ_MEM_ERROR:
         fprintf( stderr, "BZIP2 Memory Error\n" );
         break;

      case BZ_OUTBUFF_FULL:
         fprintf( stderr, "BZIP2 Outbuf Full Error\n" );
         break;

      case BZ_DATA_ERROR:
         fprintf( stderr, "BZIP2 Data Error\n" );
         break;

      case BZ_DATA_ERROR_MAGIC:
         fprintf( stderr, "BZIP2 Magic Data Error\n" );
         break;

      case BZ_UNEXPECTED_EOF:
         fprintf( stderr, "BZIP2 Unexpected EOF Error\n" );
         break;

      default:
         fprintf( stderr, "Unknown BZIP2 Error (%d)\n", ret );
         break;

   /* End of "switch" statement. */
   }

/* End of Process_bzip2_error() */
}

/*\/////////////////////////////////////////////////////////////

   Description:
      Writes error message to task log file.  Error message is
      based on "ret" value.
      
/////////////////////////////////////////////////////////////\*/
static void Process_zlib_error( int ret ){

   switch( ret ){

      case Z_MEM_ERROR:
         fprintf( stderr, "ZLIB Memory Error\n" );
         break;

      case Z_BUF_ERROR:
         fprintf( stderr, "ZLIB Buffer Error\n" );
         break;

      default:
         fprintf( stderr, "Unknown ZLIB Error (%d)\n", ret );
         break;

   /* End of "switch" statement. */
   }

/* End of Process_zlib_error() */
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

//////////////////////////////////////////////////////////////////////\*/
static int Malloc_dest_buf( int dest_len, char **dest_buf ){

   /* Allocate an output buffer the same size as the original 
      (i.e., uncompressed) data. */
   *dest_buf = malloc( dest_len );
   if( *dest_buf == NULL ){

      fprintf( stderr, "malloc Failed For %d Bytes\n", dest_len );
      return( -1 );

   }

   return( 0 );

/* End of Malloc_dest_buf() */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Dump the WMO header contents.

/////////////////////////////////////////////////////////////////////\*/
static int Dump_WMO_header( char *buf ){

   WMO_header_t *wmo_hdr = (WMO_header_t *) buf;
   AWIPS_header_t *awips_hdr;
   char cpt[8], *temp = NULL;
   int size = 0;
   char CRCRLF[4] = { 0x0d, 0x0d, 0x0a, 0x00 };
   
 /*  fprintf( stdout, "\n-------------WMO Header-------------\n");*/

   /* Write out WMO Header fields. */
   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->form_type, sizeof(wmo_hdr->form_type) );
   /*fprintf( stdout, "--->Form Type:         %s\n", cpt );*/
   size += sizeof(wmo_hdr->form_type);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->data_type, sizeof(wmo_hdr->data_type) );
   /*fprintf( stdout, "--->Data Type:         %s\n", cpt );*/
   size += sizeof(wmo_hdr->data_type);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->distribution, sizeof(wmo_hdr->distribution) );
   /*fprintf( stdout, "--->Distribution:      %s\n", cpt );*/
   size += sizeof(wmo_hdr->distribution);

   size += sizeof(wmo_hdr->space1);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->originator, sizeof(wmo_hdr->originator) );
   /*fprintf( stdout, "--->Originator:        %s\n", cpt );*/
   size += sizeof(wmo_hdr->originator);

   size += sizeof(wmo_hdr->space2);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->date_time, sizeof(wmo_hdr->date_time) );
   /*fprintf( stdout, "--->Date/Time:         %s\n", cpt );*/
   size += sizeof(wmo_hdr->date_time);

   temp = strstr( (char *) &wmo_hdr->extra, CRCRLF  );
   if( temp == NULL )
      return(-1);

   /* Account for the trailing CR/CR/LF. */
   size += ((temp - &wmo_hdr->extra) + strlen( CRCRLF ));

   /*fprintf( stdout, "\n-------------AWIPS Header-------------\n");*/

   /* Write out AWIPS Header fields. */
   awips_hdr = (AWIPS_header_t *) (temp + strlen( CRCRLF ));

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, awips_hdr->category, sizeof(awips_hdr->category) );
   /*fprintf( stdout, "--->Category:         %s\n", cpt );*/
   size += sizeof(awips_hdr->category);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, awips_hdr->product, sizeof(awips_hdr->product) );
   /*fprintf( stdout, "--->Product:          %s\n", cpt );*/
   size += sizeof(awips_hdr->product);

   /* Account for the trailing CR/CR/LF. */
   size += strlen( CRCRLF );
   
   return( size );

/* End of Dump_WMO_header() */
}

/*\//////////////////////////////////////////////////////////////

   Description:
      Unpacks the data value @ loc.  The unpacked value will be
      stored at "value".

      The Most Significant 2 bytes (MSW) of the packed value are
      stored at the byte addressed by "loc", the Least Significant
      2 bytes (LSW) are stored at 2 bytes past "loc".

      By definition:

         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff )

   Input:
      loc - starting address where packed value is stored.
      value - address to received the packed value.
 
   Output:
      value - holds the unpacked value.

   Returns:
      Always returns 0.

   Notes:

//////////////////////////////////////////////////////////////\*/
static int Unpack_value_from_ushorts( void *loc, void *value ){

   unsigned int *fw_value = (unsigned int *) value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   *fw_value =
      (unsigned int) (0xffff0000 & ((*msw) << 16)) | ((*lsw) & 0xffff);

   return 0;

/* End of Unpack_value_from_ushorts() */
}

/*\/////////////////////////////////////////////////////////////////////////

    Description: This function performs the byte swap for the message header
                block and product description block. It can be used for
                converting to and from the ICD format.

    Inputs:     prod - pointer to the message header block.

/////////////////////////////////////////////////////////////////////////\*/
static void Msg_hdr_desc_blk_swap (void *mhb){

#ifdef LITTLE_ENDIAN_MACHINE
    int i;
    unsigned short *spt;

    spt = (unsigned short *) mhb;
    for (i = 0; i < MSG_PRODUCT_LEN; i++) {
        *spt = SHORT_BSWAP (*spt);
        spt++;
    }

#endif

    return;
}
