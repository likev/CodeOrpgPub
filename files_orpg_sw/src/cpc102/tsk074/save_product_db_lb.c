/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2013/03/14 19:43:31 $
 * $Id: save_product_db_lb.c,v 1.1 2013/03/14 19:43:31 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* Include files */

#include <orpg.h>

/* Definitions */

#define	MAX_LB_MSGS_TO_SAVE		9000
#define	SLEEP_TIMER			30

/* Enumerations */

enum { NO, YES };

/* Static/global variables */

static LB_id_t  LB_in_id = ORPGDAT_PRODUCTS;
static LB_id_t  LB_out_id = -1;
static int      LB_updated_flag = NO;
static int      LB_out_num_msgs = 0;
static int      LB_out_num_LBs = 0;
static int      LB_out_max_num_msgs = MAX_LB_MSGS_TO_SAVE;
static char     LB_out_base_name[64] = "playback";

/* Function prototypes */

static void Read_options( int, char *[] );
static void Print_usage( char *[] );
static void Init( int, char *[] );
static void LB_callback_fx( int, LB_id_t, int, void * );
static void Read_LB();
static void Check_LB_out();
static int  Termination_handler( int, int );


/******************************************************************

   Description: Main function.

******************************************************************/

int main( int argc, char *argv[] )
{
  Read_options( argc, argv );
  Init( argc, argv );
  ORPGMGR_report_ready_for_operation();

  /* Main loop */
  while( 1 )
  {
    sleep( SLEEP_TIMER );
    if( LB_updated_flag )
    {
      LB_updated_flag = NO;
      Read_LB();
    }
  }

  ORPGTASK_exit( GL_EXIT_SUCCESS );
}

/**************************************************************************

   Description: Print usage info.

**************************************************************************/

static void Print_usage( char **argv )
{
  fprintf( stdout, "Usage: %s (options)\n", argv[0] );
  fprintf( stdout, "  Options:\n" );
  fprintf( stdout, "    -b str - Base name of new LB (Default: %s)\n", LB_out_base_name );
  fprintf( stdout, "    -n     - Max number of messages in new LB (Default: %d)\n", LB_out_max_num_msgs );
  fprintf( stdout, "    -h     - Help\n" );
  fprintf( stdout, "\n" );
}

/**************************************************************************

   Description: Read and process command line arguments.

**************************************************************************/

static void Read_options( int argc, char *argv[] )
{
  int c = 0;

  while( ( c = getopt( argc, argv, "b:n:h" ) ) != EOF )
  {
    switch( c )
    {
      case 'b':
        /* Get base name of copied LB */
        if( optarg == NULL )
        {
          fprintf( stderr, "Option -b must have argument\n" );
          ORPGTASK_exit( GL_EXIT_FAILURE );
        }
        strcpy( LB_out_base_name, optarg );
        break;

      case 'n':
        /* Get maximum number of msgs for copied LB */
        if( optarg == NULL )
        {
          fprintf( stderr, "Option -n must have an integer argument\n" );
          ORPGTASK_exit( GL_EXIT_FAILURE );
        }
        else if( ( LB_out_max_num_msgs = atoi( optarg ) ) < 0 )
        {
          fprintf( stderr, "Option -n argument not valid\n" );
          ORPGTASK_exit( GL_EXIT_FAILURE );
        }
        break;

      case 'h':
      case '?':
        Print_usage( argv );
        break;
    }
  }
}

/******************************************************************

   Description: Initialize variables.

******************************************************************/

