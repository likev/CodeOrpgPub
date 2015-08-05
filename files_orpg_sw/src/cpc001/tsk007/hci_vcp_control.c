/************************************************************************
 *	Module:	 hci_vcp_control.c					*
 *									*
 *	Description:  This module is used by the ORPG HCI to control	*
 *		      various VCP related activities.			*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/08 17:42:55 $
 * $Id: hci_vcp_control.c,v 1.133 2014/04/08 17:42:55 steves Exp $
 * $Revision: 1.133 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_vcp_data.h>
#include <hci_rda_adaptation_data.h>
#include <hci_wx_status.h>
#include <translate.h>

/*	Macros.								*/

#define	VALID_PERMISSION	1
#define	COMBO_NUM_COLS		4
#define	COMBO_MODE_NUM_COLS	15
#define	RDA_IN_CONTROL		2
#define	DEA_MODE_SELECT_NODE	"alg.mode_select"

/*	Local global variables						*/

Widget	Top_widget = (Widget) NULL;

/*	Widget variables.						*/

Widget	Restart_vcp_button  		= (Widget) NULL;
Widget	Velocity_low_button  		= (Widget) NULL;
Widget	Velocity_high_button  		= (Widget) NULL;
Widget	RDA_VCP_button      		= (Widget) NULL;
Widget	auto_clear_air_auto_button	= (Widget) NULL;
Widget	auto_clear_air_manual_button	= (Widget) NULL;
Widget	auto_precip_auto_button		= (Widget) NULL;
Widget	auto_precip_manual_button	= (Widget) NULL;
Widget	Lock_button			= (Widget) NULL;
Widget	mode_A_label			= (Widget) NULL;
Widget	mode_B_label			= (Widget) NULL;
Widget	default_wx_mode_combo_box	= (Widget) NULL;
Widget	default_mode_A_combo_box	= (Widget) NULL;
Widget	default_mode_B_combo_box	= (Widget) NULL;
Widget	rda_precip_combo_box		= (Widget) NULL;
Widget	rda_clear_combo_box		= (Widget) NULL;
Widget	rpg_precip_combo_box		= (Widget) NULL;
Widget	rpg_clear_combo_box		= (Widget) NULL;
Widget	exp_precip_combo_box		= (Widget) NULL;
Widget	exp_clear_combo_box		= (Widget) NULL;
Widget	rpg_frame			= (Widget) NULL;
Widget	rda_frame			= (Widget) NULL;
Widget	exp_frame			= (Widget) NULL;
Widget	previous_set_combo_box		= (Widget) NULL;

Vcp_struct	*Vcp;	/* Local VCP data buffer */
int	Rda_vcp_num [VCPMAX]; /* RDA defined VCP number lookup table */
int	Rda_vcp_type [VCPMAX]; /* RDA defined VCP type lookup table */
int	Num_rda_vcps = 0; /* RDA defined number of VCPs in VCP data buffer */
int	Rpg_vcp_num [VCPMAX]; /* RPG defined VCP number lookup table */
int	Rpg_vcp_type [VCPMAX]; /* RPG defined VCP type lookup table */
int	Num_rpg_vcps = 0; /* Number of VCPs in VCP data buffer */
int	Exp_vcp_num [VCPMAX]; /* Experimental RPG defined VCP number table */
int	Exp_vcp_type [VCPMAX]; /* Experimental RPG defined VCP type table */
int	Num_exp_vcps = 0; /* Number of VCPs in VCP data buffer */

char	Cmd [128]; /* Common buffer for building feedback string */
char	tempbuf [5];
static  int	previous_vcp_number = -999;
static	int	Unlocked_urc = 0; /* lock state for urc users */
static	int	Unlocked_roc = 0; /* lock state for roc users */
static	int	Change_RDA_vcp_flag = HCI_NO_FLAG;
static	int	Change_RPG_vcp_flag = HCI_NO_FLAG;
static	int	Change_EXP_vcp_flag = HCI_NO_FLAG;
static	int	Restart_vcp_flag = HCI_NO_FLAG;
static	short	num_precip_vcp = 0;
static	short	precip_vcp[ VCPMAX ];
static	short	num_clear_vcp = 0;
static	short	clear_vcp[ VCPMAX ];

static  int	Read_suppl_vcp_info = 1;
static 	Trans_info_t Trans_info = {0};

static  int	Deau_mode_data_update_flag = 0;
static  int	Deau_site_data_update_flag = 0;
static  int	Change_velocity_resolution_flag = 0;
static  int	Rda_vcp_updated = 1;
static  int	Current_vcp_popup_flag = 0;
static  int	Rpg_vcp_defs_popup_flag = 0;
static  int	Auto_clear_air_user_cmd = -1;
static  int	Auto_precip_user_cmd = -1;
static  int	RDA_user_selected_vcp = -1;
static  int	RPG_user_selected_vcp = -1;
static  int	EXP_user_selected_vcp = -1;
static  int	Mode_A_user_selected_vcp = -1;
static  int	Mode_B_user_selected_vcp = -1;
static	char	WX_mode_user_selected[HCI_BUF_32];

static  int     Allow_experimental = 0;

/* Structure MSF_data "command" values. */
#define CHANGE_RDA_VCP		1
#define CHANGE_RPG_VCP		2

typedef struct {

   int command;
   int new_vcp;
   int recommended_mode; 
   int recommended_vcp; 
   int auto_mode;
   int conflict;

} MSF_data;

static MSF_data msf_data;

/*	Global Variables used in this file.				*/

int     hci_activate_child (Display *d, Window w, char *cmd,
                            char *proc, char *win, int object_index);
void	hci_auto_clear_air_callback (Widget w,
		XtPointer client_data,
		XtPointer call_data);
void	hci_auto_precip_callback (Widget w,
		XtPointer client_data,
		XtPointer call_data);
void	hci_rda_vcp_update_callback();
void	hci_change_velocity_resolution( int );
void	hci_change_wx_mode_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	vcp_mode_A_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	vcp_mode_B_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	mode_status_button_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
int	default_mode_vcp_lock ();
void	update_vcp_control_properties ();
void	hci_update_rda_status_data (en_t evtcd,
		void *ptr, size_t msglen);
void	cancel_command_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	acknowledge_invalid_value (Widget w,
			XtPointer client_date, XtPointer call_data);
void	timer_proc ();
int	vcp_comp( const void *, const void * );
void	update_default_mode_vcp();
int	get_deau_permissions ( char *id, char *loca );
void	deau_mode_data_change_callback( int lb_fd, LB_id_t msg_id,
                                        int msg_len, char *group_name );
void	deau_site_data_change_callback( int lb_fd, LB_id_t msg_id,
                                        int msg_len, char *group_name );
void	suppl_vcp_change_callback( int lb_fd, LB_id_t msg_id,
                                   int msg_info, void *arg );
void	rda_vcp_change_callback( int lb_fd, LB_id_t msg_id,
                                 int msg_info, void *arg );
void	set_vcp_combo_color( Widget wid );
void	reset_vcp_combo_color( Widget wid );
void	update_clear_air_toggles();
void	update_precip_toggles();
int     Check_for_mode_conflict( int new_vcp, int *recommended_mode, 
                                 int *recommended_vcp, int *auto_mode );
