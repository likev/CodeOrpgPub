/************************************************************************
 *	Module:	 hci_RDA_control.c					*
 *									*
 *	Description: This module is used by the ORPG HCI to define	*
 *		     and display the RDA functions menu.		*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:44:51 $
 * $Id: hci_RDA_control_legacy.c,v 1.12 2012/09/10 20:44:51 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <rms_util.h>

/*	Local macro definitions		*/

#define	SPOT_BLANKING_NOT_INSTALLED	0

/*	Wideband control macros	*/

enum {WIDEBAND_CONNECT, WIDEBAND_DISCONNECT};

/*	ID of top level dialog shell widget.	*/

static	Widget	Top_widget           = (Widget) NULL;

/*	ID's of RDA State radio button widgets	*/

static	Widget	Rda_operate_button           = (Widget) NULL;
static	Widget	Rda_offline_operate_button   = (Widget) NULL;
static	Widget	Rda_restart_button           = (Widget) NULL;
static	Widget	Rda_standby_button           = (Widget) NULL;

/*	ID's of RDA Control radio button widgets	*/

static	Widget	Control_rda_button           = (Widget) NULL;
static	Widget	Control_rpg_button           = (Widget) NULL;

/*	ID's of FAA Redundant Channel Control radio button widgets */

static	Widget	Red_control_button           = (Widget) NULL;
static	Widget	Red_non_control_button       = (Widget) NULL;

/*	ID's of RDA Power Source radio button widgets	*/

static	Widget	Power_utility_button         = (Widget) NULL;
static	Widget	Power_auxiliary_button       = (Widget) NULL;

/*	ID's of Interference Suppression radio button widgets	*/

static	Widget	Interference_enable_button   = (Widget) NULL;
static	Widget	Interference_disable_button  = (Widget) NULL;

/*	ID's of Spot Blanking radio button widgets	*/

static	Widget	Spot_blanking_frame	     = (Widget) NULL;
static	Widget	Spot_blanking_enable_button  = (Widget) NULL;
static	Widget	Spot_blanking_disable_button = (Widget) NULL;

/*	ID's of various status label widgets		*/

static	Widget	Operational_mode_label       = (Widget) NULL;
static	Widget	Control_authorization_label  = (Widget) NULL;
static	Widget	Ave_transmitter_power_label  = (Widget) NULL;
static	Widget	Calibration_correction_label = (Widget) NULL;
static	Widget	Interference_rate_label      = (Widget) NULL;
static	Widget	Moments_enabled_label        = (Widget) NULL;
static	Widget	Current_state_label          = (Widget) NULL;
static	Widget	Current_control_label        = (Widget) NULL;
static	Widget	Current_red_local_status     = (Widget) NULL;
static	Widget	Current_red_red_status       = (Widget) NULL;
static	Widget	Current_red_local_time       = (Widget) NULL;
static	Widget	Current_red_red_time         = (Widget) NULL;
static	Widget	Current_power_label          = (Widget) NULL;
static	Widget	Current_interference_label   = (Widget) NULL;
static	Widget	Current_spot_blanking_label  = (Widget) NULL;

/*	ID of pushbutton widget to send command to request RDA status. */

static	Widget	Status_button                = (Widget) NULL;

/*	ID of checkbox widget to lock/unlock RDA control from RMS	*/

static	Widget	Lock_rms                     = (Widget) NULL;

/*	ID of lock button widget for spot blanking and channel control. */

static	Widget	Lock_button                  = (Widget) NULL;

static	int	Command_lock = 0; /* Lock to prevent RMS from control */
static	int	Unlocked_loca = HCI_NO_FLAG; /* ROC/URC LOCA flag */
static  int	change_rms_lock = 0; /* change rms lock flag */
static  int	config_change_popup = 0; /* RDA config change popup flag. */
static  int	User_selected_RDA_state_flag = -1;
static  int	User_selected_ISU_flag = -1;
static  int	User_selected_RDA_red_control_flag = -1;
static  int	User_selected_spot_blank_flag = -1;
static  int	Redundant_status_change_flag = HCI_NO_FLAG;

char	Cmd [128]; /* common buffer for string operations */

int     hci_activate_child (Display *d, Window w, char *cmd,
                            char *proc, char *win, int object_index);
int	hci_rda_control_security ();
void 	set_rda_button();
void	rda_state_callback (Widget w, XtPointer client_data,
			XtPointer call_data );
void	accept_red_control_callback (Widget w, XtPointer client_data,
			XtPointer call_data );
void	cancel_red_control_callback (Widget w, XtPointer client_data,
			XtPointer call_data );
void	spot_blanking_callback (Widget w, XtPointer client_data,
			XtPointer call_data );
void	issue_inter_suppr_command_callback (Widget w, XtPointer client_data,
			XtPointer call_data );
void	cancel_inter_suppr_command_callback (Widget w, XtPointer client_data,
			XtPointer call_data );
static	void	Redundant_status_change_cb (int, LB_id_t, int, void *);
static	void	timer_proc ();
static	void	update_rda_control_menu_properties ();
static	void	rda_config_change();

/************************************************************************
 *	Description: This is the main function for the RDA Control	*
 *		     task.						*
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
	Widget		state_form;
	Widget		status_form;
	Widget		state_frame;
	Widget		status_frame;
	Widget		control_frame;
	Widget		button;
	Widget		label;
	Widget		space;
	Widget 		rda_control_form;
	Widget 		red_control_list;
	Widget	 	red_control_frame;
	Widget 		red_control_form;
	Widget 		red_control_frame_l;
	Widget 		red_control_form_l;
	Widget 		red_control_frame_r;
	Widget 		red_control_form_r;
	Widget 		power_form;
	Widget 		interference_form;
	Widget 		spot_blanking_form;
	Widget 		rda_control_frame;
	Widget 		power_frame;
	Widget 		interference_frame;
	Widget 		status1_rowcol;
	Widget 		status2_rowcol;
	Widget 		status3_rowcol;
	Widget 		status4_rowcol;
	Widget 		control_rowcol;
	Widget		state_list;
	Widget		control_list;
	Widget		power_list;
	Widget		interference_list;
	Widget		spot_blanking_list;
	Widget		lock_form;

	int		n;
	int		status;
	Arg		arg [10];

	void	RDA_control_close (Widget w, XtPointer client_data,
			XtPointer call_data);
	void	toggle_spot_blanking_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	toggle_interference_suppression_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	switch_power_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	toggle_RDA_control_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	toggle_red_control_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	change_rda_state_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	request_RDA_status_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_select_rda_alarms_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_select_vcp_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_lock_button_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_lock_button_verify_callback ( Widget w,
			XtPointer client_data, XtPointer call_data);
			

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_RDC_TASK );

/*	Define the widgets for the RDA menu and display them.		*/

	Top_widget = HCI_get_top_widget();

/*	Use a form widget to organize the various menu widgets.		*/

	form   = XtVaCreateWidget ("RDA_form",
		xmFormWidgetClass,	Top_widget,
		XmNautoUnmanage,	False,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Create the buttons for requesting RDA status and performance 	*
 *	data.								*/

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
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNisAligned,		False,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, RDA_control_close, NULL);

	Status_button = XtVaCreateManagedWidget ("Get Status",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Status_button,
		XmNactivateCallback, request_RDA_status_callback, NULL);

	button = XtVaCreateManagedWidget ("RDA Alarms",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_select_rda_alarms_callback, NULL);

	button = XtVaCreateManagedWidget ("VCP and Mode Control",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_select_vcp_callback, NULL);

