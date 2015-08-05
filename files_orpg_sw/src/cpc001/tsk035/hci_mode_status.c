/************************************************************************
 *									*
 *	Module:  hci_mode_status.c					*
 *									*
 *	Description:  This module is used by the ORPG HCI to display,	*
 *		      the latest mode status data.			*
 *									*
 ************************************************************************/

/* 
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 16:55:31 $
 * $Id: hci_mode_status.c,v 1.31 2014/03/18 16:55:31 jeffs Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */  

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_wx_status.h>

/*	Global widget definitons					*/

static	Widget		Top_widget			= (Widget) NULL;
static	Widget		current_mode_label		= (Widget) NULL;
static	Widget		current_mode_time_label		= (Widget) NULL;
static	Widget		detected_mode_label		= (Widget) NULL;
static	Widget		detected_mode_time_label	= (Widget) NULL;
static	Widget		last_updated_time_label		= (Widget) NULL;
static	Widget		countdown_prefix_label		= (Widget) NULL;
static	Widget		countdown_label			= (Widget) NULL;
static	Widget		countdown_postfix_label		= (Widget) NULL;
static	Widget		op_alert_label			= (Widget) NULL;
static	Widget		op_alert_rowcol			= (Widget) NULL;
static	Widget		parameters_value_one		= (Widget) NULL;
static	Widget		parameters_value_two		= (Widget) NULL;
static	Widget		parameters_value_three		= (Widget) NULL;
static	Widget		parameters_value_four		= (Widget) NULL;
static	XmString	countdown_mode_string;
static	XmString	countdown_conflict_string;

/*	Local storage for mode detection adaptation data table	*/

char	Mode_buf [256]; /* shared buffer for string operations */

/*	Function prototypes.					*/

int     hci_activate_child (Display *d, Window w, char *cmd,
                            char *proc, char *win, int object_index);
void	close_mode_status (Widget w,
		XtPointer client_data, XtPointer call_data);