void	hci_display_rda_vcp_data( Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_set_rda_vcp_popup_flag( Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_display_rpg_vcp_definitions( Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_set_rpg_vcp_defs_popup_flag( Widget w,
		XtPointer client_data, XtPointer call_data);
void	change_RDA_vcp_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_change_RDA_vcp (Widget w,
		XtPointer client_data, XtPointer call_data);
void	change_RDA_vcp ();
void	change_RPG_vcp_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_change_RPG_vcp (Widget w,
		XtPointer client_data, XtPointer call_data);
void	change_RPG_vcp ();
void	change_EXP_vcp_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_change_EXP_vcp (Widget w,
		XtPointer client_data, XtPointer call_data);
void	change_EXP_vcp ();
void	restart_vcp_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_restart_vcp (Widget w,
		XtPointer client_data, XtPointer call_data);
void	restart_vcp ();
void	vcp_control_menu_cancel (Widget w,
		XtPointer client_data, XtPointer call_data);
void	mode_conflict_vcp_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	cancel_command_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_auto_clear_air_yes (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_auto_clear_air_no (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_auto_precip_yes (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_auto_precip_no (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_vcp_mode_A_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	cancel_vcp_mode_A_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_vcp_mode_B_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	cancel_vcp_mode_B_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void    accept_wx_mode_change_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void    cancel_wx_mode_change_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void velocity_resolution_callback (Widget, XtPointer, XtPointer );
static void accept_velocity_resolution( Widget, XtPointer, XtPointer );
static void cancel_velocity_resolution( Widget, XtPointer, XtPointer );
static void change_velocity_resolution();
static void update_velocity_resolution_widget();

/************************************************************************
 *	Description: This is the main module for the VCP control task.	*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - commandline argument buffer			*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	Widget	form;
	Widget	vcp_form;
	Widget	precip_vcp_rowcol = NULL;
	Widget	clear_vcp_rowcol = NULL;
	Widget	control_rowcol;
	Widget	main_rowcol;
	Widget	left_rowcol;
	Widget	right_rowcol;
	Widget	bottom_rowcol;
	Widget	velocity_resolution;
	Widget	close_button;
	Widget	lock_widget;
	Widget	control_mode_frame;
	Widget	control_mode_rowcol;
	Widget	auto_clear_air_radio_box;
	Widget	auto_precip_radio_box;
	Widget	mode_detection_frame;
	Widget	mode_detection_rowcol;
	Widget	mode_status_button;
	Widget	default_vcp_frame;
	Widget	default_vcp_form;
	Widget	default_wx_mode_rowcol;
	Widget	default_vcp_mode_A_rowcol;
	Widget	default_vcp_mode_B_rowcol;
	Widget	restart_vcp_frame;
	Widget	temp_frame;
	Widget	button;
	Widget	label;
	int	num_vcps = -1;
	int	num_wx_modes = -1;
	int	temp_vcp;
	short	temp_vcp_array[ WXVCPMAX ];
	int	loop1,loop2;
	int	ret;
	int	n;
	int	found, defined;
	Arg	arg [10];
	char	*char_buf;
	DEAU_attr_t *d_attr;
	XmString	str;
	char title[80] = "VCP and Mode Control";
	Rdacnt	*rdacnt;
        Orpgtat_entry_t *tat = NULL;
 
/*	Initialize HCI.							*/

	HCI_init( argc, argv, HCI_VCP_TASK );

/*      Read TAT entry to get additional arguments.                     */

        tat = ORPGTAT_get_entry( argv[0] );
        if( tat != NULL ){

           Orpgtat_args_entry_t *args = (Orpgtat_args_entry_t *) ((char *) tat + tat->args);
           char *substr = NULL;

           if( (substr = strstr( args->args, "-E" )) != NULL ){

              Allow_experimental = 1;
              HCI_LE_log( 
                      "Experimental VCPs Allowed for Weather Mode Default VCPs." );  

           }

           free( tat );

        }
        
	
/*	Initialize the toolkit and create a top level shell widget	*/

	Top_widget = HCI_get_top_widget();

/*	Build widgets.							*/

	form   = XtVaCreateManagedWidget ("vcp_control_form",
				xmFormWidgetClass, Top_widget,
				XmNbackground, hci_get_read_color (BACKGROUND_COLOR1),
				NULL);

/*	If low bandwidth, display a progress meter since we are going	*
 *	to do I/O.							*/

	HCI_PM( "Reading RDA adaptation data" );		

/*	Read the RDA adaptation data block since it contains the VCP	*
 *	information.							*/

	ret = hci_read_rda_adaptation_data ();
	
	/*  Exit because I/O operations were cancelled */	    
	if (ret == RMT_CANCELLED)
		HCI_task_exit (HCI_EXIT_SUCCESS);

	if (ret <= 0) {

	    HCI_LE_error("Reading rda adaptation data failed (%d)", ret);
            HCI_task_exit (HCI_EXIT_FAIL);

	}
	
	if (HCI_is_low_bandwidth())
	   strcat(title, " <LB> ");

/*	Set args for all toggle buttons. */

	n = 0;
	XtSetArg(arg[n],XmNforeground,hci_get_read_color(TEXT_FOREGROUND));
	n++;
	XtSetArg(arg[n],XmNbackground,hci_get_read_color(BACKGROUND_COLOR1));
	n++;
	XtSetArg(arg[n],XmNorientation,XmHORIZONTAL);
	n++;

/*	Control rowcol */

	control_rowcol = XtVaCreateManagedWidget ("control_rowcol", 
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	close_button = XtVaCreateManagedWidget (" Close ",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (close_button,
		XmNactivateCallback, vcp_control_menu_cancel, NULL);

	XtVaCreateManagedWidget("                                                      ",
	  xmLabelWidgetClass,	control_rowcol,
	  XmNbackground,	hci_get_read_color(BACKGROUND_COLOR1),
	  XmNforeground,	hci_get_read_color(TEXT_FOREGROUND),
	  XmNfontList,		hci_get_fontlist(LIST),
	  NULL);
	 
	lock_widget = hci_lock_widget( control_rowcol, default_mode_vcp_lock, HCI_LOCA_URC | HCI_LOCA_ROC );

/*	Surround main body of gui with a frame. */

	temp_frame = XtVaCreateManagedWidget ("temp_frame",
		xmFrameWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginWidth,		5,
		XmNmarginHeight,	5,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Rowcol for main body of gui. */

	main_rowcol = XtVaCreateManagedWidget ("main_rowcol", 
		xmRowColumnWidgetClass,	temp_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

/*	Left side of main body of gui. */

	left_rowcol = XtVaCreateManagedWidget ("left_rowcol", 
		xmRowColumnWidgetClass,	main_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	/* Velocity Increment. */

	XtVaCreateManagedWidget ("Velocity Increment: ",
		xmLabelWidgetClass,	left_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	velocity_resolution = XmCreateRadioBox (left_rowcol,
		"velocity_resolution", arg, n);

	Velocity_high_button = XtVaCreateManagedWidget ("0.97 kts",
		xmToggleButtonWidgetClass,	velocity_resolution,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Velocity_high_button,
		XmNvalueChangedCallback, velocity_resolution_callback,
		(XtPointer) HCI_VELOCITY_RESOLUTION_HIGH);

	Velocity_low_button = XtVaCreateManagedWidget ("1.94 kts",
		xmToggleButtonWidgetClass,	velocity_resolution,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Velocity_low_button,
		XmNvalueChangedCallback, velocity_resolution_callback,
		(XtPointer) HCI_VELOCITY_RESOLUTION_LOW);

	update_velocity_resolution_widget();

	XtManageChild(velocity_resolution);

	/* End Velocity Increment. */
	/* Start of RDA VCPs. */

        /* Loop through all RDA vcps and fill appropriate arrays.  We need
	   to do this before creating widgets. */

	num_precip_vcp = 0;
	num_clear_vcp = 0;

        for (loop1=1;loop1<VCPMAX;loop1++)
        {
          found = 0;

          Vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (loop1-1);

          if ((Vcp->vcp_num != 0) && (Vcp->vcp_num < 1000))
          {
          	defined = hci_rda_adapt_where_defined (VCP_RDA_DEFINED,
                                                   (int) Vcp->vcp_num);
            	if( !defined )
            	{
              		continue;
            	}

              		for (loop2=0;loop2<WXVCPMAX;loop2++)
              		{
                		if (hci_rda_adapt_wxmode (1,loop2) == Vcp->vcp_num)
                		{
                  			/* Precip VCP */
		  			if( !ORPGVCP_is_vcp_experimental( Vcp->vcp_num ) )
		  			{
                  				precip_vcp[ num_precip_vcp ] = Vcp->vcp_num;
                  				num_precip_vcp++;
                  				found = 1;
		  			}
                		}
                		else if (hci_rda_adapt_wxmode (0,loop2) == Vcp->vcp_num)
                		{
                  			/* Clear Air VCP */
		  			if( !ORPGVCP_is_vcp_experimental( Vcp->vcp_num ) )
		  			{
                  				clear_vcp[ num_clear_vcp ] = Vcp->vcp_num;
                  				num_clear_vcp++;
                  				found = 1;
                			}
              			}
            		}

          } /* If vcp is between 0 and 1000 */

        } /* Loop i to VCPMAX */

        /* Sort vcp arrays so gui buttons will be ordered. */

        qsort(precip_vcp, num_precip_vcp, sizeof(short),vcp_comp);
        qsort(clear_vcp, num_clear_vcp, sizeof(short),vcp_comp);

/*	Make a container and create buttons for each VCP in the VCP	*
 *	data buffer.  These buttons will be used to command the RDA	*
 *	to a local VCP.							*/

	rda_frame = XtVaCreateManagedWidget ("rda_vcp_frame",
		xmFrameWidgetClass,	left_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginWidth,		5,
		XmNmarginHeight,	5,
		NULL);

	vcp_form = XtVaCreateManagedWidget ("rda_vcp_form",
		xmFormWidgetClass,	rda_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNverticalSpacing,	1,
		NULL);

	label = XtVaCreateManagedWidget ("Change to RDA VCP",
		xmLabelWidgetClass,	rda_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	precip_vcp_rowcol = XtVaCreateManagedWidget ("precip_vcp_rowcol",
		xmRowColumnWidgetClass,	vcp_form,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	XtVaCreateManagedWidget ("Precipitation Mode:     ",
		xmLabelWidgetClass,	precip_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	clear_vcp_rowcol = XtVaCreateManagedWidget ("clear_vcp_rowcol",
		xmRowColumnWidgetClass,	vcp_form,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		precip_vcp_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	XtVaCreateManagedWidget ("Clear Air Mode:         ",
		xmLabelWidgetClass,	clear_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

        /* Create combo list/callbacks for the RDA vcps. */

	rda_precip_combo_box = XtVaCreateManagedWidget( "rda_combo_box",
		xmComboBoxWidgetClass,	precip_vcp_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist( LIST ),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL );

	XtAddCallback( rda_precip_combo_box,
		       XmNselectionCallback, change_RDA_vcp_callback,
		       NULL );

	rda_clear_combo_box = XtVaCreateManagedWidget( "rda_combo_box",
		xmComboBoxWidgetClass,	clear_vcp_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist( LIST ),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL );

	XtAddCallback( rda_clear_combo_box,
		       XmNselectionCallback, change_RDA_vcp_callback,
		       NULL );

	Num_rda_vcps = 0;

        for( loop1 = num_precip_vcp - 1; loop1 >= 0; loop1-- )
        {
	  Rda_vcp_num [Num_rda_vcps] = precip_vcp[ loop1 ];
	  Rda_vcp_type [Num_rda_vcps] = PRECIPITATION_MODE;
	  sprintf( tempbuf,"%d", precip_vcp[ loop1 ] );
	  str = XmStringCreateLocalized( tempbuf );
	  XmComboBoxAddItem( rda_precip_combo_box, str, 1, True );
	  XmComboBoxSetItem( rda_precip_combo_box, str );
	  XmStringFree( str );
          Num_rda_vcps++;
        }

        for( loop1 = num_clear_vcp - 1; loop1 >= 0; loop1-- )
        {
	  Rda_vcp_num [Num_rda_vcps] = clear_vcp[ loop1 ];
	  Rda_vcp_type [Num_rda_vcps] = CLEAR_AIR_MODE;
	  sprintf( tempbuf,"%d", clear_vcp[ loop1 ] );
	  str = XmStringCreateLocalized( tempbuf );
	  XmComboBoxAddItem( rda_clear_combo_box, str, 1, True );
	  XmComboBoxSetItem( rda_clear_combo_box, str );
	  XmStringFree( str );
          Num_rda_vcps++;
        }

	/* End of RDA VCPs. */
	/* Start of RPG VCPs. */

        /* Loop through all RPG vcps and fill appropriate arrays.  We need
	   to do this before creating widgets. */

	num_precip_vcp = 0;
	num_clear_vcp = 0;

        for (loop1=1;loop1<VCPMAX;loop1++)
        {
          found = 0;

          Vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (loop1-1);

          if ((Vcp->vcp_num != 0) && (Vcp->vcp_num < 1000))
          {
            	defined = hci_rda_adapt_where_defined (VCP_RPG_DEFINED,
                                                   (int) Vcp->vcp_num);
            	if( !defined )
            	{
              		continue;
            	}

              		for (loop2=0;loop2<WXVCPMAX;loop2++)
              		{
                		if (hci_rda_adapt_wxmode (1,loop2) == Vcp->vcp_num)
                		{
					if( !ORPGVCP_is_vcp_experimental( Vcp->vcp_num ) )
					{
                  				/* Precip VCP */
                  				precip_vcp[ num_precip_vcp ] = Vcp->vcp_num;
                  				num_precip_vcp++;
                  				found = 1;
					}
                		}
                		else if (hci_rda_adapt_wxmode (0,loop2) == Vcp->vcp_num)
                		{
					if( !ORPGVCP_is_vcp_experimental( Vcp->vcp_num ) )
					{
                  				/* Clear Air VCP */
                  				clear_vcp[ num_clear_vcp ] = Vcp->vcp_num;
                  				num_clear_vcp++;
                  				found = 1;
                			}
              			}
            		}

          } /* If vcp is between 0 and 1000 */

       	} /* Loop i to VCPMAX */

        /* Sort vcp arrays so gui buttons will be ordered. */

        qsort(precip_vcp, num_precip_vcp, sizeof(short),vcp_comp);
        qsort(clear_vcp, num_clear_vcp, sizeof(short),vcp_comp);

/*	Make a container and create buttons for each VCP in the VCP	*
 *	data buffer.  These buttons will be used to command the RDA	*
 *	to a remote VCP.						*/

	rpg_frame = XtVaCreateManagedWidget ("rda_vcp_frame",
		xmFrameWidgetClass,	left_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginWidth,		5,
		XmNmarginHeight,	5,
		NULL);

	vcp_form = XtVaCreateManagedWidget ("rpg_vcp_form",
		xmFormWidgetClass,	rpg_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNverticalSpacing,	1,
		NULL);

	label = XtVaCreateManagedWidget ("Download VCP from RPG",
		xmLabelWidgetClass,	rpg_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	precip_vcp_rowcol = XtVaCreateManagedWidget ("precip_vcp_rowcol",
		xmRowColumnWidgetClass,	vcp_form,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	XtVaCreateManagedWidget ("Precipitation Mode:     ",
		xmLabelWidgetClass,	precip_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	clear_vcp_rowcol = XtVaCreateManagedWidget ("clear_vcp_rowcol",
		xmRowColumnWidgetClass,	vcp_form,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		precip_vcp_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	XtVaCreateManagedWidget ("Clear Air Mode:         ",
		xmLabelWidgetClass,	clear_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	rpg_precip_combo_box = XtVaCreateManagedWidget( "rpg_combo_box",
		xmComboBoxWidgetClass,	precip_vcp_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist( LIST ),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL );

	XtAddCallback( rpg_precip_combo_box,
		       XmNselectionCallback, change_RPG_vcp_callback,
		       NULL );

	rpg_clear_combo_box = XtVaCreateManagedWidget( "rpg_combo_box",
		xmComboBoxWidgetClass,	clear_vcp_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist( LIST ),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL );

	XtAddCallback( rpg_clear_combo_box,
		       XmNselectionCallback, change_RPG_vcp_callback,
		       NULL );

	Num_rpg_vcps = 0;

        for( loop1 = num_precip_vcp - 1; loop1 >= 0; loop1-- )
        {
	  Rpg_vcp_num [Num_rpg_vcps] = precip_vcp[ loop1 ];
	  Rpg_vcp_type [Num_rpg_vcps] = PRECIPITATION_MODE;
	  sprintf( tempbuf,"%d", precip_vcp[ loop1 ] );
	  str = XmStringCreateLocalized( tempbuf );
	  XmComboBoxAddItem( rpg_precip_combo_box, str, 1, True );
	  XmComboBoxSetItem( rpg_precip_combo_box, str );
	  XmStringFree( str );
          Num_rpg_vcps++;
        }

        for( loop1 = num_clear_vcp - 1; loop1 >= 0; loop1-- )
        {
	  Rpg_vcp_num [Num_rpg_vcps] = clear_vcp[ loop1 ];
	  Rpg_vcp_type [Num_rpg_vcps] = CLEAR_AIR_MODE;
	  sprintf( tempbuf,"%d", clear_vcp[ loop1 ] );
	  str = XmStringCreateLocalized( tempbuf );
	  XmComboBoxAddItem( rpg_clear_combo_box, str, 1, True );
	  XmComboBoxSetItem( rpg_clear_combo_box, str );
	  XmStringFree( str );
          Num_rpg_vcps++;
        }

	/* End of RPG VCPs. */
	/* Start of Experimental VCPs. */

        /* Loop through all Experimental vcps and fill appropriate arrays. 
	   We need to do this before creating widgets. */

        num_precip_vcp = 0;
        num_clear_vcp = 0;

        /* Loop through all RPG vcps and fill appropriate arrays. */

        for (loop1=1;loop1<VCPMAX;loop1++)
        {
          found = 0;

          Vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (loop1-1);

          if ((Vcp->vcp_num != 0) && (Vcp->vcp_num < 1000))
          {
          	defined = hci_rda_adapt_where_defined (VCP_RPG_DEFINED,
                                                   (int) Vcp->vcp_num);
            	if( !defined )
            	{
              		continue;
            	}

            	for (loop2=0;loop2<WXVCPMAX;loop2++)
            	{
                	if (hci_rda_adapt_wxmode (1,loop2) == Vcp->vcp_num)
                	{
                  		/* Precip VCP */
		  		if( ORPGVCP_is_vcp_experimental( Vcp->vcp_num ) )
		  		{
                  			precip_vcp[ num_precip_vcp ] = Vcp->vcp_num;
                  			num_precip_vcp++;
                  			found = 1;
		  		}
                	}
                	else if (hci_rda_adapt_wxmode (0,loop2) == Vcp->vcp_num)
                	{
		  		if( ORPGVCP_is_vcp_experimental( Vcp->vcp_num ) )
		  		{
                  			/* Clear Air VCP */
                  			clear_vcp[ num_clear_vcp ] = Vcp->vcp_num;
                  			num_clear_vcp++;
                  			found = 1;
		  		}
                	}
            	}

          } /* If vcp is between 0 and 1000 */

        } /* Loop i to VCPMAX */

        /* Sort vcp arrays so gui buttons will be ordered. */

        qsort(precip_vcp, num_precip_vcp, sizeof(short),vcp_comp);
        qsort(clear_vcp, num_clear_vcp, sizeof(short),vcp_comp);

	if( num_precip_vcp || num_clear_vcp ){

        	exp_frame = XtVaCreateManagedWidget ("exp_vcp_frame",
                	xmFrameWidgetClass,     left_rowcol,
                	XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                	XmNmarginWidth,         5,
                	XmNmarginHeight,        5,
                	NULL);

        	vcp_form = XtVaCreateManagedWidget ("exp_vcp_form",
                	xmFormWidgetClass,      exp_frame,
                	XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                	XmNverticalSpacing,     1,
                	NULL);

        	label = XtVaCreateManagedWidget ("Download Experimental VCP from RPG",
                	xmLabelWidgetClass,     exp_frame,
                	XmNchildType,           XmFRAME_TITLE_CHILD,
                	XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                	XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                	XmNfontList,            hci_get_fontlist (LIST),
                	XmNalignment,           XmALIGNMENT_CENTER,
                	XmNtopAttachment,       XmATTACH_FORM,
                	XmNleftAttachment,      XmATTACH_FORM,
                	XmNrightAttachment,     XmATTACH_FORM,
                	NULL);

               	precip_vcp_rowcol = XtVaCreateManagedWidget ("precip_vcp_rowcol",
                       	xmRowColumnWidgetClass, vcp_form,
                       	XmNbackground,          hci_get_read_color (BLACK),
                       	XmNorientation,         XmHORIZONTAL,
                       	XmNpacking,             XmPACK_TIGHT,
                       	XmNnumColumns,          1,
               		XmNtopAttachment,       XmATTACH_WIDGET,
                       	XmNtopWidget,           label,
                       	XmNleftAttachment,      XmATTACH_FORM,
                       	XmNspacing,             1,
                       	XmNmarginWidth,         1,
                       	XmNmarginHeight,        1,
                       	NULL);

               	XtVaCreateManagedWidget ("Precipitation: ",
                       	xmLabelWidgetClass,     precip_vcp_rowcol,
                       	XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                       	XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                       	XmNfontList,            hci_get_fontlist (LIST),
                       	XmNalignment,           XmALIGNMENT_CENTER,
                       	NULL);

		clear_vcp_rowcol = XtVaCreateManagedWidget ("clear_vcp_rowcol",
			xmRowColumnWidgetClass, vcp_form,
			XmNbackground,          hci_get_read_color (BLACK),
			XmNorientation,         XmHORIZONTAL,
			XmNpacking,             XmPACK_TIGHT,
			XmNnumColumns,          1,
			XmNtopAttachment,       XmATTACH_WIDGET,
			XmNtopWidget,           precip_vcp_rowcol,
			XmNleftAttachment,      XmATTACH_FORM,
			XmNspacing,             1,
			XmNmarginWidth,         1,
			XmNmarginHeight,        1,
			NULL);
	
       	        XtVaCreateManagedWidget ("Clear Air:     ",
               	        xmLabelWidgetClass,     clear_vcp_rowcol,
                       	XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
       	                XmNfontList,    hci_get_fontlist (LIST),
               	        XmNalignment,   XmALIGNMENT_CENTER,
                       	NULL);

        	/* Create combo list/callbacks for the EXP vcps. */

		exp_precip_combo_box = XtVaCreateManagedWidget( "exp_combo_box",
			xmComboBoxWidgetClass,	precip_vcp_rowcol,
			XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
			XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
			XmNfontList,		hci_get_fontlist( LIST ),
			XmNcomboBoxType,	XmDROP_DOWN_LIST,
			XmNcolumns,		COMBO_NUM_COLS,
			XmNvisibleItemCount,	COMBO_NUM_COLS,
			NULL );

		XtAddCallback( exp_precip_combo_box,
			       XmNselectionCallback, change_EXP_vcp_callback,
			       NULL );

		exp_clear_combo_box = XtVaCreateManagedWidget( "exp_combo_box",
			xmComboBoxWidgetClass,	clear_vcp_rowcol,
			XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
			XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
			XmNfontList,		hci_get_fontlist( LIST ),
			XmNcomboBoxType,	XmDROP_DOWN_LIST,
			XmNcolumns,		COMBO_NUM_COLS,
			XmNvisibleItemCount,	COMBO_NUM_COLS,
			NULL );

		XtAddCallback( exp_clear_combo_box,
			       XmNselectionCallback, change_EXP_vcp_callback,
			       NULL );

		Num_exp_vcps = 0;

        	for( loop1 = num_precip_vcp - 1; loop1 >= 0; loop1-- )
        	{
		  Exp_vcp_num [Num_exp_vcps] = precip_vcp[ loop1 ];
		  Exp_vcp_num [Num_exp_vcps] = PRECIPITATION_MODE;
		  sprintf( tempbuf,"%d", precip_vcp[ loop1 ] );
		  str = XmStringCreateLocalized( tempbuf );
		  XmComboBoxAddItem( exp_precip_combo_box, str, 1, True );
		  XmComboBoxSetItem( exp_precip_combo_box, str );
		  XmStringFree( str );
		  Num_exp_vcps++;
       		}

        	for( loop1 = num_clear_vcp - 1; loop1 >= 0; loop1-- )
        	{
		  Exp_vcp_num [Num_exp_vcps] = clear_vcp[ loop1 ];
		  Exp_vcp_num [Num_exp_vcps] = CLEAR_AIR_MODE;
		  sprintf( tempbuf,"%d", clear_vcp[ loop1 ] );
		  str = XmStringCreateLocalized( tempbuf );
		  XmComboBoxAddItem( exp_clear_combo_box, str, 1, True );
		  XmComboBoxSetItem( exp_clear_combo_box, str );
		  XmStringFree( str );
		  Num_exp_vcps++;
		}
	}

/*	End of left side of main body of gui. */
/*	Start of right side of main body of gui. */

/*	Right side of main body of gui. */

	right_rowcol = XtVaCreateManagedWidget ("right_rowcol", 
		xmRowColumnWidgetClass,	main_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Start of Control Mode Automation. */

	control_mode_frame = XtVaCreateManagedWidget ("control_mode_frame",
		xmFrameWidgetClass,	right_rowcol,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		left_rowcol,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Control Mode Automation",
		xmLabelWidgetClass,	control_mode_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	control_mode_rowcol = XtVaCreateManagedWidget ("control_mode_rowcol",
		xmRowColumnWidgetClass,	control_mode_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		NULL);

/*	Add Clear Air toggle button set.	*/

	label = XtVaCreateManagedWidget ("Clear Air Switching:",
		xmLabelWidgetClass,	control_mode_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	auto_clear_air_radio_box = XmCreateRadioBox (control_mode_rowcol,
		"auto_clear_air_radio_box", arg, n);

	auto_clear_air_auto_button = XtVaCreateManagedWidget (" Auto ",
		xmToggleButtonWidgetClass,	auto_clear_air_radio_box,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (GREEN),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (auto_clear_air_auto_button,
		XmNvalueChangedCallback, hci_auto_clear_air_callback,
		(XtPointer) AUTO_SWITCH);

	auto_clear_air_manual_button = XtVaCreateManagedWidget (" Manual ",
		xmToggleButtonWidgetClass,	auto_clear_air_radio_box,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (auto_clear_air_manual_button,
		XmNvalueChangedCallback, hci_auto_clear_air_callback,
		(XtPointer) MANUAL_SWITCH);

	XtManageChild(auto_clear_air_radio_box);

/*	Add Precipitation toggle button set.	*/

	label = XtVaCreateManagedWidget ("Precipitation Switching:",
		xmLabelWidgetClass,	control_mode_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	auto_precip_radio_box = XmCreateRadioBox (control_mode_rowcol,
		"auto_precip_radio_box", arg, n);

	auto_precip_auto_button = XtVaCreateManagedWidget (" Auto ",
		xmToggleButtonWidgetClass,	auto_precip_radio_box,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (GREEN),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (auto_precip_auto_button,
		XmNvalueChangedCallback, hci_auto_precip_callback,
		(XtPointer) AUTO_SWITCH);

	auto_precip_manual_button = XtVaCreateManagedWidget (" Manual ",
		xmToggleButtonWidgetClass,	auto_precip_radio_box,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (auto_precip_manual_button,
		XmNvalueChangedCallback, hci_auto_precip_callback,
		(XtPointer) MANUAL_SWITCH);

	XtManageChild(auto_precip_radio_box);

/*	Add button to launch hci_mode_status task.	*/

	mode_detection_rowcol = XtVaCreateManagedWidget ("mode_detect_rowcol", 
		xmRowColumnWidgetClass,	control_mode_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_mode_frame,
		NULL);

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	mode_detection_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	mode_detection_frame = XtVaCreateManagedWidget ("mode_detection_frame",
		xmFrameWidgetClass,	mode_detection_rowcol,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginHeight,	5,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

	label = XtVaCreateManagedWidget ("Mode Automation Status ",
		xmLabelWidgetClass,	mode_detection_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	mode_status_button = XtVaCreateManagedWidget (" View/Edit ",
		xmPushButtonWidgetClass,	mode_detection_frame,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (mode_status_button,
		XmNactivateCallback, mode_status_button_callback, NULL);

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	mode_detection_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	End of Control Mode Automation. */
/*	Start of Default VCP info. */

	default_vcp_frame = XtVaCreateManagedWidget ("default_vcp_frame",
		xmFrameWidgetClass,	right_rowcol,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		control_mode_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		rpg_frame,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginWidth,		5,
		XmNmarginHeight,	5,
		NULL);

	label = XtVaCreateManagedWidget ("Select Defaults",
		xmLabelWidgetClass,	default_vcp_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	default_vcp_form = XtVaCreateManagedWidget ("default_vcp_form",
		xmFormWidgetClass,	default_vcp_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNverticalSpacing,	1,
		NULL);

	default_wx_mode_rowcol = XtVaCreateManagedWidget ("wx_mode_rowcol",
		xmRowColumnWidgetClass,	default_vcp_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rpg_frame,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	label = XtVaCreateManagedWidget ("Mode: ",
		xmLabelWidgetClass,	default_wx_mode_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Build drop-down list of default weather modes defined in
	DEAU library. */

	if( DEAU_get_attr_by_id(  ORPGSITE_DEA_WX_MODE, &d_attr ) < 0 )
	{
	  HCI_LE_error( "Unable to get Default Wx Mode attr" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	num_wx_modes = DEAU_get_allowable_values( d_attr, &char_buf );
	if( num_wx_modes < 0 )
	{
	  HCI_LE_error( "Unable to get Default Wx Modes vals" );
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	default_wx_mode_combo_box = XtVaCreateManagedWidget( "wxmode_combo_box",
		xmComboBoxWidgetClass,	default_wx_mode_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist( LIST ),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_MODE_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL );

	XtAddCallback( default_wx_mode_combo_box,
		       XmNselectionCallback, hci_change_wx_mode_callback,
		       NULL );

        for( loop1 = 0; loop1 < num_wx_modes; loop1++ )
        {
	  str = XmStringCreateLocalized( char_buf );
	  XmComboBoxAddItem( default_wx_mode_combo_box, str, 1, True );
	  XmComboBoxSetItem( default_wx_mode_combo_box, str );
	  XmStringFree( str );
	  char_buf = char_buf + strlen( char_buf ) + 1;
        }

	/* Next, read RDACNT adaptation data.   */

	ret = ORPGDA_read( ORPGDAT_ADAPTATION, (void **) &rdacnt,
			   LB_ALLOC_BUF,RDACNT);

	if( ret < 0 )
	{
	  HCI_LE_error("Unable to read RDACNT adapt info");
	}

	/* Create buttons for weather mode A    */

	default_vcp_mode_A_rowcol = XtVaCreateManagedWidget ("default_vcp_mode_A_rowcol",
		xmRowColumnWidgetClass, default_vcp_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		default_wx_mode_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		rpg_frame,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	label = XtVaCreateManagedWidget ("VCP Mode A:",
		xmLabelWidgetClass,	default_vcp_mode_A_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	default_mode_A_combo_box = XtVaCreateManagedWidget( "mode_A_combo",
		xmComboBoxWidgetClass,	default_vcp_mode_A_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL);

	XtAddCallback (default_mode_A_combo_box,
		XmNselectionCallback,
		vcp_mode_A_callback,
		NULL );

	/* Loop through RPG vcp list. For each vcp that isn't
	   zero and is defined at the RDA, fill the temporary
	   array. The array will be sorted so the vcp list
	   appears ordered on the gui. */

	num_vcps = 0;

	for( loop1 = 0; loop1 < WXVCPMAX; loop1++ )
	{
	  temp_vcp = rdacnt -> rdcwxvcp[PRECIPITATION_MODE-1][loop1];

	  if( temp_vcp != 0 )
	  {
	    for(loop2=0;loop2<VCPMAX;loop2++)
	    {
	      if( temp_vcp == 
                  (rdacnt->rdc_where_defined[VCP_RPG_DEFINED][loop2] & 
				ORPGVCP_VCP_MASK) )
	      {

                /* This controls whether or not Experimental VCPs can also be default
                   VCP for weather mode. */
		if( (Allow_experimental) || !(rdacnt->rdc_where_defined[VCP_RPG_DEFINED][loop2] & 
				ORPGVCP_EXPERIMENTAL_VCP) )
		{
	        	temp_vcp_array[ num_vcps ] = temp_vcp;
			num_vcps++;
		}
	      }
	    }
	  }
	}

	/* Sort using C's quick sort algorithm */

	qsort( temp_vcp_array, num_vcps, sizeof(short), vcp_comp );

	for( loop1 = num_vcps - 1; loop1 >=0; loop1-- )
	{
	  sprintf (tempbuf,"%d",temp_vcp_array[ loop1 ]);
	  str = XmStringCreateLocalized (tempbuf);
	  XmComboBoxAddItem( default_mode_A_combo_box, str, 1, True );
	  XmComboBoxSetItem( default_mode_A_combo_box, str );
	  XmStringFree (str);
	}

	/* Create buttons for weather mode B    */

	default_vcp_mode_B_rowcol = XtVaCreateManagedWidget ("default_vcp_mode_B_rowcol",
		xmRowColumnWidgetClass, default_vcp_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		default_vcp_mode_A_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		rpg_frame,
		XmNspacing,		1,
		XmNmarginWidth,		1,
		XmNmarginHeight,	1,
		NULL);

	label = XtVaCreateManagedWidget ("VCP Mode B:",
		xmLabelWidgetClass,	default_vcp_mode_B_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	default_mode_B_combo_box = XtVaCreateManagedWidget( "mode_B_combo",
		xmComboBoxWidgetClass,	default_vcp_mode_B_rowcol,
		XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNforeground,		hci_get_read_color( TEXT_FOREGROUND ),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		COMBO_NUM_COLS,
		XmNvisibleItemCount,	COMBO_NUM_COLS,
		NULL);

	XtAddCallback (default_mode_B_combo_box,
		XmNselectionCallback,
		vcp_mode_B_callback,
		NULL );

	/* Loop through RPG vcp list. For each vcp that isn't
	   zero and is defined at the RDA, fill the temporary
	   array. The array will be sorted so the vcp list
	   appears ordered on the gui. */

	num_vcps = 0;

	for( loop1 = 0; loop1 < WXVCPMAX; loop1++ )
	{
	  temp_vcp = rdacnt -> rdcwxvcp[CLEAR_AIR_MODE-1][loop1];

	  if( temp_vcp != 0 && temp_vcp <= VCP_MAX_MPDA )
	  {
	    for(loop2=0;loop2<VCPMAX;loop2++)
	    {
	      if( temp_vcp == 
                  (rdacnt->rdc_where_defined[VCP_RPG_DEFINED][loop2] & ORPGVCP_VCP_MASK) )
	      {
                /* This controls whether or not Experimental VCPs can also be default
                   VCP for weather mode. */
		if( (Allow_experimental) || !(rdacnt->rdc_where_defined[VCP_RPG_DEFINED][loop2] & ORPGVCP_EXPERIMENTAL_VCP) )
		{
	        	temp_vcp_array[ num_vcps ] = temp_vcp;
			num_vcps++;
		}
	      }
	    }
	  }
	}

	/* Sort using C's quick sort algorithm */

	qsort( temp_vcp_array, num_vcps, sizeof(short), vcp_comp );

	for( loop1 = num_vcps - 1; loop1 >= 0; loop1-- )
	{
	  sprintf (tempbuf,"%d",temp_vcp_array[ loop1 ]);
	  str = XmStringCreateLocalized (tempbuf);
	  XmComboBoxAddItem( default_mode_B_combo_box, str, 1, True );
	  XmComboBoxSetItem( default_mode_B_combo_box, str );
	  XmStringFree( str );
	}

	/* Free space allocated for default vcp weather mode block */

	if( rdacnt != NULL ){ free( rdacnt ); }

/*	Start of Restart VCP. */

	restart_vcp_frame = XtVaCreateManagedWidget ("restart_vcp_mode_frame",
		xmFrameWidgetClass,	right_rowcol,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		left_rowcol,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("VCP Control",
		xmLabelWidgetClass,	restart_vcp_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

        Restart_vcp_button = XtVaCreateManagedWidget ("  Restart VCP ",
                xmPushButtonWidgetClass,        restart_vcp_frame,
                XmNbackground,                  hci_get_read_color (BUTTON_BACKGROUND),
                XmNforeground,                  hci_get_read_color (BUTTON_FOREGROUND),
                XmNfontList,                    hci_get_fontlist (LIST),
                XmNalignment,                   XmALIGNMENT_CENTER,
                NULL);

        XtAddCallback (Restart_vcp_button,
                XmNactivateCallback, restart_vcp_callback, NULL);

	
/*	End of Default VCP info. */
/*	End of right side of main body of gui. */
/*	Start of bottom row. */

	bottom_rowcol = XtVaCreateManagedWidget ("bottom_rowcol", 
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		main_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	RDA_VCP_button = XtVaCreateManagedWidget (" View Current VCP ",
		xmPushButtonWidgetClass,	bottom_rowcol,
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNalignment,			XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (RDA_VCP_button,
		XmNactivateCallback, hci_set_rda_vcp_popup_flag, NULL);

	button = XtVaCreateManagedWidget (" View RPG VCP Definitions ",
		xmPushButtonWidgetClass,	bottom_rowcol,
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNalignment,			XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_set_rpg_vcp_defs_popup_flag, NULL );

	label = XtVaCreateManagedWidget ("    ",
		xmLabelWidgetClass,	bottom_rowcol,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Register callback function for DEA adaptation data. If data	*
 *	changes, then deau_change_data_callback is called.		*/

	ret = DEAU_UN_register( DEA_MODE_SELECT_NODE,
                                   (void *) deau_mode_data_change_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("mode change callback registration failed (%d)", ret);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	ret = DEAU_UN_register( SITE_INFO_DEA_NAME,
                                   (void *) deau_site_data_change_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("site change callback registration failed (%d)", ret);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Register callback function for UN Notification for 		*
 *	ORPGDAT_SUPPL_VCP_INFO.						*/

	ORPGDA_write_permission( ORPGDAT_SUPPL_VCP_INFO );

	ret = ORPGDA_UN_register( ORPGDAT_SUPPL_VCP_INFO, 
	                          TRANS_ACTIVE_MSG_ID,
	                          suppl_vcp_change_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("suppl vcp callback registration failed (%d)", ret);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Register callback function for UN Notification for 		*
 *	ORPGDAT_RDA_VCP_DATA.						*/

	ORPGDA_write_permission( ORPGDAT_RDA_VCP_DATA );

	ret = ORPGDA_UN_register( ORPGDAT_RDA_VCP_DATA, 
	                          ORPGDAT_RDA_VCP_MSG_ID,
	                          rda_vcp_change_callback );

	if( ret < 0 )
	{
	  HCI_LE_error("rda vcp change callback registration failed (%d)", ret);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Pop up main screen after all initial I/O has occurred		 */

	update_vcp_control_properties();
	update_clear_air_toggles();
	update_precip_toggles();
	update_default_mode_vcp();
		
	XtRealizeWidget (Top_widget);		
	
	XtPopup (Top_widget, XtGrabNone);

/*	Start HCI loop.							*/

	HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This Function is the callback for all of the VCP	*
 *		     control buttons in the CHANGE to RDA VCP container	*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - VCP number 				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
change_RDA_vcp_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		*temp_buf;
	char		buf[ HCI_BUF_256 ];
	int		SZ2_vcp_selected = 0;
	unsigned char	CMD_enabled_flag = 0;
	XmComboBoxCallbackStruct *cbs;

	/* Extract vcp from callback struct. */

	cbs = ( XmComboBoxCallbackStruct * ) call_data;
	XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );
	RDA_user_selected_vcp = atoi( temp_buf );
	XtFree( temp_buf );

	/* Log message. */

	HCI_LE_log("Change to RDA VCP %d selected", RDA_user_selected_vcp);

        /* Check MSF for recommended mode against mode operator wants to change to. */

        msf_data.new_vcp = RDA_user_selected_vcp;
        msf_data.command = CHANGE_RDA_VCP;
        msf_data.conflict = Check_for_mode_conflict( msf_data.new_vcp, &msf_data.recommended_mode, 
                	                            &msf_data.recommended_vcp, 
                        	                    &msf_data.auto_mode );
        HCI_LE_log("MSF Recommended Mode: %d, MSF Recommended Mode default VCP: %d, Auto Mode: %d",
               msf_data.recommended_mode,
               msf_data.recommended_vcp,
               msf_data.auto_mode );

	/* Create popup message. */

        if( ORPGVCP_is_SZ2_vcp( RDA_user_selected_vcp ) > 0 )
        {
          SZ2_vcp_selected = 1;
          /* This is a SZ2 VCP, is CMD enabled as well? */
          ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                 ORPGINFO_STATEFL_GET, &CMD_enabled_flag );
        }

        if( SZ2_vcp_selected && (int) !CMD_enabled_flag )
        {
	  sprintf( buf, "You are about to download VCP %d, but CMD\nis currently disabled. CMD provides the best\nVCP %d data quality and will be automatically\nenabled along with VCP %d if you select Yes.\nDownload VCP %d and enable CMD?", RDA_user_selected_vcp, RDA_user_selected_vcp, RDA_user_selected_vcp, RDA_user_selected_vcp );
        }
        else
        {
	  sprintf( buf, "You are about to change to VCP %d stored as\nadaptation data at the RDA.  It will take\neffect at the start of the next volume scan.\nDo you want to continue?", RDA_user_selected_vcp );
        }

        if( msf_data.conflict && msf_data.auto_mode )
        {
	  hci_confirm_popup( Top_widget, buf, mode_conflict_vcp_callback, NULL );
	}
        else
        {
	  hci_confirm_popup( Top_widget, buf, accept_change_RDA_vcp, NULL );
	}

	update_vcp_control_properties();
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "Yes" button from the CHANGE to RDA VCP	*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - VCP number		 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/


void
accept_change_RDA_vcp (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Change_RDA_vcp_flag = HCI_YES_FLAG;
}


void change_RDA_vcp()
{
        int ret;
        User_commanded_vcp_t cmded_vcp;

        cmded_vcp.last_vcp_commanded = RDA_user_selected_vcp;
        ret = ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO, (char *) &cmded_vcp,
                            sizeof(User_commanded_vcp_t), USER_COMMANDED_VCP_MSG_ID );
        if( ret < 0 )
           HCI_LE_log("ORPGDA_write( SUPPL_VCP_INFO ) Failed: %d", ret );

	HCI_PM( "Sending RDA select VCP command" );		

	ORPGRDA_send_cmd (COM4_RDACOM,
			  (int) HCI_VCP_INITIATED_RDA_CTRL_CMD,
			  CRDA_SELECT_VCP,
			  (int) RDA_user_selected_vcp,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  NULL);

	sprintf (Cmd,"Requesting change to RDA VCP %d", RDA_user_selected_vcp);

	HCI_display_feedback( Cmd );
}

/************************************************************************
 *	Description: This Function is the callback for all of the VCP	*
 *		     control buttons in the DOWNLOAD VCP from RPG	*
 *		     container.						*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - VCP number 				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
change_RPG_vcp_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		*temp_buf;
	char		buf[ HCI_BUF_256 ];
	int		SZ2_vcp_selected = 0;
	unsigned char	CMD_enabled_flag = 0;
	XmComboBoxCallbackStruct *cbs;

	/* Extract vcp from the client data. */

	cbs = ( XmComboBoxCallbackStruct * ) call_data;
	XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );
	RPG_user_selected_vcp = atoi( temp_buf );
	XtFree( temp_buf );

	/* Log message. */

	HCI_LE_log("Change to RPG VCP %d pushed", RPG_user_selected_vcp);

        /* Check MSF for recommended mode against mode operator wants to change to. */

        msf_data.new_vcp = RPG_user_selected_vcp;
        msf_data.command = CHANGE_RPG_VCP;
        msf_data.conflict = Check_for_mode_conflict( msf_data.new_vcp, &msf_data.recommended_mode, 
                	                            &msf_data.recommended_vcp, 
                        	                    &msf_data.auto_mode );
        HCI_LE_log("MSF Recommended Mode: %d, MSF Recommended Mode default VCP: %d, Auto Mode: %d Conflict: %d",
                  msf_data.recommended_mode,
                  msf_data.recommended_vcp,
                  msf_data.auto_mode,
                  msf_data.conflict );

	/* Create popup message. */

        if( ORPGVCP_is_SZ2_vcp( RPG_user_selected_vcp ) > 0 )
        {
           SZ2_vcp_selected = 1;
          /* This is a SZ2 VCP, is CMD enabled as well? */
          ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                 ORPGINFO_STATEFL_GET, &CMD_enabled_flag );
        }

        if( SZ2_vcp_selected && (int) !CMD_enabled_flag )
        {
	  sprintf( buf, "You are about to download VCP %d, but CMD\nis currently disabled. CMD provides the best\nVCP %d data quality and will be automatically\nenabled along with VCP %d if you select Yes.\nDownload VCP %d and enable CMD?", RPG_user_selected_vcp, RPG_user_selected_vcp, RPG_user_selected_vcp, RPG_user_selected_vcp );
        }
        else
        {
	  sprintf( buf, "You are about to change to VCP %d stored as\nadaptation data at the RPG.  It will take\neffect at the start of the next volume scan.\nDo you want to continue?", RPG_user_selected_vcp );
        }

        if( msf_data.conflict && msf_data.auto_mode )
        {
	  hci_confirm_popup( Top_widget, buf, mode_conflict_vcp_callback, NULL );
	}
        else
        {
	  hci_confirm_popup( Top_widget, buf, accept_change_RPG_vcp, NULL );
	}

	update_vcp_control_properties();
}

/************************************************************************
 *	Description: This Function is the callback for all of the VCP	*
 *		     control buttons in the DOWNLOAD VCP and CHANGE VCP *
 *		     when there is a conflict between requested VCP and *
 *		     weather mode.					*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - VCP number 				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
mode_conflict_vcp_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		buf[ HCI_BUF_256 ];

	/* Create popup message. */

        if( msf_data.recommended_mode == PRECIPITATION_MODE )
	   sprintf( buf, "Clear Air Mode VCP %d being requested is in conflict with the\nMode Selection Function.  The Mode Selection Function may force\nPrecipitation Mode default VCP %d after one volume scan.", msf_data.new_vcp, msf_data.recommended_vcp );

        else
	   sprintf( buf, "Precipitation Mode VCP %d being requested is in conflict with\nthe Mode Selection Function.  The Mode Selection Function may\nforce Clear Air Mode default VCP %d after one volume scan.", msf_data.new_vcp, msf_data.recommended_vcp );


	if( msf_data.command == CHANGE_RPG_VCP )
	{
	  hci_info_popup( Top_widget, buf, accept_change_RPG_vcp );
	}
	else      
	{
	  hci_info_popup( Top_widget, buf, accept_change_RDA_vcp );
	}

	update_vcp_control_properties();
}

/************************************************************************
 *      Description: This Function is the callback for all of the VCP   *
 *                   control buttons in the DOWNLOAD EXPERIMENTAL VCP 	*
 *		     from RPG container.                                *
 *                                                                      *
 *      Input:  w - Activating widget ID                                *
 *              client_data - VCP number                                *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
change_EXP_vcp_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
	char		*temp_buf;
	char		buf[ HCI_BUF_256 ];
	XmComboBoxCallbackStruct *cbs;

	/* Extract vcp from callack struct. */

	cbs = ( XmComboBoxCallbackStruct * ) call_data;
	XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );
	EXP_user_selected_vcp = atoi( temp_buf );
	XtFree( temp_buf );

	/* Log message. */

        HCI_LE_log("Change to EXPERIMENTAL VCP %d pushed", EXP_user_selected_vcp);

	/* Create popup message. */

        sprintf( buf, "You are about to change to EXPERIMENTAL VCP %d stored as\nadaptation data at the RPG.  It will take effect at the\nstart of the next volume scan.  Do you want to continue?", EXP_user_selected_vcp );

	hci_confirm_popup( Top_widget, buf, accept_change_EXP_vcp, NULL );

	update_vcp_control_properties();
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "Yes" button from the DOWNLOAD VCP from RPG	*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - VCP number		 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
accept_change_RPG_vcp (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Change_RPG_vcp_flag = HCI_YES_FLAG;
}

void change_RPG_vcp()
{
        int ret;
        User_commanded_vcp_t cmded_vcp;

        cmded_vcp.last_vcp_commanded = RPG_user_selected_vcp;
        ret = ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO, (char *) &cmded_vcp,
                            sizeof(User_commanded_vcp_t), USER_COMMANDED_VCP_MSG_ID );
        if( ret < 0 )
           HCI_LE_log("ORPGDA_write( SUPPL_VCP_INFO ) Failed: %d", ret );

	HCI_PM( "Sending download VCP command" );		

	ORPGRDA_send_cmd (COM4_DLOADVCP,
			  (int) HCI_VCP_INITIATED_RDA_CTRL_CMD,
			  (int) RPG_user_selected_vcp,
                          (int) 0,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  NULL);

	sprintf (Cmd,"Requesting download of RPG VCP %d", RPG_user_selected_vcp);

	HCI_display_feedback( Cmd );
}

/************************************************************************
 *      Description: This Function is activated when the user selects   *
 *                   the "Yes" button from the DOWNLOAD VCP from RPG    *
 *                   confirmation popup window.                         *
 *                                                                      *
 *      Input:  w - Activating widget ID                                *
 *              client_data - VCP number                                *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void
accept_change_EXP_vcp (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
	Change_EXP_vcp_flag = HCI_YES_FLAG;
}

void change_EXP_vcp()
{
        int ret;
        User_commanded_vcp_t cmded_vcp;

        cmded_vcp.last_vcp_commanded = EXP_user_selected_vcp;
        ret = ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO, (char *) &cmded_vcp,
                            sizeof(User_commanded_vcp_t), USER_COMMANDED_VCP_MSG_ID );
        if( ret < 0 )
           HCI_LE_log("ORPGDA_write( SUPPL_VCP_INFO ) Failed: %d", ret );

        HCI_PM( "Sending download VCP command" );

        ORPGRDA_send_cmd (COM4_DLOADVCP,
                          (int) HCI_VCP_INITIATED_RDA_CTRL_CMD,
                          EXP_user_selected_vcp,
                          (int) 0,
                          (int) 0,
                          (int) 0,
                          (int) 0,
                          NULL);

        sprintf (Cmd,"Requesting download of EXPERIMENTAL VCP %d", EXP_user_selected_vcp);

	HCI_display_feedback( Cmd );
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the VCP restart button.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - unused			 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
restart_vcp_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	buf[HCI_BUF_128];

	HCI_LE_log("Restart VCP pushed");
	sprintf( buf, "You are about to stop the current data collection\nand start over at the lowest elevation cut.\nDo you want to continue?" );

	hci_confirm_popup( Top_widget, buf, accept_restart_vcp, NULL );
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "Yes" button from the Restart VCP confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - unused			 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
accept_restart_vcp (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Restart_vcp_flag = HCI_YES_FLAG;
}

void restart_vcp()
{
	HCI_PM( "Sending restart VCP command" );		

	ORPGRDA_send_cmd (COM4_RDACOM,
			  (int) 1,
			  CRDA_RESTART_VCP,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  (int) 0,
			  NULL);

	sprintf (Cmd,"Requesting the VCP to be restarted");

	HCI_display_feedback( Cmd );
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "Close" button in the VCP Control window.	*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
vcp_control_menu_cancel (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Close button selected");
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This Function is used to update the properties of	*
 *		     the widgets in the VCP Control window.		*
 *									*
 *	Input:	 NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
update_vcp_control_properties ()
{
  int  i;
  int  status;
  int  vcp_number;
  int  control;
  int  wblnstat;
  Boolean  sensitivity;
  Boolean  rda_vcp_sensitivity;
  unsigned int  orpg_state_flag;
  static short * rda_vcp = NULL;
  XmString str = NULL;

  /* Only update if the top level widget is not NULL. */

  if (Top_widget == (Widget) NULL) { return; }

  /* If the RDA-RPG link is down or if the RDA is in control,
     desensitize all of the control buttons other than the
     View Current VCP. */

  control = ORPGRDA_get_status (RS_CONTROL_STATUS);
  wblnstat = ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT);

  if ((control == RDA_IN_CONTROL) || (wblnstat != RS_CONNECTED))
  {
    sensitivity = False;
  }
  else
  {
    sensitivity = True;
  }

  /* If the RDA-RPG link is down and the data is not available, 
     desensitize the View Current VCP. */
  rda_vcp_sensitivity = True;
  if( rda_vcp == NULL )
     rda_vcp = hci_rda_vcp_ptr();
  if( (wblnstat != RS_CONNECTED) || (rda_vcp == NULL) )
     rda_vcp_sensitivity = False;
   
  /* If the RDA is not in an operate state then we cannot
     command a volume or elevation restart so desensitize
     those selections. */

  if (ORPGRDA_get_status (RS_RDA_STATUS) != RS_OPERATE)
  {
    XtVaSetValues (Restart_vcp_button, XmNsensitive, False, NULL );
  }
  else
  {
    XtVaSetValues (Restart_vcp_button, XmNsensitive, sensitivity, NULL );
  }

  /* If the RDA is not connected or the RDA VCP data is not
     available, desensitize the View Current VCP. */

    XtVaSetValues (RDA_VCP_button, XmNsensitive, rda_vcp_sensitivity, NULL );

  /* Get the VCP number from the latest RDA status message to
     determine if the pattern is local or remote. */

  vcp_number = ORPGRDA_get_status (RS_VCP_NUMBER);

  /* Determine if VCP translation is active.  If active, the VCP
     that is displayed as in use will be the pre-translated VCP. */

  if( vcp_number > 0 )
  {
    if( Read_suppl_vcp_info )
    {
      int ret;

      Read_suppl_vcp_info = 0;
      ret = ORPGDA_read( ORPGDAT_SUPPL_VCP_INFO, (char *) &Trans_info,
                         sizeof(Trans_info_t), TRANS_ACTIVE_MSG_ID ); 

      if( ret < 0 )
      {
        HCI_LE_error("ORPGDA_read( SUPPL_VCP_INFO ) Failed: %d", ret);
      }

    }

    if( Trans_info.active ){ vcp_number = Trans_info.vcp_external; }
  }

  status = ORPGINFO_statefl_get_rpgstat (&orpg_state_flag);

  /* Set sensitivity of VCP drop-down lists. */

  if( sensitivity )
  {
    XtSetSensitive( rda_frame, True );
    XtSetSensitive( rpg_frame, True );
    if( exp_frame != NULL )
    {
      XtSetSensitive( exp_frame, True );
    }
  }
  else
  {
    XtSetSensitive( rda_frame, False );
    XtSetSensitive( rpg_frame, False );
    if( exp_frame != NULL )
    {
      XtSetSensitive( exp_frame, False );
    }
  }
  /* Reset color/appearance of VCP drop-down lists. */

  if( previous_vcp_number != vcp_number )
  {
    previous_vcp_number = vcp_number;
    reset_vcp_combo_color( previous_set_combo_box );
  }

  if (vcp_number < 0)
  {
    if (hci_rda_adapt_where_defined(VCP_RDA_DEFINED, -vcp_number))
    {
      for (i=0;i<Num_rda_vcps;i++)
      {
        sprintf( tempbuf,"%d", Rda_vcp_num[ i ] );
        str = XmStringCreateLocalized( tempbuf );
        if( vcp_number == -Rda_vcp_num[ i ] )
        {
          if( Rda_vcp_type[ i ] == PRECIPITATION_MODE )
          {
            XmComboBoxSelectItem( rda_precip_combo_box, str );
            set_vcp_combo_color( rda_precip_combo_box );
            previous_set_combo_box = rda_precip_combo_box;
          }
          else
          {
            XmComboBoxSelectItem( rda_clear_combo_box, str );
            set_vcp_combo_color( rda_clear_combo_box );
            previous_set_combo_box = rda_clear_combo_box;
          }
        }
        XmStringFree( str );
      }
    }
  }
  else
  {
    /* vcp_number > 0 */

    if (hci_rda_adapt_where_defined (VCP_RPG_DEFINED, vcp_number))
    {
      for (i=0;i<Num_rpg_vcps;i++)
      {
        sprintf( tempbuf,"%d", Rpg_vcp_num[ i ] );
        str = XmStringCreateLocalized( tempbuf );
        if( vcp_number == Rpg_vcp_num[ i ] )
        {
          if( Rpg_vcp_type[ i ] == PRECIPITATION_MODE )
          {
            XmComboBoxSelectItem( rpg_precip_combo_box, str );
            set_vcp_combo_color( rpg_precip_combo_box );
            previous_set_combo_box = rpg_precip_combo_box;
          }
          else
          {
            XmComboBoxSelectItem( rpg_clear_combo_box, str );
            set_vcp_combo_color( rpg_clear_combo_box );
            previous_set_combo_box = rpg_clear_combo_box;
          }
        }
        XmStringFree( str );
      }
      for (i=0;i<Num_exp_vcps;i++)
      {
        sprintf( tempbuf,"%d", Exp_vcp_num[ i ] );
        str = XmStringCreateLocalized( tempbuf );
        if( vcp_number == Exp_vcp_num[ i ] )
        {
          if( Exp_vcp_type[ i ] == PRECIPITATION_MODE )
          {
            XmComboBoxSelectItem( exp_precip_combo_box, str );
            set_vcp_combo_color( exp_precip_combo_box );
            previous_set_combo_box = exp_precip_combo_box;
          }
          else
          {
            XmComboBoxSelectItem( exp_clear_combo_box, str );
            set_vcp_combo_color( exp_clear_combo_box );
            previous_set_combo_box = exp_clear_combo_box;
          }
        }
        XmStringFree( str );
      }
    }
  }
}

/************************************************************************
  Description: This Function is activated when the user selects one of
               the Velocity Resolution radio buttons.
 ************************************************************************/

static void velocity_resolution_callback( Widget w, XtPointer y, XtPointer z )
{
  int current_flag = 0;
  char buf[HCI_BUF_256];

  XmToggleButtonCallbackStruct	*state = (XmToggleButtonCallbackStruct *) z;

  /* Only do something if the button is set. */

  if( state->set )
  {
    current_flag = hci_current_vcp_get_vel_resolution();

    if( (int) y == current_flag )
    {
      return;
    }
    else if( (int) y == HCI_VELOCITY_RESOLUTION_HIGH )
    {
      sprintf( buf, "You are about to change the velocity resolution to 0.97 kts.\nIt will take effect at the start of the next volume scan.\nDo you want to continue?" );
    }
    else
    {
      sprintf( buf, "You are about to change the velocity resolution to 1.94 kts.\nIt will take effect at the start of the next volume scan.\nDo you want to continue?" );
    }

    hci_confirm_popup( Top_widget, buf, accept_velocity_resolution, cancel_velocity_resolution );
  }
}

/************************************************************************
  Description: This function is activated when the user selects the "No"
               button from the Velocity Resolution confirmation popup.
 ************************************************************************/

static void cancel_velocity_resolution( Widget x, XtPointer y, XtPointer z )
{
  update_velocity_resolution_widget();
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
               button from the Velocity Increment confirmation popup.
 ************************************************************************/

static void accept_velocity_resolution( Widget x, XtPointer y, XtPointer z )
{
  Change_velocity_resolution_flag = HCI_YES_FLAG;
  update_velocity_resolution_widget();
}

/************************************************************************
 Description: This function calls the function responsible for changing
              the velocity resolution.
 ************************************************************************/

static void change_velocity_resolution()
{
  if( hci_current_vcp_get_vel_resolution() == HCI_VELOCITY_RESOLUTION_HIGH )
  {
    hci_change_velocity_resolution( HCI_VELOCITY_RESOLUTION_LOW );
  }
  else
  {
    hci_change_velocity_resolution( HCI_VELOCITY_RESOLUTION_HIGH );
  }
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     one of the Auto Clear Air radio buttons.		*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - AUTO_SWITCH or MANUAL_SWITCH 		*
 *		call_data - button state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_auto_clear_air_callback (
Widget		 w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int update_flag;
  char buf[HCI_BUF_128];

  XmToggleButtonCallbackStruct  *state =
        (XmToggleButtonCallbackStruct *) call_data;

  /* Only do something if the button is set. */

  if( state->set )
  {
    /* Don't do anything on FAA inactive channel. */

    if( hci_disallow_on_faa_inactive( Top_widget ) )
    {
      update_clear_air_toggles();
      return;
    }

    Auto_clear_air_user_cmd = (int) client_data;
    update_flag = hci_get_mode_B_auto_switch_flag();

    if( Auto_clear_air_user_cmd == update_flag ){ return; }

    if( Auto_clear_air_user_cmd == AUTO_SWITCH )
    {
      sprintf( buf, "You are about to enable the auto-switch to Clear Air mode.\nDo you want to continue?" );
    }
    else
    {
      sprintf( buf, "You are about to disable the auto-switch to Clear Air mode.\nDo you want to continue?" );
    }

    hci_confirm_popup( Top_widget, buf, hci_auto_clear_air_yes, hci_auto_clear_air_no );
  }
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "No" button from the Auto Clear Air		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - AUTO_SWITCH or MANUAL_SWITCH		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_auto_clear_air_no (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  update_clear_air_toggles();
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "Yes" button from the Auto Clear Air		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - AUTO_SWITCH or MANUAL_SWITCH 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_auto_clear_air_yes (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  /* Determine new button state. */

  if( Auto_clear_air_user_cmd == AUTO_SWITCH )
  {
    hci_set_mode_B_auto_switch_flag( AUTO_SWITCH );
  }
  else
  {
    hci_set_mode_B_auto_switch_flag( MANUAL_SWITCH );
  }

  update_clear_air_toggles();
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     one of the Auto Precip radio buttons.		*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - AUTO_SWITCH or MANUAL_SWITCH 		*
 *		call_data - button state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_auto_precip_callback (
Widget		 w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int update_flag;
  char buf[HCI_BUF_128];

  XmToggleButtonCallbackStruct  *state =
        (XmToggleButtonCallbackStruct *) call_data;

  /* Only do something if the button is set. */

  if( state->set )
  {
    /* Don't do anything on FAA inactive channel. */

    if( hci_disallow_on_faa_inactive( Top_widget ) )
    {
      update_precip_toggles();
      return;
    }

    Auto_precip_user_cmd = (int) client_data;
    update_flag = hci_get_mode_A_auto_switch_flag();

    if( Auto_precip_user_cmd == update_flag ){ return; }

    if( Auto_precip_user_cmd == AUTO_SWITCH )
    {
      sprintf( buf, "You are about to enable the auto-switch to Precipitation mode.\nDo you want to continue?" );
    }
    else
    {
      sprintf( buf, "You are about to disable the auto-switch to Precipitation mode.\nDo you want to continue?" );
    }

    hci_confirm_popup( Top_widget, buf, hci_auto_precip_yes, hci_auto_precip_no );
  }
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "No" button from the Auto Precip		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - AUTO_SWITCH or MANUAL_SWITCH 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_auto_precip_no (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  update_precip_toggles();
}

/************************************************************************
 *	Description: This Function is activated when the user selects	*
 *		     the "Yes" button from the Auto Precip		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - AUTO_SWITCH or MANUAL_SWITCH 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_auto_precip_yes (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  /* Determine new button state. */

  if( Auto_precip_user_cmd == AUTO_SWITCH )
  {
    hci_set_mode_A_auto_switch_flag( AUTO_SWITCH );
  }
  else
  {
    hci_set_mode_A_auto_switch_flag( MANUAL_SWITCH );
  }

  update_precip_toggles();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		the Default Wx Mode button.				*
 *									*
 *	Input:	w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_change_wx_mode_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		buf[ HCI_BUF_128 ];
	char		*mode_buf;
	XmComboBoxCallbackStruct *cbs;

	/* Extract default weather mode from callback struct. */

	cbs = ( XmComboBoxCallbackStruct * ) call_data;
	XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &mode_buf );
	strcpy( WX_mode_user_selected, mode_buf );

	sprintf( buf, "Do you want to permanently change\nthe Default Weather Mode to\n%s?", WX_mode_user_selected );

	XtFree( mode_buf );

        hci_confirm_popup( Top_widget, buf, accept_wx_mode_change_callback, cancel_wx_mode_change_callback );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button in the Default Wx Mode		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_wx_mode_change_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		buf[ HCI_BUF_512 ];
	int		ret = -1;

	ret = DEAU_set_values(  ORPGSITE_DEA_WX_MODE, 1, &WX_mode_user_selected[0], 1, 0 );

	if( ret < 0 )
	{
	  sprintf( buf, "Unable to set %s for %s (%d)",
	           WX_mode_user_selected, ORPGSITE_DEA_WX_MODE, ret );
	  hci_error_popup( Top_widget, buf, NULL );
	  HCI_LE_error( buf );
	}

	update_default_mode_vcp();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "No" button in the Default Wx Mode		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cancel_wx_mode_change_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	update_default_mode_vcp();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     an item from the Mode A drop-down list.		*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
vcp_mode_A_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		buf[ HCI_BUF_128 ];
	char		*temp_buf;
	XmComboBoxCallbackStruct *cbs;

	/* Extract vcp from callback struct. */

	cbs = ( XmComboBoxCallbackStruct * ) call_data;
	XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );
	Mode_A_user_selected_vcp = atoi( temp_buf );
	XtFree( temp_buf );

	/* Log message. */

        HCI_LE_log("Change Default Mode A VCP to %d selected", Mode_A_user_selected_vcp);

	/* Create popup message. */

	sprintf( buf, "Do you want to permanently change\nthe Default VCP for Mode A to %d?", Mode_A_user_selected_vcp );
        hci_confirm_popup( Top_widget, buf, accept_vcp_mode_A_callback, cancel_vcp_mode_A_callback );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button in the vcp_mode_A_confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_vcp_mode_A_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  /* User wants to change default VCP for mode A. */
  /* Read VCP from client_data and set new VCP.   */
  /* Call function that updates gui.              */

  char  Abuf[4];
  char  buf[HCI_BUF_512];
  int   ret;
  double value;

  sprintf(Abuf,"%d",Mode_A_user_selected_vcp);
  value = atof( Abuf );

  /* Set default VCP. */
  ret = DEAU_set_values( ORPGSITE_DEA_DEF_MODE_A_VCP, 0, &value, 1, 0);

  if( ret < 0 )
  {
    sprintf( buf, "Unable to set %f for %s (%d)",
             value, ORPGSITE_DEA_DEF_MODE_A_VCP, ret );
    hci_error_popup( Top_widget, buf, NULL );
    HCI_LE_error( buf );
  }
  update_default_mode_vcp();
}

/************************************************************************
 *      Description: This function is activated when the user selects	*
 *                   the "No" button in the vcp_mode_A_confirmation	*
 *                   popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cancel_vcp_mode_A_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  update_default_mode_vcp();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     an item from the Mode B drop-down list.		*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
vcp_mode_B_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		*temp_buf;
	char		buf[ HCI_BUF_128 ];
	XmComboBoxCallbackStruct *cbs;

        /* Extract vcp from callback struct. */

        cbs = ( XmComboBoxCallbackStruct * ) call_data;
        XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );        Mode_B_user_selected_vcp = atoi( temp_buf );
        XtFree( temp_buf );

        /* Log message. */

        HCI_LE_log("Change Default Mode B VCP to %d selected", Mode_B_user_selected_vcp);

        /* Create popup message. */

        sprintf( buf, "Do you want to permanently change\nthe Default VCP for Mode B to %d?", Mode_B_user_selected_vcp );
	hci_confirm_popup( Top_widget, buf, accept_vcp_mode_B_callback, cancel_vcp_mode_B_callback );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Yes" button in the vcp_mode_B_confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_vcp_mode_B_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  /* User wants to change default VCP for mode B. */
  /* Read VCP from client_data and set new VCP.   */
  /* Call function that updates gui.              */

  char  Bbuf[4];
  char  buf[HCI_BUF_512];
  int  ret;
  double   value;

  sprintf(Bbuf,"%d", Mode_B_user_selected_vcp);
  value = atof( Bbuf );

  /* Set default VCP. */
  ret = DEAU_set_values( ORPGSITE_DEA_DEF_MODE_B_VCP, 0, &value, 1, 0);

  if( ret < 0 )
  {
    sprintf( buf, "Unable to set %f for %s (%d)",
             value, ORPGSITE_DEA_DEF_MODE_B_VCP, ret );
    hci_error_popup( Top_widget, buf, NULL );
    HCI_LE_error( buf );
  }
  update_default_mode_vcp();
}

/************************************************************************
 *      Description: This function is activated when the user selects	*
 *                   the "No" button in the vcp_mode_B_confirmation	*
 *                   popup window.					*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cancel_vcp_mode_B_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  update_default_mode_vcp();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the mode_status button.				*
 *									*
 *	Input:  w - widget ID						*
 *	client_data - unused						*
 *	call_data - unused						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
mode_status_button_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name[100];
	   
  sprintf( task_name, "hci_mode_status -A %d -name \"Mode Automation Status\"", HCI_get_channel_number() );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );
  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name, 
                      "hci_mode_status",
                      "Mode Automation Status",
                      -1 );
}

/************************************************************************
 *	Description: This Function is used as a general purpose routine	*
 *		     to close an information popup or cancel a		*
 *		     confirmatiomn popup without doing any special	*
 *		     handling.						*
 *									*
 *	Input:  w - Activating widget ID				*
 *		client_data - unused			 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
cancel_command_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
}

/************************************************************************
 *	Description: This Function is the timer procedure for the task.	*
 *		     It's primary purpose is to monitor RDA status	*
 *		     updates so the VCP Control window widgets can	*
 *		     be updated.					*
 *									*
 *	Input:  w - Top level widget ID					*
 *		id - timer ID (unused)					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
	int	rpg_status;
	int	vel_reso_flag = hci_current_vcp_get_vel_resolution();
	int	update_flag = 0;
	int	status_update_time = 0;
	unsigned int	orpg_state_flag;
static	int	old_RDA_status_update_time = 0;
static	unsigned int	old_orpg_state_flag = 99;
static	unsigned int	old_vel_reso_flag = 99;

	HCI_PM( "Updating VCP information" );

/*	Launch popups if necessary.					*/

	if( Current_vcp_popup_flag )
	{
	  Current_vcp_popup_flag = 0;
	  hci_display_rda_vcp_data( Top_widget, NULL, NULL );
	}

	if( Rpg_vcp_defs_popup_flag )
	{
	  Rpg_vcp_defs_popup_flag = 0;
	  hci_display_rpg_vcp_definitions( Top_widget, NULL, NULL );
	}

/*	Check if user wants to change or restart VCP.			*/

	if( Change_RDA_vcp_flag == HCI_YES_FLAG )
	{
	  Change_RDA_vcp_flag = HCI_NO_FLAG;
	  change_RDA_vcp();
	}

	if( Change_RPG_vcp_flag == HCI_YES_FLAG )
	{
	  Change_RPG_vcp_flag = HCI_NO_FLAG;
	  change_RPG_vcp();
	}

	if( Change_EXP_vcp_flag == HCI_YES_FLAG )
	{
	  Change_EXP_vcp_flag = HCI_NO_FLAG;
	  change_EXP_vcp();
	}

	if( Restart_vcp_flag == HCI_YES_FLAG )
	{
	  Restart_vcp_flag = HCI_NO_FLAG;
	  restart_vcp();
	}

	if( Change_velocity_resolution_flag == HCI_YES_FLAG )
	{
	  Change_velocity_resolution_flag = HCI_NO_FLAG;
	  change_velocity_resolution();
	}

/*	Check if DEAU data has changed.					*/

	if( Deau_mode_data_update_flag )
	{
          Deau_mode_data_update_flag = 0;
	  update_precip_toggles();
	  update_clear_air_toggles();
	}

	if( Deau_site_data_update_flag )
	{
          Deau_site_data_update_flag = 0;
	  update_default_mode_vcp();
	}

/*	Check if GUI needs to be updated.				*/

	if(vel_reso_flag != old_vel_reso_flag)
	{
	    old_vel_reso_flag = vel_reso_flag;
            update_velocity_resolution_widget();
	    HCI_LE_status( "RPG velocity resolution updated" );
	}

	if ((status_update_time = ORPGRDA_status_update_time()) != old_RDA_status_update_time)
	{ 
	    old_RDA_status_update_time = status_update_time;
	    update_flag = 1;
	    HCI_LE_status("RDA status msg updated");
	}

	rpg_status = ORPGINFO_statefl_get_rpgstat (&orpg_state_flag);

	/* Exit because I/O operations were cancelled */
	if (rpg_status == RMT_CANCELLED)
	{
	    HCI_LE_status("RMT CANCELLED..Exit at rpg_status");
	    HCI_task_exit (HCI_EXIT_SUCCESS);
	}

	if (rpg_status < 0)
	    HCI_LE_error("RPG State data update failed: %d", rpg_status);

	if((rpg_status == 0) && (orpg_state_flag != old_orpg_state_flag))
	{

	    old_orpg_state_flag = orpg_state_flag;

	    update_flag = 1;
	    HCI_LE_status("RPG state data updated");

	}

	if (update_flag == 1)
	{
          update_flag = 0;
	  update_vcp_control_properties (); 
	}

	if( Rda_vcp_updated ){

	   Rda_vcp_updated = 0;
	   hci_rda_vcp_update_callback();

	}
}

/************************************************************************
 *      Description: This function is the comparison function passed    *
 *                   to the qsort algorithm. Sorting is in ascending    *
 *                   order.                                             *
 *                                                                      *
 *      Input:  vp - input value #1                                     *
 *              vq - input value #2                                     *
 *      Output: NONE                                                    *
 *      Return:  1 if p > q                                             *
 *               0 if p = q                                             *
 *              -1 if p < q                                             *
 ************************************************************************/

int
vcp_comp ( const void *vp, const void *vq )
{
  const short *p = vp;
  const short *q = vq;
  short diff = *p - *q;

  if( diff < 0 ){ return -1; }
  if( diff == 0 ){ return 0; }
  return 1;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Continue" button in a warning popup window.	*
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
 *      Description: This function is activated when the user selects   *
 *                   the lock button					*
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              security_level - lock data                              *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

int
default_mode_vcp_lock ()
{
  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_selected() )
  {
    /* Do nothing. */
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_ROC_unlocked() )
    {
      Unlocked_roc = HCI_YES_FLAG;
    }
    else if( hci_lock_URC_unlocked() )
    {
      Unlocked_urc = HCI_YES_FLAG;
    }
  }
  else if( hci_lock_close() && 
           ( Unlocked_roc == HCI_YES_FLAG || Unlocked_urc == HCI_YES_FLAG ) )
  {
    Unlocked_roc = HCI_NO_FLAG;
    Unlocked_urc = HCI_NO_FLAG;
  }

  /* Update the window objects. */

  update_default_mode_vcp();

  return HCI_LOCK_PROCEED;
}

/************************************************************************
 *      Description: This function updates the display properties of	*
 *		     the default mode/vcp buttons.			*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
update_default_mode_vcp ()
{
  int ret = -1;
  char buf[ 128];
  double value = 0.0;
  char *char_value = NULL;
  XmString str;

  /* Set/Unset sensitivity depending on ROC/URC permissions. */

  if( Unlocked_roc )
  {
    /* First, look at default weather mode. */

    sprintf( buf, ORPGSITE_DEA_WX_MODE ); 

    ret = get_deau_permissions( buf, "ROC" );
 
    if( ret == VALID_PERMISSION )
    {
      XtSetSensitive(default_wx_mode_combo_box,True);
    }
    else
    {
      XtSetSensitive(default_wx_mode_combo_box,False);
    }
  
    /* Next, look at default VCP for clear air mode. */

    sprintf( buf, ORPGSITE_DEA_DEF_MODE_B_VCP ); 

    ret = get_deau_permissions( buf, "ROC" );
 
    if( ret == VALID_PERMISSION )
    {
      XtSetSensitive(default_mode_B_combo_box,True);
    }
    else
    {
      XtSetSensitive(default_mode_B_combo_box,False);
    }

    /* Next, look at default VCP for precip mode. */

    sprintf( buf, ORPGSITE_DEA_DEF_MODE_A_VCP ); 

    ret = get_deau_permissions( buf, "ROC" );
 
    if( ret == VALID_PERMISSION )
    {
      XtSetSensitive(default_mode_A_combo_box,True);
    }
    else
    {
      XtSetSensitive(default_mode_A_combo_box,True);
    }
  }
  else if( Unlocked_urc )
  {
    /* First, look at default weather mode. */

    sprintf( buf, ORPGSITE_DEA_WX_MODE ); 

    ret = get_deau_permissions( buf, "URC" );
 
    if( ret == VALID_PERMISSION )
    {
      XtSetSensitive(default_wx_mode_combo_box,True);
    }
    else
    {
      XtSetSensitive(default_wx_mode_combo_box,False);
    }
  
    /* Next, look at default VCP for clear air mode. */

    sprintf( buf, ORPGSITE_DEA_DEF_MODE_B_VCP ); 

    ret = get_deau_permissions( buf, "URC" );
 
    if( ret == VALID_PERMISSION )
    {
      XtSetSensitive(default_mode_B_combo_box,True);
    }
    else
    {
      XtSetSensitive(default_mode_B_combo_box,False);
    }

    /* Next, look at default VCP for precip mode. */

    sprintf( buf, ORPGSITE_DEA_DEF_MODE_A_VCP ); 

    ret = get_deau_permissions( buf, "URC" );
 
    if( ret ==  VALID_PERMISSION )
    {
      XtSetSensitive(default_mode_A_combo_box,True);
    }
    else
    {
      XtSetSensitive(default_mode_A_combo_box,False);
    }
  }
  else
  {
    XtSetSensitive(default_wx_mode_combo_box,False);
    XtSetSensitive(default_mode_B_combo_box,False);
    XtSetSensitive(default_mode_A_combo_box,False);
  }

  /* Update values displayed on top of combo list. */

  ret = DEAU_get_string_values( ORPGSITE_DEA_WX_MODE, &char_value );

  if( ret < 0 )
  {
    sprintf( buf, "Unable to get %s (%d)",
             ORPGSITE_DEA_WX_MODE, ret );
    hci_error_popup( Top_widget, buf, NULL );
    HCI_LE_error( buf );
  }
  else
  {
    str = XmStringCreateLocalized( char_value );
    XmComboBoxSelectItem( default_wx_mode_combo_box, str );
    XmStringFree( str );
  }

  ret = DEAU_get_values( ORPGSITE_DEA_DEF_MODE_A_VCP, &value, 1 );

  if( ret < 0 )
  {
    sprintf( buf, "Unable to get %s (%d)",
             ORPGSITE_DEA_DEF_MODE_A_VCP, ret );
    hci_error_popup( Top_widget, buf, NULL );
    HCI_LE_error( buf );
  }
  else
  {
    sprintf( tempbuf,"%d", ( int ) value );
    str = XmStringCreateLocalized( tempbuf );
    XmComboBoxSelectItem( default_mode_A_combo_box, str );
    XmStringFree( str );
  }

  ret = DEAU_get_values( ORPGSITE_DEA_DEF_MODE_B_VCP, &value, 1 );

  if( ret < 0 )
  {
    sprintf( buf, "Unable to get %s (%d)",
             ORPGSITE_DEA_DEF_MODE_B_VCP, ret );
    hci_error_popup( Top_widget, buf, NULL );
    HCI_LE_error( buf );
  }
  else
  {
    sprintf( tempbuf,"%d", ( int ) value );
    str = XmStringCreateLocalized( tempbuf );
    XmComboBoxSelectItem( default_mode_B_combo_box, str );
    XmStringFree( str );
  }
}

/************************************************************************
 *      Description: This function checks permissions of various DEA	*
 *		     variables.						*
 *                                                                      *
 *      Input:  char * - DEA variable id				*
 *		char * - permission LOCA to check (ROC, URC, etc.)	*
 *      Output: NONE                                                    *
 *      Return: int == 1, permission is valid				*
 *		int == 0, permission isn't valid			*
 *		int <0, error calling DEAU functions			*
 ************************************************************************/

int
get_deau_permissions ( char *id, char *loca )
{
  int ret;
  DEAU_attr_t *d_attr;

  ret = DEAU_get_attr_by_id( id, &d_attr );
 
  if( ret == 0 )
  {
    ret = DEAU_check_permission( d_attr, loca );

    if( ret < 0 )
    {
      HCI_LE_error("DEAU_check_permission for %s failed (%d)", id, ret);
    }
  }
  else
  {
    HCI_LE_error("DEAU_get_attr_by_id for %s failed (%d)", id, ret);
  }

  return ret;
}

/************************************************************************
 *      Description: This function is called when the mode adaptation	*
 *                   data in the deau database changes.			*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE							*
 ************************************************************************/

void
deau_mode_data_change_callback (
int lb_fd,
LB_id_t msg_id,
int msg_len,
char *group_name )
{
  Deau_mode_data_update_flag = 1;
}

/************************************************************************
 *      Description: This function is called when the site adaptation	*
 *                   data in the deau database changes.			*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE							*
 ************************************************************************/

void
deau_site_data_change_callback (
int lb_fd,
LB_id_t msg_id,
int msg_len,
char *group_name )
{
  Deau_site_data_update_flag = 1;
}

/************************************************************************
 *      Description: This function is called when the deau data has	*
 *                   changed.						*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE							*
 ************************************************************************/

void
update_clear_air_toggles ()
{
  /* Set toggle values according to adaptation data values. */

  if( hci_get_mode_B_auto_switch_flag() == AUTO_SWITCH )
  {
    XtVaSetValues( auto_clear_air_auto_button, XmNset, True, NULL );
    XtVaSetValues( auto_clear_air_manual_button, XmNset, False, NULL );
  }
  else
  {
    XtVaSetValues( auto_clear_air_auto_button, XmNset, False, NULL );
    XtVaSetValues( auto_clear_air_manual_button, XmNset, True, NULL );
  }
}

void
update_precip_toggles ()
{
  /* Set toggle values according to adaptation data values. */

  if( hci_get_mode_A_auto_switch_flag() == AUTO_SWITCH )
  {
    XtVaSetValues( auto_precip_auto_button, XmNset, True, NULL );
    XtVaSetValues( auto_precip_manual_button, XmNset, False, NULL );
  }
  else
  {
    XtVaSetValues( auto_precip_auto_button, XmNset, False, NULL );
    XtVaSetValues( auto_precip_manual_button, XmNset, True, NULL );
  }
}

/************************************************************************
 *      Description: This function is called when the data in message	*
 *                   TRNS_ACTIVE_MSG_ID in ORPGDAT_SUPPL_VCP_INFO 	*
 *		     changes.						*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE							*
 ************************************************************************/

void	suppl_vcp_change_callback( int lb_fd, LB_id_t msg_id,
                                   int msg_info, void *arg ){

	Read_suppl_vcp_info = 1;

}

/************************************************************************
 *      Description: This function is called when the data in message	*
 *                   ORPGDAT_RDA_VCP_MSG_ID in ORPGDAT_RDA_VCP_DATA 	*
 *		     changes.						*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE							*
 ************************************************************************/

void	rda_vcp_change_callback( int lb_fd, LB_id_t msg_id,
                                 int msg_info, void *arg ){

	Rda_vcp_updated = 1;

}

/************************************************************************
 * Description: Reset color of VCP drop-down list.
 ************************************************************************/

void reset_vcp_combo_color( Widget wid )
{
  Widget text_wid;

  if( wid == NULL ){ return; }

  XtVaSetValues( wid,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                 NULL );
  XtVaGetValues( wid, XmNtextField, &text_wid, NULL );
  XtVaSetValues( text_wid,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );
}

/************************************************************************
 * Description: Set color of VCP drop-down list.
 ************************************************************************/

void set_vcp_combo_color( Widget wid )
{
  Widget text_wid;

  if( wid == NULL ){ return; }

  XtVaSetValues( wid,
                 XmNbackground, hci_get_read_color( WHITE ),
                 XmNforeground, hci_get_read_color( WHITE ),
                 NULL );
  XtVaGetValues( wid, XmNtextField, &text_wid, NULL );
  XtVaSetValues( text_wid,
                 XmNbackground, hci_get_read_color( WHITE ),
                 NULL );
}

/************************************************************************
 *	Description: This Function checks the weather status data for	*
 *		     conflict between requested VCP (and corresponding  *
 *		     mode and that recommended by the Mode Selection    *
 *		     Function.						*
 *									*
 *	Input:  new_vcp - Requested VCP       				*
 *	Output: recommended_mode - MSF recommended weather mode		*
 *	        recommended_vcp - MSF recommended mode default VCP	*
 *	        auto_mode - MSF switching flag state             	*
 *	Return: 0 if no conflict, 1 if conflict				*
 ************************************************************************/
int Check_for_mode_conflict( int new_vcp, int *recommended_mode, 
                             int *recommended_vcp, int *auto_mode ){

   int i, ret, next_mode;

   static Wx_status_t wx_status;

   next_mode = -1;

   /* Set the recommended mode based on the selected VCP.   Assumes
      PRECIPITATION_MODE: 1 and CLEAR_AIR_MODE: 0. */
   for( i = 0; i < WXVCPMAX; i++ ){

      if( hci_rda_adapt_wxmode( 1, i ) == new_vcp ){

         next_mode = PRECIPITATION_MODE;
         break;

      }

   }
   if( next_mode < 0 ){

      for( i = 0; i < WXVCPMAX; i++ ){

         if( hci_rda_adapt_wxmode( 0, i ) == new_vcp ){

            next_mode = CLEAR_AIR_MODE;
            break;

         }

      }

   }   

   *recommended_mode = next_mode;
   *recommended_vcp = new_vcp;

   /* Now read the wx status data.  Check if the recommended mode is the
      same as the next mode. */
   ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &wx_status, sizeof(Wx_status_t),
                      WX_STATUS_ID );

   /* On error, assume No Mode Conflict .... */
   if( ret < 0 )
      return 0;

   /* The mode associated with the next VCP does not match the mode the MSF
      recommends. */
   *recommended_mode = wx_status.recommended_wxstatus;
   *auto_mode = 0;
   if( wx_status.recommended_wxstatus != next_mode ){

      /* Set the Default VCP the MSF recommends for the recommended mode. */
      *recommended_vcp = wx_status.recommended_wxstatus_default_vcp;

      /* Check if in "auto" mode.  If in auto mode for the recommended VCP, 
         then set the "auto_flag" flag. */
      if( (wx_status.recommended_wxstatus == PRECIPITATION_MODE)
                                     &&
          (wx_status.mode_select_adapt.auto_mode_A) )
         *auto_mode = 1;

      else if( (wx_status.recommended_wxstatus == CLEAR_AIR_MODE)
                                     &&
               (wx_status.mode_select_adapt.auto_mode_B) )
         *auto_mode = 1;


      return 1;

   }

   return 0;
   
}

/************************************************************************
  Description: This function sets flag to launch "View Current VCP" popup.
 ************************************************************************/

void hci_set_rda_vcp_popup_flag( Widget w, XtPointer y, XtPointer z )
{
  Current_vcp_popup_flag = 1;
}

/************************************************************************
  Description: This function sets flag to launch "View RPG VCP
               Definitions" popup.
 ************************************************************************/

void hci_set_rpg_vcp_defs_popup_flag( Widget w, XtPointer y, XtPointer z )
{
  Rpg_vcp_defs_popup_flag = 1;
}

/************************************************************************
 Description: This function sets states of velocity resolution radio
              buttons.
 ************************************************************************/

static void update_velocity_resolution_widget()
{
  if( hci_current_vcp_get_vel_resolution () == HCI_VELOCITY_RESOLUTION_HIGH )
  {
    XtVaSetValues( Velocity_high_button, XmNset, True, NULL );
    XtVaSetValues( Velocity_low_button, XmNset, False, NULL );
  }
  else
  {
    XtVaSetValues( Velocity_high_button, XmNset, False, NULL );
    XtVaSetValues( Velocity_low_button, XmNset, True, NULL );
  }
}

