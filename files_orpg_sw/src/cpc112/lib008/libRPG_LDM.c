/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:29:35 $
 * $Id: libRPG_LDM.c,v 1.8 2012/10/30 12:29:35 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* Include files */

#include <rpg_ldm.h>
#include <bzlib.h>
#include <ctype.h>
#include <nl2.h>
#include <rda_rpg_messages.h>

/* Defines/enums */

/* Static/global variables */

static int  Debug_mode = NO;
static int  Compress_msg_flag = NO;
static int  Encrypt_msg_flag = NO;
static int  Num_le_log_lines = RPG_LDM_NUM_LE_LOG_LINES;
static int  RPG_ICAO_index = -1;
static char *RPG_ICAO_string = NULL;
static float  RPG_build_number = -1;
static char RPG_build_string[ RPG_LDM_MAX_RPG_BUILD_STRING ];
static int  RPG_node_index = -1;
static char *RPG_node_string = NULL;
static int  RPG_channel_number = -1;
static char *RPG_channel_string = NULL;
static int  RPG_server_index = -1;

/* Function prototypes */

static void Signal_trap_handler( int );
static int  Termination_handler( int, int );
static void Initialize_site_parameters();
static void Initialize_site_ICAO();
static void Initialize_site_channel();
static void Initialize_site_node();
static void Initialize_site_RPG_build_number();
static void Initialize_server_info();
static int  Get_node_index();
static char* Convert_SR( char );
static char* Convert_days( int );
static char* Convert_millisecs( long );
static char* Convert_minutes( int );
static void Print_RDA_RPG_msg_hdr( char * );
static char *Print_RPG_statfl( int );
static int  Get_WMO_region( char * );
static char *Get_WMO_cccc( char * );
static char *Get_WMO_bbb2( char * );
static char *Get_WMO_timestring( Graphic_product * );

/************************************************************************
 Description: Parse command line for common options.
 ************************************************************************/

