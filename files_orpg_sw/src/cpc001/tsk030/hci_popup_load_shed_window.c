/************************************************************************
 *	Module:	 hci_popup_load_shed_window.c				*
 *									*
 *	Description:  This module is used by the ORPG HCI to display	*
 *		      and edit Load Shed Category data.			*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:54 $
 * $Id: hci_popup_load_shed_window.c,v 1.64 2010/03/10 18:46:54 ccalvert Exp $
 * $Revision: 1.64 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Global widget definitions.					*/

static	Widget	Top_widget                = (Widget) NULL;
static	Widget	Form                      = (Widget) NULL;
static	Widget	Save_button	  	  = (Widget) NULL;
static	Widget	Undo_button	  	  = (Widget) NULL;
static	Widget	Restore_button	  	  = (Widget) NULL;
static	Widget	Update_button	  	  = (Widget) NULL;
static	Widget	Dist_warn_widget          = (Widget) NULL;
static	Widget	Dist_alarm_widget         = (Widget) NULL;
static	Widget	Dist_current_widget       = (Widget) NULL;
static	Widget	Storage_warn_widget       = (Widget) NULL;
static	Widget	Storage_alarm_widget      = (Widget) NULL;
static	Widget	Storage_current_widget    = (Widget) NULL;
static	Widget	Rda_radial_warn_widget    = (Widget) NULL;
static	Widget	Rda_radial_alarm_widget   = (Widget) NULL;
static	Widget	Rda_radial_current_widget = (Widget) NULL;
static	Widget	Rpg_radial_warn_widget    = (Widget) NULL;
static	Widget	Rpg_radial_alarm_widget   = (Widget) NULL;
static	Widget	Rpg_radial_current_widget = (Widget) NULL;

static	Widget	Lock_button = (Widget) NULL;

static	int	Load_shed_data [LOAD_SHED_CATEGORY_WB_USER+1][LOAD_SHED_CURRENT_VALUE+1] = {{0}};
				/* Buffer to hold current thresholds	*
				 * and settings.  When save is selected *
				 * and verified, these data are copied  *
				 * to the load shed LB.			*/
static	int	Unlocked_roc = HCI_NO_FLAG; /* ROC LOCA unlocked flag */
static	int	Change_flag    = HCI_NOT_CHANGED_FLAG;
				/* flag to indicate that a threshold	*
				 * field has been modified.		*/
static	int	Close_flag     = 0; /* Set to 1 when Close button selected */
static	int	Restore_flag   = 0; /* Set to 1 when Restore button selected */
static	int	Do_not_update  = 0; /* Set to 1 means timer proc won't force
				       a display update. */

void	timer_proc ();

void	load_shed_close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	load_shed_save_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	load_shed_undo_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	load_shed_restore_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_load_shed_restore_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	load_shed_update_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_load_shed_update_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_load_shed_save_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	cancel_load_shed_save_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	load_shed_modify_callback (Widget w,
		XtPointer client_data, XtPointer call_data);

int	load_shed_lock_callback ();
void	display_load_shed_category_data (int flag);

char	Buf [256]; /* shared buffer for string operations */

/************************************************************************
 *	Description: This is the main function for the Load Shed	*
 *		     Categories task.					*
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
	Widget		control_rowcol;
	Widget		button;
	Widget		label;
	Widget		text;
	Widget		load_shed_frame;
	Widget		load_shed_form;
	Widget		load_shed_rowcol;
	Widget		lock_form;

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_LOAD_TASK );

/*	Initialize flags.						*/

	Change_flag    = HCI_NOT_CHANGED_FLAG;
	Close_flag     = 0;

	Top_widget = HCI_get_top_widget();

/*	Allow write to ORPGDAT_LOAD_SHED_CAT LB. */

	ORPGDA_write_permission( ORPGDAT_LOAD_SHED_CAT );

