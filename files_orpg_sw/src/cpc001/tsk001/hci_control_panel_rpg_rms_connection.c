/************************************************************************
 *									*
 *	Module:  hci_control_panel_rpg_rms_connection.cw		*
 *									*
 *	Description:  This module contains the routines used to		*
 *	draw the connection and data flow between the RPG and RMS	*
 *	containers on the RPG Control/Status window.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/01/04 23:34:49 $
 * $Id: hci_control_panel_rpg_rms_connection.c,v 1.6 2011/01/04 23:34:49 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global variables. */

static	int	prev_rms_down_flag = -1;

/************************************************************************
 *	Description: This function draws the connection between the	*
 *		     RPG container and the RMS object.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							* 
 ************************************************************************/

void hci_control_panel_rpg_rms_connection( force_draw )
{
  int	x1;
  int	y1;
  int	x2;
  int	y2;
  int	i;
  int   rms_down_flag = -1;
  hci_control_panel_object_t *rms;
  hci_control_panel_object_t *rpg;

  /* Get reference to RPG object. */

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  /* Get RMS connect status. */

  rms_down_flag = hci_get_rms_down_flag();

  /* Redraw if something changes or if flag forces it. */

  if( rms_down_flag != prev_rms_down_flag || force_draw )
  {
    /* Draw the data connection between the RPG container and the
       RMS container. */

    rms = hci_control_panel_object( RMS_BUTTON );

    x1 = rpg->pixel + rpg->width/2 - hci_control_panel_data_width()/2;
    y1 = rpg->scanl + rpg->height;
    x2 = rms->pixel - hci_control_panel_data_width()/2 + 4;
    y2 = rpg->scanl;

    if( rms_down_flag )
    {
      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( RED ) );
    }
    else
    {
      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( NORMAL_COLOR ) );
    } 

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y1,
                    hci_control_panel_data_height(),
                    ( int ) ( rms->scanl - y1 - hci_control_panel_3d()/2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BLACK ) );

    /* Add a 3D look to the connection. */

    for( i = 0; i < hci_control_panel_3d(); i++ )
    {
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( x1 + hci_control_panel_data_height() + i ),
                 ( int ) y1,
                 ( int ) ( x1 + hci_control_panel_data_height() + i ),
                 ( int ) ( rms->scanl - hci_control_panel_3d()/2 ) );
    }

    prev_rms_down_flag = rms_down_flag;
  } 
}


