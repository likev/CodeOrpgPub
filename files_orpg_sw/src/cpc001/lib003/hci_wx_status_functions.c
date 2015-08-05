 /*
  * RCS info
  * $Author: ccalvert $
  * $Locker:  $
  * $Date: 2009/04/06 18:18:00 $
  * $Id: hci_wx_status_functions.c,v 1.14 2009/04/06 18:18:00 ccalvert Exp $
  * $Revision: 1.14 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: hci_wx_status_functions.c				*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI to interface with RPG mode/VCP status data.	*
 *									*
 ************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_wx_status.h>

/* Local global variables. */

static	Wx_status_t	Wx_status;	/* Buffer for wx status */
static	int		wx_status_init_flag = 0; /* Init flag */
static	int		wx_status_update_flag = 1; /* Update flag */
static	int		auto_switch_init_flag = 0; /* Init flag */
static	int		auto_switch_update_flag = 1; /* Update flag */
static	int		mode_A_auto_switch_flag = -1;
static	int		mode_B_auto_switch_flag = -1;

/* Local prototypes. */

static int wx_status_init();
static void set_wx_status_missing();
static void wx_status_callback( int, LB_id_t, int, void * );
static void auto_switch_init();
static void auto_switch_callback( int, LB_id_t, int, char * );
static void read_auto_switch_flags();

/************************************************************************
 *      Description: This function registers a callback function for	*
 *		the wx status message in the ORPGDAT_GSM_DATA LB.	*
 ***********************************************************************/

static int
wx_status_init()
{
  int status = -1;

  /* Set write permission so LB notification will work for	*
   * all HCI instances.					*/

  ORPGDA_write_permission( ORPGDAT_GSM_DATA );

  /* Register for updates to GUI info message */

  status = ORPGDA_UN_register( ORPGDAT_GSM_DATA,
                               WX_STATUS_ID,
                               wx_status_callback );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error( "ORPGDA_UN_register( WX_STATUS_ID) Failed (%d)", status );
  }

  return status;
}

/************************************************************************
 *      Description: This function is the callback function for the	*
 *              wx status message in the ORPGDAT_GSM_DATA LB and sets	*
 *		the update flag.					*
 ***********************************************************************/

static void
wx_status_callback(
int	fd,
LB_id_t	msgid,
int	msg_info,
void	*arg )
{
  wx_status_update_flag = 1;
}

/************************************************************************
 *	Description: This function reads the wx status message.		*
 ***********************************************************************/

void
hci_read_wx_status()
{
  int status = -1;

  /* Read wx status message */

  status = ORPGDA_read( ORPGDAT_GSM_DATA,
                        ( char * ) &Wx_status,
                        sizeof( Wx_status_t ),
                        WX_STATUS_ID );

  if( status < 0 )
  {
    set_wx_status_missing();
    HCI_LE_error( "ORPGDA_read( ORPGDAT_GSM_DATA ) Failed (%d)", status );
  }

}

/************************************************************************
 *	Description: This function returns the latest wx status.	*
 ***********************************************************************/

Wx_status_t
hci_get_wx_status()
{
  if( !wx_status_init_flag )
  {
    wx_status_init_flag = 1;
    wx_status_init();
  }

  if( wx_status_update_flag )
  {
    wx_status_update_flag = 0;
    hci_read_wx_status();
  }

  return Wx_status;
}

/************************************************************************
 *	Description: This function sets the elements in the wx		*
 *		     status struct to missing.				*
 ***********************************************************************/

