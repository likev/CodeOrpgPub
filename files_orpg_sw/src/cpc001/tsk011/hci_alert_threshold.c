/************************************************************************
 *	Module:	 hci_alert_threshold.c					*
 *									*
 *	Description:  This module is used by the ORPG HCI to edit	*
 *		      the alert threshold table.  It is a stand-alone	*
 *		      task which can be invoked from the HCI RPG	*
 *		      Control/Status window or by entering the command	*
 *		      "hci_alt" from the command line.			*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:50:38 $
 * $Id: hci_alert_threshold.c,v 1.88 2014/11/07 21:50:38 steves Exp $
 * $Revision: 1.88 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <alert_threshold.h>

/*	Macros.								*/

#define	TOP_WIDGET_MAX_WIDTH	925	
#define	PRODUCT_MENU_WIDTH	700	
#define	PRODUCT_CODE_MIN	15	/* Min prod code for alert pairing */
#define	PAIRING_MAX_WIDTH	128
#define	PAIRING_MAX_DISPLAY_WIDTH	70

/*	Global widget declarations					*/

Widget		Top_widget;
Widget		Form;
Widget		Data_scroll;
Widget		Label_rowcol;
int		Group          = 0;	/* ID of the currently active group */
int		Change_flag    = 0;	/* Set to 1 when unsaved edits are
					   detected; otherwise set to 0 */
int		Modify_flag    = 0; /* Edit flag */
int		Unlocked_loca = HCI_NO_FLAG; /* Treat URC/ROC LOCA the same */
int		Save_flag = HCI_NO_FLAG;
int		Undo_flag = HCI_NO_FLAG;
int		Restore_flag = HCI_NO_FLAG;
int		Update_flag = HCI_NO_FLAG;
int		Close_flag = HCI_NO_FLAG;
Widget		Lock_widget    = (Widget) NULL;
Widget		Bottom_label1   = (Widget) NULL;
Widget		Save_button    = (Widget) NULL;
Widget		Undo_button    = (Widget) NULL;
Widget		Restore_button = (Widget) NULL;
Widget		Update_button  = (Widget) NULL;

char	Buf [256];	/* Shared buffer for messages/text. */

int	Alt_updated_flag = 0;
int	Raise_flag = 0; /* Set to 1 forces window to be raised to top
			   of window heirarchy. */

void	alert_threshold_close_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	alert_threshold_group_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	alert_threshold_modify_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	alert_threshold_undo_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	accept_undo_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	alert_threshold_undo ();
void	alert_threshold_save_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	accept_alert_threshold_save_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	alert_threshold_save ();
void	acknowledge_invalid_value (Widget w, XtPointer client_data,
				XtPointer call_data);
void	change_paired_product_callback (Widget w, XtPointer client_data,
				XtPointer call_data);
void	alert_threshold_save_and_close (Widget w, XtPointer client_data,
			XtPointer call_data);
void	alert_threshold_nosave_and_close (Widget w, XtPointer client_data,
			XtPointer call_data);
void	alert_threshold_nosave (Widget w, XtPointer client_data,
			XtPointer call_data);
void	update_baseline_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	accept_update_baseline_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	update_baseline ();
void	restore_baseline_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	accept_restore_baseline_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	restore_baseline ();

void	alt_update_callback (int lbfd, LB_id_t msgid, int msglen, void *arg);

void	hci_display_alert_threshold_table (int group);

void	timer_proc ();

int	hci_alert_threshold_lock ();

/************************************************************************
 *	Description: This is the main function for the Alert Threshold	*
 *		     Editor task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments			*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	Widget	frame;
	Widget	button;
	Widget	label;
	Widget	control_rowcol;
	Widget	alert_threshold_group;
	Widget	control_form;
	int	i;
	int	n;
	Arg	arg [10];
	int	status;
	XmString	str;
	char		*group_name;

/*	Initialize HCI.						*/

	HCI_init( argc, argv, HCI_ALT_TASK );

	Top_widget = HCI_get_top_widget();

