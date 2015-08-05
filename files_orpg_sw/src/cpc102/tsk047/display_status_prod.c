#include <stdlib.h>
#include <orpg.h>
#include <prod_gen_msg.h>
#include <packet_28.h>
#include <orpg_product.h>
#include <bzlib.h>
#include <zlib.h>
#include <dsp_def.h>

/* General compression macro definitions. */
#define MIN_BYTES_TO_COMPRESS    1000

/* Macro defintions used by the bzip2 compressor. */
#define RPG_BZIP2_MIN_BLOCK_SIZE_BYTES   100000  /* corresponds to 100 Kbytes */
#define RPG_BZIP2_MIN_BLOCK_SIZE              1  /* corresponds to 100 Kbytes */
#define RPG_BZIP2_MAX_BLOCK_SIZE              9  /* corresponds to 900 Kbytes */
#define RPG_BZIP2_WORK_FACTOR                30  /* the recommended default */
#define RPG_BZIP2_NOT_VERBOSE                 0  /* turns off verbosity */
#define RPG_BZIP2_NOT_SMALL                   0  /* does not use small version */

#define MAX_PROD_SIZE                    300000
#define FILE_NAME_SIZE                      128
#define MSG_PRODUCT_LEN                      60
#define WMO_HEADER_SIZE                      30

#define STATUS_PROD_CODE                    152  /* product code for ASP 
                                                    (Archive III Status Product). */
#define LOCAL_NAME_SIZE			    200
#define MATCH_STR_SIZE			     32

#define REDTEXT				1              
#define REDCODE				"\x1B[1m\x1B[38;5;9m"
#define YLWTEXT				2
#define YLWCODE				"\x1B[1m\x1B[38;5;11m"
#define ORGTEXT 			3
#define ORGCODE 			"\x1B[1m\x1B[38;5;202m"
#define GRNTEXT				4              
#define GRNCODE				"\x1B[1m\x1B[38;5;28m"
#define GRYTEXT                         5
#define GRYCODE                         "\x1B[1m\x1B[38;5;240m"
#define BLKTEXT				6
#define BLKCODE				"\x1B[1m\x1B[38;5;16m"
#define BLUTEXT				7
#define BLUCODE				"\x1B[1m\x1B[38;5;14m"
#define WHTTEXT				8
#define WHTCODE				"\x1B[1m\x1B[38;5;15m"

#define BCKGRND				"\x1B[1m\x1B[48;5;180m"
#define RESETTC				"\x1B[39m"
#define RESET				"\033[0m"


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
static FILE *in_fp = NULL;
static int Verbose = 0;
static int Dump_headers = 0;
static int Product_header_stripped = 0;
static int Strip_WMO_product_header = 0;
static int N_files = 0;
static int Str_match = 0;
static char File_name[FILE_NAME_SIZE];
static char Dir_name[LOCAL_NAME_SIZE];
static char Match_str[MATCH_STR_SIZE];
static Ap_vol_file_t *Vol_files;
static int Add_color = 0;

/* Function Prototypes. */
static int Read_options (int argc, char **argv);
static void Dump_header_description_block( Graphic_product *phd );
static void Dump_symbology_block_header( Symbology_block *sym, int *block_len );
static void Dump_packet28_header( packet_28_t *p28, int *data_len );
static void Dump_status_prod_data( RPGP_product_t *prod );
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
static void Set_color( char *loc, int *color );
static void Print_color( char *loc, int color );

