/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:30:21 $
 * $Id: query_RPG_LDM_product.c,v 1.4 2012/10/30 12:30:21 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

/* Include files */

#include <query_product_print.h>

/* Defines/enums */

#define	PRODUCT_INDEX_INIT	-1
#define	CONTROL_WORD_SIZE	4
#define	MAX_FILENAME_LEN	256

/* Structures */

/* Static/global variables */

static int Product_index = PRODUCT_INDEX_INIT;
static int Msg_size = 0;
static int Msg_hdr_size = sizeof( RPG_LDM_msg_hdr_t );
static char *Msg_buf = NULL;
static char *Data_ptr = NULL;
static int Data_size = 0;
static int WMO_header_size = 0;
static int Print_LDM_control_word_flag = NO;
static int Print_msg_hdr_flag = NO;
static int Print_msg_flag = NO;
static int Read_input_from_file = NO;
static char Input_file[MAX_FILENAME_LEN+1];
static int Input_fd = 0;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Set_input_stream();
static void Print_usage();
static void Read_product();
static void Read_control_word();
static void Read_message();
static void Read_from_file_pointer( char *, int );

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  /* Parse command line. */
  Parse_cmd_line( argc, argv );

  /* Set the input stream. */
  Set_input_stream();

  /* Read product. */
  Read_product();

  if( Print_msg_flag )
  {
    /* Read correct product. */
    switch( Product_index )
    {
      case RPG_LDM_LDMPING_NL2_PROD:
        RPG_LDM_print_ICAO_ldmping_msg( Data_ptr );
        break;
      case RPG_LDM_ADAPT_PROD:
        RPG_LDM_print_adapt_msg( Data_ptr );
        break;
      case RPG_LDM_STATUS_LOG_PROD:
        RPG_LDM_print_status_log_msg( Data_ptr, Data_size );
        break;
      case RPG_LDM_ERROR_LOG_PROD:
        RPG_LDM_print_error_log_msg( Data_ptr, Data_size );
        break;
      case RPG_LDM_RDA_STATUS_PROD:
        RPG_LDM_print_RDA_status_msg( Data_ptr );
        break;
      case RPG_LDM_VOL_STATUS_PROD:
        RPG_LDM_print_volume_status_msg( Data_ptr );
        break;
      case RPG_LDM_WX_STATUS_PROD:
        RPG_LDM_print_wx_status_msg( Data_ptr );
        break;
      case RPG_LDM_CONSOLE_MSG_PROD:
        RPG_LDM_print_console_msg_msg( Data_ptr );
        break;
      case RPG_LDM_RPG_INFO_PROD:
        RPG_LDM_print_RPG_info_msg( Data_ptr );
        break;
      case RPG_LDM_TASK_STATUS_PROD:
        RPG_LDM_print_task_status_msg( Data_ptr, Data_size );
        break;
      case RPG_LDM_PROD_INFO_PROD:
        RPG_LDM_print_product_info_msg( Data_ptr );
        break;
      case RPG_LDM_SAVE_LOG_PROD:
        RPG_LDM_print_save_log_msg( Data_ptr );
        break;
      case RPG_LDM_BIAS_TABLE_PROD:
        RPG_LDM_print_bias_table_msg( Data_ptr );
        break;
      case RPG_LDM_RUC_DATA_PROD:
        RPG_LDM_print_RUC_data_msg( Data_ptr );
        break;
      case RPG_LDM_L2_WRITE_STATS_PROD:
        RPG_LDM_print_L2_write_stats_msg( Data_ptr, Data_size );
        break;
      case RPG_LDM_L2_READ_STATS_PROD:
        RPG_LDM_print_L2_read_stats_msg( Data_ptr, Data_size );
        break;
      case RPG_LDM_L3_PROD:
        RPG_LDM_print_L3_msg( Data_ptr, WMO_header_size );
        break;
      case RPG_LDM_SNMP_PROD:
        RPG_LDM_print_snmp_msg( Data_ptr, Data_size );
        break;
      case RPG_LDM_RTSTATS_PROD:
        RPG_LDM_print_rtstats_msg( Data_ptr );
        break;
      case RPG_LDM_SECURITY_PROD:
        RPG_LDM_print_security_msg( Data_ptr );
        break;
      case RPG_LDM_HW_STATS_PROD:
        RPG_LDM_print_hw_stats_msg( Data_ptr );
        break;
      case RPG_LDM_MISC_FILE_PROD:
        RPG_LDM_print_misc_file_msg( Data_ptr );
        break;
      case RPG_LDM_REQUEST_PROD:
        RPG_LDM_print_request_msg( Data_ptr );
        break;
      case RPG_LDM_COMMAND_PROD:
        RPG_LDM_print_command_msg( Data_ptr );
        break;
      default:
        Print_error( "Invalid index (%d)", Product_index );
        exit( 1 );
        break;
    }
  }

  return 0;
}

/************************************************************************
 Description: Print usage of this task.
 ************************************************************************/

