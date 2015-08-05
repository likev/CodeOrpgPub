/************************************************************************
 *									*
 *	Module:  save_log_extract.c					*
 *									*
 *	Description:  This module generates a save log on a user-defined*
 *		      node and extracts it to the local machine.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/06/04 17:26:01 $
 * $Id: save_log_extract.c,v 1.1 2009/06/04 17:26:01 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <orpg.h>
#include <ctype.h>

/*	Macros.		*/

#define	NO			0
#define	YES			1
#define MAX_NUM_NODES		5
#define MAX_OBUF_SIZE		2048
#define IP_ADDRESS_LENGTH	64
#define MAX_CMD_LENGTH		256
#define	MAX_FILENAME_LENGTH	256
#define MAX_DIRECTORY_LENGTH	128
#define MAX_DESCRIPTION_LENGTH	30
#define MAX_DESCRIPTION_DISPLAY	25
#define MAX_NODE_LENGTH		8
#define MAX_THROTTLE_LENGTH	3
#define	NUM_LE_LOG_MESSAGES	500
#define	BANDWIDTH_THROTTLE_MIN	1
#define	BANDWIDTH_THROTTLE_MAX	10

enum { RPGA1, RPGB1, RPGA2, RPGB2, MSCF, UNKNOWN_NODE };

typedef struct {
  char	source_node[ 8 ];               /* Node. */
  char	source_nodename[ 16 ];          /* Node name. */
  int	source_flag;                    /* Node flag. */
  int	source_channel;                 /* Node channel number. */
  char  source_ip[ IP_ADDRESS_LENGTH ]; /* Node IP. */
  int	detected;                       /* Node not found flag. */
  int	is_local;                       /* Is local node. */
  char  save_log_file_name[ MAX_DIRECTORY_LENGTH+MAX_FILENAME_LENGTH ];
} node_info_t;

/*	Global variables.	*/

static	int	FAA_flag = NO;
static	char    Save_log_directory[ MAX_DIRECTORY_LENGTH ] = "";
static	char    Default_directory[ MAX_DIRECTORY_LENGTH ] = "";
static	char	Save_log_description[ MAX_DESCRIPTION_LENGTH ] = "";
static	char	Default_description[ MAX_DESCRIPTION_LENGTH ] = "";
static	int	Selected_node = UNKNOWN_NODE;
static	int	Bandwidth_throttle = BANDWIDTH_THROTTLE_MIN;
static	int	Print_filename_flag = NO;
static	node_info_t		Node_info[ MAX_NUM_NODES ];

/*	Function prototypes.		*/

static void Parse_command_line( int argc, char *argv[] );
static void Initialize_node_info();
static void Save_log();
static void Copy_log();
static void Test_source_node_connectivity( node_info_t * );
static int  On_terminate( int, int );
static int  Rmt_transmit_data_callback( rmt_transfer_event_t * );
static void Verify_save_log_directory( char * );
static void Verify_node( char * );
static void Verify_save_log_description( char * );
static void Verify_bandwidth_throttle( char * );
static void Print_usage();

/************************************************************************
 *	Description: This is the main function for the Save Log Task.	*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit_code						*
 ************************************************************************/