/*	Use a form widget to organize the various menu widgets.		*/

	Form   = XtVaCreateWidget ("Form",
		xmFormWidgetClass,		Top_widget,
		XmNautoUnmanage,		False,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Display a set of control buttons at the top of the window.	*/

	control_rowcol = XtVaCreateWidget ("control_rowcol",
		xmRowColumnWidgetClass,		Form,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNisAligned,			False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, load_shed_close_callback, NULL);

	Save_button = XtVaCreateManagedWidget ("Save",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Save_button,
		XmNactivateCallback, load_shed_save_callback, NULL);

	Undo_button = XtVaCreateManagedWidget ("Undo",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Undo_button,
		XmNactivateCallback, load_shed_undo_callback, NULL);

	XtVaCreateManagedWidget ("   Baseline:",
		xmLabelWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Restore_button = XtVaCreateManagedWidget ("Restore",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Restore_button,
		XmNactivateCallback, load_shed_restore_callback, NULL);

	Update_button = XtVaCreateManagedWidget ("Update",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Update_button,
		XmNactivateCallback, load_shed_update_callback, NULL);

	XtManageChild (control_rowcol);

	lock_form = XtVaCreateWidget ("lock_form",
		xmFormWidgetClass,	Form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	Lock_button = hci_lock_widget( lock_form, load_shed_lock_callback, HCI_LOCA_ROC );

	XtManageChild (lock_form);

/*	Create a container and a table containing load shed category	*
 *	data for all categories.					*/

	load_shed_frame   = XtVaCreateManagedWidget ("load_shed",
		xmFrameWidgetClass,		Form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			control_rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Load Shedding Category Data",
		xmLabelWidgetClass,	load_shed_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	load_shed_form = XtVaCreateWidget ("load_shed_form",
		xmFormWidgetClass,	load_shed_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_form,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	load_shed_rowcol = XtVaCreateWidget ("load_shed_rowcol",
		xmRowColumnWidgetClass,	load_shed_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNspacing,		0,
		XmNmarginHeight,	0,
		XmNmarginWidth,		0,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	text = XtVaCreateManagedWidget ("Category_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		24,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Category");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Warning_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Warning");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Alarm_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Alarm");
	XmTextSetString (text, Buf);

	text = XtVaCreateManagedWidget ("Current_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Current");
	XmTextSetString (text, Buf);

	 XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (load_shed_rowcol);

	load_shed_rowcol = XtVaCreateWidget ("load_shed_rowcol",
		xmRowColumnWidgetClass,	load_shed_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNspacing,		0,
		XmNmarginHeight,	0,
		XmNmarginWidth,		0,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		load_shed_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	 XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (load_shed_rowcol);

	load_shed_rowcol = XtVaCreateWidget ("load_shed_rowcol",
		xmRowColumnWidgetClass,	load_shed_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNspacing,		0,
		XmNmarginHeight,	0,
		XmNmarginWidth,		0,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		load_shed_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	text = XtVaCreateManagedWidget ("product_distribution_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		24,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Distribution");
	XmTextSetString (text, Buf);

	Dist_warn_widget = XtVaCreateManagedWidget ("distribution_warn",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_PROD_DIST,
		NULL);

	XtAddCallback (Dist_warn_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Dist_warn_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Dist_warn_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	XtAddCallback (Dist_warn_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	Dist_alarm_widget = XtVaCreateManagedWidget ("distribution_alarm",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_PROD_DIST,
		NULL);

	XtAddCallback (Dist_alarm_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Dist_alarm_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Dist_alarm_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	XtAddCallback (Dist_alarm_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	Dist_current_widget = XtVaCreateManagedWidget ("distribution_current",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		LOAD_SHED_CATEGORY_PROD_DIST,
		NULL);

	 XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (load_shed_rowcol);

	load_shed_rowcol = XtVaCreateWidget ("load_shed_rowcol",
		xmRowColumnWidgetClass,	load_shed_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNspacing,		0,
		XmNmarginHeight,	0,
		XmNmarginWidth,		0,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		load_shed_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	text = XtVaCreateManagedWidget ("product_storage_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		24,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"Product Storage");
	XmTextSetString (text, Buf);

	Storage_warn_widget = XtVaCreateManagedWidget ("storage_warn",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_PROD_STORAGE,
		NULL);

	XtAddCallback (Storage_warn_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Storage_warn_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Storage_warn_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	XtAddCallback (Storage_warn_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	Storage_alarm_widget = XtVaCreateManagedWidget ("storage_alarm",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_PROD_STORAGE,
		NULL);

	XtAddCallback (Storage_alarm_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Storage_alarm_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Storage_alarm_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	XtAddCallback (Storage_alarm_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	Storage_current_widget = XtVaCreateManagedWidget ("storage_current",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		LOAD_SHED_CATEGORY_PROD_STORAGE,
		NULL);

	 XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (load_shed_rowcol);

	load_shed_rowcol = XtVaCreateWidget ("load_shed_rowcol",
		xmRowColumnWidgetClass,	load_shed_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNspacing,		0,
		XmNmarginHeight,	0,
		XmNmarginWidth,		0,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		load_shed_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	text = XtVaCreateManagedWidget ("rda_radial_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		24,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"RDA Radial");
	XmTextSetString (text, Buf);

	Rda_radial_warn_widget = XtVaCreateManagedWidget ("rda_radial_warn",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_RDA_RADIAL,
		NULL);

	XtAddCallback (Rda_radial_warn_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Rda_radial_warn_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Rda_radial_warn_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	XtAddCallback (Rda_radial_warn_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	Rda_radial_alarm_widget = XtVaCreateManagedWidget ("rda_radial_alarm",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_RDA_RADIAL,
		NULL);

	XtAddCallback (Rda_radial_alarm_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Rda_radial_alarm_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Rda_radial_alarm_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	XtAddCallback (Rda_radial_alarm_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	Rda_radial_current_widget = XtVaCreateManagedWidget ("rda_radial_current",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		LOAD_SHED_CATEGORY_RDA_RADIAL,
		NULL);

	 XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (load_shed_rowcol);

	load_shed_rowcol = XtVaCreateWidget ("load_shed_rowcol",
		xmRowColumnWidgetClass,	load_shed_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNspacing,		0,
		XmNmarginHeight,	0,
		XmNmarginWidth,		0,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		load_shed_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	text = XtVaCreateManagedWidget ("rpg_radial_label",
		xmTextWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		24,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		NULL);

	sprintf (Buf,"RPG Radial");
	XmTextSetString (text, Buf);

	Rpg_radial_warn_widget = XtVaCreateManagedWidget ("rpg_radial_warn",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_RPG_RADIAL,
		NULL);

	XtAddCallback (Rpg_radial_warn_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Rpg_radial_warn_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Rpg_radial_warn_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	XtAddCallback (Rpg_radial_warn_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_WARNING_THRESHOLD);

	Rpg_radial_alarm_widget = XtVaCreateManagedWidget ("rpg_radial_alarm",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		True,
		XmNtraversalOn,		True,
		XmNuserData,		LOAD_SHED_CATEGORY_RPG_RADIAL,
		NULL);

	XtAddCallback (Rpg_radial_alarm_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);

	XtAddCallback (Rpg_radial_alarm_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) NULL);

	XtAddCallback (Rpg_radial_alarm_widget,
		XmNlosingFocusCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	XtAddCallback (Rpg_radial_alarm_widget,
		XmNactivateCallback, load_shed_modify_callback,
		(XtPointer) LOAD_SHED_ALARM_THRESHOLD);

	Rpg_radial_current_widget = XtVaCreateManagedWidget ("rpg_radial_current",
		xmTextFieldWidgetClass,	load_shed_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		8,
		XmNmarginHeight,	2,
		XmNshadowThickness,	0,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNtraversalOn,		False,
		XmNuserData,		LOAD_SHED_CATEGORY_RPG_RADIAL,
		NULL);

	 XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	load_shed_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (load_shed_rowcol);
	XtManageChild (load_shed_form);
	XtManageChild (Form);

	HCI_PM( "Reading Load Shed information" );

	display_load_shed_category_data (-1);

	XtRealizeWidget (Top_widget);

/*	Start HCI loop. */

	HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This is the timer procedure for the Load Shed	*
 *		     Categories window.  Its primary purpose is to	*
 *		     refresh the display when load shed status/data	*
 *		     changes.						*
 *									*
 *	Input:  w - timer parent widget ID				*
 *		id - timer interval ID					*
 *	Output: NONE							*
 *	Return: 0 (unused)						*
 ************************************************************************/

void
timer_proc ()
{

	int	refresh_flag;

	refresh_flag = 0;

/*	Check to see if any of the load shed category messages have	*
 *	been updated.  If so, we need to refresh the display.		*/

	if (ORPGLOAD_update_flag (LOAD_SHED_THRESHOLD_MSG_ID) == 0) {

	    refresh_flag = 1;

/*	    If we restored from baseline, read and set all thresholds	*
 *	    in order to clear any outstanding alarms.			*/

	    if (Restore_flag) {

		int	i, j;
		int	status;

		Restore_flag = 0;

		for (i=LOAD_SHED_CATEGORY_PROD_DIST;i<LOAD_SHED_CATEGORY_WB_USER;i++) {

		    for (j=LOAD_SHED_ALARM_THRESHOLD;j>=LOAD_SHED_WARNING_THRESHOLD;j--) {

		        status = ORPGLOAD_get_data (i, j,
					    &Load_shed_data [i][j]);
			if (status != 0) {

			    HCI_LE_error("Error getting load shed data (%d,%d)",
					i, j);

			} else {

			    status = ORPGLOAD_set_data (i, j,
					    Load_shed_data [i][j]);

			    if (status != 0) {

			       HCI_LE_error("Error setting load shed data (%d,%d) %d",
					i, j, Load_shed_data [i][j]);

			    }
			}
		    }
		}
	    }
	}

	if ((ORPGLOAD_update_flag (LOAD_SHED_CURRENT_MSG_ID) == 0) &&
	    (Unlocked_roc == HCI_NO_FLAG)) {

	    refresh_flag = 1;

	}

	if (refresh_flag && (!Do_not_update)) {

	    display_load_shed_category_data (LOAD_SHED_CURRENT_MSG_ID);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button from the Load Shed Categories	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
load_shed_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

	HCI_LE_log("Load Shed Category Close selected");

	Close_flag = 1;

	if (Change_flag) {

	    load_shed_save_callback (w,
				(XtPointer) NULL,
				(XtPointer) NULL);

	} else {

	    Close_flag = 0;
	    XtDestroyWidget (Top_widget);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Save" button from the Load Shed Categories	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
load_shed_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Load Shed Save selected");
	sprintf (Buf, "Do you want to save your changes?\n");
	hci_confirm_popup( Top_widget, Buf, accept_load_shed_save_callback, cancel_load_shed_save_callback );
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
accept_load_shed_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	int	i, j;

	HCI_LE_log("Load Shed Save accepted");

	for (i=LOAD_SHED_CATEGORY_PROD_DIST;i<LOAD_SHED_CATEGORY_WB_USER;i++) {

	    for (j=LOAD_SHED_WARNING_THRESHOLD;j<=LOAD_SHED_ALARM_THRESHOLD;j++) {

		status = ORPGLOAD_set_data (i, j, Load_shed_data [i][j]);

		if (status != 0) {

		    HCI_LE_error("Error setting load shed data (%d,%d) %d",
				i, j, Load_shed_data [i][j]);

		}
	    }
	}

	ORPGLOAD_write (LOAD_SHED_THRESHOLD_MSG_ID);

	Change_flag = HCI_NOT_CHANGED_FLAG;

	if (Close_flag) {

	    Close_flag = 0;
	    XtDestroyWidget (Top_widget);

	}

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

	if (Unlocked_roc == HCI_YES_FLAG) {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	True,
		NULL);

	} else {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	False,
		NULL);

	}

	Do_not_update = 0;
	display_load_shed_category_data (-1);

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
cancel_load_shed_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Load Shed Save rejected");

	if (Close_flag) {

	    Close_flag = 0;
	    XtDestroyWidget (Top_widget);

	}

	if (Unlocked_roc == HCI_NO_FLAG) {

	    load_shed_undo_callback (w,
				 (XtPointer) NULL,
				 (XtPointer) NULL);

	}

	Do_not_update = 0;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the lock button or selects an LOCA radio button	*
 *		     or enters a password in the Password window.	*
 *									*
 *	Input: NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
load_shed_lock_callback ()
{
  Do_not_update = 1;

  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_selected() )
  {
    display_load_shed_category_data(-1);
    Do_not_update = 0;
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_ROC_unlocked() )
    {
      Unlocked_roc = HCI_YES_FLAG;
      if( ORPGEDLOCK_get_edit_status( ORPGDAT_LOAD_SHED_CAT, LOAD_SHED_THRESHOLD_MSG_ID ) == ORPGEDLOCK_EDIT_LOCKED )
      {
        sprintf( Buf, "Another user is currently editing load\nshed threshold data. Any changes may be\noverwritten by the other user." );
        hci_warning_popup( Top_widget, Buf, NULL );
      }

      /* Set the edit advisory lock. */
      ORPGEDLOCK_set_edit_lock( ORPGDAT_LOAD_SHED_CAT, LOAD_SHED_THRESHOLD_MSG_ID );
      display_load_shed_category_data(-1);

      if (Change_flag)
      {
        XtVaSetValues( Restore_button, XmNsensitive, False, NULL );
        XtVaSetValues( Update_button, XmNsensitive, False, NULL );
      }
      else
      {
        XtVaSetValues( Restore_button, XmNsensitive, True, NULL );
        XtVaSetValues( Update_button, XmNsensitive, True, NULL );
      }

      Do_not_update = 0;
    }
  }
  else if( hci_lock_close() && Unlocked_roc == HCI_YES_FLAG )
  {
    Unlocked_roc = HCI_NO_FLAG;
    if( Change_flag )
    {
      load_shed_save_callback( Save_button, NULL, NULL );
      XtVaSetValues( Restore_button, XmNsensitive, False, NULL );
      XtVaSetValues( Update_button, XmNsensitive, False, NULL );
    }
    else
    {
      display_load_shed_category_data(-1);
      Do_not_update = 0;
    }
    /* Clear the edit advisory lock since we are done editing	*/
    ORPGEDLOCK_clear_edit_lock( ORPGDAT_LOAD_SHED_CAT, LOAD_SHED_THRESHOLD_MSG_ID );
  }

  return HCI_LOCK_PROCEED;
}

/************************************************************************
 *	Description: This function is used to display selected load	*
 *		     shed category data in the Load Shed Categories	*
 *		     window.						*
 *									*
 *	Input:  flag - LOAD_SHED_THRESHOLD_MSG_ID			*
 *		       LOAD_SHED_CURRENT_MSG_ID				*
 *		       -1 (update current and threshold fields)		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_load_shed_category_data (
int	flag
)
{
	int	edit_flag;
static	int	old_edit_flag = False;
	int	foreground;
	int	background;
	int	margin;
	int	shadow;
	int	warn_status;
	int	alarm_status;
	int	status;
	int	cur_foreground;
	int	cur_background;

/*	If the Load Shed Categories window isn't open, do nothing.	*/

	if (Top_widget == (Widget) NULL) {

	    return;

	}

/*	Check the lock data to see if the window is to be locked,	*
 *	unlocked (ROC level user only), or just display LOCA.		*/

	if (Unlocked_roc == HCI_YES_FLAG) {

	    foreground = hci_get_read_color (EDIT_FOREGROUND);
	    background = hci_get_read_color (EDIT_BACKGROUND);

	    if (!old_edit_flag) {

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

	    edit_flag = True;
	    shadow = 2;
	    margin = 0;

	} else {

	    if ( hci_lock_ROC_selected() ) {

		 foreground = hci_get_read_color (LOCA_FOREGROUND);
		 background = hci_get_read_color (BACKGROUND_COLOR1);

	    } else {

		 foreground = hci_get_read_color (EDIT_FOREGROUND);
		 background = hci_get_read_color (BACKGROUND_COLOR1);

	    }

	    if (old_edit_flag) {

		XtVaSetValues (Restore_button,
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Update_button,
			XmNsensitive,	False,
			NULL);

	    }

	    edit_flag = False;
	    shadow    = 0;
	    margin    = 2;

	}

	old_edit_flag = edit_flag;

/*	Update the sensitivity/color of the warning and alarm widgets	*
 *	based on password and LOCA.					*/

	XtVaSetValues (Dist_warn_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Dist_alarm_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Storage_warn_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Storage_alarm_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Rda_radial_warn_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Rda_radial_alarm_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Rpg_radial_warn_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

	XtVaSetValues (Rpg_radial_alarm_widget,
		XmNeditable,		edit_flag,
		XmNtraversalOn,		edit_flag,
		XmNforeground,		foreground,
		XmNbackground,		background,
		XmNmarginHeight,	margin,
		XmNshadowThickness,	shadow,
		NULL);

/*	Although we don't display the INPUT_BUF category, we need	*
 *	to read it so we can write the values back if "Save" is selected*/

	if ((flag == LOAD_SHED_THRESHOLD_MSG_ID) ||
	    (flag == LOAD_SHED_CURRENT_MSG_ID) ||
	    (flag == -1)) {

	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_INPUT_BUF,
					LOAD_SHED_WARNING_THRESHOLD,
					&Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_WARNING_THRESHOLD]);
	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_INPUT_BUF,
					LOAD_SHED_ALARM_THRESHOLD,
					&Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_ALARM_THRESHOLD]);
	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_INPUT_BUF,
				        LOAD_SHED_CURRENT_VALUE,
				        &Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_CURRENT_VALUE]);

	    warn_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
					LOAD_SHED_WARNING_THRESHOLD,
					&Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_WARNING_THRESHOLD]);

	    if (warn_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_WARNING_THRESHOLD]);
	        XmTextSetString (Dist_warn_widget, Buf);

	    }

	    alarm_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
					 LOAD_SHED_ALARM_THRESHOLD,
					 &Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_ALARM_THRESHOLD]);

	    if (alarm_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_ALARM_THRESHOLD]);
	        XmTextSetString (Dist_alarm_widget, Buf);

	    }

/*	Determine whether the current value meets one of the threshold	*
 *	levels.  If so, set the appropriate color (ALARM = CYAN,	*
 *	WARNING = GREEN, NORMAL = GREEN).				*/

	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
				       LOAD_SHED_CURRENT_VALUE,
				       &Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_CURRENT_VALUE]);

	    if (status == 0) {

		if (Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_CURRENT_VALUE] >=
		    Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_ALARM_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (CYAN);

		} else if (Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_CURRENT_VALUE] >=
			   Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_WARNING_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		} else {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		}

		sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_PROD_DIST][LOAD_SHED_CURRENT_VALUE]);
		XmTextSetString (Dist_current_widget, Buf);
		XtVaSetValues (Dist_current_widget,
			XmNforeground,	cur_foreground,
			XmNbackground,	cur_background,
			NULL);

	    }

	    warn_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_STORAGE,
					LOAD_SHED_WARNING_THRESHOLD,
					&Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_WARNING_THRESHOLD]);

	    if (warn_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_WARNING_THRESHOLD]);
	        XmTextSetString (Storage_warn_widget, Buf);

	    }

	    alarm_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_STORAGE,
					 LOAD_SHED_ALARM_THRESHOLD,
					 &Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_ALARM_THRESHOLD]);

	    if (alarm_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_ALARM_THRESHOLD]);
	        XmTextSetString (Storage_alarm_widget, Buf);

	    }

