/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2014/07/21 20:20:08 $ */
/* $Id: RPG_adapt_data_to_ldm.c,v 1.6 2014/07/21 20:20:08 ccalvert Exp $ */
/* $Revision: 1.6 $ */
/* $State: Exp $ */

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define	LOOP_TIMER		5
#define BUF_SIZE		1024
#define CMD_SIZE		512
#define MSG_SIZE		"40k"
#define MAX_PRINT_MSG_LEN	512
#define	MAX_TIMESTRING_LEN	64

/* Static/global variables */

static int Adapt_data_updated_flag = NO;

/* Function prototypes */

static void Print_usage();
static void Parse_cmd_line( int, char *[] );
static void Any_change_cb( int, LB_id_t, int, void * );

/*************************************************************************
  Description: Main function for task.
 *************************************************************************/

int main( int argc, char *argv[] )
{
  char *adapt_lb_name = NULL;
  char buf[BUF_SIZE];
  char cmd[CMD_SIZE];
  int ret = 0;
  int len = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Get name of adaptation data file to send */
  if( ( adapt_lb_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA ) ) == NULL )
  {
    LE_send_msg( GL_INFO, "ORPGDA_lbname failed. Terminate." );
    exit( RPG_LDM_FAILURE );
  }
  RPG_LDM_print_debug( "Adapt file: %s\n", adapt_lb_name );

  /* Setup DEAU and register callback */
  DEAU_LB_name( adapt_lb_name );
  if( ( ret = DEAU_UN_register( NULL, Any_change_cb ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "DEAU_UN_register failed (%d). Terminate.", ret );
    exit( RPG_LDM_FAILURE );
  }
  RPG_LDM_print_debug( "Successfully registered for adapt changes" );

  /* Build command to use */
  sprintf( cmd, "file_to_ldm -f %s -s %s -k %s", adapt_lb_name, MSG_SIZE, RPG_LDM_get_LDM_key_string_from_index( RPG_LDM_ADAPT_PROD ) );
  RPG_LDM_print_debug( "CMD: %s", cmd );

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  while( 1 )
  {
    if( Adapt_data_updated_flag == YES )
    {
      Adapt_data_updated_flag = NO;
      RPG_LDM_print_debug( "Execute %s", cmd );
      if( ( ret = MISC_system_to_buffer( cmd, buf, BUF_SIZE, &len ) ) < 0 )
      {
        LE_send_msg( GL_ERROR, "MISC_system_to_buffer failed (%d)", ret );
        if( len > 0 ){ LE_send_msg( GL_ERROR, "%s", buf ); }
      }
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
  printf( "  -h           - print usage info\n" );
  printf( "  -T task name - only used when invoked by mrpg (optional)\n" );
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

/*************************************************************************
  Description: Callback when any adaptation data changes.
 *************************************************************************/

static void Any_change_cb( int fd, LB_id_t msgid, int msg_info, void *arg )
{
  Adapt_data_updated_flag = YES;
  RPG_LDM_print_debug( "Adaptation data updated" );
}

