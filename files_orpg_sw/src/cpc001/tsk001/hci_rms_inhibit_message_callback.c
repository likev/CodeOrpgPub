/************************************************************************
 *	Module:	hci_rms_inhibit_message_callback.c			*
 *									*
 *	Description:	This module handles all functions pertaining	*
 *			to inhibiting RMS control.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:58 $
 * $Id: hci_rms_inhibit_message_callback.c,v 1.13 2010/03/10 18:46:58 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/*	Macros								*/

#define	MTYP_RDACON	10

/*	File global definitions						*/

static	Widget	Dialog        = (Widget) NULL;
static  Widget	text;
char		Cmd [128];	/* Common buffer for feedback message */
char		rms_buf[128];	/* Common buffer for warning message */

void	rms_inhibit_message_close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	rms_inhibit_message_send_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	rms_inhibit_message_clear_callback (Widget w,
		XtPointer client_data, XtPointer call_data);

/************************************************************************
 *	Description: This function creates a window from which the user	*
 *		     can inhibit control from the RMS for a specified	*
 *		     amount of time.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_rms_inhibit_message ()
{
	Widget	form;
	Widget	rowcol;
	Widget	button;
	Widget	label;
	Widget	inhibit_rowcol;

/*	Log a message in the hci log file.				*/

	HCI_LE_log("RMS inhibit message function selected");

/*	If the window is already defined, do nothing.			*/

	if (Dialog != NULL)
	{
	  HCI_Shell_popup( Dialog );
	  return;
	}

/*	Create the dialog shell for the inhibit RMS time window		*/

	HCI_Shell_init( &Dialog, "RMS Inhibit Time" );

/*	Use a form to manage widgets in this window.			*/

	form = XtVaCreateWidget ("rms_inhibit_message_form",
		xmFormWidgetClass,	Dialog,
		NULL);
	
/*	Create a row of control buttons along the top of the window.	*/

	rowcol = XtVaCreateWidget ("inhibit_form_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNorientation,		XmHORIZONTAL,
		XmNcolumns,		1,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass, rowcol,
		XmNbackground,		 hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		 hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_inhibit_message_close_callback,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass, rowcol,
		XmNbackground,		 hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		 hci_get_read_color (BUTTON_FOREGROUND),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_inhibit_message_send_callback,
		NULL);
		
	button = XtVaCreateManagedWidget ("Clear",
		xmPushButtonWidgetClass, rowcol,
		XmNbackground,		 hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		 hci_get_read_color (BUTTON_FOREGROUND),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
		
	XtAddCallback (button,
		XmNactivateCallback, rms_inhibit_message_clear_callback,
		NULL);
		
	XtManageChild (rowcol);
		
/*	Beneath the control buttons create a text entry widget for the	*
 *	inhibit time.							*/

	inhibit_rowcol = XtVaCreateWidget ("inhibit_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	

	label = XtVaCreateManagedWidget ("Enter Time (1-3600 sec):",
		xmLabelWidgetClass,	inhibit_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
		
	text = XtVaCreateManagedWidget ("inhibit_time",
		xmTextWidgetClass,	inhibit_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
		
	XtAddCallback (text,
		XmNmodifyVerifyCallback, hci_verify_signed_integer_callback,
		(XtPointer) 4);
		
	XtManageChild (inhibit_rowcol);
	
	XtManageChild (form);

	HCI_Shell_start( Dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is called when the "Close" button is	*
 *		     selected in the RMS Inhibit Time window.		*
 *									*
 *	Input:  w           - ID of Close button			*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_inhibit_message_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("RMS inhibit message Close selected");
	HCI_Shell_popdown( Dialog );
}

/************************************************************************
 *	Description: This function is called when the "Clear" button is	*
 *		     selected in the RMS Inhibit Time window.		*
 *									*
 *	Input:  w           - ID of Clear button			*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_inhibit_message_clear_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmTextSetString (text,"");
}

/************************************************************************
 *	Description: This function is called when the "Send" button is	*
 *		     selected in the RMS Inhibit Time window.		*
 *									*
 *	Input:  w           - ID of Send button				*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_inhibit_message_send_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	num_seconds;
	
/*	Get the string associated with the time text widget.  It	*
 *	contains the inhibit time.					*/

	string = XmTextGetString (text);
	
/*	Decode the string into a numeric value.				*/

	if (strlen (string) == 0) {

	      num_seconds = 0;

	} else {

	    sscanf (string,"%d",&num_seconds);

	}

/*	Validate the entered time.  If OK then post an event, passing	*
 *	the time as data.						*/

	if( num_seconds >0 && num_seconds < 3601)
	{
		HCI_LE_log("Sending RMS Inhibit Message");
		EN_post (ORPGEVT_RMS_INHIBIT_MSG, &num_seconds, sizeof(num_seconds), 0);
		HCI_display_feedback( Cmd );
	}			
	else {
	
/*	If not OK then pop up an informative message.			*/

		sprintf (rms_buf,"Value must be between 1 and 3600.\nDo you want to change the value?");
                hci_confirm_popup( Dialog, rms_buf, NULL, rms_inhibit_message_close_callback );	
		}
}

