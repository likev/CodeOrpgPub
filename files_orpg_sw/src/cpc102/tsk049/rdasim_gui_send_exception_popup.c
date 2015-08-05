/************************************************************************
 *	Module:	 rdasim_send_exception_popup.c				*
 *									*
 *	Description:  This module is used by the rda simulator gui to	*
 *		      define and display a popup that will allow the	*
 *		      the user to interact with the rda simulator by	*
 *		      sending exception.				*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/31 17:59:06 $
 * $Id: rdasim_gui_send_exception_popup.c,v 1.3 2014/07/31 17:59:06 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Local include files.						*/

#include <hci.h>
#include <rdasim_simulator.h>

/*	Local definitions.	*/

#define	NUMBER_OF_EXCEPTIONS	15

/*	Global static variables.	*/

static	Widget	send_exception_popup_dialog = (Widget) NULL;
static	Widget	rda_alarm_start_number_user_input = (Widget) NULL;
static	Widget	rda_alarm_end_number_user_input = (Widget) NULL;
static	Widget	rda_alarm_set_clear_user_input = (Widget) NULL;
static	int	lbfd = -1;
static	char	parameters_to_send[ LB_PARAMETER_LENGTH ];

/*	Function prototypes.	*/

void	send_exception_popup_close (Widget w,
		XtPointer client_data, XtPointer call_data);
void	send_exception (Widget w,
		XtPointer client_data, XtPointer call_data);
void	send_exception_rda_alarms (Widget w,
		XtPointer client_data, XtPointer call_data);
void	error_with_LB( char *lb_fx_name, int error_code );
void	bad_input( char *section_name );

/************************************************************************
 *	Description: This is the callback for the "send exception"	*
 *		     pushbutton in the rda simulator gui.		*
 *									*
 *	Input:  w - Widget ID of "send exception" pushbutton.		*
 *	        client_data - file descriptor of LB			*
 *	        call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_exception_popup(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget		control_rowcol; /* Holds close button. */
	Widget		exception_frame;  /* Frame to hold exception_rowcol. */
	Widget		exception_rowcol; /* Rowcol to hold exceptions. */
	Widget		row1_rowcol;	/* One rowcol per exception so */
	Widget		row2_rowcol;    /* the spacing works out.    */
	Widget		row21_rowcol;   /* " */
	Widget		row3_rowcol;    /* " */
	Widget		row4_rowcol;    /* " */
	Widget		row5_rowcol;    /* " */
	Widget		row6_rowcol;    /* " */
	Widget		row7_rowcol;    /* " */
	Widget		row8_rowcol;    /* " */
	Widget		row9_rowcol;    /* " */
	Widget		row10_rowcol;   /* " */
	Widget		row11_rowcol;   /* " */
	Widget		row12_rowcol;   /* " */
	Widget		row13_rowcol;   /* " */
	Widget		row14_rowcol;   /* " */
	Widget		button;
	Widget		form;
	Widget		label;

/*	Set LB file descriptor.		*/

	lbfd = ( int ) client_data;

/*	Do not allow more than one send exception popup to exist at a	*
 *	time.								*/

	if (send_exception_popup_dialog != NULL)
	{
/*	  The send exception window already exist so bring it to the	*
 *	  top of the window heirarchy.				*/

	  HCI_Shell_popup( send_exception_popup_dialog );
	  return;
	}

/*	Create the top-level widget.					*/

	HCI_Shell_init( &send_exception_popup_dialog, "Send Exception" );

