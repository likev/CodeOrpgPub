/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/08/15 19:53:15 $
 * $Id: print_model_msg.c,v 1.1 2012/08/15 19:53:15 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* Include files */

#include <orpg.h>
#include <packet_29.h>
#include <rpgcs_model_data.h>

/* Definitions */

#define	PRINT_NONE			0x00000000
#define	PRINT_ALL			0x0fffffff
#define	PRINT_SIZE			0x00000001
#define	PRINT_META			0x00000002
#define	PRINT_PROD_PARAMS		0x00000004
#define	PRINT_ALL_COMPONENTS		0x00000008
#define	PRINT_SPECIFIC_COMPONENT	0x00000010
#define	PRINT_COMPONENT_DATA		0x10000000
#define	ATTR_NAME_TOKEN			"name="
#define	ATTR_TYPE_TOKEN			"type="
#define	ATTR_VALUE_TOKEN		"value="
#define	ATTR_UNITS_TOKEN		"units="

/* Enumerations */

enum {
  DATA_TYPE_UNKNOWN = -1, 
  DATA_TYPE_INT,
  DATA_TYPE_SHORT,
  DATA_TYPE_BYTE,
  DATA_TYPE_FLOAT,
  DATA_TYPE_DOUBLE,
  DATA_TYPE_STRING,
  DATA_TYPE_UINT,
  DATA_TYPE_USHORT,
  DATA_TYPE_UBYTE
};

/* Static/global variables */

static char LB_file_name[256];
static char *Model_prod = NULL;
static int Component_index = 0;
static unsigned int Print_flag = PRINT_NONE;

/* Function prototypes */

static int  Read_options( int, char *[] );
static void Print_usage( char *[] );
static void Read_model_message();
static void *Decompress_product( char *, int * );
static void *Deserialize_product( char * );
static int  Malloc_dest_buf( int, char ** );
static char *Get_comp_type_string( int );
static char *Get_grid_type_string( int );
static char *Get_product_type_string( int );
static char *Get_time_string( time_t );
static void Print_attrs( char *, char * );
static void Print_data( void *, int, int, int * );
static int  Find_data_type( char * );
static char *Find_attr( char *, char * );


/******************************************************************

   Description: Main function.

******************************************************************/

int main( int argc, char *argv[] )
{
  /* Read options.  Exit on failure. */
  if( Read_options( argc, argv ) != 0 ){ exit( 1 ); }

  Read_model_message();

  exit (0);
}

/**************************************************************************

   Description: Print usage info.

**************************************************************************/

static void Print_usage( char **argv )
{
  printf( "Usage: %s (options)\n", argv[0] );
  printf( "  Options:\n" );
  printf( "    -f str - Filename of input LB\n" );
  printf( "    -a     - Print all product info\n" );
  printf( "    -s     - Print product size info\n" );
  printf( "    -r     - Print product meta data (header)\n" );
  printf( "    -p     - Print product parameters\n" );
  printf( "    -c     - Print product components\n" );
  printf( "    -C int - Print product component with associated index\n" );
  printf( "    -d     - Print product component data\n" );
  printf( "    -h     - Help\n" );
  printf( "\n" );
  printf( "NOTE: -d option is only valid with -a, -c or -C options\n" );
  printf( "\n" );
}

/**************************************************************************

   Description: Read and process command line arguments.

**************************************************************************/

