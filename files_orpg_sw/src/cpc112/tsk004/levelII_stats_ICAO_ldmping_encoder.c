/******************************************************************

	file: levelII_stats_ICAO_ldmping_encoder.c

	Main module for the levelII_stats_ICAO_ldmping_encoder task.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/08/21 13:42:18 $
 * $Id: levelII_stats_ICAO_ldmping_encoder.c,v 1.7 2012/08/21 13:42:18 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */  

/* Local includes. */

#include <rpg_ldm.h>
#include <nl2.h>

/* Defines/enums. */

#define DEFAULT_TIMER_LOOP		5
#define DEFAULT_TIME_BETWEEN_STATS	300
#define	CMD_BUF_LEN			128
#define	TIMESTRING_BUF_LEN		64
#define	MAX_IP_LEN			24

typedef struct {
  char ip[MAX_IP_LEN];
  short server_mask;
} NL2_servers_t;

/* Static/global variables. */

static int    Termination_flag = NO;
static int    Time_between_stats = DEFAULT_TIME_BETWEEN_STATS;
static time_t Previous_stats_time = 0;
static time_t Current_stats_time = 0;
static NL2_servers_t NL2_servers[NL2_NUM_NATL_NODES];
static NL2_stats_ICAO_ldmping_NL2_t NL2_stats;

/* Function prototypes. */

static void Parse_cmd_line( int, char *[] );
static void Initialize_variables();
static void Get_stats();
static void Write_stats();
static void Print_usage( char * );

/******************************************************************
 Description: The main function.
******************************************************************/

int main( int argc, char *argv[] )
{
  /* Read command-line options. */
  Parse_cmd_line( argc, argv );

  /* Initialze variables. */
  Initialize_variables();

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  while( Termination_flag == NO )
  {
    Current_stats_time = time( NULL );
    if( ( Current_stats_time - Previous_stats_time ) > Time_between_stats )
    {
      RPG_LDM_print_debug( "Timer expired. Get/write stats" );
      Previous_stats_time = Current_stats_time;
      Get_stats();
      Write_stats();
    }
    RPG_LDM_print_debug( "Sleep %d seconds", DEFAULT_TIMER_LOOP );
    sleep( DEFAULT_TIMER_LOOP );
  }

  return 0;
}

/**************************************************************************
 Description: Initialize variables.
**************************************************************************/

static void Initialize_variables()
{
  int i = 0;

  RPG_LDM_print_debug( "Enter Initialize variables()" );

  /* Initialize ICAO. */

  NL2_stats.ICAO = RPG_LDM_get_local_ICAO_index();
  LE_send_msg( GL_INFO, "ICAO: %s - %d", RPG_LDM_get_local_ICAO(), NL2_stats.ICAO );

  /* Initialize timestamp. */

  NL2_stats.timestamp = 0;

  /* Initialize channel number */

  NL2_stats.channel = RPG_LDM_get_channel_number();
  RPG_LDM_print_debug( "Channel: %d", NL2_stats.channel );

  /* Initialize ldmping_server_mask. */

  NL2_stats.ldmping_server_mask = 0;

  /* Initialize national server info. */

  for( i = 0; i < NL2_NUM_NATL_NODES; i++ )
  {
    if( i == NL2_PRIMARY_CLUSTER_NODE1 )
    {
      strcpy( NL2_servers[i].ip, NL2_SERVER_IP_PRIMARY_CLUSTER_NODE1 );
      NL2_servers[i].server_mask = NL2_SERVER_MASK_PRIMARY_CLUSTER_NODE1;
    }
    else if( i == NL2_PRIMARY_CLUSTER_NODE2 )
    {
      strcpy( NL2_servers[i].ip, NL2_SERVER_IP_PRIMARY_CLUSTER_NODE2 );
      NL2_servers[i].server_mask = NL2_SERVER_MASK_PRIMARY_CLUSTER_NODE2;
    }
    else if( i == NL2_SECONDARY_CLUSTER_NODE1 )
    {
      strcpy( NL2_servers[i].ip, NL2_SERVER_IP_SECONDARY_CLUSTER_NODE1 );
      NL2_servers[i].server_mask = NL2_SERVER_MASK_SECONDARY_CLUSTER_NODE1;
    }
    else
    {
      strcpy( NL2_servers[i].ip, NL2_SERVER_IP_SECONDARY_CLUSTER_NODE2 );
      NL2_servers[i].server_mask = NL2_SERVER_MASK_SECONDARY_CLUSTER_NODE2;
    }
    RPG_LDM_print_debug( "I: %d MASK: %d IP: %s", i, NL2_servers[i].server_mask, NL2_servers[i].ip );
  }

  RPG_LDM_print_debug( "Leave Initialize variables()" );
}