/*\//////////////////////////////////////////////////////////////////

   Description:
      Displays the status log product.

//////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int i, ret, size, command_line_args;
   char *buf = NULL, *status_product = NULL;
   char *temp = NULL;

   command_line_args = Read_options( argc, argv );

   /* If a file name was not specified, assume a directory. 
      If directory was not specified, assume current directory. */
   if( strlen( File_name ) == 0 ){

      if( strlen( Dir_name ) == 0 )
         memcpy( Dir_name, "./", 2 );

      /* Get information about all the files in the directory. */
      N_files = DSPAUX_search_files( Dir_name, &Vol_files );

      /* Do For All ASP files. */
      for( i = 0; i < N_files; i++ ){

         status_product = (char *) malloc( MAX_PROD_SIZE );
         if( status_product == NULL ){

            fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
            exit(0);

         }

         /* Open the file for reading. */
         if( (in_fp = fopen( Vol_files[i].path, "r" )) == NULL ){

            fprintf( stderr, "Couldn't open %s for read\n", Vol_files[i].path );
            return 0;

         }

         /* Read entire status product. */
         size = MAX_PROD_SIZE;
         buf = status_product;

         if( (ret = fread( status_product, 1, size, in_fp )) <= 0 ){

            fprintf( stderr, "Read Error (%d) of Status Product data %s\n", 
                     ret, Vol_files[i].path );
            return 0;

         }

         /* Process according to any command line arguments that may have been
            specified. */
         if( Strip_WMO_product_header || !Product_header_stripped ){

            temp = (char *) malloc( MAX_PROD_SIZE );
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
                  exit(0);

               }

               buf += hdr_size;
               size -= hdr_size;

            }

            if( !Product_header_stripped ){

               buf += sizeof(Prod_header);
               size -= sizeof(Prod_header);

            }


         }

         /* Copy the product data to temp ..... discarding the
            WMO header and ORPG product header if needed. */
         memcpy( temp, buf, size );
         free( status_product );

         status_product = temp;

         /* Process this product. */
         Process_product_data( status_product );
         free( status_product );
         fclose( in_fp );

      }

      return 0;

   }

   /* A file name has been specified. */
   if( strlen( File_name ) > 0 ){

      status_product = (char *) malloc( MAX_PROD_SIZE );
      if( status_product == NULL ){

         fprintf( stderr, "malloc failed for %d bytes\n", MAX_PROD_SIZE );
         exit(0);

      }

      /* This assumes that the Archive III status product is given on the 
         command line. */
      fprintf( stdout, "Opening File: %s\n\n",  File_name );
      if( (in_fp = fopen( File_name, "r" )) == NULL ){

         fprintf( stderr, "Couldn't open %s for read\n", File_name );
         return 0;

      }

      /* Read entire status product. */
      size = MAX_PROD_SIZE;
      buf = status_product;

      if( (ret = fread( status_product, 1, size, in_fp )) <= 0 ){

         fprintf( stderr, "Read Error (%d) of Status Product data %s\n", ret, File_name );
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
         free( status_product );

         status_product = temp;

      }

      Process_product_data( status_product );
      free( status_product );

      return 0;

   }
#ifndef STANDALONE_DSP
   else{

      /* Start with the first message in the product buffer. */
      ret = ORPGDA_seek( STATPROD, 0, LB_FIRST, NULL );
      if( ret < 0 ){

         fprintf( stderr, "Unable to ORPGDA_seek( STATPROD, 0, LB_FIRST ): %d\n", ret );
         return 0;

      }

      while(1){

         char *cpt;

         size = ORPGDA_read( STATPROD, &buf, LB_ALLOC_BUF, LB_NEXT );
         if( size == LB_TO_COME ){

            sleep(10);
            continue;

         }
         else if( size == LB_NOT_FOUND ){

            /* The product might be expired out of the data base.
               Just go to the next product. */
            continue;

         }
         else if( size < 0 ){

            LB_id_t id = ORPGDA_previous_msgid( STATPROD );
            fprintf( stderr, "ORPGDA_read(STATPROD, LB_NEXT) Failed: %d (msg_id: %d)\n", 
                     size, id );
            return 0;

         }

         if( !Product_header_stripped && (size <= sizeof(Prod_header)) ){
         
            fprintf( stderr, "Invalid Product Size: %d\n", size );
            continue;

         }


         /* Strip of the ORPG product header. */
         cpt = buf;
         cpt += sizeof(Prod_header);
         size -= sizeof(Prod_header);

         /* The following function call does all product decoding. */
         Process_product_data( cpt );
         free( buf );

      }

   }
#endif

   return 0;

}

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Dumps information from product header and description blocks.

   Inputs:
      phd - pointer the Graphic_product structure.