/*	Determine whether the current value meets one of the threshold	*
 *	levels.  If so, set the appropriate color (ALARM = CYAN,	*
 *	WARNING = GREEN, NORMAL = GREEN).				*/

	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_STORAGE,
				       LOAD_SHED_CURRENT_VALUE,
				       &Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_CURRENT_VALUE]);

	    if (status == 0) {

		if (Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_CURRENT_VALUE] >=
		    Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_ALARM_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (CYAN);

		} else if (Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_CURRENT_VALUE] >=
			   Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_WARNING_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		} else {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		}

		sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_PROD_STORAGE][LOAD_SHED_CURRENT_VALUE]);
		XmTextSetString (Storage_current_widget, Buf);
		XtVaSetValues (Storage_current_widget,
			XmNforeground,	cur_foreground,
			XmNbackground,	cur_background,
			NULL);

	    }

/*	Determine whether the current value meets one of the threshold	*
 *	levels.  If so, set the appropriate color (ALARM = CYAN,	*
 *	WARNING = GREEN, NORMAL = GREEN).				*/

	    warn_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_RDA_RADIAL,
				   LOAD_SHED_WARNING_THRESHOLD,
				   &Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_WARNING_THRESHOLD]);

	    if (warn_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_WARNING_THRESHOLD]);
	        XmTextSetString (Rda_radial_warn_widget, Buf);

	    }

	    alarm_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_RDA_RADIAL,
				   LOAD_SHED_ALARM_THRESHOLD,
				   &Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_ALARM_THRESHOLD]);

	    if (alarm_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_ALARM_THRESHOLD]);
	        XmTextSetString (Rda_radial_alarm_widget, Buf);

	    }

