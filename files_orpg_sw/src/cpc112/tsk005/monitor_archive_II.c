/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/16 20:49:41 $
 * $Id: monitor_archive_II.c,v 1.5 2013/04/16 20:49:41 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

/* Include files */

#include <orpg.h> 
#include <archive_II.h> 
#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_TIMER		5
#define	PUBLISH_CLIENTS_TIMER	10
#define	CLIENT_LEN		1024
#define	TOKEN_LEN		128

/* Structures */

/* Static/global variables */

static ArchII_clients_t ArchII_clients;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void Publish_clients();
static void Get_archive_II_clients();

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  time_t current_time = 0;
  time_t previous_time = 0;

  /* Parse command line. */
  Parse_cmd_line( argc, argv );

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  /* Loop and post */
  while( 1 )
  {
    current_time = time( NULL );
    if( ( current_time - previous_time ) >= PUBLISH_CLIENTS_TIMER )
    {
      previous_time = current_time;
      Publish_clients ();
    }
    sleep( LOOP_TIMER );
  }

  return 0;
}

/************************************************************************
 Description: Determine Archive-II clients.
 ************************************************************************/

static void Get_archive_II_clients()
{
  int ret = 0;
  int buflen = 0;
  int offset = 0;
  int token_length = 0;
  int client_length = 0;
  char buf[CLIENT_LEN];
  char tok[TOKEN_LEN];
  char *cmd = "sh -c (roc_ldm_control -n)";
  char *p = NULL;

  /* No clients by default */
  ArchII_clients.num_clients = 0;

  /* Retrieve client list */
  if( ( ret = MISC_system_to_buffer( cmd, buf, CLIENT_LEN, &buflen ) ) != 0 )
  {
    LE_send_msg( GL_ERROR, "MISC_system_to_buffer failed (%d)", ret ); 
  }
  else if( buflen > 0 )
  {
    if( strstr( buf, "No connections found" ) == NULL )
    {
      p = buf;
      while( ( offset = MISC_get_token( p, "S\n", 0, tok, TOKEN_LEN ) ) > 0 )
      {
        token_length = strlen( tok );
        if( token_length > 0 && ( ( client_length + token_length + 1 ) <= MAX_LDM_CLIENT_INFO_SIZE ) )
        {
          strcpy( ArchII_clients.buf + client_length, tok );
          client_length += token_length + 1;
          ArchII_clients.num_clients++;
        }
        p += offset;
      }
    }
  }
}

/************************************************************************
 Description: Determine and then publish connected Archive II clients.
 ************************************************************************/

static void Publish_clients()
{
  int ret = 0;

  Get_archive_II_clients();

  ArchII_clients.ctime = time (NULL);

  if( ( ret = ORPGDA_write( ORPGDAT_ARCHIVE_II_INFO, (char *) &ArchII_clients, 
                  sizeof( ArchII_clients_t ), ARCHIVE_II_CLIENTS_ID ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "ORPGDA_write of clients failed (%d)", ret );
  }
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