int main( int argc, char *argv [] )
{
  char *p = NULL;
  int ret = -1;

  /* Initialize logging. */

  LE_set_option( "LB size", NUM_LE_LOG_MESSAGES );
  if( ( ret = LE_init( argc, argv ) ) < 0 )
  {
    fprintf( stderr, "Failed to initialize LE (%d)\n", ret );
    exit( 1 );
  }

  /* Register termination handler. */

  if( ORPGTASK_reg_term_handler( On_terminate ) < 0 )
  {
    LE_send_msg( GL_ERROR, "Could not register termination handler" );
    exit( 1 );
  }

  /* Is this site FAA redundant? */

  p = ORPGMISC_get_site_name( "is_redundant" );
  if( p != NULL && strcmp( p, "yes" ) == 0 ){ FAA_flag = YES; }

  /* Set defaults. */

  sprintf( Default_directory, "%s/%s", getenv( "HOME" ), "save_logs" );
  sprintf( Default_description, "%s", "save_log" );

  /* Initialize node information. */

  Initialize_node_info();

  /* Read command line. */

  Parse_command_line( argc, argv );

  /* Register callback for RMT events to throttle bandwidth usage. */

  RMT_listen_for_progress( Rmt_transmit_data_callback, NULL );

  /* Make sure target node is detected. */

  if( Node_info[ Selected_node ].detected )
  {
    Save_log( &Node_info[ Selected_node ] );
    Copy_log( &Node_info[ Selected_node ] );
    if( Print_filename_flag == YES );
    {
      printf( "%s\n", Node_info[Selected_node].save_log_file_name );
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "Node %s is not detected",
                 Node_info[Selected_node].source_nodename );
    exit( 1 );
  }

  exit( 0 );
}

/************************************************************************
 * Parse_command_line: Parse info passed in from command line.
 ************************************************************************/

static void Parse_command_line( int argc, char *argv[] )
{
  char user_defined_description[ MAX_DESCRIPTION_LENGTH ] = "";
  char user_defined_directory[ MAX_DIRECTORY_LENGTH ] = "";
  char user_defined_node[ MAX_NODE_LENGTH ] = "";
  char user_defined_throttle[ MAX_NODE_LENGTH ] = "";
  char options[ 16 ];
  int input;

  strcpy( options, "ha:d:n:t:p" );
  opterr = 0;

  while( ( input = getopt( argc, argv, options ) ) != -1 )
  {
    switch( input )
    {
      case 'a':
        strncpy( user_defined_description, optarg, MAX_DESCRIPTION_LENGTH );
        break;
      case 'd':
        strncpy( user_defined_directory, optarg, MAX_DIRECTORY_LENGTH );
        break;
      case 'n':
        strncpy( user_defined_node, optarg, MAX_NODE_LENGTH );
        break;
      case 't':
        strncpy( user_defined_throttle, optarg, MAX_THROTTLE_LENGTH );
        break;
      case 'p':
        Print_filename_flag = YES;
        break;
      case 'h':
        Print_usage();
        exit( 0 );
      case '?':
        LE_send_msg( GL_ERROR, "Option '-%c' is either invalid or missing a required argument. Use -h option for help.", (char) optopt );
        exit( 1 ); 
      default:
        Print_usage();
        exit( 1 );
    }
  }

  if( strlen( user_defined_description ) > 0 )
  {
    Verify_save_log_description( user_defined_description );
    strcpy( Save_log_description, user_defined_description );
  }
  else
  {
    strcpy( Save_log_description, Default_description );
  }

  if( strlen( user_defined_directory ) > 0 )
  {
    Verify_save_log_directory( user_defined_directory );
    strcpy( Save_log_directory, user_defined_directory );
  }
  else
  {
    strcpy( Save_log_directory, Default_directory );
  }

  if( strlen( user_defined_node ) > 0 )
  {
    Verify_node( user_defined_node );
  }

  if( strlen( user_defined_throttle ) > 0 )
  {
    Verify_bandwidth_throttle( user_defined_throttle );
  }
}

/************************************************************************
 * Print_usage: Print usage info for this task.
 ************************************************************************/

