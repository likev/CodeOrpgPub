/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2012/08/21 13:42:18 $ */
/* $Id: file_to_ldm.c,v 1.5 2012/08/21 13:42:18 ccalvert Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

/* Include files */

#include <rpg_ldm.h>
#include <ctype.h>

/* Defines/enums */

#define MAX_PRINT_MSG_LEN	512
#define	MAX_TIMESTRING_LEN	64

/* Static/global variables */

static char *File_to_transfer = NULL;
static int   LDM_msg_size_max = RPG_LDM_DEFAULT_MSG_SIZE;
static int   LDM_msg_index = RPG_LDM_MISC_FILE_PROD;
static int   LDM_msg_feed_type = RPG_LDM_EXP_FEED_TYPE;
static int   End_of_file_flag = NO;
static int   Segment_number = 0;
static int   Number_of_segments = 0;
static struct stat File_stat;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Verify_file_to_transfer( char * );
static void Verify_ldm_msg_size( char * );
static void Verify_ldm_key( char * );
static void Verify_product_index( char * );
static void Verify_ldm_feed_type( char * );
static void Read_file_to_transfer();
static  int Write_to_LB( char *, int );

/*************************************************************************
 Description: Main function for task.
 *************************************************************************/

int main( int argc, char *argv[] )
{
  /* Read command-line options */
  Parse_cmd_line( argc, argv );

  /* Until end of file, read in and write to outgoing LB. */
  while( End_of_file_flag == NO )
  {
    Read_file_to_transfer();
  }

  /* Free allocated memory. */
  free( File_to_transfer );

  return 0;
}

/************************************************************************
 Description: Print usage of this task.
 ************************************************************************/
 
static void Print_usage( char *task_name )
{
  printf( "Usage: %s [options]\n", task_name );
  printf( "  Required:\n" );
  printf( "  -f file      - file to transfer\n" );
  printf( "  -k key       - LDM key of msg\n" );
  printf( "  -i index     - product index of msg\n" );
  printf( "  Options:\n" );
  printf( "  -s size      - msg size (Default: %d)\n", RPG_LDM_DEFAULT_MSG_SIZE );
  printf( "  -F feed type - LDM feed type of msg (Default: EXP)\n" );
  printf( "  -T task name - only used when invoked by mrpg (optional)\n" );
  printf( "  -h           - print usage info\n" );
  RPG_LDM_print_usage();
  printf( "\n" );
  printf( "  NOTES:\n" );
  printf( "  * either -k or -i is required, but not both\n" );
  printf( "  * file limited to %d characters\n", FILENAME_MAX );
  printf( "  * size is of format xxx(y) where:\n" );
  printf( "      x - digit\n" );
  printf( "      y - b,B = bytes k,K = kilobytes m,M = megabytes\n" );
  printf( "      y is optional and if missing is equivalent to b,B\n" );
  printf( "  * feed type - either EXP or NEXRAD2\n" );
  printf( "\n" );
}

/************************************************************************
 Description: Parse command line and take appropriate action.
 ************************************************************************/

