/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:58:12 $
 * $Id: rdasim_gui.c,v 1.7 2010/03/10 18:58:12 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/************************************************************************
 *	Module:	 rdasim_gui.c						*
 *									*
 *	Description: This code is the front end gui to the rda_simulator*
 *		     tool. For more info, search the rda_simulator man	*
 *		     page.						*
 *                                                                      *
 ************************************************************************/

/*	Local include file definitions.		*/

#include <hci.h>
#include <rdasim_simulator.h>

/*	Local macro definitions		*/

#define	LB_PATH_LENGTH			128
#define	LB_FILENAME_LENGTH		30
#define CONFIGURATION_LEGACY		0
#define CONFIGURATION_ORDA		1
#define	RDA_CONTROL_CONTROLLING		0
#define	RDA_CONTROL_NONCONTROLLING	1
#define ANTENNAE_ROT_RATE_MIN		1.0
#define ANTENNAE_ROT_RATE_MAX		80.0
#define	RDA_CHANNEL_NUMBER_ZERO		0
#define	RDA_CHANNEL_NUMBER_ONE		1
#define	RDA_CHANNEL_NUMBER_TWO		2
#define	VERBOSITY_ZERO			0
#define	VERBOSITY_ONE			1
#define	VERBOSITY_TWO			2
#define	VERBOSITY_THREE			3
#define	VERBOSITY_FOUR			4
#define INTERVAL_SAMPLE_MIN		0.10
#define INTERVAL_SAMPLE_MAX		2.00
#define RDA_SIMULATOR_RUNNING		1
#define RDA_SIMULATOR_NOT_RUNNING	0

/*	Global widget definitions.	*/

static	Widget	Top_widget				= (Widget) NULL;
static	Widget	close_button				= (Widget) NULL;
static	Widget	configuration_legacy_button		= (Widget) NULL;
static	Widget	configuration_orda_button		= (Widget) NULL;
static	Widget	rda_control_controlling_button		= (Widget) NULL;
static	Widget	rda_control_noncontrolling_button	= (Widget) NULL;
static	Widget	antennae_rot_rate_default_button	= (Widget) NULL;
static	Widget	antennae_rot_rate_manual_button		= (Widget) NULL;
static	Widget	antennae_rot_rate_user_input		= (Widget) NULL;
static	Widget	rda_channel_num_zero_button		= (Widget) NULL;
static	Widget	rda_channel_num_one_button		= (Widget) NULL;
static	Widget	rda_channel_num_two_button		= (Widget) NULL;
static	Widget	verbosity_zero_button			= (Widget) NULL;
static	Widget	verbosity_one_button			= (Widget) NULL;
static	Widget	verbosity_two_button			= (Widget) NULL;
static	Widget	verbosity_three_button			= (Widget) NULL;
static	Widget	verbosity_four_button			= (Widget) NULL;
static	Widget	intrvl_sample_default_button		= (Widget) NULL;
static	Widget	intrvl_sample_manual_button		= (Widget) NULL;
static	Widget	intrvl_sample_user_input		= (Widget) NULL;
static	Widget	request_LB_default_button		= (Widget) NULL;
static	Widget	request_LB_manual_button		= (Widget) NULL;
static	Widget	request_LB_user_input			= (Widget) NULL;
static	Widget	response_LB_default_button		= (Widget) NULL;
static	Widget	response_LB_manual_button		= (Widget) NULL;
static	Widget	response_LB_user_input			= (Widget) NULL;
static	Widget	start_button				= (Widget) NULL;
static	Widget	stop_button				= (Widget) NULL;
static	Widget	running_label				= (Widget) NULL;
static	Widget	send_command_button			= (Widget) NULL;
static	Widget	send_exception_button			= (Widget) NULL;

/*	Function prototypes.	*/

void	gui_close (Widget w, XtPointer client_data,
			XtPointer call_data);
int	gui_destroy ();
void	configuration_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	rda_control_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	antennae_rot_rate_default_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	antennae_rot_rate_manual_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	rda_channel_num_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	verbosity_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	intrvl_sample_default_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	intrvl_sample_manual_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	request_LB_default_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	request_LB_manual_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	response_LB_default_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	response_LB_manual_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	rda_simulator_start_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	rda_simulator_stop_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	send_command_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	send_command_popup ( Widget w,
			XtPointer client_data, int mode, XtPointer call_data);
void	send_exception_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	send_exception_popup ( Widget w,
			XtPointer client_data, XtPointer call_data);
