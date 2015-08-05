/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/28 15:12:28 $
 * $Id: lb_to_ldm.c,v 1.4 2014/10/28 15:12:28 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/

/* Include files. */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <orpg.h>
#include <roc_ldm.h>

/* Defines/enums. */

#define MISSING_LB_ID	-999
#define MAX_TIMESTRING_LEN	64
#define MAX_PRINT_MSG_LEN	2048
#define	MAX_RSSD_RETRIES	5
#define	MAX_RSSD_HOST_LEN	128
#define	MIN_LB_INACTIVE_TIME	60
#define	MAX_LB_INACTIVE_TIME	120
#define	MAIN_LOOP_TIMER		1
#define	REREGISTER_LB_TIME	90

enum { NO = 0, YES };

/* Static/global variables. */

static LB_id_t ORPGDA_LB_id = MISSING_LB_ID;
static LB_id_t Input_LB_id = MISSING_LB_ID;
static char    *Input_LB_file = NULL;
static int     Input_LB_updated_flag = NO;
static int     Debug_mode = NO;
static int     Exit_flag = NO;
static int     RSSD_connect_lost_flag = NO;
static char    Input_LB_host[MAX_RSSD_HOST_LEN] = "";
static time_t  Input_LB_update_time = -1;
static int     LB_UN_register_cmd_ret = -1;
static time_t  LB_UN_last_register_cmd_time = -1;
static time_t  Current_time = -1;
static int     RSSD_fd = -1;

/* Function prototypes. */

static void Read_options( int, char *[] );
static void Set_signals();
static void Signal_handler( int );
static void Connect_to_RSSD();
static void Check_RSSD_connection();
static void RSSD_lost_connection_callback( int , char * );
       int  RMT_check_connection();
static void Get_input_LB_filename();
static void Set_LB_id();
static void Register_LB_callback();
static void Verify_LB();
static void Input_LB_callback( int, LB_id_t, int, void * );
static void Read_input_LB();
static void Print_usage( char * );
static void P_out( const char *, ... );
static void P_error( const char *, ... );
static void P_debug( const char *, ... );
static void Toggle_debug_mode();

/************************************************************************
 Description: Main function.
 ************************************************************************/

int main( int argc, char **argv )
{
  /* Process command line arguments. */

  Read_options( argc, argv );

  /* Define signals to catch. */

  Set_signals();

  /* Connect to RSSD. */

  Connect_to_RSSD();

  /* Get filename of input LB */

  Get_input_LB_filename();

  /* Initialize */
  Current_time = MISC_systime(NULL);
  Verify_LB();
  Input_LB_update_time = Current_time;

  /* Main processing loop. */

  while( !Exit_flag )
  {
    /* Get time value to use during this iteration */
    Current_time = MISC_systime(NULL);

    /* If RSSD connection lost, try to reconnect */
    if( RSSD_connect_lost_flag )
    {
      RSSD_connect_lost_flag = NO;
      Connect_to_RSSD();
    }

    /* Verify input LB is valid and active */
    Verify_LB();

    /* If input LB has been updated, read new msgs */
    if( Input_LB_updated_flag )
    {
      Input_LB_updated_flag = NO;
      P_debug( "Input LB updated flag is YES, so read LB" );
      Input_LB_update_time = Current_time;
      Read_input_LB();
    }

    /* If input LB hasn't been updated in a long time,
       something is wrong. Exit and start over. */
    if( ( Current_time - Input_LB_update_time ) > MAX_LB_INACTIVE_TIME )
    {
      P_error( "Over %d seconds since LB update. Exit.", MAX_LB_INACTIVE_TIME );
      Exit_flag = YES;
    }

    sleep(MAIN_LOOP_TIMER);
  }

  P_debug( "Exiting" );
  return 0;
}

/************************************************************************
 Description: Connect to local RSSD server.
 ************************************************************************/

static void Connect_to_RSSD()
{
  int ret = 0;
  int rssd_connected = NO;
  int rssd_connect_count = 0;

  while( rssd_connect_count < MAX_RSSD_RETRIES )
  {
    P_debug( "Attempt %d of %d to connect to rssd on %s", rssd_connect_count+1, MAX_RSSD_RETRIES, Input_LB_host );
    if( ( ret = RMT_create_connection( Input_LB_host ) ) < 0 )
    {
      P_error( "RMT_create_connection failed (%d)", ret );
      rssd_connect_count++;
    }
    else
    {
      P_debug( "RMT_create_connection succeeded (fd = %d)", ret );
      rssd_connected = YES;
      RSSD_fd = ret;
      RMT_set_lost_conn_callback( RSSD_lost_connection_callback );
      break;
    }
    sleep( 5 );
  }

  if( !rssd_connected )
  {
    P_error( "Could not connect to RMT/RSSD server. Terminate." );
    exit( 2 );
  }
}