/*	Use a form widget to organize the various menu widgets.		*/

	form   = XtVaCreateWidget ("form",
		xmFormWidgetClass,		send_exception_popup_dialog,
		XmNautoUnmanage,		False,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	control_rowcol = XtVaCreateWidget ("control_rowcol",
		xmRowColumnWidgetClass,		form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
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
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception_popup_close, NULL);

	XtManageChild (control_rowcol);

/*	Create frame to hold exception rowcols.	*/

	exception_frame   = XtVaCreateManagedWidget ("exception_frame",
		xmFrameWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			control_rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Exceptions to Send",
		xmLabelWidgetClass,	exception_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	exception_rowcol = XtVaCreateWidget ("products_rowcol",
		xmRowColumnWidgetClass,	exception_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		NUMBER_OF_EXCEPTIONS,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create a rowcol for each exception, add button/label	*
 *	to each rowcol.						*/

	row1_rowcol = XtVaCreateManagedWidget( "row1_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row1_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) FAT_RADIAL_FORWARD );

	label = XtVaCreateManagedWidget ("Forward FAT Radial",
		xmLabelWidgetClass,	row1_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row1_rowcol);

	row2_rowcol = XtVaCreateManagedWidget( "row2_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row2_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) FAT_RADIAL_BACK );

	label = XtVaCreateManagedWidget ("Backward FAT Radial",
		xmLabelWidgetClass,	row2_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row2_rowcol);

        row21_rowcol = XtVaCreateManagedWidget( "row21_rowcol",
                xmRowColumnWidgetClass, exception_rowcol,
                XmNorientation,         XmHORIZONTAL,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                XmNentryAlignment,      XmALIGNMENT_CENTER,
                NULL);

        button = XtVaCreateManagedWidget ("Send",
                xmPushButtonWidgetClass,        row21_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNactivateCallback, send_exception,
                (XtPointer) NEGATIVE_START_ANGLE );

        label = XtVaCreateManagedWidget ("Negative Start Angle",
                xmLabelWidgetClass,     row21_rowcol,
                XmNchildType,           XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment,    XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment,      XmALIGNMENT_CENTER,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

        XtManageChild (row21_rowcol);

	row3_rowcol = XtVaCreateManagedWidget( "row3_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row3_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) BAD_ELEVATION_CUT );

	label = XtVaCreateManagedWidget ("Bad Elevation Cut",
		xmLabelWidgetClass,	row3_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row3_rowcol);

	row4_rowcol = XtVaCreateManagedWidget( "row4_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row4_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) MAX_RADIAL );

	label = XtVaCreateManagedWidget ("Bad Radial (Over 400)",
		xmLabelWidgetClass,	row4_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row4_rowcol);

	row5_rowcol = XtVaCreateManagedWidget( "row5_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row5_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) UNEXPECTED_BEGINNING_ELEVATION );

	label = XtVaCreateManagedWidget ("Unexpected Beginning of Elevation",
		xmLabelWidgetClass,	row5_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row5_rowcol);

	row6_rowcol = XtVaCreateManagedWidget( "row6_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row6_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) UNEXPECTED_BEGINNING_VOLUME );

	label = XtVaCreateManagedWidget ("Unexpected Beginning of Volume",
		xmLabelWidgetClass,	row6_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row6_rowcol);

	row7_rowcol = XtVaCreateManagedWidget( "row7_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row7_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) BAD_SEGMENT_BYPASS );

	label = XtVaCreateManagedWidget ("Send a Bad Segment Number for Bypass Maps",
		xmLabelWidgetClass,	row7_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row7_rowcol);

	row8_rowcol = XtVaCreateManagedWidget( "row8_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row8_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) BAD_SEGMENT_NOTCHWIDTH );

	label = XtVaCreateManagedWidget ("Send a Bad Segment Number for Notchwidth Map",
		xmLabelWidgetClass,	row8_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row8_rowcol);

	row9_rowcol = XtVaCreateManagedWidget( "row9_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row9_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) BAD_START_VCP );

	label = XtVaCreateManagedWidget ("Send a Bad Startup Vcp",
		xmLabelWidgetClass,	row9_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row9_rowcol);

	row10_rowcol = XtVaCreateManagedWidget( "row10_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row10_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) IGNORE_VOLUME_ELEVATION_RESTART );

	label = XtVaCreateManagedWidget ("Ignore a Volume/Elevation Restart",
		xmLabelWidgetClass,	row10_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row10_rowcol);

	row11_rowcol = XtVaCreateManagedWidget( "row11_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row11_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) SKIP_START_OF_VOLUME_MSG );

	label = XtVaCreateManagedWidget ("Skip the Start of Volume Msg",
		xmLabelWidgetClass,	row11_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row11_rowcol);

	row12_rowcol = XtVaCreateManagedWidget( "row12_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row12_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) LOOPBACK_TIMEOUT );

	label = XtVaCreateManagedWidget ("Cause RPG/RDA Loopback Test Timeout",
		xmLabelWidgetClass,	row12_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row12_rowcol);

	row13_rowcol = XtVaCreateManagedWidget( "row13_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row13_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception,
		(XtPointer) LOOPBACK_SCRAMBLE );

	label = XtVaCreateManagedWidget ("Cause RPG/RDA Loopback Test Scramble",
		xmLabelWidgetClass,	row13_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row13_rowcol);

	row14_rowcol = XtVaCreateManagedWidget( "row14_rowcol",
		xmRowColumnWidgetClass,	exception_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row14_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_exception_rda_alarms,
		(XtPointer) RDA_ALARM_TEST );

	label = XtVaCreateManagedWidget ("Cause RDA Alarm",
		xmLabelWidgetClass,	row14_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("  Start Number:",
		xmLabelWidgetClass,	row14_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

        rda_alarm_start_number_user_input = XtVaCreateManagedWidget( "st_num",
		xmTextFieldWidgetClass, row14_rowcol,
		XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,            hci_get_fontlist (LIST),
		XmNcolumns,             4,
		XmNmaxLength,           4,
		XmNmarginHeight,        2,
		XmNshadowThickness,     1,
		NULL);

	label = XtVaCreateManagedWidget ("  End Number:",
		xmLabelWidgetClass,	row14_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

        rda_alarm_end_number_user_input = XtVaCreateManagedWidget( "end_num",
		xmTextFieldWidgetClass, row14_rowcol,
		XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,            hci_get_fontlist (LIST),
		XmNcolumns,             4,
		XmNmaxLength,           4,
		XmNmarginHeight,        2,
		XmNshadowThickness,     1,
		NULL);

	label = XtVaCreateManagedWidget (" Set(1)/Clear(0):",
		xmLabelWidgetClass,	row14_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

        rda_alarm_set_clear_user_input = XtVaCreateManagedWidget( "set_clr",
		xmTextFieldWidgetClass, row14_rowcol,
		XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,            hci_get_fontlist (LIST),
		XmNcolumns,             1,
		XmNmaxLength,           1,
		XmNmarginHeight,        2,
		XmNshadowThickness,     1,
		NULL);

	XtManageChild (row14_rowcol);

	XtManageChild (exception_rowcol);

	XtManageChild (form);

	HCI_Shell_start( send_exception_popup_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This callback is activated when the send exception	*
 *		     "Close" button is selected.			*
 *									*
 *	Input:  w - Widget ID of "Close" button.			*
 *	        client_data - unused					*
 *	        call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_exception_popup_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_Shell_popdown( send_exception_popup_dialog );
}

/************************************************************************
 *	Description: This callback is activated when one of the "send"	*
 *		      buttons is selected.				*
 *									*
 *	Input:  w - Widget ID of "send" button.				*
 *	        client_data - unused					*
 *	        call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_exception (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int status = -1;
  Rdasim_gui_t msg;

  /* Assemble message. */

  msg.cmd_type = EXCEPTION;
  msg.command = ( int )client_data;
  memcpy( msg.parameters, parameters_to_send, LB_PARAMETER_LENGTH );

  /* Write message to LB. */

  status = LB_write( lbfd, ( char * ) &msg,
                     sizeof( Rdasim_gui_t ), RDASIM_GUI_MSG_ID );
  
  if( status < 0 )
  {
    error_with_LB( "LB_write", status );
    return;
  }
}

/************************************************************************
 *      Description: This callback is activated when the "send"		*
 *                   buttons is selected for the RDA Alarms exception.	*
 *                   The user input for the correction must be handled.	*
 *                                                                      *
 *      Input:  w - Widget ID of "send" button.                         *
 *              client_data - unused                                    *
 *              call_data   - unused                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
send_exception_rda_alarms (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  char *input;
  int len;
  int tmp_int1, tmp_int2, tmp_int3;
  int *param_list;

  input = XmTextGetString( rda_alarm_start_number_user_input );
  len = strlen( input );
  if( len < 1 )
  {
    bad_input( "Exception: RDA Alarms" );
    return;
  }
  else
  {
    tmp_int1 = atoi( input );
  }
  XtFree( input );

  input = XmTextGetString( rda_alarm_end_number_user_input );
  len = strlen( input );
  if( len < 1 )
  {
    bad_input( "Exception: RDA Alarms" );
    return;
  }
  else
  {
    tmp_int2 = atoi( input );
  }
  XtFree( input );

  input = XmTextGetString( rda_alarm_set_clear_user_input );
  len = strlen( input );
  if( len < 1 )
  {
    bad_input( "Exception: RDA Alarms" );
    return;
  }
  else
  {
    tmp_int3 = atoi( input );
    if( tmp_int3 != 0 && tmp_int3 != 1 )
    {
      bad_input( "Exception: RDA Alarms" );
      return;
    }
  }
  XtFree( input );

  param_list = ( int * ) parameters_to_send;
  param_list[ 0 ] = tmp_int1;
  param_list[ 1 ] = tmp_int2;
  param_list[ 2 ] = tmp_int3;

  send_exception( w, client_data, call_data );
}

