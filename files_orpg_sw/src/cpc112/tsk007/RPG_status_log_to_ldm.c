/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/08/21 13:42:15 $
 * $Id: RPG_status_log_to_ldm.c,v 1.5 2012/08/21 13:42:15 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_INTERVAL		5
#define	MAX_TIME_INTERVAL	660	/* Max seconds between msgs */
#define	MAX_MESSAGE_SIZE	20000	/* Max size of msg in bytes */
#define	MAX_MSG_LEN		256	/* Max length of msg */

/* Structures */

typedef struct Linked_list {
  struct Linked_list *next;
  RPG_LDM_log_msg_t Msg_log;
} Text_node_t;

/* Static/global variables */

static LB_id_t      LB_id = ORPGDAT_SYSLOG;
static time_t       Current_time = 0;
static time_t       Previous_time = 0;
static int          Read_LB_flag = YES;
static int          Read_VST_flag = NO;
static int          Total_number_of_msgs = 0;
static int          Total_size_of_msgs = 0;
static int          Write_to_LB_flag = NO;
static Text_node_t *List_head = NULL;
static Text_node_t *List_tail = NULL;
static char        *Month_label [] = { "Jan", "Feb", "Mar", "Apr", "May",
                         "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void LB_callback_fx( int, LB_id_t );
static void VST_callback_fx( int, LB_id_t );
static void Process_log_entry();
static void Add_message_to_list( char *, unsigned int );
static void Free_list();
static void Write_to_LB();

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  int ret = 0;
  int current_volume_number = 0;
  long time_diff = 0;
  static int previous_volume_number = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Register UN events to be notified when msg received */
  ORPGDA_write_permission( LB_id );
  if( ( ret = ORPGDA_UN_register( LB_id, LB_ANY, LB_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_UN_register failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "ORPGDA_UN_register successful" );

  /* Position the read pointer for the log */
  ORPGDA_seek( LB_id, 0, LB_FIRST, NULL );

  /* Register UN events to be notified when new volume is received */
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

  /* Process log events */
  while( 1 )
  {
    /* If we need to read the LB, do it now */
    if( Read_LB_flag == YES )
    {
      Read_LB_flag = NO;
      Process_log_entry();
    }

    /* Determine if it is time to write out the messages. This is done if
       it's been MAX_TIME_INTERVAL seconds since the last write, or a new
       volume is started. */
    Current_time = time( NULL );
    time_diff = Current_time - Previous_time;
    RPG_LDM_print_debug( "Current time: %ld  Diff: %d  Max diff: %d", Current_time, time_diff, MAX_TIME_INTERVAL );
    if( time_diff > MAX_TIME_INTERVAL )
    {
      if( Previous_time > 0 )
      {
        RPG_LDM_print_debug( "Timer expired. Write to LB" );
        Write_to_LB_flag = YES;
      }
      else
      {
        /* First time through. Initialize previous time. */
        RPG_LDM_print_debug( "Initializing previous time to current time" );
        Previous_time = Current_time;
      }
    }
    else if( Read_VST_flag == YES )
    {
      Read_VST_flag = NO;
      if( ( current_volume_number = ORPGVST_get_volume_number() ) != previous_volume_number )
      {
        RPG_LDM_print_debug( "New volume (%d). Write to LB", current_volume_number );
        Write_to_LB_flag = YES;
        previous_volume_number = current_volume_number;
      }
    }
      
    if( Write_to_LB_flag == YES )
    {
      Write_to_LB_flag = NO;
      if( Total_number_of_msgs > 0 )
      {
        Write_to_LB();
      }
      else
      {
        RPG_LDM_print_debug( "No messages to write" );
      }
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
 Description: Callback if LB is updated.
 ************************************************************************/

void LB_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "LB callback function called" );
  Read_LB_flag = YES;
}

/************************************************************************
 Description: Callback if Volume message is received.
 ************************************************************************/

void VST_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "VST callback function called" );
  Read_VST_flag = YES;
}

/************************************************************************
 Description: Read log messages and add to list.
 ************************************************************************/

static void Process_log_entry()
{
  int ret, year, month, day, hour, minute, second, code = 0;
  char *buf = NULL;
  char *ptr = NULL;
  char log_msg[MAX_MSG_LEN+1];
  LE_critical_message *LE_msg = NULL;

  RPG_LDM_print_debug( "Enter Process_log_entry()" );

  while( 1 )
  {
    /* Exhaust all unread messages */
    ret = ORPGDA_read( LB_id, &buf, LB_ALLOC_BUF, LB_NEXT );
    if( ret == LB_TO_COME )
    {
      RPG_LDM_print_debug( "ORPGDA_read returned LB_TO_COME" );
      return;
    }
    else if( ret == LB_EXPIRED )
    {
      RPG_LDM_print_debug( "ORPGDA_read returned LB_EXPIRED" );
      ORPGDA_seek( LB_id, 0, LB_FIRST, NULL );
      RPG_LDM_print_debug( "Seeking to first unread message" );
      continue;
    }
    else if( ret < 0 )
    {
      LE_send_msg( GL_ERROR, "ORPGDA_read Failed (%d)", ret );
      return;
    }
    else if( ret < sizeof( LE_critical_message ) )
    {
      LE_send_msg( GL_ERROR, "ret %d < %d", ret, sizeof( LE_critical_message ) );
      return;
    }

    LE_msg = (LE_critical_message *) &buf[0];
    ptr = &LE_msg->text[0];

    /* Format the time to read month day, year hh:mm:ss */
    unix_time( &LE_msg->time, &year, &month, &day, &hour, &minute, &second );
    year = year%100;

    sprintf( log_msg, "%3s %02d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
             Month_label[month-1], day, year, hour, minute, second, ptr );
    RPG_LDM_print_debug( "MSG: %s  LEN: %d", log_msg, strlen(log_msg) );

    /* Add code */
    code = LE_msg->code;
    RPG_LDM_print_debug( "CODE: %u", code );

    free( buf );

    Add_message_to_list( log_msg, code );
  }

  RPG_LDM_print_debug( "Leave Process_log_entry()" );
}

/************************************************************************
 Description: Add message to linked list.
 ************************************************************************/

static void Add_message_to_list( char *log_msg, unsigned int code )
{
  Text_node_t *node = NULL;

  RPG_LDM_print_debug( "Enter Add_message_to_list()" );

  /* Allocate space for Text_node_t structure */
  if( (node = (Text_node_t *) malloc( sizeof( Text_node_t ) ) )   
                                       == (Text_node_t *) NULL )
  {
    LE_send_msg( GL_ERROR, "Malloc failed for %d Bytes. Terminate.", sizeof( Text_node_t ) );
    exit( RPG_LDM_MALLOC_FAILED );
  }
  node->next = (Text_node_t *) NULL;

  /* Copy the msg. */
  strcpy( node->Msg_log.text, log_msg );
  node->Msg_log.text_len = strlen( log_msg );
  node->Msg_log.code = code;

  /* Start the list if empty, else add to list */
  if( List_head == NULL ){ List_head = node; }
  else{ List_tail->next = node; }

  /* Set tail of linked list to current node */
  List_tail = node;

  /* Increment the number of messages and size so far */
  Total_number_of_msgs++;
  Total_size_of_msgs += sizeof( RPG_LDM_log_msg_t );
  RPG_LDM_print_debug( "Num msgs: %d size: %d total size: %d", Total_number_of_msgs, sizeof( RPG_LDM_log_msg_t ), Total_size_of_msgs );
  if( Total_size_of_msgs > MAX_MESSAGE_SIZE )
  {
    Write_to_LB_flag = YES;
  }

  RPG_LDM_print_debug( "Leave Add_message_to_list()" );
}

/************************************************************************
 Description: Free memory allocated to linked list.
 ************************************************************************/

static void Free_list()
{
  Text_node_t *prev_node = List_head;
  Text_node_t *next_node = NULL;

  RPG_LDM_print_debug( "Enter Free_list()" );

  if( prev_node != NULL )
  {
    while( 1 )
    {
      next_node = prev_node->next;
      free( prev_node );
      if( next_node == NULL ){ break; }
      prev_node = next_node;
    }
  }

  List_head = NULL;
  List_tail = NULL;
  Total_number_of_msgs = 0;
  Total_size_of_msgs = 0;

  RPG_LDM_print_debug( "Leave Free_list()" );
}

/************************************************************************
 Description: Write linked list to linear buffer.
 ************************************************************************/

static void Write_to_LB()
{
  Text_node_t *prev_node = List_head;
  Text_node_t *next_node = NULL;
  RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  char *product_buffer = NULL;
  int product_buffer_size = 0;
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int data_length = Total_size_of_msgs;
  int msg_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + msg_length;
  int ret = 0;
  int offset = 0;

  RPG_LDM_print_debug( "Enter Write_to_LB()" );

  RPG_LDM_print_debug( "Total messages %d", Total_number_of_msgs );
  RPG_LDM_print_debug( "Total size %d", Total_size_of_msgs );

  if( prev_node == NULL )
  {
    LE_send_msg( GL_INFO, "First node is NULL. Skip LB write." );
    return;
  }

  /* Initialize RPG LDM product header. */
  RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
  RPG_LDM_prod_hdr.timestamp = time( NULL );
  RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
  RPG_LDM_prod_hdr.feed_type = RPG_LDM_EXP_FEED_TYPE;
  RPG_LDM_prod_hdr.seq_num = 1;
  RPG_LDM_prod_hdr.data_len = msg_length;
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_STATUS_LOG_PROD );

  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_STATUS_LOG_PROD;
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

  while( Total_number_of_msgs > 0 )
  {
    next_node = prev_node->next;
    RPG_LDM_print_debug( "MSG: %d TEXTLEN: %d POS: %d #MSG: %d", sizeof(RPG_LDM_log_msg_t), prev_node->Msg_log.text_len, offset, Total_number_of_msgs );
    RPG_LDM_print_debug( "TEXT: %s", prev_node->Msg_log.text );
    memcpy( &product_buffer[offset], &(prev_node->Msg_log), sizeof( RPG_LDM_log_msg_t ) );
    Total_number_of_msgs--;
    offset += sizeof( RPG_LDM_log_msg_t );
    if( next_node == NULL )
    {
      RPG_LDM_print_debug( "Next node is NULL. Exiting Link List loop" );
      RPG_LDM_print_debug( "Total messages left: %d", Total_number_of_msgs );
      break;
    }
    prev_node = next_node;
  }

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
    RPG_LDM_print_status_log_msg( &product_buffer[prod_hdr_length+msg_hdr_length], Total_size_of_msgs );
  }

  /* Free memory allocated for outgoing message */
  free( product_buffer );
  product_buffer = NULL;

  /* Free memory allocated to linked list */
  Free_list();

  /* Set previous time to current time */
  Previous_time = Current_time;

  RPG_LDM_print_debug( "Leave Write_to_LB()" );
}

