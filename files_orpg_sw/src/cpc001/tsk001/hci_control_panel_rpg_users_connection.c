/************************************************************************
 *									*
 *	Module:  hci_control_panel_rpg_users_connection.c		*
 *									*
 *	Description:  This module contains the routines used to		*
 *	draw the connection and data flow between the RPG and USERS	*
 *	containers on the RPG Control/Status window.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:00 $
 * $Id: hci_control_panel_rpg_users_connection.c,v 1.6 2009/02/27 22:26:00 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global variables. */

static	int	prev_rpg_users_color = -1;

/************************************************************************
 *	Description: This function draws the connection between the	*
 *		     RPG and USERS containers.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_rpg_users_connection( int force_draw )
{
  int x1;
  int y1;
  int i;
  int temp_int;
  int color;
  int nb_status;
  hci_control_panel_object_t *rpg;
  hci_control_panel_object_t *user;
  hci_control_panel_object_t *narrowband;

  /* If the user widget is not defined, return. */

  user = hci_control_panel_object( USERS_BUTTON );

  if( user->widget == ( Widget ) NULL ) 
  {
    return;
  }

  /* Get reference to RPG object. */

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  /* If any of the relevant LBs have been updated, read the status. */

  if( hci_get_prod_info_update_flag() )
  {
    hci_set_prod_info_update_flag( 0 );
    hci_read_nb_connection_status();
  }

  /* Get the current narrowband connection status. */

  nb_status = hci_get_nb_connection_status();

  /* If no narrowband users are connected, don't draw the connection. */

  if( nb_status == NB_HAS_NO_CONNECTIONS ) 
  {
    return;
  }
  else if( nb_status == NB_HAS_CONNECTIONS ) 
  {
    color = hci_get_read_color( NORMAL_COLOR );
  }
  else
  {
    /* Assume failure. */
    color = hci_get_read_color( ALARM_COLOR1 );
  }

  /* Redraw if something changes or if flag forces it. */

  if( color != prev_rpg_users_color || force_draw )
  {
    /* Draw the data connection between the RPG and the Users box. */

    x1 = rpg->pixel + rpg->width + hci_control_panel_3d()/2;
    y1 = rpg->scanl + rpg->height/2 - hci_control_panel_data_height();

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y1,
                    ( int ) ( user->pixel - x1 ),
                    hci_control_panel_data_height() );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BLACK ) );

    /* Add a 3D look to the connection. */

    for( i = 0; i < hci_control_panel_3d(); i++ )
    {
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( y1 - i ),
                 ( int ) ( user->pixel - 1 ),
                 ( int ) ( y1 - i ) );
    }

    /* Since the connection is selectable (launches the Product
       Distribution Comms Status task) update object properties. */

    temp_int = rpg->height/2 - hci_control_panel_data_height();

    narrowband = hci_control_panel_object( NARROWBAND_OBJECT );
    narrowband->pixel  = rpg->pixel + rpg->width; 
    narrowband->scanl  = ( int ) ( rpg->scanl + temp_int );
    narrowband->width  = user->pixel - narrowband->pixel;
    narrowband->height = hci_control_panel_data_height();

    prev_rpg_users_color = color;
  }
}

