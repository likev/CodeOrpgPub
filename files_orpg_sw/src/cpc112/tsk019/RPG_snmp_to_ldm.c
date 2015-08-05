/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/09 16:00:43 $
 * $Id: RPG_snmp_to_ldm.c,v 1.1 2012/10/09 16:00:43 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_INTERVAL		5
#define	MAX_TIME_INTERVAL	60	/* Max seconds between msgs */
#define	MAX_MESSAGE_SIZE	20000	/* Max size of msg in bytes */

/* Structures */

typedef struct Linked_list {
  struct Linked_list *next;
  RPG_LDM_snmp_msg_t SNMP_msg;
} Text_node_t;

/* Static/global variables */

static LB_id_t      SNMP_LB_id = 0;
static time_t       Current_time = 0;
static time_t       Previous_time = 0;
static int          Start_msg_id = 0;
static int          End_msg_id = 0;
static int          Read_SNMP_LB_flag = NO;
static int          Total_number_of_msgs = 0;
static int          Total_size_of_msgs = 0;
static int          Write_to_LB_flag = NO;
static int          SNMP_file_validated_flag = NO;
static int          Msg_time_interval = MAX_TIME_INTERVAL;
static Text_node_t *List_head = NULL;
static Text_node_t *List_tail = NULL;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void Validate_SNMP_file( char * );
static void SNMP_LB_callback_fx( int, LB_id_t, int, void * );
static void Read_SNMP_messages( int, int );
static void Add_message_to_list( char *, time_t );
static void Free_list();
static void Write_to_LB();

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  int  start_id = 0;
  int  end_id = 0;
  long time_diff = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  /* Process log events */
  while( 1 )
  {
    /* If we need to read the SNMP LB, do it now */
    if( Read_SNMP_LB_flag == YES )
    {
      start_id = Start_msg_id;
      end_id = End_msg_id;
      Read_SNMP_LB_flag = NO;
      RPG_LDM_print_debug( "Read Start ID: %d End ID: %d", start_id, end_id );
      Read_SNMP_messages( start_id, end_id );
    }

    /* Determine if it is time to write out the messages. This is done if
       it's been MAX_TIME_INTERVAL seconds since the last write, or a new
       volume is started. */
    Current_time = time( NULL );
    time_diff = Current_time - Previous_time;
    RPG_LDM_print_debug( "Current time: %ld  Diff: %d  Max diff: %d", Current_time, time_diff, MAX_TIME_INTERVAL );
    if( time_diff >= MAX_TIME_INTERVAL )
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
      }
      Previous_time = Current_time;
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
  printf( "  -f file name - name of SNMP LB to monitor\n" );
  printf( "  -t int - time between message writes\n" );
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
  while( ( c = getopt( argc, argv, "f:t:T:h" ) ) != EOF )
  {
    switch( c )
    {
      case 'f':
        Validate_SNMP_file( optarg );
        break;
      case 't':
        if( ( Msg_time_interval = strtol( optarg, NULL, 10 ) ) == 0 )
        {
          LE_send_msg( GL_ERROR, "Invalid time interval option (%s)", optarg );
          exit( RPG_LDM_INVALID_COMMAND_LINE );
        }
        break;
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
        break;
      case 'T':
        /* Ignore -T option */
        break;
      case '?':
        LE_send_msg( GL_INFO, "Ignored option %c", (char) optopt );
        break;
      default:
        LE_send_msg( GL_INFO, "Illegal option %c. Terminate.", (char) c );
        Print_usage( argv[0] );
        exit( RPG_LDM_INVALID_COMMAND_LINE );
        break;
    }
  }

  if( !SNMP_file_validated_flag )
  {
    LE_send_msg( GL_INFO, "The -f option is required. Terminate." );
    exit( RPG_LDM_FAILURE );
  }
}

/************************************************************************
 Description: Callback if SNMP LB is updated.
 ************************************************************************/

void SNMP_LB_callback_fx( int data_id, LB_id_t msg_id, int len, void *data )
{
  RPG_LDM_print_debug( "SNMP LB callback function called" );
  if( !Read_SNMP_LB_flag )
  {
    Read_SNMP_LB_flag = YES;
    Start_msg_id = msg_id;
  }
  End_msg_id = msg_id;
}

/************************************************************************
 Description: Read SNMP messages and add to list.
 ************************************************************************/

