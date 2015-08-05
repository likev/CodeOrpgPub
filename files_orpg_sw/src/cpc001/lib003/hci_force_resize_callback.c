/************************************************************************
 *	Module: hci_force_resize_callback.c				*
 *	Description: This function forces the window resize callback	*
 *		     to be invoked by resizing the window by a pixel.	*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:07 $
 * $Id: hci_force_resize_callback.c,v 1.3 2009/02/27 22:26:07 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/************************************************************************
 *	Description: This function is used as a workaround to a problem	*
 *		     with the window manager related to maximizing and	*
 *		     moving windows.  When a window is maximized and	*
 *		     then moved, the behavior of widgets inside the	*
 *		     window is not correct (at least when the top level	*
 *		     widget is a form) if a user defined min or max	*
 *		     window width/height was imposed.  To cure this the	*
 *		     window is resized by 1 pixel to force a refresh.	*
 *									*
 *	Input:  w	    - *unused*					*
 *		client_data - widget ID of top level shell		*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_force_resize_callback (
Widget		w,
XtPointer	client_data,	/* Widget ID of toplevel shell	*/
XtPointer	call_data
)
{
	Dimension	height;
	Dimension	width;

	XtVaGetValues ((Widget) client_data,
		XmNwidth,	&width,
		XmNheight,	&height,
		NULL);

	XtVaSetValues ((Widget) client_data,
		XmNwidth,	width,
		XmNheight,	(height-1),
		NULL);
}
