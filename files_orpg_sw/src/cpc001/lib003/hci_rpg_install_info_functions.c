/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:24 $
 * $Id: hci_rpg_install_info_functions.c,v 1.8 2009/02/27 22:26:24 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_rpg_install_info_functions.c			*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      related to values in ~/cfg/rpg_install.info.	*
 *		      This lib ensures all values come from RPGA.	*
 *									*
 ************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_rpg_install_info.h>

/* Macros. */

#define	ADAPT_NOT_LOADED		0
#define	ADAPT_LOADED			1
#define	ADAPT_LOADED_UNKNOWN		-1
#define	DEVICES_NOT_CONFIGURED		0
#define	DEVICES_CONFIGURED		1
#define	DEVICES_CONFIGURED_UNKNOWN	-1

/* Global variables. */

static char	rpg_host[ HCI_BUF_64 ];

/************************************************************************
 *	Description: This function returns values from the rpg install	*
 *		     info file on the RPGA node.			*
 ************************************************************************/

int
hci_get_install_info( char *tag, char *buf, int bufsize )
{
  int status = -1;
  int sys_ret = -1;
  char func[ HCI_BUF_64 ];

  /* RSS_rpc function on RPGA. */

  sprintf( func, "%s:liborpg.so,ORPGMISC_get_install_info", rpg_host );
  status = RSS_rpc( func, "i-r s-i ba-%d-io i",
                    bufsize, &sys_ret, tag, buf, bufsize );

  if( status < 0 )
  {
    HCI_LE_error( "RSS_rpc failed (%d).", status );
    return status;
  }
  else if( sys_ret < 0 )
  {
    HCI_LE_error("ORPGMISC_get_install_info failed (%d).", sys_ret );
    return sys_ret;
  }

  return 0;
}

/************************************************************************
 *	Description: This function sets values in the rpg install info	*
 *		     file on the RPGA node.				*
 ************************************************************************/

int
hci_set_install_info( char *tag, char *value )
{
  int status = -1;
  int sys_ret = -1;
  char func[ HCI_BUF_64 ];

  /* RSS_rpc function on RPGA. */

  sprintf( func, "%s:liborpg.so,ORPGMISC_set_install_info", rpg_host );
  status = RSS_rpc( func, "i-r s-i s-i", &sys_ret, tag, value );
  if( status < 0 )
  {
    HCI_LE_error( "RSS_rpc failed (%d).", status );
    return status;
  }
  else if( sys_ret < 0 )
  {
    HCI_LE_error("ORPGMISC_set_install_info failed (%d).", sys_ret );
    return sys_ret; 
  }

  return 0;
}

/************************************************************************
 *      Description: This function gets the value indicating whether    *
 *                   adaptation data has been loaded.			*
 ************************************************************************/

int
hci_get_install_info_adapt_loaded()
{
  int status = -1;
  int return_code = -1;
  char buf[ HCI_BUF_32 ];
  char temp_buf[ HCI_BUF_128 ];

  /* If this is a non-operational system, assume adaptation
     data has already been loaded. */

  if( ! ORPGMISC_is_operational() )
  {
    sprintf( temp_buf, "Non-operational system. " );
    strcat( temp_buf, "Assuming adaptation data has already been loaded." );
    HCI_LE_log( temp_buf );
    return ADAPT_LOADED;
  }

  status = hci_get_install_info( ADAPT_LOADED_TAG,
                                 buf, HCI_BUF_32 );

  if( status < 0 ){ return_code = ADAPT_LOADED_UNKNOWN; }
  else if( strcmp( buf, "YES" ) == 0 ){ return_code = ADAPT_LOADED; }
  else if( strcmp( buf, "NO" ) == 0 ){ return_code = ADAPT_NOT_LOADED; }
  else{ return_code = ADAPT_LOADED_UNKNOWN; }

  return return_code;
}

/************************************************************************
 *      Description: This function sets the value indicating whether    *
 *                   adaptation data has been loaded.                   *
 ************************************************************************/

int
hci_set_install_info_adapt_loaded()
{
  char temp_buf[ HCI_BUF_128 ];

  /* If this is a non-operational system, there is no
     reason to set any flags. */

  if( ! ORPGMISC_is_operational() )
  {
    sprintf( temp_buf, "Non-operational system. " );
    strcat( temp_buf, "No need to set adaptation data loaded flag." );
    HCI_LE_log( temp_buf );
    return 0;
  }

  return hci_set_install_info( ADAPT_LOADED_TAG, "YES" );
}

/************************************************************************
 *      Description: This function gets the value indicating whether	*
 *                   hardware devices have been configured.		*
 ************************************************************************/