static void Read_SNMP_messages( int start_id, int end_id )
{
  int   i = 0;
  char *buf = NULL;
  int   ret = 0;
  LE_message *msg = NULL;

  RPG_LDM_print_debug( "Enter Read_SNMP_messages()" );

  for( i = start_id; i <= end_id; i++ )
  {
    ret = LB_read( SNMP_LB_id, &buf, LB_ALLOC_BUF, i );
    if( ret == LB_TO_COME )
    {
      RPG_LDM_print_debug( "LB_read returned LB_TO_COME" );
    }
    else if( ret < 0 )
    {
      LE_send_msg( GL_ERROR, "LB_read %d failed (%d)", i, ret );
    }
    else if( ret < sizeof( LE_message ) )
    {
      LE_send_msg( GL_ERROR, "ret %d < %d", ret, sizeof( LE_message ) );
    }
    else
    {
      RPG_LDM_print_debug( "Successful LB_read of %d bytes", ret );
      msg = (LE_message *) buf;
      Add_message_to_list( msg->text, msg->time );
    }

    free( buf );
  }

  RPG_LDM_print_debug( "Leave Read_SNMP_messages()" );
}

/************************************************************************
 Description: Add message to linked list.
 ************************************************************************/

static void Add_message_to_list( char *SNMP_msg, time_t timestamp )
{
  Text_node_t *node = NULL;
  int msg_size = 0;

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
  node->SNMP_msg.text = strdup( SNMP_msg );
  node->SNMP_msg.text_len = strlen( SNMP_msg );
  node->SNMP_msg.timestamp = timestamp;
  RPG_LDM_print_debug( "TIME: %s LEN: %d MSG: %s", RPG_LDM_get_timestring( timestamp ), node->SNMP_msg.text_len, node->SNMP_msg.text );

  /* Start the list if empty, else add to list */
  if( List_head == NULL ){ List_head = node; }
  else{ List_tail->next = node; }

  /* Set tail of linked list to current node */
  List_tail = node;

  /* Increment the number of messages and size so far */
  msg_size = node->SNMP_msg.text_len + sizeof( RPG_LDM_snmp_msg_t );
  Total_number_of_msgs++;
  Total_size_of_msgs += msg_size;
  RPG_LDM_print_debug( "Num msgs: %d size: %d total size: %d", Total_number_of_msgs, msg_size, Total_size_of_msgs );
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
      free( prev_node->SNMP_msg.text );
      prev_node->SNMP_msg.text = NULL;
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
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_SNMP_PROD );

  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_SNMP_PROD;
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
    RPG_LDM_print_debug( "MSG SIZE: %d TEXTLEN: %d POS: %d #MSG: %d", ( sizeof( RPG_LDM_snmp_msg_t ) + prev_node->SNMP_msg.text_len ), prev_node->SNMP_msg.text_len, offset, Total_number_of_msgs );
    RPG_LDM_print_debug( "TEXT: %s", prev_node->SNMP_msg.text );
    memcpy( &product_buffer[offset], &(prev_node->SNMP_msg), ( sizeof( RPG_LDM_snmp_msg_t ) + prev_node->SNMP_msg.text_len ) );
    Total_number_of_msgs--;
    offset += ( sizeof( RPG_LDM_snmp_msg_t ) + prev_node->SNMP_msg.text_len );
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
    RPG_LDM_print_snmp_msg( &product_buffer[prod_hdr_length+msg_hdr_length], Total_size_of_msgs );
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

/************************************************************************
 Description: Validate argument for file option.
 ************************************************************************/

static void Validate_SNMP_file( char *filename_buf )
{
  int ret = 0;

  RPG_LDM_print_debug( "Enter Validate_SNMP_file()" );

  if( filename_buf == NULL )
  {
    /* Option not on command line */
    LE_send_msg( GL_ERROR, "Option -f not defined. Terminate." );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else if( strlen( filename_buf ) >= FILENAME_MAX )
  {
    /* Specified file name is too long */
    LE_send_msg( GL_ERROR, "File name longer than %d. Terminate.", FILENAME_MAX );
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

  /* Open LB. */
  if( ( ret = LB_open( filename_buf, LB_READ, NULL ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "LB_open failed (%d) for %s", ret, filename_buf );
    exit( RPG_LDM_FILE_NOT_OPENED );
  }

  /* Set id of SNMP LB */
  SNMP_LB_id = ret;
  RPG_LDM_print_debug( "LB_open succeeded. LB ID = %d", SNMP_LB_id );

  /* Register UN events to be notified when msg received */
  if( ( ret = LB_UN_register( SNMP_LB_id, LB_ANY, SNMP_LB_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "LB_UN_register failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "LB_UN_register successful" );

  /* Position the read pointer for the log */
  LB_seek( SNMP_LB_id, 0, LB_FIRST, NULL );

  /* SNMP file validated */
  SNMP_file_validated_flag = YES;

  RPG_LDM_print_debug( "Leave Validate_SNMP_file()" );
}


