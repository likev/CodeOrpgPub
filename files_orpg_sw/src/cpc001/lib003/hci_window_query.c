/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:28 $
 * $Id: hci_window_query.c,v 1.6 2009/02/27 22:26:28 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_window_query.c					*
 *									*
 *	Description:  This module is used to query the window heirarchy	*
 *		      for a user specified window name.  If found, it	*
 *		      returns the window ID.  If not, it returns 0.	*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>

/************************************************************************
 *	Description: This function is used to query the window 		*
 *		     for a user specified window name.  If found it	*
 *		     returns the window ID.  If not it returns 0.	*
 *		     This function is useful when trying to restrict	*
 *		     the number of instances of a particular window	*
 *		     at a given screen.  An application can first check	*
 *		     to see if a particular window exists before 	*
 *		     invoking another instance.				*
 *									*
 *	Input:  display - display information				*
 *		window  - top window in heirarchy (i.e. RootWindow)	*
 *		name    - pointer to string containing window name to	*
 *			  check for					*
 *	Output: NONE							*
 *	Return: window ID (or 0 if not found)				*
 ************************************************************************/

Window	hci_window_query (
Display	*display,
Window	window,
char	*name
)
{
	Window	root_ret;
	Window	parent_ret;
	Window	*child_ret;
	unsigned int	num_child;
	int	i;
	int	status;
	XTextProperty	property;
	Window	ret;
	char	buf [32];

	ret = 0;

	XQueryTree (display,
	    window,
	    &root_ret,
	    &parent_ret,
	    &child_ret,
	    &num_child);

/*	For each branch in window tree recursively call this function	*
 *	until the end of the branch is reached.  If a match is found	*
 *	then the return value will be > 0.				*/

	for (i=0;i<num_child;i++) {

	    ret = hci_window_query (display, child_ret [i], name);

	    if (ret > 0) {

		XFree (child_ret);
		return (ret);

	    }
	}

/*	Get the propetries of the window.				*/

	status = XGetWMName (display,
			window,
			&property);

/*	If we were able to get the properties, then we want to see if	*
 *	the first part of the window name matches the name passed to	*
 *	this function.  If a match is found then if the configuration	*
 *	is FAA redundant we need to also match the channel annotation	*
 *	added to the end of the window name.				*/

	if (status != 0) {

	    if (!strncmp ((char *) property.value, name, strlen (name))) {

		if (HCI_get_system() == HCI_FAA_SYSTEM) {

		    sprintf (buf,"FAA:%d",
			     ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

		    if (strstr ((char *) property.value,buf) != NULL) {

			ret = window;

		    }

		} else {

		    ret = window;

		}
	    }

	    XFree (property.value);

	}

	XFree (child_ret);

	return (ret);
}