void	update_running_label();
int	is_rda_simulator_running();
int	error_check_input();
void	bad_input( char *section_name );
void	lb_create_check();
void	cleanup_LBs();
void	error_with_LB( char *lb_fx_name, int error_code );
void	rda_simulator_gui_signal_handler( int sig );

/*	Other global stuff	*/

static	char	configuration[ 3 ];
static	char	rda_control[ 3 ];
static	char	antennae_rot_rate[ 8 ];
static	char	rda_channel_num[ 5 ];
static	char	verbosity[ 5 ];
static	char	intrvl_sample[ 8 ];
static	char	request_LB[ 81 ];
static	char	response_LB[ 81 ];
static	char	lb_filename[ LB_PATH_LENGTH + LB_FILENAME_LENGTH ];
static	int	lbfd = -1;
static	void	timer_proc();
static	pid_t		rda_simulator_pid = -1;
static	int	rda_redundant_mode = 0;

/************************************************************************
 *	Description: This is the main function for the RDA Simulator	*
 *		     gui task.						*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments data		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
main (
int	argc,
char	*argv[]
)
{
	Widget		form;
	Widget		control_frame;
	Widget 		control_rowcol;
	Widget 		configuration_frame;
	Widget 		configuration_rowcol;
	Widget 		configuration_list;
	Widget 		rda_control_frame;
	Widget 		rda_control_rowcol;
	Widget 		rda_control_list;
	Widget		antennae_rot_rate_frame;
	Widget		antennae_rot_rate_rowcol;
	Widget		antennae_rot_rate_list;
	Widget 		rda_channel_num_frame;
	Widget 		rda_channel_num_rowcol;
	Widget		rda_channel_num_list;
	Widget 		verbosity_frame;
	Widget 		verbosity_rowcol;
	Widget		verbosity_list;
	Widget 		intrvl_sample_frame;
	Widget 		intrvl_sample_rowcol;
	Widget		intrvl_sample_list;
	Widget 		request_LB_frame;
	Widget 		request_LB_rowcol;
	Widget		request_LB_list;
	Widget 		response_LB_frame;
	Widget 		response_LB_rowcol;
	Widget		response_LB_list;
	Widget 		execution_frame;
	Widget 		execution_rowcol;
	Widget		label;
	Widget		space;
	int		n;
	Arg		arg [10];

/*	Initialize HCI. */

	HCI_init( argc, argv, HCI_RDASIM_GUI_TOOL );

/*	Define the widgets for the RDA simulator menu and display them.	*/

	Top_widget = HCI_get_top_widget();
	HCI_set_destroy_callback( gui_destroy );