////////////////////////////////////////////////////////////////////////\*/
static void Dump_header_description_block( Graphic_product *phd ){

   if( Dump_headers ){

      fprintf( stdout, "----------Message Header-------------------------\n");
      fprintf( stdout, "--->msg_code:   %d\n", SHORT_BSWAP_L( phd->msg_code) );
      fprintf( stdout, "--->msg_date:   %d\n", SHORT_BSWAP_L( phd->msg_date ) );
      fprintf( stdout, "--->msg_time:   %d\n", Get_uint_value( &phd->msg_time ) );
      fprintf( stdout, "--->msg_len:    %d\n", Get_uint_value( &phd->msg_len ) );
      fprintf( stdout, "--->n_blocks:   %d\n\n", SHORT_BSWAP_L( phd->n_blocks ) );

      fprintf( stdout, "----------Product description block----------\n");
      fprintf( stdout, "--->divider:    %d\n", SHORT_BSWAP_L( phd->divider ) );
      fprintf( stdout, "--->latitude:   %d\n", Get_uint_value( &phd->latitude ) );
      fprintf( stdout, "--->longitude:  %d\n", Get_uint_value( &phd->longitude ) );
      fprintf( stdout, "--->height:     %d\n", SHORT_BSWAP_L( phd->height ) );
      fprintf( stdout, "--->code:       %d\n", SHORT_BSWAP_L( phd->prod_code ) );
      fprintf( stdout, "--->op_mode:    %d\n", SHORT_BSWAP_L( phd->op_mode ) );
      fprintf( stdout, "--->vcp_num:    %d\n", SHORT_BSWAP_L( phd->vcp_num ) );
      fprintf( stdout, "--->seq_num:    %d\n", SHORT_BSWAP_L( phd->seq_num ) );
      fprintf( stdout, "--->vol_num:    %d\n", SHORT_BSWAP_L( phd->vol_num ) );
      fprintf( stdout, "--->vol_date:   %d\n", SHORT_BSWAP_L( phd->vol_date ) );
      fprintf( stdout, "--->vol_time:   %u\n", Get_uint_value( &phd->vol_time_ms ) );
      fprintf( stdout, "--->gen_date:   %d\n", SHORT_BSWAP_L( phd->gen_date ) );
      fprintf( stdout, "--->gen_time:   %d\n", Get_uint_value( &phd->gen_time ) );
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
      fprintf( stdout, "--->sym_off:    %d\n", Get_uint_value( &phd->sym_off ) );
      fprintf( stdout, "--->gra_off:    %d\n", Get_uint_value( &phd->gra_off ) );
      fprintf( stdout, "--->tab_off:    %d\n\n", Get_uint_value( &phd->tab_off ) );

   }

} /* End of Dump_header_description_block() */

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Dumps symbology block header information.

   Inputs:
      sym - pointer to symbology block.

   Outputs:
      block_len - Block Length

//////////////////////////////////////////////////////////////////////\*/
static void Dump_symbology_block_header( Symbology_block *sym, int *block_len ){

   *block_len = Get_uint_value( &sym->data_len );

   if( Dump_headers ){

      fprintf( stdout, "----------Symbology Block Header----------\n");
      fprintf( stdout, "--->divider:    %d\n", SHORT_BSWAP_L( sym->divider ) );
      fprintf( stdout, "--->block_id:   %d\n", SHORT_BSWAP_L( sym->block_id ) );
      fprintf( stdout, "--->block_len:  %d\n", Get_uint_value( &sym->block_len ) );
      fprintf( stdout, "--->n_layers:   %d\n", SHORT_BSWAP_L( sym->n_layers ) );
      fprintf( stdout, "--->divider:    %d\n", SHORT_BSWAP_L( sym->layer_divider ) );
      fprintf( stdout, "--->data_len:   %d\n\n", *block_len );

   }

} /* End of Dump_symbology_block_header() */

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Displays the contents of Packet 28 header.

   Inputs:
      p28 - pointer to packet_28_t structure

   Outputs:
      data_len - Data Length

/////////////////////////////////////////////////////////////////////\*/
static void Dump_packet28_header( packet_28_t *p28, int *data_len ){

   *data_len = Get_uint_value( &p28->num_bytes );

   if( Dump_headers ){

      fprintf( stdout, "-------------Packet 28 Header-------------\n");
      fprintf( stdout, "--->code:       %d\n", SHORT_BSWAP_L( p28->code ) );
      fprintf( stdout, "--->data_len:   %d\n\n", *data_len );

   }

} /* End of Dump_packet28_header() */

