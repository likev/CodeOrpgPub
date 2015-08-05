/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/08/21 13:42:12 $
 * $Id: RPG_bias_table_to_ldm.c,v 1.6 2012/08/21 13:42:12 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_TIMER	5      /* Time between checks for msgs */

/* Structures */

/* Static/global variables */

static int Read_data_flag = YES;
static int Lb_id = ORPGDAT_ENVIRON_DATA_MSG;
static int Msg_id = ORPGDAT_BIAS_TABLE_MSG_ID;
static char *Data_buffer = NULL;
static int Data_buffer_size = 0;
static char *Previous_data_buffer = NULL;
static int Previous_data_buffer_size = 0;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void Data_callback_fx( int, LB_id_t );
static void Read_data();
static void Write_to_LB();
static int  Is_new_buffer_different();

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  int ret = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Register UN events to be notified when data is updated */
  ORPGDA_write_permission( Lb_id );

  if( ( ret = ORPGDA_UN_register( Lb_id, Msg_id, Data_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_UN_register failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "ORPGDA_UN_register successful" );

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  /* Process events */
  while( 1 )
  {
    /* Read new data */
    if( Read_data_flag == YES )
    {
      Read_data_flag = NO;
      Read_data();
      Write_to_LB();
    }

    sleep( LOOP_TIMER );
  }
    
  return 0;
}

/************************************************************************
 Description: Print usage of this task.
 ************************************************************************/

static void Print_usage( char *task_name )
{
  printf( "Usage: %s [options]\n", task_name );
  printf( "  Options:\n" );
  printf( "  -T task name - only used when invoked by mrpg (optional)\n" );
  printf( "  -h   - print usage info\n" );
  RPG_LDM_print_usage();
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
  while( ( c = getopt( argc, argv, "T:h" ) ) != EOF )
  {
    switch( c )
    {
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
        break;
      case 'T':
        /* Ignore -T option */
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
}

/************************************************************************
 Description: Callback if data received.
 ************************************************************************/

static void Data_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "Data callback function called" );
  Read_data_flag = YES;
}

/************************************************************************
 Description: Read data from LB.
 ************************************************************************/

static void Read_data()
{
  int ret = 0;

  RPG_LDM_print_debug( "Enter Read_data()" );

  Data_buffer_size = 0;

  /* Read data from LB */
  ret = ORPGDA_read( Lb_id, &Data_buffer, LB_ALLOC_BUF, Msg_id );
  if( ret == LB_TO_COME )
  {
    LE_send_msg( GL_ERROR, "ORPGDA_read returned LB_TO_COME" );
  }
  else if( ret < 0 )
  {
    LE_send_msg( GL_ERROR, "ORPGDA_read failed (%d)\n", ret );
  }
  else
  {
    RPG_LDM_print_debug( "ORPGDA_read read %d bytes", ret );
    Data_buffer_size = ret;
  }

  RPG_LDM_print_debug( "Leave Read_data()" );
}

/************************************************************************
 Description: Write product to LB.
 ************************************************************************/

static void Write_to_LB()
{
  RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  char *product_buffer = NULL;
  int product_buffer_size = 0;
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int data_length = Data_buffer_size;
  int msg_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + msg_length;
  int ret = 0;
  int offset = 0;

  RPG_LDM_print_debug( "Enter Write_to_LB()" );

  if( Data_buffer_size == 0 )
  {
    RPG_LDM_print_debug( "Data buffer size is zero. Skip writing to LB." );
    return;
  }

  if( Is_new_buffer_different() == NO )
  {
    RPG_LDM_print_debug( "New buffer isn't different. Skip writing to LB." );
    return;
  }

  /* Initialize RPG LDM product header. */
  RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
  RPG_LDM_prod_hdr.timestamp = time( NULL );
  RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
  RPG_LDM_prod_hdr.feed_type = RPG_LDM_EXP_FEED_TYPE;
  RPG_LDM_prod_hdr.seq_num = 1;
  RPG_LDM_prod_hdr.data_len = msg_length;
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_BIAS_TABLE_PROD );

  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_BIAS_TABLE_PROD;
  RPG_LDM_msg_hdr.size = data_length;
  RPG_LDM_msg_hdr.timestamp = RPG_LDM_prod_hdr.timestamp;
  RPG_LDM_msg_hdr.segment_number = 1;
  RPG_LDM_msg_hdr.number_of_segments = 1;

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
  memcpy( &product_buffer[offset], &Data_buffer[0], data_length );

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
    RPG_LDM_print_bias_table_msg( &Data_buffer[0] );
  }

  free( product_buffer );
  free( Data_buffer );
  Data_buffer = NULL;
  Data_buffer_size = 0;

  RPG_LDM_print_debug( "Leave Write_to_LB()" );
}

/************************************************************************
 Description: Compare new buffer to previous buffer.
 ************************************************************************/

static int Is_new_buffer_different()
{
  static int need_to_init_flag = YES;
  
  if( need_to_init_flag )
  {
    /* Initialize if first time through */
    need_to_init_flag = NO;
    RPG_LDM_print_debug( "Initialize previous buffer" );
    Previous_data_buffer_size = Data_buffer_size;
    Previous_data_buffer = malloc( Previous_data_buffer_size );
    memcpy( &Previous_data_buffer[0], &Data_buffer[0], Data_buffer_size );
    RPG_LDM_print_debug( "Previous buffere size: %d", Data_buffer_size );
    RPG_LDM_print_debug( "Return previous buffer is different" );
    return YES;
  } 
    
  if( Data_buffer_size == Previous_data_buffer_size &&
      memcmp( &Data_buffer[0], &Previous_data_buffer[0], Data_buffer_size ) == 0 )
  {
    /* New and previous buffers are same */
    RPG_LDM_print_debug( "Return previous buffer is same" );
    return NO;
  }
  else
  {
    /* New and previous buffers differ */
    if( Data_buffer_size != Previous_data_buffer_size )
    {
      RPG_LDM_print_debug( "Sizes differ New: %d Previous: %d", Data_buffer_size, Previous_data_buffer_size );
    }
    else
    {
      RPG_LDM_print_debug( "New and previous buffers differ" );
    }
  
    /* Free previously allocated memory */
    if( Previous_data_buffer != NULL )
    {
      free( Previous_data_buffer );
      Previous_data_buffer = NULL;
    }
  
    /* Set previous variables to new values */
    Previous_data_buffer_size = Data_buffer_size;
    Previous_data_buffer = malloc( Previous_data_buffer_size );
    memcpy( &Previous_data_buffer[0], &Data_buffer[0], Data_buffer_size );
  }

  RPG_LDM_print_debug( "Return previous buffer is different" );
  return YES;
}