/*	Use a form widget to organize the various menu widgets.		*/

	form   = XtVaCreateWidget ("rda_form",
		xmFormWidgetClass,	Top_widget,
		XmNautoUnmanage,	False,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Create the gui control buttons		*/

	control_frame   = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	control_rowcol = XtVaCreateWidget ("control_rowcol",
		xmRowColumnWidgetClass,	control_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	close_button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (close_button,
		XmNactivateCallback, gui_close, NULL);

	XtManageChild (control_rowcol);

/*	Create the rda configuration buttons.		*/

	configuration_frame   = XtVaCreateManagedWidget ("top_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("RDA Configuration",
		xmLabelWidgetClass,	configuration_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	configuration_rowcol = XtVaCreateWidget ("configuration_rowcol",
		xmRowColumnWidgetClass,	configuration_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;


	configuration_list = XmCreateRadioBox (configuration_rowcol,
		"configuration_list", arg, n);

	configuration_legacy_button = XtVaCreateManagedWidget ("Legacy",
		xmToggleButtonWidgetClass,	configuration_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (configuration_legacy_button,
		XmNvalueChangedCallback, configuration_callback, 
		(XtPointer) CONFIGURATION_LEGACY);

	configuration_orda_button = XtVaCreateManagedWidget ("ORDA",
		xmToggleButtonWidgetClass,	configuration_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (configuration_orda_button,
		XmNvalueChangedCallback, configuration_callback, 
		(XtPointer) CONFIGURATION_ORDA);

	/* Set default by manually calling callback */

	configuration_callback( NULL, ( XtPointer )CONFIGURATION_ORDA, NULL );

	XtManageChild (configuration_list);
	XtManageChild (configuration_rowcol);

/*	Put space between the rda configuration and rda control frames.	*/

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		configuration_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the rda control buttons.		*/

	rda_control_frame   = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("RDA Control",
		xmLabelWidgetClass,	rda_control_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rda_control_rowcol = XtVaCreateWidget ("rda_control_rowcol",
		xmRowColumnWidgetClass,	rda_control_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;


	rda_control_list = XmCreateRadioBox (rda_control_rowcol,
		"rda_control_list", arg, n);
	
	rda_control_controlling_button = XtVaCreateManagedWidget ("Controlling",
		xmToggleButtonWidgetClass,	rda_control_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,	True,
		NULL);

	XtVaSetValues (rda_control_controlling_button,
		XmNsensitive, False,
		NULL);

	XtAddCallback (rda_control_controlling_button,
		XmNvalueChangedCallback, rda_control_callback, 
		(XtPointer) RDA_CONTROL_CONTROLLING);

	/* Set default by manually calling callback */

	rda_control_callback( NULL, (XtPointer)RDA_CONTROL_CONTROLLING, NULL );

	rda_control_noncontrolling_button = XtVaCreateManagedWidget ("Non-Controlling",
		xmToggleButtonWidgetClass,	rda_control_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaSetValues (rda_control_noncontrolling_button,
		XmNsensitive, False,
		NULL);

	XtAddCallback (rda_control_noncontrolling_button,
		XmNvalueChangedCallback, rda_control_callback, 
		(XtPointer) RDA_CONTROL_NONCONTROLLING);

	XtManageChild (rda_control_list);
	XtManageChild (rda_control_rowcol);

/*	Put space between rows.		*/

	space = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		configuration_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the antennae rotation rate buttons.	*/

	antennae_rot_rate_frame   = XtVaCreateManagedWidget ("top_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Antennae Rotation Rate  (deg/sec)",
		xmLabelWidgetClass,	antennae_rot_rate_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	antennae_rot_rate_rowcol = XtVaCreateWidget ("antennae_rot_rate_rowcol",
		xmRowColumnWidgetClass,	antennae_rot_rate_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;


	antennae_rot_rate_list = XmCreateRadioBox (antennae_rot_rate_rowcol,
		"antennae_rot_rate_list", arg, n);
	
	antennae_rot_rate_default_button = XtVaCreateManagedWidget ("Default",
		xmToggleButtonWidgetClass,	antennae_rot_rate_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (antennae_rot_rate_default_button,
		XmNvalueChangedCallback, antennae_rot_rate_default_callback, 
		(XtPointer) NULL);

	antennae_rot_rate_manual_button = XtVaCreateManagedWidget ("Manual (1-80)",
		xmToggleButtonWidgetClass,	antennae_rot_rate_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (antennae_rot_rate_manual_button,
		XmNvalueChangedCallback, antennae_rot_rate_manual_callback, 
		(XtPointer) NULL);

        antennae_rot_rate_user_input = XtVaCreateManagedWidget ("search_text",
                xmTextFieldWidgetClass, antennae_rot_rate_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,		4,
                XmNmaxLength,		4,
                XmNmarginHeight,	2,
                XmNshadowThickness,	1,
		XmNsensitive,		False,
                NULL);

	/* Set default by manually calling callback */

	antennae_rot_rate_default_callback( NULL, NULL, NULL );

	XtManageChild (antennae_rot_rate_list);
	XtManageChild (antennae_rot_rate_rowcol);

/*	Put space between the antennae rotation rate and rda	*
 *	channel number frames.					*/

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		antennae_rot_rate_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the rda channel number buttons.		*/

	rda_channel_num_frame   = XtVaCreateManagedWidget ("rda_channel_num_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("RDA Channel Number",
		xmLabelWidgetClass,	rda_channel_num_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rda_channel_num_rowcol = XtVaCreateWidget ("rda_channel_num_rowcol",
		xmRowColumnWidgetClass,	rda_channel_num_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	rda_channel_num_list = XmCreateRadioBox (rda_channel_num_rowcol,
		"rda_channel_num_list", arg, n);

	rda_channel_num_zero_button = XtVaCreateManagedWidget ("0",
		xmToggleButtonWidgetClass,	rda_channel_num_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (rda_channel_num_zero_button,
		XmNvalueChangedCallback, rda_channel_num_callback, 
		(XtPointer) RDA_CHANNEL_NUMBER_ZERO);

	/* Set default by manually calling callback */

	rda_channel_num_callback( NULL, (XtPointer) RDA_CHANNEL_NUMBER_ZERO, NULL );

	rda_channel_num_one_button = XtVaCreateManagedWidget ("1",
		xmToggleButtonWidgetClass,	rda_channel_num_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (rda_channel_num_one_button,
		XmNvalueChangedCallback, rda_channel_num_callback, 
		(XtPointer) RDA_CHANNEL_NUMBER_ONE);

	rda_channel_num_two_button = XtVaCreateManagedWidget ("2",
		xmToggleButtonWidgetClass,	rda_channel_num_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (rda_channel_num_two_button,
		XmNvalueChangedCallback, rda_channel_num_callback, 
		(XtPointer) RDA_CHANNEL_NUMBER_TWO);

	XtManageChild (rda_channel_num_list);
	XtManageChild (rda_channel_num_rowcol);

/*	Put space between rows.		*/

	space = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		antennae_rot_rate_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create verbosity buttons.	*/

	verbosity_frame   = XtVaCreateManagedWidget ("verbosity_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Verbosity",
		xmLabelWidgetClass,	verbosity_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	verbosity_rowcol = XtVaCreateWidget ("verbosity_rowcol",
		xmRowColumnWidgetClass,	verbosity_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	verbosity_list = XmCreateRadioBox (verbosity_rowcol,
		"verbosity_list", arg, n);

	verbosity_zero_button = XtVaCreateManagedWidget ("0",
		xmToggleButtonWidgetClass,	verbosity_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (verbosity_zero_button,
		XmNvalueChangedCallback, verbosity_callback, 
		(XtPointer) VERBOSITY_ZERO);

	/* Set default by manually calling callback */

	verbosity_callback( NULL, (XtPointer) VERBOSITY_ZERO, NULL );

	verbosity_one_button = XtVaCreateManagedWidget ("1",
		xmToggleButtonWidgetClass,	verbosity_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (verbosity_one_button,
		XmNvalueChangedCallback, verbosity_callback, 
		(XtPointer) VERBOSITY_ONE);

	verbosity_two_button = XtVaCreateManagedWidget ("2",
		xmToggleButtonWidgetClass,	verbosity_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (verbosity_two_button,
		XmNvalueChangedCallback, verbosity_callback, 
		(XtPointer) VERBOSITY_TWO);

	verbosity_three_button = XtVaCreateManagedWidget ("3",
		xmToggleButtonWidgetClass,	verbosity_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (verbosity_three_button,
		XmNvalueChangedCallback, verbosity_callback, 
		(XtPointer) VERBOSITY_THREE);

	verbosity_four_button = XtVaCreateManagedWidget ("4",
		xmToggleButtonWidgetClass,	verbosity_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (verbosity_four_button,
		XmNvalueChangedCallback, verbosity_callback, 
		(XtPointer) VERBOSITY_FOUR);

	XtManageChild (verbosity_list);
	XtManageChild (verbosity_rowcol);

/*	Put space between the verbosity and interval sample frames.	*/

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		verbosity_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the interval sample buttons.	*/

	intrvl_sample_frame   = XtVaCreateManagedWidget ("intrvl_sample_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Sample Interval (deg)",
		xmLabelWidgetClass,	intrvl_sample_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	intrvl_sample_rowcol = XtVaCreateWidget ("intrvl_sample_rowcol",
		xmRowColumnWidgetClass,	intrvl_sample_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	intrvl_sample_list = XmCreateRadioBox (intrvl_sample_rowcol,
		"intrvl_sample_list", arg, n);

	intrvl_sample_default_button = XtVaCreateManagedWidget ("Default",
		xmToggleButtonWidgetClass,	intrvl_sample_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (intrvl_sample_default_button,
		XmNvalueChangedCallback, intrvl_sample_default_callback, 
		(XtPointer) NULL);

	intrvl_sample_manual_button = XtVaCreateManagedWidget ("Manual (0.10-2.00)",
		xmToggleButtonWidgetClass,	intrvl_sample_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (intrvl_sample_manual_button,
		XmNvalueChangedCallback, intrvl_sample_manual_callback, 
		(XtPointer) NULL);

        intrvl_sample_user_input = XtVaCreateManagedWidget ("search_text",
                xmTextFieldWidgetClass, intrvl_sample_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             4,
                XmNmaxLength,		4,
                XmNmarginHeight,        2,
                XmNshadowThickness,     1,
		XmNsensitive,		False,
                NULL);

	/* Set default by manually calling callback */

	intrvl_sample_default_callback( NULL, NULL, NULL );

	XtManageChild (intrvl_sample_list);
	XtManageChild (intrvl_sample_rowcol);

/*	Put space between rows.		*/

	space = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		verbosity_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create request LB buttons.	*/

	request_LB_frame   = XtVaCreateManagedWidget ("top_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Request LB",
		xmLabelWidgetClass,	request_LB_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	request_LB_rowcol = XtVaCreateWidget ("request_LB_rowcol",
		xmRowColumnWidgetClass,	request_LB_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;


	request_LB_list = XmCreateRadioBox (request_LB_rowcol,
		"request_LB_list", arg, n);
	
	request_LB_default_button = XtVaCreateManagedWidget ("Default",
		xmToggleButtonWidgetClass,	request_LB_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (request_LB_default_button,
		XmNvalueChangedCallback, request_LB_default_callback, 
		(XtPointer) NULL);

	request_LB_manual_button = XtVaCreateManagedWidget ("Manual",
		xmToggleButtonWidgetClass,	request_LB_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (request_LB_manual_button,
		XmNvalueChangedCallback, request_LB_manual_callback, 
		(XtPointer) NULL);

        request_LB_user_input = XtVaCreateManagedWidget ("search_text",
                xmTextFieldWidgetClass, request_LB_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             40,
		XmNmaxLength,		80,
                XmNmarginHeight,        2,
                XmNshadowThickness,     1,
		XmNsensitive,		False,
                NULL);

	/* Set default by manually calling callback */

	request_LB_default_callback( NULL, NULL, NULL );

	XtManageChild (request_LB_list);
	XtManageChild (request_LB_rowcol);

/*	Put space between rows.		*/

	space = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		request_LB_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create response LB buttons.		*/

	response_LB_frame   = XtVaCreateManagedWidget ("top_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Response LB",
		xmLabelWidgetClass,	response_LB_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	response_LB_rowcol = XtVaCreateWidget ("response_LB_rowcol",
		xmRowColumnWidgetClass,	response_LB_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	1);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;


	response_LB_list = XmCreateRadioBox (response_LB_rowcol,
		"response_LB_list", arg, n);
	
	response_LB_default_button = XtVaCreateManagedWidget ("Default",
		xmToggleButtonWidgetClass,	response_LB_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			True,
		NULL);

	XtAddCallback (response_LB_default_button,
		XmNvalueChangedCallback, response_LB_default_callback, 
		(XtPointer) NULL);

	response_LB_manual_button = XtVaCreateManagedWidget ("Manual",
		xmToggleButtonWidgetClass,	response_LB_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (response_LB_manual_button,
		XmNvalueChangedCallback, response_LB_manual_callback, 
		(XtPointer) NULL);

        response_LB_user_input = XtVaCreateManagedWidget ("search_text",
                xmTextFieldWidgetClass, response_LB_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             40,
		XmNmaxLength,		80,
                XmNmarginHeight,        2,
                XmNshadowThickness,     1,
		XmNsensitive,		False,
                NULL);

	/* Set default by manually calling callback */

	response_LB_default_callback( NULL, NULL, NULL );

	XtManageChild (response_LB_list);
	XtManageChild (response_LB_rowcol);

/*	Put space between rows.		*/

	space = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		response_LB_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the execution buttons. 	*/

	execution_frame   = XtVaCreateManagedWidget ("execution_frame",
		xmFrameWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	execution_rowcol = XtVaCreateWidget ("execution_rowcol",
		xmRowColumnWidgetClass,	execution_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	start_button = XtVaCreateManagedWidget ("Start",
		xmPushButtonWidgetClass,	execution_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (start_button,
		XmNactivateCallback, rda_simulator_start_callback, NULL);

	stop_button = XtVaCreateManagedWidget ("Stop",
		xmPushButtonWidgetClass,	execution_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (stop_button,
		XmNactivateCallback, rda_simulator_stop_callback, NULL);

	running_label = XtVaCreateManagedWidget ("NOT RUNNING",
		xmLabelWidgetClass,	execution_rowcol,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		request_LB_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		space,
		XmNforeground,		hci_get_read_color (WHITE),
		XmNbackground,		hci_get_read_color (RED),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (execution_rowcol);

	send_command_button = XtVaCreateManagedWidget ("Send Command",
		xmPushButtonWidgetClass,	execution_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (send_command_button,
		XmNactivateCallback, send_command_callback, NULL );

	send_exception_button = XtVaCreateManagedWidget ("Send Exception",
		xmPushButtonWidgetClass,	execution_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (send_exception_button,
		XmNactivateCallback, send_exception_callback, NULL );

	XtManageChild (execution_rowcol);

/*	Manage top-level form and bring up window.	*/

	XtManageChild (form);
	XtRealizeWidget (Top_widget);

/*	If user sends Control-c signal, handle it. */

	sigset( SIGINT, rda_simulator_gui_signal_handler );

/*	Create LB to communicate with rda simulator.	*/

	lb_create_check();

/*	Start HCI event loop. */

	HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

/*	Clean up LBs that were created		*/

	cleanup_LBs();

	return 0;
}

/************************************************************************
 *	Description: This function is the timer procedure. 		*
 *									*
 *	Input:  w - timer parent widget ID				*
 *		id - timer interval ID					*
 *	Output: NONE							*
 *	Return: 0 (unused)						*
 ************************************************************************/

void
timer_proc ()
{
  update_running_label();
}

/************************************************************************
 *	Description: This function updates the running label.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
update_running_label ()
{
  int running_code = is_rda_simulator_running();
  XmString label_string;

  if( running_code == RDA_SIMULATOR_RUNNING )
  {
    label_string = XmStringCreateLocalized( "RUNNING" );
    XtVaSetValues( running_label,
		XmNlabelString,	label_string,
		XmNforeground,	hci_get_read_color (BLACK),
		XmNbackground,	hci_get_read_color (GREEN),
		NULL );
  }
  else
  {
    label_string = XmStringCreateLocalized( "NOT RUNNING" );
    XtVaSetValues( running_label,
		XmNlabelString,	label_string,
		XmNforeground,	hci_get_read_color (WHITE),
		XmNbackground,	hci_get_read_color (RED),
		NULL );
  }

  XmStringFree( label_string );
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
gui_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtDestroyWidget( Top_widget );
}

/************************************************************************
 *	Description: This function is activated when the RDA Simulator	*
 *		     gui window is destroyed.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
gui_destroy ()
{
  rda_simulator_stop_callback ( NULL, NULL, NULL );
  if( lbfd > 0 ){ LB_remove( lb_filename ); }
  return HCI_OK_TO_EXIT;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a button in the configuration frame.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - configuration macro			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
configuration_callback(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  if( ( int ) client_data == CONFIGURATION_LEGACY )
  {
    sprintf( configuration, "-L" );
  }
  else
  {
    sprintf( configuration, "-O" );
  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a button in the rda control frame.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - rda control macro				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rda_control_callback(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  if( ( int ) client_data == RDA_CONTROL_CONTROLLING )
  {
    sprintf( rda_control, " " );
  }
  else
  {
    sprintf( rda_control, "-n" );
  }
}
/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the default button in the antennae rotation rate	*
 *		     frame.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused          				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
antennae_rot_rate_default_callback(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( antennae_rot_rate_user_input,
		 XmNvalue, "",
		 XmNsensitive,	False,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the manual button in the antennae rotation	rate	*
 *		     frame.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused          				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
antennae_rot_rate_manual_callback(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( antennae_rot_rate_user_input,
		 XmNsensitive,	True,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a button in the rda channel number frame.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - rda channel number macro			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rda_channel_num_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  if( ( int ) client_data == RDA_CHANNEL_NUMBER_ONE )
  {
    rda_redundant_mode = 1;
    sprintf( rda_channel_num, "-c 1" );
  }
  else if( ( int ) client_data == RDA_CHANNEL_NUMBER_TWO )
  {
    rda_redundant_mode = 1;
    sprintf( rda_channel_num, "-c 2" );
  }
  else
  {
    rda_redundant_mode = 0;
    sprintf( rda_channel_num, " " );
  }

  if( rda_redundant_mode == 0 )
  {
    XtVaSetValues (rda_control_controlling_button,
	XmNsensitive, False,
	NULL);
    XtVaSetValues (rda_control_noncontrolling_button,
	XmNsensitive, False,
	NULL);
  }
  else
  {
    XtVaSetValues (rda_control_controlling_button,
	XmNsensitive, True,
	NULL);
    XtVaSetValues (rda_control_noncontrolling_button,
	XmNsensitive, True,
	NULL);
  }

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a button in the verbosity frame.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - verbosity macro				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
verbosity_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  if( ( int ) client_data == VERBOSITY_ONE )
  {
    sprintf( verbosity, "-v 1" );
  }
  else if( ( int ) client_data == VERBOSITY_TWO )
  {
    sprintf( verbosity, "-v 2" );
  }
  else if( ( int ) client_data == VERBOSITY_THREE )
  {
    sprintf( verbosity, "-v 3" );
  }
  else if( ( int ) client_data == VERBOSITY_FOUR )
  {
    sprintf( verbosity, "-v 4" );
  }
  else
  {
    sprintf( verbosity, " " );
  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the default button in the interval sample frame.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
intrvl_sample_default_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( intrvl_sample_user_input,
		 XmNvalue, "",
		 XmNsensitive,	False,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the manual button in the interval sample frame.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
intrvl_sample_manual_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( intrvl_sample_user_input,
		 XmNsensitive,	True,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the default button in the request LB frame.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
request_LB_default_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( request_LB_user_input,
		 XmNvalue, "",
		 XmNsensitive,	False,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the manual button in the request LB frame.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
request_LB_manual_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( request_LB_user_input,
		 XmNsensitive,	True,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the default button in the response LB frame.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
response_LB_default_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( response_LB_user_input,
		 XmNvalue, "",
		 XmNsensitive,	False,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the manual button in the response LB frame.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
response_LB_manual_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtVaSetValues( response_LB_user_input,
		 XmNsensitive,	True,
		 NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the start button in the execution frame.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rda_simulator_start_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char cmd[ 200 ];

  if( !error_check_input() )
  {
    sprintf( cmd, "rda_simulator %s %s %s %s %s %s %s %s -g &",
             configuration,
             rda_control,
             antennae_rot_rate,
             rda_channel_num,
             verbosity,
             intrvl_sample,
             request_LB,
             response_LB );
    printf("\n\n\nRDA SIMULATOR COMMAND:\t%s\n",cmd);
    rda_simulator_pid = MISC_system_to_buffer( cmd, NULL, 0, NULL );
    printf("RDA SIMULATOR PID:\t%d\n\n\n", ( int )rda_simulator_pid);
    XtVaSetValues( send_command_button,
		   XmNsensitive,	True,
		   NULL);
    XtVaSetValues( send_exception_button,
		   XmNsensitive,	True,
		   NULL);
  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the stop button in the execution frame.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rda_simulator_stop_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int kill_return_code = -1;
  char msg_buf[ 256 ];
  char *msg_a, *msg_b, *msg_c;

  if( rda_simulator_pid > 0 )
  {
    kill_return_code = kill( rda_simulator_pid, SIGTERM );
    if( kill_return_code < 0 )
    {
      msg_a = "Problems killing rda_simulator process";
      msg_b = "Return code from 'kill' command is:";
      msg_c = "User should kill process manually from the command line.";
      sprintf( msg_buf, "%s (PID: %d).\n%s %d.\n%s",
               msg_a, ( int )rda_simulator_pid,msg_b, kill_return_code, msg_c );
      hci_warning_popup( Top_widget, msg_buf, NULL );
    }
    XtVaSetValues( send_command_button,
		   XmNsensitive,	False,
		   NULL);
    XtVaSetValues( send_exception_button,
		   XmNsensitive,	False,
		   NULL);
  }
}

/************************************************************************
 *	Description: This function checks the rda_simulator pid to see	*
 *		     if the process is running.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: macros RDA_SIMULATOR_RUNNING, RDA_SIMULATOR_NOT RUNNING	*
 ************************************************************************/

int
is_rda_simulator_running ()
{
  int kill_return_code = -1;
  
  if( rda_simulator_pid > 0 )
  {
    kill_return_code = kill( rda_simulator_pid, 0 );
    if( kill_return_code == 0 ){ return RDA_SIMULATOR_RUNNING; }
  }

  return RDA_SIMULATOR_NOT_RUNNING;
}

/************************************************************************
 *	Description: This function checks the unser input to make sure	*
 *		     the values are what is expected.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: int ( 0 - no error, -1 - error )			*
 ************************************************************************/

int
error_check_input ()
{
  char *text_input;
  int  len;
  float tmp_float;
  FILE *tmp_file;

  /* Antennae rotation rate */

  text_input = XmTextGetString( antennae_rot_rate_user_input );
  len = strlen( text_input );
  if( len < 1 ){ sprintf( antennae_rot_rate, " " ); }
  else
  {
    tmp_float = atof( text_input );
    if( tmp_float < ANTENNAE_ROT_RATE_MIN || tmp_float > ANTENNAE_ROT_RATE_MAX )
    {
      bad_input( "Antennae Rotation Rate" );
      return -1;
    }
    else{ sprintf( antennae_rot_rate, "-a %4.1f", tmp_float ); }
  }
  XtFree( text_input );

  /* Interval sample */

  text_input = XmTextGetString( intrvl_sample_user_input );
  len = strlen( text_input );
  if( len < 1 ){ sprintf( intrvl_sample, " " ); }
  else
  {
    tmp_float = atof( text_input );
    if( tmp_float < INTERVAL_SAMPLE_MIN || tmp_float > INTERVAL_SAMPLE_MAX )
    {
      bad_input( "Interval Sample" );
      return -1;
    }
    else{ sprintf( intrvl_sample, "-s %4.2f", tmp_float ); }
  }
  XtFree( text_input );

  /* Request LB */

  text_input = XmTextGetString( request_LB_user_input );
  len = strlen( text_input );
  if( len < 1 ){ sprintf( request_LB, " " ); }
  else
  {
    tmp_file = fopen( text_input, "r+" );
    if( tmp_file == NULL )
    {
      bad_input( "Request LB" );
      return -1;
    }
    else
    {
      fclose( tmp_file );
      sprintf( request_LB, "-i %s", text_input );
    }
  }
  XtFree( text_input );

  /* Response LB */

  text_input = XmTextGetString( response_LB_user_input );
  len = strlen( text_input );
  if( len < 1 ){ sprintf( response_LB, " " ); }
  else
  {
    tmp_file = fopen( text_input, "r+" );
    if( tmp_file == NULL )
    {
      bad_input( "Response LB" );
      return -1;
    }
    else
    {
      fclose( tmp_file );
      sprintf( request_LB, "-i %s", text_input );
    }
  }
  XtFree( text_input );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the		*
 *		     error_check_input() function detects an error.	*
 *									*
 *	Input:  name of section that contains an error			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
bad_input ( char *section_name )
{
  char msg_buf[ HCI_BUF_256 ];
  char *msg_pre = "Error in section: ";
  char *msg_post = "\n\nMake sure all input is within the valid ranges and files have \nbeen previously created and are read/write permissable.\n";
  sprintf( msg_buf, "%s %s %s", msg_pre, section_name, msg_post );
  hci_warning_popup( Top_widget, msg_buf, NULL );
}

/************************************************************************
 *	Description: This function creates the LB used to communicate	*
 *		     with the rda simulator.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
lb_create_check ()
{
  LB_attr attr;
  char path[ LB_PATH_LENGTH ];

  MISC_get_work_dir( path, LB_PATH_LENGTH );
  sprintf( lb_filename, "%s/%s", path, RDA_SIMULATOR_LB );

 /* See if LB will open. */

  lbfd = LB_open( lb_filename, LB_WRITE, &attr );

  if (lbfd < 0)
  {
    /* LB wouldn't open, see if it needs to be created. */

    attr.maxn_msgs = 1;
    attr.msg_size = sizeof( Rdasim_gui_t );
    attr.types = LB_DB;
    attr.remark[0] = '\0';
    attr.mode = 0664;
    attr.tag_size = 5 << NRA_SIZE_SHIFT;  /* set nra (ie. # registrations allowed) to 5 */

    lbfd = LB_open( lb_filename, LB_CREATE, &attr );

    if( lbfd < 0 )
    {
      /* Still haveing LB errors, so popup error dialog. */

      error_with_LB( "LB_open", lbfd );
    }
  }
}

/************************************************************************
 *	Description: This function is called before exit to clean up 	*
 *		     any created LBs.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cleanup_LBs ()
{
  if( lbfd > 0 ){ LB_remove( lb_filename ); }
}

/************************************************************************
 *      Description: This function is activated when an error is	*
 *                   returned from an LB function.			*
 *                                                                      *
 *      Input:  name of section that contains an error                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
error_with_LB ( char *lb_fx_name, int error_code )
{
  char msg_buf[ HCI_BUF_256 ];
  char *msg_a = "Error with LB command:";
  char *msg_b = "Return code is:";

  sprintf( msg_buf, "%s %s.\n%s %d\n.", msg_a, lb_fx_name, msg_b, error_code );
  hci_warning_popup( Top_widget, msg_buf, NULL );
}

/************************************************************************
 *	Description: This function is activated when the send command	*
 *		     button is pushed.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_command_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  /* Call send command popup and pass LB file descriptor. */

  send_command_popup( w, ( XtPointer ) lbfd, rda_redundant_mode, call_data );
}

/************************************************************************
 *	Description: This function is activated when the send exception	*
 *		     button is pushed.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_exception_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  /* Call send exception popup and pass LB file descriptor. */

  send_exception_popup( w, ( XtPointer ) lbfd, call_data );
}

/************************************************************************
 *      Description: This function is activated when the user sends a	*
 *		     signal from the terminal.				*
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
rda_simulator_gui_signal_handler ( int sig )
{
  if( sig == SIGINT )
  {
    XtDestroyWidget( Top_widget );
  }
}