/*\/////////////////////////////////////////////////////////////////////

   Description:
      Displays the status product messages.

   Inputs:
      prod - pointer to RPGP_product_t structure

/////////////////////////////////////////////////////////////////////\*/
static void Dump_status_prod_data( RPGP_product_t *prod ){

   if( Dump_headers ){

      fprintf( stdout, "----------------RPGP_product_t------------\n" );
      fprintf( stdout, "--->name:          %s\n", prod->name );
      fprintf( stdout, "--->description:   %s\n", prod->description );
      fprintf( stdout, "--->product_id:    %d\n", prod->product_id );
      fprintf( stdout, "--->product_type:  %d\n", prod->type );
      fprintf( stdout, "--->gen_time:      %d\n", prod->gen_time );
      fprintf( stdout, "--->radar name:    %s\n", prod->radar_name );
      fprintf( stdout, "--->latitude:      %f\n", prod->radar_lat );
      fprintf( stdout, "--->longitude:     %f\n", prod->radar_lon );
      fprintf( stdout, "--->height:        %f\n", prod->radar_height );
      fprintf( stdout, "--->volume_time:   %d\n", prod->volume_time );
      fprintf( stdout, "--->elev_time:     %d\n", prod->elevation_time );
      fprintf( stdout, "--->elev_angle:    %f\n", prod->elevation_angle );
      fprintf( stdout, "--->vol_number:    %d\n", prod->volume_number );
      fprintf( stdout, "--->op_mode:       %d\n", prod->operation_mode );
      fprintf( stdout, "--->VCP:           %d\n", prod->vcp );
      fprintf( stdout, "--->elev_number:   %d\n", prod->elevation_number );
      fprintf( stdout, "--->compress:      %d\n", prod->compress_type );
      fprintf( stdout, "--->decomp size:   %d\n", prod->size_decompressed );
      fprintf( stdout, "---># prod parm:   %d\n", prod->numof_prod_params );
      fprintf( stdout, "---># components:  %d\n\n", prod->numof_components );

   }

   /* Process components. */
   if( prod->numof_components > 0 )
      Dump_components( prod->numof_components, prod );

} /* End of Dump_status_prod_data() */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Dumps the component information in Packet 28. 

   Inputs:
      num_comps - number of components.
      prod - pointer to RPGP_product_t structure. 