static void Parse_cmd_line( int argc, char *argv[] )
{
  int c = 0;

  RPG_LDM_parse_cmd_line( argc, argv );

  opterr = 0;
  optind = 0;
  while( ( c = getopt( argc, argv, "f:s:k:i:F:T:h" ) ) != EOF )
  {
    switch( c )
    {
      case 'f':
        if( optarg != NULL ){ Verify_file_to_transfer( optarg ); }
        break;
      case 'k':
        if( optarg != NULL ){ Verify_ldm_key( optarg ); }
        break;
      case 'i':
        if( optarg != NULL ){ Verify_product_index( optarg ); }
        break;
      case 's':
        if( optarg != NULL ){ Verify_ldm_msg_size( optarg ); }
        break;
      case 'F':
        if( optarg != NULL ){ Verify_ldm_feed_type( optarg ); }
        break;
      case 'T':
        /* Ignore -T option */
        break;
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
        break;
      case '?':
        LE_send_msg( GL_INFO, "Ignored option %c", (char) c );
        break;
      default:
        LE_send_msg( GL_INFO, "Illegal option %c. Terminate.", (char) c );
        Print_usage( argv[0] );
        exit( RPG_LDM_INVALID_COMMAND_LINE );
        break;
    }
  }

  /* Determine how many segments are needed to send the file */
  Number_of_segments = (int) (File_stat.st_size / LDM_msg_size_max);
  if ( File_stat.st_size % LDM_msg_size_max )
  {
    Number_of_segments++;
  }

  RPG_LDM_print_debug( "Stat file size:     %d", File_stat.st_size );
  RPG_LDM_print_debug( "Max msg size:       %d", LDM_msg_size_max );
  RPG_LDM_print_debug( "Number of segments: %d", Number_of_segments );

  RPG_LDM_print_debug( "Leave Parse_cmd_line()" );
}

/************************************************************************
 Description: Verify file to be transferred.
 ************************************************************************/

