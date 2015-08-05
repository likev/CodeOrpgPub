/************************************************************************
 *									*
 *	Module:  hci_control_panel_draw.c				*
 *									*
 *	Description:  This module draws the RPG Control/Status gui.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:48:02 $
 * $Id: hci_control_panel_draw.c,v 1.10 2012/09/10 20:48:02 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/************************************************************************
 *	Description: This function draws the RPG Control/Status gui by	*
 *		     calling various functions.				*
 *									*
 *	Input:  force_draw - if 1, force a redraw, if 0, redraw only	*
 *			     if something has changed.			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_draw( int force_draw )
{
  hci_control_panel_object_t *top;
  Dimension width;
  Dimension height;

  /* Get reference to top-level widget. If it is null, return. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Determine the new positions for the pushbutton widgets and
     resize and move them accordingly. */

  hci_control_panel_RDA_button( force_draw );
  hci_control_panel_RPG_button( force_draw );
  if( HCI_has_rms() )
  {
    hci_control_panel_RMS_button( force_draw );
  }
  hci_control_panel_USERS_button( force_draw );
  hci_control_panel_applications( force_draw );

  /* Draw the power source type. */

  hci_control_panel_power( force_draw );

  /* Draw the tower/TPS status. */

  hci_control_panel_tower( force_draw );

  /* Draw elevation lines radiating from the radome. */

  hci_control_panel_elevation_lines( force_draw );

  /* Draw radome on top of the tower. */

  hci_control_panel_radome( force_draw );

  /* Updates azimuth position in radome. */

  hci_update_antenna_position();

  /* Draw VCP button. */

  hci_control_panel_vcp( force_draw );

  /* Draw the comm connections between the various major system components. */

  hci_control_panel_rpg_rda_connection( force_draw );
  hci_control_panel_rpg_users_connection( force_draw );
  if( HCI_has_rms() )
  {
    hci_control_panel_rpg_rms_connection( force_draw );
  }

  /* Draw the RDA controls status. */

  hci_control_panel_control_status( force_draw );

  /* Display the system time in the frame title bar. */

  hci_control_panel_set_system_time( force_draw );

  /* Display the start time of the current radar volume being processed. */

  hci_control_panel_set_volume_time( force_draw );

  /* Display the current RDA status info. */

  hci_control_panel_rda_status( force_draw );

  /* Display the current RPG status info. */

  hci_control_panel_rpg_status ( force_draw );

  /* Display the current USERS status info. */

  hci_control_panel_users_status ( force_draw );

  /* Display feedback from last command. */

  hci_display_feedback_string( force_draw );

  /* Display the latest system log status and alarm messages. */

  hci_control_panel_system_log_messages( force_draw );

  /* Display the PRF Mode, VAD Update, and precip category beneath
     the User button. */

  hci_control_panel_status( force_draw );

  /* Display the Mode Select categories (mode conflict, switching, etc.). */

  hci_control_panel_mode_select( force_draw );

  /* Display active RDA alarms to the left of the RDA Radome/Tower. */

  if( ORPGRDA_get_rda_config( NULL ) == ORPGRDA_LEGACY_CONFIG )
  {
    hci_control_panel_rda_alarms( force_draw );
  }
  else
  {
    hci_control_panel_orda_alarms( force_draw );
  }

  /* Set the highlight color around the environmental winds button
     based on whether the winds have expired or not. */

  hci_control_panel_environmental_winds( force_draw );

  /* Get the current size of the main window and
      save in the top objects data. */

  XtVaGetValues( top->widget,
                 XmNwidth, &width,
                 XmNheight, &height,
                 NULL);

  /* Make the new Status/Control Window visible. */

  XCopyArea( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_window(),
             hci_control_panel_gc(),
             0,
             0,
             width,
             height,
             0,
             0 );
}