void	close_save_mode_status (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_close_save_mode_status (Widget w,
		XtPointer client_data, XtPointer call_data);
void	reject_close_save_mode_status (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_modify_mode_parameters (Widget w,
		XtPointer client_data, XtPointer call_data);
void	display_mode_detection_data     ();
void	timer_proc ();


/************************************************************************
 *	Description: This is the main function for the Mode		*
 *		     Status task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv[]
)
{
	Widget		form;
	Widget		form_rowcol;
	Widget		button_rowcol;
	Widget		mode_frame;
	Widget		mode_rowcol;
	Widget		current_mode_rowcol;
	Widget		detected_mode_rowcol;
	Widget		last_updated_rowcol;
	Widget		op_alert_frame;
	Widget		countdown_timer_rowcol;
	Widget		parameters_frame;	
	Widget		parameters_rowcol;	
	Widget		parameters_header_rowcol;	
	Widget		parameters_header_one;	
	Widget		parameters_header_two;	
	Widget		parameters_header_three;	
	Widget		parameters_header_four;	
	Widget		parameters_value_rowcol;	
	int		text_box_width = 16;
	Widget		label;
	Widget		button;

/*	Initialize HCI. */

	HCI_init( argc, argv, HCI_MODE_STATUS_TASK );

/*	Check to see if the parent widget has been defined.  It must	*
 *	be defined first so that variaous X properties are defined.	*/

	Top_widget = HCI_get_top_widget();

/*	The mode status window doesn't exist so create one.  It	*
 *	consists of a popup dialog and a child scrolled window		*/

	form = XtVaCreateManagedWidget ("mode_list_form",
		xmFormWidgetClass,	Top_widget,
		NULL);
		
	form_rowcol = XtVaCreateManagedWidget ("form_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

/*	If low bandwidth, display a progress meter.			*/

	HCI_PM( "Reading Mode Data" );		
	
/*	Display a row at the top of the window containing the Close	*
 *	and Modify Parameters buttons.					*/

	button_rowcol = XtVaCreateManagedWidget ("button_rowcol",
		xmRowColumnWidgetClass,	form_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, close_mode_status,
		NULL);

	button = XtVaCreateManagedWidget ("Modify Parameters",
		xmPushButtonWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_modify_mode_parameters,
		NULL);

/*	Next, create a container to hold the label informing the user	*
 *	of the current mode and time/date current mode was detected.	*/

	mode_frame = XtVaCreateManagedWidget ("mode_frame",
		xmFrameWidgetClass,	form_rowcol,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		button_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Status",
		xmLabelWidgetClass,	mode_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	mode_rowcol = XtVaCreateManagedWidget ("mode_rowcol",
		xmRowColumnWidgetClass,	mode_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	current_mode_rowcol = XtVaCreateManagedWidget ("current_mode_rowcol",
		xmRowColumnWidgetClass,	mode_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Current Mode:     ",
		xmLabelWidgetClass,	current_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	current_mode_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	current_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNlabelType,		XmSTRING,
		NULL);

	label = XtVaCreateManagedWidget ("  Since:",
		xmLabelWidgetClass,	current_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	current_mode_time_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	current_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	detected_mode_rowcol = XtVaCreateManagedWidget ("detected_mode_rowcol",
		xmRowColumnWidgetClass,	mode_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Recommended Mode: ",
		xmLabelWidgetClass,	detected_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	detected_mode_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	detected_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("  Since:",
		xmLabelWidgetClass,	detected_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	detected_mode_time_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	detected_mode_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	last_updated_rowcol = XtVaCreateManagedWidget ("last_updated_rowcol",
		xmRowColumnWidgetClass,	mode_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Volume Scan Time When Last Updated:",
		xmLabelWidgetClass,	last_updated_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	last_updated_time_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	last_updated_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Next, create a containiner to hold the mode timing.		*/

	countdown_timer_rowcol = XtVaCreateManagedWidget ("countdown_timer_rowcol",
		xmRowColumnWidgetClass,	form_rowcol,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		mode_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

/*	Countdown is to a conflict or to a mode change. Define	*
 *	strings for both.					*/

	countdown_mode_string = XmStringCreateLocalized( "Time Until Clear Air Mode:" ); 
	countdown_conflict_string = XmStringCreateLocalized( "Time Until Mode Conflict: " ); 

	countdown_prefix_label = XtVaCreateManagedWidget ("Time Until Clear Air Mode:",
		xmLabelWidgetClass,	countdown_timer_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	countdown_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	countdown_timer_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	countdown_postfix_label = XtVaCreateManagedWidget (" minutes ",
		xmLabelWidgetClass,	countdown_timer_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Next, create a container to hold the Conflict Status.		*/

	op_alert_frame = XtVaCreateManagedWidget ("op_alert_frame",
		xmFrameWidgetClass,	form_rowcol,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		countdown_timer_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNmarginHeight,	5,
		XmNmarginWidth,		5,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Conflict Status",
		xmLabelWidgetClass,	op_alert_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	op_alert_rowcol = XtVaCreateManagedWidget ("op_alert_rowcol",
		xmRowColumnWidgetClass,	op_alert_frame,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		countdown_timer_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	op_alert_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	op_alert_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	In the bottom half of the window display mode detection	status	*
 *	data.								*/

	parameters_frame = XtVaCreateManagedWidget ("parameters frame",
		xmFrameWidgetClass,	form_rowcol,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		op_alert_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	parameters_rowcol = XtVaCreateManagedWidget ("parameters rowcol",
		xmRowColumnWidgetClass,	parameters_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	parameters_header_rowcol = XtVaCreateManagedWidget ("param header rc",
		xmRowColumnWidgetClass,	parameters_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		NULL);

	parameters_header_one = XtVaCreateManagedWidget ("param header one",
		xmTextWidgetClass,	parameters_header_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNrows,		2,
		XmNeditMode,		XmMULTI_LINE_EDIT,	
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf(Mode_buf,"  Refl (dBZ) \n  Threshold  ");
	XmTextSetString(parameters_header_one,Mode_buf);

	parameters_header_two = XtVaCreateManagedWidget ("param header two",
		xmTextWidgetClass,	parameters_header_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNrows,		2,
		XmNeditMode,		XmMULTI_LINE_EDIT,	
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf(Mode_buf," Area (km^2) \n  Threshold  ");
	XmTextSetString(parameters_header_two,Mode_buf);

	parameters_header_three = XtVaCreateManagedWidget ("param header three",
		xmTextWidgetClass,	parameters_header_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNrows,		2,
		XmNeditMode,		XmMULTI_LINE_EDIT,	
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf(Mode_buf," Area (km^2) \n  Detected   ");
	XmTextSetString(parameters_header_three,Mode_buf);

	parameters_header_four = XtVaCreateManagedWidget ("param header four",
		xmTextWidgetClass,	parameters_header_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNrows,		2,
		XmNeditMode,		XmMULTI_LINE_EDIT,	
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf(Mode_buf,"     Area    \n   Exceeded  ");
	XmTextSetString(parameters_header_four,Mode_buf);

	parameters_value_rowcol = XtVaCreateManagedWidget ("parameters value",
		xmRowColumnWidgetClass,	parameters_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		NULL);

	parameters_value_one = XtVaCreateManagedWidget ("",
		xmTextFieldWidgetClass,	parameters_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	parameters_value_two = XtVaCreateManagedWidget ("",
		xmTextFieldWidgetClass,	parameters_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	parameters_value_three = XtVaCreateManagedWidget ("",
		xmTextFieldWidgetClass,	parameters_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	parameters_value_four = XtVaCreateManagedWidget ("",
		xmTextFieldWidgetClass,	parameters_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		text_box_width,
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

/*	Popupate the table with real data.				*/

	display_mode_detection_data ();
	
	XtRealizeWidget (Top_widget);

/*	Start HCI loop. */

	HCI_start( timer_proc, HCI_THREE_SECONDS, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This function is used to display mode		*
 *		     detection status data in the Mode Status window.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_mode_detection_data (
)
{
  Wx_status_t wx_status;
  XmString str;
  char temp_buf[ 80 ];
  double precip_mode_zthresh;
  int current_mode;
  int recommended_mode;
  int rain_area;
  int time_diff;
  int rain_area_threshold;
  int mode_A_switch;
  int mode_B_switch;
  int mode_deselect;
  time_t curr_vol_time;
  time_t clr_air_time;
  time_t curr_mode_time;
  time_t rec_mode_start_time;
  time_t conflict_start_time;

  /* Get latest weather status data values. */

  wx_status = hci_get_wx_status();

  current_mode = wx_status.current_wxstatus;
  recommended_mode = wx_status.recommended_wxstatus;
  rain_area = ( int ) wx_status.precip_area;
  rain_area_threshold = wx_status.mode_select_adapt.precip_mode_area_thresh;
  curr_vol_time = wx_status.a3052t.curr_time;
  clr_air_time = wx_status.a3052t.time_to_cla;
  curr_mode_time = wx_status.current_wxstatus_time;
  rec_mode_start_time = wx_status.recommended_wxstatus_start_time;
  precip_mode_zthresh = wx_status.mode_select_adapt.precip_mode_zthresh;
  conflict_start_time = wx_status.conflict_start_time;
  mode_A_switch = wx_status.mode_select_adapt.auto_mode_A;
  mode_B_switch = wx_status.mode_select_adapt.auto_mode_B;
  mode_deselect = wx_status.wxstatus_deselect;

  /* Fill in hci components with latest data. */

  /* Current mode info. */

  if( current_mode == MODE_A )
  {
    sprintf( temp_buf, "  Precip   " );
    XtVaSetValues( current_mode_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }
  else if( current_mode == MODE_B )
  {
    sprintf( temp_buf, " Clear Air " );
    XtVaSetValues( current_mode_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }
  else
  {
    sprintf( temp_buf, "    NA     " );
    XtVaSetValues( current_mode_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }

  str = XmStringCreateLocalized( temp_buf );
  XtVaSetValues( current_mode_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  if( curr_mode_time == WX_STATUS_UNDEFINED )
  {
    sprintf( temp_buf, "             NA             " );
    XtVaSetValues( current_mode_time_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  else
  {
    strftime( temp_buf, 50, " %b %d, %Y - %X UT ", gmtime( &curr_mode_time ) );
    XtVaSetValues( current_mode_time_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }

  str = XmStringCreateLocalized( temp_buf );
  XtVaSetValues( current_mode_time_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  /* Recommended mode info. */

  if( recommended_mode == MODE_A )
  {
    sprintf( temp_buf, "  Precip   " );
    XtVaSetValues( detected_mode_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }
  else if( recommended_mode == MODE_B )
  {
    sprintf( temp_buf, " Clear Air " );
    XtVaSetValues( detected_mode_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }
  else
  {
    sprintf( temp_buf, "    NA     " );
    XtVaSetValues( detected_mode_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }

  str = XmStringCreateLocalized( temp_buf );
  XtVaSetValues( detected_mode_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  if( conflict_start_time == WX_STATUS_UNDEFINED )
  {
    sprintf( temp_buf, "             NA             " );
    XtVaSetValues( detected_mode_time_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  else
  {
    strftime( temp_buf, 50, " %b %d, %Y - %X UT ", gmtime( &rec_mode_start_time ) );
    XtVaSetValues( detected_mode_time_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }

  str = XmStringCreateLocalized( temp_buf );
  XtVaSetValues( detected_mode_time_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  /* Volume scan update time. */

  if( ( curr_vol_time == WX_STATUS_UNDEFINED ) || ( curr_vol_time == 0 ) )
  {
    sprintf( temp_buf, "             NA             " );
    XtVaSetValues( last_updated_time_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  else
  {
    strftime( temp_buf, 50, " %b %d, %Y - %X UT ", gmtime( &curr_vol_time ) );
    XtVaSetValues( last_updated_time_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }

  str = XmStringCreateLocalized( temp_buf );
  XtVaSetValues( last_updated_time_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  /* Timer countdown (if applicable). */

  if( mode_A_switch == AUTO_SWITCH && current_mode == MODE_A &&
      ( rain_area - rain_area_threshold <= 0 ) &&
      ( clr_air_time - curr_vol_time > 0 ) )
  {
    /* If time is applicable, color-in label. */

    XtVaSetValues( countdown_prefix_label,
                   XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                   NULL );
    XtVaSetValues( countdown_label,
                   XmNbackground, hci_get_read_color( WHITE ),
                   XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                   NULL );
    XtVaSetValues( countdown_postfix_label,
                   XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                   NULL );

    /* Use prefix string depending on mode B switch setting. */

    if( mode_B_switch == MANUAL_SWITCH )
    {
      XtVaSetValues( countdown_prefix_label,
                     XmNlabelString, countdown_conflict_string, NULL );
    }
    else
    {
      XtVaSetValues( countdown_prefix_label,
                     XmNlabelString, countdown_mode_string, NULL );
    }

    /* Compute time difference between time until Clear Air */
    /* and the current time. Round to the nearest minute.   */

    time_diff = (clr_air_time - curr_vol_time + 30) / 60;
    if( time_diff < 0 ){ time_diff = 0; }
    sprintf( temp_buf, " %2d ", time_diff );
    str = XmStringCreateLocalized( temp_buf );
    XtVaSetValues( countdown_label, XmNlabelString, str, NULL );
    XmStringFree( str );
  }
  else
  {
    /* If timer isn't applicable, blank-out label. */

    XtVaSetValues( countdown_prefix_label,
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL );
    XtVaSetValues( countdown_label,
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL );
    XtVaSetValues( countdown_postfix_label,
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL );
  }

  if( current_mode == WX_STATUS_UNDEFINED )
  {
    sprintf( temp_buf, "NA" );
    XtVaSetValues( op_alert_rowcol, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    XtVaSetValues( op_alert_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  else if( recommended_mode == WX_STATUS_UNDEFINED )
  {
    sprintf( temp_buf, "NA" );
    XtVaSetValues( op_alert_rowcol, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    XtVaSetValues( op_alert_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  else if( current_mode == recommended_mode )
  {
    sprintf( temp_buf, "Current and Recommended Weather Modes Agree" );
    XtVaSetValues( op_alert_rowcol, XmNbackground,
                   hci_get_read_color( NORMAL_COLOR ), NULL );
    XtVaSetValues( op_alert_label, XmNbackground,
                   hci_get_read_color( NORMAL_COLOR ), NULL );
  }
  else
  {
    if( mode_deselect > 0 )
    {
      sprintf( temp_buf, "Current and Recommended Weather Modes Conflict - Transitioning" );
    }
    else 
    {
      sprintf( temp_buf, "Current and Recommended Weather Modes Conflict" );
    }
    XtVaSetValues( op_alert_rowcol, XmNbackground,
                   hci_get_read_color( WARNING_COLOR ), NULL );
    XtVaSetValues( op_alert_label, XmNbackground,
                   hci_get_read_color( WARNING_COLOR ), NULL );
  }

  str = XmStringCreateLocalized( temp_buf );
  XtVaSetValues( op_alert_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  if( precip_mode_zthresh <= -10.0 )
  {
    sprintf( temp_buf, "     %5.1f      ", precip_mode_zthresh );
  }
  else if( precip_mode_zthresh < 0.0 )
  {
    sprintf( temp_buf, "      %4.1f      ", precip_mode_zthresh );
  }
  else if( precip_mode_zthresh < 10.0 )
  {
    sprintf( temp_buf, "      %3.1f       ", precip_mode_zthresh );
  }
  else
  {
    sprintf( temp_buf, "      %4.1f      ", precip_mode_zthresh );
  }

  XmTextSetString( parameters_value_one, temp_buf );
  XtVaSetValues( parameters_value_one, XmNbackground,
                 hci_get_read_color( WHITE ), NULL );

  if( rain_area_threshold < 10 )
  {
    sprintf( temp_buf, "       %1d        ", rain_area_threshold );
  }
  else if( rain_area_threshold < 100 )
  {
    sprintf( temp_buf, "       %2d       ", rain_area_threshold );
  }
  else if( rain_area_threshold < 1000 )
  {
    sprintf( temp_buf, "      %3d       ", rain_area_threshold );
  }
  else if( rain_area_threshold < 10000 )
  {
    sprintf( temp_buf, "      %4d      ", rain_area_threshold );
  }
  else if( rain_area_threshold < 100000 )
  {
    sprintf( temp_buf, "     %5d      ", rain_area_threshold );
  }
  else if( rain_area_threshold < 1000000 )
  {
    sprintf( temp_buf, "     %6d     ", rain_area_threshold );
  }
  else
  {
    sprintf( temp_buf, "    %7d     ", rain_area_threshold );
  }

  XmTextSetString( parameters_value_two, temp_buf );
  XtVaSetValues( parameters_value_two, XmNbackground,
                 hci_get_read_color( WHITE ), NULL );

  if( rain_area == WX_STATUS_UNDEFINED )
  {
    sprintf( temp_buf, "       NA       " );
    XtVaSetValues( parameters_value_three, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR1 ), NULL );
  }
  else
  {
    if( rain_area < 10 )
    {
      sprintf( temp_buf, "       %1d        ", rain_area );
    }
    else if( rain_area < 100 )
    {
      sprintf( temp_buf, "       %2d       ", rain_area );
    }
    else if( rain_area < 1000 )
    {
      sprintf( temp_buf, "      %3d       ", rain_area );
    }
    else if( rain_area < 10000 )
    {
      sprintf( temp_buf, "      %4d      ", rain_area );
    }
    else if( rain_area < 100000 )
    {
      sprintf( temp_buf, "     %5d      ", rain_area );
    }
    else if( rain_area < 1000000 )
    {
      sprintf( temp_buf, "     %6d     ", rain_area );
    }
    else
    {
      sprintf( temp_buf, "    %7d     ", rain_area );
    }

    XtVaSetValues( parameters_value_three, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }

  XmTextSetString( parameters_value_three, temp_buf );

  if( rain_area == ( float ) WX_STATUS_UNDEFINED )
  {
    sprintf( temp_buf, "       NA       " );
    XtVaSetValues( parameters_value_four, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR1 ), NULL );
  }
  else
  {
    if( ( rain_area - rain_area_threshold ) > 0 )
    {
      sprintf( temp_buf, "      YES       " );
      XtVaSetValues( parameters_value_four, XmNbackground,
                     hci_get_read_color( WARNING_COLOR ), NULL );
    }
    else
    {
      sprintf( temp_buf, "       NO       " );
      XtVaSetValues( parameters_value_four, XmNbackground,
                     hci_get_read_color( NORMAL_COLOR ), NULL );
    }
  }

  XmTextSetString( parameters_value_four, temp_buf );

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the Mode Status window.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - ununsed					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
close_mode_status (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Mode Status Close selected");
	XtDestroyWidget (Top_widget);
}

/************************************************************************
 *	Description: This function is activated when the Modify		*
 *		     Parameters button is pressed.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - ununsed					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_modify_mode_parameters (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char window_name[256];
  char task_name[256];
  char process_name[50];
  char channel_number[32];

  sprintf (window_name,"Algorithms");

  sprintf (channel_number,"-A 0");
  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    sprintf( channel_number, "-A %1d",
             ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) );
  }

  sprintf( process_name, "hci_apps_adapt %s", channel_number );
  sprintf( task_name, "%s -name \"Algorithms\" -I \"Mode Selection\"", process_name);

  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      process_name,
                      window_name,
                      -1 );
}

/************************************************************************
 *      Description: This function is the timer procedure for the       *
 *                   Precipitation Status task.  Its main function is   *
 *                   to force a window update when adaptation data      *
 *                   changes are detected.                              *
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              id - timer interval ID                                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
timer_proc ()
{
  /* Re-populate the table with data.	*/
  display_mode_detection_data();
}

