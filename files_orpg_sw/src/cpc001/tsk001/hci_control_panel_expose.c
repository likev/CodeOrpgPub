/************************************************************************
 *									*
 *	Module:  hci_control_panel_expose.c				*
 *									*
 *	Description:  This module handles HCI main window expose	*
 *		      events by copying the main window pixmap to	*
 *		      the window.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:57 $
 * $Id: hci_control_panel_expose.c,v 1.12 2009/02/27 22:25:57 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/************************************************************************
 *	Description: This callback is invoked when an expose event is	*
 *		     generated by the RPG Control/Status window.  This	*
 *		     will automatically happen when the window is	*
 *		     resized, moved, other windows moved over it, etc.	*
 *		     It can be called directly to force the window to	*
 *		     be refreshed.  Normally, one would want to call	*
 *		     the resize procedure instead since it will update	*
 *		     all window objects first.				*
 *									*
 *	Input:  w           - ID of top level widget if activated	*
 *			      normally.  It is unused by this function.	*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_expose( Widget w,
                               XtPointer client_data,
                               XtPointer call_data )
{
  /* All this function does regulary is to copy the pixmap into the
     visible window.  All drawing is done to the pixmap so that all
     changes become visible at the same time. */

  XCopyArea( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_window(),
             hci_control_panel_gc(),
             0,
             0,
             hci_control_panel_width(),
             hci_control_panel_height(),
             0,
             0 );
}
