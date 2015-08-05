/************************************************************************
 *		 							*
 *	Module:  hci_environmental_winds.c				*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI to interface with the environmental winds	*
 *	table.								*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:53 $
 * $Id: hci_environmental_winds.c,v 1.126 2010/03/10 18:46:53 ccalvert Exp $
 * $Revision: 1.126 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_environmental_wind.h>
#include <hail_algorithm.h>
#include <storm_cell_track.h>

/*	Local constants.						*/

enum	{EDIT_NONE=0,
	 EDIT_WNDDIR,
	 EDIT_WNDSPD,
	 EDIT_HAIL_0_HEIGHT,
	 EDIT_HAIL_MINUS_20_HEIGHT,
	 EDIT_STORM_DIRECTION,
	 EDIT_STORM_SPEED
};

#define UNITS_KTS	0
#define	UNITS_MPS	1

#define	MAX_ENV_WIND_SPEED	128.1	/*  Max wind speed (m/s)	*/
#define	HCI_EWT_LB		(EWT_UPT/ITC_IDRANGE)*ITC_IDRANGE
#define	HCI_HAIL_LB		(MODEL_HAIL/ITC_IDRANGE)*ITC_IDRANGE

/*	Conversion factors.					*/

#define	PIOVER2		(3.14159/2.0)
#define PI3OVER2	(3*3.14159/2.0)

#define	MAX_STORM_WIND_SPEED	99.9

/*	Global definitions.						*/

Widget		Close1_button  = (Widget) NULL;
Widget		Save1_button  = (Widget) NULL;
Widget		Undo1_button  = (Widget) NULL;
Widget		Clear1_button = (Widget) NULL;
Widget		Close2_button  = (Widget) NULL;
Widget		Save2_button  = (Widget) NULL;
Widget		Undo2_button  = (Widget) NULL;
Widget		Clear2_button = (Widget) NULL;
Widget		Data_Entry_button = (Widget) NULL;
Widget		Draw_widget  = (Widget) NULL;
Widget		Storm_widget = (Widget) NULL;
Widget		Data_widget  = (Widget) NULL;
Widget		Data_form    = (Widget) NULL;
Window		Winds_window = (Window) NULL;
Window		Storm_window = (Window) NULL;
Pixmap		Winds_pixmap = (Pixmap) NULL;
Pixmap		Storm_pixmap = (Pixmap) NULL;
GC		Winds_gc = (GC) NULL;
Dimension	Winds_width  = 530;	/* Winds graph width */
Dimension	Winds_height = 850;	/* Winds graph height */
Dimension	Storm_width  = 180;     /* Storm motion graph width */
Dimension	Storm_height = 100;     /* Storm motion graph height */
Dimension	Old_winds_height = 400; /* Old winds graph width */
Dimension	Old_winds_width  = 800; /* Old winds graph height */
Widget		Vad_update_yes              = (Widget) NULL;
Widget		Vad_update_no               = (Widget) NULL;
Widget		Model_update_yes            = (Widget) NULL;
Widget		Model_update_no             = (Widget) NULL;
Widget		Units_kts                   = (Widget) NULL;
Widget		Units_mps                   = (Widget) NULL;
Widget		display_current             = (Widget) NULL;
Widget		display_model               = (Widget) NULL;
Widget		Direction_text [LEN_EWTAB]  = {(Widget) NULL};
Widget		Speed_text     [LEN_EWTAB]  = {(Widget) NULL};
Widget		Hail_update_label           = (Widget) NULL;
Widget		Hail_0_height_widget        = (Widget) NULL;
Widget		Hail_minus_20_height_widget = (Widget) NULL;
Widget		Storm_direction_widget      = (Widget) NULL;
Widget		Storm_speed_widget          = (Widget) NULL;
Widget		Data_entry_spd_units_label  = (Widget) NULL;
Widget		default_storm_motion_speed_label = (Widget) NULL;
XSegment	Windbarb [512]; /* windbarb data buffer */
Boolean		Interpolate_flag = True;       /* Interpolation flag for
						  data entry window winds */

static float	Units_conversion = HCI_MPS_TO_KTS; /* Active display units */
static int	Disable_vad_flag = HCI_NO_FLAG; /* VAD Update flag */
static int	Disable_model_flag = HCI_NO_FLAG; /* Model Update flag */
static int	Enw_update_flag = HCI_NO_FLAG; /* winds LB update flag */
static int	Model_update_flag = HCI_NO_FLAG; /* Model LB update flag */
static int	Change_hail_flag  = HCI_NOT_CHANGED_FLAG; /* hail edit flag */
static int	Change_storm_flag = HCI_NOT_CHANGED_FLAG; /* storm edit flag */
static int	Change_winds_flag = HCI_NOT_CHANGED_FLAG; /* winds edit flag */
static int	Need_to_verify_VAD_flag = HCI_NO_FLAG;
static int	Verifying_VAD_flag = HCI_NO_FLAG;
static int	Need_to_verify_Model_flag = HCI_NO_FLAG;
static int	Verifying_Model_flag = HCI_NO_FLAG;
static int	Display_flag = -1; /* Which ewt to display */
static int	Algorithm_data_updated = HCI_YES_FLAG; /* Adapt data flag */
static int 	Save_flag = HCI_NO_FLAG;
static int 	Save_popup_flag = HCI_NO_FLAG;
static int	Winds_edit_type = EDIT_NONE; /* Edit flag */
static int	Level = 0;	/* Current selected level */
static float	Hail_0_height        = 0.0; /* Height 0 C level (Kft) */
static float	Hail_minus_20_height = 0.0; /* Height -20 C level (Kft) */
static int	Hail_date [3]        = {0}; /* Hail temp update date (m/d/y) */
static int      Hail_time[3]         = {0}; /* Hail temp update time (h:m:s) */
static int	Storm_direction      = 0;   /* Default storm direcion (deg) */
static float	Storm_speed          = 0.0; /* Default storm speed (Kts) */
static float	Rda_elevation = 0.0;        /* RDA elevation in feet. */
static Siteadp_adpt_t Site;		    /* Stores the Site information. */

/*	Translation table when cursor over storm motion graph */

String	Storm_translations =
	"<PtrMoved>:	storm_motion_input(move) \n\
	<Btn1Down>:	storm_motion_input(down1)   ManagerGadgetArm() \n\
	<Btn1Up>:	storm_motion_input(up1)     ManagerGadgetActivate() \n\
	<Btn2Down>:	storm_motion_input(down2)   ManagerGadgetArm() \n\
	<Btn1Up>:	storm_motion_input(up2)     ManagerGadgetActivate() \n\
	<Btn3Down>:	storm_motion_input(down3)   ManagerGadgetArm() \n\
	<Btn3Up>:	storm_motion_input(up3)     ManagerGadgetActivate()";

/*	Translation table when cursor over winds graph */

String	Wind_translations =
	"<PtrMoved>:	environmental_winds_input(move) \n\
	<Btn1Down>:	environmental_winds_input(down1)   ManagerGadgetArm() \n\
	<Btn1Up>:	environmental_winds_input(up1)     ManagerGadgetActivate() \n\
	<Btn2Down>:	environmental_winds_input(down2)   ManagerGadgetArm() \n\
	<Btn1Up>:	environmental_winds_input(up2)     ManagerGadgetActivate() \n\
	<Btn3Down>:	environmental_winds_input(down3)   ManagerGadgetArm() \n\
	<Btn3Up>:	environmental_winds_input(up3)     ManagerGadgetActivate()";

/*	Global variables						*/

Widget		Top_widget;
Display		*Xdisplay;
unsigned int	Xdepth;
short		Environmental_winds_units_flag = UNITS_KTS;
int		Vad_user_cmd = -1;
int		Model_update_user_cmd = -1;

/*	Local prototypes						*/

void	environmental_winds_expose( Widget, XtPointer, XtPointer );
void	environmental_winds_resize( Widget, XtPointer, XtPointer );
void	environmental_winds_input( Widget, XEvent *, String *, int * );
void	storm_motion_input( Widget, XEvent *, String *, int * );
void	environmental_winds_close( Widget, XtPointer, XtPointer );
void	environmental_winds_close_nosave( Widget, XtPointer, XtPointer );
void	data_entry_close_callback( Widget, XtPointer, XtPointer );
void	environmental_winds_save_callback( Widget, XtPointer, XtPointer );
void	accept_environmental_winds_save( Widget, XtPointer, XtPointer );
void	environmental_winds_save();
void	environmental_winds_save_popup();
void	environmental_winds_undo( Widget, XtPointer, XtPointer );
void	environmental_winds_edit_direction( Widget, XtPointer, XtPointer );
void	environmental_winds_edit_speed( Widget, XtPointer, XtPointer );
void	environmental_winds_vad_update( Widget, XtPointer, XtPointer );
void	accept_vad_update( Widget, XtPointer, XtPointer );
void	cancel_vad_update( Widget, XtPointer, XtPointer );
void	model_update_callback( Widget, XtPointer, XtPointer );
void	accept_model_update( Widget, XtPointer, XtPointer );
void	cancel_model_update( Widget, XtPointer, XtPointer );
void	environmental_winds_units( Widget, XtPointer, XtPointer );
void	change_display_callback( Widget, XtPointer, XtPointer );
void	hci_environmental_winds_clear( Widget, XtPointer, XtPointer );
void	environmental_winds_data_entry( Widget, XtPointer, XtPointer );
void	direction_entry_callback( Widget, XtPointer, XtPointer );
void	speed_entry_callback( Widget, XtPointer, XtPointer );
void	change_environmental_data_callback( Widget, XtPointer, XtPointer );
void	extract_wind_components( float, float, float *, float * );
void	verify_sig_level_wind_callback( Widget, XtPointer, XtPointer );
void	decode_sig_wind_data_callback( Widget, XtPointer, XtPointer );
void	verify_sig_wind_data_callback( Widget, XtPointer, XtPointer );
void	interpolate_toggle_callback( Widget, XtPointer, XtPointer );
void	disable_vad_callback( Widget, XtPointer, XtPointer );
void	no_disable_vad_callback( Widget, XtPointer, XtPointer );
void	disable_model_callback( Widget, XtPointer, XtPointer );
void	no_disable_model_callback( Widget, XtPointer, XtPointer );
void	update_table_text();
void	environmental_winds_update( en_t, void *, size_t );
void	model_update_notification( int, LB_id_t, int, void * ); 
void	default_storm_motion_display();
void	hail_temperature_height_display( Pixel, Pixel, Pixel, Pixel );
void	environmental_winds_display( Pixel, Pixel, Pixel, Pixel );
void	hci_make_windbarb( XSegment *, float, float, int, int, float, int * );
float	hci_find_azimuth( int, int, int, int );
void	timer_proc();
void    adaptation_callback( int, LB_id_t, int, void * );
void    Get_hail_algorithm_data();
void    Get_storm_tracking_algorithm_data();
int     Set_storm_tracking_algorithm_data( int, float );
int     Set_hail_algorithm_data( float, float, int, int, int, int, int, int );
void	update_model_update_toggles();
void	update_vad_toggles();
int	data_widget_open();

/*	Definitions for various edit cursors.				*/

Cursor		Draw_cursor = (Cursor) NULL;
Cursor		Move_cursor = (Cursor) NULL;

/*	Declaration for the data structure containing the global 	*
 *	environmental wind data.					*/

int	id;
char	Buf [256];
char	max_dir_buf[ 64 ] = " Height: 00.0 - Direction: 000  ";
int	max_dir_width = -1;;
int	dir_x = -1;
int	dir_y = -1;
char	max_speed_buf[ 64 ] = " Height: 00.0 - Speed: 000  ";
int	max_speed_width = -1;;
int	speed_x = -1;
int	speed_y = -1;
int	font_height = -1;
XFontStruct *fontinfo;

/*	Julian time data.						*/

time_t	tm;

/************************************************************************
 *	Description: This is the main function for the HCI Environmental*
 *		     Winds Editor task.					*
 *									*
 *	Input:  argc - number of command line arguments			*
 *		argv - command line data				*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	Widget		form;
	Widget		button_rowcol;
	Widget		vad_rowcol;
	Widget		vad_update_list;
	Widget		model_update_rowcol;
	Widget		model_update_list;
	Widget		units_rowcol;
	Widget		units_update_list;
	Widget		display_rowcol;
	Widget		display_list;
	XGCValues	gcv;
	int		status;
	XtActionsRec	actions;
	A3cd97		*enw;
	int		retval;
	Arg		arg [10];
	int		n;

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_WIND_TASK );

	Top_widget = HCI_get_top_widget();
	Xdisplay = HCI_get_display();
	Xdepth   = XDefaultDepth (Xdisplay, DefaultScreen(Xdisplay));

/*	Verify site data available.					*/

	if( (status = ORPGSITE_get_site_data( &Site )) < 0 ){

            HCI_LE_error("Unable to Read Site Information (%d)", status);
            HCI_task_exit( HCI_EXIT_FAIL );

	}

/*	Get the elevation of the radar from adaptation data.		*/

	Rda_elevation = (float) Site.rda_elev;
	Rda_elevation = Rda_elevation/1000.0;

/*	Register for any updates to the Hail Algorithm object data	*/
	retval = DEAU_UN_register( HAIL_DEA_NAME, adaptation_callback );
	if( retval < 0 )
	{
          HCI_LE_error("Unable to register HAIL DEAU callback %d", retval );
	  HCI_task_exit (HCI_EXIT_FAIL);	    
	}

/*	Get the Hail algorithm data					*/
	Get_hail_algorithm_data();

/*	Register for any updates to the Storm Cell Tracking object data	*/
	retval = DEAU_UN_register( STORM_CELL_TRACK_DEA_NAME, adaptation_callback );
	if( retval < 0 )
	{
          HCI_LE_error("Unable to register STORM CELL TRACK DEAU callback %d", retval );
	  HCI_task_exit (HCI_EXIT_FAIL);	    
	}

/*	Get the Storm Cell Tracking algorithm data			*/
	Get_storm_tracking_algorithm_data();

