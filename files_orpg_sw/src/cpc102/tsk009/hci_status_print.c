/****************************************************************
 *								*
 *	hci_RPG_status_print.c - This task is used to display	*
 *	the RPG syslog and allow user to print some or all.	*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:48:00 $
 * $Id: hci_status_print.c,v 1.5 2010/03/10 18:48:00 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Macros.								*/

#define	MAX_MSGS_DISPLAY	250 /* Max # messages to display */
#define	MAX_MSGS_BUFFER		10000 /* Max # messages allowed in buffer */
#define	HCI_PRINT_FILENAME	"/tmp/.rpg_system_log" /* temp filename */

/*	Global widget definitions					*/

Widget		Top_widget     = (Widget) NULL;
Widget		Filter_msgs_button = (Widget) NULL;
Widget		Filter_error_button  = (Widget) NULL;
Widget		Page_left_button     = (Widget) NULL;
Widget		Page_right_button    = (Widget) NULL;
Widget		Msg_frame      = (Widget) NULL;
Widget		Msg_scroll     = (Widget) NULL;
Widget		Msg_form       = (Widget) NULL;
Widget		Msg_entry [MAX_MSGS_DISPLAY] = {(Widget) NULL};
Widget		Details_frame  = (Widget) NULL;
Widget		Details_scroll = (Widget) NULL;
Widget		Details_form   = (Widget) NULL;
Widget		Update_text    = (Widget) NULL; /* unused */

Widget		Print_dialog   = (Widget) NULL;
Widget		Print_all      = (Widget) NULL;
Widget		Print_msg_num  = (Widget) NULL;
Widget		Print_rowcol   = (Widget) NULL;
Widget		Date_text      = (Widget) NULL;
Widget		Time_text      = (Widget) NULL;
Widget		Log_label      = (Widget) NULL;
Widget		Filter_text    = (Widget) NULL;

Widget		Message_rowcol = (Widget) NULL;
Widget		Filter_frame   = (Widget) NULL;

/*	Widgets used to identify RPG alarms		*/

Widget		Filter_msgs_dialog       = (Widget) NULL;
Widget		Rda_alarm_SEC_button     = (Widget) NULL;
Widget		Rda_alarm_MR_button      = (Widget) NULL;
Widget		Rda_alarm_MM_button      = (Widget) NULL;
Widget		Rda_alarm_INOP_button    = (Widget) NULL;
Widget		Rpg_alarm_MR_button      = (Widget) NULL;
Widget		Rpg_alarm_MM_button      = (Widget) NULL;
Widget		Rpg_alarm_LS_button      = (Widget) NULL;
Widget		Rpg_info_button          = (Widget) NULL;
Widget		Rpg_gen_status_button    = (Widget) NULL;
Widget		Rpg_warn_status_button   = (Widget) NULL;
Widget		Rpg_comms_status_button  = (Widget) NULL;

/* To save the state of small rda filter window */

Boolean         RDA_SEC_stat = True;
Boolean         RDA_MR_stat = True;
Boolean         RDA_MM_stat = True;
Boolean         RDA_INOP_stat = True;
Boolean         RPG_info_stat = True;
Boolean         RPG_gen_stat = True;
Boolean         RPG_warn_stat = True;
Boolean         RPG_comms_stat = True;
Boolean         RPG_MR_stat = True;
Boolean         RPG_MM_stat = True;
Boolean         RPG_LS_warn_stat = True;
Boolean         RPG_LS_alarm_stat = True;

char		Old_search_string [128]  = {""}; /* old search filter string */
char		Search_string [128]  = {""};     /* new search filter string */

int		New_msg_flag = 1; /* new log message flag */

int		List_index = 1; /* current selected list item */
LB_status	Log_status; /* system log file status data */
int		Msgs_read = 0; /* number of log messages read */
int		Print_num_msgs = MAX_MSGS_BUFFER; /* number of messages to print */
int		Print_all_flag = 1; /* print all messages flag */
int		Active_page    = 0; /* Current display page */

#define	FILTER_RPG_GENERAL_STATUS_MSG		0x0001 /* status type message filter */
#define	FILTER_RPG_WARNING_STATUS_MSG		0x0002 /* warning type message filter */
#define FILTER_RPG_INFO_STATUS_MSG              0x0004 /* information message filter */
#define FILTER_RPG_COMMS_STATUS_MSG             0x0008 /* NB comms message filter */
#define FILTER_RDA_SEC_ALARM_ACTIVATED_MSG	0x0010 /* RDA Secondary alarm type
							  message filter */
#define FILTER_RDA_MAR_ALARM_ACTIVATED_MSG	0x0020 /* RDA MAR alarm type
							  message filter */
#define FILTER_RDA_MAM_ALARM_ACTIVATED_MSG	0x0040 /* RDA MAM alarm type
							  message filter */
#define FILTER_RDA_INOP_ALARM_ACTIVATED_MSG	0x0080 /* RDA INOP alarm type
							  message filter */
#define FILTER_RDA_NA_ALARM_ACTIVATED_MSG	0x0100 /* RDA Not Applicable alarm type
							  message filter */
#define FILTER_RDA_ALARM_CLEARED_MSG		0x0200 /* RDA alarm cleared type
							  message filter */
#define FILTER_RPG_MAR_ALARM_ACTIVATED_MSG	0x0400 /* RPG MAR alarm type
							  message filter */
#define FILTER_RPG_MAM_ALARM_ACTIVATED_MSG	0x0800 /* RPG MAM alarm type
							  message filter */
#define FILTER_RPG_LS_ALARM_ACTIVATED_MSG	0x1000 /* RPG LS alarm type
							  message filter */
#define FILTER_RPG_ALARM_CLEARED_MSG		0x2000 /* RPG alarm cleared type
							  message filter */

/*	Filter data */

int	Filter_status = FILTER_RPG_GENERAL_STATUS_MSG       |
			FILTER_RPG_WARNING_STATUS_MSG       |
			FILTER_RPG_INFO_STATUS_MSG          |
			FILTER_RPG_COMMS_STATUS_MSG         |
			FILTER_RDA_SEC_ALARM_ACTIVATED_MSG  |
			FILTER_RDA_MAR_ALARM_ACTIVATED_MSG  |
			FILTER_RDA_MAM_ALARM_ACTIVATED_MSG  |
			FILTER_RDA_INOP_ALARM_ACTIVATED_MSG |
			FILTER_RDA_NA_ALARM_ACTIVATED_MSG   |
			FILTER_RDA_ALARM_CLEARED_MSG        |
			FILTER_RPG_MAR_ALARM_ACTIVATED_MSG  |
			FILTER_RPG_MAM_ALARM_ACTIVATED_MSG  |
			FILTER_RPG_ALARM_CLEARED_MSG        |
			FILTER_RPG_LS_ALARM_ACTIVATED_MSG; 

