#include <stdlib.h>
#include <orpg.h>
#include <prod_gen_msg.h>
#include <packet_28.h>
#include <orpg_product.h>
#include <bzlib.h>
#include <zlib.h>
#include <vp_def.h>
#include <stdio.h>

#define DETAILED_OUTPUT		 1

/* General compression macro definitions. */
#define MIN_BYTES_TO_COMPRESS    1000

/* Macro defintions used by the bzip2 compressor. */
#define RPG_BZIP2_MIN_BLOCK_SIZE_BYTES   100000  /* corresponds to 100 Kbytes */
#define RPG_BZIP2_MIN_BLOCK_SIZE              1  /* corresponds to 100 Kbytes */
#define RPG_BZIP2_MAX_BLOCK_SIZE              9  /* corresponds to 900 Kbytes */
#define RPG_BZIP2_WORK_FACTOR                30  /* the recommended default */
#define RPG_BZIP2_NOT_VERBOSE                 0  /* turns off verbosity */
#define RPG_BZIP2_NOT_SMALL                   0  /* does not use small version */

#define MAX_PROD_SIZE                    2000000
#define FILE_NAME_SIZE                      128
#define MSG_PRODUCT_LEN                      60
#define WMO_HEADER_SIZE                      30
#define LOCAL_NAME_SIZE			    200
#define MATCH_STR_SIZE			     32

/* Message Code Macros */
#define MSG_CODE_STORM_STRUCTURE      62
#define MSG_CODE_RCM                  74
#define MSG_CODE_SUPP_PRECIP_DATA     82
#define MSG_CODE_US_LAYER_CR         137

/* For writing in color. */
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31;1m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34;1m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"


typedef struct wmo_header {

   char form_type[2];
   char data_type[2];
   char distribution[2];
   char space1;
   char originator[4];
   char space2;
   char date_time[6];
   char extra;

} WMO_header_t;

typedef struct awips_header {

   char category[3];
   char product[3];
   char eoh[2];

} AWIPS_header_t;

/* Global variables. */
static int Prod_code = 0;
static FILE *in_fp = NULL;
static int Verbose = 0;
static int Product_header_stripped = 0;
static int Strip_WMO_product_header = 0;
static int N_files = 0;
static int Str_match = 0;
static char File_name[FILE_NAME_SIZE];
static char Dir_name[LOCAL_NAME_SIZE];
static char Match_str[MATCH_STR_SIZE];
static Ap_vol_file_t *Vol_files;
static int Sym_off = 0;
static int Gra_off = 0;
static int Tab_off = 0;

#define NUM_COMPRESSED		24	
static int Compressed_prods[NUM_COMPRESSED] = { 32, 94, 99, 134, 135, 136,
                                                138, 149, 152, 153, 154, 155,
                                                159, 161, 163, 165, 170, 172,
                                                173, 174, 175, 176, 177, 195 };

/* Function Prototypes. */
static int Read_options (int argc, char **argv);
static void Dump_header_description_block( Graphic_product *phd );
static void Dump_symbology_block_header( Symbology_block *sym );
static void Dump_graphic_block_header( Graphic_alpha_block *gra );
static void Dump_packet28_header( packet_28_t *p28, int *data_len );
static void Dump_generic_prod_data( RPGP_product_t *prod );
static void Dump_components( int num_comps, RPGP_product_t *prod );
static unsigned int Get_uint_value( void *addr );
static void Process_product_data( char *buf );
static void* DSP_decompress_product( void *src );
static int Decompress_product( void *bufptr, char **dest );
static void Process_bzip2_error( int ret );
static void Process_zlib_error( int ret );
static int Malloc_dest_buf( int dest_len, char **dest_buf );
static int Dump_WMO_header( char *buf );
static int Unpack_value_from_ushorts( void *loc, void *value );
static void Msg_hdr_desc_blk_swap (void *mhb);
static int Julian_to_date( int julian_date, int *year, int *month,
                           int *day );
static int Convert_time( unsigned int time, int *hour, int *minute, 
                         int *second );
static int Process_display_data_packets( unsigned short *block, int tlen );
static int Validate_display_data_packets( unsigned short *block, int tlen );
static void Test_product( char *product, int size_read );