/************************************************************************
 Description: Verify RSSD connection.
 ************************************************************************/

static void RSSD_lost_connection_callback( int fd, char *ip )
{
  P_error( "RSSD lost connection cb: %d %s (%d)", fd, ip, RSSD_fd );
  if( fd == RSSD_fd )
  {
    P_error( "RSSD fd lost. Try to reconnect." );
    RSSD_connect_lost_flag = YES;
  }
}

/************************************************************************
 Description: Verify RSSD connection.
 ************************************************************************/

static void Check_RSSD_connection()
{
  int ret = 0;

  if( ( ret = RMT_is_connected( Input_LB_host ) ) == NO )
  {
    P_error( "RSSD LOST CONNECTION DETECTED)" );
    P_error( "RMT_is_connected returned NO (%d)", ret );
    RSSD_connect_lost_flag= YES;
  }
  P_debug( "RMT_is_connected returned %d", ret );

  if( ( ret = RMT_check_connection() ) != RMT_SUCCESS )
  {
    P_error( "RSSD LOST CONNECTION DETECTED)" );
    P_error( "RMT check_connection returned %d", ret );
    RSSD_connect_lost_flag= YES;
  }
  P_debug( "RMT_check_connection returned %d", ret );
}

/************************************************************************
 Description: Get filename associated with input LB id.
 ************************************************************************/

static void Get_input_LB_filename()
{
  if( ( Input_LB_file = ORPGDA_lbname( ORPGDA_LB_id ) ) != NULL )
  {
    P_debug( "LB id %d = %s", ORPGDA_LB_id, Input_LB_file );
  }
  else
  {
    P_error( "Can't translate LB id %d to a file. Terminate.", ORPGDA_LB_id );
    exit( 3 );
  }
}

/************************************************************************
 Description: Verify LB can still be accessed. Reset if needed.
 ************************************************************************/

static void Verify_LB()
{
  LB_info lb_info;
  int ret = 0;
  int time_diff = 0;

  P_debug( "Verify LB" );

  /* Calculate number of seconds since last LB update */
  time_diff = Current_time - Input_LB_update_time;

  if( time_diff > MIN_LB_INACTIVE_TIME )
  {
    P_debug( "LB inactivity %d > %d seconds", time_diff, MIN_LB_INACTIVE_TIME );
    /* Make sure RSSD connection is valid */
    Check_RSSD_connection();
    /* Test LB id using LB_msg_info. */
    if( ( ret = LB_msg_info( Input_LB_id, LB_LATEST, &lb_info ) ) != LB_SUCCESS  )
    {
      P_debug( "LB msg info check failed (%d)", ret );
      /* Function failed, so reset LB id. */
      if( Input_LB_id != MISSING_LB_ID )
      {
        P_debug( "LB ID previously initialized...reinitializing"  );
        /* LB id was previously initialized. */
        LB_close( Input_LB_id );
        Input_LB_id = MISSING_LB_ID;
      }
      else
      {
        P_debug( "LB ID never initialized" );
      }
      Set_LB_id();
    }
    else
    {
      P_debug( "LB check succeeded" );
    }
    Register_LB_callback();
  }
}

/************************************************************************
 Description: Set ID of input LB.
 ************************************************************************/

static void Set_LB_id()
{
  int ret = 0;

  P_debug( "Set LB ID of %s", Input_LB_file );

  /* Open LB. */
  if( ( ret = LB_open( Input_LB_file, LB_READ, NULL ) ) < 0 )
  {
    P_error( "LB_open failed (%d) for %s", ret, Input_LB_file );
    exit(1);
  }

  Input_LB_id = ret;
  P_debug( "LB_open succeeded. LB ID = %d", Input_LB_id );
}

/************************************************************************
 Description: Register callback for input LB.
 ************************************************************************/