char	Sys_msg  [MAX_MSGS_BUFFER][HCI_LE_MSG_MAX_LENGTH]; /* system log message data */
int	Msg_type [MAX_MSGS_BUFFER] = {0};              /* system log message type */

/*	Date/Time filtering data */

int	Date   = 0;
int	Hhmmss = 0;
int	Month  = 0;
int	Day    = 0;
int	Year   = 0;
int	Hour   = 0;
int	Minute = 0;
int	Second = 0;

void	close_system_status_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_msg_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	search_text_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_log_message_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	update_max_msgs_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	filter_button_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	print_log_messages_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	print_message_number_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	print_all_messages_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	print_print_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	print_close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	clear_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_page_button_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_select_msgs_callback (Widget w,
		XtPointer client_data, XtPointer call_data);

void	new_system_log_msg (int fd, LB_id_t msg_id,
		int msg_info, void *arg);
void	update_system_log_msgs ();
void	timer_proc();

void	display_system_status_msgs (int mode);
void    toggle_rda_alarms_button(Widget w);
void    toggle_rpg_alarms_button(Widget w);
void    toggle_rpg_msgs_button(Widget w);

/************************************************************************
 *	Description: This is the main function for the RPG Status task.	*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	Widget	button;
	Widget	label;
	Widget	form;
	Widget	rowcol;
	Widget	filter_frame;
	Widget	filter_rowcol;
	int		status;
	char		*msg;
	LE_critical_message	*le_msg;
	int		offset;
	int		le_msg_len;
	XmString	str;
	char		buf [16];

        unsigned int rpg_msg, rpg_alarm, rda_alarm ;
	
	/*  Initialize HCI. */

	HCI_init( argc, argv, HCI_STATUS_PRINT_TOOL );

	Top_widget = HCI_get_top_widget();

/*	If low bandwidth, display a progress meter.			*/

	HCI_PM( "Reading System Log Messages" );