/*	Use a form widget to manage placement of various widgets	*
 *	in window.							*/

	form = XtVaCreateWidget ("environmental_winds_form",
		xmFormWidgetClass,	Top_widget,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	If low bandwidth, display a progress meter since we need to do	*
 *	I/O.								*/

	HCI_PM("Initialize Task Information");		

/*      Register for winds update events.                               */

        ORPGDA_write_permission( HCI_EWT_LB );
        status = ORPGDA_UN_register ( HCI_EWT_LB,
                                      LBID_EWT_UPT, 
				      (void *) model_update_notification);

        if (status != 0) {

            HCI_LE_status("UN_register EWT_UPT Updates: %d", status);

        }

/*	Register for winds update events.				*/

	status = EN_register (ORPGEVT_ENVWND_UPDATE,
			(void *) environmental_winds_update);

	if (status != 0) {

	    HCI_LE_log("EN_register ORPGEVT_ENVWND_UPDATE: %d", status);

	}

/*	Initialize the environmental wind data buffers.			*/

	HCI_PM("Reading wind data");		
	
	status = hci_read_environmental_wind_data ();
	
	if (status == RMT_CANCELLED)
     	    HCI_task_exit (HCI_EXIT_SUCCESS);
	else
	if (status < 0)
	{
	    HCI_LE_error("Read environmental winds data failed: %d", status);
	    HCI_task_exit (HCI_EXIT_FAIL);	    
	} 
	
/*	Use a row_column widget to manage action selections.		*/

	button_rowcol = XtVaCreateWidget ("environmental_winds_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Close1_button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,button_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
	XtAddCallback (Close1_button,
		XmNactivateCallback, environmental_winds_close, NULL);

	Save1_button = XtVaCreateManagedWidget ("Save",
		xmPushButtonWidgetClass,button_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);
	XtAddCallback (Save1_button,
		XmNactivateCallback, environmental_winds_save_callback, NULL);

	Undo1_button = XtVaCreateManagedWidget ("Undo",
		xmPushButtonWidgetClass,button_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);
	XtAddCallback (Undo1_button,
		XmNactivateCallback, environmental_winds_undo, NULL);
	
	Clear1_button = XtVaCreateManagedWidget ("Clear",
		xmPushButtonWidgetClass,button_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Clear1_button,
		XmNactivateCallback, hci_environmental_winds_clear, NULL);

	Data_Entry_button = XtVaCreateManagedWidget ("Data Entry",
		xmPushButtonWidgetClass,button_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Data_Entry_button,
		XmNactivateCallback,
		environmental_winds_data_entry,
		(XtPointer) NULL);
	
	XtManageChild (button_rowcol);

/*	Set args for all toggle buttons.				*/

	n = 0;
	XtSetArg (arg[n], XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg[n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg[n], XmNorientation,	XmHORIZONTAL); n++;
	XtSetArg (arg[n], XmNmarginHeight,	1); n++;
	XtSetArg (arg[n], XmNmarginWidth,	1); n++;
	XtSetArg (arg[n], XmNspacing,		0); n++;

/*	Use a row_column widget to manage the VAD update		*
 *	toggle selections.						*/

	vad_rowcol = XtVaCreateWidget ("vad_update_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		button_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("     VAD Update: ",
		xmLabelWidgetClass,	vad_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	vad_update_list = XmCreateRadioBox (vad_rowcol,
		"vad_update_list", arg, n);

	Vad_update_yes = XtVaCreateManagedWidget ("On",
		xmToggleButtonWidgetClass,	vad_update_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (NORMAL_COLOR),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (Vad_update_yes,
		XmNvalueChangedCallback,
		environmental_winds_vad_update,
		(XtPointer) HCI_YES_FLAG);

	Vad_update_no = XtVaCreateManagedWidget ("Off",
		xmToggleButtonWidgetClass,	vad_update_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (Vad_update_no,
		XmNvalueChangedCallback,
		environmental_winds_vad_update,
		(XtPointer) HCI_NO_FLAG);

	XtManageChild (vad_update_list);
	XtManageChild (vad_rowcol);

/*	Highlight the currently active toggle				*/

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

	if (enw->envwndflg == HCI_YES_FLAG) {

	    XmToggleButtonSetState (Vad_update_yes,
			True, False);
	    XmToggleButtonSetState (Vad_update_no,
			False, False);

	} else {

	    XmToggleButtonSetState (Vad_update_no,
			True, False);
	    XmToggleButtonSetState (Vad_update_yes,
			False, False);

	}

/*	Use a row_column widget to manage the model update		*
 *	toggle selections.						*/

	model_update_rowcol = XtVaCreateWidget ("model_update_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		vad_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("   Model Update: ",
		xmLabelWidgetClass,	model_update_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	model_update_list = XmCreateRadioBox (model_update_rowcol,
		"model_update_list", arg, n);

	Model_update_yes = XtVaCreateManagedWidget ("On",
		xmToggleButtonWidgetClass,	model_update_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (NORMAL_COLOR),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (Model_update_yes,
		XmNvalueChangedCallback,
		model_update_callback,
		(XtPointer) MODEL_UPDATE_ALLOWED);

	Model_update_no = XtVaCreateManagedWidget ("Off",
		xmToggleButtonWidgetClass,	model_update_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WARNING_COLOR),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (Model_update_no,
		XmNvalueChangedCallback,
		model_update_callback,
		(XtPointer) MODEL_UPDATE_DISALLOWED);

	XtManageChild (model_update_list);
	XtManageChild (model_update_rowcol);

/*	Highlight the currently active toggle				*/

	retval = hci_get_model_update_flag();

	if( retval == MODEL_UPDATE_ALLOWED)
	{
	    XmToggleButtonSetState (Model_update_yes,
			True, False);
	    XmToggleButtonSetState (Model_update_no,
			False, False);
	}
	else
	{
	    XmToggleButtonSetState (Model_update_no,
			True, False);
	    XmToggleButtonSetState (Model_update_yes,
			False, False);
	}

/*	Use a row_column widget to manage the display			*
 *	toggle selections.						*/

	display_rowcol = XtVaCreateWidget ("display_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		model_update_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("        Display: ",
		xmLabelWidgetClass,	display_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	display_list = XmCreateRadioBox (display_rowcol,
		"display_list", arg, n);

	display_current = XtVaCreateManagedWidget ("Current",
		xmToggleButtonWidgetClass,	display_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (display_current,
		XmNvalueChangedCallback,
		change_display_callback,
		(XtPointer) DISPLAY_CURRENT);

	display_model = XtVaCreateManagedWidget ("Model",
		xmToggleButtonWidgetClass,	display_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (display_model,
		XmNvalueChangedCallback,
		change_display_callback,
		(XtPointer) DISPLAY_MODEL);

	XtManageChild (display_list);
	XtManageChild (display_rowcol);

/*	Set initial toggle values. 					*/

	Display_flag = hci_get_ewt_display_flag();
	if (Display_flag == DISPLAY_CURRENT) {

	    XmToggleButtonSetState (display_current,
			True, False);
	    XmToggleButtonSetState (display_model,
			False, False);

	} else {

	    XmToggleButtonSetState (display_model,
			True, False);
	    XmToggleButtonSetState (Units_kts,
			False, False);

	}

/*	Use a row_column widget to manage the units			*
 *	toggle selections.						*/

	units_rowcol = XtVaCreateWidget ("vad_update_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		display_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("          Units: ",
		xmLabelWidgetClass,	units_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	units_update_list = XmCreateRadioBox (units_rowcol,
		"units_update_list", arg, n);

	Units_kts = XtVaCreateManagedWidget ("kts",
		xmToggleButtonWidgetClass,	units_update_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (Units_kts,
		XmNvalueChangedCallback,
		environmental_winds_units,
		(XtPointer) UNITS_KTS);

	Units_mps = XtVaCreateManagedWidget ("m/s",
		xmToggleButtonWidgetClass,	units_update_list,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNhighlightThickness,	0,
		NULL);

	XtAddCallback (Units_mps,
		XmNvalueChangedCallback,
		environmental_winds_units,
		(XtPointer) UNITS_MPS);

	XtManageChild (units_update_list);
	XtManageChild (units_rowcol);

	if (Environmental_winds_units_flag == UNITS_KTS) {

	    XmToggleButtonSetState (Units_kts,
			True, False);
	    XmToggleButtonSetState (Units_mps,
			False, False);

	} else {

	    XmToggleButtonSetState (Units_mps,
			True, False);
	    XmToggleButtonSetState (Units_kts,
			False, False);

	}

/*	Define the drawing_area widget to graphically display the	*
 *	vertical wind profile.						*/

	actions.string = "storm_motion_input";
	actions.proc   = (XtActionProc) storm_motion_input;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	Storm_widget = XtVaCreateWidget (
		"storm_motion_drawing_area",
		xmDrawingAreaWidgetClass,	form,
		XmNwidth,			Storm_width,
		XmNheight,			Storm_height,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNtopAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtranslations,		XtParseTranslationTable (Storm_translations),
		NULL);

	XtManageChild (Storm_widget);

/*	Define the drawing_area widget to graphically display the	*
 *	vertical wind profile.						*/

	actions.string = "environmental_winds_input";
	actions.proc   = (XtActionProc) environmental_winds_input;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	Draw_widget = XtVaCreateWidget (
		"environmental_winds_drawing_area",
		xmDrawingAreaWidgetClass,	form,
		XmNwidth,			Winds_width,
		XmNheight,			Winds_height,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNtopWidget,			vad_rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtranslations,		XtParseTranslationTable (Wind_translations),
		NULL);

	XtAddCallback (Draw_widget,
		XmNexposeCallback, environmental_winds_expose, NULL);
	XtAddCallback (Draw_widget,
		XmNresizeCallback, environmental_winds_resize, NULL);

	XtManageChild (Draw_widget);
	XtManageChild (form);
	XtRealizeWidget (Top_widget);

/*	Define the base X display variables.				*/

	Storm_window = XtWindow (Storm_widget);

	Storm_pixmap = XCreatePixmap (Xdisplay,
		Storm_window,
		Storm_width,
		Storm_height,
		Xdepth);

	Winds_window = XtWindow (Draw_widget);

	Winds_pixmap = XCreatePixmap (Xdisplay,
		Winds_window,
		Winds_width,
		Winds_height,
		Xdepth);

	if (Winds_gc == (GC) NULL) {

	    gcv.foreground = hci_get_read_color (BLACK);
	    gcv.background = hci_get_read_color (WHITE);
	    gcv.graphics_exposures = FALSE;

	    Winds_gc = XCreateGC (Xdisplay,
		Winds_window,
		GCBackground | GCForeground | GCGraphicsExposures, &gcv);

	    XSetFont (Xdisplay,
		      Winds_gc,
		      hci_get_font (MEDIUM));

	}

	XSetForeground (Xdisplay,
			Winds_gc,
			hci_get_read_color (WHITE));
	XFillRectangle (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			0, 0,
			Winds_width,
			Winds_height);

/*	Define a different cursor for moving and drawing inside the	*
 *	winds graph.							*/

	if (Draw_cursor == (Cursor) NULL) {

	    Draw_cursor = XCreateFontCursor (Xdisplay, XC_pencil);

	}

	if (Move_cursor == (Cursor) NULL) {

	    Move_cursor = XCreateFontCursor (Xdisplay, XC_plus);

	}

	XtRealizeWidget (Top_widget);
				 
/*	Set font/spacing variables for later use.			*/

	fontinfo = hci_get_fontinfo(SCALED);
	font_height = fontinfo->ascent + fontinfo->descent;
	max_dir_width = XTextWidth( fontinfo,
				    max_dir_buf,
				    strlen( max_dir_buf ) );
	dir_x = ( ( Winds_width /2 ) - max_dir_width ) / 2;
	dir_y = ( Winds_height / 6 ) - 30;
	max_speed_width = XTextWidth( fontinfo,
				      max_speed_buf,
				      strlen( max_speed_buf ) );
	speed_x = ( ( Winds_width /2 ) - max_speed_width ) / 2;
	speed_x += ( Winds_width / 2 );
	speed_y = ( Winds_height / 6 ) - 30;

/*	Start HCI loop.							*/

	HCI_start( timer_proc, HCI_ONE_SECOND, RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This function is the callback for window resize	*
 *		     events.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_resize (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Pixel	pixel1, pixel2;
	Pixel	scanl1, scanl2;
	Pixel	pixel, scanl;
	int	middle_pixel;
	int	i;
	char	buf [128];

	if ((Draw_widget  == (Widget) NULL) ||
	    (Winds_window == (Window) NULL) ||
	    (Winds_pixmap == (Pixmap) NULL) ||
	    (Winds_gc     == (GC)     NULL)) {

	    return;

	}

/*	Get the new drawing area width and height.  Destroy the old	*
 *	pixmap first so we can create a new one at the new size.	*/

	XtVaGetValues (Draw_widget,
		XmNwidth,	&Winds_width,
		XmNheight,	&Winds_height,
		NULL);

	XFreePixmap (Xdisplay, Winds_pixmap);

	Winds_pixmap = XCreatePixmap (Xdisplay,
		Winds_window,
		Winds_width,
		Winds_height,
		Xdepth);
	
/*	Define the boundaries (in pixels/scanlines) for the region	*
 *	in the drawing area to contain the wind plot.			*/

	pixel1 = Winds_width/10;
	scanl1 = Winds_height/6;
	pixel2 = Winds_width - Winds_width/10;
	scanl2 = Winds_height - Winds_height/10;
	middle_pixel = (pixel1+pixel2)/2;

/*	Initialize the drawing area by filling the wind direction	*
 *	and wind speed graph background to white and the rest green	*/

	XSetForeground (Xdisplay,
			Winds_gc,
			hci_get_read_color (BACKGROUND_COLOR1));
	XFillRectangle (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			0, 0,
			Winds_width,
			Winds_height);

	XSetForeground (Xdisplay,
			Winds_gc,
			hci_get_read_color (WHITE));

	XFillRectangle (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			pixel1, scanl1,
			(int) (middle_pixel-pixel1-70),
			(int) (scanl2-scanl1));

	XFillRectangle (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			(int) (middle_pixel+70),
			scanl1,
			(int) (middle_pixel-pixel1-70),
			(int) (scanl2-scanl1));

	XSetLineAttributes (Xdisplay,
			Winds_gc,
			(unsigned int)	2,
			LineSolid,
			CapButt,
			JoinMiter);

	XSetForeground (Xdisplay,
			Winds_gc,
			hci_get_read_color (BLACK));

/*	Draw the template for the plot.					*/

	XDrawLine (Xdisplay,
		   Winds_pixmap,
		   Winds_gc,
		   pixel1, scanl2,
		   pixel2, scanl2);

	XDrawLine (Xdisplay,
		   Winds_pixmap,
		   Winds_gc,
		   middle_pixel, scanl2,
		   middle_pixel, scanl1);

	XSetLineAttributes (Xdisplay,
			Winds_gc,
			(unsigned int)	1,
			LineOnOffDash,
			CapButt,
			JoinMiter);

	for (i=0;i<5;i++) {

		pixel = i*(middle_pixel-pixel1-70)/4;

		sprintf (buf,"%i", i*90);

		XDrawString (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			pixel1 + pixel - (strlen (buf)*4),
			scanl2+15,
			buf, strlen (buf));

		sprintf (buf,"%i", ((int) (i*(MAX_ENV_WIND_SPEED/4)*Units_conversion+0.5)));

		XDrawString (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			middle_pixel+70+pixel - (strlen (buf)*4),
			scanl2+15,
			buf, strlen (buf));

		XDrawLine (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			pixel1+pixel, scanl1,
			pixel1+pixel, scanl2);
		XDrawLine (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			middle_pixel+70+pixel, scanl1,
			middle_pixel+70+pixel, scanl2);

	}

	pixel = (pixel1+pixel2)/4;

	sprintf (buf,"deg");

	XDrawString (Xdisplay,
		Winds_pixmap,
		Winds_gc,
		pixel,
		scanl2+30,
		buf, strlen (buf));

	if (Environmental_winds_units_flag == UNITS_KTS) {

	    sprintf (buf,"kts");

	} else {

	    sprintf (buf,"m/s");

	}

	pixel = 3*(pixel1+pixel2)/4;

	XDrawString (Xdisplay,
		Winds_pixmap,
		Winds_gc,
		pixel,
		scanl2+30,
		buf, strlen (buf));

	for (i=0;i<=LEN_EWTAB;i=i+10) {

		scanl = scanl2 - i*(scanl2-scanl1)/(LEN_EWTAB);
		sprintf (buf,"%4.1f", (i+Rda_elevation));

		XDrawString (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			pixel1 - 40,
			scanl+5,
			buf, 4);

		XDrawLine (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			pixel1, scanl,
			(int) (middle_pixel-70), scanl);

		XDrawLine (Xdisplay,
			Winds_pixmap,
			Winds_gc,
			(int) (middle_pixel+70), scanl,
			pixel2, scanl);

	}

	XSetLineAttributes (Xdisplay,
		Winds_gc,
		(unsigned int)	1,
		LineSolid,
		CapButt,
		JoinMiter);

/*	Xdisplay the wind data over the template.			*/

	environmental_winds_display (pixel1, scanl1, pixel2, scanl2);
	hail_temperature_height_display (pixel1, scanl1, pixel2, scanl2);

	XCopyArea (Xdisplay,
		   Winds_pixmap,
		   Winds_window,
		   Winds_gc,
		   0, 0,
		   Winds_width,
		   Winds_height,
		   0, 0);

	Old_winds_width  = Winds_width;
	Old_winds_height = Winds_height;

/*	Initialize the default storm motion pixmap.		*/

	default_storm_motion_display ();

	update_table_text ();
}

/************************************************************************
 *	Description: This function is the callback for window expose	*
 *		     events.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_expose (
Widget		w,
XtPointer	call_data,
XtPointer	client_data
)
{

/*	If any of the X variables are undefined, return.		*/

	if ((Draw_widget  == (Widget) NULL) ||
	    (Winds_window == (Window) NULL) ||
	    (Winds_pixmap == (Pixmap) NULL) ||
	    (Winds_gc     == (GC)     NULL)) {

	    return;

	}

/*	Get the width and height of the top level drawing area widget	*/

	XtVaGetValues (Draw_widget,
		XmNwidth,	&Winds_width,
		XmNheight,	&Winds_height,
		NULL);

/*	If the width or height changed, force the resize procedure	*/

	if ((Winds_width  != Old_winds_width) ||
	    (Winds_height != Old_winds_height)) {

	    environmental_winds_resize (w,
					(XtPointer) NULL,
					(XtPointer) NULL);

	} else {

		XCopyArea (Xdisplay,
			   Winds_pixmap,
			   Winds_window,
			   Winds_gc,
			   0, 0,
			   Winds_width,
			   Winds_height,
			   0, 0);

		XCopyArea (Xdisplay,
			   Storm_pixmap,
			   Storm_window,
			   Winds_gc,
			   0, 0,
			   Storm_width,
			   Storm_height,
			   0, 0);

	}
}

/************************************************************************
 *	Description: This function graphically displays the default	*
 *		     storm motion vector in the upper right corner of	*
 *		     the environmental data editor window.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
default_storm_motion_display (
)
{
	int	center_pixel;
	int	center_scanl;
	float	scale;
	int	npoints;
	char	buf [128];

/*	Get the coordinates to be used as the center of the graph.	*/

	center_pixel = Storm_width/2;
	center_scanl = 20 + (Storm_height-30)/2;
	scale  = MAX_STORM_WIND_SPEED/((Storm_height-30.0)/2.0);

/*	Clear the graph region.						*/

	XSetForeground (Xdisplay,
			Winds_gc,
			hci_get_read_color (WHITE));

	XFillRectangle (Xdisplay,
			Storm_pixmap,
			Winds_gc,
			0, 0,
			Storm_width,
			Storm_height);

/*	Make the lines wider for better visibility			*/
	XSetLineAttributes (Xdisplay,
		Winds_gc,
		(unsigned int)	2,
		LineSolid,
		CapButt,
		JoinMiter);

/*	Draw a border around the graph region.				*/
	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (BLACK));

	XDrawRectangle (Xdisplay,
			Storm_pixmap,
			Winds_gc,
			1, 1,
			Storm_width-2,
			Storm_height-2);

/*	Display the storm motion as a wind barb.			*/

	hci_make_windbarb (Windbarb,
	       (float) Storm_speed,
	       (float) Storm_direction,
	       (int) (center_pixel),
	       (int) (center_scanl),
	       (float) ((Storm_height-30)/2.0),
	       (int *) &npoints);

	XDrawSegments (Xdisplay,
		       Storm_pixmap,
		       Winds_gc,
		       Windbarb,
		       npoints);

	XSetLineAttributes (Xdisplay,
		Winds_gc,
		(unsigned int)	0,
		LineSolid,
		CapButt,
		JoinMiter);

	XDrawLine (Xdisplay,
		   Storm_pixmap,
		   Winds_gc,
		   center_pixel-4,
		   center_scanl,
		   center_pixel+4,
		   center_scanl);

	XDrawLine (Xdisplay,
		   Storm_pixmap,
		   Winds_gc,
		   center_pixel,
		   center_scanl-4,
		   center_pixel,
		   center_scanl+4);

/*	Also, display the storm motion textually.			*/

	sprintf (buf,"Storm Motion [%3d,%4.1f]",
		Storm_direction, Storm_speed);
	XDrawString (Xdisplay,
	 	   Storm_pixmap,
		   Winds_gc,
		   (int) (4),
		   (int) (15),
		   buf,
		   strlen (buf));

	XCopyArea (Xdisplay,
		   Storm_pixmap,
		   Storm_window,
		   Winds_gc,
		   0, 0,
		   Storm_width,
		   Storm_height,
		   0, 0);

}


/************************************************************************
 *	Description: This function is used to display graphically the	*
 *		     heights of the 0 deg C and -20 deg C temperature	*
 *		     levels.  They are represented by horizontal lines	*
 *		     inside the wind profile plots.			*
 *									*
 *	Input:  left_pixel   - leftmost pixel of wind graph		*
 *		top_scanl    - topmost scanline of wind graph		*
 *		right_pixel  - rightmost pixel of wind graph		*
 *		bottom_scanl - bottommost scanline of wind graph	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/


void
hail_temperature_height_display (
Pixel	left_pixel,
Pixel	top_scanl,
Pixel	right_pixel,
Pixel	bottom_scanl
)
{
	int	pixel1;
	int	pixel2;
	int	scanl_0;
	int	scanl_minus_20;
	float	scale;
	char	buf [128];
	int	temp_pixel;

/*	Determine the right pixel of the left graph (pixel1) and the	*
 *	left pixel of the right graph (pixel2).				*/

	scale  = (bottom_scanl - top_scanl)/69.9;
	pixel1 = (right_pixel+left_pixel)/2 - 70;
	pixel2 = (right_pixel+left_pixel)/2 + 70;

/*	Use a thick line to display the hail temperature heights in 	*
 *	each wind graph.						*/

	XSetLineAttributes (Xdisplay,
		Winds_gc,
		(unsigned int)	3,
		LineSolid,
		CapButt,
		JoinMiter);

/*	Determine the scanline for the 0 and -20 degree hail		*
 *	temperature heights.						*/

	scanl_0        = (int) (bottom_scanl -
				(Hail_0_height-Rda_elevation)*scale);
	scanl_minus_20 = (int) (bottom_scanl -
				(Hail_minus_20_height-Rda_elevation)*scale);

/*	Display the 0 degree line in cyan.				*/

	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (CYAN));

	XDrawLine (Xdisplay,
	 	   Winds_pixmap,
		   Winds_gc,
		   left_pixel,
		   scanl_0,
		   pixel1,
		   scanl_0);

	XDrawLine (Xdisplay,
	 	   Winds_pixmap,
		   Winds_gc,
		   pixel2,
		   scanl_0,
		   right_pixel,
		   scanl_0);

/*	Display the -20 degree line in dark blue.			*/

	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (BLUE));

	XDrawLine (Xdisplay,
	 	   Winds_pixmap,
		   Winds_gc,
		   left_pixel,
		   scanl_minus_20,
		   pixel1,
		   scanl_minus_20);

	XDrawLine (Xdisplay,
	 	   Winds_pixmap,
		   Winds_gc,
		   pixel2,
		   scanl_minus_20,
		   right_pixel,
		   scanl_minus_20);

	XSetLineAttributes (Xdisplay,
		Winds_gc,
		(unsigned int)	1,
		LineSolid,
		CapButt,
		JoinMiter);

/*	Annotate each line to the right of the right graph.		*/

	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (BLACK));

	XDrawString (Xdisplay,
	 	   Winds_pixmap,
		   Winds_gc,
		   (int) (right_pixel+4),
		   (int) (scanl_0+4),
		   "0 C",
		   3);

	XDrawString (Xdisplay,
	 	   Winds_pixmap,
		   Winds_gc,
		   (int) (right_pixel+4),
		   (int) (scanl_minus_20+4),
		   "-20 C",
		   5);

/*	Display the hail update time above the right graph.		*/

        sprintf (buf,"Hail Updated %2.2d/%2.2d/%2.2d - %2.2d:%2.2d:%2.2d",
                        Hail_date[1], Hail_date[2], Hail_date[0],
                        Hail_time[0], Hail_time[1], Hail_time[2]);

/*	Make sure string is centered in the horizontal.			*/

	temp_pixel = Winds_width / 2;
	temp_pixel -= XTextWidth( fontinfo, buf, strlen( buf ) );
	temp_pixel /= 2;
	if( temp_pixel < 0 ){ temp_pixel = 0; }
	temp_pixel += Winds_width / 2;

	XDrawString (Xdisplay,
		     Winds_pixmap,
		     Winds_gc,
		     (int) temp_pixel,
		     (int) (top_scanl-35),
		     buf,
		     strlen (buf));

}

/************************************************************************
 *	Description: This function displays graphically the contents	*
 *		     of the environmental winds table.  A pair of lines	*
 *		     representing the direction and speed are drawn	*
 *		     along with a set of wind barbs at every other	*
 *		     level.						*
 *									*
 *	Input:  left_pixel   - leftmost pixel of wind graph		*
 *		top_scanl    - topmost scanline of wind graph		*
 *		right_pixel  - rightmost pixel of wind graph		*
 *		bottom_scanl - bottommost scanline of wind graph	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_display (
Pixel	left_pixel,
Pixel	top_scanl,
Pixel	right_pixel,
Pixel	bottom_scanl
)
{
	int	i;
	int	pixel1, scanl1;
	int	pixel2, scanl2;
	Pixel	middle_pixel;
	int	month, day, year;
	int	hour, minute, second;
	int	npoints;
	A3cd97	*enw;
	char	buf [128];
	int	temp_pixel;

/*	Calculate the middle pixel.  It will be used to display a	*
 *	vertical wind barb profile between the direction and speed	*
 *	graphs.								*/

	middle_pixel = (left_pixel + right_pixel)/2;

	XSetLineAttributes (Xdisplay,
		Winds_gc,
		(unsigned int)	1,
		LineSolid,
		CapButt,
		JoinMiter);

/*	Plot the wind barbs at every other level in the middle of	*
 *	the display window.						*/

	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (BLACK));

/*	Print the time the table was last updated at the top of the	*
 *	drawing area.							*/

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

	if (enw->sound_time == 0) {

	    tm = 1;

	} else {

	    tm = enw->sound_time * 60;

	}

	unix_time (&tm,
		   &year,
		   &month,
		   &day,
		   &hour,
		   &minute,
		   &second);

	sprintf (buf,"Winds Updated %2.2d/%2.2d/%2.2d - %2.2d:%2.2d:%2.2d",
			month, day, year%100, hour, minute, second);

/*	Make sure string is centered in the horizontal.			*/

	temp_pixel = Winds_width / 2;
	temp_pixel -= XTextWidth( fontinfo, buf, strlen( buf ) );
	temp_pixel /= 2;
	if( temp_pixel < 0 ){ temp_pixel = 0; }

	XDrawString (Xdisplay,
		     Winds_pixmap,
		     Winds_gc,
		     (int) temp_pixel,
		     (int) (top_scanl-35),
		     buf,
		     strlen (buf));

/*	Xdisplay mouse button help text along the bottom of the display	*/

	XSetFont (Xdisplay,
		  Winds_gc,
		  hci_get_font (SMALL));

	sprintf (buf,"Edit/Clear data in either graph by dragging the cursor while the left/right mouse button is pressed");

	XDrawString (Xdisplay,
		     Winds_pixmap,
		     Winds_gc,
		     10,
		     (int) (Winds_height-2),
		     buf,
		     strlen (buf));

	XSetFont (Xdisplay,
		  Winds_gc,
		  hci_get_font (MEDIUM));

/*	Plot a wind barb at every other level.				*/

	for (i=1;i<LEN_EWTAB;i=i+2) {

	    if ((enw->ewtab [WNDDIR][i] >=   0) &&
		(enw->ewtab [WNDDIR][i] <= 360)) {

		if ((enw->ewtab [WNDSPD][i] >=    0) &&
		    (enw->ewtab [WNDSPD][i] <= MAX_ENV_WIND_SPEED)) {

		    hci_make_windbarb (Windbarb,
			      (float) enw->ewtab [WNDSPD][i] * Units_conversion,
			      (float) (enw->ewtab [WNDDIR][i]),
			      (int) (middle_pixel),
			      (int) (bottom_scanl - i*(bottom_scanl-top_scanl)/69.9),
			      (float) 60.0,
			      (int *) &npoints);

		    XDrawSegments (Xdisplay,
	 		       Winds_pixmap,
			       Winds_gc,
			       Windbarb,
			       npoints);

		}
	    }
	}

/*	Plot the wind direction on the left graph			*/

	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (BLUE));

	if (Winds_edit_type == EDIT_WNDDIR) {

		XSetLineAttributes (Xdisplay,
			Winds_gc,
			(unsigned int)	3,
			LineSolid,
			CapButt,
			JoinMiter);

	} else {

		XSetLineAttributes (Xdisplay,
			Winds_gc,
			(unsigned int)	1,
			LineSolid,
			CapButt,
			JoinMiter);

	}

/*	Draw a line representing the directional value between each	*
 *	point in the table.  If data are missing at a particular level,	*
 *	write an X at the location where the last valid data were	*
 *	read.  Also it is assumed that the wind direction between two	*
 *	consecutive levels does not change more than 180 degrees	*/

	for (i=1;i<LEN_EWTAB;i++) {

	    if ((enw->ewtab [WNDDIR][i] >=   0) &&
		(enw->ewtab [WNDDIR][i] <= 360)) {

		if ((enw->ewtab [WNDDIR][i-1] >=   0) &&
		    (enw->ewtab [WNDDIR][i-1] <= 360)) {

/*		    Check to see if the difference in direction between	*
 *		    levels is more than 180 degrees.			*/

		    if (fabs ((double) (enw->ewtab [WNDDIR][i]-
			enw->ewtab [WNDDIR][i-1])) > 180.0) {

			if (enw->ewtab [WNDDIR][i] > 180.0) {

			    pixel1 = left_pixel + 
			    	(enw->ewtab [WNDDIR][i]*
			    	(middle_pixel-left_pixel-70))/360;
			    scanl1 = bottom_scanl - i*
			    	(bottom_scanl-top_scanl)/69.9;
			    pixel2 = middle_pixel-70;
			    scanl2 = bottom_scanl - (i-
				(360-enw->ewtab [WNDDIR][i])/180)*
			        (bottom_scanl-top_scanl)/69.9;

			    XDrawLine (Xdisplay,
				       Winds_pixmap,
				       Winds_gc,
				       pixel1, scanl1,
				       pixel2, scanl2);

			    pixel1 = left_pixel + 
			    	(enw->ewtab [WNDDIR][i-1]*
			    	(middle_pixel-left_pixel-70))/360;
			    scanl1 = bottom_scanl - (i-1)*
			    	(bottom_scanl-top_scanl)/69.9;
			    pixel2 = left_pixel;

			    XDrawLine (Xdisplay,
				       Winds_pixmap,
				       Winds_gc,
				       pixel1, scanl1,
				       pixel2, scanl2);

			} else {

			    pixel1 = left_pixel + 
			    	(enw->ewtab [WNDDIR][i-1]*
			    	(middle_pixel-left_pixel-70))/360;
			    scanl1 = bottom_scanl - (i-1)*
			    	(bottom_scanl-top_scanl)/69.9;
			    pixel2 = middle_pixel-70;
			    scanl2 = bottom_scanl - (i-1 +
				(360-enw->ewtab [WNDDIR][i-1])/180)*
			        (bottom_scanl-top_scanl)/69.9;

			    XDrawLine (Xdisplay,
				       Winds_pixmap,
				       Winds_gc,
				       pixel1, scanl1,
				       pixel2, scanl2);

			    pixel1 = left_pixel + 
			    	(enw->ewtab [WNDDIR][i]*
			    	(middle_pixel-left_pixel-70))/360;
			    scanl1 = bottom_scanl - i*
			    	(bottom_scanl-top_scanl)/69.9;
			    pixel2 = left_pixel;

			    XDrawLine (Xdisplay,
				       Winds_pixmap,
				       Winds_gc,
				       pixel1, scanl1,
				       pixel2, scanl2);

			}

		    } else {

			pixel1 = left_pixel + 
			    (enw->ewtab [WNDDIR][i-1]*
			    (middle_pixel-left_pixel-70))/360;
			scanl1 = bottom_scanl - (i-1)*
			    (bottom_scanl-top_scanl)/69.9;
			pixel2 = left_pixel +
			    (enw->ewtab [WNDDIR][i]*
			    (middle_pixel-left_pixel-70))/360;
			scanl2 = bottom_scanl - i*
			    (bottom_scanl-top_scanl)/69.9;

			XDrawLine (Xdisplay,
				   Winds_pixmap,
				   Winds_gc,
				   pixel1, scanl1,
				   pixel2, scanl2);

		    }

		} else {

		    pixel2 = left_pixel +
			(enw->ewtab [WNDDIR][i]*
			(middle_pixel-left_pixel-70))/360;
		    scanl2 = bottom_scanl - i*
			(bottom_scanl-top_scanl)/69.9;

		    XDrawLine (Xdisplay,
			       Winds_pixmap,
			       Winds_gc,
			       pixel2-2, scanl2-2,
			       pixel2+2, scanl2+2);
		    XDrawLine (Xdisplay,
			       Winds_pixmap,
			       Winds_gc,
			       pixel2+2, scanl2-2,
			       pixel2-2, scanl2+2);

		}
	    }
	}

/*	Plot the wind speed on the graph				*/

	XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (RED));

	if (Winds_edit_type == EDIT_WNDSPD) {

		XSetLineAttributes (Xdisplay,
			Winds_gc,
			(unsigned int)	3,
			LineSolid,
			CapButt,
			JoinMiter);

	} else {

		XSetLineAttributes (Xdisplay, 
			Winds_gc,
			(unsigned int)	1,
			LineSolid,
			CapButt,
			JoinMiter);

	}

	for (i=1;i<LEN_EWTAB;i++) {

	    if ((enw->ewtab [WNDSPD][i] >= 0) &&
	        (enw->ewtab [WNDSPD][i] <= MAX_ENV_WIND_SPEED)) {

		if ((enw->ewtab [WNDSPD][i-1] >= 0) &&
	            (enw->ewtab [WNDSPD][i-1] <= MAX_ENV_WIND_SPEED)) {

		    pixel1 = middle_pixel + 70 + 
			(enw->ewtab [WNDSPD][i-1]*
			(right_pixel-middle_pixel-70))/MAX_ENV_WIND_SPEED;
		    scanl1 = bottom_scanl - (i-1)*
			(bottom_scanl-top_scanl)/69.9;
		    pixel2 = middle_pixel + 70 +
			(enw->ewtab [WNDSPD][i]*
			(right_pixel-middle_pixel-70))/MAX_ENV_WIND_SPEED;
		    scanl2 = bottom_scanl - i*
			(bottom_scanl-top_scanl)/69.9;

		    XDrawLine (Xdisplay,
			       Winds_pixmap,
			       Winds_gc,
			       pixel1, scanl1,
			       pixel2, scanl2);

		} else {

		    pixel2 = middle_pixel + 70 +
			(enw->ewtab [WNDSPD][i]*
			(right_pixel-middle_pixel-70))/MAX_ENV_WIND_SPEED;
		    scanl2 = bottom_scanl - i*
			(bottom_scanl-top_scanl)/69.9;

		    XDrawLine (Xdisplay,
			       Winds_pixmap,
			       Winds_gc,
			       pixel2-2, scanl2-2,
			       pixel2+2, scanl2+2);
		    XDrawLine (Xdisplay,
			       Winds_pixmap,
			       Winds_gc,
			       pixel2+2, scanl2-2,
			       pixel2-2, scanl2+2);

		}
	    }
	}
}

/************************************************************************
 *	Description: This function handles all mouse events while the	*
 *		     cursor is inside the storm motion graph region.	*
 *									*
 *	Input:  w - Widget ID of storm motion drawing area widget	*
 *		event - X event data					*
 *		arg   - user arguments data				*
 *		num_args - number of arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
storm_motion_input (
Widget		w,
XEvent		*event,
String		*args,
int		*num_args
)
{
static	int	inside_flag = 0;
static	int	button_down = 0;
	int	pixel, scanl;
	int	center_pixel, center_scanl;
	int	right_pixel, left_pixel;
	int	top_scanl, bottom_scanl;
	float	azimuth;
	float	speed;
	float	scale;
	float	x, y;

/*	Get the cursor location from event data				*/
	pixel = event->xbutton.x;
	scanl = event->xbutton.y;

/*	Determine the relative location of the cursor from the center	*
 *	of the storm motion region.  The angle determines the direction	*
 *	and distance the speed.						*/
	center_pixel = Storm_width/2;
	center_scanl = 20 + (Storm_height-30)/2;

	left_pixel   = center_pixel - (Storm_height-30)/2;
	right_pixel  = center_pixel + (Storm_height-30)/2;
	top_scanl    = 30;
	bottom_scanl = Storm_height;

	scale = MAX_STORM_WIND_SPEED/((Storm_height-30.0)/2.0);

	x = (pixel-center_pixel);
	y = (scanl-center_scanl);

/*	If the button is down, then we need to recompute the direction	*
 *	and speed based on the location of the cursor with respect to	*
 *	the region center.						*/

	if (button_down) {

	    if ((pixel > right_pixel) ||
	        (pixel < left_pixel)  || 
	        (scanl < top_scanl)   ||
	        (scanl > bottom_scanl)) {

		if (scanl >= top_scanl) {

		    if( pixel > right_pixel )
		        pixel = right_pixel;

		    else if( pixel < left_pixel )
		        pixel = left_pixel;

		    if( scanl > bottom_scanl )
		        scanl = bottom_scanl;

		    XWarpPointer (Xdisplay,
				  Storm_window,
				  Storm_window,
				  0,
				  0,
				  Storm_width,
				  Storm_height,
				  pixel,
				  scanl);

		}

	    }

	    if ((pixel >= left_pixel)  &&
		(pixel <= right_pixel) &&
		(scanl >= top_scanl)   &&
		(scanl <= bottom_scanl)) {

		azimuth = hci_find_azimuth (pixel, scanl,
				center_pixel, center_scanl);
		speed   = sqrt ((double) x*x + y*y)*scale;

		if( speed > MAX_STORM_WIND_SPEED )
		    speed = MAX_STORM_WIND_SPEED;

		Storm_direction = (int) azimuth;
		Storm_speed = speed;

		if (Change_storm_flag == HCI_NOT_CHANGED_FLAG) {

		    if( Display_flag == DISPLAY_CURRENT) {

		        Change_storm_flag = HCI_CHANGED_FLAG;
		        XtVaSetValues (Save1_button,
			  	       XmNsensitive,	True,
				       NULL);

		    }

		    XtVaSetValues (Undo1_button,
				XmNsensitive,	True,
				NULL);

		    if( data_widget_open() ) {

		        if( Display_flag == DISPLAY_CURRENT) 
		            XtVaSetValues (Save2_button,
			  	           XmNsensitive,	True,
				           NULL);
		        XtVaSetValues (Undo2_button,
				XmNsensitive,	True,
				NULL);

		    }

		}

/*		Display the new wind barb.			*/
		default_storm_motion_display ();

		update_table_text ();

		XCopyArea (Xdisplay,
			   Storm_pixmap,
			   Storm_window,
			   Winds_gc,
			   0, 0,
			   Storm_width,
			   Storm_height,
			   0, 0);

	    }

	}

/*	Change the cursor shape if the cursor when the cursor is first	*
 *	moved inside the region.					*/

	if ((pixel > left_pixel)  &&
	    (pixel < right_pixel) &&
	    (scanl > top_scanl)   &&
	    (scanl < bottom_scanl)) {

	    if (inside_flag == 0) {

	        XDefineCursor (Xdisplay,
		       Storm_window,
		       Move_cursor);
		inside_flag = 1;

	    }

/*	Else, the cursor is outside so restore the shape.		*/

	} else {

	    if (inside_flag == 1) {

	        XUndefineCursor (Xdisplay,
			         Storm_window);
		inside_flag = 0;

	    }
	}

/*	If the left mouse button was pressed, change the cursor and	*
 *	set the button down flag to indicate that moving the cursor	*
 *	will change the default storm motion.				*/

	if (!strcmp (args[0], "down1")) {

	    button_down = 1;

	    HCI_LE_log("Default Storm Motion button pushed");
	        
	    if ((pixel < right_pixel)  &&
	        (pixel > left_pixel)   && 
	        (scanl < bottom_scanl) &&
		(scanl > top_scanl)) {

		XDefineCursor (Xdisplay,
			       Storm_window,
			       Draw_cursor);

	    }

/*	The left mouse button is now up so clear the button down flag	*
 *	and stop updating the default storm motion as the cursor is	*
 *	moved.								*/

	} else if (!strcmp (args[0],"up1")) {

	    HCI_LE_log("Default Storm Motion button released");

	    XUndefineCursor (Xdisplay,
			     Storm_window);

	    button_down = 0;

	}
}

/************************************************************************
 *	Description: This function handles all mouse events while the	*
 *		     cursor is inside the environmental winds graph	*
 *		     region.						*
 *									*
 *	Input:  w - Widget ID of winds drawing area widget		*
 *		event - X event data					*
 *		arg   - user arguments data				*
 *		num_args - number of arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_input (
Widget		w,
XEvent		*event,
String		*args,
int		*num_args
)
{
	int	i;
static	int	level1, level2;
static	int	current_level;
static	float	value, value1, value2;
static	float	current_value;
static	int	inside_flag = 0;
static	int	button_down = 0;

	int	level_0;
	int	level_20;
	int	left_pixel, top_scanl;
	int	right_pixel, bottom_scanl;
	int	middle_pixel;
	float	height;
	float	number;
	int	pixel;
	int	scanl;
	A3cd97	*enw;

	left_pixel   = Winds_width/10;
	right_pixel  = Winds_width - left_pixel;
	top_scanl    = Winds_height/6;
	bottom_scanl = Winds_height - Winds_height/10;
	middle_pixel = (right_pixel + left_pixel)/2;
	pixel        = event->xbutton.x;
	scanl        = event->xbutton.y;

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

/*	If we are currently editing wind direction, then keep the	*
 *	cursor in the edit area of the graph, allowing wrap around	*
 *	at 0/360 degrees.						*/

	if (button_down) {

	    if (Winds_edit_type == EDIT_WNDDIR) {

	        if ((event->xbutton.x >= (middle_pixel-70)) ||
	            (event->xbutton.x <  left_pixel)        || 
	            (event->xbutton.y >  bottom_scanl)) {

		    if (event->xbutton.y >= top_scanl) {

		        if (event->xbutton.x >= (middle_pixel-70)) {

		            pixel = left_pixel;

		        } else if (event->xbutton.x < left_pixel) {

		            pixel = middle_pixel-71;

		        }

		        if (event->xbutton.y > bottom_scanl) {

		            scanl = bottom_scanl;

		        }

		        XWarpPointer (Xdisplay,
				  Winds_window,
				  Winds_window,
				  0,
				  0,
				  Winds_width,
				  Winds_height,
				  pixel,
				  scanl);

		    }
	        }

/*	If we are currently editing wind speed, then keep the		*
 *	cursor in the edit area of the graph.				*/

	    } else if (Winds_edit_type == EDIT_WNDSPD) {

	        if ((event->xbutton.x > right_pixel)       ||
	            (event->xbutton.x < (middle_pixel+70)) || 
	            (event->xbutton.y > bottom_scanl)) {

		    if (event->xbutton.y >= top_scanl) {

		        if (event->xbutton.x > right_pixel) {

		            pixel = right_pixel;

		        } else if (event->xbutton.x < (middle_pixel+70)) {

		            pixel = middle_pixel+70;

		        }

		        if (event->xbutton.y > bottom_scanl) {

		            scanl = bottom_scanl;

		        }

		        XWarpPointer (Xdisplay,
				  Winds_window,
				  Winds_window,
				  0,
				  0,
				  Winds_width,
				  Winds_height,
				  pixel,
				  scanl);

		    }
	        }

/*	If we are currently editing hail 0C height, then keep the	*
 *	cursor along the right side of the wind speed graph.		*/

	    } else if ((Winds_edit_type == EDIT_HAIL_0_HEIGHT) ||
	    	       (Winds_edit_type == EDIT_HAIL_MINUS_20_HEIGHT)) {

	        if ((event->xbutton.x > (right_pixel+20)) ||
	            (event->xbutton.x < (right_pixel+ 2)) || 
		    (event->xbutton.y < top_scanl) ||
	            (event->xbutton.y > bottom_scanl)) {

		    if (event->xbutton.y >= top_scanl) {

		        if (event->xbutton.x > right_pixel+20) {

		            pixel = right_pixel+20;

		        } else if (event->xbutton.x < (right_pixel+2)) {

		            pixel = middle_pixel+2;

		        }

		        if (event->xbutton.y > bottom_scanl) {

		            scanl = bottom_scanl;

			} else if (event->xbutton.y < top_scanl) {

		            scanl = top_scanl;

		        }

		        XWarpPointer (Xdisplay,
				  Winds_window,
				  Winds_window,
				  0,
				  0,
				  Winds_width,
				  Winds_height,
				  pixel,
				  scanl);

		    }
	        }
	    }

/*	    Use a interpolation to fill in the values for the levels	*
 *	    between the current cursor location and the location	*
 *	    of the cursor when the left mouse button was pressed.	*/

	    if (button_down == 1) {

		if (Winds_edit_type == EDIT_WNDDIR) {

		    if ((event->xbutton.x >= left_pixel)      &&
		        (event->xbutton.x <= middle_pixel-70) &&
		        (event->xbutton.y >= (top_scanl-5))   &&
		        (event->xbutton.y <= (bottom_scanl+5))) {

		        if (event->xbutton.y < top_scanl)
		            event->xbutton.y = top_scanl;
		        if (event->xbutton.y > bottom_scanl)
		            event->xbutton.y = bottom_scanl;

			level2 = 69.9 * (bottom_scanl - event->xbutton.y) /
				 ((float)(bottom_scanl - top_scanl))+0.5;
			value2 = 360 * (event->xbutton.x - left_pixel) /
				 ((float) (middle_pixel-left_pixel-70));

			current_level = level2;
			current_value = value2;

			if (level2 < level1) {

			    i      = level1;
			    level1 = level2;
			    level2 = i;
			    value  = value1;
			    value1 = value2;
			    value2 = value;

			}

			enw->ewtab [WNDDIR][level1] = value1;

			if (fabs ((double)(value2-value1)) > 180.0) {

			    if (value1 > value2) {

				value2 = value2+360;

			    } else {

				value1 = value1+360;

			    }
			}

			for (i=level1;i<level2;i++) {

			    enw->ewtab [WNDDIR][i+1] =
				value1 + (i-level1+1)*(value2-value1)/
				(level2-level1);

			    if (enw->ewtab [WNDDIR][i+1] >= 360.0) {

				enw->ewtab [WNDDIR][i+1] =
					enw->ewtab [WNDDIR][i+1]-360;

			    }
			}

/*		        Use the resize proc to redraw the wind		*
 *		        profile with the updated winds.			*/

		        environmental_winds_resize ((Widget) NULL,
						    (XtPointer) NULL,
						    (XtPointer) NULL);

		    }

		} else if (Winds_edit_type == EDIT_WNDSPD) {

		    if ((event->xbutton.x >= middle_pixel+70) &&
		        (event->xbutton.x <= right_pixel)     &&
		        (event->xbutton.y >= (top_scanl-5))   &&
		        (event->xbutton.y <= (bottom_scanl+5))) {

		        if (event->xbutton.y < top_scanl)
		            event->xbutton.y = top_scanl;
		        if (event->xbutton.y > bottom_scanl)
		            event->xbutton.y = bottom_scanl;

			level2 = 69.9 * (bottom_scanl - event->xbutton.y) /
				 ((float)(bottom_scanl - top_scanl))+0.5;
			value2 = MAX_ENV_WIND_SPEED * (event->xbutton.x-middle_pixel-70) /
				 ((float) (right_pixel-middle_pixel-70));

			current_level = level2;
			current_value = value2;

			if (level2 < level1) {

			    i      = level1;
			    level1 = level2;
			    level2 = i;
			    value  = value1;
			    value1 = value2;
			    value2 = value;

			}

			enw->ewtab [WNDSPD][level1] = value1;

			for (i=level1;i<level2;i++) {

			    enw->ewtab [WNDSPD][i+1] =
				(value1 + (i-level1+1)*(value2-value1)/
				(level2-level1));

			}

/*		        Use the resize proc to redraw the wind		*
 *		        profile with the updated winds.			*/

		        environmental_winds_resize ((Widget) NULL,
						    (XtPointer) NULL,
						    (XtPointer) NULL);
		    }

		} else if (Winds_edit_type == EDIT_HAIL_0_HEIGHT) {

		    value = 69.9 * (bottom_scanl - event->xbutton.y) /
				 ((float)(bottom_scanl - top_scanl))+0.5;
		    Hail_0_height = value + Rda_elevation;

		    if (Change_hail_flag == HCI_NOT_CHANGED_FLAG) {

		        if( Display_flag == DISPLAY_CURRENT ) {

		            Change_hail_flag = HCI_CHANGED_FLAG;
		            XtVaSetValues (Save1_button,
				           XmNsensitive,	True,
				           NULL);

			}

		        XtVaSetValues (Undo1_button,
				XmNsensitive,	True,
				NULL);

		        if ( data_widget_open() ) {

		            if( Display_flag == DISPLAY_CURRENT )
		                XtVaSetValues (Save2_button,
				               XmNsensitive,	True,
				               NULL);
		            XtVaSetValues (Undo2_button,
				XmNsensitive,	True,
				NULL);

		        }

		    }

/*		    Use the resize proc to redraw the hail height	*
 *		    label at the new level.				*/

		     environmental_winds_resize ((Widget) NULL,
					    (XtPointer) NULL,
					    (XtPointer) NULL);

		} else if (Winds_edit_type == EDIT_HAIL_MINUS_20_HEIGHT) {

		    value = 69.9 * (bottom_scanl - event->xbutton.y) /
				 ((float)(bottom_scanl - top_scanl))+0.5;
		    Hail_minus_20_height = value + Rda_elevation;

		    if (Change_hail_flag == HCI_NOT_CHANGED_FLAG) {

		        if( Display_flag == DISPLAY_CURRENT ) {

		            Change_hail_flag = HCI_CHANGED_FLAG;
		            XtVaSetValues (Save1_button,
			  	           XmNsensitive,	True,
				           NULL);

			}

		        XtVaSetValues (Undo1_button,
				XmNsensitive,	True,
				NULL);

		        if ( data_widget_open() ) {

		            if( Display_flag == DISPLAY_CURRENT )
		                XtVaSetValues (Save2_button,
				               XmNsensitive,	True,
				               NULL);
		            XtVaSetValues (Undo2_button,
				XmNsensitive,	True,
				NULL);

		        }

	    	    }

/*		    Use the resize proc to redraw the hail height	*
 *		    label at the new level.				*/

		    environmental_winds_resize ((Widget) NULL,
					    (XtPointer) NULL,
					    (XtPointer) NULL);
		}

		level1 = current_level;
		value1 = current_value;

	    } else if (button_down == 3) {

		if ((event->xbutton.x >= left_pixel)      &&
		    (event->xbutton.x <= right_pixel)     &&
		    (event->xbutton.y >= (top_scanl-5))   &&
		    (event->xbutton.y <= (bottom_scanl+5))) {

		    if (event->xbutton.y < top_scanl)
			event->xbutton.y = top_scanl;
		    if (event->xbutton.y > bottom_scanl)
			event->xbutton.y = bottom_scanl;

		    level2 = 69.9 * (bottom_scanl - event->xbutton.y) /
			      ((float)(bottom_scanl - top_scanl))+0.5;

		    current_level = level2;

		    if (level2 < level1) {

			i      = level1;
			level1 = level2;
			level2 = i;

		    }

		    for (i=level1;i<=level2;i++) {

			enw->ewtab    [WNDSPD][i] = MTTABLE;
			enw->ewtab    [WNDDIR][i] = MTTABLE;
			enw->newndtab [i][ECOMP]  = MTTABLE;
			enw->newndtab [i][NCOMP]  = MTTABLE;

		    }

		    level1 = current_level;

/*		    Use the resize proc to redraw the wind		*
 *		    profile with the updated winds.			*/

		    environmental_winds_resize ((Widget) NULL,
						(XtPointer) NULL,
						(XtPointer) NULL);

		}
	    }
	}

	if ((event->xbutton.x <= (middle_pixel-70)) &&
	    (event->xbutton.x >= left_pixel)        &&
	    (event->xbutton.y >= top_scanl)         &&
	    (event->xbutton.y <= bottom_scanl)) { 

	    number = 360.0 * ((float)(pixel-left_pixel))/
			   (middle_pixel-left_pixel-70);
	    height = 70.0 * ((float)(bottom_scanl-scanl))/
			    (bottom_scanl-top_scanl);

/*	Make sure spacing is correct.				*/

	    if( height+Rda_elevation < 10.0 )
	    {
	      sprintf (Buf," Height:   %3.1f - Direction: %3d  ",
		height+Rda_elevation, (int) number);
	    }
	    else
	    {
	      sprintf (Buf," Height: %4.1f - Direction: %3d  ",
		height+Rda_elevation, (int) number);
	    }

/*	Draw white rectangle to erase previous info.			*/

	    XSetForeground( Xdisplay,
                    Winds_gc,
                    hci_get_read_color( WHITE ) );

	    XFillRectangle( Xdisplay,
                    Winds_window,
                    Winds_gc,
                    dir_x - 2,
                    dir_y,
                    max_dir_width + 2,
                    font_height + 3 );

/*	Border white rectangle with black.				*/

	    XSetForeground( Xdisplay,
                    Winds_gc,
                    hci_get_read_color( TEXT_FOREGROUND ) );

	    XDrawRectangle( Xdisplay,
                    Winds_window,
                    Winds_gc,
                    dir_x - 2,
                    dir_y,
                    max_dir_width + 2,
                    font_height + 3 );

/*	Draw string.							*/


	    XSetBackground (Xdisplay,
		Winds_gc,
		hci_get_read_color (WHITE));

	    XDrawImageString (Xdisplay,
		Winds_window,
		Winds_gc,
		dir_x,
		dir_y + font_height - 1,
		Buf, strlen (Buf));

	    if (inside_flag == 0) {

	        XDefineCursor (Xdisplay,
			       Winds_window,
			       Move_cursor);
		inside_flag = 1;

	    }

	} else if ((event->xbutton.x >= (middle_pixel+70)) &&
		   (event->xbutton.x <= right_pixel)       &&
		   (event->xbutton.y >= top_scanl)         &&
		   (event->xbutton.y <= bottom_scanl)) {

	    number = MAX_ENV_WIND_SPEED * ((float)(pixel-middle_pixel-70))/
			   (right_pixel-middle_pixel-70);
	    number = number*Units_conversion + 0.5;
	    height = 70.0 * ((float)(bottom_scanl-scanl))/
			    (bottom_scanl-top_scanl);

/*	Make sure spacing is correct.				*/

	    if( height+Rda_elevation < 10.0 )
	    {
	      sprintf (Buf," Height:   %3.1f - Speed: %3d  ",
		height+Rda_elevation, (int) number);
	    }
	    else
	    {
	      sprintf (Buf," Height: %4.1f - Speed: %3d  ",
		height+Rda_elevation, (int) number);
	    }

/*	Draw white rectangle to erase previous info.			*/

	    XSetForeground( Xdisplay,
                    Winds_gc,
                    hci_get_read_color( WHITE ) );

	    XFillRectangle( Xdisplay,
                    Winds_window,
                    Winds_gc,
                    speed_x - 2,
                    speed_y,
                    max_speed_width + 2,
                    font_height + 3 );

/*	Border white rectangle with black.				*/

	    XSetForeground( Xdisplay,
                    Winds_gc,
                    hci_get_read_color( TEXT_FOREGROUND ) );

	    XDrawRectangle( Xdisplay,
                    Winds_window,
                    Winds_gc,
                    speed_x - 2,
                    speed_y,
                    max_speed_width + 2,
                    font_height + 3 );

/*	Draw string.							*/

	    XSetBackground (Xdisplay,
		Winds_gc,
		hci_get_read_color (WHITE));
	    XSetForeground (Xdisplay,
		Winds_gc,
		hci_get_read_color (BLACK));

	    XDrawImageString (Xdisplay,
		Winds_window,
		Winds_gc,
		speed_x,
		speed_y + font_height - 1,
		Buf, strlen (Buf));

	    if (inside_flag == 0) {

	        XDefineCursor (Xdisplay,
		       Winds_window,
		       Move_cursor);
		inside_flag = 1;

	    }

	} else if ((event->xbutton.x < (right_pixel+20)) &&
		   (event->xbutton.x > (right_pixel+2))   &&
		   (event->xbutton.y > top_scanl)         &&
		   (event->xbutton.y < bottom_scanl)) {

	} else {

	    if (inside_flag == 1) {

	        XUndefineCursor (Xdisplay,
			         Winds_window);
		inside_flag = 0;

	    }
	}

	if ((!strcmp (args[0], "down1")) ||
	    (!strcmp (args[0], "down3"))) {

	    if (!strcmp (args[0], "down1")) {

		button_down = 1;

	    } else {

		button_down = 3;

	    }

	    HCI_LE_log("Environmental Data Editor button pushed");
	        
	    if ((event->xbutton.x <  (middle_pixel-70)) &&
	        (event->xbutton.x >= left_pixel)        &&
	        (event->xbutton.y <= bottom_scanl)      &&
		(event->xbutton.y >= top_scanl)) {

		if (Change_winds_flag == HCI_NOT_CHANGED_FLAG) {

		    if( Display_flag == DISPLAY_CURRENT ) {

		        Change_winds_flag = HCI_CHANGED_FLAG;
		        XtVaSetValues (Save1_button,
			               XmNsensitive,	True,
			               NULL);

		    }

		    XtVaSetValues (Undo1_button,
			XmNsensitive,	True,
			NULL);

		    if ( data_widget_open() ) {

		        if( Display_flag == DISPLAY_CURRENT )
			    XtVaSetValues (Save2_button,
			               	   XmNsensitive,	True,
				           NULL);
			XtVaSetValues (Undo2_button,
				XmNsensitive,	True,
				NULL);

		    }
		}

		Winds_edit_type = EDIT_WNDDIR;

		XDefineCursor (Xdisplay,
			Winds_window,
			Draw_cursor);

	    } else if ((event->xbutton.x <  right_pixel)       &&
	        (event->xbutton.x >= (middle_pixel+70)) && 
	        (event->xbutton.y <= bottom_scanl)      &&
		(event->xbutton.y >= top_scanl)) {

		if (Change_winds_flag == HCI_NOT_CHANGED_FLAG) {

		    if( Display_flag == DISPLAY_CURRENT ) {

		        Change_winds_flag = HCI_CHANGED_FLAG;
		        XtVaSetValues (Save1_button,
			               XmNsensitive,	True,
			               NULL);

		    }

		    XtVaSetValues (Undo1_button,
			XmNsensitive,	True,
			NULL);

		    if ( data_widget_open() ) {

		        if( Display_flag == DISPLAY_CURRENT )
			    XtVaSetValues (Save2_button,
				           XmNsensitive,	True,
				           NULL);
			XtVaSetValues (Undo2_button,
				XmNsensitive,	True,
				NULL);

		    }
		}

		Winds_edit_type = EDIT_WNDSPD;

		XDefineCursor (Xdisplay,
			       Winds_window,
			       Draw_cursor);

	    } else if ((event->xbutton.x < (right_pixel+20)) &&
		       (event->xbutton.x > (right_pixel+ 2)) && 
		       (event->xbutton.y <= bottom_scanl)    &&
		       (event->xbutton.y >= top_scanl)) {

		level_0  = (int) (bottom_scanl -
				(Hail_0_height-Rda_elevation)*(bottom_scanl-top_scanl)/69.9);
		level_20 = (int) (bottom_scanl -
				(Hail_minus_20_height-Rda_elevation)*(bottom_scanl-top_scanl)/69.9);

		if (abs ((int) (level_0-event->xbutton.y)) < 10) {

		    Winds_edit_type = EDIT_HAIL_0_HEIGHT;

		    XDefineCursor (Xdisplay,
			       Winds_window,
			       Draw_cursor);

		} else if (abs ((int) (level_20-event->xbutton.y)) < 10) {

		    Winds_edit_type = EDIT_HAIL_MINUS_20_HEIGHT;

		    XDefineCursor (Xdisplay,
			       Winds_window,
			       Draw_cursor);

		}
	    }

/*	    If the left button was pressed, check to see if the	*
 *	    cursor is inside the wind profile drawing area.  If	*
 *	    it is, determine which component (direction/speed)	*
 *	    is active and mark that point.				*/

	    if (Winds_edit_type == EDIT_WNDDIR) {

		if ((event->xbutton.x >= left_pixel)      &&
	            (event->xbutton.x <= right_pixel)     &&
	            (event->xbutton.y >= (top_scanl-5))   &&
	            (event->xbutton.y <= (bottom_scanl+5))) {

		    if (event->xbutton.y < top_scanl)
		        event->xbutton.y = top_scanl;
		    if (event->xbutton.y > bottom_scanl)
		        event->xbutton.y = bottom_scanl;

		    level1 = 69.9 * (bottom_scanl - event->xbutton.y) /
				 ((float)(bottom_scanl - top_scanl))+0.5;
		    value1 = 360 * (event->xbutton.x - left_pixel) /
				 ((float) (middle_pixel-left_pixel-70));

		}

	    } else if (Winds_edit_type == EDIT_WNDSPD) {

	        if ((event->xbutton.x >= left_pixel)      &&
	            (event->xbutton.x <= right_pixel)     &&
	     	    (event->xbutton.y >= (top_scanl-5))   &&
	            (event->xbutton.y <= (bottom_scanl+5))) {

		    if (event->xbutton.y < top_scanl)
		        event->xbutton.y = top_scanl;
		    if (event->xbutton.y > bottom_scanl)
		        event->xbutton.y = bottom_scanl;

		    level1 = 69.9 * (bottom_scanl - event->xbutton.y) /
				 ((float)(bottom_scanl - top_scanl))+0.5;
		    value1 = MAX_ENV_WIND_SPEED * (event->xbutton.x - middle_pixel-70) /
				 ((float) (right_pixel-middle_pixel-70));

		}

	    }

	} else if ((!strcmp (args[0],"up1")) ||
		   (!strcmp (args[0],"up3"))) {

	    HCI_LE_log("Button released");

	    if ((Winds_edit_type == EDIT_WNDDIR) ||
		(Winds_edit_type == EDIT_WNDSPD)) {

	        XDefineCursor (Xdisplay,
			       Winds_window,
			       Move_cursor);

	    } else if ((Winds_edit_type == EDIT_HAIL_0_HEIGHT) ||
		       (Winds_edit_type == EDIT_HAIL_MINUS_20_HEIGHT)) {

	        XUndefineCursor (Xdisplay,
			         Winds_window);

	    }

	    Winds_edit_type = EDIT_NONE;
	    button_down = 0;

	}
	
}

/************************************************************************
 *	Description: This function is the callback invoked when the	*
 *		     Close button is selected in the Environmental	*
 *		     Data Editor window.				*
 *									*
 *	Input:  w - ID of button					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char buf[HCI_BUF_128];

  HCI_LE_log( "Close button pushed" );

  /* If any unsaved edits are detected, prompt the user first about
     saving their edits. */

  if( (Change_winds_flag == HCI_CHANGED_FLAG) ||
      (Change_hail_flag == HCI_CHANGED_FLAG) ||
      (Change_storm_flag == HCI_CHANGED_FLAG) )
  {
    sprintf( buf, "You made changes but did not save them.\nDo you want to save your changes?" );
    hci_confirm_popup( Top_widget, buf, environmental_winds_save_callback, environmental_winds_close_nosave );
  }
  else
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}

