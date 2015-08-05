/************************************************************************
 *	Module:	 hci_RMS_control_button.c				*
 *									*
 *	Description:  This module is used by the ORPG HCI to define	*
 *		      and display the RMS Control menu.			*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:50 $
 * $Id: hci_RPG_rms_button.c,v 1.14 2010/03/10 18:46:50 ccalvert Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/*	Local include files.						*/

#include <hci_control_panel.h>

enum {RMS_INHIBIT, RMS_FREE_TEXT};

/*	Local static variables.						*/

static	Widget	Rms_control_dialog  = (Widget) NULL;
			/* Widget ID of RMS Control window dialog shell */

void	rms_control_button_close (Widget w,
		XtPointer client_data, XtPointer call_data);
void	rms_command_callback (Widget w,
		XtPointer client_data, XtPointer call_data);

/************************************************************************
 *	Description: This module defines the widgets for the RMS	*
 *	control menu and displays them.					*
 *									*
 *	Inpput: w           - ID of RMS button invoking callback	*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_RMS_control_button (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	form;
	Widget	control_rowcol;
	Widget	system_frame;
	Widget	system_form;
	Widget	startup_frame;
	Widget	startup_rowcol;
	Widget	button;
	Widget	label;

/*	Do not allow more than one RMS control menu to exist at one	*
 *	time.								*/

	if (Rms_control_dialog != NULL)
	{
/*	    The RMS Control window exists so bring it to the top of	*
 *	    the window heirarchy.					*/

	    HCI_Shell_popup( Rms_control_dialog );
	    return;
	}

	HCI_LE_log("RMS Control button selected");
	
/*	Define the widgets for the RMS Messages window and display them.*/

	HCI_Shell_init( &Rms_control_dialog, "RMS Messages" );

/*	Use a form widget to organize the various menu widgets.		*/

	form   = XtVaCreateWidget ("form",
		xmFormWidgetClass,		Rms_control_dialog,
		XmNautoUnmanage,		False,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	control_rowcol = XtVaCreateWidget ("control_rowcol",
		xmRowColumnWidgetClass,		form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNisAligned,			False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_control_button_close, NULL);

	XtManageChild (control_rowcol);

	system_frame   = XtVaCreateManagedWidget ("top_frame",
		xmFrameWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			control_rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget (" RPG to RMS Messages",
		xmLabelWidgetClass,	system_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	system_form = XtVaCreateWidget ("system_rowcol",
		xmRowColumnWidgetClass,	system_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	startup_frame   = XtVaCreateManagedWidget ("startup_frame",
		xmFrameWidgetClass,		system_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	startup_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	startup_rowcol = XtVaCreateWidget ("startup_rowcol",
		xmRowColumnWidgetClass,	startup_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("  RMS Inhibit  ",
		xmPushButtonWidgetClass,startup_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_command_callback,
		(XtPointer) RMS_INHIBIT);

	button = XtVaCreateManagedWidget ("RMS Free Text",
		xmPushButtonWidgetClass,startup_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_command_callback,
		(XtPointer) RMS_FREE_TEXT);

	XtManageChild (startup_rowcol);

	XtManageChild (system_form);
	
	XtManageChild (form);

	HCI_Shell_start( Rms_control_dialog, NO_RESIZE_HCI );	
}

/************************************************************************
 *	Description: This module is called when the RMS Control window	*
 *	"Close" button is selected.					*
 *									*
 *	Inpput: w           - ID of "Close" pushbutton widget		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_control_button_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("RMS Control Close button selected");
	HCI_Shell_popdown( Rms_control_dialog );
}

/************************************************************************
 *	Description: This module is called when the one of the RMS 	*
 *	control pushbuttons is selected.				*
 *									*
 *	Input:  w           - ID of pushbutton widget invoking callback	*
 *		client_data - RMS command				*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_command_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	buf [HCI_BUF_128];

/*	If the RMS interface is down, popup an informative message to	*
 *	the user.							*/

	if(hci_get_rms_down_flag()){
	
		sprintf (buf,"RMS interface is down");
                hci_warning_popup( Rms_control_dialog, buf, rms_control_button_close );	
		return;
				
		}/* End if */
	else {

/*	The RMS interface is up so get the command from client data and	*
 *	invoke it.							*/

		switch ((int) client_data) {

	   	 case RMS_INHIBIT :

			HCI_LE_log("RMS Inhibit message selected");
			hci_rms_inhibit_message ();
			break;

	   	 case RMS_FREE_TEXT :

		   	HCI_LE_log("RMS Free Text message selected");
			hci_rms_text_message ();
			break;

	   	 default :

		   	HCI_LE_log("Unknown RMS command");
			return;

		}/* End switch */
		
	}/* End Else */

	
}