static void Print_usage()
{
  fprintf( stderr, "\nThis task generates a save log on one of the\n" );
  fprintf( stderr, "RPG nodes and copies to the local machine.\n\n" );
  fprintf( stderr, "The following options are valid:\n" );
  fprintf( stderr, "  -h - Print this message\n" );
  fprintf( stderr, "  -a - Description to put in save log file name\n" );
  fprintf( stderr, "       [Default: save_log]\n" );
  fprintf( stderr, "       *Only alphanumerics, underscores, and periods\n" );
  fprintf( stderr, "  -n - Node to obtain save log from\n" );
  fprintf( stderr, "       [Default: Local node]\n" );
  fprintf( stderr, "       *Choices: RPGA1, RPGA2, RPGB1, RPGB2, MSCF\n" );
  fprintf( stderr, "  -d - User-defined directory to copy save log into\n" );
  fprintf( stderr, "       [Default: ~/save_logs]\n" );
  fprintf( stderr, "  -t - Factor to throttle bandwidth [1-10]\n" );
  fprintf( stderr, "       [Default: 1 ] (no throttling)\n" );
  fprintf( stderr, "       *Example: 2 = use up to 1/2 available bandwidth\n" );
  fprintf( stderr, "       *Example: 5 = use up to 1/5 available bandwidth\n" );
  fprintf( stderr, "  -p - Print name of save log file when finished\n" );
  fprintf( stderr, "       [Default: do not print]\n" );
  fprintf( stderr, "\n\n" );
}

/************************************************************************
 * Verify_save_log_description: Verify save log description passed on command
 *     line is valid.
 ************************************************************************/

static void Verify_save_log_description( char *desc )
{
  int i;

  if( strlen( desc ) > MAX_DESCRIPTION_LENGTH )
  {
    LE_send_msg( GL_ERROR, "-a argument length > %d", MAX_DESCRIPTION_LENGTH );
    exit( 1 );
  }

  for( i = 0; i < strlen( desc ); i++ )
  {
    if( desc[i] != '.' && desc[i] != '_' && !isalnum( desc[i] ) )
    {
      LE_send_msg( GL_ERROR, "Description %s has illegal characters", desc );
      exit( 1 );
    }
  }
}

/************************************************************************
 * Verify_save_log_directory: Verify save log directory passed on command
 *     line is valid.
 ************************************************************************/

static void Verify_save_log_directory( char *dir )
{
  struct stat stat_buf;

  if( strlen( dir ) > MAX_DIRECTORY_LENGTH )
  {
    LE_send_msg( GL_ERROR, "-d argument length > %d", MAX_DIRECTORY_LENGTH );
    exit( 1 );
  }
  else if( stat( dir, &stat_buf ) < 0 )
  {
    LE_send_msg( GL_ERROR, "Unable to stat %s", dir );
    exit( 1 );
  }
  else if( !S_ISDIR( stat_buf.st_mode ) )
  {
    LE_send_msg( GL_ERROR, "Argument %s is not a directory", dir );
    exit( 1 );
  }
}

/************************************************************************
 * Verify_node: Verify node name passed on command line is valid.	*
 ************************************************************************/

static void Verify_node( char *node )
{
  if( strcmp( node, "RPGA1" ) == 0 || strcmp( node, "RPGA" ) == 0 ||
      strcmp( node, "rpga1" ) == 0 || strcmp( node, "rpga" ) == 0 ||
       strcmp( node, "rpg1" ) == 0 ||  strcmp( node, "rpg" ) == 0 )
  {
    Selected_node = RPGA1;
  }
  else if( strcmp( node, "RPGB1" ) == 0 || strcmp( node, "RPGB" ) == 0 ||
           strcmp( node, "rpgb1" ) == 0 || strcmp( node, "rpgb" ) == 0 )
  {
    Selected_node = RPGB1;
  }
  else if( strcmp( node, "RPGA2" ) == 0 || strcmp( node, "RPG2" ) == 0 ||
           strcmp( node, "rpga2" ) == 0 || strcmp( node, "rpg2" ) == 0 )
  {
    Selected_node = RPGA2;
  }
  else if( strcmp( node, "RPGB2" ) == 0 || strcmp( node, "rpgb2" ) == 0 )
  {
    Selected_node = RPGB2;
  }
  else if( strcmp( node, "MSCF" ) == 0 || strcmp( node, "mscf" ) == 0 )
  {
    Selected_node = MSCF;
  }
  else
  {
    LE_send_msg( GL_ERROR, "Invlalid -n argument. Use -h option for help." );
    exit( 1 );
  }

  if( ( Selected_node == RPGA2 || Selected_node == RPGB2 ) && !FAA_flag )
  {
    LE_send_msg( GL_ERROR, "Node %s is invalid for non-FAA systems.", node );
    exit( 1 );
  }
}