/************************************************************************
 *	Description: This function closes the Environmental Data Editor	*
 *		     task without saving unsaved edits.			*
 *									*
 *	Input:  w - ID of button (unused)				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_close_nosave (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This function is the callback for the Save button	*
 *		     n both windows.  It can be invoked directly.	*
 *									*
 *	Input:  w - ID of button (unused)				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_save_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  Save_popup_flag = HCI_YES_FLAG;
}

/************************************************************************
 *      Description: This function is the callback for the Save button  *
 *                   n both windows.  It can be invoked directly.       *
 *                                                                      *
 *      Input:  w - ID of button (unused)                               *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
environmental_winds_save_popup()
{
  char buf[HCI_BUF_128];

  /* Prompt the user for verification before saving anything. */

  sprintf( buf, "You are about to replace environmental\nadaptation data.  Do you want\nto continue?" );
  hci_confirm_popup( Top_widget, buf, accept_environmental_winds_save, NULL );
}

/************************************************************************
 *	Description: This function is the callback for the Yes button	*
 *		     in the Save verification prompt.			*
 *									*
 *	Input:  w - ID of button (unused)				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_environmental_winds_save (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  Save_flag = HCI_YES_FLAG;
  Need_to_verify_VAD_flag = HCI_YES_FLAG;
  Need_to_verify_Model_flag = HCI_YES_FLAG;
  XtSetSensitive( Close1_button, False );
  if( data_widget_open() ) { XtSetSensitive( Close2_button, False ); }
}

/************************************************************************
 *      Description: This function is the callback for the Yes button   *
 *                   in the VAD update verification prompt.             *
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
disable_vad_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  Disable_vad_flag = HCI_YES_FLAG;
  Need_to_verify_VAD_flag = HCI_NO_FLAG;
  Verifying_VAD_flag = HCI_NO_FLAG;
}

/************************************************************************
 *      Description: This function is the callback for the No button    *
 *                   in the VAD update verification prompt.    		*
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
no_disable_vad_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  Disable_vad_flag = HCI_NO_FLAG;
  Need_to_verify_VAD_flag = HCI_NO_FLAG;
  Verifying_VAD_flag = HCI_NO_FLAG;
}

/************************************************************************
 *      Description: This function is the callback for the Yes button   *
 *                   in the Model update verification prompt.           *
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
disable_model_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  Disable_model_flag = HCI_YES_FLAG;
  Need_to_verify_Model_flag = HCI_NO_FLAG;
  Verifying_Model_flag = HCI_NO_FLAG;
}

/************************************************************************
 *      Description: This function is the callback for the No button    *
 *                   in the Model update verification prompt.           *
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
no_disable_model_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  Disable_model_flag = HCI_NO_FLAG;
  Need_to_verify_Model_flag = HCI_NO_FLAG;
  Verifying_Model_flag = HCI_NO_FLAG;
}

/************************************************************************
 *      Description: This function saves the environmental wind data.   *
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void environmental_winds_save()
{
	int	status, retval, i;
	float	u, v;
	A3cd97	*enw;
	char	buf [256];
	char	attr [256];
	int	num_valid_values = 0;
	
/*	If low bandwidth, display a progress meter.			*/

	HCI_PM("Saving Environmental winds data");

