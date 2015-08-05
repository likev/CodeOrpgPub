/************************************************************************
 *	Module:	 hci_selectable_product_parameters.c			*
 *									*
 *	Description:  This module is used by the ORPG HCI to edit	*
 *		      the selectable product parameters.		*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/07/24 17:46:24 $
 * $Id: hci_selectable_product_parameters.c,v 1.89 2012/07/24 17:46:24 ccalvert Exp $
 * $Revision: 1.89 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <product_parameters.h>
#include <vad.h>

/*	The following macros define the bits which indicate if a	*
 *	level is active for VAD or VAD and RCM products			*/

#define	VAD_HEIGHT_LEVEL	0x0001
#define	RCM_HEIGHT_LEVEL	0x0002

/*	The following macros define the selectable parameter categories	*/

enum {CELL_PRODUCT_PARAMETERS=1,
      OHP_THP_DATA_LEVELS,
      STP_DATA_LEVELS,
      VAD_RCM_HEIGHT_SELECTIONS,
      VELOCITY_DATA_LEVELS
};

/*	The following macros define bits that indicate whether a	*
 *	category has been modified or not.				*/

#define	CEL_CHANGE_FLAG		0x0001
#define	VAD_CHANGE_FLAG		0x0008
#define	OHP_CHANGE_FLAG		0x0010
#define	STP_CHANGE_FLAG		0x0020
#define	VEL_CHANGE_FLAG		0x0080

/*	Misc macros.							*/

#define	STR_LEN			16

/*	The following macros define the different Velocity Level	*
 *	tables.								*/

enum {VELOCITY_LEVEL_TABLE_PRECIP_16_05=0,
      VELOCITY_LEVEL_TABLE_PRECIP_16_10,
      VELOCITY_LEVEL_TABLE_PRECIP_08_05,
      VELOCITY_LEVEL_TABLE_PRECIP_08_10,
      VELOCITY_LEVEL_TABLE_CLEAR_16_05,
      VELOCITY_LEVEL_TABLE_CLEAR_16_10,
      VELOCITY_LEVEL_TABLE_CLEAR_08_05,
      VELOCITY_LEVEL_TABLE_CLEAR_08_10,
      NUM_VELOCITY_LEVELS
};

/*	Define the names of the velocity data tables			*/

char	*Vel_table [] = {
	"Velocity Data Levels (Precip 16/0.97)",
	"Velocity Data Levels (Precip 16/1.94)",
	"Velocity Data Levels (Precip 8/0.97)",
	"Velocity Data Levels (Precip 8/1.94)",
	"Velocity Data Levels (Clear Air 16/0.97)",
	"Velocity Data Levels (Clear Air 16/1.94)",
	"Velocity Data Levels (Clear Air 8/0.97)",
	"Velocity Data Levels (Clear Air 8/1.94)"
};

char	*Vel_dea_name_table [] = {
	VEL_DATA_LVL_PRC16_97_DEA_NAME,
	VEL_DATA_LVL_PRC16_194_DEA_NAME,
	VEL_DATA_LVL_PRC8_97_DEA_NAME,
	VEL_DATA_LVL_PRC8_194_DEA_NAME,
	VEL_DATA_LVL_CLR16_97_DEA_NAME,
	VEL_DATA_LVL_CLR16_194_DEA_NAME,
	VEL_DATA_LVL_CLR8_97_DEA_NAME,
	VEL_DATA_LVL_CLR8_194_DEA_NAME
};

Widget		Top_widget;	/* Top level widget ID.	*/
Widget		Form;		/* Parent widget to everything.	*/
Widget		Parameter_form; /* Parent widget to all category forms */
int		Change_flag    = HCI_NOT_CHANGED_FLAG;
				/* Data change flag.  Use one of the macros
				   to set a bit.  Or the macros together when
				   multiple categories are modified. */
int		Unlocked_loca = HCI_NO_FLAG; /* Treat ROC/URC LOCA the same */
Widget		Lock_widget    = (Widget) NULL;
				/* ID of Lock button. */
Widget		Save_button    = (Widget) NULL;
				/* ID of Save button. */
Widget		Undo_button    = (Widget) NULL;
				/* ID of Undo button. */
Widget		Update_button  = (Widget) NULL;
				/* ID of Update button. */
Widget		Restore_button = (Widget) NULL;
				/* ID of Restore button. */

/*	Prevent modify processing during undo and restore events */
static int processing_undo_or_restore = 0;

/*	Type of values to display (baseline or non-baseline)	*/

int	Baseline_flag = HCI_NO_FLAG;

/*	Global variables used for Cell Product Parameters menu.		*/

Widget	Cell_product_frame    = (Widget) NULL;
Widget	Cell_product_form     = (Widget) NULL;
Widget	*Cell_object          = (Widget *) NULL;
int	Cell_num_elements     = 0;
char	**Cell_names          = NULL;
char	**Cell_units          = NULL;
char	**Cell_elements       = NULL;
char	**Cell_max_elements   = NULL;
char	**Cell_min_elements   = NULL;
char	**Cell_branch_names   = NULL;
int	Cell_prod_updated     = 0;

/*	Global variables used for OHP/THP, OHA Product Parameters menu.	*/

Widget	Ohp_level_frame       = (Widget) NULL;
Widget	Ohp_level_form        = (Widget) NULL;
Widget	*Ohp_object           = (Widget *) NULL;
int	Ohp_num_elements      = 0;
char	**Ohp_elements        = NULL;
int	Ohp_thp_updated       = 0;

/*	Global variables used for STP, STA Product Parameters menu.		*/

Widget	Stp_level_frame       = (Widget) NULL;
Widget	Stp_level_form        = (Widget) NULL;
Widget	*Stp_object           = (Widget *) NULL;
int	Stp_num_elements          = 0;
char	**Stp_elements        = NULL;
char	**Stp_max_elements    = NULL;
char	**Stp_min_elements    = NULL;
int	Stp_updated           = 0;

/*	Global variables used for Velocity Data Levels menu.		*/

Widget	Vel_level_frame       = (Widget) NULL;
Widget	Vel_level_form        = (Widget) NULL;
Widget	Velocity_label        = (Widget) NULL;
Widget	Vel_table_form [NUM_VELOCITY_LEVELS] = {(Widget) NULL};
Widget	*Vel_object [NUM_VELOCITY_LEVELS] = {(Widget *) NULL};
int	Vel_level_table       = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
int	Vel_num_elements [NUM_VELOCITY_LEVELS] = {0};
char	**Vel_elements [NUM_VELOCITY_LEVELS];
char	Vel_max_elements [NUM_VELOCITY_LEVELS][STR_LEN];
int	Vel_updated           = 0;

/*	Global variables used for VAD/RCM Heights menu.			*/

Widget	Vad_object [NUM_VAD_RCM_DATA_LEVELS];
Widget	Rcm_object [NUM_VAD_RCM_DATA_LEVELS];
Widget	Vad_height_frame      = (Widget) NULL;
Widget	Vad_height_form       = (Widget) NULL;
int	Vad_num_elements      = MAX_VAD_HEIGHTS;
int	Vad_data[NUM_VAD_RCM_DATA_LEVELS] = {0};
char	Vad_elements[MAX_VAD_HEIGHTS][STR_LEN];
int	Rcm_num_elements  = MAX_RCM_HEIGHTS;
int	Rcm_data [NUM_VAD_RCM_DATA_LEVELS] = {0};
char	Rcm_elements[MAX_RCM_HEIGHTS][STR_LEN];
int	Vad_rcm_min_height    = 0;
int	Vad_rcm_updated       = 0;

int	Current_category = CELL_PRODUCT_PARAMETERS;
			/* Defined the currently active (displayed)
			   category. */
Widget	Current_button   = (Widget) NULL;
			/* ID of the currently selected category radio
			   button. */
int	Edit_category    = CELL_PRODUCT_PARAMETERS;
			/* Current category being edited. */
Widget	Edit_button      = (Widget) NULL;
			/* ID of radio button used to select current edit
			   category. */
int	Raise_flag = 0;	/* Flag set when item from combo box menu selected.
			   The timer proc then raises the Selectable
			   Product Parameters window to the top of the
			   heirarchy.  This is a fix for a problem with
			   selecting an item from a combo box menu which
			   extends outside the main window boundary. */
int	Error_flag = 0;	/* Flag set when a invalid data is entered by
			   the user. */
int	Save_flag = HCI_NO_FLAG;
int	Undo_flag = HCI_NO_FLAG;
int	Restore_flag = HCI_NO_FLAG;
int	Update_flag = HCI_NO_FLAG;
int	Close_flag = HCI_NO_FLAG;

char	Buf [512];	/* Shared buffer used to format various strings,
			   etc. */
char	Old_tbuf [256] = " "; /* Shared buffer used to format various
			   strings, etc. */

static int Screen_need_update = 0;
			/* The screen needs to be updated because an event
			   is received */

/*	Function prototypes. */