////////////////////////////////////////////////////////////////////\*/
static void Dump_components( int num_comps, RPGP_product_t *prod ){

   int type, size, color, i, j;
   char *loc, *loc1;
   char *substr = NULL;

   /* Set background color. */
   if( Add_color )
      fprintf( stdout, BCKGRND "\n" );

   for( i = 0; i < num_comps; i++ ){

      type = *((int *) prod->components[i]);    
      if( type == RPGP_TEXT_COMP ){

         if( Dump_headers )
            fprintf( stdout, "----------------RPGP_text_t--------------\n" );

         RPGP_text_t *comp = (RPGP_text_t *) prod->components[i];

         /* Initialize color. */
         color = 0;

         if( comp->numof_comp_params > 0 ){

            if( Verbose )
               fprintf( stdout, "%d,  ", i );

            for( j = 0; j < comp->numof_comp_params; j++ ){

               size = strlen( comp->comp_params[j].attrs );
               if( size > 128 )
                  size = 128;

               loc = strstr( comp->comp_params[j].attrs, "Value=" );
               if( loc != NULL ){

                  loc += strlen( "Value=" );
                  Set_color( loc, &color );

                  loc1 = strstr( loc, ";" );
                  if( loc1 != NULL ){

                     *loc1 = '\0';
                     if( Verbose )
                        fprintf( stdout,"%s,  ", loc ); 

                  }

               }

            } /* End of for loop. */
               
         }

         if( !Str_match )
            Print_color( comp->text, color );

         else{

            if( (substr = strstr( comp->text, Match_str )) != NULL ) 
               Print_color( comp->text, color );

         }

      }
      else if( type == RPGP_RADIAL_COMP ){

         if( Dump_headers )
            fprintf( stdout, "----------------RPGP_radial_t--------------\n" );

         RPGP_radial_t *radial = (RPGP_radial_t *) prod->components[i];

         if( radial->description != NULL )
             fprintf( stdout, "--->Description:      %s\n", radial->description );
         fprintf( stdout, "--->Bin Size:         %f\n", radial->bin_size );
         fprintf( stdout, "--->First Range:      %f\n", radial->first_range ); 
         fprintf( stdout, "--->Num Comp Params:  %d\n", radial->numof_comp_params ); 
         fprintf( stdout, "--->Num Radials:      %d\n", radial->numof_radials ); 

         for( i = 0; i < radial->numof_radials; i++ ){

            RPGP_radial_data_t *data = (RPGP_radial_data_t *) &radial->radials[i];
            RPGP_data_t *vals = (RPGP_data_t *) &(radial->radials[i].bins);

            fprintf( stdout, "---------- Radial %3d ----------\n", i );
            fprintf( stdout, "------>Azimuth:    %f\n", data->azimuth );
            fprintf( stdout, "------>Elevation:  %f\n", data->elevation );
            fprintf( stdout, "------>Width:      %f\n", data->width );
            fprintf( stdout, "------># Bins:     %d\n", data->n_bins );
            fprintf( stdout, "--------->Attr:    %s\n", vals->attrs );

         }

      }
      else
         fprintf( stderr, "Unsupported Component Type\n" ); 

   }

   fprintf( stdout, RESET "\n" );

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
   Add_color = 0;
   Dump_headers = 0;
   Product_header_stripped = 0;
   Strip_WMO_product_header = 0;
   Str_match = 0;
   Dir_name[0] = '\0';
   Match_str[0] = '\0';

   while ((c = getopt (argc, argv, "csvdD:wg:h")) != EOF) {

      switch (c) {

         case 'c':
            Add_color = 1;
            break;

         /* Display all products (only valid when not data base query) */
         case 'v':
            Verbose = 1;
            arg = 1;
            break;

         case 's':
            Product_header_stripped = 1;
            arg = 1;
            break;

         case 'w':
            Strip_WMO_product_header = 1;
            Product_header_stripped = 1;
            arg = 1;
            break;

         case 'd':
            Dump_headers = 1;
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
            printf ("       -s Product Header Stripped\n");
            printf ("       -w Strip WMO Header\n");
            printf ("       -D Directory\n" );
            printf ("       -g Match String\n" );
            printf ("       -d Dump Product Header/Description Block/Packet 28 Header\n");
            printf ("       -c Add Color (Default: No Color)\n" );

            printf ("\n\nNote 1:  File Name Cannot be Specified if -D Option Specified\n" );
            printf ("\nNote 2:  If -D Option Specified, the file nameing convention is assumed:\n" );
            printf ("             ICAO.YYYY_MM_DD_HH_mm.ASP\n" );
            printf ("\n\nIf no file name specified, tool will first read all products\n" ); 
            printf ("in product data store, then display each new product after it is\n" );
            printf ("generated.\n" );
            printf ("If a file name is specified, it is assumed this file has an ORPG\n" );
            printf ("product header.  Use -s option if header has been stripped ... that.\n" );
            printf ("is, the product does not contain the ORPG header.  Use -w option if\n" );
            printf ("file contains WMO header and needs to be stripped.  If -w specified,\n" );
            printf ("-s is assmumed. \n" );
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
   Dump_header_description_block( phd_p );

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

         if( Verbose )
            fprintf( stdout, "\n >>>>>>> Decompressing Status Product <<<<<<< \n\n" );
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

   cpt += sizeof(Graphic_product);
   sym_p = (Symbology_block *) cpt;
   Dump_symbology_block_header( sym_p, &block_length );

   cpt += sizeof(Symbology_block);
   packet28_p = (packet_28_t *) cpt;
   Dump_packet28_header( packet28_p, &data_length );

   cpt += sizeof(packet_28_t);
   if( RPGP_product_deserialize( cpt, data_length, (void *) &prod ) < 0 ){

      fprintf( stderr, "Could Not Deserialize Serialized Data\n" );

      if( temp != NULL )
         free( temp );

      return;

   }

   Dump_status_prod_data( prod );
   RPGP_product_free(prod);
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

   if( Verbose )
      fprintf( stdout, "\n-------------WMO Header-------------\n");

   /* Write out WMO Header fields. */
   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, wmo_hdr->form_type, sizeof(wmo_hdr->form_type) );
      fprintf( stdout, "--->Form Type:         %s\n", cpt );

   }

   size += sizeof(wmo_hdr->form_type);

   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, wmo_hdr->data_type, sizeof(wmo_hdr->data_type) );
      fprintf( stdout, "--->Data Type:         %s\n", cpt );

   }

   size += sizeof(wmo_hdr->data_type);

   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, wmo_hdr->distribution, sizeof(wmo_hdr->distribution) );
      fprintf( stdout, "--->Distribution:      %s\n", cpt );

   }

   size += sizeof(wmo_hdr->distribution);
   size += sizeof(wmo_hdr->space1);

   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, wmo_hdr->originator, sizeof(wmo_hdr->originator) );
      fprintf( stdout, "--->Originator:        %s\n", cpt );

   }

   size += sizeof(wmo_hdr->originator);
   size += sizeof(wmo_hdr->space2);

   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, wmo_hdr->date_time, sizeof(wmo_hdr->date_time) );
      fprintf( stdout, "--->Date/Time:         %s\n", cpt );

   }

   size += sizeof(wmo_hdr->date_time);

   temp = strstr( (char *) &wmo_hdr->extra, CRCRLF  );
   if( temp == NULL )
      return(-1);

   /* Account for the trailing CR/CR/LF. */
   size += ((temp - &wmo_hdr->extra) + strlen( CRCRLF ));

   if( Verbose )
      fprintf( stdout, "\n-------------AWIPS Header-------------\n");

   /* Write out AWIPS Header fields. */
   awips_hdr = (AWIPS_header_t *) (temp + strlen( CRCRLF ));

   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, awips_hdr->category, sizeof(awips_hdr->category) );
      fprintf( stdout, "--->Category:         %s\n", cpt );

   }

   size += sizeof(awips_hdr->category);

   if( Verbose ){

      memset( cpt, 0, sizeof( cpt ) );
      memcpy( cpt, awips_hdr->product, sizeof(awips_hdr->product) );
      fprintf( stdout, "--->Product:          %s\n", cpt );

   }

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

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Sets the color of text based on attribute.

