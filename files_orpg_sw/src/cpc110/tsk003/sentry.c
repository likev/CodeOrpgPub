/************************************************************************

      The main source file for MSCF Power Control HCI.

************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 18:43:58 $
 * $Id: sentry.c,v 1.2 2013/07/22 18:43:58 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* RPG include files. */

#include <hci.h>
#include <mscf_power_control.h>

/* Macros. */
enum { CMD_OUTLET_ON = 1, CMD_OUTLET_OFF = 2, CMD_OUTLET_REBOOT = 3 }; 

/* Global variables. */
extern int      Verbose;
extern char	Pv_cmd[MAX_CMD_LEN];
extern char	Pc_addr[MAX_ADDRESS_LEN];
extern char	*Current_text_pointer;
extern char	Pc_cmd[MAX_CMD_LEN];
extern char	Pc_ret_strs[NUM_PWRCTRL_OPTIONS][MAX_STRING_LEN];
extern char	Pc_key[MAX_CMD_LEN];
extern char	Pn_cmd[MAX_CMD_LEN];
extern char	Pn_key[MAX_CMD_LEN];
extern char	Ps_cmd[MAX_CMD_LEN];
extern char	Ps_ret_strs[NUM_PWRCTRL_STATES][MAX_STRING_LEN];
extern char	Ps_key[MAX_CMD_LEN];
extern int	Num_outlets;
extern Power_status_t  Outlet_status[MAX_NUM_OUTLETS];
extern int      Power_control_flag;
extern int      Channel_number;
extern int      System_flag;


/*  Function prototypes. */

/***************************************************************************

 Description: Get names of outlets.

 For Sentry power administrator.

 ***************************************************************************/
void Sentry_get_outlet_names()
{
  char cmd[MAX_CMD_LEN];      /* SNMP command */
  char *res;                  /* result */
  char *sub;                  /* substring */
  int ind;                    /* device index */
  int s_ind;                  /* index of switch */
  int o_ind;                  /* index of outlet */
  char *label;
  int len;

  sprintf( cmd, Pn_cmd, Pc_addr );

  if( Verbose )
    LE_send_msg( GL_INFO, "Get_outlet_names-->Send_snmp_command(  %s )\n", cmd );
  Send_snmp_command( cmd );

  Num_outlets = 0;

  while( ( ind = Sentry_get_next_result( Pn_key, &res, &s_ind, &o_ind ) ) > 0 )
  {
    if( Num_outlets >= MAX_NUM_OUTLETS )
    {
      LE_send_msg( GL_ERROR, "Outlet %d is more than max",
                   Num_outlets, MAX_NUM_OUTLETS );
      break;
    }

    if( ind != Num_outlets + 1 )
    {
      LE_send_msg( GL_ERROR, "PC line name out of order" );
      break;
    }

    if( Verbose )
      LE_send_msg( GL_INFO, "Get_outlet_names-->Pn_key: %s, Result: %s, switch: %d, outlet: %d\n",
                   Pn_key, res, s_ind, o_ind );

    /* Set switch/outlet index for device's struct */

    Outlet_status[Num_outlets].active = PC_ACTIVE_YES;
    Outlet_status[Num_outlets].switch_index = s_ind;
    Outlet_status[Num_outlets].outlet_index = o_ind;
    XtSetSensitive( Outlet_status[Num_outlets].icon, True );

    /* Set label for device */

    label = Outlet_status[Num_outlets].label;
    strncpy( label, res, MAX_LABEL_LEN );
    label[MAX_LABEL_LEN] = '\0';

    /* If FAA configuration, labels with contain channel information
       in the form:  outlet_name-CHX where X is the channel number.  
       Replace the "-" with a line feed. */
    len = strlen( label );
    if( (System_flag == HCI_FAA_SYSTEM) 
                     &&
        (sub = strstr( label, "-CH" )) != NULL )
        sub[0] = '\n';

    if( Verbose ){

      LE_send_msg( GL_INFO, "Get_outlet_names-->Outlet_status[%d]\n", Num_outlets+1 );
      LE_send_msg( GL_INFO, "                   Outlet_status.active:       %d\n", 
                   Outlet_status[Num_outlets].active );
      LE_send_msg( GL_INFO, "                   Outlet_status.switch_index: %d\n", 
                   Outlet_status[Num_outlets].switch_index );
      LE_send_msg( GL_INFO, "                   Outlet_status.outlet_index: %d\n", 
                   Outlet_status[Num_outlets].outlet_index );
      LE_send_msg( GL_INFO, "                   Outlet_status.label       : %s\n", 
                   Outlet_status[Num_outlets].label );
    }
    Num_outlets++;
  }
}

