/************************************************************************

      The main source file for wideband_agent.

************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/05 16:20:52 $
 * $Id: wideband_agent.c,v 1.4 2014/12/05 16:20:52 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* Include files. */

#include <orpg.h>

/* Enumerations. */

enum { WB_PRIMARY_UNKNOWN, WB_PRIMARY_UP, WB_PRIMARY_DOWN };
enum { NO, YES };

/* Macros. */

#define NONOP_DEA_NAME		"alg.Nonoperational"
#define WB_PRIM_POLL_DEA_NAME	"alg.Nonoperational.wb_prim_poll"
#define WB_PRIM_STBL_DEA_NAME	"alg.Nonoperational.wb_prim_stbl"
#define WB_PRIM_MON_DEA_NAME	"alg.Nonoperational.wb_prim_monitor"
#define	NWS_ROUTER_OCTET	131
#define	NWSR_ROUTER_OCTET	231
#define	NO_ROUTER_OCTET		-1
#define	MAX_TEXT_OUTPUT_LEN	2048
#define	MAX_CMD_LEN		256
#define	NUM_LOG_MSGS		500

/* Static variables. */

static int Debug_flag = NO;
static int Loop_flag = YES;
static int Monitor_flag = NO;
static int Site_subnet = -1;
static int Router_octet = NO_ROUTER_OCTET;
static int Polling_interval = -1;
static int Polling_interval_min = -1;
static int Polling_interval_max = -1;
static int Stability_interval = -1;
static int Stability_interval_max = -1;
static int Stability_interval_min = -1;
static int WB_adapt_updated_flag = NO;

/* Prototypes */

static int  Init_system_info();
static int  Init_adapt_params();
static void Read_adapt_params();
static int  Get_wb_primary_status();
static int  Cleanup_fxn( int, int );
static int  Read_options( int, char ** );
static int  Get_prim_monitor();
static int  Get_prim_poll();
static int  Get_prim_poll_min();
static int  Get_prim_poll_max();
static int  Get_prim_stbl();
static int  Get_prim_stbl_min();
static int  Get_prim_stbl_max();
static void WB_adapt_cb( int, LB_id_t, int, void * );
static void Pdebug( const char *, ... );

/**************************************************************************

    The main function.

**************************************************************************/

