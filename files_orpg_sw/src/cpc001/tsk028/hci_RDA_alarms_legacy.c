/************************************************************************
 *									*
 *	Module:  hci_RDA_alarms_orda.c					*
 *									*
 *	Description:  This module is used by the ORPG HCI to display,	*
 *		      in chronological order, all rda alarms.		*
 *									*
 ************************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:57:26 $
 * $Id: hci_RDA_alarms_legacy.c,v 1.21 2010/03/10 18:57:26 ccalvert Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */  

/*	Local include file definitions.					*/

#include <hci.h>

/*	Local constants.						*/

#define	VISIBLE_RDA_ALARMS	10	/* 10 Alarms visible (default)	*/

enum {SEARCH_MONTH, SEARCH_DAY,    SEARCH_YEAR,
      SEARCH_HOUR,  SEARCH_MINUTE, SEARCH_SECOND};

/*	Global widget definitions					*/

static	Widget	Top_widget = (Widget) NULL; /* Top widget */
static	Widget	Data_scroll      = (Widget) NULL; /* Scroll list for messages */
static	Widget	Form             = (Widget) NULL; /* Manager for window */

/*	Search/filter widgets		*/

static	Widget	Search_text      = (Widget) NULL;
static	Widget	Search_month     = (Widget) NULL;
static	Widget	Search_day       = (Widget) NULL;
static	Widget	Search_year      = (Widget) NULL;
static	Widget	Search_hour      = (Widget) NULL;
static	Widget	Search_minute    = (Widget) NULL;
static	Widget	Search_second    = (Widget) NULL;

/*	Device filter toggle widgets	*/

static	Widget	ARC_button       = (Widget) NULL;
static	Widget	CTR_button       = (Widget) NULL;
static	Widget	PED_button       = (Widget) NULL;
static	Widget	RSP_button       = (Widget) NULL;
static	Widget	USR_button       = (Widget) NULL;
static	Widget	UTL_button       = (Widget) NULL;
static	Widget	WID_button       = (Widget) NULL;
static	Widget	XMT_button       = (Widget) NULL;
static	Widget	ALL_button       = (Widget) NULL;

static	int	Max_displayable_alarms = 500; /* Maximum # alarms which are
				displayed in the scrolled list. */
static	int	Month    = 0;	/* Month of latest message to display */
static	int	Day      = 0;	/* Day of latest message to be displayed */
static	int	Year     = 0;	/* Year of latest message to be displayed */
static	int	Hour     = 0;	/* Hour of latest message to be displayed */
static	int	Minute   = 0;	/* Minute of latest message to be displayed */
static	int	Second   = 0;	/* Second of latest message to be displayed */
static	int	Hhmmss   = 0;	/* Hour*10000 + Minute*100 + Second */
static	int	Yyyymmdd = 0;	/* Year*10000 + Month*100 + Day */

static	int	Select_all    = 0; /* 1 = All devices selected */
static	int	All           = 1; /* All/None label state; 1 = All */
static	unsigned int	Device_filter = 0;
static	int Config_change_popup_flag = 0;
char	Alarm_buf [128]; /* Common buffer for building strings */

/*	Labels for device categories	*/

static	char	*Alarm_device [] = {
	"   ", "XMT", "UTL", "RSP", "CTR",
	"PED", "ARC", "USR", "WID"
};

/*	Labels for alarm type	*/

static	char	*Alarm_type [] = {
	" ", "E", "F", "O"
};

static	char	Old_search_string [64] = {""}; /* Old search string data */
static	char	Search_string [64] = {""};     /* New search string data */

static	int		device_filter_updated_flag = 0;

static	void	timer_proc();

int	destroy_rda_alarm_list ();
void	close_rda_alarm_list (Widget w,
		XtPointer client_data, XtPointer call_data);