/**************************************************************************
 Description: Get levelII ldmping stats.
**************************************************************************/

static void Get_stats()
{
  static char *ldmping_cmd = "ldmping -i 0 -t 3";
  char cmd[CMD_BUF_LEN] = "";
  unsigned short flag_mask = 0;
  int ret;
  int i;

  RPG_LDM_print_debug( "Enter Get_stats()" );

  NL2_stats.timestamp = time( NULL );
  NL2_stats.ldmping_server_mask = 0;

  for( i = 0; i < NL2_NUM_NATL_NODES; i++ )
  {
    sprintf( cmd, "%s %s", ldmping_cmd, NL2_servers[i].ip );
    RPG_LDM_print_debug( "CMD: %s", cmd );
    if( ( ret = MISC_system_to_buffer( cmd, NULL, -1, NULL ) ) < 0 )
    {
      LE_send_msg( GL_ERROR, "MISC_system_to_buffer (%s) failed (%d)",
                   cmd, ret );
      continue;
    }
    else if( ( ret = ret >> 8 ) == 0 )
    {
      flag_mask = flag_mask | NL2_servers[i].server_mask;
    }
  }

  RPG_LDM_print_debug( "Get_stats ldmping mask: %d", flag_mask );
  NL2_stats.ldmping_server_mask = flag_mask;

  RPG_LDM_print_debug( "Leave Get_stats()" );
}

/**************************************************************************
 Description: Write stats to LDM queue.
**************************************************************************/

static void Write_stats()
{
  RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  char *product_buffer = NULL;
  int product_buffer_size = 0;
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int data_length = sizeof( NL2_stats_ICAO_ldmping_NL2_t );
  int msg_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + msg_length;
  int ret = 0;
  int offset = 0;

  RPG_LDM_print_debug( "Enter Write_stats()" );

  /* Initialize RPG LDM product header */
  RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
  RPG_LDM_prod_hdr.timestamp = time( NULL );
  RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
  RPG_LDM_prod_hdr.feed_type = RPG_LDM_EXP_FEED_TYPE;
  RPG_LDM_prod_hdr.seq_num = 1;
  RPG_LDM_prod_hdr.data_len = msg_length;
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_LDMPING_NL2_PROD );

  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_LDMPING_NL2_PROD;
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
  memcpy( &product_buffer[offset], &NL2_stats, data_length );

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
    RPG_LDM_print_ICAO_ldmping_msg( (char *) &NL2_stats );
  }

  /* Free memory allocated for outgoing message */
  free( product_buffer );
  product_buffer = NULL;

  RPG_LDM_print_debug( "Leave Write_stats()" );
}

/**************************************************************************
 Description: This function parses the command line.
**************************************************************************/

static void Parse_cmd_line( int argc, char **argv )
{
  int c = 0;

  RPG_LDM_parse_cmd_line( argc, argv );

  opterr = 0;
  optind = 0;
  while( ( c = getopt( argc, argv, "t:T:h" ) ) != EOF )
  {
    switch( c )
    {
      case 't':
        if( optarg == NULL )
        {
          LE_send_msg( GL_ERROR, "Option -t must have argument. Terminate." );
          exit( RPG_LDM_INVALID_COMMAND_LINE );
        }
        else if( ( Time_between_stats = atoi( optarg ) ) < 1 )
        {
          LE_send_msg( GL_ERROR, "Option -t argument not valid. Terminate." );
          exit( RPG_LDM_INVALID_COMMAND_LINE );
        }
        break;
      case 'T':
        /* Ignore -T option */
        break;
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
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

  RPG_LDM_print_debug( "Leave Parse_cmd_line()" );
}

/**************************************************************************
 Description: This function prints the usage info.
**************************************************************************/

static void Print_usage( char *task_name )
{
  printf( "Usage: %s [options]\n", task_name );
  printf( "  Options:\n" );
  printf( "  -t int   - timer to publish stats\n" );
  printf( "  -T task name - only used when invoked by mrpg (optional)\n" );
  printf( "  -h       - print usage info\n" );
  RPG_LDM_print_usage();
  printf( "\n" );
}