/*\//////////////////////////////////////////////////////////////////

   Description:
      Displays and Validates product files.

//////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int i, ret, size, size_read = 0, tested_file = 0, command_line_args;
   char *buf = NULL, *product = NULL;
   char *temp = NULL;

   command_line_args = Read_options( argc, argv );

   /* A file name was not specified ..... process the directory. */
   if( strlen( File_name ) ==  0 ){

      /* If directory name not specified, assume current directory. */
      if( strlen( Dir_name ) == 0 )
         memcpy( Dir_name, "./", 2 );

      /* Get information about all the files in the directory. */
      N_files = DSPAUX_search_files( Dir_name, &Vol_files );

      /* Do For All Product Files files. */
      for( i = 0; i < N_files; i++ ){

         product = (char *) malloc( MAX_PROD_SIZE );
         if( product == NULL ){

            fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
            exit(0);

         }

         /* Open the file for reading. */
         if( (in_fp = fopen( Vol_files[i].path, "r" )) == NULL ){

            fprintf( stderr, "Couldn't open %s for read\n", Vol_files[i].path );
            return 0;

         }

         /* Read entire product. */
         size = MAX_PROD_SIZE;
         buf = product;

         if( (ret = fread( product, 1, size, in_fp )) <= 0 ){

            fprintf( stderr, "Read Error (%d) of Product data %s\n", 
                     ret, Vol_files[i].path );
            return 0;

         }

         /* Display the file name. */
         fprintf( stderr, "\n================================================================================\n" );
         fprintf( stderr, KGRN " File: %s\n" RESET, Vol_files[i].path );
         fprintf( stderr, "=================================================================================\n" );

         /* Check if product has WMO or ORPG header. */
         tested_file = 0;
         size_read = ret;
         if( !tested_file ){

            Test_product( product, size_read );
            tested_file = 1;

         }

         /* Process according to any command line arguments that may have been
            specified. */
         if( Strip_WMO_product_header || !Product_header_stripped ){

            temp = (char *) malloc( MAX_PROD_SIZE );
            if( temp == NULL ){

               fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
               exit(0);

            }

            buf = product;
            size = ret;

            /* Account for various headers. */
            if( Strip_WMO_product_header ){

               int hdr_size = Dump_WMO_header( buf );

               if( hdr_size < 0 ){

                  fprintf( stderr, "\n!!!!!!! Error Parsing WMO/AWIPS Header !!!!!!!\n" );
                  exit(0);

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
            free( product );

            product = temp;

         }

         /* Process this product. */
         Process_product_data( product );
         free( product );
         fclose( in_fp );

      }

      return 0;

   }

   /* A file name has been specified. */
   if( strlen( File_name ) > 0 ){

      product = (char *) malloc( MAX_PROD_SIZE );
      if( product == NULL ){

         fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
         exit(0);

      }

      /* This assumes that the product is given on the command line. */
      fprintf( stderr, "\n================================================================================\n" );
      fprintf( stderr, KGRN "File: %s\n" RESET, File_name );
      fprintf( stderr, "=================================================================================\n" );
      if( (in_fp = fopen( File_name, "r" )) == NULL ){

         fprintf( stderr, "Couldn't open %s for read\n", File_name );
         return 0;

      }

      /* Read entire status product. */
      size = MAX_PROD_SIZE;
      buf = product;

      if( (ret = fread( product, 1, size, in_fp )) <= 0 ){

         fprintf( stderr, "Read Error (%d) of Status Product data %s\n", ret, File_name );
         return 0;

      }

      /* Check if product has WMO or ORPG header. */
      size_read = ret;
      Test_product( product, size_read );

      if( Strip_WMO_product_header || !Product_header_stripped ){

         char *temp = (char *) malloc( MAX_PROD_SIZE );
         if( temp == NULL ){

            fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
            exit(0);

         }

         buf = product;
         size = ret;

         /* Account for various headers. */
         if( Strip_WMO_product_header ){

            int hdr_size = Dump_WMO_header( buf );

            if( hdr_size < 0 ){

               fprintf( stderr, "\n!!!!!!! Error Parsing WMO/AWIPS Header !!!!!!!\n" );
               exit(0);

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
         free( product );

         product = temp;

      }

      Process_product_data( product );
      free( product );

      return 0;

   }

   return 0;

}

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Checks the past product for various headers and determines in the 
      appropriate flags were set.

   Input:
      product - address of product file data.
      size_read - size, in bytes, of product file data.
   
////////////////////////////////////////////////////////////////////////\*/
static void Test_product( char *product, int size_read ){

   Graphic_product *phd = NULL;
   char response;

   /* Test if product has an ORPG header. */
   if( size_read >= (sizeof(Prod_header) + sizeof(Graphic_product)) ){

      phd = (Graphic_product *) (product + sizeof(Prod_header));
      if( (phd->divider == -1) 
                  &&
          (phd->msg_code == phd->prod_code) ){

         if( Product_header_stripped ){

            fprintf( stderr, KRED "\nProduct appears to contain an ORPG Header but the \"-s\" option was not specified.\n" );
            fprintf( stderr, "Do you want to enable the \"-s\" switch? (y/n)\n" RESET );
            scanf( "%c", &response );
            if( (response == 'y') || response == 'Y' ){

               Product_header_stripped = 0;
               return;

            }

         }

      }
      else if( strncmp( product, "SDUS", 4 ) != 0 ){

         if( !Product_header_stripped ){

            fprintf( stderr, KRED "\nProduct appears to not contain an ORPG Header but the \"-s\" option was specified.\n" );
            fprintf( stderr, "Do you want to disable the \"-s\" switch? (y/n)\n" RESET );
            scanf( "%c", &response );
            if( (response == 'y') || response == 'Y' ){

               Product_header_stripped = 1;
               return;

            }

         }

      }

      /* Check if it has a WMO/AWIPS header. */
      if( !Strip_WMO_product_header && (strncmp( product, "SDUS", 4 ) == 0) ){

         fprintf( stderr, KRED "Product appears to contain a WMO header but the \"-w\" option was not specified.\n" );
         fprintf( stderr, "Do you want to enable the \"-w\" switch? (y/n)\n" RESET );
         scanf( "%c", &response );
         if( (response == 'y') || response == 'Y' ){

            Strip_WMO_product_header = 1;
            return;

         }

      }
      
   }

   return;

}

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Dumps information from product header and description blocks.

   Inputs:
      phd - pointer the Graphic_product structure.

////////////////////////////////////////////////////////////////////////\*/
static void Dump_header_description_block( Graphic_product *phd ){

   int cdate, year, month, day;
   int ctime, hour, minute, second;
   int vol_num, op_mode, height;
   int divider;

   fprintf( stdout, KBLU "\n----------Message Header-------------------------\n" RESET );
   fprintf( stdout, "--->msg_code:   %d\n", SHORT_BSWAP_L( phd->msg_code) );
   cdate = SHORT_BSWAP_L( phd->msg_date );
   Julian_to_date( cdate, &year, &month, &day );
   if( year > 2000 )
      year -= 2000;
   else if( year > 1900 )
      year -= 1900;
   fprintf( stdout, "--->msg_date:   %d (%02d/%02d/%02d)\n", cdate, month, day, year);
   ctime = Get_uint_value( &phd->msg_time );
   Convert_time( ctime, &hour, &minute, &second );
   fprintf( stdout, "--->msg_time:   %d (%02d:%02d:%02d)\n", ctime, hour, minute, second );
   fprintf( stdout, "--->msg_len:    %d\n", Get_uint_value( &phd->msg_len ) );
   fprintf( stdout, "--->n_blocks:   %d\n\n", SHORT_BSWAP_L( phd->n_blocks ) );

   fprintf( stdout, KBLU "----------Product Description Block----------\n" RESET );
   divider = (short) SHORT_BSWAP_L( phd->divider );
   if( divider == -1 )
      fprintf( stdout, "--->divider:    %d\n", divider );
   else
      fprintf( stdout, KRED "--->divider:    %d INVALID (expecting -1)\n" RESET, divider );
   fprintf( stdout, "--->latitude:   %d\n", Get_uint_value( &phd->latitude ) );
   fprintf( stdout, "--->longitude:  %d\n", Get_uint_value( &phd->longitude ) );

   /* Validate height. */
   height = (short) SHORT_BSWAP_L( phd->height );
   if( (height >= -100) && (height <= 11000) )
      fprintf( stdout, "--->height:     %d\n", height );
   else
      fprintf( stdout, KRED "--->height:     %d INVALID (-100 <= height <= 10000 ft MSL)\n" RESET, height );

   Prod_code = SHORT_BSWAP_L( phd->prod_code );
   fprintf( stdout, "--->code:       %d\n", Prod_code );

   /* Validate op mode. */
   op_mode = (int) SHORT_BSWAP_L( phd->op_mode );
   if( (op_mode >= 0) && (op_mode <= 2) )
      fprintf( stdout, "--->op_mode:    %d\n", op_mode );
   else
      fprintf( stdout, KRED "--->op_mode:    %d INVALID: (0 <= op_mode<= 2)\n" RESET, op_mode );

   fprintf( stdout, "--->vcp_num:    %d\n", SHORT_BSWAP_L( phd->vcp_num ) );
   fprintf( stdout, "--->seq_num:    %d\n", SHORT_BSWAP_L( phd->seq_num ) );

   /* Validate volume number. */
   vol_num = SHORT_BSWAP_L( phd->vol_num );
   if( (vol_num >= 1) && (vol_num <= 80) )
      fprintf( stdout, "--->vol_num:    %d\n", vol_num );
   else
      fprintf( stdout, KRED "--->vol_num:    %d INVALID: (1 <= vol_num <= 80)\n" RESET, vol_num );

   /* Convert Volume start date. */
   cdate = SHORT_BSWAP_L( phd->vol_date );
   Julian_to_date( cdate, &year, &month, &day );
   if( year > 2000 )
      year -= 2000;
   else if( year > 1900 )
      year -= 1900;
   fprintf( stdout, "--->vol_date:   %d (%02d/%02d/%02d)\n", cdate, month, day, year);

   /* Convert Volume start time. */
   ctime = Get_uint_value( &phd->vol_time_ms );
   Convert_time( ctime, &hour, &minute, &second );
   fprintf( stdout, "--->vol_time:   %d (%02d:%02d:%02d)\n", ctime, hour, minute, second );

   /* Convert product generation date. */
   cdate = SHORT_BSWAP_L( phd->gen_date );
   Julian_to_date( cdate, &year, &month, &day );
   if( year > 2000 )
      year -= 2000;
   else if( year > 1900 )
      year -= 1900;
   fprintf( stdout, "--->gen_date:   %d (%02d/%02d/%02d)\n", cdate, month, day, year);

   /* Convert product generation time. */
   ctime = Get_uint_value( &phd->gen_time );
   Convert_time( ctime, &hour, &minute, &second );
   fprintf( stdout, "--->gen_time:   %d (%02d:%02d:%02d)\n", ctime, hour, minute, second );

   fprintf( stdout, "--->param_1:    %d\n", SHORT_BSWAP_L( phd->param_1 ) );
   fprintf( stdout, "--->param_2:    %d\n", SHORT_BSWAP_L( phd->param_2 ) );
   fprintf( stdout, "--->elev_ind:   %d\n", SHORT_BSWAP_L( phd->elev_ind) );
   fprintf( stdout, "--->param_3:    %d\n", SHORT_BSWAP_L( phd->param_3) );
   fprintf( stdout, "--->param_4:    %d\n", SHORT_BSWAP_L( phd->param_4 ) );
   fprintf( stdout, "--->param_5:    %d\n", SHORT_BSWAP_L( phd->param_5 ) );
   fprintf( stdout, "--->param_6:    %d\n", SHORT_BSWAP_L( phd->param_6 ) );
   fprintf( stdout, "--->param_7:    %d\n", SHORT_BSWAP_L( phd->param_7 ) );
   fprintf( stdout, "--->param_8:    %d\n", SHORT_BSWAP_L( phd->param_8 ) );
   fprintf( stdout, "--->param_9:    %d\n", SHORT_BSWAP_L( phd->param_9 ) );
   fprintf( stdout, "--->param_10:   %d\n", SHORT_BSWAP_L( phd->param_10 ) );

   /* Get block offsets. */
   Sym_off = Get_uint_value( &phd->sym_off );
   Gra_off = Get_uint_value( &phd->gra_off );
   Tab_off = Get_uint_value( &phd->tab_off );
   fprintf( stdout, "--->sym_off:    %d\n", Sym_off );
   fprintf( stdout, "--->gra_off:    %d\n", Gra_off );
   fprintf( stdout, "--->tab_off:    %d\n\n", Tab_off );

} /* End of Dump_header_description_block() */

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Dumps symbology block header information.

   Inputs:
      sym - pointer to symbology block.

//////////////////////////////////////////////////////////////////////\*/
static void Dump_symbology_block_header( Symbology_block *sym ){

   int i, divider, block_id, block_length, data_len, n_layers;
   unsigned short *sym_data = NULL;


   fprintf( stdout, KBLU "----------Symbology Block Header----------\n" RESET );

   /* Validate divider. */
   divider = (short) SHORT_BSWAP_L( sym->divider );
   if( divider == -1 )
      fprintf( stdout, "--->divider:    %d\n", divider);
   else
      fprintf( stdout, KRED "--->divider:    %d INVALID (expecting -1)\n" RESET, divider );

   /* Validate block ID. */
   block_id = (short) SHORT_BSWAP_L( sym->block_id );
   if( block_id == 1 )
      fprintf( stdout, "--->block_id:   %d\n", block_id);
   else
      fprintf( stdout, KRED "--->block_id:   %d INVALID (expecting 1)\n" RESET, block_id );

   /* Validate the block length. */
   block_length = Get_uint_value( &sym->block_len );
   if( (block_length > 0 ) && (block_length <= 2000000) )
      fprintf( stdout, "--->block_len:  %d\n", block_length );
   else
      fprintf( stderr, KRED "--->block_len:  %d INVALID (1 <= block_len <= 2000000)\n" RESET,  
               block_length );

   /* Validate the number of layers. */
   n_layers = SHORT_BSWAP_L( sym->n_layers );
   if( (n_layers >= 1) && (n_layers <= 18) )
      fprintf( stdout, "--->n_layers:   %d\n", n_layers );
   else
      fprintf( stdout, KRED "--->n_layers:   %d INVALID (1 <= n_layers <= 18)\n" RESET, n_layers );

   /* Validate layer divider. */
   divider = (short) SHORT_BSWAP_L( sym->layer_divider );
   if( divider == -1 )
      fprintf( stdout, "--->divider:    %d\n", divider);
   else
      fprintf( stdout, KRED "--->divider:    %d INVALID (expecting -1)\n" RESET, divider );

   /* Get the address of the first data layer. */
   sym_data = (unsigned short *) sym + 5;

   /* Do for Each Layer of the Product. */
   for( i = 0; i < n_layers; i++ ){

      fprintf( stdout, KBLU "\n----------Symbology Block (Layer %d)----------\n" RESET, i+1 );

      /* Get the data length. */
      data_len = Get_uint_value( sym_data + 1 );

      /* Validate the length of data layer. */
      if( (data_len >= 1) && (data_len <= 2000000) )
         fprintf( stdout, "--->data_len:   %d\n\n", data_len );
      else 
         fprintf( stdout, KRED "--->data_len:   %d INVALID (1 <= data_len <= 2000000)\n\n" RESET, data_len );

      /* Byte-swap the Symbology Block data. */
      MISC_short_swap( sym_data + 3, ((data_len + 1)/ sizeof(unsigned short)) );

      /* Process the individual packets (may need additional byte-swapping). */
      Process_display_data_packets( sym_data + 3, data_len );

      /* Validate the layer packets. */
      Validate_display_data_packets( sym_data + 3, data_len );

      /* Go to next data layer. */
      sym_data += (3 + (data_len + 1)/2);

   }

} /* End of Dump_symbology_block_header() */


/*\//////////////////////////////////////////////////////////////////////

   Description:
      Dumps graphic block header information.

   Inputs:
      gra - pointer to graphic alphanumeric block.

//////////////////////////////////////////////////////////////////////\*/
static void Dump_graphic_block_header( Graphic_alpha_block *gra ){

   int i, divider, block_id, block_length, page_len, n_pages;
   unsigned short *gra_data = NULL;


   fprintf( stdout, KBLU "----------Graphic Alphanumeric Block Header----------\n" RESET );

   /* Validate divider. */
   divider = (short) SHORT_BSWAP_L( gra->divider );
   if( divider == -1 )
      fprintf( stdout, "--->divider:    %d\n", divider);
   else
      fprintf( stdout, "--->divider:    %d INVALID (expecting -1)\n", divider );

   /* Validate block ID. */
   block_id = (short) SHORT_BSWAP_L( gra->block_id );
   if( block_id == 2 )
      fprintf( stdout, "--->block_id:   %d\n", block_id);
   else
      fprintf( stdout, "--->block_id:   %d INVALID (expecting 2)\n", block_id );

   /* Validate the block length. */
   block_length = Get_uint_value( &gra->block_len );
   if( (block_length > 0 ) && (block_length <= 65535) )
      fprintf( stdout, "--->block_len:  %d\n", block_length );
   else
      fprintf( stdout, "--->block_len:  %d INVALID (1 <= block_len <= 65535)\n", block_length );

   /* Validate the number of pages. */
   n_pages = SHORT_BSWAP_L( gra->n_pages );
   if( (n_pages >= 1) && (n_pages <= 48) )
      fprintf( stdout, "--->n_pages:   %d\n", n_pages );
   else
      fprintf( stdout, "--->n_pages:   %d INVALID (1 <= n_pages <= 48)\n", n_pages );

   /* Byte-swap the Graphic Alphanumeric Block data. */
   MISC_short_swap( gra, ((block_length + 1)/sizeof(unsigned short)) );

   /* Get the address of the first page. */
   gra_data = (unsigned short *) gra + 5;

   /* Do for Each Page of the Product. */
   for( i = 0; i < n_pages; i++ ){

      int page_num = (short) SHORT_BSWAP_L( *gra_data );

      fprintf( stdout, KBLU "\n----------Graphic Block (Page %d)----------\n" RESET , page_num );

      /* Get the page length. */
      page_len = (short) *(gra_data + 1);

      /* Process the individual packets (may need additional byte-swapping). */
      Process_display_data_packets( gra_data + 2, page_len );

      /* Validate the page packets. */
      Validate_display_data_packets( gra_data + 2, page_len );

      /* Go to next page. */
      gra_data += (2 + (page_len + 1)/2);

   }

} /* End of Dump_graphic_block_header() */


/*\//////////////////////////////////////////////////////////////////////

   Description:
      Dumps tabular alphanumeric block header information.

   Inputs:
      tab - pointer to tabular alphanumeric block.

//////////////////////////////////////////////////////////////////////\*/
static void Dump_tabular_block_header( Tabular_alpha_block *tab ){

   int i, divider, block_id, block_length, n_pages;
   unsigned short *tab_data = NULL;
   char str[120];
   int cnt = 0;

   fprintf( stdout, KBLU "----------Tabular Alphanumeric Block Header----------\n" RESET );

   /* Validate divider. */
   divider = (short) SHORT_BSWAP_L( tab->divider );
   if( divider == -1 )
      fprintf( stdout, "--->divider:    %d\n", divider);
   else
      fprintf( stdout, "--->divider:    %d INVALID (expecting -1)\n", divider );

   /* Validate block ID. */
   block_id = (short) SHORT_BSWAP_L( tab->block_id );
   if( block_id == 3 )
      fprintf( stdout, "--->block_id:   %d\n", block_id);
   else
      fprintf( stdout, "--->block_id:   %d INVALID (expecting 3)\n", block_id );

   /* Validate the block length. */
   block_length = Get_uint_value( &tab->block_len );
   if( (block_length > 0 ) && (block_length <= 65535) )
      fprintf( stdout, "--->block_len:  %d\n", block_length );
   else
      fprintf( stdout, "--->block_len:  %d INVALID (1 <= block_len <= 65535)\n", block_length );

   /* Set the pointer to the start of the tabular data.  That is, skip the second product
       header and product description block. */
   tab_data = (unsigned short *) tab;
   tab_data += (sizeof(Tabular_alpha_block) + sizeof(Graphic_product))/sizeof(short);

   /* Validate divider. */
   divider = (short) SHORT_BSWAP_L( *tab_data );
   if( divider == -1 )
      fprintf( stdout, "--->divider:    %d\n", divider);
   else
      fprintf( stdout, "--->divider:    %d INVALID (expecting -1)\n", divider );

   /* Validate the number of pages. */
   n_pages = SHORT_BSWAP_L( *(tab_data + 1) );
   if( (n_pages >= 1) && (n_pages <= 48) )
      fprintf( stdout, "--->n_pages:    %d\n", n_pages );
   else
      fprintf( stdout, "--->n_pages:    %d INVALID (1 <= n_pages <= 48)\n", n_pages );

   /* Increment to the start of the page. */
   tab_data += 2;

   /* Clear the string that is to be used for displaying the tabular data. */
   memset( str, 0, 120 );

   /* Do for Each Page of the Product. */
   for( i = 0; i < n_pages; i++ ){

      int len, k;
      short value;

      fprintf( stdout, KBLU "\n----------Tabular Aphanumeric Block (Page %d)----------\n" RESET , i+1 );

      while (1) {

         value = SHORT_BSWAP_L( *tab_data );
         if( value == -1 ){

            fprintf( stderr, "--->End Of Page Detected\n" );
            tab_data++;
            break;

         }

         len = SHORT_BSWAP_L( *tab_data )/sizeof(short);
         tab_data++;
         for( k = 0; k < len; k++ ){

            memcpy( &str[cnt], tab_data, sizeof(short) );
            cnt += sizeof(short);
            tab_data++;

         }

         str[cnt] = '\0';
         fprintf( stderr, "--->Line: %s\n", str );
         cnt = 0;
         memset(str, 0, 120);

      }

   }

} /* End of Dump_graphic_block_header() */


/* Macro definitions for specific packet types. */
/* Packet code 27 - Superob */
#define SUPEROB_CELL_SIZE_BYTES    18


/***************************************************************************

    Description: Swaps bytes in shorts for all byte fields in display data 
		packets and layers.

    Inputs:	block - pointer to the display data packets.
		tlen - total bytes of the display data packets.

    Output:	block - modified display data packets.

    NOTES:	This function contains product specific processing.  In
		the future, we will want to remove this.	

***************************************************************************/
static int Process_display_data_packets( unsigned short *block, int tlen ){

    unsigned short *end = block + (tlen / 2);
    unsigned short *start = block; 

    while (block < end) {

	switch (block[0]) {
	    int i, n_radials, n_rows, n_cells;
	    unsigned int len;

	    case 1:
		/* Write Text (No Value) */
		len = (block[1] - 4 + 1) / 2;

		/* Swap text data. */
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

	    case 2:
		/* Write Special Symbols (No Value) */
		len = (block[1] + 1) / 2;
		block += 2 + len;
		break;

	    case 3:
  		/* Mesocyclone (3) */
		len = (block[1] + 1) / 2;	/* length of packet, in shorts */
		block += 2 + len;
		break;

	    case 4:
		/* Wind Barb */
		block += 7;
                break;

	    case 5:
		/* Vector Arrow */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 6:
		/* Linked Vector (No Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 7:
		/* Unlinked Vector (No Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 8:
		/* Write Text (Uniform Value) */
		len = (block[1] - 6 + 1) / 2;

		/* Swap text data. */
		MISC_short_swap (block + 5, len);
		block += 5 + len;
		break;

	    case 9:
		/* Linked Vector (Uniform Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 10:
		/* Unlinked Vector (Uniform Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 11:
  		/* 3DC Shear (11) */
		block += 2 + (block[1] + 1) / 2;	
		break;

	    case 12:
		/* TVS Symbol */
		len = (block[1] - 4 + 1) / 2;
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

	    case 13:
	    case 14:
		/* Hail Positive (13) and Hail Probable (14) */
		len = (block[1] - 4 + 1) / 2;
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

	    case 15:
                /* Storm ID Packet. */
		len = (block[1] - 4 + 1) / 2;

		/* Swap text data. */
		MISC_short_swap( block + 4, len );
		block += 4 + len;
		break;

	    case 16:
		/* Digital Radial Data Array */
		n_radials = block[6];
		block += 7;
		for (i = 0; i < n_radials; i++) {

                    /* NOTE:  The length needs to be halved. */
		    len = (block[0] + 1)/2;
                    MISC_short_swap( block + 3, len );
		    block += 3 + len;
		}
		break;

	    case 17:
		/* Digital Precipitation Data Array */
		n_rows = block[4];
		block += 5;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 18:
		/* Digital Rate Array */
		n_rows = block[4];
		block += 5;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 19:
		/* HDA Hail */
		len = (block[1] + 1) / 2;
		block += 2 + len;
		break;

	    case 20:
		/* MDA Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 21:
		/* Cell Trend Data */

		/* Swap cell ID. */
		MISC_short_swap( block + 2, 1 );
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 22:
		/* Cell Trend Volume Scan Times */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 23:
		/* SCIT Past Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 24:
		/* SCIT Forecast Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 25:
		/* STI Circle */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 26:
		/* ETVS Symbol */
		len = (block[1] - 4 + 1) / 2;
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

            case 27:
                /* Superob Data */
                len = ( block[1] << 16 ) | ( block[2] & 0xffff );
                n_cells = (len - sizeof(unsigned short)) / SUPEROB_CELL_SIZE_BYTES;

                /* Skip to the start of the next packet. */
                block +=  4 + (n_cells * (SUPEROB_CELL_SIZE_BYTES/sizeof(unsigned short)));
                break;

            case 28:
                /* Generic product format data. */
                len = ( block[2] << 16 ) | ( block[3] & 0xffff );
		MISC_short_swap (block + 4, len/2);
                block += 4 + ((len + 1)/2);
                break;

	    case 30:
		/* LFM Grid Adaptation Parameters (Pre-edit) */
		SHORT_SSWAP ( (block + 1) );
		SHORT_SSWAP ( (block + 3) );
		SHORT_SSWAP ( (block + 5) );
		SHORT_SSWAP ( (block + 7) );
		block += 11;
		break;

	    case 31:
		/* Radar Coded Message (Pre-edit) */
		block += 2;
		break;

	    case 32:
		/* Radar Coded Message Composite Reflectivity (Pre-edit) */
		n_rows = block[1];
		block += 2;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 0xaf1f:
		/* Radial Run-Length-Encode */
		n_radials = block[6];
		block += 7;

		for (i = 0; i < n_radials; i++) {
		    len = block[0];

                    /* SPECIAL CASE: Product Code 137 (ULR) */
                    if( Prod_code == MSG_CODE_US_LAYER_CR ){

                       /* Only swap the RLE data, not the RLE header. */
                       MISC_short_swap( block + 3, len ); 

                    }
		    block += 3 + len;
		}
		break;


	    case 0x0802:
		/* Contour Value */
		block += 3;
		break;

	    case 0x0e03:
		/* Linked Contour Vectors */
                len = (block[4] + 1)/2;
                block += len + 5;
		break;

	    case 0x3501:
		/* Unlinked Contour Vectors */
                len = (block[1] + 1)/2;
		block += len + 2;
		break;

	    case 0xba0f:
	    case 0xba07:
		/* Raster Run-Length-Encode */
		n_rows = block[9];
		block += 11;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    default:
		fprintf( stderr, KRED "UMC: Unrecognized Display Data Packet %d in Product %d\n" RESET, 
                             (int)block[0], Prod_code);
                fprintf( stderr, "--->block offset: %d\n", (int) (block - start) );
		return -1;

	}

    }

    /* Normal return. */
    return 0;
}

/* Used for multiple packets. */
#define MIN_PIXEL			-2048
#define MAX_PIXEL			 2048

/* Used in Write Text Packet (Uniform Value - code 8). */
#define MIN_COLOR			    0
#define MAX_COLOR			   15

/* Used in AF1F Packet (Radial RLE) */
#define MAX_FIRST_RNG_BIN		  460
#define MAX_NUM_RNG_BINS		  460
#define MIN_SCALE_FACTOR		    1
#define MAX_SCALE_FACTOR		 8000
#define MAX_NUM_RADIALS			  400
#define MIN_NUM_RLE_HW			    1
#define MAX_NUM_RLE_HW			  230

/* Used in Digital Data Array Packet (code 16) */
#define MAX_DIG_FIRST_RNG_BIN		  230
#define MAX_DIG_NUM_RNG_BINS		 1840
#define MIN_DIG_SCALE_FACTOR		    1
#define MAX_DIG_SCALE_FACTOR		 1000
#define MAX_DIG_NUM_RADIALS		  720

/* Used in multiple packets. */
#define MAX_RAD_START_ANG		 3599
#define MAX_RAD_DELTA		 	   20

/* Used in Hail Packet. */
#define HAIL_FLAG			 -999
#define MAX_HAIL_SIZE                       4
#define MAX_HAIL_PROB                     100

/* Used in STI Radius Packet. */
#define MAX_STI_RADIUS			  512

/* Used in the Special Symbol Packet. */
#define MAX_FEATURE_TYPE		   11

/* Used in Vector Arrow Packet. */
#define MAX_ARROW_DIR			  359
#define MAX_ARROW_LEN			  512
#define MAX_ARROW_HEAD_LEN		  512

/* Used in Wind Barb Packet. */
#define MAX_WIND_DIR			  359
#define MAX_WIND_SPEED			  195
#define MAX_WIND_BARB_COLOR		    5

/* Used in Cell Trends Packet. */
#define MAX_TREND_CODE			    8
#define MAX_TREND_VOLUMES   		   10
#define MAX_TREND_VOLUME_TIME  		 1439
#define MIN_TREND_PIXEL			-4096
#define MAX_TREND_PIXEL			 4095

/* Used in Set Color Levels for Contours Packet. */
#define MAX_CONTOUR_COLOR		   15

/* Used in  Linked Contour Vectors. */
#define MIN_NUM_VECTORS			    4
#define MAX_NUM_VECTORS			32764

/* Used in Raster Data Packet. */
#define MAX_SCALE_INT			   67
#define MAX_NUM_ROWS			  464

/* Used in Digital Precipitation Data Array. */
#define NUM_LFM_BOXES_ROW_17		  131
#define NUM_ROWS_17			  131

/* Used in Digital Precipitation Rate Data Array. */
#define NUM_LFM_BOXES_ROW_18		   13
#define NUM_ROWS_18			   13

/***************************************************************************

    Description: Values in the packets are validated against the RPG to 
                 Class 1 ICD.

    Inputs:	 block - pointer to the display data packets.
		 tlen - total bytes of the display data packets.

***************************************************************************/
static int Validate_display_data_packets( unsigned short *block, int tlen ){

    unsigned short *end = block + (tlen / 2);
    unsigned short *start = block; 
    unsigned short *sptr = NULL;
    short value = 0;
    unsigned short u_value = 0, temp;
    char buf[100], c_value = 0;

    int skip, i, j, n_volumes = 0, n_vectors = 0, n_radials = 0;
    int n_rows = 0, n_cells = 0, n_rng_bins = 0, sum_runs = 0;
    int clen = 0;
    unsigned int len;

    RPGP_product_t *prod = NULL;

    /* Parse the product. */
    while (block < end) {

	switch (block[0]) {

	    case 1:
                fprintf( stderr, KBLU "Packet 1 (Text - No Value)\n" RESET );

		/* Write Text (No Value) */
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Starting Point. */
                value = (short) block[2];
                fprintf( stderr, "--->I start: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr, 
                            KRED "------>Invalid I Starting Point: %d (%d <= I start <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Starting Point. */
                value = (short) block[3];
                fprintf( stderr, "--->J start: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid J Starting Point: %d (%d <= J start <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                clen = len*sizeof(short);
                memcpy( &buf[0], &block[4], clen ); 
                buf[clen] = '\0'; 
                fprintf( stderr, "--->Text:    %s\n", &buf[0] );

		block += 4 + len;
		break;

	    case 2:
                fprintf( stderr, KBLU "Packet 2 (Special Symbol - No Value)\n" RESET );

		/* Write Special Symbols (No Value) */
		len = (block[1] + 1) / 2;

                /* Check the I Starting Point. */
                value = (short) block[2];
                fprintf( stderr, "--->I start: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid I Starting Point: %d (%d <= I start <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Starting Point. */
                value = (short) block[3];
                fprintf( stderr, "--->J start: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid J Starting Point: %d (%d <= J start <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

		block += 2 + len;
		break;

	    case 3:
  		/* Mesocyclone (3) */
                fprintf( stderr, KBLU "Packet 3 (Mesocyclone)\n" RESET );

		len = (block[1] + 1) / 2;	/* length of packet, in shorts */

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Radius of Mesocyclone. */
                value = (short) block[3];
                fprintf( stderr, "--->Radius:     %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid Radius: %d (%d <= Radius <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

		block += 2 + len;
		break;

	    case 4:
		/* Wind Barb */
                fprintf( stderr, KBLU "Packet 4 (Wind Barb)\n" RESET );

                /* Check the Wind Barb Color. */
                value = (short) block[2];
                fprintf( stderr, "--->Color:     %d\n", value );
                if( (value < 1) || (value > MAX_WIND_BARB_COLOR) )
                   fprintf( stderr,     
                            KRED "------>Invalid Wind Barb Color: %d (1 <= Color <= %d)\n" RESET, 
                            value, MAX_WIND_BARB_COLOR );

                /* Check the I Position. */
                value = (short) block[3];
                fprintf( stderr, "--->I Start:   %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Starting Point: %d (%d <= I start <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[4];
                fprintf( stderr, "--->J Start:   %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Starting Point: %d (%d <= J start <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Wind Direction. */
                value = (short) block[5];
                fprintf( stderr, "--->Direction: %d\n", value );
                if( (value < 0) || (value > MAX_WIND_DIR) )
                   fprintf( stderr,     
                            KRED "------>Invalid Wind Direction: %d (0 <= Direction <= %d)\n" RESET, 
                            value, MAX_WIND_DIR );

                /* Check the Wind Speed. */
                value = (short) block[6];
                fprintf( stderr, "--->Speed:     %d\n", value );
                if( (value < 0) || (value > MAX_WIND_SPEED) )
                   fprintf( stderr,     
                            KRED "------>Invalid Wind Speed: %d (0 <= Speed <= %d)\n" RESET,
                            value, MAX_WIND_SPEED );


		block += 7;
                break;

	    case 5:
		/* Vector Arrow */
                fprintf( stderr, KBLU "Packet 5 (Vector Arrow)\n" RESET );

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Arrow Direction. */
                value = (short) block[4];
                if( (value < 0) || (value > MAX_ARROW_DIR) )
                   fprintf( stderr,      
                            KRED "------>Invalid Arrow Direction: %d\n" RESET, value );

                /* Check the Arrow Length. */
                value = (short) block[5];
                if( (value < 1) || (value > MAX_ARROW_LEN) )
                   fprintf( stderr,       
                            KRED "------>Invalid Arrow Length: %d\n" RESET, value );

                /* Check the Arrow Direction. */
                value = (short) block[6];
                if( (value < 1) || (value > MAX_ARROW_HEAD_LEN) )
                   fprintf( stderr,       
                            KRED "------>Invalid Arrow Head Length: %d\n" RESET, value );


		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 6:
		/* Linked Vector (No Value) */
                fprintf( stderr, KBLU "Packet 6 (Linked Vector - No Value)\n" RESET );

		n_vectors = (block[1] + 1) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector. */
                   value = (short) block[2+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      fprintf( stderr,     
                               KRED "------>Invalid Vector Coordinate: %d (%d <= Vect Coord <= %d\n" RESET, 
                               value, MIN_PIXEL, MAX_PIXEL );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 7:
		/* Unlinked Vector (No Value) */
                fprintf( stderr, KBLU "Packet 7 (Unlinked Vector - No Value)\n" RESET );

		n_vectors = (block[1] + 1) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector. */
                   value = (short) block[2+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      fprintf( stderr,     
                               KRED "------>Invalid Vector Coordinate: %d (%d <= Vect Coord <= %d\n" RESET, 
                               value, MIN_PIXEL, MAX_PIXEL );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 8:
		/* Write Text (Uniform Value) */
                fprintf( stderr, KBLU "Packet 8 (Text - Uniform Value)\n" RESET );

		len = (block[1] - 6 + 1) / 2;

                /* Check the Text Color Value. */
                value = (short) block[2];
                fprintf( stderr, "--->Color:   %d\n", value );
                if( (value < MIN_COLOR) || (value > MAX_COLOR) )
                   fprintf( stderr,       
                            KRED "------>Invalid Color: %d (%d <= Color <= %d)\n" RESET,
                            value, MIN_COLOR,  MAX_COLOR );

                /* Check the I Starting Point. */
                value = (short) block[3];
                fprintf( stderr, "--->I start: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Starting Point: %d (%d <= I start <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Starting Point. */
                value = (short) block[4];
                fprintf( stderr, "--->J start: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Starting Point: %d (%d <= J start <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                clen = len*sizeof(short);
                memcpy( &buf[0], &block[5], clen ); 
                buf[clen] = '\0'; 
                fprintf( stderr, "--->Text:    %s\n", &buf[0] );
		block += 5 + len;
		break;

	    case 9:
		/* Linked Vector (Uniform Value) */
                fprintf( stderr, KBLU "Packet 9 (Linked Vector - Uniform Value)\n" RESET );

                /* Check the Text Color Value. */
                value = (short) block[2];
                fprintf( stderr, "--->Color:   %d\n", value );
                if( (value < MIN_COLOR) || (value > MAX_COLOR) )
                   fprintf( stderr,       
                            KRED "------>Invalid Color: %d (%d <= Color <= %d)\n" RESET,
                            value, MIN_COLOR, MAX_COLOR );

		n_vectors = (block[1] + 1) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector Coordinate. */
                   value = (short) block[3+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      fprintf( stderr,     
                               KRED "------>Invalid Vector Coordinate: %d (%d <= Vect Coord <= %d\n" RESET, 
                               value, MIN_PIXEL, MAX_PIXEL );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 10:
		/* Unlinked Vector (Uniform Value) */
                fprintf( stderr, KBLU "Packet 10 (Unlinked Vector - Uniform Value)\n" RESET );

                /* Check the Text Color Value. */
                value = (short) block[2];
                fprintf( stderr, "--->Color:   %d\n", value );
                if( (value < MIN_COLOR) || (value > MAX_COLOR) )
                   fprintf( stderr,       
                            KRED "------>Invalid Color: %d (%d <= Color <= %d)\n" RESET,
                            value, MIN_COLOR, MAX_COLOR );

                /* The number of vectors is the number of bytes in block less 2 bytes for color level
                   divided by 2 since each vector coordinate is 2 bytes. */
		n_vectors = (block[1] - 2) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector Coordinate. */
                   value = (short) block[3+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      fprintf( stderr,     
                               KRED "------>Invalid Vector Coordinate: %d (%d <= Vect Coord <= %d\n" RESET, 
                               value, MIN_PIXEL, MAX_PIXEL );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 11:
  		/* 3DC Shear (11) */
                fprintf( stderr, KBLU "Packet 11 (3DC Shear)\n" RESET );

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Radius of Mesocyclone. */
                value = (short) block[3];
                fprintf( stderr, "--->Radius:     %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,     
                            KRED "------>Invalid Radius: %d (%d <= Radius <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

		block += 2 + (block[1] + 1) / 2;	
		break;

	    case 12:
		/* TVS Symbol */
                fprintf( stderr, KBLU "Packet 12 (TVS)\n" RESET );

		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );


		block += 4 + len;
		break;

	    case 13:
	    case 14:
		/* Hail Positive (13) and Hail Probable (14) */
                fprintf( stderr, KBLU "Packet 13/14 (HAIL Positive/Probable)\n" RESET );

		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

		block += 4 + len;
		break;

	    case 15:
                /* Storm ID Packet. */
                fprintf( stderr, KBLU "Packet 15 (Storm ID)\n" RESET );
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position: %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the First Character in Cell ID.  Note:  Since this is a Little Endian
                   machine, the lower address is the letter, the upper address the number. */
                c_value = (block[4] & 0x00ff);
                if( (c_value < 'A') || (c_value > 'Z') )
                   fprintf( stderr, KRED "------>Invalid First Character Cell ID: %c (A <= 1st Char <= Z)\n" RESET, 
                            c_value );

                /* Check the Last Character in Cell ID. */
                c_value = (block[4] & 0xff00) >> 8;
                if( (c_value < '0') || (c_value > '9') )
                   fprintf( stderr, KRED "------>Invalid Second Character Cell ID: %c (0 <= 2nd Char <= 9\n" RESET, 
                            c_value );


		block += 4 + len;
		break;

	    case 16:
                fprintf( stderr, KBLU "Packet 16 (Digital Radial)\n" RESET );

                /* Initialize skip. */
                skip = 0;

		/* Digital Radial Data Array */
		n_radials = block[6];

                /* Check the Index of First Range Bin. */
                value = (short) block[1];
                if( (value < 0) || (value > MAX_DIG_FIRST_RNG_BIN) )
                   fprintf( stderr,       
                            KRED "------>Invalid First Bin: %d (0 <= 1st Bin <= %d)\n" RESET,
                            value, MAX_DIG_FIRST_RNG_BIN );

                /* Check the number of range bins. */
                value = (short) block[2];
                if( (value < 0) || (value > MAX_DIG_NUM_RNG_BINS) )
                   fprintf( stderr,       
                            KRED "------>Invalid Num Bins: %d (0 <= Num Bins <= %d)\n" RESET,
                            value, MAX_DIG_NUM_RNG_BINS );

                /* Check the I Center of Sweep. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Center: %d (%d <= I Center <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Center of Sweep. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Center: %d (%d <= J Center <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Scale Factor. */
                value = (short) block[5];
                if( (value < MIN_DIG_SCALE_FACTOR) || (value > MAX_DIG_SCALE_FACTOR) )
                   fprintf( stderr,       
                            KRED "------>Invalid Scale Factor: %d  (%d <= Scale Fact <= %d)\n" RESET,
                            value, MIN_DIG_SCALE_FACTOR, MAX_DIG_SCALE_FACTOR );

                /* Check the Number of Radials. */
                if( (n_radials < 1) || (n_radials > MAX_DIG_NUM_RADIALS) ){

                   fprintf( stderr,       
                            KRED "------>Invalid Num Radials: %d (1 <= Num Rads <= %d)\n" RESET,
                            value, MAX_DIG_NUM_RADIALS );
                   skip = 1;

                }

#ifdef DETAILED_OUTPUT
                fprintf( stderr, "Packet 16 Header:\n" );
                fprintf( stderr, "--->First Rng Bin:  %5d\n", (short) block[1] );
                fprintf( stderr, "---># Rng Bins:     %5d\n", (short) block[2] );
                fprintf( stderr, "--->I Center:       %5d\n", (short) block[3] );
                fprintf( stderr, "--->J Center:       %5d\n", (short) block[4] );
                fprintf( stderr, "--->Scale Factor:   %5d\n", (short) block[5] );
                fprintf( stderr, "---># Radials:      %5d\n", (short) block[6] );
#endif

		block += 7;

                if( !skip ){

   	   	   for (i = 0; i < n_radials; i++) {

                       /* Check the Number of Bytes In Radial. */
                       value = (short) block[0];
                       if( (value < 1) || (value > MAX_DIG_NUM_RNG_BINS) ){

                          fprintf( stderr, KRED "------>Invalid Number of Range Bins: (1 <= %d <= %d)\n" RESET, 
                                   value, MAX_DIG_NUM_RNG_BINS );
                          skip = 1;
                          break;

                       }

                       /* Check the Radial Start Angle. */
                       value = (short) block[1];
                       if( (value < 0) || (value > MAX_RAD_START_ANG) )
                          fprintf( stderr, KRED "------>Invalid Radial Start Angle: (0 <= %d <= %d)\n" RESET, 
                                   value, MAX_RAD_START_ANG );

                       /* Check the Radial Angle Delta. */
                       value = (short) block[2];
                       if( (value < 0) || (value > MAX_RAD_DELTA) )
                          fprintf( stderr, KRED "------>Invalid Radial Delta Angle: (0 <= %d <= %d)\n" RESET, 
                                   value, MAX_RAD_DELTA );

#ifdef DETAILED_OUTPUT
                       fprintf( stderr, "------>Radial: %3d, Rng Bins: %3d, Start Angle: %4d, Delta Angle: %2d\n",
                                i, (short) block[0], (short) block[1], (short) block[2] );
#endif

                       /* NOTE:  The length needs to be halved. */
		       len = (block[0] + 1)/2;
		       block += 3 + len;

                    }

		}
		break;

	    case 17:
		/* Digital Precipitation Data Array */
                fprintf( stderr, KBLU "Packet 17 (Digital Precipitation Array)\n" RESET );

		n_rows = block[4];

                /* Check the Number of LFM Boxes in Row. */
                value = (short) block[3];
                if( value != NUM_LFM_BOXES_ROW_17 )
                   fprintf( stderr,  KRED "------>Invalid # of LFM Boxes in Row: %d\n" RESET, value );

                /* Check the Number of Rows. */
                if( n_rows != NUM_ROWS_17 )
                   fprintf( stderr,  KRED "------>Invalid # of Rows: %d\n" RESET, n_rows );

		block += 5;
		for (i = 0; i < n_rows; i++) {

		    len = (block[0] + 1) / 2;

                    /* Insure the number of range bins matches the sum of the run values. */
                    sum_runs = 0;
                    for( j = 0; j < len; j++ ){

                       unsigned short temp;

                       u_value = block[1+j];
                       temp = ((u_value & 0xff00) >> 8);
                       sum_runs += temp;

                    } 

                    if( sum_runs != NUM_LFM_BOXES_ROW_17 )
                       fprintf( stderr,
                                KRED "------>Invalid Number of LFM Boxes (%d != %d) in Row\n" RESET, 
                                sum_runs, NUM_LFM_BOXES_ROW_17 );

		    block += 1 + len;
		}
		break;

	    case 18:
		/* Digital Rate Array */
                fprintf( stderr, KBLU "Packet 18 (Digital Rate Array)\n" RESET );

		n_rows = block[4];

                /* Check the Number of LFM Boxes in Row. */
                value = (short) block[3];
                if( value != NUM_LFM_BOXES_ROW_18 )
                   fprintf( stderr,  KRED "------>Invalid # of LFM Boxes in Row: %d\n" RESET, value );

                /* Check the Number of Rows. */
                if( n_rows != NUM_ROWS_18 )
                   fprintf( stderr, 
                            KRED "------>Invalid # of Rows: %d\n" RESET, n_rows );

		block += 5;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;

                    /* Insure the number of range bins matches the sum of the run values. */
                    sum_runs = 0;
                    for( j = 0; j < len; j++ ){

                       u_value = block[1+j];
                       temp = ((u_value & 0xff00) >> 8);
                       sum_runs += (temp >> 4);

                       temp = (u_value & 0x00ff);
                       sum_runs += (temp >> 4);

                    } 

                    if( sum_runs != NUM_LFM_BOXES_ROW_18 )
                       fprintf( stderr,
                                KRED "------>Invalid Number of LFM Boxes (%d != %d) in Row)\n" RESET, 
                                sum_runs, NUM_LFM_BOXES_ROW_18 );
		    block += 1 + len;
		}
		break;

	    case 19:
		/* HDA Hail */
                fprintf( stderr, KBLU "Packet 19 (HDA Hail)\n" RESET );

		len = (block[1] + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Probability of Hail. */
                value = (short) block[4];
                fprintf( stderr, "--->Prob Hail:    %d\n", value);
                if( (value != HAIL_FLAG) 
                           && 
                    ((value < 0) || (value > MAX_HAIL_PROB)) )
                   fprintf( stderr, 
                            KRED "------>Invalid Probability of Hail: %d (0 <= Prob Hail <= %d)\n" RESET, 
                            value, MAX_HAIL_PROB );

                /* Check the Probability of Severe Hail. */
                value = (short) block[5];
                fprintf( stderr, "--->Prob Svr Hail: %d\n", value);
                if( (value != HAIL_FLAG) 
                           && 
                    ((value < 0) || (value > MAX_HAIL_PROB)) )
                   fprintf( stderr, 
                            KRED "------>Invalid Probability of Svr Hail: %d (0 <= Prob Svr Hail <= %d)\n" RESET, 
                            value, MAX_HAIL_PROB );

                /* Check the Max Hail Size. */
                value = (short) block[6];
                fprintf( stderr, "--->Max Hail Size: %d\n", value);
                if( (value < 0) || (value > MAX_HAIL_SIZE) )
                   fprintf( stderr, 
                            KRED "------>Invalid Max Hail Size: %d (0 <= Max Hail Size <= %d)\n" RESET, 
                            value, MAX_HAIL_SIZE );

		block += 2 + len;
		break;

	    case 20:
		/* MDA Data */
                fprintf( stderr, KBLU "Packet 20 (MDA)\n" RESET );

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Point Feature Type. */
                value = (short) block[4];
                fprintf( stderr, "--->Feat Type:    %d\n", value);
                if( (value < 1) || (value > MAX_FEATURE_TYPE) )
                   fprintf( stderr,       
                            KRED "------>Invalid Feat Type: %d (1 <= Feat Type <= %d)\n" RESET,
                            value, MAX_FEATURE_TYPE );


		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 21:
		/* Cell Trend Data */
                fprintf( stderr, KBLU "Packet 21 (Cell Trend)\n" RESET );

                /* Check the First Character in Cell ID.  Note:  Since this is a Little Endian
                   machine, the lower address is the letter, the upper address the number. */
                c_value = (block[2] & 0x00ff);
                if( (c_value < 'A') || (c_value > 'Z') )
                   fprintf( stderr, 
                            KRED "------>Invalid First Character Cell ID: %c (A <= 1st char <= Z)\n" RESET, c_value );

                /* Check the Last Character in Cell ID. */
                c_value = (block[2] & 0xff00) >> 8;
                if( (c_value < '0') || (c_value > '9') )
                   fprintf( stderr, 
                            KRED "------>Invalid Second Character Cell ID: %c (0 <= 2nd char <= 9)\n" RESET, c_value );

                /* Check the I Position. */
                value = (short) block[3];
                fprintf( stderr, "--->I Position:    %d\n", value);
                if( (value < MIN_TREND_PIXEL) || (value > MAX_TREND_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_TREND_PIXEL, MAX_TREND_PIXEL );

                /* Check the J Position. */
                value = (short) block[4];
                fprintf( stderr, "--->J Position:    %d\n", value);
                if( (value < MIN_TREND_PIXEL) || (value > MAX_TREND_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_TREND_PIXEL, MAX_TREND_PIXEL );

                /* Check the Trend Code. */
                value = (short) block[5];
                fprintf( stderr, "--->Trend Code:    %d\n", value);
                if( (value < 1) || (value > MAX_TREND_CODE) )
                   fprintf( stderr, 
                            KRED "------>Invalid Trend Code: %d (1 <= Code <= %d)\n" RESET, 
                            value, MAX_TREND_CODE );

                /* Check the Trend Number of Volumes. */
                value = (short) block[6];
                temp = ((value & 0xff00) >> 8);
                fprintf( stderr, "--->Trend Vols:    %d\n", temp );
                if( (temp < 1) || (temp > MAX_TREND_VOLUMES) )
                   fprintf( stderr, 
                            KRED "------>Invalid Trend # Volumes: %d (1 <= # Vols <= %d)\n" RESET, 
                            temp, MAX_TREND_VOLUMES );

                /* Check the Trend Volume Pointer. */
                temp = (value & 0x00ff);
                fprintf( stderr, "--->Trend Vol Ptr: %d\n", temp );
                if( (temp < 1) || (temp > MAX_TREND_VOLUMES) )
                   fprintf( stderr, 
                            KRED "------>Invalid Trend Volume Pointer: %d (1 <= Vol Ptr <= %d)\n" RESET, 
                            temp, MAX_TREND_VOLUMES );

		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 22:
		/* Cell Trend Volume Scan Times */
                fprintf( stderr, KBLU "Packet 22 (Cell Trend Volume Times)\n" RESET );

                /* Check the Trend Number of Volumes. */
                n_volumes = (block[2] & 0xff00) >> 8;
                fprintf( stderr, "--->Trend # Vols:   %d\n", n_volumes );
                if( (n_volumes < 1) || (n_volumes > MAX_TREND_VOLUMES) )
                   fprintf( stderr, 
                            KRED "------>Invalid Trend Number of Volumes: %d (1 <= Trend Vols <= %d)\n" RESET, 
                            n_volumes, MAX_TREND_VOLUMES );

                else{

                   /* Check the Trend Volume Pointer. */
                   value = (block[2] & 0x00ff);
                   fprintf( stderr, "--->Trend Vol Ptr:  %d\n", n_volumes );
                   if( (value < 1) || (value > MAX_TREND_VOLUMES) )
                      fprintf( stderr, 
                               KRED "------>Invalid Trend Volume Pointer: %d (1 <= Vol Ptr <= %d)\n" RESET, 
                               value, MAX_TREND_VOLUMES );

                   for( i = 0; i < n_volumes; i++ ){

                      /* Check the Trend Volume Times. */
                      value = (short) block[3+i];
                      fprintf( stderr, "--->Trend Vol Time: %d\n", n_volumes );
                         fprintf( stderr, 
                                  KRED "------>Invalid Trend Volume Time: %d (0 <= Vol Time <= %d)\n" RESET, 
                                  value, MAX_TREND_VOLUME_TIME );

                   }

                }

		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 23:
		/* SCIT Past Data */
                fprintf( stderr, KBLU "Packet 23 (SCIT Past Data)\n" RESET );
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 24:
		/* SCIT Forecast Data */
                fprintf( stderr, KBLU "Packet 24 (SCIT Forecast Data)\n" RESET );
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 25:
		/* STI Circle */
                fprintf( stderr, KBLU "Packet 25 (STI Circle)\n" RESET );

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Radius of STI Circle. */
                value = (short) block[4];
                fprintf( stderr, "--->Radius:       %d\n", value);
                if( (value < 1) || (value > MAX_STI_RADIUS) )
                   fprintf( stderr,       
                            KRED "------>Invalid Radius: %d (1 <= Radius <= %d)\n" RESET,
                            value, MAX_STI_RADIUS );

		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 26:
		/* ETVS Symbol */
                fprintf( stderr, KBLU "Packet 26 (ETVS)\n" RESET );

		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                fprintf( stderr, "--->I Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Position. */
                value = (short) block[3];
                fprintf( stderr, "--->J Position:   %d\n", value);
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,       
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET,
                            value, MIN_PIXEL, MAX_PIXEL );

		block += 4 + len;
		break;

            case 27:
                /* Superob Data */
                fprintf( stderr, KBLU "Packet 27 (SuperOb)\n" RESET );

                len = ( block[1] << 16 ) | ( block[2] & 0xffff );
                n_cells = (len - sizeof(unsigned short)) / SUPEROB_CELL_SIZE_BYTES;

                /* Skip to the start of the next packet. */
                block +=  4 + (n_cells * (SUPEROB_CELL_SIZE_BYTES/sizeof(unsigned short)));
                break;

            case 28:
                /* Generic product format data. */
                fprintf( stderr, KBLU "Packet 28 (Generic Format)\n" RESET );

                /* Dump the packet 28 header. */
                Dump_packet28_header( (void *) &block[0], &clen );

                sptr = (unsigned short *) ((char *) &block[0] + sizeof(packet_28_t)); 
                if( RPGP_product_deserialize( (char *) sptr, clen, (void *) &prod ) < 0 ){

                   fprintf( stderr, KRED "Could Not Deserialize Serialized Data\n" RESET );
                   break;

                }

                Dump_generic_prod_data( (void *) prod );
                len = ( block[2] << 16 ) | ( block[3] & 0xffff );
                block += 4 + ((len + 1)/2);
                break;

	    case 30:
		/* LFM Grid Adaptation Parameters (Pre-edit) */
                fprintf( stderr, KBLU "Packet 30 (LFM Grid Adapt)\n" RESET );

		block += 11;
		break;

	    case 31:
		/* Radar Coded Message (Pre-edit) */
                fprintf( stderr, KBLU "Packet 31 (RCM)\n" RESET );

		block += 2;
		break;

	    case 32:
		/* Radar Coded Message Composite Reflectivity (Pre-edit) */
                fprintf( stderr, KBLU "Packet 32 (RCM Composite Reflectivity)\n" RESET );

		n_rows = block[1];
		block += 2;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 0xaf1f:
		/* Radial Run-Length-Encode */
                fprintf( stderr, KBLU "Packet 0xaf1f (Radial Run Length Encode)\n" RESET );

		n_radials = block[6];

                /* If the number of radials is bad, then validating the rest
                   of the packet. */
                skip = 0;

                /* Check the Index of First Range Bin. */
                value = (short) block[1];
                if( (value < 0) || (value > MAX_FIRST_RNG_BIN) )
                   fprintf( stderr,
                            KRED "------>Invalid First Range Bin: %d (0 <= 1st Bin Index <= %d)\n" RESET, 
                            value, MAX_FIRST_RNG_BIN );

                /* Check the number of range bins. */
                n_rng_bins = (short) block[2];
                if( (n_rng_bins < 1) || (n_rng_bins > MAX_NUM_RNG_BINS) )
                   fprintf( stderr, KRED "------>Invalid Num Range Bins: %d (1 <= # Bins <= %d)\n" RESET, 
                            value, MAX_NUM_RNG_BINS );

                /* Check the I Center of Sweep. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,
                            KRED "------>Invalid I Position: %d (%d <= I Center <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Center of Sweep. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,
                            KRED "------>Invalid J Position: %d (%d <= J Center <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the Scale Factor. */
                value = (short) block[5];
                if( (value < MIN_SCALE_FACTOR) || (value > MAX_SCALE_FACTOR) )
                   fprintf( stderr,
                            KRED "------>Invalid Scale Factor: %d (%d <= Scale Factor <= %d)\n" RESET, 
                            value, MIN_SCALE_FACTOR, MAX_SCALE_FACTOR );

                /* Check the Number of Radials. */
                if( (n_radials < 1) || (n_radials > MAX_NUM_RADIALS) ){

                   fprintf( stderr,
                            KRED "------>Invalid Num Radials: %d (1 <= # Radials <= %d)\n" RESET, 
                            value, MAX_NUM_RADIALS );
                   skip = 1;

                }

#ifdef DETAILED_OUTPUT
                fprintf( stderr, "Packet AF1F Header:\n" );
                fprintf( stderr, "--->First Rng Bin:  %5d\n", (short) block[1] );
                fprintf( stderr, "---># Rng Bins:     %5d\n", (short) block[2] );
                fprintf( stderr, "--->I Center:       %5d\n", (short) block[3] );
                fprintf( stderr, "--->J Center:       %5d\n", (short) block[4] );
                fprintf( stderr, "--->Scale Factor:   %5d\n", (short) block[5] );
                fprintf( stderr, "---># Radials:      %5d\n", (short) n_radials );
#endif
		block += 7;

                if( !skip ){

		   for (i = 0; i < n_radials; i++) {

		      len = block[0];

                      /* Check the Number of RLE Halfwords. */
                      if( (len < MIN_NUM_RLE_HW) || (len > MAX_NUM_RLE_HW) ){

                         fprintf( stderr, 
                                  KRED "------>Invalid Number RLE Halfwords in Radial: %d (%d <= RLE Words <= %d)\n" RESET, 
                                  len, MIN_NUM_RLE_HW, MAX_NUM_RLE_HW );
                         skip = 1;
                         break;

                       }

                       /* Check the Radial Start Angle. */
                       value = (short) block[1];
                       if( (value < 0) || (value > MAX_RAD_START_ANG) )
                          fprintf( stderr,
                                   KRED "------>Invalid Radial Start Angle: %d (0 <= Start Angle <= %d)\n" RESET, 
                                   value, MAX_RAD_START_ANG );

                       /* Check the Radial Angle Delta. */
                       value = (short) block[2];
                       if( (value < 0) || (value > MAX_RAD_DELTA) )
                          fprintf( stderr,
                                   KRED "------>Invalid Radial Delta Angle: %d (0 <= Delta Angle <= %d\n" RESET, 
                                   value, MAX_RAD_DELTA );
    
#ifdef DETAILED_OUTPUT
                       fprintf( stderr, "------>Radial: %3d, Halfwords: %3d, Start Angle: %4d, Delta Angle: %2d\n",
                                i, len, (short) block[1], (short) block[2] );
#endif
                       /* Insure the number of range bins matches the sum of the run values. */
                       sum_runs = 0;
                       for( j = 0; j < len; j++ ){

                          u_value = block[3+j];
                          temp = ((u_value & 0xff00) >> 8);
                          sum_runs += (temp >> 4);

                          temp = (u_value & 0x00ff);
                          sum_runs += (temp >> 4);

                       } 

                       if( sum_runs != n_rng_bins )
                          fprintf( stderr,
                                   KRED "Invalid Number of Range Bins (%d != %d) in Radial\n" RESET, 
                                   sum_runs, n_rng_bins );

		       block += 3 + len;
		   }

                }
		break;

	    case 0x0802:
		/* Contour Value */
                fprintf( stderr, KBLU "Packet 0x0802 (Contour Value)\n" RESET );

                /* Check the Color Value Indicator. */
                value = (short) block[1];
                if( value != 0x0002 )
                   fprintf( stderr,
                            KRED "------>Invalid Color Value Indicator Set Color Levels: %x\n" RESET, value );

                /* Check the Value (Level) of Contour. */
                value = (short) block[2];
                fprintf( stderr, "--->Contour Level Value: %d\n", value );
                if( (value < 0) || (value > MAX_CONTOUR_COLOR) )
                   fprintf( stderr,
                            KRED "------>Invalid Contour Level Value: %d\n", value );

		block += 3;
		break;

	    case 0x0e03:
		/* Linked Contour Vectors */
                fprintf( stderr, KBLU "Packet 0x0802 (Linked Contour Vectors)\n" RESET );

                /* Check the I Starting Point. */
                value = (short) block[2];
                fprintf( stderr, "--->I Start Pt:     %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr, 
                            KRED "------>Invalid I Starting Point: %d\n" RESET, value );

                /* Check the J Starting Point. */
                value = (short) block[3];
                fprintf( stderr, "--->J Start Pt:     %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr, 
                            KRED "------>Invalid J Starting Point: %d\n" RESET, value );

                /* Check the Length of Vectors. */
                value = (short) block[4];
                fprintf( stderr, "--->Len of Vectors: %d\n", value );
                if( (value < MIN_NUM_VECTORS) || (value > MAX_NUM_VECTORS) )
                   fprintf( stderr, 
                            KRED "------>Invalid Length of Vectors: %d\n" RESET, value );

                len = (block[4] + 1)/2;
                for( i = 0; i < len; i++ ){

                   /* Check the Vector End Point. */
                   value = (short) block[5+i];
                   fprintf( stderr, "--->Vector End Pt:  %d\n", value );
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      fprintf( stderr, 
                               KRED "------>Invalid Vector End Point: %d\n" RESET, value );

                }

                block += len + 5;
		break;

	    case 0x3501:
		/* Unlinked Contour Vectors */
                fprintf( stderr, KBLU "Packet 0x0802 (Unlinked Contour Vectors)\n" RESET );

                /* Check the Length of Vectors. */
                value = (short) block[1];
                fprintf( stderr, "--->Len of Vectors: %d\n", value );
                if( (value < MIN_NUM_VECTORS) || (value > MAX_NUM_VECTORS) )
                   fprintf( stderr, 
                            KRED "Invalid Length of Vectors: %d\n" RESET, value );

                len = (block[1] + 1)/2;
                for( i = 0; i < len; i++ ){

                   /* Check the Vector End Point. */
                   value = (short) block[2+i];
                   fprintf( stderr, "--->Vector End Pt:  %d\n", value );
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      fprintf( stderr, 
                               KRED "Invalid Vector End Point: %d\n" RESET, value );

                }

		block += len + 2;
		break;

	    case 0xba0f:
	    case 0xba07:
		/* Raster Run-Length-Encode */
                fprintf( stderr, KBLU "Packet 0xba0f/ba07 (Raster Run-Length Encode)\n" RESET );

                /* Check the I Coordinate Start. */
                value = (short) block[3];
                fprintf( stderr, "--->I Position:   %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,
                            KRED "------>Invalid I Position: %d (%d <= I Pos <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the J Coordinate Start. */
                value = (short) block[4];
                fprintf( stderr, "--->J Position:   %d\n", value );
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   fprintf( stderr,
                            KRED "------>Invalid J Position: %d (%d <= J Pos <= %d)\n" RESET, 
                            value, MIN_PIXEL, MAX_PIXEL );

                /* Check the X Scale INT. */
                value = (short) block[5];
                fprintf( stderr, "--->X Scale INT:  %d\n", value );
                if( (value < 1) || (value > MAX_SCALE_INT) )
                   fprintf( stderr,
                            KRED "------>Invalid X Scale INT: %d (1 <= Scale <= %d)\n" RESET, 
                            value, MAX_SCALE_INT );

                /* Check the Y Scale INT. */
                value = (short) block[7];
                fprintf( stderr, "--->Y Scale INT:  %d\n", value );
                if( (value < 1) || (value > MAX_SCALE_INT) )
                   fprintf( stderr,
                            KRED "------>Invalid Y Scale INT: %d (1 <= Scale <= %d)\n" RESET, 
                            value, MAX_SCALE_INT );

                /* Check the Number of Rows. */
		n_rows = block[9];
                fprintf( stderr, "--->N Rows:       %d\n", n_rows );
                if( (n_rows < 1) || (n_rows > MAX_NUM_ROWS) )
                   fprintf( stderr,
                            KRED "------>Invalid Number of Rows: %d (1 <= # Rows <= %d)\n" RESET, 
                            n_rows, MAX_NUM_ROWS );

		block += 11;
		for (i = 0; i < n_rows; i++) {

		    len = (block[0] + 1) / 2;
		    block += 1 + len;

		}
		break;

	    default:
		fprintf( stderr, KRED "UMC: Unrecognized Display Data Packet %d in Product %d\n" RESET, 
                             (int)block[0], Prod_code);
                fprintf( stderr, "--->block offset: %d\n", (int) (block - start) );
		return -1;

	}

    }

    /* Normal return. */
    return 0;
}
/*\/////////////////////////////////////////////////////////////////////

   Description:
      Displays the contents of Packet 28 header.

   Inputs:
      p28 - pointer to packet_28_t structure

   Outputs:
      data_len - Data Length

/////////////////////////////////////////////////////////////////////\*/
static void Dump_packet28_header( packet_28_t *p28, int *data_len ){

   unsigned short *sptr = (unsigned short *) &p28->num_bytes;
   *data_len = ( sptr[0] << 16 ) | ( sptr[1] & 0xffff );

   fprintf( stdout, KBLU "-------------Packet 28 Header-------------\n" RESET );
   fprintf( stdout, "--->code:       %d\n", p28->code );
   fprintf( stdout, "--->data_len:   %d\n\n", *data_len );

} /* End of Dump_packet28_header() */

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Displays the generic product messages.

   Inputs:
      prod - pointer to RPGP_product_t structure

/////////////////////////////////////////////////////////////////////\*/
static void Dump_generic_prod_data( RPGP_product_t *prod ){

   int year, mon, day, hr, min, sec;
   time_t ctime;

   fprintf( stdout, KBLU "----------------RPGP_product_t------------\n" RESET );
   fprintf( stdout, "--->name:          %s\n", prod->name );
   fprintf( stdout, "--->description:   %s\n", prod->description );
   fprintf( stdout, "--->product_id:    %d\n", prod->product_id );
   fprintf( stdout, "--->product_type:  %d\n", prod->type );
 
   /* Convert the generation time. */
   ctime = (time_t) prod->gen_time;
   unix_time( (time_t *) &ctime, &year, &mon, &day, &hr, &min, &sec );
   if( year >= 2000 )
      year -= 2000;
   else if( year >= 1900 )
      year -= 1900;
   fprintf( stdout, "--->gen_time:      %10u (%02d/%02d/%02d %02d:%02d:%02d)\n", 
            (unsigned int) ctime, mon, day, year, hr, min, sec );

   fprintf( stdout, "--->radar name:    %s\n", prod->radar_name );
   fprintf( stdout, "--->latitude:      %f\n", prod->radar_lat );
   fprintf( stdout, "--->longitude:     %f\n", prod->radar_lon );
   fprintf( stdout, "--->height:        %f\n", prod->radar_height );

   /* Convert the volume scan start time. */
   ctime = (time_t) prod->volume_time;
   unix_time( (time_t *) &ctime, &year, &mon, &day, &hr, &min, &sec );
   if( year >= 2000 )
      year -= 2000;
   else if( year >= 1900 )
      year -= 1900;
   fprintf( stdout, "--->volume_time:   %10u (%02d/%02d/%02d %02d:%02d:%02d)\n", 
            (unsigned int) ctime, mon, day, year, hr, min, sec );

   /* Convert the elevation scan start time. */
   ctime = (time_t) prod->elevation_time;
   unix_time( (time_t *) &ctime, &year, &mon, &day, &hr, &min, &sec );
   if( year >= 2000 )
      year -= 2000;
   else if( year >= 1900 )
      year -= 1900;
   fprintf( stdout, "--->elev_time:     %10u (%02d/%02d/%02d %02d:%02d:%02d)\n", 
            (unsigned int) ctime, mon, day, year, hr, min, sec );

   fprintf( stdout, "--->elev_angle:    %f\n", prod->elevation_angle );
   fprintf( stdout, "--->vol_number:    %d\n", prod->volume_number );
   fprintf( stdout, "--->op_mode:       %d\n", prod->operation_mode );
   fprintf( stdout, "--->VCP:           %d\n", prod->vcp );
   fprintf( stdout, "--->elev_number:   %d\n", prod->elevation_number );
   fprintf( stdout, "--->compress:      %d\n", prod->compress_type );
   fprintf( stdout, "--->decomp size:   %d\n", prod->size_decompressed );
   fprintf( stdout, "---># prod parm:   %d\n", prod->numof_prod_params );
   fprintf( stdout, "---># components:  %d\n\n", prod->numof_components );

   /* Process components. */
   if( prod->numof_components > 0 )
      Dump_components( prod->numof_components, prod );

} /* End of Dump_generic_prod_data() */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Dumps the component information in Packet 28. 

   Inputs:
      num_comps - number of components.
      prod - pointer to RPGP_product_t structure. 

////////////////////////////////////////////////////////////////////\*/
static void Dump_components( int num_comps, RPGP_product_t *prod ){

   int type, size, loc_type, i, j, k;
   char *loc, *loc1;
   char *substr = NULL;

   for( i = 0; i < num_comps; i++ ){

      type = *((int *) prod->components[i]);    
      if( type == RPGP_TEXT_COMP ){

         RPGP_text_t *comp = (RPGP_text_t *) prod->components[i];

         if( comp->numof_comp_params > 0 ){

            if( Verbose ){

               fprintf( stdout, "%d,  ", i );

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
                        fprintf( stdout,"%s,  ", loc ); 

                     }

                  }

               } /* End of for loop. */
               
            }

         }

         if( !Str_match )
            fprintf( stdout, "%s", comp->text );

         else{

            if( (substr = strstr( comp->text, Match_str )) != NULL ) 
               fprintf( stdout, "%s", comp->text );

         }

      }
      else if( type == RPGP_RADIAL_COMP ){

         fprintf( stdout, KBLU "----------------RPGP_radial_t--------------\n" RESET );

         RPGP_radial_t *radial = (RPGP_radial_t *) prod->components[i];

         if( radial->description != NULL )
             fprintf( stdout, "--->Description:      %s\n", radial->description );
         fprintf( stdout, "--->Bin Size:         %f\n", radial->bin_size );
         fprintf( stdout, "--->First Range:      %f\n", radial->first_range ); 
         fprintf( stdout, "--->Num Comp Params:  %d\n", radial->numof_comp_params ); 
         fprintf( stdout, "--->Num Radials:      %d\n", radial->numof_radials ); 

         for( j = 0; j < radial->numof_radials; j++ ){

            RPGP_radial_data_t *data = (RPGP_radial_data_t *) &radial->radials[j];
            RPGP_data_t *vals = (RPGP_data_t *) &(radial->radials[j].bins);

            fprintf( stdout, "---------- Radial %3d ----------\n", j );
            fprintf( stdout, "------>Azimuth:    %f\n", data->azimuth );
            fprintf( stdout, "------>Elevation:  %f\n", data->elevation );
            fprintf( stdout, "------>Width:      %f\n", data->width );
            fprintf( stdout, "------># Bins:     %d\n", data->n_bins );
            fprintf( stdout, "--------->Attr:    %s\n", vals->attrs );

         }

      }
      else if( type == RPGP_GRID_COMP ){

         fprintf( stdout, KBLU "----------------RPGP_grid_t--------------\n" RESET );

      }
      else if( type == RPGP_AREA_COMP ){

         RPGP_area_t *area = (RPGP_area_t *) prod->components[i];

         fprintf( stdout, KBLU "----------------RPGP_area_t--------------\n" RESET );
         fprintf( stdout, "--->Num Comp Params:  %d\n", area->numof_comp_params ); 
         fprintf( stdout, "-------Component Parameters--------------\n" );
         for( j = 0; j < area->numof_comp_params; j++ ){

            fprintf( stderr, "--->Comp Param %d\n", j+1 );
            fprintf( stderr, "------>id:    %s\n", area->comp_params[j].id );
            fprintf( stderr, "------>attrs: %s\n", area->comp_params[j].attrs );

         } /* End of for loop. */

         fprintf( stdout, "--->Num of Points:   %d\n", area->numof_points ); 
         fprintf( stdout, "---------------Points--------------------\n" );
         loc_type = RPGP_LOCATION_TYPE( area->area_type );
         if( loc_type == RPGP_LATLON_LOCATION ){
 
            RPGP_location_t *latlon = (RPGP_location_t *) area->points;
            for( k = 0; k < area->numof_points; k++ ){

               fprintf( stderr, "--->Point: %d\n", k+1 );
               fprintf( stderr, "------>Latitude:  %f\n", latlon[k].lat );
               fprintf( stderr, "------>Longitude: %f\n", latlon[k].lon );

            }

         }
         else if( loc_type == RPGP_XY_LOCATION ){
 
            RPGP_xy_location_t *xy = (RPGP_xy_location_t *) area->points;
            for( k = 0; k < area->numof_points; k++ ){

               fprintf( stderr, "--->Point: %d\n", k+1 );
               fprintf( stderr, "------>X:  %f\n", xy[k].x );
               fprintf( stderr, "------>Y:  %f\n", xy[k].y );

            }

         }
         else if( loc_type == RPGP_AZRAN_LOCATION ){

            RPGP_azran_location_t *azran = (RPGP_azran_location_t *) area->points;
            for( k = 0; k < area->numof_points; k++ ){

               fprintf( stderr, "--->Point: %d\n", k+1 );
               fprintf( stderr, "------>Az:   %f\n", azran[k].azi );
               fprintf( stderr, "------>Ran:  %f\n", azran[k].range );

            }

         }
         else
            fprintf( stderr, "--->Unknown Location Type\n" );

      }
      else if( type == RPGP_TABLE_COMP ){

         fprintf( stdout, KBLU "----------------RPGP_table_t--------------\n" RESET );

      }
      else if( type == RPGP_EVENT_COMP ){

         fprintf( stdout, KBLU "----------------RPGP_event_t--------------\n" RESET );

      }
      else
         fprintf( stderr, "Unsupported Component Type\n" ); 

   }

} /* End of Dump_components. */

/*\/////////////////////////////////////////////////////////////////////

       Description:  This function reads command line arguments.

       Input:        argc - Number of command line arguments.
                     argv - Command line arguments.

       Output:       Usage message

       Returns:      0 on success or -1 on failure

       Notes:

///////////////////////////////////////////////////////////////////\*/
static int Read_options (int argc, char **argv){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int arg = 0, c;         /* used by getopt */

   Verbose = 0;
   Product_header_stripped = 1;
   Strip_WMO_product_header = 0;
   Str_match = 0;
   Dir_name[0] = '\0';
   Match_str[0] = '\0';

   while ((c = getopt (argc, argv, "svD:wg:h")) != EOF) {

      switch (c) {

         /* Display all products (only valid when not data base query) */
         case 'v':
            Verbose = 1;
            arg = 1;
            break;

         case 's':
            Product_header_stripped = 0;
            arg = 1;
            break;

         case 'w':
            Strip_WMO_product_header = 1;
            Product_header_stripped = 1;
            arg = 1;
            break;

         case 'D':
            strncpy (Dir_name, optarg, LOCAL_NAME_SIZE);
            Dir_name[LOCAL_NAME_SIZE - 1] = '\0';
            fprintf( stderr, "Directory Name: %s\n", Dir_name );
            arg = 1;
            break;

         case 'g':
            strncpy (Match_str, optarg, MATCH_STR_SIZE);
            Match_str[MATCH_STR_SIZE - 1] = '\0';
            fprintf( stderr, "Match String: %s\n", Match_str );
            arg = 1;
            Str_match = 1;
            break;

         /* Print out the usage information. */
         case 'h':
         default:
            printf ("Usage: %s [options] [file_name]\n", argv[0]);
            printf ("       Options:\n");
            printf ("       -v Verbose Mode\n");
            printf ("       -s Contains ORPG Product Header\n");
            printf ("       -w Strip WMO Header\n");
            printf ("       -D Directory\n" );
            printf ("       -g Match String\n" );

            printf ("\n\nNote 1:  File Name Cannot be Specified if -D Option Specified\n" );
            printf ("\n\nIf no file name specified, tool assumes directory.\n" ); 
            printf ("If directory not specified, current directory is assumed.\n" );
            printf ("\nIf a file name is specified, it is assumed this file has an ORPG\n" );
            printf ("product header.  Use -s option if ORPG header needs to be stripped.\n" );
            printf ("That is, the product containis an ORPG header.\n");
            printf ("Use -w option if file contains WMO header and needs to be stripped.\n");
            printf ("If -w specified, -s is assumed. \n" );
            exit(0);

      }

   }

   /* Get file_name, if specified.  */
   strcpy (File_name, "");
   if( optind == (argc - 1) ){

      strncpy (File_name, argv[optind], FILE_NAME_SIZE);
      File_name[FILE_NAME_SIZE - 1] = '\0';

   }

   /* If file name specified and directory specified, report an error. */
   if( (strlen( File_name ) > 0) && (strlen( Dir_name ) > 0) ){

      fprintf( stderr, "File Name Cannot be Specified If Directory Option Used\n" );
      exit(0);

   }

   return arg;

} /* End of Read_options() */

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
static void Process_product_data( char *buf ){

   int i, compr;
   Graphic_product *phd_p = NULL;
   Symbology_block *sym_p = NULL;
   Graphic_alpha_block *gra_p = NULL;
   Tabular_alpha_block *tab_p = NULL;
   char  *cpt = NULL, *temp = NULL;

   /* Set pointer to beginning of product ... the ICD Product Header Block. */
   cpt = buf;

   /* Process Message Header and Product Description Block. */
   phd_p = (Graphic_product *) cpt;
   Dump_header_description_block( phd_p );

   /* Check to see whether this product is compressed or not. */
   compr = 0;
   for( i = 0; i < NUM_COMPRESSED; i++ ){

      if( Prod_code == Compressed_prods[i] ){

         compr = 1;
         break;

      }

   }

   /* Product can be compressed.  Check if it is compressed. */
   if( compr ){

      compr = (int) SHORT_BSWAP_L( phd_p->param_8 );
      if( compr < 0 ){

         fprintf( stderr, "\n !!!!!!! Compression Type %d (%d) Not Supported. !!!!!!! \n", 
                  compr, phd_p->param_8 );
         fprintf( stderr, "--->Skipping Product.\n" );
         return;

      }

   }

   /* Product is compressed.   Need to decompress. */
   if( compr > 0 ){

      if( (compr == COMPRESSION_BZIP2) || (compr == COMPRESSION_ZLIB) ){

         fprintf( stdout, "\n >>>>>>> Decompressing Product <<<<<<< \n\n" );
         temp = (char *) DSP_decompress_product( buf );

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

   /* Adjust the product pointer. */

   /* Does product have a Symbology Block? */
   if( Sym_off > 0 ){

      /* Process Symbology Block. */ 
      sym_p = (Symbology_block *) (cpt + (Sym_off*sizeof(short)));
      Dump_symbology_block_header( sym_p );

   }

   /* Does product have a Graphic Alphanumeric Block? */
   if( Gra_off > 0 ){

      /* Process Graphic Alphanumeric Block. */ 
      gra_p = (Graphic_alpha_block *) (cpt + (Gra_off*sizeof(short)));
      Dump_graphic_block_header( gra_p ); 

   }

   /* Does product have a Tabular Alphanumeric Block? */
   if( Tab_off > 0 ){

      /* Process Tabular Alphanumeric Block. */ 
      tab_p = (Tabular_alpha_block *) (cpt + (Tab_off*sizeof(short)));
      Dump_tabular_block_header( tab_p ); 

   }

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

         long_dest_len = (unsigned long) dest_len;
         long_src_len = (unsigned long) src_len;

         /* Do the zlib decompression. */
         ret = uncompress( (unsigned char *) (*dest + sizeof(Graphic_product)),
                           &long_dest_len, (unsigned char *) prod_data, long_src_len );

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
   
   fprintf( stdout, KBLU "\n-------------WMO Header-------------\n" RESET );

   /* Write out WMO Header fields. */
   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->form_type, sizeof(wmo_hdr->form_type) );
   fprintf( stdout, "--->Form Type:         %s\n", cpt );
   size += sizeof(wmo_hdr->form_type);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->data_type, sizeof(wmo_hdr->data_type) );
   fprintf( stdout, "--->Data Type:         %s\n", cpt );
   size += sizeof(wmo_hdr->data_type);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->distribution, sizeof(wmo_hdr->distribution) );
   fprintf( stdout, "--->Distribution:      %s\n", cpt );
   size += sizeof(wmo_hdr->distribution);

   size += sizeof(wmo_hdr->space1);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->originator, sizeof(wmo_hdr->originator) );
   fprintf( stdout, "--->Originator:        %s\n", cpt );
   size += sizeof(wmo_hdr->originator);

   size += sizeof(wmo_hdr->space2);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, wmo_hdr->date_time, sizeof(wmo_hdr->date_time) );
   fprintf( stdout, "--->Date/Time:         %s\n", cpt );
   size += sizeof(wmo_hdr->date_time);

   temp = strstr( (char *) &wmo_hdr->extra, CRCRLF  );
   if( temp == NULL )
      return(-1);

   /* Account for the trailing CR/CR/LF. */
   size += ((temp - &wmo_hdr->extra) + strlen( CRCRLF ));

   fprintf( stdout, KBLU "\n-------------AWIPS Header-------------\n" RESET );

   /* Write out AWIPS Header fields. */
   awips_hdr = (AWIPS_header_t *) (temp + strlen( CRCRLF ));

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, awips_hdr->category, sizeof(awips_hdr->category) );
   fprintf( stdout, "--->Category:         %s\n", cpt );
   size += sizeof(awips_hdr->category);

   memset( cpt, 0, sizeof( cpt ) );
   memcpy( cpt, awips_hdr->product, sizeof(awips_hdr->product) );
   fprintf( stdout, "--->Product:          %s\n", cpt );
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


#define BASEYR		2440587

/********************************************************************
  Description:
     This takes as input the modified Julian date, and returns
     the year, month, and day.   

  Inputs:
     julian_date - Modified Julian date.  Assumes day 1 is 
                   Jan 1, 1970.

  Outputs:
     year - whole year
     month - month number
     day - day number

  Note:
     Reference algorithm from Fleigel and Van Flandern,
     Communications of the ACM, Volume 11, no. 10,
     (October 1968), p 657.

  Assumption:
     Day 1 is Jan 1, 1970.  This routine developed for use with
     Julian date as provided in RDA/RPG radial header.

********************************************************************/
static int Julian_to_date( int julian_date, int *year, int *month,
                           int *day ){

   int julian_1;
   int   l, n;

   /* Convert Julian date to base year of Julian calendar */
   julian_1 = BASEYR + julian_date;

   /* Compute year, month, and day */
   l = julian_1 + 68569;
   n = 4*l/146097;
   l = l -  (146097*n + 3)/4;
   *year = 4000*(l+1)/1461001;
   l = l - 1461*(*year)/4 + 31;
   *month = 80*l/2447;
   *day = l - 2447*(*month)/80;
   l = *month/11;
   *month = *month + 2 - 12*l;
   *year = 100*(n - 49) + (*year) + l;

   return 0;

/* End of Julian_to_date() */
}

/******************************************************************************

   Description:
      Function to convert time, in seconds, to hours, minutes, 
      and seconds.

   Inputs:
      time - time, in number of seconds since mignight

   Outputs:
      hour - clock hour (0 - 23)
      minutes - number of minutes past hour (0 - 59)
      seconds - number of seconds past minute (0 - 59)

   Returns:
      Returns 0 on success and -1 on error.

*******************************************************************************/
static int Convert_time( unsigned int time, int *hour, int *minute, int *second ){

   int timevalue = time;

   /* Time value must be in the range 0 - 86400. */
   if( time > 86400 )
     return(-1);

   /* Extract the number of hours. */
   *hour = timevalue/3600;

   /* Extract the number of minutes. */
   timevalue -= (*hour)*3600;
   *minute = timevalue/60;

   /* Extract the number of seconds. */
   timevalue -= (*minute)*60;
   *second = timevalue;

   /* Put hour, minute, and second in range. */
   if( *hour == 24 )
      *hour = 0;

   if( *minute == 60 )
      *minute = 0;

   if( *second == 60 )
      *second = 0;

   return 0;

/* End of Convert_time() */
}