void RPG_LDM_parse_cmd_line( int argc, char *argv[] )
{
  int c = 0;
  int ret = 0;
  char **cmd_line;

  /* Duplicate argv so it isn't modified */
  if( ( cmd_line = malloc( argc * sizeof( char * ) ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "libRPG_LDM: Could not malloc cmd_line" );
    exit( RPG_LDM_MALLOC_FAILED );
  }
  for( c = 0; c < argc; c++ )
  {
    if( ( cmd_line[c] = strdup( argv[c] ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "libRPG_LDM: Could not malloc cmd_line[%d]", c );
      exit( RPG_LDM_MALLOC_FAILED );
    }
  }

  opterr = 0;
  optind = 0;

  while( ( c = getopt( argc, cmd_line, "cel:x" ) ) != EOF )
  {
    switch( c )
    {
      case 'c':
        Compress_msg_flag = YES;
        break; 
      case 'e':
        Encrypt_msg_flag = YES;
        break;
      case 'l':
        if( optarg == NULL )
        {
          LE_send_msg( GL_ERROR, "libRPG_LDM: Option -l must have argument" );
          exit( RPG_LDM_INVALID_COMMAND_LINE );
        }
        else if( ( Num_le_log_lines = atoi( optarg ) ) < 1 )
        {
          LE_send_msg( GL_ERROR, "libRPG_LDM: Option -l argument not valid" );
          exit( RPG_LDM_INVALID_COMMAND_LINE );
        }
        break;
      case 'x':
        Debug_mode = YES;
        break;
      case '?':
      default:
        break;
    }
  }

  /* Free memory allocated for the command line */
  for( c = 0; c < argc; c++ ){ free( cmd_line[c] ); }
  free( cmd_line );

  /* Initialize LE/CS services */
  if( ( ret = ORPGMISC_init( argc, argv, Num_le_log_lines, 0, -1, 0 ) ) < 0 )
  {
    fprintf( stderr, "libRPG_LDM: ORPGMISC_init failed (%d)", ret );
    exit( RPG_LDM_ORPGMISC_INIT_FAILED );
  }
  RPG_LDM_print_debug( "libRPG_LDM: ORPGMISC_init successful" );
  RPG_LDM_print_debug( "libRPG_LDM: CMD ARGS NUM: %d", argc );
  for( c = 0; c < argc; c++ )
  {
    RPG_LDM_print_debug( "libRPG_LDM: CMD OPT %d: %s", c, argv[c] );
  }
  RPG_LDM_print_debug( "libRPG_LDM: Compress flag: %d", Compress_msg_flag );
  RPG_LDM_print_debug( "libRPG_LDM: Encrypt flag: %d", Encrypt_msg_flag );
  RPG_LDM_print_debug( "libRPG_LDM: Num LE log lines: %d", Num_le_log_lines );
  RPG_LDM_print_debug( "libRPG_LDM: Debug flag: %d", Debug_mode );

  /* Register for library specific signals */
  RPG_LDM_print_debug( "libRPG_LDM: Register signal SIGUSR2" );
  signal( SIGUSR2, Signal_trap_handler );

  /* Register for termination signals from the RPG */
  RPG_LDM_print_debug( "libRPG_LDM: Register term handler" );
  if( ORPGTASK_reg_term_handler( Termination_handler ) != 0  )
  {
    LE_send_msg( GL_ERROR, "Could not register termination handler" );
    exit(RPG_LDM_FAILURE);
  }

  /* Initialize site parameters */
  RPG_LDM_print_debug( "libRPG_LDM: Initialize site parameters" );
  Initialize_site_parameters();
}

/************************************************************************
 Description: Print usage of common options.
 ************************************************************************/

void RPG_LDM_print_usage()
{
  printf( "  -c   - compress (bzip2) msg (Default: %d)\n", Compress_msg_flag );
  printf( "  -e   - encrypt msg (not yet implemented)\n" );
  printf( "  -l # - number of lines in log file\n" );
  printf( "  -x   - debug mode (Default: %d)\n", Debug_mode );
  printf( "         SIGUSR2 signal toggles debug mode\n" ); 
}

/************************************************************************
 Description: Handler for trapped signals.
 ************************************************************************/

static void Signal_trap_handler( int signal )
{
  if( signal == SIGUSR2 )
  {
    RPG_LDM_print_debug( "libRPG_LDM: Signal SIGUSR2 received" );
    if( Debug_mode == YES )
    {
      RPG_LDM_print_debug( "libRPG_LDM: Setting debug mode to NO" );
      Debug_mode = NO;
    }
    else
    {
      RPG_LDM_print_debug( "libRPG_LDM: Setting debug mode to YES" );
      Debug_mode = YES;
    }
  }
  else
  {
    LE_send_msg( GL_INFO, "libRPG_LDM: Unhandled signal (%d) received...ignore", signal );
  }
}


/************************************************************************
 Description: Handle task being terminated.
 ************************************************************************/

static int Termination_handler( int signal, int exit_code )
{
  time_t current_time = time(NULL);

  if( exit_code == ORPGTASK_EXIT_NORMAL_SIG )
  {
    LE_send_msg( GL_INFO, "Task terminated normally by signal %d (%s) @ %s\n",
                 signal, ORPGTASK_get_sig_name( signal ),
                 asctime( gmtime( &current_time ) ) );
  }
  else
  {
    LE_send_msg( GL_INFO, "Task terminated abnormally by signal %d (%s) @ %s\n",
                 signal, ORPGTASK_get_sig_name( signal ),
                 asctime( gmtime( &current_time ) ) );
  }

  return 0;
}

/************************************************************************
 Description: Initialize site parameters.
 ************************************************************************/

static void Initialize_site_parameters()
{
  Initialize_site_ICAO();
  Initialize_site_channel();
  Initialize_site_node();
  Initialize_site_RPG_build_number();
  Initialize_server_info();
}

/************************************************************************
 Description: Initialize ICAO for site.
 ************************************************************************/

static void Initialize_site_ICAO()
{
  if( ( RPG_ICAO_string = ORPGMISC_get_site_name( "site" ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "libRPG_LDM: Unable to get ICAO" );
    exit( RPG_LDM_FAILURE );
  }
  RPG_ICAO_index = RPG_LDM_get_ICAO_index_from_string( RPG_ICAO_string );
  RPG_LDM_print_debug( "libRPG_LDM: Initializing ICAO to %s (%d)", RPG_ICAO_string, RPG_ICAO_index );
}

/************************************************************************
 Description: Initialize channel for site.
 ************************************************************************/

static void Initialize_site_channel()
{
  if( ( RPG_channel_string = ORPGMISC_get_site_name( "channel_num" ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "libRPG_LDM: Unable to get channel" );
    exit( RPG_LDM_FAILURE );
  }
  RPG_channel_number = atoi( RPG_channel_string );
  RPG_LDM_print_debug( "libRPG_LDM: Initializing channel to %s (%d)", RPG_channel_string, RPG_channel_number );
}

/************************************************************************
 Description: Initialize node for site.
 ************************************************************************/

static void Initialize_site_node()
{
  int i = 0;

  if( ( RPG_node_string = ORPGMISC_get_site_name( "type" ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "libRPG_LDM: Unable to get node" );
    exit( RPG_LDM_FAILURE );
  }
  for( i = 0; i < strlen( RPG_node_string ); i++ )
  {
    RPG_node_string[i] = toupper( RPG_node_string[i] );
  }
  RPG_node_index = Get_node_index();
  RPG_LDM_print_debug( "libRPG_LDM: Initializing node to %s (%d)", RPG_node_string, RPG_node_index );
}

/************************************************************************
 Description: Initialize RPG build number for site.
 ************************************************************************/

#define	ORPGMISC_RPG_BUILD_SCALE	10.0
static void Initialize_site_RPG_build_number()
{
  if( ( RPG_build_number = ORPGMISC_RPG_build_number()/ORPGMISC_RPG_BUILD_SCALE ) < 0 )
  {
    LE_send_msg( GL_ERROR, "libRPG_LDM: Unable to get build" );
    exit( RPG_LDM_FAILURE );
  }
  sprintf( RPG_build_string, "%4.1f", RPG_build_number );
  RPG_LDM_print_debug( "libRPG_LDM: Initializing build to %s (%4.1f)", RPG_build_string, RPG_build_number );
}

/************************************************************************
 Description: Initialize RPG server info for site.
 ************************************************************************/

static void Initialize_server_info()
{
  if( RPG_node_index == RPG_LDM_RPGA_NODE_INDEX )
  {
    if( RPG_channel_number == 1 )
    {
      RPG_server_index = RPG_LDM_RPGA_CH1_INDEX;
    }
    else
    {
      RPG_server_index = RPG_LDM_RPGA_CH2_INDEX;
    }
  }
  else if( RPG_node_index == RPG_LDM_RPGB_NODE_INDEX )
  {
    if( RPG_channel_number == 1 )
    {
      RPG_server_index = RPG_LDM_RPGB_CH1_INDEX;
    }
    else
    {
      RPG_server_index = RPG_LDM_RPGB_CH2_INDEX;
    }
  }
  else
  {
    RPG_server_index = RPG_LDM_MSCF_INDEX;
  }
}

/************************************************************************
 Description: Initialize RPG LDM product header struct.
 ************************************************************************/

void RPG_LDM_prod_hdr_init( RPG_LDM_prod_hdr_t *hdr )
{
  RPG_LDM_print_debug( "Initialize RPG_LDM_prod_hdr" );
  hdr->timestamp = RPG_LDM_TIME_INIT;
  hdr->key[0] = '\0';
  hdr->flags = RPG_LDM_PROD_FLAGS_CLEAR;
  hdr->feed_type = RPG_LDM_PROD_FEED_TYPE_INIT;
  hdr->seq_num = 0;
  hdr->data_len = 0;
  hdr->spare148 = 0;
  hdr->spare150 = 0;
  hdr->spare152 = 0;
  hdr->spare154 = 0;
}

/************************************************************************
 Description: Initialize RPG LDM message header struct.
 ************************************************************************/

void RPG_LDM_msg_hdr_init( RPG_LDM_msg_hdr_t *hdr )
{
  RPG_LDM_print_debug( "Initialize RPG_LDM_msg_hdr" );
  hdr->code = RPG_LDM_MSG_CODE_INIT;
  hdr->size = RPG_LDM_MSG_SIZE_INIT;;
  hdr->timestamp = RPG_LDM_TIME_INIT;
  hdr->server_mask = RPG_LDM_get_local_server_mask();
  hdr->build = RPG_LDM_get_scaled_RPG_build_number(); 
  hdr->ICAO[0] = RPG_ICAO_string[0];
  hdr->ICAO[1] = RPG_ICAO_string[1];
  hdr->ICAO[2] = RPG_ICAO_string[2];
  hdr->ICAO[3] = RPG_ICAO_string[3];
  hdr->flags = RPG_LDM_PROD_FLAGS_CLEAR;
  if( RPG_channel_number == 1 )
  {
    hdr->flags |= RPG_LDM_MSG_FLAGS_CHANNEL1;
  }
  else if( RPG_channel_number == 2 )
  {
    hdr->flags |= RPG_LDM_MSG_FLAGS_CHANNEL2;
  }
  hdr->size_uncompressed = RPG_LDM_MSG_SIZE_INIT; /* Set if compressed */
  hdr->segment_number = RPG_LDM_MSG_SEG_NUM_INIT;
  hdr->number_of_segments = RPG_LDM_MSG_NUM_SEGS_INIT;
  hdr->wmo_header_size = 0;
  hdr->spare38 = 0;
  hdr->spare40 = 0;
  hdr->spare42 = 0;
  hdr->spare44 = 0;
  hdr->spare46 = 0;
}

/************************************************************************
 Description: Write message to outgoing LB.
 ************************************************************************/

int RPG_LDM_write_to_LB( char *databuf )
{
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  RPG_LDM_prod_hdr_t *prod_hdr = (RPG_LDM_prod_hdr_t *) &databuf[0];
  RPG_LDM_msg_hdr_t *msg_hdr = (RPG_LDM_msg_hdr_t *) &databuf[prod_hdr_length];
  int data_length = msg_hdr->size;
  int message_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + prod_hdr->data_len;
  int uncompressed_offset = prod_hdr_length + msg_hdr_length;
  char *compressed_data_buf = NULL;
  unsigned int compressed_data_len = 0;
  char *data_ptr = databuf;
  int ret = 0;
  int return_code = 0;

  RPG_LDM_print_debug( "libRPG_LDM: Enter RPG_LDM_write_to_LB" );

  if( prod_hdr->key == NULL )
  {
    LE_send_msg( GL_ERROR, "libRPG_LDM: LDM key is NULL....do nothing" );
    return RPG_LDM_NULL_LDM_KEY;
  }

  if( prod_hdr->feed_type != RPG_LDM_NEXRAD_FEED_TYPE )
  {
    RPG_LDM_print_debug( "Prod hdr length:     %d", prod_hdr_length );
    RPG_LDM_print_debug( "Msg hdr length:      %d", msg_hdr_length );
    RPG_LDM_print_debug( "Data length:         %d", data_length );
    RPG_LDM_print_debug( "Message length:      %d", message_length );
    RPG_LDM_print_debug( "Product length:      %d", product_length );
    RPG_LDM_print_debug( "Uncompressed offset: %d", uncompressed_offset );

    /* Set timestamp of product header */
    if( prod_hdr->timestamp == RPG_LDM_TIME_INIT )
    {
      prod_hdr->timestamp = time( NULL );
    }

    /* Compress data */
    if( Compress_msg_flag == YES )
    {
      msg_hdr->flags |= RPG_LDM_MSG_FLAGS_COMPRESS_BZIP2;
      /* BZ2 docs: buf >= (101% uncompressed size + 600 bytes. Use 10%. */
      compressed_data_len = (unsigned int) (uncompressed_offset + ((data_length*1.1) + 600));
      RPG_LDM_print_debug( "Compressed buf length = %d", compressed_data_len );
      if( ( compressed_data_buf = malloc( compressed_data_len ) ) == NULL )
      {
        LE_send_msg( GL_ERROR, "libRPG_LDM: malloc compressed_buf failed" );
        exit( RPG_LDM_MALLOC_FAILED );
      }
      /* Copy header into compressed buffer, but it will not be compressed */
      memcpy( &compressed_data_buf[0], &databuf[0], uncompressed_offset );

      /* Compress data */
      if( ( ret = BZ2_bzBuffToBuffCompress( &compressed_data_buf[uncompressed_offset], &compressed_data_len, &databuf[uncompressed_offset], data_length, 9, 0, 30 ) ) != BZ_OK )
      {
        LE_send_msg( GL_ERROR, "libRPG_LDM: BZ compression failed (%d)", ret );
        exit( RPG_LDM_COMPRESS_FAILED );
      }
      RPG_LDM_print_debug( "Successfully compressed data from %d to %d", data_length, compressed_data_len );
      /* Set pointer to compressed buffer and update message length */
      data_ptr = compressed_data_buf;
      msg_hdr = (RPG_LDM_msg_hdr_t *) &data_ptr[prod_hdr_length];
      msg_hdr->size_uncompressed = msg_hdr->size;
      msg_hdr->size = compressed_data_len;
      product_length = uncompressed_offset + compressed_data_len;
    }

    /* Encrypt data */
    if( Encrypt_msg_flag == YES )
    {
      LE_send_msg( GL_INFO, "libRPG_LDM: Encryption selected but not yet implemented" );
    }

    /* Print header values if in debug mode */
    if( RPG_LDM_get_debug_mode() )
    {
      RPG_LDM_print_prod_hdr( &data_ptr[0] );
      RPG_LDM_print_msg_hdr( &data_ptr[prod_hdr_length] );
    }
  }

  LE_send_msg( GL_INFO, "libRPG_LDM: Writing %d bytes to LB", product_length );
  if( ( ret = ORPGDA_write( ORPGDAT_LDM_WRITER_INPUT, (char *) &data_ptr[0], product_length, LB_ANY ) ) < 0 )
  {
    /* Write failed */
    LE_send_msg( GL_ERROR, "libRPG_LDM: Write to LDM WRITER INPUT LB failed (%d)", ret );
    return_code = RPG_LDM_LB_WRITE_FAILED;
  }

  /* If applicable, free allocated memory */
  if( compressed_data_buf != NULL )
  {
    RPG_LDM_print_debug( "Attempting to free compressed data buffer" );
    free( compressed_data_buf );
    compressed_data_buf = NULL;
  }

  /* All went well, so return size of data written to LB */
  return_code = product_length;

  RPG_LDM_print_debug( "libRPG_LDM: Write to LB (%d)", return_code );

  return return_code;
}

/************************************************************************
 Description: Print debug statements (if enabled) to task log file.
 ************************************************************************/

void RPG_LDM_print_debug( const char *format, ... )
{
  char buf[RPG_LDM_MAX_PRINT_MSG_LEN];
  va_list arg_ptr;

  if( Debug_mode == YES )
  {
    /* Extract print format */
    va_start( arg_ptr, format );
    vsprintf( buf, format, arg_ptr );
    va_end( arg_ptr );
    LE_send_msg( GL_INFO, "DEBUG: %s", buf );
  }
}

/**************************************************************************
 Description: This function converts node char to integer index.
**************************************************************************/

static int Get_node_index()
{
  if( strcmp( RPG_node_string, "RPGA" ) == 0 )
  {
    return RPG_LDM_RPGA_NODE_INDEX;
  }
  else if( strcmp( RPG_node_string, "RPGB" ) == 0 )
  {
    return RPG_LDM_RPGB_NODE_INDEX;
  }
  return RPG_LDM_MSCF_NODE_INDEX;
}

/**************************************************************************
 Description: This function returns LDM key of associated message index.
**************************************************************************/

char *RPG_LDM_get_LDM_key_string_from_index( int index )
{
  return ROC_L2_get_LDM_key_string_from_index( index );
}

/**************************************************************************
 Description: This function returns LDM key of associated message index.
**************************************************************************/

int RPG_LDM_get_LDM_key_index_from_string( char *key )
{
  return ROC_L2_get_LDM_key_index_from_string( key );
}

/**************************************************************************
 Description: This function returns integer index of ICAO.
**************************************************************************/

int RPG_LDM_get_local_ICAO_index()
{
  return RPG_ICAO_index;
}

/**************************************************************************
 Description: This function returns string of ICAO.
**************************************************************************/

char *RPG_LDM_get_local_ICAO()
{
  return RPG_ICAO_string;
}

/**************************************************************************
 Description: This function returns index of ICAO associated with string.
**************************************************************************/

int RPG_LDM_get_ICAO_index_from_string( char *icao  )
{
  return ROC_L2_get_ICAO_index_from_string( icao );
}

/**************************************************************************
 Description: This function returns string of ICAO associated with index.
**************************************************************************/

char *RPG_LDM_get_ICAO_string_from_index( int index )
{
  return ROC_L2_get_ICAO_string_from_index( index );
}

/**************************************************************************
 Description: This function returns integer of channel number.
**************************************************************************/

int RPG_LDM_get_channel_number()
{
  return RPG_channel_number;
}

/**************************************************************************
 Description: This function returns string of channel number.
**************************************************************************/

char *RPG_LDM_get_channel_string()
{
  return RPG_channel_string;
}

/**************************************************************************
 Description: This function returns integer index of node.
**************************************************************************/

int RPG_LDM_get_node_index()
{
  return RPG_node_index;
}

/**************************************************************************
 Description: This function returns string of node.
**************************************************************************/

char *RPG_LDM_get_node_string()
{
  return RPG_node_string;
}

/**************************************************************************
 Description: This function returns index of local server.
**************************************************************************/

int RPG_LDM_get_local_server_index()
{
  return RPG_server_index;
}

/**************************************************************************
 Description: This function returns mask of local server.
**************************************************************************/

int RPG_LDM_get_local_server_mask()
{
  return ROC_L2_get_server_mask_from_index( RPG_server_index );
}

/**************************************************************************
 Description: This function returns string of local server.
**************************************************************************/

char *RPG_LDM_get_local_server_string()
{
  return ROC_L2_get_server_string_from_index( RPG_server_index );
}

/**************************************************************************
 Description: This function returns index of server string.
**************************************************************************/

int RPG_LDM_get_server_index_from_string( char *buf )
{
  return ROC_L2_get_server_index_from_string( buf );
}

/**************************************************************************
 Description: This function returns index of server mask.
**************************************************************************/

int RPG_LDM_get_server_index_from_mask( int mask )
{
  return ROC_L2_get_server_index_from_mask( mask );
}

/**************************************************************************
 Description: This function returns mask of server string.
**************************************************************************/

int RPG_LDM_get_server_mask_from_string( char *buf )
{
  return ROC_L2_get_server_mask_from_string( buf );
}

/**************************************************************************
 Description: This function returns mask of server index.
**************************************************************************/

int RPG_LDM_get_server_mask_from_index( int index )
{
  return ROC_L2_get_server_mask_from_index( index );
}

/**************************************************************************
 Description: This function returns string of server index.
**************************************************************************/

char *RPG_LDM_get_server_string_from_index( int index )
{
  return ROC_L2_get_server_string_from_index( index );
}

/**************************************************************************
 Description: This function returns string of server mask.
**************************************************************************/

char *RPG_LDM_get_server_string_from_mask( int mask )
{
  return ROC_L2_get_server_string_from_mask( mask );
}

/**************************************************************************
 Description: This function returns RPG build number as float.
**************************************************************************/

float RPG_LDM_get_RPG_build_number()
{
  return RPG_build_number;
}

/**************************************************************************
 Description: This function returns RPG build number as scaled integer.
**************************************************************************/

int RPG_LDM_get_scaled_RPG_build_number()
{
  return (int) ( RPG_build_number * RPG_LDM_BUILD_SCALE );
}

/**************************************************************************
 Description: This function returns string of RPG build.
**************************************************************************/

char *RPG_LDM_get_RPG_build_string()
{
  return RPG_build_string;
}

/**************************************************************************
 Description: This function returns index of LDM feed type string.
**************************************************************************/

int RPG_LDM_get_feed_type_index_from_string( char *buf )
{
  return ROC_L2_get_feed_type_index_from_string( buf );
}

/**************************************************************************
 Description: This function returns index of LDM feed type code.
**************************************************************************/

int RPG_LDM_get_feed_type_index_from_code( int code )
{
  return ROC_L2_get_feed_type_index_from_code( code );
}

/**************************************************************************
 Description: This function returns code of LDM feed type string.
**************************************************************************/

int RPG_LDM_get_feed_type_code_from_string( char *buf )
{
  return ROC_L2_get_feed_type_code_from_string( buf );
}

/**************************************************************************
 Description: This function returns code of LDM feed type index.
**************************************************************************/

int RPG_LDM_get_feed_type_code_from_index( int index )
{
  return ROC_L2_get_feed_type_code_from_index( index );
}

/**************************************************************************
 Description: This function returns string of LDM feed type index.
**************************************************************************/

char *RPG_LDM_get_feed_type_string_from_index( int index )
{
  return ROC_L2_get_feed_type_string_from_index( index );
}

/**************************************************************************
 Description: This function returns string of LDM feed type code.
**************************************************************************/

char *RPG_LDM_get_feed_type_string_from_code( int code )
{
  return ROC_L2_get_feed_type_string_from_code( code );
}

/**************************************************************************
 Description: This function returns string of product header flags.
**************************************************************************/

char *RPG_LDM_get_prod_hdr_flag_string( int flags )
{
  return ROC_L2_get_prod_hdr_flag_string( flags );
}

/**************************************************************************
 Description: This function returns string of message header flags.
**************************************************************************/

char *RPG_LDM_get_msg_hdr_flag_string( int flags )
{
  return ROC_L2_get_msg_hdr_flag_string( flags );
}

/**************************************************************************
 Description: This function returns the debug mode.
**************************************************************************/

int RPG_LDM_get_debug_mode()
{
  return Debug_mode;
}

/**************************************************************************
 Description: This function returns string of passed in time (epoch secs).
**************************************************************************/

char *RPG_LDM_get_timestring( time_t epoch_secs )
{
  static char timebuf[64];

  if( epoch_secs < 1 || ! strftime( timebuf, 64, "%m/%d/%Y %H:%M:%S", gmtime( &epoch_secs ) ) )
  {
    sprintf( timebuf, "\?\?/\?\?/???? ??:??:??" );
  }

  return &timebuf[0];
}

/**************************************************************************
 Description: This function sets the LDM key for given product index.
**************************************************************************/

void RPG_LDM_set_LDM_key( RPG_LDM_prod_hdr_t *prod_hdr, int index )
{
  char timebuf[64];

  strftime( timebuf, 64, "%Y%m%d%H%M%S", gmtime( &(prod_hdr->timestamp) ) );
  sprintf( &(prod_hdr->key[0]), "%s/%s/%s/VCP%d/B%2.1f/%s/CH%1d", RPG_LDM_get_LDM_key_string_from_index( index ), timebuf, RPG_ICAO_string, ORPGVST_get_vcp(), RPG_LDM_get_RPG_build_number(), RPG_node_string, RPG_channel_number );
}

/**************************************************************************
 Description: This function prints a product.
**************************************************************************/

void RPG_LDM_print_msg( char *buf )
{
  RPG_LDM_msg_hdr_t *hdr = ( RPG_LDM_msg_hdr_t *) buf;
  int hdr_size = sizeof( RPG_LDM_msg_hdr_t );

  switch( hdr->code )
  {
    case RPG_LDM_LDMPING_NL2_PROD:
      RPG_LDM_print_ICAO_ldmping_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_ADAPT_PROD:
      RPG_LDM_print_adapt_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_STATUS_LOG_PROD:
      RPG_LDM_print_status_log_msg( &buf[hdr_size], hdr->size );
      break;
    case RPG_LDM_ERROR_LOG_PROD:
      RPG_LDM_print_error_log_msg( &buf[hdr_size], hdr->size );
      break;
    case RPG_LDM_RDA_STATUS_PROD:
      RPG_LDM_print_RDA_status_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_VOL_STATUS_PROD:
      RPG_LDM_print_volume_status_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_WX_STATUS_PROD:
      RPG_LDM_print_wx_status_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_CONSOLE_MSG_PROD:
      RPG_LDM_print_console_msg_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_RPG_INFO_PROD:
      RPG_LDM_print_RPG_info_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_TASK_STATUS_PROD:
      RPG_LDM_print_task_status_msg( &buf[hdr_size], hdr->size );
      break;
    case RPG_LDM_PROD_INFO_PROD:
      RPG_LDM_print_product_info_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_SAVE_LOG_PROD:
      RPG_LDM_print_save_log_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_BIAS_TABLE_PROD:
      RPG_LDM_print_bias_table_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_RUC_DATA_PROD:
      RPG_LDM_print_RUC_data_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_L2_WRITE_STATS_PROD:
      RPG_LDM_print_L2_write_stats_msg( &buf[hdr_size], hdr->size );
      break;
    case RPG_LDM_L2_READ_STATS_PROD:
      RPG_LDM_print_L2_read_stats_msg( &buf[hdr_size], hdr->size );
      break;
    case RPG_LDM_L3_PROD:
      RPG_LDM_print_L3_msg( &buf[hdr_size], hdr->wmo_header_size );
      break;
    case RPG_LDM_SNMP_PROD:
      RPG_LDM_print_snmp_msg( &buf[hdr_size], hdr->size );
      break;
    case RPG_LDM_RTSTATS_PROD:
      RPG_LDM_print_rtstats_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_SECURITY_PROD:
      RPG_LDM_print_security_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_HW_STATS_PROD:
      RPG_LDM_print_hw_stats_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_MISC_FILE_PROD:
      RPG_LDM_print_misc_file_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_REQUEST_PROD:
      RPG_LDM_print_request_msg( &buf[hdr_size] );
      break;
    case RPG_LDM_COMMAND_PROD:
      RPG_LDM_print_command_msg( &buf[hdr_size] );
      break;
    default:
      LE_send_msg( GL_ERROR, "Invalid index (%d)", hdr->code );
      break;
  }
}

/************************************************************************
 Description: Print product header.
 ************************************************************************/

void RPG_LDM_print_prod_hdr( char *buf )
{
  RPG_LDM_prod_hdr_t *prod_hdr = (RPG_LDM_prod_hdr_t *) buf;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "PRODUCT HEADER" ); 
  LE_send_msg( GL_INFO, "Timestamp:   %ld (%s)", prod_hdr->timestamp, RPG_LDM_get_timestring( prod_hdr->timestamp ) ); 
  LE_send_msg( GL_INFO, "Key:         %s", prod_hdr->key ); 
  LE_send_msg( GL_INFO, "Flags:       %d (%s)", prod_hdr->flags, RPG_LDM_get_prod_hdr_flag_string( prod_hdr->flags ) ); 
  LE_send_msg( GL_INFO, "Feed Type:   %d (%s)", prod_hdr->feed_type, RPG_LDM_get_feed_type_string_from_code( prod_hdr->feed_type ) ); 
  LE_send_msg( GL_INFO, "Seq Number:  %d", prod_hdr->seq_num ); 
  LE_send_msg( GL_INFO, "Data Length: %d", prod_hdr->data_len ); 
  LE_send_msg( GL_INFO, "Spare148:    %d", prod_hdr->spare148 ); 
  LE_send_msg( GL_INFO, "Spare150:    %d", prod_hdr->spare150 ); 
  LE_send_msg( GL_INFO, "Spare152:    %d", prod_hdr->spare152 ); 
  LE_send_msg( GL_INFO, "Spare154:    %d", prod_hdr->spare154 ); 
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print message header.
 ************************************************************************/

void RPG_LDM_print_msg_hdr( char *buf )
{
  RPG_LDM_msg_hdr_t *msg_hdr = (RPG_LDM_msg_hdr_t *) buf;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "MESSAGE HEADER" ); 
  LE_send_msg( GL_INFO, "Code:        %d (%s)", msg_hdr->code, RPG_LDM_get_LDM_key_string_from_index( msg_hdr->code ) );
  LE_send_msg( GL_INFO, "Size:        %d bytes", msg_hdr->size );
  LE_send_msg( GL_INFO, "Timestamp:   %ld (%s)", msg_hdr->timestamp, RPG_LDM_get_timestring( msg_hdr->timestamp ) );
  LE_send_msg( GL_INFO, "Server Mask: %d (%s)", msg_hdr->server_mask, RPG_LDM_get_server_string_from_mask( msg_hdr->server_mask ) );
  LE_send_msg( GL_INFO, "Build:       %d", msg_hdr->build );
  LE_send_msg( GL_INFO, "ICAO:        %c%c%c%c", msg_hdr->ICAO[0], msg_hdr->ICAO[1], msg_hdr->ICAO[2], msg_hdr->ICAO[3] );
  LE_send_msg( GL_INFO, "Flags:       %d (%s)", msg_hdr->flags, RPG_LDM_get_msg_hdr_flag_string( msg_hdr->flags ) );
  LE_send_msg( GL_INFO, "Size (U):    %d bytes", msg_hdr->size_uncompressed );
  LE_send_msg( GL_INFO, "Seg Num:     %d", msg_hdr->segment_number );
  LE_send_msg( GL_INFO, "Num Segs:    %d", msg_hdr->number_of_segments );
  LE_send_msg( GL_INFO, "WMO Hdr Size:%d", msg_hdr->wmo_header_size );
  LE_send_msg( GL_INFO, "Spare38:     %d", msg_hdr->spare38 );
  LE_send_msg( GL_INFO, "Spare40:     %d", msg_hdr->spare40 );
  LE_send_msg( GL_INFO, "Spare42:     %d", msg_hdr->spare42 );
  LE_send_msg( GL_INFO, "Spare44:     %d", msg_hdr->spare44 );
  LE_send_msg( GL_INFO, "Spare46:     %d", msg_hdr->spare46 );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print ICAO ldmping message.
 ************************************************************************/

void RPG_LDM_print_ICAO_ldmping_msg( char *buf )
{
  NL2_stats_ICAO_ldmping_NL2_t *msg = (NL2_stats_ICAO_ldmping_NL2_t *) &buf[0];

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "ICAO LDMPING MESSAGE" );
  LE_send_msg( GL_INFO, "Timestamp:    %ld (%s)", msg->timestamp, RPG_LDM_get_timestring( msg->timestamp ) );
  LE_send_msg( GL_INFO, "ICAO:         %d (%s)", msg->ICAO, RPG_LDM_get_ICAO_string_from_index( msg->ICAO ) );
  LE_send_msg( GL_INFO, "Channel:      %d", msg->channel );
  if( msg->ldmping_server_mask & RPG_LDM_NL2_PRIMARY_NODE1_MASK )
  {
    LE_send_msg( GL_INFO, "Ldmping TOC1: YES" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Ldmping TOC1: NO" );
  }
  if( msg->ldmping_server_mask & RPG_LDM_NL2_PRIMARY_NODE2_MASK )
  {
    LE_send_msg( GL_INFO, "Ldmping TOC2: YES" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Ldmping TOC2: NO" );
  }
  if( msg->ldmping_server_mask & RPG_LDM_NL2_SECONDARY_NODE1_MASK )
  {
    LE_send_msg( GL_INFO, "Ldmping ROC1: YES" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Ldmping ROC1: NO" );
  }
  if( msg->ldmping_server_mask & RPG_LDM_NL2_SECONDARY_NODE2_MASK )
  {
    LE_send_msg( GL_INFO, "Ldmping ROC2: YES" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Ldmping ROC2: NO" );
  }
  LE_send_msg( GL_INFO, "Spare10:      %d", msg->spare10 );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print adapt message.
 ************************************************************************/

void RPG_LDM_print_adapt_msg( char *buf )
{
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "ADAPTATION DATA MESSAGE" );
  LE_send_msg( GL_INFO, "Adaptation data message is not supported in this tool" );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print status log message.
 ************************************************************************/

void RPG_LDM_print_status_log_msg( char *buf, int msg_size )
{
/* rpg_ldm.h RPG_LDM_log_msg_t */

  RPG_LDM_log_msg_t *msg = NULL;
  int offset = 0;
  int count = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "STATUS LOG MESSAGE" );
  offset = 0;
  while( offset < msg_size )
  {
    count++;
    msg = (RPG_LDM_log_msg_t *) &buf[offset];
    LE_send_msg( GL_INFO, "Count %3d Code:   %u Length:  %d", count, msg->code, msg->text_len );
    LE_send_msg( GL_INFO, "Count %3d Msg:    %s", count, msg->text );
    offset += sizeof( RPG_LDM_log_msg_t );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print error log message.
 ************************************************************************/

void RPG_LDM_print_error_log_msg( char *buf, int msg_size )
{
/* rpg_ldm.h RPG_msg_t */

  RPG_LDM_log_msg_t *err_msg = NULL;
  int offset = 0;
  int count = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "ERROR LOG MESSAGE" );
  offset = 0;
  while( offset < msg_size )
  {
    count++;
    err_msg = (RPG_LDM_log_msg_t *) &buf[offset];
    LE_send_msg( GL_INFO, "Count %3d Code:   %u Length:  %d", count, err_msg->code, err_msg->text_len );
    LE_send_msg( GL_INFO, "Count %3d Msg:    %s", count, err_msg->text );
    offset += sizeof( RPG_LDM_log_msg_t );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print RDA status message.
 ************************************************************************/

#define	RDA_BUILD_SCALE	100

void RPG_LDM_print_RDA_status_msg( char *buf )
{
/*gen_stat_msg.h ORDA_status_t, RDA_RPG_comms_status_t */
/*rda_status.h ORDA_status_msg_t */
/*rda_rpg_message_header.h RDA_RPG_message_header_t */

  ORDA_status_t *msg = (ORDA_status_t *) &buf[0];
  ORDA_status_msg_t status = msg->status_msg;
  RDA_RPG_comms_status_t comms = msg->wb_comms;
  int i = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "RDA STATUS MESSAGE" );
  LE_send_msg( GL_INFO, "WB COMMS STRUCT" );
  if( comms.wblnstat == RS_CONNECT_PENDING )
  {
    LE_send_msg( GL_INFO, "WB Status:            PENDING CONNECT" );
  }
  else if( comms.wblnstat == RS_DISCONNECT_PENDING )
  {
    LE_send_msg( GL_INFO, "WB Status:            PENDING DISCONNECT" );
  }
  else if( comms.wblnstat == RS_DISCONNECTED_HCI )
  {
    LE_send_msg( GL_INFO, "WB Status:            DISCONNECTED BY HCI" );
  }
  else if( comms.wblnstat == RS_DISCONNECTED_CM )
  {
    LE_send_msg( GL_INFO, "WB Status:            DISCONNECTED BY COMMS MGR" );
  }
  else if( comms.wblnstat == RS_DISCONNECTED_SHUTDOWN )
  {
    LE_send_msg( GL_INFO, "WB Status:            DISCONNECTED BY SHUTDOWN" );
  }
  else if( comms.wblnstat == RS_CONNECTED )
  {
    LE_send_msg( GL_INFO, "WB Status:            CONNECTED" );
  }
  else if( comms.wblnstat == RS_DOWN )
  {
    LE_send_msg( GL_INFO, "WB Status:            DOWN" );
  }
  else if( comms.wblnstat == RS_WBFAILURE )
  {
    LE_send_msg( GL_INFO, "WB Status:            FAILURE" );
  }
  else if( comms.wblnstat == RS_DISCONNECTED_RMS )
  {
    LE_send_msg( GL_INFO, "WB Status:            DISCONNECTED BY RMS" );
  }
  else
  {
    LE_send_msg( GL_INFO, "WB Status:            UNKNOWN (%d)", comms.wblnstat );
  }
  LE_send_msg( GL_INFO, "RDA Display Blanking: %d", comms.rda_display_blanking );
  LE_send_msg( GL_INFO, "WB Failed Flag:       %d", comms.wb_failed );
  Print_RDA_RPG_msg_hdr( &buf[sizeof(RDA_RPG_comms_status_t)] );
  LE_send_msg( GL_INFO, "RDA STATUS STRUCT" );
  if( status.rda_status == RS_STARTUP )
  {
    LE_send_msg( GL_INFO, "RDA Status:           STARTUP" );
  }
  else if( status.rda_status == RS_STANDBY )
  {
    LE_send_msg( GL_INFO, "RDA Status:           STANDBY" );
  }
  else if( status.rda_status == RS_STANDBY )
  {
    LE_send_msg( GL_INFO, "RDA Status:           STANDBY" );
  }
  else if( status.rda_status == RS_RESTART )
  {
    LE_send_msg( GL_INFO, "RDA Status:           RESTART" );
  }
  else if( status.rda_status == RS_OPERATE )
  {
    LE_send_msg( GL_INFO, "RDA Status:           OPERATE" );
  }
  else if( status.rda_status == RS_OFFOPER )
  {
    LE_send_msg( GL_INFO, "RDA Status:           OFFLINE OPERATE" );
  }
  else
  {
    LE_send_msg( GL_INFO, "RDA Status:           UNKNOWN (%d)", status.rda_status );
  }
  if( status.op_status == OS_INDETERMINATE )
  {
    LE_send_msg( GL_INFO, "OP Status:            INDETERMINATE" );
  }
  else if( status.op_status == OS_ONLINE )
  {
    LE_send_msg( GL_INFO, "OP Status:            ONLINE" );
  }
  else if( status.op_status == OS_MAINTENANCE_REQ )
  {
    LE_send_msg( GL_INFO, "OP Status:            MAINTENANCE_REQUIRED" );
  }
  else if( status.op_status == OS_MAINTENANCE_MAN )
  {
    LE_send_msg( GL_INFO, "OP Status:            MAINTENANCE_MANDATORY" );
  }
  else if( status.op_status == OS_COMMANDED_SHUTDOWN )
  {
    LE_send_msg( GL_INFO, "OP Status:            COMMANDED SHUTDOWN" );
  }
  else if( status.op_status == OS_INOPERABLE )
  {
    LE_send_msg( GL_INFO, "OP Status:            INOPERABLE" );
  }
  else if( status.op_status == OS_WIDEBAND_DISCONNECT )
  {
    LE_send_msg( GL_INFO, "OP Status:            WIDEBAND DISCONNECT" );
  }
  else
  {
    LE_send_msg( GL_INFO, "OP Status:            UNKNOWN (%d)", status.op_status );
  }
  if( status.control_status == CS_LOCAL_ONLY )
  {
    LE_send_msg( GL_INFO, "Control Status:       LOCAL" );
  }
  else if( status.control_status == CS_RPG_REMOTE )
  {
    LE_send_msg( GL_INFO, "Control Status:       REMOTE" );
  }
  else if( status.control_status == CS_EITHER )
  {
    LE_send_msg( GL_INFO, "Control Status:       EITHER" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Control Status:       UNKNOWN (%d)", status.control_status );
  }
  if( status.aux_pwr_state == RS_COMMANDED_SWITCHOVER )
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        RDA STATUS COMMANDED SWITCHOVER" );
  }
  else if( status.aux_pwr_state == AP_UTILITY_PWR_AVAIL )
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        UTILITY POWER AVAILABLE" );
  }
  else if( status.aux_pwr_state == AP_GENERATOR_ON )
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        GENERATOR ON" );
  }
  else if( status.aux_pwr_state == AP_TRANS_SWITCH_MAN )
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        TRANSITIONING MANUAL SWITCH" );
  }
  else if( status.aux_pwr_state == AP_COMMAND_SWITCHOVER )
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        AUX PWR COMMAND SWITCHOVER" );
  }
  else if( status.aux_pwr_state == AP_SWITCH_AUX_PWR )
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        SWITCH TO AUXILLARY POWER" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Aux Pwr State:        UNKNOWN (%d)", status.aux_pwr_state );
  }
  LE_send_msg( GL_INFO, "Ave Trans Pwr:        %d Watts", status.ave_trans_pwr );
  LE_send_msg( GL_INFO, "Ref Calib Corr:       %d dB", status.ref_calib_corr );
  if( status.data_trans_enbld & BD_ENABLED_NONE )
  {
    LE_send_msg( GL_INFO, "Data Trans Enabled:   NONE" );
  }
  else
  {
    if( status.data_trans_enbld & BD_REFLECTIVITY )
    {
      LE_send_msg( GL_INFO, "Data Trans Enabled:   REFLECTIVITY" );
    }
    if( status.data_trans_enbld & BD_VELOCITY )
    {
      LE_send_msg( GL_INFO, "Data Trans Enabled:   VELOCITY" );
    }
    if( status.data_trans_enbld & BD_WIDTH )
    {
      LE_send_msg( GL_INFO, "Data Trans Enabled:   SPECTRUM WIDTH" );
    }
  }
  if( status.vcp_num < 0 )
  {
    LE_send_msg( GL_INFO, "VCP:                  %d (LOCAL)", status.vcp_num * -1 );
  }
  else
  {
    LE_send_msg( GL_INFO, "VCP:                  %d (REMOTE)", status.vcp_num );
  }
  if( status.rda_control_auth == CA_NO_ACTION )
  {
    LE_send_msg( GL_INFO, "RDA Control Auth:     NO ACTION" );
  }
  else if( status.rda_control_auth == CA_LOCAL_CONTROL_REQUESTED )
  {
    LE_send_msg( GL_INFO, "RDA Control Auth:     LOCAL_CONTROL_REQUESTED" );
  }
  else if( status.rda_control_auth == CA_REMOTE_CONTROL_ENABLED )
  {
    LE_send_msg( GL_INFO, "RDA Control Auth:     REMOTE CONTROL ENABLED" );
  }
  else
  {
    LE_send_msg( GL_INFO, "RDA Control Auth:     UNKNOWN (%d)", status.rda_control_auth );
  }
  LE_send_msg( GL_INFO, "RDA Build:            %5.2f", (float)(status.rda_build_num/RDA_BUILD_SCALE) );
  if( status.op_mode == OP_MAINTENANCE_MODE )
  {
    LE_send_msg( GL_INFO, "OP Mode:              MAINTENANCE" );
  }
  else if( status.op_mode == OP_OPERATIONAL_MODE )
  {
    LE_send_msg( GL_INFO, "OP Mode:              OPERATIONAL" );
  }
  else if( status.op_mode == OP_OFFLINE_MAINTENANCE_MODE )
  {
    LE_send_msg( GL_INFO, "OP Mode:              OFFLINE MAINTENANCE" );
  }
  else
  {
    LE_send_msg( GL_INFO, "OP Mode:              UNKNOWN (%d)", status.op_mode );
  }
  if( status.super_res == SR_NOCHANGE )
  {
    LE_send_msg( GL_INFO, "Super Res:            NO CHANGE" );
  }
  else if( status.super_res == SR_ENABLED )
  {
    LE_send_msg( GL_INFO, "Super Res:            ENABLED" );
  }
  else if( status.super_res == SR_DISABLED )
  {
    LE_send_msg( GL_INFO, "Super Res:            DISABLED" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Super Res:            UNKNOWN (%d)", status.super_res );
  }
  if( status.cmd == CMD_ENABLED )
  {
    LE_send_msg( GL_INFO, "CMD:                  ENABLED" );
  }
  else if( status.cmd == CMD_DISABLED )
  {
    LE_send_msg( GL_INFO, "CMD:                  DISABLED" );
  }
  else
  {
    LE_send_msg( GL_INFO, "CMD:                  UNKNOWN (%d)", status.cmd );
  }
  if( status.avset == AVSET_ENABLED )
  {
    LE_send_msg( GL_INFO, "AVSET:                ENABLED" );
  }
  else if( status.avset == AVSET_DISABLED )
  {
    LE_send_msg( GL_INFO, "AVSET:                DISABLED" );
  }
  else
  {
    LE_send_msg( GL_INFO, "AVSET:                UNKNOWN (%d)", status.avset );
  }
  if( status.rda_alarm == AS_NO_ALARMS )
  {
    LE_send_msg( GL_INFO, "RDA Alarm:            NONE" );
  }
  else
  {
    if( status.rda_alarm == AS_TOW_UTIL )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            TOWER/UTILITIES" );
    }
    else if( status.rda_alarm == AS_PEDESTAL )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            PEDESTAL" );
    }
    else if( status.rda_alarm == AS_TRANSMITTER )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            TRANSMITTER" );
    }
    else if( status.rda_alarm == AS_RECV )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            RECEIVER" );
    }
    else if( status.rda_alarm == AS_RDA_CONTROL )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            RDA CONTROL" );
    }
    else if( status.rda_alarm == AS_RPG_COMMUN )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            RPG COMMUNICATION" );
    }
    else if( status.rda_alarm == AS_SIGPROC )
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            SIGNAL PROCESSOR" );
    }
    else
    {
      LE_send_msg( GL_INFO, "RDA Alarm:            UNKNOWN (%d)", status.rda_status );
    }
  }
  if( status.command_status == RS_NO_ACKNOWLEDGEMENT )
  {
    LE_send_msg( GL_INFO, "Command Status:       NO ACKNOWLEDGEMENT" );
  }
  else if( status.command_status == RS_REMOTE_VCP_RECEIVED )
  {
    LE_send_msg( GL_INFO, "Command Status:       REMOTE VCP RECEIVED" );
  }
  else if( status.command_status == RS_CLUTTER_BYPASS_MAP_RECEIVED )
  {
    LE_send_msg( GL_INFO, "Command Status:       CLUTTER BYPASS MAP RECEIVED" );
  }
  else if( status.command_status == RS_CLUTTER_CENSOR_ZONES_RECEIVED )
  {
    LE_send_msg( GL_INFO, "Command Status:       CLUTTER CENSOR ZONES RECEIVED" );
  }
  else if( status.command_status == RS_REDUND_CHNL_STBY_CMD_ACCEPTED )
  {
    LE_send_msg( GL_INFO, "Command Status:       RED CHANNEL STANDBY CMD ACCEPTED" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Command Status:       UNKNOWN (%d)", status.command_status );
  }
  if( status.channel_status == RDA_IS_CONTROLLING )
  {
    LE_send_msg( GL_INFO, "Channel Status:       CONTROLLING" );
  }
  else if( status.channel_status == RDA_IS_NON_CONTROLLING )
  {
    LE_send_msg( GL_INFO, "Channel Status:       NON-CONTROLLING" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Channel Status:       UNKNOWN (%d)", status.channel_status );
  }
  if( status.spot_blanking_status == SB_NOT_INSTALLED )
  {
    LE_send_msg( GL_INFO, "Spot Blank Status:    NOT INSTALLED" );
  }
  else if( status.spot_blanking_status == SB_ENABLED )
  {
    LE_send_msg( GL_INFO, "Spot Blank Status:    ENABLED" );
  }
  else if( status.spot_blanking_status == SB_DISABLED )
  {
    LE_send_msg( GL_INFO, "Spot Blank Status:    DISABLED" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Spot Blank Status:    UNKNOWN (%d)", status.spot_blanking_status );
  }
  LE_send_msg( GL_INFO, "Bypass Map Date:      %d (%s)", status.bypass_map_date, Convert_days( status.bypass_map_date ) );
  LE_send_msg( GL_INFO, "Bypass Map Time:      %d (%s)", status.bypass_map_time, Convert_minutes( status.bypass_map_time ) );
  LE_send_msg( GL_INFO, "Clutter Map Date:     %d (%s)", status.clutter_map_date, Convert_days( status.clutter_map_date ) );
  LE_send_msg( GL_INFO, "Clutter Map Time:     %d (%s)", status.clutter_map_time, Convert_minutes( status.clutter_map_time ) );
  LE_send_msg( GL_INFO, "Vert Ref Calib Corr:  %d dB", status.vc_ref_calib_corr );
  if( status.tps_status == TP_NOT_INSTALLED )
  {
    LE_send_msg( GL_INFO, "TPS Status:           NOT INSTALLED" );
  }
  else if( status.tps_status == TP_OFF )
  {
    LE_send_msg( GL_INFO, "TPS Status:           OFF" );
  }
  else if( status.tps_status == TP_OK )
  {
    LE_send_msg( GL_INFO, "TPS Status:           OK" );
  }
  else
  {
    LE_send_msg( GL_INFO, "TPS Status:           UNKNOWN", status.tps_status );
  }
  if( status.rms_control_status == 0 )
  {
    LE_send_msg( GL_INFO, "RMS Control Status:   NON-RMS SYSTEM" );
  }
  else if( status.rms_control_status == 2 )
  {
    LE_send_msg( GL_INFO, "RMS Control Status:   RMS IN CONTROL" );
  }
  else if( status.rms_control_status == 4 )
  {
    LE_send_msg( GL_INFO, "RMS Control Status:   RDA MMI IN CONTROL" );
  }
  else
  {
    LE_send_msg( GL_INFO, "RMS Control Status:   UNKNOWN (%d)", status.rms_control_status );
  }
  LE_send_msg( GL_INFO, "Perf Check Status:      %d", status.perf_check_status );
  for( i = 0; i < MAX_RDA_ALARMS_PER_MESSAGE; i++ )
  {
    LE_send_msg( GL_INFO, "Alarm Code %02d:      %d", i+1, status.alarm_code[i] );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print volume status message.
 ************************************************************************/

#define	ELEVATION_SCALE			10.0
#define	BAMS_SCALE			180.0/32768 /* degrees */
#define	AZ_RATE_SCALE			22.5/16384 /* degrees/sec */
#define	THRESHOLD_PARAM_DB_SCALE	8	/* 1/8 dB */

void RPG_LDM_print_volume_status_msg( char *buf )
{
/* gen_stat_msg.h Vol_stat_gsm_t */
/* vcp.h Ele_attr */

  Ele_attr *ele_attr = NULL;
  int i = 0;

  Vol_stat_gsm_t *msg = (Vol_stat_gsm_t *) &buf[0];

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "VOLUME STATUS MESSAGE" );
  LE_send_msg( GL_INFO, "Volume number:     %ld", msg->volume_number );
  LE_send_msg( GL_INFO, "Volume scan time:  %ld (%s)", msg->cv_time, Convert_millisecs( msg->cv_time ) );
  LE_send_msg( GL_INFO, "Volume scan date:  %d (%s)", msg->cv_julian_date, Convert_days( msg->cv_julian_date ) );
  if( msg->initial_vol )
  {
    LE_send_msg( GL_INFO, "Initial volume:    Yes" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Initial volume:    No" );
  }
  if( msg->pv_status )
  {
    LE_send_msg( GL_INFO, "Previous status:   Completed" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Previous status:   Aborted" );
  }
  LE_send_msg( GL_INFO, "Expected duration: %d seconds", msg->expected_vol_dur );
  LE_send_msg( GL_INFO, "Volume Scan:       %d", msg-> volume_scan );
  if( msg-> mode_operation == 1 )
  {
    LE_send_msg( GL_INFO, "Mode:              Clear Air" );
  }
  else if( msg-> mode_operation == 2 )
  {
    LE_send_msg( GL_INFO, "Mode:              Precipitation" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Mode:              Maintenance" );
  }
  if( msg-> dual_pol_expected )
  {
    LE_send_msg( GL_INFO, "DP expected:       Yes" );
  }
  else
  {
    LE_send_msg( GL_INFO, "DP expected:       No" );
  }
  LE_send_msg( GL_INFO, "VCP:               %d", msg-> vol_cov_patt ); 
  LE_send_msg( GL_INFO, "VCP table slot:    %d", msg-> rpgvcpid );
  LE_send_msg( GL_INFO, "Num elev in VCP:   %d", msg-> num_elev_cuts );
  for( i = 0; i < msg->num_elev_cuts; i++ )
  {
    LE_send_msg( GL_INFO, "Index / Elev:      %02d / %-6.2f deg", msg->elev_index[i], msg->elevations[i]/ELEVATION_SCALE );
  }
  LE_send_msg( GL_INFO, "SR cut mask:       %d", msg->super_res_cuts );
  LE_send_msg( GL_INFO, "VCP Str size:      %d half-words", msg->current_vcp_table.msg_size );
  if( msg->current_vcp_table.type == 2 )
  {
    LE_send_msg( GL_INFO, "VCP Str type:      Constant Elevation Cut" );
  }
  else if( msg->current_vcp_table.type == 4 )
  {
    LE_send_msg( GL_INFO, "VCP Str type:      Horizontal Raster Scan" );
  }
  else if( msg->current_vcp_table.type == 8 )
  {
    LE_send_msg( GL_INFO, "VCP Str type:      Vertical Raster Scan" );
  }
  else if( msg->current_vcp_table.type == 16 )
  {
    LE_send_msg( GL_INFO, "VCP Str type:      Searchlight" );
  }
  LE_send_msg( GL_INFO, "VCP Str VCP:       %d", msg->current_vcp_table.vcp_num );
  LE_send_msg( GL_INFO, "VCP Str #elev:     %d", msg->current_vcp_table.n_ele );
  LE_send_msg( GL_INFO, "VCP Str CltMap #:  %d", msg->current_vcp_table.clutter_map_num );
  if( (int) msg->current_vcp_table.vel_resolution == 2 )
  {
    LE_send_msg( GL_INFO, "VCP Str Vel Res:   0.5 m/s" );
  }
  else
  {
    LE_send_msg( GL_INFO, "VCP Str Vel Res:   1.0 m/s" );
  }
  if( (int) msg->current_vcp_table.pulse_width == 2 )
  {
    LE_send_msg( GL_INFO, "VCP Str Puls Wid:  Short" );
  }
  else
  {
    LE_send_msg( GL_INFO, "VCP Str Puls Wid:  Long" );
  }
  if( msg->current_vcp_table.sample_resolution == 0 )
  {
    LE_send_msg( GL_INFO, "VCP Str Smpl Res:  250m" );
  }
  else
  {
    LE_send_msg( GL_INFO, "VCP Str Smpl Res:  50m" );
  }
  LE_send_msg( GL_INFO, "VCP Str spare1:    %d", msg->current_vcp_table.spare1 );
  LE_send_msg( GL_INFO, "VCP Str spare2:    %d", msg->current_vcp_table.spare2 );
  LE_send_msg( GL_INFO, "VCP Str spare3:    %d", msg->current_vcp_table.spare3 );
  LE_send_msg( GL_INFO, "VCP Str spare4:    %d", msg->current_vcp_table.spare4 );
  LE_send_msg( GL_INFO, "" );
  for( i = 0; i < msg->current_vcp_table.n_ele; i++ )
  {
    ele_attr = (Ele_attr *) (msg->current_vcp_table.vcp_ele[i]);
    LE_send_msg( GL_INFO, "Elev Num %02d:" );
    LE_send_msg( GL_INFO, "ELEVATION ANGLE: %5.2f deg", ele_attr->ele_angle*BAMS_SCALE );
    if( (int) ele_attr->phase == 1 )
    {
      LE_send_msg( GL_INFO, "PHASE:             RANDOM" );
    }
    else if( (int) ele_attr->phase == 2 )
    {
      LE_send_msg( GL_INFO, "PHASE:             SZ2" );
    }
    else
    {
      LE_send_msg( GL_INFO, "PHASE:             CONSTANT" );
    }
    if( (int) ele_attr->wave_type == 1 )
    {
      LE_send_msg( GL_INFO, "WAVE TYPE:         LOG CHANNEL" );
    }
    else
    {
      LE_send_msg( GL_INFO, "WAVE TYPE:         LINEAR CHANNEL" );
    }
    LE_send_msg( GL_INFO, "SUPER RES:       %d (%s)", (int) ele_attr->super_res, Convert_SR( ele_attr->super_res ) );
    LE_send_msg( GL_INFO, "Elev%02d: SURV_PRF#: %1d SURV_PULSE_CNT: %3d AZ_RATE: %5.2f deg/s", i, (int) ele_attr->surv_prf_num, (int) ele_attr->surv_pulse_cnt, ele_attr->azi_rate*AZ_RATE_SCALE );
    LE_send_msg( GL_INFO, "Elev%02d: THRESH PARAM REF/VEL/SW: %5.1f dB /%5.1f dB /%5.1f dB", i, ele_attr->surv_thr_parm/THRESHOLD_PARAM_DB_SCALE, ele_attr->vel_thrsh_parm/THRESHOLD_PARAM_DB_SCALE, ele_attr->spw_thrsh_parm/THRESHOLD_PARAM_DB_SCALE );
    LE_send_msg( GL_INFO, "Elev%02d: THRESH PARAM ZDR/PHI/CC: %5.1f dB /%5.1f dB /%5.1f dB", i, ele_attr->zdr_thrsh_parm/THRESHOLD_PARAM_DB_SCALE, ele_attr->phase_thrsh_parm/THRESHOLD_PARAM_DB_SCALE, ele_attr->corr_thrsh_parm/THRESHOLD_PARAM_DB_SCALE );
    LE_send_msg( GL_INFO, "Elev%02d: SEG1: AZ_ANG: %6.2f PRF: %1d PULSE_CNT: %3d IGNORE: %d", i, ele_attr->azi_ang_1*BAMS_SCALE, ele_attr->dop_prf_num_1, ele_attr->pulse_cnt_1, ele_attr->not_used_1 );
    LE_send_msg( GL_INFO, "Elev%02d: SEG2: AZ_ANG: %6.2f PRF: %1d PULSE_CNT: %3d IGNORE: %d", i, ele_attr->azi_ang_2*BAMS_SCALE, ele_attr->dop_prf_num_2, ele_attr->pulse_cnt_2, ele_attr->not_used_2 );
    LE_send_msg( GL_INFO, "Elev%02d: SEG3: AZ_ANG: %6.2f PRF: %1d PULSE_CNT: %3d IGNORE: %d", i, ele_attr->azi_ang_3*BAMS_SCALE, ele_attr->dop_prf_num_3, ele_attr->pulse_cnt_3, ele_attr->not_used_3 );
    LE_send_msg( GL_INFO, "" );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Convert Super-Res flag to string.
 ************************************************************************/

static char* Convert_SR( char sr_flag )
{
  static char sr_string[128];
  int has_a_value_flag = NO;

  strcpy( sr_string, "UNKNOWN" );

  if( sr_flag & 0x01 )
  {
    has_a_value_flag = YES;
    strcpy( sr_string, "0.5 deg az" );
  }

  if( sr_flag & 0x02 )
  {
    if( has_a_value_flag == NO )
    {
      has_a_value_flag = YES;
      strcpy( sr_string, "0.25 km BREF" );
    }
    else
    {
      strcat( sr_string, ", 0.25 km BREF" );
    }
  }

  if( sr_flag & 0x04 )
  {
    if( has_a_value_flag == NO )
    {
      has_a_value_flag = YES;
      strcpy( sr_string, "300 km Doppler" );
    }
    else
    {
      strcat( sr_string, ", 300 km Doppler" );
    }
  }

  if( sr_flag & 0x08 )
  {
    if( has_a_value_flag == NO )
    {
      has_a_value_flag = YES;
      strcpy( sr_string, "DP to 300 km" );
    }
    else
    {
      strcat( sr_string, ", DP to 300 km" );
    }
  }

  return sr_string;
}

/************************************************************************
 Description: Print wx status message.
 ************************************************************************/

#define MODE_A  2
#define MODE_B  1
void RPG_LDM_print_wx_status_msg( char *buf )
{
/* gen_stat_msg.h Wx_status_t, A3052t */
/* mode_select.h Mode_select_entry_t */

  Wx_status_t *msg = (Wx_status_t *) &buf[0];
  Mode_select_entry_t msf = msg->mode_select_adapt;
  A3052t a3052 = msg->a3052t;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "WEATHER STATUS MESSAGE" );
  if( msg->current_wxstatus == MODE_A )
  {
    LE_send_msg( GL_INFO, "Current Wx Status:                  MODE A (PRECIP)" );
  }
  else if( msg->current_wxstatus == MODE_B )
  {
    LE_send_msg( GL_INFO, "Current Wx Status:                  MODE B (CLEAR AIR)" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Current Wx Status:                  UNKNOWN" );
  }
  LE_send_msg( GL_INFO, "Current VCP:                        %d", msg->current_vcp );
  if( msg->recommended_wxstatus == MODE_A )
  {
    LE_send_msg( GL_INFO, "Recommended Wx Status:              MODE A (PRECIP)" );
  }
  else if( msg->recommended_wxstatus == MODE_B )
  {
    LE_send_msg( GL_INFO, "Recommended Wx Status:              MODE B (CLEAR AIR)" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Recommended Wx Status:              UNKNOWN" );
  }
  if( msg->wxstatus_deselect == 1 )
  {
    LE_send_msg( GL_INFO, "Wx Status Deselect:                 YES (MODE CHANGE)" );
  }
  else if( msg->wxstatus_deselect == 0 )
  {
    LE_send_msg( GL_INFO, "Wx Status Deselect:                 NO (NO MODE CHANGE)" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Wx Status Deselect:                 UNKNOWN" );
  }
  LE_send_msg( GL_INFO, "Recommended Start Time:             %ld (%s)", msg->recommended_wxstatus_start_time, RPG_LDM_get_timestring( msg->recommended_wxstatus_start_time ) );
  if( msg->recommended_wxstatus_default_vcp < 0 )
  {
    LE_send_msg( GL_INFO, "Recommended Def VCP:                UNKNOWN" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Recommended Def VCP:                %d", msg->recommended_wxstatus_default_vcp );
  }
  LE_send_msg( GL_INFO, "Conflict Start Time:                %ld (%s)", msg->conflict_start_time, RPG_LDM_get_timestring( msg->conflict_start_time ) );
  LE_send_msg( GL_INFO, "Current Wx Status Time:             %ld (%s)", msg->current_wxstatus_time, RPG_LDM_get_timestring( msg->current_wxstatus_time ) );
  LE_send_msg( GL_INFO, "Precip Area:                        %5.2f km^2", msg->precip_area );
  LE_send_msg( GL_INFO, "MSF Precip Z Thresh:                %5.2f dBZ", msf.precip_mode_zthresh );
  LE_send_msg( GL_INFO, "MSF Precip Area Thresh:             %d km^2", msf.precip_mode_area_thresh );
  if( msf.auto_mode_A )
  {
    LE_send_msg( GL_INFO, "MSF Auto Mode A:                    YES (AUTO)" );
  }
  else
  {
    LE_send_msg( GL_INFO, "MSF Auto Mode A:                    NO (MANUAL)" );
  }
  if( msf.auto_mode_B )
  {
    LE_send_msg( GL_INFO, "MSF Auto Mode B:                    YES (AUTO)" );
  }
  else
  {
    LE_send_msg( GL_INFO, "MSF Auto Mode B:                    NO (MANUAL)" );
  }
  LE_send_msg( GL_INFO, "MSF Mode B Select Time:             %d min", msf.mode_B_selection_time );
  if( msf.ignore_mode_conflict )
  {
    LE_send_msg( GL_INFO, "MSF Ignore Mode Conflict:           YES" );
  }
  else
  {
    LE_send_msg( GL_INFO, "MSF Ignore Mode Conflict:           NO" );
  }
  LE_send_msg( GL_INFO, "MSF Mode Conflict Duration:         %d hr", msf.mode_conflict_duration );
  if( msf.use_hybrid_scan )
  {
    LE_send_msg( GL_INFO, "MSF Use Hybrid Scan:                YES" );
  }
  else
  {
    LE_send_msg( GL_INFO, "MSF Use Hybrid Scan:                NO" );
  }
  LE_send_msg( GL_INFO, "MSF Clutter Thresh:                 %d%%", msf.clutter_thresh );
  LE_send_msg( GL_INFO, "MSF Spare:                          %d", msf.spare );
  LE_send_msg( GL_INFO, "MSF Supp Detect Alg Last Run:       %ld (%s)", a3052.curr_time, RPG_LDM_get_timestring( a3052.curr_time ) );
  LE_send_msg( GL_INFO, "MSF Supp Precip Last Detected:      %ld (%s)", a3052.last_time, RPG_LDM_get_timestring( a3052.last_time ) );
  LE_send_msg( GL_INFO, "MSF Supp End Cat 1 Precip Detected: %ld (%s)", a3052.time_to_cla, RPG_LDM_get_timestring( a3052.time_to_cla ) );
  LE_send_msg( GL_INFO, "MSF Supp Current Precip Cat:        %d", a3052.pcpctgry );
  LE_send_msg( GL_INFO, "MSF Supp Previous Precip Cat:       %d", a3052.prectgry );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print console msg message.
 ************************************************************************/

void RPG_LDM_print_console_msg_msg( char *buf )
{
/* rda_rpg_console_message.h RDA_RPG_console_message_t */

  RDA_RPG_console_message_t *msg = (RDA_RPG_console_message_t *) &buf[0];
  char *console_msg = NULL;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "CONSOLE MESSAGE" );
  Print_RDA_RPG_msg_hdr( &buf[0] );
  LE_send_msg( GL_INFO, "Message" );
  LE_send_msg( GL_INFO, "Size:        %d bytes", msg->size );
  if( ( console_msg = malloc( msg->size ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Malloc of console_msg failed for %d bytes", msg->size );
    exit( 1 );
  }
  memcpy( &console_msg[0], &(msg->message[0]), msg->size );
  console_msg[msg->size] = '\0';
  LE_send_msg( GL_INFO, "MSG:         %s", console_msg );
  free( console_msg );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print RPG info message.
 ************************************************************************/

void RPG_LDM_print_RPG_info_msg( char *buf )
{
/* orpginfo.h Orpginfo_statefl_t */

  Orpginfo_statefl_t *msg = (Orpginfo_statefl_t *) &buf[0];

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "RPG INFO MESSAGE" );
  LE_send_msg( GL_INFO, "RPG OP Status:        %d (deprecated)", msg->rpg_op_status );
  LE_send_msg( GL_INFO, "RPG Alarms:           %d (deprecated)", msg->rpg_alarms );
  LE_send_msg( GL_INFO, "Commanded RPG Status: %s", Print_RPG_statfl( msg->rpg_status_cmded ) );
  LE_send_msg( GL_INFO, "Current RPG Status:   %s", Print_RPG_statfl( msg->rpg_status ) );
  LE_send_msg( GL_INFO, "Previous RPG Status:  %s", Print_RPG_statfl( msg->rpg_status_prev ) );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Convert RPG state info codes to strings.
 ************************************************************************/

static char *Print_RPG_statfl( int statfl_flag )
{
  static char statfl_string[16];

  if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_UNKNOWN )
  {
    strcpy( statfl_string, "UNKNOWN" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_RESTART )
  {
    strcpy( statfl_string, "RESTART" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_OPERATE )
  {
    strcpy( statfl_string, "OPERATE" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_STANDBY )
  {
    strcpy( statfl_string, "STANDBY" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN )
  {
    strcpy( statfl_string, "SHUTDOWN" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_TEST )
  {
    strcpy( statfl_string, "TEST" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT26 )
  {
    strcpy( statfl_string, "BIT26" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT25 )
  {
    strcpy( statfl_string, "BIT25" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT24 )
  {
    strcpy( statfl_string, "BIT24" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT23 )
  {
    strcpy( statfl_string, "BIT23" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT22 )
  {
    strcpy( statfl_string, "BIT22" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT21 )
  {
    strcpy( statfl_string, "BIT21" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT20 )
  {
    strcpy( statfl_string, "BIT20" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT19 )
  {
    strcpy( statfl_string, "BIT19" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT18 )
  {
    strcpy( statfl_string, "BIT18" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT17 )
  {
    strcpy( statfl_string, "BIT17" );
  }
  else if( statfl_flag == ORPGINFO_STATEFL_RPGSTAT_BIT16 )
  {
    strcpy( statfl_string, "BIT16" );
  }
  else
  {
    strcpy( statfl_string, "????" );
  }

  return statfl_string;
}

/************************************************************************
 Description: Print task status message.
 ************************************************************************/

void RPG_LDM_print_task_status_msg( char *buf, int msg_size )
{
/* mrpg.h Mrpg_process_status_t */

  Mrpg_process_status_t *msg = NULL;
  int offset = 0;
  char *task_name = NULL;
  int task_name_len = 0;
  char *node_name = NULL;
  int node_name_len = 0;
  char *cmd_line = NULL;
  int cmd_line_len = 0;
  int task_count = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "TASK STATUS MESSAGE" );
  while( offset < msg_size )
  {
    msg = (Mrpg_process_status_t *) &buf[offset];

    LE_send_msg( GL_INFO, "TASK STATUS:        %d", ++task_count );
    LE_send_msg( GL_INFO, "Size:               %d bytes", msg->size );
    LE_send_msg( GL_INFO, "Task Name Offset:   %d bytes", msg->name_off );
    LE_send_msg( GL_INFO, "Node Name Offset:   %d bytes", msg->node_off );
    LE_send_msg( GL_INFO, "Cmd Line Offset:    %d bytes", msg->cmd_off );
    LE_send_msg( GL_INFO, "Instance Num:       %d", msg->instance );
    LE_send_msg( GL_INFO, "PID:                %d", msg->pid );
    LE_send_msg( GL_INFO, "Accum CPU Usage:    %d milliseconds", msg->cpu );
    LE_send_msg( GL_INFO, "Stack/Heap Usage:   %d Kb", msg->mem );
    if( msg->status == MRPG_PS_NOT_STARTED )
    {
      LE_send_msg( GL_INFO, "Process Run Status: NOT STARTED" );
    }
    else if( msg->status == MRPG_PS_STARTED )
    {
      LE_send_msg( GL_INFO, "Process Run Status: STARTED" );
    }
    else if( msg->status == MRPG_PS_ACTIVE )
    {
      LE_send_msg( GL_INFO, "Process Run Status: ACTIVE" );
    }
    else if( msg->status == MRPG_PS_FAILED )
    {
      LE_send_msg( GL_INFO, "Process Run Status: FAILED" );
    }
    else
    {
      LE_send_msg( GL_INFO, "Process Run Status: UNKNOWN (%d)", msg->status );
    }
    LE_send_msg( GL_INFO, "Process Life Time:  %d seconds", msg->life );
    LE_send_msg( GL_INFO, "Time Retrieved:     %ld (%s)", msg->info_t, RPG_LDM_get_timestring( msg->info_t ) );

    /* Get task name */
    task_name_len = msg->node_off - msg->name_off;
    if( ( task_name = malloc( task_name_len + 1 ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Malloc task_name failed for %d bytes", task_name_len + 1 );
      exit( 1 );
    }
    memcpy( task_name, &buf[offset+msg->name_off], task_name_len );
    task_name[task_name_len] = '\0';
    LE_send_msg( GL_INFO, "Task Name:          %s", task_name );
    free( task_name );
    task_name = NULL;

    /* Get node name */
    node_name_len = msg->cmd_off - msg->node_off;
    if( ( node_name = malloc( node_name_len + 1 ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Malloc node_name failed for %d bytes", node_name_len + 1 );
      exit( 1 );
    }
    memcpy( node_name, &buf[offset+msg->node_off], node_name_len );
    node_name[node_name_len] = '\0';
    LE_send_msg( GL_INFO, "Node Name:          %s", node_name );
    free( node_name );
    node_name = NULL;

    /* Get command line */
    cmd_line_len = msg->size - msg->cmd_off;
    if( ( cmd_line = malloc( cmd_line_len + 1 ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Malloc cmd line failed for %d bytes", cmd_line_len + 1 );
      exit( 1 );
    }
    memcpy( cmd_line, &buf[offset+msg->cmd_off], cmd_line_len );
    cmd_line[cmd_line_len] = '\0';
    LE_send_msg( GL_INFO, "Cmd Line:           %s", cmd_line );
    free( cmd_line );
    cmd_line = NULL;

    /* Set offset to next message */
    offset += msg->size;
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print prod info message.
 ************************************************************************/

void RPG_LDM_print_product_info_msg( char *buf )
{
/* prod_distri_info.h Pd_distri_info */

  Pd_distri_info *msg = (Pd_distri_info *) &buf[0];
  Pd_line_entry *ent = NULL;
  char port_passwd[PASSWORD_LEN+1];
  int i = 0;
  int offset = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "PROD INFO MESSAGE" );
  LE_send_msg( GL_INFO, "Num Retries:                   %d", msg->nb_retries );
  LE_send_msg( GL_INFO, "Max Msg Trans Time:            %d seconds", msg->nb_timeout );
  LE_send_msg( GL_INFO, "Connect Time Limit:            %d minutes", msg->connect_time_limit );
  LE_send_msg( GL_INFO, "Number of lines:               %d", msg->n_lines );
  LE_send_msg( GL_INFO, "Offset to Line List:           %d bytes", msg->line_list );
  for( i = 0; i < msg->n_lines; i++ )
  {
    offset = msg->line_list + ( i * sizeof( Pd_line_entry ) );
    ent = (Pd_line_entry *) &buf[offset];
    LE_send_msg( GL_INFO, "Line %02d - Index:             %d", i, (int) ent->line_ind );
    LE_send_msg( GL_INFO, "Line %02d - CM Index:          %d", i, (int) ent->cm_ind );
    LE_send_msg( GL_INFO, "Line %02d - P_serv Index:      %d", i, (int) ent->p_server_ind );
    LE_send_msg( GL_INFO, "Line %02d - Type:              %d", i, (int) ent->line_type );
    if( (int) ent->link_state == LINK_ENABLED )
    {
      LE_send_msg( GL_INFO, "Line %02d - State:             ENABLED", i );
    }
    else if( (int) ent->link_state == LINK_DISABLED )
    {
      LE_send_msg( GL_INFO, "Line %02d - State:             DISABLED", i );
    }
    else
    {
      LE_send_msg( GL_INFO, "Line %02d - State:             UNKNOWN (%d)", i, (int) ent->link_state );
    }
    if( (int) ent->protocol == PROTO_TCP )
    {
      LE_send_msg( GL_INFO, "Line %02d - Protocol:          TCP", i );
    }
    else if( (int) ent->protocol == PROTO_X25 )
    {
      LE_send_msg( GL_INFO, "Line %02d - Protocol:          X25", i );
    }
    else
    {
      LE_send_msg( GL_INFO, "Line %02d - Protocol:          UNKNOWN (%d)", i, (int) ent->protocol );
    }
    LE_send_msg( GL_INFO, "Line %02d - Num PVCs:          %d", i, (int) ent->n_pvcs );
    LE_send_msg( GL_INFO, "Line %02d - Spare:             %d", i, (int) ent->not_used );
    LE_send_msg( GL_INFO, "Line %02d - Packet Size:       %d", i, ent->packet_size );
    LE_send_msg( GL_INFO, "Line %02d - Baud Rate:         %d bits/second", i, ent->baud_rate );
    LE_send_msg( GL_INFO, "Line %02d - Connect TimeLimit: %d minutes", i, ent->conn_time_limit );
    memcpy( &port_passwd[0], &(ent->port_password[0]), PASSWORD_LEN );
    port_passwd[PASSWORD_LEN] = '\0';
    LE_send_msg( GL_INFO, "Line %02d - Password:          %s", i, port_passwd );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print save log message.
 ************************************************************************/

void RPG_LDM_print_save_log_msg( char *buf )
{
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "SAVE LOG MESSAGE" );
  LE_send_msg( GL_INFO, "Save log message is not supported in this tool" );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print bias table message.
 ************************************************************************/

void RPG_LDM_print_bias_table_msg( char *buf )
{
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "BIAS TABLE MESSAGE" );
  LE_send_msg( GL_INFO, "Bias table message is not supported in this tool" );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print RUC data message.
 ************************************************************************/

void RPG_LDM_print_RUC_data_msg( char *buf )
{
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "RUC DATA MESSAGE" );
  LE_send_msg( GL_INFO, "RUC data message is not supported in this tool" );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print L2 write stats message.
 ************************************************************************/

void RPG_LDM_print_L2_write_stats_msg( char *buf, int msg_size )
{
/* rpg_ldm.h RPG_LDM_LB_write_stats_t */

  char key[RPG_LDM_MAX_KEY_LEN+1];
  RPG_LDM_LB_write_stats_t *msg = NULL;
  int offset = 0;
  int product_count = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "LB WRITE STATS MESSAGE" );
  while( offset < msg_size )
  {
    msg = (RPG_LDM_LB_write_stats_t *) &buf[offset];

    memcpy( &key[0], &(msg->key[0]), RPG_LDM_MAX_KEY_LEN );
    key[RPG_LDM_MAX_KEY_LEN] = '\0';
    LE_send_msg( GL_INFO, "LB WRITE STATS:      %d", ++product_count );
    LE_send_msg( GL_INFO, "Key:                 %s", key );
    LE_send_msg( GL_INFO, "Num Msgs Total:      %d", msg->num_msgs_total );
    LE_send_msg( GL_INFO, "Num Msgs per Month:  %f", (float) msg->num_msgs_per_month/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Num Msgs per Day:    %f", (float) msg->num_msgs_per_day/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Num Msgs per Hour:   %f", (float) msg->num_msgs_per_hour/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Num Msgs per Volume: %f", (float) msg->num_msgs_per_volume/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Msg Size Total       %d bytes", msg->msg_size_total );
    LE_send_msg( GL_INFO, "Size per Month:      %f bytes/month", (float) msg->msg_size_per_month/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Size per Day:        %f bytes/day", (float) msg->msg_size_per_day/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Size per Hour:       %f bytes/hour", (float) msg->msg_size_per_hour/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Size per Volume:     %f bytes/volume", (float) msg->msg_size_per_volume/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Reset Time:          %ld (%s)", msg->reset_time, RPG_LDM_get_timestring( msg->reset_time ) );
    LE_send_msg( GL_INFO, "Spare172:            %d", msg->spare172 );
    LE_send_msg( GL_INFO, "Spare174:            %d", msg->spare174 );
    LE_send_msg( GL_INFO, "Spare176:            %d", msg->spare176 );
    LE_send_msg( GL_INFO, "Spare178:            %d", msg->spare178 );
    LE_send_msg( GL_INFO, "Spare180:            %d", msg->spare180 );
    LE_send_msg( GL_INFO, "Spare182:            %d", msg->spare182 );
    offset += sizeof( RPG_LDM_LB_write_stats_t );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print L2 read stats message.
 ************************************************************************/

void RPG_LDM_print_L2_read_stats_msg( char *buf, int msg_size )
{
/* rpg_ldm.h RPG_LDM_LB_write_stats_t */

  char key[RPG_LDM_MAX_KEY_LEN+1];
  RPG_LDM_LB_write_stats_t *msg = NULL;
  int offset = 0;
  int product_count = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "LB WRITE STATS MESSAGE" );
  while( offset < msg_size )
  {
    msg = (RPG_LDM_LB_write_stats_t *) &buf[offset];

    memcpy( &key[0], &(msg->key[0]), RPG_LDM_MAX_KEY_LEN );
    key[RPG_LDM_MAX_KEY_LEN] = '\0';
    LE_send_msg( GL_INFO, "LB WRITE STATS:      %d", ++product_count );
    LE_send_msg( GL_INFO, "Key:                 %s", key );
    LE_send_msg( GL_INFO, "Num Msgs Total:      %d", msg->num_msgs_total );
    LE_send_msg( GL_INFO, "Num Msgs per Month:  %f", (float) msg->num_msgs_per_month/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Num Msgs per Day:    %f", (float) msg->num_msgs_per_day/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Num Msgs per Hour:   %f", (float) msg->num_msgs_per_hour/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Num Msgs per Volume: %f", (float) msg->num_msgs_per_volume/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Msg Size Total       %d bytes", msg->msg_size_total );
    LE_send_msg( GL_INFO, "Size per Month:      %f bytes/month", (float) msg->msg_size_per_month/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Size per Day:        %f bytes/day", (float) msg->msg_size_per_day/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Size per Hour:       %f bytes/hour", (float) msg->msg_size_per_hour/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Size per Volume:     %f bytes/volume", (float) msg->msg_size_per_volume/RPG_LDM_NUM_L2_MSGS_SCALE );
    LE_send_msg( GL_INFO, "Reset Time:          %ld (%s)", msg->reset_time, RPG_LDM_get_timestring( msg->reset_time ) );
    LE_send_msg( GL_INFO, "Spare172:            %d", msg->spare172 );
    LE_send_msg( GL_INFO, "Spare174:            %d", msg->spare174 );
    LE_send_msg( GL_INFO, "Spare176:            %d", msg->spare176 );
    LE_send_msg( GL_INFO, "Spare178:            %d", msg->spare178 );
    LE_send_msg( GL_INFO, "Spare180:            %d", msg->spare180 );
    LE_send_msg( GL_INFO, "Spare182:            %d", msg->spare182 );
    offset += sizeof( RPG_LDM_LB_write_stats_t );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print RPG Level-III message.
 ************************************************************************/

void RPG_LDM_print_L3_msg( char *buf, int wmo_hdr_size )
{
/* product.h Graphic_product */

  Graphic_product *gp = NULL;
  char *wmo_hdr = NULL;
  int offset = 0;
  int index = 0;

  if( wmo_hdr_size > 0 )
  {
    offset = wmo_hdr_size;
    if( ( wmo_hdr = malloc( sizeof( char ) * ( offset + 1 ) ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Unable to malloc size %d", offset + 1 ); 
    }
    else
    {
      memcpy( wmo_hdr, buf, offset );
      wmo_hdr[offset] = '\0';
      for( index = 0; index < offset; index++ )
      {
        if( wmo_hdr[index] == '\r' || wmo_hdr[index] == '\n' )
        {
          wmo_hdr[index] = ' ';
        }
      }
      LE_send_msg( GL_INFO, "" ); 
      LE_send_msg( GL_INFO, "LEVEL-III WMO HEADER" );
      LE_send_msg( GL_INFO, "%s", wmo_hdr );
      free( wmo_hdr );
    }
  }

  gp = (Graphic_product *) &buf[offset];


  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "LEVEL-III PRODUCT MESSAGE" );
  LE_send_msg( GL_INFO, "Msg code: %d", SHORT_BSWAP( gp->msg_code ) );
  LE_send_msg( GL_INFO, "Msg date: %d", SHORT_BSWAP( gp->msg_date ) );
  LE_send_msg( GL_INFO, "Msg time: %d", INT_BSWAP( gp->msg_time ) );
  LE_send_msg( GL_INFO, "Msg len:  %d", INT_BSWAP( gp->msg_len ) );
  LE_send_msg( GL_INFO, "Msg src:  %d", SHORT_BSWAP( gp->src_id ) );
  LE_send_msg( GL_INFO, "Msg dest: %d", SHORT_BSWAP( gp->dest_id ) );
  LE_send_msg( GL_INFO, "# blocks: %d", SHORT_BSWAP( gp->n_blocks ) );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print RPG SNMP message.
 ************************************************************************/

void RPG_LDM_print_snmp_msg( char *buf, int msg_size )
{
/* rpg_ldm.h RPG_LDM_snmp_msg_t */

  RPG_LDM_snmp_msg_t *msg = NULL;
  int offset = 0;
  int count = 0;

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "SNMP PRODUCT MESSAGE" );
  offset = 0;
  while( offset < msg_size )
  {
    count++;
    msg = (RPG_LDM_snmp_msg_t *) &buf[offset];
    LE_send_msg( GL_INFO, "Count %3d Time:   %ld (%s)", count, msg->timestamp, RPG_LDM_get_timestring( msg->timestamp ) );
    LE_send_msg( GL_INFO, "Count %3d Length: %d", count, msg->text_len );
    LE_send_msg( GL_INFO, "Count %3d Msg:    %s", count, msg->text );
    offset += ( sizeof( RPG_LDM_snmp_msg_t ) + strlen( msg->text ) );
  }
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print rtstats message.
 ************************************************************************/

void RPG_LDM_print_rtstats_msg( char *buf )
{
/* rpg_ldm.h RPG_LDM_rtstats_t */

  RPG_LDM_rtstats_t *msg = (RPG_LDM_rtstats_t *) &buf[0];
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "RTSTATS MESSAGE" );
  LE_send_msg( GL_INFO, "Server Mask:             %d (%s)", msg->server_mask, RPG_LDM_get_server_string_from_mask( msg->server_mask ) );
  LE_send_msg( GL_INFO, "Generation:              %ld (%s)", msg->generation_time, RPG_LDM_get_timestring( msg->generation_time ) );
  LE_send_msg( GL_INFO, "Arrival:                 %ld (%s)", msg->arrival_time, RPG_LDM_get_timestring( msg->arrival_time ) );
  LE_send_msg( GL_INFO, "Feed Type:               %d (%s)", msg->feed_type_code, RPG_LDM_get_feed_type_string_from_code( msg->feed_type_code ) );
  LE_send_msg( GL_INFO, "# Prods this hour:       %d", msg->num_prods_current_hour );
  LE_send_msg( GL_INFO, "# Bytes this hour:       %d", msg->num_bytes_current_hour );
  LE_send_msg( GL_INFO, "Latency (Current):       %10.2f", msg->current_latency );
  LE_send_msg( GL_INFO, "Latency (Avg this Hour): %10.2f", msg->avg_latency_current_hour );
  LE_send_msg( GL_INFO, "Latency (Max this Hour): %10.2f", msg->max_latency_current_hour );
  LE_send_msg( GL_INFO, "Max Latency timestamp:   %ld (%s)", msg->max_latency_timestamp, RPG_LDM_get_timestring( msg->max_latency_timestamp ) );
  LE_send_msg( GL_INFO, "Client:                  %s", msg->client_name );
  LE_send_msg( GL_INFO, "Server:                  %s", msg->server_name );
  LE_send_msg( GL_INFO, "Spare120:                %d", msg->spare120 );
  LE_send_msg( GL_INFO, "Spare122:                %d", msg->spare122 );
  LE_send_msg( GL_INFO, "Spare124:                %d", msg->spare124 );
  LE_send_msg( GL_INFO, "Spare126:                %d", msg->spare126 );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print security message.
 ************************************************************************/

void RPG_LDM_print_security_msg( char *buf )
{
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "SECURITY MESSAGE" );
  LE_send_msg( GL_INFO, "Security message is not supported in this tool" );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print hw status message.
 ************************************************************************/

void RPG_LDM_print_hw_stats_msg( char *buf )
{
/* rpg_ldm.h RPG_LDM_hw_stats_t */

  RPG_LDM_hw_stats_t *msg = (RPG_LDM_hw_stats_t *) &buf[0];

  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "HW STATS MESSAGE" );
  LE_send_msg( GL_INFO, "Server Mask:    %d (%s)", msg->server_mask, RPG_LDM_get_server_string_from_mask( msg->server_mask ) );
  LE_send_msg( GL_INFO, "Uptime:         %d seconds", msg->uptime );
  LE_send_msg( GL_INFO, "Percent Idle:   %5.2f %%", (float) msg->pct_idle );
  LE_send_msg( GL_INFO, "Load Avg (1m):  %-6.2f", (float) msg->load_avg_1min/RPG_LDM_LOAD_AVG_SCALE );
  LE_send_msg( GL_INFO, "Load Avg (5m):  %-6.2f", (float) msg->load_avg_5min /RPG_LDM_LOAD_AVG_SCALE );
  LE_send_msg( GL_INFO, "Load Avg (15m): %-6.2f", (float) msg->load_avg_15min/RPG_LDM_LOAD_AVG_SCALE );
  LE_send_msg( GL_INFO, "Mem Total:      %d bytes", msg->mem_total );
  LE_send_msg( GL_INFO, "Mem Free:       %d bytes", msg->mem_free );
  LE_send_msg( GL_INFO, "Swap Total:     %d bytes", msg->swap_total );
  LE_send_msg( GL_INFO, "Swap Free:      %d bytes", msg->swap_free );
  LE_send_msg( GL_INFO, "Disk Total:     %d bytes", msg->disk_total );
  LE_send_msg( GL_INFO, "Disk Free:      %d bytes", msg->disk_free );
  LE_send_msg( GL_INFO, "Spare48:        %d", msg->spare48 );
  LE_send_msg( GL_INFO, "Spare50:        %d", msg->spare50 );
  LE_send_msg( GL_INFO, "Spare52:        %d", msg->spare52 );
  LE_send_msg( GL_INFO, "Spare54:        %d", msg->spare54 );
  LE_send_msg( GL_INFO, "Spare56:        %d", msg->spare56 );
  LE_send_msg( GL_INFO, "Spare58:        %d", msg->spare58 );
  LE_send_msg( GL_INFO, "Spare60:        %d", msg->spare60 );
  LE_send_msg( GL_INFO, "Spare62:        %d", msg->spare62 );
  LE_send_msg( GL_INFO, "Spare64:        %d", msg->spare64 );
  LE_send_msg( GL_INFO, "Spare66:        %d", msg->spare66 );
  LE_send_msg( GL_INFO, "Spare68:        %d", msg->spare68 );
  LE_send_msg( GL_INFO, "Spare70:        %d", msg->spare70 );
  LE_send_msg( GL_INFO, "Spare72:        %d", msg->spare72 );
  LE_send_msg( GL_INFO, "Spare74:        %d", msg->spare74 );
  LE_send_msg( GL_INFO, "Spare76:        %d", msg->spare76 );
  LE_send_msg( GL_INFO, "Spare78:        %d", msg->spare78 );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print miscellaneous file message.
 ************************************************************************/

void RPG_LDM_print_misc_file_msg( char *buf )
{
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "MISCELLANEOUS FILE MESSAGE" );
  LE_send_msg( GL_INFO, "Miscellaneous file message is not supported in this tool" );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print request message.
 ************************************************************************/

void RPG_LDM_print_request_msg( char *buf )
{
/* rpg_ldm.h RPG_LDM_request_t */

  RPG_LDM_request_t *msg = (RPG_LDM_request_t *) &buf[0];
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "REQUEST MESSAGE" );
  LE_send_msg( GL_INFO, "Server Mask:  %d (%s)", msg->server_mask, RPG_LDM_get_server_string_from_mask( msg->server_mask ) );
  LE_send_msg( GL_INFO, "Request Code: %d", msg->request_code );
  LE_send_msg( GL_INFO, "Params:       %s", msg->request_params );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print command message.
 ************************************************************************/

void RPG_LDM_print_command_msg( char *buf )
{
/* rpg_ldm.h RPG_LDM_cmd_t */

  RPG_LDM_cmd_t *msg = (RPG_LDM_cmd_t *) &buf[0];
  LE_send_msg( GL_INFO, "" ); 
  LE_send_msg( GL_INFO, "COMMAND MESSAGE" );
  LE_send_msg( GL_INFO, "Server Mask:  %d (%s)", msg->server_mask, RPG_LDM_get_server_string_from_mask( msg->server_mask ) );
  LE_send_msg( GL_INFO, "Command Code: %d", msg->cmd_code );
  LE_send_msg( GL_INFO, "" ); 
}

/************************************************************************
 Description: Print RPG message header.
 ************************************************************************/

static void Print_RDA_RPG_msg_hdr( char *buf )
{
/* rda_rpg_message_header.h RDA_RPG_message_header_t */
/* rda_rpg_messages.h type MACROS */

  RDA_RPG_message_header_t *hdr = ( RDA_RPG_message_header_t *) buf;

  LE_send_msg( GL_INFO, "RDA RPG MESSAGE HEADER" );
  LE_send_msg( GL_INFO, "Size:           %d bytes", hdr->size );
  if( (int) hdr->rda_channel == RDA_RPG_MSG_HDR_LEGACY_CFG )
  {
    LE_send_msg( GL_INFO, "RDA Channel:  LEGACY RDA CONFIGURATION" );
  }
  else if( (int) hdr->rda_channel == RDA_RPG_MSG_HDR_ORDA_CFG )
  {
    LE_send_msg( GL_INFO, "RDA Channel:  OPEN RDA CONFIGURATION NON-REDUNDANT" );
  }
  else if( (int) hdr->rda_channel & RDA_RPG_MSG_HDR_ORDA_CFG )
  {
    LE_send_msg( GL_INFO, "RDA Channel:  OPEN RDA CONFIGURATION CHANNEL %d", (int) hdr->rda_channel - RDA_RPG_MSG_HDR_ORDA_CFG );
  }
  else
  {
    LE_send_msg( GL_INFO, "RDA Channel:  UNKNOWN RDA CONFIGURATION (%d)", (int) hdr->rda_channel );
  }
  if( (int) hdr->type == MESSAGE_TYPE_RDA_STATUS )
  {
    LE_send_msg( GL_INFO, "Message Type: RDA STATUS" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_PERFORMANCE_MAINTENANCE )
  {
    LE_send_msg( GL_INFO, "Message Type: RDA PERFORMANCE MAINTENANCE" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_CONSOLE_RDA_TO_RPG )
  {
    LE_send_msg( GL_INFO, "Message Type: CONSOLE MESSAGE RDA TO RPG" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_CONSOLE_RPG_TO_RDA )
  {
    LE_send_msg( GL_INFO, "Message Type: CONSOLE MESSAGE RPG TO RDA" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_RDA_VCP )
  {
    LE_send_msg( GL_INFO, "Message Type: RDA VCP" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_RDA_CONTROL )
  {
    LE_send_msg( GL_INFO, "Message Type: RDA CONTROL" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_VCP )
  {
    LE_send_msg( GL_INFO, "Message Type: VCP" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_CLUTTER_CENSOR_ZONES )
  {
    LE_send_msg( GL_INFO, "Message Type: CLUTTER CENSOR ZONES" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_REQUEST_DATA )
  {
    LE_send_msg( GL_INFO, "Message Type: REQUEST DATA" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_CLUTTER_FILTER_RDA_TO_RPG )
  {
    LE_send_msg( GL_INFO, "Message Type: CLUTTER FILTER RDA TO RPG" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_CLUTTER_FILTER_RPG_TO_RDA )
  {
    LE_send_msg( GL_INFO, "Message Type: CLUTTER FILTER RPG TO RDA" );
  }
  else if( (int) hdr->type == MESSAGE_TYPE_GENERIC_DIGITAL_RADAR_DATA )
  {
    LE_send_msg( GL_INFO, "Message Type: GENERIC DIGITAL RADAR DATA" );
  }
  else
  {
    LE_send_msg( GL_INFO, "MESSAGE TYPE: UNKNOWN (%d)", (int) hdr->type );
  }
  LE_send_msg( GL_INFO, "Seq Num:        %d", hdr->sequence_num );
  LE_send_msg( GL_INFO, "Date:           %d (%s)", hdr->julian_date, Convert_days( hdr->julian_date ) );
  LE_send_msg( GL_INFO, "Time:           %d (%s)", hdr->milliseconds, Convert_millisecs( hdr->milliseconds ) );
  LE_send_msg( GL_INFO, "Num Segs:       %d", hdr->num_segs );
  LE_send_msg( GL_INFO, "Seg Num:        %d", hdr->seg_num );
}

/************************************************************************
 Description: Convert days since 1970 to a string.
 ************************************************************************/

static char* Convert_days( int days )
{
  static char timebuf[64];

  time_t num_secs = (days - 1) * 86400;

  strftime( timebuf, 64, "%Y/%m/%d", gmtime( &num_secs ) );

  return timebuf;
}

/************************************************************************
 Description: Convert milliseconds since midnight to a string.
 ************************************************************************/

static char* Convert_millisecs( long msecs )
{
  static char timebuf[64];

  time_t num_secs = msecs/RPG_LDM_NUM_MILLISECS_IN_SECS;

  strftime( timebuf, 64, "%H:%M:%S", gmtime( &num_secs ) );

  return timebuf;
}

/************************************************************************
 Description: Convert minutes since midnight to a string.
 ************************************************************************/

static char* Convert_minutes( int mins )
{
  static char timebuf[64];

  time_t num_secs = mins * 60;

  strftime( timebuf, 64, "%H:%M:%S", gmtime( &num_secs ) );

  return timebuf;
}

/**************************************************************************
 Description: Get WMO header for the product.
**************************************************************************/

char *RPG_LDM_get_WMO_header( Graphic_product *gp )
{
  static char wmo_hdr[RPG_LDM_MAX_WMO_HDR_LEN];
  char *icao = NULL;
  char *wmo_timestring = NULL;
  char wmo_ttaai[6];
  int wmo_region = -1;
  char *wmo_cccc = NULL;
  char wmo_bbb1[4];
  char *wmo_bbb2 = NULL;
  int param1 = 0;
  int param2 = 0;
  int param3 = 0;
  int product_code = SHORT_BSWAP( gp->prod_code );

  RPG_LDM_print_debug( "Enter Get_WMO_header with product %d", product_code );

  if( ( icao = RPG_LDM_get_local_ICAO() ) == NULL )
  {
    LE_send_msg( GL_ERROR, "RPG_LDM_get_local_ICAO() returned NULL" );
    return NULL;
  }
  else if( strlen( icao ) != RPG_LDM_MIN_ICAO_LEN )
  {
    LE_send_msg( GL_ERROR, "RPG_LDM_get_local_ICAO() has unexpected length" );
    return NULL;
  }

  if( ( wmo_region = Get_WMO_region( icao ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "Invalid WMO region" );
    return NULL;
  }

  if( ( wmo_timestring = Get_WMO_timestring( gp ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Invalid WMO time string" );
    return NULL;
  }

  if( ( wmo_cccc = Get_WMO_cccc( icao ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Invalid WMO CCCC" );
    return NULL;
  }

  if( ( wmo_bbb2 = Get_WMO_bbb2( icao ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Invalid WMO BBB2" );
    return NULL;
  }

  switch( product_code )
  {
    case 2: /* GSM */
      strcpy( wmo_ttaai, "NXUS6" );
      strcpy( wmo_bbb1, "N0Q" );
      break;
    case 9: /* ALRTMSG */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 16: /* BREF16 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 17: /* BREF17 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 18: /* BREF18 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 19: /* R, BREF19 */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "N0R" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 20: /* R, BREF20 */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS7" );
        strcpy( wmo_bbb1, "N0Z" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 21: /* BREF21 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 22: /* BVEL22 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 23: /* BVEL23 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 24: /* BVEL24 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 25: /* BVEL25 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 26: /* BVEL26 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 27: /* V, BVEL27 */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "N0V" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 28: /* SW, BSPC28 */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NSP" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 29: /* BSPC29 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 30: /* SW, BSPC30 */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NSW" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 31: /* HYUSPACC */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 32: /* DHR, HYBRDREF */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "DHR" );
      break;
    case 33: /* HYBRGREF */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 34: /* CFC, CFCPROD */
      param1 = SHORT_BSWAP( gp->param_1 );
      if( param1 & RPG_LDM_CFC_PROD_SEG1_BITMASK )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NC1" );
      }
      else if( param1 == RPG_LDM_CFC_PROD_SEG2_BITMASK )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NC2" );
      }
      else if( param1 == RPG_LDM_CFC_PROD_SEG3_BITMASK )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NC3" );
      }
      else if( param1 == RPG_LDM_CFC_PROD_SEG4_BITMASK )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NC4" );
      }
      else if( param1 == RPG_LDM_CFC_PROD_SEG5_BITMASK )
      {
        strcpy( wmo_ttaai, "SDUS6" );
        strcpy( wmo_bbb1, "NC5" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param1 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 35: /* CRP35 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 36: /* CR, CRP36 */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NCO" );
      break;
    case 37: /* CR, CRP37 */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "NCR" );
      break;
    case 38: /* CR, CRP38 */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NCZ" );
      break;
    case 41: /* ET, ETPRODD */
      strcpy( wmo_ttaai, "SDUS7" );
      strcpy( wmo_bbb1, "NET" );
      break;
    case 48: /* VWP, HPLOTS */
      strcpy( wmo_ttaai, "SDUS3" );
      strcpy( wmo_bbb1, "NVW" );
      break;
    case 50: /* VCS52 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 51: /* VCS53 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 55: /* SRMRVREG */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 56: /* SRM, SRMRVMAP */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "N0S" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N1S" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N2S" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS3" );
        strcpy( wmo_bbb1, "N3S" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 57: /* VIL, VILPROD */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "NVL" );
      break;
    case 58: /* STI, STMTRDAT */
      strcpy( wmo_ttaai, "SDUS3" );
      strcpy( wmo_bbb1, "NST" );
      break;
    case 59: /* HI, HAILCAT */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NHI" );
      break;
    case 61: /* TVS, TVSPROD */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NTV" );
      break;
    case 62: /* SS, STRUCDAT */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NSS" );
      break;
    case 63: /* RFAVLYR1 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 64: /* RFAVLYR2 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 65: /* LRM, RFMXLYR1 */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NLL" );
      break;
    case 66: /* LRM, RFMXLYR2 */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NML" );
      break;
    case 67: /* APR, RMXAPLYR */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NLA" );
      break;
    case 73: /* ALRTPROD */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 74: /* RCM, POSEDRCM */
      strcpy( wmo_ttaai, "SDUS4" );
      strcpy( wmo_bbb1, "RCM" );
      break;
    case 75: /* FTM, FTXTMSG */
      strcpy( wmo_ttaai, "NOUS6" );
      strcpy( wmo_bbb1, "FTM" );
      break;
    case 78: /* OHP, HY1HRACC */
      strcpy( wmo_ttaai, "SDUS3" );
      strcpy( wmo_bbb1, "N1P" );
      break;
    case 79: /* THP, HY3HRACC */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "N3P" );
      break;
    case 80: /* STP, HYSTMTOT */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "NTP" );
      break;
    case 81: /* DPA, HY1HRDIG */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "DPA" );
      break;
    case 82: /* SPD, HYSUPPLE */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "SPD" );
      break;
    case 84: /* VAD */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 85: /* VCSR8 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 86: /* VCSV8 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 89: /* RFAVLYR3 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 90: /* LRM, RFMXLYR3 */
      strcpy( wmo_ttaai, "SDUS6" );
      strcpy( wmo_bbb1, "NHL" );
      break;
    case 93: /* ITWSDBV */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 94: /* DR */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "N0Q" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "NAQ" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N1Q" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "NBQ" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N2Q" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N3Q" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 95: /* CRPAPE95 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 96: /* CRPAPE96 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 97: /* CRPAPE97 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 98: /* CRPAPE98 */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 99: /* DV, BVEL8BIT */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "N0U" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS5" );
        strcpy( wmo_bbb1, "NAU" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N1U" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "NBU" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N2U" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N3U" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 132: /* RECCLREF */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 133: /* RECCLDOP */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 134: /* DVL, HRVIL */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "DVL" );
      break;
    case 135: /* EET, HREET */
      strcpy( wmo_ttaai, "SDUS7" );
      strcpy( wmo_bbb1, "EET" );
      break;
    case 136: /* SUPEROBVEL */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 137: /* ULRDATA */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 138: /* DSP, HYDIGSTM */
      strcpy( wmo_ttaai, "SDUS5" );
      strcpy( wmo_bbb1, "DSP" );
      break;
    case 140: /* MIGFA */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 141: /* MD, MDAPROD */
      strcpy( wmo_ttaai, "SDUS3" );
      strcpy( wmo_bbb1, "NMD" );
      break;
    case 143: /* TRU */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 144: /* OSWACCUM */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 145: /* OSDACCUM */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 146: /* SSWACCUM */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 147: /* SSDACCUM */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 149: /* DMDPROD */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 150: /* USWACCUM */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 151: /* USDACCUM */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 152: /* ASP, STATPROD */
      strcpy( wmo_ttaai, "SDUS4" );
      strcpy( wmo_bbb1, "RSL" );
      break;
    case 153: /* SR_BREF8BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 154: /* SR_BVEL8BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 155: /* SR_BWID8BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 156: /* NTDA_EDR */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 157: /* NTDA_CONF */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 158: /* ZDR4BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 159: /* DZD, ZDR8BIT */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N0X" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NAX" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N1X" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NBX" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N2X" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS2" );
        strcpy( wmo_bbb1, "N3X" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 160: /* CC4BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 161: /* DCC, CC8BIT */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N0C" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NAC" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N1C" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NBC" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N2C" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N3C" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 162: /* KDP4BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 163: /* DKD, KDP8BIT */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N0K" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NAK" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N1K" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NBK" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N2K" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N3K" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 164: /* HC4BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 165: /* DHC, HC8BIT */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N0H" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NAH" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N1H" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NBH" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N2H" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N3H" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 166: /* ML, MLPROD */
      param3 = SHORT_BSWAP( gp->param_3 );
      if( param3 == 5 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N0M" );
      }
      else if( param3 == 9 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NAM" );
      }
      else if( param3 == 13 || param3 == 15 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N1M" );
      }
      else if( param3 == 18 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "NBM" );
      }
      else if( param3 == 24 || param3 == 25 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N2M" );
      }
      else if( param3 == 31 || param3 == 34 || param3 == 35 )
      {
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "N3M" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param3 %d has no WMO header", product_code, param3 );
        return NULL;
      }
      break;
    case 169: /* OHA, OHAPROD */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "OHA" );
      break;
    case 170: /* DAA, DAAPROD */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "DAA" );
      break;
    case 171: /* STA, STAPROD */
      strcpy( wmo_ttaai, "SDUS3" );
      strcpy( wmo_bbb1, "PTA" );
      break;
    case 172: /* DSA, DSAPROD */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "DTA" );
      break;
    case 173: /* DUA, DUAPROD */
      param1 = SHORT_BSWAP( gp->param_1 );
      param2 = SHORT_BSWAP( gp->param_2 );
      /* param1 is end accumulation time in minutes after 00Z (0-1439) */
      /* param2 is accumulation time span in minutes (15-1440) */
      if( ( param1 % 60 ) == 0 && param2 == 180 )
      {
        /* 3hr DUA each hour */
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "DU3" );
      }
      else if( param1 == 720 && param2 == 1440 )
      {
        /* 24hr DUA at 12Z */
        strcpy( wmo_ttaai, "SDUS8" );
        strcpy( wmo_bbb1, "DU6" );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Product code %d with param1 %d and param2 %d has no WMO header", product_code, param1, param2 );
        return NULL;
      }
      break;
    case 174: /* DOD, DODPROD */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "DOD" );
      break;
    case 175: /* DSD, DSDPROD */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "DSD" );
      break;
    case 176: /* DPR, DPRPROD */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "DPR" );
      break;
    case 177: /* HHC, HHC8BIT */
      strcpy( wmo_ttaai, "SDUS8" );
      strcpy( wmo_bbb1, "HHC" );
      break;
    case 178: /* IHL */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 179: /* HHL */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 194: /* BREF7BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 195: /* DRQ */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    case 199: /* BVEL7BIT */
      LE_send_msg( GL_INFO, "Product code %d has no WMO header", product_code );
      return NULL;
      break;
    default :
      LE_send_msg( GL_ERROR, "Unknown product code %d", product_code );
      return NULL;
      break;
  }

  /* Format of WMO header:  "SDUS54 KCRI 150452\r\r\nN0QCRI\r\r\n" */
  sprintf( wmo_hdr, "%s%1d %s %s\r\r\n%s%s\r\r\n", wmo_ttaai, wmo_region, wmo_cccc, wmo_timestring, wmo_bbb1, wmo_bbb2 );

  RPG_LDM_print_debug( "Leave Get_WMO_header with header %s", wmo_hdr );

  return &wmo_hdr[0];
}

/**************************************************************************
 Description: Get WMO timestring for given product header.
**************************************************************************/

static char *Get_WMO_timestring( Graphic_product *gp )
{
  int gen_date = SHORT_BSWAP( gp->gen_date );
  int gen_time = INT_BSWAP( gp->gen_time );
  time_t epoch_secs = ( ( gen_date - 1 ) * RPG_LDM_SECONDS_PER_DAY ) + gen_time;
  static char timebuf[7];

    if( epoch_secs < 1 || ! strftime( timebuf, 7, "%d%H%M", gmtime( &epoch_secs ) ) )
  {
    LE_send_msg( GL_ERROR, "Bad timestring for %ld epoch time (gen_date: %d gen_time: %d)", epoch_secs, gen_date, gen_time );
    return NULL;
  }

  return &timebuf[0];
}

/**************************************************************************
 Description: Get WMO region for given ICAO.
**************************************************************************/

static int Get_WMO_region( char *icao )
{
       if( strcmp( icao, "KABR" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KABX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KAKQ" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KAMA" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KAMX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KAPX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KARX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KATX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KBBX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KBGM" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KBHX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KBIS" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KBLX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KBMX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KBOX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KBRO" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KBUF" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KBYX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KCAE" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KCBW" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KCBX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KCCX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KCLE" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KCLX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KCRP" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KCXX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KCYS" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KDAX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KDDC" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KDFX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KDGX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KDIX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KDLH" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KDMX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KDOX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KDTX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KDVN" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KDYX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KEAX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KEMX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KENX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KEOX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KEPZ" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KESX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KEVX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KEWX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KEYX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KFCX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KFDR" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KFDX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KFFC" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KFSD" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KFSX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KFTG" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KFWS" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KGGW" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KGJX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KGLD" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KGRB" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KGRK" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KGRR" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KGSP" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KGWX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KGYX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KHDX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KHGX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KHNX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KHPX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KHTX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KICT" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KICX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KILN" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KILX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KIND" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KINX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KIWA" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KIWX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KJAX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KJGX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KJKL" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KLBB" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KLCH" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KLGX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KLIX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KLNX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KLOT" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KLRX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KLSX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KLTX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KLVX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KLWX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KLZK" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KMAF" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KMAX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KMBX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KMHX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KMKX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KMLB" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KMOB" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KMPX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KMQT" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KMRX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KMSX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KMTX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KMUX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KMVX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KMXX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KNKX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KNQA" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KOAX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KOHX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KOKX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KOTX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KPAH" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KPBZ" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KPDT" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KPOE" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KPUX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KRAX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KRGX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KRIW" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KRLX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KRTX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KSFX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KSGF" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KSHV" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KSJT" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KSOX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KSRX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KTBW" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KTFX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "KTLH" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KTLX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KTWX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KTYX" ) == 0 ){ return WMO_NORTHEAST; }
  else if( strcmp( icao, "KUDX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KUEX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KVAX" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "KVBX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KVNX" ) == 0 ){ return WMO_SOUTHCENTRAL; }
  else if( strcmp( icao, "KVTX" ) == 0 ){ return WMO_WEST_COAST; }
  else if( strcmp( icao, "KVWX" ) == 0 ){ return WMO_NORTHCENTRAL; }
  else if( strcmp( icao, "KYUX" ) == 0 ){ return WMO_ROCKY_MTS; }
  else if( strcmp( icao, "PABC" ) == 0 ){ return WMO_CENTRAL_PACIFIC; }
  else if( strcmp( icao, "PACG" ) == 0 ){ return WMO_SOUTHEAST_AK; }
  else if( strcmp( icao, "PAEC" ) == 0 ){ return WMO_NORTHERN_AK; }
  else if( strcmp( icao, "PAHG" ) == 0 ){ return WMO_CENTRAL_PACIFIC; }
  else if( strcmp( icao, "PAIH" ) == 0 ){ return WMO_CENTRAL_PACIFIC; }
  else if( strcmp( icao, "PAKC" ) == 0 ){ return WMO_CENTRAL_PACIFIC; }
  else if( strcmp( icao, "PAPD" ) == 0 ){ return WMO_NORTHERN_AK; }
  else if( strcmp( icao, "PGUA" ) == 0 ){ return WMO_PACIFIC; }
  else if( strcmp( icao, "PHKI" ) == 0 ){ return WMO_PACIFIC; }
  else if( strcmp( icao, "PHKM" ) == 0 ){ return WMO_PACIFIC; }
  else if( strcmp( icao, "PHMO" ) == 0 ){ return WMO_PACIFIC; }
  else if( strcmp( icao, "PHWA" ) == 0 ){ return WMO_PACIFIC; }
  else if( strcmp( icao, "TJUA" ) == 0 ){ return WMO_SOUTHEAST; }
  else if( strcmp( icao, "LPLA" ) == 0 ){ return WMO_PACIFIC; } /* ??? */
  else if( strcmp( icao, "RKJK" ) == 0 ){ return WMO_PACIFIC; } /* ??? */
  else if( strcmp( icao, "RKSG" ) == 0 ){ return WMO_PACIFIC; } /* ??? */
  else if( strcmp( icao, "RODN" ) == 0 ){ return WMO_PACIFIC; } /* ??? */
  else if( strcmp( icao, "RSHI" ) == 0 ){ return WMO_PACIFIC; } /* ??? */
  else if( strcmp( icao, "RCWF" ) == 0 ){ return WMO_PACIFIC; } /* ??? */
  else if( strcmp( icao, "KOUN" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "NOP3" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "NOP4" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "ROP3" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "ROP4" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "FOP1" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "DOP1" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "DAN1" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else if( strcmp( icao, "KCRI" ) == 0 ){ return WMO_SOUTHCENTRAL; } /* ??? */
  else
  {
    LE_send_msg( GL_ERROR, "Invalid ICAO %s for region", icao );
  }
  return -1;
}

/**************************************************************************
 Description: Get WMO header CCCC for given ICAO.
**************************************************************************/

static char *Get_WMO_cccc( char *icao )
{
  static char cccc[5];

       if( strcmp( icao, "KABR" ) == 0 ){ strcpy( cccc, "KABR" ); }
  else if( strcmp( icao, "KABX" ) == 0 ){ strcpy( cccc, "KABQ" ); }
  else if( strcmp( icao, "KAKQ" ) == 0 ){ strcpy( cccc, "KAKQ" ); }
  else if( strcmp( icao, "KAMA" ) == 0 ){ strcpy( cccc, "KAMA" ); }
  else if( strcmp( icao, "KAMX" ) == 0 ){ strcpy( cccc, "KMFL" ); }
  else if( strcmp( icao, "KAPX" ) == 0 ){ strcpy( cccc, "KAPX" ); }
  else if( strcmp( icao, "KARX" ) == 0 ){ strcpy( cccc, "KARX" ); }
  else if( strcmp( icao, "KATX" ) == 0 ){ strcpy( cccc, "KSEW" ); }
  else if( strcmp( icao, "KBBX" ) == 0 ){ strcpy( cccc, "KSTO" ); }
  else if( strcmp( icao, "KBGM" ) == 0 ){ strcpy( cccc, "KBGM" ); }
  else if( strcmp( icao, "KBHX" ) == 0 ){ strcpy( cccc, "KEKA" ); }
  else if( strcmp( icao, "KBIS" ) == 0 ){ strcpy( cccc, "KBIS" ); }
  else if( strcmp( icao, "KBLX" ) == 0 ){ strcpy( cccc, "KBYZ" ); }
  else if( strcmp( icao, "KBMX" ) == 0 ){ strcpy( cccc, "KBMX" ); }
  else if( strcmp( icao, "KBOX" ) == 0 ){ strcpy( cccc, "KBOX" ); }
  else if( strcmp( icao, "KBRO" ) == 0 ){ strcpy( cccc, "KBRO" ); }
  else if( strcmp( icao, "KBUF" ) == 0 ){ strcpy( cccc, "KBUF" ); }
  else if( strcmp( icao, "KBYX" ) == 0 ){ strcpy( cccc, "KKEY" ); }
  else if( strcmp( icao, "KCAE" ) == 0 ){ strcpy( cccc, "KCAE" ); }
  else if( strcmp( icao, "KCBW" ) == 0 ){ strcpy( cccc, "KCAR" ); }
  else if( strcmp( icao, "KCBX" ) == 0 ){ strcpy( cccc, "KBOI" ); }
  else if( strcmp( icao, "KCCX" ) == 0 ){ strcpy( cccc, "KCTP" ); }
  else if( strcmp( icao, "KCLE" ) == 0 ){ strcpy( cccc, "KCLE" ); }
  else if( strcmp( icao, "KCLX" ) == 0 ){ strcpy( cccc, "KCHS" ); }
  else if( strcmp( icao, "KCRP" ) == 0 ){ strcpy( cccc, "KCRP" ); }
  else if( strcmp( icao, "KCXX" ) == 0 ){ strcpy( cccc, "KBTV" ); }
  else if( strcmp( icao, "KCYS" ) == 0 ){ strcpy( cccc, "KCYS" ); }
  else if( strcmp( icao, "KDAX" ) == 0 ){ strcpy( cccc, "KSTO" ); }
  else if( strcmp( icao, "KDDC" ) == 0 ){ strcpy( cccc, "KDDC" ); }
  else if( strcmp( icao, "KDFX" ) == 0 ){ strcpy( cccc, "KEWX" ); }
  else if( strcmp( icao, "KDGX" ) == 0 ){ strcpy( cccc, "KJAN" ); }
  else if( strcmp( icao, "KDIX" ) == 0 ){ strcpy( cccc, "KPHI" ); }
  else if( strcmp( icao, "KDLH" ) == 0 ){ strcpy( cccc, "KDLH" ); }
  else if( strcmp( icao, "KDMX" ) == 0 ){ strcpy( cccc, "KDMX" ); }
  else if( strcmp( icao, "KDOX" ) == 0 ){ strcpy( cccc, "KAKQ" ); }
  else if( strcmp( icao, "KDTX" ) == 0 ){ strcpy( cccc, "KDTX" ); }
  else if( strcmp( icao, "KDVN" ) == 0 ){ strcpy( cccc, "KDVN" ); }
  else if( strcmp( icao, "KDYX" ) == 0 ){ strcpy( cccc, "KSJT" ); }
  else if( strcmp( icao, "KEAX" ) == 0 ){ strcpy( cccc, "KEAX" ); }
  else if( strcmp( icao, "KEMX" ) == 0 ){ strcpy( cccc, "KTWC" ); }
  else if( strcmp( icao, "KENX" ) == 0 ){ strcpy( cccc, "KALY" ); }
  else if( strcmp( icao, "KEOX" ) == 0 ){ strcpy( cccc, "KTAE" ); }
  else if( strcmp( icao, "KEPZ" ) == 0 ){ strcpy( cccc, "KEPZ" ); }
  else if( strcmp( icao, "KESX" ) == 0 ){ strcpy( cccc, "KVEF" ); }
  else if( strcmp( icao, "KEVX" ) == 0 ){ strcpy( cccc, "KMOB" ); }
  else if( strcmp( icao, "KEWX" ) == 0 ){ strcpy( cccc, "KEWX" ); }
  else if( strcmp( icao, "KEYX" ) == 0 ){ strcpy( cccc, "KVEF" ); }
  else if( strcmp( icao, "KFCX" ) == 0 ){ strcpy( cccc, "KRNK" ); }
  else if( strcmp( icao, "KFDR" ) == 0 ){ strcpy( cccc, "KOUN" ); }
  else if( strcmp( icao, "KFDX" ) == 0 ){ strcpy( cccc, "KABQ" ); }
  else if( strcmp( icao, "KFFC" ) == 0 ){ strcpy( cccc, "KFFC" ); }
  else if( strcmp( icao, "KFSD" ) == 0 ){ strcpy( cccc, "KFSD" ); }
  else if( strcmp( icao, "KFSX" ) == 0 ){ strcpy( cccc, "KFGZ" ); }
  else if( strcmp( icao, "KFTG" ) == 0 ){ strcpy( cccc, "KBOU" ); }
  else if( strcmp( icao, "KFWS" ) == 0 ){ strcpy( cccc, "KFWD" ); }
  else if( strcmp( icao, "KGGW" ) == 0 ){ strcpy( cccc, "KGGW" ); }
  else if( strcmp( icao, "KGJX" ) == 0 ){ strcpy( cccc, "KGJT" ); }
  else if( strcmp( icao, "KGLD" ) == 0 ){ strcpy( cccc, "KGLD" ); }
  else if( strcmp( icao, "KGRB" ) == 0 ){ strcpy( cccc, "KGRB" ); }
  else if( strcmp( icao, "KGRK" ) == 0 ){ strcpy( cccc, "KFWD" ); }
  else if( strcmp( icao, "KGRR" ) == 0 ){ strcpy( cccc, "KGRR" ); }
  else if( strcmp( icao, "KGSP" ) == 0 ){ strcpy( cccc, "KGSP" ); }
  else if( strcmp( icao, "KGWX" ) == 0 ){ strcpy( cccc, "KJAN" ); }
  else if( strcmp( icao, "KGYX" ) == 0 ){ strcpy( cccc, "KGYX" ); }
  else if( strcmp( icao, "KHDX" ) == 0 ){ strcpy( cccc, "KEPZ" ); }
  else if( strcmp( icao, "KHGX" ) == 0 ){ strcpy( cccc, "KHGX" ); }
  else if( strcmp( icao, "KHNX" ) == 0 ){ strcpy( cccc, "KHNX" ); }
  else if( strcmp( icao, "KHPX" ) == 0 ){ strcpy( cccc, "KPAH" ); }
  else if( strcmp( icao, "KHTX" ) == 0 ){ strcpy( cccc, "KHUN" ); }
  else if( strcmp( icao, "KICT" ) == 0 ){ strcpy( cccc, "KICT" ); }
  else if( strcmp( icao, "KICX" ) == 0 ){ strcpy( cccc, "KLSC" ); }
  else if( strcmp( icao, "KILN" ) == 0 ){ strcpy( cccc, "KILN" ); }
  else if( strcmp( icao, "KILX" ) == 0 ){ strcpy( cccc, "KILX" ); }
  else if( strcmp( icao, "KIND" ) == 0 ){ strcpy( cccc, "KIND" ); }
  else if( strcmp( icao, "KINX" ) == 0 ){ strcpy( cccc, "KTSA" ); }
  else if( strcmp( icao, "KIWA" ) == 0 ){ strcpy( cccc, "KPSR" ); }
  else if( strcmp( icao, "KIWX" ) == 0 ){ strcpy( cccc, "KIWX" ); }
  else if( strcmp( icao, "KJAX" ) == 0 ){ strcpy( cccc, "KJAX" ); }
  else if( strcmp( icao, "KJGX" ) == 0 ){ strcpy( cccc, "KFFC" ); }
  else if( strcmp( icao, "KJKL" ) == 0 ){ strcpy( cccc, "KJKL" ); }
  else if( strcmp( icao, "KLBB" ) == 0 ){ strcpy( cccc, "KLUB" ); }
  else if( strcmp( icao, "KLCH" ) == 0 ){ strcpy( cccc, "KLCH" ); }
  else if( strcmp( icao, "KLGX" ) == 0 ){ strcpy( cccc, "KSEW" ); }
  else if( strcmp( icao, "KLIX" ) == 0 ){ strcpy( cccc, "KLIX" ); }
  else if( strcmp( icao, "KLNX" ) == 0 ){ strcpy( cccc, "KLBF" ); }
  else if( strcmp( icao, "KLOT" ) == 0 ){ strcpy( cccc, "KLOT" ); }
  else if( strcmp( icao, "KLRX" ) == 0 ){ strcpy( cccc, "KLKN" ); }
  else if( strcmp( icao, "KLSX" ) == 0 ){ strcpy( cccc, "KLSX" ); }
  else if( strcmp( icao, "KLTX" ) == 0 ){ strcpy( cccc, "KILM" ); }
  else if( strcmp( icao, "KLVX" ) == 0 ){ strcpy( cccc, "KLMK" ); }
  else if( strcmp( icao, "KLWX" ) == 0 ){ strcpy( cccc, "KLWX" ); }
  else if( strcmp( icao, "KLZK" ) == 0 ){ strcpy( cccc, "KLZK" ); }
  else if( strcmp( icao, "KMAF" ) == 0 ){ strcpy( cccc, "KMAF" ); }
  else if( strcmp( icao, "KMAX" ) == 0 ){ strcpy( cccc, "KMFR" ); }
  else if( strcmp( icao, "KMBX" ) == 0 ){ strcpy( cccc, "KBIS" ); }
  else if( strcmp( icao, "KMHX" ) == 0 ){ strcpy( cccc, "KMHX" ); }
  else if( strcmp( icao, "KMKX" ) == 0 ){ strcpy( cccc, "KMKX" ); }
  else if( strcmp( icao, "KMLB" ) == 0 ){ strcpy( cccc, "KMLB" ); }
  else if( strcmp( icao, "KMOB" ) == 0 ){ strcpy( cccc, "KMOB" ); }
  else if( strcmp( icao, "KMPX" ) == 0 ){ strcpy( cccc, "KMPX" ); }
  else if( strcmp( icao, "KMQT" ) == 0 ){ strcpy( cccc, "KMQT" ); }
  else if( strcmp( icao, "KMRX" ) == 0 ){ strcpy( cccc, "KMRX" ); }
  else if( strcmp( icao, "KMSX" ) == 0 ){ strcpy( cccc, "KMSO" ); }
  else if( strcmp( icao, "KMTX" ) == 0 ){ strcpy( cccc, "KSLC" ); }
  else if( strcmp( icao, "KMUX" ) == 0 ){ strcpy( cccc, "KMTR" ); }
  else if( strcmp( icao, "KMVX" ) == 0 ){ strcpy( cccc, "KFGF" ); }
  else if( strcmp( icao, "KMXX" ) == 0 ){ strcpy( cccc, "KBMX" ); }
  else if( strcmp( icao, "KNKX" ) == 0 ){ strcpy( cccc, "KSGX" ); }
  else if( strcmp( icao, "KNQA" ) == 0 ){ strcpy( cccc, "KMEG" ); }
  else if( strcmp( icao, "KOAX" ) == 0 ){ strcpy( cccc, "KOAX" ); }
  else if( strcmp( icao, "KOHX" ) == 0 ){ strcpy( cccc, "KOHX" ); }
  else if( strcmp( icao, "KOKX" ) == 0 ){ strcpy( cccc, "KOKX" ); }
  else if( strcmp( icao, "KOTX" ) == 0 ){ strcpy( cccc, "KOTX" ); }
  else if( strcmp( icao, "KPAH" ) == 0 ){ strcpy( cccc, "KPAH" ); }
  else if( strcmp( icao, "KPBZ" ) == 0 ){ strcpy( cccc, "KPBZ" ); }
  else if( strcmp( icao, "KPDT" ) == 0 ){ strcpy( cccc, "KPDT" ); }
  else if( strcmp( icao, "KPOE" ) == 0 ){ strcpy( cccc, "KLCH" ); }
  else if( strcmp( icao, "KPUX" ) == 0 ){ strcpy( cccc, "KPUB" ); }
  else if( strcmp( icao, "KRAX" ) == 0 ){ strcpy( cccc, "KRAH" ); }
  else if( strcmp( icao, "KRGX" ) == 0 ){ strcpy( cccc, "KREV" ); }
  else if( strcmp( icao, "KRIW" ) == 0 ){ strcpy( cccc, "KRIW" ); }
  else if( strcmp( icao, "KRLX" ) == 0 ){ strcpy( cccc, "KRLX" ); }
  else if( strcmp( icao, "KRTX" ) == 0 ){ strcpy( cccc, "KPQR" ); }
  else if( strcmp( icao, "KSFX" ) == 0 ){ strcpy( cccc, "KPIH" ); }
  else if( strcmp( icao, "KSGF" ) == 0 ){ strcpy( cccc, "KSGF" ); }
  else if( strcmp( icao, "KSHV" ) == 0 ){ strcpy( cccc, "KSHV" ); }
  else if( strcmp( icao, "KSJT" ) == 0 ){ strcpy( cccc, "KSJT" ); }
  else if( strcmp( icao, "KSOX" ) == 0 ){ strcpy( cccc, "KSGX" ); }
  else if( strcmp( icao, "KSRX" ) == 0 ){ strcpy( cccc, "KTSA" ); }
  else if( strcmp( icao, "KTBW" ) == 0 ){ strcpy( cccc, "KTBW" ); }
  else if( strcmp( icao, "KTFX" ) == 0 ){ strcpy( cccc, "KTFX" ); }
  else if( strcmp( icao, "KTLH" ) == 0 ){ strcpy( cccc, "KTAE" ); }
  else if( strcmp( icao, "KTLX" ) == 0 ){ strcpy( cccc, "KOUN" ); }
  else if( strcmp( icao, "KTWX" ) == 0 ){ strcpy( cccc, "KTOP" ); }
  else if( strcmp( icao, "KTYX" ) == 0 ){ strcpy( cccc, "KBTV" ); }
  else if( strcmp( icao, "KUDX" ) == 0 ){ strcpy( cccc, "KUNR" ); }
  else if( strcmp( icao, "KUEX" ) == 0 ){ strcpy( cccc, "KGID" ); }
  else if( strcmp( icao, "KVAX" ) == 0 ){ strcpy( cccc, "KJAX" ); }
  else if( strcmp( icao, "KVBX" ) == 0 ){ strcpy( cccc, "KLOX" ); }
  else if( strcmp( icao, "KVNX" ) == 0 ){ strcpy( cccc, "KOUN" ); }
  else if( strcmp( icao, "KVTX" ) == 0 ){ strcpy( cccc, "KLOX" ); }
  else if( strcmp( icao, "KVWX" ) == 0 ){ strcpy( cccc, "KPAH" ); }
  else if( strcmp( icao, "KYUX" ) == 0 ){ strcpy( cccc, "KPSR" ); }
  else if( strcmp( icao, "PABC" ) == 0 ){ strcpy( cccc, "PACR" ); }
  else if( strcmp( icao, "PACG" ) == 0 ){ strcpy( cccc, "PAJK" ); }
  else if( strcmp( icao, "PAEC" ) == 0 ){ strcpy( cccc, "PAFG" ); }
  else if( strcmp( icao, "PAHG" ) == 0 ){ strcpy( cccc, "PAFC" ); }
  else if( strcmp( icao, "PAIH" ) == 0 ){ strcpy( cccc, "PAFC" ); }
  else if( strcmp( icao, "PAKC" ) == 0 ){ strcpy( cccc, "PACR" ); }
  else if( strcmp( icao, "PAPD" ) == 0 ){ strcpy( cccc, "PAFG" ); }
  else if( strcmp( icao, "PGUA" ) == 0 ){ strcpy( cccc, "PGUM" ); }
  else if( strcmp( icao, "PHKI" ) == 0 ){ strcpy( cccc, "PHFO" ); }
  else if( strcmp( icao, "PHKM" ) == 0 ){ strcpy( cccc, "PHFO" ); }
  else if( strcmp( icao, "PHMO" ) == 0 ){ strcpy( cccc, "PHFO" ); }
  else if( strcmp( icao, "PHWA" ) == 0 ){ strcpy( cccc, "PHFO" ); }
  else if( strcmp( icao, "TJUA" ) == 0 ){ strcpy( cccc, "TJSJ" ); }
  else if( strcmp( icao, "LPLA" ) == 0 ){ strcpy( cccc, "LPLA" ); } /* ??? */
  else if( strcmp( icao, "RKJK" ) == 0 ){ strcpy( cccc, "RKJK" ); } /* ??? */
  else if( strcmp( icao, "RKSG" ) == 0 ){ strcpy( cccc, "RKSG" ); } /* ??? */
  else if( strcmp( icao, "RODN" ) == 0 ){ strcpy( cccc, "RODN" ); } /* ??? */
  else if( strcmp( icao, "RSHI" ) == 0 ){ strcpy( cccc, "RSHI" ); } /* ??? */
  else if( strcmp( icao, "RCWF" ) == 0 ){ strcpy( cccc, "RCWF" ); } /* ??? */
  else if( strcmp( icao, "KOUN" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "NOP3" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "NOP4" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "ROP3" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "ROP4" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "FOP1" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "DOP1" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "DAN1" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else if( strcmp( icao, "KCRI" ) == 0 ){ strcpy( cccc, "KOUN" ); } /* ??? */
  else
  {
    LE_send_msg( GL_ERROR, "Invalid ICAO %s for CCCC", icao );
    return NULL;
  }

  return &cccc[0];
}

/**************************************************************************
 Description: Get WMO header BBB2 for given ICAO.
**************************************************************************/

static char *Get_WMO_bbb2( char *icao )
{
  static char bbb2[4];

       if( strcmp( icao, "KABR" ) == 0 ){ strcpy( bbb2, "ABR" ); }
  else if( strcmp( icao, "KABX" ) == 0 ){ strcpy( bbb2, "ABX" ); }
  else if( strcmp( icao, "KAKQ" ) == 0 ){ strcpy( bbb2, "AKQ" ); }
  else if( strcmp( icao, "KAMA" ) == 0 ){ strcpy( bbb2, "AMA" ); }
  else if( strcmp( icao, "KAMX" ) == 0 ){ strcpy( bbb2, "AMX" ); }
  else if( strcmp( icao, "KAPX" ) == 0 ){ strcpy( bbb2, "APX" ); }
  else if( strcmp( icao, "KARX" ) == 0 ){ strcpy( bbb2, "ARX" ); }
  else if( strcmp( icao, "KATX" ) == 0 ){ strcpy( bbb2, "ATX" ); }
  else if( strcmp( icao, "KBBX" ) == 0 ){ strcpy( bbb2, "BBX" ); }
  else if( strcmp( icao, "KBGM" ) == 0 ){ strcpy( bbb2, "BGM" ); }
  else if( strcmp( icao, "KBHX" ) == 0 ){ strcpy( bbb2, "BHX" ); }
  else if( strcmp( icao, "KBIS" ) == 0 ){ strcpy( bbb2, "BIS" ); }
  else if( strcmp( icao, "KBLX" ) == 0 ){ strcpy( bbb2, "BLX" ); }
  else if( strcmp( icao, "KBMX" ) == 0 ){ strcpy( bbb2, "BMX" ); }
  else if( strcmp( icao, "KBOX" ) == 0 ){ strcpy( bbb2, "BOX" ); }
  else if( strcmp( icao, "KBRO" ) == 0 ){ strcpy( bbb2, "BRO" ); }
  else if( strcmp( icao, "KBUF" ) == 0 ){ strcpy( bbb2, "BUF" ); }
  else if( strcmp( icao, "KBYX" ) == 0 ){ strcpy( bbb2, "BYX" ); }
  else if( strcmp( icao, "KCAE" ) == 0 ){ strcpy( bbb2, "CAE" ); }
  else if( strcmp( icao, "KCBW" ) == 0 ){ strcpy( bbb2, "CBW" ); }
  else if( strcmp( icao, "KCBX" ) == 0 ){ strcpy( bbb2, "CBX" ); }
  else if( strcmp( icao, "KCCX" ) == 0 ){ strcpy( bbb2, "CCX" ); }
  else if( strcmp( icao, "KCLE" ) == 0 ){ strcpy( bbb2, "CLE" ); }
  else if( strcmp( icao, "KCLX" ) == 0 ){ strcpy( bbb2, "CLX" ); }
  else if( strcmp( icao, "KCRP" ) == 0 ){ strcpy( bbb2, "CRP" ); }
  else if( strcmp( icao, "KCXX" ) == 0 ){ strcpy( bbb2, "CXX" ); }
  else if( strcmp( icao, "KCYS" ) == 0 ){ strcpy( bbb2, "CYS" ); }
  else if( strcmp( icao, "KDAX" ) == 0 ){ strcpy( bbb2, "DAX" ); }
  else if( strcmp( icao, "KDDC" ) == 0 ){ strcpy( bbb2, "DDC" ); }
  else if( strcmp( icao, "KDFX" ) == 0 ){ strcpy( bbb2, "DFX" ); }
  else if( strcmp( icao, "KDGX" ) == 0 ){ strcpy( bbb2, "DGX" ); }
  else if( strcmp( icao, "KDIX" ) == 0 ){ strcpy( bbb2, "DIX" ); }
  else if( strcmp( icao, "KDLH" ) == 0 ){ strcpy( bbb2, "DLH" ); }
  else if( strcmp( icao, "KDMX" ) == 0 ){ strcpy( bbb2, "DMX" ); }
  else if( strcmp( icao, "KDOX" ) == 0 ){ strcpy( bbb2, "DOX" ); }
  else if( strcmp( icao, "KDTX" ) == 0 ){ strcpy( bbb2, "DTX" ); }
  else if( strcmp( icao, "KDVN" ) == 0 ){ strcpy( bbb2, "DVN" ); }
  else if( strcmp( icao, "KDYX" ) == 0 ){ strcpy( bbb2, "DYX" ); }
  else if( strcmp( icao, "KEAX" ) == 0 ){ strcpy( bbb2, "EAX" ); }
  else if( strcmp( icao, "KEMX" ) == 0 ){ strcpy( bbb2, "EMX" ); }
  else if( strcmp( icao, "KENX" ) == 0 ){ strcpy( bbb2, "ENX" ); }
  else if( strcmp( icao, "KEOX" ) == 0 ){ strcpy( bbb2, "EOX" ); }
  else if( strcmp( icao, "KEPZ" ) == 0 ){ strcpy( bbb2, "EPZ" ); }
  else if( strcmp( icao, "KESX" ) == 0 ){ strcpy( bbb2, "ESX" ); }
  else if( strcmp( icao, "KEVX" ) == 0 ){ strcpy( bbb2, "EVX" ); }
  else if( strcmp( icao, "KEWX" ) == 0 ){ strcpy( bbb2, "EWX" ); }
  else if( strcmp( icao, "KEYX" ) == 0 ){ strcpy( bbb2, "EYX" ); }
  else if( strcmp( icao, "KFCX" ) == 0 ){ strcpy( bbb2, "FCX" ); }
  else if( strcmp( icao, "KFDR" ) == 0 ){ strcpy( bbb2, "FDR" ); }
  else if( strcmp( icao, "KFDX" ) == 0 ){ strcpy( bbb2, "FDX" ); }
  else if( strcmp( icao, "KFFC" ) == 0 ){ strcpy( bbb2, "FFC" ); }
  else if( strcmp( icao, "KFSD" ) == 0 ){ strcpy( bbb2, "FSD" ); }
  else if( strcmp( icao, "KFSX" ) == 0 ){ strcpy( bbb2, "FSX" ); }
  else if( strcmp( icao, "KFTG" ) == 0 ){ strcpy( bbb2, "FTG" ); }
  else if( strcmp( icao, "KFWS" ) == 0 ){ strcpy( bbb2, "FWS" ); }
  else if( strcmp( icao, "KGGW" ) == 0 ){ strcpy( bbb2, "GGW" ); }
  else if( strcmp( icao, "KGJX" ) == 0 ){ strcpy( bbb2, "GJX" ); }
  else if( strcmp( icao, "KGLD" ) == 0 ){ strcpy( bbb2, "GLD" ); }
  else if( strcmp( icao, "KGRB" ) == 0 ){ strcpy( bbb2, "GRB" ); }
  else if( strcmp( icao, "KGRK" ) == 0 ){ strcpy( bbb2, "GRK" ); }
  else if( strcmp( icao, "KGRR" ) == 0 ){ strcpy( bbb2, "GRR" ); }
  else if( strcmp( icao, "KGSP" ) == 0 ){ strcpy( bbb2, "GSP" ); }
  else if( strcmp( icao, "KGWX" ) == 0 ){ strcpy( bbb2, "GWX" ); }
  else if( strcmp( icao, "KGYX" ) == 0 ){ strcpy( bbb2, "GYX" ); }
  else if( strcmp( icao, "KHDX" ) == 0 ){ strcpy( bbb2, "HDX" ); }
  else if( strcmp( icao, "KHGX" ) == 0 ){ strcpy( bbb2, "HGX" ); }
  else if( strcmp( icao, "KHNX" ) == 0 ){ strcpy( bbb2, "HNX" ); }
  else if( strcmp( icao, "KHPX" ) == 0 ){ strcpy( bbb2, "HPX" ); }
  else if( strcmp( icao, "KHTX" ) == 0 ){ strcpy( bbb2, "HTX" ); }
  else if( strcmp( icao, "KICT" ) == 0 ){ strcpy( bbb2, "ICT" ); }
  else if( strcmp( icao, "KICX" ) == 0 ){ strcpy( bbb2, "ICX" ); }
  else if( strcmp( icao, "KILN" ) == 0 ){ strcpy( bbb2, "ILN" ); }
  else if( strcmp( icao, "KILX" ) == 0 ){ strcpy( bbb2, "ILX" ); }
  else if( strcmp( icao, "KIND" ) == 0 ){ strcpy( bbb2, "IND" ); }
  else if( strcmp( icao, "KINX" ) == 0 ){ strcpy( bbb2, "INX" ); }
  else if( strcmp( icao, "KIWA" ) == 0 ){ strcpy( bbb2, "IWA" ); }
  else if( strcmp( icao, "KIWX" ) == 0 ){ strcpy( bbb2, "IWX" ); }
  else if( strcmp( icao, "KJAX" ) == 0 ){ strcpy( bbb2, "JAX" ); }
  else if( strcmp( icao, "KJGX" ) == 0 ){ strcpy( bbb2, "JGX" ); }
  else if( strcmp( icao, "KJKL" ) == 0 ){ strcpy( bbb2, "JKL" ); }
  else if( strcmp( icao, "KLBB" ) == 0 ){ strcpy( bbb2, "LBB" ); }
  else if( strcmp( icao, "KLCH" ) == 0 ){ strcpy( bbb2, "LCH" ); }
  else if( strcmp( icao, "KLGX" ) == 0 ){ strcpy( bbb2, "LGX" ); }
  else if( strcmp( icao, "KLIX" ) == 0 ){ strcpy( bbb2, "LIX" ); }
  else if( strcmp( icao, "KLNX" ) == 0 ){ strcpy( bbb2, "LNX" ); }
  else if( strcmp( icao, "KLOT" ) == 0 ){ strcpy( bbb2, "LOT" ); }
  else if( strcmp( icao, "KLRX" ) == 0 ){ strcpy( bbb2, "LRX" ); }
  else if( strcmp( icao, "KLSX" ) == 0 ){ strcpy( bbb2, "LSX" ); }
  else if( strcmp( icao, "KLTX" ) == 0 ){ strcpy( bbb2, "LTX" ); }
  else if( strcmp( icao, "KLVX" ) == 0 ){ strcpy( bbb2, "LVX" ); }
  else if( strcmp( icao, "KLWX" ) == 0 ){ strcpy( bbb2, "LWX" ); }
  else if( strcmp( icao, "KLZK" ) == 0 ){ strcpy( bbb2, "LZK" ); }
  else if( strcmp( icao, "KMAF" ) == 0 ){ strcpy( bbb2, "MAF" ); }
  else if( strcmp( icao, "KMAX" ) == 0 ){ strcpy( bbb2, "MAX" ); }
  else if( strcmp( icao, "KMBX" ) == 0 ){ strcpy( bbb2, "MBX" ); }
  else if( strcmp( icao, "KMHX" ) == 0 ){ strcpy( bbb2, "MHX" ); }
  else if( strcmp( icao, "KMKX" ) == 0 ){ strcpy( bbb2, "MKX" ); }
  else if( strcmp( icao, "KMLB" ) == 0 ){ strcpy( bbb2, "MLB" ); }
  else if( strcmp( icao, "KMOB" ) == 0 ){ strcpy( bbb2, "MOB" ); }
  else if( strcmp( icao, "KMPX" ) == 0 ){ strcpy( bbb2, "MPX" ); }
  else if( strcmp( icao, "KMQT" ) == 0 ){ strcpy( bbb2, "MQT" ); }
  else if( strcmp( icao, "KMRX" ) == 0 ){ strcpy( bbb2, "MRX" ); }
  else if( strcmp( icao, "KMSX" ) == 0 ){ strcpy( bbb2, "MSX" ); }
  else if( strcmp( icao, "KMTX" ) == 0 ){ strcpy( bbb2, "MTX" ); }
  else if( strcmp( icao, "KMUX" ) == 0 ){ strcpy( bbb2, "MUX" ); }
  else if( strcmp( icao, "KMVX" ) == 0 ){ strcpy( bbb2, "MVX" ); }
  else if( strcmp( icao, "KMXX" ) == 0 ){ strcpy( bbb2, "MXX" ); }
  else if( strcmp( icao, "KNKX" ) == 0 ){ strcpy( bbb2, "NKX" ); }
  else if( strcmp( icao, "KNQA" ) == 0 ){ strcpy( bbb2, "NQA" ); }
  else if( strcmp( icao, "KOAX" ) == 0 ){ strcpy( bbb2, "OAX" ); }
  else if( strcmp( icao, "KOHX" ) == 0 ){ strcpy( bbb2, "OHX" ); }
  else if( strcmp( icao, "KOKX" ) == 0 ){ strcpy( bbb2, "OKX" ); }
  else if( strcmp( icao, "KOTX" ) == 0 ){ strcpy( bbb2, "OTX" ); }
  else if( strcmp( icao, "KPAH" ) == 0 ){ strcpy( bbb2, "PAH" ); }
  else if( strcmp( icao, "KPBZ" ) == 0 ){ strcpy( bbb2, "PBZ" ); }
  else if( strcmp( icao, "KPDT" ) == 0 ){ strcpy( bbb2, "PDT" ); }
  else if( strcmp( icao, "KPOE" ) == 0 ){ strcpy( bbb2, "POE" ); }
  else if( strcmp( icao, "KPUX" ) == 0 ){ strcpy( bbb2, "PUX" ); }
  else if( strcmp( icao, "KRAX" ) == 0 ){ strcpy( bbb2, "RAX" ); }
  else if( strcmp( icao, "KRGX" ) == 0 ){ strcpy( bbb2, "RGX" ); }
  else if( strcmp( icao, "KRIW" ) == 0 ){ strcpy( bbb2, "RIW" ); }
  else if( strcmp( icao, "KRLX" ) == 0 ){ strcpy( bbb2, "RLX" ); }
  else if( strcmp( icao, "KRTX" ) == 0 ){ strcpy( bbb2, "RTX" ); }
  else if( strcmp( icao, "KSFX" ) == 0 ){ strcpy( bbb2, "SFX" ); }
  else if( strcmp( icao, "KSGF" ) == 0 ){ strcpy( bbb2, "SGF" ); }
  else if( strcmp( icao, "KSHV" ) == 0 ){ strcpy( bbb2, "SHV" ); }
  else if( strcmp( icao, "KSJT" ) == 0 ){ strcpy( bbb2, "SJT" ); }
  else if( strcmp( icao, "KSOX" ) == 0 ){ strcpy( bbb2, "SOX" ); }
  else if( strcmp( icao, "KSRX" ) == 0 ){ strcpy( bbb2, "SRX" ); }
  else if( strcmp( icao, "KTBW" ) == 0 ){ strcpy( bbb2, "BTW" ); }
  else if( strcmp( icao, "KTFX" ) == 0 ){ strcpy( bbb2, "TFX" ); }
  else if( strcmp( icao, "KTLH" ) == 0 ){ strcpy( bbb2, "TLH" ); }
  else if( strcmp( icao, "KTLX" ) == 0 ){ strcpy( bbb2, "TLX" ); }
  else if( strcmp( icao, "KTWX" ) == 0 ){ strcpy( bbb2, "TWX" ); }
  else if( strcmp( icao, "KTYX" ) == 0 ){ strcpy( bbb2, "TYX" ); }
  else if( strcmp( icao, "KUDX" ) == 0 ){ strcpy( bbb2, "UDX" ); }
  else if( strcmp( icao, "KUEX" ) == 0 ){ strcpy( bbb2, "UEX" ); }
  else if( strcmp( icao, "KVAX" ) == 0 ){ strcpy( bbb2, "VAX" ); }
  else if( strcmp( icao, "KVBX" ) == 0 ){ strcpy( bbb2, "VBX" ); }
  else if( strcmp( icao, "KVNX" ) == 0 ){ strcpy( bbb2, "VNX" ); }
  else if( strcmp( icao, "KVTX" ) == 0 ){ strcpy( bbb2, "VTX" ); }
  else if( strcmp( icao, "KVWX" ) == 0 ){ strcpy( bbb2, "VWX" ); }
  else if( strcmp( icao, "KYUX" ) == 0 ){ strcpy( bbb2, "YUX" ); }
  else if( strcmp( icao, "PABC" ) == 0 ){ strcpy( bbb2, "ABC" ); }
  else if( strcmp( icao, "PACG" ) == 0 ){ strcpy( bbb2, "ACG" ); }
  else if( strcmp( icao, "PAEC" ) == 0 ){ strcpy( bbb2, "AEC" ); }
  else if( strcmp( icao, "PAHG" ) == 0 ){ strcpy( bbb2, "AHG" ); }
  else if( strcmp( icao, "PAIH" ) == 0 ){ strcpy( bbb2, "AIH" ); }
  else if( strcmp( icao, "PAKC" ) == 0 ){ strcpy( bbb2, "AKC" ); }
  else if( strcmp( icao, "PAPD" ) == 0 ){ strcpy( bbb2, "APD" ); }
  else if( strcmp( icao, "PGUA" ) == 0 ){ strcpy( bbb2, "GUA" ); }
  else if( strcmp( icao, "PHKI" ) == 0 ){ strcpy( bbb2, "HKI" ); }
  else if( strcmp( icao, "PHKM" ) == 0 ){ strcpy( bbb2, "HKM" ); }
  else if( strcmp( icao, "PHMO" ) == 0 ){ strcpy( bbb2, "HMO" ); }
  else if( strcmp( icao, "PHWA" ) == 0 ){ strcpy( bbb2, "HWA" ); }
  else if( strcmp( icao, "TJUA" ) == 0 ){ strcpy( bbb2, "JUA" ); }
  else if( strcmp( icao, "LPLA" ) == 0 ){ strcpy( bbb2, "PLA" ); } /* ??? */
  else if( strcmp( icao, "RKJK" ) == 0 ){ strcpy( bbb2, "KJK" ); } /* ??? */
  else if( strcmp( icao, "RKSG" ) == 0 ){ strcpy( bbb2, "KSG" ); } /* ??? */
  else if( strcmp( icao, "RODN" ) == 0 ){ strcpy( bbb2, "ODN" ); } /* ??? */
  else if( strcmp( icao, "RSHI" ) == 0 ){ strcpy( bbb2, "SHI" ); } /* ??? */
  else if( strcmp( icao, "RCWF" ) == 0 ){ strcpy( bbb2, "CWF" ); } /* ??? */
  else if( strcmp( icao, "KOUN" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "NOP3" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "NOP4" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "ROP3" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "ROP4" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "FOP1" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "DOP1" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "DAN1" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else if( strcmp( icao, "KCRI" ) == 0 ){ strcpy( bbb2, "OUN" ); } /* ??? */
  else
  {
    LE_send_msg( GL_ERROR, "Invalid ICAO %s for BBB2", icao );
    return NULL;
  }

  return &bbb2[0];
}