////////////////////////////////////////////////////////////////////////\*/
static void Set_color( char *loc, int *color ){
 
   if( strstr( loc, "RPG INFO" ) != NULL )
      *color = GRYTEXT;

   else if( strstr( loc, "RPG GEN STATUS" ) != NULL )
      *color = BLKTEXT;

   else if( strstr( loc, "RPG WARNING" ) != NULL )
      *color = YLWTEXT;

   else if( strstr( loc, "NB COMMS" ) != NULL )
      *color = GRNTEXT;

   else if( strstr( loc, "RPG MAM ALARM" ) != NULL )
      *color = ORGTEXT;

   else if( strstr( loc, "RDA MAM ALARM" ) != NULL )
      *color = ORGTEXT;

   else if( strstr( loc, "RPG MAR ALARM" ) != NULL )
      *color = YLWTEXT;

   else if( strstr( loc, "RDA MAR ALARM" ) != NULL )
      *color = YLWTEXT;

   else if( strstr( loc, "RPG LS ALARM" ) != NULL )
      *color = BLUTEXT;

   else if( strstr( loc, "RDA SECONDARY ALARM" ) != NULL )
      *color = WHTTEXT;

   else if( strstr( loc, "RDA INOP ALARM" ) != NULL )
      *color = REDTEXT;

   else if( strstr( loc, "RDA ALARM CLEARED" ) != NULL )
      *color = GRNTEXT;

   else if( strstr( loc, "RPG ALARM CLEARED" ) != NULL )
      *color = GRNTEXT;

   else
      *color = BLKTEXT;

/* End of Set_color() */
}


/*\////////////////////////////////////////////////////////////////////////

   Description:
      Writes out the text "text" with color "color".
      If the Add_color flag is set to 0, prints the text in black.

////////////////////////////////////////////////////////////////////////\*/
static void Print_color( char *text, int color ){

   if( !Add_color ){

      fprintf( stdout, "%s", text );
      return;

   }

   fprintf( stdout, BCKGRND );

   if( color == BLKTEXT )
      fprintf( stdout, "%s" RESETTC, text );

   else if( color == GRNTEXT )
      fprintf( stdout, GRNCODE "%s" RESETTC, text );

   else if( color == YLWTEXT )
      fprintf( stdout, YLWCODE "%s" RESETTC, text );

   else if( color == GRYTEXT )
      fprintf( stdout, GRYCODE "%s" RESETTC, text );

   else if( color == ORGTEXT )
      fprintf( stdout, ORGCODE "%s" RESETTC, text );

/* End of Print_color() */
}