/************************************************************************
 * Verify_bandwidth_throttle: Verify throttle factor passed on command
 *     line is valid.
 ************************************************************************/

static void Verify_bandwidth_throttle( char *throttle )
{
  int i;

  for( i = 0; i < strlen( throttle ); i++ )
  {
    if( ! isdigit( throttle[i] ) )
    {
      LE_send_msg( GL_ERROR, "Throttle %s contains non-digits", throttle );
      exit( 1 );
    }
  }

  i = atoi( throttle );

  if( i < BANDWIDTH_THROTTLE_MIN || i > BANDWIDTH_THROTTLE_MAX )
  {
    LE_send_msg( GL_ERROR, "Throttle factor (%d) is outside limits", i );
    exit( 1 );
  }

  Bandwidth_throttle = i;
}

/************************************************************************
 * Save_log: Perform save log on targeted node.
 ************************************************************************/

static void Save_log()
{
  int rss_ret = -1;
  int cmd_ret = -1;
  int n_bytes = -1;
  char output_buf[ MAX_OBUF_SIZE ] = "";
  char cmd[ MAX_CMD_LENGTH ] = "";
  char func[ MAX_CMD_LENGTH ] = "";
  char *tok = NULL;

  /* Build command string. */

  sprintf( cmd, "save_log" );
  strcat( cmd, " -a " );
  strcat( cmd, Save_log_description );
  strcat( cmd, " -p " );

  if( ! ORPGMISC_is_operational() ){ strcat( cmd, " -s " ); }

  if( Node_info[ Selected_node ].is_local )
  {
    /* Run command on local machine. */
    cmd_ret = MISC_system_to_buffer( cmd, output_buf, MAX_OBUF_SIZE, &n_bytes );
    cmd_ret = cmd_ret >> 8;
  }
  else
  {
    /* Run command on remote machine. */
    sprintf( func, "%s:MISC_system_to_buffer", Node_info[Selected_node].source_ip );
    rss_ret = RSS_rpc( func, "i-r s-i ba-%d-io i ia-io", MAX_OBUF_SIZE,
                   &cmd_ret, cmd, output_buf, MAX_OBUF_SIZE, &n_bytes );
    cmd_ret = cmd_ret >> 8;
    if( rss_ret != 0 )
    {
      /* RSS failed. */
      LE_send_msg( GL_ERROR, "RSS_rpc failed (%d) for Node: %s",
                   rss_ret, Node_info[Selected_node].source_nodename );
      exit( 1 );
    }
  }

  if( cmd_ret != 0 )
  {
    /* Save log failed. */
    LE_send_msg( GL_ERROR, "Command: %s Failed (%d) for Node: %s",
                 cmd, cmd_ret, Node_info[Selected_node].source_nodename );
    LE_send_msg( GL_ERROR, "%ms", output_buf );
    exit( 1 );
  }
  else
  {
    /* Save log succeeded. */
    tok = strtok( output_buf, "\n" ); /* Set up tokenizer to strip newlines. */
    strcpy( Node_info[Selected_node].save_log_file_name, tok );
  }
}

/************************************************************************
 * Copy_log: Copy save log to appropriate directory.
 ************************************************************************/

