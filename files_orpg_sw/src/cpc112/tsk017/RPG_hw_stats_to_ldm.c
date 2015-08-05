/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/08/21 13:42:14 $
 * $Id: RPG_hw_stats_to_ldm.c,v 1.5 2012/08/21 13:42:14 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_INTERVAL		1	/* Seconds between loops */
#define	MSG_INTERVAL		300	/* Seconds between msgs */
#define	UPTIME_FILE		"/proc/uptime"
#define	UPTIME_LINE_LEN		128
#define	LOAD_AVG_FILE		"/proc/loadavg"
#define	LOAD_AVG_LINE_LEN	128
#define	MEMINFO_FILE		"/proc/meminfo"
#define	MEMINFO_LINE_LEN	128
#define	MAX_CMD_LEN		128
#define	MAX_BUF_LEN		256

/* Structures */

/* Static/global variables */

static RPG_LDM_hw_stats_t Hw_stats;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void Set_server_mask();
static void Read_uptime_file();
static void Read_load_average_file();
static void Read_meminfo_file();
static int  Get_token_value( char * );
static void Get_disk_info();
static void Write_to_LB();
static int Convert_string_to_double( const char *, double * );

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  time_t current_time = 0;
  time_t previous_msg_time = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Set server mask */
  Set_server_mask();

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  /* Loop and handle events */
  while( 1 )
  {
    current_time = time(NULL);
    if( ( current_time - previous_msg_time ) > MSG_INTERVAL )
    {
      RPG_LDM_print_debug( "Creating product at %ld (%s)", current_time, RPG_LDM_get_timestring( current_time ) );
      previous_msg_time = current_time;
      Read_uptime_file();
      Read_load_average_file();
      Read_meminfo_file();
      Get_disk_info();
      Write_to_LB();
    }
    RPG_LDM_print_debug( "sleep for %d seconds", LOOP_INTERVAL );
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
        LE_send_msg( GL_ERROR, "Illegal option %c. Terminate.", (char) c );
        Print_usage( argv[0] );
        exit( RPG_LDM_INVALID_COMMAND_LINE );
        break;
    }
  }
}

/************************************************************************
 Description: Set server mask.
 ************************************************************************/

static void Set_server_mask()
{
  Hw_stats.server_mask = RPG_LDM_get_local_server_mask();
  RPG_LDM_print_debug( "Server Mask: %d (%s)", Hw_stats.server_mask, RPG_LDM_get_server_string_from_mask( Hw_stats.server_mask ) );
}

/************************************************************************
 Description: Read /proc/uptime file.
 ************************************************************************/