/*	If this is an RMS site, then we need to add a check box so	*
 *	we can prevent the RMS from sending RDA control commands while	*
 *	the HCI is also sending them.					*/

	if (HCI_has_rms()) {

	    Lock_rms = XtVaCreateManagedWidget ("Lock RMS",
		xmToggleButtonWidgetClass,	control_rowcol,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
		
	XtAddCallback (Lock_rms,
		XmNvalueChangedCallback, hci_lock_button_verify_callback, NULL);
		
	set_rda_button();
	}

	XtManageChild (control_rowcol);

	lock_form = XtVaCreateWidget ("lock_form",
		xmFormWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	We need to define a lock button since the spot blanking and	*
 *	FAA redundant channel control require passwords.		*/

	Lock_button = hci_lock_widget (lock_form, hci_rda_control_security, HCI_LOCA_URC | HCI_LOCA_ROC | HCI_LOCA_FAA_OVERRIDE );

	XtManageChild (lock_form);

/*	Define a set of labels and buttons to display the RDA state	*
 *	and change it.							*/

	state_frame   = XtVaCreateManagedWidget ("top_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("RDA State",
		xmLabelWidgetClass,	state_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	state_form = XtVaCreateWidget ("control_form",
		xmFormWidgetClass,	state_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("State: ",
		xmLabelWidgetClass,	state_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	Current_state_label = XtVaCreateManagedWidget ("    UNKNOWN    ",
		xmLabelWidgetClass,	state_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);
		
	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmVERTICAL);       n++;
	XtSetArg (arg [n], XmNspacing,		0);                  n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNnumColumns,	2);                  n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;


	state_list = XmCreateRadioBox (state_form,
		"state_list", arg, n);
	
	Rda_standby_button = XtVaCreateManagedWidget ("Standby",
		xmToggleButtonWidgetClass,	state_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Rda_standby_button,
		XmNvalueChangedCallback, change_rda_state_callback, 
		(XtPointer) RS_STANDBY);

	Rda_operate_button = XtVaCreateManagedWidget ("Operate",
		xmToggleButtonWidgetClass,	state_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Rda_operate_button,
		XmNvalueChangedCallback, change_rda_state_callback, 
		(XtPointer) RS_OPERATE);

	Rda_restart_button = XtVaCreateManagedWidget ("Restart",
		xmToggleButtonWidgetClass,	state_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Rda_restart_button,
		XmNvalueChangedCallback, change_rda_state_callback, 
		(XtPointer) RS_RESTART);

	Rda_offline_operate_button = XtVaCreateManagedWidget ("Offline Operate",
		xmToggleButtonWidgetClass,	state_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Rda_offline_operate_button,
		XmNvalueChangedCallback, change_rda_state_callback, 
		(XtPointer) RS_OFFOPER);

	XtManageChild (state_list);
	XtManageChild (state_form);

/*	Put a space between the state and RDA Control frames.		*/

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		state_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the widgets defining the buttons to change the RDA	*
 *	control source.  Highlight the currently active source.		*/

	rda_control_frame   = XtVaCreateManagedWidget ("rda_control_frame",
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

	rda_control_form = XtVaCreateWidget ("rda_control_form",
		xmFormWidgetClass,	rda_control_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Control: ",
		xmLabelWidgetClass,	rda_control_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	Current_control_label = XtVaCreateManagedWidget ("  UNKNOWN   ",
		xmLabelWidgetClass,	rda_control_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNorientation,	XmVERTICAL);         n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	control_list = XmCreateRadioBox (rda_control_form,
		"control_list", arg, n);

	Control_rda_button = XtVaCreateManagedWidget ("Enable Local (RDA)",
		xmToggleButtonWidgetClass,	control_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Control_rda_button,
		XmNvalueChangedCallback, toggle_RDA_control_callback, 
		(XtPointer) CS_LOCAL_ONLY);

	Control_rpg_button = XtVaCreateManagedWidget ("Select Remote (RPG)",
		xmToggleButtonWidgetClass,	control_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Control_rpg_button,
		XmNvalueChangedCallback, toggle_RDA_control_callback,
		(XtPointer) CS_RPG_REMOTE);

	XtManageChild (control_list);
	XtManageChild (rda_control_form);

/*	Create the widgets defining the buttons to change the RDA	*
 *	redundant channel state.  Highlight the currently active source.*/

	if (HCI_get_system() == HCI_FAA_SYSTEM) {

	    status = ORPGDA_UN_register( ORPGDAT_REDMGR_CHAN_MSGS,
				ORPGRED_CHANNEL_STATUS_MSG,
				Redundant_status_change_cb );

	    if( status != LB_SUCCESS )
	    {
	      HCI_LE_error( "Unable to register for local ch updates (%d)", status);
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }

	    status = ORPGDA_UN_register( ORPGDAT_REDMGR_CHAN_MSGS,
				ORPGRED_REDUN_CHANL_STATUS_MSG,
				Redundant_status_change_cb );

	    if( status != LB_SUCCESS )
	    {
	      HCI_LE_error( "Unable to register for redundant ch updates (%d)", status);
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }

/*	    Put a space between the state and RDA Control frames.	*/

	    label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		rda_control_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    red_control_frame   = XtVaCreateManagedWidget ("red_control_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    label = XtVaCreateManagedWidget ("Redundant Control",
		xmLabelWidgetClass,	red_control_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    red_control_form = XtVaCreateWidget ("red_control_form",
		xmFormWidgetClass,	red_control_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    red_control_frame_l = XtVaCreateManagedWidget ("red_control_frame_l",
		xmFrameWidgetClass,	red_control_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    label = XtVaCreateManagedWidget ("Local Channel",
		xmLabelWidgetClass,	red_control_frame_l,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    red_control_frame_r = XtVaCreateManagedWidget ("red_control_frame_r",
		xmFrameWidgetClass,	red_control_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		red_control_frame_l,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    label = XtVaCreateManagedWidget ("Redundant Channel",
		xmLabelWidgetClass,	red_control_frame_r,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    red_control_form_l = XtVaCreateWidget ("red_control_form_l",
		xmFormWidgetClass,	red_control_frame_l,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    label = XtVaCreateManagedWidget ("Status: ",
		xmLabelWidgetClass,	red_control_form_l,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	    Current_red_local_status = XtVaCreateManagedWidget ("      UNKNOWN       ",
		xmLabelWidgetClass,	red_control_form_l,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	    label = XtVaCreateManagedWidget ("Adapt:  ",
		xmLabelWidgetClass,	red_control_form_l,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	    Current_red_local_time = XtVaCreateManagedWidget ("00/00/00 00:00:00 UT",
		xmLabelWidgetClass,	red_control_form_l,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Current_red_local_status,
		NULL);

	    n = 0;

	    XtSetArg (arg [n], XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND));    n++;
	    XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	    XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	    XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	    XtSetArg (arg [n], XmNorientation,	XmVERTICAL);         n++;
	    XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	    XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	    XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	    red_control_list = XmCreateRadioBox (red_control_form_l,
		"red_control_list", arg, n);

	    Red_control_button = XtVaCreateManagedWidget ("Controlling",
		xmToggleButtonWidgetClass,	red_control_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    XtAddCallback (Red_control_button,
		XmNvalueChangedCallback, toggle_red_control_callback, 
		(XtPointer) RDA_IS_CONTROLLING);

	    if (ORPGRED_channel_num (ORPGRED_MY_CHANNEL) != 1) {

		Red_non_control_button = XtVaCreateManagedWidget ("Non-controlling",
			xmToggleButtonWidgetClass,	red_control_list,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNselectColor,		hci_get_read_color (WHITE),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		XtAddCallback (Red_non_control_button,
			XmNvalueChangedCallback, toggle_red_control_callback,
			(XtPointer) RDA_IS_NON_CONTROLLING);

	    }

	    XtManageChild (red_control_list);
	    XtManageChild (red_control_form_l);

	    red_control_form_r = XtVaCreateWidget ("red_control_form_r",
		xmFormWidgetClass,	red_control_frame_r,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	    label = XtVaCreateManagedWidget ("Status: ",
		xmLabelWidgetClass,	red_control_form_r,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	    Current_red_red_status = XtVaCreateManagedWidget ("      UNKNOWN       ",
		xmLabelWidgetClass,	red_control_form_r,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	    label = XtVaCreateManagedWidget ("Adapt:  ",
		xmLabelWidgetClass,	red_control_form_r,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	    Current_red_red_time = XtVaCreateManagedWidget ("00/00/00 00:00:00 UT",
		xmLabelWidgetClass,	red_control_form_r,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Current_red_red_status,
		NULL);

	    XtManageChild (red_control_form_r);
	    XtManageChild (red_control_form);

	}

/*	Put a space between the state and RDA state and power frames.	*/

	space = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		state_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the widgets defining the buttons to change the RDA	*
 *	power source.  Highlight the currently active source.		*/

	power_frame   = XtVaCreateManagedWidget ("power_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("RDA Power Source",
		xmLabelWidgetClass,	power_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	power_form = XtVaCreateWidget ("power_form",
		xmFormWidgetClass,	power_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Source: ",
		xmLabelWidgetClass,	power_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	Current_power_label = XtVaCreateManagedWidget (" UNKNOWN ",
		xmLabelWidgetClass,	power_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNorientation,	XmVERTICAL);         n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	power_list = XmCreateRadioBox (power_form,
		"power_list", arg, n);

	Power_utility_button = XtVaCreateManagedWidget ("Utility",
		xmToggleButtonWidgetClass,	power_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Power_utility_button,
		XmNvalueChangedCallback, switch_power_callback,
		(XtPointer) UTILITY_POWER);

	Power_auxiliary_button = XtVaCreateManagedWidget ("Auxiliary",
		xmToggleButtonWidgetClass,	power_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Power_auxiliary_button,
		XmNvalueChangedCallback, switch_power_callback,
		(XtPointer) AUXILLIARY_POWER);

	XtManageChild (power_list);
	XtManageChild (power_form);


/*	Put a space between the power and interference frames.		*/

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		power_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the buttons for the Interference Suppression Enable/	*
 *	Disable selections.  YES means that it is enabled and NO	*
 *	means it is disabled.						*/

	  interference_frame   = XtVaCreateManagedWidget ("interference_frame",
	  	  xmFrameWidgetClass,	form,
	  	  XmNtopAttachment,	XmATTACH_WIDGET,
	  	  XmNtopWidget,		space,
	  	  XmNleftAttachment,	XmATTACH_WIDGET,
	  	  XmNleftWidget,		label,
	  	  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
	  	  NULL);

	  label = XtVaCreateManagedWidget ("Interference Suppr.",
		  xmLabelWidgetClass,	interference_frame,
		  XmNchildType,		XmFRAME_TITLE_CHILD,
		  XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		  XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNfontList,		hci_get_fontlist (LIST),
		  NULL);

	  interference_form = XtVaCreateWidget ("interference_form",
		  xmFormWidgetClass,	interference_frame,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  NULL);
  
	  label = XtVaCreateManagedWidget ("Status: ",
		  xmLabelWidgetClass,	interference_form,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNfontList,		hci_get_fontlist (LIST),
		  XmNleftAttachment,	XmATTACH_FORM,
		  XmNtopAttachment,	XmATTACH_FORM,
		  NULL);
  
	  Current_interference_label = XtVaCreateManagedWidget ("UNKNOWN ",
		  xmLabelWidgetClass,	interference_form,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNfontList,		hci_get_fontlist (LIST),
		  XmNleftAttachment,	XmATTACH_WIDGET,
		  XmNleftWidget,		label,
		  XmNtopAttachment,	XmATTACH_FORM,
		  NULL);
  
	  n = 0;
  
	  XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	  XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	  XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	  XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	  XtSetArg (arg [n], XmNorientation,	XmVERTICAL);         n++;
	  XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	  XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	  XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;
  
	  interference_list = XmCreateRadioBox (interference_form,
		  "interference_list", arg, n);
  
	  Interference_enable_button = XtVaCreateManagedWidget ("Enabled",
		  xmToggleButtonWidgetClass,	interference_list,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNselectColor,		hci_get_read_color (WHITE),
		  XmNfontList,		hci_get_fontlist (LIST),
		  NULL);
  
	  XtAddCallback (Interference_enable_button,
		  XmNvalueChangedCallback, toggle_interference_suppression_callback,
		  (XtPointer) ISU_ENABLED);
  
	  Interference_disable_button = XtVaCreateManagedWidget ("Disabled",
		  xmToggleButtonWidgetClass,	interference_list,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNselectColor,		hci_get_read_color (WHITE),
		  XmNfontList,		hci_get_fontlist (LIST),
		  NULL);
  
	  XtAddCallback (Interference_disable_button,
		  XmNvalueChangedCallback, toggle_interference_suppression_callback,
		  (XtPointer) ISU_DISABLED);
  
	  XtManageChild (interference_list);
	  XtManageChild (interference_form);
  
/*	Put a space between the interference and sport blanking frames.	*/

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		interference_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Create the buttons for the Spot Blanking Status selections.	*/

	Spot_blanking_frame   = XtVaCreateManagedWidget ("spot_blanking_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		space,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Spot Blanking Status",
		xmLabelWidgetClass,	Spot_blanking_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	spot_blanking_form = XtVaCreateWidget ("spot_blanking_form",
		xmFormWidgetClass,	Spot_blanking_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Status: ",
		xmLabelWidgetClass,	spot_blanking_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	Current_spot_blanking_label = XtVaCreateManagedWidget ("UNKNOWN ",
		xmLabelWidgetClass,	spot_blanking_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR2),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (WHITE));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);                  n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);                  n++;
	XtSetArg (arg [n], XmNorientation,	XmVERTICAL);       n++;
	XtSetArg (arg [n], XmNtopAttachment,	XmATTACH_WIDGET);    n++;
	XtSetArg (arg [n], XmNtopWidget,	label);		     n++;
	XtSetArg (arg [n], XmNleftAttachment,	XmATTACH_FORM);	     n++;

	spot_blanking_list = XmCreateRadioBox (spot_blanking_form,
		"spot_blanking_list", arg, n);

	Spot_blanking_enable_button = XtVaCreateManagedWidget ("Enabled",
		xmToggleButtonWidgetClass,	spot_blanking_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Spot_blanking_enable_button,
		XmNvalueChangedCallback, toggle_spot_blanking_callback,
		(XtPointer) SB_ENABLED);

	Spot_blanking_disable_button = XtVaCreateManagedWidget ("Disabled",
		xmToggleButtonWidgetClass,	spot_blanking_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Spot_blanking_disable_button,
		XmNvalueChangedCallback, toggle_spot_blanking_callback,
		(XtPointer) SB_DISABLED);

	XtManageChild (spot_blanking_list);
	XtManageChild (spot_blanking_form);

/*	Create a container and set of labels to display various RDA	*
 *	status information.						*/

	status_frame   = XtVaCreateManagedWidget ("status_frame",
		xmFrameWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			interference_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	status_form = XtVaCreateWidget ("status_form",
		xmFormWidgetClass,		status_frame,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	status1_rowcol = XtVaCreateWidget ("status1_rowcol",
		xmRowColumnWidgetClass,		status_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_COLUMN,
		XmNcolumns,			1,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	status2_rowcol = XtVaCreateWidget ("status2_rowcol",
		xmRowColumnWidgetClass,		status_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_COLUMN,
		XmNcolumns,			1,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			status1_rowcol,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("S",
		xmLabelWidgetClass,		status_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			status2_rowcol,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	status3_rowcol = XtVaCreateWidget ("status3_rowcol",
		xmRowColumnWidgetClass,		status_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_COLUMN,
		XmNcolumns,			1,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			label,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	status4_rowcol = XtVaCreateWidget ("status4_rowcol",
		xmRowColumnWidgetClass,		status_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_COLUMN,
		XmNcolumns,			1,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			status3_rowcol,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Operational Mode:   ",
		xmLabelWidgetClass,	status1_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Operational_mode_label = XtVaCreateManagedWidget ("                ",
		xmLabelWidgetClass,	status2_rowcol,
		XmNbackground,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Control Authority:  ",
		xmLabelWidgetClass,	status1_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Control_authorization_label = XtVaCreateManagedWidget ("                ",
		xmLabelWidgetClass,	status2_rowcol,
		XmNbackground,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Transmitter Power:  ",
		xmLabelWidgetClass,	status1_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Ave_transmitter_power_label = XtVaCreateManagedWidget ("                ",
		xmLabelWidgetClass,	status2_rowcol,
		XmNbackground,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Calib. Correction: ",
		xmLabelWidgetClass,	status3_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Calibration_correction_label = XtVaCreateManagedWidget ("            ",
		xmLabelWidgetClass,	status4_rowcol,
		XmNbackground,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	  XtVaCreateManagedWidget ("Interference Rate: ",
		  xmLabelWidgetClass,	status3_rowcol,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNfontList,		hci_get_fontlist (LIST),
		  NULL);
  
	  Interference_rate_label = XtVaCreateManagedWidget ("            ",
		  xmLabelWidgetClass,	status4_rowcol,
		  XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		  XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		  XmNfontList,		hci_get_fontlist (LIST),
		  NULL);

	XtVaCreateManagedWidget ("Moments Enabled:   ",
		xmLabelWidgetClass,	status3_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Moments_enabled_label = XtVaCreateManagedWidget ("            ",
		xmLabelWidgetClass,	status4_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (status1_rowcol);
	XtManageChild (status2_rowcol);
	XtManageChild (status3_rowcol);
	XtManageChild (status4_rowcol);
	XtManageChild (status_form);

/*	If the site is not FAA redundant and spot blanking is not	*
 *	installed, then do not manage the lock button.  Otherwise, we	*
 *	need to manage it.						*/

	if ((HCI_get_system() != HCI_FAA_SYSTEM) &&
	    (ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS) == SPOT_BLANKING_NOT_INSTALLED)) {

	    XtUnmanageChild (Lock_button);

	}

	if (ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS) == SPOT_BLANKING_NOT_INSTALLED) {

	    XtUnmanageChild (Spot_blanking_frame);

	}

/*	If low bandwidth, display a progress meter.			*/

	HCI_PM( "Reading RDA Status Information" );

/*	Use information in the latest RDA status message to update the	*
 *	properties (colors/sensitivity) of the buttons and labels.	*/

	update_rda_control_menu_properties ();

	XtManageChild (form);

/*	If I/O was cancelled, do not pop up shell.			*/

	if (ORPGRDA_status_io_status() != RMT_CANCELLED) {

	    XtRealizeWidget (Top_widget);

	    /* Start HCI loop. */

	    HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );
	}

	return 0;
}

/************************************************************************
 *	Description: This function is the timer procedure for the RDA	*
 *		     Control task.  Its main purpose is to update the	*
 *		     window properties when the wideband state changes	*
 *		     or when new RDA status data are received.		*
 *									*
 *	Input:  w - timer parent widget ID				*
 *		id - timer interval ID					*
 *	Output: NONE							*
 *	Return: 0 (unused)						*
 ************************************************************************/

void
timer_proc ()
{
  static  int     old_wbstat = -1;
  static	time_t	old_RDA_status_update_time = -1;

  /* If the RDA configuration changes, call function to display
     popup window and exit. */

  if( ORPGRDA_get_rda_config( NULL ) != ORPGRDA_LEGACY_CONFIG )
  {
    rda_config_change();
  }

  if ((ORPGRDA_status_update_flag() == 0) ||
      (ORPGRDA_status_update_time () != old_RDA_status_update_time) ||
      (ORPGRDA_get_wb_status(ORPGRDA_WBLNSTAT) != old_wbstat) ||
      (Redundant_status_change_flag == HCI_YES_FLAG) )
  {
    Redundant_status_change_flag = HCI_NO_FLAG;
    old_wbstat = ORPGRDA_get_wb_status(ORPGRDA_WBLNSTAT);
    old_RDA_status_update_time = ORPGRDA_status_update_time ();
    update_rda_control_menu_properties ();
  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the RDA Control/Status	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
RDA_control_close (
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
)
{
	HCI_LE_log("RDA Close pushed");
	XtDestroyWidget (Top_widget);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the RDA State radio buttons in the RDA	*
 *		     Control/Status window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data -           				*
 *			      RS_OPERATE				*
 *			      RS_OFFOPER				*
 *			      RS_RESTART				*
 *			      RS_STANDBY				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
change_rda_state_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	User_selected_RDA_state_flag = (int) client_data;

	if (state->set) {

		sprintf( buf, "You are about to change the RDA state.\nDo you want to continue?" );
                hci_confirm_popup( Top_widget, buf, rda_state_callback, NULL);
		XtVaSetValues (w, XmNset, False, NULL );
	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the RDA State confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data -           				*
 *			      RS_OPERATE				*
 *			      RS_OFFOPER				*
 *			      RS_RESTART				*
 *			      RS_STANDBY				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rda_state_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
/*	If "RS_OPERATE" selected then check to see if it is different 	*
 *	than the current state and RPG control is allowed.  If so,	*
 *	generate the command.						*/

	if (User_selected_RDA_state_flag == RS_OPERATE) {

	    HCI_LE_log("RDA Operate selected");

	    if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		HCI_display_feedback( Cmd );

	    } else {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Change RDA to Operate state");

		HCI_display_feedback( Cmd );

		ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_OPERATE,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  NULL);

	    }

/*	If "RS_OFFOPER" selected then check to see if it is 		*
 *	different than the current state and RPG control is allowed.	*
 *	If so, generate the command.					*/

	} else if (User_selected_RDA_state_flag == RS_OFFOPER) {

	    HCI_LE_log("RDA Offline Operate selected");

	    if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		HCI_display_feedback( Cmd );

	    } else {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Change RDA to Offline Operate state");

		HCI_display_feedback( Cmd );

		ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_OFFOPER,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  NULL);

	    }

/*	If "RS_RESTART" selected then check to see if it is 		*
 *	different than the current state and RPG control is allowed.	*
 *	If so, generate the command.					*/

	} else if (User_selected_RDA_state_flag == RS_RESTART) {

	    HCI_LE_log("RDA Restart selected");

	    if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		HCI_display_feedback( Cmd );

	    } else {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Change RDA to Restart state");

		HCI_display_feedback( Cmd );

		ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_RESTART,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  NULL);

	    }

/*	If "RS_STANDBY" selected then check to see if it is	 	*
 *	different than the current state and RPG control is allowed.	*
 *	If so, generate the command.					*/

	} else if (User_selected_RDA_state_flag == RS_STANDBY) {

	    HCI_LE_log("RDA Standby selected");

	    if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		HCI_display_feedback( Cmd );

	    } else {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Change RDA to Standby state");

		HCI_display_feedback( Cmd );

		ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_STANDBY,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  NULL);

	    }
	}

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the Interference Suppression Unit radio	*
 *		     buttons.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - ISU_DISABLED				*
 *			      ISU_ENABLED				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
toggle_interference_suppression_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    User_selected_ISU_flag = (int) client_data;

/*	If "INTERFERENCE_UNIT_DISABLED" selected then check to see if 	*
 *	it is different than the current state and RPG control is	*
 *	allowed.  If so, generate the command.				*/

	    if (User_selected_ISU_flag == ISU_DISABLED) {

	        HCI_LE_log("Interference Unit Disable selected");

	        if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		    Generate a feedback message which can be displayed	*
 *		    in the RPG Control/Status window.			*/

	            sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		    HCI_display_feedback( Cmd );

	        } else {

	    	    sprintf( buf, "You are about to issue a request to disable\ninterference suppression.  Do you want to continue?" );
                   hci_confirm_popup( Top_widget, buf, issue_inter_suppr_command_callback, cancel_inter_suppr_command_callback );

	        }

/*	If "YES" selected then check to see if it is different than the	*
 *	current state.  If so, update the ITC and change the button	*
 *	which is highlighted.  Otherwise, do nothing.			*/

	    } else if (User_selected_ISU_flag == ISU_ENABLED) {

	        HCI_LE_log("Interference Unit Enable selected");

	        if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		    Generate a feedback message which can be displayed	*
 *		    in the RPG Control/Status window.			*/

	            sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		    HCI_display_feedback( Cmd );

	        } else {

	    	    sprintf( buf, "You are about to issue a request to enable\ninterference suppression.  Do you want to continue?" );
                    hci_confirm_popup( Top_widget, buf, issue_inter_suppr_command_callback, cancel_inter_suppr_command_callback );

	        }
	    }

	    XtVaSetValues (w,
		XmNset,		False,
		NULL);
	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Interference Suppression	*
 *		     Unit confirmation popup window.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cancel_inter_suppr_command_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the Interference Suppression	*
 *		     Unit confirmation popup window.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - ISU_DISABLED				*
 *			      ISU_ENABLED				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
issue_inter_suppr_command_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	if (User_selected_ISU_flag == ISU_DISABLED) {

/*	    Generate a feedback message which can be displayed	*
 *	    in the RPG Control/Status window.			*/

	    sprintf (Cmd,"Disable Interference Suppression Unit");

	    HCI_display_feedback( Cmd );

	    ORPGRDA_send_cmd (COM4_RDACOM,
			  (int) HCI_INITIATED_RDA_CTRL_CMD,
			  CRDA_CTLISU,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  NULL);

	} else if (User_selected_ISU_flag == ISU_ENABLED) {

/*	    Generate a feedback message which can be displayed	*
 *	    in the RPG Control/Status window.			*/

	    sprintf (Cmd,"Enable Interference Suppression Unit");

	    HCI_display_feedback( Cmd );

	    ORPGRDA_send_cmd (COM4_RDACOM,
			  (int) HCI_INITIATED_RDA_CTRL_CMD,
			  CRDA_CTLISU,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  NULL);

	}

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the Redundant Control radio buttons.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - RDA_IS_CONTROLLING			*
 *			      RDA_IS_NONCONTROLLING			*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
toggle_red_control_callback (
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	User_selected_RDA_red_control_flag = (int) client_data;

	if (state->set) {

	    sprintf( buf, "You are about to change the control\nstate of this channel.\nDo you want to continue?" );
            hci_confirm_popup( Top_widget, buf, accept_red_control_callback, cancel_red_control_callback );

	}

	XtVaSetValues (w, XmNset, False, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Redundant Control		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cancel_red_control_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Redundant Control		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - RDA_IS_CONTROLLING			*
 *			      RDA_IS_NONCONTROLLING			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_red_control_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	if (User_selected_RDA_red_control_flag == RDA_IS_CONTROLLING) {

	    HCI_LE_log("Command RDA channel %d to Controlling",
		    ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

	    sprintf (Cmd,"Command RDA channel %d to Controlling",
		    ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

/*	    Generate a feedback message which can be displayed	*
 *	    in the RPG Control/Status window.			*/

	    HCI_display_feedback( Cmd );

	    ORPGRDA_send_cmd (COM4_RDACOM,
			      (int) HCI_INITIATED_RDA_CTRL_CMD,
			      CRDA_CHAN_CTL,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      NULL);

	} else {

	    HCI_LE_log("Command RDA channel %d to Non-controlling",
		    ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

/*	    Generate a feedback message which can be displayed	*
 *	    in the RPG Control/Status window.			*/

	    sprintf (Cmd,"Command RDA channel %d to Non-controlling",
		    ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

	    HCI_display_feedback( Cmd );

	    ORPGRDA_send_cmd (COM4_RDACOM,
			      (int) HCI_INITIATED_RDA_CTRL_CMD,
			      CRDA_CHAN_NONCTL,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      NULL);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the RDA control radio buttons.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - CS_LOCAL_ONLY				*
 *			      CS_RPG_REMOTE				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
toggle_RDA_control_callback (
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    if ((int) client_data == CS_LOCAL_ONLY) {

		HCI_LE_log("Enable RDA control of RDA selected");

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

		sprintf (Cmd,"Enable RDA control of RDA");

	        HCI_display_feedback( Cmd );

		ORPGRDA_send_cmd (COM4_RDACOM,
			      (int) HCI_INITIATED_RDA_CTRL_CMD,
			      CRDA_ENALOCAL,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      (int) 0,
			      NULL);

	    } else {

		HCI_LE_log("Request RPG to control RDA selected");

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

		sprintf (Cmd,"Request RPG to control RDA");

	        HCI_display_feedback( Cmd );

		ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_REQREMOTE,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  NULL);

	    }

	    XtVaSetValues (w,
		XmNset,		False,
		NULL);
	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the RDA Power Source radio buttons.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - UTILITY_POWER				*
 *			      AUXILLIARY_POWER				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
switch_power_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    Verify_power_source_change (w, client_data);

	    XtVaSetValues (w,
		XmNset,		False,
		NULL);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Get Status" button in the RDA Control/Status	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
request_RDA_status_callback (
	Widget		w,
	XtPointer	client_data,
	XtPointer	call_data
)
{

	HCI_LE_log("Request RDA status data selected");

/*	Generate a feedback message which can be displayed	*
 *	in the RPG Control/Status window.			*/

	sprintf (Cmd,"Requesting RDA Status data");

	HCI_display_feedback( Cmd );

	ORPGRDA_send_cmd (COM4_REQRDADATA,
			  (int) HCI_INITIATED_RDA_CTRL_CMD,
			  DREQ_STATUS,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  (int) 0,
		  	  NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the Spot Blanking radio buttons.		*
 *		     NOTE: This is a restricted command and requires	*
 *		     a password to invoke.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - SB_ENABLED				*
 *			      SB_DISABLED				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
toggle_spot_blanking_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	verify_spot_blanking (Widget w, XtPointer client_data);

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    verify_spot_blanking (w, client_data);

	    XtVaSetValues (w,
		XmNset,		False,
		NULL);

	}
}

/************************************************************************
 *	Description: This function is activated after the user selects	*
 *		     one of the Spot Blanking radio buttons.  It	*
 *		     generates a confrmation popup window.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - SB_ENABLED				*
 *			      SB_DISABLED				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
verify_spot_blanking (
Widget		w,
XtPointer	client_data
)
{
	char buf[HCI_BUF_128];

	User_selected_spot_blank_flag = (int) client_data;

	sprintf( buf, "You are about to change the spot blanking mode.\nDo you want to continue?" );

        hci_confirm_popup( Top_widget, buf, spot_blanking_callback, NULL );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the Spot Blanking		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - SB_ENABLED				*
 *			      SB_DISABLED				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
spot_blanking_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
/*	If "SPOT_BLANKING_DISABLED" selected then check to see if 	*
 *	it is different than the current state and RPG control is	*
 *	allowed.  If so, generate the command.				*/

	if (User_selected_spot_blank_flag == SB_DISABLED) {

	    HCI_LE_log("Spot Blanking Status Disable selected");

	    if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		HCI_display_feedback( Cmd );

	    } else {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

		sprintf (Cmd,"Disable Spot Blanking Status");

		HCI_display_feedback( Cmd );

	    	ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_SB_DIS,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
		  		  NULL);

	    }

/*	If "SPOT_BLANKING_ENABLED" selected then check to see if 	*
 *	it is different than the current state and RPG control is	*
 *	allowed.  If so, generate the command.				*/

	} else if (User_selected_spot_blank_flag == SB_ENABLED) {

	    HCI_LE_log("Spot Blanking Status Enable selected");

	    if (ORPGRDA_get_status (RS_CONTROL_STATUS) == CS_LOCAL_ONLY) {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

	        sprintf (Cmd,"Rejected - RDA currently in control of RDA");

		HCI_display_feedback( Cmd );

	    } else {

/*		Generate a feedback message which can be displayed	*
 *		in the RPG Control/Status window.			*/

		strcpy (Cmd,"Enable Spot Blanking Status");

		HCI_display_feedback( Cmd );

	    	ORPGRDA_send_cmd (COM4_RDACOM,
				  (int) HCI_INITIATED_RDA_CTRL_CMD,
				  CRDA_SB_ENAB,
				  (int) 0,
				  (int) 0,
				  (int) 0,
				  (int) 0,
		  		  NULL);

	    }
	}
}

/************************************************************************
 *	Description: This function is used to update the objects in the	*
 *		     RDA Control/Status window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
update_rda_control_menu_properties ()
{
	char	buf  [80];
	XmString	string;
	Pixel	highlight_color;
	Pixel	foreground_color;
	Boolean	sensitivity;
	short	moments;
	int	wbstat;
	int	blanking;
	int	status;
	int	calib;
	int	red_type;
	int	other_ch_state;
	int	my_ch_state;
	int	rs_cntrl_stat;

	foreground_color  = hci_get_read_color (BUTTON_FOREGROUND);

/*	Check the display blanking flag.  If it is non-zero, then	*
 *	display blanking is enabled and only the RDA status element	*
 *	which corresponds to the half word index defined by the flag	*
 *	is displayed.							*/

	wbstat   = ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT);
	blanking = ORPGRDA_get_wb_status (ORPGRDA_DISPLAY_BLANKING);
	red_type = HCI_get_system();
	other_ch_state = ORPGRED_rda_control_status (ORPGRED_OTHER_CHANNEL);
	my_ch_state = ORPGRED_rda_control_status (ORPGRED_MY_CHANNEL);
	rs_cntrl_stat = ORPGRDA_get_status (RS_CONTROL_STATUS);

	if (blanking) {

	    highlight_color = hci_get_read_color (BUTTON_FOREGROUND);

	} else {

	    highlight_color = hci_get_read_color (WHITE);

	}

/*	If the RDA Control window exists, then update the child widget	*
 *	properties based on the RDA control and wideband states.	*/

	if (Top_widget != (Widget) NULL) {

	    if (ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS) != SPOT_BLANKING_NOT_INSTALLED) {

		Dimension	width;

		XtVaSetValues (Top_widget,
			XmNmaxWidth,	(Dimension) 1024,
			XmNallowShellResize,	True,
			NULL);

		XtManageChild (Lock_button);
		XtManageChild (Spot_blanking_frame);
		XtManageChild (XtParent (Spot_blanking_frame));

		XtVaGetValues (Top_widget,
			XmNwidth,	&width,
			NULL);

		XtVaSetValues (Top_widget,
			XmNminWidth,	width,
			XmNmaxWidth,	width,
			NULL);
		XtVaSetValues (Top_widget,
			XmNallowShellResize,	False,
			NULL);

	    }

/*	    If the wideband is not connected, then we want to 		*
 *	    desensitize the "Get Status" button.			*/

	    if (wbstat != RS_CONNECTED) {

		XtVaSetValues (Status_button,
			XmNsensitive,	False,
			NULL);

	    } else {

		XtVaSetValues (Status_button,
			XmNsensitive,	True,
			NULL);

	    }

/*	If the RDA is in control, desensitize the commands which cannot	*
 *	be invoked.							*/

	    if (((rs_cntrl_stat == CS_RPG_REMOTE) ||
		 (rs_cntrl_stat == CS_EITHER)) &&
		(wbstat == RS_CONNECTED)) {

		sensitivity = True;

	    } else {

		sensitivity = False;

	    }

	    if (red_type == HCI_FAA_SYSTEM &&  
		other_ch_state == RDA_IS_CONTROLLING) {

		XtVaSetValues (Rda_operate_button,
			XmNsensitive,	False,
			XmNset,		False,
			NULL);

	    } else {

		XtVaSetValues (Rda_operate_button,
			XmNsensitive,	sensitivity,
			XmNset,		False,
			NULL);

	    }

	    XtVaSetValues (Rda_offline_operate_button,
		XmNsensitive,	sensitivity,
		XmNset,		False,
		NULL);

	    XtVaSetValues (Rda_restart_button,
		XmNsensitive,	sensitivity,
		XmNset,		False,
		NULL);

	    XtVaSetValues (Rda_standby_button,
		XmNsensitive,	sensitivity,
		XmNset,		False,
		NULL);


	    XtVaSetValues (Power_utility_button,
		XmNsensitive,	sensitivity,
		XmNset,		False,
	    	NULL);

	    XtVaSetValues (Power_auxiliary_button,
		XmNsensitive,	sensitivity,
		XmNset,		False,
		NULL);

	      XtVaSetValues (Interference_disable_button,
		  XmNsensitive,	sensitivity,
		  XmNset,		False,
	    	  NULL);
  
	      XtVaSetValues (Interference_enable_button,
		  XmNsensitive,	sensitivity,
		  XmNset,		False,
		  NULL);

	    if (hci_lock_URC_selected() || hci_lock_ROC_selected()) {

		    XtVaSetValues (Spot_blanking_disable_button,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (LOCA_FOREGROUND),
			XmNset,		False,
			NULL);

		    XtVaSetValues (Spot_blanking_enable_button,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (LOCA_FOREGROUND),
			XmNset,		False,
			NULL);

		} else {

		    XtVaSetValues (Spot_blanking_disable_button,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNset,		False,
			NULL);

		    XtVaSetValues (Spot_blanking_enable_button,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNset,		False,
			NULL);

	    }

	    if (Unlocked_loca == HCI_YES_FLAG) {

		    XtVaSetValues (Spot_blanking_disable_button,
			XmNsensitive,	sensitivity,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNset,		False,
			NULL);

		    XtVaSetValues (Spot_blanking_enable_button,
			XmNsensitive,	sensitivity,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNset,		False,
			NULL);

		} else {

		    XtVaSetValues (Spot_blanking_disable_button,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNset,		False,
			NULL);

		    XtVaSetValues (Spot_blanking_enable_button,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNset,		False,
			NULL);

	    }

	    if (red_type == HCI_FAA_SYSTEM) {

		if (hci_lock_URC_selected() || hci_lock_ROC_selected()) {

			XtVaSetValues (Red_control_button,
				XmNsensitive,	False,
				XmNforeground,	hci_get_read_color (LOCA_FOREGROUND),
				XmNset,		False,
				NULL);

			if (ORPGRED_channel_num (ORPGRED_MY_CHANNEL) != 1) {

			    XtVaSetValues (Red_non_control_button,
				XmNsensitive,	False,
				XmNforeground,	hci_get_read_color (LOCA_FOREGROUND),
				XmNset,		False,
				NULL);

			}

		} else {

			XtVaSetValues (Red_control_button,
				XmNsensitive,	False,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNset,		False,
				NULL);

			if (ORPGRED_channel_num (ORPGRED_MY_CHANNEL) != 1) {

			    XtVaSetValues (Red_non_control_button,
				XmNsensitive,	False,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNset,		False,
				NULL);

			}
		}

		if (Unlocked_loca == HCI_YES_FLAG) {

			XtVaSetValues (Red_control_button,
				XmNsensitive,	sensitivity,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNset,		False,
				NULL);

			if (ORPGRED_channel_num (ORPGRED_MY_CHANNEL) != 1) {

			    XtVaSetValues (Red_non_control_button,
				XmNsensitive,	sensitivity,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNset,		False,
				NULL);

			}

		} else {

			XtVaSetValues (Red_control_button,
				XmNsensitive,	False,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNset,		False,
				NULL);

			if (ORPGRED_channel_num (ORPGRED_MY_CHANNEL) != 1) {

			    XtVaSetValues (Red_non_control_button,
				XmNsensitive,	False,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNset,		False,
				NULL);

			}
		}
	    }

/*	    Check to see if display blanking disabled for next item	*/

	    if ((blanking && rs_cntrl_stat == CS_LOCAL_ONLY) ||
		!((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		XtVaSetValues (Rda_operate_button,
			XmNsensitive,	False,
			NULL);

	 	XtVaSetValues (Rda_offline_operate_button,
			XmNsensitive,	False,
			NULL);

		XtVaSetValues (Rda_restart_button,
			XmNsensitive,	False,
			NULL);

		XtVaSetValues (Rda_standby_button,
			XmNsensitive,	False,
			NULL);

		if (blanking != RS_CONTROL_STATUS) {

		    sprintf (buf,"    UNKNOWN    ");

		} else {

		    switch (ORPGRDA_get_status (RS_RDA_STATUS)) {

		 	case RS_OPERATE :

			    sprintf (buf,"    OPERATE    ");
			    break;

			case RS_OFFOPER:

			    sprintf (buf,"OFFLINE OPERATE");
			    break;

			case RS_RESTART:

			    sprintf (buf,"    RESTART    ");
			    break;

			case RS_STARTUP:

			    sprintf (buf,"   START-UP    ");
			    break;

			case RS_STANDBY:

			    sprintf (buf,"    STANDBY    ");
			    break;

			default:

			    sprintf (buf,"    UNKNOWN    ");
			    break;

		    }
	        }

	    } else {

		switch (ORPGRDA_get_status (RS_RDA_STATUS)) {

	 	    case RS_OPERATE :

			sprintf (buf,"    OPERATE    ");

			XtVaSetValues (Rda_operate_button,
				XmNsensitive,	False,
				NULL);
			break;

		    case RS_OFFOPER:

			sprintf (buf,"OFFLINE OPERATE");

		 	XtVaSetValues (Rda_offline_operate_button,
				XmNsensitive,	False,
				NULL);
			break;

		    case RS_RESTART:

			sprintf (buf,"    RESTART    ");

			XtVaSetValues (Rda_restart_button,
				XmNsensitive,	False,
				NULL);
			break;

		    case RS_STARTUP:

			sprintf (buf,"   START-UP    ");
			break;

		    case RS_STANDBY:

			sprintf (buf,"    STANDBY    ");

			XtVaSetValues (Rda_standby_button,
				XmNsensitive,	False,
				NULL);
			break;

		    default:

			sprintf (buf,"    UNKNOWN    ");
			break;

		}
	    }

	    string = XmStringCreateLocalized (buf);

	    XtVaSetValues (Current_state_label,
		XmNlabelString,	string,
		NULL);

	    XmStringFree (string);

	    if (red_type == HCI_FAA_SYSTEM) {

	        if (wbstat != RS_CONNECTED) {

		    sprintf (buf,"      UNKNOWN       ");

	        }
	        else {

		    if (my_ch_state == ORPGRED_RDA_CONTROLLING) {

		        XtVaSetValues (Red_control_button,
			    XmNsensitive,	False,
			    NULL);

		        sprintf (buf,"    CONTROLLING     ");

		    } else if( my_ch_state == ORPGRED_RDA_NON_CONTROLLING) {

		        if (ORPGRED_channel_num (ORPGRED_MY_CHANNEL) != 1) {

			    XtVaSetValues (Red_non_control_button,
				    XmNsensitive,	False,
				    NULL);

		        }

		        sprintf (buf,"  NON-CONTROLLING   ");

		    } else {
		        sprintf (buf,"      UNKNOWN       ");
		     
	            }
	        }

		string = XmStringCreateLocalized (buf);

		XtVaSetValues (Current_red_local_status,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

		{
		    time_t	rtime;
		    time_t	ltime;
		    int		fg;
		    int		bg;
		    int		month, day, year;
		    int		hour, minute, second;

		    ltime = ORPGRED_adapt_dat_time (ORPGRED_MY_CHANNEL);
		    rtime = ORPGRED_adapt_dat_time (ORPGRED_OTHER_CHANNEL);

		    if (rtime == ltime) {

			fg = hci_get_read_color (TEXT_FOREGROUND);
			bg = hci_get_read_color (BACKGROUND_COLOR2);

		    } else {

			fg = hci_get_read_color (WHITE);
			bg = hci_get_read_color (ALARM_COLOR1);

		    }

		    if (ltime > 0) {

			unix_time (&ltime,
			       &year,
			       &month,
			       &day,
			       &hour,
			       &minute,
			       &second);

		    } else {

			month  = 0;
			day    = 0;
			year   = 0;
			hour   = 0;
			minute = 0;
			second = 0;

		    }

		    sprintf (buf,"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d UT",
				month, day, year%100, hour, minute, second);

		    string = XmStringCreateLocalized (buf);

		    XtVaSetValues (Current_red_local_time,
			XmNlabelString,	string,
			XmNforeground,	fg,
			XmNbackground,	bg,
			NULL);

		    XmStringFree (string);

		    if (rtime > 0) {

			unix_time (&rtime,
			       &year,
			       &month,
			       &day,
			       &hour,
			       &minute,
			       &second);

		    } else {

			month  = 0;
			day    = 0;
			year   = 0;
			hour   = 0;
			minute = 0;
			second = 0;

		    }

		    sprintf (buf,"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d UT",
				month, day, year%100, hour, minute, second);

		    string = XmStringCreateLocalized (buf);

		    XtVaSetValues (Current_red_red_time,
			XmNlabelString,	string,
			XmNforeground,	fg,
			XmNbackground,	bg,
			NULL);

		    XmStringFree (string);

		}

	        if (wbstat != RS_CONNECTED) {

		    sprintf (buf,"      UNKNOWN       ");

	        }
	        else {

		    if (other_ch_state == RDA_IS_CONTROLLING) {

		        sprintf (buf,"    CONTROLLING     ");

		    } else {

		        sprintf (buf,"  NON-CONTROLLING   ");

		    }
	        }

		string = XmStringCreateLocalized (buf);

		XtVaSetValues (Current_red_red_status,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);
	    }

/*	    Check to see if display blanking disabled for next item	*/

	    if ((blanking && rs_cntrl_stat == CS_LOCAL_ONLY) ||
		!((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		XtVaSetValues (Control_rda_button,
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Control_rpg_button,
			XmNsensitive,	False,
			NULL);

		if (blanking != RS_CONTROL_STATUS) {

		    sprintf (buf,"  UNKNOWN   ");

		} else {

		    switch (rs_cntrl_stat) {
	
		        case CS_LOCAL_ONLY:

			    sprintf (buf,"LOCAL (RDA) ");
			    break;

		        case CS_RPG_REMOTE:

			    sprintf (buf,"REMOTE (RPG)");
			    break;

		        case CS_EITHER:

			    sprintf (buf,"   EITHER   ");
			    break;

		        default:

			    sprintf (buf,"  UNKNOWN   ");
			    break;

		    }
		}

	    } else {

		switch (rs_cntrl_stat) {
	
		    case CS_LOCAL_ONLY:

			XtVaSetValues (Control_rda_button,
				XmNsensitive,	False,
				NULL);
			XtVaSetValues (Control_rpg_button,
				XmNsensitive,	True,
				NULL);
			sprintf (buf,"LOCAL (RDA) ");
			break;

		    case CS_RPG_REMOTE:

			XtVaSetValues (Control_rda_button,
				XmNsensitive,	True,
				NULL);
			XtVaSetValues (Control_rpg_button,
				XmNsensitive,	False,
				NULL);
			sprintf (buf,"REMOTE (RPG)");
			break;

		    case CS_EITHER:

			XtVaSetValues (Control_rda_button,
				XmNsensitive,	False,
				NULL);
			XtVaSetValues (Control_rpg_button,
				XmNsensitive,	False,
				NULL);
			sprintf (buf,"   EITHER   ");
			break;

		    default:

			sprintf (buf,"  UNKNOWN   ");
			break;

		}
	    }

	    string = XmStringCreateLocalized (buf);

	    XtVaSetValues (Current_control_label,
		XmNlabelString,	string,
		NULL);

	    XmStringFree (string);

/*	    Check to see if display blanking disabled for next item	*/

	    if ((blanking && rs_cntrl_stat == CS_LOCAL_ONLY) ||
		!((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		XtVaSetValues (Power_utility_button,
			XmNsensitive,	False,
		    	NULL);

		XtVaSetValues (Power_auxiliary_button,
			XmNsensitive,	False,
			NULL);

		if (blanking != RS_AUX_POWER_GEN_STATE) {

		    sprintf (buf," UNKNOWN ");

		} else {

		    if ((ORPGRDA_get_status (RS_AUX_POWER_GEN_STATE) & 1)) {

			sprintf (buf,"AUXILIARY");

		    } else {

			sprintf (buf," UTILITY ");

		    }
		}

	    } else {

		if ((ORPGRDA_get_status (RS_AUX_POWER_GEN_STATE) & 1)) {

		    sprintf (buf,"AUXILIARY");

		    XtVaSetValues (Power_auxiliary_button,
			XmNsensitive,	False,
			NULL);

		} else {

		    sprintf (buf," UTILITY ");

		    XtVaSetValues (Power_utility_button,
			XmNsensitive,	False,
		    	NULL);

		}
	    }

	    string = XmStringCreateLocalized (buf);

	    XtVaSetValues (Current_power_label,
		XmNlabelString,	string,
		NULL);

	    XmStringFree (string);

/*	    Check to see if display blanking disabled for next item	*/

	    if ((blanking && rs_cntrl_stat == CS_LOCAL_ONLY) ||
		!((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {
  
		XtVaSetValues (Interference_disable_button,
			  XmNsensitive,	False,
		    	  NULL);
  
		XtVaSetValues (Interference_enable_button,
			  XmNsensitive,	False,
			  NULL);
  
		if (blanking != RS_INTERFERENCE_DETECT_RATE) {
  
		      sprintf (buf,"UNKNOWN ");
  
	        } else {
	  	
		    if (ORPGRDA_get_status (RS_ISU) == ISU_DISABLED) {
  
			  sprintf (buf,"DISABLED");
	  	
		    } else if (ORPGRDA_get_status (RS_ISU) == ISU_ENABLED) {
  
			  sprintf (buf,"ENABLED ");
  
		    } else {
  
			  sprintf (buf,"UNKNOWN ");
  
		    }
	        }
  
	    } else {
  
		if (ORPGRDA_get_status (RS_ISU) == ISU_DISABLED) {
  
		      XtVaSetValues (Interference_disable_button,
			  XmNsensitive,	False,
		    	  NULL);
  
		      sprintf (buf,"DISABLED");
  
		} else if (ORPGRDA_get_status (RS_ISU) == ISU_ENABLED) {
  
		      XtVaSetValues (Interference_enable_button,
			  XmNsensitive,	False,
			  NULL);
  
		      sprintf (buf,"ENABLED ");
  
		} else {
  
		      sprintf (buf,"UNKNOWN ");
  
	        }
	    }
  
	    string = XmStringCreateLocalized (buf);
  
	    XtVaSetValues (Current_interference_label,
		  XmNlabelString,	string,
		  NULL);
  
	    XmStringFree (string);

/*	    Update the properties of the spot blanking items.	*/

	    if ((blanking && rs_cntrl_stat == CS_LOCAL_ONLY) ||
		!((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		XtVaSetValues (Spot_blanking_disable_button,
			XmNsensitive,	False,
			NULL);

		XtVaSetValues (Spot_blanking_enable_button,
			XmNsensitive,	False,
			NULL);

		if (blanking != RS_SPOT_BLANKING_STATUS) {

		    sprintf (buf,"UNKNOWN ");

		} else {
		
		    if (ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS) == SB_DISABLED) {

			sprintf (buf,"DISABLED");

		    } else {

			sprintf (buf,"ENABLED ");

		    }
	        }

	    } else {

		if (ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS) == SB_DISABLED) {

		    XtVaSetValues (Spot_blanking_disable_button,
			XmNsensitive,	False,
			NULL);

		    sprintf (buf,"DISABLED");

		} else {

		    XtVaSetValues (Spot_blanking_enable_button,
			XmNsensitive,	False,
			NULL);

		    sprintf (buf,"ENABLED ");

	        }
	    }

	    string = XmStringCreateLocalized (buf);

	    XtVaSetValues (Current_spot_blanking_label,
		XmNlabelString,	string,
		NULL);

	    XmStringFree (string);

/*	Update the status label Widgets					*/

	    if (((blanking == 0) ||
		(blanking == RS_OPERATIONAL_MODE)) &&
		((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		status = ORPGRDA_get_status (RS_OPERATIONAL_MODE);

		if (status == OP_MAINTENANCE_MODE) {

		    sprintf (buf,"  MAINTENANCE   ");

		} else if (status == OP_OPERATIONAL_MODE) {

		    sprintf (buf,"  OPERATIONAL   ");

		} else if (status == OP_OFFLINE_MAINTENANCE_MODE) {

		    /* We don't check explicitly for configuration since legacy
		       should never send this value. */
		    sprintf (buf,"  MAINTENANCE   ");

		} else {

		    sprintf (buf,"    UNKNOWN     ");

		}

		highlight_color = hci_get_read_color (WHITE);

	    } else {

		sprintf (buf,"    UNKNOWN     ");
		highlight_color = hci_get_read_color (BACKGROUND_COLOR2);

	    }

	    string = XmStringCreateSimple (buf);

	    XtVaSetValues (Operational_mode_label,
		    XmNlabelString,	string,
		    XmNbackground,	highlight_color,
		    NULL);

	    XmStringFree (string);

	    if (((blanking == 0) ||
		(blanking == RS_RDA_CONTROL_AUTH)) &&
		((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		status = ORPGRDA_get_status (RS_RDA_CONTROL_AUTH);

		if (status == CA_LOCAL_CONTROL_REQUESTED) {

		    sprintf (buf,"LOCAL REQUESTED ");

		} else if (status == CA_REMOTE_CONTROL_ENABLED) {

		    sprintf (buf," REMOTE ENABLED ");

		} else {

		    sprintf (buf,"   NO ACTION    ");

		}

		highlight_color = hci_get_read_color (WHITE);

	    } else {

		sprintf (buf,"    UNKNOWN     ");
		highlight_color = hci_get_read_color (BACKGROUND_COLOR2);

	    }

	    string = XmStringCreateSimple (buf);

	    XtVaSetValues (Control_authorization_label,
		    XmNlabelString,	string,
		    XmNbackground,	highlight_color,
		    NULL);

	    XmStringFree (string);

	    if (((blanking == 0) ||
		(blanking == RS_AVE_TRANS_POWER)) &&
		((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		sprintf (buf,"   %4d Watts   ",
		ORPGRDA_get_status (RS_AVE_TRANS_POWER));
		highlight_color = hci_get_read_color (WHITE);

	    } else {

		sprintf (buf,"    UNKNOWN     ");
		highlight_color = hci_get_read_color (BACKGROUND_COLOR2);

	    }

	    string = XmStringCreateSimple (buf);

	    XtVaSetValues (Ave_transmitter_power_label,
		    XmNlabelString,	string,
		    XmNbackground,	highlight_color,
		    NULL);

	    XmStringFree (string);

	    if (((blanking == 0) ||
		(blanking == RS_REFL_CALIB_CORRECTION)) &&
		((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		calib = ORPGRDA_get_status (RS_REFL_CALIB_CORRECTION);
		sprintf (buf," %7.2f dB ", ((float) calib)/4.0 );

		highlight_color = hci_get_read_color (WHITE);

	    } else {

		sprintf (buf,"  UNKNOWN   ");
		highlight_color = hci_get_read_color (BACKGROUND_COLOR2);

	    }

	    string = XmStringCreateSimple (buf);

	    XtVaSetValues (Calibration_correction_label,
		    XmNlabelString,	string,
		    XmNbackground,	highlight_color,
		    NULL);

	    XmStringFree (string);

	    if (((blanking == 0) ||
		  (blanking == RS_INTERFERENCE_DETECT_RATE)) &&
		  ((wbstat == RS_CONNECTED) ||
		    (wbstat == RS_DISCONNECT_PENDING))) {
  
		  sprintf (buf," %5d/sec  ",
		      ORPGRDA_get_status (RS_INTERFERENCE_DETECT_RATE));
		  highlight_color = hci_get_read_color (WHITE);
  
	    } else {
  
		  sprintf (buf,"  UNKNOWN   ");
		  highlight_color = hci_get_read_color (BACKGROUND_COLOR2);
  
	    }
  
	    string = XmStringCreateLocalized (buf);
  
	    XtVaSetValues (Interference_rate_label,
		      XmNlabelString,	string,
		      XmNbackground,	highlight_color,
		      NULL);
  
	    XmStringFree (string);

/*	Highlight the currently transmitted data types.			*/

	    if (((blanking == 0) ||
		(blanking == RS_DATA_TRANS_ENABLED)) &&
		((wbstat == RS_CONNECTED) ||
		  (wbstat == RS_DISCONNECT_PENDING))) {

		moments = (ORPGRDA_get_status (RS_DATA_TRANS_ENABLED)>>2) & 7;

		switch (moments) {

		    case 0 : /*  No moments enabled  */

			sprintf (buf,"    NONE    ");
			break;

		    case 1 : /*  Reflectivity enabled  */

			sprintf (buf,"     R      ");
			break;

		    case 2 : /*  Velocity enabled  */

			sprintf (buf,"     V      ");
			break;

		    case 3 : /*  Reflectivity, Velocity enabled  */

			sprintf (buf,"   R   V    ");
			break;

		    case 4 : /*  Spectrum Width enabled  */

			sprintf (buf,"     W      ");
			break;

		    case 5 : /*  Reflectivity, Spectrum Width enabled  */

			sprintf (buf,"   R   W    ");
			break;

		    case 6 : /*  Velocty, Spectrum Width enabled  */

			sprintf (buf,"   V   W    ");
			break;

		    case 7 : /*  Reflectivity, Velocity, Spectrum Width enabled  */

			sprintf (buf,"  R  V  W   ");
			break;

		}

		highlight_color = hci_get_read_color (WHITE);

	    } else {

		highlight_color = hci_get_read_color (BACKGROUND_COLOR2);
		sprintf (buf,"  UNKNOWN   ");

	    }

	    string = XmStringCreateLocalized (buf);

	    XtVaSetValues (Moments_enabled_label,
		    XmNlabelString,	string,
		    XmNbackground,	highlight_color,
		    NULL);

	    XmStringFree (string);

	}
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "RDA Alarms" button from the RDA Control/Status*
 *                   window.                                            *
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_select_rda_alarms_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  char task_name[ 100 ] = "";

  sprintf (task_name, "hci_rda_legacy -A %d -name \"RDA Alarms\"", HCI_get_channel_number() );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );
  hci_activate_child( HCI_get_display(),
		      RootWindowOfScreen( HCI_get_screen() ),
		      task_name,
		      "hci_rda_legacy",
		      "RDA Alarms",
		      -1 );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "VCP" button from the RDA Control/Status	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_select_vcp_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name[ 100 ] = "";

  sprintf( task_name, "hci_vcp -A %d -name \"VCP and Mode Control\"", HCI_get_channel_number() );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );
  hci_activate_child( HCI_get_display(),
		      RootWindowOfScreen( HCI_get_screen() ),
		      task_name,
		      "hci_vcp",
		      "VCP and Mode Control",
		      -1 );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button in the Lock RMS confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_lock_button_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int ret;
	
	if (change_rms_lock) {

	    ret = rms_rda_rpg_lock (RMS_RDA_LOCK, RMS_SET_LOCK);
	   
	    if ( ret < 0 ){
	   		HCI_LE_error("Unable to set RDA lock");
	   		}
	   else {
	   	Command_lock = 1;
		}
		
	} else {

	   ret = rms_rda_rpg_lock (RMS_RDA_LOCK, RMS_CLEAR_LOCK);
	   
	   if ( ret < 0 ){
	   		HCI_LE_error("Unable to clear RDA lock");
	   		}
	   else{
	    	Command_lock = 0;
	    	}

	}
	
	set_rda_button ();
	
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the Lock RMS check box.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_lock_button_verify_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];
	
	 XmToggleButtonCallbackStruct    *state =
		(XmToggleButtonCallbackStruct *) call_data;
	
	change_rms_lock = state->set;
	
	if (rms_get_lock_status (RMS_RDA_LOCK)){
		sprintf ( buf,"You are about to enable\nthe RMMS to RDA commands.\nDo you want to continue?");
		}
	else {
		sprintf ( buf,"You are about to disable\nthe RMMS to RDA commands.\nDo you want to continue?");
		}
		
        hci_confirm_popup( Top_widget, buf, hci_lock_button_callback, set_rda_button );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Lock RMS confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
set_rda_button ()

{

	int ret;
	
	ret = rms_get_lock_status (RMS_RDA_LOCK);
	
	 if ( ret < 0 ){
	   		HCI_LE_error("Unable to get RDA lock status");
	   		}/* End if */
	   		
	 else {  		
 		if (ret == RMS_COMMAND_LOCKED) {

			XtVaSetValues (Lock_rms,
				XmNset,	True,
				NULL);
	   		}
	   	else if (ret == RMS_COMMAND_UNLOCKED){

			XtVaSetValues (Lock_rms,
				XmNset,	False,
				NULL);
			}
		}/* End else */		
						
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the lock button or selects a LOCA radio button or	*
 *		     enters a password in the Password window.		*
 *									*
 *	Input:  w - widget ID						*
 *		security - lock data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
hci_rda_control_security ()
{
  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_URC_unlocked() || hci_lock_ROC_unlocked() )
    {
      Unlocked_loca = HCI_YES_FLAG;
    }
  }
  else if( hci_lock_close() && Unlocked_loca == HCI_YES_FLAG )
  {
      Unlocked_loca = HCI_NO_FLAG;
  }

  update_rda_control_menu_properties ();

  return HCI_LOCK_PROCEED;
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
   if( Top_widget != (Widget) NULL && !config_change_popup )
   {
     config_change_popup = 1; /* Indicate popup has been launched. */
     hci_rda_config_change_popup();
   }
   else
   {
     return;
   }
}

/************************************************************************
 *  Description: This function is called when the Redundant Manager     *
 *               posts a change.                                        *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

void Redundant_status_change_cb( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  Redundant_status_change_flag = HCI_YES_FLAG;
}