static void Copy_log()
{
  int rss_ret = -1, cmd_ret = -1;
  int n_bytes = -1;
  char output_buf[ MAX_OBUF_SIZE ];
  char cmd[ MAX_CMD_LENGTH ];
  char src_filename[ MAX_DIRECTORY_LENGTH + MAX_FILENAME_LENGTH ];
  char dest_filename[ MAX_DIRECTORY_LENGTH + MAX_FILENAME_LENGTH ];

  /* Set destination file name. This is the same regardless
     if save log is local or remote. */

  sprintf( dest_filename, "%s/%s", Save_log_directory,
           Node_info[Selected_node].save_log_file_name );

  /* Take action depending if target node is local or remote. */

  if( Node_info[ Selected_node ].is_local )
  {
    /* If user did not define a different target directory, don't copy. */
    if( strcmp( Default_directory, Save_log_directory ) == 0 ){ return; }

    /* Local node is a simple "cp". */
    sprintf( src_filename, "%s/%s", Default_directory,
             Node_info[Selected_node].save_log_file_name );
    sprintf( cmd, "cp -f %s %s", src_filename, dest_filename );
    cmd_ret = MISC_system_to_buffer( cmd, output_buf, MAX_OBUF_SIZE, &n_bytes );
    cmd_ret = cmd_ret >> 8;

    if( cmd_ret != 0 )
    {
      /* Copy failed. */
      LE_send_msg( GL_ERROR, "Copy failed (%d) for %s", cmd_ret, cmd );
      LE_send_msg( GL_ERROR, "%ms", output_buf );
      exit( 1 );
    }
  }
  else
  {
    /* Remote machine uses RSS, so add target node ip to source filename. */
    sprintf( src_filename, "%s:%s/%s", Node_info[ Selected_node ].source_ip,
             Default_directory, Node_info[Selected_node].save_log_file_name );
    rss_ret = RSS_copy( src_filename, dest_filename );

    if( rss_ret != RSS_SUCCESS )
    {
      /* RSS copy failed. */
      LE_send_msg( GL_ERROR, "RSS_copy failed (%d) for %s",
                   rss_ret, src_filename );
      exit( 1 );
    }
  }
} 

/************************************************************************
 * Rmt_transmit_data_callback: Callback for data transfer from RMT library.
 ************************************************************************/

static int Rmt_transmit_data_callback( rmt_transfer_event_t *event )
{
  static double	previous_transmit_time = -1; /* seconds */
  static int init_flag = NO;
  double current_transmit_time = -1;
  int millisecond_diff = -1;
  struct timeval tv;

  if( init_flag == NO )
  {
    init_flag = YES;
    gettimeofday( &tv, NULL );
    previous_transmit_time = (double)tv.tv_sec + (double)(tv.tv_usec/1000000.0);
    return 0;
  }
  
  gettimeofday( &tv, NULL );
  current_transmit_time = (double)tv.tv_sec + (double)(tv.tv_usec/1000000.0);
  millisecond_diff = (current_transmit_time-previous_transmit_time)*1000;

  if( Bandwidth_throttle > 1 )
  {
    msleep( (Bandwidth_throttle-1)*millisecond_diff );
    gettimeofday( &tv, NULL );
    current_transmit_time = (double)tv.tv_sec + (double)(tv.tv_usec/1000000.0);
  }

  previous_transmit_time = current_transmit_time;

  return 0;
}

/************************************************************************
 * Initialize_node_info: Initialize node information.
 ************************************************************************/

