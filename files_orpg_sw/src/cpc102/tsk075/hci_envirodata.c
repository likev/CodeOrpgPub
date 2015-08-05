/************************************************************************
 *									*
 *	Module:  hci_envirodata.c					*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      to open a window for displaying environmental	*
 *		      data.  It was repurposed from the basedata 	*
 *		      display software in the RPG's HCI.                *
 *									*
 *************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/20 15:05:31 $
 * $Id: hci_envirodata.c,v 1.4 2014/03/20 15:05:31 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_envirodata.h>
#include <hci_scan_info.h>
#include <rpgcs_model_data.h>
#include <rpgcs_latlon.h>
#include <rpgcs_time_funcs.h>

/*	Macros.								*/

#define	MODEL_DATA_WIDTH	  550 /* default width of window */
#define	MODEL_DATA_HEIGHT	  470 /* default height of window */
#define	RAW_MODE		    0 /* interrogation mode (only mode implemented at this time) */
#define MAX_LEVEL_BUTTONS          32 /* max model level buttons */
#define COLOR_BAR_WIDTH		   50 /* width of color bar */
#define COLOR_BOX_WIDTH		   25 /* width of color box */
#define TICK_MARK_LENGTH	   15 /* length of data tick mark */
#define	REFLECTIVITY_PRODUCT	   19 /* Using reflectivity product for colors for convenience */
#define	DISPLAY_MODE_STATIC	    0 /* Only mode implemented at this time */
#define PASCAL_TO_MB             0.01 /* Convert Pascal units to millibars */

/*	Global widget variables.				*/

Widget	Zoom_button       = (Widget) NULL;
Widget	Raw_button        = (Widget) NULL;
Widget	T_button          = (Widget) NULL;
Widget	RH_button         = (Widget) NULL;
Widget	SP_button         = (Widget) NULL;
Widget	Map_off_button    = (Widget) NULL;
Widget	Map_on_button     = (Widget) NULL;
Widget	Filter_off_button = (Widget) NULL;
Widget	Filter_on_button  = (Widget) NULL;
Widget	Grid_off_button   = (Widget) NULL;
Widget	Grid_on_button    = (Widget) NULL;
Widget	Model_label       = (Widget) NULL;
Widget	Apply_button      = (Widget) NULL;
Widget	Button [MAX_LEVEL_BUTTONS];
Widget	Azran_label;
Widget	Height_label;
Widget	Value_label;
Widget	Azran_data;
Widget	Height_data;
Widget	Value_data;
Widget	Left_button;
Widget	Middle_button;
Widget	Right_button;

Widget	Data_range_dialog      = (Widget) NULL;
Widget	Temperature_min_scale  = (Widget) NULL;
Widget	Temperature_max_scale  = (Widget) NULL;
Widget	Rel_humidity_min_scale = (Widget) NULL;
Widget	Rel_humidity_max_scale = (Widget) NULL;
Widget	Sfc_pressure_min_scale = (Widget) NULL;
Widget	Sfc_pressure_max_scale = (Widget) NULL;

char	Buf [100]; /* common buffer for string functions. */

int	Display_mode     = RAW_MODE;         /* cursor mode inside display area */
float	Temperature_min  = TEMPERATURE_MIN;  /* Minimum temperature to display */
float	Temperature_max  = TEMPERATURE_MAX;  /* Maximum temperature to display */
float	Rel_humidity_min = REL_HUMIDITY_MIN; /* Minimum relative humidity to display */
float	Rel_humidity_max = REL_HUMIDITY_MAX; /* Maximum relative humidity to display */
float	Sfc_pressure_min = SFC_PRESSURE_MIN; /* Minimum surface pressure to display */
float	Sfc_pressure_max = SFC_PRESSURE_MAX; /* Maximum surface pressure to display */

int	Grid_overlay_flag= 1; /* Used to automatically display polar	*
			       * grid when model data updated.   	*/
int	Map_overlay_flag = 0; /* Used to automatically display maps	*
		               * when model data updated.		*/
int	Field_changed    = 0; /* Used to flag when model field changed by operator */
int	Filter_mode      = 0; /* Used to control whether data outside   *
			       * data range is not displayed (0) or set *
			       * to min/max color (1).		        */

/*	Translation table for base data window.  We need a mechanism	*
 *	to capture mouse events for interrogation.      		*/

String	Translations =
	"<Btn1Down>:  hci_envirodata_draw(down1)   ManagerGadgetArm() \n\
	<Btn1Up>:     hci_envirodata_draw(up1)     ManagerGadgetActivate() \n\
	<Btn1Motion>: hci_envirodata_draw(motion1) ManagerGadgetButtonMotion() \n\
	<Btn2Down>:   hci_envirodata_draw(down2)   ManagerGadgetArm() \n\
	<Btn3Down>:   hci_envirodata_draw(down3)   ManagerGadgetArm()";

/*	Global variables						*/

char		ED_current_field [32]; /* Current displayed model field */
Display		*ED_display;           /* Display info */
Widget		Top_widget;            /* Top level window */
Widget		ED_label;              /* Level/Time label at top of window */
Dimension	ED_width;              /* width of display region */
Dimension	ED_height;             /* height of display region */
Dimension	ED_depth;              /* depth (bits) of display */
Widget		ED_buttons_rowcol;     /* Level buttons manager */
Widget		ED_draw_widget;        /* drawing area widget for base data */
Window		ED_window;             /* visible Drawable */
Pixmap		ED_pixmap;             /* invisible Drawable */
GC		ED_gc;                 /* graphics context */
int		ED_center_pixel;       /* center pixel of display region */
int		ED_center_scanl;       /* center scanline of display region */
int             ED_pixel_size;         /* Pixel size of current model grid box */
int             ED_scanl_size;         /* Scanl size of current model grid box */
float		ED_scale_x;            /* pixel/km in X direction */
float		ED_scale_y;            /* pixel/km in Y direction */
float		ED_value_max;          /* maximum value to display for active moment */
float		ED_value_min;          /* minimum value to display for active moment */
float		ED_color_scale;        /* scale for converting data to color index */
float		ED_display_range = MAX_RANGE; /* maximum range */
int		ED_display_level = 0;  /* Model level number */
float		ED_x_offset;           /* X offset of radar from region center */
float		ED_y_offset;           /* Y offset of radar from region center */
int             ED_rows;               /* Number of current model data grid rows */
int             ED_columns;            /* Number of current model data grid columns */
int		ED_data_type = TEMPERATURE;   /* active data type */
int		ED_zoom_factor = 1;    /* current zoom factor (zoom not currently implemented) */
float		*ED_data  = (float *) NULL;   /* Pointer to radial data */
int		ED_color [PRODUCT_COLORS];    /* color lookup table */
int		ED_levels = 18;        /* Default number of levels in model */
int             ED_model;              /* Model name code from rpgcs_model_data.h */
int             ED_model_x_offset;     /* Helps center the model data, drawn from upper left corner */
int             ED_model_y_offset;     /* Helps center the model data, drawn from upper left corner */
int		Update_flag = 0;       /* Data update flag */