/*	First check to see how many messages the log msg LB currently	*
 *	has in it.  This is what we will read if it is less than the	*
 *	max number of messages to process.				*/

	Msgs_read = 0;

	status = ORPGDA_stat (ORPGDAT_SYSLOG, &Log_status);

	if (status == LB_SUCCESS) {

	    if (Log_status.n_msgs > MAX_MSGS_BUFFER) {

		status = ORPGDA_seek (ORPGDAT_SYSLOG,
				  -(MAX_MSGS_BUFFER-1),
				  LB_LATEST,
				  NULL);

	    } else {

		status = ORPGDA_seek (ORPGDAT_SYSLOG,
				  0,
				  LB_FIRST,
				  NULL);

	    }

	    msg = calloc (HCI_LE_MSG_MAX_LENGTH*MAX_MSGS_BUFFER,1);

	    if (status >= 0) {

		status = ORPGDA_read (ORPGDAT_SYSLOG,
		      (char *) msg,
		      HCI_LE_MSG_MAX_LENGTH*MAX_MSGS_BUFFER,
		      (LB_MULTI_READ | (MAX_MSGS_BUFFER - 1)));
	    }

	    if (status >= 0) {

		offset = 0;

		while (status > offset) {

		    le_msg = (LE_critical_message *)(msg+offset);
		    le_msg_len = ALIGNED_SIZE (sizeof (LE_critical_message)+
					       strlen (le_msg->text));
		    offset += le_msg_len;

		    memcpy (Sys_msg [Msgs_read], le_msg, le_msg_len);

		    /* 
                       Get the RPG/RDA alarm level so we can set the appropriate message type. 
                       The message type is used for:

                       1) color encoding the messages, and 
                       2) message filtering
                    */
		    rpg_msg = (le_msg->code & HCI_LE_RPG_STATUS_MASK);
                    rpg_alarm = (le_msg->code & HCI_LE_RPG_ALARM_MASK);
		    rda_alarm = (le_msg->code & HCI_LE_RDA_ALARM_MASK);
#ifdef debugit
HCI_LE_log("le_msg->code: %x, le_msg->text: %s", le_msg->code, le_msg->text );
HCI_LE_log("--->rpg_msg: %x, rpg_alarm: %x, rda_alarm: %x", rpg_msg, rpg_alarm, rda_alarm );
#endif

                    /* Check if it is an RPG alarm. */
                    if( rpg_alarm ){

                        switch( rpg_alarm ){

			   case HCI_LE_RPG_ALARM_MAR:
			   default:
			        Msg_type [Msgs_read] = FILTER_RPG_MAR_ALARM_ACTIVATED_MSG;
                                break;

			   case HCI_LE_RPG_ALARM_MAM:
			        Msg_type [Msgs_read] = FILTER_RPG_MAM_ALARM_ACTIVATED_MSG;
                                break;

			   case HCI_LE_RPG_ALARM_LS:
			        Msg_type [Msgs_read] = FILTER_RPG_LS_ALARM_ACTIVATED_MSG;
                                break;

                        }

		    	if( le_msg->code & HCI_LE_RPG_ALARM_CLEAR )
			    Msg_type [Msgs_read] |= FILTER_RPG_ALARM_CLEARED_MSG;

                    /* Check if it is an RDA alarm. */
		    } else if ( rda_alarm ){

			switch( rda_alarm ){

                           case HCI_LE_RDA_ALARM_NA:
			   default:
			        Msg_type [Msgs_read] = FILTER_RDA_NA_ALARM_ACTIVATED_MSG;
                                break;

			   case HCI_LE_RDA_ALARM_SEC:
			        Msg_type [Msgs_read] = FILTER_RDA_SEC_ALARM_ACTIVATED_MSG;
                                break;

			   case HCI_LE_RDA_ALARM_MAR:
			        Msg_type [Msgs_read] = FILTER_RDA_MAR_ALARM_ACTIVATED_MSG;
                                break;

			   case HCI_LE_RDA_ALARM_MAM:
			        Msg_type [Msgs_read] = FILTER_RDA_MAM_ALARM_ACTIVATED_MSG;
                                break;

			   case HCI_LE_RDA_ALARM_INOP:
			        Msg_type [Msgs_read] = FILTER_RDA_INOP_ALARM_ACTIVATED_MSG;
                                break;

                        }

		    	if( le_msg->code & HCI_LE_RDA_ALARM_CLEAR )
			    Msg_type [Msgs_read] |= FILTER_RDA_ALARM_CLEARED_MSG;

                    /* Check for RPG message. */
		    } else if ( rpg_msg ){

			if( (rpg_msg == HCI_LE_RPG_STATUS_WARN) ) 
			    Msg_type [Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;

		        else if (rpg_msg == HCI_LE_RPG_STATUS_INFO)
			    Msg_type [Msgs_read] = FILTER_RPG_INFO_STATUS_MSG;

		        else if (rpg_msg == HCI_LE_RPG_STATUS_COMMS) 
			    Msg_type [Msgs_read] = FILTER_RPG_COMMS_STATUS_MSG;

                        else if(rpg_msg == HCI_LE_RPG_STATUS_GEN)
			    Msg_type [Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;

                    /* All other messages. */
		    } else {

			if( le_msg->code & GL_ERROR_BIT )
			    Msg_type [Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;

		        else
			    Msg_type [Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;

		    }
#ifdef debugit
HCI_LE_log("Msg_type: %x", Msg_type[Msgs_read] );
#endif

		    Msgs_read++;

		    if (Msgs_read >= MAX_MSGS_BUFFER) {

			HCI_LE_error("Number of messages read excedes number allowed.");
			break;

		    }
		}
	    }

	    free (msg);

	}

/*	Build wigets.							*/

	form = XtVaCreateWidget ("rpg_status_form",
		xmFormWidgetClass,	Top_widget,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	HCI_PM( "Initialize RPG Status Information" );		

	rowcol = XtVaCreateWidget ("rpg_status_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, close_system_status_callback,
		NULL);

	button = XtVaCreateManagedWidget ("Print Log Messages",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, print_log_messages_callback,
		NULL);

	XtManageChild (rowcol);

	filter_frame = XtVaCreateManagedWidget ("filter_frame",
		xmFrameWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Message Filter",
		xmLabelWidgetClass,	filter_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	filter_rowcol = XtVaCreateWidget ("filter_rowcol",
		xmRowColumnWidgetClass,	filter_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_TIGHT,
		NULL);

	XtVaCreateManagedWidget ("Prev",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Page_left_button = XtVaCreateManagedWidget ("page_left",
		xmArrowButtonWidgetClass,	filter_rowcol,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNsensitive,			False,
		XmNarrowDirection,		XmARROW_LEFT,
		XmNborderWidth,			0,
		NULL);

	XtAddCallback (Page_left_button,
		XmNactivateCallback, hci_page_button_callback,
		(XtPointer) XmARROW_LEFT);

	label = XtVaCreateManagedWidget ("page_label",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	sprintf (buf,"--");
	str = XmStringCreateLocalized (buf);

	XtVaSetValues (label,
		XmNlabelString,	str,
		NULL);

	XmStringFree (str);

	Page_right_button = XtVaCreateManagedWidget ("page_right",
		xmArrowButtonWidgetClass,	filter_rowcol,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNsensitive,			False,
		XmNarrowDirection,		XmARROW_RIGHT,
		XmNborderWidth,			0,
		NULL);

	XtAddCallback (Page_right_button,
		XmNactivateCallback, hci_page_button_callback,
		(XtPointer) XmARROW_RIGHT);

	XtVaCreateManagedWidget ("Next",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("  ",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Filter_msgs_button = XtVaCreateManagedWidget ("Message Filter",
		xmPushButtonWidgetClass,	filter_rowcol,
		XmNselectColor,			hci_get_read_color (WARNING_COLOR),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Filter_msgs_button,
                       XmNactivateCallback, hci_select_msgs_callback, NULL);

	label = XtVaCreateManagedWidget ("  ",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("  Search: ",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Filter_text = XtVaCreateManagedWidget ("filter_text",
		xmTextFieldWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		12,
		NULL);

	if (strlen (Search_string)) {

	    XmTextSetString (Filter_text, Search_string);

	}

	XtAddCallback (Filter_text,
		XmNactivateCallback, search_text_callback, NULL);
	XtAddCallback (Filter_text,
		XmNlosingFocusCallback, search_text_callback, NULL);

	XtVaCreateManagedWidget ("Sp",
		xmLabelWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	button = XtVaCreateManagedWidget ("Clear",
		xmPushButtonWidgetClass,	filter_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, clear_callback, NULL);

	XtManageChild (filter_rowcol);

	Msg_frame = XtVaCreateManagedWidget ("Msg_frame",
		xmFrameWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		filter_frame,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("System Log Messages",
		xmLabelWidgetClass,	Msg_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Msg_scroll = XtVaCreateManagedWidget ("Msg_scroll",
		xmScrolledWindowWidgetClass,	Msg_frame,
		XmNheight,		382,
		XmNwidth,		740,
		XmNscrollingPolicy,	XmAUTOMATIC,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	{
		Widget	clip;

		XtVaGetValues (Msg_scroll,
			XmNclipWindow,	&clip,
			NULL);

		XtVaSetValues (clip,
			XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

	}

	display_system_status_msgs (1);

	XtManageChild (form);

/*	Register for system log update events.			*/

	status = ORPGDA_UN_register (ORPGDAT_SYSLOG, LB_ANY, new_system_log_msg);

	if (status != LB_SUCCESS) {

	    HCI_LE_error("ORPGDA_UN_register (ORPGDAT_SYSLOG) failed (%d)",
		status);

	}

/*	Register for system log expire events (handle clearing log).	*/

	status = ORPGDA_UN_register (ORPGDAT_SYSLOG, LB_MSG_EXPIRED, new_system_log_msg);

	if (status != LB_SUCCESS) {

	    HCI_LE_error("ORPGDA_UN_register (ORPGDAT_SYSLOG) failed (%d)",
		status);

	}

	XtRealizeWidget(Top_widget);

	HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This is the timer proc for the RPG Status task.	*
 *		     It checks to see if any pertinent RPG status has	*
 *		     changed or if new messages have been written to	*
 *		     the system status log.  If so, the objects in the	*
 *		     window are updated.				*
 *									*
 *	Input:  w - top level widget ID					*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
	int	status;
static	unsigned int	old_rpgalrm = 0;
	unsigned int	new_rpgalrm;
static	int	old_rpg_state = 0;
static	int	old_rpg_mode  = 0;
static	int	old_value           = 0;
	int	cur_value;
static	int	old_alarm_threshold = 0;
	int	cur_alarm_threshold;
static	int	old_wxmode  = -1;
static	int	old_wb_status = -1;
	int	new_wxmode;
	int	update_flag = 0;
	Mrpg_state_t	mrpg;

	update_flag = 0;

/*	If the RDA-RPG Interface status has changed we want to refresh	*
 *	the display.							*/

	status = ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT);

	if (status != old_wb_status) {

	    update_flag = 1;
	    old_wb_status = status;

	}

/*	Next we want to check the RPG state and mode.  If either has	*
 *	changed we want to refresh the display.				*/

	status = ORPGMGR_get_RPG_states (&mrpg);

	if (status == 0) {

/*	    First the RPG state		*/

	    if (old_rpg_state != mrpg.state) {

		update_flag = 1;
		old_rpg_state = mrpg.state;

	    }

/*	    Next the RPG operability mode (test or operate)	*/

	    if (old_rpg_mode != mrpg.test_mode) {

		update_flag = 1;
		old_rpg_mode = mrpg.test_mode;

	    }
	}

/*	Next, lets see if the state of the RPG alarm bits have change.	*
 *	If so, we want to refresh the display.				*/

	status = ORPGINFO_statefl_get_rpgalrm (&new_rpgalrm);

	if (status == 0) {

	    if (old_rpgalrm != new_rpgalrm) {

		update_flag = 1;
		old_rpgalrm = new_rpgalrm;

	    }
	}

/*	If the weather mode has changed we want to force a refresh.	*/

	new_wxmode = ORPGVST_get_mode ();

	if (old_wxmode != new_wxmode) {

	    update_flag = 1;
	    old_wxmode = new_wxmode;

	}

/*	Next, we need to check the product distribution load shed	*
 *	data to see if we need to update the Distribution load shed	*
 *	alarm.								*/

	status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
				    LOAD_SHED_CURRENT_VALUE,
				    &cur_value);

	if (status == 0) {

	    if (cur_value != old_value) {

		old_value   = cur_value;
		update_flag = 1;

	    }
	}

	status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
				    LOAD_SHED_ALARM_THRESHOLD,
				    &cur_alarm_threshold);

	if (status == 0) {

	    if (cur_alarm_threshold != old_alarm_threshold) {

		old_alarm_threshold = cur_alarm_threshold;
		update_flag         = 1;

	    }
	}

/*	If any new system status log messages were written we need to	*
 *	update the display list.					*/

	if (New_msg_flag) {

	    New_msg_flag = 0;
	    update_system_log_msgs ();

	}
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
close_system_status_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("RPG Status Close selected");
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This function displays the lastest RPG system log	*
 *		     messages in the RPG Status window.  Each message	*
 *		     is color coded based on its type.			*
 *									*
 *	Input:  mode - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_system_status_msgs (
int	mode
)
{
	int	i;
	int	cnt;
	int	msg_cnt;
	char	buf [256];
	int	month, day, year;
	int	hour, minute, second;
	int	value;
	int	slider_size;
static	int	old_cnt = 0;
static	int	old_filter_status = 0;
	char	*ptr;
	Widget	vsb;	 /* Widget ID of vertical scroll bar */
	LE_critical_message	*le_msg;

/*	If the message list hasn't been created, create it now.		*/

	if (Msg_form == (Widget) NULL) {

	    Msg_form = XtVaCreateWidget ("Msg_form",
		xmFormWidgetClass,	Msg_scroll,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNverticalSpacing,	0,
		NULL);

	    Msg_entry [0] = XtVaCreateManagedWidget ("label",
			xmTextWidgetClass, Msg_form,
			XmNfontList,	hci_get_fontlist (LIST),
			XmNcolumns,	128,
			XmNeditable,	False,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNmarginHeight,	0,
			XmNborderWidth,		0,
			XmNshadowThickness,	0,
			XmNhighlightColor,	hci_get_read_color (BLACK),
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

	    XtAddCallback (Msg_entry [0],
			XmNfocusCallback, select_log_message_callback,
			(XtPointer) 0);

	    for (i=1;i<MAX_MSGS_DISPLAY;i++) {

		Msg_entry [i] = XtVaCreateManagedWidget ("label",
			xmTextWidgetClass, Msg_form,
			XmNfontList,	hci_get_fontlist (LIST),
			XmNcolumns,	128,
			XmNeditable,	False,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		Msg_entry [i-1],
			XmNmarginHeight,	0,
			XmNborderWidth,		0,
			XmNshadowThickness,	0,
			XmNhighlightColor,	hci_get_read_color (BLACK),
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

		XtAddCallback (Msg_entry [i],
			XmNfocusCallback, select_log_message_callback,
			(XtPointer) i);

	    }
	}

	msg_cnt = 0;
	cnt     = 0;

/*	For each message read from the log, put it in the list and set	*
 *	the background color based on meaage severity.			*/

	for (i=Msgs_read-1;i>=0;i--) {

	    if (cnt >= MAX_MSGS_DISPLAY)
		break;

	    le_msg = (LE_critical_message *) Sys_msg [i];

	    unix_time (&le_msg->time,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

	    year = year%100;

	    ptr = strstr (le_msg->text, ":");

	    if (ptr == NULL) {

		ptr = le_msg->text;

	    } else {

		ptr = ptr + 1;

	    }

	    sprintf (buf,"%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
			HCI_get_month(month), day, year, hour,
			minute, second, ptr);

	    if ((!strlen (Search_string)) ||
		(strlen (Search_string) &&
		(hci_string_in_string (buf,Search_string) != 0))) {

#ifdef debugit
HCI_LE_log("Msg_type: %x, Filter_status: %x, Msg: %s",
             Msg_type[i], Filter_status, ptr );
#endif
		if ( (Msg_type [i] & Filter_status) == Msg_type[i] ) {

		    if (msg_cnt >= Active_page*MAX_MSGS_DISPLAY) {

			if ((Msg_type [i] & FILTER_RPG_INFO_STATUS_MSG) == 
			     FILTER_RPG_INFO_STATUS_MSG) {

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (GRAY),
				NULL);

			} else if ((Msg_type [i] & FILTER_RPG_GENERAL_STATUS_MSG) == 
				    FILTER_RPG_GENERAL_STATUS_MSG) {

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
				NULL);

			} else if ((Msg_type [i] & FILTER_RPG_COMMS_STATUS_MSG) == 
				    FILTER_RPG_COMMS_STATUS_MSG) {

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (WHITE),
				XmNbackground,	hci_get_read_color (SEAGREEN),
				NULL);

                        /* This needs to be before the checks for RPG Alarm Type to
                           ensure the alarm is displayed in the proper color. */
			} else if ((Msg_type [i] & FILTER_RPG_ALARM_CLEARED_MSG)) {

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (NORMAL_COLOR),
				NULL);

			} else if ((Msg_type [i] & FILTER_RPG_MAM_ALARM_ACTIVATED_MSG) == 
				      FILTER_RPG_MAM_ALARM_ACTIVATED_MSG ){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (ALARM_COLOR2),
				NULL);

			} else if ((Msg_type [i] & FILTER_RPG_MAR_ALARM_ACTIVATED_MSG) == 
				      FILTER_RPG_MAR_ALARM_ACTIVATED_MSG){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (WARNING_COLOR),
				NULL);

			} else if ((Msg_type [i] & FILTER_RPG_LS_ALARM_ACTIVATED_MSG) ==
				       FILTER_RPG_LS_ALARM_ACTIVATED_MSG) {

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (CYAN),
				NULL);

			} else if ((Msg_type [i] & FILTER_RPG_WARNING_STATUS_MSG) == 
				    FILTER_RPG_WARNING_STATUS_MSG ){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (WARNING_COLOR),
				NULL);

                        /* This needs to be before the checks for RDA Alarm Type to
                           ensure the alarm is displayed in the proper color. */
			} else if (Msg_type [i] & FILTER_RDA_ALARM_CLEARED_MSG) {

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (NORMAL_COLOR),
				NULL);

			} else if ((Msg_type [i] & FILTER_RDA_NA_ALARM_ACTIVATED_MSG) == 
				    FILTER_RDA_NA_ALARM_ACTIVATED_MSG){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
				NULL);

			} else if ((Msg_type [i] & FILTER_RDA_SEC_ALARM_ACTIVATED_MSG) == 
				    FILTER_RDA_SEC_ALARM_ACTIVATED_MSG){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (WHITE),
				NULL);

			} else if ((Msg_type [i] & FILTER_RDA_MAM_ALARM_ACTIVATED_MSG) ==
				    FILTER_RDA_MAM_ALARM_ACTIVATED_MSG){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (ALARM_COLOR2),
				NULL);

			} else if ((Msg_type [i] & FILTER_RDA_MAR_ALARM_ACTIVATED_MSG) == 
				    FILTER_RDA_MAR_ALARM_ACTIVATED_MSG){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (WARNING_COLOR),
				NULL);

			} else if ( (Msg_type [i] & FILTER_RDA_INOP_ALARM_ACTIVATED_MSG) == 
				     FILTER_RDA_INOP_ALARM_ACTIVATED_MSG){

			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (WHITE),
				XmNbackground,	hci_get_read_color (ALARM_COLOR1),
				NULL);

			} else {
			
			    XtVaSetValues (Msg_entry [cnt],
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
				NULL);

			}


			XmTextSetString (Msg_entry [cnt], buf);

			XtVaSetValues (Msg_entry [cnt],
				XmNuserData,	(XtPointer) i,
				NULL);

			cnt++;

		    }

		    msg_cnt++;

		}
	    }
	}

	if (i > 0) {

	    XtVaSetValues (Page_right_button,
		XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
		XmNsensitive,	True,
		NULL);

	} else {

	    XtVaSetValues (Page_right_button,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);

	}

	if (Active_page == 0) {

	    XtVaSetValues (Page_left_button,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);

	} else {

	    XtVaSetValues (Page_left_button,
		XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
		XmNsensitive,	True,
		NULL);

	}

	if (cnt < old_cnt) {

	    for (i=cnt;i<old_cnt;i++) {

		XmTextSetString (Msg_entry [i], " ");

		XtVaSetValues (Msg_entry [i],
			XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNuserData,	(XtPointer) -1,
			NULL);


	    }
	}

	old_cnt = cnt;

	XtManageChild (Msg_form);
	XtManageChild (Msg_scroll);

/*	We want to get the ID of the vertical scrollbar so we can	*
 *	control the scroll increment to keep it from jumping around.	*/

	XtVaGetValues (Msg_scroll,
		XmNverticalScrollBar,	&vsb,
		NULL);

	XtVaGetValues (vsb,
		XmNsliderSize,		&slider_size,
		XmNvalue,		&value,
		NULL);

	if ((Filter_status != old_filter_status) ||
	    (mode)) {

	    old_filter_status = Filter_status;

	    XmScrollBarSetValues (vsb, 1, slider_size, 10, 100, True);

	} else {

	    XmScrollBarSetValues (vsb, value, slider_size, 10, 100, False);

	}
}

/************************************************************************
 *	Description: This function is activated when the user defines	*
 *		     a search string in the Search edit box.		*
 *									*
 *	Input:  w - search text widget ID				*
 *		client_data - unused					*
 *		call_data - unused					*
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

	Active_page = 0;

	XtVaSetValues (Page_left_button,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);

	text = XmTextGetString (w);

	strcpy (Search_string, text);

	XtFree (text);

/*	If the new string is dfferent from the old one, redisplay the	*
 *	system status log messages.					*/

	if ((strlen (Search_string) != strlen (Old_search_string)) &&
	    strcmp (Search_string,Old_search_string)) {

	    display_system_status_msgs (1);
	    strcpy (Old_search_string, Search_string);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Clear" button.  The contents of the search	*
 *		     string exit box is cleared.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
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
	strcpy (Old_search_string, "");
	strcpy (Search_string, "");

	XmTextSetString (Filter_text, "");

	display_system_status_msgs (1);

}

/************************************************************************
 *	Description: This function is activated when a new system log	*
 *		     message is written to the system log file.		*
 *									*
 *	Input:  fd - file descriptor					*
 *		msg_id - ID of updated message				*
 *		msg_info - length (bytes) of new message		*
 *		arg - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
new_system_log_msg (
int	fd,
LB_id_t	msg_id,
int	msg_info,
void	*arg
)
{
	New_msg_flag = 1;
}

/************************************************************************
 *	Description: This function reads all new system status log	*
 *		     messages and displays them.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
update_system_log_msgs (
)
{
static	int	num_msgs = 0;
	int	status;
	int	i;
	char	*new_msg;
	LB_status	lstat;
	LE_critical_message	*le_msg;

	unsigned int rpg_msg, rpg_alarm, rda_alarm;

	lstat.attr    = NULL;
	lstat.n_check = 0;

	if (ORPGDA_stat (ORPGDAT_SYSLOG, &lstat) == LB_SUCCESS) {

/*	    If the number of messages in the log is smaller than the	*
 *	    last time, then we assume it has been cleaned and needs	*
 *	    to be initialized.						*/

	    if (lstat.n_msgs < num_msgs) {

		ORPGDA_seek (ORPGDAT_SYSLOG, 0, LB_FIRST, NULL);
		Msgs_read = 0;

	    }

	    num_msgs = lstat.n_msgs;

	}

	while (1) {

	    status = ORPGDA_read (ORPGDAT_SYSLOG,
		      (char *) &new_msg,
		      LB_ALLOC_BUF,
		      LB_NEXT);

	    if (status < 0) {

		break;

	    }

/*	    if the message buffer is full, then we must expire the	*
 *	    oldest message before we can add the new message.  Lets	*
 *	    shift everything in the buffer one place.			*/

	    if (Msgs_read == MAX_MSGS_BUFFER) {

		for (i=1;i<MAX_MSGS_BUFFER;i++) {

		    memcpy (Sys_msg [i-1], Sys_msg [i], HCI_LE_MSG_MAX_LENGTH);
		    Msg_type [i-1] = Msg_type [i];

		}

		Msgs_read--;

	    }

	    memcpy (Sys_msg [Msgs_read], new_msg, status);

	    le_msg = (LE_critical_message *) new_msg;

	    /* Get the RPG/RDA alarm level so we can set the appropriate type. */
	    rpg_msg = (le_msg->code & HCI_LE_RPG_STATUS_MASK);
	    rpg_alarm = (le_msg->code & HCI_LE_RPG_ALARM_MASK);
	    rda_alarm = (le_msg->code & HCI_LE_RDA_ALARM_MASK);

            /* Check if RPG alarm. */
	    if( rpg_alarm ){

                switch( rpg_alarm ){

		   case HCI_LE_RPG_ALARM_MAR:
                   default:
		        Msg_type [Msgs_read] = FILTER_RPG_MAR_ALARM_ACTIVATED_MSG;
                        break;

		   case HCI_LE_RPG_ALARM_MAM:
		        Msg_type [Msgs_read] = FILTER_RPG_MAM_ALARM_ACTIVATED_MSG;
                        break;

		   case HCI_LE_RPG_ALARM_LS:
		        Msg_type [Msgs_read] = FILTER_RPG_LS_ALARM_ACTIVATED_MSG;
                        break;

                }

	    	if( le_msg->code & HCI_LE_RPG_ALARM_CLEAR )
		    Msg_type [Msgs_read] |= FILTER_RPG_ALARM_CLEARED_MSG;

            /* Check if RDA alarm. */
	    } else if ( rda_alarm ){

		switch( rda_alarm ){

                   case HCI_LE_RDA_ALARM_NA:
		        Msg_type [Msgs_read] = FILTER_RDA_NA_ALARM_ACTIVATED_MSG;
                        break;

		   case HCI_LE_RDA_ALARM_SEC:
                   default:
		        Msg_type [Msgs_read] = FILTER_RDA_SEC_ALARM_ACTIVATED_MSG;
                        break;

		   case HCI_LE_RDA_ALARM_MAR:
		        Msg_type [Msgs_read] = FILTER_RDA_MAR_ALARM_ACTIVATED_MSG;
                        break;

		   case HCI_LE_RDA_ALARM_MAM:
		        Msg_type [Msgs_read] = FILTER_RDA_MAM_ALARM_ACTIVATED_MSG;
                        break;

		   case HCI_LE_RDA_ALARM_INOP:
		        Msg_type [Msgs_read] = FILTER_RDA_INOP_ALARM_ACTIVATED_MSG;
                        break;

                }

	    	if( le_msg->code & HCI_LE_RDA_ALARM_CLEAR )
		    Msg_type [Msgs_read] |= FILTER_RDA_ALARM_CLEARED_MSG;

            /* Check if RPG message. */
	    } else if( rpg_msg ){

		if( rpg_msg == HCI_LE_RPG_STATUS_WARN )
		   Msg_type [Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;

	        else if( rpg_msg == HCI_LE_RPG_STATUS_INFO )
		   Msg_type [Msgs_read] = FILTER_RPG_INFO_STATUS_MSG;

	        else if( rpg_msg == HCI_LE_RPG_STATUS_COMMS )
		   Msg_type [Msgs_read] = FILTER_RPG_COMMS_STATUS_MSG;

                else if( rpg_msg == HCI_LE_RPG_STATUS_GEN )
		   Msg_type [Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;

            /* All other types of messages. */
	    } else {

		if( le_msg->code & GL_ERROR_BIT )
		   Msg_type [Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;

	        else
		   Msg_type [Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;

	    }

	    Msgs_read++;

	    free (new_msg);

	}

	display_system_status_msgs (0);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a system log message from the list.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
select_log_message_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtPointer	data;

	XtVaGetValues (w,
		XmNuserData,	&data,
		NULL);

	List_index = (int) data;

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the Message Filter Display check boxes.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - filter ID					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
filter_button_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	Active_page = 0;

	XtVaSetValues (Page_left_button,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);

        /* make sure to change the state of the rda and rpg buttons */
        toggle_rda_alarms_button(w);
        toggle_rpg_alarms_button(w);
        toggle_rpg_msgs_button(w);

	if (state->set) {

	    Filter_status = Filter_status | ((int) client_data);

	} else {

	    Filter_status = Filter_status & (~((int) client_data));

	}

	display_system_status_msgs (1);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Print Log Messages" button.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
print_log_messages_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	print_form;
	Widget	print_rowcol;
	Widget	print_options_frame;
	Widget	print_options_form;
	Widget	button;
	char	buf[HCI_BUF_16];

	HCI_LE_log("Print Log Messages selected");

/*	If the Print RPG Log Messages window is defined, do nothing.	*/

	if (Print_dialog != NULL)
	{
	  HCI_Shell_popup( Print_dialog );
	  return;
	}

/*	Create the Print RPG Log Messages window.			*/

	HCI_Shell_init( &Print_dialog, "Print RPG Log Messages" );

/*	Use a form to manage the window contents.			*/

	print_form = XtVaCreateWidget ("print_form",
		xmFormWidgetClass,	Print_dialog,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	print_rowcol = XtVaCreateManagedWidget ("print_rowcol",
		xmRowColumnWidgetClass,	print_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		NULL);


	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	print_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, print_close_callback, NULL);

	button = XtVaCreateManagedWidget ("Print",
		xmPushButtonWidgetClass,	print_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, print_print_callback, NULL);

	print_options_frame = XtVaCreateManagedWidget ("print_options_frame",
		xmFrameWidgetClass,	print_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		print_rowcol,
		NULL);

	print_options_form = XtVaCreateWidget ("print_options_form",
		xmFormWidgetClass,	print_options_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Print_all = XtVaCreateManagedWidget ("Print All Messages",
		xmToggleButtonWidgetClass,	print_options_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNset,			True,
		NULL);

	if (Print_all_flag)
           XtVaSetValues(Print_all, XmNset, True, NULL);
	else
	   XtVaSetValues(Print_all, XmNset, False, NULL);

	XtAddCallback (Print_all,
		XmNvalueChangedCallback, print_all_messages_callback, NULL);

	Print_rowcol = XtVaCreateWidget ("print_select_rowcol",
		xmRowColumnWidgetClass,	print_options_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Print_all,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Print ",
		xmLabelWidgetClass,	Print_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
	
	Print_msg_num = XtVaCreateManagedWidget ("print_msg_num",
		xmTextFieldWidgetClass,	Print_rowcol,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		5,
		NULL);

	if (Print_all_flag) {

	    XtVaSetValues (Print_msg_num,
		XmNsensitive,		False,
		XmNeditable,		False,
		NULL);

	} else {

	    XtVaSetValues (Print_msg_num,
		XmNsensitive,		True,
		XmNeditable,		True,
		NULL);

	}

	sprintf (buf,"%d",Print_num_msgs);
	XmTextSetString (Print_msg_num, buf);

	XtAddCallback (Print_msg_num,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 5);
	XtAddCallback (Print_msg_num,
		XmNfocusCallback, hci_gain_focus_callback, NULL);
	XtAddCallback (Print_msg_num,
		XmNactivateCallback, print_message_number_callback, NULL);
	XtAddCallback (Print_msg_num,
		XmNlosingFocusCallback, print_message_number_callback, NULL);

	XtVaCreateManagedWidget (" messages ",
		xmLabelWidgetClass,	Print_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (Print_rowcol);
	XtManageChild (print_options_form);
	XtManageChild (print_form);
	HCI_Shell_start( Print_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is activated when the user changes	*
 *		     the value in the Print messages edit box.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
print_message_number_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*text;
	int	msgs;
	char	buf [128];
	char	old_text [8] = "-1";

	text = XmTextGetString (w);

/*	If the value hasn't changed, do nothing.			*/

	if (!strcmp (text, old_text)) {
	
	    XtFree (text);
	    return;

	}

/*	If a value is detected, validate it.				*/

	if (strlen (text)) {

	    sscanf (text,"%d",&msgs);

	    if ((msgs < 1) || (msgs > MAX_MSGS_BUFFER)) {

		sprintf (buf,"You entered an invalid number of messages\nto print (%d).  It should be in the range\n1 to %d.", msgs, MAX_MSGS_BUFFER);
                hci_warning_popup( Print_dialog, buf, NULL );

	    } else {

		Print_num_msgs = msgs;

	    }

	} else {

	    Print_num_msgs = 1;

	}

	sprintf (buf,"%5d", Print_num_msgs);
	XmTextSetString (w, buf);

	strcpy (old_text, buf);

	XtFree (text);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Print All Messages" check box.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
print_all_messages_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    XtVaSetValues (Print_msg_num,
		XmNsensitive,	False,
		XmNeditable,	False,
		NULL);
	    Print_all_flag = 1;

	} else {

	    XtVaSetValues (Print_msg_num,
		XmNsensitive,	True,
		XmNeditable,	True,
		NULL);
	    Print_all_flag = 0;

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the Print RPG Log Messages	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
print_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_Shell_popdown( Print_dialog );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Print" button in the Print RPG Log Messages	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
print_print_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	FILE	*fp;
	int	i;
	int	month, day, year;
	int	hour, minute, second;
	int	msg_cnt;
	struct tm	*gmt_time;
	time_t	seconds;
	char	*ptr, buf [128];
	LE_critical_message	*le_msg;

	if ((fp=fopen (HCI_PRINT_FILENAME,"w")) == NULL) {

	    HCI_LE_error("Unable to open scratch file %s for printing",
		HCI_PRINT_FILENAME);
	    sprintf (buf,"ERROR: Unable to print system log");
	    HCI_display_feedback( buf );
	
	} else {

	    msg_cnt = 0;

	    seconds = time (NULL);

	    gmt_time = gmtime (&seconds);

	    if (strftime (buf, 64,
		          "%A %B %d, %Y   %H:%M:%S UT \n",
		          gmt_time) == 0) {

		sprintf (buf,"   ");

	    }

	    fprintf (fp,"RPG SYSTEM LOG -------> %s\n\n",buf);

	    for (i=Msgs_read-1;i>=0;i--) {

		le_msg = (LE_critical_message *) Sys_msg [i];

		unix_time (&le_msg->time,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

		year = year%100;

                ptr = strstr (le_msg->text, ":");

                if (ptr == NULL) {

                    ptr = le_msg->text;

                } else {

                    ptr = ptr + 1;

                }

                sprintf (buf,"%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                        HCI_get_month(month), day, year, hour,
                        minute, second, ptr);

		if ((!strlen (Search_string)) ||
		    (strlen (Search_string) &&
		    (hci_string_in_string (buf,Search_string) != 0))) {

		    if ( (Msg_type [i] & Filter_status) == Msg_type [i] ) {

			fprintf (fp,"%s",buf);
	
			msg_cnt++;

			if (msg_cnt >= Print_num_msgs && !Print_all_flag) {

			    break;

			}
		    }
		}
	    }

	    fclose (fp);
	    sprintf (buf,"print_log %s &", HCI_PRINT_FILENAME);
	    system (buf);
	    sprintf (buf,"System Log file sent to printer");
	    HCI_display_feedback( buf );
	}

	HCI_Shell_popdown( Print_dialog );
}

/************************************************************************
 *	Description: This function is activated when one selects one of	*
 *		     the message list page arrow buttons.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - XmARROW_LEFT or XmARROW_RIGHT		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_page_button_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	switch ((int) client_data) {

	    case XmARROW_LEFT:

		Active_page--;
		if (Active_page < 0)
		    Active_page = 0;
		break;

	    case XmARROW_RIGHT:

		Active_page++;
		break;

	}

	display_system_status_msgs (1);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the Message Filter button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_select_msgs_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	form;
	Widget	form_rowcol;
	Widget	control_rowcol;
	Widget	rpg_msgs_rowcol;
	Widget	rpg_msgs_filter_frame;
	Widget	rda_alarm_rowcol;
	Widget	rda_alarm_filter_frame;
	Widget	rpg_alarm_rowcol;
	Widget	rpg_alarm_filter_frame;
	Widget	button;
	Widget	label;

	void	hci_close_msgs_dialog_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

/*	If the window is already defined, do nothing.		*/

	if (Filter_msgs_dialog != NULL)
	{
	  HCI_Shell_popup( Filter_msgs_dialog );
	  return;
	}

	HCI_Shell_init( &Filter_msgs_dialog, "Message Filter" );

/*	Use a form to manage the contents of the window.		*/

	form = XtVaCreateWidget ("filter_msgs_form",
		xmFormWidgetClass,	Filter_msgs_dialog,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	form_rowcol = XtVaCreateManagedWidget ("control_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	/* Control button(s). */

	control_rowcol = XtVaCreateManagedWidget ("control_rowcol",
		xmRowColumnWidgetClass,	form_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass, control_rowcol,
		XmNforeground,		hci_get_read_color (WHITE),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_close_msgs_dialog_callback, NULL);

	/* RPG Messages. */

	rpg_msgs_filter_frame = XtVaCreateManagedWidget ("rpg_msgs_frame",
		xmFrameWidgetClass,	form_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Select RPG Message",
		xmLabelWidgetClass,	rpg_msgs_filter_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rpg_msgs_rowcol = XtVaCreateManagedWidget ("rpg_msgs_rowcol",
		xmRowColumnWidgetClass,	rpg_msgs_filter_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNrightAttachment,	XmATTACH_WIDGET,
		NULL);

	Rpg_info_button = XtVaCreateManagedWidget ("Informational",
		xmToggleButtonWidgetClass,	rpg_msgs_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (GRAY),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_info_stat,
		NULL);

	XtAddCallback (Rpg_info_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_INFO_STATUS_MSG);

	Rpg_gen_status_button = XtVaCreateManagedWidget ("General Status",
		xmToggleButtonWidgetClass,	rpg_msgs_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfillOnSelect,	False,
		XmNselectColor,		hci_get_read_color (BACKGROUND_COLOR1),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_gen_stat,
		NULL);

	XtAddCallback (Rpg_gen_status_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_GENERAL_STATUS_MSG);

	Rpg_comms_status_button = XtVaCreateManagedWidget ("Narrowband Communications",
		xmToggleButtonWidgetClass,	rpg_msgs_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (SEAGREEN),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_comms_stat,
		NULL);

	XtAddCallback (Rpg_comms_status_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_COMMS_STATUS_MSG);

	Rpg_warn_status_button = XtVaCreateManagedWidget ("Warnings/Errors",
		xmToggleButtonWidgetClass,	rpg_msgs_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_warn_stat,
		NULL);

	XtAddCallback (Rpg_warn_status_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_WARNING_STATUS_MSG);

	/* Empty label for spacing. */

	XtVaCreateManagedWidget( " ",
	        xmLabelWidgetClass,	form_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	/* RPG Alarms. */

	rpg_alarm_filter_frame = XtVaCreateManagedWidget ("rpg_alarm_frame",
		xmFrameWidgetClass,	form_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rpg_msgs_filter_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Select RPG Alarm",
		xmLabelWidgetClass,	rpg_alarm_filter_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rpg_alarm_rowcol = XtVaCreateManagedWidget ("rpg_alarm_rowcol",
		xmRowColumnWidgetClass,	rpg_alarm_filter_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNrightAttachment,	XmATTACH_WIDGET,
		NULL);

	Rpg_alarm_LS_button = XtVaCreateManagedWidget ("Load Shed (LS)",
		xmToggleButtonWidgetClass,	rpg_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (CYAN),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_LS_alarm_stat,
		NULL);

	XtAddCallback (Rpg_alarm_LS_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_LS_ALARM_ACTIVATED_MSG);

	Rpg_alarm_MR_button = XtVaCreateManagedWidget ("Maintenance Required (MR)",
		xmToggleButtonWidgetClass,	rpg_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_MR_stat,
		NULL);

	XtAddCallback (Rpg_alarm_MR_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_MAR_ALARM_ACTIVATED_MSG);

	Rpg_alarm_MM_button = XtVaCreateManagedWidget ("Maintenance Mandatory (MM)",
		xmToggleButtonWidgetClass,	rpg_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (ALARM_COLOR2),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RPG_MM_stat,
		NULL);

	XtAddCallback (Rpg_alarm_MM_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RPG_MAM_ALARM_ACTIVATED_MSG);

	/* Empty label for spacing. */

	XtVaCreateManagedWidget( " ",
	        xmLabelWidgetClass,	form_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	/* RDA Alarms. */

	rda_alarm_filter_frame = XtVaCreateManagedWidget ("rda_alarm_frame",
		xmFrameWidgetClass,	form_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rpg_alarm_filter_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Select RDA Alarm",
		xmLabelWidgetClass,	rda_alarm_filter_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rda_alarm_rowcol = XtVaCreateManagedWidget ("rda_alarm_rowcol",
		xmRowColumnWidgetClass,	rda_alarm_filter_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNrightAttachment,	XmATTACH_WIDGET,
		NULL);

	Rda_alarm_SEC_button = XtVaCreateManagedWidget ("Secondary (SEC)",
		xmToggleButtonWidgetClass,	rda_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RDA_SEC_stat,
		NULL);

	XtAddCallback (Rda_alarm_SEC_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RDA_SEC_ALARM_ACTIVATED_MSG);

	Rda_alarm_MR_button = XtVaCreateManagedWidget ("Maintenance Required (MR)",
		xmToggleButtonWidgetClass,	rda_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RDA_MR_stat,
		NULL);

	XtAddCallback (Rda_alarm_MR_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RDA_MAR_ALARM_ACTIVATED_MSG);

	Rda_alarm_MM_button = XtVaCreateManagedWidget ("Maintenance Mandatory (MM)",
		xmToggleButtonWidgetClass,	rda_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (ALARM_COLOR2),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RDA_MM_stat,
		NULL);

	XtAddCallback (Rda_alarm_MM_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RDA_MAM_ALARM_ACTIVATED_MSG);

	Rda_alarm_INOP_button = XtVaCreateManagedWidget ("Inoperable (INOP)",
		xmToggleButtonWidgetClass,	rda_alarm_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (ALARM_COLOR1),
                XmNindicatorOn,         XmINDICATOR_CHECK_BOX,
		XmNfontList,		hci_get_fontlist (LIST),
                XmNset,                 RDA_INOP_stat,
		NULL);

	XtAddCallback (Rda_alarm_INOP_button,
		XmNvalueChangedCallback, filter_button_callback,
		(XtPointer) FILTER_RDA_INOP_ALARM_ACTIVATED_MSG);

	XtManageChild (form);

	HCI_Shell_start( Filter_msgs_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Close" button from the Message Filter window. *
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_close_msgs_dialog_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        HCI_Shell_popdown( Filter_msgs_dialog );
}


/************************************************************************
 *	Description: This function toggles the saved state of           *
 *	             respective rda alarm filter buttons                *
 *									*
 *	Input:  w - widget ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void toggle_rda_alarms_button(Widget w)
{
  /* Just toggle the value of the variable that is passed if it is an rda
   * Widget, otherwise do nothing
   */
  if (w == Rda_alarm_SEC_button)
    RDA_SEC_stat = !RDA_SEC_stat;
  else if (w ==  Rda_alarm_MR_button)
    RDA_MR_stat = !RDA_MR_stat;
  else if (w == Rda_alarm_MM_button)
    RDA_MM_stat = !RDA_MM_stat;
  else if (w == Rda_alarm_INOP_button)
    RDA_INOP_stat = !RDA_INOP_stat;
}

/************************************************************************
 *	Description: This function toggles the saved state of           *
 *	             respective rpg alarm filter buttons                *
 *									*
 *	Input:  w - widget ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void toggle_rpg_alarms_button(Widget w)
{
  /* Just toggle the value of the variable that is passed if it is an rda
   * Widget, otherwise do nothing
   */
  if (w ==  Rpg_alarm_MR_button)
    RPG_MR_stat = !RPG_MR_stat;
  else if (w == Rpg_alarm_MM_button)
    RPG_MM_stat = !RPG_MM_stat;
  else if (w == Rpg_alarm_LS_button)
    RPG_LS_alarm_stat = !RPG_LS_alarm_stat;
}

/************************************************************************
 *      Description: This function toggles the saved state of           *
 *                   respective rpg alarm filter buttons                *
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void toggle_rpg_msgs_button(Widget w)
{
  /* Just toggle the value of the variable that is passed if it is an rda
   * Widget, otherwise do nothing
   */
  if (w ==  Rpg_info_button)
    RPG_info_stat = !RPG_info_stat;
  else if (w == Rpg_gen_status_button)
    RPG_gen_stat = !RPG_gen_stat;
  else if (w == Rpg_warn_status_button)
    RPG_warn_stat = !RPG_warn_stat;
  else if (w == Rpg_comms_status_button)
    RPG_comms_stat = !RPG_comms_stat;
}