/*	Determine whether the current value meets one of the threshold	*
 *	levels.  If so, set the appropriate color (ALARM = CYAN,	*
 *	WARNING = GREEN, NORMAL = GREEN).				*/

	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_RDA_RADIAL,
				       LOAD_SHED_CURRENT_VALUE,
				       &Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_CURRENT_VALUE]);

	    if (status == 0) {

		if (Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_CURRENT_VALUE] >=
		    Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_ALARM_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (CYAN);

		} else if (Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_CURRENT_VALUE] >=
			   Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_WARNING_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		} else {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		}

		sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_RDA_RADIAL][LOAD_SHED_CURRENT_VALUE]);
		XmTextSetString (Rda_radial_current_widget, Buf);
		XtVaSetValues (Rda_radial_current_widget,
			XmNforeground,	cur_foreground,
			XmNbackground,	cur_background,
			NULL);

	    }

	    warn_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_RPG_RADIAL,
				   LOAD_SHED_WARNING_THRESHOLD,
				   &Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_WARNING_THRESHOLD]);

	    if (warn_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_WARNING_THRESHOLD]);
	        XmTextSetString (Rpg_radial_warn_widget, Buf);

	    }

	    alarm_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_RPG_RADIAL,
				   LOAD_SHED_ALARM_THRESHOLD,
				   &Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_ALARM_THRESHOLD]);

	    if (alarm_status == 0) {

	        sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_ALARM_THRESHOLD]);
	        XmTextSetString (Rpg_radial_alarm_widget, Buf);

	    }