/***************************************************************************
 Description: Get status of outlets.
***************************************************************************/
void Sentry_get_outlet_status()
{
  char cmd[MAX_CMD_LEN];      /* SNMP command */
  char *res;                  /* result */
  int ind;                    /* device index */
  int i;                      /* looping variable */
  int s_ind;                  /* index of switch */
  int o_ind;                  /* index of outlet */

  if( Num_outlets == 0 ){ return; }

  /* Get Power Control status of each device. */

  sprintf( cmd, Ps_cmd, Pc_addr );
  if( Verbose )
    LE_send_msg( GL_INFO, "Get_outlet_status-->Send_snmp_command( %s )\n", cmd );

  Send_snmp_command( cmd );

  for( i = 0; i < Num_outlets; i++ )
  {
    ind = Sentry_get_next_result( Ps_key, &res, &s_ind, &o_ind );
    if( ind != i + 1 )
    {
      LE_send_msg (GL_ERROR, "PC outlet %d status not found", i);
      break;
    }

    if( strstr( res, Ps_ret_strs[PC_ON_STATE]) != NULL )
    {
      Outlet_status[i].on = PC_ON_STATE;
    }
    else if( strstr( res, Ps_ret_strs[PC_OFF_STATE] ) != NULL )
    {
      Outlet_status[i].on = PC_OFF_STATE;
    }
    else
    {
      LE_send_msg (GL_ERROR, "Get_outlet_status-->status %s (%d) unexpected", res, strlen(res) );
      if( Verbose ){
        LE_send_msg (GL_INFO, "--->Expected States (on): %s (%d)\n", 
                     Ps_ret_strs[PC_ON_STATE], strlen(Ps_ret_strs[PC_ON_STATE]) );
        LE_send_msg (GL_INFO, "--->Expected States (off): %s (%d)\n", 
                     Ps_ret_strs[PC_OFF_STATE], strlen(Ps_ret_strs[PC_OFF_STATE]) ); 
      }
    }
    if( Verbose )
      LE_send_msg( GL_INFO, "\n" );
  }
}

/***************************************************************************
 Description: Parses next line of output returned from SNMP.
 ***************************************************************************/
int Sentry_get_next_result( char *key, char **result, int *s_ind, int *o_ind )
{
  int switch_index = 0;       /* index of switch */
  int outlet_index = 0;       /* index of outlet */
  int index_to_return = 0;    /* index returned considering switch/outlet */
  char *temp_ptr;

  /* Output of SNMP command should be of the formats:
     Sentry3-MIB::outletName.1.1.2 = STRING: "LAN"
     Sentry3-MIB::outletStatus.2.1.6 = INTEGER: on(1) */

  if( Verbose )
    LE_send_msg( GL_INFO, "Sentry_get_next_result-->key: %s\n", key );

  /* If SNMP output is NULL, then return error code. */
  if( Current_text_pointer == NULL ){ return -1; }

  /* If key is not found in SNMP output, then return error code. */
  if( ( temp_ptr = strstr( Current_text_pointer, key ) ) == NULL ){ return -1; }

  /* Skip over key. */
  temp_ptr += strlen( key );

  /* Read in switch index and outlet index. */
  sscanf( temp_ptr, "%d.1.%d", &switch_index,&outlet_index );

  /* Set pointers so values are returned to calling function. */

  *s_ind = switch_index;
  *o_ind = outlet_index;

  /* Device indices should increment with no repeating values.    *
   * With multiple switches, an outlet index could repeat.        *
   * To prevent this, the index of an outlet in a switch is       *
   * converted to an overall index.                               */      

  index_to_return = ( switch_index - 1 ) * MAX_OUTLETS_PER_SWITCH;
  index_to_return += outlet_index;

  /* Read in value of key. */

  if( ( temp_ptr = strstr( Current_text_pointer, "INTEGER: " ) ) != NULL )
  {
    while( *temp_ptr != ':' ) { temp_ptr++; }
  }
  else if( ( temp_ptr = strstr( Current_text_pointer, "STRING: " ) ) != NULL )
  {
    while( *temp_ptr != ':' ) { temp_ptr++; }
  }
  temp_ptr++;

  *result = temp_ptr;
  while( *temp_ptr != '\n' && *temp_ptr != '\0' && *temp_ptr != '"' )
  {
    temp_ptr++;
  }
  Current_text_pointer = temp_ptr;
  if( *temp_ptr != '\0' )
  { 
    *temp_ptr = '\0';
    Current_text_pointer++;
  }

  return index_to_return;
}