void	hci_display_selected_level    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_expose_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_close_callback  (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_data_range_callback        (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_filter_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_grid_callback   (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_map_callback    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_temperature     (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_rel_humidity    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_sfc_pressure    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	hci_envirodata_draw (Widget w, XEvent *event, String *args,
			int	*num_args);
void	hci_close_data_range_dialog_callback (Widget w, XtPointer client_data,
			XtPointer call_data);

float	hci_find_azimuth (int pixel1, int scanl1, int pixel2, int scanl2);

void	hci_envirodata_display (float *data_ptr,
			Display *display, Drawable window,
			GC gc, float x_off, float y_off, float scale_x,
			float scale_y, int center_pixel, int center_scanl,
			float min, float max, int *color, int field,
			int filter, int row, int columns, int pixel, int scanl);

void	hci_envirodata_display_clear (Display *display, Drawable window,
			GC gc, int width, int height, int color);

void	hci_envirodata_level_buttons     (int number_levels,
                                          int rows,
                                          int columns,
                                          RPGCS_model_grid_params_t *params[]);
void	hci_envirodata_color_bar             ();
void	hci_envirodata_overlay_grid          ();
void	timer_proc ();
void	draw_static_label(time_t, time_t, float);
static void *Get_grids( int model, char *buf, char *field );

/************************************************************************
 *	Description: This is the main function for the HCI Environmental*
 *		     Model Data Viewer tool.				*
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
        XtPointer unused1 = NULL;
        XtPointer unused2 = NULL;

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_ENVIROVIEWER_TASK );

	Top_widget  = HCI_get_top_widget();
	ED_display  = HCI_get_display();
	ED_depth    = XDefaultDepth (ED_display, DefaultScreen(ED_display));

        hci_envirodata(Top_widget, unused1, unused2);

/*	Start HCI loop.							*/

	HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );
        return 0;
}

/************************************************************************
 *	Description: This function opens the HCI Environmental Model	*
 *		     Data display window.				*
 *									*
 *	Input:  w - ID of "View Model" button				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
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
	Widget		field_rowcol;
	Widget		field_list;
	Widget		filter_list;
	Widget		grid_list;
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
	
/*	Make "Temperature" the default field to display.		*/

	strcpy (ED_current_field,"  Temperature");

	Top_widget = HCI_get_top_widget();
	ED_display = HCI_get_display();
	ED_depth   = XDefaultDepth (ED_display, DefaultScreen (ED_display));

/*	Initialize the read-only color database for widgets and		*
 *	drawing colors.	Using REFLECTIVITY_PRODUCT colors for lack	*
 *      of anything better.                                             */

	hci_initialize_product_colors (ED_display, HCI_get_colormap());
	hci_get_product_colors (REFLECTIVITY_PRODUCT, ED_color);

/*	Set various window properties.					*/

	XtVaSetValues (Top_widget,
		XmNminWidth,		250,
		XmNminHeight,		250,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Make the initial size of the model data display window something*
 *	reasonable. Define a minimum size ot hold all the widgets	*/

	ED_width  = MODEL_DATA_WIDTH; 
	ED_height = MODEL_DATA_HEIGHT;

/*	Create the form widget which will manage the drawing_area and	*
 *	row_column widgets.						*/

	form = XtVaCreateWidget ("form",
		xmFormWidgetClass,		Top_widget,
		NULL);

	ED_label = XtVaCreateManagedWidget ("       ",
		xmLabelWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
/* XmNrecomputeSize to False prevents widget from resizing to original size.*/
                XmNrecomputeSize, False,
		NULL);

/*	Use another form widget to manage all of the control buttons	*
 *	on the left side of the envirodata display window.		*/

	control_form = XtVaCreateWidget ("control_form",
		xmFormWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			ED_label,
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
		XmNactivateCallback, hci_envirodata_close_callback,
		NULL);

	XtManageChild (close_rowcol);

/*	Display a label at the top of the control form that contains	*
 *	the model name.	                                 		*/

	Model_label = XtVaCreateManagedWidget ("     ",
		xmLabelWidgetClass,	control_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LARGE),
		NULL);

	frame = XtVaCreateManagedWidget ("level_frame",
		xmFrameWidgetClass,	control_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Model_label,
		NULL);

/*	Create the row_column widget for the model level buttons to 	*
 *	be placed along the left side of the form.  The buttons are	*
 *	displayed with level nummber as labels in four columns.		*/

	ED_buttons_rowcol = XtVaCreateWidget ("envirodata_rowcol",
		xmRowColumnWidgetClass,		frame,
		XmNorientation,			XmVERTICAL,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,			XmPACK_COLUMN,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNnumColumns,			4,
		NULL);

/*	Create the pushbuttons for the model level selections.  	*/

	for (i=0;i<MAX_LEVEL_BUTTONS;i++) {

	    Button [i] = XtVaCreateManagedWidget ("    ",
		xmPushButtonWidgetClass,	ED_buttons_rowcol,
		XmNforeground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNuserData,			(XtPointer) (i),
		XmNalignment,			XmALIGNMENT_CENTER,
		NULL);

	    XtAddCallback (Button [i],
		XmNactivateCallback, hci_display_selected_level,
		(XtPointer) (i));

	}

/*	Use the function hci_envirodata_level_buttons () to define	*
 *	the model name and levels and create the level buttons.        	*/

	hci_envirodata_level_buttons (ED_levels, 0, 0, NULL);

	XtManageChild (ED_buttons_rowcol);

	label = XtVaCreateManagedWidget ("space",
		xmLabelWidgetClass,		control_form,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			ED_buttons_rowcol,
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
		XmNvalueChangedCallback, hci_envirodata_filter_callback,
		(XtPointer) 0);

	Filter_off_button = XtVaCreateManagedWidget ("Off",
		xmToggleButtonWidgetClass,	filter_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Filter_off_button,
		XmNvalueChangedCallback, hci_envirodata_filter_callback,
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

	XtVaCreateManagedWidget ("Field:",
		xmLabelWidgetClass,	field_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	field_list = XmCreateRadioBox (field_rowcol,
		"field_list", arg, n);

	T_button = XtVaCreateManagedWidget ("T ",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNset,				True,
		NULL);

	XtAddCallback (T_button,
		XmNvalueChangedCallback, hci_envirodata_temperature, NULL);

	RH_button = XtVaCreateManagedWidget ("RH ",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (RH_button,
		XmNvalueChangedCallback, hci_envirodata_rel_humidity, NULL);

	SP_button = XtVaCreateManagedWidget ("SP",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (SP_button,
		XmNvalueChangedCallback, hci_envirodata_sfc_pressure, NULL);

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
		XmNvalueChangedCallback, hci_envirodata_grid_callback,
		(XtPointer) 1);

	Grid_off_button = XtVaCreateManagedWidget ("Off",
		xmToggleButtonWidgetClass,	grid_list,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		XmNselectColor,			hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Grid_off_button,
		XmNvalueChangedCallback, hci_envirodata_grid_callback,
		(XtPointer) 0);

	XtManageChild (grid_list);
	XtManageChild (grid_rowcol);
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

	Height_label = XtVaCreateManagedWidget ("Geoptl. Hgt.: ",
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
 *	the model data.  It will occupy the upper right portion		*
 *	of the form.							*/

	actions.string = "hci_envirodata_draw";
	actions.proc   = (XtActionProc) hci_envirodata_draw;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	ED_draw_widget = XtVaCreateWidget ("envirodata_drawing_area",
		xmDrawingAreaWidgetClass,	form,
		XmNwidth,			ED_width,
		XmNheight,			ED_height,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			ED_label,
		XmNleftAttachment,		XmATTACH_WIDGET,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_WIDGET,
		XmNleftWidget,			ED_buttons_rowcol,
		XmNbottomWidget,		properties_form,
		XmNtranslations,		XtParseTranslationTable (Translations),
		NULL);

/*	Add an expose callback for the drawing_area in order to allow	*
 *	holes to be filled in the display when other windows are moved	*
 *	across it.							*/

	XtAddCallback (ED_draw_widget,
		XmNexposeCallback, hci_envirodata_expose_callback, NULL);

/*	Permit the user to resize the model data display window.	*/

/*	XtAddCallback (ED_draw_widget,
		XmNresizeCallback, hci_envirodata_resize_callback, NULL);*/

	XtManageChild (ED_draw_widget);
	XtManageChild (form);

	XtRealizeWidget (Top_widget);

/*	Define the various window variables to be used as arguments in 	*
 *	the various Xlib and Xt calls.					*/

	ED_window  = XtWindow      (ED_draw_widget);
	ED_pixmap  = XCreatePixmap (ED_display,
		ED_window,
		ED_width,
		ED_height,
		ED_depth);

/*	Define the Graphics Context to be used in drawing data		*/

	gcv.foreground = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
	gcv.background = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
	gcv.graphics_exposures = FALSE;

	ED_gc = XCreateGC (ED_display,
	      ED_window,
	      GCBackground | GCForeground | GCGraphicsExposures, &gcv);

	XSetFont (ED_display, ED_gc, hci_get_font (LIST));

	XtVaSetValues (ED_draw_widget,
		XmNbackground,	hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
		NULL);

/*	Clear the data display portion of the window by filling it with	*
 *	the background color.						*/

	XSetForeground (ED_display, ED_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (ED_display,
		ED_pixmap,
		ED_gc,
		0, 0,
		ED_width,
		ED_height);

/*	Define the data display window properties.			*/

	ED_center_pixel = (ED_width-COLOR_BAR_WIDTH)/2;
	ED_center_scanl = ED_height/2;
	ED_scale_x      = ED_zoom_factor * (ED_width-COLOR_BAR_WIDTH)/
			   (2.0*ED_display_range);
	ED_scale_y      = -ED_scale_x;

	ED_value_max    = TEMPERATURE_MAX;
	ED_value_min    = TEMPERATURE_MIN;
	ED_color_scale  = (ED_value_max-ED_value_min)/(PRODUCT_COLORS-1);

	XtRealizeWidget (Top_widget);

	hci_envirodata_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function handles the main window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the model data pixmap to the	*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Set the clip region to the full window, update the window by	*
 *	copying the model data pixmap to it to fill in any holes left	*
 *	by any previously overlain window and the restoring the clip	*
 *	region to the model data display region only.			*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = ED_width;
	clip_rectangle.height  = ED_height;

	XSetClipRectangles (ED_display,
			    ED_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XCopyArea (ED_display,
		   ED_pixmap,
		   ED_window,
		   ED_gc,
		   0,
		   0,
		   ED_width,
		   ED_height,
		   0,
		   0);

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = ED_width-COLOR_BAR_WIDTH;
	clip_rectangle.height  = ED_height;

	XSetClipRectangles (ED_display,
			    ED_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);
}

/************************************************************************
 *	Description: This function handles the main window resize	*
 *		     callback by redrawing all model data window	*
 *		     components.					*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_resize_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Get the new size of the model data window.			*/

	if ((ED_draw_widget == (Widget) NULL)    ||
	    (ED_display     == (Display *) NULL) ||
	    (ED_window      == (Window) NULL)    ||
	    (ED_pixmap      == (Pixmap) NULL)    ||
	    (ED_gc          == (GC) NULL)) {

	    return;

	}
	HCI_LE_log("resize callback:  old size = %d %d", ED_width, ED_height);
	XtVaGetValues (ED_draw_widget,
		XmNwidth,	&ED_width,
		XmNheight,	&ED_height,
		NULL);
	HCI_LE_log("resize callback:  new size = %d %d\n", ED_width, ED_height);
/*	Destroy the old pixmap since the size has changed and create	*
 *	create a new one.						*/

	if (ED_pixmap != (Window) NULL) {

	    XFreePixmap (ED_display, ED_pixmap);

	}

	ED_pixmap = XCreatePixmap (ED_display,
		ED_window,
		ED_width,
		ED_height,
		ED_depth);

/*	Recompute the scale factors (in pixels/km) and a new window	*
 *	center pixel/scanline coordinate.				*/

	ED_scale_x      = ED_zoom_factor * (ED_width-COLOR_BAR_WIDTH)/
			   (2*ED_display_range);
	ED_scale_y	= -ED_scale_x;
	ED_center_pixel = (ED_width-COLOR_BAR_WIDTH) / 2;
	ED_center_scanl = ED_height / 2;

/*	Set the clip region to the entire window so we can create a	*
 *	color bar along the right size.					*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = ED_width;
	clip_rectangle.height  = ED_height;

	XSetClipRectangles (ED_display,
			    ED_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XSetForeground (ED_display, ED_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (ED_display,
			ED_pixmap,
			ED_gc,
			0, 0,
			ED_width,
			ED_height);

	hci_envirodata_color_bar ();

/*	Restore the clip window to the model display area only.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = ED_width-COLOR_BAR_WIDTH;
	clip_rectangle.height  = ED_height;

	XSetClipRectangles (ED_display,
			    ED_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	If we are looking at previously displayed data then redisplay it*/

	hci_display_selected_level (Button [ED_display_level],
	                           (XtPointer) ED_display_level,
			           (XtPointer) NULL);

/*	Call the expose callback to write the model data pixmap to the	*
 *	base data window.						*/

	hci_envirodata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function draws a color bar along the right	*
 *		     side of the model data window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_color_bar ()
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
	clip_rectangle.width   = ED_width;
	clip_rectangle.height  = ED_height;

	XSetClipRectangles (ED_display,
			    ED_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	Clear an area to the right of the model data display to place	*
 *	the color bar.							*/

	XSetForeground (ED_display, ED_gc,
		hci_get_read_color (BACKGROUND_COLOR1));

	XFillRectangle (ED_display,
			ED_window,
			ED_gc,
			(int) (ED_width-COLOR_BAR_WIDTH),
			0,
			COLOR_BAR_WIDTH,
			ED_height);

	XFillRectangle (ED_display,
			ED_pixmap,
			ED_gc,
			(int) (ED_width-COLOR_BAR_WIDTH),
			0,
			COLOR_BAR_WIDTH,
			ED_height);

/*	Define the width of each color bar box and the height scale	*/

	width = 10;

        xheight = ED_height/(2.0*(PRODUCT_COLORS-1));
	step = 1;

	height  = xheight+1;

/*	For each color element, create a box and fill it with the color	*/

	for (i=step;i<PRODUCT_COLORS;i=i+step) {

		scanl = ED_height/4 + xheight*(i/step-1);

		XSetForeground (ED_display, ED_gc,
			ED_color [i]);

		XFillRectangle (ED_display,
			ED_window,
			ED_gc,
			(int) (ED_width-TICK_MARK_LENGTH),
			scanl,
			width,
			height);

		XFillRectangle (ED_display,
			ED_pixmap,
			ED_gc,
			(int) (ED_width-TICK_MARK_LENGTH),
			scanl,
			width,
			height);

	}

	XSetForeground (ED_display, ED_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XDrawRectangle (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-TICK_MARK_LENGTH),
		(int) (ED_height/4),
		width,
		(int) (ED_height/2));

	XDrawRectangle (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-TICK_MARK_LENGTH),
		(int) (ED_height/4),
		width,
		(int) (ED_height/2));


/*	Draw text labels to the left of the color bar for the minimum,	*
 *	middle, and maximum values.					*/

/*      Position the min value */
	scanl = ED_height/4;

/*      Format the min value */

	if (ED_value_min < 0.0) {

	   sprintf (text,"%d",(int) (ED_value_min-0.5));

	} else {

	   sprintf (text,"%d",(int) (ED_value_min+0.5));

	}

	XDrawString (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-COLOR_BAR_WIDTH),
		scanl+5,
		text,
		strlen (text));

	XDrawString (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-COLOR_BAR_WIDTH),
		scanl+5,
		text,
		strlen (text));

	XDrawLine (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (ED_width-TICK_MARK_LENGTH),
		scanl);

	XDrawLine (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (ED_width-TICK_MARK_LENGTH),
		scanl);

/*      Format the max value */

	if (ED_value_max < 0.0) {

	    sprintf (text,"%d",(int) (ED_value_max-0.5));

	} else {

            sprintf (text,"%d",(int) (ED_value_max+0.5));

	}

/*      Position the max value */

	scanl = ED_height/4 + xheight*(PRODUCT_COLORS-1);

	XDrawString (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	XDrawString (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	XDrawLine (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (ED_width-TICK_MARK_LENGTH),
		scanl);

	XDrawLine (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (ED_width-TICK_MARK_LENGTH),
		scanl);

/*      Position the middle value */

	scanl = ED_height/4 + xheight*(PRODUCT_COLORS-1)/2;

/*      Format the middle value */

	if ((ED_value_max+ED_value_min) < 0.0) {

	    sprintf (text,"%d",(int) ((ED_value_max+ED_value_min)/2-0.5));

	} else {

	    sprintf (text,"%d",(int) ((ED_value_max+ED_value_min)/2+0.5));

	}

	XDrawString (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	XDrawString (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-COLOR_BAR_WIDTH),
		(int) (scanl+5),
		text,
		strlen (text));

	XDrawLine (ED_display,
		ED_window,
		ED_gc,
		(int) (ED_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (ED_width-TICK_MARK_LENGTH),
		scanl);

	XDrawLine (ED_display,
		ED_pixmap,
		ED_gc,
		(int) (ED_width-COLOR_BOX_WIDTH),
		scanl,
		(int) (ED_width-TICK_MARK_LENGTH),
		scanl);

/*	Reset the clip rectangle so that drawing in the model data	*
 *	display region will not write over the color bar.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = ED_width-COLOR_BAR_WIDTH;
	clip_rectangle.height  = ED_height;

	XSetClipRectangles (ED_display,
			    ED_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);
}


/************************************************************************
 *	Description: This function creates a vertical column of		*
 *		     pushbuttons along the left side of the envirodata	*
 *		     display window.  A button is created for each	*
 *		     level in the current environmental model.  	*
 *									*
 *	Input:  number_levels  Number of levels in the model            *
 *              params  RPGCS_model_grid parameters contain the pressure*
 *                      levels included in the model                    *
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_level_buttons (int number_levels, int rows, int columns,
                              RPGCS_model_grid_params_t *params[])
{
	XmString	text;
	int		i;

	void	hci_data_range_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
        HCI_LE_log("Entering hci_envirodata_level_buttons, number of levels = %d",number_levels);
/*	Update the Model Label in the Environmental Model Display Window.	*
 *      Extra spaces are needed to keep window from resizing when displaying    *
 *      the surface data as only one level selection box is drawn in that case. */

        if(ED_model == RUC13) {
           sprintf (Buf,"          %s    %2d %s %2d               ",
                    "      RAP13",rows,"x",columns);
        }
        else if(ED_model == RUC40) {
           sprintf (Buf,"          %s    %2d %s %2d               ",
                    "      RAP40",rows,"x",columns);
        }
        else if(ED_model == RUC80) {
           sprintf (Buf,"          %s    %2d %s %2d               ",
                    "      RAP80",rows,"x",columns);
        }
        else {
           sprintf(Buf,"Model Unknown");
        }
           
	text = XmStringCreateLocalized (Buf);

	XtVaSetValues (Model_label,
		XmNlabelString,	text,
		NULL);

	XmStringFree (text);


	for (i=0;i<number_levels;i++) {

           if (ED_data_type != SFC_PRESSURE) {     
                if (params != NULL) 
                   sprintf (Buf," %4.0f ",params[i]->level_value);
                else
                   sprintf (Buf," %d ",i);
           }
           else {
                   sprintf (Buf," %s ","SFC");
           }

	   text = XmStringCreateLocalized (Buf);

	   XtVaSetValues (Button [i],
		XmNlabelString,	text,
		XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
		NULL);

	   XmStringFree (text);

/*	   Manage all of those which are active.		*/
           XtManageChild (Button [i]);

	}

/*	Unmanage all inactive level buttons.		*/

	for (i=number_levels;i<MAX_LEVEL_BUTTONS;i++) {

	    XtUnmanageChild (Button [i]);

	}
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the envirodata display	*
 *		     window to temperature.				*
 *									*
 *	Input:  w - temperature radio button ID	       			*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_temperature (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

/*	    Reset the current field, range, min, and max fields.	*/

	    Field_changed = 1;
	    ED_data_type = TEMPERATURE;
	    strcpy (ED_current_field,"   Temperature");

	    ED_value_min  = Temperature_min;
	    ED_value_max  = Temperature_max;

	    ED_color_scale = (ED_value_max - ED_value_min)/(PRODUCT_COLORS-1);

	    hci_get_product_colors (REFLECTIVITY_PRODUCT, ED_color);

/*	    If we are looking at a previously displayed model then  	*
 *	    redisplay it.						*/

	    hci_display_selected_level (Button [ED_display_level],
		                       (XtPointer) ED_display_level,
			               (XtPointer) NULL);

/*	    Redisplay the color bar for the new field.			*/

	    hci_envirodata_color_bar ();

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    hci_envirodata_expose_callback ((Widget) NULL,
		                          (XtPointer) NULL,
		                          (XtPointer) NULL);
	}
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the envirodata display	*
 *		     window to relative humidity.			*
 *									*
 *	Input:  w - relative humidity radio button ID			*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_rel_humidity (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

/*	    Reset the current field, range, min, and max fields.	*/

	    Field_changed = 1;
	    ED_data_type = REL_HUMIDITY;
	    strcpy (ED_current_field,"Relative Humidity");

	    ED_value_min  = Rel_humidity_min;
	    ED_value_max  = Rel_humidity_max;

	    ED_color_scale = (ED_value_max - ED_value_min)/(PRODUCT_COLORS-1);

	    hci_get_product_colors (REFLECTIVITY_PRODUCT, ED_color);

/*	    If we are looking at a previously displayed model then  	*
 *	    redisplay it.						*/

            hci_display_selected_level (Button [ED_display_level],
			               (XtPointer) ED_display_level,
			               (XtPointer) NULL);

/*	    Redisplay the color bar for the new field.			*/

	    hci_envirodata_color_bar ();

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    hci_envirodata_expose_callback ((Widget) NULL,
		                          (XtPointer) NULL,
		                          (XtPointer) NULL);
	}
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the envirodata display	*
 *		     window to surface pressure.			*
 *									*
 *	Input:  w - surface pressure radio button ID			*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_sfc_pressure (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

/*	    Reset the current field, range, min, and max fields.	*/

	    Field_changed = 1;
	    ED_data_type = SFC_PRESSURE;
	    strcpy (ED_current_field,"Surface Pressure");

	    ED_value_min  = Sfc_pressure_min;
	    ED_value_max  = Sfc_pressure_max;

	    ED_color_scale = (ED_value_max - ED_value_min)/(PRODUCT_COLORS-1);

	    hci_get_product_colors (REFLECTIVITY_PRODUCT, ED_color);

/*	    If we are looking at a previously displayed model then 	*
 *	    redisplay it. 					 	*/

            hci_display_selected_level (Button [ED_display_level],
			               (XtPointer) ED_display_level,
			               (XtPointer) NULL);

/*	    Redisplay the color bar for the new field.			*/

	    hci_envirodata_color_bar ();

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    hci_envirodata_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
	}
}

/************************************************************************
 *	Description: This function is used to display the selected	*
 *		     model data level from the list of level buttons.	*
 *									*
 *	Input:  w - push button ID					*
 *		client_data - level index		 		*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE 							*
 ************************************************************************/

void
hci_display_selected_level (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	ret;
	int	i, j, index;
        RPGCS_model_grid_data_t *grid_data = NULL;
        RPGCS_model_attr_t          *attrs = NULL;
        int     model, rows, columns;
        int     units;
        char    *databuf = NULL;
        int     pixel, scanl;
        int     radar_grid_i, radar_grid_j;
        Siteadp_adpt_t site;
	char	buf [32];
        double  dbl_range, dbl_azimuth;

/*	Get the current model level from the button client data.	*/

	ED_display_level = (int) client_data;

/*	Clear the contents of the base data window.			*/

	hci_envirodata_display_clear (ED_display,
			    ED_pixmap,
			    ED_gc,
			    ED_width,
			    ED_height,
			    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	if (ED_data == NULL) {
            HCI_LE_log("ED_data is NULL, allocating...");
	    ED_data = calloc (SIZEOF_ENVIRODATA, sizeof(float));

	    if (ED_data == NULL) {

		HCI_LE_error("calloc (SIZEOF_BASEDATA, sizeof(float)) failed");
		return;

            }
	}

        /* Get the model data. */

        if( (model = RPGCS_get_model_data( ORPGDAT_ENVIRON_DATA_MSG, RUC_ANY_TYPE, &databuf )) < 0 ){

            HCI_LE_error("RPGCS_get_model_data failed\n" );

            XSetFont (ED_display, ED_gc, hci_get_font (LARGE));
	    XSetForeground (ED_display, ED_gc,hci_get_read_color (PRODUCT_FOREGROUND_COLOR));
	    sprintf (buf,"Product Not Available");

	    XDrawString (ED_display,
		         ED_pixmap,
		         ED_gc,
		         (int) (ED_center_pixel -
			      4*strlen (buf)),
		         ED_center_scanl + 4,
		         buf,
		         strlen (buf));

            XSetFont (ED_display, ED_gc, hci_get_font (LIST));

	    hci_envirodata_expose_callback ((Widget) NULL,
	                                (XtPointer) NULL,
		                        (XtPointer) NULL);
            return;
        }

        /* Get the model attributes. */

        attrs = RPGCS_get_model_attrs( model, databuf );
        if( attrs == NULL ){

           HCI_LE_error("Model Attributes Processing Failure\n" );

           if( databuf != NULL )
              RPGP_product_free( databuf );

        }

        /* This initializes modules used to convert map projected data to
           meteorological coordinates. */

        RPGCS_lambert_init( attrs );

        /* Find the closest grid point to the radar location. */

        if( (ret = ORPGSITE_get_site_data( &site )) < 0 ){
           HCI_LE_error("ORPGSITE_get_site_data() Failed: %d\n", ret );

           if( databuf != NULL )
              RPGP_product_free( databuf );
        }

        /* Convert the site lat/lon to model grid point. */

        if( ( ret = RPGCS_lambert_latlon_to_grid_point( ((double) site.rda_lat)/1000.0,
                                                        ((double) site.rda_lon)/1000.0,
                                                        &radar_grid_i, &radar_grid_j )) < 0 ){
           if( ret == RPGCS_DATA_POINT_NOT_IN_GRID )
              HCI_LE_error("Site Lat/Lon Not Within Model Grid\n" );
           else
              HCI_LE_error("RPGCS_lambert_latlon_to_grid_point Failed\n" );

           if( databuf != NULL )
              RPGP_product_free( databuf );
           return;
        }

        /* Set the model name */

        ED_model = attrs->model;

        /* Get the selected grid data */

        if (ED_data_type == TEMPERATURE) {
           grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_TEMP);
        } else if (ED_data_type == REL_HUMIDITY) {
           grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_RH);
        } else if (ED_data_type == SFC_PRESSURE) {
           grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_SFCP);
        }

        if (grid_data == NULL) {

           XmString str;

	   HCI_LE_error("Get_grids () failed");
           if( databuf != NULL )
               RPGP_product_free( databuf );

           if( attrs != NULL ){
              free( attrs );
              attrs = NULL;
           }

           /* Display blank label at top of window */

           sprintf( buf,"        " );
           str = XmStringCreateLocalized( buf );
           XtVaSetValues( ED_label, XmNlabelString, str, NULL );
           XmStringFree( str );

           XSetFont (ED_display, ED_gc, hci_get_font (LARGE));
	   XSetForeground (ED_display, ED_gc,hci_get_read_color (PRODUCT_FOREGROUND_COLOR));
           if( ED_data_type == TEMPERATURE )
	      sprintf (buf,"Temperature Data Not Available");
           else if( ED_data_type == REL_HUMIDITY )
	      sprintf (buf,"Relative Humidity Data Not Available");
           else if( ED_data_type == SFC_PRESSURE )
	      sprintf (buf,"Sfc Pressure Data Not Available");

	   XDrawString (ED_display,
	                ED_pixmap,
	                ED_gc,
	                (int) (ED_center_pixel -
		        4*strlen (buf)),
	                ED_center_scanl + 4,
	                buf,
	                strlen (buf));

           XSetFont (ED_display, ED_gc, hci_get_font (LIST));

	   hci_envirodata_expose_callback ((Widget) NULL,
	                                   (XtPointer) NULL,
	   	                           (XtPointer) NULL);
	   return;
        }

        /* Determine the number of rows (Ys) to draw for the grid */

        rows = grid_data->dimensions[1];
        ED_rows = grid_data->dimensions[1];

        /* Determine the number of columns (Xs) to draw for the grid */

        columns = grid_data->dimensions[0];
        ED_columns = grid_data->dimensions[0];

        /* Verify the the lower left corner lat/lon postion as given in the model and */
        /* as the RPG sees it in azran coordinates                                    */

        RPGCS_lambert_grid_point_azran(0, 0, &dbl_range, &dbl_azimuth);
        HCI_LE_log("lower left azran: %f deg %f nm", dbl_azimuth, (dbl_range*KM_TO_NM));

        /* Determine the number of levels in the model. */

        ED_levels = grid_data->num_levels;
        HCI_LE_log("NUMBER OF LEVELS IN GRID  %d ED_display_level=%d", ED_levels, ED_display_level);
        if (ED_levels >= MAX_LEVEL_BUTTONS) ED_levels = MAX_LEVEL_BUTTONS-1;

        /* If the current display level is higher than the available levels, reset     */
        /* to the highest available level. Typically, there are more temperature       */
        /* levels than relative humidity levels, and of course, only one surface level */

        if (ED_display_level >= ED_levels) {
           ED_display_level = ED_levels-1;
        }
           
        /* Display date/time label at top of window */

	draw_static_label(attrs->valid_time,
                          attrs->forecast_period,
                          (float) grid_data->params[ED_display_level]->level_value);

        /* Display the level buttons */

        hci_envirodata_level_buttons (ED_levels, rows, columns, grid_data->params);

        /* Compute the scale based on number of rows and columns.  The RAP13 grid     */
        /* size is 13.545 km and the RAP40 is 40.635 km per grid. The RAP13 typically */
        /* comes with approximately 65 grid boxes in the X direction, the RAP40 comes */
        /* with about 22 grid boxes in the X direction.                               */

        if(ED_model == RUC13) {

           /* RAP13 grid */
           pixel = RPGC_NINT(ED_zoom_factor * (13.545) * ED_scale_x);
           scanl = RPGC_NINT(ED_zoom_factor * (13.545) * (-ED_scale_y));
           if (radar_grid_i == 30) ED_model_x_offset = 37;
           if (radar_grid_i == 31) ED_model_x_offset = 30;
           if (radar_grid_i == 32) ED_model_x_offset = 23;
           if (radar_grid_j == 30) ED_model_y_offset = 21;
           if (radar_grid_j == 31) ED_model_y_offset = 14;
           if (radar_grid_j == 32) ED_model_y_offset = 7;
        }
         else if( attrs->model == RUC40 ){

           /* RAP40 grid */
           pixel = RPGC_NINT(ED_zoom_factor * (40.635) * ED_scale_x);
           scanl = RPGC_NINT(ED_zoom_factor * (40.635) * (-ED_scale_y));
           if (radar_grid_i == 11) ED_model_x_offset = -2;
           if (radar_grid_i == 10) ED_model_x_offset = 20;
           if (radar_grid_j == 11) ED_model_y_offset = -15;
           if (radar_grid_j == 10) ED_model_y_offset = 5;

        }
         else {

           /* Unknown grid! */
	   HCI_LE_error("Unknown grid type!");
           pixel = 1;
           scanl = 1;
        }

        ED_pixel_size = pixel;
        ED_scanl_size = scanl;

        HCI_LE_log("columns %d rows %d levels %d ED_data_type %d", columns, rows, ED_levels, ED_data_type);
        HCI_LE_log("radar_grid_i %d radar_grid_j %d", radar_grid_i, radar_grid_j);
        HCI_LE_log("ED_x_offset %f ED_y_offset %f", ED_x_offset, ED_y_offset);
        HCI_LE_log("ED_model_x_offset %d ED_model_y_offset %d", ED_model_x_offset, ED_model_y_offset);
        HCI_LE_log("ED_scale_x %f ED_scale_y %f", ED_scale_x, ED_scale_y);
        HCI_LE_log("ED_center_pixel %d ED_center scanl %d", ED_center_pixel, ED_center_scanl);
        HCI_LE_log("ED_value_min %f ED_value_max %f", ED_value_min, ED_value_max);
        HCI_LE_log("ED_data_type %d Filter_mode", ED_data_type, Filter_mode);
        HCI_LE_log("ED_zoom_factor %d ED_display_range %f", ED_zoom_factor, ED_display_range);
        HCI_LE_log("ED_width %d ED_height %d", ED_width, ED_height);
        HCI_LE_log("ED_pixel_size %d  ED_scanl_size %d", ED_pixel_size, ED_scanl_size);

        for (j=0; j<rows; j++) {

           /* Copy this row for drawing */

           for (i=0; i<columns; i++) {

              /* Set the index into the grid */

              index = j*columns+i;

              /* Note:  Data starts with southern-most row so we swap row order to */
              /*        draw starting at the top (northern edge) of the window.    */

              ED_data[index] = RPGCS_get_data_value( grid_data, ED_display_level, i, rows-1-j, &units );

              if (ED_data[index] != MODEL_BAD_VALUE) {
                 if (ED_data_type == TEMPERATURE) {

                    /* We want the units of temperature in degrees C. */
                    if( units == RPGCS_KELVIN_UNITS ) ED_data[index] += KELVIN_TO_C;
                 }
                 else if (ED_data_type == REL_HUMIDITY) {

                    /* No unit change needed for relative humidity */
                 }
                 else if (ED_data_type == SFC_PRESSURE) {

                    /* We want the units of surface pressure in millibars */
                    if( units == RPGCS_PASCAL_UNITS ) ED_data[index] *= PASCAL_TO_MB;
                 }
              }
           }/* end for all columns */
	} /* end for all rows */
       
        /* Draw the grid of data */

        hci_envirodata_display (ED_data,
	                      ED_display,
	                      ED_pixmap,
		              ED_gc,
		             (ED_x_offset+ED_model_x_offset),
			     (ED_y_offset+ED_model_y_offset),
			      ED_scale_x,
		              ED_scale_y,
			      ED_center_pixel,
			      ED_center_scanl,
			      ED_value_min,
		              ED_value_max,
		              ED_color,
			      ED_data_type,
			      Filter_mode,
                              rows,
                              columns,
                              pixel,
                              scanl);

/*	Display a polar grid if enabled.				*/

	if (Grid_overlay_flag) {
	    hci_envirodata_overlay_grid ();
	}

/*	Invoke the expose callback to make changes visible.		*/

	hci_envirodata_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

/*      Free the memory for the grid */

        if (grid_data != NULL){
           RPGCS_free_model_field( model, (char *) grid_data );
           grid_data = NULL;
        }
        if( databuf != NULL ){
           RPGP_product_free( databuf );
           databuf = NULL;
        }
        if( attrs != NULL ){
           free( attrs  );
           attrs = NULL;
        }

}

/************************************************************************
 *	Description: This function handles all mouse button events	*
 *		     inside the model data display region.		*
 *									*
 *	Input:  w - drawing area widget ID				*
 *		event - X event data					*
 *		args - number of user argunments			*
 *		num_args - user arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_draw (
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
        int     i, j;
	float	azimuth, range;
	float	height;
	float	value;
        int     units;
        int     model;
        char    *databuf = NULL;
	XmString	string;
        RPGCS_model_grid_data_t *grid_data = NULL;

	float	hci_envirodata_interrogate (char *buf,
				float azimuth, float range,
				int field, int ptr);

/*	Ignore button clicks outside of drawing area. */

	if( event->xbutton.x > ( ED_width - COLOR_BAR_WIDTH ) )
	{
	  return;
	}

	value = 0.0;

	if (!strcmp (args[0], "down1")) {

/*	    If the left button is pressed, save the value of this point for	*
 *	    future reference.  For editing clutter suppression regions,		*
 *	    this point marks the left side of the sector.	cpc001tsk005backup.tar		*/

	    first_pixel = event->xbutton.x;
	    first_scanl = event->xbutton.y;

/*	    Set the button down flag to indicate that the left mouse button	*
 *	    has been pressed.  This is necessary for letting the rubber-	*
 *	    banding code to know when to start (for motion events when the	*
 *	    left mouse button is pressed).					*/

	    button_down = 1;

            /* Interrogate the selected data point for its value */
	    azimuth = hci_find_azimuth (event->xbutton.x,
		 		        event->xbutton.y,
				        (int) (ED_center_pixel +
				               ED_x_offset*ED_scale_x),
				        (int) (ED_center_scanl +
				               ED_y_offset*ED_scale_y));

	    x1 = (ED_center_pixel-event->xbutton.x)/
		  ED_scale_x + ED_x_offset;
	    y1 = (ED_center_scanl-event->xbutton.y)/
		  ED_scale_y + ED_y_offset;

            /* Convert x1,y1, that are radar relative, to model grid box (i,j) */
            i = (event->xbutton.x - ED_model_x_offset) / ED_pixel_size;
            j = (event->xbutton.y - ED_model_y_offset) / ED_scanl_size;

            HCI_LE_log("i = %d (x = %f) j= %d (y = %f)", i, x1, j, y1);

            /* Get the model data. */
            if( (model = RPGCS_get_model_data( ORPGDAT_ENVIRON_DATA_MSG, RUC_ANY_TYPE, &databuf )) < 0 ){

               if( model != LB_NOT_FOUND )
                  HCI_LE_error("RPGCS_get_model_data failed during interrogation\n" );
            }

            if ( i < 0 || i > ED_columns-1 || j < 0 || j > ED_rows-1) {
               sprintf (Buf,"No Data");
               value = -999999;
               string = XmStringCreateLocalized (Buf);

	       XtVaSetValues (Height_data,
		              XmNlabelString,	string,
			      NULL);

	       XmStringFree (string);

            }
            else {

               /* Get the geopotential height field */
               grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_GH);

               /* Find the geopotential height value for the selected point */
               height = RPGCS_get_data_value(grid_data, ED_display_level, i, ED_rows-1-j, &units );

               /* Find the range of the selected point */
	       range = sqrt ((double) (x1*x1 + y1*y1));

	       sprintf (Buf,"%d deg, %d nm",
		        (int) azimuth, (int) (range*HCI_KM_TO_NM +0.5));

	       string = XmStringCreateLocalized (Buf);

	       XtVaSetValues (Azran_data,
		    	      XmNlabelString,	string,
			      NULL);

               XmStringFree (string);

               if (height > 999999 || height < 0 ) {
                  sprintf (Buf,"No Data");
               } else {
                  sprintf (Buf,"%d m (MSL)", (int) (height+0.5));
               }

               string = XmStringCreateLocalized (Buf);

	       XtVaSetValues (Height_data,
		              XmNlabelString,	string,
			      NULL);

	       XmStringFree (string);

               if (grid_data != NULL){
                   RPGCS_free_model_field( model, (char *) grid_data );
                   grid_data = NULL;
               }

               /* Get the selected grid data */

               if (ED_data_type == TEMPERATURE) {
                  grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_TEMP);
               } else if (ED_data_type == REL_HUMIDITY) {
                  grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_RH);
               } else if (ED_data_type == SFC_PRESSURE) {
                  grid_data = (RPGCS_model_grid_data_t *)Get_grids(model, databuf, RPGCS_MODEL_SFCP);
               }

               if (grid_data == NULL) {
	          HCI_LE_error("Get_grids () failed during interrogation");
                  if( databuf != NULL )
                     RPGP_product_free( databuf );
                  return;
               }

               value = RPGCS_get_data_value(grid_data, ED_display_level, i, ED_rows-1-j, &units );

	       if (ED_data_type == TEMPERATURE) {

		  if (value < TEMPERATURE_MIN) {

		    sprintf (Buf,"No Data");

		  } else {

                    /* We want the units of temperature in degrees C. */
                    if( units == RPGCS_KELVIN_UNITS ) value += KELVIN_TO_C;
		    sprintf (Buf,"%3.1f C", value);
                  }

	       } else if (ED_data_type == REL_HUMIDITY) {

		  if ((value < REL_HUMIDITY_MIN) ||
		      (value > REL_HUMIDITY_MAX)) {

	            sprintf (Buf,"No Data");
                  } else {

		    sprintf (Buf,"%3.0f %%", value);

		  }

               } else if (ED_data_type == SFC_PRESSURE) {

		  if (value < SFC_PRESSURE_MIN) {

		    sprintf (Buf,"No Data");

		  } else {

                    /* We want the units of pressure in millibars. */
                    if( units == RPGCS_PASCAL_UNITS ) value *= PASCAL_TO_MB;
		    sprintf (Buf,"%d mb", (int) value);

		  }
               }
            }
	    string = XmStringCreateLocalized (Buf);

	    XtVaSetValues (Value_data,
		 	   XmNlabelString,	string,
			   NULL);

            XmStringFree (string);

            if( databuf != NULL )
               RPGP_product_free( databuf );

            /* Free the model grid memory */

            if (grid_data != NULL){
               RPGCS_free_model_field( model, (char *) grid_data );
               grid_data = NULL;
            }

 	} else if (!strcmp (args[0], "up1")) {

/*	    If the left mouse button is released, stop the rubber-banding	*
 *	    code and unset the button down flag.				*/

	    HCI_LE_log("Base Data left button released");
	    button_down  = 0;

	} else if (!strcmp (args[0], "motion1")) {

/*	    If the cursor is being dragged with the left mouse button		*
 *	    down, repeatedly draw the sector defined by the point where		*
 *	    the button was pressed and where it currently is.  Use the		*
 *	    left hand rule for determining the direction in which to draw	*
 *	    the sector.								*/

	} else if (!strcmp (args[0], "down2")) {

	    HCI_LE_log("Base Data middle button pushed");

	} else if (!strcmp (args[0], "down3")) {

	    HCI_LE_log("Base Data right button pushed");
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
	Widget	temperature_frame;
	Widget	rel_humidity_frame;
	Widget	sfc_pressure_frame;
	Widget	label;
	char	buf [16];
	XmString	str;

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

/*	Create a set of slider bars to control the temperature data range */

	temperature_frame = XtVaCreateWidget ("temperature_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	temperature_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Temperature_min_scale = XtVaCreateWidget ("temperature_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Temperature (C)", 27,
		XmNshowValue,		True,
		XmNminimum,		(int) TEMPERATURE_MIN,
		XmNmaximum,		(int) TEMPERATURE_MAX,
		XmNvalue,		(int) Temperature_min,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Temperature_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 0);

	sprintf (buf,"%d",(int) TEMPERATURE_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Temperature_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) TEMPERATURE_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Temperature_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Temperature_min_scale);

	Temperature_max_scale = XtVaCreateWidget ("temperature_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Temperature (C)", 24,
		XmNshowValue,		True,
		XmNminimum,		(int) TEMPERATURE_MIN,
		XmNmaximum,		(int) TEMPERATURE_MAX,
		XmNvalue,		(int) Temperature_max,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Temperature_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 1);

	sprintf (buf,"%d",(int) TEMPERATURE_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Temperature_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) TEMPERATURE_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Temperature_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Temperature_max_scale);
	XtManageChild (rowcol);
	XtManageChild (temperature_frame);

/*	Create a set of slider bars to control the relative humidity data range	*/

	rel_humidity_frame = XtVaCreateWidget ("rel_humidity_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		temperature_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	rel_humidity_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);


	if (Rel_humidity_min < REL_HUMIDITY_MIN) {

	    Rel_humidity_min = REL_HUMIDITY_MIN;

	}

	Rel_humidity_min_scale = XtVaCreateWidget ("rel_humidity_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Rel. Humidity (%)", 26,
		XmNshowValue,		True,
		XmNminimum,		(int) (REL_HUMIDITY_MIN),
		XmNmaximum,		(int) (REL_HUMIDITY_MAX),
		XmNvalue,		(int) (Rel_humidity_min),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Rel_humidity_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 2);

	sprintf (buf,"%d",(int) (REL_HUMIDITY_MIN));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Rel_humidity_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (REL_HUMIDITY_MAX));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Rel_humidity_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Rel_humidity_min_scale);

	if (Rel_humidity_max > REL_HUMIDITY_MAX) {

	    Rel_humidity_max = REL_HUMIDITY_MAX;

	}

	Rel_humidity_max_scale = XtVaCreateWidget ("rel_humidity_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Rel. Humidity (%)", 26,
		XmNshowValue,		True,
		XmNminimum,		(int) (REL_HUMIDITY_MIN),
		XmNmaximum,		(int) (REL_HUMIDITY_MAX),
		XmNvalue,		(int) (Rel_humidity_max),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Rel_humidity_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 3);

	sprintf (buf,"%d",(int) (REL_HUMIDITY_MIN));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Rel_humidity_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (REL_HUMIDITY_MAX));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Rel_humidity_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Rel_humidity_max_scale);
	XtManageChild (rowcol);
	XtManageChild (rel_humidity_frame);