static void Print_usage( char *task_name )
{
  Print_out( "Usage: %s -h (print usage info)", task_name );
  Print_out( "Usage: cat file | %s [Options (except -f)]", task_name );
  Print_out( "Usage: %s [Options]", task_name );
  Print_out( "  Options:" );
  Print_out( "  -x     - debug mode" );
  Print_out( "  -f str - name of input file (stdin by default)" );
  Print_out( "  -L     - print LDM control word" );
  Print_out( "  -R     - print RPG_LDM msg header (Default)" );
  Print_out( "  -M     - print RPG_LDM msg" );
}

/************************************************************************
 Description: Parse command line and take appropriate action.
 ************************************************************************/

static void Parse_cmd_line( int argc, char *argv[] )
{
  int c = 0;

  while( ( c = getopt( argc, argv, "hxLRMf:" ) ) != EOF )
  {
    switch( c )
    {
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
        break;
      case 'x':
        Set_debug_mode( YES );
        break;
      case 'L':
        Print_LDM_control_word_flag = YES;
        break;
      case 'R':
        Print_msg_hdr_flag = YES;
        break;
      case 'M':
        Print_msg_flag = YES;
        break;
      case 'f':
        if( optarg != NULL )
        {
          strncpy( Input_file, optarg, MAX_FILENAME_LEN );
          if( strlen( optarg ) >= MAX_FILENAME_LEN )
          {
            Input_file[MAX_FILENAME_LEN] = '\0';
          }
          Read_input_from_file = YES;
        } 
        break;
      case '?':
        Print_out( "Ignored option %c", (char) c );
        break;
      default:
        Print_error( "Illegal option %c. Terminate.", (char) c );
        Print_usage( argv[0] );
        exit( RPG_LDM_INVALID_COMMAND_LINE );
        break;
    }
  }

  /* By default, at least print msg header */
  if( !Print_LDM_control_word_flag && !Print_msg_flag )
  {
    Print_msg_hdr_flag = YES;
  }
}

/************************************************************************
 Description: Set input put stream information.
 ************************************************************************/

static void Set_input_stream()
{
  int ret = 0;

  if( Read_input_from_file )
  {
    if( ( Input_fd = open( Input_file, O_RDONLY ) ) < 0 )
    {
      Print_error( "Unable to open %s: %s", Input_file, strerror( errno ) );
      exit( 1 );
    }
  }
  else
  {
    Input_fd = STDIN_FILENO;
    /* If input stream is stdin, set it to be unbuffered. */
    if( ( ret = setvbuf( stdin, (char *) NULL, _IONBF, sizeof( _IONBF ) ) ) != 0 )
    {
      Print_error( "Setvbuf failed (%d)", ret );
      exit( 1 );
    }
  }
}

/************************************************************************
 Description: Read product from file pointer.
 ************************************************************************/

static void Read_product()
{
  RPG_LDM_msg_hdr_t *msg_hdr = NULL;

  /* Read control word to get size of message. */
  Read_control_word();

  if( Print_LDM_control_word_flag )
  {
    Print_out( "CONTROL WORD = %d bytes", Msg_size );
  }

  /* Read message. */
  Read_message();

  msg_hdr = (RPG_LDM_msg_hdr_t *) &Msg_buf[0];

  if( Print_msg_hdr_flag )
  {
    Print_RPG_LDM_msg_hdr( &Msg_buf[0], FORMAT_MSG_HDR_ALL );
  }

  Product_index = msg_hdr->code;
  Data_ptr = &Msg_buf[Msg_hdr_size];
  Data_size = msg_hdr->size;
  WMO_header_size = msg_hdr->wmo_header_size;
}

/************************************************************************
 Description: Read control word from file pointer.
 ************************************************************************/

static void Read_control_word()
{
  char *cw_buf = NULL;

  if( ( cw_buf = malloc( CONTROL_WORD_SIZE ) ) == NULL )
  {
    Print_error( "Malloc of cw_buf failed for %d bytes", CONTROL_WORD_SIZE );
    exit( 1 );
  }

  Read_from_file_pointer( cw_buf, CONTROL_WORD_SIZE );

  memcpy( &Msg_size, &cw_buf[0], CONTROL_WORD_SIZE );
  Print_debug( "Control word -> Msg_size: %d", Msg_size );

  free( cw_buf );
}

/************************************************************************
 Description: Read message from file pointer.
 ************************************************************************/

static void Read_message()
{
  if( ( Msg_buf = malloc( Msg_size ) ) == NULL )
  {
    Print_error( "Malloc of Msg_buf failed for %d bytes", Msg_size );
  }

  Read_from_file_pointer( Msg_buf, Msg_size );
}

/************************************************************************
 Description: Read message from file pointer.
 ************************************************************************/

static void Read_from_file_pointer( char *buf, int bytes_to_read )
{
  int bytes_read = 0;

  bytes_read = read( Input_fd, buf, bytes_to_read );
  Print_debug( "Read bytes: %d", bytes_read );

  if( bytes_read > 0 && bytes_read < bytes_to_read )
  {
    Print_error( "Only %d bytes of %d read", bytes_read, bytes_to_read );
    exit( 1 );
  }
  else if( bytes_read < 0 )
  {
    Print_error( "Read failed (%d)", bytes_read );
    exit( 1 );
  }
  else if( bytes_read > bytes_to_read )
  {
    Print_error( "Expected %d bytes, but read %d", bytes_to_read, bytes_read );
    exit( 1 );
  }
}

