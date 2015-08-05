/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:29:35 $
 * $Id: RPG_LDM_msgs_monitor.c,v 1.6 2012/10/30 12:29:35 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_INTERVAL		5	/* Max seconds between msgs */
#define	MAX_LB_INPUT_STRING_LEN	16
#define	MAX_PRODUCT_STRING_LEN	64

/* Structures */

/* Static/global variables */

static time_t   Stats_start_time = 0;
static int      Read_writer_LB_flag = NO;
static int      Read_reader_LB_flag = NO;
static int      Read_VST_msg_flag = NO;
static int      Write_stats_flag = NO;
static int      Writer_num_msgs[RPG_LDM_NUM_LDM_PRODS] = {0};
static long     Writer_total_size[RPG_LDM_NUM_LDM_PRODS] = {0};
static int      Reader_num_msgs[RPG_LDM_NUM_LDM_PRODS] = {0};
static long     Reader_total_size[RPG_LDM_NUM_LDM_PRODS] = {0};
static int      Num_volumes = 0;
static int      Num_hours = 0;
static int      Num_days = 0;
static int      Num_months = 0;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void LB_writer_callback_fx( int, LB_id_t );
static void LB_reader_callback_fx( int, LB_id_t );
static void VST_callback_fx( int, LB_id_t );
static void Process_writer_LB();
static void Process_reader_LB();
static void Write_stats_of_outgoing_msgs();
static void Write_stats_of_incoming_msgs();

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  time_t current_time = 0;
  struct tm *gmt_time = NULL;
  static int previous_hour = 0;
  static int previous_day = 0;
  static int previous_month = 0;
  static int previous_volume = 0;
  int current_hour = 0;
  int current_day = 0;
  int current_month = 0;
  int current_volume = 0;
  int ret = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Register UN events to be notified for writer LB */
  ORPGDA_write_permission( ORPGDAT_LDM_WRITER_INPUT );
  if( ( ret = ORPGDA_UN_register( ORPGDAT_LDM_WRITER_INPUT, LB_ANY, LB_writer_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_UN_register(ORPGDAT_LB_WRITER_INPUT) failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "ORPGDA_UN_register(ORPGDAT_LB_WRITER_INPUT) successful" );

  /* Register UN events to be notified for reader LB */
  ORPGDA_write_permission( ORPGDAT_LDM_READER_INPUT );
  if( ( ret = ORPGDA_UN_register( ORPGDAT_LDM_READER_INPUT, LB_ANY, LB_reader_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_UN_register(ORPGDAT_LB_READER_INPUT) failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "ORPGDA_UN_register(ORPGDAT_LB_READER_INPUT) successful" );

  /* Position the read pointer for the LBs */
  ORPGDA_seek( ORPGDAT_LDM_WRITER_INPUT, 0, LB_LATEST, NULL );
  ORPGDA_seek( ORPGDAT_LDM_READER_INPUT, 0, LB_LATEST, NULL );

  /* Register UN events to be notified when new volume status is received */
  ORPGDA_write_permission( ORPGDAT_GSM_DATA );
  if( ( ret = ORPGDA_UN_register( ORPGDAT_GSM_DATA, VOL_STAT_GSM_ID, VST_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_UN_register(ORPGDAT_GSM_DATA) failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "ORPGDA_UN_register(ORPGDAT_GSM_DATA) successful" );

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  /* Set start time of stats collection */
  Stats_start_time = time( NULL );

  /* Loop and handle events */
  while( 1 )
  {
    /* Read if needed */
    if( Read_writer_LB_flag == YES )
    {
      Read_writer_LB_flag = NO;
      Process_writer_LB();
    }
    if( Read_reader_LB_flag == YES )
    {
      Read_reader_LB_flag = NO;
      Process_reader_LB();
    }

    /* Check for new times or volume */
    current_time = time( NULL );
    gmt_time = gmtime( &current_time );

    if( ( current_hour = gmt_time->tm_hour ) != previous_hour )
    {
      Num_hours++;
      previous_hour = current_hour;
      /* Write out stats at change of hour */
      Write_stats_flag = YES;
      RPG_LDM_print_debug( "New hour (%d) detected. Number %d.", current_hour, Num_hours );
      RPG_LDM_print_debug( "Set write to LB flag" );
    }
    if( ( current_day = gmt_time->tm_mday ) != previous_day )
    {
      Num_days++;
      previous_day = current_day;
      RPG_LDM_print_debug( "New day (%d) detected. Number %d.", current_day, Num_days );
    }
    if( ( current_month = ( gmt_time->tm_mon + 1 ) ) != previous_month )
    {
      Num_months++;
      previous_month = current_month;
      RPG_LDM_print_debug( "New month (%d) detected. Number %d.", current_month, Num_months );
    }
    if( Read_VST_msg_flag == YES )
    {
      Read_VST_msg_flag = NO;
      if( ( current_volume = ORPGVST_get_volume_number() ) != previous_volume )
      {
        Num_volumes++;
        previous_volume = current_volume;
        RPG_LDM_print_debug( "New volume (%d) detected. Number %d.", current_volume, Num_volumes );
      }
    }

    RPG_LDM_print_debug( "Current time: %4d%02d%02d%02d%02d Vol: %d", gmt_time->tm_year + 1900, current_month, current_day, current_hour, gmt_time->tm_min, current_volume );
    RPG_LDM_print_debug( "Num VOLS: %d HRS: %d DAYS: %d MTHS: %d", Num_volumes, Num_hours, Num_days, Num_months );

    if( Write_stats_flag == YES )
    {
      Write_stats_flag = NO;
      Write_stats_of_outgoing_msgs();
      Write_stats_of_incoming_msgs();
    }

    sleep( LOOP_INTERVAL );
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
 Description: Callback if RPG LDM writer LB updated.
 ************************************************************************/

static void LB_writer_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "LB writer callback function called" );
  Read_writer_LB_flag = YES;
}

/************************************************************************
 Description: Callback if RPG LDM reader LB updated.
 ************************************************************************/

static void LB_reader_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "LB reader callback function called" );
  Read_reader_LB_flag = YES;
}

/************************************************************************
 Description: Callback if Volume Status message is received.
 ************************************************************************/

static void VST_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "VST callback function called" );
  Read_VST_msg_flag = YES;
}

/************************************************************************
 Description: Read any new messages for LDM writer LB.
 ************************************************************************/

static void Process_writer_LB()
{
  char *buf = NULL;
  RPG_LDM_prod_hdr_t *RPG_LDM_prod_hdr = NULL;
  int ret = 0;
  int index = 0;

  RPG_LDM_print_debug( "Enter Process_writer_LB()" );

  while( 1 )
  {
    /* Exhaust all unread messages */
    ret = ORPGDA_read( ORPGDAT_LDM_WRITER_INPUT, &buf, LB_ALLOC_BUF, LB_NEXT );
    if( ret == LB_TO_COME )
    {
      RPG_LDM_print_debug( "ORPGDA_read returned LB_TO_COME" );
      return;
    }
    else if( ret == LB_EXPIRED )
    {
      RPG_LDM_print_debug( "ORPGDA_read returned LB_EXPIRED" );
      ORPGDA_seek( ORPGDAT_LDM_WRITER_INPUT, 0, LB_FIRST, NULL );
      RPG_LDM_print_debug( "Seeking to first unread message\n" );
      continue;
    }
    else if( ret < 0 )
    {
      LE_send_msg( GL_ERROR, "ORPGDA_read( ORPGDAT_LDM_WRITER_INPUT ) Failed (%d)\n", ret );
      return;
    }

    /* Use key to find index for the product */
    RPG_LDM_prod_hdr = (RPG_LDM_prod_hdr_t *) buf;
    if( ( index = RPG_LDM_get_LDM_key_index_from_string( RPG_LDM_prod_hdr->key ) ) >= 0 )
    {
      /* Increment count and size for given product */
      Writer_num_msgs[index]++;
      Writer_total_size[index] += ret;
      RPG_LDM_print_debug( "WRITER: Key: %s Index: %d Num: %d Size: %d", RPG_LDM_get_LDM_key_string_from_index( index ), index, Writer_num_msgs[index], Writer_total_size[index] );
    }
    else
    {
      LE_send_msg( GL_INFO, "Failed to get index for %s\n", RPG_LDM_prod_hdr->key );
    }

    /* Free allocated memory */
    RPG_LDM_print_debug( "Free allocated memory" );
    free( buf );
  }

  RPG_LDM_print_debug( "Leave Process_writer_LB()" );
}

/************************************************************************
 Description: Read any new messages for LDM reader LB.
 ************************************************************************/

static void Process_reader_LB()
{
  char *buf = NULL;
  RPG_LDM_prod_hdr_t *RPG_LDM_prod_hdr = NULL;
  int ret = 0;
  int index = 0;

  RPG_LDM_print_debug( "Enter Process_reader_LB()" );

  while( 1 )
  {
    /* Exhaust all unread messages */
    ret = ORPGDA_read( ORPGDAT_LDM_READER_INPUT, &buf, LB_ALLOC_BUF, LB_NEXT );
    if( ret == LB_TO_COME )
    {
      RPG_LDM_print_debug( "ORPGDA_read returned LB_TO_COME" );
      return;
    }
    else if( ret == LB_EXPIRED )
    {
      RPG_LDM_print_debug( "ORPGDA_read returned LB_EXPIRED" );
      ORPGDA_seek( ORPGDAT_LDM_READER_INPUT, 0, LB_FIRST, NULL );
      RPG_LDM_print_debug( "Seeking to first unread message\n" );
      continue;
    }
    else if( ret < 0 )
    {
      LE_send_msg( GL_ERROR, "ORPGDA_read( ORPGDAT_LDM_READER_INPUT ) Failed (%d)\n", ret );
      return;
    }

    /* Use key to find index for the product */
    RPG_LDM_prod_hdr = (RPG_LDM_prod_hdr_t *) buf;
    if( ( index = RPG_LDM_get_LDM_key_index_from_string( RPG_LDM_prod_hdr->key ) ) != RPG_LDM_FAILURE )
    {
      /* Increment count and size for given product */
      Reader_num_msgs[index]++;
      Reader_total_size[index] += ret;
      RPG_LDM_print_debug( "READER: Key: %s Index: %d Num: %d Size: %d", RPG_LDM_prod_hdr->key, index, Reader_num_msgs[index], Reader_total_size[index] );
    }
    else
    {
      LE_send_msg( GL_INFO, "Failed to get index for %s\n", RPG_LDM_prod_hdr->key );
    }

    /* Free allocated memory */
    RPG_LDM_print_debug( "Free allocated memory" );
    free( buf );
  }

  RPG_LDM_print_debug( "Leave Process_reader_LB()" );
}

/************************************************************************
 Description: Write linked list stats of outbound msgs to linear buffer.
 ************************************************************************/

static void Write_stats_of_outgoing_msgs()
{
  RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  RPG_LDM_LB_write_stats_t msg_stats;
  char *product_buffer = NULL;
  int product_buffer_size = 0;
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int stats_length = sizeof( RPG_LDM_LB_write_stats_t );
  int data_length = stats_length * RPG_LDM_NUM_LDM_PRODS;
  int msg_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + msg_length;
  int ret = 0;
  int offset = 0;
  int num = 0;
  int size = 0;
  int prod_index = 0;

  RPG_LDM_print_debug( "Enter Write_stats_of_outgoing_msgs()" );

  /* Initialize data common to all products for the given LB */
  msg_stats.reset_time = Stats_start_time;
  msg_stats.spare172 = 0;
  msg_stats.spare174 = 0;
  msg_stats.spare176 = 0;
  msg_stats.spare178 = 0;
  msg_stats.spare180 = 0;
  msg_stats.spare182 = 0;

  /* Allocate memory for message */
  product_buffer_size = product_length;
  RPG_LDM_print_debug( "Product buffer size: %d", product_buffer_size );
  if( ( product_buffer = malloc( product_buffer_size ) ) == NULL )
  { 
    /* Memory allocation error */
    LE_send_msg( GL_ERROR, "Could not malloc product_buffer. Terminate." );
    exit( RPG_LDM_MALLOC_FAILED );
  }

  offset = prod_hdr_length + msg_hdr_length;
  /* Loop over all products */
  for( prod_index = 0; prod_index < RPG_LDM_NUM_LDM_PRODS; prod_index++ )
  {
    strcpy( msg_stats.key, RPG_LDM_get_LDM_key_string_from_index( prod_index ) );
    num = Writer_num_msgs[prod_index];
    size = Writer_total_size[prod_index];

    msg_stats.num_msgs_total = num;
    msg_stats.num_msgs_per_month = ( (float) (num/Num_months) * RPG_LDM_NUM_L2_MSGS_SCALE );
    msg_stats.num_msgs_per_day = ( (float) (num/Num_days) * RPG_LDM_NUM_L2_MSGS_SCALE ); 
    msg_stats.num_msgs_per_hour = ( (float) (num/Num_hours) * RPG_LDM_NUM_L2_MSGS_SCALE ); 
    if( Num_volumes > 0 )
    {
      msg_stats.num_msgs_per_volume = ( (float) (num/Num_volumes) * RPG_LDM_NUM_L2_MSGS_SCALE ); 
    }
    else
    {
      msg_stats.num_msgs_per_volume = ( 0.0 ); 
    }
    msg_stats.msg_size_total = size;
    msg_stats.msg_size_per_month = ( (float) (size/Num_months) * RPG_LDM_NUM_L2_MSGS_SCALE );
    msg_stats.msg_size_per_day =  ( (float) (size/Num_days) * RPG_LDM_NUM_L2_MSGS_SCALE );
    msg_stats.msg_size_per_hour = ( (float) (size/Num_hours) * RPG_LDM_NUM_L2_MSGS_SCALE ); 
    if( Num_volumes > 0 )
    {
      msg_stats.msg_size_per_volume = ( (float) (size/Num_volumes) * RPG_LDM_NUM_L2_MSGS_SCALE );
    }
    else
    {
      msg_stats.msg_size_per_volume = ( 0.0 );
    }

    /* Copy stats into appropriate place in output buffer */
    RPG_LDM_print_debug( "Copy message into product buffer" );
    memcpy( &product_buffer[offset], &msg_stats, stats_length );
    offset += stats_length;
  }

  /* Initialize RPG LDM product header. */
  RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
  RPG_LDM_prod_hdr.timestamp = time( NULL );;
  RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
  RPG_LDM_prod_hdr.feed_type = RPG_LDM_EXP_FEED_TYPE;
  RPG_LDM_prod_hdr.seq_num = 1;
  RPG_LDM_prod_hdr.data_len = msg_length;
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_L2_WRITE_STATS_PROD );
  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_L2_WRITE_STATS_PROD;
  RPG_LDM_msg_hdr.size = data_length;
  RPG_LDM_msg_hdr.timestamp = RPG_LDM_prod_hdr.timestamp;
  RPG_LDM_msg_hdr.segment_number = 1;
  RPG_LDM_msg_hdr.number_of_segments = 1;

  /* Copy header/data into output product buffer */
  offset = 0;
  RPG_LDM_print_debug( "Copy product header into product buffer" );
  memcpy( &product_buffer[offset], &RPG_LDM_prod_hdr, prod_hdr_length );
  offset += prod_hdr_length;
  RPG_LDM_print_debug( "Copy message header into product buffer" );
  memcpy( &product_buffer[offset], &RPG_LDM_msg_hdr, msg_hdr_length );
 
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
    RPG_LDM_print_L2_write_stats_msg( &product_buffer[prod_hdr_length+msg_hdr_length], data_length );
  }

  /* Free allocated memory */
  RPG_LDM_print_debug( "Free message buffer" ); 
  free( product_buffer );

  RPG_LDM_print_debug( "Leave Write_stats_of_outgoing_msgs" );
}

/************************************************************************
 Description: Write linked list stats of inbound msgs to linear buffer.
 ************************************************************************/

static void Write_stats_of_incoming_msgs()
{
  /* Need to implement */
}