/*	If the hail data edited, then save it.				*/

	if (Change_hail_flag == HCI_CHANGED_FLAG) {

	    time_t	tm;

	    tm = time (NULL);

	    unix_time (&tm,
		   &Hail_date[0],
		   &Hail_date[1],
		   &Hail_date[2],
		   &Hail_time[0],
		   &Hail_time[1],
		   &Hail_time[2]);

/* 	    Strip off the century. 				*/
	    Hail_date[0] = Hail_date[0]%100;

	    sprintf (attr,"Last Update: %2.2d/%2.2d/%2.2d - %2.2d:%2.2d:%2.2d",
			Hail_date[1], Hail_date[2], Hail_date[0],
			Hail_time[0], Hail_time[1], Hail_time[2]);

/*	    Get the Hail algorithm data					*/
	    retval = Set_hail_algorithm_data( Hail_0_height, Hail_minus_20_height,
                                              Hail_date[0], Hail_date[1], Hail_date[2],
                                              Hail_time[0], Hail_time[1], Hail_time[2] );
	    if( retval >= 0 ){

	        HCI_LE_log("Hail Detection data updated");
	   	sprintf (buf,"Hail Detection data updated");

	    } else {

		HCI_LE_log("Unable to update Hail Detection data");
		sprintf (buf,"Unable to update Hail Detection data");

	    }

	    HCI_display_feedback( buf );
	    Change_hail_flag = HCI_NOT_CHANGED_FLAG;

	}