static int Read_options( int argc, char **argv )
{
  int c = 0;
  LB_file_name[0] = '\0';

  while( ( c = getopt( argc, argv, "acC:df:hprs" ) ) != EOF )
  {
    switch( c )
    {
      case 'f':
        /* Get file name of LB to read */
        if( optarg == NULL )
        {
          fprintf( stderr, "Option -f must have argument\n" );
          exit( 1 );
        }
        strcpy( LB_file_name, optarg );
        printf( "LB file name: %s\n", LB_file_name );
        break;

      case 'a':
        Print_flag |= PRINT_ALL;
        break;

      case 'c':
        Print_flag |= PRINT_ALL_COMPONENTS;
        break;

      case 'C':
        Print_flag |= PRINT_SPECIFIC_COMPONENT;
        if( optarg == NULL )
        {
          fprintf( stderr, "Option -C must have an integer argument\n" );
          exit( 1 );
        }
        else if( ( Component_index = atoi( optarg ) ) < 0 )
        {
          fprintf( stderr, "Option -C argument not valid\n" );
          exit( 1 );
        }
        break;

      case 'd':
        Print_flag |= PRINT_COMPONENT_DATA;
        break;

      case 'p':
        Print_flag |= PRINT_PROD_PARAMS;
        break;

      case 'r':
        Print_flag |= PRINT_META;
        break;

      case 's':
        Print_flag |= PRINT_SIZE;
        break;

      case 'h':
      case '?':
        Print_usage( argv );
        break;
    }
  }

  /* Make sure something is printed */
  if( Print_flag == PRINT_NONE )
  {
    fprintf( stderr, "No print options defined\n" );
    exit( 1 );
  }

  /* Make sure LB file name is defined */
  if( strlen( LB_file_name ) == 0 )
  {
    fprintf( stderr, "-f option not defined\n" );
    exit( 1 );
  }

  /* Only print data when certain flags are used */
  if( Print_flag & PRINT_COMPONENT_DATA )
  {
    if( ( Print_flag != PRINT_ALL ) &&
        !( Print_flag & PRINT_ALL_COMPONENTS ) &&
        !( Print_flag & PRINT_SPECIFIC_COMPONENT ) )
    {
      fprintf( stderr, "-d option only allowed with -a, -c or -C options\n" );
      exit( 1 );
    }
  }

  return 0;
}