int
hci_get_install_info_dev_configured()
{
  int status = -1;
  int return_code = -1;
  char buf[ HCI_BUF_32 ];
  char temp_buf[ HCI_BUF_128 ];

  /* If this is a non-operational system, assume devices
     have already been configured. */

  if( ! ORPGMISC_is_operational() )
  {
    sprintf( temp_buf, "Non-operational system. " );
    strcat( temp_buf, "Assuming hardware devices have been configured." );
    HCI_LE_log( temp_buf );
    return DEVICES_CONFIGURED;
  }

  status = hci_get_install_info( DEV_CONFIGURED_TAG,
                                 buf, HCI_BUF_32 );

  if( status < 0 ){ return_code = DEVICES_CONFIGURED_UNKNOWN; }
  else if( strcmp( buf, "YES" ) == 0 ){ return_code = DEVICES_CONFIGURED; }
  else if( strcmp( buf, "NO" ) == 0 ){ return_code = DEVICES_NOT_CONFIGURED; }
  else{ return_code = DEVICES_CONFIGURED_UNKNOWN; }

  return return_code;
}

/************************************************************************
 *      Description: This function sets the value indicating whether    *
 *                   hardware devices have been configured.		*
 ************************************************************************/

int
hci_set_install_info_dev_configured()
{
  char temp_buf[ HCI_BUF_128 ];

  /* If this is a non-operational system, there is no
     reason to set any flags. */

  if( ! ORPGMISC_is_operational() )
  {
    sprintf( temp_buf, "Non-operational system. " );
    strcat( temp_buf, "No need to set devices configured flag." );
    HCI_LE_log( temp_buf );
    return 0;
  }

  return hci_set_install_info( DEV_CONFIGURED_TAG, "YES" );
}

/************************************************************************
 *	Description: This function sets the rpg_host variable.		*
 ************************************************************************/

int
hci_install_info_set_rpg_host( int argc, char **argv )
{
  int status = -1;
  int i = -1;
  int found_token = -1;
  int channel_number = -1;
  char temp_buf[ HCI_BUF_128 ];
  char *host_buf = NULL;
  char *channel_buf = NULL;

  /* If this is a non-operational system, assume rpg host is local. */

  if( ! ORPGMISC_is_operational() )
  {
    sprintf( temp_buf, "Non-operational system. " );
    strcat( temp_buf, "Assuming rpg host is local host." );
    HCI_LE_log( temp_buf );
    sprintf( rpg_host, "%s", "" );
    return 0;
  }

  /* Determine node this is running on. */

  if( ( host_buf = ORPGMISC_get_site_name( "type" ) ) == NULL )
  {
    HCI_LE_error("ORPGMISC_get_site_name(\"type\") is NULL" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else if( strcmp( host_buf, "rpga" ) == 0 || strcmp( host_buf, "rpgb" ) == 0 )
  {
    /* Determine channel from site info on RPGA and RPGB. */

    if( ( channel_buf = ORPGMISC_get_site_name( "channel_num" ) ) == NULL )
    {
      HCI_LE_error("ORPGMISC_get_site_name(\"channel_num\") is NULL" );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
    else if( strcmp( channel_buf, "1" ) == 0 )
    {
      channel_number = 1;
    }
    else if( strcmp( channel_buf, "2" ) == 0 )
    {
      channel_number = 2;
    }
    else
    {
      HCI_LE_error("ORPGMISC_get_site_name(\"channel_num\") is invalid" );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
  }
  else if( strcmp( host_buf, "mscf" ) == 0 )
  {
    /* This is an MSCF. Get channel info by parsing command line
       for "-A" option. If option not found, return error. */

    found_token = 0;
    for( i = 0; i < argc - 1; i++ )
    {
      /* 'A' is channel number. */
      if( strcmp( argv[ i ], "-A" ) == 0 )
      {
        found_token = 1;
        sscanf( argv[ i + 1 ], "%d", &channel_number ); 
        /* Sanity check. */
        if( channel_number != 0 &&
            channel_number != 1 &&
            channel_number != 2 )
        {
          return -1;
        }
      }
    }
    if( !found_token )
    {
      /* Invalid TYPE. */
      HCI_LE_error( "Unable to find -A command-line option " );
      return -1;
    }
  }
  else
  {
    HCI_LE_error( "ORPGMISC_get_site_name(\"type\") is invalid" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* We have channel number, so use ORPGMGR_discover_host_ip to
     set rpg_host to IP of RPGA to query. */

  status = ORPGMGR_discover_host_ip( "rpga", channel_number,
                                  rpg_host, HCI_BUF_64 );

  if( status < 0 )
  {
    /* Unable to determine rpg_host. */
    HCI_LE_error( "ORPGMGR_discover_host_ip failed (%d).", status );
    return status;
  }
  else if( status == 0 )
  {
    /* RPGA node not found. */
    HCI_LE_error(" ORPGMGR_discover_host_ip() Unable to find RPGA."  );
    return status;
  }

  return 0;
}