/***************************************************************************
 Description: Sends SNMP power control commands.
 ***************************************************************************/
int Sentry_PC_power_control( int sw_ind, int o_ind )
{
  int flag, ret, ret_flag, ret_val = 1, ind, outlet_ind, switch_ind;
  char *result = NULL, *test_str = NULL, *substr = NULL;
  char source_ip[ MAX_ADDRESS_LEN ] = "";
  char output_buffer[ MAX_OUT_TEXT_LEN ];
  char cmd[ MAX_CMD_LEN ];
  char remote_func[ MAX_CMD_LEN ];
  char command_text[MAX_CMD_LEN];
  int n_bytes = -1;
  int sys_ret = -1;

  LE_send_msg( GL_INFO, "Enter PC_power_control" );

  if( Power_control_flag == PC_TURN_ON )
  {
    flag = CMD_OUTLET_ON;
    test_str = Pc_ret_strs[PC_TURN_ON];
  }
  else if( Power_control_flag == PC_TURN_OFF )
  {
    flag = CMD_OUTLET_OFF;
    test_str = Pc_ret_strs[PC_TURN_OFF];
  }
  else if( Power_control_flag == PC_REBOOT )
  {
    flag = CMD_OUTLET_REBOOT;
    test_str = Pc_ret_strs[PC_REBOOT];
  }
  else
  {
    /* Should never happen. */
    return -1;
  }

  ind = ( ( sw_ind - 1 ) * MAX_OUTLETS_PER_SWITCH ) + o_ind;

  /* Check if outlet index corresponds to an outlet defined as a
     Pc_host_name_#: tag in MSCF conf file. If so, execute the
     corresponding Pc_host_shutdown_cmd_#: command and wait
     Pc_host_shutdown_delay#: seconds before continuing. */

  if( ( Power_control_flag != PC_TURN_ON ) && ( strlen( Outlet_status[ind - 1].shutdown_cmd ) > 0 ) )
  {
    /* Unblock EN events so ORPGMGR function works properly. */

    (void) EN_cntl_unblock();

    /* Get ip of remote host. */

    LE_send_msg( GL_INFO, "Attempting to get IP of remote host." );

    ret = ORPGMGR_discover_host_ip( Outlet_status[ind - 1].host_name,
                         Channel_number, source_ip, MAX_ADDRESS_LEN );

    if( ret < 0 )
    {
      LE_send_msg( GL_ERROR, "ORPGMGR_discover_host_ip failed (%d) for %s",
        ret, Outlet_status[ind -1].host_name );
      LE_send_msg( GL_ERROR, "Unable to gracefully shutdown" );
      LE_send_msg( GL_ERROR, "Continuing with power control action" );
    }
    else if( ret == 0 )
    {
      LE_send_msg( GL_ERROR, "Host: %s not found", Outlet_status[ind-1].host_name );
      LE_send_msg( GL_ERROR, "Unable to gracefully shutdown" );
      LE_send_msg( GL_ERROR, "Continuing with power control action" );
    }
    else if( strlen( source_ip ) == 0 )
    {
      LE_send_msg (GL_ERROR, "Unable to control power for local node" );
      return 0;
    }
    else
    {
      /* Build function to call remotely. */

      sprintf( remote_func, "%s:MISC_system_to_buffer", source_ip );
      sprintf( cmd, Outlet_status[ind - 1].shutdown_cmd );

      /* Capture output in output_buffer. It isn't used now, but
         could be used at a later time. */

      LE_send_msg( GL_INFO, "Attempting RSS_rpc of: %s", remote_func );
      LE_send_msg( GL_INFO, "Attempting CMD: %s", cmd );

      ret = RSS_rpc( remote_func, "i-r s-i ba-%d-io i ia-io",
                     MAX_OUT_TEXT_LEN, &sys_ret, cmd,
                     output_buffer, MAX_OUT_TEXT_LEN, &n_bytes );

      if( ret < 0 )
      {
        LE_send_msg( GL_ERROR, "RSS_rpc failed (%d)", ret );
        LE_send_msg( GL_ERROR, "Unable to gracefully shutdown" );
        LE_send_msg( GL_ERROR, "Continuing with power control action" );
      }
      else if( sys_ret != 0 )
      {
        LE_send_msg( GL_ERROR, "MISC_system_to_buffer failed (%d)", sys_ret );
        LE_send_msg( GL_ERROR, "Unable to gracefully shutdown" );
        LE_send_msg( GL_ERROR, "Continuing with power control action" );
      }
      else
      {
        /*  Continue to block EN events. */

        (void) EN_cntl_block();

        /* Popup to countdown until outlet turn off/reboot. */

        if( Outlet_status[ind - 1].shutdown_delay > 0 )
        {
          sprintf( command_text,
                   "Waiting for graceful shutdown of %s",
                    Outlet_status[ind - 1].host_name );
          LE_send_msg( GL_INFO, "Display countdown popup for %d seconds",
                       Outlet_status[ind - 1].shutdown_delay );
          Popup_wait_for_command( "Waiting...", command_text,
                                  Outlet_status[ind - 1].shutdown_delay );
        }
      }
    }
  }

  /* Execute power control command. */

  sprintf( cmd, Pc_cmd, Pc_addr, sw_ind, o_ind, flag );

  LE_send_msg( GL_INFO, "Execute %s", cmd );

  LE_send_msg( GL_INFO, "PC_power_control-->Send_snmp_command( %s )\n", cmd );
  Send_snmp_command( cmd );

  /* Check output of command to make sure there were no errors. */

  if( Current_text_pointer == NULL )
  {
    LE_send_msg( GL_INFO, "Output of Send_snmp_command is NULL" );
  }
  else
  {
    LE_send_msg( GL_INFO, "Check output of Send_snmp_command" );
    LE_send_msg( GL_INFO, "%ms", Current_text_pointer );
  }

  ret = Sentry_get_next_result( Pc_key, &result, &switch_ind, &outlet_ind );

  if( Verbose )
    LE_send_msg( GL_INFO, "PC_power_control-->ret: %d, result: %s, test_str: %s, strlen(test_str): %d\n",
                 ret, result, test_str, strlen( test_str ) );

  if( (ret != ind)
           ||
      ((substr = strstr( result, test_str )) == NULL)
           ||
      ((ret_val = sscanf( substr + strlen( test_str ), "%d", &ret_flag )) != 1)
           ||
      (flag != ret_flag) )
  {
    LE_send_msg( GL_ERROR, "PC_power_control-->Failed" );

    /* Write out why this failed .... */
    if( ret != ind )
       LE_send_msg( GL_INFO, "---->Because ret: %d != ind: %d\n", ret, ind );

    if( strstr( result, test_str ) == NULL )
       LE_send_msg( GL_INFO, "---->Because test_str: %s is not a substr of result: %s\n", 
                    test_str, result );

    if( flag != ret_flag )
       LE_send_msg( GL_INFO, "---->Because flag: %d != ret_flag: %d\n", 
                    flag, ret_flag );

    if( ret_val != 1 )
       LE_send_msg( GL_INFO, "---->Because ret_val: %d != 1\n", ret_val );

    return -1;
  }

  return 1;
}

