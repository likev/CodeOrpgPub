/************************************************************************
 *									*
 *	Module:  hci_timer_proc.c					*
 *									*
 *	Description:  This module is used to regularly check the	*
 *		      HCI processing and check for changes to various	*
 *		      status information and update the main HCI	*
 *		      display window accordingly.  It is the callback	*
 *		      for Xt timer events.				*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:25 $
 * $Id: hci_timer_proc.c,v 1.155 2009/02/27 22:26:25 ccalvert Exp $
 * $Revision: 1.155 $
 * $State: Exp $
 */

/* Local include files. */

#include <hci_control_panel.h>

/* Staic Variables. */

static  int 		progress_meter_first_time = 1;
static	int		busy_check_flag = 0;
static	time_t		old_RDA_status_update_time = 0;
static	int		old_power_state = 0;
static	int		need_to_clear_HCI = HCI_YES_FLAG;
static	int		prev_vcp = -1;
static	int		prev_wb_status = -1;
static	int		prev_rda_state = -1;
static	int		prev_nb_status = -1;

/* Function prototypes. */

void	check_for_busy_flags();
void	check_for_new_data();
void	hci_request_new_rda_performance_data();
int	get_redraw_flag();
int	hci_child_process_active(char *app_name);

/************************************************************************
 *	Description: This function draws the RPG Control/Status gui by	*
 *		     calling various functions.				*
 *									*
 *	Input:  Widget w - not used					*
 *		XtIntervalId - not used					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_timer_proc()
{
  int redraw_flag = -1;

  /* Check for busy flags from gui tasks. */

  check_for_busy_flags();

  /* Make sure latest radial info event is available. */

  hci_set_latest_radial_info();

  /* Check for new data. */

  check_for_new_data();

  /* Check for conditions that warrant an erase and forced redraw
     of the control panel. */

  redraw_flag = get_redraw_flag();

  /* If a forced redraw is needed, erase the control panel first. */

  if( redraw_flag == FORCE_REDRAW )
  {
    hci_control_panel_erase();
  }

  /* Draw control panel. */

  hci_control_panel_draw( redraw_flag );

  /* Destroy progress meter. */

  if( progress_meter_first_time )
  {
    progress_meter_first_time = 0;
  }
}

/************************************************************************
 *	Description: This function checks for busy flags from HCI gui	*
 *		     objects and resets them if necessary.		*
 *									*
 *	Input:	NONE							*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void check_for_busy_flags()
{
  hci_control_panel_object_t	*object;
  int	i;

  /* Check the busy flag for each GUI object. If set, check to
     see if a process exists with that name. If it doesn't, then
     it probably means that the process died before it reached the
     main loop so the event was never posted to clear the busy
     flag. In this case we want to clear the busy flag and restore
     the cursor (if need be). Only do this check every 4th time. */

  if( busy_check_flag <= 0 )
  {
    for( i = RDA_CONTROL_BUTTON; i < LAST_OBJECT; i++ )
    {
      object = hci_control_panel_object( i );

      if( object->flags & BUSY_OBJECT )
      {
        if( hci_child_process_active( object->app_name ) == 0 )
        {
          object->flags = object->flags & ( ~BUSY_OBJECT );
        }
      }
    }

    busy_check_flag = 4;
  }

  busy_check_flag--;
}