static void Initialize_node_info()
{
  if( FAA_flag )
  {
    strcpy( Node_info[ RPGA1 ].source_node, "RPGA" );
    strcpy( Node_info[ RPGA1 ].source_nodename, "RPGA 1" );
    Node_info[ RPGA1 ].source_flag = RPGA1;
    Node_info[ RPGA1 ].source_channel = 1;
    Test_source_node_connectivity( &Node_info[ RPGA1 ] );

    strcpy( Node_info[ RPGA2 ].source_node, "RPGA" );
    strcpy( Node_info[ RPGA2 ].source_nodename, "RPGA 2" );
    Node_info[ RPGA2 ].source_flag = RPGA2;
    Node_info[ RPGA2 ].source_channel = 2;
    Test_source_node_connectivity( &Node_info[ RPGA2 ] );

    strcpy( Node_info[ RPGB1 ].source_node, "RPGB" );
    strcpy( Node_info[ RPGB1 ].source_nodename, "RPGB 1" );
    Node_info[ RPGB1 ].source_flag = RPGB1;
    Node_info[ RPGB1 ].source_channel = 1;
    Test_source_node_connectivity( &Node_info[ RPGB1 ] );

    strcpy( Node_info[ RPGB2 ].source_node, "RPGB" );
    strcpy( Node_info[ RPGB2 ].source_nodename, "RPGB 2" );
    Node_info[ RPGB2 ].source_flag = RPGB2;
    Node_info[ RPGB2 ].source_channel = 2;
    Test_source_node_connectivity( &Node_info[ RPGB2 ] );

    strcpy( Node_info[ MSCF ].source_node, "MSCF" );
    strcpy( Node_info[ MSCF ].source_nodename, "MSCF" );
    Node_info[ MSCF ].source_flag = MSCF;
    Node_info[ MSCF ].source_channel = 1;
    Test_source_node_connectivity( &Node_info[ MSCF ] );
  }
  else
  {
    strcpy( Node_info[ RPGA1 ].source_node, "RPGA" );
    strcpy( Node_info[ RPGA1 ].source_nodename, "RPGA" );
    Node_info[ RPGA1 ].source_flag = RPGA1;
    Node_info[ RPGA1 ].source_channel = 1;
    Test_source_node_connectivity( &Node_info[ RPGA1 ] );

    strcpy( Node_info[ RPGB1 ].source_node, "RPGB" );
    strcpy( Node_info[ RPGB1 ].source_nodename, "RPGB" );
    Node_info[ RPGB1 ].source_flag = RPGB1;
    Node_info[ RPGB1 ].source_channel = 1;
    Test_source_node_connectivity( &Node_info[ RPGB1 ] );

    strcpy( Node_info[ MSCF ].source_node, "MSCF" );
    strcpy( Node_info[ MSCF ].source_nodename, "MSCF" );
    Node_info[ MSCF ].source_flag = MSCF;
    Node_info[ MSCF ].source_channel = 1;
    Test_source_node_connectivity( &Node_info[ MSCF ] );
  }
}

/************************************************************************
 * Test_source_node_connectivity: Test if nodes are detectable over network.
 ************************************************************************/

static void Test_source_node_connectivity( node_info_t *obj )
{
  int ret = -1;
  
  /* Test for source node. */

  ret = ORPGMGR_discover_host_ip( obj->source_node, obj->source_channel,
                                  obj->source_ip, IP_ADDRESS_LENGTH);

  if( ret > 0 )
  {
    obj->detected = YES; /* Node is detectable. */
    if( strlen( obj->source_ip ) == 0 )
    {
      /* Is the local node. */
      obj->is_local = YES;
      Selected_node = obj->source_flag;
    }
    else
    {
      /* Is not the local node. */
      obj->is_local = NO;
    }
  }
  else
  {
    /* Node is not detectable, set values accordingly. */
    obj->detected = NO;
    obj->source_ip[0] = '\0';
    obj->is_local = NO;

    if( ret < 0 )
    {
      /* Command failed. */
      LE_send_msg( GL_ERROR, "ORPGMGR_discover_host_ip failed (%d) for %s",
                   ret, obj->source_nodename );
    }
    else if( ret == 0 )
    {
      /* Command succeeded, but node not found on network. */
      LE_send_msg( GL_ERROR, "System %s not detected", obj->source_nodename );
    }
  }
}

/***************************************************************************/
/* ON_TERMINATE: Termination handler.                                      */
/***************************************************************************/

static int On_terminate( int signal, int flag )
{
  LE_send_msg( GL_ERROR, "Termination signal handler called" );
  return 0;
}