void	selectable_parameters_close (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_select (Widget w,
			XtPointer client_data, XtPointer call_data);
void	velocity_table_select (Widget w,
			XtPointer client_data, XtPointer call_data);
void	vad_rcm_height_select (Widget w,
			XtPointer client_data, XtPointer call_data);
void	rcm_editable_pup_select (Widget w,
			XtPointer client_data, XtPointer call_data);
static	void	gain_focus_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_modify (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_toggle (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_undo_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_undo ();
void	selectable_parameters_save_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	accept_selectable_parameters_save (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_save ();
void	selectable_parameters_restore_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	accept_selectable_parameters_restore (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_restore ();
void	selectable_parameters_update_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	accept_selectable_parameters_update (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_update ();
void	acknowledge_invalid_value (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_save_and_close_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_nosave_and_close_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
void	selectable_parameters_nosave (Widget w,
			XtPointer client_data, XtPointer call_data);
void	modify_widgets_for_change();
void	modify_widgets_for_no_change();

void	display_selectable_parameters (int category);

void	timer_proc ();

int	selectable_parameters_lock ();

int	read_cell_prod();
int	read_ohp_thp();
int	read_stp();
int	read_vel( );
int	read_vad_rcm();
int	save_cell_prod();
int	save_ohp_thp();
int	save_stp();
int	save_rcm();
int	save_vad();
int	save_vel();
int	verify_cell_prod_value( int indx, char *buf );
int	verify_ohp_thp_value( int indx, char *buf );
int	verify_stp_value( int indx, char *buf );
int	verify_vel_value( int indx, char *buf );

void	build_cell_product_parameters_widgets    (Widget parent);
void	build_ohp_thp_data_levels_widgets        (Widget parent);
void	build_stp_data_levels_widgets            (Widget parent);
void	build_velocity_data_levels_widgets       (Widget parent);
void	build_vad_rcm_height_levels_widgets      (Widget parent);

void	cell_prod_callback();
void	ohp_thp_callback();
void	stp_callback();
void	vad_rcm_callback();
void	vel_precip_16_97_callback();
void	vel_precip_16_194_callback();
void	vel_precip_8_97_callback();
void	vel_precip_8_194_callback();
void	vel_clear_16_97_callback();
void	vel_clear_16_194_callback();
void	vel_clear_8_97_callback();
void	vel_clear_8_194_callback();

void	*Vel_callback_table [] = {
	vel_precip_16_97_callback,
	vel_precip_16_194_callback,
	vel_precip_8_97_callback,
	vel_precip_8_194_callback,
	vel_clear_16_97_callback,
	vel_clear_16_194_callback,
	vel_clear_8_97_callback,
	vel_clear_8_194_callback
};

/************************************************************************
 *	Description: This is the main function for the Selectable	*
 *		     Product Parameters task.				*
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
	Widget	frame;
	Widget	button;
	Widget	control_rowcol;
	Widget	select_frame;
	Widget	select_rowcol;
	Widget	selectable_parameters_category;
	Widget	lock_form;
	int	n;
	Arg	arg [10];
	
/*	Initialize HCI.					*/

	HCI_init( argc, argv, HCI_SPP_TASK );

	Top_widget = HCI_get_top_widget();

/*	Build widgets.							*/

	Form   = XtVaCreateWidget ("selectable_parameters_form",
		xmFormWidgetClass,		Top_widget,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);
		
/*	If the HCI is running low bandwidth, pop up a progress meter	*
 *	window.								*/

	HCI_PM( "Initialize Task Information" );		

/*	Create the main control bar across the top of the window as	*
 *	in other HCI tasks.  The control bar contains the following	*
 *	buttons: Close, Save, Undo, Restore, Update.  A lock button	*
 *	will also be added to the right.  The Save and Undo buttons	*
 *	are sensitized only when the window is unlocked and unsaved	*
 *	edits are detected.						*/

	frame = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,	Form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	control_rowcol = XtVaCreateWidget ("control_rowcol", 
		xmRowColumnWidgetClass,	frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	button = XtVaCreateManagedWidget (" Close ",
		xmPushButtonWidgetClass,control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtraversalOn,		True,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, selectable_parameters_close, NULL);

	Save_button = XtVaCreateManagedWidget (" Save ",
		xmPushButtonWidgetClass,control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtraversalOn,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Save_button,
		XmNactivateCallback, selectable_parameters_save_callback, 
		(XtPointer) (-1));

/*	The Undo button rereads the data from the LB.  It will		*
 *	not recover data before the last Save operation.		*/

	Undo_button = XtVaCreateManagedWidget ("Undo",
		xmPushButtonWidgetClass,control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtraversalOn,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Undo_button,
		XmNactivateCallback, selectable_parameters_undo_callback, NULL);

/*	The Restore and Update buttons are only selectable if the	*
 *	window is unlocked.  The Update button is only selectable	*
 *	when there are no unsaved edits detected.			*/

	XtVaCreateManagedWidget ("            Baseline: ",
		xmLabelWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Restore_button = XtVaCreateManagedWidget ("Restore",
		xmPushButtonWidgetClass,control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtraversalOn,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Restore_button,
		XmNactivateCallback, selectable_parameters_restore_callback, NULL);

	Update_button = XtVaCreateManagedWidget ("Update",
		xmPushButtonWidgetClass,control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtraversalOn,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Update_button,
		XmNactivateCallback, selectable_parameters_update_callback, NULL);

	XtManageChild (control_rowcol);

/*	Create a new frome which will hold a set of radio buttons	*
 *	for each seletable parameter category.  It will be positioned	*
 *	immediately below the control frame.				*/

	select_frame = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,	Form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		frame,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	select_rowcol = XtVaCreateWidget ("select_rowcol", 
		xmRowColumnWidgetClass,	select_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

	XtVaCreateManagedWidget ("   Category: ",
		xmLabelWidgetClass,	select_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

/*	Create a set of radio buttons for selecting each category.	*/

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;
	XtSetArg (arg [n], XmNpacking,		XmPACK_COLUMN); n++;
	XtSetArg (arg [n], XmNnumColumns,	3); n++;

	selectable_parameters_category = XmCreateRadioBox (select_rowcol,
		"select_category", arg, n);
		
	HCI_PM( "Read Selectable Parameters Data" );		

/*	Create a button for Cell Product Parameters.		*/

	button = XtVaCreateManagedWidget ("Cell Product",
		xmToggleButtonWidgetClass,	selectable_parameters_category,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNtraversalOn,		False,
		XmNset,			True,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNvalueChangedCallback, selectable_parameters_select,
		(XtPointer) CELL_PRODUCT_PARAMETERS);

/*	Make Cell Product Parameters the default.		*/

	Edit_button    = button;
	Current_button = button;

/*	Create a button for OHP/THP, OHA Levels Product Parameters.		*/

	button = XtVaCreateManagedWidget ("OHP/THP, OHA Data Levels",
		xmToggleButtonWidgetClass,	selectable_parameters_category,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNvalueChangedCallback, selectable_parameters_select,
		(XtPointer) OHP_THP_DATA_LEVELS);

/*	Create a button for STP, STA Data Levels Product Parameters.	*/

	button = XtVaCreateManagedWidget ("STP, STA Data Levels",
		xmToggleButtonWidgetClass,	selectable_parameters_category,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNvalueChangedCallback, selectable_parameters_select,
		(XtPointer) STP_DATA_LEVELS);

/*	Create a button for VAD/RCM Heights Product Parameters.	*/

	button = XtVaCreateManagedWidget ("VAD and RCM Heights",
		xmToggleButtonWidgetClass,	selectable_parameters_category,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNvalueChangedCallback, selectable_parameters_select,
		(XtPointer) VAD_RCM_HEIGHT_SELECTIONS);

/*	Create a button for Velocity Data Levels Product Parameters.	*/

	button = XtVaCreateManagedWidget ("Velocity Data Levels",
		xmToggleButtonWidgetClass,	selectable_parameters_category,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNvalueChangedCallback, selectable_parameters_select,
		(XtPointer) VELOCITY_DATA_LEVELS);

	XtManageChild (selectable_parameters_category);
	XtManageChild (select_rowcol);

/*	Create a pixmap and drawn button for the window lock.  It is	*
 *	always to be placed in the upper right cornet of the window.	*/

	lock_form = XtVaCreateWidget ("lock_form",
		xmFormWidgetClass,	Form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Create the lock widget and pass the LOCA mask identifying
	which LOCA's apply to this window.	*/

	Lock_widget = hci_lock_widget( lock_form, selectable_parameters_lock, HCI_LOCA_URC | HCI_LOCA_ROC );

	XtManageChild (lock_form);

/*	The last thing we will do is create a form which will hold all	*
 *	of the widgets used to edit selectable parameter adaptation	*
 *	data.								*/

	Parameter_form = XtVaCreateWidget ("parameter_form",
		xmFormWidgetClass,	Form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		select_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

/*	Build a form and widgets for the cell product parameters	*
 *	selection.							*/

	build_cell_product_parameters_widgets (Parameter_form);

/*	Build a form and widgets for the OHP/THP, OHA data levels		*
 *	selection.							*/

	build_ohp_thp_data_levels_widgets (Parameter_form);

/*	Build a form and widgets for the STP, STA data levels		*
 *	selection.							*/

	build_stp_data_levels_widgets (Parameter_form);

/*	Build a form and widgets for the VAD and RCM height levels	*
 *	selection.							*/

	build_vad_rcm_height_levels_widgets (Parameter_form);

/*	Build a form and widgets for the velocity data levels		*
 *	selection.							*/

	build_velocity_data_levels_widgets (Parameter_form);
	
/*	Display the default selectable parameter category.		*/

	display_selectable_parameters (Current_category);
	
/*	Assume any error means I/O has been cancelled.			*
 *	If cancel gets better, this should only test for		*
 *	an error code of RMT_CANCELLED					*/

	if ((ORPGPAT_io_status() < 0) ||
	    (ORPGALT_io_status() < 0))
	    HCI_task_exit(HCI_EXIT_SUCCESS);

/*	Manage eveything and popup the window.				*/

	XtManageChild (Parameter_form);
	XtManageChild (Form);
	XtRealizeWidget( Top_widget );

/*	Start HCI loop.							*/

	HCI_start( timer_proc, HCI_HALF_SECOND, RESIZE_HCI );	

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
selectable_parameters_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

/*	No actual saving is done in this routine.  It is responsible	*
 *	for popping up a confirmation window first.			*/

	sprintf( buf, "Do you want to save your changes?" );
	hci_confirm_popup( Top_widget, buf, accept_selectable_parameters_save, NULL );
}

void
accept_selectable_parameters_save (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  Save_flag = HCI_YES_FLAG;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button from the Close and Save		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
selectable_parameters_save_and_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Close/save pushed");

	accept_selectable_parameters_save (w,
				(XtPointer) -1,
				(XtPointer) NULL);

	Close_flag = HCI_YES_FLAG;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Close and Save		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
selectable_parameters_nosave_and_close (
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
 *		     the "Yes" button from the Save confirmation popup	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
selectable_parameters_save ()
{
/*	Since we are saving all edits, we need to desensitize the Save	*
 *	and Undo buttons.						*/

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

/*	If low bandwidth, popup the progress meter window.		*/

	HCI_PM( "Writing Selectable Parameters information" );

/*	This section is for non-baseline data.				*/

	Baseline_flag = HCI_NO_FLAG;

/*	If edits were made to Cell Product Parameters, save them.	*/

	if (Change_flag & CEL_CHANGE_FLAG)
	{
	  /* Display a message in the Feedback line of the RPG Control	*
	   * status window.						*/
	  if( save_cell_prod() )
	    sprintf( Buf, "Unable to update Cell Product Parameters" );
	  else
	    sprintf( Buf, "Cell Product Parameters updated" );

	  HCI_display_feedback( Buf );
	}

/*	If edits were made to VAD/RCM Height Parameters, save them.	*/

	if (Change_flag & VAD_CHANGE_FLAG)
	{
	  /* Display a message in the Feedback line of the RPG Control	*
	   * status window.						*/
	  if( save_rcm() )
	    sprintf( Buf, "Unable to update RCM Heights" );
	  else
	    sprintf( Buf, "RCM Heights updated" );
	  if( save_vad() )
	    strcat( Buf, " - Unable to update VAD Heights" );
	  else
	    strcat( Buf, " - VAD Heights updated" );

	  HCI_display_feedback( Buf );
	}

/*	If edits were made to OHP/THP, OHA Levels Parameters, save them.	*/

	if (Change_flag & OHP_CHANGE_FLAG)
	{
	  /* Display a message in the Feedback line of the RPG Control	*
	   * status window.						*/
	  if( save_ohp_thp() )
	    sprintf( Buf, "Unable to update OHP/THP, OHA Levels" );
	  else
	    sprintf( Buf, "OHP/THP, OHA Levels updated" );

	  HCI_display_feedback( Buf );
	}

/*	If edits were made to STP, STA Levels Parameters, save them.	*/

	if (Change_flag & STP_CHANGE_FLAG)
	{
	  /* Display a message in the Feedback line of the RPG Control	*
	   * status window.						*/
	  if( save_stp() )
	    sprintf( Buf, "Unable to update STP, STA Data Levels" );
	  else
	    sprintf( Buf, "STP, STA Data Levels updated" );

	  HCI_display_feedback( Buf );
	}

/*	If edits were made to Velocity Levels Parameters, save them.	*/

	if (Change_flag & VEL_CHANGE_FLAG)
	{
	  /* Display a message in the Feedback line of the RPG Control	*
	   * status window.						*/
	  if( save_vel() )
	    sprintf( Buf, "Unable to update Velocity Data Levels" );
	  else
	    sprintf( Buf, "Velocity Data Levels updated" );

	  HCI_display_feedback( Buf );
	}

/*	Reset the change flag.					*/

	Change_flag = HCI_NOT_CHANGED_FLAG;

/*	Since we are saving all edits, we need to sensitize the Update	*
 *	button.								*/

	if (Unlocked_loca == HCI_NO_FLAG) {

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
selectable_parameters_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

/*	If no edits are detected, exit the task.		*/

	if (Change_flag == HCI_NOT_CHANGED_FLAG) {

	    HCI_task_exit (HCI_EXIT_SUCCESS);

/*	Edits were detected so prompt the user about saving them first.	*/

	} else {

	    sprintf( buf, "You modified selectable parameters but\ndid not save your changes.  Do you\nwant to save your changes?" );
	    hci_confirm_popup( Top_widget, buf, selectable_parameters_save_and_close, selectable_parameters_nosave_and_close );
	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a "Category" radio button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - category ID				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
selectable_parameters_select (
Widget		 w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	If the category button state is set, then unmanage the table	*
 *	corresponding to the previous set category and manage the table	*
 *	corresponding to the new selected category.			*/

	if (state->set) {

	    Current_button = w;

	    switch (Current_category) {

		case CELL_PRODUCT_PARAMETERS :

		    XtUnmanageChild (Cell_product_frame);
		    break;

		case OHP_THP_DATA_LEVELS :

		    XtUnmanageChild (Ohp_level_frame);
		    break;

		case STP_DATA_LEVELS :

		    XtUnmanageChild (Stp_level_frame);
		    break;

		case VELOCITY_DATA_LEVELS :

		    XtUnmanageChild (Vel_level_frame);
		    break;

		case VAD_RCM_HEIGHT_SELECTIONS :

		    XtUnmanageChild (Vad_height_frame);
		    break;

	    }

/*	    If, in the process of unmanaging the old table an error was	*
 *	    detected in validating a table edit, do not change the	*
 *	    category so the user will have a chance to correct it.	*/

	    if (!Error_flag) {

		Current_category = (int) client_data;

	    }

	    switch (Current_category) {

		case CELL_PRODUCT_PARAMETERS :

		    XtManageChild (Cell_product_frame);
		    break;

		case OHP_THP_DATA_LEVELS :

		    XtManageChild (Ohp_level_frame);
		    break;

		case STP_DATA_LEVELS :

		    XtManageChild (Stp_level_frame);
		    break;

		case VELOCITY_DATA_LEVELS :

		    XtManageChild (Vel_level_frame);
		    break;

		case VAD_RCM_HEIGHT_SELECTIONS :

		    XtManageChild (Vad_height_frame);
		    break;

	    }

	    display_selectable_parameters ((int) client_data);
	}
}

/************************************************************************
 *	Description: This function is is the timer procedure for the	*
 *		     Selectable Product Parameters task.		*
 *									*
 *	Input:  w - timer parent widget ID				*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
	int error_flag = 0;

	if( Save_flag == HCI_YES_FLAG )
	{
	  Save_flag = HCI_NO_FLAG;
	  selectable_parameters_save();
	}

	if( Undo_flag == HCI_YES_FLAG )
	{
	  Undo_flag = HCI_NO_FLAG;
	  selectable_parameters_undo();
	}

	if( Restore_flag == HCI_YES_FLAG )
	{
	  Restore_flag = HCI_NO_FLAG;
	  selectable_parameters_restore();
	}

	if( Update_flag == HCI_YES_FLAG )
	{
	  Update_flag = HCI_NO_FLAG;
	  selectable_parameters_update();
	}

	if( Close_flag == HCI_YES_FLAG )
	{
	  HCI_task_exit( HCI_EXIT_SUCCESS );
	}

/*	If adaptation data has been changed, update values and GUI.	*/

	strcpy( Buf, "" );

	if( Cell_prod_updated )
        {
          if( read_cell_prod() )
          {
	    error_flag = 1;
            strcpy( Buf, "Error reading updated Cell Product Data\n" );
          }
          else
          {
            Screen_need_update = 1;
          }
        }
	if( Ohp_thp_updated )
        {
          if( read_ohp_thp() )
          {
	    error_flag = 1;
            strcpy( Buf, "Error reading updated OHP/THP, OHA Data Levels\n" );
          }
          else
          {
            Screen_need_update = 1;
          }
        }
	if( Stp_updated )
        {
          if( read_stp() )
          {
	    error_flag = 1;
            strcpy( Buf, "Error reading updated STP, STA Data Levels\n" );
          }
          else
          {
            Screen_need_update = 1;
          }
        }
	if( Vad_rcm_updated )
        {
          if( read_vad_rcm() )
          {
	    error_flag = 1;
            strcpy( Buf, "Error reading updated VAD/RCM Heights\n" );
          }
          else
          {
            Screen_need_update = 1;
          }
        }
	if( Vel_updated )
        {
          if( read_vel() )
          {
	    error_flag = 1;
            strcpy( Buf, "Error reading updated Velocity Data Levels\n" );
          }
          else
          {
            Screen_need_update = 1;
          }
        }

	if( error_flag )
	{
	  strcat( Buf, "\nUnable to display updated values" );
	}

/*	If the raise flag was set, then raise the window to the top	*
 *	of the window heirarchy.  This was added to correct a problem	*
 *	inherent with selecting an item from a combobox menu when it	*
 *	extends beyond the border of the parent window.  The focus	*
 *	would go to the window beneath the menu.			*/

	if (Raise_flag) {

	    XRaiseWindow (HCI_get_display(), HCI_get_window());
	    Raise_flag = 0;

	}

	if (Screen_need_update) {
	    Screen_need_update = 0;
	    display_selectable_parameters (Current_category);
	}
}

/************************************************************************
 *	Description: This function is used to display selectable	*
 *		     product parameter information for the specified	*
 *		     category.  It should be called whenever a category	*
 *		     is changed or its data updated.			*
 *									*
 *	Input:  category - category ID					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_selectable_parameters (
int	category
)
{
	int	foreground;
	int	background;
	Boolean	edit_flag;
	Boolean	set_flag;
	int	i;
	int	table;

/*	If the URC or ROC LOCA has been unlocked, then set the edit	*
 *	flag and color fields.						*/

	if (Unlocked_loca == HCI_YES_FLAG) {

	    foreground = hci_get_read_color (EDIT_FOREGROUND);
	    background = hci_get_read_color (EDIT_BACKGROUND);
	    edit_flag  = True;
	
	} else {

/*	    If the URC or ROC LOCA has been selected by the user	*
 *	    in the password dialog, set the color fileds so that	*
 *	    foreground color indicates the field can be edited for	*
 *	    the selected level of change authority.			*/

	    if ( hci_lock_URC_selected() || hci_lock_ROC_selected() ) {

		foreground = hci_get_read_color (LOCA_FOREGROUND);
		background = hci_get_read_color (BACKGROUND_COLOR1);

	    } else {

/*	    The window is not unlocked and the level of change		*
 *	    authority flag is not set so set colors to non-edit		*
 *	    state.							*/

		foreground = hci_get_read_color (TEXT_FOREGROUND);
		background = hci_get_read_color (BACKGROUND_COLOR1);

	    }

/*	    Set the edit flag to false so we won't allow edits.		*/

	    edit_flag = False;

	}

/*	For the specified category, update each widget sensitivity and	*
 *	color.								*/

	switch (category) {

	    case VAD_RCM_HEIGHT_SELECTIONS :

/*		VAD/RCM Height Parameters require URC level password	*
 *		for editing.						*/

		for (i=Vad_rcm_min_height-1;i<NUM_VAD_RCM_DATA_LEVELS;i++) {

		    if (Vad_data [i] != 0) {

			set_flag = True;

		    } else {

			set_flag = False;

		    }

		    XtVaSetValues (Vad_object [i],
			XmNsensitive,		edit_flag,
			XmNbackground,		background,
			NULL);

		    XmToggleButtonSetState (Vad_object [i], set_flag, False);

		    if (Rcm_data [i] != 0) {

			set_flag = True;

		    } else {

			set_flag = False;

		    }

		    XtVaSetValues (Rcm_object [i],
			XmNsensitive,		edit_flag,
			XmNbackground,		background,
			NULL);

		    XmToggleButtonSetState (Rcm_object [i], set_flag, False);

		}

		break;

	    case STP_DATA_LEVELS :

/*		STP, STA Data Level Parameters require URC level password	*
 *		for editing.						*/

		for (i=2;i<Stp_num_elements;i++)
		{
		  XtVaSetValues (Stp_object [i],
			XmNforeground,	foreground,
			XmNbackground,	background,
			XmNeditable,		edit_flag,
			XmNtraversalOn,		edit_flag,
			NULL);

		  sprintf (Buf,"%s", Stp_elements[i]);
		  XmTextSetString (Stp_object [i], Buf);
		}

		break;

	    case VELOCITY_DATA_LEVELS :

/*		Velocity Data Level Parameters require URC level	*
 *		password for editing.					*/

/*		For each velocity table, update each widget property.	*/

		for (table =  VELOCITY_LEVEL_TABLE_PRECIP_16_05;
		     table < NUM_VELOCITY_LEVELS;
		     table++)
		{

		  for (i=Vel_num_elements[table]/2+1;i<Vel_num_elements [table]-1;i++)
		  {

		    XtVaSetValues (Vel_object [table][i],
				XmNforeground,	foreground,
				XmNbackground,	background,
				XmNeditable,		edit_flag,
				XmNtraversalOn,		edit_flag,
				NULL);

		  }

		  for (i=0;i<Vel_num_elements[table];i++)
		  {
/*		    Add a space to the end of the string.  For	*
 *		    some reason, XmTextSetString() cuts off the *
 *		    last character in some cases.		*/

		    sprintf( Buf, "%s ", Vel_elements[table][i]);
		    XmTextSetString (Vel_object [table][i], Buf);
		  }
		}

		XtManageChild (Vel_level_form);

		break;

	    case OHP_THP_DATA_LEVELS :

/*		OHP/THP, OHA Data Level Parameters require URC level		*
 *		password for editing.					*/

		for (i=2;i<Ohp_num_elements;i++)
		{
		  XtVaSetValues (Ohp_object [i],
			XmNforeground,	foreground,
			XmNbackground,	background,
			XmNeditable,		edit_flag,
			XmNtraversalOn,		edit_flag,
			NULL);

		  sprintf (Buf,"%s", Ohp_elements[i]);
		  XmTextSetString (Ohp_object [i], Buf);
		}

		break;

	    case CELL_PRODUCT_PARAMETERS :

/*		Cell Product Parameters require URC level		*
 *		password for editing.					*/

		for (i=0;i<Cell_num_elements;i++)
		{

		  XtVaSetValues (Cell_object [i],
			XmNforeground,	foreground,
			XmNbackground,	background,
			XmNeditable,		edit_flag,
			XmNtraversalOn,		edit_flag,
			NULL);

		  sprintf (Buf,"%s", Cell_elements[i]);
		  XmTextSetString (Cell_object [i], Buf);
		}

		break;

	}

/*	If the window unlocked for editing and if there are any edits	*
 *	detected, sensitize the Save and Undo buttons.			*/

	if ((Change_flag != HCI_NOT_CHANGED_FLAG) &&
	    (edit_flag == True))
	{
	  modify_widgets_for_change();
	}
	else
	{
/*	The window is locked or no edits are detected so desensitize	*
 *	the Save and Undo buttons.					*/

	    XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);
	}
}

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     selectable product parameter text widgets is	*
 *		     changed or loses focus.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - category ID				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
selectable_parameters_modify (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtPointer	data;
  char		*str;
  char		tbuf [STR_LEN*2];
  int		ival;
  int		i;

  Error_flag = 0;

  XtVaGetValues (w, XmNuserData, &data, NULL );

  str = XmTextGetString (w);
  strcpy (tbuf,str);
  XtFree (str);

  /* If we haven't changed anything, do not try to set the data	*
   * object or the change flag will get set.			*/

  if (!strcmp (tbuf,Old_tbuf))
  {
    return;
  }

  /* Ignore modify events during an undo or a restore from baseline */

  if (processing_undo_or_restore)
  {
     strcpy (Old_tbuf,tbuf);
     return;
  }

  switch ((int) client_data)
  {
    case CELL_PRODUCT_PARAMETERS :

      if( verify_cell_prod_value( (int)data, tbuf ) )
      {
        hci_error_popup( Top_widget, "Invalid Cell Product Parameter value", NULL );
        HCI_LE_error("Invalid Cell Product Parameter value" );
        Error_flag = 1;
      }
      else
      {
        strcpy (Old_tbuf,tbuf);
        sprintf( Cell_elements[ (int)data ], tbuf );

        if (Change_flag == HCI_NOT_CHANGED_FLAG)
        {
          modify_widgets_for_change();
        }

        Change_flag = Change_flag | CEL_CHANGE_FLAG;
      }

      break;

    case OHP_THP_DATA_LEVELS :

      if( verify_ohp_thp_value( (int)data, tbuf ) )
      {
        hci_error_popup( Top_widget, "Invalid OHP/THP, OHA Parameter value", NULL );
        HCI_LE_error("Invalid OHP/THP, OHA Parameter value" );
        Error_flag = 1;
      }
      else
      {
        strcpy (Old_tbuf,tbuf);
        sprintf( Ohp_elements[ (int)data ], tbuf );

        if (Change_flag == HCI_NOT_CHANGED_FLAG)
        {
          modify_widgets_for_change();
        }

        Change_flag = Change_flag | OHP_CHANGE_FLAG;
      }

      break;

    case STP_DATA_LEVELS :

      if( verify_stp_value( (int)data, tbuf ) )
      {
        hci_error_popup( Top_widget, "Invalid STP, STA Parameter value", NULL );
        HCI_LE_error("Invalid STP, STA Parameter value" );
        Error_flag = 1;
      }
      else
      {
        strcpy (Old_tbuf,tbuf);
        sprintf( Stp_elements[ (int)data ], tbuf );

        if (Change_flag == HCI_NOT_CHANGED_FLAG)
        {
          modify_widgets_for_change();
        }

        Change_flag = Change_flag | STP_CHANGE_FLAG;
      }

      break;

    case VELOCITY_DATA_LEVELS :

      if( verify_vel_value( (int)data, tbuf ) )
      {
        hci_error_popup( Top_widget, "Invalid Velocity Parameter value", NULL );
        HCI_LE_error("Invalid Velocity Parameter value" );
        Error_flag = 1;
      }
      else
      {
        strcpy (Old_tbuf,tbuf);
        sprintf( Vel_elements[Vel_level_table][ (int)data ], tbuf );

        /* Since the velocity tables are symmetric we only allow*
         * the user to edit the positive half.  However, when a	*
         * change is made, it must be reflected in the negative	*
         * half at the same time.				*/

        i = Vel_num_elements [Vel_level_table] - ((int) data) - 1;
        sscanf (tbuf,"%d",&ival);
        sprintf (tbuf,"-%d",ival);

        sprintf( Vel_elements[Vel_level_table][ i ], tbuf );

        if (Change_flag == HCI_NOT_CHANGED_FLAG)
        {
          modify_widgets_for_change();
        }

        Change_flag = Change_flag | VEL_CHANGE_FLAG;

        break;
      }
  }

  /* If an error was detected during validation the set the active	*
   * category back to the one containing the error.			*/

  if (Error_flag)
  {
    XmToggleButtonSetState (Edit_button, True, True);
  }
  else
  {
    strcpy (Old_tbuf,tbuf);
    Edit_button   = Current_button;
    Edit_category = Current_category;
  }

  /* Refresh the data displayed for the current edit category.	*/

  display_selectable_parameters (Edit_category);
}


/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Continue" button from the warning popup	*
 *		     window after invalid data was entered.		*
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
	Error_flag = 0;
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
selectable_parameters_undo_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Undo_flag = HCI_YES_FLAG;
}

void selectable_parameters_undo ()
{
	int error_flag = 0;

/*	If low bandwidth, popup the progress meter window.	*/

	HCI_PM( "Restoring selectable parameters information" );

	processing_undo_or_restore = 1;

/*	This section is for non-baseline data.				*/

	Baseline_flag = HCI_NO_FLAG;

/*	Check the change flag bit for each category.  If is set, then	*
 *	edits were made which should be undone.				*/

	strcpy( Buf, "" );

	if (Change_flag & CEL_CHANGE_FLAG)
	{
	  if( read_cell_prod() )
	  {
	    error_flag = 1;
	    strcat( Buf, "Error reading Cell Product Data\n" );
	  }
	}

	if (Change_flag & VAD_CHANGE_FLAG)
	{
	  if( read_vad_rcm() )
	  {
	    error_flag = 1;
	    strcat( Buf, "Error reading VAD/RCM Heights\n" );
	  }
	}

	if (Change_flag & OHP_CHANGE_FLAG)
	{
	  if( read_ohp_thp() )
	  {
	    error_flag = 1;
	    strcat( Buf, "Error reading OHP/THP, OHA Data Levels\n" );
	  }
	}

	if (Change_flag & STP_CHANGE_FLAG)
	{
	  if( read_stp() )
	  {
	    error_flag = 1;
	    strcat( Buf, "Error reading STP, STA Data Levels\n" );
	  }
	}

	if (Change_flag & VEL_CHANGE_FLAG)
	{
	  if( read_vel() )
	  {
	    error_flag = 1;
	    strcat( Buf, "Error reading Velocity Data Levels\n" );
	  }
	}

	if( error_flag )
	{
	  strcat( Buf, "\nUnable to undo changes" );
	  hci_error_popup( Top_widget, Buf, NULL );
	}

	processing_undo_or_restore = 0;

/*	Refresh the current category data display.		*/

	display_selectable_parameters (Current_category);

/*	Desensitize the Save and Undo buttons since there are	*
 *	no unsaved edits.					*/

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

	if (Unlocked_loca == HCI_YES_FLAG) {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	True,
		NULL);

	}

	Change_flag = HCI_NOT_CHANGED_FLAG;

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the lock button and selects a LOCA or enters a	*
 *		     password.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
selectable_parameters_lock ()
{
  char buf[HCI_BUF_128];

  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_unlocked() )
  {
    /*  If the user entered a valid URC or ROC level password,	*
     *  then check to see if anyone else is editing selectable	*
     *  product parameters data.  If so, popup a warning popup	*
     *  so they can decide to cancel editing or proceed.		*/

    if( hci_lock_URC_unlocked() || hci_lock_ROC_unlocked() )
    {
      Unlocked_loca = HCI_YES_FLAG;
      if(DEAU_lock_de(CELL_PROD_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VAD_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(OHP_THP_DATA_LVLS_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(STP_DATA_LVLS_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_PRC16_97_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_PRC16_194_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_PRC8_97_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_PRC8_194_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_CLR16_97_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_CLR16_194_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_CLR8_97_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK) ||
         DEAU_lock_de(VEL_DATA_LVL_CLR8_194_DEA_NAME,LB_TEST_EXCLUSIVE_LOCK))
      {
        sprintf( buf, "Another user is currently editing selectable\nproduct parameters data. Any changes may be\noverwritten by the other user." );
	hci_warning_popup( Top_widget, buf, NULL );
      }

      if( DEAU_lock_de(CELL_PROD_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock CELL PROD LB" );
      }
      if( DEAU_lock_de(VAD_RCM_HEIGHTS_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VAD/RCM LB" );
      }
      if( DEAU_lock_de(OHP_THP_DATA_LVLS_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock OHP/THP, OHA LB" );
      }
      if( DEAU_lock_de(STP_DATA_LVLS_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock STP, STA LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_PRC16_97_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL PRCP 16/97 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_PRC16_194_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL PRCP 16/194 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_PRC8_97_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL PRCP 8/97 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_PRC8_194_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL PRCP 8/194 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_CLR16_97_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL CLR 16/97 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_CLR16_194_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL CLR 16/194 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_CLR8_97_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL CLR 8/97 LB" );
      }
      if( DEAU_lock_de(VEL_DATA_LVL_CLR8_194_DEA_NAME,LB_SHARED_LOCK) != LB_SUCCESS )
      {
        HCI_LE_error("Unable to lock VEL CLR 8/194 LB" );
      }

      /* Desensitize the Save and Undo buttons and sensitize the	*
       * Restore and Update buttons.				*/

      modify_widgets_for_no_change();

      /* Refresh the current category data properties.		*/

      display_selectable_parameters (Current_category);
    }
  }
  else if( hci_lock_close() && Unlocked_loca == HCI_YES_FLAG )
  {
    Unlocked_loca = HCI_NO_FLAG;

    if( Change_flag )
    {
      /* Prompt the user to save unsaved edits. */
      sprintf( buf, "You modified the selectable parameters but\ndid not save your changes.  Do you\nwant to save your changes?" );
      hci_confirm_popup( Top_widget, buf, accept_selectable_parameters_save, selectable_parameters_undo );
    }
    else
    {
      XtVaSetValues( Restore_button, XmNsensitive, False, NULL );
      XtVaSetValues( Update_button, XmNsensitive, False, NULL );
      XtVaSetValues( Save_button, XmNsensitive, False, NULL );
      XtVaSetValues( Undo_button, XmNsensitive, False, NULL );
    }

    /* We are locking the window from an unlock state so
       cancel all of the edit locks. Don't worry about
       return codes, since there's nothing we can do. */

    if( DEAU_lock_de(CELL_PROD_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock CELL PROD LB" );
    }
    if( DEAU_lock_de(VAD_RCM_HEIGHTS_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VAD/RCM LB" );
    }
    if( DEAU_lock_de(OHP_THP_DATA_LVLS_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock OHP/THP, OHA LB" );
    }
    if( DEAU_lock_de(STP_DATA_LVLS_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock STP, STA LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_PRC16_97_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL PRCP 16/97 LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_PRC16_194_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL PRCP 16/194 LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_PRC8_97_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL PRCP 8/97 LB" );
      }
    if( DEAU_lock_de(VEL_DATA_LVL_PRC8_194_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL PRCP 8/194 LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_CLR16_97_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL CLR 16/97 LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_CLR16_194_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL CLR 16/194 LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_CLR8_97_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
      HCI_LE_error("Unable to unlock VEL CLR 8/97 LB" );
    }
    if( DEAU_lock_de(VEL_DATA_LVL_CLR8_194_DEA_NAME,LB_UNLOCK) != LB_SUCCESS )
    {
     HCI_LE_error("Unable to unlock VEL CLR 8/194 LB" );
    }
  }

  /* Refresh the current category data properties.		*/

  display_selectable_parameters( Current_category );

  return HCI_LOCK_PROCEED;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button from the Save confirmation popup
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
selectable_parameters_nosave (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
/*	Refresh the current category data properties.		*/

	display_selectable_parameters (Current_category);
}

/************************************************************************
 *	Description: This function creates the Cell Product		*
 *		     Parameters widgets.				*
 *									*
 *	Input:  parent - widget ID of parent managing this form		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
build_cell_product_parameters_widgets (
Widget	parent
)
{
	Widget	rowcol;
	Widget	text;
	Widget	label;
	int	i;
	int	ret;
	char	*branch_names = NULL;

/*	Register for any updates to this object class	*/

	ret = DEAU_UN_register( CELL_PROD_DEA_NAME, cell_prod_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("CELL_PROD DEAU_UN_register failed: %d", ret );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}
	 
/*	Change the label of the frame container widget to indicate	*
 *	the type of data contained.					*/

	Cell_product_frame = XtVaCreateManagedWidget ("cell_product_frame",
		xmFrameWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Cell Product Parameters",
		xmLabelWidgetClass,	Cell_product_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create the form for the Cell Product Parameters item.	*/

	Cell_product_form = XtVaCreateWidget ("Cell_product_form",
		xmFormWidgetClass,	Cell_product_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Display a label across the top of the form indicating which	*
 *	data set is currently being displayed.				*/

	rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Cell_product_form,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	text = XtVaCreateManagedWidget ("Parameter Name",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		48,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Parameter Name");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Minimum",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Minimum");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Maximum",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Maximum");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Current",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Current");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Units",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		12,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Units");
	XmTextSetString (text, Buf);

	XtManageChild (rowcol);

	Cell_num_elements = DEAU_get_branch_names(CELL_PROD_DEA_NAME,&branch_names);

	if (Cell_num_elements > 0)
	{
	  Cell_object = (Widget *) calloc (sizeof( Widget ),Cell_num_elements);
	  if( Cell_object == NULL )
	  {
	    HCI_LE_error("Cell_object calloc failed for %d bytes", sizeof( Widget )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Cell_elements = calloc( sizeof( char * ), Cell_num_elements );
	  if( Cell_elements == NULL )
	  {
	    HCI_LE_error("Cell_elements calloc failed for %d bytes", sizeof( char * )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Cell_names = calloc( sizeof( char * ), Cell_num_elements );
	  if( Cell_names == NULL )
	  {
	    HCI_LE_error("Cell_names calloc failed for %d bytes", sizeof( char * )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Cell_units = calloc( sizeof( char * ), Cell_num_elements );
	  if( Cell_units == NULL )
	  {
	    HCI_LE_error("Cell_units calloc failed for %d bytes", sizeof( char * )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Cell_max_elements = calloc( sizeof( char * ), Cell_num_elements );
	  if( Cell_max_elements == NULL )
	  {
	    HCI_LE_error("Cell_max_elements calloc failed for %d bytes", sizeof( char * )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Cell_min_elements = calloc( sizeof( char * ), Cell_num_elements );
	  if( Cell_min_elements == NULL )
	  {
	    HCI_LE_error("Cell_min_elements calloc failed for %d bytes", sizeof( char * )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Cell_branch_names = calloc( sizeof( char * ), Cell_num_elements );
	  if( Cell_branch_names == NULL )
	  {
	    HCI_LE_error("Cell_branch_names calloc failed for %d bytes", sizeof( char * )*Cell_num_elements );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  for( i = 0; i < Cell_num_elements; i++ )
	  {
	    Cell_elements[ i ] = calloc( sizeof( char ), STR_LEN );
	    if( Cell_elements[ i ] == NULL )
	    {
	      HCI_LE_error("Cell_elements[%d] calloc failed for %d bytes",i, sizeof( char )*STR_LEN );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	    Cell_names[ i ] = calloc( sizeof( char ), STR_LEN*4 );
	    if( Cell_names[ i ] == NULL )
	    {
	      HCI_LE_error("Cell_names[%d] calloc failed for %d bytes",i, sizeof( char )*STR_LEN );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	    Cell_units[ i ] = calloc( sizeof( char ), STR_LEN );
	    if( Cell_units[ i ] == NULL )
	    {
	      HCI_LE_error("Cell_units[%d] calloc failed for %d bytes",i, sizeof( char )*STR_LEN );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	    Cell_max_elements[ i ] = calloc( sizeof( char * ), STR_LEN );
	    if( Cell_max_elements[ i ] == NULL )
	    {
	      HCI_LE_error("Cell_max_elements[%d] calloc failed for %d bytes",i, sizeof( char * )*STR_LEN );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	    Cell_min_elements[ i ] = calloc( sizeof( char * ), STR_LEN );
	    if( Cell_min_elements[ i ] == NULL )
	    {
	      HCI_LE_error("Cell_min_elements[%d] calloc failed for %d bytes",i, sizeof( char * )*STR_LEN );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	    Cell_branch_names[ i ] = calloc( sizeof( char * ), STR_LEN );
	    if( Cell_branch_names[ i ] == NULL )
	    {
	      HCI_LE_error("Cell_branch_names[%d] calloc failed for %d bytes",i, sizeof( char * )*STR_LEN );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	    strcpy( Cell_branch_names[ i ], branch_names );
	    branch_names += strlen( branch_names ) + 1;
	  }
	}
	else
	{
	  HCI_LE_error("Failed getting CELL PROD DEAU branches: %d", Cell_num_elements );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	if( read_cell_prod() )
	{
	  HCI_LE_error("Error reading Cell Prod Data" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	for (i=0;i<Cell_num_elements;i++) {

	    rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Cell_product_form,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	    text = XtVaCreateManagedWidget ("cell_product",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		48,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%s", Cell_names[i]);
	    XmTextSetString (text, Buf);

	    text = XtVaCreateManagedWidget ("cell_min",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%s", Cell_min_elements[i]);
	    XmTextSetString (text, Buf);

	    text = XtVaCreateManagedWidget ("cell_max",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%s", Cell_max_elements[i]);
	    XmTextSetString (text, Buf);

	    Cell_object [i] = XtVaCreateManagedWidget ("cell_object",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		10,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNuserData,		(XtPointer) i,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf, Cell_elements[i]);
	    XmTextSetString (Cell_object [i], Buf);

	    XtAddCallback (Cell_object [i],
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);
	    XtAddCallback (Cell_object [i],
		XmNfocusCallback, gain_focus_callback,
		(XtPointer) NULL);
	    XtAddCallback (Cell_object [i],
		XmNlosingFocusCallback, selectable_parameters_modify,
		(XtPointer) CELL_PRODUCT_PARAMETERS);
	    XtAddCallback (Cell_object [i],
		XmNactivateCallback, selectable_parameters_modify,
		(XtPointer) CELL_PRODUCT_PARAMETERS);

	    text = XtVaCreateManagedWidget ("cell_units",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		12,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%s", Cell_units[i]);
	    XmTextSetString (text, Buf);

	    XtManageChild (rowcol);
	}

	XtManageChild (Cell_product_form);

}

/************************************************************************
 *	Description: This function creates the OHP/THP, OHA Data Level	*
 *		     Parameters widgets.				*
 *									*
 *	Input:  parent - widget ID of parent managing this form		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
build_ohp_thp_data_levels_widgets (
Widget	parent
)
{
	Widget	rowcol;
	Widget	text;
	Widget	label;
	Widget	ohp_form;
	char	temp_name[STR_LEN*4];
	int	i;
	int	ret;

/*	Register for any updates to this object class	*/

	ret = DEAU_UN_register( OHP_THP_DATA_LVLS_DEA_NAME, ohp_thp_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("OHP_THP, OHA DEAU_UN_register failed: %d", ret );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Change the label of the frame container widget to indicate	*
 *	the type of data contained.					*/

	Ohp_level_frame = XtVaCreateManagedWidget ("Ohp_level_frame",
		xmFrameWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Display a label across the top of the form indicating which	*
 *	data set is currently being displayed.				*/

	label = XtVaCreateManagedWidget ("OHP/THP, OHA Data Levels",
		xmLabelWidgetClass,	Ohp_level_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create the form for the OHP/THP, OHA Data Levels item.	*/

	Ohp_level_form = XtVaCreateWidget ("Ohp_level_form",
		xmFormWidgetClass,	Ohp_level_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	ohp_form = XtVaCreateWidget ("ohp_form",
		xmFormWidgetClass,	Ohp_level_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNverticalSpacing,	1,
		NULL);

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("---------INSTRUCTIONS---------",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("Permissible value range is from",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("0.0 to 12.7 inches in multiples",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("of 0.05.  The value entered     ",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("represents the minimum value   ",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("of the data level.             ",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("                               ",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("NOTE: These thresholds affect both  ",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("      legacy PPS and dual-pol QPE products.",
		xmLabelWidgetClass,	ohp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (ohp_form);

	rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Ohp_level_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			ohp_form,
		NULL);

	label = XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	text = XtVaCreateManagedWidget ("OHP, OHA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		5,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Code");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		STR_LEN,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Current (inches)");
	XmTextSetString (text, Buf);

	label = XtVaCreateManagedWidget ("  Spacer  ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	text = XtVaCreateManagedWidget ("OHP, OHA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		5,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Code");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		STR_LEN,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Current (inches)");
	XmTextSetString (text, Buf);

	XtManageChild (rowcol);

	sprintf( temp_name, "%s.%s", OHP_THP_DATA_LVLS_DEA_NAME, "code" );
	Ohp_num_elements = DEAU_get_number_of_values( temp_name );

	if (Ohp_num_elements > 0)
	{
	  Ohp_object = (Widget *) calloc (sizeof( Widget ), Ohp_num_elements);
	  if( Ohp_object == NULL )
	  {
	    HCI_LE_error("Ohp_object calloc failed for %d bytes", sizeof( Widget )*Ohp_num_elements );
	      HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Ohp_elements = calloc( sizeof( char * ), Ohp_num_elements );
	  if( Ohp_elements == NULL )
	  {
	    HCI_LE_error("Ohp_elements calloc failed for %d bytes", sizeof( char * )*Ohp_num_elements );
	      HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  for( i = 0; i < Ohp_num_elements; i++ )
	  {
	    Ohp_elements[ i ] = calloc( sizeof( char ), STR_LEN );
	    if( Ohp_elements[ i ] == NULL )
	    {
	      HCI_LE_error("Ohp_elements[%d] calloc failed for %d bytes", i, sizeof( char )*STR_LEN );
	        HCI_task_exit( HCI_EXIT_FAIL );
	    }
	  }
	}
	else
	{
	  HCI_LE_error("Failed getting OHP/THP, OHA number of values" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	if( read_ohp_thp() )
	{
	  HCI_LE_error("Error reading OHP/THP, OHA Data Levels" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	for (i=0;i<Ohp_num_elements/2;i++) {

	    rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Ohp_level_form,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			rowcol,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			ohp_form,
		NULL);

	    label = XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    text = XtVaCreateManagedWidget ("OHP, OHA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		2,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%2d ", i+1);
	    XmTextSetString (text, Buf);

	    if (i == 0) {

		label = XtVaCreateManagedWidget ("Spacer",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

	    } else if (i == 1) {

		label = XtVaCreateManagedWidget ("  >   ",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

	    } else {

		label = XtVaCreateManagedWidget ("  >=  ",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

	    }

	    Ohp_object [i] = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		7,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		(XtPointer) i,
		NULL);

	      sprintf (Buf,"%s", Ohp_elements[i]);
	      XmTextSetString (Ohp_object [i], Buf);

	    if (i>1) {

		XtAddCallback (Ohp_object [i],
			XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
			(XtPointer) 5);
		XtAddCallback (Ohp_object [i],
			XmNfocusCallback, gain_focus_callback,
			(XtPointer) NULL);
		XtAddCallback (Ohp_object [i],
			XmNlosingFocusCallback, selectable_parameters_modify,
			(XtPointer) OHP_THP_DATA_LEVELS);
		XtAddCallback (Ohp_object [i],
			XmNactivateCallback, selectable_parameters_modify,
			(XtPointer) OHP_THP_DATA_LEVELS);

	    }

	    label = XtVaCreateManagedWidget ("    Spacer      ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    text = XtVaCreateManagedWidget ("OHP, OHA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		2,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%2d ", (i + Ohp_num_elements/2 + 1));
	    XmTextSetString (text, Buf);

	    label = XtVaCreateManagedWidget ("  >=  ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    Ohp_object [i+Ohp_num_elements/2] = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		7,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		(XtPointer) (i+Ohp_num_elements/2),
		NULL);

	    sprintf (Buf,"%s", Ohp_elements[i+Ohp_num_elements/2]);
            XmTextSetString (Ohp_object [i+Ohp_num_elements/2], Buf);

	    XtAddCallback (Ohp_object [i+Ohp_num_elements/2],
		XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
		(XtPointer) 5);
	    XtAddCallback (Ohp_object [i+Ohp_num_elements/2],
		XmNfocusCallback, gain_focus_callback,
		(XtPointer) NULL);
	    XtAddCallback (Ohp_object [i+Ohp_num_elements/2],
		XmNlosingFocusCallback, selectable_parameters_modify,
		(XtPointer) OHP_THP_DATA_LEVELS);
	    XtAddCallback (Ohp_object [i+Ohp_num_elements/2],
		XmNactivateCallback, selectable_parameters_modify,
		(XtPointer) OHP_THP_DATA_LEVELS);

	    XtManageChild (rowcol);

	}

	XtManageChild (Ohp_level_form);
	XtUnmanageChild (Ohp_level_frame);
}

/************************************************************************
 *	Description: This function creates the STP, STA Data Level	*
 *		     Parameters widgets.				*
 *									*
 *	Input:  parent - widget ID of parent managing this form		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
build_stp_data_levels_widgets (
Widget	parent
)
{
	Widget	rowcol;
	Widget	text;
	Widget	label;
	Widget	stp_form;
	Widget	stp_label;
	int	i;
	int	ret;
	char	temp_name[STR_LEN*4];

/*	Register for any updates to this object class	*/

	ret = DEAU_UN_register( STP_DATA_LVLS_DEA_NAME, stp_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("STP, STA DEAU_UN_register failed: %d", ret );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Change the label of the frame container widget to indicate	*
 *	the type of data contained.					*/

	Stp_level_frame = XtVaCreateManagedWidget ("Stp_level_frame",
		xmFrameWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Display a label across the top of the form indicating which	*
 *	data set is currently being displayed.				*/

	label = XtVaCreateManagedWidget ("STP, STA Data Levels",
		xmLabelWidgetClass,	Stp_level_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create the form for the STP, STA Data Levels item.	*/

	Stp_level_form = XtVaCreateWidget ("Stp_level_form",
		xmFormWidgetClass,	Stp_level_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	stp_form = XtVaCreateWidget ("stp_form",
		xmFormWidgetClass,	Stp_level_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNverticalSpacing,	1,
		NULL);

	stp_label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("---------INSTRUCTIONS---------",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("Permissible value range is from",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("0.0 to 25.4 inches in multiples",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("of 0.1.  The value entered     ",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("represents the minimum value   ",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("of the data level.             ",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("                               ",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("NOTE: These thresholds affect both  ",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	stp_label = XtVaCreateManagedWidget ("      legacy PPS and dual-pol QPE products.",
		xmLabelWidgetClass,	stp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		stp_label,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (stp_form);
	
	rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Stp_level_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			stp_form,
		NULL);

	label = XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	text = XtVaCreateManagedWidget ("STP, STA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		5,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Code");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		STR_LEN,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Current (inches)");
	XmTextSetString (text, Buf);

	label = XtVaCreateManagedWidget ("  Spacer  ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	text = XtVaCreateManagedWidget ("STP, STA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		5,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Code");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		STR_LEN,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Current (inches)");
	XmTextSetString (text, Buf);

	XtManageChild (rowcol);

	sprintf( temp_name, "%s.%s", STP_DATA_LVLS_DEA_NAME, "code" );
	Stp_num_elements = DEAU_get_number_of_values( temp_name );

	if (Stp_num_elements > 0)
	{
	  Stp_object = (Widget *) calloc (sizeof( Widget ), Stp_num_elements);
	  if( Stp_object == NULL )
	  {
	    HCI_LE_error("Stp_object calloc failed for %d bytes", sizeof( Widget )*Stp_num_elements );
	      HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  Stp_elements = calloc( sizeof( char * ), Stp_num_elements );
	  if( Stp_elements == NULL )
	  {
	    HCI_LE_error("Stp_elements calloc failed for %d bytes", sizeof( char * )*Stp_num_elements );
	      HCI_task_exit( HCI_EXIT_FAIL );
	  }
	  for( i = 0; i < Stp_num_elements; i++ )
	  {
  	    Stp_elements[ i ] = calloc( sizeof( char ), STR_LEN );
	    if( Stp_elements[ i ] == NULL )
	    {
	      HCI_LE_error("Stp_elements[%d] calloc failed for %d bytes", i, sizeof( char )*STR_LEN );
	        HCI_task_exit( HCI_EXIT_FAIL );
	    }
	  }
	}
	else
	{
  	  HCI_LE_error("Failed getting STP, STA number of values" );
          HCI_task_exit( HCI_EXIT_FAIL );
	}

	if( read_stp() )
	{
	  HCI_LE_error("Error reading STP, STA Data Levels" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	for (i=0;i<Stp_num_elements/2;i++) {

	    rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Stp_level_form,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			rowcol,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			stp_form,
		NULL);

	    label = XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    text = XtVaCreateManagedWidget ("STP, STA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		2,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%2d ", i+1);
	    XmTextSetString (text, Buf);

	    if (i == 0) {

		label = XtVaCreateManagedWidget ("Spacer",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

	    } else if (i == 1) {

		label = XtVaCreateManagedWidget ("  >   ",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

	    } else {

		label = XtVaCreateManagedWidget ("  >=  ",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

	    }

	    Stp_object [i] = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		7,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		(XtPointer) i,
		NULL);

	    sprintf (Buf,"%s", Stp_elements[i]);
	    XmTextSetString (Stp_object [i], Buf);

	    if (i>1) {

		XtAddCallback (Stp_object [i],
			XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
			(XtPointer) 4);
		XtAddCallback (Stp_object [i],
			XmNfocusCallback, gain_focus_callback,
			(XtPointer) NULL);
		XtAddCallback (Stp_object [i],
			XmNlosingFocusCallback, selectable_parameters_modify,
			(XtPointer) STP_DATA_LEVELS);
		XtAddCallback (Stp_object [i],
			XmNactivateCallback, selectable_parameters_modify,
			(XtPointer) STP_DATA_LEVELS);

	    }

	    label = XtVaCreateManagedWidget ("    Spacer      ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    text = XtVaCreateManagedWidget ("STP, STA Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		2,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"%2d ", (i + Stp_num_elements/2 + 1));
	    XmTextSetString (text, Buf);

	    label = XtVaCreateManagedWidget ("  >=  ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    Stp_object [i+Stp_num_elements/2] = XtVaCreateManagedWidget ("Current (inches)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		7,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		(XtPointer) (i+Stp_num_elements/2),
		NULL);

	    sprintf (Buf,"%s", Stp_elements[i+Stp_num_elements/2]);
	    XmTextSetString (Stp_object [i+Stp_num_elements/2], Buf);

	    XtAddCallback (Stp_object [i+Stp_num_elements/2],
		XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
		(XtPointer) 4);
	    XtAddCallback (Stp_object [i+Stp_num_elements/2],
		XmNfocusCallback, gain_focus_callback,
		(XtPointer) NULL);
	    XtAddCallback (Stp_object [i+Stp_num_elements/2],
		XmNlosingFocusCallback, selectable_parameters_modify,
		(XtPointer) STP_DATA_LEVELS);
	    XtAddCallback (Stp_object [i+Stp_num_elements/2],
		XmNactivateCallback, selectable_parameters_modify,
		(XtPointer) STP_DATA_LEVELS);

	    XtManageChild (rowcol);

	}

	XtManageChild (Stp_level_form);
	XtUnmanageChild (Stp_level_frame);
}

/************************************************************************
 *	Description: This function creates the Velocity Data Level	*
 *		     Parameters widgets.				*
 *									*
 *	Input:  parent - widget ID of parent managing this form		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
build_velocity_data_levels_widgets (
Widget	parent
)
{
	Widget	rowcol;
	Widget	text;
	Widget	label;
	Widget	vel_form;
	Widget	table_frame;
	Widget	velocity_table;
	Widget	button;
	int	i, j;
	int	n;
	int	table;
	int	ret;
	char	temp_name[STR_LEN*4];
	XmString	str;
	char	buf2 [STR_LEN*4];
	float	val;
	Arg	arg [10];

/*	NOTE: Since there are 8 tables, at the present time we are	*
 *	treating them as 8 separate property objects.			*/

	for (i  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
	     i < NUM_VELOCITY_LEVELS;
	     i++)
	{
	  /* Register for any updates to this object class	*/

	  ret = DEAU_UN_register( Vel_dea_name_table[ i ], Vel_callback_table[ i ] );

	  if( ret < 0 )
	  {
	    HCI_LE_error("Velocity table %d DEAU_UN_register failed: %d", i, ret );
	    HCI_task_exit( HCI_EXIT_FAIL );
	  }
	}

/*	Change the label of the frame container widget to indicate	*
 *	the type of data contained.					*/

	Vel_level_frame = XtVaCreateManagedWidget ("Vel_level_frame",
		xmFrameWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Display a label across the top of the form indicating which	*
 *	data set is currently being displayed.				*/

	label = XtVaCreateManagedWidget ("Velocity Data Levels",
		xmLabelWidgetClass,	Vel_level_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create the form for the Velocity Data Levels item.	*/

	Vel_level_form = XtVaCreateWidget ("Vel_level_form",
		xmFormWidgetClass,	Vel_level_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	table_frame = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,		Vel_level_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Select Velocity Table",
		xmLabelWidgetClass,	table_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmVERTICAL);    n++;
	XtSetArg (arg [n], XmNpacking,		XmPACK_COLUMN); n++;
	XtSetArg (arg [n], XmNnumColumns,	2); n++;

	velocity_table = XmCreateRadioBox (table_frame,
		"select_table", arg, n);

	for (i  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
	     i < NUM_VELOCITY_LEVELS;
	     i++) {

	    XmString	str;

	    str = XmStringCreateLocalized ((Vel_table [i]+21));

	    button = XtVaCreateManagedWidget ("Velocity Table",
		xmToggleButtonWidgetClass,	velocity_table,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNtraversalOn,		False,
		XmNset,			False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    if (i == VELOCITY_LEVEL_TABLE_PRECIP_16_05) {

		XtVaSetValues (button,
			XmNset,			True,
			NULL);

	    } 

	    XtAddCallback (button,
		XmNvalueChangedCallback, velocity_table_select,
		(XtPointer) i);

	    XmStringFree (str);

	}

	XtManageChild (velocity_table);

	vel_form = XtVaCreateWidget ("vel_form",
		xmFormWidgetClass,	Vel_level_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		table_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNverticalSpacing,	1,
		NULL);

	label = XtVaCreateManagedWidget ("------------INSTRUCTIONS------------",
		xmLabelWidgetClass,	vel_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("Select a table from the above list. Edits to the",
		xmLabelWidgetClass,	vel_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("+ side are reflected in the - side. The allowable",
		xmLabelWidgetClass,	vel_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Velocity_label = XtVaCreateManagedWidget ("value range is from 2 to 128 kts.",
		xmLabelWidgetClass,	vel_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_BEGINNING,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (vel_form);
	
	for (table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
	     table < NUM_VELOCITY_LEVELS;
	     table++)
	{
	    sprintf( temp_name, "%s.%s", Vel_dea_name_table[ table ], "code" );
	    Vel_num_elements [table] = DEAU_get_number_of_values( temp_name );

	    if (Vel_num_elements [table] > 0)
	    {
	      Vel_object [table] = (Widget *) calloc (sizeof(Widget), Vel_num_elements [table]);
	      if( Vel_object[table] == NULL )
	      {
	        HCI_LE_error("Vel_object[%d] calloc failed for %d bytes", table, sizeof( Widget )*Vel_num_elements[table] );
	          HCI_task_exit( HCI_EXIT_FAIL );
	      }
	      Vel_elements[ table ] = calloc( sizeof( char * ), Vel_num_elements[ table ] );
	      if( Vel_elements[table] == NULL )
	      {
	        HCI_LE_error("Vel_elements[%d] calloc failed for %d bytes", table, sizeof( char * )*Vel_num_elements[table] );
	          HCI_task_exit( HCI_EXIT_FAIL );
	      }
	      for( i = 0; i < Vel_num_elements[ table ]; i++ )
	      {
	        Vel_elements[table][ i ] = calloc( sizeof( char ), STR_LEN );
	        if( Vel_elements[table][ i ] == NULL )
	        {
	          HCI_LE_error("Vel_elements[%d][%d] calloc failed for %d bytes", table, i, sizeof( char )*STR_LEN );
	            HCI_task_exit( HCI_EXIT_FAIL );
	        }
	      }
	    }
	    else
	    {
	      HCI_LE_error("Failed getting Velocity table %d number of values", table );
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
	}

	if( read_vel() )
	{
	  HCI_LE_error("Error reading Velocity Data Levels" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	for (table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
	     table < NUM_VELOCITY_LEVELS;
	     table++)
	{
	    Vel_table_form [table] = XtVaCreateWidget ("Vel_table_form",
		xmFormWidgetClass,	Vel_level_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		table_frame,
		NULL);

	    rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Vel_table_form [table],
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	    label = XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    text = XtVaCreateManagedWidget ("Vel Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		5,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"Code");
	    XmTextSetString (text, Buf);

	    text = XtVaCreateManagedWidget ("Current (knots)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		STR_LEN,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"Current (knots)");
	    XmTextSetString (text, Buf);

	    label = XtVaCreateManagedWidget ("  Spacer  ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	    text = XtVaCreateManagedWidget ("Vel Code",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		5,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"Code");
	    XmTextSetString (text, Buf);

	    text = XtVaCreateManagedWidget ("Current (knots)",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNcolumns,		STR_LEN,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		2,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	    sprintf (Buf,"Current (knots)");
	    XmTextSetString (text, Buf);

	    XtManageChild (rowcol);

	    for (i=0;i<Vel_num_elements [table]/2;i++) {

		rowcol = XtVaCreateWidget ("label_rowcol",
			xmRowColumnWidgetClass,		Vel_table_form [table],
			XmNbackground,			hci_get_read_color (BLACK),
			XmNorientation,			XmHORIZONTAL,
			XmNpacking,			XmPACK_TIGHT,
			XmNnumColumns,			1,
			XmNspacing,			0,
			XmNmarginHeight,		0,
			XmNmarginWidth,			0,
			XmNtopAttachment,		XmATTACH_WIDGET,
			XmNtopWidget,			rowcol,
			XmNleftAttachment,		XmATTACH_FORM,
			NULL);

		label = XtVaCreateManagedWidget ("Spacer",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		text = XtVaCreateManagedWidget ("Vel Code",
			xmTextFieldWidgetClass,	rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNcolumns,		2,
			XmNmarginHeight,	2,
			XmNshadowThickness,	0,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			NULL);

		sprintf (Buf,"%2d ", i+1);
		XmTextSetString (text, Buf);

		label = XtVaCreateManagedWidget ("Spacer",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		Vel_object [table][i] = XtVaCreateManagedWidget ("Current (knots)",
			xmTextFieldWidgetClass,	rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			XmNuserData,		(XtPointer) i,
			NULL);

	        sprintf (Buf,"%s", Vel_elements[table][i]);
		XmTextSetString (Vel_object [table][i], Buf);

		label = XtVaCreateManagedWidget ("    Spacer      ",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		text = XtVaCreateManagedWidget ("Vel Code",
			xmTextFieldWidgetClass,	rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNcolumns,		2,
			XmNmarginHeight,	2,
			XmNshadowThickness,	0,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			NULL);

		sprintf (Buf,"%2d ", i+Vel_num_elements[table]/2+1);
		XmTextSetString (text, Buf);

		label = XtVaCreateManagedWidget ("Spacer",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		j = i + Vel_num_elements[table]/2;

		Vel_object [table][j] = XtVaCreateManagedWidget ("Current (knots)",
			xmTextFieldWidgetClass,	rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNtraversalOn,		False,
			XmNuserData,		(XtPointer) j,
			NULL);

	        sprintf (Buf,"%s", Vel_elements[table][j]);
		XmTextSetString (Vel_object [table][j], Buf);

		if (j < Vel_num_elements [table]) {

		    XtAddCallback (Vel_object [table][j],
			XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
			(XtPointer) 6);
		    XtAddCallback (Vel_object [table][j],
			XmNfocusCallback, gain_focus_callback,
			(XtPointer) NULL);
		    XtAddCallback (Vel_object [table][j],
			XmNlosingFocusCallback, selectable_parameters_modify,
			(XtPointer) VELOCITY_DATA_LEVELS);
		    XtAddCallback (Vel_object [table][j],
			XmNactivateCallback, selectable_parameters_modify,
			(XtPointer) VELOCITY_DATA_LEVELS);

		}

		XtManageChild (rowcol);

	    }

	    XtManageChild (Vel_table_form [table]);
	}

/*	Only manage the currently active table.			*/

	for (table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
	     table < NUM_VELOCITY_LEVELS;
	     table++)
	{
	  if (Vel_level_table != table)
	  {
	    XtUnmanageChild (Vel_table_form [table]);
	  }
	}

	XtManageChild (Vel_level_form);

	sscanf (Vel_max_elements[Vel_level_table],"%f",&val);
	sprintf (buf2,"%d",(int) val);

	sprintf (Buf,"value range is from 2 to %s kts.", buf2);

	str = XmStringCreateLocalized (Buf);

	XtVaSetValues (Velocity_label,
		XmNlabelString,	str,
		NULL);

	XmStringFree (str);

	XtUnmanageChild (Vel_level_frame);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the velocity table radio buttons.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - velocity table ID				*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
velocity_table_select (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmString	str;
	char	buf2 [12];
	float	val;

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set)
	{
	    XtUnmanageChild (Vel_table_form [Vel_level_table]);

	    Vel_level_table = (int) client_data;

	    XtManageChild (Vel_table_form [Vel_level_table]);

	    XtManageChild (Vel_level_form);

	    sscanf (Vel_max_elements[Vel_level_table],"%f",&val);
	    sprintf (buf2,"%d",(int) val);
	    
	    sprintf (Buf,"value range is from 2 to %s kts.", buf2);

	    str = XmStringCreateLocalized (Buf);

	    XtVaSetValues (Velocity_label,
		XmNlabelString,	str,
		NULL);

	    XmStringFree (str);
	}
}

/************************************************************************
 *	Description: This function creates the VAD/RCM Heights		*
 *		     Parameters widgets.				*
 *									*
 *	Input:  parent - widget ID of parent managing this form		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
build_vad_rcm_height_levels_widgets (
Widget	parent
)
{
	Widget	rowcol;
	Widget	label;
	int	i;
	int	j;
	int	level;
	XmString	str;
	int	ret;

/*	We need to make sure that the first allowable VAD/RCM height	*
 *	is the first 1kft level above the radar.			*/

	Vad_rcm_min_height = (int) ((HCI_rda_elevation()/1000)+1);

/*	Register for any updates to this object class	*/

	ret = DEAU_UN_register( VAD_RCM_HEIGHTS_DEA_NAME, vad_rcm_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("VAD/RCM DEAU_UN_register failed: %d", ret );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Change the label of the frame container widget to indicate	*
 *	the type of data contained.					*/

	Vad_height_frame = XtVaCreateManagedWidget ("Vad_height_frame",
		xmFrameWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Display a label across the top of the form indicating which	*
 *	data set is currently being displayed.				*/

	label = XtVaCreateManagedWidget ("VAD and RCM Height Selections",
		xmLabelWidgetClass,	Vad_height_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create the form for the VAD/RCM Height Levels item.	*/

	Vad_height_form = XtVaCreateWidget ("Vad_height_form",
		xmFormWidgetClass,	Vad_height_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Vad_height_form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("  Level VAD RCM   Level VAD RCM   Level VAD RCM   Level VAD RCM   Level VAD RCM  Level VAD RCM   Level VAD RCM ",
		xmLabelWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (rowcol);

	/* Build widgets. */

	for (i=0;i<NUM_VAD_RCM_DATA_LEVELS/7;i++) {

	    rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,		Vad_height_form,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNspacing,			0,
		XmNmarginHeight,		0,
		XmNmarginWidth,			0,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	    for (j=0;j<7;j++) {

		level = i+1 + j*10;

		label = XtVaCreateManagedWidget ("Spce",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		label = XtVaCreateManagedWidget ("level",
			xmLabelWidgetClass,	rowcol,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNfontList,		hci_get_fontlist (LIST),
			NULL);

		sprintf (Buf,"%2d ",level);
		str = XmStringCreateLocalized (Buf);
		XtVaSetValues (label,
			XmNlabelString,	str,
			NULL);
		XmStringFree (str);

		Vad_object [level-1] = XtVaCreateManagedWidget (" ",
			xmToggleButtonWidgetClass,	rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNselectColor,		hci_get_read_color (WHITE),
			XmNfontList,		hci_get_fontlist (LIST),
			XmNset,			False,
			XmNsensitive,		False,
			XmNtraversalOn,		False,
			XmNuserData,		(XtPointer) level,
			NULL);

		XtAddCallback (Vad_object [level-1],
			XmNvalueChangedCallback, vad_rcm_height_select,
			(XtPointer) VAD_HEIGHT_LEVEL);

		Rcm_object [level-1] = XtVaCreateManagedWidget (" ",
			xmToggleButtonWidgetClass,	rowcol,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNselectColor,		hci_get_read_color (WHITE),
			XmNfontList,		hci_get_fontlist (LIST),
			XmNset,			False,
			XmNsensitive,		False,
			XmNtraversalOn,		False,
			XmNuserData,		(XtPointer) level,
			NULL);

		XtAddCallback (Rcm_object [level-1],
			XmNvalueChangedCallback, vad_rcm_height_select,
			(XtPointer) RCM_HEIGHT_LEVEL);

	    }

	    XtManageChild (rowcol);

	}

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	Vad_height_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		NULL);

	label = XtVaCreateManagedWidget ("Height levels are represented in kft.",
		xmLabelWidgetClass,	Vad_height_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	Vad_height_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	label = XtVaCreateManagedWidget ("NOTE: Up to 30 VAD height levels may be selected.  Up to 19 RCM height levels",
		xmLabelWidgetClass,	Vad_height_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	label = XtVaCreateManagedWidget ("     may be chosen.  An RCM level must be paired with a VAD height level.",
		xmLabelWidgetClass,	Vad_height_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	if( read_vad_rcm() )
	{
	  HCI_LE_error("Error reading VAD/RCM Heights" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	XtManageChild (Vad_height_form);
	XtUnmanageChild (Vad_height_frame);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the VAD or RCM height toggles.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - VAD_HEIGHT_LEVEL or RCM_HEIGHT_LEVEL	*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
vad_rcm_height_select (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	i;
	int	err;
	int	vad_indx;
	int	rcm_indx;
	XtPointer	data;

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	XtVaGetValues (w,
		XmNuserData,	&data,
		NULL);

	if (state->set) {

	    switch ((int) client_data) {

		case VAD_HEIGHT_LEVEL :

/*		    We already reached the max number of VAD levels.	*
 *		    Unset the toggle and display a message to the user.	*/

		    if (Vad_num_elements >= MAX_VAD_HEIGHTS) {

			XmToggleButtonSetState (w, False, False);
			sprintf (Buf,"You have already reached the maximum\nnumber of VAD heights.  Deselect\na height and try again.");
			err = 1;
			break;

		    } else {

			Vad_data [((int) data)-1] = 1;
			err = 0;
		    }
		    break;
		
		case RCM_HEIGHT_LEVEL :

/*		    We already reached the max number of RCM levels.	*
 *		    Unset the toggle and display a message to the user.	*/

		    if (Rcm_num_elements >= MAX_RCM_HEIGHTS) {

			XmToggleButtonSetState (w, False, False);
			sprintf (Buf,"You have already reached the maximum\nnumber of RCM heights.  Deselect\na height and try again.");
			err = 1;
			break;

		    } else {

/*			If a VAD height has not yet been set for this	*
 *			height then it must be before an RCM height can	*
 *			be defined.  If we have already reached the	*
 *			VAD heights limit, then we cannot do this.	*/

			if (Vad_data [((int) data)-1] == 0) {

			    if (Vad_num_elements >= MAX_VAD_HEIGHTS) {

				XmToggleButtonSetState (w, False, False);
				sprintf (Buf,"You have reached the maximum number\nof VAD heights. RCM heights are\npaired with VAD heights. A VAD height\nmust be deselected before this RCM\nheight is selected.");
				err = 1;
				break;

			    }
			}

			Vad_data     [((int) data)-1] = 1;
			Rcm_data [((int) data)-1] = 1;
			err = 0;
		    }
		    break;

		default:

		    err = 0;
		    break;
	    }

	} else {

	    err = 0;

	    if (Change_flag == HCI_NOT_CHANGED_FLAG)
	    {
	      Change_flag = Change_flag | VAD_CHANGE_FLAG;
	      modify_widgets_for_change();
	    }

	    switch ((int) client_data) {

		case VAD_HEIGHT_LEVEL :

/*                  We already reached the min number of VAD levels.    *
 *                  Unset the toggle and display a message to the user. */

		    if (Vad_num_elements == 1) {

        		XmToggleButtonSetState (w, True, True);
		        sprintf (Buf,"You have already reached the minimum\nnumber of VAD heights.");
		        err = 1;
  		      break;

		    } else if( Rcm_num_elements == 1 && Rcm_data[((int)data)-1] == 1 ) {

        		XmToggleButtonSetState (w, True, True);
		        sprintf (Buf,"You have reached the minimum number\nof RCM heights. RCM heights are\npaired with VAD heights. A RCM height\nmust be selected before this VAD\nheight is deselected." );
		        err = 1;
  		      break;

		    } else {

        		Vad_data [((int) data)-1] = 0;
        		Rcm_data [((int) data)-1] = 0;
 		        err = 0;
    		    }
		    break;

		case RCM_HEIGHT_LEVEL :

/*                  We already reached the min number of RCM levels.    *
 *                  Unset the toggle and display a message to the user. */

		    if (Rcm_num_elements == 1) {

        		XmToggleButtonSetState (w, True, True);
		        sprintf (Buf,"You have already reached the minimum\nnumber of RCM heights.");
		        err = 1;
  		      break;

    		    } else {

        	       Rcm_data [((int) data)-1] = 0;
 		       err = 0;
    		    }
		    break;


	    }
	}

	if (err) {

	    hci_warning_popup( Top_widget, Buf, acknowledge_invalid_value );

	} else {

/*	No error occurred so lets update the adaptation data.  We	*
 *	update the entire array set right now to ensure everything	*
 *	is in the proper ascending order.				*/

	    vad_indx = 0;
	    rcm_indx = 0;

	    if (Change_flag == HCI_NOT_CHANGED_FLAG)
	    {
	      Change_flag = Change_flag | VAD_CHANGE_FLAG;
	      modify_widgets_for_change();
	    }

	    for (i=0;i<NUM_VAD_RCM_DATA_LEVELS;i++) {


		if (vad_indx < MAX_VAD_HEIGHTS) {
		  if (Vad_data [i] != 0) {
		      sprintf (Vad_elements[vad_indx],"%d",(i+1)*1000);
		      vad_indx++;
		  }
		}

		if (rcm_indx < MAX_RCM_HEIGHTS) {
		  if (Rcm_data [i] != 0) {
		      sprintf (Rcm_elements[rcm_indx],"%d",(i+1)*1000);
		      rcm_indx++;
		  }
		}
	    }

	    Vad_num_elements = vad_indx;
	    Rcm_num_elements = rcm_indx;

	    sprintf (Buf,"0");

	    for (i=vad_indx;i<MAX_VAD_HEIGHTS;i++) {

		sprintf(Vad_elements[i], Buf);
	    }

	    for (i=rcm_indx;i<MAX_RCM_HEIGHTS;i++) {

		sprintf(Rcm_elements[i], Buf);
	    }
	}

	display_selectable_parameters (Current_category);
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
selectable_parameters_restore_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];
	sprintf( buf, "You are about to restore the product parameters\nadaptation data to baseline values.\nDo you want to continue?" );
	hci_confirm_popup( Top_widget, buf, accept_selectable_parameters_restore, NULL );
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
accept_selectable_parameters_restore (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Restore_flag = HCI_YES_FLAG;
}

void selectable_parameters_restore()
{
	int	error_flag = 0;

	processing_undo_or_restore = 1;

/*	If low bandwidth, popup the progress meter window.	*/

	HCI_PM( "Restoring Baseline Adaptation Data" );

/*	This section is for baseline data.				*/

	Baseline_flag = HCI_YES_FLAG;

/*	Check the category and restore the data from the baseline	*
 *	copy.								*/

	switch (Current_category) {

	    case CELL_PRODUCT_PARAMETERS :

		if( read_cell_prod() )
		{
		  error_flag = 1;
	  	  HCI_LE_error("An Error occurred restoring Cell Product Data" );
		  hci_error_popup( Top_widget, "Error reading Cell Product Data\nUnable to restore baseline data", NULL );
		}
		else
		{
		  Baseline_flag = HCI_NO_FLAG;
		  if( save_cell_prod() )
		  {
	  	    HCI_LE_error("An Error occurred saving Cell Product Data" );
		    hci_error_popup( Top_widget, "Error saving Cell Product Data\nUnable to restore baseline data", NULL );
		  }
		}
		break;

	    case VAD_RCM_HEIGHT_SELECTIONS :

		if( read_vad_rcm() )
		{
		  error_flag = 1;
	  	  HCI_LE_error("An Error occurred restoring VAD/RCM Heights" );
		  hci_error_popup( Top_widget, "Error reading VAD/RCM Heights\nUnable to restore baseline data", NULL );
		}
		else
		{
		  Baseline_flag = HCI_NO_FLAG;
		  if( save_vad() )
		  {
	  	    HCI_LE_error("An Error occurred saving VAD/RCM Heights" );
		    hci_error_popup( Top_widget, "Error saving VAD/RCM Heights\nUnable to restore baseline data", NULL );
		  }
		  if( save_rcm() )
		  {
	  	    HCI_LE_error("An Error occurred saving VAD/RCM Heights" );
		    hci_error_popup( Top_widget, "Error saving VAD/RCM Heights\nUnable to restore baseline data", NULL );
		  }
		}
		break;

	    case OHP_THP_DATA_LEVELS :

		if( read_ohp_thp() )
		{
		  error_flag = 1;
	  	  HCI_LE_error("An Error occurred restoring OHP/THP, OHA Data Levels" );
		  hci_error_popup( Top_widget, "Error reading OHP/THP, OHA Data Levels\nUnable to restore baseline data", NULL );
		}
		else
		{
		  Baseline_flag = HCI_NO_FLAG;
		  if( save_ohp_thp() )
		  {
	  	    HCI_LE_error("An Error occurred saving OHP/THP, OHA Data Levels" );
		    hci_error_popup( Top_widget, "Error saving OHP/THP, OHA Data Levels\nUnable to restore baseline data", NULL );
		  }
		}
		break;

	    case STP_DATA_LEVELS :

		if( read_stp() )
		{
		  error_flag = 1;
	  	  HCI_LE_error("An Error occurred restoring STP, STA Data Levels" );
		  hci_error_popup( Top_widget, "Error reading STP, STA Data Levels\nUnable to restore baseline data", NULL );
		}
		else
		{
		  Baseline_flag = HCI_NO_FLAG;
		  if( save_stp() )
		  {
	  	    HCI_LE_error("An Error occurred saving STP, STA Data Levels" );
		    hci_error_popup( Top_widget, "Error saving STP, STA Data Levels\nUnable to restore baseline data", NULL );
		  }
		}
		break;

	    case VELOCITY_DATA_LEVELS :
		    
		if( read_vel() )
		{
		  error_flag = 1;
	  	  HCI_LE_error("An Error occurred restoring Velocity Data Levels" );
		  hci_error_popup( Top_widget, "Error reading Velocity Data Levels\nUnable to restore baseline data", NULL );
		}
		else
		{
		  Baseline_flag = HCI_NO_FLAG;
		  if( save_vel() )
		  {
	  	    HCI_LE_error("An Error occurred saving Velocity Data Levels" );
		    hci_error_popup( Top_widget, "Error saving Velocity Data Levels\nUnable to restore baseline data", NULL );
		  }
		}
		break;

	}

/*	If read failed, then values seen by the user have not changed.
        If read was successful, but save failed, the values seen by the
        user have changed. Set Change_flag appropriately. */

	if( !error_flag )
	{
	  Change_flag = HCI_NOT_CHANGED_FLAG;
	}

/*	Desensitize the Save and Undo buttons since there shouldn't	*
 *	be any unsaved edits.						*/

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

	if (Unlocked_loca == HCI_YES_FLAG) {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	True,
		NULL);

	}

/*	Refresh the current category data properties.		*/

	display_selectable_parameters (Current_category);

	processing_undo_or_restore = 0;
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
selectable_parameters_update_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];
	sprintf( buf, "You are about to replace the baseline\n product parameters data values.\nDo you want to continue?" );
	hci_confirm_popup( Top_widget, buf, accept_selectable_parameters_update, NULL );
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
accept_selectable_parameters_update (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Update_flag = HCI_YES_FLAG;
}

void selectable_parameters_update()
{
/*	This section is for baseline data.				*/

	Baseline_flag = HCI_YES_FLAG;

/*	For the current category, update its parameters from the	*
 *	baseline copy.							*/

	HCI_PM( "Updating Baseline Adaptation Data" );

	switch (Current_category) {

	    case CELL_PRODUCT_PARAMETERS :

		if( save_cell_prod() )
		  sprintf( Buf, "Unable to update baseline Cell Product Parameters" );
		else
		  sprintf( Buf, "Baseline Cell Product Parameters updated" );
		break;

	    case VAD_RCM_HEIGHT_SELECTIONS :

		if( save_rcm() )
		  sprintf( Buf, "Unable to update baseline RCM Heights" );
		else
		  sprintf( Buf, "Baseline RCM Heights updated" );
		if( save_vad() )
		  strcat( Buf, " - Unable to update baseline VAD Heights" );
		else
		  strcat( Buf, " - Baseline VAD Heights updated" );
		break;

	    case OHP_THP_DATA_LEVELS :

		if( save_ohp_thp() )
		  sprintf( Buf, "Unable to update baseline OHP/THP, OHA Levels" );
		else
		  sprintf( Buf, "Baseline OHP/THP, OHA Levels updated" );
		break;

	    case STP_DATA_LEVELS :

		if( save_stp() )
		  sprintf( Buf, "Unable to update baseline STP, STA Data Levels" );
		else
		  sprintf( Buf, "Baseline STP, STA Data Levels updated" );
		break;

	    case VELOCITY_DATA_LEVELS :

		if( save_vel() )
		  sprintf( Buf, "Unable to update baseline Velocity Data Levels" );
		else
		  sprintf( Buf, "Baseline Velocity Data Levels updated" );
		break;

	}

/*	Display a message in the Feedback line of the RPG Control/	*
 *	status window.							*/

	HCI_display_feedback( Buf );
}

/************************************************************************
 *	Description: This function is activated when an editable text	*
 *		     widget gains keyboard focus.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
gain_focus_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*str;

/*	Get the current contents of the text widget receiving focus.	*/

	str = XmTextGetString (w);

/*	Save the contents so we can compare the value after the modify	*
 *	or losing focus callback is invoked.				*/

	strcpy (Old_tbuf, str);
	XtFree (str);

/*	Set the current edit button and category items.			*/

	Edit_button   = Current_button;
	Edit_category = Current_category;
}

/************************************************************************
 *	Description: This function is activated when the cell prod	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void cell_prod_callback()
{
  Cell_prod_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the OHP/THP, OHA	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void ohp_thp_callback()
{
  Ohp_thp_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the STP, STA	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void stp_callback()
{
  Stp_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the VAD/RCM	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vad_rcm_callback()
{
  Vad_rcm_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_precip_16_97_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_precip_16_194_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_precip_8_97_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_precip_8_194_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_clear_16_97_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_clear_16_194_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_clear_8_97_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function is activated when the Velocity	*
 *		     adaptation data is updated.			*
 *									*
 ************************************************************************/

void vel_clear_8_194_callback()
{
  Vel_updated = 1;
}

/************************************************************************
 *	Description: This function saves Cell Prod data.		*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int save_cell_prod()
{
  int i, ret;
  char temp_name[STR_LEN*4];
  int temp_int;
  double temp_dbl;
  int error_flag = 0;

  for( i = 0; i < Cell_num_elements; i++ )
  {
    /* Build name of DEA variable. */

    sprintf( temp_name, "%s.%s", CELL_PROD_DEA_NAME, Cell_branch_names[i] );

    /* Convert string to int, then int to double. */

    sscanf( Cell_elements[i], "%d", &temp_int );
    temp_dbl = (double)temp_int;

    /* Set value. */

    if( ( ret = DEAU_set_values( temp_name, 0, &temp_dbl, 1, Baseline_flag ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("DEAU_set_values (%s - baseline flag %d) failed (%d).", temp_name, Baseline_flag, ret );
    }
  }

  return error_flag;
}

/************************************************************************
 *	Description: This function saves OHP/THP, OHA data.		*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int save_ohp_thp()
{
  char temp_name[STR_LEN*4];
  char *tmp_buf = NULL;
  int i, ret;
  int error_flag = 0;

  /* Read values into temporary buffer. */

  for( i = 0; i < Ohp_num_elements; i++ )
  {
    tmp_buf = STR_append( tmp_buf, Ohp_elements[i], strlen(Ohp_elements[i])+1 );
  }

  /* Build name of DEA variable. */

  sprintf( temp_name, "%s.%s", OHP_THP_DATA_LVLS_DEA_NAME, "code" );

  /* Set values. */

  if( ( ret = DEAU_set_values( temp_name, 1, tmp_buf, Ohp_num_elements, Baseline_flag ) ) < 0 )
  {
    error_flag = 1;
    HCI_LE_error("DEAU_set_values (%s - baseline flag %d) failed (%d).", temp_name, Baseline_flag, ret );
  }

  /* Free allocated memory. */

  STR_free( tmp_buf );

  return error_flag;
}

/************************************************************************
 *	Description: This function saves STP, STA data.			*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int save_stp()
{
  char temp_name[STR_LEN*4];
  char *tmp_buf = NULL;
  int i, ret;
  int error_flag = 0;

  /* Read values into temporary buffer. */

  for( i = 0; i < Stp_num_elements; i++ )
  {
    tmp_buf = STR_append( tmp_buf, Stp_elements[i], strlen(Stp_elements[i])+1 );
  }

  /* Build name of DEA variable. */

  sprintf( temp_name, "%s.%s", STP_DATA_LVLS_DEA_NAME, "code" );

  /* Set values. */

  if( ( ret = DEAU_set_values( temp_name, 1, tmp_buf, Stp_num_elements, Baseline_flag ) ) < 0 )
  {
    error_flag = 1;
    HCI_LE_error("DEAU_set_values (%s - baseline flag %d) failed (%d).", temp_name, Baseline_flag, ret );
  }

  /* Free allocated memeory. */

  STR_free( tmp_buf );

  return error_flag;
}

/************************************************************************
 *	Description: This function saves RCM data.			*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int save_rcm()
{
  char temp_name[STR_LEN*4];
  char *tmp_rcm_buf = NULL;
  int i, ret;
  int error_flag = 0;

  /* Sanity check. */

  if( Rcm_num_elements < 1 )
  {
    HCI_LE_error("Num RCM elements (%d) < 1", Rcm_num_elements );
    return 1;
  }
  
  /* Read RCM values into temporary buffer. */

  for( i = 0; i < Rcm_num_elements; i++ )
  {
    tmp_rcm_buf = STR_append( tmp_rcm_buf, Rcm_elements[i], strlen(Rcm_elements[i])+1 );
  }

  /* Build name of RCM DEA variable. */

  sprintf( temp_name, "%s.%s", VAD_RCM_HEIGHTS_DEA_NAME, "rcm" );

  /* Set RCM values. */

  if( ( ret = DEAU_set_values( temp_name, 1, tmp_rcm_buf, Rcm_num_elements, Baseline_flag ) ) < 0 )
  {
    error_flag = 1;
    HCI_LE_error("DEAU_set_values (%s - baseline flag %d) failed (%d).", temp_name, Baseline_flag, ret );
  }

  /* Free allocated memory. */

  STR_free( tmp_rcm_buf );

  return error_flag;
}

/************************************************************************
 *	Description: This function saves VAD data.			*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int save_vad()
{
  char temp_name[STR_LEN*4];
  char *tmp_vad_buf = NULL;
  int i, ret;
  int error_flag = 0;

  /* Sanity check. */

  if( Vad_num_elements < 1 )
  {
    HCI_LE_error("Num VAD elements (%d) < 1", Vad_num_elements );
    return 1;
  }
  
  /* Read VAD values into temporary buffer. */

  for( i = 0; i < Vad_num_elements; i++ )
  {
    tmp_vad_buf = STR_append( tmp_vad_buf, Vad_elements[i], strlen(Vad_elements[i])+1 );
  }

  /* Build name of VAD DEA variable. */

  sprintf( temp_name, "%s.%s", VAD_RCM_HEIGHTS_DEA_NAME, "vad" );

  /* Set VAD values. */

  if( ( ret = DEAU_set_values( temp_name, 1, tmp_vad_buf, Vad_num_elements, Baseline_flag ) ) < 0 )
  {
    error_flag = 1;
    HCI_LE_error("DEAU_set_values (%s - baseline flag %d) failed (%d).", temp_name, Baseline_flag, ret );
  }

  /* Free allocated memory. */

  STR_free( tmp_vad_buf );

  return error_flag;
}

/************************************************************************
 *	Description: This function saves Velocity data.			*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int save_vel()
{
  int table;
  char temp_name[STR_LEN*4];
  char *tmp_buf = NULL;
  int i, ret;
  int error_flag = 0;

  for (table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
       table < NUM_VELOCITY_LEVELS;
       table++)
  {
    /* Read values into temporary buffer. */

    for( i = 0; i < Vel_num_elements[table]; i++ )
    {
      tmp_buf = STR_append( tmp_buf, Vel_elements[table][i], strlen(Vel_elements[table][i])+1 );
    }

    /* Build DEA variable name. */

    sprintf( temp_name, "%s.%s", Vel_dea_name_table[ table ], "code" );

    /* Set values. */

    if( ( ret = DEAU_set_values( temp_name, 1, tmp_buf, Vel_num_elements[table], Baseline_flag ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("DEAU_set_values (%s - baseline flag %d) failed (%d).", temp_name, Baseline_flag, ret );
    }

    /* Free allocated memory. */

    STR_free( tmp_buf );
    tmp_buf = NULL;
  }

  return error_flag;
}

/************************************************************************
 *	Description: This function modifies widget sensitivities if	*
 *		     a change is detected.				*
 *									*
 ************************************************************************/

void modify_widgets_for_change()
{
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
}

/************************************************************************
 *	Description: This function modifies widget sensitivities if	*
 *		     a change is not detected				*
 *									*
 ************************************************************************/

void modify_widgets_for_no_change()
{
  XtVaSetValues (Save_button,
	XmNsensitive,	False,
	NULL);
  XtVaSetValues (Undo_button,
	XmNsensitive,	False,
	NULL);
  XtVaSetValues (Restore_button,
	XmNsensitive,	True,
	NULL);
  XtVaSetValues (Update_button,
	XmNsensitive,	True,
	NULL);
}

/************************************************************************
 *	Description: This function verifies the value of the Cell	*
 *		     Product Parameter is valid.			*
 *									*
 ************************************************************************/

int verify_cell_prod_value( int indx, char *buf )
{
  int val, minval, maxval;

  sscanf( buf, "%d", &val );
  sscanf( Cell_min_elements[indx], "%d", &minval );
  sscanf( Cell_max_elements[indx], "%d", &maxval );

  if( val < minval || val > maxval )
  {
    HCI_LE_error("Cell Prod value outside valid range" );
    return 1;
  }

  return 0;
}

/************************************************************************
 *	Description: This function verifies the value of the OHP/THP, OHA	*
 *		     Parameter is valid.				*
 *									*
 ************************************************************************/

int verify_ohp_thp_value( int indx, char *buf )
{
  float val, upper_val, lower_val;
  int int_val;
  char *tok;

  /* Convert buf to a float. */

  sscanf( buf, "%f", &val );

  /* Make sure value is between previous/next value. End points
     can be tricky, but the first two values in the Ohp_thp_elements
     array are hard-coded. Thus, the only tricky part will be if
     the index is the end of the array . */

  if( indx != (Ohp_num_elements-1) )
  {
    sscanf( Ohp_elements[indx+1], "%f", &upper_val );
  }
  else
  {
    upper_val = 12.7;
  }
  sscanf( Ohp_elements[indx-1], "%f", &lower_val );

  /* Make sure value is a multiple of 0.05. First
     ensure that value has a decimal. */

  if( ( tok = strstr( buf, "." ) ) == NULL )
  {
    HCI_LE_error("OHP/THP, OHA value has no decimal" );
    return 2;
  }

  /* Increment past decimal. */

  tok++;

  /* Make sure fractional part of value only has no more than two digits. */

  if( strlen( tok ) > 2 )
  {
    HCI_LE_error("OHP/THP, OHA value has too many post decimal digits" );
    return 3;
  }

  /* Value should be an increment of 5. */

  sscanf( tok, "%d", &int_val );
  if( strlen( tok ) == 1 ){ int_val*=10; }
  if( int_val%5 )
  {
    HCI_LE_error("OHP/THP, OHA value not multiple of 0.05" );
    return 4;
  }

  /* Check ranges. */
 
  if( val < lower_val || val > upper_val )
  {
    HCI_LE_error("OHP/THP, OHA value outside of valid range" );
    return 1;
  }

  return 0;
}

/************************************************************************
 *	Description: This function verifies the value of the STP, STA	*
 *		     Parameter is valid.				*
 *									*
 ************************************************************************/

int verify_stp_value( int indx, char *buf )
{
  float val, upper_val, lower_val;
  char *tok;

  /* Convert buf to a float. */

  sscanf( buf, "%f", &val );

  /* Make sure value is between previous/next value. End points
     can be tricky, but the first two values in the Stp_elements
     array are hard-coded. Thus, the only tricky part will be if
     the index is the end of the array . */

  if( indx != (Stp_num_elements-1) )
  {
    sscanf( Stp_elements[indx+1], "%f", &upper_val );
  }
  else
  {
    upper_val = 25.4;
  }
  sscanf( Stp_elements[indx-1], "%f", &lower_val );

  /* Make sure value is a multiple of 0.1. First
     ensure that value has a decimal. */

  if( ( tok = strstr( buf, "." ) ) == NULL )
  {
    HCI_LE_error("STP, STA value has no decimal" );
    return 2;
  }

  /* Increment past decimal. */

  tok++;

  /* Make sure fractional part of value only has one digit. */

  if( strlen( tok ) != 1 )
  {
    HCI_LE_error("STP, STA value has too many post-decimal digits" );
    return 3;
  }

  /* Check ranges. */
 
  if( val < lower_val || val > upper_val )
  {
    HCI_LE_error("STP, STA value outside of valid range" );
    return 1;
  }

  return 0;
}

/************************************************************************
 *	Description: This function verifies the value of the Velocity	*
 *		     Parameter is valid.				*
 *									*
 ************************************************************************/

int verify_vel_value( int indx, char *buf )
{
  int val, upper_val, lower_val;

  /* Convert buf to integer. */

  sscanf( buf, "%d", &val );

  /* The value for the first positive velocity level can't be
     1 because -1 is a hard-coded value. */

  if( ( indx == Vel_num_elements[Vel_level_table] / 2 + 1 ) && val == 1 )
  {
    HCI_LE_error("Velocity value of first + value can't be 1" );
    return 1;
  }

  /* Make sure value is between previous/next value. The last
     value in the velocity data array is hard-coded as RF, so
     treat the next-to-last value as the last value. */

  if( indx == Vel_num_elements[Vel_level_table] - 2 )
  {
    sscanf( Vel_max_elements[Vel_level_table], "%d", &upper_val );
  }
  else
  {
    sscanf( Vel_elements[Vel_level_table][indx+1], "%d", &upper_val );
  }
  sscanf( Vel_elements[Vel_level_table][indx-1], "%d", &lower_val );

  if( val < lower_val || val > upper_val )
  {
    HCI_LE_error("Velocity value outside of valid range" );
    return 2;
  }

  return 0;
}

/************************************************************************
 *	Description: This function reads in the Cell Prod data.		*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int read_cell_prod( int baseline_flag )
{
  int i, ret;
  char *min_max_string = NULL;
  char temp_name[STR_LEN*4];
  DEAU_attr_t *d_attr;
  int error_flag = 0;
  double *temp_elements = NULL;

  Cell_prod_updated = 0;

  /* Define temporary buffer to hold values until they are known to be
     valid. The check for Cell_num_elements > 0 should be unnecessary
     at this point, but you can't be too careful. */

  if (Cell_num_elements > 0)
  {
    temp_elements = calloc( sizeof( double ), Cell_num_elements );
    if( temp_elements == NULL )
    {
      HCI_LE_error("cell prod temp_elements calloc failed for %d bytes", sizeof( double )*Cell_num_elements );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
  }
  else
  {
    HCI_LE_error("Cell_num_elements < 1 (%d)", Cell_num_elements );
    return 1;
  }

  /* Loop through Cell Prod branches and read in adaptation data values. */

  for( i = 0; i < Cell_num_elements; i++ )
  {
    sprintf( temp_name, "%s.%s", CELL_PROD_DEA_NAME, Cell_branch_names[i] );

    if( ( ret = DEAU_get_attr_by_id( temp_name, &d_attr ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("get_attr_by_id (%s) failed (%d)",
                   temp_name, ret );
      break;
    }

    sprintf (Cell_names[i],"%s", d_attr->ats[ DEAU_AT_NAME ]);
    sprintf (Cell_units[i],"%s", d_attr->ats[ DEAU_AT_UNIT ]);

    if( ( ret = DEAU_get_min_max_values( d_attr, &min_max_string) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("get_min_max_value (%s) failed (%d)",
                   temp_name, ret );
      break;
    }

    sprintf( Cell_min_elements[ i ], min_max_string );
    /* Increment pointer to max value. */
    min_max_string += strlen( min_max_string ) + 1;
    sprintf( Cell_max_elements[ i ], min_max_string );

    if( !Baseline_flag )
    {
      if( ( ret = DEAU_get_values( temp_name, &temp_elements[i], 1 ) ) < 0 )
      {
        error_flag = 1;
        HCI_LE_error("get_values (%s) failed (%d)",
                     temp_name, ret );
        break;
      }
    }
    else
    {
      if( ( ret = DEAU_get_baseline_values( temp_name, &temp_elements[i], 1 ) ) < 0 )
      {
        error_flag = 1;
        HCI_LE_error("get_values (%s -baseline) failed (%d)",
                     temp_name, ret );
        break;
      }
    }
  }

  /* If no error detected, make temporary values permanent. */

  if( !error_flag )
  {
    for( i = 0; i < Cell_num_elements; i++ )
    {
      sprintf( Cell_elements[ i ], "%g", temp_elements[ i ] );
    }
  }

  /* Free allocated memory. */

  free( temp_elements );

  return error_flag;
}

/************************************************************************
 *	Description: This function reads in the OHP/THP, OHA data.		*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int read_ohp_thp()
{
  int i, ret;
  char temp_name[STR_LEN*4];
  char *temp_elements = NULL;
  int error_flag = 0;

  Ohp_thp_updated = 0;

  /* Sanity check. */

  if( Ohp_num_elements < 1 )
  {
    HCI_LE_error("Ohp_num_elements < 1 (%d)", Ohp_num_elements );
    return 1;
  }

  /* Read in OHP/THP, OHA adaptation data values. */

  sprintf( temp_name, "%s.%s", OHP_THP_DATA_LVLS_DEA_NAME, "code" );

  if( !Baseline_flag )
  {
    if( ( ret = DEAU_get_string_values( temp_name, &temp_elements ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("get_values (%s) failed (%d)",
                   temp_name, ret );
    }
  }
  else
  {
    if( ( ret = DEAU_get_baseline_string_values( temp_name, &temp_elements ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("get_values (%s -baseline) failed (%d)",
                   temp_name, ret );
    }
  }

  /* If no error, then make sure number of returned elements is
     what was expected. If so, populate the permanent buffer. */

  if( !error_flag )
  {
    if( ret != Ohp_num_elements )
    {
      error_flag = 1;
      HCI_LE_error("num temp Ohp elements (%d) != num Ohp elements (%d)  (baseline_flag = %d)", ret, Ohp_num_elements, Baseline_flag );
    }
    else
    {
      for( i = 0; i < Ohp_num_elements; i++ )
      {
        sprintf( Ohp_elements[ i ], temp_elements );
        temp_elements += strlen( temp_elements ) + 1;
      }
    }
  }

  return error_flag;
}

/************************************************************************
 *	Description: This function reads in the STP, STA data.		*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int read_stp()
{
  int i, ret;
  char temp_name[STR_LEN*4];
  char *temp_elements = NULL;
  int error_flag = 0;

  Stp_updated = 0;

  /* Sanity check. */

  if( Stp_num_elements < 1 )
  {
    HCI_LE_error("Stp_num_elements < 1 (%d)", Stp_num_elements );
    return 1;
  }

  /* Read in STP, STA adaptation data values. */

  sprintf( temp_name, "%s.%s", STP_DATA_LVLS_DEA_NAME, "code" );

  if( !Baseline_flag )
  {
    if( ( ret = DEAU_get_string_values( temp_name, &temp_elements ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("get_values (%s) failed (%d)",
                   temp_name, ret );
    }
  }
  else
  {
    if( ( ret = DEAU_get_baseline_string_values( temp_name, &temp_elements ) ) < 0 )
    {
      error_flag = 1;
      HCI_LE_error("get_values (%s -baseline) failed (%d)",
                   temp_name, ret );
    }
  }

  /* If no error, then make sure number of returned elements is
     what was expected. If so, populate the permanent buffer. */

  if( !error_flag )
  {
    if( ret != Stp_num_elements )
    {
      error_flag = 1;
      HCI_LE_error("num temp Stp elements (%d) != num Stp elements (%d)  (baseline_flag = %d)", ret, Stp_num_elements, Baseline_flag );
    }
    else
    {
      for( i = 0; i < Stp_num_elements; i++ )
      {
        sprintf( Stp_elements[ i ], temp_elements );
        temp_elements += strlen( temp_elements ) + 1;
      }
    }
  }

  return error_flag;
}

/************************************************************************
 *	Description: This function reads in the Velocity Parameter data.*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int read_vel()
{
  int i, ret;
  int table;
  char temp_name[STR_LEN*4];
  char **temp_elements[ NUM_VELOCITY_LEVELS ];
  char temp_max_elements[ NUM_VELOCITY_LEVELS ][STR_LEN];
  char *temp_value = NULL;
  int error_flag = 0;

  Vel_updated = 0;

  /* Sanity check. Shouldn't need this, but you never know. */

  for( table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
       table < NUM_VELOCITY_LEVELS;
       table++ )
  {
    if( Vel_num_elements[table] < 1 )
    {
      HCI_LE_error("Vel_num_elements[%d] < 1 (%d)",
                   table, Vel_num_elements[table] );
      return 1;
    }
  }

  /* Initialize temporary buffers. Values are written to permanent
     buffers only when all values have been read and no errors detected. */

  for( table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
       table < NUM_VELOCITY_LEVELS;
       table++ )
  {
    temp_elements[table] = calloc( sizeof( char * ), Vel_num_elements[table] );
    if( temp_elements[table] == NULL )
    {
      HCI_LE_error("vel temp_elements[%d] calloc failed for %d bytes", table, sizeof( char * )*Vel_num_elements[table] );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
    for( i = 0; i < Vel_num_elements[table]; i++ )
    {
      temp_elements[table][i] = calloc( sizeof( char ), STR_LEN );
      if( temp_elements[table][i] == NULL )
      {
        HCI_LE_error("vel temp_elements[%d][%d] calloc failed for %d bytes", table, i, sizeof( char )*STR_LEN );
        HCI_task_exit( HCI_EXIT_FAIL );
      }
    }
  }
 
  /* Loop through each velocity table and read in the adaptation data. */

  for (table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
       table < NUM_VELOCITY_LEVELS;
       table++)
  {
    sprintf( temp_name, "%s.%s", Vel_dea_name_table[table], "code" );

    if( !Baseline_flag )
    {
      if( ( ret = DEAU_get_string_values( temp_name, &temp_value ) ) < 0 )
      {
        error_flag = 1;
        HCI_LE_error("get_values (%s) failed (%d)",
                     temp_name, ret );
        break;
      }
    }
    else
    {
      if( ( ret = DEAU_get_baseline_string_values( temp_name, &temp_value ) ) < 0 )
      {
        error_flag = 1;
        HCI_LE_error("get_values (%s -baseline) failed (%d)",
                     temp_name, ret );
        break;
      }
    }

    /* If no error, then make sure number of returned elements is
       what was expected. If so, populate the temporary buffer. */

    if( !error_flag )
    {
      if( ret != Vel_num_elements[table] )
      {
        error_flag = 1;
        HCI_LE_error("num temp Vel elements (%d) at %d != num Vel elements (%d)  (baseline_flag = %d)", ret, table, Vel_num_elements[table], Baseline_flag );
        break;
      }
      else
      {
        for( i = 0; i < ret; i++ )
        {
          sprintf( temp_elements[table][i], temp_value );
          temp_value += strlen( temp_value ) + 1;
        }
      }
    }

    /* If no error, populate max elements buffer for this table. */

    if( !error_flag )
    {
      sprintf( temp_name, "%s.%s", Vel_dea_name_table[table], "max" );

      if( ( ret = DEAU_get_string_values( temp_name, &temp_value ) ) < 0 )
      {
        error_flag = 1;
        HCI_LE_error("get_values (%s) failed (%d)",
                     temp_name, ret );
        break;
      }
      else
      {
        strcpy( temp_max_elements[table], temp_value );
      }
    }
  }

  /* If no error, then assume everything is okay. Copy temporary
     variables to permanent buffer. */

  if( !error_flag )
  {
    for( table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
         table < NUM_VELOCITY_LEVELS;
         table++ )
    {
      for( i = 0; i < Vel_num_elements[table]; i++ )
      {
        strcpy( Vel_elements[table][i], temp_elements[table][i] );
      }
      sprintf( Vel_max_elements[table], temp_max_elements[table] );
    }
  }

  /* Free allocated memory. */

  for( table  = VELOCITY_LEVEL_TABLE_PRECIP_16_05;
       table < NUM_VELOCITY_LEVELS;
       table++ )
  {
    for( i = 0; i < Vel_num_elements[table]; i++ )
    {
      free( temp_elements[table][i] );
    }
    free( temp_elements[table] );
  }

  return error_flag;
}

/************************************************************************
 *	Description: This function reads in the VAD/RCM data.		*
 *		     Return 0 on sucess, 1 for error.			*
 ************************************************************************/

int read_vad_rcm()
{
  int i, ret, ival;
  char temp_name[STR_LEN*4];
  int temp_rcm_num_elements = 0;
  int temp_vad_num_elements = 0;
  char *temp_values = NULL;
  char temp_rcm_elements[MAX_RCM_HEIGHTS][STR_LEN];
  char temp_vad_elements[MAX_VAD_HEIGHTS][STR_LEN];

  Vad_rcm_updated = 0;

  /* Read in RCM adaptation data values. */

  sprintf( temp_name, "%s.%s", VAD_RCM_HEIGHTS_DEA_NAME, "rcm" );

  if( !Baseline_flag )
  {
    temp_rcm_num_elements = DEAU_get_number_of_values( temp_name );
    if( temp_rcm_num_elements < 0 )
    {
      HCI_LE_error("get_number_of_values (%s) failed (%d)",
                   temp_name, temp_rcm_num_elements );
      return 1;
    }
    else if( temp_rcm_num_elements == 0 )
    {
      HCI_LE_error("%s = 0", temp_name );
    }
    else if( temp_rcm_num_elements > MAX_RCM_HEIGHTS )
    {
      HCI_LE_error("%s = %d set to max allowed (%d)",
                   temp_name, temp_rcm_num_elements, MAX_RCM_HEIGHTS );
      temp_rcm_num_elements = MAX_RCM_HEIGHTS;
    }

    if( temp_rcm_num_elements > 0 )
    {
      if( ( ret = DEAU_get_string_values( temp_name, &temp_values ) ) < 0 )
      {
        HCI_LE_error("get_string_values (%s) failed (%d)",
                     temp_name, ret );
        return 1;
      }
    }
  }
  else
  {
    temp_rcm_num_elements = DEAU_get_number_of_baseline_values( temp_name );
    if( temp_rcm_num_elements < 0 )
    {
      HCI_LE_error("get_number_of_values (%s -baseline) failed (%d)", temp_name, temp_rcm_num_elements );
      return 1;
    }
    else if( temp_rcm_num_elements == 0 )
    {
      HCI_LE_error("%s (baseline) = 0", temp_name );
    }
    else if( temp_rcm_num_elements > MAX_RCM_HEIGHTS )
    {
      HCI_LE_error("%s (baseline) = %d set to max allowed (%d)", temp_name, temp_rcm_num_elements, MAX_RCM_HEIGHTS );
      temp_rcm_num_elements = MAX_RCM_HEIGHTS;
    }

    if( temp_rcm_num_elements > 0 )
    {
      if( ( ret = DEAU_get_baseline_string_values( temp_name, &temp_values ) ) < 0 )
      {
        HCI_LE_error("get_string_values (%s -baseline) failed (%d)", temp_name, ret );
        return 1;
      }
    }
  }

  /* Read heights into temporary buffer. */

  for( i = 0; i < temp_rcm_num_elements; i++ )
  {
    sprintf( temp_rcm_elements[i], temp_values );
    temp_values += strlen( temp_values ) + 1;
  }

  /* Read in VAD adaptation data values. */

  sprintf( temp_name, "%s.%s", VAD_RCM_HEIGHTS_DEA_NAME, "vad" );

  if( !Baseline_flag )
  {
    temp_vad_num_elements = DEAU_get_number_of_values( temp_name );
    if( temp_vad_num_elements < 0 )
    {
      HCI_LE_error("get_number_of_values (%s) failed (%d)",
                   temp_name, temp_vad_num_elements );
      return 1;
    }
    else if( temp_vad_num_elements == 0 )
    {
      HCI_LE_error("%s = 0", temp_name );
    }
    else if( temp_vad_num_elements > MAX_VAD_HEIGHTS )
    {
      HCI_LE_error("%s = %d set to max allowed (%d)",
                   temp_name, temp_vad_num_elements, MAX_VAD_HEIGHTS );
      temp_vad_num_elements = MAX_VAD_HEIGHTS;
    }

    if( temp_vad_num_elements > 0 )
    {
      if( ( ret = DEAU_get_string_values( temp_name, &temp_values ) ) < 0 )
      {
        HCI_LE_error("get_string_values (%s) failed (%d",
                     temp_name, ret );
        return 1;
      }
    }
  }
  else
  {
    temp_vad_num_elements = DEAU_get_number_of_baseline_values( temp_name );
    if( temp_vad_num_elements < 0 )
    {
      HCI_LE_error("get_number_of_values (%s -baseline) failed (%d)", temp_name, temp_vad_num_elements );
      return 1;
    }
    else if( temp_vad_num_elements == 0 )
    {
      HCI_LE_error("%s (baseline) = 0", temp_name );
    }
    else if( temp_vad_num_elements > MAX_VAD_HEIGHTS )
    {
      HCI_LE_error("%s (baseline) = %d set to max allowed (%d)", temp_name, temp_vad_num_elements, MAX_VAD_HEIGHTS );
      temp_vad_num_elements = MAX_VAD_HEIGHTS;
    }

    if( temp_vad_num_elements > 0 )
    {
      if( ( ret = DEAU_get_baseline_string_values( temp_name, &temp_values ) ) < 0 )
      {
        HCI_LE_error("get_string_values (%s -baseline) failed (%d)", temp_name, ret );
        return 1;
      }
    }
  }

  /* Read heights into temporary buffer. */

  for( i = 0; i < temp_vad_num_elements; i++ )
  {
    sprintf( temp_vad_elements[i], temp_values );
    temp_values += strlen( temp_values ) + 1;
  }


  /* No errors reading data, so set permanent variables
     from temporary buffers. */

  /* Copy RCM values. */

  Rcm_num_elements = temp_rcm_num_elements;
  for( i = 0; i < Rcm_num_elements; i++ )
  {
    strcpy( Rcm_elements[i], temp_rcm_elements[i] );
  }

  /* Copy VAD values. */

  Vad_num_elements = temp_vad_num_elements;
  for( i = 0; i < Vad_num_elements; i++ )
  {
    strcpy( Vad_elements[i], temp_vad_elements[i] );
  }

  /* Make sure checkboxes are up to date, First, clear all of them.	*/

  for(i=Vad_rcm_min_height-1;i<NUM_VAD_RCM_DATA_LEVELS;i++)
  {
    XmToggleButtonSetState (Vad_object[i], False, False );
    XmToggleButtonSetState (Rcm_object[i], False, False );
    Vad_data[i] = 0;
    Rcm_data[i] = 0;
  }

  /* Loop through VAD data and set the checkboxes corresponding		*
   * to the heights defined in adaptation data.				*/

  for(i=0;i<Vad_num_elements;i++)
  {
    sscanf (Vad_elements[i],"%d",&ival);
    if(ival < Vad_rcm_min_height)
    {
      HCI_LE_error("VAD layer (%d) < VAD min (%d)", ival, Vad_rcm_min_height );
      continue;
    }
    ival = ival/1000;
    if((ival > 0) && (ival <= NUM_VAD_RCM_DATA_LEVELS))
    {
      Vad_data [ival-1] = 1;
      XmToggleButtonSetState (Vad_object [ival-1],True,False);
    }
  }

  /* Loop through RCM data and set the checkboxes corresponding		*
   * to the heights defined in adaptation data.				*/

  for(i=0;i<Rcm_num_elements;i++)
  {
    sscanf (Rcm_elements[i],"%d",&ival);
    if(ival < Vad_rcm_min_height)
    {
      HCI_LE_error("RCM layer (%d) < RCM min (%d)", ival, Vad_rcm_min_height);
      continue;
    }
    ival = ival/1000;
    if((ival > 0) && (ival <= NUM_VAD_RCM_DATA_LEVELS))
    {
      if( Vad_data [ival-1] != 0 ){ Rcm_data [ival-1] = 1; }
      XmToggleButtonSetState (Vad_object [ival-1],True,False);
    }
  }

  return 0;
}