/*	If the storm motion data edited, then save it.			*/

	if (Change_storm_flag == HCI_CHANGED_FLAG) {

	    retval = Set_storm_tracking_algorithm_data( Storm_direction, Storm_speed );
	    if( retval > 0 ){

                HCI_LE_log("Unable to update Storm Cell Tracking data");
                sprintf (buf,"Unable to update Storm Cell Tracking data");


	    } else {

                HCI_LE_log("Storm Cell Tracking data updated");
                sprintf (buf,"Storm Cell Tracking data updated");

	    }

	    HCI_display_feedback( buf );
	    Change_storm_flag = HCI_NOT_CHANGED_FLAG;

	}

/*	If the environmental winds data edited, then save it.		*/

	if (Change_winds_flag == HCI_CHANGED_FLAG) {

	    HCI_LE_log("Environmental Winds data updated");
	    sprintf (buf,"Environmental Winds data updated");
	    Change_winds_flag = HCI_NOT_CHANGED_FLAG;

	    HCI_display_feedback( buf );

	    enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

/*	Convert speed/direction to u/v component and update the		*
 *	structure.							*/

	    for (i=0;i<LEN_EWTAB;i++) {

	        if ((enw->ewtab [WNDDIR][i] != MTTABLE) &&
		    (enw->ewtab [WNDSPD][i] != MTTABLE)) {

	    	    extract_wind_components (enw->ewtab [WNDDIR][i],
					 enw->ewtab [WNDSPD][i],
				     	 &u,
				    	 &v);

/*	The following code rounds the u/v components to the nearest	*
 *	integer (similar to fortran NINT function).			*/

		    if (u < 0)
		        u = u - 0.5;
		    else
		        u = u + 0.5;

		    if (v < 0)
		        v = v - 0.5;
		    else
		        v = v + 0.5;

	    	    enw->newndtab [i][ECOMP] = (short) u;
	    	    enw->newndtab [i][NCOMP] = (short) v;
		    num_valid_values++;

		} else {

		    enw->ewtab [WNDDIR][i]   = MTTABLE;
		    enw->ewtab [WNDSPD][i]   = MTTABLE;
	    	    enw->newndtab [i][ECOMP] = MTTABLE;
	    	    enw->newndtab [i][NCOMP] = MTTABLE;

	        }
	    }

	    if (Disable_vad_flag == HCI_YES_FLAG) {

		Disable_vad_flag = HCI_NO_FLAG;
		status = hci_set_vad_update_flag (HCI_NO_FLAG);
		
	    }

	    if (Disable_model_flag == HCI_YES_FLAG) {

		Disable_model_flag = HCI_NO_FLAG;
		status = hci_set_model_update_flag (HCI_NO_FLAG);
		
	    }

	    status = hci_write_environmental_wind_data ();

	    if (status > 0)
		Winds_edit_type = EDIT_NONE;
	
	}

	XtVaSetValues (Save1_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo1_button,
		XmNsensitive,	False,
		NULL);

	if ( data_widget_open() ) {

	    XtVaSetValues (Save2_button,
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Undo2_button,
		XmNsensitive,	False,
		NULL);

	}

/*	Use the resize proc to redraw the wind		*
 *	profile with the updated winds.			*/

	environmental_winds_resize ((Widget) NULL,
				    (XtPointer) NULL,
				    (XtPointer) NULL);

}

/************************************************************************
 *	Description: This function is the callback for the VAD Update	*
 *		     radio buttons.					*
 *									*
 *	Input:  w - ID of button					*
 *		client_data - (YES or NO)				*
 *		call_data - button state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_vad_update (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int	update_flag;
  char buf[HCI_BUF_64];

  XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

  /* Only do something if the button is set. */

  if( state->set )
  {
    Vad_user_cmd = (int) client_data;
    update_flag = hci_get_vad_update_flag();

    if (Vad_user_cmd == update_flag) { return; }

    if( Vad_user_cmd == HCI_YES_FLAG )
    {
      sprintf( buf, "Do you want to enable VAD Update?" );
    }
    else
    {
      sprintf( buf, "Do you want to disable VAD Update?" );
    }

    hci_confirm_popup( Top_widget, buf, accept_vad_update, cancel_vad_update );
  }
}

/************************************************************************
 *	Description: This function is the callback for the Model Update	*
 *		     radio buttons.					*
 *									*
 *	Input:  w - ID of button					*
 *		client_data - (YES or NO)				*
 *		call_data - button state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
model_update_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int update_flag;
  char buf[HCI_BUF_64];

  XmToggleButtonCallbackStruct	*state =
        (XmToggleButtonCallbackStruct *) call_data;

  /* Only do something if the button is set. */

  if( state->set )
  { 
    Model_update_user_cmd = (int) client_data;
    update_flag = hci_get_model_update_flag();

    if( Model_update_user_cmd == update_flag ){ return; }

    if( Model_update_user_cmd == MODEL_UPDATE_ALLOWED )
    {
      sprintf( buf, "Do you want to enable Model Update?" );
    }
    else
    {
      sprintf( buf, "Do you want to disable Model Update?" );
    }

    hci_confirm_popup( Top_widget, buf, accept_model_update, cancel_model_update );
  }
}

/************************************************************************
 Description: This function is the OK callback for the Model Update popup.
 ************************************************************************/

void
accept_model_update(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int status = -1;
  char buf[HCI_BUF_128];

  status = hci_set_model_update_flag( Model_update_user_cmd );

  if( status < 0 )
  {
    if( Model_update_user_cmd == MODEL_UPDATE_ALLOWED )
    {
      sprintf( buf, "Failure enabling Model Update" );
    }
    else
    {
      sprintf( buf, "Failure disabling Model Update" );
    }

    hci_error_popup( Top_widget, buf, NULL );
  }

  update_model_update_toggles();
}

/************************************************************************
 Description: This function is the CANCEL callback for the Model Update popup.
 ************************************************************************/

void
cancel_model_update(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  update_model_update_toggles();
}

/************************************************************************
 Description: This function updates the appearance of the model update
              toggle buttons.
 ************************************************************************/

void update_model_update_toggles()
{
  int update_flag = hci_get_model_update_flag();

  if( update_flag == MODEL_UPDATE_ALLOWED )
  {
    XmToggleButtonSetState (Model_update_yes, True, False );
    XmToggleButtonSetState (Model_update_no, False, False );
  }
  else
  {
    XmToggleButtonSetState (Model_update_no, True, False );
    XmToggleButtonSetState (Model_update_yes, False, False );
  }
}

/*	The following routine is used to toggle the wind speed units	*
 *	between knots and meters per second.				*/

/************************************************************************
 *	Description: This function is the callback for the Wind units	*
 *		     radio buttons.					*
 *									*
 *	Input:  w - ID of button					*
 *		client_data - (UNITS_KTS or UNITS_MPS)			*
 *		call_data - button state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_units (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    Environmental_winds_units_flag = (int) client_data;

	    if (Environmental_winds_units_flag == UNITS_KTS) {

               	HCI_LE_log("units KTS pushed");

		XmToggleButtonSetState (Units_kts,
			True, False);
		XmToggleButtonSetState (Units_mps,
			False, False);

		if( Units_conversion != (float) HCI_MPS_TO_KTS )
		{
		  Units_conversion = HCI_MPS_TO_KTS;
		  Storm_speed *= Units_conversion;
		}

/*	    If the Data entry window is open we need to change the	*
 *	    speed units label.						*/

		if ( data_widget_open() ) {

		    XmString	str;

		    str = XmStringCreateLocalized ("kft   deg  kts");

		    XtVaSetValues (Data_entry_spd_units_label,
			XmNlabelString,	str,
			NULL);

		    XmStringFree (str);

		    str = XmStringCreateLocalized( "Speed (0-99.9 kts)         ");

		    XtVaSetValues (default_storm_motion_speed_label,
			XmNlabelString,	str,
			NULL);

		    XmStringFree (str);

		}

	    } else {

                HCI_LE_log("units MPS pushed");

	        XmToggleButtonSetState (Units_mps,
			True, False);
	        XmToggleButtonSetState (Units_kts,
			False, False);

		if( Units_conversion != (float) 1.0 )
		{
		  Units_conversion = 1.0;
		  Storm_speed *= KTS_TO_MPS;
		}

/*	    If the Data entry window is open we need to change the	*
 *	    speed units label.						*/

	        if ( data_widget_open() ) {

		    XmString	str;

		    str = XmStringCreateLocalized ("kft   deg  m/s");

		    XtVaSetValues (Data_entry_spd_units_label,
			XmNlabelString,	str,
			NULL);

		    XmStringFree (str);

		    str = XmStringCreateLocalized( "Speed (0-99.9 m/s)         ");

		    XtVaSetValues (default_storm_motion_speed_label,
			XmNlabelString,	str,
			NULL);

		    XmStringFree (str);

		}
	    }

	    environmental_winds_resize (w,
		(XtPointer) NULL,
		(XtPointer) NULL);

	}
}

/*	The following routine is used to toggle the display from the	*
 *	current ewt to the model ewt.					*/