/*	Register for Alert data changes.				*/

	status = DEAU_UN_register ( ALERTING_DEA_NAME, alt_update_callback);

	if ( status < 0 )
	{
	  HCI_LE_error("DEAU_UN_register failed (%d)", status);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Build widgets.  Use a form widget as the manager.  Make its	*
 *	scope global so we can remanage it when we update components.	*/

	Form   = XtVaCreateWidget ("alert_threshold_form",
		xmFormWidgetClass,		Top_widget,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	If low bandwidth, display a progress meter			*/

	HCI_PM("Initialize Task Information");		

/*	At the top of the window place all of the control widgets and	*
 *	and place them inside a frame.					*/

	frame = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,		Form,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		NULL);

	control_form = XtVaCreateWidget ("control_form",
		xmFormWidgetClass,	frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	control_rowcol = XtVaCreateManagedWidget ("control_rowcol", 
		xmRowColumnWidgetClass,		control_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Define a button for closing the application.		*/

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNalignment,			XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, alert_threshold_close_callback, NULL);

/*	Define a button for saving edits.			*/

	Save_button = XtVaCreateManagedWidget ("Save",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Save_button,
		XmNactivateCallback, alert_threshold_save_callback, NULL);

/*	Define a button for undoing edits.			*/

	Undo_button = XtVaCreateManagedWidget ("Undo",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Undo_button,
		XmNactivateCallback, alert_threshold_undo_callback, NULL);

/*	Define a set of buttons for managing a backup copy of the	*
 *	alert threshold data.  One button for restoring from the backup	*
 *	and another for copying to it.					*/

	XtVaCreateManagedWidget ("  Baseline: ",
		xmLabelWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Restore_button = XtVaCreateManagedWidget ("Restore",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNsensitive,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Restore_button,
		XmNactivateCallback, restore_baseline_callback,
		NULL);

	Update_button = XtVaCreateManagedWidget ("Update",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNsensitive,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Update_button,
		XmNactivateCallback, update_baseline_callback,
		NULL);

/*	Define a set of radio buttons for controlling which alert	*
 *	threshold group is displayed.					*/

	XtVaCreateManagedWidget ("  Group: ",
		xmLabelWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;

	alert_threshold_group = XmCreateRadioBox (control_rowcol,
		"select_group", arg, n);
		
	HCI_PM("Read Alert/Threshold Data");		

/*	Initialize the active group to the first.		*/

	Group = ORPGALT_get_group_id (0);

/*	Get the group information from adaptation data.  None of the	*
 *	group information is hard-coded so that new groups can be	*
 *	added in the future without modifying this code.		*/

	for (i=0;i<ORPGALT_groups ();i++) {

	    group_name = ORPGALT_get_group_name (
				ORPGALT_get_group_id (i));
	    str = XmStringCreateLocalized (group_name);

	    button = XtVaCreateManagedWidget ("group",
		xmToggleButtonWidgetClass,	alert_threshold_group,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNtraversalOn,			False,
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	    XtAddCallback (button,
		XmNvalueChangedCallback, alert_threshold_group_callback,
		(XtPointer) ORPGALT_get_group_id (i));

	    if (i == 0) {

		XtVaSetValues (button,
			XmNlabelString,	str,
			XmNset,		True,
			NULL);

	    } else {

		XtVaSetValues (button,
			XmNlabelString,	str,
			XmNset,		False,
			NULL);

	    }

	    XmStringFree (str);

	}
	
	if (ORPGALT_io_status() < 0)
	    HCI_task_exit(HCI_EXIT_SUCCESS);

	XtManageChild (alert_threshold_group);

/*	Create a pixmap and drawn button for the window lock.		*/

	label = XtVaCreateManagedWidget ("  ",
		xmLabelWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create lock widget with appropriate LOCAs. */

	Lock_widget = hci_lock_widget( control_rowcol, hci_alert_threshold_lock, HCI_LOCA_ROC | HCI_LOCA_URC );

	XtManageChild (control_form);

/*	Next we need to create a scrolled window widget to contain the	*
 *	set of widgets for the table.					*/

	Data_scroll = XtVaCreateManagedWidget ("data_scroll",
		xmScrolledWindowWidgetClass,		Form,
		XmNheight,			260,
		XmNscrollingPolicy,		XmAUTOMATIC,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			frame,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	{
	    Widget	clip;
	    Widget	vsb;

	    XtVaGetValues (Data_scroll,
		XmNclipWindow,	&clip,
		XmNverticalScrollBar,	&vsb,
		NULL);

	    XtVaSetValues (clip,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    XtUnmanageChild (vsb);
	}

/*	Create a label widget beneath the table containing information	*
 *	pertinent to items in the table.				*/

	Bottom_label1 = XtVaCreateManagedWidget ("TVS: 1 = ETVS Detected, 2 = TVS Detected",
		xmLabelWidgetClass,	Form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Data_scroll,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (Form);

/*	Set max width value. Otherwise, spacing is messed up and Lock
	doesn't show up on 15" RPG KVM. */

	XtVaSetValues( Top_widget, XmNmaxWidth, TOP_WIDGET_MAX_WIDTH, NULL );

/*	Go ahead and build the table from data for the default group.	*/


	hci_display_alert_threshold_table (Group);
	
	/*  Assume any error means I/O has been cancelled
	    If cancel gets better, this should only test for
	    an error code of RMT_CANCELLED */
	if ((ORPGPAT_io_status() < 0) ||
	    (ORPGALT_io_status() < 0))
	    HCI_task_exit(HCI_EXIT_SUCCESS);

	XtPopup (Top_widget, XtGrabNone);
	
	/* This is executed twice because the form is sized incorrectly the first time -
	   The first time simply makes the I/O occur */
	hci_display_alert_threshold_table (Group);

	/* Start HCI loop. */

	HCI_start( timer_proc, HCI_HALF_SECOND, NO_RESIZE_HCI );	

	/* Out of HCI loop and ready to exit. */

	HCI_task_exit (HCI_EXIT_SUCCESS);

	return 0;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Save" button.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_256];

	sprintf( buf, "You are about to replace the alert/threshold\nadaptation data values. This will replace values\nfor all groups, and not just the active one.\n\nDo you want to continue?" );
        hci_confirm_popup( Top_widget, buf, accept_alert_threshold_save_callback, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the save confirmation window	*
 *		     after being invoked from the close callback.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_save_and_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Close/save pushed");
	Close_flag = HCI_YES_FLAG;
	accept_alert_threshold_save_callback( w, NULL, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the save confirmation window	*
 *		     after being invoked from the close callback.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_nosave_and_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Close/nosave pushed");
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the save confirmation window.*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_alert_threshold_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Save_flag = HCI_YES_FLAG;
}

void alert_threshold_save ()
{
	int	status;

	HCI_LE_log("Save pushed");

/*	Unset the change flag and desensitize the Save and Undo buttons	*/

	Change_flag = 0;

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

/*	If the window is currently unlocked for editing, sensitize the	*
 *	Restore and Update buttons.					*/

	if ( Unlocked_loca == HCI_YES_FLAG ) {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	True,
		NULL);
	
	} else {

/*	else, the window is locked so desensitize the Restore and	*
 *	Update buttons.							*/

	    XtVaSetValues (Restore_button,
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	False,
		NULL);

	}

	HCI_PM("Writing alert/threshold information");		

	status = ORPGALT_write ();

	if (status < 0) {

	    HCI_LE_error("ORPGALT_write returned error: %d", status);
            sprintf (Buf,"Unable to update Alert/Threshold data");

	}
	else {
	    HCI_LE_status("Alert/Threshold data updated" );
            sprintf (Buf,"Alert/Threshold data updated");
	}
	
	HCI_display_feedback( Buf );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_256];

/*	If no unsaved edits are detected, then exit.			*/

	if (!Change_flag) {

	    HCI_task_exit (HCI_EXIT_SUCCESS);

	} else {

/*	else, display a warning popup so the user has one last chance	*
 *	to save them.							*/

	    sprintf( buf, "You modified the alert threshold data\nbut did not save your changes.  Do you\nwant to save your changes?" );
            hci_confirm_popup( Top_widget, buf, alert_threshold_save_and_close, alert_threshold_nosave_and_close );
	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a "Group" radio button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - group ID					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_group_callback (
Widget		 w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	If the button state changed because it was set, get the new	*
 *	group ID from the widget client data and refresh the alert	*
 *	threshold table.						*/

	if (state->set) {

	    Group = (int) client_data;
	    hci_display_alert_threshold_table ((int) client_data);
	    XtManageChild (Form);

	}
}

/************************************************************************
 *	Description: This function is the timer procedure for the Alert	*
 *		     Threshold Editor window.  It ensures that the	*
 *		     window is updated without the input focus being	*
 *		     in the window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
/*	If Alert data has been changed, update widgets.			*/

        if( Alt_updated_flag )
        {
	  HCI_PM("Updating alert/threshold information");		
	  Alt_updated_flag = 0;
          hci_display_alert_threshold_table (Group);
        }

	if( Save_flag == HCI_YES_FLAG )
	{
	  Save_flag = HCI_NO_FLAG;
	  alert_threshold_save();
	}

	if( Undo_flag == HCI_YES_FLAG )
	{
	  Undo_flag = HCI_NO_FLAG;
	  alert_threshold_undo();
	}

	if( Restore_flag == HCI_YES_FLAG )
	{
	  Restore_flag = HCI_NO_FLAG;
	  restore_baseline();
	}

	if( Update_flag == HCI_YES_FLAG )
	{
	  Update_flag = HCI_NO_FLAG;
	  update_baseline();
	}

	if( Close_flag == HCI_YES_FLAG )
	{
	  Close_flag = HCI_NO_FLAG;
	  HCI_task_exit( HCI_EXIT_SUCCESS );
	}

/*	If we selected something from a drop down menu, raise the	*
 *	window to the top of the window heirarchy in case it extended	*
 *	beyond the window border.					*/

	if (Raise_flag)
	{
	    XRaiseWindow (HCI_get_display(), HCI_get_window());
	    Raise_flag = 0;
	}
}

/************************************************************************
 *	Description: This function is used to build and display the	*
 *		     alert threshold table for the specified group.	*
 *									*
 *	Input: 	group - group ID					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_display_alert_threshold_table (
int	group
)
{
	int	first_time;
	int	alert_type;
	int	paired_type;
	int	i, j;
	int	id;
	int	num;
	int	items;
	int	buf_num;
	char	*string;
	char	buf [128];
	char	prod_name [PAIRING_MAX_WIDTH+1];
	Dimension	height;
	Widget	product_menu;
	Widget	product_list;
	Widget	product_text;
	int	n;
	int	prod_code;
	int	len;
	Widget	text;
	Widget	table_rowcol = (Widget) NULL;
static	Widget	form = (Widget) NULL;
	int	shadow;
	int	margin;
static	int	edit_flag = False;
	int	foreground;
	int	background;
	int	pos;
	XmString	str;

/*	If the table has been previously displayed destroy it since	*
 *	we are going to build a new one.				*/

	if (form != (Widget) NULL) {

	Widget	vsb;

	XtVaGetValues (Data_scroll,
			XmNhorizontalScrollBar, &vsb,
			NULL);

	XtVaSetValues (vsb,
			XmNvalue, 0,
			NULL);

	    XtDestroyWidget (form);

	}

/*	Temporarily relax the window size limits so we can build a	*
 *	new table.  Later, we will set new limits so the window 	*
 *	can't be resized.						*/

	XtVaSetValues (Top_widget,
		XmNmaxHeight,	1024,
		XmNminHeight,	10,
		NULL);

	XtVaSetValues (Bottom_label1,
		XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	form = XtVaCreateWidget ("table_form",
		xmFormWidgetClass,	Data_scroll,
		XmNbackground,		hci_get_read_color (BLACK),
		NULL);

/*	Next we are going to build the widgets for the alert threshold	*
 *	table.  At the top we need to create a set of labels defining	*
 *	the columns in the table.					*/

	Label_rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		NULL);

	text = XtVaCreateManagedWidget (" Category",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		17,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf,"Category");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Units",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Units");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Min",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf,"Min");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget (" Max",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		7,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Max");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Th1",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Th1");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Th2",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Th2");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Th3",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Th3");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Th4",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Th4");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Th5",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Th5");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Th6",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		6,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Th6");
	XmTextSetString (text, buf);

	text = XtVaCreateManagedWidget ("Paired Product",
		xmTextWidgetClass,	Label_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		PAIRING_MAX_DISPLAY_WIDTH,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (buf," Paired Product");
	XmTextSetString (text, buf);

	XtManageChild (Label_rowcol);

	first_time = 1;
	items      = 0;

/*	Define the foreground/background colors to use for the	*
 *	threshold data.  If the table is locked by another user	*
 *	or user doesn't have proper change authority, show the	*
 *	data as read only.					*/

	if ( Unlocked_loca == HCI_YES_FLAG ) {

	    if (!edit_flag) {

		if (Change_flag) {

		    XtVaSetValues (Restore_button,
			XmNsensitive,	False,
			NULL);
		    XtVaSetValues (Update_button,
			XmNsensitive,	False,
			NULL);

		} else {

		    XtVaSetValues (Restore_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Update_button,
			XmNsensitive,	True,
			NULL);

		}

	    }

	    foreground = hci_get_read_color (EDIT_FOREGROUND);
	    background = hci_get_read_color (EDIT_BACKGROUND);
	    edit_flag  = True;
	    shadow     = 2;
	    margin     = 0;

	} else {

	    if (hci_lock_URC_selected() || hci_lock_ROC_selected()) {

		foreground = hci_get_read_color (LOCA_FOREGROUND);
		background = hci_get_read_color (LOCA_BACKGROUND);

	    } else {

		foreground = hci_get_read_color (EDIT_FOREGROUND);
		background = hci_get_read_color (BACKGROUND_COLOR1);

	    }

	    edit_flag = False;
	    shadow    = 0;
	    margin    = 2;

	}

/*	For each defined category, create a table entry if it is in	*
 *	the input group.						*/

	for (i=0;i<ORPGALT_categories ();i++) {

	    id = ORPGALT_get_category (i);

	    if ((id > 0) &&
		(ORPGALT_get_group (id) == group)) {

		items++;

/*		If this is the first entry, we want it to be attached	*
 *		to the top of the scrolled window.			*/

		if (first_time) {

		    table_rowcol = XtVaCreateWidget ("table_rowcol",
			xmRowColumnWidgetClass,	form,
			XmNbackground,		hci_get_read_color (BLACK),
			XmNorientation,		XmHORIZONTAL,
			XmNpacking,		XmPACK_TIGHT,
			XmNnumColumns,		1,
			XmNspacing,		0,
			XmNmarginHeight,	0,
			XmNmarginWidth,		0,
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		Label_rowcol,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNentryAlignment,	XmALIGNMENT_BEGINNING,
			NULL);

		    first_time = 0;

		} else {

/*		else, we want it attached to the entry defined before	*
 *		it.							*/

		    table_rowcol = XtVaCreateWidget ("table_rowcol",
			xmRowColumnWidgetClass,	form,
			XmNbackground,		hci_get_read_color (BLACK),
			XmNorientation,		XmHORIZONTAL,
			XmNpacking,		XmPACK_TIGHT,
			XmNnumColumns,		1,
			XmNspacing,		0,
			XmNmarginHeight,	0,
			XmNmarginWidth,		0,
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		table_rowcol,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNentryAlignment,	XmALIGNMENT_BEGINNING,
			NULL);

		}

		string = ORPGALT_get_name (id);
		strcpy (buf,string);


/*		If this group contains a TVS, make the info label	*
 *		at the bottom of the window visible.			*/

		if (strstr (buf,"TVS") != NULL) {

		    XtVaSetValues (Bottom_label1,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			NULL);

		}

		text = XtVaCreateManagedWidget ("Category",
			xmTextWidgetClass,	table_rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
			XmNcolumns,		16,
			XmNmarginHeight,	2,
			XmNshadowThickness,	shadow,
			XmNmarginWidth,		margin,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (text, buf);

		string = ORPGALT_get_unit (id);
		strcpy (buf,string);

		text = XtVaCreateManagedWidget ("Units",
			xmTextWidgetClass,	table_rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
			XmNcolumns,		10,
			XmNmarginHeight,	2,
			XmNshadowThickness,	shadow,
			XmNmarginWidth,		margin,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (text, buf);

		num = ORPGALT_get_min (id);
		sprintf (buf,"%3d",num);

		text = XtVaCreateManagedWidget ("Min",
			xmTextWidgetClass,	table_rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
			XmNcolumns,		6,
			XmNmarginHeight,	2,
			XmNshadowThickness,	shadow,
			XmNmarginWidth,		margin,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (text, buf);

		num = ORPGALT_get_max (id);
		sprintf (buf,"%3d",num);

		text = XtVaCreateManagedWidget ("Max",
			xmTextWidgetClass,	table_rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
			XmNcolumns,		6,
			XmNmarginHeight,	2,
			XmNshadowThickness,	shadow,
			XmNmarginWidth,		margin,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (text, buf);

		for (j=1;j<=6;j++) {

		    if (j <= ORPGALT_get_thresholds (id)) {

			num = ORPGALT_get_threshold (id, j);
			sprintf (buf,"%3d ",num);

			text = XtVaCreateManagedWidget ("Thx",
				xmTextWidgetClass,	table_rowcol,
				XmNbackground,		background,
				XmNforeground,		foreground,
				XmNcolumns,		6,
				XmNmarginHeight,	2,
				XmNshadowThickness,	shadow,
				XmNmarginWidth,		margin,
				XmNborderWidth,		0,
				XmNuserData,		(XtPointer) id,
				XmNfontList,		hci_get_fontlist (LIST),
				XmNeditable,		edit_flag,
				XmNtraversalOn,		edit_flag,
				NULL);

			XtAddCallback (text,
				XmNmodifyVerifyCallback,
				hci_verify_signed_integer_callback,
				(XtPointer) 3);

			XtAddCallback (text,
				XmNfocusCallback,
				hci_gain_focus_callback,
				(XtPointer) j);

			XtAddCallback (text,
				XmNlosingFocusCallback,
				alert_threshold_modify_callback,
				(XtPointer) j);

			XtAddCallback (text,
				XmNactivateCallback,
				alert_threshold_modify_callback,
				(XtPointer) j);

			XmTextSetString (text, buf);

		    } else {

			text = XtVaCreateManagedWidget ("Thx",
				xmTextWidgetClass,	table_rowcol,
				XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
				XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
				XmNcolumns,		6,
				XmNmarginHeight,	2,
				XmNshadowThickness,	0,
				XmNmarginWidth,		2,
				XmNborderWidth,		0,
				XmNfontList,		hci_get_fontlist (LIST),
				XmNeditable,		False,
				XmNtraversalOn,		False,
				NULL);

		    }
		}

/*		If the user has at least URC level clearance, then	*
 *		build a menu of all products which can be paired with	*
 *		this category.						*/

		if ( Unlocked_loca == HCI_YES_FLAG ) {

		    product_menu = XtVaCreateWidget ("prd",
			xmComboBoxWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (EDIT_FOREGROUND),
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNfontList,	hci_get_fontlist (LIST),
			XmNcomboBoxType,	XmDROP_DOWN_LIST,
			XmNwidth,		PRODUCT_MENU_WIDTH,
			XmNborderWidth,		0,
			XmNmarginHeight,	0,
			XmNuserData,		(XtPointer) id,
			XmNvisibleItemCount,	24,
			XmNlayoutDirection,	XmRIGHT_TO_LEFT,
			NULL);

		    XtVaGetValues (product_menu,
			XmNlist,	&product_list,
			XmNtextField,	&product_text,
			NULL);

		    XtVaSetValues (product_list,
			XmNuserData,		(XtPointer) id,
			XmNforeground,	hci_get_read_color (EDIT_FOREGROUND),
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNstringDirection,	XmSTRING_DIRECTION_L_TO_R,
			NULL);

		    XtVaSetValues (product_text,
			XmNuserData,		(XtPointer) id,
			XmNforeground,	hci_get_read_color (EDIT_FOREGROUND),
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			NULL);

		    paired_type = ORPGALT_get_type (id);

		    num = ORPGALT_get_prod_code (id);
		    sprintf (buf,"Product %d", num);

/*		    Build the menu of valid products which can be	*
 *		    paired with this category of alert.			*/

/*		    Go through each product defined in the product	*
 *		    attributes table.					*/

		    n = 0;
                    pos = -1;

		    for (prod_code = PRODUCT_CODE_MIN+1;
			 prod_code < ORPGPAT_MAX_PRODUCT_CODE;
			 prod_code++) {

			buf_num = ORPGPAT_get_prod_id_from_code (prod_code);

/*			If the product is a valid distributable product	*
 *			then continue.					*/

			if( (buf_num != ORPGPAT_ERROR) && (buf_num != 0) ){

/*			    Get the alert type tag associated with the	*
 *			    product.					*/

			    alert_type = ORPGPAT_get_alert (buf_num);

/*			    If the alert tag is allowed for this	*
 *			    category, then add item to menu.		*/

			    if ((alert_type&paired_type)) {

			        string  = ORPGPAT_get_description (buf_num, STRIP_NOTHING);
			        len = sprintf (prod_name,"[%2d] - %s", prod_code, string);
			        memset (&prod_name[len], ' ', PAIRING_MAX_WIDTH - len);
			        prod_name[PAIRING_MAX_WIDTH] = '\0';

				str = XmStringCreateLocalized (prod_name);
				XmListAddItemUnselected (product_list, str, 0);
				XmStringFree (str);

			        if (num == prod_code) {

				    pos = n;

				}

				n++;

			    }
		        }
		    }

                    if( pos >= 0 )
			XtVaSetValues (product_menu,
				XmNselectedPosition,	pos,
			        NULL);

		    XtAddCallback (product_menu,
			    XmNselectionCallback, change_paired_product_callback,
			    NULL);

		    XtManageChild (product_menu);

/*		The user doesn't have the proper change level so only	*
 *		display the paired product (Non-editable).		*/

		} else {

		    num = ORPGALT_get_prod_code (id);

		    for (j=0;j<ORPGPAT_num_tbl_items ();j++) {

			buf_num   = ORPGPAT_get_prod_id (j);
		        prod_code = ORPGPAT_get_code (buf_num);

			if (num == prod_code) {

                            alert_type = ORPGPAT_get_alert (buf_num);
		            paired_type = ORPGALT_get_type (id);

/*                          If the alert tag is allowed for this        *
 *                          category, then display product info         */

                            len = 0;
                            if ((alert_type&paired_type)) {
 
		 		string  = ORPGPAT_get_description (buf_num, STRIP_NOTHING);
				len = sprintf (prod_name,"[%2d] - %s", prod_code, string);

                            }

			    memset (&prod_name[len], ' ', PAIRING_MAX_WIDTH - len);
			    prod_name[PAIRING_MAX_WIDTH] = '\0';

			    text = XtVaCreateManagedWidget ("Thx",
				xmTextWidgetClass,	table_rowcol,
				XmNbackground,		background,
				XmNforeground,		foreground,
				XmNcolumns,		PAIRING_MAX_DISPLAY_WIDTH,
				XmNmarginHeight,	2,
				XmNshadowThickness,	0,
				XmNmarginWidth,		2,
				XmNborderWidth,		0,
				XmNfontList,		hci_get_fontlist (LIST),
				XmNeditable,		False,
				XmNtraversalOn,		False,
				NULL);

			    XmTextSetString (text, prod_name);

			    break;

			}
		    }
		}

		XtManageChild (table_rowcol);

	    }
	}

	XtManageChild (form);
	XtManageChild (Data_scroll);
	XtManageChild (Form);

	XtVaGetValues (table_rowcol,
		XmNheight,	&height,
		NULL);

	height = height*(items+2) + 6;

	XtVaSetValues (Data_scroll,
		XmNheight,	height,
		NULL);

	XtVaGetValues (Top_widget,
		XmNheight,	&height,
		NULL);

	XtVaSetValues (Top_widget,
		XmNminHeight,	height,
		XmNmaxHeight,	height,
		NULL);

}

/************************************************************************
 *	Description: This function is called when an item in the table	*
 *		     loses focus or when the user presses the return	*
 *		     key while the focus is over one of the table items.*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - row element				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_modify_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtPointer	data;
	int		indx;
	int		item;
	char		*string;
	int		value;
	int		min;
	int		max;

/*	If this module was called because of a XmTextSetString() call	*
 *	within this module, do nothing and return.			*/

	if (Modify_flag) {

	    return;

	}

	Modify_flag = 1;

	XtVaGetValues (w,
		XmNuserData,	&data,
		NULL);
		
	indx = (int) data;
	item = (int) client_data;

/*	Determine the minimum and maximum values allowed for the item.	*/

	if (item == 1) {

	    min = ORPGALT_get_min (indx);

	} else {

	    min = ORPGALT_get_threshold (indx, item-1)+1;

	}

	if (item == ORPGALT_get_thresholds (indx)) {

	    max = ORPGALT_get_max (indx);

	} else {

	    max = ORPGALT_get_threshold (indx, item+1)-1;

	}

/*	Get the new value and decode it.			*/

	string = XmTextGetString (w);

	if (strlen (string) && strcmp (string," ")) {

	    sscanf (string,"%d",&value);

	} else {

	    value = ORPGALT_get_threshold (indx, item);
	    sprintf (Buf,"%3d ",value);
	    XmTextSetString (w, Buf);
	    XtFree (string);
	    Modify_flag = 0;
	    return;

	}

/*	Validate the new value.  If it is out of range, display a	*
 *	warning popup and reset it to its original value.		*/

	if ((value > max) ||
	    (value < min)) {

	    sprintf (Buf,
		"You entered an invalid value of %d!\nThe valid range is %d to %d\n",
		value,
	        min,
	        max);

            hci_warning_popup( Top_widget, Buf, NULL );

	    value = ORPGALT_get_threshold (indx, item);

	} else {

	    if (value != ORPGALT_get_threshold (indx, item)) {

		ORPGALT_set_threshold (indx, item, value);

		if (!Change_flag) {

		    XtVaSetValues (Save_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Undo_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Restore_button,
			XmNsensitive,	False,
			NULL);
		    XtVaSetValues (Update_button,
			XmNsensitive,	False,
			NULL);

		    Change_flag = 1;

		}
	    }
	}

	sprintf (Buf,"%3d ", value);
	XmTextSetString (w, Buf);
	XtFree (string);

	Modify_flag = 0;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     "No" from a warning popup window.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
acknowledge_invalid_value (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Undo" button.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_undo_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char buf[HCI_BUF_256];

  /* If this callback is from the "Undo" button, then display
     a confirmation popup. If not, then assume the confirmation
     has already been done. */

  if( w == Undo_button )
  {
    sprintf( buf, "You are about to undo any unsaved edits to\nthe alert/threshold adaptation data values.\nThis will undo unsaved edits for all groups,\nand not just the active one.\n\nDo you want to continue?" );
    hci_confirm_popup( Top_widget, buf, accept_undo_callback, NULL );
  }
  else
  {
	Undo_flag = HCI_YES_FLAG;
  }
}

void
accept_undo_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Undo_flag = HCI_YES_FLAG;
}

void alert_threshold_undo ()
{
	int	status;
	
	HCI_PM("Restoring alert/threshold information");		
		
	status = ORPGALT_read ();

	if (status >= 0) {


	    hci_display_alert_threshold_table (Group);

	}

	Change_flag = 0;

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

/*	If the window is currently unlocked for editing, sensitize the	*
 *	Restore and Update buttons.					*/

	if ( Unlocked_loca == HCI_YES_FLAG ) {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	True,
		NULL);
	
	} else {

/*	else, the window is locked so desensitize the Restore and	*
 *	Update buttons.							*/

	    XtVaSetValues (Restore_button,
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	False,
		NULL);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     an item from the paired product menu.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - product code				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
change_paired_product_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtPointer	data;
	char		*text;
	int		prod_code;
	Widget		combo_text;
	XmComboBoxCallbackStruct *cbs =
		(XmComboBoxCallbackStruct *) call_data;

	XtVaGetValues (w,
		XmNuserData,	&data,
		NULL);

	if (cbs->event != NULL) {

	    Raise_flag = 1;

/*	Set the new paired product code.			*/

	    XtVaGetValues (w,
		XmNtextField,	&combo_text,
		NULL);

	    text = XmTextGetString (combo_text);

	    sscanf (text+1,"%d",&prod_code);

	    XtFree (text);

	    ORPGALT_set_prod_code ((int) data, prod_code);

	    if (!Change_flag) {

		XtVaSetValues (Save_button,
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Undo_button,
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Restore_button,
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Update_button,
			XmNsensitive,	False,
			NULL);

		Change_flag = 1;

	    }
	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the lock icon button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
hci_alert_threshold_lock ()
{

  char buf[HCI_BUF_256];

  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_URC_unlocked() || hci_lock_ROC_unlocked() )
    {
      Unlocked_loca = HCI_YES_FLAG;
      if( ORPGALT_get_edit_status() == ORPGALT_EDIT_LOCKED )
      {
        sprintf( Buf, "Another user is currently editing alert\nthreshold adaptation data. Any changes\nmay be overwritten by the other user.");
        hci_warning_popup( Top_widget, Buf, NULL );
      }
      /* Set advisory lock. */
      ORPGALT_set_edit_lock();
    }
  }
  else if( hci_lock_close() && Unlocked_loca == HCI_YES_FLAG )
  {
    Unlocked_loca = HCI_NO_FLAG;

    if( Change_flag  )
    {
      /* Prompt the user to save any unsaved edits. */
      sprintf( buf, "You modified the alert threshold data\nbut did not save your changes.  Do you\nwant to save your changes?" );
      hci_confirm_popup( Top_widget, buf, accept_alert_threshold_save_callback, alert_threshold_nosave );

      Change_flag = 0;

      XtVaSetValues( Save_button, XmNsensitive, False, NULL );
      XtVaSetValues( Undo_button, XmNsensitive, False, NULL );
    }

    ORPGALT_clear_edit_lock();
    XtVaSetValues( Restore_button, XmNsensitive, False, NULL );
    XtVaSetValues( Update_button, XmNsensitive, False, NULL );
  }

  hci_display_alert_threshold_table (Group);

  return HCI_LOCK_PROCEED;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Save confirmation popup	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
alert_threshold_nosave (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	alert_threshold_undo_callback( w, NULL, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Restore" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
restore_baseline_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_256];

	sprintf( buf, "You are about to restore the alert/threshold\nadaptation data to baseline values. This will\nrestore all groups, and not just the active one.\n\nDo you want to continue?" );
        hci_confirm_popup( Top_widget, buf, accept_restore_baseline_callback, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the Restore confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_restore_baseline_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  Restore_flag = HCI_YES_FLAG;
}

void restore_baseline ()
{
	int	status;

        status = ORPGALT_restore_baseline();

	if (status < 0) {

                HCI_LE_error("restore baseline failed (%d)", status);

        } else {

            /*  Read the new threshold information into the ORPGALT api */
            status = ORPGALT_read();
            if (status < 0)
            {
              HCI_LE_error("Unable to read alert thresholds after restore from baseline, status = %d", status);
            }
            else
            {
                /*    Post an event so that everyone else will know that this     *
                 *    data store has been updated.                                */
                EN_post (ORPGEVT_WX_ALERT_ADAPT_UPDATE, NULL, 0, 0);
            }
            if (status >= 0)
            {
                HCI_LE_log("Alert/Threshold data restored from baseline");
            }
            hci_display_alert_threshold_table (Group);
        }

	hci_display_alert_threshold_table (Group);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Update" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
update_baseline_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_256];

	sprintf( buf, "You are about to replace the baseline alert/threshold\nadaptation data values. This will replace values for\nall groups, and not just the active one.\n\nDo you want to continue?" );
        hci_confirm_popup( Top_widget, buf, accept_update_baseline_callback, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the Update confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_update_baseline_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  Update_flag = HCI_YES_FLAG;
}

void update_baseline ()
{
	int	status;

        status = ORPGALT_update_baseline();

        if (status < 0) {

            HCI_LE_error("Unable to update ORPGALT_BASELINE_CATEGORY_MSG_ID");
            sprintf (Buf,"Unable to update baseline Alert/Threshold data");

        } else {

            HCI_LE_status("Alert/Threshold baseline data updated");
            sprintf (Buf,"Alert/Threshold baseline data updated");

        }

	HCI_display_feedback( Buf );
}

/************************************************************************
 *	Description: Callback for changes to Alert DEA data.		*
 ************************************************************************/

void alt_update_callback (int lbfd, LB_id_t msgid, int msglen, void *arg)
{
  Alt_updated_flag = 1;
}
