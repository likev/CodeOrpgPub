/************************************************************************
 *									*
 *	Module:  hci_control_panel_users_status.c			*
 *									*
 *	Description:  This module is used to display various status	*
 *		      above the USERS box in the HCI main window.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:02 $
 * $Id: hci_control_panel_users_status.c,v 1.4 2009/02/27 22:26:02 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	char	previous_comms_relay_buf[ 24 ] = "";

/************************************************************************
 *	Description: This function displays the current RPG status and	*
 *		     mode above the RPG container.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_users_status ( int force_draw )
{
  char buf [48];
  char buf1 [24];
  char buf2 [24];
  int max_label_width;
  int height, descent;
  int comms_relay_status;
  Pixel fg_color = -1;
  Pixel bg_color = -1;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *users;
  int x1 = -1;
  int x2 = -1;
  int x3 = -1;
  int y_comms = -1;

  /* Get a pointer to the USERS container object data since we want
     to display the status. */

  users = hci_control_panel_object( USERS_BUTTON );

  /* Set font information. */

  fontinfo = hci_get_fontinfo( SCALED );
  height = fontinfo->ascent + fontinfo->descent;
  descent = fontinfo->descent;

  /* Determine max width of possible values for labels
     above the USERS button. This ensures that each
     label is the same width regardless of the string
     used to create it. The string used to define the
     max width isn't necessarily the one with the most
     characters. It depends on the value returned by 
     the XTextWidth() function. Each possible string
     value will need to be tested to determine the
     widest one to use. */

  strcpy( buf1, " Relay: " ); /* Longest label tag. */
  strcpy( buf2, " DISCONNECTED " ); /* Longest label value. */
  sprintf( buf, "%s%s", buf1, buf2 ); /* Longest label. */
  max_label_width = XTextWidth( fontinfo, buf, strlen( buf ) );

  x1 = users->pixel - ( ( max_label_width - users->width )/2 );
  x2 = x1 + XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  x3 = x1 + max_label_width;
  y_comms = users->scanl - 1.8*height;

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {

    /******************* Comms Relay Status label *******************/

    /* Get the Comms Relay status. */

    comms_relay_status = ORPGRED_comms_relay_state( ORPGRED_MY_CHANNEL );

    switch( comms_relay_status )
    {
      case ORPGRED_COMMS_RELAY_ASSIGNED :
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      bg_color = hci_get_read_color( NORMAL_COLOR );
      sprintf( buf2, "    CONNECTED " );
      break;

      case ORPGRED_COMMS_RELAY_UNASSIGNED :
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      bg_color = hci_get_read_color( BACKGROUND_COLOR2 );
      sprintf( buf2, " DISCONNECTED " );
      break;

      case ORPGRED_COMMS_RELAY_UNKNOWN :

      default :

      fg_color = hci_get_read_color( WHITE );
      bg_color = hci_get_read_color( ALARM_COLOR1 );
      sprintf( buf2, "      UNKNOWN " );
      break;
    }

    /* If redraw is forced, redraw tag. */

    if( force_draw )
    {
      strcpy( buf1, " Relay: " );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BACKGROUND_COLOR2 ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      x1,
                      y_comms,
                      x2 - x1,
                      height );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( TEXT_FOREGROUND ) );

      XDrawString( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   x1,
                   y_comms - descent + height,
                   buf1,
                   strlen( buf1 ) );
    }

    /* If value has changed (or redraw is forced), redraw value. */

    if( strcmp( buf2, previous_comms_relay_buf ) != 0 || force_draw )
    {
      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      bg_color );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      x2,
                      y_comms,
                      x3 -x2,
                      height );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      fg_color );

      XSetBackground( HCI_get_display(),
                      hci_control_panel_gc(),
                      bg_color );

      XDrawImageString( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        x2,
                        y_comms - descent + height,
                        buf2,
                        strlen( buf2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BLACK ) );

      XDrawRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      x1,
                      y_comms - 1,
                      max_label_width,
                      height + 1 );

      strcpy( previous_comms_relay_buf, buf2 );
    }
  }
}

