/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:07 $
 * $Id: hci_gain_focus_callback.c,v 1.4 2009/02/27 22:26:07 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include <hci.h>

/************************************************************************
 *	Description: This function can be used as the gain focus	*
 *		     callback for text widgets.				*
 *									*
 *	Input:  w - text widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: angle (degrees)						*
 ************************************************************************/

void
hci_gain_focus_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
  XmTextShowPosition( w, XmTextGetCursorPosition( w ) );
}