static void Register_LB_callback()
{
  int ret = 0;

  P_debug( "Register LB callback" );

  if( Input_LB_id < 0 )
  {
    P_debug( "Can't register invalid Input LB id (%d)", Input_LB_id );
    return;
  }

  if( ( Current_time - LB_UN_last_register_cmd_time ) > REREGISTER_LB_TIME )
  {
    if( LB_UN_register_cmd_ret == LB_SUCCESS )
    {
      P_debug( "LB callback previously registered...deregister it" );
      EN_control( EN_DEREGISTER );
      if( ( ret = LB_UN_register( Input_LB_id, LB_ANY, Input_LB_callback ) ) != LB_SUCCESS )
      {
        P_error( "LB callback de-register failed (%d)", ret );
      }
    }

    if( ( ret = LB_UN_register( Input_LB_id, LB_ANY, Input_LB_callback ) ) != LB_SUCCESS )
    {
      if( ret != LB_UN_register_cmd_ret )
      {
        P_error( "LB callback register failed (%d)", ret );
      }
    }
    else
    {
      P_debug( "LB callback registered" );
    }
    LB_UN_register_cmd_ret = ret;
    LB_UN_last_register_cmd_time = Current_time;
  }
  else
  {
    P_debug( "LB re-register timer not expired...do nothing" );
    return;
  }
}

/************************************************************************
 Description: Callback when input LB is updated.
 ************************************************************************/

static void Read_input_LB()
{
  int ret = 0;
  char *buf = NULL;

  /* Do until no more unread messages in LB. */

  while( 1 )
  {
    if( ( ret = LB_read( Input_LB_id, &buf, LB_ALLOC_BUF, LB_NEXT ) ) < 0 )
    {
      if( ret == LB_TO_COME )
      {
        P_debug( "LB_read is LB_TO_COME" );
        break;
      }
      else if( ret == LB_EXPIRED )
      {
        P_debug( "LB__read is LB_EXPIRED" );
        continue;
      }
      else
      {
        P_error( "LB_read returned %d", ret );
        break;
      }
    }
    P_debug( "Size read from input LB is %d", ret );

    if( ( ret = ROC_LDM_write_to_pq( buf ) ) != 0 )
    {
      P_error( "ROC_LDM_write_to_pq failed (%d)", ret );
    }

    /* Free the read buffer. */

    if( buf != NULL ){ free( buf ); }
    buf = NULL;
  }

  return;
}

/************************************************************************
 Description: Callback when input LB is updated.
 ************************************************************************/

static void Input_LB_callback( int fd, LB_id_t mid, int minfo, void *arg )
{
   /* Set the update flag. */
   P_debug( "Input_LB_callback executed" );
   Input_LB_updated_flag = YES;
}

/************************************************************************
 Description: This function reads command line arguments.
 ************************************************************************/

static void Read_options( int argc, char *argv[] )
{
  int c = 0;

  opterr = 0;
  optind = 0;
  while( ( c = getopt( argc, argv, "hi:p:x" ) ) != EOF )
  {
    switch( c )
    {
      case 'h':
        Print_usage( argv[0] );
        exit( 0 );
      case 'x':
        Debug_mode = YES;
        ROC_LDM_print_debug_on();
        break;
      case 'i':
        if( optarg != NULL )
        {
          if( ( ORPGDA_LB_id = atoi( optarg ) ) < 0 )
          {
            ORPGDA_LB_id = MISSING_LB_ID;
          }
        }
        break;
      case 'p':
        if( optarg != NULL )
        {
          if( optarg[0] == '-' )
          {
            fprintf( stderr, "Invalid argument for -p option. Terminate.\n" );
            Print_usage( argv[0] );
            exit( 5 );
          }
          else if( strlen( optarg ) < MAX_RSSD_HOST_LEN )
          {
            strcpy( Input_LB_host, optarg );
          }
          else
          {
            strncpy( Input_LB_host, optarg, MAX_RSSD_HOST_LEN );
            Input_LB_host[MAX_RSSD_HOST_LEN-1] = '\0';
          }
        }
        break;
      case '?':
      default:
        Print_usage( argv[0] );
        exit( 6 );
    }
  }

  if( ORPGDA_LB_id == MISSING_LB_ID )
  {
    fprintf( stderr, "Missing/invalid argument for -i option. Terminate.\n" );
    Print_usage( argv[0] );
    exit( 7 );
  }
  else if( strlen( Input_LB_host ) == 0 )
  {
    fprintf( stderr, "Missing/invalid argument for -p option. Terminate.\n" );
    Print_usage( argv[0] );
    exit( 8 );
  }
}

/************************************************************************
 Description: This function prints usage information.
 ************************************************************************/

