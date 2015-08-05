/************************************************************************
 *									*
 *	Module:  hci_control_panel_erase.c				*
 *									*
 *	Description:  This module erases the RPC Control/Status window.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:57 $
 * $Id: hci_control_panel_erase.c,v 1.2 2009/02/27 22:25:57 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/************************************************************************
 *	Description: This function handles all resize events for the	*
 *		     RPG Control/Status window.  It can be called	*
 *		     by any function wishing to refresh the display.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_erase()
{
  Dimension width;
  Dimension height;
  hci_control_panel_object_t *top;

  /* Get top level widget. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the current size of the main window and
     save in the top objects data. */

  XtVaGetValues( top->widget,
                 XmNwidth, &width,
                 XmNheight, &height,
                 NULL );

  /* Paint the background of the drawing area to
     the first background color. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( BACKGROUND_COLOR1 ) );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  0,
                  0,
                  width,
                  height );
}
