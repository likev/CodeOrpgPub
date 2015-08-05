/************************************************************************
 *	Module:	 hci_RPG_products_button.c				*
 *									*
 *	Description:  This module is used by the ORPG HCI to define	*
 *		      and display the RPG Products menu which is	*
 *		      activated by selecting the "Products" button in	*
 *		      the RPG container of the RPG Control/Status	*
 *		      window.						*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:45 $
 * $Id: hci_RPG_products_button.c,v 1.15 2009/02/27 22:25:45 ccalvert Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/*	Local include files.						*/

#include <hci_control_panel.h>

/*	Global static variables.						*/
static	Widget	Rpg_products_dialog = (Widget) NULL;
			/* Widget ID of RPG Products window parent */

void	rpg_products_button_close (Widget w,
		XtPointer client_data, XtPointer call_data);

/************************************************************************
 *	Description: This is the callback for the RPG "Products"	*
 *		     pushbutton in the RPG Control/Status window.	*
 *									*
 *	Input:  w - Widget ID of "Products" pushbutton.			*
 *	        client_data - *unused*					*
 *	        call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_RPG_products_button (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	button;
	Widget	control_rowcol;
	Widget	display_frame;
	Widget	display_rowcol;
	Widget	edit_frame;
	Widget	edit_rowcol;
	Widget	form;
	Widget	label;

/*	Do not allow more than one RPG products menu to exist at one	*
 *	time.								*/

	if (Rpg_products_dialog != NULL)
	{
/*	    The RPG Products window already exist so bring it to the	*
 *	    top of the window heirarchy.				*/

	    HCI_Shell_popup( Rpg_products_dialog );
	    return;
	}

	HCI_LE_log("RPG Products button selected");

/*	Define the widgets for the RPG menu and display them.		*/

	HCI_Shell_init( &Rpg_products_dialog, "RPG Products" );

/*	Use a form widget to organize the various menu widgets.		*/

	form   = XtVaCreateWidget ("form",
		xmFormWidgetClass,		Rpg_products_dialog,
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
		XmNactivateCallback, rpg_products_button_close, NULL);

	XtManageChild (control_rowcol);

/*	Since we have two types of buttons in this menu we will use	*
 *	frames to contain them.						*/

/*	Organize product adaptation data selections in the first	*
 *	group.								*/

	edit_frame   = XtVaCreateManagedWidget ("edit_frame",
		xmFrameWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			control_rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Adaptation Data",
		xmLabelWidgetClass,	edit_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	edit_rowcol = XtVaCreateWidget ("products_rowcol",
		xmRowColumnWidgetClass,	edit_frame,
		XmNorientation,		XmVERTICAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Add selection to activate Alert/Threshold editor.		*/

	button = XtVaCreateManagedWidget ("Alert/Threshold",
		xmPushButtonWidgetClass,edit_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) ALERTS_BUTTON);

/*	Add selection to activate Product Generation List editor.	*/

	button = XtVaCreateManagedWidget ("Generation List",
		xmPushButtonWidgetClass,edit_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) RPG_PRODUCTS_BUTTON);

/*	Add selection to activate Selectable Product Parameters editor.	*/

	button = XtVaCreateManagedWidget ("  Selectable Parameters  ",
		xmPushButtonWidgetClass,edit_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) PRODUCT_PARAMETERS_BUTTON);

/*	Add selection to activate Algorithms Adaptation Data editor.	*/
		
	button = XtVaCreateManagedWidget ("Algorithms",
		xmPushButtonWidgetClass,edit_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) RPG_ALGORITHMS_BUTTON);
		

	XtManageChild (edit_rowcol);

/*	Organize product status selections in the second group.		*/

	display_frame   = XtVaCreateManagedWidget ("display_frame",
		xmFrameWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			edit_frame,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Display",
		xmLabelWidgetClass,	display_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	display_rowcol = XtVaCreateWidget ("display_rowcol",
		xmRowColumnWidgetClass,	display_frame,
		XmNorientation,		XmVERTICAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Add selection to activate Products in Database display task.	*/

	button = XtVaCreateManagedWidget ("Products in Database",
		xmPushButtonWidgetClass,display_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) PRODUCT_STATUS_BUTTON);

	XtManageChild (display_rowcol);

	XtManageChild (form);

	HCI_Shell_start( Rpg_products_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This callback is activated when the RPG Products 	*
 *		     "Close" button is selected.			*
 *									*
 *	Input:  w - Widget ID of "Close" button.			*
 *	        client_data - *unused*					*
 *	        call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rpg_products_button_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("RPG Products Close selected");
	HCI_Shell_popdown( Rpg_products_dialog );
}
