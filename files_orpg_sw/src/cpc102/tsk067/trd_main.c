/************************************************************************
 *									*
 *	Module:  trd_main.c						*
 *									*
 *	Description: This task is used to display polar terrain data	*
 *		and products used by the hydromet processing.		*
 *									*
 *************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/04/12 18:08:04 $
 * $Id: trd_main.c,v 1.10 2010/04/12 18:08:04 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <trd.h>

/*	Macros.								*/

#define	WINDOW_WIDTH		  600 /* default width of window */
#define	WINDOW_HEIGHT		  600 /* default height of window */
#define	ZOOM_FACTOR_MAX		   64 /* maximum magnification factor */
#define	ZOOM_FACTOR_MIN		    1 /* minimum magnification factor */
#define	HALF_DEGREE		  0.5 /* 1/2 degree angle */
#define	TENTH_DEGREE		  0.1 /* 1/10 degree angle */
#define	RAW_MODE		    0 /* interrogation mode */
#define	ZOOM_MODE		    1 /* zoom mode */
#define	MAX_AZIMUTHS		 3600 /* max azimuths allowed in cut */
#define	MAX_ELEVATION_SCANS	   30 /* max cuts allowed in volume */

char	Buf [256]; /* common buffer for string functions. */

/*	Global widget variables.					*/

Widget	Zoom_button       = (Widget) NULL;
Widget	Reset_button      = (Widget) NULL;
Widget	Raw_button        = (Widget) NULL;
Widget	Terrain_button    = (Widget) NULL;
Widget	Blockage_button   = (Widget) NULL;
Widget	Map_off_button    = (Widget) NULL;
Widget	Map_on_button     = (Widget) NULL;
Widget	Filter_off_button = (Widget) NULL;
Widget	Filter_on_button  = (Widget) NULL;
Widget	Grid_off_button   = (Widget) NULL;
Widget	Grid_on_button    = (Widget) NULL;
Widget	Apply_button      = (Widget) NULL;
Widget	Button [32];
Widget	Azran_label;
Widget	Value_label;
Widget	Azran_data;
Widget	Value_data;
Widget	Left_button;
Widget	Middle_button;
Widget	Right_button;
Widget	Control_form;

Widget	Data_range_dialog     = (Widget) NULL;
Widget	Terrain_min_scale     = (Widget) NULL;
Widget	Terrain_max_scale     = (Widget) NULL;
Widget	Blockage_min_scale    = (Widget) NULL;
Widget	Blockage_max_scale    = (Widget) NULL;

Widget	Blockage_combo_rowcol = (Widget) NULL;
Widget	Blockage_list         = (Widget) NULL;

float	Azint = TENTH_DEGREE; /* 1/0 azimuth interval for painting */

float	Terrain_min        = TERRAIN_MIN;  /* Minimum height to display 
					        (meters) */
float	Terrain_max        = TERRAIN_MAX;  /* Maximum height to display
					        (meters) */
float	Blockage_min       = BLOCKAGE_MIN; /* Minimum blockage to display
						(1/10 percent) */
float	Blockage_max       = BLOCKAGE_MAX; /* Maximum blockage to display
						(1/10 percent) */
int	Grid_overlay_flag  = 1;	/* Used to automatically display polar	*
				 * grid whan radar data updated.	*/
int	Map_overlay_flag   = 0; /* Used to automatically display maps	*
				 * when radar data updated.		*/
int	Display_mode       = RAW_MODE;

/*	Translation table for data display window.  We need a mechanism	*
 *	to capture mouse events for interrogation and zooming.		*/

String	Translations =
       "<Btn1Down>:   trd_draw(down1)   ManagerGadgetArm() \n\
	<Btn1Up>:     trd_draw(up1)     ManagerGadgetActivate() \n\
	<Btn1Motion>: trd_draw(motion1) ManagerGadgetButtonMotion() \n\
	<Btn2Down>:   trd_draw(down2)   ManagerGadgetArm() \n\
	<Btn3Down>:   trd_draw(down3)   ManagerGadgetArm()";

/*	Global variables						*/

Display		*TRD_display; /* Display info */
Widget		TRD_dialog;   /* Top level window */
Widget		TRD_label;    /* Elevation/Time label at top of window */
Widget		TRD_min_label;/* Data minimum value label */
Widget		TRD_max_label;/* Data minimum value label */
Dimension	TRD_width;    /* width of display region */
Dimension	TRD_height;   /* height of display region */
Dimension	TRD_xsn_width = MAX_BINS_ALONG_RADIAL;
			      /* width of xsn display region */
Dimension	TRD_xsn_height = 150;   /* height of xsn display region */
Dimension	TRD_depth;    /* depth (bits) of display */
Widget		TRD_draw_widget; /* drawing area widget for data */
Widget		TRD_xsn_widget; /* drawing area widget for xsn data */
Window		TRD_window;   /* visible Drawable */
Pixmap		TRD_pixmap;   /* invisible Drawable */
Window		TRD_xsn_window;   /* visible Drawable */
Pixmap		TRD_xsn_pixmap;   /* invisible Drawable */
GC		TRD_gc;       /* graphics context */
float		TRD_color_scale; /* scale for converting data to color index */
int		TRD_lb;        /* TRD LB ID */
Colormap	TRD_cmap;              /* colormap */
int		TRD_zoom_factor = 1;   /* current zoom factor (1 to 64) */
char		*TRD_data  = (char *) NULL; /* Pointer to radial data */
int		TRD_data_type = TERRAIN; /* TERRAIN or BLOCKAGE */
float		TRD_display_range  = (float) MAX_RADIAL_RANGE;
				/* Maximum radial range (km) */
int		TRD_num_levels     = 0; /* Number of blockage levels */
int		TRD_blockage_level = 0; /* Active blockage level */
float		Azimuth = 0.0;

Gui_radial_t	TRD_radial;	/* Structure containing radial data
				   properties for the display module. */
Gui_color_t	TRD_color;	/* Structure containing color control
				   data for the display module. */

int		Update_flag = 0; /* Data update flag */

int Terrain_LBfd;

int Blockage_LBfd;

char Input_file_path[255] = "";