/*	Create a set of slider bars to control the surface pressure data range */

	sfc_pressure_frame = XtVaCreateWidget ("sfc_pressure_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rel_humidity_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	sfc_pressure_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Sfc_pressure_min_scale = XtVaCreateWidget ("sfc_pressure_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Sfc Pressure (mb)", 25,
		XmNshowValue,		True,
		XmNminimum,		(int) (SFC_PRESSURE_MIN),
		XmNmaximum,		(int) (SFC_PRESSURE_MAX),
		XmNvalue,		(int) (Sfc_pressure_min),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Sfc_pressure_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 4);

	sprintf (buf,"%d",(int) (SFC_PRESSURE_MIN));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Sfc_pressure_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (SFC_PRESSURE_MAX));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Sfc_pressure_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Sfc_pressure_min_scale);

	Sfc_pressure_max_scale = XtVaCreateWidget ("sfc_pressure_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Sfc Pressure (mb)", 25,
		XmNshowValue,		True,
		XmNminimum,		(int) (SFC_PRESSURE_MIN),
		XmNmaximum,		(int) (SFC_PRESSURE_MAX),
		XmNvalue,		(int) (Sfc_pressure_max),
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (Sfc_pressure_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 5);

	sprintf (buf,"%d",(int) (SFC_PRESSURE_MIN));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Sfc_pressure_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) (SFC_PRESSURE_MAX));
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Sfc_pressure_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild (Sfc_pressure_max_scale);
	XtManageChild (rowcol);
	XtManageChild (sfc_pressure_frame);

	XtManageChild (form);

	HCI_Shell_start( Data_range_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is activated when one of the		*
 *		     slider bars is moved	.			*
 *									*
 *	Input:  w - slider bar ID					*
 *		client_data -  0 (temperature minumum)			*
 *			       1 (temperature maximum)			*
 *			       2 (rel. humidity minimum)		*
 *			       3 (rel. humidity maximum)		*
 *			       4 (surface pressure minimum)		*
 *			       5 (surface pressure maximum)		*
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

	    case 0 :	/*  Temperature Min  */

		Temperature_min = (float) cbs->value;

		if (Temperature_min >= Temperature_max) {

		    Temperature_min = Temperature_max-1;

		    XtVaSetValues (Temperature_min_scale,
			XmNvalue,	(int) Temperature_min,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 1 :	/*  Temperature Max  */

		Temperature_max = (float) cbs->value;

		if (Temperature_max <= Temperature_min) {

		    Temperature_max = Temperature_min+1;

		    XtVaSetValues (Temperature_max_scale,
			XmNvalue,	(int) Temperature_max,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 2 :	/*  Relative Humidity Min  */

		Rel_humidity_min = (float) cbs->value;

		if (Rel_humidity_min >= Rel_humidity_max) {

		    Rel_humidity_min = Rel_humidity_max-1;

		    XtVaSetValues (Rel_humidity_min_scale,
			XmNvalue,	(int) (Rel_humidity_min),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 3 :	/*  Relative Humidity Max */

		Rel_humidity_max = (float) cbs->value;

		if (Rel_humidity_max <= Rel_humidity_min) {

		    Rel_humidity_max = Rel_humidity_min+1;

		    XtVaSetValues (Rel_humidity_max_scale,
			XmNvalue,	(int) (Rel_humidity_max),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 4 :	/*  Surface Pressure Min  */

		Sfc_pressure_min = (float) cbs->value;

		if (Sfc_pressure_min >= Sfc_pressure_max) {

		    Sfc_pressure_min = Sfc_pressure_max-1;

	 	    XtVaSetValues (Sfc_pressure_min_scale,
			XmNvalue,	(int) (Sfc_pressure_min),
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 5 :	/*  Surface Pressure Max  */

		Sfc_pressure_max = (float) cbs->value;

		if (Sfc_pressure_max <= Sfc_pressure_min) {

		    Sfc_pressure_max = Sfc_pressure_min+1;

		    XtVaSetValues (Sfc_pressure_max_scale,
			XmNvalue,	(int) (Sfc_pressure_max),
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
	switch (ED_data_type) {

	    case TEMPERATURE :

		ED_value_min = Temperature_min;
		ED_value_max = Temperature_max;
		ED_color_scale  = (ED_value_max-ED_value_min)/
				    (PRODUCT_COLORS-1);
		break;

	    case REL_HUMIDITY :

		ED_value_min = Rel_humidity_min;
		ED_value_max = Rel_humidity_max;
		ED_color_scale  = (ED_value_max-ED_value_min)/
				    (PRODUCT_COLORS-1);
		break;

	    case SFC_PRESSURE :

		ED_value_min = Sfc_pressure_min;
		ED_value_max = Sfc_pressure_max;
		ED_color_scale  = (ED_value_max-ED_value_min)/
				    (PRODUCT_COLORS-1);
		break;

	}

/*	If we are looking at a previously displayed model then redisplay it */

        hci_envirodata_resize_callback ((Widget) NULL,
	  		              (XtPointer) NULL,
			              (XtPointer) NULL);

	Update_flag = 0;

	XtVaSetValues (Apply_button,
		       XmNsensitive,	False,
		       NULL);

}

/************************************************************************
 *	Description: This function is activated when one of the "Map"	*
 *		     radio buttons is selected.	 			*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data -  Map overlay flag				*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_map_callback (
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

	    hci_envirodata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
	}
}

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     "Filter" radio buttons is selected.		*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data -  filter mode				*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_filter_callback (
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

	    hci_envirodata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     "Grid" radio buttons is selected.			*
 *									*
 *	Input:  w - radio button ID					*
 *		client_data -  grid overlay mode			*
 *		call_data - toggle button data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_grid_callback (
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

	    hci_envirodata_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function displays a polar grid over a		*
 *		     model data display.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_overlay_grid (
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

	XSetForeground (ED_display, ED_gc,
		hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

/*	Based on the current zoom factor, determine the distance	*
 *	between polar grid rings (in nautical miles).			*/

	switch (ED_zoom_factor) {

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

	x = fabs ((double) ED_x_offset);
	y = fabs ((double) ED_y_offset);

	if (x > y) {

	    start_range = x - ED_display_range/ED_zoom_factor;
	    stop_range  = x + ED_display_range/ED_zoom_factor;

	} else {

	    start_range = y - ED_display_range/ED_zoom_factor;
	    stop_range  = y + ED_display_range/ED_zoom_factor;

	}

	start_range = (start_range/step)*step;
	stop_range  = (stop_range/step)*step + step;

	if (start_range < step) {

	    start_range = step;

	}

	for (i=step;i<=stop_range;i=i+step) {

	    size  = i*ED_scale_x/HCI_KM_TO_NM;

	    pixel = ED_center_pixel + ED_x_offset*ED_scale_x -
		    size;
	    scanl = ED_center_scanl + ED_y_offset*ED_scale_y -
		    size;

	    XDrawArc (ED_display,
		      ED_pixmap,
		      ED_gc,
		      pixel,
		      scanl,
		      size*2,
		      size*2,
		      0,
		      -(360*64));

	    sprintf (buf,"%i nm",i);

	    XDrawString (ED_display,
			 ED_pixmap,
			 ED_gc,
			 (int) (ED_center_pixel +
				ED_x_offset*ED_scale_x -
				4*strlen (buf)),
			 scanl + 4,
			 buf,
			 strlen (buf));
	}

/*	Next display the "spokes" outward from the center.		*/

	for (i=0;i<360;i=i+step) {

	    pixel = (int) (((1+1.0/ED_zoom_factor)*ED_display_range *
		     cos ((double) (i+90)*HCI_DEG_TO_RAD) +
		     ED_x_offset) * ED_scale_x +
		     ED_center_pixel);
	    scanl = (int) (((1+1.0/ED_zoom_factor)*ED_display_range *
		     sin ((double) (i-90)*HCI_DEG_TO_RAD) +
		     ED_y_offset) * ED_scale_y +
		     ED_center_scanl);

	    XDrawLine (ED_display,
		       ED_pixmap,
		       ED_gc,
		       (int) (ED_center_pixel +
			      ED_x_offset*ED_scale_x),
		       (int) (ED_center_scanl +
			      ED_y_offset*ED_scale_y),
		       pixel,
		       scanl);

	}

	hci_envirodata_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the RPG Environmental Model	*
 *		     Display window.					*
 *									*
 *	Input:  w - Close button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_envirodata_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Enviro Data window Close pushed");
	HCI_task_exit (HCI_EXIT_SUCCESS);
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

/************************************************************************
 *      Description: This function is called to draw the model label    *
 *                   at the top of the display.                         *
 *                                                                      *
 *      Input:  model_run_time                                          *
 *              forecast_period                                         *
 *              level (in millibars)                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void
draw_static_label(time_t model_run_time, time_t forecast_period, float level)
{

  int		myr = 0, mmo = 0, mdy = 0, mhr = 0, mmin = 0, msec = 0;
  int		fyr = 0, fmo = 0, fdy = 0, fhr = 0, fmin = 0, fsec = 0;

  XmString	str;
  HCI_LE_log("*Starting draw_static_label()********");

  RPGCS_unix_time_to_ymdhms( model_run_time, &myr, &mmo, &mdy, &mhr, &mmin, &msec );
  RPGCS_unix_time_to_ymdhms( forecast_period, &fyr, &fmo, &fdy, &fhr, &fmin, &fsec );

  if (ED_data_type == SFC_PRESSURE ) {
     /* Unique label needed for surface pressure (pressure level varies at the surface) */
     sprintf( Buf,"%s %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d  %s %02d",
              ED_current_field, "    Valid Date/Time:",
              mmo, mdy, myr, mhr, mmin, msec, "Fcst Hour:", fhr );
  }
  else {
     sprintf( Buf,"%s %s %4.0f %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d  %s %02d",
              ED_current_field, "Level:", level, "mb  Valid Date/Time:",
              mmo, mdy, myr, mhr, mmin, msec, "Fcst Hour:", fhr );
  }

  str = XmStringCreateLocalized( Buf );
  XtVaSetValues( ED_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

/******************************************************************

   Description:
      Obtain requested model grids (all levels) for a particular
      data field.

   Inputs:
      model - model of interest
      buf - buffer holding the model of interest
      field - ID of requested grid field

   Returns:
      Pointer to grids

******************************************************************/
static void *Get_grids(int model, char *buf, char *field) {

   void *grid_data = NULL;

   /* Get the requested field. */

   grid_data = RPGCS_get_model_field( model, buf, field );

   /* Was field available? */

   if( grid_data == NULL  ){

     LE_send_msg( GL_ERROR, "Grid field %s not available.\n", field); 
   }

   return grid_data;
   
/* End of Get_grids() */
}

/************************************************************************
 *	Description: This is a dummy function, required for the call to	*
 *		     HCI_start.  Maybe there is a way to eliminate it 	*
 *		     but I don't know what that is.			*
 *									*
 *	Input:  NONE							*
 *	 								*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
timer_proc ()
{
  int refresh_flag;

/* Dummy function			*/

  refresh_flag = 0;

}
