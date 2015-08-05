/************************************************************************
 *									*
 *	Module:  hci_basedata.c						*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      to open a window for displaying base data and	*
 *		      change elevation/volume scans.			*
 *									*
 *************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/01/17 20:52:16 $
 * $Id: hci_basedata.c,v 1.97 2014/01/17 20:52:16 steves Exp $
 * $Revision: 1.97 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_basedata.h>
#include <hci_scan_info.h>

/*	Macros.								*/

#define	BASE_DATA_WIDTH		  550 /* default width of window */
#define	BASE_DATA_HEIGHT	  470 /* default height of window */
#define	ZOOM_FACTOR_MAX		   64 /* maximum magnification factor */
#define	ZOOM_FACTOR_MIN		    1 /* minimum magnification factor */
#define	HALF_DEGREE		  0.5 /* 1/2 degree angle */
#define	RAW_MODE		    0 /* interrogation mode */
#define	ZOOM_MODE		    1 /* zoom mode */
#define	MAX_AZIMUTHS		  400 /* max azimuths allowed in cut */
#define	MAX_ELEVATION_SCANS	   30 /* max cuts allowed in volume */
#define MAX_ELEV_BUTTONS           32 /* max elevation buttons */
#define COLOR_BAR_WIDTH		   50 /* width of color bar */
#define COLOR_BOX_WIDTH		   25 /* width of color box */
#define TICK_MARK_LENGTH	   15 /* length of data tick mark */

#define	REFLECTIVITY_PRODUCT	19    /* reflectivity product for colors */
#define	VELOCITY_PRODUCT	25    /* velocity product for colors */
#define	SPECTRUM_WIDTH_PRODUCT	28    /* spectrum width product for colors */

#define	DISPLAY_MODE_STATIC	0 /* single cut mode (not updated) */
#define	DISPLAY_MODE_DYNAMIC	1 /* real-time mode (updated) */

char		Buf [64]; /* common buffer for string functions. */

/*	Global widget variables.					*/

Widget	Zoom_button       = (Widget) NULL;
Widget	Raw_button        = (Widget) NULL;
Widget	R_button          = (Widget) NULL;
Widget	V_button          = (Widget) NULL;
Widget	W_button          = (Widget) NULL;
Widget	Map_off_button    = (Widget) NULL;
Widget	Map_on_button     = (Widget) NULL;
Widget	Filter_off_button = (Widget) NULL;
Widget	Filter_on_button  = (Widget) NULL;
Widget	Grid_off_button   = (Widget) NULL;
Widget	Grid_on_button    = (Widget) NULL;
Widget	Vcp_label         = (Widget) NULL;
Widget	Apply_button      = (Widget) NULL;
Widget	Button [MAX_ELEV_BUTTONS];
Widget	Azran_label;
Widget	Height_label;
Widget	Value_label;
Widget	Azran_data;
Widget	Height_data;
Widget	Value_data;
Widget	Left_button;
Widget	Middle_button;
Widget	Right_button;

Widget	Data_range_dialog        = (Widget) NULL;
Widget	Reflectivity_min_scale   = (Widget) NULL;
Widget	Reflectivity_max_scale   = (Widget) NULL;
Widget	Velocity_min_scale       = (Widget) NULL;
Widget	Velocity_max_scale       = (Widget) NULL;
Widget	Spectrum_width_min_scale = (Widget) NULL;
Widget	Spectrum_width_max_scale = (Widget) NULL;

float	Azint = HALF_DEGREE; /* 1/2 azimuth interval for painting */

float	Scan_elevation = 0.0; /* Current scan elevation angle (deg) */
time_t	Start_time = 0; /* Current volume start time, UTC. */
int	Elevation_number = 0; /* Current elevation cut number */
int	Display_mode = RAW_MODE; /* cursor mode inside display area */
float	Reflectivity_min   = REFLECTIVITY_MIN; /* Minimum reflectivity factor
				to display */
float	Reflectivity_max   = REFLECTIVITY_MAX; /* Maximum reflectivity factor
				to display */
float	Velocity_min       = VELOCITY_MIN; /* Minimum velocity to display */
float	Velocity_max       = VELOCITY_MAX; /* Maximum velocity to display */
float	Spectrum_width_min = SPECTRUM_WIDTH_MIN; /* Minimum spectrum width to display */
float	Spectrum_width_max = SPECTRUM_WIDTH_MAX; /* Maximum spectrum width to display */
int	Grid_overlay_flag  = 1;	/* Used to automatically display polar	*
				 * grid whan radar data updated.	*/
int	SuperRes_cut_flag  = 0;	/* Current cut is Super Resolution cut. */
int	Map_overlay_flag   = 0; /* Used to automatically display maps	*
				 * when radar data updated.		*/
int	Field_changed      = 0; /* Used to flag when moment changed by	*
				 * operator.			 	*/
int	Filter_mode        = 0; /* Used to control whether data outside *
				 * data range not displayed (0) or set  *
				 * to min/max color (1).		*/
int	Prev_vel_resolution = DOPPLER_RESOLUTION_HIGH;

/*	Translation table for base data window.  We need a mechanism	*
 *	to capture mouse events for interrogation and zooming.		*/

String	Translations =
	"<Btn1Down>:   hci_basedata_draw(down1)   ManagerGadgetArm() \n\
	<Btn1Up>:     hci_basedata_draw(up1)     ManagerGadgetActivate() \n\
	<Btn1Motion>: hci_basedata_draw(motion1) ManagerGadgetButtonMotion() \n\
	<Btn2Down>:   hci_basedata_draw(down2)   ManagerGadgetArm() \n\
	<Btn3Down>:   hci_basedata_draw(down3)   ManagerGadgetArm()";

/*	Global variables						*/

char		BD_current_field [32]; /* Current displayed moment */
Display		*BD_display; /* Display info */
Widget		Top_widget;   /* Top level window */
Widget		BD_label;    /* Elevation/Time label at top of window */
Dimension	BD_width;    /* width of display region */
Dimension	BD_height;   /* height of display region */
Dimension	BD_depth;    /* depth (bits) of display */
Widget		BD_buttons_rowcol; /* Elevation buttons manager */
Widget		BD_draw_widget; /* drawing area widget for base data */
Window		BD_window;   /* visible Drawable */
Pixmap		BD_pixmap;   /* invisible Drawable */
GC		BD_gc;       /* graphics context */
int		BD_center_pixel; /* center pixel of display region */
int		BD_center_scanl; /* center scanline of display region */
float		BD_scale_x;  /* pixel/km in X direction */
float		BD_scale_y;  /* pixel/km in Y direction */
float		BD_value_max; /* maximum value to display for active moment */
float		BD_value_min; /* minimum value to display for active moment */
float		BD_color_scale; /* scale for converting data to color index */
float		BD_reflectivity_range [MAX_BINS_ALONG_RADIAL];
			     /* distance of bin from radar along beam
				(reflectivity moment) */
float		BD_doppler_range [MAX_BINS_ALONG_RADIAL];
			     /* distance of bin from radar along beam
				(Doppler moments) */
float		*BD_range;   /* pointer to active bin/range table */
float		BD_display_range = MAX_RADIAL_RANGE; /* maximum range */
int		BD_display_scan = -1; /* Scan cut number */
int		BD_scan; /* current elevation number */
float		BD_x_offset;  /* X offset of radar from region center */
float		BD_y_offset;  /* Y offset of radar from region center */
int		BD_display_mode = DISPLAY_MODE_DYNAMIC; /* display mode */
int		BD_data_type = REFLECTIVITY; /* active data type */
int		BD_lb;        /* basedata file pointer */
int		BD_scan_ptr [MAX_ELEVATION_SCANS][MAX_AZIMUTHS];
			      /* scan lookup table */
int		BD_azi_ptr  [MAX_ELEVATION_SCANS][MAX_AZIMUTHS];
			      /* azimuth lookup table */
float		BD_current_elevation; /* Active elevation angle */
int		BD_current_hour;      /* Active hour */
int		BD_current_minute;    /* Active minute */
int		BD_current_second;    /* Active second */
int		BD_size;	      /* Unused */
int		BD_zoom_factor = 1;   /* current zoom factor (1 to 64) */
char		*BD_data  = (char *) NULL; /* Pointer to radial data */
int		BD_color [PRODUCT_COLORS]; /* color lookup table */
int		BD_elevations = 0;    /* number of elevation cuts in scan */

int		New_volume  = 0; /* New volume flag */
int		Current_vcp = 0; /* Active VCP number */

int		Update_flag = 0; /* Data update flag */
int		Scan_info_update_flag = 0; /* Data update flag */