/************************************************************************
 *	Description: This function is the callback for the display	*
 *		     radio buttons.					*
 *									*
 *	Input:  w - ID of button					*
 *		client_data - (DISPLAY_CURRENT or DISPLAY_MODEL)	*
 *		call_data - button state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
change_display_callback(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

        
	if (state->set) {

	    if ((int) client_data == DISPLAY_CURRENT) {

	        Display_flag = (int) client_data;
	        hci_set_ewt_display_flag(Display_flag);
	        Display_flag = hci_get_ewt_display_flag();

               	HCI_LE_log("Display Current pushed");

		XmToggleButtonSetState (display_current,
			True, False);
		XmToggleButtonSetState (display_model,
			False, False);

                XtVaSetValues (Clear1_button,
                               XmNsensitive, True,
                               NULL);

                XtVaSetValues (Data_Entry_button,
                               XmNsensitive, True,
                               NULL);

                if ( data_widget_open() ) 
                   XtVaSetValues (Clear2_button,
                                  XmNsensitive, True,
                                  NULL);

	    } else {

                HCI_LE_log("Display Model pushed");

/*      If any unsaved edits are detected, prompt the user first about  *
 *      saving their edits.                                             */

                if ((Change_winds_flag == HCI_CHANGED_FLAG) ||
                    (Change_hail_flag == HCI_CHANGED_FLAG) ||
                    (Change_storm_flag == HCI_CHANGED_FLAG)) {

                   sprintf( buf, "You made changes but did not save them.\nIn order to change displays, you must\nfirst save your changes" );

	           hci_warning_popup( Top_widget, buf, NULL );

		   XmToggleButtonSetState (display_current,
		 	 True, False);
		   XmToggleButtonSetState (display_model,
			False, False);

                   return;

                }

	        Display_flag = (int) client_data;
	        hci_set_ewt_display_flag(Display_flag);
	        Display_flag = hci_get_ewt_display_flag();

	        XmToggleButtonSetState (display_model,
			True, False);
	        XmToggleButtonSetState (display_current,
 			False, False);

                XtVaSetValues (Clear1_button,
                               XmNsensitive, False,
                               NULL);

                XtVaSetValues (Data_Entry_button,
                               XmNsensitive, False,
                               NULL);

                if ( data_widget_open() ) 
                   XtVaSetValues (Clear2_button,
                                  XmNsensitive, False,
                                  NULL);
                
	    }

	    Get_hail_algorithm_data();
	    environmental_winds_resize (w,
		(XtPointer) NULL,
		(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is used to compute the u and v wind	*
 *		     components for a given direction and speed.	*
 *									*
 *	Input:  direction - Wind direction (0-360 degrees)		*
 *		speed - units doesn;t matter				*
 *	Output: u - U (east-west) component				*
 *		v - V (north-south) component				*
 *	Return: NONE							*
 ************************************************************************/

void
extract_wind_components (
float	direction,
float	speed,
float	*u,
float	*v
)
{
	*u = -speed * sin ((double) direction*HCI_DEG_TO_RAD);
	*v = -speed * cos ((double) direction*HCI_DEG_TO_RAD);
}

/************************************************************************
 *	Description: This function displays the Environmental Data	*
 *		     Entry window.  It is invoked then the "Data Entry"	*
 *		     button is selected in the main window.		*
 *									*
 *	Input:  w - ID of "Data Entry" button				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_data_entry (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	rowcol;
	Widget	close_rowcol;
	Widget	form1;
	Widget	winds_frame;
	Widget	hail_frame;
	Widget	storm_frame;
	Widget	label;
	Widget	lvl_label;
	Widget	text;
	Widget	scroll;
	Widget	scroll_form;
	Widget	toggle;
	int	i;
	XmString	str = NULL;
	XmString	default_storm_speed_value = NULL;
	A3cd97	*enw;
	char	buf [128];

/*	Check to see if the window already open.  If it is just		*
 *	return.  If it isn't, create it.				*/

	if ( data_widget_open() )
	{
	  HCI_Shell_popup( Data_widget );
	  return;
	}

	HCI_Shell_init( &Data_widget, "Environmental Data Entry" );
	
/*	Get a pointer to the local environmental winds data buffer	*/

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

/*	Use a form to manage the contents of the window.  Use a global	*
 *	variable since we need to check it ouside this function.	*/

	Data_form = XtVaCreateWidget ("winds_form",
		xmFormWidgetClass,	Data_widget,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Organize control buttons in a rowcolumn at the top of the	*
 *	window.								*/

	close_rowcol = XtVaCreateWidget ("close_rowcol",
		xmRowColumnWidgetClass,	Data_form,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_COLUMN,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	Close2_button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	close_rowcol,
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Close2_button,
		XmNactivateCallback, data_entry_close_callback, NULL);

	Save2_button = XtVaCreateManagedWidget ("Save",
		xmPushButtonWidgetClass,close_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);
	XtAddCallback (Save2_button,
		XmNactivateCallback, environmental_winds_save_callback, NULL);

	Undo2_button = XtVaCreateManagedWidget ("Undo",
		xmPushButtonWidgetClass,close_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		False,
		NULL);
	XtAddCallback (Undo2_button,
		XmNactivateCallback, environmental_winds_undo, NULL);
	
	Clear2_button = XtVaCreateManagedWidget ("Clear",
		xmPushButtonWidgetClass,close_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Clear2_button,
		XmNactivateCallback, hci_environmental_winds_clear, NULL);

/*	Set the sensitivity of the Save and Undo buttons based on the	*
 *	current change state.						*/

	if ((Change_winds_flag == HCI_CHANGED_FLAG) ||
	    (Change_hail_flag  == HCI_CHANGED_FLAG) ||
	    (Change_storm_flag == HCI_CHANGED_FLAG)) {

	    XtVaSetValues (Save2_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Undo2_button,
		XmNsensitive,	True,
		NULL);

	}

	XtManageChild (close_rowcol);

/*	Display the environmental winds data in the left half of the	*
 *	window.								*/

	winds_frame = XtVaCreateManagedWidget ("winds_frame",
		xmFrameWidgetClass,	Data_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		close_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Environmental Winds Data",
		xmLabelWidgetClass,	winds_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

	form1 = XtVaCreateWidget ("winds_form",
		xmFormWidgetClass,	winds_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	rowcol = XtVaCreateWidget ("data_entry_rowcol",
		xmRowColumnWidgetClass,	form1,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_COLUMN,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Coded Msg (PPBB):",
		xmLabelWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	text = XtVaCreateManagedWidget ("",
	 	xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		15,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	XtAddCallback (text,
		XmNmodifyVerifyCallback,verify_sig_wind_data_callback, NULL);
	XtAddCallback (text,
		XmNactivateCallback,decode_sig_wind_data_callback, NULL);

	XtManageChild (rowcol);

	toggle = XtVaCreateManagedWidget ("Interpolate between levels   ",
		xmToggleButtonWidgetClass,	form1,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNset,			Interpolate_flag,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtAddCallback (toggle,
		XmNvalueChangedCallback, interpolate_toggle_callback, NULL);

	label = XtVaCreateManagedWidget ("SpaceSpace",
		xmLabelWidgetClass,	form1,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		toggle,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	 lvl_label = XtVaCreateManagedWidget ("Lvl   Dir  Spd",
		xmLabelWidgetClass,	form1,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		toggle,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNmarginHeight,	1,
		NULL);

	 Data_entry_spd_units_label = XtVaCreateManagedWidget ("kft   deg  kts",
		xmLabelWidgetClass,	form1,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		lvl_label,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		XmNmarginHeight,	1,
		NULL);

	switch (Environmental_winds_units_flag) {

	    XmString	str;

	    case UNITS_KTS :

		default_storm_speed_value = XmStringCreateLocalized ("Speed (0-99.9 kts)         ");

		str = XmStringCreateLocalized ("kft   deg  kts");

		XtVaSetValues (Data_entry_spd_units_label,
			XmNlabelString,	str,
			NULL);

		XmStringFree (str);

		break;

	    case UNITS_MPS :

		default_storm_speed_value = XmStringCreateLocalized ("Speed (0-99.9 m/s)         ");

		str = XmStringCreateLocalized ("kft   deg  m/s");

		XtVaSetValues (Data_entry_spd_units_label,
			XmNlabelString,	str,
			NULL);

		XmStringFree (str);

		break;

	}

/*	Use a scroll window for the winds table since it has 70		*
 *	entries.							*/

	scroll = XtVaCreateManagedWidget ("winds_scroll",
		xmScrolledWindowWidgetClass,	form1,
		XmNwidth,			160,
		XmNheight,			400,
		XmNscrollingPolicy,		XmAUTOMATIC,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			Data_entry_spd_units_label,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			label,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

/*	Get the ID of the clip window in the scrolled widget so we can	*
 *	set the color the same as everything else.			*/

	{
		Widget	hsb;
		Widget	vsb;
		Widget	clip;

		XtVaGetValues (scroll,
			XmNclipWindow,	&clip,
			XmNhorizontalScrollBar,	&hsb,
			XmNverticalScrollBar,	&vsb,
			NULL);

		XtUnmanageChild (hsb);

		XtVaSetValues (clip,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);
		XtVaSetValues (vsb,
			XmNtraversalOn,	False,
			NULL);

	}

	scroll_form = XtVaCreateWidget ("winds_form",
		xmFormWidgetClass,	scroll,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNverticalSpacing,	1,
		NULL);

	for (i=0;i<LEN_EWTAB;i++) {

	    if (i == 0) {

		rowcol = XtVaCreateWidget ("data_entry_rowcol",
			xmRowColumnWidgetClass,	scroll_form,
			XmNorientation,		XmHORIZONTAL,
			XmNpacking,		XmPACK_TIGHT,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNspacing,		0,
			XmNmarginHeight,	1,
			XmNmarginWidth,		1,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_FORM,
			NULL);

	    } else {

		rowcol = XtVaCreateWidget ("data_entry_rowcol",
			xmRowColumnWidgetClass,	scroll_form,
			XmNorientation,		XmHORIZONTAL,
			XmNpacking,		XmPACK_TIGHT,
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNspacing,		0,
			XmNmarginHeight,	1,
			XmNmarginWidth,		1,
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		rowcol,
			XmNleftAttachment,	XmATTACH_FORM,
			NULL);

	    }

	    sprintf (buf,"%4.1f ",i+Rda_elevation);
	    str = XmStringCreateLocalized (buf);

	    XtVaCreateManagedWidget ("level",
		xmLabelWidgetClass,	rowcol,
		XmNlabelString,		str,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	0,
		NULL);

	    XmStringFree (str);

	    Direction_text [i] = XtVaCreateManagedWidget ("Direction (deg):",
	 	xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		3,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		XmNshadowThickness,	1,
		NULL);

	    if (enw->ewtab [WNDDIR][i] == MTTABLE) {

		strcpy (buf,"");

	    } else {

		sprintf (buf,"%3d", (int) (enw->ewtab [WNDDIR][i]+0.5));

	    }

	    XmTextSetString (Direction_text [i], buf);

	    XtAddCallback (Direction_text [i],
		XmNmodifyVerifyCallback,hci_verify_unsigned_integer_callback,
		(XtPointer) 3);
	    XtAddCallback (Direction_text [i],
		XmNfocusCallback,hci_gain_focus_callback,
		(XtPointer) i);
	    XtAddCallback (Direction_text [i],
		XmNlosingFocusCallback,direction_entry_callback,
		(XtPointer) i);
	    XtAddCallback (Direction_text [i],
		XmNactivateCallback,direction_entry_callback,
		(XtPointer) i);

	    Speed_text [i] = XtVaCreateManagedWidget ("Speed (kts):",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		5,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		XmNshadowThickness,	1,
		NULL);

	    if (enw->ewtab [WNDSPD][i] == MTTABLE) {

		strcpy (buf,"");

	    } else {

		sprintf (buf,"%5.1f", (enw->ewtab [WNDSPD][i]*Units_conversion));

	    }

	    XmTextSetString (Speed_text [i], buf);

	    XtAddCallback (Speed_text [i],
		XmNmodifyVerifyCallback,hci_verify_unsigned_float_callback,
		(XtPointer) 5);
	    XtAddCallback (Speed_text [i],
		XmNfocusCallback,hci_gain_focus_callback,
		(XtPointer) i);
	    XtAddCallback (Speed_text [i],
		XmNlosingFocusCallback,speed_entry_callback,
		(XtPointer) i);
	    XtAddCallback (Speed_text [i],
		XmNactivateCallback,speed_entry_callback,
		(XtPointer) i);

	    XtManageChild (rowcol);

	}

	XtManageChild (scroll_form);
	XtManageChild (form1);

/*	Group the Hail temperature height data to the right of the	*
 *	winds data group.						*/

	hail_frame = XtVaCreateManagedWidget ("hail_frame",
		xmFrameWidgetClass,	Data_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		close_rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		winds_frame,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Hail Temperature Heights",
		xmLabelWidgetClass,	hail_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

	form1 = XtVaCreateWidget ("hail_form",
		xmFormWidgetClass,	hail_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

        sprintf (buf,"Last Update: %2.2d/%2.2d/%2.2d - %2.2d:%2.2d:%2.2d",
                        Hail_date[1], Hail_date[2], Hail_date[0],
                        Hail_time[0], Hail_time[1], Hail_time[2]);

	str = XmStringCreateLocalized (buf);

	Hail_update_label = XtVaCreateManagedWidget ("last_hail_update",
		xmLabelWidgetClass,	form1,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XmStringFree (str);

	rowcol = XtVaCreateWidget ("hail_entry_rowcol",
		xmRowColumnWidgetClass,	form1,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Hail_update_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Height -20 C (0-70 kft MSL)",
		xmLabelWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Hail_minus_20_height_widget = XtVaCreateManagedWidget ("-20_deg_height",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		4,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf (buf,"%4.1f",Hail_minus_20_height);
	XmTextSetString (Hail_minus_20_height_widget, buf);

	XtAddCallback (Hail_minus_20_height_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
		(XtPointer) 4);
	XtAddCallback (Hail_minus_20_height_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) EDIT_HAIL_MINUS_20_HEIGHT);
	XtAddCallback (Hail_minus_20_height_widget,
		XmNlosingFocusCallback, change_environmental_data_callback,
		(XtPointer) EDIT_HAIL_MINUS_20_HEIGHT);
	XtAddCallback (Hail_minus_20_height_widget,
		XmNactivateCallback, change_environmental_data_callback,
		(XtPointer) EDIT_HAIL_MINUS_20_HEIGHT);

	XtManageChild (rowcol),

	rowcol = XtVaCreateWidget ("hail_entry_rowcol",
		xmRowColumnWidgetClass,	form1,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Height 0 C (0-70 kft MSL)  ",
		xmLabelWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Hail_0_height_widget = XtVaCreateManagedWidget ("0_deg_height",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		4,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf (buf,"%4.1f",Hail_0_height);
	XmTextSetString (Hail_0_height_widget, buf);

	XtAddCallback (Hail_0_height_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
		(XtPointer) 4);
	XtAddCallback (Hail_0_height_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) EDIT_HAIL_0_HEIGHT);
	XtAddCallback (Hail_0_height_widget,
		XmNlosingFocusCallback, change_environmental_data_callback,
		(XtPointer) EDIT_HAIL_0_HEIGHT);
	XtAddCallback (Hail_0_height_widget,
		XmNactivateCallback, change_environmental_data_callback,
		(XtPointer) EDIT_HAIL_0_HEIGHT);

	XtManageChild (rowcol);
	XtManageChild (form1);

	label = XtVaCreateManagedWidget ("Space",
		xmLabelWidgetClass,	Data_form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		hail_frame,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		winds_frame,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Group the default storm motion data to the right of the	winds	*
 *	data group and below the hail data group.			*/

	storm_frame = XtVaCreateManagedWidget ("storm_frame",
		xmFrameWidgetClass,	Data_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		winds_frame,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Default Storm Motion",
		xmLabelWidgetClass,	storm_frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		NULL);

	form1 = XtVaCreateWidget ("storm_form",
		xmFormWidgetClass,	storm_frame,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	rowcol = XtVaCreateWidget ("storm_entry_rowcol",
		xmRowColumnWidgetClass,	form1,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Direction (0-360 deg)      ",
		xmLabelWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Storm_direction_widget = XtVaCreateManagedWidget ("direction",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		3,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf (buf,"%3d",Storm_direction);
	XmTextSetString (Storm_direction_widget, buf);

	XtAddCallback (Storm_direction_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
		(XtPointer) 3);
	XtAddCallback (Storm_direction_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) EDIT_STORM_DIRECTION);
	XtAddCallback (Storm_direction_widget,
		XmNlosingFocusCallback, change_environmental_data_callback,
		(XtPointer) EDIT_STORM_DIRECTION);
	XtAddCallback (Storm_direction_widget,
		XmNactivateCallback, change_environmental_data_callback,
		(XtPointer) EDIT_STORM_DIRECTION);

	XtManageChild (rowcol);

	rowcol = XtVaCreateWidget ("storm_entry_rowcol",
		xmRowColumnWidgetClass,	form1,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNpacking,		XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	default_storm_motion_speed_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNlabelString,		default_storm_speed_value,
		NULL);

	XmStringFree( default_storm_speed_value );

	Storm_speed_widget = XtVaCreateManagedWidget ("speed",
		xmTextFieldWidgetClass,	rowcol,
		XmNbackground,		hci_get_read_color (EDIT_BACKGROUND),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		4,
		XmNhighlightOnEnter,	True,
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);

	sprintf (buf,"%4.1f",Storm_speed);
	XmTextSetString (Storm_speed_widget, buf);

	XtAddCallback (Storm_speed_widget,
		XmNmodifyVerifyCallback, hci_verify_unsigned_float_callback,
		(XtPointer) 4);
	XtAddCallback (Storm_speed_widget,
		XmNfocusCallback, hci_gain_focus_callback,
		(XtPointer) EDIT_STORM_SPEED);
	XtAddCallback (Storm_speed_widget,
		XmNlosingFocusCallback, change_environmental_data_callback,
		(XtPointer) EDIT_STORM_SPEED);
	XtAddCallback (Storm_speed_widget,
		XmNactivateCallback, change_environmental_data_callback,
		(XtPointer) EDIT_STORM_SPEED);

	XtManageChild (rowcol);
	XtManageChild (form1);

	XtManageChild (Data_form);

	HCI_Shell_start( Data_widget, RESIZE_HCI );
}

/******The following function is no longer used*****/

void
verify_sig_level_wind_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;
	int	i;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
	    return;

	if (cbs->text->length == 0)	/* Handle Backspace */
	    return;

/*	Filter out everything other than digits and punctuation		*/

	for (len=0;len<cbs->text->length;len++) {

	    if ((!isdigit ((int) cbs->text->ptr [len])) &&
		(!ispunct ((int) cbs->text->ptr [len]))) {

		for (i=len; (i+1)<cbs->text->length;i++) {

		    cbs->text->ptr [i] = cbs->text->ptr [i+1];

		}

		cbs->text->length--;
		len--;

	    }

	}

/*	If something other than a digit or punctuation was detected	*
 *	don't keep it.							*/

	if (cbs->text->length == 0) { 

	    cbs->doit = False;

	}

}

/************************************************************************
 *	Description: This function is the callback for data entered in	*
 *		     any of the wind direction edit boxes.  It is	*
 *		     invoked after the user has entered data.		*
 *									*
 *	Input:  w - text widget ID					*
 *		client_data - wind level index (0-69)			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
direction_entry_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	direction;
	char	*text;
	int	level;
	static	int	old_level = -1;
	A3cd97	*enw;
	XmToggleButtonCallbackStruct	toggle_data;

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

	level = (int) client_data;

	text = XmTextGetString (w);

/*	If the edit box is not blank, decode the direction.		*/

	if (strlen (text) && (strcmp (text," ") > 0)) {

	        sscanf (text, "%d", &direction);

/*	    If the value is out of range inform the user.		*/

	    if ((direction < 0) ||
	        (direction > 360)) {

		sprintf( Buf, "You entered an invalid direction of %d.\nThe valid range is 0 to 360 deg\n", direction );
	        hci_warning_popup( Data_widget, Buf, NULL );

	        if ((enw->ewtab [WNDDIR][(int) client_data] < 0.0) ||
		    (enw->ewtab [WNDDIR][(int) client_data] > 360.0)) {

		    strcpy (Buf,"");

	        } else {

		    direction = (int) enw->ewtab [WNDDIR][(int) client_data];
		    sprintf (Buf,"%3d ",(int) direction);

	        }

	        XmTextSetString (w,Buf);

	        return;

	    }

	} else {

	    direction = MTTABLE;

	}

/*	Set the winds update flag so will know to prompt the user	*
 *	to save changes in case he/she closes the task.			*/

	if (Change_winds_flag == HCI_NOT_CHANGED_FLAG) {

	    if( Display_flag == DISPLAY_CURRENT ){

   	        Change_winds_flag = HCI_CHANGED_FLAG;
	        XtVaSetValues (Save1_button,
		               XmNsensitive,	True,
		               NULL);

	    }

	    XtVaSetValues (Undo1_button,
		XmNsensitive,	True,
		NULL);

	    if ( data_widget_open() ) {

		if( Display_flag == DISPLAY_CURRENT )
		    XtVaSetValues (Save2_button,
		 	           XmNsensitive,	True,
			           NULL);
		XtVaSetValues (Undo2_button,
			XmNsensitive,	True,
			NULL);

	    }

			
	}

	enw->ewtab [WNDDIR][level] = (float) direction;

/*	If the interpolate flag is set then we need to interpolate	*
 *	between the current and previous level if this is not the	*
 *	first level entered.						*/

	if (Interpolate_flag) {

	    toggle_data.set = 1;

	    interpolate_toggle_callback ((Widget) NULL, (XtPointer) NULL,
		(XtPointer) &toggle_data);

	} else {

	    old_level = -1;

	}

/*	Use the resize proc to redraw the wind		*
 *	profile with the updated winds.			*/

	environmental_winds_resize ((Widget) NULL,
				    (XtPointer) NULL,
				    (XtPointer) NULL);

	XtFree (text);

}

/************************************************************************
 *	Description: This function is the callback for data entered in	*
 *		     any of the wind speed edit boxes.  It is invoked	*
 *		     after the user has entered data.			*
 *									*
 *	Input:  w - text widget ID					*
 *		client_data - wind level index (0-69)			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
speed_entry_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	float	speed;
	char	*text;
	int	level;
	static	int	old_level = -1;
	A3cd97	*enw;
	XmToggleButtonCallbackStruct	toggle_data;

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

	level = (int) client_data;

	text = XmTextGetString (w);

/*	If the edit box is not blank, decode the speed.			*/

	if (strlen (text) && (strcmp (text," ") > 0)) {

	    sscanf (text, "%f", &speed);

/*	    Convert the wind speed from knots to mps (if needed).	*/

	    speed = speed/Units_conversion;

/*	    If the value is out of range inform the user.		*/

	    if ((speed < 0) ||
	        (speed > MAX_ENV_WIND_SPEED)) {

	        if (Environmental_winds_units_flag == UNITS_KTS) {

	            sscanf (text, "%f ", &speed);

		    sprintf (Buf,
		    "You entered an invalid value of %4.1f.\nThe valid range is 0 to 249 kts\n",
		    speed);

	        } else {

		    sprintf (Buf,
		    "You entered an invalid value of %4.1f.\nThe valid range is 0 to 128 m/s\n",
		    speed);

	        }

	        hci_warning_popup( Data_widget, Buf, NULL );

	        if ((enw->ewtab [WNDSPD][(int) client_data] < 0.0) ||
		    (enw->ewtab [WNDSPD][(int) client_data] > MAX_ENV_WIND_SPEED)) {

		    strcpy (Buf,"");

	        } else {

		    speed = enw->ewtab [WNDSPD][(int) client_data]*Units_conversion;
		    sprintf (Buf,"%4.1f ",speed);

	        }

	        XmTextSetString (w,Buf);

	        return;

	    }

	} else {

	    speed = MTTABLE;

	}

/*	Set the winds update flag so will know to prompt the user	*
 *	to save changes in case he/she closes the task.			*/

	if (Change_winds_flag == HCI_NOT_CHANGED_FLAG) {

	    if( Display_flag == DISPLAY_CURRENT ){

	        Change_winds_flag = HCI_CHANGED_FLAG;
	        XtVaSetValues (Save1_button,
		               XmNsensitive,	True,
		               NULL);

	    }

	    XtVaSetValues (Undo1_button,
		XmNsensitive,	True,
		NULL);

	    if ( data_widget_open() ) {

	        if( Display_flag == DISPLAY_CURRENT )
		    XtVaSetValues (Save2_button,
			           XmNsensitive,	True,
			           NULL);
		XtVaSetValues (Undo2_button,
			XmNsensitive,	True,
			NULL);

	    }

			
	}

	enw->ewtab [WNDSPD][(int) client_data] = speed;

/*	If the interpolate flag is set then we need to interpolate	*
 *	between the current and previous level if this is not the	*
 *	first level entered.						*/

	if (Interpolate_flag) {

	    toggle_data.set = 1;

	    interpolate_toggle_callback ((Widget) NULL, (XtPointer) NULL,
		(XtPointer) &toggle_data);

	} else {

	    old_level = -1;

	}

/*	Use the resize proc to redraw the wind		*
 *	profile with the updated winds.			*/

	environmental_winds_resize ((Widget) NULL,
				    (XtPointer) NULL,
				    (XtPointer) NULL);

	XtFree (text);

}

/************************************************************************
 *	Description: This function is the callback for the Interpolate	*
 *		     Between Levels toggle button in the Environmental	*
 *		     Data Entry window.					*
 *									*
 *	Input:  w - ID of toggle button					*
 *		client_data - unused					*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
interpolate_toggle_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	i,j,k;
	double	ucomp1, ucomp2;
	double	vcomp1, vcomp2;
	double	uslope, vslope;
	double	unew, vnew;
	int	indx;
	A3cd97	*enw;

	XmToggleButtonCallbackStruct *state =
		(XmToggleButtonCallbackStruct *) call_data;

	Interpolate_flag = state->set;

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

/*	If the interpolate flag is set, interpolate between all known	*
 *	levels.								*/

	if (Interpolate_flag) {

	    for (i=0;i<LEN_EWTAB-1;i++) {

		if ((enw->ewtab [WNDDIR][i] != MTTABLE) &&
		    (enw->ewtab [WNDSPD][i] != MTTABLE)) {

		    if ((enw->ewtab [WNDDIR][i+1] == MTTABLE) ||
			(enw->ewtab [WNDSPD][i+1] == MTTABLE)) {

			for (j=i+2;j<LEN_EWTAB;j++) {

			    if ((enw->ewtab [WNDDIR][j] != MTTABLE) &&
				(enw->ewtab [WNDSPD][j] != MTTABLE)) {

				ucomp1 = (-1) * enw->ewtab [WNDSPD][i] *
				sin ((double) (enw->ewtab [WNDDIR][i]*HCI_DEG_TO_RAD));
				ucomp2 = (-1) * enw->ewtab [WNDSPD][j] *
				sin ((double) (enw->ewtab [WNDDIR][j]*HCI_DEG_TO_RAD));
				vcomp1 = (-1) * enw->ewtab [WNDSPD][i] *
				cos ((double) (enw->ewtab [WNDDIR][i]*HCI_DEG_TO_RAD));
				vcomp2 = (-1) * enw->ewtab [WNDSPD][j] *
				cos ((double) (enw->ewtab [WNDDIR][j]*HCI_DEG_TO_RAD));

				uslope = (ucomp2 - ucomp1) / (j-i);
				vslope = (vcomp2 - vcomp1) / (j-i);

				indx = 1;

				for (k=i+1;k<j;k++) {

				    unew = ucomp1 + indx*uslope;
				    vnew = vcomp1 + indx*vslope;

				    indx++;

				    if (unew < 0) {

				        enw->ewtab [WNDDIR][k] = (PIOVER2 -
					atan (vnew/unew))/HCI_DEG_TO_RAD;

				    } else if (unew > 0) {

				        enw->ewtab [WNDDIR][k] = (PI3OVER2 -
					atan (vnew/unew))/HCI_DEG_TO_RAD;

				    } else {

				        if (vnew < 0) {

					    enw->ewtab [WNDDIR][k] = 0.0;

				        } else {

					    enw->ewtab [WNDDIR][k] = 180.0;

				        }
				    }

				    enw->ewtab [WNDSPD][k] = sqrt (vnew*vnew + unew*unew);

			        }

				break;

			    }
			}
		    }
		}
	    }
	}

	environmental_winds_resize ((Widget) NULL,
				    (XtPointer) NULL,
				    (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is the callback used to decode the	*
 *		     significant level wind data after all data have	*
 *		     been entered.					*
 *									*
 *	Input:  w - ID of coded msg edit box				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
decode_sig_wind_data_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*text;
	int	i, j;
	int	ones;
	int	tens;
	int	levels;
	int	wind;
	int	direction;
	int	speed;
	int	spaces;
	int	indx;
	char	buf [64];
	double	ucomp1, ucomp2, vcomp1, vcomp2;
	double	uslope, vslope;
	double	unew, vnew;
	int	Wind_height    [LEN_EWTAB]; /* local wind height table */
	float	Wind_direction [LEN_EWTAB]; /* local wind direction table */
	float	Wind_speed     [LEN_EWTAB]; /* local wind speed table */
	A3cd97	*enw;

	text = XmTextGetString (w);

	enw  = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

/*	Significant wind data are ordered into groups of 5 characters.	*
 *	The first group always begins with a "9" and indicates the	*
 *	height group.  Up to 3 heights can be defined at a time.  Next	*
 *	are up to 3 wind definitions for the wind group.  Next begins	*
 *	a new height group definition.  This is repeated until the "="	*
 *	character is detected.						*/

	i = 0;
	Level = 0;

	while (strncmp (text+i,"=",1)) {

	    while (!strncmp (text+i," ",1)) { /* Ignore spaces */

		i++;

	    }

	    if (strncmp (text+i,"9",1)) {

		HCI_LE_error("ERROR decoding sig wind data %s", text+i);
		return;

	    }

	    sscanf (text+i+1,"%1d",&tens);

	    levels = 0;
	    spaces = 0;

	    for (j=0;j<3;j++) {

		if (strncmp (text+i+2+j,"/",1)) {

		    sscanf (text+i+2+j,"%1d",&ones);

		    Wind_height [Level] = (int) ((tens*10 + ones) -
						Rda_elevation + 0.5);

		    if (Wind_height [Level] < 0) {

			Wind_height [Level] = 0;

			HCI_LE_status("WARNING-decoded wind level below surface");
			sprintf (buf,"WARNING: Decoded wind level below surface");

	    		HCI_display_feedback( buf );
		    }

		    while (!strncmp (text+i+5+j*5+spaces," ",1)) { /* Ignore spaces */

			spaces++;

		    }

		    sscanf (text+i+5+j*5+spaces,"%5d",&wind);

		    direction = wind/100;
		    speed     = wind - direction*100;

/*		    If the units digit is 1 or 6, then add 100 kts	*
 *		    to the speed and subtract 1 degree from direction	*/

		    Wind_direction [Level] = direction - direction%5;
		    Wind_speed     [Level] = 100*(direction%5) + speed;
		    Wind_speed     [Level] = Wind_speed [Level]/HCI_MPS_TO_KTS;

		    levels++;
		    Level++;

		}
	    }

	    i = i + 5*levels + 5 + spaces;

	}

	if (Level > 0) {

	    enw->ewtab [WNDDIR][Wind_height [0]] = Wind_direction [0];
	    enw->ewtab [WNDSPD][Wind_height [0]] = Wind_speed [0];

/*	    Set the winds update flag so will know to prompt the user	*
 *	    to save changes in case he/she closes the task.		*/

	    if (Change_winds_flag == HCI_NOT_CHANGED_FLAG) {

		XtVaSetValues (Save1_button,
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Undo1_button,
			XmNsensitive,	True,
			NULL);

		if ( data_widget_open() ) {

		    XtVaSetValues (Save2_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Undo2_button,
			XmNsensitive,	True,
			NULL);

		}

		Change_winds_flag = HCI_CHANGED_FLAG;

	    }
	}

/*	Interpolate the winds between levels in the coded message.	*/

	for (i=1;i<Level;i++) {

	    ucomp1 = (-1) * Wind_speed [i-1] *
			sin ((double) (Wind_direction [i-1]*HCI_DEG_TO_RAD));
	    ucomp2 = (-1) * Wind_speed [i] *
			sin ((double) (Wind_direction [i]*HCI_DEG_TO_RAD));
	    vcomp1 = (-1) * Wind_speed [i-1] *
			cos ((double) (Wind_direction [i-1]*HCI_DEG_TO_RAD));
	    vcomp2 = (-1) * Wind_speed [i] *
			cos ((double) (Wind_direction [i]*HCI_DEG_TO_RAD));

	    uslope = (ucomp2-ucomp1)/
			(Wind_height [i]-Wind_height [i-1]);
	    vslope = (vcomp2-vcomp1)/
			(Wind_height [i]-Wind_height [i-1]);

	    indx = 0;

	    for (j=(int)Wind_height [i-1];j<(int)Wind_height [i];j++) {

		indx++;

		unew = ucomp1 + indx*uslope;
		vnew = vcomp1 + indx*vslope;

/*		Interpolate the wind direction.			*/

		if (unew < 0) {

		    enw->ewtab [WNDDIR][j+1] = (PIOVER2 -
				atan (vnew/unew))/HCI_DEG_TO_RAD;

		} else if (unew > 0) {

		    enw->ewtab [WNDDIR][j+1] = (PI3OVER2 -
				atan (vnew/unew))/HCI_DEG_TO_RAD;

		} else {

		    if (vnew < 0) {

			enw->ewtab [WNDDIR][j+1] = 0.0;

		    } else {

			enw->ewtab [WNDDIR][j+1] = 180.0;

		    }
		}

/*		Interpolate the wind speed.				*/

		enw->ewtab [WNDSPD][j+1] = sqrt (vnew*vnew + unew*unew);
		
	    }
	}

/*	Use the resize proc to redraw the wind		*
 *	profile with the updated winds.			*/

	environmental_winds_resize ((Widget) NULL,
				    (XtPointer) NULL,
				    (XtPointer) NULL);

	XtFree (text);

}

/************************************************************************
 *	Description: This function is the callback for data entered in	*
 *		     the significant wind level edit box.  It is	*
 *		     invoked while the user is entering data.		*
 *									*
 *	Input:  w - text widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
verify_sig_wind_data_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;
	int	i;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
	    return;

	if (cbs->text->length == 0)	/* Handle Backspace */
	    return;

/*	Filter out everything other than digits, "/", and "="		*/

	for (len=0;len<cbs->text->length;len++) {

	    if ((!isdigit ((int)cbs->text->ptr [len])) &&
		(cbs->text->ptr [len] != 47) &&
		(cbs->text->ptr [len] != 32) &&
		(cbs->text->ptr [len] != 61)) {

		for (i=len; (i+1)<cbs->text->length;i++) {

		    cbs->text->ptr [i] = cbs->text->ptr [i+1];

		}

		cbs->text->length--;
		len--;

	    }

	}

/*	If something other than a digit or punctuation was detected	*
 *	don't keep it.							*/

	if (cbs->text->length == 0) { 

	    cbs->doit = False;

	}

}

/************************************************************************
 *	Description: This function updates the contents of the text	*
 *		     widgets in the Environmental Data Entry window.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
update_table_text ()
{
	int	i;
	A3cd97	*enw;
	XmString	str;
	char	tbuf [256];
        char	*empty_string = "";

	if ( data_widget_open() ) {

	    enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

	    for (i=0;i<LEN_EWTAB;i++) {

	        if (enw->ewtab [WNDDIR][i] == MTTABLE) {

	 	    sprintf (tbuf,"%s",empty_string);

	        } else {

		    sprintf (tbuf,"%3d ", (int) (enw->ewtab [WNDDIR][i]+0.5));

	        }

	        XmTextSetString (Direction_text [i], tbuf);

	        if (enw->ewtab [WNDSPD][i] == MTTABLE) {

		    sprintf (tbuf,"%s",empty_string);

	        } else {

		    sprintf (tbuf,"%5.1f ", (enw->ewtab [WNDSPD][i]*Units_conversion));

	        }

	        XmTextSetString (Speed_text [i], tbuf);

	    }

	    sprintf (tbuf,"%4.1f ",Hail_0_height);	
	    XmTextSetString (Hail_0_height_widget, tbuf);

	    sprintf (tbuf,"%4.1f ",Hail_minus_20_height);	
	    XmTextSetString (Hail_minus_20_height_widget, tbuf);

            sprintf (tbuf,"Last Update: %2.2d/%2.2d/%2.2d - %2.2d:%2.2d:%2.2d",
                        Hail_date[1], Hail_date[2], Hail_date[0],
                        Hail_time[0], Hail_time[1], Hail_time[2]);

	    str = XmStringCreateLocalized (tbuf);

	    XtVaSetValues (Hail_update_label,
		XmNlabelString,	str,
		NULL);

	    XmStringFree (str);

	    sprintf (tbuf,"%3d ",(int) Storm_direction);	
	    XmTextSetString (Storm_direction_widget, tbuf);

	    sprintf (tbuf,"%4.1f ", Storm_speed);	
	    XmTextSetString (Storm_speed_widget, tbuf);

	}

}

/************************************************************************
 *	Description: This function is activated when the "Clear" button	*
 *		     is selected in either the Environmental Data Entry	*
 *		     or Environmental Data Editior windows.  The winds	*
 *		     data are cleared.					*
 *									*
 *	Input:  w - Clear button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_environmental_winds_clear (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	i;
	A3cd97	*enw;

	enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

	for (i=0;i<LEN_EWTAB;i++) {

	    enw->ewtab    [WNDSPD][i] = MTTABLE;
	    enw->ewtab    [WNDDIR][i] = MTTABLE;
	    enw->newndtab [i][ECOMP]  = MTTABLE;
	    enw->newndtab [i][NCOMP]  = MTTABLE;

	}

	Change_winds_flag = HCI_CHANGED_FLAG;

	XtVaSetValues (Save1_button,
		XmNsensitive,	True,
		NULL);
	XtVaSetValues (Undo1_button,
		XmNsensitive,	True,
		NULL);

	if ( data_widget_open() ) {

	    XtVaSetValues (Save2_button,
		XmNsensitive,	True,
		NULL);
	    XtVaSetValues (Undo2_button,
		XmNsensitive,	True,
		NULL);

	}

	environmental_winds_resize (w,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This is the event notification callback for the	*
 *		     environmental winds update event.			*
 *									*
 *	Input:  evtcd - event code					*
 *		info - event data					*
 *		msglen - size (bytes) of event data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_update (
en_t	evtcd,
void	*info,
size_t	msglen
)
{
	Enw_update_flag = HCI_YES_FLAG;
}

/************************************************************************
 *      Description: This is the LB notification callback for model  	*
 *                   updates.                                           *
 *                                                                      *
 *      Input:  evtcd - event code                                      *
 *              info - event data                                       *
 *              msglen - size (bytes) of event data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
model_update_notification (
int     fd,
LB_id_t msgid,
int     msg_info,
void   *arg
)
{
        Model_update_flag = HCI_YES_FLAG;
}


/************************************************************************
 *	Description: This is the main timer procedure for the		*
 *		     Environmental Data Editor task.  It checks for	*
 *		     changes in various update flags and forces the	*
 *		     windows to be refreshed if they need to be.	*
 *									*
 *	Input:  w - Widget associated with the timer			*
 *	 	id - timer ID						*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
timer_proc ()
{
  int status;
  A3cd97 *enw;
  int refresh_flag;
  char buf[HCI_BUF_128];

  if( Save_popup_flag == HCI_YES_FLAG )
  {
    Save_popup_flag = HCI_NO_FLAG;
    environmental_winds_save_popup();
  }
  else if( Save_flag == HCI_YES_FLAG && Need_to_verify_VAD_flag == HCI_YES_FLAG )
  {
    if( Verifying_VAD_flag == HCI_YES_FLAG ){ return; }
    if( Display_flag != DISPLAY_MODEL &&
        Change_winds_flag == HCI_YES_FLAG &&
        hci_get_vad_update_flag() == HCI_YES_FLAG )
    {
      Verifying_VAD_flag = HCI_YES_FLAG;
      sprintf( buf, "VAD Update is enabled.  Do you want to\ndisable it?" );
      hci_confirm_popup( Top_widget, buf, disable_vad_callback, no_disable_vad_callback );
    }
    else
    {
      Need_to_verify_VAD_flag = HCI_NO_FLAG;
    }
  }
  else if(Save_flag == HCI_YES_FLAG && Need_to_verify_Model_flag == HCI_YES_FLAG)
  {
    if( Verifying_Model_flag == HCI_YES_FLAG ){ return; }
    if( Change_winds_flag == HCI_YES_FLAG &&
        hci_get_model_update_flag() == HCI_YES_FLAG )
    {
      Verifying_Model_flag = HCI_YES_FLAG;
      sprintf( buf, "Model Update is enabled.  Do you want to\ndisable it?" );
      hci_confirm_popup( Top_widget, buf, disable_model_callback, no_disable_model_callback );
    }
    else
    {
      Need_to_verify_Model_flag = HCI_NO_FLAG;
    }
  }
  else if( Save_flag == HCI_YES_FLAG )
  {
    Save_flag = HCI_NO_FLAG;
    environmental_winds_save();
    XtSetSensitive( Close1_button, True );
    if( data_widget_open() ) { XtSetSensitive( Close2_button, True ); }
  }

/*	If a data update has occurred, reread the data.			*/

	refresh_flag = 0;

	if (Enw_update_flag == HCI_YES_FLAG ||
            Model_update_flag == HCI_YES_FLAG ||
            Algorithm_data_updated == HCI_YES_FLAG) {
	
	    HCI_PM("Refreshing Environmental Data");	

	    if (Enw_update_flag == HCI_YES_FLAG && (Change_winds_flag == HCI_NOT_CHANGED_FLAG)) {

	        Enw_update_flag = HCI_NO_FLAG;

		status = hci_read_environmental_wind_data ();

/*	   	Highlight the currently active toggle		*/

		enw = (A3cd97 *) hci_get_environmental_wind_data_ptr ();

		if (enw->envwndflg == HCI_YES_FLAG) {

		    XmToggleButtonSetState (Vad_update_yes,
			True, False);
		    XmToggleButtonSetState (Vad_update_no,
			False, False);

		} else {

		    XmToggleButtonSetState (Vad_update_no,
			True, False);
		    XmToggleButtonSetState (Vad_update_yes,
			False, False);

		}

                if (hci_get_model_update_flag() == HCI_YES_FLAG) {

                    XmToggleButtonSetState (Model_update_yes,
                        True, False);
                    XmToggleButtonSetState (Model_update_no,
                        False, False);

                } else {

                    XmToggleButtonSetState (Model_update_no,
                        True, False);
                    XmToggleButtonSetState (Model_update_yes,
                        False, False);

                }

		refresh_flag = 1;

	    }

            if (Model_update_flag == HCI_YES_FLAG) {

                Model_update_flag = HCI_NO_FLAG;
                status = hci_get_model_update_flag ();

/*              Highlight the currently active toggle           */

                if (status == HCI_YES_FLAG) {

                    XmToggleButtonSetState (Model_update_yes,
                        True, False);
                    XmToggleButtonSetState (Model_update_no,
                        False, False);

                } else {

                    XmToggleButtonSetState (Model_update_no,
                        True, False);
                    XmToggleButtonSetState (Model_update_yes,
                        False, False);

                }

                refresh_flag = 1;

            }


	    if (Algorithm_data_updated == HCI_YES_FLAG) {

	        Algorithm_data_updated = HCI_NO_FLAG;

/*      	Get the Hail algorithm data                                     */
                Get_hail_algorithm_data();

/*      	Get the Storm Cell Tracking algorithm data                      */
		Get_storm_tracking_algorithm_data();

		refresh_flag = 1;

	    }

	    if (refresh_flag) {

		environmental_winds_resize ((Widget) NULL,
		    (XtPointer) NULL,
		    (XtPointer) NULL);

	    }
	}
}

/************************************************************************
 *	Description: This function is activated when the "Close" button	*
 *		     is selected in the the Environmental Data Entry	*
 *		     window.						*
 *									*
 *	Input:  w - Close button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
data_entry_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  HCI_Shell_popdown( Data_widget );
}

/************************************************************************
 *	Description: This function is activated when one of the hail	*
 *		     or default storm motion fields are edited in the	*
 *		     environmental Data Entry window.			*
 *									*
 *	Input:  w - text widget ID					*
 *		client_data - parameter identifier			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
change_environmental_data_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char		*text;
	int		err;
	char		tbuf [128];
	char		ebuf [128];
	double 		value;
	int 		ivalue;
	DEAU_attr_t 	*at = NULL;
	char 		*p = NULL;

	err = 0;
	text = XmTextGetString (w);
	strcpy (tbuf,text);
	XtFree (text);

/*	If something was detected in the string then process it.	*/

	if (strlen (tbuf)) {

/*	    Check which field to change from client data		*/

	    switch ((int) client_data) {

		case EDIT_HAIL_0_HEIGHT :

		    sscanf( tbuf, "%lf", &value );
            	    sprintf( tbuf, "%s.height_0", HAIL_DEA_NAME );
	    	    if( DEAU_get_attr_by_id( tbuf, &at ) < 0 )
			at = NULL;

            	    if( DEAU_check_data_range( tbuf, DEAU_T_DOUBLE, 1, (char *) &value ) < 0 ){

		    	err = 1;
		    	sprintf (tbuf,"%4.1f",Hail_0_height);
			if( (at != NULL)  &&  (DEAU_get_min_max_values (at, &p) == 7) ){

			    sprintf( ebuf, "%4.1f is out of range, must be in the range %s <= x <= ",
                                     value, p);
			    p += strlen (p) + 1;
             		    sprintf ( ebuf + strlen (ebuf), "%s.\n", p);

			}
			else
			    sprintf (ebuf,"%4.1f is not valid for %s\n", value, tbuf );

		    } else {

		    	err = 0;
		    	Hail_0_height = value;
		    	sprintf (tbuf,"%4.1f",Hail_0_height);

		    	if (Change_hail_flag == HCI_NOT_CHANGED_FLAG) {

			    Change_hail_flag = HCI_CHANGED_FLAG;

			    XtVaSetValues (Save1_button,
					XmNsensitive,	True,
					NULL);
			    XtVaSetValues (Undo1_button,
					XmNsensitive,	True,
					NULL);

			    if ( data_widget_open() ) {
 
			         XtVaSetValues (Save2_button,
					XmNsensitive,	True,
					NULL);
			        XtVaSetValues (Undo2_button,
					XmNsensitive,	True,
					NULL);

			    }
		        }
		    }

		    break;

		case EDIT_HAIL_MINUS_20_HEIGHT :

		    sscanf( tbuf, "%lf", &value );
            	    sprintf( tbuf, "%s.height_minus_20", HAIL_DEA_NAME );
	    	    if( DEAU_get_attr_by_id( tbuf, &at ) < 0 )
			at = NULL;

            	    if( DEAU_check_data_range( tbuf, DEAU_T_DOUBLE, 1, (char *) &value ) < 0 ){

			err = 1;
			sprintf (tbuf,"%4.1f",Hail_minus_20_height);
			if( (at != NULL) && (DEAU_get_min_max_values (at, &p) == 7) ){

			    sprintf( ebuf, "%4.1f is out of range, must be in the range %s <= x <= ",
                                     value, p);
			    p += strlen (p) + 1;
             		    sprintf ( ebuf + strlen (ebuf), "%s.\n", p);

			}
			else
			    sprintf (ebuf,"%4.1f is not valid for %s\n", value, tbuf );

		    } else {

			err = 0;
			Hail_minus_20_height = value;
			sprintf (tbuf,"%4.1f",Hail_minus_20_height);

			if (Change_hail_flag == HCI_NOT_CHANGED_FLAG) {

			    Change_hail_flag = HCI_CHANGED_FLAG;

			    XtVaSetValues (Save1_button,
					XmNsensitive,	True,
					NULL);
			    XtVaSetValues (Undo1_button,
					XmNsensitive,	True,
					NULL);

			    if ( data_widget_open() ) {

			        XtVaSetValues (Save2_button,
					XmNsensitive,	True,
					NULL);
				XtVaSetValues (Undo2_button,
					XmNsensitive,	True,
					NULL);

			    }
		    	}
		    }

		    break;

		case EDIT_STORM_DIRECTION :

		    sscanf( tbuf, "%d", &ivalue );
		    sprintf( tbuf, "%s.default_dir", STORM_CELL_TRACK_DEA_NAME );
	    	    if( DEAU_get_attr_by_id( tbuf, &at ) < 0 )
		    {
			at = NULL;
		    }

            	    if( DEAU_check_data_range( tbuf, DEAU_T_INT, 1, (char *) &ivalue ) < 0 ){

			err = 1;
		        sprintf (tbuf,"%3d",Storm_direction);
			if( (at != NULL) && (DEAU_get_min_max_values (at, &p) == 7) ){

			    sprintf( ebuf, "%3d is out of range, must be in the range %s <= x <= ",
                                     ivalue, p);
			    p += strlen (p) + 1;
             		    sprintf ( ebuf + strlen (ebuf), "%s.\n", p);

			}
			else
			    sprintf (ebuf,"%3d is not valid for %s\n", ivalue, tbuf );

		    } else {

			err = 0;
			Storm_direction = ivalue;
			sprintf (tbuf,"%3d",Storm_direction);

			if (Change_storm_flag == HCI_NOT_CHANGED_FLAG) {

			    Change_storm_flag = HCI_CHANGED_FLAG;

			    XtVaSetValues (Save1_button,
					XmNsensitive,	True,
					NULL);
			    XtVaSetValues (Undo1_button,
					XmNsensitive,	True,
					NULL);

			    if ( data_widget_open() ) {

				XtVaSetValues (Save2_button,
					XmNsensitive,	True,
					NULL);
				XtVaSetValues (Undo2_button,
					XmNsensitive,	True,
					NULL);

			    }
		    	}

			default_storm_motion_display ();

		    }

		    break;

		case EDIT_STORM_SPEED :

		    sscanf( tbuf, "%lf", &value );
		    sprintf( tbuf, "%s.default_spd", STORM_CELL_TRACK_DEA_NAME );
	    	    if( DEAU_get_attr_by_id( tbuf, &at ) < 0 )
			at = NULL;

            	    if( DEAU_check_data_range( tbuf, DEAU_T_DOUBLE, 1, (char *) &value ) < 0 ){

			err = 1;
			sprintf (tbuf,"%4.1f",Storm_speed);
			if( (at != NULL) && (DEAU_get_min_max_values (at, &p) == 7) ){

			    sprintf( ebuf, "%4.1f is out of range, must be in the range %s <= x <= ",
                                     value, p);
			    p += strlen (p) + 1;
             		    sprintf ( ebuf + strlen (ebuf), "%s.\n", p);

			}
			else
			    sprintf (ebuf,"%4.1f is not valid for %s\n", value, tbuf );

		    } else {

		        err = 0;
			Storm_speed = value;
			sprintf (tbuf,"%4.1f",Storm_speed);

			if (Change_storm_flag == HCI_NOT_CHANGED_FLAG) {

			    Change_storm_flag = HCI_CHANGED_FLAG;

			    XtVaSetValues (Save1_button,
					XmNsensitive,	True,
					NULL);
			    XtVaSetValues (Undo1_button,
					XmNsensitive,	True,
					NULL);

			    if ( data_widget_open() ) {

				XtVaSetValues (Save2_button,
					XmNsensitive,	True,
					NULL);
				XtVaSetValues (Undo2_button,
					XmNsensitive,	True,
					NULL);

			    }
		    	}

			default_storm_motion_display ();

		    }

		    break;

		default :

		    return;

	    }

	} else {

	    return;

	}

	XmTextSetString (w, tbuf);

/*	If an error was detected then display an informative message	*/

	if (err) {

	    hci_warning_popup( Data_widget, ebuf, NULL );
	}

/*	Force an update by calling the resize procedure.		*/

	environmental_winds_resize (w,
			(XtPointer) NULL,
			(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function is the callback for the "Undo"	*
 *		     buttons in the Environmental Data Editor and	*
 *		     Environmental Data Entry windows.	Data are	*
 *		     reread from file.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
environmental_winds_undo (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

/*	Set the local update flags to force a reread during the next	*
 *	timer interval.							*/

	Enw_update_flag        = HCI_YES_FLAG;
	Algorithm_data_updated = HCI_YES_FLAG;

	Change_hail_flag       = HCI_NOT_CHANGED_FLAG;
	Change_storm_flag      = HCI_NOT_CHANGED_FLAG;
	Change_winds_flag      = HCI_NOT_CHANGED_FLAG;

	XtVaSetValues (Save1_button,
		XmNsensitive,	False,
		NULL);
	XtVaSetValues (Undo1_button,
		XmNsensitive,	False,
		NULL);

	if ( data_widget_open() ) {

	    XtVaSetValues (Save2_button,
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Undo2_button,
		XmNsensitive,	False,
		NULL);

	}
}

/************************************************************************
 *      Description: This function is called when the user selects the  *
 *                   "Yes" button from the VAD Update confirmation      *
 *                   popup window.                                      *
 *                                                                      *
 *      Input:  w           - ID of yes button                          *
 *              client_data - YES or NO                                 *
 *              call_data   - *unused*                                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
accept_vad_update (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  int status = -1;
  char buf[HCI_BUF_128];

  status = hci_set_vad_update_flag( Vad_user_cmd );

  if( status < 0 )
  {
    if( Vad_user_cmd == HCI_YES_FLAG )
    {
      sprintf( buf, "Failure enabling VAD Update (%d)", status );
    }
    else
    {
      sprintf( buf, "Failure disabling VAD Update (%d)", status );
    }

    hci_error_popup( Top_widget, buf, NULL );
    HCI_LE_error( buf );
  }

  update_vad_toggles();
}

/************************************************************************
 *      Description: This function is called when the user selects the  *
 *                   "No" button from the VAD Update confirmation       *
 *                   popup window.                                      *
 *                                                                      *
 *      Input:  w           - ID of yes button                          *
 *              client_data - YES or NO                                 *
 *              call_data   - *unused*                                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
cancel_vad_update (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  update_vad_toggles();
}

/************************************************************************
 Description: This function updates the appearance of the VAD
              toggle buttons.
 ************************************************************************/

void update_vad_toggles()
{
  int update_flag = hci_get_vad_update_flag();

  if( update_flag == HCI_YES_FLAG )
  {
    XmToggleButtonSetState (Vad_update_yes, True, False );
    XmToggleButtonSetState (Vad_update_no, False, False );
  }
  else
  {
    XmToggleButtonSetState (Vad_update_no, True, False );
    XmToggleButtonSetState (Vad_update_yes, False, False );
  }
}

/************************************************************************
 *      Description: This function is called when the hail algorithm    *
 *                   adaptation data or storm cell tracking adaptation  *
 *                   has been updated.                                  *
 *                                                                      *
 *      Input:  See deau or lb manpage                                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void adaptation_callback( 
int fd, 
LB_id_t msgid, 
int msg_info, 
void *arg 
)
{
  Algorithm_data_updated = HCI_YES_FLAG;
}

/************************************************************************
 *      Description: This function is called when the hail algorithm    *
 *                   adaptation data needs to be read.                  *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void Get_hail_algorithm_data(){

  Hail_temps_t hail_temps;
  static char buf[64];
  double value;
  int retval = -1;

  if( Display_flag == DISPLAY_MODEL )
  {
    retval = ORPGDA_read( HCI_HAIL_LB,
                       (char *) &hail_temps, sizeof( Hail_temps_t ),
                       LBID_MODEL_HAIL );

    if( retval > 0 )
    {
      Hail_0_height = (float) hail_temps.height_0;
      Hail_minus_20_height = (float) hail_temps.height_minus_20;
      Hail_date[ 0 ] = hail_temps.hail_date_yy;
      Hail_date[ 1 ] = hail_temps.hail_date_mm;
      Hail_date[ 2 ] = hail_temps.hail_date_dd;
      Hail_time[ 0 ] = hail_temps.hail_time_hr;
      Hail_time[ 1 ] = hail_temps.hail_time_min;
      Hail_time[ 2 ] = hail_temps.hail_time_sec;
    }
    else
    {
      HCI_LE_error("ORPGDA_read(LBID_MODEL_HAIL) failed (%d)", retval);
    }
  }
  else
  {
    sprintf( buf, "%s.height_0", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 ) > 0 )
        Hail_0_height = (float) value;

    sprintf( buf, "%s.height_minus_20", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_minus_20_height = (float) value;

    sprintf( buf, "%s.hail_date_yy", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_date[0] = (int) value;

    sprintf( buf, "%s.hail_date_mm", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_date[1] = (int) value;

    sprintf( buf, "%s.hail_date_dd", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_date[2] = (int) value;

    sprintf( buf, "%s.hail_time_hr", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_time[0] = (int) value;

    sprintf( buf, "%s.hail_time_min", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_time[1] = (int) value;

    sprintf( buf, "%s.hail_time_sec", HAIL_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 )  > 0 )
        Hail_time[2] = (int) value;
  }
}

/************************************************************************
 *      Description: This function is called when the hail algorithm    *
 *                   adaptation data needs to be written.               *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
int Set_hail_algorithm_data( float height_0, float height_minus20, int year,
                             int mon, int day, int hour, int min, int sec ){

    static char buf[64];
    int retval;
    double value;

    sprintf( buf, "%s.height_0", HAIL_DEA_NAME );
    value = (double) height_0;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 )) < 0 )
        return( retval );

    Hail_0_height = (float) height_0;

    sprintf( buf, "%s.height_minus_20", HAIL_DEA_NAME );
    value = (double) height_minus20;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_minus_20_height = (float) height_minus20;

    sprintf( buf, "%s.hail_date_yy", HAIL_DEA_NAME );
    value = (double) year;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_date[0] = (int) year;

    sprintf( buf, "%s.hail_date_mm", HAIL_DEA_NAME );
    value = (double) mon;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_date[1] = (int) mon;

    sprintf( buf, "%s.hail_date_dd", HAIL_DEA_NAME );
    value = (double) day;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_date[2] = (int) day;

    sprintf( buf, "%s.hail_time_hr", HAIL_DEA_NAME );
    value = (double) hour;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_time[0] = (int) hour;

    sprintf( buf, "%s.hail_time_min", HAIL_DEA_NAME );
    value = (double) min;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_time[1] = (int) min;

    sprintf( buf, "%s.hail_time_sec", HAIL_DEA_NAME );
    value = (double) sec;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 ))  < 0 )
        return( retval );

    Hail_time[2] = (int) sec;

    return 0;
}

/************************************************************************
 *      Description: This function is called when the storm tracking    *
 *                   algorithm adaptation data needs to be read.        *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void Get_storm_tracking_algorithm_data(){

    static char buf[64];
    double value;

    sprintf( buf, "%s.default_dir", STORM_CELL_TRACK_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 ) )
       Storm_direction = (float) value;

    sprintf( buf, "%s.default_spd", STORM_CELL_TRACK_DEA_NAME );
    if( DEAU_get_values( buf, &value, 1 ) )
       Storm_speed = (float) value;

}

/************************************************************************
 *      Description: This function is called when the storm tracking    *
 *                   algorithm adaptation data needs to be written.     *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
int Set_storm_tracking_algorithm_data( int direction, float speed ){

    static char buf[64];
    int retval;
    double value;

    /* Set the group elements. */
    sprintf( buf, "%s.default_dir", STORM_CELL_TRACK_DEA_NAME );
    value = (double) direction;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 )) < 0 )
        return( retval );

    Storm_direction = direction;

    sprintf( buf, "%s.default_spd", STORM_CELL_TRACK_DEA_NAME );
    value = (double) speed;
    if( (retval = DEAU_set_values( buf, 0, &value, 1, 0 )) < 0 )
         return( retval );

    Storm_speed = speed;

    return 0;

}

/************************************************************************
 *      Description: This function returns a flag to indicate if the    *
 *                   Data Entry Widget is open or not.                  *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: 1 if window is open, 0 if not                           *
 ************************************************************************/

int data_widget_open()
{
  if( Data_widget != (Widget) NULL ){ return HCI_YES_FLAG; }
  return HCI_NO_FLAG;
}