/*	Determine whether the current value meets one of the threshold	*
 *	levels.  If so, set the appropriate color (ALARM = CYAN,	*
 *	WARNING = GREEN, NORMAL = GREEN).				*/

	    status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_RPG_RADIAL,
				       LOAD_SHED_CURRENT_VALUE,
				       &Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_CURRENT_VALUE]);

	    if (status == 0) {

		if (Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_CURRENT_VALUE] >=
		    Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_ALARM_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (CYAN);

		} else if (Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_CURRENT_VALUE] >=
			   Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_WARNING_THRESHOLD]) {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		} else {

		    cur_foreground = hci_get_read_color (TEXT_FOREGROUND);
		    cur_background = hci_get_read_color (NORMAL_COLOR);

		}

		sprintf (Buf,"%d ",Load_shed_data [LOAD_SHED_CATEGORY_RPG_RADIAL][LOAD_SHED_CURRENT_VALUE]);
		XmTextSetString (Rpg_radial_current_widget, Buf);
		XtVaSetValues (Rpg_radial_current_widget,
			XmNforeground,	cur_foreground,
			XmNbackground,	cur_background,
			NULL);

	    }
	}

	XtManageChild (Form);

}

/************************************************************************
 *	Description: This function is activated when the user changes	*
 *		     a warning or alarm threshold edit box or when it	*
 *		     loses focus.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - LOAD_SHED_WARNING_THRESHOLD		*
 *			      LOAD_SHED_ALARM_THRESHOLD			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
load_shed_modify_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtPointer	data;
	char		*string;
	int		threshold;
	int		min_val;
	int		max_val;

/*	Get the category from widgets user data.			*/

	XtVaGetValues (w,
		XmNuserData,	&data,
		NULL);