void	device_toggle_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	search_text_callback   (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_all_callback     (Widget w,
		XtPointer client_data, XtPointer call_data);
void	clear_callback     (Widget w,
		XtPointer client_data, XtPointer call_data);
void	filter_callback     (Widget w,
		XtPointer client_data, XtPointer call_data);

void	display_rda_alarm_list();

void	set_max_alarms_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	set_date_time_callback  (Widget w,
		XtPointer client_data, XtPointer call_data);
void	device_filter_updated (int fd, LB_id_t msg_id,
                               int msg_info, void *arg);
void	read_device_filter();
void	rda_config_change();

/************************************************************************
 *      Description: This is the main function for the RDA Alarms task. *
 *                                                                      *
 *      Input:  argc - number of commandline arguments                  *
 *              argv - pointer to commandline arguments data            *
 *      Output: NONE                                                    *
 *      Return: exit code                                               *
 ************************************************************************/

int
main (
int     argc,
char    *argv[]
)
{
	Widget	form;
	Widget	label;
	Widget	bottom_label;
	Widget	button;
	Widget	rowcol;
	Widget	text;
	Widget	filter_form;
	Widget	device_frame;
	Widget	device_rowcol;
	Widget	search_frame;
	Widget	search_form;
	Widget	search_rowcol;
	Widget	code_frame;
	Widget	code_rowcol;
	int	status;

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_RDA_TASK );

	HCI_set_destroy_callback( destroy_rda_alarm_list );
        Top_widget = HCI_get_top_widget();

/*	Register for changes to LB that holds Device filter	*/

        status = ORPGDA_UN_register (ORPGDAT_HCI_DATA,
                                     HCI_RDA_DEVICE_DATA_MSG_ID,
                                     device_filter_updated);
                                                                                
        if (status != LB_SUCCESS)
        {
            HCI_LE_error("Failed to register for RDA device filter updates (%d)", status);
        }

/*	Read in device filter value.					*/

	status = ORPGDA_read( ORPGDAT_HCI_DATA,
	                      ( char * ) &Device_filter,
	                      sizeof( unsigned int ),
	                      HCI_RDA_DEVICE_DATA_MSG_ID );

/*	If device filter read is successful, use it,	*/
/*	if it isn't, assume all filters are set.	*/

        if( status <= 0 )
	{
	  HCI_LE_error("Error (%d) reading RDA device filter from HCI_DATA LB, assume all filters are set.", status );
	  Device_filter = ARC_MASK | CTR_MASK | PED_MASK | RSP_MASK |
	 		USR_MASK | UTL_MASK | WID_MASK | XMT_MASK;
	  Select_all= 1;
	}
        else if( (Device_filter == 0) || (Device_filter != (ARC_MASK | CTR_MASK | PED_MASK | RSP_MASK |
	 		                                    USR_MASK | UTL_MASK | WID_MASK | XMT_MASK)) )
        {
          Select_all = 0;
          All = 0;
        }

/*	As in most other HCI windows use a form widget as the parent.	*/

	form = XtVaCreateWidget ("rda_alarm_list_form",
		xmFormWidgetClass,	Top_widget,
		NULL);

/*	Create a rowcolumn widget to manage control buttons along the	*
 *	top of the window.						*/

	rowcol = XtVaCreateWidget ("alarm_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Define a pushbutton to close the window.			*/

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, close_rda_alarm_list, NULL);

/*	Define a label and text entry widget so the user can define	*
 *	the alarm list length.						*/

	XtVaCreateManagedWidget ("     Maximum Displayable Alarms: ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	text = XtVaCreateManagedWidget ("max_alarms",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		4,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (text,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 4);

	XtAddCallback (text,
		XmNactivateCallback, set_max_alarms_callback,
		NULL);

	XtAddCallback (text,
		XmNlosingFocusCallback, set_max_alarms_callback,
		NULL);

	sprintf (Alarm_buf,"%d", Max_displayable_alarms);
	XmTextSetString (text, Alarm_buf);

	XtManageChild (rowcol);

/*	Create a form containing a set of message filters for the	*
 *	RDA alarm message list.						*/

	filter_form = XtVaCreateWidget ("filter_form",
		xmFormWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Create a set of selections to filter by device category.	*/

	device_frame  = XtVaCreateManagedWidget ("device_frame",
		xmFrameWidgetClass,	filter_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Device",
		xmLabelWidgetClass,	device_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	device_rowcol = XtVaCreateWidget ("device_rowcol",
		xmRowColumnWidgetClass,	device_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		2,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Archive II category	*/

	ARC_button = XtVaCreateManagedWidget ("ARC   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & ARC_MASK) {

	    XtVaSetValues (ARC_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (ARC_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (ARC_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_ARC);

/*	RDA Control category	*/

	CTR_button = XtVaCreateManagedWidget ("CTR   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & CTR_MASK) {

	    XtVaSetValues (CTR_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (CTR_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (CTR_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_CTR);

/*	Antenna/Pedestal category	*/

	PED_button = XtVaCreateManagedWidget ("PED   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & PED_MASK) {

	    XtVaSetValues (PED_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (PED_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (PED_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_PED);

/*	Receiver/Signal Processor category	*/

	RSP_button = XtVaCreateManagedWidget ("RSP   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & RSP_MASK) {

	    XtVaSetValues (RSP_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (RSP_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (RSP_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_RSP);

/*	User link category	*/

	USR_button = XtVaCreateManagedWidget ("USR   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & USR_MASK) {

	    XtVaSetValues (USR_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (USR_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (USR_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_USR);

/*	Tower/Utilities category	*/

	UTL_button = XtVaCreateManagedWidget ("UTL   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & UTL_MASK) {

	    XtVaSetValues (UTL_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (UTL_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (UTL_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_UTL);

/*	Wideband link category	*/

	WID_button = XtVaCreateManagedWidget ("WID   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & WID_MASK) {

	    XtVaSetValues (WID_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (WID_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (WID_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_WID);

/*	Transmitter category	*/

	XMT_button = XtVaCreateManagedWidget ("XMT   ",
		xmToggleButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNindicatorOn,		XmINDICATOR_CHECK_BOX,
		NULL);

	if (Device_filter & XMT_MASK) {

	    XtVaSetValues (XMT_button,
		XmNset,		True,
		NULL);

	} else {

	    XtVaSetValues (XMT_button,
		XmNset,		False,
		NULL);

	}

	XtAddCallback (XMT_button,
		XmNvalueChangedCallback, device_toggle_callback,
		(XtPointer) ORPGRDA_DEVICE_XMT);

/*	Button to toggle on/off all categories	*/

	if( All == 1 )
	{
	  ALL_button = XtVaCreateManagedWidget ("None",
		xmPushButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);
	}
	else
	{
	  ALL_button = XtVaCreateManagedWidget ("All",
		xmPushButtonWidgetClass,	device_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);
	}

	XtAddCallback (ALL_button,
		XmNactivateCallback, select_all_callback,
		NULL);

	XtManageChild (device_rowcol);

/*	Create a set of selections to filter by date/time and text	*
 *	pattern.							*/

	search_frame  = XtVaCreateManagedWidget ("search_frame",
		xmFrameWidgetClass,	filter_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		device_frame,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Filter Parameters",
		xmLabelWidgetClass,	search_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	search_form = XtVaCreateWidget ("search_form",
		xmFormWidgetClass,	search_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	search_rowcol = XtVaCreateWidget ("search_rowcol",
		xmRowColumnWidgetClass,	search_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

/*	Month/Day/Year	*/

	XtVaCreateManagedWidget ("   MM/DD/YYYY  ",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_month = XtVaCreateManagedWidget ("Search_month",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		2,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	if (Month > 0) {

	    sprintf (Alarm_buf,"%2d ",Month);
	    XmTextSetString (Search_month, Alarm_buf);

	}

	XtAddCallback (Search_month,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 2);

	XtAddCallback (Search_month,
		XmNactivateCallback, set_date_time_callback,
		(XtPointer) SEARCH_MONTH);

	XtAddCallback (Search_month,
		XmNlosingFocusCallback, set_date_time_callback,
		(XtPointer) SEARCH_MONTH);

	XtVaCreateManagedWidget ("/",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_day = XtVaCreateManagedWidget ("Search_day",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		2,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	if (Day > 0) {

	    sprintf (Alarm_buf,"%2d ",Day);
	    XmTextSetString (Search_day, Alarm_buf);

	}

	XtAddCallback (Search_day,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 2);

	XtAddCallback (Search_day,
		XmNactivateCallback, set_date_time_callback,
		(XtPointer) SEARCH_DAY);

	XtAddCallback (Search_day,
		XmNlosingFocusCallback, set_date_time_callback,
		(XtPointer) SEARCH_DAY);

	XtVaCreateManagedWidget ("/",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_year = XtVaCreateManagedWidget ("Search_year",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		4,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	XtAddCallback (Search_year,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 4);

	XtAddCallback (Search_year,
		XmNactivateCallback, set_date_time_callback,
		(XtPointer) SEARCH_YEAR);

	XtAddCallback (Search_year,
		XmNlosingFocusCallback, set_date_time_callback,
		(XtPointer) SEARCH_YEAR);

/*	Hour/Minute/Second	*/

	XtVaCreateManagedWidget ("   HH:MM:SS  ",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_hour = XtVaCreateManagedWidget ("Search_hour",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		2,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	if (Hour > 0) {

	    sprintf (Alarm_buf,"%2d ",Hour);
	    XmTextSetString (Search_hour, Alarm_buf);

	}

	XtAddCallback (Search_hour,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 2);

	XtAddCallback (Search_hour,
		XmNactivateCallback, set_date_time_callback,
		(XtPointer) SEARCH_HOUR);

	XtAddCallback (Search_hour,
		XmNlosingFocusCallback, set_date_time_callback,
		(XtPointer) SEARCH_HOUR);

	XtVaCreateManagedWidget (":",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_minute = XtVaCreateManagedWidget ("Search_minute",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		2,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	if (Minute > 0) {

	    sprintf (Alarm_buf,"%2d ",Minute);
	    XmTextSetString (Search_minute, Alarm_buf);

	}

	XtAddCallback (Search_minute,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 2);

	XtAddCallback (Search_minute,
		XmNactivateCallback, set_date_time_callback,
		(XtPointer) SEARCH_MINUTE);

	XtAddCallback (Search_minute,
		XmNlosingFocusCallback, set_date_time_callback,
		(XtPointer) SEARCH_MINUTE);

	XtVaCreateManagedWidget (":",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_second = XtVaCreateManagedWidget ("Search_second",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNcolumns,		2,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	if (Second > 0) {

	    sprintf (Alarm_buf,"%2d ",Second);
	    XmTextSetString (Search_second, Alarm_buf);

	}

	XtAddCallback (Search_second,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 2);

	XtAddCallback (Search_second,
		XmNactivateCallback, set_date_time_callback,
		(XtPointer) SEARCH_SECOND);

	XtAddCallback (Search_second,
		XmNlosingFocusCallback, set_date_time_callback,
		(XtPointer) SEARCH_SECOND);

	XtManageChild (search_rowcol);

/*	Search Pattern	*/

	search_rowcol = XtVaCreateWidget ("string_rowcol",
		xmRowColumnWidgetClass,	search_form,
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		search_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("   Search:",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Search_text = XtVaCreateManagedWidget ("search_text",
		xmTextFieldWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		16,
		XmNmarginHeight,	2,
		XmNshadowThickness,	1,
		NULL);

	if (strlen (Search_string)) {

	    XmTextSetString (Search_text, Search_string);

	}

	XtAddCallback (Search_text,
		XmNactivateCallback, search_text_callback, NULL);

	XtAddCallback (Search_text,
		XmNlosingFocusCallback, search_text_callback, NULL);

	label = XtVaCreateManagedWidget ("Sp",
		xmLabelWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Button to clear date/time/serch pattern	fields	*/

	button = XtVaCreateManagedWidget ("Clear",
		xmPushButtonWidgetClass,	search_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, clear_callback, NULL);

	XtManageChild (search_rowcol);
	XtManageChild (search_form);
	XtManageChild (filter_form);

/*	Create labels defining message contents and color codes.	*/

	code_frame = XtVaCreateManagedWidget ("code_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		filter_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	code_rowcol = XtVaCreateWidget ("code_rowcol",
		xmRowColumnWidgetClass,	code_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("          Alarm Code Color:",
		xmLabelWidgetClass,	code_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget (" SEC  ",
		xmLabelWidgetClass,	code_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("  MR  ",
		xmLabelWidgetClass,	code_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (WARNING_COLOR),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("  MM  ",
		xmLabelWidgetClass,	code_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (ALARM_COLOR2),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget (" INOP ",
		xmLabelWidgetClass,	code_rowcol,
		XmNforeground,		hci_get_read_color (WHITE),
		XmNbackground,		hci_get_read_color (ALARM_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (code_rowcol);

	bottom_label = XtVaCreateManagedWidget ("Type: E = Edge Detect,  O = Occurrence,  F = Filtered Occurrence",
		xmLabelWidgetClass,	form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);


	label = XtVaCreateManagedWidget ("     RDA Date/Time   Device Type Code    Description",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		code_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_BEGINNING,
		NULL);

/*	Build the contents of the scrolled window by creating and	*
 *	adding a list item as each alarm entry is read from the alarm	*
 *	LB.								*/

	Data_scroll = XtVaCreateManagedWidget ("data_scroll",
		xmScrolledWindowWidgetClass,	form,
		XmNheight,		260,
		XmNwidth,		800,
		XmNscrollingPolicy,	XmAUTOMATIC,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNbottomWidget,	bottom_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	{
		Widget	clip;

		XtVaGetValues (Data_scroll,
			XmNclipWindow,	&clip,
			NULL);

		XtVaSetValues (clip,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

	}

	XtVaSetValues (XtParent (Data_scroll),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*      If low bandwidht, display a progress meter.                     */

        HCI_PM( "Reading RDA Alarms" );

        display_rda_alarm_list ();

        XtManageChild (form);

/*      If I/O was cancelled, do not pop up dialog      */

        if (ORPGRDA_alarm_io_status () != RMT_CANCELLED) {

            XtRealizeWidget (Top_widget);

	    /* Start HCI loop. */

	    HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );
        }

        return 0;
}

/************************************************************************
 *      Description: This is the timer procedure for the RDA Alarms     *
 *                   task.  Its primary purpose is to update the window *
 *                   when status data chanes.                           *
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              id - timer interval ID                                  *
 *      Output: NONE                                                    *
 *      Return: 0 (unused)                                              *
 ************************************************************************/

static void
timer_proc ()
{
        Mrpg_state_t    mrpg;
static  unsigned int    old_mrpg_state = -1;
static  int     shutdown_flag = 0;
        int     status;

/* If the RDA configuration changes, call function to display
   popup window and exit. */

   if( ORPGRDA_get_rda_config( NULL ) != ORPGRDA_LEGACY_CONFIG )
   {
     rda_config_change();
   }

/* Check if device filter has changed. */

  if( device_filter_updated_flag )
  {
    read_device_filter();
  }

/*      If the RPG state changes, we want to refresh the display in     *
 *      case the user cleared the system log during a shutdown.         */

        status = ORPGMGR_get_RPG_states (&mrpg);

        if (status == 0) {

            if (mrpg.state != old_mrpg_state) {

                if (mrpg.state == MRPG_ST_OPERATING) {

                    if (shutdown_flag) {

                        ORPGRDA_clear_alarms ();
                        display_rda_alarm_list ();
                        shutdown_flag = 0;

                    }

                } else if (mrpg.state == MRPG_ST_OPERATING) {

                    shutdown_flag = 1;

                }

                old_mrpg_state = mrpg.state;
                display_rda_alarm_list ();

            }
        }

/*      If new RDA alarms are detected,, update the list.               */

        if (ORPGRDA_alarm_update_flag() == 0) {

/*          If low bandwidth, dislay a progress meter.                  */

            HCI_PM( "Reading RDA Alarms" );

            display_rda_alarm_list ();
        }
}

/************************************************************************
 *	Description: This routine constructs and displays the RDA alarm	*
 *		     list.  If the list is displayed prior to invoking	*
 *		     this routine, it is destoryed and a new one is	*
 *		     created.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_rda_alarm_list (
)
{
	Widget	label = (Widget) NULL;
	int	month;
	int	day;
	int	year;
	int	hour;
	int	minute;
	int	second;
	int	code;
	int	alarm;
	int	channel;
	int	i;
	int	first_time;
	RDA_alarm_entry_t	*alarm_data;
	XmString	str;
	char	string [10];
	int	count;
	int	found;
	int	ltime;
	int	date;

/*	If the parent is not defined, then return;		*/

	if (Top_widget == (Widget) NULL) {

	    return;

	}

/*	If a list was perviously displayed, destroy it and	*
 *	create a new one.					*/

	if (Form != (Widget) NULL)
	{
	    XtDestroyWidget (Form);
	}

/*	The widgets in the Alarm list are managed by a form widget.	*
 *	The variable "first_time" is used to define the first widget	*
 *	separately from the others as it is attached to the top of the	*
 *	form.  All subsequent widgets are attached to the preceding	*
 *	one.								*/

	Form = XtVaCreateWidget ("alarm_form",
		xmFormWidgetClass,	Data_scroll,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNverticalSpacing,	1,
		NULL);

	first_time = 1;

/*	Read the RDA Alarm data from the RDA ALARMS LB.  The data are	*
 *	statically stored within the library routine and can be		*
 *	retrieved via library functions.  If this is the first time	*
 *	this routine is invoked, then the entire LB is read.		*
 *	Otherwise, only new (unread) messages are read.			*/

	count = 0;
	found = 0;

	for (i=ORPGRDA_get_num_alarms ()-1;i>=0;i--) {

/*	    Check to see if we have reached our specified number of	*
 *	    displayable alarms.  If so, we're done.			*/

	    if (count >= Max_displayable_alarms) {

		break;

	    }

	    month   = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_MONTH);
	    day     = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_DAY);
	    year    = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_YEAR);
	    hour    = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_HOUR);
	    minute  = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_MINUTE);
	    second  = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_SECOND);
	    code    = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_CODE);
	    alarm   = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_ALARM);
	    channel = ORPGRDA_get_alarm (i, ORPGRDA_ALARM_CHANNEL);

	    if ((alarm < 1) || (alarm > 800)) {

		HCI_LE_error("Invalid RDA alarm code %d", alarm);
		continue;

	    }

/*	    Check to see if a time filter specified.  If so, determine	*
 *	    if time older or the same as speciied time.  If so, we	*
 *	    want to display it.						*/

/*	    Once we find a match we no longer want to check the times.	*
 *	    This is because the times are ordered in reverse		*
 *	    chronological order.  Once a date/time is matched, all the	*
 *	    rest are assumed to be matched also.			*/

	    if (!found) {

		if ((Hhmmss   > 0) ||
		    (Yyyymmdd > 0)) {

		    time_t	t;
		    int	c_year;
		    int	c_month;
		    int	c_day;
		    int	c_hour;
		    int	c_minute;
		    int	c_second;
		    int	c_yyyymmdd;

		    t = time (NULL);

		    unix_time (&t,
			   &c_year,
			   &c_month,
			   &c_day,
			   &c_hour,
			   &c_minute,
			   &c_second);

		    c_yyyymmdd = c_year*10000 + c_month*100 + c_day;

		    if (Yyyymmdd > 0) {

			date = year*10000 + month*100 + day;

			if (date <= Yyyymmdd) {

			    if (Hhmmss > 0) {

				ltime = hour*10000 + minute*100 + second;

				if ((ltime <= Hhmmss) ||
				    ((date  < c_yyyymmdd) &&
				     (date < Yyyymmdd))) {

				    found = 1;

				}

			    } else {

				found = 1;

			    }
			}

		    } else {

			date  = year*10000 + month*100 + day;
			ltime = hour*10000 + minute*100 + second;

			if ((ltime <= Hhmmss) ||
			    (date < c_yyyymmdd)) {

			    found = 1;

			}
		    }

		} else {

		    found = 1;

		}
	    }

	    alarm_data = (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data (alarm);

/*	    Check to see if alarm device code matches filter check	*
 *	    boxes.							*/

	    if ((((Device_filter & ARC_MASK) && (alarm_data->device == ORPGRDA_DEVICE_ARC)) ||
		 ((Device_filter & CTR_MASK) && (alarm_data->device == ORPGRDA_DEVICE_CTR)) ||
		 ((Device_filter & PED_MASK) && (alarm_data->device == ORPGRDA_DEVICE_PED)) ||
		 ((Device_filter & RSP_MASK) && (alarm_data->device == ORPGRDA_DEVICE_RSP)) ||
		 ((Device_filter & USR_MASK) && (alarm_data->device == ORPGRDA_DEVICE_USR)) ||
		 ((Device_filter & UTL_MASK) && (alarm_data->device == ORPGRDA_DEVICE_UTL)) ||
		 ((Device_filter & WID_MASK) && (alarm_data->device == ORPGRDA_DEVICE_WID)) ||
		 ((Device_filter & XMT_MASK) && (alarm_data->device == ORPGRDA_DEVICE_XMT)) ||
		 Select_all) && found) { 

/*		Display alarm if search string can be found in the	*
 *		alarm description.					*/

		if (!strlen (Search_string) ||
		    (strlen (Search_string) &&
		     (hci_string_in_string ((char *) alarm_data->alarm_text,Search_string) != 0))) {

		    count++;

		    if (first_time) {

			label = XtVaCreateManagedWidget ("label",
				xmLabelWidgetClass,	Form,
				XmNfontList,		hci_get_fontlist (LIST),
				XmNtopAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNrightAttachment,	XmATTACH_FORM,
				XmNalignment,		XmALIGNMENT_BEGINNING,
				NULL);

			first_time = 0;
	
		    } else {

			label = XtVaCreateManagedWidget ("label",
				xmLabelWidgetClass,	Form,
				XmNfontList,		hci_get_fontlist (LIST),
				XmNtopAttachment,	XmATTACH_WIDGET,
				XmNtopWidget,		label,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNrightAttachment,	XmATTACH_FORM,
				XmNalignment,		XmALIGNMENT_BEGINNING,
				NULL);

		    }

		    if (code == 0) {

			sprintf (string, "CLEARED");

		    } else {

			sprintf (string, "ACTIVATED");

		    }

		    if (channel) {

			sprintf (Alarm_buf,"%2d/%2.2d/%4d %2.2d:%2.2d:%2.2d  [%3s]  [%1s] [%3d] -- RDA:%d ALARM %s: %s",
				month, day, year, hour, minute, second,
				Alarm_device [alarm_data->device],
				Alarm_type   [alarm_data->type],
				alarm,
				channel,
				string, alarm_data->alarm_text);

		    } else {

			sprintf (Alarm_buf,"%2d/%2.2d/%4d %2.2d:%2.2d:%2.2d  [%3s]  [%1s] [%3d] -- RDA ALARM %s: %s",
				month, day, year, hour, minute, second,
				Alarm_device [alarm_data->device],
				Alarm_type   [alarm_data->type],
				alarm,
				string, alarm_data->alarm_text);

		    }

		    str = XmStringCreateLocalized (Alarm_buf);

/*		    Display cleared alarms in green background.			*/
                    if( code == 0 ){

		    	XtVaSetValues (label,
				XmNlabelString,	str,
				XmNbackground,	hci_get_read_color (NORMAL_COLOR),
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				NULL);

		    }
		    else {

		    	switch (alarm_data->state) {

				case ORPGRDA_STATE_NOT_APPLICABLE :

				    XtVaSetValues (label,
					XmNlabelString,	str,
					XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
					XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
					NULL);
	
				    break;
	
				case ORPGRDA_STATE_SECONDARY :

				    XtVaSetValues (label,
					XmNlabelString,	str,
					XmNbackground,	hci_get_read_color (WHITE),
					XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
					NULL);
	
				    break;
	
				case ORPGRDA_STATE_MAINTENANCE_MANDATORY :
	
				    XtVaSetValues (label,
					XmNlabelString,	str,
					XmNbackground,	hci_get_read_color (ALARM_COLOR2),
					XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
					NULL);
	
				    break;
	
				case ORPGRDA_STATE_MAINTENANCE_REQUIRED :
	
				    XtVaSetValues (label,
					XmNlabelString,	str,
					XmNbackground,	hci_get_read_color (WARNING_COLOR),
					XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
					NULL);
	
				    break;
	
				case ORPGRDA_STATE_INOPERATIVE :
	
				    XtVaSetValues (label,
					XmNlabelString,	str,
					XmNbackground,	hci_get_read_color (ALARM_COLOR1),
					XmNforeground,	hci_get_read_color (WHITE),
					NULL);
	
			 	    break;

			    }

		    }

		    XmStringFree (str);

		}
	    }
	}

	XtManageChild (Form);
	XtManageChild (Data_scroll);
}

/************************************************************************
 *	Description: This function is called when the RDA Alarm window	*
 *		     is destoryed.					*
 *									*
 *	Input:  w           - ID of top level widget			*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
destroy_rda_alarm_list ()
{
  int status = 0;
  HCI_LE_log( "RDA Alarms Destroy" );
  status = ORPGDA_write( ORPGDAT_HCI_DATA,
                        ( char * ) &Device_filter,
                        sizeof( unsigned int ),
                        HCI_RDA_DEVICE_DATA_MSG_ID );
  return HCI_OK_TO_EXIT;
}

/************************************************************************
 *	Description: This function is called when the Close button is	*
 *		     selected in the RDA Alarms window.			*
 *									*
 *	Input:  w           - ID of button invoking callback		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
close_rda_alarm_list (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  HCI_LE_log( "RDA Alarms Close" );
  XtDestroyWidget( Top_widget );
}

/************************************************************************
 *	Description: This function is called after text is entered in	*
 *		     the search pattern edit box of the RDA Alarms	*
 *		     window.						*
 *									*
 *	Input:  w           - ID of widget invoking callback		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
search_text_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*text;

/*	Get the text associated with the edit box.		*/

	text = XmTextGetString (w);

/*	Save it in the search string buffer.			*/

	strcpy (Search_string, text);

	XtFree (text);
}

/************************************************************************
 *	Description: This function is called when one of the device	*
 *		     filter checkboxes in the RDA Alarms window is	*
 *		     toggled.						*
 *									*
 *	Input:  w           - ID of widget invoking callback		*
 *		client_data - device ID					*
 *		call_data   - toggle state info				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
device_toggle_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Use the widgets client data to determine which device was	*
 *	selected.  Use the associated device mask to set/clear its	*
 *	bit in the device filter variable.				*/

	switch ((int) client_data) {

	    case ORPGRDA_DEVICE_ARC :		/* Archive II */

		if (state->set) {

		    Device_filter = Device_filter | ARC_MASK;

		} else {

		    Device_filter = Device_filter & ~ARC_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_CTR :		/* Control */

		if (state->set) {

		    Device_filter = Device_filter | CTR_MASK;

		} else {

		    Device_filter = Device_filter & ~CTR_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_PED :		/* Pedestal/Antenna */

		if (state->set) {

		    Device_filter = Device_filter | PED_MASK;

		} else {

		    Device_filter = Device_filter & ~PED_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_RSP :		/* Receiver/Signal Processor */

		if (state->set) {

		    Device_filter = Device_filter | RSP_MASK;

		} else {

		    Device_filter = Device_filter & ~RSP_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_USR :		/* User */

		if (state->set) {

		    Device_filter = Device_filter | USR_MASK;

		} else {

		    Device_filter = Device_filter & ~USR_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_UTL :		/* Tower/Utilities */

		if (state->set) {

		    Device_filter = Device_filter | UTL_MASK;

		} else {

		    Device_filter = Device_filter & ~UTL_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_WID :		/* Wideband */

		if (state->set) {

		    Device_filter = Device_filter | WID_MASK;

		} else {

		    Device_filter = Device_filter & ~WID_MASK;

		}

		break;

	    case ORPGRDA_DEVICE_XMT :		/* Transmitter */

		if (state->set) {

		    Device_filter = Device_filter | XMT_MASK;

		} else {

		    Device_filter = Device_filter & ~XMT_MASK;

		}

		break;

	}

/*	Since one of these buttons have been toggled, deactivate	*
 *	the select all alarm messages flag and redisplay the alarm	*
 *	list.								*/

	Select_all = 0;
	display_rda_alarm_list ();
}

/************************************************************************
 *	Description: This function is called when the All/None button	*
 *		     is selected in the RDA Alarms window.		*
 *									*
 *	Input:  w           - ID of button invoking callback		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
select_all_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmString	str;

/*	Set/Clear All flag.					*/

	All = 1 - All;

/*	If cleared, unset all bits in the device filter variable.	*/

	if (All == 0) {

	    Select_all = 0;

/*	    Change the button label to "All" to indicate that selecting	*
 *	    it will set all device bits.				*/

	    sprintf (Alarm_buf,"All");
	    str = XmStringCreateLocalized (Alarm_buf);

	    XtVaSetValues (w,
		XmNlabelString,	str,
		NULL);

	    XmStringFree (str);

	    Device_filter = 0;

	    XtVaSetValues (ARC_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (CTR_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (PED_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (RSP_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (USR_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (UTL_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (WID_button,
		XmNset,	False,
		NULL);
	    XtVaSetValues (XMT_button,
		XmNset,	False,
		NULL);

	} else {	/* Select all device types. */

	    Select_all = 1;

/*	    Change the button label to "None" to indicate that		*
 *	    selecting it will clear all device bits.			*/

	    sprintf (Alarm_buf,"None");
	    str = XmStringCreateLocalized (Alarm_buf);

	    XtVaSetValues (w,
		XmNlabelString,	str,
		NULL);

	    XmStringFree (str);

	    Device_filter = ARC_MASK | CTR_MASK | PED_MASK |
			    RSP_MASK | USR_MASK | UTL_MASK | WID_MASK |
			    XMT_MASK;

	    XtVaSetValues (ARC_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (CTR_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (PED_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (RSP_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (USR_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (UTL_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (WID_button,
		XmNset,	True,
		NULL);
	    XtVaSetValues (XMT_button,
		XmNset,	True,
		NULL);

	}

/*	Redisplay the RDA Alarm message data to reflect the new device	*
 *	filter state.							*/

	display_rda_alarm_list ();

}

/************************************************************************
 *	Description: This function is called when one of the search	*
 *		     date/time edit fields in the RDA Alarms window is	*
 *		     changed.						*
 *									*
 *	Input:  w           - ID of edit box invoking callback		*
 *		client_data - Date/Time field ID			*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
set_date_time_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*text;
	int	ival;
	int	err;
	char	tbuf [8];

/*	Get the new value from the edit box label			*/

	text = XmTextGetString (w);

	err = 0;

/*	Use the client data to determine which date/time field was	*
 *	changed and set the corresponding local data element.		*/

	switch ((int) client_data) {

	    case SEARCH_MONTH :

		if (strlen (text) == 0) {

		    Month = 0;

		} else {

		    sscanf (text,"%d",&ival);

/*		    Validate the month			*/

		    if (ival > 12) {

			sprintf (Alarm_buf,"You entered an invalid month (%d).\nThe valid range is 0 (ignore) to 12.", ival);
			err = 1;

		    } else {

			Month = ival;

		    }

		    sprintf (tbuf,"%2d ",Month);
		    XmTextSetString (w, tbuf);

		}

		break;

	    case SEARCH_DAY :

		if (strlen (text) == 0) {

		    Day = 0;

		} else {

		    sscanf (text,"%d",&ival);

/*		    Validate the day			*/

		    if (ival > 31) {

			sprintf (Alarm_buf,"You entered an invalid day (%d).\nThe valid range is 0 (ignore) to 31.", ival);
			err = 1;

		    } else {

			Day = ival;

		    }

		    sprintf (tbuf,"%2d ",Day);
		    XmTextSetString (w, tbuf);

		}

		break;

	    case SEARCH_YEAR :

		if (strlen (text) == 0) {

		    Year = 0;

		} else {

		    sscanf (text,"%d",&ival);

/*		    Validate the year			*/

		    if (ival != 0 && (ival < 1990 || ival > 2050) ) {

			sprintf (Alarm_buf,"You entered an invalid year (%d).\nThe valid range is 1990 to 2050.", ival);
			err = 1;

		    } else {

			Year = ival;

		    }

		    sprintf (tbuf,"%4d ",Year);
		    XmTextSetString (w, tbuf);

		}

		break;

	    case SEARCH_HOUR :

		if (strlen (text) == 0) {

		    Hour = 0;

		} else {

		    sscanf (text,"%d",&ival);

/*		    Validate the hour			*/

		    if (ival > 23) {

			sprintf (Alarm_buf,"You entered an invalid hour (%d).\nThe valid range is 0 to 23.", ival);
			err = 1;

		    } else {

			Hour = ival;

		    }

		    sprintf (tbuf,"%2.2d ",Hour);
		    XmTextSetString (w, tbuf);

		}

		break;

	    case SEARCH_MINUTE :

		if (strlen (text) == 0) {

		    Minute = 0;

		} else {

		    sscanf (text,"%d",&ival);

/*		    Validate the minute			*/

		    if (ival > 59) {

			sprintf (Alarm_buf,"You entered an invalid minute (%d).\nThe valid range is 0 to 59.", ival);
			err = 1;

		    } else {

			Minute = ival;

		    }

		    sprintf (tbuf,"%2.2d ",Minute);
		    XmTextSetString (w, tbuf);

		}

		break;

	    case SEARCH_SECOND :

		if (strlen (text) == 0) {

		    Second = 0;

		} else {

		    sscanf (text,"%d",&ival);

/*		    Validate the second			*/

		    if (ival > 59) {

			sprintf (Alarm_buf,"You entered an invalid second (%d).\nThe valid range is 0 to 59.", ival);
			err = 1;

		    } else {

			Second = ival;

		    }

		    sprintf (tbuf,"%2.2d ",Second);
		    XmTextSetString (w, tbuf);

		}

		break;

	    default :

		XtFree (text);
		return;

	}

	XtFree (text);

/*	If an error occurred, pop up an informative message.		*/

	if (err) {

	    hci_warning_popup( Top_widget, Alarm_buf, NULL );

	}
}

/************************************************************************
 *      Description: This function is called to filter the messages     *
 *                   according to date/time/search filters.             *
 *                                                                      *
 ***********************************************************************/

void
filter_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
static  int     hhmmss;
static  int     yyyymmdd;

/*	Otherwise, set the date and time data fields.			*/

	Hhmmss   = Hour*10000 + Minute*100 + Second;
	Yyyymmdd = Year*10000 + Month*100 + Day;

/*	If the new data and time or search string is different		*
 *	from before, redisplay the RDA alarm message list.		*/

	if ((Hhmmss   != hhmmss) ||
            (Yyyymmdd != yyyymmdd) ||
	    (strcmp (Old_search_string, Search_string))) {

	      hhmmss = Hhmmss;
              yyyymmdd = Yyyymmdd;
	      strcpy (Old_search_string, Search_string);
              display_rda_alarm_list ();
	}
}

/************************************************************************
 *	Description: This function is called when one of the edit box	*
 *		     defining the maximum number of displayable alarms	*
 *		     in the RDA Alarms window is changed.		*
 *									*
 *	Input:  w           - ID of edit box invoking callback		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
set_max_alarms_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*text;

	text = XmTextGetString (w);

	sscanf (text,"%d",&Max_displayable_alarms);

	XtFree (text);

	display_rda_alarm_list ();
}

/************************************************************************
 *	Description: This function is called when the Clear button	*
 *		     is selected in the RDA Alarms window.		*
 *									*
 *	Input:  w           - ID of button invoking callback		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
clear_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
/*	Clear all filter fields.	*/

	Hhmmss   = 0;
	Yyyymmdd = 0;
	Month    = 0;
	Day      = 0;
	Year     = 0;
	Hour     = 0;
	Minute   = 0;
	Second   = 0;
	strcpy (Old_search_string,"");
	strcpy (Search_string,"");

	XmTextSetString (Search_text,   "");
	XmTextSetString (Search_month,  "");
	XmTextSetString (Search_day,    "");
	XmTextSetString (Search_year,   "");
	XmTextSetString (Search_hour,   "");
	XmTextSetString (Search_minute, "");
	XmTextSetString (Search_second, "");

	display_rda_alarm_list ();
}

/************************************************************************
 *      Description:  The following function is the callback invoked    *
 *      when a new message is written to the RDA device filter message  *
 *      LB                                      .                       *
 *                                                                      *
 *      Input:  fd       - File descriptor of LB with new message       *
 *              msg_id   - The ID of the new message in the LB          *
 *              msg_info - The length (bytes) of the new message        *
 *              *arg     - user registered argument (unused).           *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
                                                                                
void
device_filter_updated (
int     fd,
LB_id_t msg_id,
int     msg_info,
void    *arg
)
{
  device_filter_updated_flag = 1;
}

/************************************************************************
 *      Description:  The following function reads the RDA device	*
 *      filter message LB.						*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
                                                                                
void read_device_filter()
{
	int status = 0;

	device_filter_updated_flag = 0;

/*	Read in device filter value.					*/
   
	status = ORPGDA_read( ORPGDAT_HCI_DATA,
			      ( char * ) &Device_filter,
			      sizeof( unsigned int ),
			      HCI_RDA_DEVICE_DATA_MSG_ID );

/*	If device filter read is successful, use it,	*/
/*	if it isn't, assume all filters are set.	*/

        if( status <= 0 )
	{
	  HCI_LE_error( "Error (%d) reading RDA device filter from HCI_DATA LB, assume all filters are set.", status );
	  Device_filter = ARC_MASK | CTR_MASK | PED_MASK | RSP_MASK |
	 		USR_MASK | UTL_MASK | WID_MASK | XMT_MASK;
	  Select_all= 1;
	}
        else if( (Device_filter == 0) || (Device_filter != (ARC_MASK | CTR_MASK | PED_MASK | RSP_MASK |
                                                            USR_MASK | UTL_MASK | WID_MASK | XMT_MASK)) )
        {
          Select_all = 0;
          All = 0;
        }

	if( Device_filter & ARC_MASK )
	{
	  XtVaSetValues( ARC_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( ARC_button, XmNset, False, NULL );
	}

	if( Device_filter & CTR_MASK )
	{
	  XtVaSetValues( CTR_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( CTR_button, XmNset, False, NULL );
	}

	if( Device_filter & PED_MASK )
	{
	  XtVaSetValues( PED_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( PED_button, XmNset, False, NULL );
	}

	if( Device_filter & RSP_MASK )
	{
	  XtVaSetValues( RSP_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( RSP_button, XmNset, False, NULL );
	}

	if( Device_filter & USR_MASK )
	{
	  XtVaSetValues( USR_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( USR_button, XmNset, False, NULL );
	}

	if( Device_filter & UTL_MASK )
	{
	  XtVaSetValues( UTL_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( UTL_button, XmNset, False, NULL );
	}

	if( Device_filter & WID_MASK )
	{
	  XtVaSetValues( WID_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( WID_button, XmNset, False, NULL );
	}

	if( Device_filter & XMT_MASK )
	{
	  XtVaSetValues( XMT_button, XmNset, True, NULL );
	}
        else
        {
	  XtVaSetValues( XMT_button, XmNset, False, NULL );
	}

	Select_all = 0;
	display_rda_alarm_list();
}

/************************************************************************
 *  Description: This function is called when the RDA configuration     *
 *         changes.                                                     *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

void rda_config_change()
{
   if( Top_widget != (Widget) NULL && !Config_change_popup_flag )
   {
     Config_change_popup_flag = 1;
     hci_rda_config_change_popup();
   }
   else
   {
     return;
   }
}