void	hci_display_selected_scan    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_expose_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_close_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_data_range_callback      (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_filter_callback   (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_grid_callback   (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_map_callback    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_reflectivity    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_velocity        (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_spectrum_width  (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_basedata_draw (Widget w, XEvent *event, String *args,
			int	*num_args);
void	hci_basedata_display_mode    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_close_data_range_dialog_callback (Widget w, XtPointer client_data,
			XtPointer call_data);

float	hci_find_azimuth (int pixel1, int scanl1, int pixel2, int scanl2);

void	hci_basedata_display (char *data_ptr,
			Display *display, Drawable window,
			GC gc, float x_off, float y_off, float scale_x,
			float scale_y, int center_pixel, int center_scanl,
			float min, float max, int *color, int field,
			int filter);

void	hci_basedata_display_clear (Display *display, Drawable window,
			GC gc, int width, int height, int color);

void	hci_initialize_basedata_scan_table ();
void	hci_basedata_elevation_buttons     ();
void	hci_basedata_color_bar             ();
void	hci_basedata_overlay_grid          ();

void	timer_proc ();
void	hci_scan_info_update (en_t evtcd, void *info, size_t msglen);
void	draw_dynamic_label(int);
void	draw_static_label(int);

/************************************************************************
 *	Description: This is the main function for the HCI Base Data	*
 *		     Display task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments			*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	void	hci_basedata (int argc, char **argv);
	
	hci_basedata (argc, argv);

	return (0);
}

/************************************************************************
 *	Description: This is function opens the HCI Base Data Display	*
 *		     window.						*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments			*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

void
hci_basedata (
int	argc,
char	*argv []
)
{
	Widget		form;
	Widget		frame;
	Widget		options_frame;
	Widget		options_form;
	Widget		control_form;
	Widget		button;
	Widget		label;
	Widget		close_rowcol;
	Widget		filter_rowcol;
	Widget		grid_rowcol;
	Widget		mode_rowcol;
	Widget		field_rowcol;
	Widget		field_list;
	Widget		filter_list;
	Widget		grid_list;
	Widget		mode_list;
	Widget		properties_form;
	Widget		properties_rowcol;
	Widget		properties_label_rowcol;
	Widget		properties_value_rowcol;
	Widget		button_label_rowcol;
	Widget		button_value_rowcol;
	int		i;
	XGCValues	gcv;
	XtActionsRec	actions;
	Arg		arg [10];
	int		n;
	int		status;

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_BASEDATA_TASK );

/*	Set write permission for RDA status LB. */

	ORPGDA_write_permission( ORPGDAT_RDA_STATUS );
	
/*	Initialize the basedata pointer table				*/

	hci_initialize_basedata_scan_table ();

/*	Check the current velocity resolution so the Velocity		*
 *	range can be initialized properly.				*/

	if (hci_basedata_velocity_resolution () == DOPPLER_RESOLUTION_HIGH) {

	    Velocity_min = Velocity_min/2;
	    Velocity_max = Velocity_max/2;
	    Prev_vel_resolution = DOPPLER_RESOLUTION_HIGH;
	}
	else
	{
	    Prev_vel_resolution = DOPPLER_RESOLUTION_LOW;
	}

/*	Initialize scan_info data.					*/

	status = EN_register (ORPGEVT_SCAN_INFO,
			      (void *) hci_scan_info_update);

	HCI_LE_log("EN_register ORPGEVT_SCAN_INFO: %d", status);

/*	Make "Reflectivity" the default field to display.		*
 *	Eventually, this probably should be a configurable parameter	*
 *	read in at startup time along with other hci resources (ie.,	*
 *	colors, window sizes, etc.).					*/

	strcpy (BD_current_field,"  Reflectivity");

	Top_widget = HCI_get_top_widget();
	BD_display = HCI_get_display();

	BD_depth   = XDefaultDepth (BD_display,
		DefaultScreen (BD_display));

/*	Initialize the read-only color database for widgets and		*
 *	drawing colors.							*/

	hci_initialize_product_colors (BD_display, HCI_get_colormap());
	hci_get_product_colors (REFLECTIVITY_PRODUCT, BD_color);

/*	Set various window properties.					*/

	XtVaSetValues (Top_widget,
		XmNminWidth,		250,
		XmNminHeight,		250,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Make the initial size of the base data display window something	*
 *	reasonable. The user can resize it as needed. Define a minimum  *
 *	size to be able to hold all of the widgets.			*/
/*	NOTE:  This is something which should eventually get put into	*
 *	a resource file so it can be configured at startup.		*/

	BD_width  = BASE_DATA_WIDTH; 
	BD_height = BASE_DATA_HEIGHT;

/*	Create the form widget which will manage the drawing_area and	*
 *	row_column widgets.						*/

	form = XtVaCreateWidget ("form",
		xmFormWidgetClass,		Top_widget,
		NULL);

	BD_label = XtVaCreateManagedWidget ("BD Label",
		xmLabelWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
/* XmNrecomputeSize to False prevents widget from resizing to original size.
   Otherwise, widget resizes to original size every new elevation scan. */
                XmNrecomputeSize, False,
		NULL);

/*	Use another form widget to manage all of the control buttons	*
 *	on the left side of the basedata display window.		*/

	control_form = XtVaCreateWidget ("control_form",
		xmFormWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			BD_label,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	frame = XtVaCreateManagedWidget ("close_frame",
		xmFrameWidgetClass,	control_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	close_rowcol = XtVaCreateWidget ("close_rowcol",
		xmRowColumnWidgetClass,		frame,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,			XmPACK_COLUMN,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNorientation,			XmHORIZONTAL,
		XmNnumColumns,			1,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	close_rowcol,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_basedata_close_callback,
		NULL);

	XtManageChild (close_rowcol);

/*	Display a label at the top of the control form which contains	*
 *	the VCP number of the current volume scan.			*/

	Vcp_label = XtVaCreateManagedWidget ("VCP   ",
		xmLabelWidgetClass,	control_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LARGE),
		NULL);

/*	Create a "Scan" button which will set the display mode to	*
 *	dynamic.  This displays data as it is received.  By default,	*
 *	the display mode is dynamic.					*/

	Button [0] = XtVaCreateManagedWidget ("Scan",
		xmPushButtonWidgetClass,	control_form,
		XmNforeground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			Vcp_label,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNfontList,			hci_get_fontlist (LIST),
		XmNuserData,			(XtPointer) -1,
		NULL);

	XtAddCallback (Button [0],
		XmNactivateCallback, hci_display_selected_scan,
		(XtPointer) -1);

	frame = XtVaCreateManagedWidget ("elevation_frame",
		xmFrameWidgetClass,	control_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Button [0],
		NULL);

/*	Create the row_column widget for the elevation scan buttons to 	*
 *	be placed along the left side of the form.  The buttons are	*
 *	displayed with elevation angle as labels in two columns.	*/

	BD_buttons_rowcol = XtVaCreateWidget ("basedata_rowcol",
		xmRowColumnWidgetClass,		frame,
		XmNorientation,			XmVERTICAL,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,			XmPACK_COLUMN,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNnumColumns,			3,
		NULL);

/*	Create the pushbuttons for the elevation scan selections.  When	*
 *	the VCP is changed, this list is updated accordingly.		*/

	for (i=1;i<MAX_ELEV_BUTTONS;i++) {

	    Button [i] = XtVaCreateManagedWidget ("    ",
		xmPushButtonWidgetClass,	BD_buttons_rowcol,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNuserData,			(XtPointer) (i),
		XmNalignment,			XmALIGNMENT_CENTER,
		NULL);

	    XtAddCallback (Button [i],
		XmNactivateCallback, hci_display_selected_scan,
		(XtPointer) (i));

	}

/*	Use the function hci_basedata_elevation_buttons () to define	*
 *	the VCP and elevation angles and create the elevation angle	*
 *	buttons.  This function will be called whenever the VCP is	*
 *	changed.							*/

	hci_set_scan_info_flag (1);

	Current_vcp = hci_get_scan_vcp_number ();

	hci_basedata_elevation_buttons ();

	XtManageChild (BD_buttons_rowcol);

	label = XtVaCreateManagedWidget ("space",
		xmLabelWidgetClass,		control_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			BD_buttons_rowcol,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	button = XtVaCreateManagedWidget ("Data Range",
		xmPushButtonWidgetClass,	control_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			label,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, hci_data_range_callback,
		(XtPointer) NULL);

/*	Define toggle button properties. 				*/

	n = 0;
	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);            n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);            n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;
	XtSetArg (arg [n], XmNpacking,		XmPACK_TIGHT); n++;

/*	Create the row_column widget which will hold the data filter	*
 *	selection buttons.						*/ 

	filter_rowcol = XtVaCreateWidget ("filter_rowcol",
		xmRowColumnWidgetClass,		control_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			button,
		XmNorientation,			XmHORIZONTAL,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,			1,
		NULL);

/*	Create a Label for each set of buttons and the buttons		*
 *	themselves.							*/

	XtVaCreateManagedWidget ("Filter: ",
		xmLabelWidgetClass,	filter_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	filter_list = XmCreateRadioBox (filter_rowcol,
		"filter_list", arg, n);

	Filter_on_button = XtVaCreateManagedWidget ("On",
		xmToggleButtonWidgetClass,	filter_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNset,				True,
		NULL);

	XtAddCallback (Filter_on_button,
		XmNvalueChangedCallback, hci_basedata_filter_callback,
		(XtPointer) 0);

	Filter_off_button = XtVaCreateManagedWidget ("Off",
		xmToggleButtonWidgetClass,	filter_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Filter_off_button,
		XmNvalueChangedCallback, hci_basedata_filter_callback,
		(XtPointer) 1);

	XtManageChild (filter_list);
	XtManageChild (filter_rowcol);

	label = XtVaCreateManagedWidget ("space",
		xmLabelWidgetClass,		control_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			filter_rowcol,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	options_frame = XtVaCreateManagedWidget ("options_frame",
		xmFrameWidgetClass,		control_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			label,
		NULL);

	options_form = XtVaCreateWidget ("options_form",
		xmFormWidgetClass,		options_frame,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNverticalSpacing,		1,
		NULL);

/*	Create the row_column widget which will hold the field 		*
 *	selection buttons.						*/ 

	field_rowcol = XtVaCreateWidget ("field_rowcol",
		xmRowColumnWidgetClass,		options_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNorientation,			XmHORIZONTAL,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,			1,
		NULL);

/*	Create a Label for each set of buttons and the buttons		*
 *	themselves.							*/

	XtVaCreateManagedWidget ("Moment: ",
		xmLabelWidgetClass,	field_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	field_list = XmCreateRadioBox (field_rowcol,
		"field_list", arg, n);

	R_button = XtVaCreateManagedWidget ("R ",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNset,				True,
		NULL);

	XtAddCallback (R_button,
		XmNvalueChangedCallback, hci_basedata_reflectivity, NULL);

	V_button = XtVaCreateManagedWidget ("V ",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (V_button,
		XmNvalueChangedCallback, hci_basedata_velocity, NULL);

	W_button = XtVaCreateManagedWidget ("W",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (W_button,
		XmNvalueChangedCallback, hci_basedata_spectrum_width, NULL);

	XtManageChild (field_list);
	XtManageChild (field_rowcol);

/*	Create the row_column widget which will hold the grid overlay	*
 *	selection buttons.						*/ 

	grid_rowcol = XtVaCreateWidget ("grid_rowcol",
		xmRowColumnWidgetClass,		options_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			field_rowcol,
		XmNorientation,			XmHORIZONTAL,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,			1,
		NULL);

/*	Create a Label for each set of buttons and the buttons		*
 *	themselves.							*/

	XtVaCreateManagedWidget ("Grid:   ",
		xmLabelWidgetClass,	grid_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	grid_list = XmCreateRadioBox (grid_rowcol,
		"grid_list", arg, n);

	Grid_on_button = XtVaCreateManagedWidget ("On ",
		xmToggleButtonWidgetClass,	grid_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNset,				True,
		NULL);

	XtAddCallback (Grid_on_button,
		XmNvalueChangedCallback, hci_basedata_grid_callback,
		(XtPointer) 1);

	Grid_off_button = XtVaCreateManagedWidget ("Off",
		xmToggleButtonWidgetClass,	grid_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Grid_off_button,
		XmNvalueChangedCallback, hci_basedata_grid_callback,
		(XtPointer) 0);

	XtManageChild (grid_list);
	XtManageChild (grid_rowcol);

/*	Create the row_column widget which will hold the mode		*
 *	selection buttons.						*/ 

	mode_rowcol = XtVaCreateWidget ("mode_rowcol",
		xmRowColumnWidgetClass,		options_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			grid_rowcol,
		XmNorientation,			XmHORIZONTAL,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,			1,
		NULL);

	XtVaCreateManagedWidget ("Mode:   ",
		xmLabelWidgetClass,		mode_rowcol,
		XmNbackground,	 		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	mode_list = XmCreateRadioBox (mode_rowcol,
		"mode_list", arg, n);

	Raw_button = XtVaCreateManagedWidget ("Raw",
		xmToggleButtonWidgetClass,	mode_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNset,				True,
		NULL);

	XtAddCallback (Raw_button,
		XmNvalueChangedCallback, hci_basedata_display_mode,
		(XtPointer) RAW_MODE);

	Zoom_button = XtVaCreateManagedWidget ("Zoom",
		xmToggleButtonWidgetClass,	mode_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Zoom_button,
		XmNvalueChangedCallback, hci_basedata_display_mode,
		(XtPointer) ZOOM_MODE);

	XtManageChild (mode_list);
	XtManageChild (mode_rowcol);

	XtManageChild (options_form);
	XtManageChild (control_form);

/*	Create a widget along the bottom of the window to left of the	*
 *	control buttons which will contain various data attributes.	*/

	properties_form = XtVaCreateWidget ("properties_form",
		xmFormWidgetClass,		form,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			control_form,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	properties_rowcol = XtVaCreateManagedWidget ("properties_rowcol",
		xmRowColumnWidgetClass,		properties_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	button_label_rowcol = XtVaCreateManagedWidget ("button_label_rowcol",
		xmRowColumnWidgetClass,		properties_rowcol,
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("Left Button: ",
		xmLabelWidgetClass,	button_label_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Middle Button: ",
		xmLabelWidgetClass,	button_label_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Right Button: ",
		xmLabelWidgetClass,	button_label_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	button_value_rowcol = XtVaCreateManagedWidget ("button_value_rowcol",
		xmRowColumnWidgetClass,		properties_rowcol,
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Left_button = XtVaCreateManagedWidget ("Interrogate",
		xmLabelWidgetClass,	button_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Middle_button = XtVaCreateManagedWidget ("N/A",
		xmLabelWidgetClass,	button_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Right_button = XtVaCreateManagedWidget ("N/A",
		xmLabelWidgetClass,	button_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
 
	properties_label_rowcol = XtVaCreateManagedWidget ("properties_label_rowcol",
		xmRowColumnWidgetClass,		properties_rowcol,
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Azran_label = XtVaCreateManagedWidget ("Azran: ",
		xmLabelWidgetClass,	properties_label_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Height_label = XtVaCreateManagedWidget ("Height: ",
		xmLabelWidgetClass,	properties_label_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Value_label = XtVaCreateManagedWidget ("Value: ",
		xmLabelWidgetClass,	properties_label_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	properties_value_rowcol = XtVaCreateManagedWidget ("properties_value_rowcol",
		xmRowColumnWidgetClass,		properties_rowcol,
		XmNorientation,			XmVERTICAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Azran_data = XtVaCreateManagedWidget ("               ",
		xmLabelWidgetClass,	properties_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Height_data = XtVaCreateManagedWidget ("               ",
		xmLabelWidgetClass,	properties_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Value_data = XtVaCreateManagedWidget ("       ",
		xmLabelWidgetClass,	properties_value_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
 
	XtManageChild (properties_form);

/*	Create the drawing_area widget which will be used to display	*
 *	base level radar data.  It will occupy the upper right portion	*
 *	of the form.							*/

	actions.string = "hci_basedata_draw";
	actions.proc   = (XtActionProc) hci_basedata_draw;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	BD_draw_widget = XtVaCreateWidget ("basedata_drawing_area",
		xmDrawingAreaWidgetClass,	form,
		XmNwidth,			BD_width,
		XmNheight,			BD_height,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			BD_label,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			BD_buttons_rowcol,
		XmNbottomWidget,		properties_form,
		XmNtranslations,		XtParseTranslationTable (Translations),
		NULL);

/*	Add an expose callback for the drawing_area in order to allow	*
 *	holes to be filled in the display when  other windows are moved	*
 *	across it.							*/

	XtAddCallback (BD_draw_widget,
		XmNexposeCallback, hci_basedata_expose_callback, NULL);

/*	Permit the user to resize the base data display window.		*/

	XtAddCallback (BD_draw_widget,
		XmNresizeCallback, hci_basedata_resize_callback, NULL);

	XtManageChild (BD_draw_widget);
	XtManageChild (form);

	XtRealizeWidget (Top_widget);

/*	Define the various window variables to be used as arguments in 	*
 *	the various Xlib and Xt calls.					*/

	BD_window  = XtWindow      (BD_draw_widget);
	BD_pixmap  = XCreatePixmap (BD_display,
		BD_window,
		BD_width,
		BD_height,
		BD_depth);

/*	Define the Graphics Context to be used in drawing data		*/

	gcv.foreground = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
	gcv.background = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
	gcv.graphics_exposures = FALSE;

	BD_gc = XCreateGC (BD_display,
	      BD_window,
	      GCBackground | GCForeground | GCGraphicsExposures, &gcv);

	XSetFont (BD_display, BD_gc, hci_get_font (LIST));

	XtVaSetValues (BD_draw_widget,
		XmNbackground,	hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
		NULL);

/*	Clear the data display portion of the window by filling it with	*
 *	the background color.						*/

	XSetForeground (BD_display, BD_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (BD_display,
		BD_pixmap,
		BD_gc,
		0, 0,
		BD_width,
		BD_width);

/*	Define the data display window properties.			*/

	BD_center_pixel = (BD_width-COLOR_BAR_WIDTH)/2;
	BD_center_scanl = BD_height/2;
	BD_scale_x      = BD_zoom_factor * (BD_width-COLOR_BAR_WIDTH)/
			   (2.0*BD_display_range);
	BD_scale_y      = -BD_scale_x;

	BD_value_max    = REFLECTIVITY_MAX;
	BD_value_min    = REFLECTIVITY_MIN;
	BD_color_scale  = (BD_value_max-BD_value_min)/(PRODUCT_COLORS-1);

/*	Create a lookup table containing the range of all gates along	*
 *	a given radial.							*/

	for (i=0;i<MAX_BINS_ALONG_RADIAL;i++) {

		BD_reflectivity_range [i] =
			((hci_basedata_range_adjust (BD_data_type) + i) *
			  hci_basedata_bin_size (BD_data_type)) /
				HCI_METERS_PER_KM;
		BD_doppler_range [i] =
			((hci_basedata_range_adjust (BD_data_type) + i) *
			  hci_basedata_bin_size (BD_data_type)) /
				HCI_METERS_PER_KM;

	}

	BD_range = BD_reflectivity_range;

	XtRealizeWidget (Top_widget);

	hci_basedata_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	/* Start HCI loop. */

	HCI_start( timer_proc, HCI_HALF_SECOND, RESIZE_HCI );
}

/************************************************************************
 *	Description: This function handles the base window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the base data pixmap to the	*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Set the clip region to the full window, update the window by	*
 *	copying the base data pixmap to it to fill in any holes left	*
 *	by any previously overlain window and the restoring the clip	*
 *	region to the base data display region only.			*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BD_width;
	clip_rectangle.height  = BD_height;

	XSetClipRectangles (BD_display,
			    BD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XCopyArea (BD_display,
		   BD_pixmap,
		   BD_window,
		   BD_gc,
		   0,
		   0,
		   BD_width,
		   BD_height,
		   0,
		   0);

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BD_width-COLOR_BAR_WIDTH;
	clip_rectangle.height  = BD_height;

	XSetClipRectangles (BD_display,
			    BD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

}

/************************************************************************
 *	Description: This function handles the base window resize	*
 *		     callback by redrawing all base data window		*
 *		     components.					*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_resize_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Get the new size of the base data window.			*/

	if ((BD_draw_widget == (Widget) NULL)    ||
	    (BD_display     == (Display *) NULL) ||
	    (BD_window      == (Window) NULL)    ||
	    (BD_pixmap      == (Pixmap) NULL)    ||
	    (BD_gc          == (GC) NULL)) {

	    return;

	}

	XtVaGetValues (BD_draw_widget,
		XmNwidth,	&BD_width,
		XmNheight,	&BD_height,
		NULL);

/*	Destroy the old pixmap since the size has changed and create	*
 *	create a new one.						*/

	if (BD_pixmap != (Window) NULL) {

	    XFreePixmap (BD_display, BD_pixmap);

	}

	BD_pixmap = XCreatePixmap (BD_display,
		BD_window,
		BD_width,
		BD_height,
		BD_depth);

/*	Recompute the scale factors (in pixels/km) and a new window	*
 *	center pixel/scanline coordinate.				*/

	BD_scale_x      = BD_zoom_factor * (BD_width-COLOR_BAR_WIDTH)/
			   (2*BD_display_range);
	BD_scale_y	= -BD_scale_x;
	BD_center_pixel = (BD_width-COLOR_BAR_WIDTH) / 2;
	BD_center_scanl = BD_height / 2;

/*	Set the clip region to the entire window so we can create a	*
 *	color bar along the right size.					*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BD_width;
	clip_rectangle.height  = BD_height;

	XSetClipRectangles (BD_display,
			    BD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XSetForeground (BD_display, BD_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (BD_display,
			BD_pixmap,
			BD_gc,
			0, 0,
			BD_width,
			BD_height);

	hci_basedata_color_bar ();

/*	Restore the clip window to the data display area only.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BD_width-COLOR_BAR_WIDTH;
	clip_rectangle.height  = BD_height;

	XSetClipRectangles (BD_display,
			    BD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	If we are looking at a previously read scan then lets display	*
 *	it.								*/

	if (BD_display_mode == DISPLAY_MODE_STATIC) {

		hci_display_selected_scan (Button [BD_display_scan],
			(XtPointer) BD_display_scan,
			(XtPointer) NULL);

	}

/*	If we want a grid overlay then display it.			*/

	if (Grid_overlay_flag) {

	    hci_basedata_overlay_grid ();

	}

/*	Update the title in the window header.				*/

	if (BD_display_mode == DISPLAY_MODE_STATIC)
	{
		draw_static_label(-1);
	}
	else
	{
		draw_dynamic_label(0);
	}

/*	Call the expose callback to write the base data pixmap to the	*
 *	base data window.						*/

	hci_basedata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function draws a color bar along the right	*
 *		     side of the base data window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_color_bar ()
{
	int	i;

	int	width;
	int	height;
	int	scanl;
	float	xheight;
	char	text [16];
	XRectangle	clip_rectangle;
	int	step;

/*	Define the clip region to include the entire display region.	*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BD_width;
	clip_rectangle.height  = BD_height;

	XSetClipRectangles (BD_display,
			    BD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	Clear an area to the right of the base data display to place	*
 *	the color bar.							*/

	XSetForeground (BD_display, BD_gc,
		hci_get_read_color (BACKGROUND_COLOR1));

	XFillRectangle (BD_display,
			BD_window,
			BD_gc,
			(int) (BD_width-COLOR_BAR_WIDTH),
			0,
			COLOR_BAR_WIDTH,
			BD_height);

	XFillRectangle (BD_display,
			BD_pixmap,
			BD_gc,
			(int) (BD_width-COLOR_BAR_WIDTH),
			0,
			COLOR_BAR_WIDTH,
			BD_height);

/*	Define the width of each color bar box and the height scale	*/

	width = 10;

	if (BD_data_type == SPECTRUM_WIDTH) {

	    xheight = BD_height/(2.0*(PRODUCT_COLORS/2-1));
	    step = 2;

	} else {

	    xheight = BD_height/(2.0*(PRODUCT_COLORS-1));
	    step = 1;

	}

	height  = xheight+1;

/*	For each color element, create a box and fill it with the color	*/

	for (i=step;i<PRODUCT_COLORS;i=i+step) {

		scanl = BD_height/4 + xheight*(i/step-1);

		XSetForeground (BD_display, BD_gc,
			BD_color [i]);

		XFillRectangle (BD_display,
			BD_window,
			BD_gc,
			(int) (BD_width-TICK_MARK_LENGTH),
			scanl,
			width,
			height);

		XFillRectangle (BD_display,
			BD_pixmap,
			BD_gc,
			(int) (BD_width-TICK_MARK_LENGTH),
			scanl,
			width,
			height);

	}

	XSetForeground (BD_display, BD_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XDrawRectangle (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-TICK_MARK_LENGTH),
		(int) (BD_height/4),
		width,
		(int) (BD_height/2));

	XDrawRectangle (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-TICK_MARK_LENGTH),
		(int) (BD_height/4),
		width,
		(int) (BD_height/2));


/*	Draw text labels to the left of the color bar for the minimin,	*
 *	middle, and maximum values.					*/

	scanl = BD_height/4;

	if (BD_data_type == REFLECTIVITY) {

	    if (BD_value_min < 0.0) {

		sprintf (text,"%d",(int) (BD_value_min-0.5));

	    } else {

		sprintf (text,"%d",(int) (BD_value_min+0.5));

	    }

	} else {

	    if (BD_value_min < 0.0) {

		sprintf (text,"%d",(int) ((BD_value_min*HCI_MPS_TO_KTS)-0.5));

	    } else {

		sprintf (text,"%d",(int) ((BD_value_min*HCI_MPS_TO_KTS)+0.5));

	    }

	}

	XDrawString (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		scanl+5,
		text,
		strlen (text));

	XDrawString (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		scanl+5,
		text,
		strlen (text));

	XDrawLine (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	XDrawLine (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	if (BD_data_type == REFLECTIVITY) {

	    if (BD_value_max < 0.0) {

		sprintf (text,"%d",(int) (BD_value_max-0.5));

	    } else {

		sprintf (text,"%d",(int) (BD_value_max+0.5));

	    }

	} else {

	    if (BD_value_max < 0.0) {

		sprintf (text,"%d",(int) ((BD_value_max*HCI_MPS_TO_KTS)-0.5));

	    } else {

		sprintf (text,"%d",(int) ((BD_value_max*HCI_MPS_TO_KTS)+0.5));

	    }

	}

	if (BD_data_type == REFLECTIVITY) {

	    scanl = BD_height/4 + xheight*(PRODUCT_COLORS-1);

	    XDrawString (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	    XDrawString (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	    XDrawLine (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	    XDrawLine (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	} else {

	    if (BD_data_type == SPECTRUM_WIDTH) {

		scanl = BD_height/4 + xheight*(PRODUCT_COLORS/2-2);

	    } else {

		scanl = BD_height/4 + xheight*(PRODUCT_COLORS-2);

	    }

	    XDrawString (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	    XDrawString (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	    XDrawLine (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	    XDrawLine (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	    if (BD_data_type == SPECTRUM_WIDTH) {
 
		scanl = BD_height/4 + xheight*(PRODUCT_COLORS/2-1);

	    } else {
 
		scanl = BD_height/4 + xheight*(PRODUCT_COLORS-1);

	    }

	    XDrawString (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		"RF",
		2);

	    XDrawString (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		"RF",
		2);

	    XDrawLine (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	    XDrawLine (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	}

	if (BD_data_type == REFLECTIVITY) {

	    scanl = BD_height/4 + xheight*(PRODUCT_COLORS-1)/2;

	    if ((BD_value_max+BD_value_min) < 0.0) {

		sprintf (text,"%d",(int) ((BD_value_max+BD_value_min)/2-0.5));

	    } else {

		sprintf (text,"%d",(int) ((BD_value_max+BD_value_min)/2+0.5));

	    }

	} else {

	    if (BD_data_type == VELOCITY) {

	    	scanl = BD_height/4 + xheight*(PRODUCT_COLORS-2)/2;

	    } else {

	    	scanl = BD_height/4 + xheight*(PRODUCT_COLORS/2-2)/2;

	    }

	    if ((BD_value_max+BD_value_min) < 0.0) {

		sprintf (text,"%d",(int) (((BD_value_max+BD_value_min)*HCI_MPS_TO_KTS)/2-0.5));

	    } else {

		sprintf (text,"%d",(int) (((BD_value_max+BD_value_min)*HCI_MPS_TO_KTS)/2+0.5));

	    }

	}

	XDrawString (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	XDrawString (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	XDrawLine (BD_display,
		BD_window,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

	XDrawLine (BD_display,
		BD_pixmap,
		BD_gc,
		(int) (BD_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (BD_width-TICK_MARK_LENGTH),
		scanl);

/*	Reset the clip rectangle so that drawing in the base data	*
 *	display region will not write over the color bar.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BD_width-COLOR_BAR_WIDTH;
	clip_rectangle.height  = BD_height;

	XSetClipRectangles (BD_display,
			    BD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

}

#define ONE_BAM  0.043945

/************************************************************************
 *	Description: This function creates a vertical column of		*
 *		     pushbuttons along the left side of the basedata	*
 *		     display window.  A button is created for each	*
 *		     elevation scan level in the current VCP.  In	*
 *		     addition, a special "scan" button is created so	*
 *		     the user can view a constantly updated display of	*
 *		     the radar data being received from the RDA.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_elevation_buttons ()
{
	XmString	text;
	int		i;
	int		number_cuts;
	float		old_angle = -99;

	void	hci_data_range_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

/*	Get the number of elevation cuts in the current VCP	*/

	number_cuts = hci_get_scan_number_elevation_cuts ();

/*	Update the VCP Label in the Base Data Display Window.	*/

	sprintf (Buf,"VCP %2d", hci_get_scan_vcp_number ());
	text = XmStringCreateLocalized (Buf);

	XtVaSetValues (Vcp_label,
		XmNlabelString,	text,
		NULL);

	XmStringFree (text);

/*	Now update elevation angle labels for all cuts.			*/

	BD_elevations = 0;

	for (i=1;i<=number_cuts;i++) {

	     if( fabsf(old_angle - hci_get_scan_elevation_angle (i-1)) >= ONE_BAM ){

		BD_elevations++;
		old_angle = hci_get_scan_elevation_angle (i-1);
		sprintf (Buf," %4.1f ",hci_get_scan_elevation_angle (i-1));
		text = XmStringCreateLocalized (Buf);

		XtVaSetValues (Button [BD_elevations],
			XmNlabelString,	text,
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			NULL);

		XmStringFree (text);

/*		Manage all of those which are active.		*/

		XtManageChild (Button [BD_elevations]);

	    }
	}

/*	Unmanage all inactive elevation cut buttons.		*/

	for (i=BD_elevations+1;i<MAX_ELEV_BUTTONS;i++) {

	    XtUnmanageChild (Button [i]);

	}
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to reflectivity.				*
 *									*
 *	Input:  w - reflectivity radio button ID			*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_reflectivity (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

	    HCI_LE_log("Base Data DZ selected");

/*	    Reset the current field, range, min, and max fields.	*/

	    Field_changed = 1;
	    BD_data_type = REFLECTIVITY;
	    strcpy (BD_current_field,"  Reflectivity");

	    BD_range    = BD_reflectivity_range;

	    BD_value_min  = Reflectivity_min;
	    BD_value_max  = Reflectivity_max;

	    BD_color_scale = (BD_value_max - BD_value_min)/(PRODUCT_COLORS-1);

	    hci_get_product_colors (REFLECTIVITY_PRODUCT, BD_color);

/*	    If we are looking at a previously read scan then lets 	*
 *	    display it.							*/

	    if (BD_display_mode == DISPLAY_MODE_STATIC) {

		hci_display_selected_scan (Button [BD_display_scan],
			(XtPointer) BD_display_scan,
			(XtPointer) NULL);

/*	    Else, lets clear the display.				*/

	    } else {

	        hci_basedata_display_clear (BD_display,
			    BD_pixmap,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	        hci_basedata_display_clear (BD_display,
			    BD_window,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	    }

/*	    Redisplay the color bar for the new field.			*/

	    hci_basedata_color_bar ();

/*	    Display the polar grid if enabled.				*/

	    if (Grid_overlay_flag) {

	        hci_basedata_overlay_grid ();

	    }

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    hci_basedata_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to velocity.				*
 *									*
 *	Input:  w - velocity radio button ID				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_velocity (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

	    HCI_LE_log("Base Data VR selected");

/*	    Reset the current field, range, min, and max fields.	*/

	    Field_changed = 1;
	    BD_data_type = VELOCITY;
	    strcpy (BD_current_field,"      Velocity");

	    BD_range = BD_doppler_range;

	    BD_value_min  = Velocity_min;
	    BD_value_max  = Velocity_max;

	    BD_color_scale = (BD_value_max - BD_value_min)/(PRODUCT_COLORS-1);

	    hci_get_product_colors (VELOCITY_PRODUCT, BD_color);

/*	    If we are looking at a previously read scan then lets 	*
 *	    display it.							*/

	    if (BD_display_mode == DISPLAY_MODE_STATIC) {

		hci_display_selected_scan (Button [BD_display_scan],
			(XtPointer) BD_display_scan,
			(XtPointer) NULL);

/*	    Else, lets clear the display.				*/

	    } else {

	        hci_basedata_display_clear (BD_display,
			    BD_pixmap,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	        hci_basedata_display_clear (BD_display,
			    BD_window,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	    }

/*	    Redisplay the color bar for the new field.			*/

	    hci_basedata_color_bar ();

/*	    Display the polar grid if enabled.				*/

	    if (Grid_overlay_flag) {

	        hci_basedata_overlay_grid ();

	    }

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    hci_basedata_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to spectrum width.				*
 *									*
 *	Input:  w - spectrum width radio button ID			*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_spectrum_width (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

	    HCI_LE_log("Base Data SW selected");

/*	    Reset the current field, range, min, and max fields.	*/

	    Field_changed = 1;
	    BD_data_type = SPECTRUM_WIDTH;
	    strcpy (BD_current_field,"Spectrum Width");

	    BD_range = BD_doppler_range;

	    BD_value_min  = Spectrum_width_min;
	    BD_value_max  = Spectrum_width_max;

	    BD_color_scale = (BD_value_max - BD_value_min)/(PRODUCT_COLORS-1);

	    hci_get_product_colors (SPECTRUM_WIDTH_PRODUCT, BD_color);

/*	    If we are looking at a previously read scan then lets 	*
 *	    display it.							*/

	    if (BD_display_mode == DISPLAY_MODE_STATIC) {

		hci_display_selected_scan (Button [BD_display_scan],
			(XtPointer) BD_display_scan,
			(XtPointer) NULL);

/*	    Else, lets clear the display.				*/

	    } else {

	        hci_basedata_display_clear (BD_display,
			    BD_pixmap,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	        hci_basedata_display_clear (BD_display,
			    BD_window,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	    }

/*	    Redisplay the color bar for the new field.			*/

	    hci_basedata_color_bar ();

/*	    Display the polar grid if enabled.				*/

	    if (Grid_overlay_flag) {

	    	hci_basedata_overlay_grid ();

	    }

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    hci_basedata_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is used to display the selected	*
 *		     elevation scan from the list of elevation buttons.	*
 *		     If the "Scan" button is selected, dynamic display	*
 *		     mode is activated and data from all scans are	*
 *		     displays as they are ingested.			*
 *									*
 *	Input:  w - push button ID					*
 *		client_data - scan index (-1 for dynamic mode)		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_display_selected_scan (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	int	i;
	char	buf [32];
	int	vol_num;
	int	sub_type;
	LB_id_t	start_id;
	LB_id_t	stop_id;
	int	new_vel_resolution;
static	int	first_time = 1;

	HCI_LE_log("Base Data scan %d selected", (int) client_data);

/*	Get the current elevation scan from the button client data.	*/

	BD_display_scan = (int) client_data;

/*	Clear the contents of the base data window.			*/

	hci_basedata_display_clear (BD_display,
			    BD_pixmap,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

/*	If we are in dynamic display mode we read and display the	*
 *	data directly from the raw RPG input buffer.			*/

	if (BD_display_scan == -1) {

/*	    If we had previously displayed a static scan from the	*
 *	    replay database, free memory associated with it.		*/

	    if (BD_data != NULL) {

		free (BD_data);
		BD_data = NULL;

	    }

	    BD_display_mode = DISPLAY_MODE_DYNAMIC;

	    hci_basedata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

/*	    Set the foreground/background colors of the static		*
 *	    elevation cut buttons to white/steelblue to indicate	*
 *	    they are not active.					*/

	    for (i=1;i<=hci_get_scan_number_elevation_cuts ();i++) {

		XtVaSetValues (Button [i],
		    XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
		    XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
		    NULL);

	    }

/*	    Set the "scan" button colors to steelblue on white		*
 *	    to indicate dynamic mode set.				*/

	    XtVaSetValues (w,
		    XmNbackground,	hci_get_read_color (BUTTON_FOREGROUND),
		    XmNforeground,	hci_get_read_color (BUTTON_BACKGROUND),
		    NULL);

	    draw_dynamic_label(1);

	    if (Grid_overlay_flag) {

		hci_basedata_overlay_grid ();

	    }

/*	    Set the data pointer to the current active basedata		*
 *	    message and reset the scan index to reflect the current	*
 *	    elevation cut for interrogation.				*/

	    BD_scan = hci_basedata_elevation_number ();

	} else {

/*	    For the rest of the cases we read the data from the replay	*
 *	    database since we are guaranteed that at least one full	*
 *	    volume of data will be available.				*/

	    if (first_time) {	/* we want to register if first time */

		first_time = 0;

		status = ORPGBDR_reg_radial_replay_type (BASEDATA,
					ORPGDAT_BASEDATA_REPLAY,
					ORPGDAT_BASEDATA_ACCT);

		if (status != 0) {

		    HCI_LE_error("ORPGBDR_reg_radial_replay_type (%d)", status);

		    XSetFont (BD_display, BD_gc, hci_get_font (LARGE));

		    XSetForeground (BD_display, BD_gc,
			hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

		    sprintf (buf,"Product Not Available");

		    XDrawString (BD_display,
			 BD_pixmap,
			 BD_gc,
			 (int) (BD_center_pixel -
				4*strlen (buf)),
			 BD_center_scanl + 4,
			 buf,
			 strlen (buf));

		    XSetFont (BD_display, BD_gc, hci_get_font (LIST));

		    hci_basedata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

		    return;

		}
	    }

	    if( BD_data_type == VELOCITY )
	    {
	      new_vel_resolution = hci_basedata_velocity_resolution();
	      if( new_vel_resolution != Prev_vel_resolution )
	      {
	        Prev_vel_resolution = new_vel_resolution;
	        if( new_vel_resolution == DOPPLER_RESOLUTION_HIGH )
	        {
	          Velocity_min = VELOCITY_MIN/2;
	          Velocity_max = VELOCITY_MAX/2; 
	        }
	        else
	        {
	          Velocity_min = VELOCITY_MIN;
	          Velocity_max = VELOCITY_MAX; 
	        }
		BD_value_max = Velocity_max;
		BD_value_min = Velocity_min;
	        hci_basedata_color_bar();
	      }
	    }

	    if (BD_data == NULL) {

		BD_data = calloc (SIZEOF_BASEDATA, 1);

		if (BD_data == NULL) {

		    HCI_LE_error("calloc (SIZEOF_BASEDATA, 1) failed");
		    return;

		}
	    }

/*	    Set the display mode to static so we don't get the display	*
 *	    updated with new data when we are finished.			*/

	    BD_display_mode = DISPLAY_MODE_STATIC;

	    for (i=0;i<hci_get_scan_number_elevation_cuts ();i++) {

	        XtVaSetValues (Button [i],
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			NULL);

	    }

	    XtVaSetValues (w,
		XmNbackground,	hci_get_read_color (BUTTON_FOREGROUND),
		XmNforeground,	hci_get_read_color (BUTTON_BACKGROUND),
		NULL);

/*	    Read the first radial in the elevation cut and extract the	*
 *	    time and elevation information and display it in the Base	*
 *	    Data Window header.						*/
	
	    vol_num = ORPGVST_get_volume_number();

            if( BD_data_type == REFLECTIVITY )
               sub_type = (BASEDATA_TYPE | REFLDATA_TYPE);

            else
               sub_type = (BASEDATA_TYPE | COMBBASE_TYPE);
 
	    start_id = ORPGBDR_get_start_of_elevation_msgid (BASEDATA,
				sub_type,
				vol_num,
				BD_display_scan);
 
	    stop_id  = ORPGBDR_get_end_of_elevation_msgid (BASEDATA,
				sub_type,
				vol_num,
				BD_display_scan);

	    if ((start_id <= 0) || (stop_id <= 0)) {
 
		vol_num--;

		start_id = ORPGBDR_get_start_of_elevation_msgid (BASEDATA,
				sub_type,
				vol_num,
				BD_display_scan);
 
		stop_id  = ORPGBDR_get_end_of_elevation_msgid (BASEDATA,
				sub_type,
				vol_num,
				BD_display_scan);
	
		if ((start_id <= 0) || (stop_id <= 0)) {

		    XSetFont (BD_display, BD_gc, hci_get_font (LARGE));

		    XSetForeground (BD_display, BD_gc,
			hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

		    sprintf (buf,"Product Not Available");

		    XDrawString (BD_display,
			 BD_pixmap,
			 BD_gc,
			 (int) (BD_center_pixel -
				4*strlen (buf)),
			 BD_center_scanl + 4,
			 buf,
			 strlen (buf));

		    XSetFont (BD_display, BD_gc, hci_get_font (LIST));

		    hci_basedata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

		    return;

		}
	    }

	    draw_static_label(vol_num);

	    for (i=start_id;i<=stop_id;i++) {

		status = ORPGBDR_read_radial (BASEDATA,
				BD_data, SIZEOF_BASEDATA, (LB_id_t) i);

		if (status <= 0) {

		    HCI_LE_error("ORPGBDR_read_radial () failed (%d)", status);
		    break;

		}

		if (BD_data_type == REFLECTIVITY) {

		    hci_basedata_display (BD_data,
				      BD_display,
				      BD_pixmap,
				      BD_gc,
				      BD_x_offset,
				      BD_y_offset,
				      BD_scale_x,
				      BD_scale_y,
				      BD_center_pixel,
				      BD_center_scanl,
				      BD_value_min,
				      BD_value_max,
				      BD_color,
				      BD_data_type,
				      Filter_mode);

		} else {

		    hci_basedata_display (BD_data,
				      BD_display,
				      BD_pixmap,
				      BD_gc,
				      BD_x_offset,
				      BD_y_offset,
				      BD_scale_x,
				      BD_scale_y,
				      BD_center_pixel,
				      BD_center_scanl,
				      BD_value_min*HCI_MPS_TO_KTS,
				      BD_value_max*HCI_MPS_TO_KTS,
				      BD_color,
				      BD_data_type,
				      Filter_mode);

		}
	    }

	}

/*	display a polar grid if enabled.				*/

	if (Grid_overlay_flag) {

	    hci_basedata_overlay_grid ();

	}

/*	Invoke the expose callback to make changes visible.		*/

	hci_basedata_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function handles all mouse button events	*
 *		     inside the basedata display region.		*
 *									*
 *	Input:  w - drawing area widget ID				*
 *		event - X event data					*
 *		args - number of user argunments			*
 *		num_args - user arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_draw (
Widget		w,
XEvent		*event,
String		*args,
int		*num_args
)
{
	static	int	button_down = 0;
	static	int	first_pixel;
	static	int	first_scanl;
	float	x1, y1;
	float	azimuth, range;
	float	height;
	float	value;
	XmString	string;

	float	hci_basedata_interrogate (char *buf,
				float azimuth, float range,
				int field, int ptr);

/*	Ignore button clicks outside of drawing area. */

	if( event->xbutton.x > ( BD_width - COLOR_BAR_WIDTH ) )
	{
	  return;
	}

	value = 0.0;

	if (!strcmp (args[0], "down1")) {

	    HCI_LE_log("Base Data left button pushed");

/*	If the left button is pressed, save the value of this point for	*
 *	future reference.  For editing clutter suppression regions,	*
 *	this point marks the left side of the sector.			*/

	    first_pixel = event->xbutton.x;
	    first_scanl = event->xbutton.y;

/*	Set the button down flag to indicate that the left mouse button	*
 *	has been pressed.  This is necessary for letting the rubber-	*
 *	banding code to know when to start (for motion events when the	*
 *	left mouse button is pressed).					*/

	    button_down = 1;

	    if (Display_mode == ZOOM_MODE) {

		BD_zoom_factor = BD_zoom_factor*2;

		if (BD_zoom_factor > ZOOM_FACTOR_MAX) {

		    BD_zoom_factor = ZOOM_FACTOR_MAX;
		    return;

		}

		BD_x_offset = (BD_center_pixel-first_pixel)/
				BD_scale_x + BD_x_offset;
		BD_y_offset = (BD_center_scanl-first_scanl)/
				BD_scale_y + BD_y_offset;

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BD_center_pixel +
				         BD_x_offset*BD_scale_x),
				  (int) (BD_center_scanl +
				         BD_y_offset*BD_scale_y));

		x1 = BD_x_offset;
		y1 = BD_y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > BD_display_range) {

		   BD_x_offset = (BD_x_offset/range)*BD_display_range;
		   BD_y_offset = (BD_y_offset/range)*BD_display_range;

		}

		BD_scale_x  = BD_zoom_factor * (BD_width-COLOR_BAR_WIDTH)/
				(2*BD_display_range);
		BD_scale_y  = - BD_zoom_factor * (BD_width-COLOR_BAR_WIDTH)/
				(2*BD_display_range);

	        hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    } else {

		azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BD_center_pixel +
				         BD_x_offset*BD_scale_x),
				  (int) (BD_center_scanl +
				         BD_y_offset*BD_scale_y));

		x1 = (BD_center_pixel-event->xbutton.x)/
		     BD_scale_x + BD_x_offset;
		y1 = (BD_center_scanl-event->xbutton.y)/
		     BD_scale_y + BD_y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		sprintf (Buf,"%d deg, %d nm",
			(int) azimuth, (int) (range*HCI_KM_TO_NM +0.5));

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Azran_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

		height = range*sin((double) Scan_elevation*HCI_DEG_TO_RAD)
		       + range*range/(2.0*1.21*6371.0);

		sprintf (Buf,"%d ft (ARL)", (int) (3281*height+0.5));

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Height_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

		if (BD_display_mode == DISPLAY_MODE_DYNAMIC) {

		    BD_scan = hci_basedata_elevation_number();

		}

		if (BD_display_scan == -1) {

		    int iazi;

		    iazi = azimuth+0.5;

		    if (iazi > 359)
		       iazi = 0;

		    value = hci_basedata_interrogate (NULL,
				azimuth,
				range,
				BD_data_type,
				BD_azi_ptr [BD_scan][iazi]);

		} else {

		    value = hci_basedata_interrogate (BD_data,
				(azimuth+0.5),
				range,
				BD_data_type,
				BD_display_scan);

		}

		if (BD_data_type == REFLECTIVITY) {

		    if (value < -32.0) {

		        sprintf (Buf,"No Data");

		    } else {

		        sprintf (Buf,"%d dBZ", (int) value);

		    }

		} else if (BD_data_type == VELOCITY) {

		    if (hci_basedata_velocity_resolution () != DOPPLER_RESOLUTION_HIGH) {

		        if (value > 999) {

		            sprintf (Buf,"RF     ");

	  	        } else if ((value < (-127.0*HCI_MPS_TO_KTS)) ||
				   (value > ( 126.0*HCI_MPS_TO_KTS))) {

		            sprintf (Buf,"No Data");

		        } else {

		            sprintf (Buf,"%d kts", (int) value);

		        }

		    } else {

		        if (value  > 999) {

		            sprintf (Buf,"RF     ");

	  	        } else if ((value < (-63.5*HCI_MPS_TO_KTS)) ||
				   (value > ( 63.0*HCI_MPS_TO_KTS))) {

		            sprintf (Buf,"No Data");

		        } else {

		            sprintf (Buf,"%d kts", (int) value);

		        }

		    }

		} else if (BD_data_type == SPECTRUM_WIDTH) {

		    if (value >999) {

		        sprintf (Buf,"RF     ");

		    } else if (value < 0.0) {

		        sprintf (Buf,"No Data");

		    } else {

			/*  Clip the upper limit at 20.0 */
                        if(value > 20.0) value = 20.0;

		        sprintf (Buf,"%d kts", (int) value);

		    }

		}

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Value_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

	    }

 	} else if (!strcmp (args[0], "up1")) {

/*	If the left mouse button is released, stop the rubber-banding	*
 *	code and unset the button down flag.				*/

	    HCI_LE_log("Base Data left button released");
	    button_down  = 0;

	} else if (!strcmp (args[0], "motion1")) {

/*	If the cursor is being dragged with the left mouse button	*
 *	down, repeatedly draw the sector defined by the point where	*
 *	the button was pressed and where it currently is.  Use the	*
 *	left hand rule for determining the direction in which to draw	*
 *	the sector.							*/


	} else if (!strcmp (args[0], "down2")) {

	    HCI_LE_log("Base Data middle button pushed");

	    if (Display_mode == ZOOM_MODE) {

	        first_pixel = event->xbutton.x;
	        first_scanl = event->xbutton.y;

	        BD_x_offset = (BD_center_pixel-first_pixel)/
				BD_scale_x + BD_x_offset;
	        BD_y_offset = (BD_center_scanl-first_scanl)/
				BD_scale_y + BD_y_offset;

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BD_center_pixel +
				         BD_x_offset*BD_scale_x),
				  (int) (BD_center_scanl +
				         BD_y_offset*BD_scale_y));

		x1 = BD_x_offset;
		y1 = BD_y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > BD_display_range) {

		   BD_x_offset = (BD_x_offset/range)*BD_display_range;
		   BD_y_offset = (BD_y_offset/range)*BD_display_range;

		}

	        hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    }

	} else if (!strcmp (args[0], "down3")) {

	    HCI_LE_log("Base Data right button pushed");

	    if (Display_mode == ZOOM_MODE) {

	        first_pixel = event->xbutton.x;
	        first_scanl = event->xbutton.y;

	        BD_zoom_factor = BD_zoom_factor/2;

	        if (BD_zoom_factor < ZOOM_FACTOR_MIN) {

	            BD_zoom_factor = ZOOM_FACTOR_MIN;
	            return;

	        }

	        BD_x_offset = (BD_center_pixel-first_pixel)/
				BD_scale_x + BD_x_offset;
	        BD_y_offset = (BD_center_scanl-first_scanl)/
				BD_scale_y + BD_y_offset;
	        BD_scale_x  = BD_zoom_factor * (BD_width-COLOR_BAR_WIDTH)/
			        (2*BD_display_range);
	        BD_scale_y  = - BD_zoom_factor * (BD_width-COLOR_BAR_WIDTH)/
			        (2*BD_display_range);

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BD_center_pixel +
				         BD_x_offset*BD_scale_x),
				  (int) (BD_center_scanl +
				         BD_y_offset*BD_scale_y));

		x1 = BD_x_offset;
		y1 = BD_y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > BD_display_range) {

		   BD_x_offset = (BD_x_offset/range)*BD_display_range;
		   BD_y_offset = (BD_y_offset/range)*BD_display_range;

		}

	        hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    }
	}
}

/************************************************************************
 *	Description: This function is used to compute the azimuth	*
 *		     coordinate of a screen pixel element relative to	*
 *		     a reference screen coordinate (radar).		*
 *									*
 *	Input:  second_pixel - pixel coordinate of point		*
 *		second_scanl - scanline coordinate of point		*
 *		first_pixel  - reference pixel coordinate		*
 *		first_scanl  - reference scanline coordinate		*
 *	Output: NONE							*
 *	Return: azimuth angle (degrees)					*
 ************************************************************************/

float
hci_find_azimuth (
int	second_pixel,
int	second_scanl,
int	first_pixel,
int	first_scanl
)
{
	float	x1, y1, xr, yr;
	float	azimuth;

	xr = first_pixel;
	yr = first_scanl;
	x1 = second_pixel;
	y1 = second_scanl;

	if (x1 <= xr) {

	    if (y1 >= yr) {

/*	----------NW Quadrant----------		*/

		if (x1 == xr) {

		    azimuth = 180.0;

		} else {

		    azimuth = 270.0 + atan (((double) (yr-y1))/(xr-x1))/HCI_DEG_TO_RAD;

		}

	    } else {

/*	----------SW Quadrant----------		*/

		if (x1 == xr) {

		    azimuth = 0.0;

		} else {

		    azimuth = 270.0 - atan (((double) (y1-yr))/(xr-x1))/HCI_DEG_TO_RAD;

		}

	    }

	} else {

	    if (y1 <= yr) {

/*	----------NE Quadrant----------		*/

		if (y1 == yr) {

		    azimuth = 90.0;

		} else {

		    azimuth = 90.0 - atan (((double) (yr-y1))/(x1-xr))/HCI_DEG_TO_RAD;

		}

	    } else {

/*	----------SE Quadrant----------		*/

		azimuth = 90.0 + atan (((double) (y1-yr))/(x1-xr))/HCI_DEG_TO_RAD;

	    }

	}

	return azimuth;

}

/************************************************************************
 *	Description: This function is activated when one of the Mode	*
 *		     radio buttons is selected.				*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data - RAW_MODE, ZOOM_MODE			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_display_mode (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	string [32];
	XmString	str;

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only process when button is set.				*/

	if (state->set) {

	    switch ((int) client_data) {

	   	 case RAW_MODE:

		    Display_mode = RAW_MODE;

		    sprintf (string,"Interrogate");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Left_button,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string,"N/A");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Middle_button,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string,"N/A");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Right_button,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    XtVaSetValues (Azran_label,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			NULL);
		    XtVaSetValues (Height_label,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			NULL);
		    XtVaSetValues (Value_label,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			NULL);

		    break;

	        case ZOOM_MODE:

		    Display_mode = ZOOM_MODE;

		    sprintf (string,"Center and Zoom +");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Left_button,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string,"Center Only");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Middle_button,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string,"Center and Zoom -");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Right_button,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    XtVaSetValues (Azran_label,
			XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);
		    XtVaSetValues (Height_label,
			XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);
		    XtVaSetValues (Value_label,
			XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

		    sprintf (string,"               ");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Azran_data,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string,"               ");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Height_data,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string,"       ");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Value_data,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    break;

	    }
	}
}

/************************************************************************
 *	Description: This function is activated when the "Data Range"	*
 *		     button is selected.  The Set Data Range window is	*
 *		     created and displayed.				*
 *									*
 *	Input:  w - Data Range ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_data_range_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	form;
	Widget	rowcol;
	Widget	button;
	Widget	reflectivity_frame;
	Widget	velocity_frame;
	Widget	spectrum_width_frame;
	Widget	label;
	char	buf [16];
	XmString	str;
	float	res;

	void	hci_change_data_range_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_new_data_range_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	HCI_LE_log("Change data range selected");

/*	If the window already exists do nothing and return.		*/

	if( Data_range_dialog != NULL )
	{
          HCI_Shell_popup( Data_range_dialog );
	  return;
	}

	HCI_Shell_init( &Data_range_dialog, "Set Data Range" );

/*	Use a form to manage the contents of the window.		*/

	form = XtVaCreateWidget ("data_range_form",
		xmFormWidgetClass,	Data_range_dialog,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Define a row of buttons to close the window and to apply the	*
 *	changes.							*/

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

        XtAddCallback (button,
                XmNactivateCallback,
                hci_close_data_range_dialog_callback,
                NULL);

	Apply_button = XtVaCreateManagedWidget ("Apply",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	if (Update_flag) {

	    XtVaSetValues (Apply_button,
		XmNsensitive,	True,
		NULL);

	} else {

	    XtVaSetValues (Apply_button,
		XmNsensitive,	False,
		NULL);

	}

	XtAddCallback (Apply_button,
		XmNactivateCallback, hci_change_data_range_callback, NULL);

	XtManageChild (rowcol);

/*	Creatye a set of slider bars to control the reflectivity data	*
 *	range.								*/

	reflectivity_frame = XtVaCreateWidget ("reflectivity_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	reflectivity_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Reflectivity_min_scale = XtVaCreateWidget ("reflectivity_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Reflectivity (dBZ)", 27,
		XmNshowValue,		True,
		XmNminimum,		(int) REFLECTIVITY_MIN,
		XmNmaximum,		(int) REFLECTIVITY_MAX,
		XmNvalue,		(int) Reflectivity_min,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Reflectivity_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 0);

	sprintf (buf,"%d",(int) REFLECTIVITY_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Reflectivity_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) REFLECTIVITY_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Reflectivity_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Reflectivity_min_scale);

	Reflectivity_max_scale = XtVaCreateWidget ("reflectivity_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Reflectivity (dBZ)", 27,
		XmNshowValue,		True,
		XmNminimum,		(int) REFLECTIVITY_MIN,
		XmNmaximum,		(int) REFLECTIVITY_MAX,
		XmNvalue,		(int) Reflectivity_max,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Reflectivity_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 1);

	sprintf (buf,"%d",(int) REFLECTIVITY_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Reflectivity_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) REFLECTIVITY_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Reflectivity_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Reflectivity_max_scale);
	XtManageChild (rowcol);
	XtManageChild (reflectivity_frame);

/*	Creatye a set of slider bars to control the velocity data	*
 *	range.								*/

	velocity_frame = XtVaCreateWidget ("velocity_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		reflectivity_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	velocity_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	if (hci_basedata_velocity_resolution () == DOPPLER_RESOLUTION_HIGH) {

	    res = 0.5;

	} else {

	    res = 1.0;

	}

	if (Velocity_min < VELOCITY_MIN*res) {

	    Velocity_min = VELOCITY_MIN*res;

	}

	Velocity_min_scale = XtVaCreateWidget ("velocity_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Velocity (kts)", 23,
		XmNshowValue,		True,
		XmNminimum,		(int) (VELOCITY_MIN*HCI_MPS_TO_KTS*res),
		XmNmaximum,		(int) (VELOCITY_MAX*HCI_MPS_TO_KTS*res),
		XmNvalue,		(int) (Velocity_min*HCI_MPS_TO_KTS),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Velocity_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 2);

	sprintf (buf,"%d",(int) (VELOCITY_MIN*HCI_MPS_TO_KTS*res));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Velocity_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (VELOCITY_MAX*HCI_MPS_TO_KTS*res));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Velocity_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Velocity_min_scale);

	if (Velocity_max > VELOCITY_MAX*res) {

	    Velocity_max = VELOCITY_MAX*res;

	}

	Velocity_max_scale = XtVaCreateWidget ("velocity_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Velocity (kts)", 23,
		XmNshowValue,		True,
		XmNminimum,		(int) (VELOCITY_MIN*HCI_MPS_TO_KTS*res),
		XmNmaximum,		(int) (VELOCITY_MAX*HCI_MPS_TO_KTS*res),
		XmNvalue,		(int) (Velocity_max*HCI_MPS_TO_KTS),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Velocity_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 3);

	sprintf (buf,"%d",(int) (VELOCITY_MIN*HCI_MPS_TO_KTS*res));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Velocity_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (VELOCITY_MAX*HCI_MPS_TO_KTS*res));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Velocity_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Velocity_max_scale);
	XtManageChild (rowcol);
	XtManageChild (velocity_frame);

/*	Creatye a set of slider bars to control the spectrum width data	*
 *	range.								*/

	spectrum_width_frame = XtVaCreateWidget ("spectrum_width_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		velocity_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	spectrum_width_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Spectrum_width_min_scale = XtVaCreateWidget ("spectrum_width_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Spectrum Width (kts)", 29,
		XmNshowValue,		True,
		XmNminimum,		(int) (SPECTRUM_WIDTH_MIN*HCI_MPS_TO_KTS),
		XmNmaximum,		(int) (SPECTRUM_WIDTH_MAX*HCI_MPS_TO_KTS),
		XmNvalue,		(int) (Spectrum_width_min*HCI_MPS_TO_KTS),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Spectrum_width_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 4);

	sprintf (buf,"%d",(int) (SPECTRUM_WIDTH_MIN*HCI_MPS_TO_KTS));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Spectrum_width_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (SPECTRUM_WIDTH_MAX*HCI_MPS_TO_KTS));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Spectrum_width_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Spectrum_width_min_scale);

	Spectrum_width_max_scale = XtVaCreateWidget ("spectrum_width_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Spectrum Width (kts)", 29,
		XmNshowValue,		True,
		XmNminimum,		(int) (SPECTRUM_WIDTH_MIN*HCI_MPS_TO_KTS),
		XmNmaximum,		(int) (SPECTRUM_WIDTH_MAX*HCI_MPS_TO_KTS),
		XmNvalue,		(int) (Spectrum_width_max*HCI_MPS_TO_KTS),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Spectrum_width_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 5);

	sprintf (buf,"%d",(int) (SPECTRUM_WIDTH_MIN*HCI_MPS_TO_KTS));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Spectrum_width_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (SPECTRUM_WIDTH_MAX*HCI_MPS_TO_KTS));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Spectrum_width_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Spectrum_width_max_scale);
	XtManageChild (rowcol);
	XtManageChild (spectrum_width_frame);

	XtManageChild (form);

	HCI_Shell_start( Data_range_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is activated when one of the		*
 *		     slider bars is moved	.			*
 *									*
 *	Input:  w - slider bar ID					*
 *		client_data -  0 (reflectivity minumum)			*
 *			       1 (reflectivity maximum)			*
 *			       2 (velocity maximum)			*
 *			       3 (velocity maximum)			*
 *			       4 (spectrum width maximum)		*
 *			       5 (spectrum width maximum)		*
 *		call_data - slider widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/


void
hci_new_data_range_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *) call_data;

	switch ((int) client_data) {

	    case 0 :	/*	Reflectivity Min	*/

		Reflectivity_min = (float) cbs->value;

		if (Reflectivity_min >= Reflectivity_max) {

		    Reflectivity_min = Reflectivity_max-1;

		    XtVaSetValues (Reflectivity_min_scale,
			XmNvalue,	(int) Reflectivity_min,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 1 :	/*	Reflectivity Max	*/

		Reflectivity_max = (float) cbs->value;

		if (Reflectivity_max <= Reflectivity_min) {

		    Reflectivity_max = Reflectivity_min+1;

		    XtVaSetValues (Reflectivity_max_scale,
			XmNvalue,	(int) Reflectivity_max,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 2 :	/*	Velociity Min		*/

		Velocity_min = (float) cbs->value/HCI_MPS_TO_KTS;

		if (Velocity_min >= Velocity_max) {

		    Velocity_min = Velocity_max-1;

		    XtVaSetValues (Velocity_min_scale,
			XmNvalue,	(int) (Velocity_min*HCI_MPS_TO_KTS),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 3 :	/*	Velocity Max		*/

		Velocity_max = (float) cbs->value/HCI_MPS_TO_KTS;

		if (Velocity_max <= Velocity_min) {

		    Velocity_max = Velocity_min+1;

		    XtVaSetValues (Velocity_max_scale,
			XmNvalue,	(int) (Velocity_max*HCI_MPS_TO_KTS),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 4 :	/*	Spectrum Width Min	*/

		Spectrum_width_min = (float) cbs->value/HCI_MPS_TO_KTS;

		if (Spectrum_width_min >= Spectrum_width_max) {

		    Spectrum_width_min = Spectrum_width_max-1;

	 	    XtVaSetValues (Spectrum_width_min_scale,
			XmNvalue,	(int) (Spectrum_width_min*HCI_MPS_TO_KTS),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 5 :	/*	Spectrum Width Max	*/

		Spectrum_width_max = (float) cbs->value/HCI_MPS_TO_KTS;

		if (Spectrum_width_max <= Spectrum_width_min) {

		    Spectrum_width_max = Spectrum_width_min+1;

		    XtVaSetValues (Spectrum_width_max_scale,
			XmNvalue,	(int) (Spectrum_width_max*HCI_MPS_TO_KTS),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    default :

		break;

	}
}

/************************************************************************
 *	Description: This function is activated when the "Apply" button	*
 *		     is selected in the Set Data Range window.		*
 *									*
 *	Input:  w - Apply button ID					*
 *		client_data -  unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_change_data_range_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	switch (BD_data_type) {

	    case REFLECTIVITY :

		BD_value_min = Reflectivity_min;
		BD_value_max = Reflectivity_max;
		BD_color_scale  = (BD_value_max-BD_value_min)/
				    (PRODUCT_COLORS-1);
		break;

	    case VELOCITY :

		BD_value_min = Velocity_min;
		BD_value_max = Velocity_max;
		BD_color_scale  = (BD_value_max-BD_value_min)/
				    (PRODUCT_COLORS-2);
		break;

	    case SPECTRUM_WIDTH :

		BD_value_min = Spectrum_width_min;
		BD_value_max = Spectrum_width_max;
		BD_color_scale  = (BD_value_max-BD_value_min)/
				    (PRODUCT_COLORS-2);
		break;

	}

/*	If we are looking at a previously read scan then lets display	*
 *	it.								*/

	if (BD_display_mode == DISPLAY_MODE_STATIC) {

	    hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	} else {

	    hci_basedata_display_clear (BD_display,
			    BD_pixmap,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	    hci_basedata_display_clear (BD_display,
			    BD_window,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	    hci_basedata_color_bar ();

/*	    If we want a grid overlay then display it.			*/

	    if (Grid_overlay_flag) {

	        hci_basedata_overlay_grid ();

	    }

	}

	Update_flag = 0;

	XtVaSetValues (Apply_button,
		XmNsensitive,	False,
		NULL);

}

/************************************************************************
 *	Description: This function is activated when one of the "Map"	*
 *		     radio buttons are selected.			*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data -  Map overlay flag				*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_map_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if the button is set				*/

	if (state->set) {

	    Map_overlay_flag = (int) client_data;

/*	    Update the map overlay toggle flag (0 = off, 1 = on		*/

	    hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/*	The following module toggles the data filter buttons		*/

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     "Filter" radio buttons are selected.		*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data -  filter mode				*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_filter_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if the button is set.				*/

	if (state->set) {

	    Filter_mode = (int) client_data;

/*	    Update the data filter flag (0 = off, 1 = on)		*/

	    hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     "Grid" radio buttons are selected.		*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data -  grid overlay mode			*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_grid_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if the button is set.				*/

	if (state->set) {

	    Grid_overlay_flag = (int) client_data;

/*	    Update the grid overlay toggle flag (0 = off, 1 = on)	*/

	    hci_basedata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function displays a polar grid over a		*
 *		     base data display (PPI).				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_overlay_grid (
)
{
	int	i;
	int	size;
	int	pixel;
	int	scanl;
	int	step;
	char	buf [32];
	float	x, y;
	int	start_range;
	int	stop_range;

	XSetForeground (BD_display, BD_gc,
		hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

/*	Based on the current zoom factor, determine the distancce	*
 *	between polar grid rings (in nautical miles).			*/

	switch (BD_zoom_factor) {

	    case 1 :
	    case 2 :

		step = 30;
		break;

	    case 4 :
	    case 8 :

		step = 10;
		break;

	    default :

		step = 5;
		break;

	}

/*	First display the circles outward from the radar center.	*/

	x = fabs ((double) BD_x_offset);
	y = fabs ((double) BD_y_offset);

	if (x > y) {

	    start_range = x - BD_display_range/BD_zoom_factor;
	    stop_range  = x + BD_display_range/BD_zoom_factor;

	} else {

	    start_range = y - BD_display_range/BD_zoom_factor;
	    stop_range  = y + BD_display_range/BD_zoom_factor;

	}

	start_range = (start_range/step)*step;
	stop_range  = (stop_range/step)*step + step;

	if (start_range < step) {

	    start_range = step;

	}
/*
	for (i=start_range;i<=stop_range;i=i+step) {
*/
	for (i=step;i<=stop_range;i=i+step) {

	    size  = i*BD_scale_x/HCI_KM_TO_NM;

	    pixel = BD_center_pixel + BD_x_offset*BD_scale_x -
		    size;
	    scanl = BD_center_scanl + BD_y_offset*BD_scale_y -
		    size;

	    XDrawArc (BD_display,
		      BD_pixmap,
		      BD_gc,
		      pixel,
		      scanl,
		      size*2,
		      size*2,
		      0,
		      -(360*64));

	    sprintf (buf,"%i nm",i);

	    XDrawString (BD_display,
			 BD_pixmap,
			 BD_gc,
			 (int) (BD_center_pixel +
				BD_x_offset*BD_scale_x -
				4*strlen (buf)),
			 scanl + 4,
			 buf,
			 strlen (buf));
	}

/*	Next display the "spokes" outward from the center.		*/

	for (i=0;i<360;i=i+step) {

	    pixel = (int) (((1+1.0/BD_zoom_factor)*BD_display_range *
		     cos ((double) (i+90)*HCI_DEG_TO_RAD) +
		     BD_x_offset) * BD_scale_x +
		     BD_center_pixel);
	    scanl = (int) (((1+1.0/BD_zoom_factor)*BD_display_range *
		     sin ((double) (i-90)*HCI_DEG_TO_RAD) +
		     BD_y_offset) * BD_scale_y +
		     BD_center_scanl);

	    XDrawLine (BD_display,
		       BD_pixmap,
		       BD_gc,
		       (int) (BD_center_pixel +
			      BD_x_offset*BD_scale_x),
		       (int) (BD_center_scanl +
			      BD_y_offset*BD_scale_y),
		       pixel,
		       scanl);

	}

	hci_basedata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function initilializes the base data scan	*
 *		     lookup table.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_initialize_basedata_scan_table (
)
{
	int	i;
	int	j;
	int	status;
	int	ptr;

	status = hci_basedata_id ();

	if (status <= 0) {

	    HCI_LE_error("Unable to get LB file descriptor: %d", status);
	    HCI_task_exit (HCI_EXIT_SUCCESS);

	}

/*	Initialize the scan table with all zeros.			*/

	for (i=0;i<MAX_ELEVATION_SCANS;i++) {

	    for (j=0;j<MAX_AZIMUTHS;j++) {

		BD_scan_ptr [i][j] = 0;
		BD_azi_ptr  [i][j] = 0;

	    }
	}

/*	We start by moving the message pointer to the beginning of the	*
 *	basedata LB.							*/

	status = hci_basedata_seek (LB_FIRST);

	if (status != LB_SUCCESS) {

	    HCI_LE_error("LB seek failed during initialization: %d", status);
	    HCI_task_exit (HCI_EXIT_SUCCESS);

	}

/*	Now lets read the entire LB and save the message ID of each	*
 *	radial in the current virtual volume scan.			*/

	status = 1;

	while ((status > 0)           ||
	       (status == LB_EXPIRED) ||
	       (status == LB_BUF_TOO_SMALL)) {

	 	/*  Only partially read base data when in low bandwidth mode */
	 	if (HCI_is_low_bandwidth())
		{
		  status = hci_basedata_read_radial (LB_NEXT, HCI_BASEDATA_PARTIAL_READ );
		  if( status == LB_EXPIRED )
		  {
		    status = hci_basedata_read_radial (LB_LATEST, HCI_BASEDATA_PARTIAL_READ );
		  }
		}
		else 	
		{
		  status = hci_basedata_read_radial (LB_NEXT, HCI_BASEDATA_COMPLETE_READ );
		  if( status == LB_EXPIRED )
		  {
		    status = hci_basedata_read_radial (LB_LATEST, HCI_BASEDATA_COMPLETE_READ );
		  }
		}

	    if ((status > 0) ||
		(status == LB_BUF_TOO_SMALL)) {

		ptr = hci_basedata_msgid ();

		BD_scan_ptr [hci_basedata_elevation_number()][hci_basedata_azimuth_number()] = ptr;
		BD_azi_ptr  [hci_basedata_elevation_number()][(int) (hci_basedata_azimuth()+0.5)]  = ptr;

	    }
	}

}

/************************************************************************
 *	Description: This function is used as a timer to update the	*
 *		     basedata display at a defined interval.  This is	*
 *		     especially needed for dynamic display mode.	*
 *									*
 *	Input:  w - parent widget of timer				*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
	int	ptr;
	int	status;
	int	update_flag;
static	int     old_elevation = -99;
static	int	old_vcp = 0;
	int	vcp;
	int	waveform;
	int	new_vel_resolution;

/*	Check to see if Scan Info has been updated. */

	if( Scan_info_update_flag )
	{
	  Scan_info_update_flag = 0;

	  hci_set_scan_info_flag (1);

	  if (hci_get_scan_vcp_number () != Current_vcp) {

		New_volume  = 1;
		Current_vcp = hci_get_scan_vcp_number ();

	  }
	}

/*	Now lets read the new LB msgs and save the message ID of each	*
 *	radial in the current virtual volume scan.			*/

	status      = 1;
	update_flag = 0;

/*	The following check is needed to ensure that the basedata LB	*
 *	isn't read for new messages while the LB pointer is set to	*
 *	an earlier scan for a static display.				*/

	if (hci_basedata_get_lock_state ()) {

	    return;

	}

	hci_basedata_set_lock_state (1);

/*	Check to see if the VCP changed.  If so, change the buttons	*
 *	to reflect the new elevation cuts available.			*/

	if (New_volume) {

	    vcp = hci_get_scan_vcp_number ();

	    if (vcp != old_vcp) {

		old_vcp = vcp;
		hci_basedata_elevation_buttons ();
		HCI_LE_status("New VCP %d detected", vcp);

	    }

	    New_volume = 0;

	}

	while ((status > 0) ||
	       (status == LB_BUF_TOO_SMALL)) {

/*	    Only partially read base data when in low bandwidth mode	*/

 	    if (HCI_is_low_bandwidth())
	    {
	      status = hci_basedata_read_radial (LB_NEXT, HCI_BASEDATA_PARTIAL_READ );
	      if( status == LB_EXPIRED )
	      {
	        status = hci_basedata_read_radial (LB_LATEST, HCI_BASEDATA_PARTIAL_READ );
	      }
	    }
	    else 	
	    {
	      status = hci_basedata_read_radial (LB_NEXT, HCI_BASEDATA_COMPLETE_READ );
	      if( status == LB_EXPIRED )
	      {
	        status = hci_basedata_read_radial (LB_LATEST, HCI_BASEDATA_COMPLETE_READ );
	      }
	    }

/*	    If a new message was read then process it.			*/

	    if ((status > 0) ||
		(status == LB_BUF_TOO_SMALL)) {

/*		Get the message id and add it to the azran pointer	*
 *		so we can interrogate the displayed data and locate	*
 *		the corresponding data.					*/

		ptr = hci_basedata_msgid ();

		BD_scan_ptr [hci_basedata_elevation_number()][hci_basedata_azimuth_number()] = ptr;
		BD_azi_ptr  [hci_basedata_elevation_number()][(int) hci_basedata_azimuth()]  = ptr;

/*		If we are dynamically displaying radial data then check	*
 *		to see if the elevation changed before displaying it.	*
 *		If it did change, then clear the display and update the	*
 *		window scan/date/time title information.		*/

		if (BD_display_mode == DISPLAY_MODE_DYNAMIC) {

		    Elevation_number = hci_basedata_elevation_number ();

		    if ((Elevation_number != old_elevation) || Field_changed) {

		      if( BD_data_type == VELOCITY )
		      {
		        new_vel_resolution = hci_basedata_velocity_resolution();
			if( new_vel_resolution != Prev_vel_resolution )
			{
			  Prev_vel_resolution = new_vel_resolution;
			  if( new_vel_resolution == DOPPLER_RESOLUTION_HIGH )
			  {
			    Velocity_min = VELOCITY_MIN/2;
			    Velocity_max = VELOCITY_MAX/2; 
			  }
			  else
			  {
			    Velocity_min = VELOCITY_MIN;
			    Velocity_max = VELOCITY_MAX; 
			  }
			  BD_value_max = Velocity_max;
			  BD_value_min = Velocity_min;
			  hci_basedata_color_bar();
			}
		      }

			/* Clear previous pixmap. */

			hci_basedata_display_clear (BD_display,
			    BD_pixmap,
			    BD_gc,
			    BD_width,
			    BD_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

			Field_changed = 0;

			draw_dynamic_label(1);

		        old_elevation = Elevation_number;

                        waveform = ORPGVCP_get_waveform( hci_get_scan_vcp_number (),
                                                         old_elevation-1 );

			if ((BD_data_type == REFLECTIVITY) &&
			    (waveform != VCP_WAVEFORM_CD) ){

			    hci_basedata_display_clear (BD_display,
				    BD_pixmap,
				    BD_gc,
				    BD_width,
				    BD_height,
				    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

			} else if ((BD_data_type != REFLECTIVITY) &&
			           (waveform != VCP_WAVEFORM_CS) ){

			    hci_basedata_display_clear (BD_display,
				    BD_pixmap,
				    BD_gc,
				    BD_width,
				    BD_height,
				    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

			}

/*	      	    If we want a grid overlay then display it.		*/

		        if (Grid_overlay_flag) {

			    hci_basedata_overlay_grid ();

		        }

		    }

		    if (BD_data_type == REFLECTIVITY) {

			hci_basedata_display (NULL,
				      BD_display,
				      BD_pixmap,
				      BD_gc,
				      BD_x_offset,
				      BD_y_offset,
				      BD_scale_x,
				      BD_scale_y,
				      BD_center_pixel,
				      BD_center_scanl,
				      BD_value_min,
				      BD_value_max,
				      BD_color,
				      BD_data_type,
				      Filter_mode);

		    } else {

			hci_basedata_display (NULL,
				      BD_display,
				      BD_pixmap,
				      BD_gc,
				      BD_x_offset,
				      BD_y_offset,
				      BD_scale_x,
				      BD_scale_y,
				      BD_center_pixel,
				      BD_center_scanl,
				      BD_value_min*HCI_MPS_TO_KTS,
				      BD_value_max*HCI_MPS_TO_KTS,
				      BD_color,
				      BD_data_type,
				      Filter_mode);

		    }

		    update_flag = 1;

		}

	    } else if (Field_changed) {

		Field_changed = 0;
		if( BD_display_mode == DISPLAY_MODE_DYNAMIC ) 
		{
		  draw_dynamic_label(0);
		}
	    }
	}

	hci_basedata_set_lock_state (0);

	if (update_flag) {

	        hci_basedata_expose_callback ((Widget) NULL,
				(XtPointer) NULL,
				(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the RPG Base Data Display	*
 *		     window.						*
 *									*
 *	Input:  w - Close button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Base Data window Close pushed");
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This function handles all scan info events.  It	*
 *		     checks for new scan events and sets the current	*
 *		     VCP number and update flag.  This is needed so	*
 *		     the elevation buttons will be updated when the	*
 *		     VCP changes.					*
 *									*
 *	Input:  evtcd - event code					*
 *		info  - event data					*
 *		msglen - length (bytes) of event data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_scan_info_update (
en_t	evtcd,
void	*info,
size_t	msglen
)
{
	orpgevt_scan_info_t	*scan_info = (orpgevt_scan_info_t *) info;

	if ((scan_info->key == ORPGEVT_BEGIN_VOL) ||
	    (scan_info->key == ORPGEVT_BEGIN_ELEV)) {

	    Scan_info_update_flag = 1;

	}
}

/************************************************************************
 *      Description: This function is activated when the "Close"        *
 *                   button is selected in the Set Data Range window.   *
 *                                                                      *
 *      Input:  w - Close button ID                                     *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_close_data_range_dialog_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        HCI_Shell_popdown(Data_range_dialog);
}

void
draw_dynamic_label( int flag )
{
  int		start_date = 0;
  XmString	str;

  static int	yr = 0, mo = 0, dy = 0, hr = 0, min = 0, sec = 0;

  if( flag )
  {
    /* Is this a super resolution cut? */
    if( ORPGVST_is_rpg_elev_superres( ORPGVST_get_index(Elevation_number-1) ) )
    {
      SuperRes_cut_flag = 1;
    }
    else
    {
      SuperRes_cut_flag = 0;
    }

    start_date = ORPGVST_get_volume_date();
    Start_time = ORPGVST_get_volume_time() / 1000;
    Start_time += ((start_date-1)*86400);

    if( Start_time > 0 )
    {
      unix_time( &Start_time, &yr, &mo, &dy, &hr, &min, &sec );

      if( yr >= 2000 ){ yr -= 2000; }
      else{ yr -= 1900; }
    }

    Scan_elevation = hci_basedata_target_elevation ();
  }

  if( SuperRes_cut_flag )
  {
    sprintf( Buf,"%s: %s %5.1f %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d",
             BD_current_field, "Elevation", Scan_elevation, "SR  Date/Time:",
             mo, dy, yr, hr, min, sec );
  }
  else
  {
    sprintf( Buf,"%s: %s %5.1f     %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d",
             BD_current_field, "Elevation", Scan_elevation, "Date/Time:",
             mo, dy, yr, hr, min, sec );
  }

  str = XmStringCreateLocalized( Buf );
  XtVaSetValues( BD_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

void
draw_static_label(int vol_num)
{
  float		angs[MAX_ELEVATION_SCANS];
  int		num_angs = MAX_ELEVATION_SCANS;
  int		vcp = 0;
  XmString	str;

  static int	elev_date = 0;
  static int	yr = 0, mo = 0, dy = 0, hr = 0, min = 0, sec = 0;
  static time_t	start_time = 0;

  if( vol_num > -1 )
  {
    /* call ORPGVCP function to get all elev angs for this vcp.  
       Use the RDA VCP definition.  */
    vcp = hci_get_scan_vcp_number();
    vcp |= (ORPGVCP_RDAVCP | ORPGVCP_VOLNUM);
    num_angs = ORPGVCP_get_all_elevation_angles( vcp, num_angs, angs, vol_num );
    ORPGBDR_get_start_date_and_time (BASEDATA, vol_num,
                                     &elev_date, &start_time );

    start_time += (elev_date-1)*86400;

    if( start_time > 0 )
    {
      unix_time( &start_time, &yr, &mo, &dy, &hr, &min, &sec );

      if( yr >= 2000 ){ yr -= 2000; }
      else{ yr -= 1900; }
    }

    /* Set the scan elev to the proper angle chosen by the user */
    Scan_elevation = angs[BD_display_scan - 1];
  }

  sprintf( Buf,"%s: %s %5.1f     %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d",
           BD_current_field, "Elevation", Scan_elevation, "Date/Time:",
           mo, dy, yr, hr, min, sec );

  str = XmStringCreateLocalized( Buf );
  XtVaSetValues( BD_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}