static void Verify_file_to_transfer( char *filename_buf )
{
  RPG_LDM_print_debug( "Enter Verify_file_to_transfer()" );

  if( filename_buf == NULL )
  {
    /* Option not on command line */
    LE_send_msg( GL_ERROR, "Option -f not defined. Terminate." );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( strlen( filename_buf ) >= FILENAME_MAX )
  {
    /* Specified file name is too long */
    LE_send_msg( GL_ERROR, "Option -f file name longer than %d. Terminate.", FILENAME_MAX );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( access( filename_buf, F_OK ) < 0 )
  {
    /* Specified file does not exist */
    LE_send_msg( GL_ERROR, "File %s does not exist. Terminate.", filename_buf );
    exit( RPG_LDM_FILE_NOT_EXIST );
  }
  else if( access( filename_buf, R_OK ) < 0 )
  {
    /* Specified file is not readable */
    LE_send_msg( GL_ERROR, "File %s is not readable. Terminate.", filename_buf );
    exit( RPG_LDM_FILE_NOT_READABLE );
  }
  else if( stat( filename_buf, &File_stat ) < 0 )
  {
    /* Failed to get stat of specified file */
    LE_send_msg( GL_ERROR, "Stat of file %s failed. Terminate.", filename_buf );
    exit( RPG_LDM_FILE_STAT_FAILED );
  }
  else if( ( File_to_transfer = (char *) malloc( strlen(filename_buf)+1 ) ) == NULL )
  {
    /* Memory allocation error */
    LE_send_msg( GL_ERROR, "Could not malloc File_to_transfer. Terminate." );
    exit( RPG_LDM_MALLOC_FAILED );
  }

  /* Copy file name to static memory for later use */
  strcpy( File_to_transfer, filename_buf );
  File_to_transfer[strlen(filename_buf)] = '\0';
  RPG_LDM_print_debug( "File to transfer: %s", File_to_transfer );

  RPG_LDM_print_debug( "Leave Verify_file_to_transfer()" );
}

/************************************************************************
 Description: Verify key of LDM message.
 ************************************************************************/

static void Verify_ldm_key( char *msg_key_buf )
{
  RPG_LDM_print_debug( "Enter Verify_ldm_key()" );

  if( msg_key_buf == NULL )
  {
    /* Option not on command line */
    LE_send_msg( GL_ERROR, "Option -k not defined. Terminate." );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( strlen( msg_key_buf ) >= RPG_LDM_MAX_KEY_LEN )
  {
    /* Specified key argument is too long */
    LE_send_msg( GL_ERROR, "Option -k argument longer than %d. Terminate.", RPG_LDM_MAX_KEY_LEN );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }

  RPG_LDM_print_debug( "LDM msg key: %s", msg_key_buf );

  /* Get msg index */
  if( ( LDM_msg_index = RPG_LDM_get_LDM_key_index_from_string( msg_key_buf ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "%s is an invalid key", msg_key_buf );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  RPG_LDM_print_debug( "LDM msg index: %d", LDM_msg_index );

  RPG_LDM_print_debug( "Leave Verify_ldm_key()" );
}

/************************************************************************
 Description: Verify product index of message.
 ************************************************************************/

static void Verify_product_index( char *index_buf )
{
  int index = 0;

  RPG_LDM_print_debug( "Enter Verify_product_index()" );

  if( index_buf == NULL )
  {
    LE_send_msg( GL_ERROR, "index_buf is NULL" );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( !isdigit( index_buf[0] ) )
  {
    LE_send_msg( GL_ERROR, "%s has an invalid index format", index_buf );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  index = atoi( index_buf );

  /* Make sure index is valid */
  if( RPG_LDM_get_LDM_key_string_from_index( index ) == NULL )
  {
    LE_send_msg( GL_ERROR, "%s is an invalid index", index );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  LDM_msg_index = index;
  RPG_LDM_print_debug( "LDM msg index: %d", LDM_msg_index );

  RPG_LDM_print_debug( "Leave Verify_product_index()" );
}

/************************************************************************
 Description: Verify size of LDM message.
 ************************************************************************/

static void Verify_ldm_msg_size( char *msg_size_buf )
{
  int num_bytes = 0;
  int ret = 0;
  char size_type = 'x';

  RPG_LDM_print_debug( "Enter Verify_ldm_msg_size()" );

  if( msg_size_buf == NULL )
  {
    LE_send_msg( GL_INFO, "Option -s does not have a value. Terminate." );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( strlen( msg_size_buf ) >= RPG_LDM_MAX_MSG_SIZE_OPTION_LEN )
  {
    /* Specified size argument is too long */
    LE_send_msg( GL_ERROR, "Option -s argument longer than %d. Terminate.", RPG_LDM_MAX_MSG_SIZE_OPTION_LEN );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( ( ret = sscanf( msg_size_buf, "%d%c", &num_bytes, &size_type ) ) != 2 )
  {
    if( ret != 1 || size_type != 'x' )
    {
      /* Invalid format of argument */
      LE_send_msg( GL_ERROR, "Option -s has invalid format. Terminate." );
      exit( RPG_LDM_INVALID_COMMAND_LINE );
    }
    else if( num_bytes < 1 )
    {
      /* Size must be greater than zero */
      LE_send_msg( GL_ERROR, "Option -s cannot be negative. Terminate." );
      exit( RPG_LDM_INVALID_COMMAND_LINE );
    }
    size_type = 'b';
  }

  /* Determine if size is in bytes, kilobytes, or megabytes */
  switch( size_type )
  {
    case 'b':
    case 'B':
      LDM_msg_size_max = num_bytes;
      break;
    case 'k':
    case 'K':
      LDM_msg_size_max = num_bytes * RPG_LDM_KB;
      break;
    case 'm':
    case 'M':
      LDM_msg_size_max = num_bytes * RPG_LDM_MB;
      break;
    default:
      LE_send_msg( GL_ERROR, "Option -s has illegal size type. Terminate." );
      exit( RPG_LDM_INVALID_COMMAND_LINE );
      break;
  }
  RPG_LDM_print_debug( "Max msg size is %d", LDM_msg_size_max );

  /* Make sure size doesn't exceed maximum allowed */
  if( LDM_msg_size_max > RPG_LDM_MAX_MSG_SIZE )
  {
    LE_send_msg( GL_ERROR, "LDM message size %d exceeds maximum of %d. Terminate.", LDM_msg_size_max, RPG_LDM_MAX_MSG_SIZE );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }

  RPG_LDM_print_debug( "Leave Verify_ldm_msg_size()" );
}

/************************************************************************
 Description: Verify feed type of LDM message.
 ************************************************************************/

static void Verify_ldm_feed_type( char *msg_feed_type_buf )
{
  RPG_LDM_print_debug( "Enter verify_ldm_feed_type()" );

  if( msg_feed_type_buf == NULL )
  {
    /* Option not on command line. Use default of EXP. */
    RPG_LDM_print_debug( "Feed type not on command line. Using default." );
    LDM_msg_feed_type = RPG_LDM_EXP_FEED_TYPE;
  }
  else if( strlen( msg_feed_type_buf ) >= RPG_LDM_MAX_FEED_TYPE_OPTION_LEN )
  {
    /* Specified feed type argument is too long */
    LE_send_msg( GL_ERROR, "Option -F argument longer than %d. Terminate.", RPG_LDM_MAX_FEED_TYPE_OPTION_LEN );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( strcmp( msg_feed_type_buf, "EXP" ) == 0 )
  {
    /* Is EXP feed type */
    LDM_msg_feed_type = RPG_LDM_EXP_FEED_TYPE;
  }
  else if( strcmp( msg_feed_type_buf, "NEXRAD2" ) == 0 )
  {
    /* Is NEXRAD2 feed type */
    LDM_msg_feed_type = RPG_LDM_NEXRAD_FEED_TYPE;
  }
  else
  {
    /* Invalid value of argument */
    LE_send_msg( GL_ERROR, "Option -F is an invalid value. Terminate." );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  RPG_LDM_print_debug( "Feedtype: %s (%d)", msg_feed_type_buf, LDM_msg_feed_type );

  RPG_LDM_print_debug( "Leave verify_ldm_feed_type()" );
}

/************************************************************************
 Description: Read in file to transfer in pre-defined size chunks.
 ************************************************************************/

static void Read_file_to_transfer()
{
  static int file_open = NO;
  static FILE *file_to_transfer_ptr = NULL;
  static char *read_buf = NULL;
  static int file_size = 0;
  static int write_size = 0;
  int data_len = 0;
  int ret = 0;

  RPG_LDM_print_debug( "Enter Read_file_to_transfer()" );

  /* Open file if not already open */
  if( file_open == NO )
  {
    file_open = YES;
    if( ( file_to_transfer_ptr = fopen( File_to_transfer, "r" ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Unable to open %s. Terminate.", File_to_transfer );
      exit( RPG_LDM_FILE_NOT_OPENED );
    }
    RPG_LDM_print_debug( "Successfully opened %s", File_to_transfer );
    if( ( read_buf = malloc( LDM_msg_size_max ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Could not malloc read_buf. Terminate." );
      exit( RPG_LDM_MALLOC_FAILED );
    }
    RPG_LDM_print_debug( "Successfully malloced read_buf (%d)", LDM_msg_size_max );
  }

  /* Read in up to LDM_msg_size_max bytes from file */
  ret = fread( read_buf, sizeof( char ), LDM_msg_size_max, file_to_transfer_ptr );
  RPG_LDM_print_debug( "Read %d bytes", ret );
  if( ret < 0 )
  {
    /* Negative value means an error */
    LE_send_msg( GL_ERROR, "Fread failed (%d). Terminate.", ret );
    exit( RPG_LDM_READ_FAILED );
  }
  else if( ret == 0 )
  {
    /* Zero bytes read isn't good either */
    LE_send_msg( GL_ERROR, "Fread read zero bytes. Terminate." );
    exit( RPG_LDM_READ_FAILED );
  }
  else if( ret < LDM_msg_size_max )
  {
    /* End of file */
    End_of_file_flag = YES;
    RPG_LDM_print_debug( "Setting end of file flag" );
  }
  /* Since read is good, set data length to return value */
  data_len = ret;

  /* Add data length to total file size */
  file_size += data_len;

  /* Write to LB */
  if( ( ret = Write_to_LB( read_buf, data_len ) ) > 0 )
  {
    write_size += ret;
  }

  /* Print total size after reading in last of file */
  if( End_of_file_flag == YES )
  {
    LE_send_msg( GL_INFO, "Total size read %d written %d", file_size, write_size );
  }

  RPG_LDM_print_debug( "Leave Read_file_to_transfer()" );
}

/************************************************************************
 Description: Write LDM msg to outgoing linear buffer.
 ************************************************************************/

static int Write_to_LB( char *data_buf, int data_length )
{
  static RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  static RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  char *product_buffer = NULL;
  int product_buffer_size = 0;
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int msg_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + msg_length;
  int ret = 0;
  int offset = 0;
  static int first_time = YES;

  RPG_LDM_print_debug( "Enter Write_to_LB()" );

  RPG_LDM_print_debug( "Data length:     %d", data_length );
  RPG_LDM_print_debug( "Prod hdr length: %d", prod_hdr_length );
  RPG_LDM_print_debug( "Msg hdr length:  %d", msg_hdr_length );
  RPG_LDM_print_debug( "Message length:  %d", msg_length );
  RPG_LDM_print_debug( "Product length:  %d", product_length );

  /* Increment segment number */
  Segment_number++;

  /* Initialize RPG LDM product header. */
  if( first_time )
  {
    RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
    RPG_LDM_prod_hdr.timestamp = time( NULL );
    RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
    RPG_LDM_prod_hdr.feed_type = LDM_msg_feed_type;
    RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, LDM_msg_index );
  }
  RPG_LDM_prod_hdr.seq_num = Segment_number;
  RPG_LDM_prod_hdr.data_len = msg_length;

  /* Initialize RPG LDM message header. */
  if( first_time )
  {
    RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
    RPG_LDM_msg_hdr.code = LDM_msg_index;
    RPG_LDM_msg_hdr.timestamp = RPG_LDM_prod_hdr.timestamp;
    RPG_LDM_msg_hdr.number_of_segments = Number_of_segments;
  }
  RPG_LDM_msg_hdr.size = data_length;
  RPG_LDM_msg_hdr.segment_number = Segment_number;

  /* Set flag to indicate no longer first time. */
  first_time = NO;

  /* Allocate memory for product. */
  product_buffer_size = product_length;
  RPG_LDM_print_debug( "Product buffer size: %d", product_buffer_size );
  if( ( product_buffer = malloc( product_buffer_size ) ) == NULL )
  {
    /* Memory allocation error */
    LE_send_msg( GL_ERROR, "Could not malloc product_buffer. Terminate." );
    exit( RPG_LDM_MALLOC_FAILED );
  }

  /* Copy header/data into output product buffer */
  offset = 0;
  RPG_LDM_print_debug( "Copy product header into product buffer" );
  memcpy( &product_buffer[offset], &RPG_LDM_prod_hdr, prod_hdr_length );
  offset += prod_hdr_length;
  RPG_LDM_print_debug( "Copy message header into product buffer" );
  memcpy( &product_buffer[offset], &RPG_LDM_msg_hdr, msg_hdr_length );
  offset += msg_hdr_length;
  RPG_LDM_print_debug( "Copy message into product buffer" );
  memcpy( &product_buffer[offset], &data_buf[0], data_length );

  /* Write to outgoing linear buffer */
  if( ( ret = RPG_LDM_write_to_LB( &product_buffer[0] ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "RPG_LDM_write_to_LB failed (%d)", ret );
  }
  else
  {
    LE_send_msg( GL_INFO, "%d bytes written to LB", ret );
  }

  /* Print contents of message (if debug mode) */
  if( RPG_LDM_get_debug_mode() )
  {
    RPG_LDM_print_misc_file_msg( &data_buf[0] );
  }

  /* Free allocated memory. */
  free( product_buffer );
  product_buffer = NULL;

  RPG_LDM_print_debug( "Leave Write_to_LB()" );

  return ret;
}