int main( int argc, char **argv )
{
  int ret = 0;
  unsigned char flag = 0;
  int stbl_timer = 0;
  int stbl_timer_active = NO;
  int prev_stat = WB_PRIMARY_UNKNOWN;
  int curr_stat = WB_PRIMARY_UNKNOWN;
  int last_known_stat = WB_PRIMARY_UNKNOWN;

  /* Initialize logging service. */
  if( ( ret = ORPGMISC_init( argc, argv, NUM_LOG_MSGS, 0, -1, 0 ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGMISC_init Failed (%d)", ret );
    Pdebug( "Exit with failure" );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }
  /* Initialize system information. */
  else if( Init_system_info() < 0 )
  {
    Pdebug( "Exit with failure" );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }
  /* Initialize adaptable parameter values. */
  else if( Init_adapt_params() < 0 )
  {
    Pdebug( "Exit with failure" );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }
  /* Read command line options. Call after initializing adaptable
     parameters because they are used for error checking */
  else if( Read_options( argc, argv ) < 0 )
  {
    Pdebug( "Exit with failure" );
    ORPGTASK_exit( GL_EXIT_FAILURE );
  }

  Pdebug( "Monitor flag           = %d", Monitor_flag );
  Pdebug( "Router octet           = %d", Router_octet );
  Pdebug( "Site subnet            = %d", Site_subnet );
  Pdebug( "Polling interval       = %d", Polling_interval );
  Pdebug( "Polling interval min   = %d", Polling_interval_min );
  Pdebug( "Polling interval max   = %d", Polling_interval_max );
  Pdebug( "Stability interval     = %d", Stability_interval );
  Pdebug( "Stability interval min = %d", Stability_interval_min );
  Pdebug( "Stability interval max = %d", Stability_interval_max );

  /* Register termination handler. */
  Pdebug( "Call ORPGTASK_reg_term_handler( Cleanup_fxn )" );
  ORPGTASK_reg_term_handler( Cleanup_fxn );

  /* Tell RPG manager task is ready. */
  Pdebug( "Call ORPGMGR_ready_for_operation()" );
  ORPGMGR_report_ready_for_operation();

  /* Loop forever. */
  while( Loop_flag )
  {
    Pdebug( "Loop iteration" );
    if( Router_octet != NO_ROUTER_OCTET && Monitor_flag )
    {
      prev_stat = last_known_stat;
      Pdebug( "prev_stat = %d", prev_stat );

      if( ( curr_stat = Get_wb_primary_status() ) != WB_PRIMARY_UNKNOWN )
      {
        if( prev_stat == WB_PRIMARY_DOWN )
        {
          /* If status changed from down to up, then start
             stability counter. Otherwise, don't do anything. */
          if( curr_stat == WB_PRIMARY_UP )
          {
            Pdebug( "primary status went from DOWN to UP" );
            Pdebug( "Activate stability timer" );
            stbl_timer_active = YES;
            stbl_timer = 0;
          }
          else
          {
            Pdebug( "primary status remained DOWN" );
          }
        }
        else if( prev_stat == WB_PRIMARY_UP )
        {
          /* If status changed from up to down, then throw alarm. */
          if( curr_stat == WB_PRIMARY_DOWN )
          {
            Pdebug( "primary status went from UP to DOWN" );
            Pdebug( "Activate alarm and deactivate stability timer" );
            stbl_timer_active = NO;
            LE_send_msg( GL_STATUS | LE_RPG_AL_MAR, "Wideband Link ---> Backup Communications\n" );
            ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_RDAWB,
                                        ORPGINFO_STATEFL_SET, &flag );
          }
          else if( stbl_timer_active )
          {
            Pdebug( "primary status remained UP with activated timer" );
            stbl_timer += Polling_interval;
            Pdebug( "stability timer now: %d (clear at %d)", stbl_timer, Stability_interval  );
            if( stbl_timer > Stability_interval )
            {
              Pdebug( "Primary status stable so clear alarm." );
              if( ( ret = ORPGINFO_statefl_rpg_alarm(
                             ORPGINFO_STATEFL_RPGALRM_RDAWB,
                             ORPGINFO_STATEFL_CLR, &flag ) ) < 0 )
              {
                /* Unable to clear alarm. The stability timer is not
                   deactivated so as to keep trying until alarm clears. */
                LE_send_msg( GL_INFO, "Alarm clearing failed (%d)", ret );
              }
              else
              {
                /* Alarm successfully cleared. Deactivate stability
                   timer to keep from endlessly incrementing it. */
                Pdebug( "Alarm cleared. Deactivating stability timer." );
                stbl_timer_active = NO;
              }
            }
          }
          else
          {
            Pdebug( "primary status remained UP" );
          }
        }

        last_known_stat = curr_stat;
        Pdebug( "last_known_stat = %d", last_known_stat );
      }
      else
      {
        LE_send_msg( GL_INFO, "WB status unknown. Skipping interval." );
      }
    }

    if( WB_adapt_updated_flag == YES )
    {
      Pdebug( "WB adaptation data updated. Read new values.)" );
      WB_adapt_updated_flag = NO;
      Read_adapt_params();
    }

    Pdebug( "msleep( %d * 1000 )", Polling_interval );
    msleep( Polling_interval * 1000 );
  }

  exit( 0 );
}

/**********************************************************************
    
    Get system information.
  
**********************************************************************/

static int Read_options( int argc, char **argv )
{
  extern char *optarg;  /* used by getopt */
  int c = 0;            /* used by getopt */
  int help_flag = NO;    /* user wants to print usage */

  Pdebug( "In Read_options." );

  while( ( c = getopt( argc, argv, "hp:s:x") ) != EOF )
  {
    Pdebug( "c = %c", c );
    switch( c )
    {
      /* Change the polling interval. */
      case 'p':
        Pdebug( "case p" );
        Pdebug( "optarg:%s:", optarg );
        Polling_interval = atoi( optarg );
        Pdebug( "User defined Polling interval = %d", Stability_interval );
        if( Polling_interval < Polling_interval_min ||
            Polling_interval > Polling_interval_max )
        {
          printf( "-p arg must be between %d and %d\n",
                  Polling_interval_min, Polling_interval_max );
          Pdebug( "Read_options returns -1" );
          return -1;
        }
        break;

      /* Change the stability interval. */
      case 's':
        Pdebug( "case s" );
        Pdebug( "optarg:%s:", optarg );
        Stability_interval = atoi( optarg );
        Pdebug( "Usser defined Stability interval = %d", Stability_interval );
        if( Stability_interval < Stability_interval_min ||
            Stability_interval > Stability_interval_max )
        {
          printf( "-s arg must be between %d and %d\n",
                   Stability_interval_min, Stability_interval_max );
          Pdebug( "Read_options returns -1" );
          return -1;
        }
        break;

      /* Turn debug on. */
      case 'x':
        Pdebug( "case x" );
        Debug_flag = YES;
        Pdebug( "Setting Debug_flag = %d", Debug_flag );
        break;

      /* Print usage, or an error was encountered. */
      case 'h':
        Pdebug( "case h" );
        Pdebug( "print usage" );
        help_flag = YES;
      case '?':
        Pdebug( "case ?" );
        Pdebug( "print usage" );
        printf( "Usage: %s [options]\n", argv[0] );
        printf( "       Options:\n" );
        printf( "       -p int - polling interval (secs)\n" );
        printf( "       -s int - stability interval (secs)\n" );
        printf( "       -x     - debugging\n" );
        printf( "       -h     - usage\n" );
        printf( "\n" );
        if( help_flag )
        {
          exit( 0 );
        }
        else
        {
          Pdebug( "Read_options returns -1" );
          return -1;
        }
        break;
    }
  }

  Pdebug( "Read_options returns 0" );

  return 0;
}

/**********************************************************************
    
    Initialize system information.
  
**********************************************************************/

static int Init_system_info()
{
  char *system_buf = NULL;
  char cmd[ MAX_CMD_LEN ];
  char obuf[ MAX_TEXT_OUTPUT_LEN ];
  int n_bytes = 0;
  int ret = 0;

  Pdebug( "In Init_system_info" );

  if( ORPGMISC_is_operational() )
  {
    Pdebug( "Operational system - Proceed" );
    /* Determine system this task is running on. */
    if( ( system_buf = ORPGMISC_get_site_name( "system" ) ) == NULL )
    {
      LE_send_msg( GL_INFO, "ORPGMISC_get_site_name() is NULL" );
      Pdebug( "Init_system_info returns -1" );
      return -1;
    }
    else if( strcmp( system_buf, "NWS" ) == 0 )
    {
      Pdebug( "system_buf = %s", system_buf );
      Pdebug( "Init_system_info returns %d", NWS_ROUTER_OCTET );
      Router_octet = NWS_ROUTER_OCTET;
    }
    else if( strcmp( system_buf, "NWSR" ) == 0 )
    {
      Pdebug( "system_buf = %s", system_buf );
      Pdebug( "Init_system_info returns %d", NWSR_ROUTER_OCTET );
      Router_octet = NWSR_ROUTER_OCTET;
    }
    else if( strcmp( system_buf, "FAA" ) == 0 )
    {
      Pdebug( "system_buf = %s", system_buf );
      Pdebug( "Init_system_info returns NO ROUTER OCTET" );
      Router_octet = NO_ROUTER_OCTET;
    }
    else if( strcmp( system_buf, "DODFR" ) == 0 )
    {
      Pdebug( "system_buf = %s", system_buf );
      Pdebug( "Init_system_info returns NO ROUTER OCTET" );
      Router_octet = NO_ROUTER_OCTET;
    }
    else
    {
      Pdebug( "system_buf = %s", system_buf );
      LE_send_msg( GL_INFO, "ORPGMISC_get_site_name() is invalid: %s", system_buf );
      return -1;
      Pdebug( "Init_system_info returns -1" );
    }

    /* Command to get router subnet ID. */
    strcpy( cmd, "find_adapt -V SUBNET_ID" );
    Pdebug( "CMD: %s", cmd );

    /* Run command. */
    ret = MISC_system_to_buffer( cmd, obuf, MAX_TEXT_OUTPUT_LEN, &n_bytes );
    ret = ret >> 8;
    Pdebug( "ret: %d  len: %d n_bytes: %d", ret, strlen( obuf ), n_bytes );

    if( ret != 0 )
    {
      LE_send_msg( GL_INFO, "Command: %s failed (%d)", cmd, ret );
      Pdebug( "Init_system_info returns -1" );
      return -1;
    }
    else if( strlen( obuf ) == 0 )
    {
      LE_send_msg( GL_INFO, "Command: %s output of size 0", cmd );
      Pdebug( "Init_system_info returns -1" );
      return -1;
    }
    else if( n_bytes == 0 )
    {
      LE_send_msg( GL_INFO, "Command: %s n_bytes = 0", cmd );
      Pdebug( "Init_system_info returns -1" );
      return -1;
    }
    else
    {
      Pdebug( "obuf: %s", obuf );
      Site_subnet = atoi( obuf );
    }
  }
  else
  {
    LE_send_msg( GL_INFO, "Non-operational: will skip polling" );
  }

  Pdebug( "Init_system_info returns 0" );

  return 0;
}

/**********************************************************************
    
    Initialize adaptable parameter values.
  
**********************************************************************/

static int Init_adapt_params()
{
  int ret = 0;

  Pdebug( "In Init_adapt_params" );

  /* Register for changes to needed adaptable parameters. */
  if( ( ret = DEAU_UN_register( NONOP_DEA_NAME, (void *) WB_adapt_cb ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "DEAU_UN_register %s failed (%d)", NONOP_DEA_NAME, ret );
    return ret;
  }
  else if( ( Monitor_flag = Get_prim_monitor() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Monitor_flag );
    return Monitor_flag;
  }
  else if( ( Polling_interval = Get_prim_poll() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Polling_interval );
    return Polling_interval;
  }
  else if( ( Polling_interval_min = Get_prim_poll_min() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Polling_interval_min );
    return Polling_interval_min;
  }
  else if( ( Polling_interval_max = Get_prim_poll_max() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Polling_interval_max );
    return Polling_interval_max;
  }
  else if( ( Stability_interval = Get_prim_stbl() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Stability_interval );
    return Stability_interval;
  }
  else if( ( Stability_interval_min = Get_prim_stbl_min() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Stability_interval_min );
    return Stability_interval_min;
  }
  else if( ( Stability_interval_max = Get_prim_stbl_max() ) < 0 )
  {
    Pdebug( "Init_adapt_params returns %d", Stability_interval_max );
    return Stability_interval_max;
  }

  Pdebug( "Init_adapt_params returns 0" );

  return 0;
}

/**********************************************************************
    
    Read updated adaptable parameter values.
  
**********************************************************************/

static void Read_adapt_params()
{
  int temp = 0;

  Pdebug( "In Read_adapt_params" );

  /* Only update for a valid value. */

  if( ( temp = Get_prim_monitor() ) < 0 )
  {
    Pdebug( "temp = %d", temp );
    LE_send_msg( GL_INFO, "Unable to update Monitor flag" );
  }
  else
  {
    Monitor_flag = temp;
    LE_send_msg( GL_INFO, "Updating Monitor flag to %d", temp );
  }

  if( ( temp = Get_prim_poll() ) > 0 )
  {
    Polling_interval = temp;
    LE_send_msg( GL_INFO, "Updating Polling interval to %d", temp );
  }
  else
  {
    Pdebug( "temp = %d", temp );
    LE_send_msg( GL_INFO, "Unable to update Polling interval" );
  }

  if( ( temp = Get_prim_stbl() ) > 0 )
  {
    Stability_interval = temp;
    LE_send_msg( GL_INFO, "Updating Stability interval to %d", temp );
  }
  else
  {
    Pdebug( "temp = %d", temp );
    LE_send_msg( GL_INFO, "Unable to update Stability interval" );
  }

  Pdebug( "Leaving Read_adapt_params" );

  return;
}
/**********************************************************************
    
    Get wideband primary comms status.
  
**********************************************************************/

static int Get_wb_primary_status()
{
  int ret = -1;
  char cmd[ MAX_CMD_LEN ];
  char obuf[ MAX_TEXT_OUTPUT_LEN ];
  int n_bytes = -1;

  Pdebug( "In Get_wb_primary_status" );

  /* SNMP command to run. */
  sprintf( cmd, "snmpget -v2c -c npios rtr ipCidrRouteStatus.172.25.%d.%d.255.255.255.255.0.0.0.0.0", Site_subnet, Router_octet );
  Pdebug( "CMD: %s", cmd );

  /* Run SNMP command. */
  ret = MISC_system_to_buffer( cmd, obuf,
                               MAX_TEXT_OUTPUT_LEN, &n_bytes );
  ret = ret >> 8;
  Pdebug( "ret: %d  len: %d n_bytes: %d", ret, strlen( obuf ), n_bytes );

  /* Check for errors. */
  if( ret != 0 )
  {
    LE_send_msg( GL_INFO, "Command: %s failed (%d)", cmd, ret );
    Pdebug( "Get_wb_primary_status returns WB primary comms UNKNOWN" );
    return WB_PRIMARY_UNKNOWN;
  }
  else if( strlen( obuf ) == 0 )
  {
    LE_send_msg( GL_INFO, "Command: %s output of size 0", cmd );
    Pdebug( "Get_wb_primary_status returns WB primary comms UNKNOWN" );
    return WB_PRIMARY_UNKNOWN;
  }
  else if( n_bytes == 0 )
  {
    LE_send_msg( GL_INFO, "Command: %s n_bytes = 0", cmd );
    Pdebug( "Get_wb_primary_status returns WB primary comms UNKNOWN" );
    return WB_PRIMARY_UNKNOWN;
  }

  Pdebug( "obuf: %s", obuf );

  /* Look for string indicating primary comms is down. */
  if( strstr( obuf, "No Such Instance" ) != NULL )
  {
    Pdebug( "String No Such Instance found" );
    Pdebug( "Get_wb_primary_status returns WB primary comms DOWN" );
    return WB_PRIMARY_DOWN;
  }

  Pdebug( "Get_wb_primary_status returns WB primary comms UP" );

  return WB_PRIMARY_UP;
}

/**********************************************************************
    
    Get update to Wideband/primary adaptable parameters..
  
**********************************************************************/

void WB_adapt_cb( int fd, LB_id_t msgid, int msg_info, void *arg )
{
  Pdebug( "In WB_adapt_cb" );
  LE_send_msg( GL_INFO, "%s updated", NONOP_DEA_NAME );
  WB_adapt_updated_flag = YES;
  Pdebug( "Leaving WB_adapt_cb" );
}

/**********************************************************************
    
    Cleanup task when signaled by RPG to quit.
  
**********************************************************************/

static int Cleanup_fxn( int signal, int status )
{
  Pdebug( "In Cleanup_fxn" );
  LE_send_msg( GL_INFO, "Signal %d Received", signal );
  Loop_flag = NO;
  Pdebug( "Cleanup_fxn returns 0" );
  return 0;
}

/**********************************************************************
    
    Get Wideband prim comms monitoring flag.
  
**********************************************************************/

static int Get_prim_monitor()
{
  int ret = 0;
  char *p = NULL;

  Pdebug( "In Get_prim_monitor" );

  if( ( ret = DEAU_get_string_values( WB_PRIM_MON_DEA_NAME, &p ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "DEAU_get_string_values %s failed (%d)", WB_PRIM_MON_DEA_NAME, ret );
  }
  else
  {
    Pdebug( "p = %s", *p );
    if( strcmp( p, "Yes" ) == 0 ){ ret = YES; }
    else{ ret = NO; }
  }

  Pdebug( "Get_prim_monitor returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Get Wideband prim comms polling time parameter.
  
**********************************************************************/

static int Get_prim_poll()
{
  int ret = 0;
  double get_val = 0.0;

  Pdebug( "In Get_prim_poll" );

  if( ( ret = DEAU_get_values( WB_PRIM_POLL_DEA_NAME, &get_val, sizeof( double * ) ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "DEAU_get_values %s failed (%d)", WB_PRIM_POLL_DEA_NAME, ret );
  }
  else
  {
    Pdebug( "get_val %f", get_val );
    ret = (int) get_val;
  }

  Pdebug( "Get_prim_poll returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Get Wideband primary comms polling time parameter minimum.
  
**********************************************************************/

static int Get_prim_poll_min()
{
  int ret = 0;
  DEAU_attr_t *d_attr = NULL;
  static char *p;

  Pdebug( "In Get_prim_poll_min" );

  /* Get DEAU attributes so can pull out the min value. */
  if( ( ret = DEAU_get_attr_by_id( WB_PRIM_POLL_DEA_NAME, &d_attr ) ) < 0 )
  {
    /* DEAU_get_attr_by_id returned an error */
    LE_send_msg( GL_INFO, "DEAU_get_attr_by_id %s failed (%d)", WB_PRIM_POLL_DEA_NAME, ret );
  }
  else if( d_attr == NULL )
  {
    /* DEAU attributes data was NULL. */
    LE_send_msg( GL_INFO, "%s: d_attr == NULL", WB_PRIM_POLL_DEA_NAME );
    ret = -1;
  }
  else
  {
    /* Get string containing min and max. */
    if( ( ret = DEAU_get_min_max_values( d_attr, &p ) ) < 0 )
    {
      /* DEAU_get_min_max_values returned an error. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s failed (%d)", WB_PRIM_POLL_DEA_NAME, ret );
    }
    else if( p == NULL )
    {
      /* Retuned string was NULL. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s p == NULL", WB_PRIM_POLL_DEA_NAME );
      ret = -1;
    }
    else
    {
      /* String is of format min\0max\0. */
      Pdebug( "p = %s", *p );
      ret = atoi( p );
    }
  }

  Pdebug( "Get_prim_poll_min returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Get Wideband primary comms polling time parameter maximum.
  
**********************************************************************/

static int Get_prim_poll_max()
{
  int ret = 0;
  DEAU_attr_t *d_attr = NULL;
  static char *p;

  Pdebug( "In Get_prim_poll_max" );

  /* Get DEAU attributes so can pull out the max value. */
  if( ( ret = DEAU_get_attr_by_id( WB_PRIM_POLL_DEA_NAME, &d_attr ) ) < 0 )
  {
    /* DEAU_get_attr_by_id returned an error */
    LE_send_msg( GL_INFO, "DEAU_get_attr_by_id %s failed (%d)", WB_PRIM_POLL_DEA_NAME, ret );
  }
  else if( d_attr == NULL )
  {
    /* DEAU attributes data was NULL. */
    LE_send_msg( GL_INFO, "%s: d_attr == NULL", WB_PRIM_POLL_DEA_NAME );
    ret = -1;
  }
  else
  {
    /* Get string containing min and max. */
    if( ( ret = DEAU_get_min_max_values( d_attr, &p ) ) < 0 )
    {
      /* DEAU_get_min_max_values returned an error. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s failed (%d)", WB_PRIM_POLL_DEA_NAME, ret );
    }
    else if( p == NULL )
    {
      /* Retuned string was NULL. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s p == NULL", WB_PRIM_POLL_DEA_NAME );
      ret = -1;
    }
    else
    {
      /* String is of format min\0max\0. */
      Pdebug( "p = %s", *p );
      ret = atoi( p + strlen( p ) + 1 );
    }
  }

  Pdebug( "Get_prim_poll_max returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Get Wideband primary comms stability time parameter.
  
**********************************************************************/

static int Get_prim_stbl()
{
  int ret = 0;
  double get_val = 0.0;

  Pdebug( "In Get_prim_stbl" );

  if( ( ret = DEAU_get_values( WB_PRIM_STBL_DEA_NAME, &get_val, sizeof( double * ) ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "DEAU_get_values %s failed (%d)", WB_PRIM_STBL_DEA_NAME, ret );
  }
  else
  {
    Pdebug( "get_val %f", get_val );
    ret = (int) get_val;
  }

  Pdebug( "Get_prim_stbl returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Get Wideband primary comms stability time parameter minimum.
  
**********************************************************************/

static int Get_prim_stbl_min()
{
  int ret = 0;
  DEAU_attr_t *d_attr = NULL;
  static char *p;

  Pdebug( "In Get_prim_stbl_min" );

  /* Get DEAU attributes so can pull out the min value. */
  if( ( ret = DEAU_get_attr_by_id( WB_PRIM_STBL_DEA_NAME, &d_attr ) ) < 0 )
  {
    /* DEAU_get_attr_by_id returned an error */
    LE_send_msg( GL_INFO, "DEAU_get_attr_by_id %s failed (%d)", WB_PRIM_STBL_DEA_NAME, ret );
  }
  else if( d_attr == NULL )
  {
    /* DEAU attributes data was NULL. */
    LE_send_msg( GL_INFO, "%s: d_attr == NULL", WB_PRIM_STBL_DEA_NAME );
    ret = -1;
  }
  else
  {
    /* Get string containing min and max. */
    if( ( ret = DEAU_get_min_max_values( d_attr, &p ) ) < 0 )
    {
      /* DEAU_get_min_max_values returned an error. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s failed (%d)", WB_PRIM_STBL_DEA_NAME, ret );
    }
    else if( p == NULL )
    {
      /* Retuned string was NULL. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s p == NULL", WB_PRIM_STBL_DEA_NAME );
      ret = -1;
    }
    else
    {
      /* String is of format min\0max\0. */
      Pdebug( "p = %s", *p );
      ret = atoi( p );
    }
  }

  Pdebug( "Get_prim_stbl_min returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Get Wideband primary comms stability time parameter maximum.
  
**********************************************************************/

static int Get_prim_stbl_max()
{
  int ret = 0;
  DEAU_attr_t *d_attr = NULL;
  static char *p;

  Pdebug( "In Get_prim_stbl_max" );

  /* Get DEAU attributes so can pull out the max value. */
  if( ( ret = DEAU_get_attr_by_id( WB_PRIM_STBL_DEA_NAME, &d_attr ) ) < 0 )
  {
    /* DEAU_get_attr_by_id returned an error */
    LE_send_msg( GL_INFO, "DEAU_get_attr_by_id %s failed (%d)", WB_PRIM_STBL_DEA_NAME, ret );
  }
  else if( d_attr == NULL )
  {
    /* DEAU attributes data was NULL. */
    LE_send_msg( GL_INFO, "%s: d_attr == NULL", WB_PRIM_STBL_DEA_NAME );
    ret = -1;
  }
  else
  {
    /* Get string containing min and max. */
    if( ( ret = DEAU_get_min_max_values( d_attr, &p ) ) < 0 )
    {
      /* DEAU_get_min_max_values returned an error. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s failed (%d)", WB_PRIM_STBL_DEA_NAME, ret );
    }
    else if( p == NULL )
    {
      /* Retuned string was NULL. */
      LE_send_msg( GL_INFO, "DEAU_get_min_max_values() %s p == NULL", WB_PRIM_STBL_DEA_NAME );
      ret = -1;
    }
    else
    {
      /* String is of format min\0max\0. */
      Pdebug( "p = %s", *p );
      ret = atoi( p + strlen( p ) + 1 );
    }
  }

  Pdebug( "Get_prim_stbl_max returns %d", ret );

  return ret;
}

/**********************************************************************
    
    Print debug messages.
  
**********************************************************************/

static void Pdebug( const char *format, ... )
{
  char *buf = NULL;
  va_list arg_ptr;

  if( Debug_flag )
  {
    /* Extract print format. */
    buf = malloc( MAX_TEXT_OUTPUT_LEN );
    va_start( arg_ptr, format );
    vsprintf( buf, format, arg_ptr );
    va_end( arg_ptr );
    LE_send_msg( GL_INFO, "DEBUG: %s", buf );
    free( buf );
  }
}