static void
set_wx_status_missing()
{
  Wx_status.current_wxstatus = WX_STATUS_UNDEFINED;
  Wx_status.current_vcp = WX_STATUS_UNDEFINED;
  Wx_status.recommended_wxstatus = WX_STATUS_UNDEFINED;
  Wx_status.recommended_wxstatus_start_time = ( time_t ) WX_STATUS_UNDEFINED;
  Wx_status.recommended_wxstatus_default_vcp = WX_STATUS_UNDEFINED;
  Wx_status.conflict_start_time = ( time_t ) WX_STATUS_UNDEFINED;
  Wx_status.current_wxstatus_time = ( time_t ) WX_STATUS_UNDEFINED;
  Wx_status.precip_area = ( float ) WX_STATUS_UNDEFINED;
  Wx_status.a3052t.curr_time = ( time_t ) WX_STATUS_UNDEFINED;
  Wx_status.a3052t.last_time = ( time_t ) WX_STATUS_UNDEFINED;
  Wx_status.a3052t.time_to_cla = ( time_t ) WX_STATUS_UNDEFINED;
  Wx_status.a3052t.pcpctgry = ( int ) WX_STATUS_UNDEFINED;
  Wx_status.a3052t.prectgry = ( int ) WX_STATUS_UNDEFINED;
}

/************************************************************************
 *      Description: This function is used to access mode select        *
 *                   adaptation data and get the value of the manual    *
 *                   override flag for mode A.                          *
 ************************************************************************/

int
hci_get_mode_A_auto_switch_flag()
{
  if( !auto_switch_init_flag )
  {
    auto_switch_init_flag = 1;
    auto_switch_init();
  }

  if( auto_switch_update_flag )
  {
    auto_switch_update_flag = 0;
    read_auto_switch_flags();
  }

  return mode_A_auto_switch_flag;
}

/************************************************************************
 *      Description: This function is used to set the value of the	*
 *                   manual override flag for mode A.                   *
 ************************************************************************/

int
hci_set_mode_A_auto_switch_flag( int flag )
{
  double set_value = (double)flag;

  return DEAU_set_values( DEA_AUTO_SWITCH_MODE_A, 0, &set_value, 1, 0 );
}

/************************************************************************
 *      Description: This function is used to access mode select        *
 *                   adaptation data and get the value of the manual    *
 *                   override flag for mode B.                          *
 ************************************************************************/

int
hci_get_mode_B_auto_switch_flag()
{
  if( !auto_switch_init_flag )
  {
    auto_switch_init_flag = 1;
    auto_switch_init();
  }

  if( auto_switch_update_flag )
  {
    auto_switch_update_flag = 0;
    read_auto_switch_flags();
  }

  return mode_B_auto_switch_flag;
}

/************************************************************************
 *      Description: This function is used to set the value of the	*
 *                   manual override flag for mode B.                   *
 ************************************************************************/

int
hci_set_mode_B_auto_switch_flag( int flag )
{
  double set_value = (double)flag;

  return DEAU_set_values( DEA_AUTO_SWITCH_MODE_B, 0, &set_value, 1, 0 );
}

/************************************************************************
 *      Description: This function reads the auto switch values from	*
 *                   the DEAU DB.					*
 ************************************************************************/

static void
read_auto_switch_flags()
{
  double get_value = 0.0;

  if( DEAU_get_values( DEA_AUTO_SWITCH_MODE_A, &get_value, 1 ) > 0 )
  {
    mode_A_auto_switch_flag = ( int )get_value;
  }
  else
  {
    mode_A_auto_switch_flag = -1;
  }

  if( DEAU_get_values( DEA_AUTO_SWITCH_MODE_B, &get_value, 1 ) > 0 )
  {
    mode_B_auto_switch_flag = ( int )get_value;
  }
  else
  {
    mode_B_auto_switch_flag = -1;
  }
}

/************************************************************************
 *      Description: This function registers callbacks for the auto-	*
 *                   switch flags.					*
 ************************************************************************/

static void
auto_switch_init()
{
  int ret;

  /* Register callback function for Mode A Auto-Switch flag. */

  if( ( ret = DEAU_UN_register( DEA_AUTO_SWITCH_NODE, (void *) auto_switch_callback ) ) < 0 )
  {
    HCI_LE_error( "DEAU_UN_register %s failed (%d)", DEA_AUTO_SWITCH_NODE, ret );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
}

/************************************************************************
 *      Description: DEAU callback function for Auto-Switch flags.	*
 ************************************************************************/

static void
auto_switch_callback( int lb_fd, LB_id_t msg_id, int msg_len, char *group_name )
{
  auto_switch_update_flag = 1;
}

