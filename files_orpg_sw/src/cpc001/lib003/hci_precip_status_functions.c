 /*
  * RCS info
  * $Author: ccalvert $
  * $Locker:  $
  * $Date: 2009/04/06 18:17:58 $
  * $Id: hci_precip_status_functions.c,v 1.10 2009/04/06 18:17:58 ccalvert Exp $
  * $Revision: 1.10 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: hci_precip_status_functions.c				*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI to interface with RPG precip status data.	*
 *									*
 ************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_precip_status.h>

/* Local global variables. */

static	Precip_status_t	Precip_status;	/* Buffer for precip status */
static	int		precip_status_init_flag = 0;	/* Init flag */
static	int		precip_status_update_flag = 1;	/* Update flag */

/* Local prototypes. */

static int precip_status_init();
static void precip_status_callback( int, LB_id_t, int, void * );
static void set_precip_status_missing();
 
/************************************************************************
 *      Description: This function registers a callback function for	*
 *		the precip status message in the HCI LB.		*
 ***********************************************************************/

static int
precip_status_init()
{
  int status = -1;

  /* Register for updates to GUI info message */

  status = ORPGDA_UN_register( ORPGDAT_HCI_DATA,
                               HCI_PRECIP_STATUS_MSG_ID,
                               precip_status_callback );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error("ORPGDA_UN_register( HCI_PRECIP_STATUS_MSG_ID) Failed (%d)",
                 status );
  }

  return status;
}

/************************************************************************
 *      Description: This function is the callback function for the	*
 *              precip status message in the HCI LB and sets the update *
 *		flag.							*
 ***********************************************************************/

static void
precip_status_callback(
int	fd,
LB_id_t	msgid,
int	msg_info,
void	*arg )
{
  precip_status_update_flag = 1;
}

/************************************************************************
 *      Description: This function reads the precip status message.	*
 ***********************************************************************/

void
hci_read_precip_status()
{
  int status = -1;

  /* Read precip status message */

  status = ORPGDA_read( ORPGDAT_HCI_DATA,
                        ( char * ) &Precip_status,
                        sizeof( Precip_status_t ),
                        HCI_PRECIP_STATUS_MSG_ID );

  if( status < 0 )
  {
    set_precip_status_missing();
    HCI_LE_error("ORPGDA_read( ORPGDAT_HCI_DATA ) Failed (%d) ", status);
  }

}

/************************************************************************
 *	Description: This function returns the latest precip status.	*
 ***********************************************************************/

Precip_status_t
hci_get_precip_status()
{
  if( !precip_status_init_flag )
  {
    precip_status_init_flag = 1;
    precip_status_init();
  }

  if( precip_status_update_flag )
  {
    precip_status_update_flag = 0;
    hci_read_precip_status();
  }

  return Precip_status;
}

/************************************************************************
 *	Description: This function sets the elements in the precip	*
 *		     status struct to missing.				*
 ***********************************************************************/

void
set_precip_status_missing()
{
  Precip_status.current_precip_status = PRECIP_STATUS_UNKNOWN;
  Precip_status.rain_area_trend = TREND_UNKNOWN;
  Precip_status.time_last_exceeded_raina = TIME_LAST_EXC_RAINA_UNKNOWN;
  Precip_status.time_remaining_to_reset_accum = RESET_ACCUM_UNKNOWN;
  Precip_status.rain_area = PRECIP_AREA_UNKNOWN;
  Precip_status.rain_area_diff = PRECIP_AREA_DIFF_UNKNOWN;
  Precip_status.rain_dbz_thresh_rainz = RAIN_DBZ_THRESH_UNKNOWN;
  Precip_status.rain_area_thresh_raina = RAIN_AREA_THRESH_UNKNOWN;
  Precip_status.rain_time_thresh_raint = RAIN_TIME_THRESH_UNKNOWN;
}