static void Print_usage( char *arg0 )
{
  fprintf( stderr, "Usage: %s -i int -p str [options]\nOptions:\n", arg0 );
  fprintf( stderr, "-h - print usage\n" );
  fprintf( stderr, "-i - LB integer id to read (Required)\n" );
  fprintf( stderr, "-p - Machine name with LB to read (Required)\n" );
  fprintf( stderr, "-x - debug mode, (Default: no)\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "Signals caught:\n" );
  fprintf( stderr, "SIGPIPE - ignore\n" );
  fprintf( stderr, "SIGUSR2 - toggle debug on/off\n" );
  fprintf( stderr, "SIGINT  - terminate\n" );
  fprintf( stderr, "SIGTERM - terminate\n" );
}

/************************************************************************
 Description: This function prints usage information.
 ************************************************************************/

static void Set_signals()
{
  P_debug( "Register callback for signal SIGPIPE" );
  signal( SIGPIPE, Signal_handler );
  P_debug( "Register callback for signal SIGUSR2" );
  signal( SIGUSR2, Signal_handler );
  P_debug( "Register callback for signal SIGINT" );
  signal( SIGINT, Signal_handler );
  P_debug( "Register callback for signal SIGTERM" );
  signal( SIGTERM, Signal_handler );
}

/************************************************************************
 Description: Handle signals.
 ************************************************************************/

static void Signal_handler( int sig )
{
  switch( sig )
  {
    case SIGPIPE :
      P_out( "signal SIGPIPE received...ignore" );
      return;
      break;
    case SIGUSR2 :
      P_out( "signal SIGUSR2 received...toggle debug mode" );
      Toggle_debug_mode();
      ROC_LDM_toggle_debug();
      break;
    case SIGINT :
      P_out( "signal SIGINT received...terminate" );
      Exit_flag = YES;
      break;
    case SIGTERM :
      P_out( "signal SIGTERM received...terminate" );
      Exit_flag = YES;
      break;
    default :
      P_error( "Signal_handler: unhandled signal: %d", sig );
  }
}

/************************************************************************
 Description: Toggle debug mode on/off.
 ************************************************************************/

static void Toggle_debug_mode()
{
  if( Debug_mode )
  {
    P_debug( "Toggle debug mode to NO" );
    Debug_mode = NO;
  }
  else
  {
    P_debug( "Toggle debug mode to YES" );
    Debug_mode = YES;
  }
  ROC_LDM_toggle_debug();
}

/************************************************************************
 Description: Print messages to stdout if debug flag is set.
 ************************************************************************/

void P_debug( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  char timebuf[MAX_TIMESTRING_LEN];
  long timestamp = 0;
  va_list arg_ptr; 
  
  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  if( Debug_mode )
  {
    /* Extract print format. */
    va_start( arg_ptr, format );
    vsprintf( buf, format, arg_ptr );
    va_end( arg_ptr );
  
    /* Create timestamp. */
    timestamp = time( NULL );
    if( ! strftime( timebuf, MAX_TIMESTRING_LEN, "%m/%d/%Y %H:%M:%S", gmtime( &timestamp ) ) )
    {
      sprintf( timebuf, "\?\?/\?\?/???? ??:??:??" );
    } 

    /* Print message to stdout. */
    fprintf( stdout, "%s DEBUG: >> %s\n", timebuf, buf );
    fflush( stdout );
  }
}

/************************************************************************
 Description: Print messages to stderr.
 ************************************************************************/

void P_error( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  char timebuf[MAX_TIMESTRING_LEN];
  long timestamp = 0;
  va_list arg_ptr; 
  
  /* If no format, nothing to do. */
  if( format == NULL ){ return; }
  
  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );
  
  /* Create timestamp. */
  timestamp = time( NULL );
  if( ! strftime( timebuf, MAX_TIMESTRING_LEN, "%m/%d/%Y %H:%M:%S", gmtime( &timestamp ) ) )
  {
    sprintf( timebuf, "\?\?/\?\?/???? ??:??:??" );
  } 
  
  /* Print message to stderr. */
  fprintf( stdout, "%s ERROR: >> %s\n", timebuf, buf );
  fflush( stdout );
} 

/************************************************************************
 Description: Print messages to stdout.
 ************************************************************************/

void P_out( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  char timebuf[MAX_TIMESTRING_LEN];
  long timestamp = 0;
  va_list arg_ptr; 
  
  /* If no format, nothing to do. */
  if( format == NULL ){ return; }
  
  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );
  
  /* Create timestamp. */
  timestamp = time( NULL );
  if( ! strftime( timebuf, MAX_TIMESTRING_LEN, "%m/%d/%Y %H:%M:%S", gmtime( &timestamp ) ) )
  {
    sprintf( timebuf, "\?\?/\?\?/???? ??:??:??" );
  } 

  /* Print message to stdout. */
  fprintf( stdout, "%s >> %s\n", timebuf, buf );
  fflush( stdout );
}