/******************************************************************

   Description: Read model message from LB.

******************************************************************/
static void Read_model_message()
{
  int ret = 0;
  int lb_id = 0;
  int lb_msg_size = 0;
  int lb_uncompressed_msg_size = 0;
  char *buf = NULL;
  int i,j = 0;
  int data_type = DATA_TYPE_UNKNOWN;
  RPGP_ext_data_t *ext_prod = NULL;
  RPGP_grid_t *ext_grid = NULL;
  RPGP_data_t *ext_data = NULL;

  /* Open LB file passed in on command line */
  if( ( ret = LB_open( LB_file_name, LB_READ, NULL ) ) < 0 )
  {
    fprintf( stderr, "LB_open of %s failed: %d\n", LB_file_name, ret );
    exit( 1 );
  }

  /* Open succeeded, so set return value to LB ID */
  lb_id = ret;

  /* Read model message from LB */
  if( ( ret = LB_read( lb_id, &buf, LB_ALLOC_BUF, ORPGDAT_RUC_DATA_MSG_ID ) ) < 0 )
  {
    fprintf( stderr, "LB_read failed: %d\n", ret );
    exit( 1 );
  }
  else if( ret == 0 )
  {
    fprintf( stderr, "LB_read returned 0\n" );
    exit( 1 );
  }

  /* Read succeeded, so set return value to message size */
  lb_msg_size = ret;

  /* Decompress message, if needed */
  Model_prod = Decompress_product( buf, &ret );
  if( Model_prod != buf )
  {
    /* Message decompressed. Set size of decompressed message. */
    lb_uncompressed_msg_size = ret;
    free( buf );
    buf = Model_prod;
  }
  else
  {
    /* Message not compressed, so uncompressed
       size is same as message size */
    lb_uncompressed_msg_size = lb_msg_size;
  }

  /* Print message size info, if applicable */
  if( Print_flag & PRINT_SIZE )
  {
    printf( "LB msg size:       %d bytes\n", lb_msg_size );
    if( lb_msg_size != lb_uncompressed_msg_size )
    {
      printf( "Uncompressed size: %d bytes\n", ret );
    }
    else
    {
      printf( "Product was not compressed\n" );
    }
  }

  /* Deserialize product. */
  Model_prod = Deserialize_product( buf );
  free( buf );

  /* Check for deserialize failure. */
  if( Model_prod == NULL )
  {
    fprintf( stderr, "Deserialize product failed\n" );
    exit( 1 );
  }

  ext_prod = (RPGP_ext_data_t *) Model_prod;

  /* Print message meta info, if applicable */
  if( Print_flag & PRINT_META )
  {
    printf( "ext_prod->name:              %s\n", ext_prod->name );
    printf( "ext_prod->description:       %s\n", ext_prod->description );
    printf( "ext_prod->product_id:        %d\n", ext_prod->product_id );
    printf( "ext_prod->type:              %s (%d)\n", Get_product_type_string( ext_prod->type ), ext_prod->type );
    printf( "ext_prod->get_time:          %s (%ul)\n", Get_time_string( ext_prod->gen_time ), ext_prod->gen_time );
    printf( "ext_prod->spare[0]:          %d\n", ext_prod->spare[0] );
    printf( "ext_prod->spare[1]:          %d\n", ext_prod->spare[1] );
    printf( "ext_prod->spare[2]:          %d\n", ext_prod->spare[2] );
    printf( "ext_prod->spare[3]:          %d\n", ext_prod->spare[3] );
    printf( "ext_prod->spare[4]:          %d\n", ext_prod->spare[4] );
    printf( "ext_prod->compress_type:     %d\n", ext_prod->compress_type );
    printf( "ext_prod->size_decompressed: %d\n", ext_prod->size_decompressed );
    printf( "ext_prod->numof_prod_params: %d\n", ext_prod->numof_prod_params );
    printf( "ext_prod->numof_components:  %d\n", ext_prod->numof_components );
  }

  /* Print product parameters, if applicable */
  if( Print_flag & PRINT_PROD_PARAMS )
  {
    for( i = 0; i < ext_prod->numof_prod_params; i++ )
    {
      printf( "ext_prod->prod_params[%d]\n", i );
      printf( "    id:    %s\n", ext_prod->prod_params[i].id );
      Print_attrs( "    attrs: ", ext_prod->prod_params[i].attrs );
    }
  }

  /* Loop through all product components */
  for( i = 0; i < ext_prod->numof_components; i++ )
  {
    /* Print this components info, if applicable */
    if( ( Print_flag & PRINT_ALL_COMPONENTS ) ||
        ( ( Print_flag & PRINT_SPECIFIC_COMPONENT ) && Component_index == i ) )
    {
      printf( "ext_prod->components[%d]\n", i );
      ext_grid = (RPGP_grid_t *) ext_prod->components[i];
      if( ext_grid->comp_type != RPGP_GRID_COMP )
      {
        printf( "Component is not Grid data (%d)\n", ext_grid->comp_type );
        continue;
      }
      printf( "    comp_type:         %s\n", Get_comp_type_string( ext_grid->comp_type ) );
      printf( "    n_dimensions:      %d\n", ext_grid->n_dimensions );
      for( j = 0; j < ext_grid->n_dimensions; j++ )
      {
        printf( "    dimension[%d]:     %d\n", j, ext_grid->dimensions[j] );
      }
      printf( "    grid_type:         %s\n", Get_grid_type_string( ext_grid->grid_type ) );
      printf( "    numof_comp_params: %d\n", ext_grid->numof_comp_params );
      for( j = 0; j < ext_grid->numof_comp_params; j++ )
      {
        printf( "    comp_params[%d]\n", j );
        printf( "        id: %s\n", ext_grid->comp_params[j].id );
        Print_attrs( "        attrs: ", ext_grid->comp_params[j].attrs );
      }
      ext_data = (RPGP_data_t *) &ext_grid->data;
      Print_attrs( "    attrs: ", ext_data->attrs );
      /* Print this components data, if applicable */
      if( Print_flag & PRINT_COMPONENT_DATA )
      {
        /* Determine data type of this component */
        if( ( data_type = Find_data_type( ext_data->attrs ) ) < 0 )
        {
          fprintf( stderr, "Unable to find data type\n" );
          exit( 1 );
        }
        /* Print data */
        Print_data( ext_data->data, data_type, ext_grid->n_dimensions, ext_grid->dimensions );
      }
    }
  }
}

/**************************************************************************

   Description: Decompress product and return size.
                Code copied from cpc101/lib004/rpgcs_model_data.c

**************************************************************************/