/*	Extract the new value from the text widget text			*/

	string = XmTextGetString (w);

	if (strlen (string)) {

	    sscanf (string,"%d",&threshold);

	} else {

	    sprintf (Buf,"%d ", Load_shed_data [(int) data][(int) client_data]);
	    XmTextSetString (w, Buf);
	    XtFree (string);
	    return;

	}

/*	Validate the new value and display a warning if out of range.	*/

	min_val = MIN_LOAD_SHED_VALUE;
	max_val = MAX_LOAD_SHED_VALUE;

	if ((int) client_data == LOAD_SHED_WARNING_THRESHOLD) {

	    max_val = Load_shed_data [(int) data][LOAD_SHED_ALARM_THRESHOLD];

	} else {

	    min_val = Load_shed_data [(int) data][LOAD_SHED_WARNING_THRESHOLD];

	}

	if ((threshold < min_val) ||
	    (threshold > max_val)) {

	    sprintf (Buf,
		"You entered an invalid value of %d!\nThe valid range is %d to %d\n",
		threshold,
		min_val,
		max_val);

	    hci_warning_popup( Top_widget, Buf, NULL );
	    sprintf (Buf,"%d ", Load_shed_data [(int) data][(int) client_data]);
	    XmTextSetString (w,Buf);

	} else {

	    if (threshold != Load_shed_data [(int) data][(int) client_data]) {

		Load_shed_data [(int) data][(int) client_data] = threshold;

		if (!Change_flag) {

		    Change_flag = HCI_CHANGED_FLAG;

		    XtVaSetValues (Save_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Undo_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Restore_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Update_button,
			XmNsensitive,	True,
			NULL);

		}

		switch ((int) data) {

		    case LOAD_SHED_CATEGORY_RDA_RADIAL :
		    case LOAD_SHED_CATEGORY_RPG_RADIAL :

			switch ((int) client_data) {

			    case LOAD_SHED_WARNING_THRESHOLD :

				if (threshold < Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_WARNING_THRESHOLD]) {

				    Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_WARNING_THRESHOLD] = threshold;

				}
				break;

			    case LOAD_SHED_ALARM_THRESHOLD :

				if (threshold > Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_ALARM_THRESHOLD]) {

				    Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_ALARM_THRESHOLD] = threshold;

				}
				break;

			    case LOAD_SHED_CURRENT_VALUE :

				if (threshold > Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_CURRENT_VALUE]) {

				    Load_shed_data [LOAD_SHED_CATEGORY_INPUT_BUF][LOAD_SHED_CURRENT_VALUE] = threshold;

				}
				break;
			}
			break;

		}
	    }
	}

	XtFree (string);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Restore" button in the Load Shed Categories	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