void	trd_expose_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_close_callback  (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_data_range_callback     (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_filter_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_grid_callback   (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_map_callback    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_display_mode    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_type_select_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_select_blockage_level_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_xsn_expose_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_xsn_resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	trd_reset_zoom_callback (Widget w, XtPointer client_data,
			XtPointer call_data);

void	trd_draw (Widget w, XEvent *event, String *args,
		  int *num_args);

float	hci_find_azimuth (int pixel1, int scanl1, int pixel2, int scanl2);

void	trd_display ();

void	trd_color_bar ();
void	trd_overlay_grid ();
void	trd_overlay_selected_radial ();

void	trd_define_colors (Pixel *color);
void	timer_proc();

int	hci_find_best_color (Display *display, Colormap cmap, XColor *color);

static int Open_LBs();
static int Read_and_decompress_data( int LB_fd, LB_id_t msg_id, char **out );
static int option_cb( char, char * );
static void print_usage();

/************************************************************************
 *	Description: This is the main function for the HCI Terrain	*
 *		     Data Display task.					*
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
	Widget		form;
	Widget		frame;
	Widget		options_frame;
	Widget		options_form;
	Widget		button;
	Widget		label;
	Widget		close_rowcol;
	Widget		filter_rowcol;
	Widget		grid_rowcol;
	Widget		map_rowcol;
	Widget		mode_rowcol;
	Widget		field_rowcol;
	Widget		field_list;
	Widget		filter_list;
	Widget		grid_list;
	Widget		map_list;
	Widget		mode_list;
	Widget		properties_form;
	Widget		properties_rowcol;
	Widget		button_rowcol;
	Widget		blockage_combo;
	Widget		minmax_rowcol;
	XGCValues	gcv;
	XtActionsRec	actions;
	Arg		arg [10];
	int		i,j;
	int		n;
	char		title[80];
	int		len;
	XmString	str;
	short		min_range = 0;
	short		max_range = 0;
	short		min_radial = 0;
	short		max_radial = 0;
	short		*sdata;
	short		min_value = 9999;
	short		max_value = -9999;

/*	Initialize custom options.					*/
	HCI_set_custom_args( "p:", option_cb, print_usage );
/*	Initialize HCI.							*/
	HCI_partial_init( argc, argv, HCI_TRD_TOOL );

	TRD_dialog = HCI_get_top_widget();

	TRD_display = HCI_get_display();

	TRD_cmap = HCI_get_colormap();

	TRD_depth   = XDefaultDepth (TRD_display,
		DefaultScreen (TRD_display));

/*	Set various window properties.					*/

	XtVaSetValues (TRD_dialog,
		XmNminWidth,		250,
		XmNminHeight,		250,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/* 	Open LBs for reading. */
	if( Open_LBs() < 0 )
	{
	   HCI_LE_error("Failed to open LBs");
           HCI_task_exit(HCI_EXIT_FAIL);
	}

/*	Initialize the data buffer by reading terrain data.		*/

        TRD_data = NULL;
        len = Read_and_decompress_data( Terrain_LBfd, 1, &TRD_data ); 

/*	Initialize the data buffer by openning the terrain data file	*
 *	and determining the data ranges in the file.			*/

	    sdata = (short *) TRD_data;

	    for (i=0;i<3600;i++) {

	        for (j=0;j<MAX_BINS_ALONG_RADIAL;j++) {

		    if (sdata [i*MAX_BINS_ALONG_RADIAL+j] > max_value) {

			max_value = sdata [i*MAX_BINS_ALONG_RADIAL+j];
			max_range = j;
			max_radial = i;

		    } else if (sdata [i*MAX_BINS_ALONG_RADIAL+j] < min_value) {

			min_value = sdata [i*MAX_BINS_ALONG_RADIAL+j];
			min_range = j;
			min_radial = i;

		    }
	        }
	    }

	    Terrain_min = (float) min_value;
	    Terrain_max = (float) max_value;
	    TRD_color.min_value = (float) min_value;
	    TRD_color.max_value = (float) max_value;

/*	Make the initial size of the terrain display window something	*
 *	reasonable. The user can resize it as needed. Define a minimum  *
 *	size to be able to hold all of the widgets.			*/
/*	NOTE:  This is something which should eventually get put into	*
 *	a resource file so it can be configured at startup.		*/

	TRD_width  = WINDOW_WIDTH; 
	TRD_height = WINDOW_HEIGHT;

/*	Create the form widget which will manage the drawing_area and	*
 *	row_column widgets.						*/

	form = XtVaCreateWidget ("form",
		xmFormWidgetClass,	TRD_dialog,
		NULL);

	sprintf (title,"Terrain Data (1 meter resolution)");
	str = XmStringCreateLocalized (title);

	TRD_label = XtVaCreateManagedWidget ("TRD Label",
		xmLabelWidgetClass,	form,
		XmNlabelString,		str,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

/*	Use another form widget to manage all of the control buttons	*
 *	on the left side of the basedata display window.		*/

	Control_form = XtVaCreateWidget ("control_form",
		xmFormWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		TRD_label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	frame = XtVaCreateManagedWidget ("close_frame",
		xmFrameWidgetClass,	Control_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	close_rowcol = XtVaCreateWidget ("close_rowcol",
		xmRowColumnWidgetClass,	frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	close_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, trd_close_callback,
		NULL);

	XtManageChild (close_rowcol);

	label = XtVaCreateManagedWidget ("space",
		xmLabelWidgetClass,	Control_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		close_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	button = XtVaCreateManagedWidget ("Data Range",
		xmPushButtonWidgetClass,	Control_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, trd_data_range_callback,
		(XtPointer) NULL);

/*	Create the row_column widget which will hold the data filter	*
 *	selection buttons.						*/ 

	filter_rowcol = XtVaCreateWidget ("filter_rowcol",
		xmRowColumnWidgetClass,	Control_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		button,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

/*	Create a Label for each set of buttons and the buttons		*
 *	themselves.							*/

	XtVaCreateManagedWidget ("Filter: ",
		xmLabelWidgetClass,	filter_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);            n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);            n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;

	filter_list = XmCreateRadioBox (filter_rowcol,
		"filter_list", arg, n);

	Filter_on_button = XtVaCreateManagedWidget (" On ",
		xmToggleButtonWidgetClass,	filter_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNset,			True,
		NULL);

	XtAddCallback (Filter_on_button,
		XmNvalueChangedCallback, trd_filter_callback,
		(XtPointer) 0);

	Filter_off_button = XtVaCreateManagedWidget ("Off ",
		xmToggleButtonWidgetClass,	filter_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Filter_off_button,
		XmNvalueChangedCallback, trd_filter_callback,
		(XtPointer) 3);

	XtManageChild (filter_list);
	XtManageChild (filter_rowcol);

	label = XtVaCreateManagedWidget ("space",
		xmLabelWidgetClass,	Control_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		filter_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	options_frame = XtVaCreateManagedWidget ("options_frame",
		xmFrameWidgetClass,	Control_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		NULL);

	options_form = XtVaCreateWidget ("options_form",
		xmFormWidgetClass,		options_frame,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNverticalSpacing,		1,
		NULL);

/*	Create the row_column widget which will hold the field 		*
 *	selection buttons.						*/ 

	field_rowcol = XtVaCreateWidget ("field_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

/*	Create a Label for each set of buttons and the buttons		*
 *	themselves.							*/

	XtVaCreateManagedWidget ("Type:  ",
		xmLabelWidgetClass,	field_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);            n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);            n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;

	field_list = XmCreateRadioBox (field_rowcol,
		"field_list", arg, n);

	Terrain_button = XtVaCreateManagedWidget ("Terrain   ",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNset,			True,
		NULL);

	XtAddCallback (Terrain_button,
		XmNvalueChangedCallback, trd_type_select_callback,
		(XtPointer) TERRAIN);

	Blockage_button = XtVaCreateManagedWidget ("Blockage ",
		xmToggleButtonWidgetClass,	field_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNset,			False,
		NULL);

	XtAddCallback (Blockage_button,
		XmNvalueChangedCallback, trd_type_select_callback,
		(XtPointer) BLOCKAGE);

	XtManageChild (field_list);
	XtManageChild (field_rowcol);

/*	Create the row_column widget which will hold the grid overlay	*
 *	selection buttons.						*/ 

	grid_rowcol = XtVaCreateWidget ("grid_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		field_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

/*	Create a Label for each set of buttons and the buttons		*
 *	themselves.							*/

	XtVaCreateManagedWidget ("Grid:  ",
		xmLabelWidgetClass,	grid_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);            n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);            n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;

	grid_list = XmCreateRadioBox (grid_rowcol,
		"grid_list", arg, n);

	Grid_on_button = XtVaCreateManagedWidget (" On ",
		xmToggleButtonWidgetClass,	grid_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNset,			True,
		NULL);

	XtAddCallback (Grid_on_button,
		XmNvalueChangedCallback, trd_grid_callback,
		(XtPointer) 1);

	Grid_off_button = XtVaCreateManagedWidget ("Off ",
		xmToggleButtonWidgetClass,	grid_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Grid_off_button,
		XmNvalueChangedCallback, trd_grid_callback,
		(XtPointer) 0);

	XtManageChild (grid_list);
	XtManageChild (grid_rowcol);

/*	Create the row_column widget which will hold the map overlay	*
 *	selection buttons.						*/ 

	map_rowcol = XtVaCreateWidget ("map_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		grid_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	XtVaCreateManagedWidget ("Map:   ",
		xmLabelWidgetClass,	map_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);            n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);            n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;

	map_list = XmCreateRadioBox (map_rowcol,
		"map_list", arg, n);

	Map_on_button = XtVaCreateManagedWidget (" On ",
		xmToggleButtonWidgetClass,	map_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Map_on_button,
		XmNvalueChangedCallback, trd_map_callback,
		(XtPointer) 1);

	Map_off_button = XtVaCreateManagedWidget ("Off ",
		xmToggleButtonWidgetClass,	map_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Map_on_button,
		XmNvalueChangedCallback, trd_map_callback,
		(XtPointer) 1);

	XtManageChild (map_list);
	XtManageChild (map_rowcol);

/*	Create the row_column widget which will hold the mode		*
 *	selection buttons.						*/ 

	mode_rowcol = XtVaCreateWidget ("mode_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		map_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	XtVaCreateManagedWidget ("Mode:  ",
		xmLabelWidgetClass,	mode_rowcol,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNmarginHeight,	1);            n++;
	XtSetArg (arg [n], XmNmarginWidth,	1);            n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;

	mode_list = XmCreateRadioBox (mode_rowcol,
		"mode_list", arg, n);

	Raw_button = XtVaCreateManagedWidget ("Raw ",
		xmToggleButtonWidgetClass,	mode_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNset,			True,
		NULL);

	XtAddCallback (Raw_button,
		XmNvalueChangedCallback, trd_display_mode,
		(XtPointer) RAW_MODE);

	Zoom_button = XtVaCreateManagedWidget ("Zoom ",
		xmToggleButtonWidgetClass,	mode_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Zoom_button,
		XmNvalueChangedCallback, trd_display_mode,
		(XtPointer) ZOOM_MODE);

	Reset_button = XtVaCreateManagedWidget ("Reset",
		xmPushButtonWidgetClass,	mode_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Reset_button,
		XmNactivateCallback, trd_reset_zoom_callback, NULL);

	XtManageChild (mode_list);
	XtManageChild (mode_rowcol);

/*	Create the row_column widget which will hold the blockage	*
 *	selection buttons.						*/ 

	Blockage_combo_rowcol = XtVaCreateWidget ("Blockage_combo_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		mode_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	XtVaCreateManagedWidget ("Blockage Level:  ",
		xmLabelWidgetClass,	Blockage_combo_rowcol,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	blockage_combo = XtVaCreateWidget ("blockage_combo",
		xmComboBoxWidgetClass,	Blockage_combo_rowcol,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,	hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNwidth,		85,
		XmNvisibleItemCount,	10,
		NULL);

	XtVaGetValues (blockage_combo,
		XmNlist,	&Blockage_list,
		NULL);

	for (i=0;i<TRD_num_levels;i++) {

	    sprintf (title,"%4.1f", (i-10)/10.0);
	    str = XmStringCreateLocalized (title);
	    XmListAddItemUnselected (Blockage_list, str, 0);
	    XmStringFree (str);

	}

	XtManageChild (blockage_combo);
	XtManageChild (Blockage_combo_rowcol);

	minmax_rowcol = XtVaCreateWidget ("minmax_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Blockage_combo_rowcol,
		XmNorientation,		XmVERTICAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	sprintf (title,"Min: %d ft at %5.1f deg,%d nm",
		(int) (Terrain_min*3.28+0.5), min_radial/10.0,
		(int) (min_range*HCI_KM_TO_NM+0.5));
	str = XmStringCreateLocalized (title);

	TRD_min_label = XtVaCreateManagedWidget ("Min Value:",
		xmLabelWidgetClass,	minmax_rowcol,
		XmNlabelString,		str,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
	XmStringFree (str);

	sprintf (title,"Max: %d ft at %5.1f deg,%d nm",
		(int) (Terrain_max*3.28+0.5), max_radial/10.0,
		(int) (max_range*HCI_KM_TO_NM+0.5));
	str = XmStringCreateLocalized (title);

	TRD_max_label = XtVaCreateManagedWidget ("Max Value",
		xmLabelWidgetClass,	minmax_rowcol,
		XmNlabelString,		str,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
	XmStringFree (str);

	XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	minmax_rowcol,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
	XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	minmax_rowcol,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (minmax_rowcol);

	TRD_xsn_widget = XtVaCreateWidget ("topography_xsn_widget",
		xmDrawingAreaWidgetClass,	options_form,
		XmNwidth,		TRD_xsn_width,
		XmNheight,		TRD_xsn_height,
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		minmax_rowcol,
		NULL);

	XtManageChild (TRD_xsn_widget);
	XtManageChild (options_form);
	XtManageChild (Control_form);

	XtVaSetValues (blockage_combo,
		XmNselectedPosition,	0,
		NULL);

	XtAddCallback (blockage_combo,
		XmNselectionCallback, trd_select_blockage_level_callback,
		NULL);

	if (TRD_data_type != BLOCKAGE) {

	    XtSetSensitive( Blockage_combo_rowcol, False );

	}

/*	Create a widget along the bottom of the window to left of the	*
 *	control buttons which will contain various data attributes.	*/

	properties_form = XtVaCreateWidget ("properties_form",
		xmFormWidgetClass,	form,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		Control_form,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	button_rowcol = XtVaCreateWidget ("button_rowcol",
		xmRowColumnWidgetClass,	properties_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		2,
		XmNpacking,		XmPACK_COLUMN,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	XtVaCreateManagedWidget ("Left Button: ",
		xmLabelWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Middle Button: ",
		xmLabelWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtVaCreateManagedWidget ("Right Button: ",
		xmLabelWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Left_button = XtVaCreateManagedWidget ("Interrogate",
		xmLabelWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Middle_button = XtVaCreateManagedWidget ("N/A",
		xmLabelWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Right_button = XtVaCreateManagedWidget ("N/A",
		xmLabelWidgetClass,	button_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
 
	XtManageChild (button_rowcol);

	properties_rowcol = XtVaCreateWidget ("button_rowcol",
		xmRowColumnWidgetClass,	properties_form,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		button_rowcol,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		2,
		XmNpacking,		XmPACK_COLUMN,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	Azran_label = XtVaCreateManagedWidget ("Azran: ",
		xmLabelWidgetClass,	properties_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Value_label = XtVaCreateManagedWidget ("Value: ",
		xmLabelWidgetClass,	properties_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Azran_data = XtVaCreateManagedWidget ("        ",
		xmLabelWidgetClass,	properties_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Value_data = XtVaCreateManagedWidget ("        ",
		xmLabelWidgetClass,	properties_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
 
	XtManageChild (properties_rowcol);
	XtManageChild (properties_form);

/*	Create the drawing_area widget which will be used to display	*
 *	base level radar data.  It will occupy the upper right portion	*
 *	of the form.							*/

	actions.string = "trd_draw";
	actions.proc   = (XtActionProc) trd_draw;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	TRD_draw_widget = XtVaCreateWidget ("topography_drawing_area",
		xmDrawingAreaWidgetClass,	form,
		XmNwidth,		TRD_width,
		XmNheight,		TRD_height,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		TRD_label,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		Control_form,
		XmNbottomWidget,	properties_form,
		XmNtranslations,	XtParseTranslationTable (Translations),
		NULL);

/*	Add an expose callback for the drawing_area in order to allow	*
 *	holes to be filled in the display when  other windows are moved	*
 *	across it.							*/

	XtAddCallback (TRD_draw_widget,
		XmNexposeCallback, trd_expose_callback, NULL);

/*	Permit the user to resize the terrain data display window.	*/

	XtAddCallback (TRD_draw_widget,
		XmNresizeCallback, trd_resize_callback, NULL);

	XtManageChild (TRD_draw_widget);
	XtManageChild (form);

	XtRealizeWidget (TRD_dialog);

/*	Define the various window variables to be used as arguments in 	*
 *	the various Xlib and Xt calls.					*/

	TRD_window  = XtWindow      (TRD_draw_widget);
	TRD_pixmap  = XCreatePixmap (TRD_display,
				      TRD_window,
				      TRD_width,
				      TRD_height,
				      TRD_depth);
	TRD_xsn_window  = XtWindow  (TRD_xsn_widget);

/*	Define the Graphics Context to be used in drawing data		*/

	gcv.foreground = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
	gcv.background = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
	gcv.graphics_exposures = FALSE;

	TRD_gc = XCreateGC (TRD_display,
	      		     TRD_window,
	      		     GCBackground | GCForeground | GCGraphicsExposures,
			     &gcv);

	XSetFont (TRD_display, TRD_gc, hci_get_font (LIST));

	XtVaSetValues (TRD_draw_widget,
		XmNbackground,	hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
		NULL);

	XtVaSetValues (TRD_xsn_widget,
		XmNbackground,	hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
		NULL);

/*	Clear the data display portion of the window by filling it with	*
 *	the background color.						*/

	XSetForeground (TRD_display, TRD_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (TRD_display,
			TRD_pixmap,
			TRD_gc,
			0,
			0,
			TRD_width,
			TRD_width);

/*	Define the data display window properties.			*/

	TRD_radial.azimuth_width  = 0.50;
	TRD_radial.range_start    = 0.0;
	TRD_radial.range_interval = 1.0;
	TRD_radial.bins           = MAX_BINS_ALONG_RADIAL;
	TRD_radial.center_pixel   = (TRD_width-75)/2;
	TRD_radial.center_scanl   = TRD_height/2;
	TRD_radial.scale_x        = TRD_zoom_factor * (TRD_width-75)/
			   		       (2.0*TRD_display_range);
	TRD_radial.scale_y        = -TRD_radial.scale_x;

	TRD_color.filter          = 0;
/*
	TRD_color.max_value       = TERRAIN_MAX;
	TRD_color.min_value       = TERRAIN_MIN;
*/
	TRD_color.num_colors      = TERRAIN_COLORS;

	trd_define_colors (TRD_color.color);

	TRD_color_scale  = (TRD_color.max_value-TRD_color.min_value)/(TERRAIN_COLORS-1);

	XtRealizeWidget (TRD_dialog);

	XtVaGetValues (TRD_xsn_widget,
		XmNwidth,	&TRD_xsn_width,
		XmNheight,	&TRD_xsn_height,
		NULL);

	TRD_xsn_pixmap  = XCreatePixmap (TRD_display,
				      TRD_xsn_window,
				      TRD_xsn_width,
				      TRD_xsn_height,
				      TRD_depth);

/*	Add an expose callback for the drawing_area in order to allow	*
 *	holes to be filled in the display when  other windows are moved	*
 *	across it.							*/

	XtAddCallback (TRD_xsn_widget,
		XmNexposeCallback, trd_xsn_expose_callback, NULL);

/*	Permit the user to resize the terrain data display window.	*/

	XtAddCallback (TRD_xsn_widget,
		XmNresizeCallback, trd_xsn_resize_callback, NULL);

	trd_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	trd_xsn_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	HCI_start( timer_proc, HCI_HALF_SECOND, RESIZE_HCI ); 

	return 0;
}

/************************************************************************
 *	Description: This function handles the data window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the terrain data pixmap to the	*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_xsn_expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

	XCopyArea (TRD_display,
		   TRD_xsn_pixmap,
		   TRD_xsn_window,
		   TRD_gc,
		   0,
		   0,
		   TRD_xsn_width,
		   TRD_xsn_height,
		   0,
		   0);
}

/************************************************************************
 *	Description: This function handles the data window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the terrain data pixmap to the	*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Set the clip region to the full window, update the window by	*
 *	copying the terrain data pixmap to it to fill in any holes left	*
 *	by any previously overlain window and the restoring the clip	*
 *	region to the terrain data display region only.			*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = TRD_width;
	clip_rectangle.height  = TRD_height;

	XSetClipRectangles (TRD_display,
			    TRD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XCopyArea (TRD_display,
		   TRD_pixmap,
		   TRD_window,
		   TRD_gc,
		   0,
		   0,
		   TRD_width,
		   TRD_height,
		   0,
		   0);

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = TRD_width-75;
	clip_rectangle.height  = TRD_height;

	XSetClipRectangles (TRD_display,
			    TRD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	Highlight the radial that is selected for display in the XSN	*
 *	window.								*/

	trd_overlay_selected_radial ();

}

/************************************************************************
 *	Description: This function handles the terrain window resize	*
 *		     callback by redrawing all terrain data window	*
 *		     components.					*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_xsn_resize_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	pixel;
	int	old_pixel;
	int	scanl;
	int	i,j;
	int	value = 0;
	short	*sdata = NULL;
	unsigned char	*bdata = NULL;
	int	azimuth_index;
	float	scale_x;
	float	scale_y;
	float	color_scale;
	int	color_index;
	char	buf [16];

	if (TRD_data_type == TERRAIN) {

	    sdata = (short *) TRD_data;

	} else {

	    bdata = (unsigned char *) TRD_data;

	}

	XSetForeground (TRD_display,
			TRD_gc,
			hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XFillRectangle (TRD_display,
			TRD_xsn_pixmap,
			TRD_gc,
			0, 0,
			TRD_xsn_width, TRD_xsn_height);

	scale_x = (TRD_xsn_width-20)/MAX_RADIAL_RANGE;
	scale_y = ((float) TRD_xsn_height-20)/
		  (TRD_color.max_value - TRD_color.min_value);
	color_scale = (TRD_color.max_value-TRD_color.min_value)/
		      (TRD_color.num_colors-1);

	azimuth_index = Azimuth*10;

	pixel     = 20;
	old_pixel = 20;

	for (i=0;i<MAX_BINS_ALONG_RADIAL;i++) {

	    pixel = i*scale_x + 20;
	    scanl = (TRD_color.max_value - value)*scale_y;

	    if (TRD_data_type == TERRAIN) {
 
		value = sdata [azimuth_index*MAX_BINS_ALONG_RADIAL + i];

	    } else {
 
		value = bdata [azimuth_index*MAX_BINS_ALONG_RADIAL + i];

	    }

	    color_index = (int) ((value-TRD_color.min_value)/color_scale+1);

	    if (color_index < 0) {

		color_index = 0;

	    } else if (color_index > TRD_color.num_colors-1) {

		color_index = TRD_color.num_colors-1;

	    }

	    XSetForeground (TRD_display,
			    TRD_gc,
			    TRD_color.color [color_index]);

	    for (j=old_pixel+1;j<=pixel;j++) {

	        XDrawLine (TRD_display,
		       TRD_xsn_pixmap,
		       TRD_gc,
		       j, scanl,
		       j, TRD_xsn_height-20);

	    }

	    old_pixel = pixel;

	}

	XSetForeground (TRD_display,
			TRD_gc,
			hci_get_read_color (CYAN));

	sprintf (buf,"Azimuth %4.1f", Azimuth);
	XDrawString (TRD_display,
		     TRD_xsn_pixmap,
		     TRD_gc,
		     25,15,
		     buf,
		     strlen(buf));

	XSetForeground (TRD_display,
			TRD_gc,
			hci_get_read_color (WHITE));

	XDrawLine (TRD_display,
		   TRD_xsn_pixmap,
		   TRD_gc,
		   20, 0,
		   20, TRD_xsn_height-20);

	XDrawLine (TRD_display,
		   TRD_xsn_pixmap,
		   TRD_gc,
		   20, TRD_xsn_height-20,
		   TRD_xsn_width, TRD_xsn_height-20);

	for (i=0;i<125;i=i+50) {

	    pixel = (int) (scale_x*(i/HCI_KM_TO_NM)+0.5)+20;

	    XDrawLine (TRD_display,
		   TRD_xsn_pixmap,
		   TRD_gc,
		   pixel, TRD_xsn_height-20,
		   pixel, TRD_xsn_height-15);

	    sprintf (buf,"%d nm", i);
	    XDrawString (TRD_display,
		     TRD_xsn_pixmap,
		     TRD_gc,
		     pixel-12, TRD_xsn_height-2,
		     buf,
		     strlen(buf));

	}

	for (i=0;i<=(TRD_color.max_value-TRD_color.min_value);i=i+(TRD_color.max_value-TRD_color.min_value)/2) {

	    scanl = (int) (TRD_xsn_height - 20 - scale_y*i+0.5);

	    XDrawLine (TRD_display,
		   TRD_xsn_pixmap,
		   TRD_gc,
		   15, scanl,
		   TRD_xsn_width, scanl);

	}

	trd_xsn_expose_callback ((Widget) NULL,
		(XtPointer) NULL, (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function handles the terrain window resize	*
 *		     callback by redrawing all terrain data window	*
 *		     components.					*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_resize_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Get the new size of the terrain data window.			*/

	if ((TRD_draw_widget == (Widget) NULL)    ||
	    (TRD_display     == (Display *) NULL) ||
	    (TRD_window      == (Window) NULL)    ||
	    (TRD_pixmap      == (Pixmap) NULL)    ||
	    (TRD_gc          == (GC) NULL)) {

	    return;

	}

	XtVaGetValues (TRD_draw_widget,
		XmNwidth,	&TRD_width,
		XmNheight,	&TRD_height,
		NULL);

/*	Destroy the old pixmap since the size has changed and create	*
 *	create a new one.						*/

	if (TRD_pixmap != (Window) NULL) {

	    XFreePixmap (TRD_display, TRD_pixmap);

	}

	TRD_pixmap = XCreatePixmap (TRD_display,
		TRD_window,
		TRD_width,
		TRD_height,
		TRD_depth);

/*	Recompute the scale factors (in pixels/km) and a new window	*
 *	center pixel/scanline coordinate.				*/

	TRD_radial.scale_x      = TRD_zoom_factor * (TRD_width-75)/
					       (2*TRD_display_range);
	TRD_radial.scale_y	= -TRD_radial.scale_x;
	TRD_radial.center_pixel = (TRD_width-75) / 2;
	TRD_radial.center_scanl = TRD_height / 2;

/*	Set the clip region to the entire window so we can create a	*
 *	color bar along the right size.					*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = TRD_width;
	clip_rectangle.height  = TRD_height;

	XSetClipRectangles (TRD_display,
			    TRD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XSetForeground (TRD_display, TRD_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (TRD_display,
			TRD_pixmap,
			TRD_gc,
			0, 0,
			TRD_width,
			TRD_height);

	trd_color_bar ();

/*	Restore the clip window to the data display area only.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = TRD_width-75;
	clip_rectangle.height  = TRD_height;

	XSetClipRectangles (TRD_display,
			    TRD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	trd_display ();

/*	If we want a grid overlay then display it.			*/

	if (Grid_overlay_flag) {

	    trd_overlay_grid ();

	}

/*	Call the expose callback to write the terrain data pixmap to	*
 *	the terrain data window.					*/

	trd_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function draws a color bar along the right	*
 *		     side of the terrain data window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_color_bar ()
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
	clip_rectangle.width   = TRD_width;
	clip_rectangle.height  = TRD_height;

	XSetClipRectangles (TRD_display,
			    TRD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	Clear an area to the right of the terrain data display to place	*
 *	the color bar.							*/

	XSetForeground (TRD_display, 
			TRD_gc,
			hci_get_read_color (BACKGROUND_COLOR1));

	XFillRectangle (TRD_display,
			TRD_window,
			TRD_gc,
			(int) (TRD_width-75),
			0,
			75,
			TRD_height);

	XFillRectangle (TRD_display,
			TRD_pixmap,
			TRD_gc,
			(int) (TRD_width-75),
			0,
			75,
			TRD_height);

/*	Define the width of each color bar box and the height scale	*/

	width = 10;

	xheight = TRD_height/(2.0*(TERRAIN_COLORS-1));
	step = 1;

	height  = xheight+1;

/*	For each color element, create a box and fill it with the color	*/

	for (i=TERRAIN_COLORS-1;i>= step;i=i-step) {

	    scanl = TRD_height/4 + xheight*(i/step-1);

	    XSetForeground (TRD_display,
			    TRD_gc,
			    TRD_color.color [TERRAIN_COLORS-i]);

	    XFillRectangle (TRD_display,
			    TRD_window,
			    TRD_gc,
			    (int) (TRD_width-15),
			    scanl,
			    width,
			    height);

	    XFillRectangle (TRD_display,
			    TRD_pixmap,
			    TRD_gc,
			    (int) (TRD_width-15),
			    scanl,
			    width,
			    height);

	}

	XSetForeground (TRD_display,
			TRD_gc,
			hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XDrawRectangle (TRD_display,
			TRD_window,
			TRD_gc,
			(int) (TRD_width-15),
			(int) (TRD_height/4),
			width,
			(int) (TRD_height/2));

	XDrawRectangle (TRD_display,
			TRD_pixmap,
			TRD_gc,
			(int) (TRD_width-15),
			(int) (TRD_height/4),
			width,
			(int) (TRD_height/2));

/*	Draw text labels to the left of the color bar for the minimin,	*
 *	middle, and maximum values.					*/

	scanl = TRD_height/4 + xheight*(TERRAIN_COLORS-1);

	if (TRD_data_type == TERRAIN) {

	    if (TRD_color.min_value < 0.0) {

		sprintf (text," %d",(int) (TRD_color.min_value*3.28-0.5));

	    } else {

		sprintf (text," %d",(int) (TRD_color.min_value*3.28+0.5));

	    }

	} else {

	    sprintf (text," %d",(int) TRD_color.min_value);

	}

	XDrawString (TRD_display,
		     TRD_window,
		     TRD_gc,
		     (int) (TRD_width-75),
		     scanl+5,
		     text,
		     strlen (text));

	XDrawString (TRD_display,
		     TRD_pixmap,
		     TRD_gc,
		     (int) (TRD_width-75),
		     scanl+5,
		     text,
		     strlen (text));

	XDrawLine (TRD_display,
		   TRD_window,
		   TRD_gc,
		   (int) (TRD_width-25),
		   scanl,
		   (int) (TRD_width-15),
		   scanl);

	XDrawLine (TRD_display,
		   TRD_pixmap,
		   TRD_gc,
		   (int) (TRD_width-25),
		   scanl,
		   (int) (TRD_width-15),
		   scanl);

	if (TRD_data_type == TERRAIN) {

	    if (TRD_color.max_value < 0.0) {

		sprintf (text," %d",(int) (TRD_color.max_value*3.28-0.5));

	    } else {

		sprintf (text," %d",(int) (TRD_color.max_value*3.28+0.5));

	    }

	} else {

	    sprintf (text," %d",(int) TRD_color.max_value);

	}

	scanl = TRD_height/4;

	XDrawString (TRD_display,
		    TRD_window,
		    TRD_gc,
		    (int) (TRD_width-75),
		    (int) (scanl+5),
		    text,
		    strlen (text));

	XDrawString (TRD_display,
		    TRD_pixmap,
		    TRD_gc,
		    (int) (TRD_width-75),
		    (int) (scanl+5),
		    text,
		    strlen (text));

	XDrawLine (TRD_display,
		   TRD_window,
		   TRD_gc,
		   (int) (TRD_width-25),
		   scanl,
		   (int) (TRD_width-15),
		   scanl);

	XDrawLine (TRD_display,
		   TRD_pixmap,
		   TRD_gc,
		   (int) (TRD_width-25),
		   scanl,
		   (int) (TRD_width-15),
		   scanl);


	scanl = TRD_height/4 + xheight*(TERRAIN_COLORS-1)/2;

	if (TRD_data_type == TERRAIN) {

	    if ((TRD_color.max_value+TRD_color.min_value) < 0.0) {

		sprintf (text," %d",(int) (3.28*(TRD_color.max_value+TRD_color.min_value)/2-0.5));

	    } else {

		sprintf (text," %d",(int) (3.28*(TRD_color.max_value+TRD_color.min_value)/2+0.5));

	    }

	} else {

	    sprintf (text," %d",(int)(TRD_color.max_value+TRD_color.min_value)/2);

	}

	XDrawString (TRD_display,
		     TRD_window,
		     TRD_gc,
		     (int) (TRD_width-75),
		     (int) (scanl+5),
		     text,
		     strlen (text));

	XDrawString (TRD_display,
		     TRD_pixmap,
		     TRD_gc,
		     (int) (TRD_width-75),
		     (int) (scanl+5),
		     text,
		     strlen (text));

	XDrawLine (TRD_display,
		   TRD_window,
		   TRD_gc,
		   (int) (TRD_width-25),
		   scanl,
		   (int) (TRD_width-15),
		   scanl);

	XDrawLine (TRD_display,
		   TRD_pixmap,
		   TRD_gc,
		   (int) (TRD_width-25),
		   scanl,
		   (int) (TRD_width-15),
		   scanl);

/*	Reset the clip rectangle so that drawing in the terrain data	*
 *	display region will not write over the color bar.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = TRD_width-75;
	clip_rectangle.height  = TRD_height;

	XSetClipRectangles (TRD_display,
			    TRD_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being displayed in the terrain display	*
 *		     window.						*
 *									*
 *	Input:  w - Terrain button ID					*
 *		client_data - TERRAIN or BLOCKAGE			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_type_select_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

	int i,j;
	short	*sdata;
	unsigned char	*bdata;
	int	min_range = 0;
	int	min_radial = 0;
	int	max_range = 0;
	int	max_radial = 0;
	short	min_value = 9999;
	short	max_value = -9999;
	int	len;
	char	buf [80];
	XmString	str;

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if when the button is set.				*/

	if (state->set) {

	    TRD_data_type = (int) client_data;

	    switch ((int) client_data) {

		case TERRAIN :

		    HCI_LE_log("Terrain selected" );

		    XtSetSensitive( Blockage_combo_rowcol, False );

/*		    Reset the current field, range, min, and max	*
 *		    fields.						*/

		    free (TRD_data);
                    TRD_data = NULL;

		    len = Read_and_decompress_data( Terrain_LBfd, 1, &TRD_data );

		    if (len <= 0) {

			HCI_LE_error("Read_and_decompress_data() Failed: %d", len );
			HCI_task_exit (HCI_EXIT_FAIL);

		    }

		    sdata = (short *) TRD_data;

		    for (i=0;i<3600;i++) {

			for (j=0;j<MAX_BINS_ALONG_RADIAL;j++) {

			    if (sdata [i*MAX_BINS_ALONG_RADIAL+j] > max_value) {

			        max_value = sdata [i*MAX_BINS_ALONG_RADIAL+j];
			        max_range = j;
			        max_radial = i;

			    } else if (sdata [i*MAX_BINS_ALONG_RADIAL+j] < min_value) {

			        min_value = sdata [i*MAX_BINS_ALONG_RADIAL+j];
			        min_range = j;
			        min_radial = i;

			    }
		        }
		    }

		    TRD_color.min_value = (float) min_value;
		    TRD_color.max_value = (float) max_value;
		    Terrain_min = (float) min_value;
		    Terrain_max = (float) max_value;

		    sprintf (buf,"Terrain Data (1 meter resolution)");
		    str = XmStringCreateLocalized (buf);
		    XtVaSetValues (TRD_label,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

	 	    sprintf (buf,"Min: %d ft at %5.1f deg,%d nm",
			(int)(Terrain_min*3.28+0.5), ((float) min_radial)/10.0,
			(int) (min_range*HCI_KM_TO_NM+0.5));

		    str = XmStringCreateLocalized (buf);
		    XtVaSetValues (TRD_min_label,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (buf,"Max: %d ft at %5.1f deg,%d nm",
			(int)(Terrain_max*3.28+0.5), ((float) max_radial)/10.0,
			(int) (max_range*HCI_KM_TO_NM+0.5));

		    str = XmStringCreateLocalized (buf);
		    XtVaSetValues (TRD_max_label,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    break;

		case BLOCKAGE :

		    HCI_LE_log("Blockage selected");
		    XtSetSensitive( Blockage_combo_rowcol, True );

/*		    Reset the current field, range, min, and max	*
 *		    fields.						*/

		    free (TRD_data);
                    TRD_data = NULL;

		    len = Read_and_decompress_data( Blockage_LBfd, TRD_blockage_level, &TRD_data );

		    if (len <= 0) {

			HCI_LE_error("Read_and_decompress_data() Failed: %d", len );
			HCI_task_exit (HCI_EXIT_FAIL);

		    }

		    bdata = (unsigned char *) TRD_data;

		    for (i=0;i<3600;i++) {

		        for (j=0;j<MAX_BINS_ALONG_RADIAL;j++) {

			    if (bdata [i*MAX_BINS_ALONG_RADIAL+j] > max_value) {

			        max_value = bdata [i*MAX_BINS_ALONG_RADIAL+j];
			        max_range = j;
			        max_radial = i;

			    } else if (bdata [i*MAX_BINS_ALONG_RADIAL+j] < min_value) {

			        min_value = bdata [i*MAX_BINS_ALONG_RADIAL+j];
			        min_range = j;
			        min_radial = i;

			    }
		        }
		    }

		    TRD_color.min_value = 0;
		    TRD_color.max_value = 100;

		    sprintf (buf,"Blockage Data (%%)");
		    str = XmStringCreateLocalized (buf);
		    XtVaSetValues (TRD_label,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

	 	    sprintf (buf,"Min: %d %% at %5.1f deg,%d nm",
			(int) min_value, ((float) min_radial)/10.0,
			(int) (min_range*HCI_KM_TO_NM+0.5));

		    str = XmStringCreateLocalized (buf);
		    XtVaSetValues (TRD_min_label,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (buf,"Max: %d %% at %5.1f deg,%d nm",
			(int) max_value, ((float) max_radial)/10.0,
			(int) (max_range*HCI_KM_TO_NM+0.5));

		    str = XmStringCreateLocalized (buf);
		    XtVaSetValues (TRD_max_label,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

	    }

	    TRD_color_scale = (TRD_color.max_value -
				TRD_color.min_value)/
				(TERRAIN_COLORS-1);

	    trd_display ();


/*	    Redisplay the color bar for the new field.			*/

	    trd_color_bar ();

/*	    Display the polar grid if enabled.				*/

	    if (Grid_overlay_flag) {

	        trd_overlay_grid ();

	    }

	    trd_xsn_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

/*	    Directly invoke the expose callback to make the changes	*
 *	    visible.							*/

	    trd_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function displays the terrain or blockage	*
 *		     data.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_display ()
{

	int	i;

/*	Clear the contents of the terrain data window.			*/

	XSetForeground (TRD_display, TRD_gc,
		   hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XFillRectangle (TRD_display, TRD_pixmap, TRD_gc,
			0, 0, TRD_width, TRD_height);

	trd_expose_callback ((Widget) NULL,
				     (XtPointer) NULL,
				     (XtPointer) NULL);

	if (Grid_overlay_flag) {

	    trd_overlay_grid ();

	}

	if (TRD_data == NULL) {

	    return;

	}

	for (i=0;i<3600;i++) {

	    TRD_radial.azimuth = i/10.0;

	    if (TRD_data_type == TERRAIN) {

		Gui_display_radial_data (&TRD_data[i*TRD_radial.bins*2],
				     DATA_TYPE_SHORT,
				     TRD_display,
				     TRD_pixmap,
				     TRD_gc,
				     &TRD_radial,
				     &TRD_color);

	    } else {

		Gui_display_radial_data (&TRD_data[i*TRD_radial.bins],
				     DATA_TYPE_BYTE,
				     TRD_display,
				     TRD_pixmap,
				     TRD_gc,
				     &TRD_radial,
				     &TRD_color);

	    }
	}

/*	display a polar grid if enabled.				*/

	if (Grid_overlay_flag) {

	    trd_overlay_grid ();

	}

/*	Invoke the expose callback to make changes visible.		*/

	trd_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function handles all mouse button events	*
 *		     inside the terrain data display region.		*
 *									*
 *	Input:  w - drawing area widget ID				*
 *		event - X event data					*
 *		args - number of user argunments			*
 *		num_args - user arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_draw (
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
	float	range;
	float	value;
	short	*sdata;
	unsigned char	*bdata;
	int	azimuth_index;
	XmString	string;

	value = 0.0;

	if (!strcmp (args[0], "down1")) {

	    HCI_LE_log("Terrain Data left button pushed");

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

		TRD_zoom_factor = TRD_zoom_factor*2;

		if (TRD_zoom_factor > ZOOM_FACTOR_MAX) {

		    TRD_zoom_factor = ZOOM_FACTOR_MAX;
		    return;

		}

/*		Adjust the azimuth width to reduce specking	*/

		if (TRD_zoom_factor == 1) {

		    TRD_radial.azimuth_width = 0.5;

		} else if (TRD_zoom_factor == 2) {

		    TRD_radial.azimuth_width = 0.4;

		} else if (TRD_zoom_factor == 4) {

		    TRD_radial.azimuth_width = 0.25;

		} else {

		    TRD_radial.azimuth_width = 0.15;

		}

		TRD_radial.x_offset = (TRD_radial.center_pixel-first_pixel)/
				TRD_radial.scale_x + TRD_radial.x_offset;
		TRD_radial.y_offset = (TRD_radial.center_scanl-first_scanl)/
				TRD_radial.scale_y + TRD_radial.y_offset;

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (TRD_radial.center_pixel +
				         TRD_radial.x_offset*TRD_radial.scale_x),
				  (int) (TRD_radial.center_scanl +
				         TRD_radial.y_offset*TRD_radial.scale_y));

		x1 = TRD_radial.x_offset;
		y1 = TRD_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > TRD_display_range) {

		   TRD_radial.x_offset = (TRD_radial.x_offset/range)*TRD_display_range;
		   TRD_radial.y_offset = (TRD_radial.y_offset/range)*TRD_display_range;

		}

		TRD_radial.scale_x  = TRD_zoom_factor * (TRD_width-75)/
						   (2*TRD_display_range);
		TRD_radial.scale_y  = - TRD_zoom_factor * (TRD_width-75)/
						   (2*TRD_display_range);

	        trd_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    } else {

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (TRD_radial.center_pixel +
				         TRD_radial.x_offset*TRD_radial.scale_x),
				  (int) (TRD_radial.center_scanl +
				         TRD_radial.y_offset*TRD_radial.scale_y));

		x1 = (TRD_radial.center_pixel-event->xbutton.x)/
		     TRD_radial.scale_x + TRD_radial.x_offset;
		y1 = (TRD_radial.center_scanl-event->xbutton.y)/
		     TRD_radial.scale_y + TRD_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		sprintf (Buf,"%3d deg, %3d nm ",
			(int) Azimuth, (int) (range*HCI_KM_TO_NM+0.5));

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Azran_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

		azimuth_index = (int) (Azimuth*10+0.5);

		if (TRD_data_type == TERRAIN) {

		    sdata = (short *) TRD_data;

		    value = sdata [azimuth_index*MAX_BINS_ALONG_RADIAL + (int) range];

		    sprintf (Buf,"%3d m (%d ft)",
			(int) value, (int) (value*3.28));

		} else {

		    bdata = (unsigned char *) TRD_data;

		    value = bdata [azimuth_index*MAX_BINS_ALONG_RADIAL + (int) range];

		    sprintf (Buf,"%d %%", (int) value);

		}

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Value_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

	        trd_xsn_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

		trd_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    }

 	} else if (!strcmp (args[0], "up1")) {

/*	If the left mouse button is released, stop the rubber-banding	*
 *	code and unset the button down flag.				*/

	    HCI_LE_log("Terrain Data left button released");

	    button_down  = 0;

	} else if (!strcmp (args[0], "motion1")) {

/*	If the cursor is being dragged with the left mouse button	*
 *	down, repeatedly draw the sector defined by the point where	*
 *	the button was pressed and where it currently is.  Use the	*
 *	left hand rule for determining the direction in which to draw	*
 *	the sector.							*/

	} else if (!strcmp (args[0], "down2")) {

	    HCI_LE_log("Terrain Data middle button pushed");

	    if (Display_mode == ZOOM_MODE) {

	        first_pixel = event->xbutton.x;
	        first_scanl = event->xbutton.y;

	        TRD_radial.x_offset = (TRD_radial.center_pixel-first_pixel)/
				TRD_radial.scale_x + TRD_radial.x_offset;
	        TRD_radial.y_offset = (TRD_radial.center_scanl-first_scanl)/
				TRD_radial.scale_y + TRD_radial.y_offset;

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (TRD_radial.center_pixel +
				         TRD_radial.x_offset*TRD_radial.scale_x),
				  (int) (TRD_radial.center_scanl +
				         TRD_radial.y_offset*TRD_radial.scale_y));

		x1 = TRD_radial.x_offset;
		y1 = TRD_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > TRD_display_range) {

		   TRD_radial.x_offset = (TRD_radial.x_offset/range)*TRD_display_range;
		   TRD_radial.y_offset = (TRD_radial.y_offset/range)*TRD_display_range;

		}

	        trd_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    }

	} else if (!strcmp (args[0], "down3")) {

	    HCI_LE_log("Terrain Data right button pushed");

	    if (Display_mode == ZOOM_MODE) {

	        first_pixel = event->xbutton.x;
	        first_scanl = event->xbutton.y;

	        TRD_zoom_factor = TRD_zoom_factor/2;

	        if (TRD_zoom_factor < ZOOM_FACTOR_MIN) {

	            TRD_zoom_factor = ZOOM_FACTOR_MIN;
	            return;

	        }

/*		Adjust the azimuth width to reduce specking	*/

		if (TRD_zoom_factor == 1) {

		    TRD_radial.azimuth_width = 0.5;

		} else if (TRD_zoom_factor == 2) {

		    TRD_radial.azimuth_width = 0.4;

		} else if (TRD_zoom_factor == 4) {

		    TRD_radial.azimuth_width = 0.25;

		} else {

		    TRD_radial.azimuth_width = 0.15;

		}

	        TRD_radial.x_offset = (TRD_radial.center_pixel-first_pixel)/
				TRD_radial.scale_x + TRD_radial.x_offset;
	        TRD_radial.y_offset = (TRD_radial.center_scanl-first_scanl)/
				TRD_radial.scale_y + TRD_radial.y_offset;
	        TRD_radial.scale_x  = TRD_zoom_factor * (TRD_width-75)/
			        (2*TRD_display_range);
	        TRD_radial.scale_y  = - TRD_zoom_factor * (TRD_width-75)/
			        (2*TRD_display_range);

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (TRD_radial.center_pixel +
				         TRD_radial.x_offset*TRD_radial.scale_x),
				  (int) (TRD_radial.center_scanl +
				         TRD_radial.y_offset*TRD_radial.scale_y));

		x1 = TRD_radial.x_offset;
		y1 = TRD_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > TRD_display_range) {

		   TRD_radial.x_offset = (TRD_radial.x_offset/range)*TRD_display_range;
		   TRD_radial.y_offset = (TRD_radial.y_offset/range)*TRD_display_range;

		}

	        trd_resize_callback ((Widget) NULL,
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
 *	Description: This function resets the Zoom and center.		*
 *									*
 *	Input:  w - Reset button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_reset_zoom_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

	TRD_zoom_factor = 1;

	TRD_radial.azimuth_width = 0.5;

	TRD_radial.x_offset = 0.0;
	TRD_radial.y_offset = 0.0;

	TRD_radial.scale_x  = TRD_zoom_factor * (TRD_width-75)/
					   (2*TRD_display_range);
	TRD_radial.scale_y  = - TRD_zoom_factor * (TRD_width-75)/
					   (2*TRD_display_range);

	trd_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

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
trd_display_mode (
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
		    XtVaSetValues (Value_label,
			XmNforeground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

		    sprintf (string," ");
		    str = XmStringCreateLocalized (string);
		    XtVaSetValues (Azran_data,
			XmNlabelString,	str,
			NULL);
		    XmStringFree (str);

		    sprintf (string," ");
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
trd_data_range_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	form;
	Widget	rowcol;
	Widget	button;
	Widget	terrain_frame;
	Widget	blockage_frame;
	Widget	label;
	char	buf [16];
	XmString	str;

	void	hci_change_data_range_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_new_data_range_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	hci_close_data_range_dialog_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	HCI_LE_log("Change data range selected");

/*	If the window already exists do nothing and return.		*/

	if (Data_range_dialog != NULL)
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

/*	Creatye a set of slider bars to control the terrain data	*
 *	range.								*/

	terrain_frame = XtVaCreateWidget ("terrain_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	terrain_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Terrain_min_scale = XtVaCreateWidget ("terrain_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Terrain Height (meters)", 32,
		XmNshowValue,		True,
		XmNminimum,		(int) TERRAIN_MIN,
		XmNmaximum,		(int) TERRAIN_MAX,
		XmNvalue,		(int) Terrain_min,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtAddCallback (Terrain_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 0);

	sprintf (buf,"%d",(int) TERRAIN_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Terrain_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) TERRAIN_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Terrain_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	XtManageChild (Terrain_min_scale);

	Terrain_max_scale = XtVaCreateWidget ("terrain_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Terrain Height (meters)", 32,
		XmNshowValue,		True,
		XmNminimum,		(int) TERRAIN_MIN,
		XmNmaximum,		(int) TERRAIN_MAX,
		XmNvalue,		(int) Terrain_max,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtAddCallback (Terrain_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 1);

	sprintf (buf,"%d",(int) TERRAIN_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Terrain_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) TERRAIN_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Terrain_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	XtManageChild (Terrain_max_scale);
	XtManageChild (rowcol);
	XtManageChild (terrain_frame);

/*	Creatye a set of slider bars to control the blockage data	*
 *	range.								*/

	blockage_frame = XtVaCreateWidget ("blockage_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		terrain_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	blockage_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Blockage_min_scale = XtVaCreateWidget ("blockage_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Blockage (pct)", 23,
		XmNshowValue,		True,
		XmNminimum,		(int) BLOCKAGE_MIN,
		XmNmaximum,		(int) BLOCKAGE_MAX,
		XmNvalue,		(int) Blockage_min,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtAddCallback (Blockage_min_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 2);

	sprintf (buf,"%d",(int) BLOCKAGE_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Blockage_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) BLOCKAGE_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Blockage_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	XtManageChild (Blockage_min_scale);

	Blockage_max_scale = XtVaCreateWidget ("blockage_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Blockage (pct)", 23,
		XmNshowValue,		True,
		XmNminimum,		(int) BLOCKAGE_MIN,
		XmNmaximum,		(int) BLOCKAGE_MAX,
		XmNvalue,		(int) Blockage_max,
		XmNorientation,		XmHORIZONTAL,
		XmNdecimalPoints,	0,
		XmNscaleMultiple,	1,
		XmNwidth,		300,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtAddCallback (Blockage_max_scale,
		XmNvalueChangedCallback,
		hci_new_data_range_callback,
		(XtPointer) 3);

	sprintf (buf,"%d",(int) BLOCKAGE_MIN);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Blockage_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) BLOCKAGE_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Blockage_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XmStringFree (str);

	XtManageChild (Blockage_max_scale);
	XtManageChild (rowcol);
	XtManageChild (blockage_frame);

/*	Define a row of buttons to close the window and to apply the	*
 *	changes.							*/

	rowcol = XtVaCreateWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopWidget,		blockage_frame,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback,
		hci_close_data_range_dialog_callback,
		NULL);

	Apply_button = XtVaCreateManagedWidget ("Apply",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
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
	XtManageChild (form);
	HCI_Shell_start( Data_range_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is activated when the "Close"	*
 *		     button is selected in the Set Data Range window.	*
 *									*
 *	Input:  w - Close button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_close_data_range_dialog_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_Shell_popdown( Data_range_dialog );
}

/************************************************************************
 *	Description: This function is activated when one of the		*
 *		     slider bars is moved	.			*
 *									*
 *	Input:  w - slider bar ID					*
 *		client_data -  0 (terrain minumum)			*
 *			       1 (terrain maximum)			*
 *			       2 (blockage maximum)			*
 *			       3 (blockage maximum)			*
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

	    case 0 :	/*	Terrain Min	*/

		Terrain_min = (float) cbs->value;

		if (Terrain_min >= Terrain_max) {

		    Terrain_min = Terrain_max-1;

		    XtVaSetValues (Terrain_min_scale,
			XmNvalue,	(int) Terrain_min,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 1 :	/*	Terrain Max	*/

		Terrain_max = (float) cbs->value;

		if (Terrain_max <= Terrain_min) {

		    Terrain_max = Terrain_min+1;

		    XtVaSetValues (Terrain_max_scale,
			XmNvalue,	(int) Terrain_max,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 2 :	/*	Blockage Min		*/

		Blockage_min = (float) cbs->value;

		if (Blockage_min >= Blockage_max) {

		    Blockage_min = Blockage_max-1;

		    XtVaSetValues (Blockage_min_scale,
			XmNvalue,	(int) Blockage_min,
			NULL);

		}

		if (!Update_flag) {

		    Update_flag = 1;

		    XtVaSetValues (Apply_button,
			XmNsensitive,	True,
			NULL);

		}

		break;

	    case 3 :	/*	Blockage Max		*/

		Blockage_max = (float) cbs->value;

		if (Blockage_max <= Blockage_min) {

		    Blockage_max = Blockage_min+1;

		    XtVaSetValues (Blockage_max_scale,
			XmNvalue,	(int) Blockage_max,
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
	switch (TRD_data_type) {

	    case TERRAIN :

		TRD_color.min_value = (float) Terrain_min;
		TRD_color.max_value = (float) Terrain_max;
		TRD_color_scale  = (TRD_color.max_value-TRD_color.min_value)/
				    (TERRAIN_COLORS-1);
		break;

	    case BLOCKAGE :

		TRD_color.min_value = (float) Blockage_min;
		TRD_color.max_value = (float) Blockage_max;
		TRD_color_scale  = (TRD_color.max_value-TRD_color.min_value)/
				    (BLOCKAGE_COLORS-1);
		break;

	}

	trd_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

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
trd_map_callback (
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

	    trd_resize_callback ((Widget) NULL,
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
trd_filter_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

/*	Only do this if the button is set.				*/

	if (state->set) {

	    TRD_color.filter = (int) client_data;

/*	    Update the data filter flag (0 = off, 1 = on)		*/

	    trd_resize_callback ((Widget) NULL,
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
trd_grid_callback (
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

	    trd_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function displays a polar grid over a		*
 *		     terrain data display.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_overlay_grid (
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

	XSetForeground (TRD_display,
			TRD_gc,
			hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

/*	Based on the current zoom factor, determine the distancce	*
 *	between polar grid rings (in nautical miles).			*/

	switch (TRD_zoom_factor) {

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

	x = fabs ((double) TRD_radial.x_offset);
	y = fabs ((double) TRD_radial.y_offset);

	if (x > y) {

	    start_range = x - TRD_display_range/TRD_zoom_factor;
	    stop_range  = x + TRD_display_range/TRD_zoom_factor;

	} else {

	    start_range = y - TRD_display_range/TRD_zoom_factor;
	    stop_range  = y + TRD_display_range/TRD_zoom_factor;

	}

	start_range = (start_range/step)*step;
	stop_range  = (stop_range/step)*step + step;

	if (start_range < step) {

	    start_range = step;

	}

	for (i=step;i<=stop_range;i=i+step) {

	    size  = i*TRD_radial.scale_x/HCI_KM_TO_NM;

	    pixel = TRD_radial.center_pixel + TRD_radial.x_offset*TRD_radial.scale_x -
		    size;
	    scanl = TRD_radial.center_scanl + TRD_radial.y_offset*TRD_radial.scale_y -
		    size;

	    XDrawArc (TRD_display,
		      TRD_pixmap,
		      TRD_gc,
		      pixel,
		      scanl,
		      size*2,
		      size*2,
		      0,
		      -(360*64));

	    sprintf (buf,"%i nm",i);

	    XDrawString (TRD_display,
			 TRD_pixmap,
			 TRD_gc,
			 (int) (TRD_radial.center_pixel +
				TRD_radial.x_offset*TRD_radial.scale_x -
				4*strlen (buf)),
			 scanl + 4,
			 buf,
			 strlen (buf));
	}

/*	Next display the "spokes" outward from the center.		*/

	for (i=0;i<360;i=i+step) {

	    pixel = (int) (((1+1.0/TRD_zoom_factor)*TRD_display_range *
		     cos ((double) (i+90)*HCI_DEG_TO_RAD) +
		     TRD_radial.x_offset) * TRD_radial.scale_x +
		     TRD_radial.center_pixel);
	    scanl = (int) (((1+1.0/TRD_zoom_factor)*TRD_display_range *
		     sin ((double) (i-90)*HCI_DEG_TO_RAD) +
		     TRD_radial.y_offset) * TRD_radial.scale_y +
		     TRD_radial.center_scanl);

	    XDrawLine (TRD_display,
		       TRD_pixmap,
		       TRD_gc,
		       (int) (TRD_radial.center_pixel +
			      TRD_radial.x_offset*TRD_radial.scale_x),
		       (int) (TRD_radial.center_scanl +
			      TRD_radial.y_offset*TRD_radial.scale_y),
		       pixel,
		       scanl);

	}

	trd_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function displays the highlighted radial	*
 *		     in the terrain data display window.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_overlay_selected_radial (
)
{
	int	pixel;
	int	scanl;
static	int	flag = 0;

	flag = 1 - flag;

	if (flag) {

	    return;

	}

	XSetForeground (TRD_display,
			TRD_gc,
			hci_get_read_color (BLACK));

	pixel = (int) (((1+1.0/TRD_zoom_factor)*TRD_display_range/2 *
		     cos ((double) (Azimuth-90)*HCI_DEG_TO_RAD) +
		     TRD_radial.x_offset) * TRD_radial.scale_x +
		     TRD_radial.center_pixel);
	scanl = (int) (((1+1.0/TRD_zoom_factor)*TRD_display_range/2 *
		     sin ((double) (Azimuth+90)*HCI_DEG_TO_RAD) +
		     TRD_radial.y_offset) * TRD_radial.scale_y +
		     TRD_radial.center_scanl);

	XDrawLine (TRD_display,
		   TRD_window,
		   TRD_gc,
		   (int) (TRD_radial.center_pixel +
			  TRD_radial.x_offset*TRD_radial.scale_x),
		   (int) (TRD_radial.center_scanl +
			  TRD_radial.y_offset*TRD_radial.scale_y),
		   pixel,
		   scanl);

}

/************************************************************************
 *	Input:  w - parent widget of timer				*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
	trd_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the RPG Terrain Data Display	*
 *		     window.						*
 *									*
 *	Input:  w - Close button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Terrain Data window Close pushed");
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

void
trd_define_colors (
Pixel *color
)
{
	int	status;
	int	i;
	XColor	xcolor;
	int	red;

	for (i=0;i<TERRAIN_COLORS;i++) {

	    red   = (short) (255-i*(2*256/TERRAIN_COLORS));	

	    if (red < 0)
		red = -red;

	    xcolor.green = (short) (i*(2*256/TERRAIN_COLORS));

	    if (xcolor.green > 255)
		xcolor.green = (short) (255 - ((i-32)*512/TERRAIN_COLORS));

	    xcolor.red   = (short) red << 8;
	    xcolor.green = xcolor.green << 8;
	    xcolor.blue  = (short) (255-i*(256/TERRAIN_COLORS)) << 8;

	    status = hci_find_best_color (TRD_display, TRD_cmap, &xcolor);

	    color[i] = xcolor.pixel;
	}
}
/************************************************************************
 *	Description: This function handles blockage level selections	*
 *		     by updating the TRD_blockage_level variable and	*
 *		     reading data for the new level and refreshing the	*
 *		     display.						*
 *									*
 *	Input:  w - ID of calling widget				*
 *		client_data - unused					*
 *		call_data - combo box data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
trd_select_blockage_level_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int i,j;
	unsigned char	*bdata;
	int	min_range = 0;
	int	min_radial = 0;
	int	max_range = 0;
	int	max_radial = 0;
	short	min_value = 9999;
	short	max_value = -9999;
	int	len;
	char	buf [80];
	XmString	str;

	XmComboBoxCallbackStruct *cbs =
		(XmComboBoxCallbackStruct *) call_data;

	TRD_blockage_level = (int) cbs->item_position;

	HCI_LE_log("New Blockage level %d selected", TRD_blockage_level);

/*	Free previously read data.				*/

	free (TRD_data);
        TRD_data = NULL;

/*	Get data for the selected blockage level.		*/

	len = Read_and_decompress_data( Blockage_LBfd, TRD_blockage_level, &TRD_data );

	if (len <= 0) {

	    HCI_LE_error("Read_and_decompress_data() Failed: %d", len);
	    HCI_task_exit (HCI_EXIT_FAIL);

	}

/*	Since the data are shorts cast the data buffer to a short	*
 *	pointer.							*/

	bdata = (unsigned char *) TRD_data;

/*	Scan the data for the new level and display the min/max values.	*/

	for (i=0;i<3600;i++) {

	    for (j=0;j<MAX_BINS_ALONG_RADIAL;j++) {

		if (bdata [i*MAX_BINS_ALONG_RADIAL+j] > max_value) {

		    max_value = bdata [i*MAX_BINS_ALONG_RADIAL+j];
		    max_range = j;
		    max_radial = i;

		} else if (bdata [i*MAX_BINS_ALONG_RADIAL+j] < min_value) {

		    min_value = bdata [i*MAX_BINS_ALONG_RADIAL+j];
		    min_range = j;
		    min_radial = i;

		}
	    }
	}

	TRD_color.min_value = 0;
	TRD_color.max_value = 100;

	Blockage_min = (float) min_value;
	Blockage_max = (float) max_value;

	TRD_color_scale = (TRD_color.max_value -
				TRD_color.min_value)/
				(TERRAIN_COLORS-1);

	sprintf (buf,"Min: %d %% at %5.1f deg,%d nm",
		(int) Blockage_min, ((float) min_radial)/10.0, (int) min_range);

	str = XmStringCreateLocalized (buf);
	XtVaSetValues (TRD_min_label,
		XmNlabelString,	str,
		NULL);
	XmStringFree (str);

	sprintf (buf,"Max: %d %% at %5.1f deg,%d nm",
		(int) Blockage_max, ((float) max_radial)/10.0, (int) max_range);

	str = XmStringCreateLocalized (buf);
	XtVaSetValues (TRD_max_label,
		XmNlabelString,	str,
		NULL);
	XmStringFree (str);

	trd_display ();

/*	Redisplay the color bar for the new field.			*/

	trd_color_bar ();

/*	Display the polar grid if enabled.				*/

	if (Grid_overlay_flag) {

	    trd_overlay_grid ();

	}

	trd_xsn_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

/*	Directly invoke the expose callback to make the changes	*
 *	visible.							*/

	trd_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

}

/****************************************************************************

   Description:
      Open the LBs used in this program.

   Returns:
      Returns negative number on error, or returns 0 on success.

*****************************************************************************/
static int Open_LBs(){

   LB_status status;
   LB_attr attr;
   char *path_name = NULL;
   char *dir = NULL;
   int len;

   Terrain_LBfd = -1;
   Blockage_LBfd = -1;

   if( strlen( Input_file_path ) == 0 )
   {
     memset( Input_file_path, 0, 255 );
     if( ( dir = getenv( "ORPGDIR" ) ) != NULL )
     {
       HCI_LE_log( "ORPGDIR: %s", dir );
       strcat( Input_file_path, dir );
       if( Input_file_path[ strlen( Input_file_path ) - 1 ] != '/' )
       {
          strcat( Input_file_path, "/precip/" );
       }
       else
       {
          strcat( Input_file_path, "precip/" );
       }
     }
   }

   /* Build full path name of legacy file */      
   if( Input_file_path[ strlen( Input_file_path ) - 1 ] != '/' )
       Input_file_path[ strlen( Input_file_path ) ] = '/';

   path_name = calloc( 1, (strlen( Input_file_path ) + strlen("terrain.lb") + 1) );
            
   if( path_name != NULL ){

      strcpy( path_name, Input_file_path );
      strcat( path_name, "terrain.lb" );
      HCI_LE_log( "Opening Terrain file: %s", path_name );

      if( ( Terrain_LBfd = LB_open( path_name, LB_READ, NULL ) ) > 0 )
      {
        HCI_LE_log("Terrain file %s Successfully Opened", path_name );
      }

      free( path_name );
   }

   path_name = calloc( 1, (strlen( Input_file_path ) + strlen("blockage.lb") + 1) );
            
   if( path_name != NULL ){

      strcpy( path_name, Input_file_path );
      strcat( path_name, "blockage.lb" );
      HCI_LE_log( "Opening Blockage file: %s", path_name );

      if( ( Blockage_LBfd = LB_open( path_name, LB_READ, NULL ) ) > 0 )
      {
        HCI_LE_log( "Blockage file %s Successfully Opened", path_name );

        /* Get info about the blockage LB so we can determine the number
           of levels defined. */
        status.n_check = 0;
        status.attr = &attr;
        len = LB_stat( Blockage_LBfd, &status );

        if (len == LB_SUCCESS)
           TRD_num_levels = status.n_msgs;
      }

      free( path_name );
   }

   if( Terrain_LBfd <= 0 )
   {
     HCI_LE_error( "Terrain File Open Failed (%d)", Terrain_LBfd );
     return -1;
   }
   else if( Blockage_LBfd <= 0 )
   {
     HCI_LE_error( "Blockage File Open Failed (%d)", Blockage_LBfd );
     return -1;
   }

   return 0;
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      Input_file_path - path of terrain/blockage data file (excludes file name) 

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int option_cb( char opt, char *optarg ){
   
  int ret;

  if( opt == 'p' && optarg != NULL && strlen( optarg ) < 255 )
  {
     /* Initialize the Input_file_path to the current
        directory. If the environmental variable
        ORPGDIR is defined, initialize it to that value. */

    memset( Input_file_path, 0, 255 );
    ret = sscanf( optarg, "%s", Input_file_path );
    if( ret == EOF )
    {
      HCI_LE_error( "sscanf failed reading LB path %s", optarg );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
    else
    {
      HCI_LE_log( "Input_file_path = %s", Input_file_path );
    }
  }

  return 0;
}

static void print_usage()
{   
      printf ("\t\t-p Input File Path Name (required)\n" );
}

/****************************************************************************

   Description:
      Read and decompress data specified by LB_fd.  Output is stored in buffer
      pointed to by out.

   Inputs:
      LB_fd - Input fd of LB.
      msg_id - input LB message ID.

   Outputs:
      out - pointer to buffer holding decompressed data

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
int Read_and_decompress_data( int LB_fd, LB_id_t msg_id, char **out ){

   ORPGCMP_hdr_t *hdr;
   char *tmp = NULL, *tout = NULL;
   int dlen, len;

   len = LB_read( LB_fd, &tmp, LB_ALLOC_BUF, msg_id );
   if( len <= 0 ){

       HCI_LE_error("LB_read( %d ) Failed: %d",  LB_fd, len);
       HCI_task_exit(HCI_EXIT_FAIL);

   }

   hdr = (ORPGCMP_hdr_t *) tmp;
   dlen = ORPGCMP_decompress( (int) hdr->code, tmp, len, &tout );
   if( dlen < 0 ){

      /* Problem with data in library. */
      HCI_LE_error( "ORPGCMP_decompress Failed: %d", dlen );
      HCI_task_exit(HCI_EXIT_FAIL);

   }
   else if( (dlen == 0) && (tout == NULL) ){

      /* Data isn't compressed. */
      HCI_LE_log( "Data read from %d is not compressed", LB_fd );
      *out = tmp;
      return( len );

   }

   /* Data sucessfully decompressed. */
   free( tmp );
   *out = tout;
   return( dlen );
}