static void *Decompress_product( char *bufptr, int *size )
{
  int hdr_size, ret;
  short divider;
  unsigned int dest_len, src_len;
  unsigned short alg;
  unsigned short msw, lsw;
  char *prod_data = NULL;
  char *dest = NULL;

  External_data_msg_hdr *ext_hdr =
         (External_data_msg_hdr *) (bufptr + sizeof( Prod_msg_header_icd ));

  /* Check if message has an ICD Message Header.  Handle it  either way. */
  if( (divider = (short) SHORT_BSWAP_L( ext_hdr->divider )) == -1 )
  {
    hdr_size = sizeof( Prod_msg_header_icd );
    prod_data = (char *) (((char *) bufptr) + sizeof( Prod_msg_header_icd ));
  }
  else
  {
    hdr_size = 0;
    prod_data = bufptr;
    ext_hdr = (External_data_msg_hdr *) prod_data;
  }

  /* Get the algorithm used for compression so we know how to decompress. */
  alg = SHORT_BSWAP_L( ext_hdr->comp_type );
  if( (alg != COMPRESSION_BZIP2) && (alg != COMPRESSION_ZLIB) )
  {
    return( (void *) bufptr );
  }

  /* Get the decompressed size of the data packets.  This size is stored in 
      the External Data Message Header. */
  msw = SHORT_BSWAP_L( ext_hdr->decomp_sz_msw );
  lsw = SHORT_BSWAP_L( ext_hdr->decomp_sz_lsw );
  src_len = dest_len = (unsigned int ) (0xffff0000 & ((msw) << 16)) | ((lsw) & 0xffff);

  /* Allocate the destination buffer.  The total size of the buffer to 
     allocate must include sufficient size to hold the decompressed data 
     packets plus the External Data Message Header. */
  if( (dest_len == 0) ||
      ((ret = Malloc_dest_buf( dest_len + sizeof(External_data_msg_hdr), &dest )) < 0) )
  {
    return( NULL );
  }

  /* Do the decompression based on the algorithm used. */
  switch( alg )
  {
    case COMPRESSION_BZIP2:
    {
      /* Do the bzip2 decompression. */
      dest_len = MISC_decompress( MISC_BZIP2, prod_data + sizeof(External_data_msg_hdr), src_len,
                                     dest + sizeof(External_data_msg_hdr), dest_len );

      /* Process Non-Normal return. */
      if( dest_len < 0 )
      {
        /* Free the destination buffer. */
        free(dest);
        *size = 0;
        return( NULL );
      }
      break;
    }

    case COMPRESSION_ZLIB:
    {
      /* Do the zlib decompression. */
      dest_len = MISC_decompress( MISC_GZIP, prod_data + sizeof(External_data_msg_hdr), src_len,
                                     dest + sizeof(External_data_msg_hdr), dest_len );

      /* Process Non-Normal return. */
      if( dest_len < 0 )
      {
        /* Free the destination buffer. */
        free(dest);
        *size = 0;
        return( NULL );
      }
      break;
    }

    default:
    {
      fprintf( stderr, "Decompression Method Not Supported (%x)\n", alg );
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
}

/**************************************************************************

   Description: Malloc destination buffer.
                Code copied from cpc101/lib004/rpgcs_model_data.c

**************************************************************************/

static int Malloc_dest_buf( int dest_len, char **dest_buf )
{
  /* Allocate an output buffer the same size as the original 
      (i.e., uncompressed) data. */
  *dest_buf = malloc( dest_len );
  if( *dest_buf == NULL )
  {
    fprintf( stderr, "malloc Failed For %d Bytes\n", dest_len );
    return( -1 );
  }

  return 0;
}

/**************************************************************************

   Description: Deserialize product.
                Code copied from cpc101/lib004/rpgcs_model_data.c

**************************************************************************/

static void* Deserialize_product( char *data )
{
  int ret, size;
  unsigned short size_msw, size_lsw;
  char *serialized_data, *deserialized_data;
  packet_29_t *packet29 = (packet_29_t *) (data + sizeof(External_data_msg_hdr));

  /* Check to make sure the data is either packet 28 or 29 */
  if( ( SHORT_BSWAP_L( packet29->code ) != 28 )
                         &&
      ( SHORT_BSWAP_L( packet29->code ) != 29 ) )
  {
    printf( "Packet code not 28 or 29 (%d)!\n", SHORT_BSWAP_L(packet29->code) );
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

  if( ret < 0 ){ return NULL; }

  return deserialized_data;
}

/**************************************************************************

   Description: Convert component type to string.

**************************************************************************/

static char *Get_comp_type_string( int comp_type )
{
  static char buf[20];

  if( comp_type == RPGP_RADIAL_COMP )
  {
    strcpy( buf, "RADIAL COMPONENT" );
  }
  else if( comp_type == RPGP_GRID_COMP )
  {
    strcpy( buf, "GRID COMPONENT" );
  }
  else if( comp_type == RPGP_AREA_COMP )
  {
    strcpy( buf, "AREA COMPONENT" );
  }
  else if( comp_type == RPGP_TEXT_COMP )
  {
    strcpy( buf, "TEXT COMPONENT" );
  }
  else if( comp_type == RPGP_TABLE_COMP )
  {
    strcpy( buf, "TABLE COMPONENT" );
  }
  else if( comp_type == RPGP_EVENT_COMP )
  {
    strcpy( buf, "EVENT COMPONENT" );
  }
  else
  {
    strcpy( buf, "COMPONENT UNKNOWN" );
  }

  return buf;
}

/**************************************************************************

   Description: Convert grid type to string.

**************************************************************************/

static char *Get_grid_type_string( int grid_type )
{
  static char buf[28];

  if( grid_type == RPGP_GT_ARRAY )
  {
    strcpy( buf, "ARRAY GRID TYPE" );
  }
  else if( grid_type == RPGP_GT_EQUALLY_SPACED )
  {
    strcpy( buf, "EQUALLY SPACED GRID TYPE" );
  }
  else if( grid_type == RPGP_GT_LAT_LON )
  {
    strcpy( buf, "LAT LON GRID TYPE" );
  }
  else if( grid_type == RPGP_GT_POLAR )
  {
    strcpy( buf, "POLAR GRID TYPE" );
  }
  else
  {
    strcpy( buf, "GRID TYPE UNKNOWN" );
  }

  return buf;
}

/**************************************************************************

   Description: Convert product type to string.

**************************************************************************/

static char *Get_product_type_string( int product_type )
{
  static char buf[28];

  if( product_type == RPGP_VOLUME )
  {
    strcpy( buf, "VOLUME PRODUCT TYPE" );
  }
  else if( product_type == RPGP_ELEVATION )
  {
    strcpy( buf, "ELEVATION PRODUCT TYPE" );
  }
  else if( product_type == RPGP_TIME )
  {
    strcpy( buf, "TIME BASED PRODUCT TYPE" );
  }
  else if( product_type == RPGP_ON_DEMAND )
  {
    strcpy( buf, "ON DEMAND PRODUCT TYPE" );
  }
  else if( product_type == RPGP_ON_REQUEST )
  {
    strcpy( buf, "ON REQUEST PRODUCT TYPE" );
  }
  else if( product_type == RPGP_RADIAL )
  {
    strcpy( buf, "RADIAL PRODUCT TYPE" );
  }
  else if( product_type == RPGP_EXTERNAL )
  {
    strcpy( buf, "EXTERNAL PRODUCT TYPE" );
  }
  else
  {
    strcpy( buf, "PRODUCT TYPE UNKNOWN" );
  }

  return buf;
}

/**************************************************************************

   Description: Convert time to string.

**************************************************************************/

static char *Get_time_string( time_t epoch_seconds )
{
  static char buf[64];

  if( epoch_seconds < 1 || ! strftime( buf, 64, "%m/%d/%Y %H:%M:%S", gmtime( &epoch_seconds ) ) )
  {
    sprintf( buf, "\?\?/\?\?/???? ??:??:??" );
  }

  return &buf[0];
}

/**************************************************************************

   Description: Format print of attributes.

**************************************************************************/

static void Print_attrs( char *tag, char *attrs )
{
  char *tok = NULL;
  char *attrs_copy  = NULL;

  /* Create duplicate string to tokenize */
  if( ( attrs_copy = strdup( attrs ) ) == NULL )
  {
    fprintf( stderr, "Unable to strdup %s\n", attrs );
    return;
  }

  /* Attributes are delineated with a semi-colon.
     Loop and print them all. */
  if( ( tok = strtok( attrs_copy, ";" ) ) == NULL )
  {
    printf( "%s ATTR NOT FOUND\n", tag );
  }
  else
  {
    printf( "%s%s\n", tag, tok );
  }
 
  while( ( tok = strtok( NULL, ";" ) ) != NULL )
  {
    printf( "%s%s\n", tag, tok );
  }

  /* Free memory allocated with strdup */
  free( attrs_copy );
}

/**************************************************************************

   Description: Print data.

**************************************************************************/

static void Print_data( void *data, int type, int dim_n, int *dim )
{
  static int data_index = 0;
  int i = 0;
  int loop = 0;

  /* Convert index to number of items for this dimension */
  if( dim_n > 0 ){ loop = dim[dim_n-1]; }

  /* Loop through all items for this dimension */
  for( i = 0; i < loop; i++ )
  {
    if( dim_n != 1 )
    {
      /* Keep calling recursively with the next dimension
         until the last dimension is encountered */
      printf( "ROW: %d\n", i );
      Print_data( data, type, dim_n-1, dim );
      printf( "\n" );
    }
    else
    {
      /* Convert void pointer to appropriate data type */

      switch( type )
      {
        case DATA_TYPE_INT:
        {
          int *arr = (int *) data;
          printf( "%d ", arr[data_index] );
          break;
        }

        case DATA_TYPE_SHORT:
        {
          short *arr = (short *) data;
          printf( "%d ", arr[data_index] );
          break;
        }

        case DATA_TYPE_BYTE:
        {
          char *arr = (char *) data;
          printf( "%c ", arr[data_index] );
          break;
        }

        case DATA_TYPE_FLOAT:
        {
          float *arr = (float *) data;
          printf( "%f ", arr[data_index] );
          break;
        }

        case DATA_TYPE_DOUBLE:
        {
          double *arr = (double *) data;
          printf( "%g ", arr[data_index] );
          break;
        }

        case DATA_TYPE_STRING:
        {
          fprintf( stderr, "Can't print data of STRING type\n" );
/*
          char *arr = (char *) data;
          printf( "%s ", &arr[data_index] );
*/
          break;
        }

        case DATA_TYPE_UINT:
        {
          unsigned int *arr = (unsigned int *) data;
          printf( "%ud ", arr[data_index] );
          break;
        }

        case DATA_TYPE_USHORT:
        {
          unsigned short *arr = (unsigned short *) data;
          printf( "%ud ", arr[data_index] );
          break;
        }

        case DATA_TYPE_UBYTE:
        {
          unsigned char *arr = (unsigned char *) data;
          printf( "%ud ", arr[data_index] );
          break;
        }

        default:
        case DATA_TYPE_UNKNOWN:
        {
          fprintf( stderr, "Can't print data of UNKNOWN type\n" );
          exit( 1 );
        }
      }
      data_index++;
    }
  }
}

/**************************************************************************

   Description: Find and set type of data from attributes.

**************************************************************************/

static int Find_data_type( char *attrs )
{
  char *attr = NULL;

  if( ( attr = Find_attr( ATTR_TYPE_TOKEN, attrs ) ) == NULL )
  {
    return DATA_TYPE_UNKNOWN;
  }

  if( strstr( attr, "int" ) != NULL )
  {
    return DATA_TYPE_INT;
  }
  else if( strstr( attr, "short" ) != NULL )
  {
    return DATA_TYPE_SHORT;
  }
  else if( strstr( attr, "byte" ) != NULL )
  {
    return DATA_TYPE_BYTE;
  }
  else if( strstr( attr, "float" ) != NULL )
  {
    return DATA_TYPE_FLOAT;
  }
  else if( strstr( attr, "double" ) != NULL )
  {
    return DATA_TYPE_DOUBLE;
  }
  else if( strstr( attr, "string" ) != NULL )
  {
    return DATA_TYPE_STRING;
  }
  else if( strstr( attr, "uint" ) != NULL )
  {
    return DATA_TYPE_UINT;
  }
  else if( strstr( attr, "ushort" ) != NULL )
  {
    return DATA_TYPE_USHORT;
  }
  else if( strstr( attr, "ubyte" ) != NULL )
  {
    return DATA_TYPE_UBYTE;
  }

  fprintf( stderr, "Data type :%s: not defined\n", attr );
  return DATA_TYPE_UNKNOWN;
}

/**************************************************************************

   Description: Find token from attributes.

**************************************************************************/

static char *Find_attr( char *token_to_find, char *attrs )
{
  char *tok = NULL;
  char *attrs_copy  = NULL;
  static char token_to_return[128];

  /* Create duplicate string to tokenize */
  if( ( attrs_copy = strdup( attrs ) ) == NULL )
  {
    fprintf( stderr, "Unable to strdup %s\n", attrs );
    return NULL;
  }

  /* Initialize buffer to return */
  token_to_return[0] = '\0';

  /* Attributes are delineated with a semi-colon.
     Loop and find the desired token. */
  if( ( tok = strtok( attrs_copy, ";" ) ) == NULL )
  {
    /* Attributes string has no semi-colons */
    fprintf( stderr, "Bad format of attrs %s\n", attrs );
  }
  else
  {
    /* Token found. Copy to buffer to return. */
    if( strstr( tok, token_to_find ) != NULL )
    {
      strcpy( tok, token_to_return );
    }
    else
    {
      while( ( tok = strtok( NULL, ";" ) ) != NULL )
      {
        /* Token found. Copy to buffer to return. */
        if( strstr( tok, token_to_find ) != NULL )
        {
          strcpy( token_to_return, tok );
          break;
        }
      }
    }
  }

  /* Free memory allocated with strdup */ 
  free( attrs_copy );

  return &token_to_return[0];
}