static void Init( int argc, char *argv[] )
{
  int ret = 0;

  /* Initialize Log-Error services. */
  if( ( ret = ORPGMISC_LE_init( argc, argv, 1500, 0, -1, 0 ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "ORPGMISC_LE_init() Failed (ret: %d)", ret );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }

  /* Register termination handler */
   if( ( ret = ORPGTASK_reg_term_handler( Termination_handler ) ) < 0 )
  {
    LE_send_msg (GL_ERROR, "Term handler register Failed (ret: %d)", ret);
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }

  /* Read options.  Exit on failure. */
  ORPGDA_write_permission( LB_in_id );

  /* Register callback */
  if( ( ret = ORPGDA_UN_register( LB_in_id, LB_ANY, LB_callback_fx ) ) != LB_SUCCESS )
  {
    LE_send_msg( GL_ERROR, "Unable to register callback for lb (%d)", ret );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }

  LE_send_msg( GL_INFO, "LB in id:            %d", LB_in_id );
  LE_send_msg( GL_INFO, "LB updated_flag:     %d", LB_updated_flag );
  LE_send_msg( GL_INFO, "LB out_num_msgs:     %d", LB_out_num_msgs );
  LE_send_msg( GL_INFO, "LB out_num_LBs:      %d", LB_out_num_LBs );
  LE_send_msg( GL_INFO, "LB out_max_num_msgs: %d", LB_out_max_num_msgs );
  LE_send_msg( GL_INFO, "LB out_base_name:    %s", LB_out_base_name );

  Check_LB_out();
}

/******************************************************************

   Description: Read Products DB LB.

******************************************************************/

static void Read_LB()
{
  int ret = 0;
  int lb_msg_size = 0;
  char *buf = NULL;

  while( 1 )
  {
    /* Exhaust all unread messages */
    if( ( ret = ORPGDA_read( LB_in_id, &buf, LB_ALLOC_BUF, LB_NEXT ) ) == LB_TO_COME )
    {
      break;
    }
    else if( ret == LB_EXPIRED )
    {
      ORPGDA_seek( LB_in_id, 0, LB_FIRST, NULL );
      continue;
    }
    else if( ret < 0 )
    {
      LE_send_msg( GL_ERROR, "ORPGDA_read Failed (%d)", ret );
      break;
    }
    else
    {
      lb_msg_size = ret;
    }

    Check_LB_out();

    if( ( ret = LB_write( LB_out_id, buf, lb_msg_size, LB_ANY ) ) < 0 )
    {
      LE_send_msg( GL_ERROR, "ORPGDA_write failed (%d)", ret );
    }

    LB_out_num_msgs++;
  }
}

/******************************************************************

   Description: Products DB LB callback.

******************************************************************/

static void LB_callback_fx( int fd, LB_id_t mid, int minfo, void *arg )
{
  LB_updated_flag = YES;
}

/******************************************************************

   Description: Check if output LB is full.

******************************************************************/

static void Check_LB_out()
{
  char work_dir[448];
  char lb_file_name[512];
  LB_attr lb_att;
  int ret = 0;

  if( LB_out_num_LBs != 0 && LB_out_num_msgs != LB_out_max_num_msgs )
  {
    return;
  }

  LE_send_msg( GL_INFO, "New LB out needed" );
  if( LB_out_num_LBs == 0 )
  {
    LE_send_msg( GL_INFO, "LB_out_num_LBs == %d", LB_out_num_LBs );
  }
  else
  {
    LE_send_msg( GL_INFO, "LB_out_num_msgs == %d", LB_out_num_msgs );
  }

  if( (int) LB_out_id > 0 )
  {
    LE_send_msg( GL_INFO, "Closing LB_out_id %d", (int) LB_out_id );
    LB_close( LB_out_id );
  }

  LB_out_num_LBs++;
  LB_out_num_msgs = 0;

  if( ( ret = MISC_get_work_dir( work_dir, 448 ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "MISC_get_work_dir failed (%d)", ret );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }
  LE_send_msg( GL_INFO, "work dir: %s", work_dir );

  sprintf( lb_file_name, "%s/%s.%d.lb", work_dir, LB_out_base_name, LB_out_num_LBs );
  LE_send_msg( GL_INFO, "Creating %s", lb_file_name );

  sprintf( lb_att.remark, "Save products DB LB %d", LB_out_num_LBs );
  lb_att.msg_size = 0;
  lb_att.tag_size = 0;
  lb_att.maxn_msgs = LB_out_max_num_msgs;
  lb_att.types = LB_DB;
  lb_att.mode = 0644;

  if( (int) ( LB_out_id = LB_open( &lb_file_name[0], LB_CREATE, &lb_att ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "Failed (%d) to open :%s:", (int) LB_out_id, lb_file_name );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }
  LE_send_msg( GL_INFO, "New LB out id: %d", (int) LB_out_id );
}

/******************************************************************

   Description: Termination handler.

******************************************************************/

int Termination_handler( int signal, int exit_code )
{

  time_t current_time = time( NULL );

  LE_send_msg( GL_INFO, "Task terminated by signal %d (%s) @ %s",
               signal, ORPGTASK_get_sig_name( signal ),
               asctime( gmtime( &current_time ) ) );
  return 0;
}