load_shed_restore_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];
	sprintf( buf, "You are about to restore the load shed\nadaptation data to baseline values.\nDo you want to continue?" );
	hci_confirm_popup( Top_widget, buf, accept_load_shed_restore_callback, NULL );
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
accept_load_shed_restore_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	char	buf [128], *b;

	status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT, 
		&b, LB_ALLOC_BUF, LOAD_SHED_THRESHOLD_BASELINE_MSG_ID);
	if (status > 0) {
	    status = ORPGDA_write (ORPGDAT_LOAD_SHED_CAT, 
				b, status, LOAD_SHED_THRESHOLD_MSG_ID);
	    free (b);
	}

	if (status < 0) {

	    HCI_LE_error("Unable to restore LOAD_SHED_THRESHOLD_MSG_ID (%d)",
		status);
	    sprintf (buf,"Unable to restore baseline load shed data");

	} else {

	    Restore_flag = 1;
	    HCI_LE_log("Load Shed threshold data restored from baseline");
	    sprintf (buf,"Load Shed threshold data restored from baseline");
	    Change_flag = HCI_NOT_CHANGED_FLAG;
	    display_load_shed_category_data (-1);

	}

/*	Generate a feedback message to be displayed in the RPG		*
 *	Control/Status window.						*/

	HCI_display_feedback( buf );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Update" button in the Load Shed Categories	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