static void Read_uptime_file()
{
  FILE *infile = NULL;
  char input_line[UPTIME_LINE_LEN+1];
  char *tok = NULL;
  int uptime = 0;
  int idle = 0;

  Hw_stats.uptime = RPG_LDM_MISSING_VALUE;
  Hw_stats.pct_idle = RPG_LDM_MISSING_VALUE;

  /* Open/read file */
  if( ( infile = fopen( UPTIME_FILE, "r" ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to open %s", UPTIME_FILE );
    return;
  }
  else if( fgets( input_line, UPTIME_LINE_LEN, infile ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to read %s", UPTIME_FILE );
    return;
  }
  fclose( infile );

  RPG_LDM_print_debug( "Uptime line: %s", input_line );

  /* Parse first token to get uptime */
  if( ( tok = strtok( input_line, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get first token from %s", input_line );
  }
  else if( ( uptime = atoi( tok ) ) == 0 )
  {
    LE_send_msg( GL_ERROR, "Unable to convert %s to int", tok );
  }
  else
  {
    RPG_LDM_print_debug( "Uptime: %d", uptime );
    Hw_stats.uptime = uptime;
  }

  /* Parse second token to get idle time */
  if( ( tok = strtok( NULL, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get second token from %s", input_line );
  }
  else if( ( idle = atoi( tok ) ) == 0 )
  {
    LE_send_msg( GL_ERROR, "Unable to convert %s to int", tok );
  }
  else
  {
    RPG_LDM_print_debug( "Idle: %d", idle );
    Hw_stats.pct_idle = (int) ( ( (float)idle/(float)uptime )*RPG_LDM_PCT_IDLE_SCALE );
  }
}

/************************************************************************
 Description: Read /proc/loadavg file.
 ************************************************************************/

static void Read_load_average_file()
{
  FILE *infile = NULL;
  char input_line[LOAD_AVG_LINE_LEN+1];
  char *tok = NULL;
  double load_avg_1min = 0.0;
  double load_avg_5min = 0.0;
  double load_avg_15min = 0.0;

  Hw_stats.load_avg_1min = RPG_LDM_MISSING_VALUE;
  Hw_stats.load_avg_5min = RPG_LDM_MISSING_VALUE;
  Hw_stats.load_avg_15min = RPG_LDM_MISSING_VALUE;

  /* Open/read file */
  if( ( infile = fopen( LOAD_AVG_FILE, "r" ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to open %s", LOAD_AVG_FILE );
    return;
  }
  else if( fgets( input_line, LOAD_AVG_LINE_LEN, infile ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to read %s", LOAD_AVG_FILE );
    return;
  }
  fclose( infile );

  RPG_LDM_print_debug( "Load avg line: %s", input_line );

  /* Parse first token to get 1 minute load average */
  if( ( tok = strtok( input_line, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get first token from %s", input_line );
  }
  else if( Convert_string_to_double( tok, &load_avg_1min ) )
  {
    LE_send_msg( GL_ERROR, "Unable to convert %s to double", tok );
  }
  else
  {
    Hw_stats.load_avg_1min = (int) (load_avg_1min*RPG_LDM_LOAD_AVG_SCALE);
    RPG_LDM_print_debug( "Load Avg 1 minute: %6.2f %d", load_avg_1min, Hw_stats.load_avg_1min );
  }

  /* Parse second token to get 5 minute load average */
  if( ( tok = strtok( NULL, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get second token from %s", input_line );
  }
  else if( Convert_string_to_double( tok, &load_avg_5min ) )
  {
    LE_send_msg( GL_ERROR, "Unable to convert %s to double", tok );
  }
  else
  {
    Hw_stats.load_avg_5min = (int) (load_avg_5min*RPG_LDM_LOAD_AVG_SCALE);
    RPG_LDM_print_debug( "Load Avg 5 minute: %6.2f %d", load_avg_5min, Hw_stats.load_avg_5min );
  }

  /* Parse third token to get 15 minute load average */
  if( ( tok = strtok( NULL, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get third token from %s", input_line );
  }
  else if( Convert_string_to_double( tok, &load_avg_15min ) )
  {
    LE_send_msg( GL_ERROR, "Unable to convert %s to double", tok );
  }
  else
  {
    Hw_stats.load_avg_15min = (int) (load_avg_15min*RPG_LDM_LOAD_AVG_SCALE);
    RPG_LDM_print_debug( "Load Avg 15 minute: %6.2f %d", load_avg_15min, Hw_stats.load_avg_15min );
  }
}

/************************************************************************
 Description: Read /proc/meminfo file.
 ************************************************************************/

static void Read_meminfo_file()
{
  FILE *infile = NULL;
  char input_line[MEMINFO_LINE_LEN+1];
  int mem_total = -1;
  int mem_free = -1;
  int swap_total = -1;
  int swap_free = -1;

  Hw_stats.mem_total = RPG_LDM_MISSING_VALUE;
  Hw_stats.mem_free = RPG_LDM_MISSING_VALUE;
  Hw_stats.swap_total = RPG_LDM_MISSING_VALUE;
  Hw_stats.swap_free = RPG_LDM_MISSING_VALUE;

  /* Open/read file */
  if( ( infile = fopen( MEMINFO_FILE, "r" ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to open %s", MEMINFO_FILE );
    return;
  }

  /* Loop through file and get values */
  while( fgets( input_line, MEMINFO_LINE_LEN, infile ) != NULL )
  {
    RPG_LDM_print_debug( "Meminfo line: %s", input_line );
    if( strstr( input_line, "MemTotal:" ) != NULL )
    {
      mem_total = Get_token_value( input_line );
      RPG_LDM_print_debug( "Memory total: %d", mem_total );
      Hw_stats.mem_total = mem_total;
    }
    else if( strstr( input_line, "MemFree:" ) != NULL )
    {
      mem_free = Get_token_value( input_line );
      RPG_LDM_print_debug( "Memory free: %d", mem_free );
      Hw_stats.mem_free = mem_free;
    }
    else if( strstr( input_line, "SwapTotal:" ) != NULL )
    {
      swap_total = Get_token_value( input_line );
      RPG_LDM_print_debug( "Swap total: %d", swap_total );
      Hw_stats.swap_total = swap_total;
    }
    else if( strstr( input_line, "SwapFree:" ) != NULL )
    {
      swap_free = Get_token_value( input_line );
      RPG_LDM_print_debug( "Swap free: %d", swap_free );
      Hw_stats.swap_free = swap_free;
    }
  }

  /* Close file */
  fclose( infile );
}

/************************************************************************
 Description: Parse line to get value.
 ************************************************************************/

static int Get_token_value( char *input_line )
{
  int value = 0;
  char *tok = NULL;

  RPG_LDM_print_debug( "Token line: %s", input_line );

  /* Parse first token to get tag */
  if( ( tok = strtok( input_line, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get first token from %s", input_line );
  }
  /* Parse second token to get value */
  else if( ( tok = strtok( NULL, " " ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Unable to get second token from %s", value );
  }
  else if( ( value = atoi( tok ) ) == 0 )
  {
    LE_send_msg( GL_ERROR, "Unable to convert %s to int", tok );
    value = RPG_LDM_MISSING_VALUE;
  }
  else
  {
    RPG_LDM_print_debug( "Token value: %d", value );
  }

  return value;
}

/************************************************************************
 Description: Get disk drive statistics.
 ************************************************************************/

static void Get_disk_info()
{
  char cmd[MAX_CMD_LEN];
  char buf[MAX_BUF_LEN];
  char ignore_string[64];
  int ret = 0;
  int ret_bytes = 0;
  int ignore_int = 0;
  int disk_total = 0;
  int disk_free = 0;

  Hw_stats.disk_total = RPG_LDM_MISSING_VALUE;
  Hw_stats.disk_free = RPG_LDM_MISSING_VALUE;

  /* Get disk stats of main mount point */
  sprintf( cmd, "sh -c \"df -k / | egrep '^ *\\/dev\\/sd'\"" );
  RPG_LDM_print_debug( "Command: %s", cmd );
  if( ( ret = MISC_system_to_buffer( cmd, buf, MAX_BUF_LEN, &ret_bytes ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "Error (%d) running %s", ret, cmd );
    return;
  }
  else if( ret_bytes < 1 )
  {
    LE_send_msg( GL_ERROR, "Command (%s) returned no output", cmd );
    return;
  }
  else if( sscanf( buf, "%s %d %d %d", ignore_string, &disk_total, &ignore_int, &disk_free ) != 4 )
  {
    LE_send_msg( GL_ERROR, "Sscanf did not find 4 blocks in %s", buf );
    return;
  }

  RPG_LDM_print_debug( "Return: %s", buf );
  RPG_LDM_print_debug( "Disk total: %d free: %d", disk_total, disk_free );
  Hw_stats.disk_total = disk_total;
  Hw_stats.disk_free = disk_free;
}

/************************************************************************
 Description: Write linked list to linear buffer.
 ************************************************************************/

static void Write_to_LB()
{
  RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  char *product_buffer = NULL;
  int product_buffer_size = 0;
  int prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int data_length = sizeof( RPG_LDM_hw_stats_t );
  int msg_length = msg_hdr_length + data_length;
  int product_length = prod_hdr_length + msg_length;
  int ret = 0;
  int offset = 0;

  RPG_LDM_print_debug( "Enter Write_to_LB()" );

  /* Initialize RPG LDM product header. */
  RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
  RPG_LDM_prod_hdr.timestamp = time( NULL );
  RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
  RPG_LDM_prod_hdr.feed_type = RPG_LDM_EXP_FEED_TYPE;
  RPG_LDM_prod_hdr.seq_num = 1;
  RPG_LDM_prod_hdr.data_len = msg_length;
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_HW_STATS_PROD );

  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_HW_STATS_PROD;
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
  memcpy( &product_buffer[offset], &Hw_stats, data_length );
  
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
    RPG_LDM_print_hw_stats_msg( (char * ) &Hw_stats );
  }
 
  /* Free allocated memory */
  RPG_LDM_print_debug( "Free message buffer" ); 
  free( product_buffer );

  RPG_LDM_print_debug( "Leave Write_to_LB()" );
}

/************************************************************************
 Description: Convert string to double.
 ************************************************************************/

static int Convert_string_to_double( const char *str, double *val )
{
  errno = 0;
  *val = atof( str );
  if( errno != 0 )
  {
    LE_send_msg( GL_ERROR, "Error: %s", strerror( errno ) );
    return 1;
  }
  return 0;
}