/************************************************************************
 *	Description: This function checks for new data to read in.	*
 *									*
 *	Input:	NONE							*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void check_for_new_data()
{
  int power_state;

  /* Check to see if new environmental wind data has been received. */

  if( hci_get_env_wind_update_flag() )
  {
    /* Since we are about to read the new message, clear the flag. */
    hci_set_env_wind_update_flag( 0 );
    HCI_LE_status( "New Environmental Wind message read" );
    hci_read_environmental_wind_data();
  }

  /* Check to see if any RDA console messages have been
     received.  If so, then pop up the console message window if
     it isn't displayed already. */

  if( hci_get_RDA_message_flag() )
  {
    /* Since we are about to read all new messages, clear the
       flag set when the event was handled. */

    hci_set_RDA_message_flag( 0 );
	
    HCI_LE_status( "New Console message read" );

    hci_get_console_message();
  }

  /* Check to see if a new RDA Performance Maintenance message
     was received since the last timer event. */

  if( ORPGRDA_get_rda_config( NULL ) == ORPGRDA_ORDA_CONFIG )
  {
    if( hci_get_orda_pmd_update_flag() )
    {
      /* Since we are about to read the new message, clear the flag. */
      hci_set_orda_pmd_update_flag( 0 );
      HCI_LE_status( "New RDA PMD message read" );
      hci_read_orda_pmd();
    }
  }
  else
  {
    if( hci_get_rda_performance_update_flag() )
    {
      /* Since we are about to read the new message, clear the flag. */
      hci_set_rda_performance_update_flag( 0 );
      HCI_LE_status( "New RDA PMD message read" );
      hci_read_rda_performance_data();
    }
  }

  /* Check to see if a new RDA status message was received since
     the last timer event. */

  if( ORPGRDA_status_update_time() != old_RDA_status_update_time )
  {
    old_RDA_status_update_time = ORPGRDA_status_update_time();

    /* Check to see if the power source changed to Auxilliary. If
       it did, then make a request for RDA performance data so we
       can get the proper fuel tank level. */

    power_state = ORPGRDA_get_status( RS_AUX_POWER_GEN_STATE ) & BIT_0_MASK;

    if( power_state && ( power_state != old_power_state ) )
    {
      hci_request_new_rda_performance_data();
    }

    old_power_state = power_state;
  }
}

/************************************************************************
 *	Description: This function determines the redraw flag.		*
 *									*
 *	Input:	NONE							*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

int get_redraw_flag()
{
  /* By default, we only want to redraw small parts of the control
     as status/events change (redraw_flag = REDRAW_IF_CHANGED).
     However, certain conditions such as wideband disconnects/reconnects,
     new VCPs, etc. warrant a large number of changes to the control
     panel. In these cases, it is easier to simply erase the control
     control panel (call hci_control_panel_erase()) and force a redraw
     of the whole thing (redraw_flag = FORCE_REDRAW). */

  int wbstat = -1;
  int rda_state = -1;
  int current_vcp = -1;
  int nb_status = -1;
  int return_flag = REDRAW_IF_CHANGED;

  /* If wideband status has changed, force redraw. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  if( wbstat != prev_wb_status )
  {
    return_flag = FORCE_REDRAW;
    prev_wb_status = wbstat;
  }

  /* If RDA status has changed, force redraw. */

  rda_state = ORPGRDA_get_status( RS_RDA_STATUS );

  if( rda_state != prev_rda_state )
  {
    return_flag = FORCE_REDRAW;
    prev_rda_state= rda_state;
  }

  /* If VCP has changed, force redraw. */

  current_vcp = ORPGVST_get_vcp();

  if( current_vcp != prev_vcp )
  { 
    return_flag = FORCE_REDRAW;
    prev_vcp = current_vcp;
  }

  /* If narrowband line status has changed, force redraw. */

  nb_status = hci_get_nb_connection_status();

  if( nb_status != prev_nb_status )
  { 
    return_flag = FORCE_REDRAW;
    prev_nb_status = nb_status;
  }

  /* Check if external module wants the HCI cleared. */

  if( need_to_clear_HCI )
  {
    need_to_clear_HCI = HCI_NO_FLAG;
    return_flag = FORCE_REDRAW;
  }

  return return_flag;
}

/************************************************************************
 *	Description: This function determines the redraw flag.		*
 *									*
 *	Input:	NONE							*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void hci_control_panel_clear_HCI()
{
  need_to_clear_HCI = HCI_YES_FLAG;
}