load_shed_update_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];
	sprintf( buf, "You are about to replace the baseline\nload shed threshold data values.\nDo you want to continue?" );
	hci_confirm_popup( Top_widget, buf, accept_load_shed_update_callback, NULL );
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
accept_load_shed_update_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	char	*b;

	status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT, 
		&b, LB_ALLOC_BUF, LOAD_SHED_THRESHOLD_MSG_ID);
	if (status > 0) {
	    status = ORPGDA_write (ORPGDAT_LOAD_SHED_CAT, 
			b, status, LOAD_SHED_THRESHOLD_BASELINE_MSG_ID);
	    free (b);
	}

	if (status < 0) {

	    HCI_LE_error("Unable to update LOAD_SHED_THRESHOLD_BASELINE_MSG_ID (%d)",
		status);
	    sprintf (Buf,"Unable to update baseline load shed threshold data");

	} else {

	    HCI_LE_log("Load shed threshold data baseline updated");
	   sprintf (Buf,"Load Shed threshold baseline data updated");

	}

/*	Generate a feedback message to be displayed in the RPG		*
 *	Control/Status window.						*/

	HCI_display_feedback( Buf );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Undo" button from the Load Shed Categories	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
load_shed_undo_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;

/*	Force a rearead of the load shed threshold message and update	*
 *	the display.							*/

	status = ORPGLOAD_read (LOAD_SHED_THRESHOLD_MSG_ID);

	if (status < 0) {

	    HCI_LE_error("Unable to UNDO load shed threshold changes (%d)",
		status);

	}

	display_load_shed_category_data (-1);

	XtVaSetValues (Save_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo_button,
		XmNsensitive,	False,
		NULL);

	Change_flag = HCI_NOT_CHANGED_FLAG;

	if (Unlocked_roc == HCI_YES_FLAG) {

	    XtVaSetValues (Restore_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Update_button,
		XmNsensitive,	True,
		NULL);

	}
}
