/************************************************************************
 *									*
 *	Module:  hci_blockage_main.c					*
 *									*
 *	Description: This task is used to display polar blockage data.	*
 *									*
 *************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/15 15:27:50 $
 * $Id: hci_blockage_main.c,v 1.15 2014/07/15 15:27:50 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Macros.								*/

#define BLOCKAGE_MIN		0
#define BLOCKAGE_MAX		100
#define BLOCKAGE_COLORS		64
#define	WINDOW_WIDTH		630	/* default width of window */
#define	WINDOW_HEIGHT		560	/* default height of window */
#define	ZOOM_FACTOR_MAX		64	/* maximum magnification factor */
#define	ZOOM_FACTOR_MIN		1	/* minimum magnification factor */
#define	RAW_MODE		0	/* interrogation mode */
#define	ZOOM_MODE		1	/* zoom mode */
#define MAX_BINS_ALONG_RADIAL	230
#define MAX_RADIAL_RANGE	230.0
#define	XSN_X_OFFSET		30
#define	XSN_Y_OFFSET		22
#define	XSN_WIDTH		MAX_BINS_ALONG_RADIAL
#define	XSN_HEIGHT		200
#define	SELECTED_ELEV_INIT	-999.0

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>
#include <Xm/ComboBox.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_blockage.h>
#include <hci_vcp_data.h>

char	Buf [256]; /* common buffer for string functions. */

/*	Global widget variables.					*/

Widget	Zoom_button       = (Widget) NULL;
Widget	Reset_button      = (Widget) NULL;
Widget	Raw_button        = (Widget) NULL;
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
Widget	Blockage_min_scale    = (Widget) NULL;
Widget	Blockage_max_scale    = (Widget) NULL;

Widget	Blockage_combo        = (Widget) NULL;

float	Blockage_min       = BLOCKAGE_MIN; /* Minimum blockage to display
						(1/10 percent) */
float	Blockage_max       = BLOCKAGE_MAX; /* Maximum blockage to display
						(1/10 percent) */
int	Grid_overlay_flag  = 1;	/* Used to automatically display polar	*
				 * grid whan radar data updated.	*/
int	Display_mode       = RAW_MODE;

/*	Translation table for data display window.  We need a mechanism	*
 *	to capture mouse events for interrogation and zooming.		*/

String	Translations =
       "<Btn1Down>:   blk_draw(down1)   ManagerGadgetArm() \n\
	<Btn1Up>:     blk_draw(up1)     ManagerGadgetActivate() \n\
	<Btn1Motion>: blk_draw(motion1) ManagerGadgetButtonMotion() \n\
	<Btn2Down>:   blk_draw(down2)   ManagerGadgetArm() \n\
	<Btn3Down>:   blk_draw(down3)   ManagerGadgetArm()";

/*	Global variables						*/

Display		*BLK_display; /* Display info */
Widget		Top_widget;   /* Top level window */
Widget		BLK_label;    /* Elevation/Time label at top of window */
Widget		BLK_min_label;/* Data minimum value label */
Widget		BLK_max_label;/* Data minimum value label */
Dimension	BLK_width;    /* width of display region */
Dimension	BLK_height;   /* height of display region */
Dimension	BLK_xsn_width = XSN_WIDTH;
			      /* width of xsn display region */
Dimension	BLK_xsn_height = XSN_HEIGHT; /* height of xsn display region */
Dimension	BLK_depth;    /* depth (bits) of display */
Widget		BLK_draw_widget; /* drawing area widget for data */
Widget		BLK_xsn_widget; /* drawing area widget for xsn data */
Window		BLK_window;   /* visible Drawable */
Pixmap		BLK_pixmap;   /* invisible Drawable */
Window		BLK_xsn_window;   /* visible Drawable */
Pixmap		BLK_xsn_pixmap;   /* invisible Drawable */
GC		BLK_gc;       /* graphics context */
float		BLK_color_scale; /* scale for converting data to color index */
int		BLK_lb;        /* BLK LB ID */
int		BLK_zoom_factor = 1;   /* current zoom factor (1 to 64) */
char		*BLK_data  = (char *) NULL; /* Pointer to radial data */
float		BLK_display_range  = (float) MAX_RADIAL_RANGE;
				/* Maximum radial range (km) */
int		BLK_num_levels     = 0; /* Number of blockage levels */
int		BLK_blockage_level = 0; /* Active blockage level */
float		Azimuth = 0.0;

Gui_radial_t	BLK_radial;	/* Structure containing radial data
				   properties for the display module. */
Gui_color_t	BLK_color;	/* Structure containing color control
				   data for the display module. */
int		Update_flag = 0; /* Data update flag */
int		Blockage_LBfd;
XFontStruct	*Fontinfo;

/* 20120119 CCR NA12-00054 use the PPS/QPE transitions */

double block_thresh     = 0.0; /* hydromet_prep.alg, default 50.0 */
double Kdp_max_beam_blk = 0.0; /* dp_precip.alg,     default 70.0 */
double Kdp_min_beam_blk = 0.0; /* dp_precip.alg,     default 20.0 */
double Min_blockage     = 0.0; /* dp_precip.alg,     default  5.0 */

void	blk_expose_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_close_callback  (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_data_range_callback     (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_grid_callback   (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_display_mode    (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_select_blockage_level_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_xsn_expose_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_xsn_resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	blk_reset_zoom_callback (Widget w, XtPointer client_data,
			XtPointer call_data);
void	ok_callback (Widget w, XtPointer client_data,
			XtPointer call_data);

void	blk_draw (Widget w, XEvent *event, String *args,
		  int *num_args);

float	hci_find_azimuth (int pixel1, int scanl1, int pixel2, int scanl2);

void	blk_display ();
void	blk_color_bar ();
void	blk_overlay_grid ();
void	blk_overlay_selected_radial ();

/* 20120119 CCR NA12-00054                                         *
 *          blk_define_colors()  uses the old interpolated colors. *
 *          blk_define_colors2() uses the PPS/QPE transitions.     */

void	blk_define_colors (Pixel *color);
void	blk_define_colors2 (Pixel *color);

void	timer_proc ();
int	hci_find_best_color (Display *display, Colormap cmap, XColor *color);
int	initialize_blockage_data();
static int Open_Blockage_Data_LB();
static int Read_and_decompress_data( int LB_fd, LB_id_t msg_id, char **out );

/************************************************************************
 *	Description: This is the main function for the Blockage		*
 *		     Data Display task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments			*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int main( int argc, char *argv [] )
{
	Widget		form;
	Widget		frame;
	Widget		options_frame;
	Widget		options_form;
	Widget		button;
	Widget		label;
	Widget		close_rowcol;
	Widget		grid_rowcol;
	Widget		mode_rowcol;
	Widget		grid_list;
	Widget		mode_list;
	Widget		properties_form;
	Widget		properties_rowcol;
	Widget		button_rowcol;
	Widget		blockage_rowcol;
	Widget		minmax_rowcol;
	XGCValues	gcv;
	XtActionsRec	actions;
	Arg		arg [10];
	int		i;
	int		n;
	XmString	str;
	short		min_range = 0;
	short		max_range = 0;
	short		min_radial = 0;
	short		max_radial = 0;
	char		title[64];
	float		lowest_VCP_elev = 0.0;
	float		blockage_elev = 0.0;
	float		selected_elev = SELECTED_ELEV_INIT;
	XmComboBoxCallbackStruct cbs;

/*	Initialization HCI. */

	HCI_init( argc, argv, HCI_BLOCKAGE_TASK );

/*	Create the top level window for displaying blockage data.	*/

	Top_widget = HCI_get_top_widget();
	BLK_display = HCI_get_display();

	BLK_depth   = XDefaultDepth (BLK_display,
		DefaultScreen (BLK_display));

	Fontinfo = hci_get_fontinfo( SCALED );

/*	Make the initial size of the blockage display window something	*
 *	reasonable. The user can resize it as needed. Define a minimum  *
 *	size to be able to hold all of the widgets.			*/
/*	NOTE:  This is something which should eventually get put into	*
 *	a resource file so it can be configured at startup.		*/

	BLK_width  = WINDOW_WIDTH;
	BLK_height = WINDOW_HEIGHT;

/*	Create the form widget which will manage the drawing_area and	*
 *	row_column widgets.						*/

	form = XtVaCreateManagedWidget ("form",
		xmFormWidgetClass,	Top_widget,
		NULL);

/*	Put blank line at top to be a "buffer".				*/

	BLK_label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Use another form widget to manage all of the control buttons	*
 *	on the left side of the basedata display window.		*/

	Control_form = XtVaCreateManagedWidget ("control_form",
		xmFormWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		BLK_label,
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

	close_rowcol = XtVaCreateManagedWidget ("close_rowcol",
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
		XmNactivateCallback, blk_close_callback,
		NULL);

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
		XmNactivateCallback, blk_data_range_callback,
		(XtPointer) NULL);

	options_frame = XtVaCreateManagedWidget ("options_frame",
		xmFrameWidgetClass,	Control_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		button,
		NULL);

	options_form = XtVaCreateManagedWidget ("options_form",
		xmFormWidgetClass,		options_frame,
		XmNbackground,			hci_get_read_color (BLACK),
		XmNverticalSpacing,		1,
		NULL);

/*	Create the row_column widget which will hold the grid overlay	*
 *	selection buttons.						*/

	grid_rowcol = XtVaCreateManagedWidget ("grid_rowcol",
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
		XmNvalueChangedCallback, blk_grid_callback,
		(XtPointer) 1);

	Grid_off_button = XtVaCreateManagedWidget ("Off ",
		xmToggleButtonWidgetClass,	grid_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Grid_off_button,
		XmNvalueChangedCallback, blk_grid_callback,
		(XtPointer) 0);

	XtManageChild (grid_list);

/*	Create the row_column widget which will hold the mode		*
 *	selection buttons.						*/

	mode_rowcol = XtVaCreateManagedWidget ("mode_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		grid_rowcol,
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
		XmNvalueChangedCallback, blk_display_mode,
		(XtPointer) RAW_MODE);

	Zoom_button = XtVaCreateManagedWidget ("Zoom ",
		xmToggleButtonWidgetClass,	mode_list,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Zoom_button,
		XmNvalueChangedCallback, blk_display_mode,
		(XtPointer) ZOOM_MODE);

	Reset_button = XtVaCreateManagedWidget ("Reset",
		xmPushButtonWidgetClass,	mode_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNselectColor,		hci_get_read_color (WHITE),
		NULL);

	XtAddCallback (Reset_button,
		XmNactivateCallback, blk_reset_zoom_callback, NULL);

	XtManageChild (mode_list);

/*	Create the row_column widget which will hold the blockage	*
 *	selection buttons.						*/

	blockage_rowcol = XtVaCreateManagedWidget ("blockage_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		mode_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	XtVaCreateManagedWidget ("Elevation Angle:  ",
		xmLabelWidgetClass,	blockage_rowcol,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Blockage_combo = XtVaCreateManagedWidget ("blockage_combo",
		xmComboBoxWidgetClass,	blockage_rowcol,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,	hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNwidth,		80,
		XmNvisibleItemCount,	10,
		NULL);

	XtAddCallback (Blockage_combo,
		XmNselectionCallback, blk_select_blockage_level_callback,
		NULL);

	minmax_rowcol = XtVaCreateManagedWidget ("minmax_rowcol",
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		blockage_rowcol,
		XmNorientation,		XmVERTICAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNspacing,		1,
		NULL);

	sprintf (title,"Min: %d %% at %5.1f deg,%d nm",
		(int) Blockage_min, ((float) min_radial)/10.0, (int) min_range);
	str = XmStringCreateLocalized (title);

	BLK_min_label = XtVaCreateManagedWidget ("Min Value:",
		xmLabelWidgetClass,	minmax_rowcol,
		XmNlabelString,		str,
		XmNbackground,	 	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
	XmStringFree (str);

	sprintf (title,"Max: %d %% at %5.1f deg,%d nm",
		(int) Blockage_max, ((float) max_radial)/10.0, (int) max_range);
	str = XmStringCreateLocalized (title);

	BLK_max_label = XtVaCreateManagedWidget ("Max Value",
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

	BLK_xsn_widget = XtVaCreateManagedWidget ("topography_xsn_widget",
		xmDrawingAreaWidgetClass,	options_form,
		XmNwidth,		BLK_xsn_width,
		XmNheight,		BLK_xsn_height,
		xmRowColumnWidgetClass,	options_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		minmax_rowcol,
		NULL);

/*	Create a widget along the bottom of the window to left of the	*
 *	control buttons which will contain various data attributes.	*/

	properties_form = XtVaCreateManagedWidget ("properties_form",
		xmFormWidgetClass,	form,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		Control_form,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	button_rowcol = XtVaCreateManagedWidget ("button_rowcol",
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

	properties_rowcol = XtVaCreateManagedWidget ("button_rowcol",
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

/*	Create the drawing_area widget which will be used to display	*
 *	base level radar data.  It will occupy the upper right portion	*
 *	of the form.							*/

	actions.string = "blk_draw";
	actions.proc   = (XtActionProc) blk_draw;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	BLK_draw_widget = XtVaCreateManagedWidget ("topography_drawing_area",
		xmDrawingAreaWidgetClass,	form,
		XmNwidth,		BLK_width,
		XmNheight,		BLK_height,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		BLK_label,
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

	XtAddCallback (BLK_draw_widget,
		XmNexposeCallback, blk_expose_callback, NULL);

/*	Permit the user to resize the data display window.	*/

	XtAddCallback (BLK_draw_widget,
		XmNresizeCallback, blk_resize_callback, NULL);

	XtRealizeWidget (Top_widget);

/*	Define the various window variables to be used as arguments in 	*
 *	the various Xlib and Xt calls.					*/

	BLK_window  = XtWindow      (BLK_draw_widget);
	BLK_pixmap  = XCreatePixmap (BLK_display,
				      BLK_window,
				      BLK_width,
				      BLK_height,
				      BLK_depth);
	BLK_xsn_window  = XtWindow  (BLK_xsn_widget);

/*	Define the Graphics Context to be used in drawing data		*/

	gcv.foreground = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
	gcv.background = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
	gcv.graphics_exposures = FALSE;

	BLK_gc = XCreateGC (BLK_display,
	      		     BLK_window,
	      		     GCBackground | GCForeground | GCGraphicsExposures,
			     &gcv);

	XSetFont (BLK_display, BLK_gc, hci_get_font (LIST));

	XtVaSetValues (BLK_draw_widget,
		XmNbackground,	hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
		NULL);

	XtVaSetValues (BLK_xsn_widget,
		XmNbackground,	hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
		NULL);

/*	Clear the data display portion of the window by filling it with	*
 *	the background color.						*/

	XSetForeground (BLK_display, BLK_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (BLK_display,
			BLK_pixmap,
			BLK_gc,
			0,
			0,
			BLK_width,
			BLK_width);

/*	Define the data display window properties.			*/

	BLK_radial.azimuth_width  = 0.50;
	BLK_radial.range_start    = 0.0;
	BLK_radial.range_interval = 1.0;
	BLK_radial.bins           = MAX_BINS_ALONG_RADIAL;
	BLK_radial.center_pixel   = (BLK_width-75)/2;
	BLK_radial.center_scanl   = BLK_height/2;
	BLK_radial.scale_x        = BLK_zoom_factor * (BLK_width-75)/
			   		       (2.0*BLK_display_range);
	BLK_radial.scale_y        = -BLK_radial.scale_x;

	/* Old: BLK_color.filter  = 0;                             */
	/* 20120119 CCR NA12-00054. Make transparent at 0 % level. */

	BLK_color.filter          = 4;
	BLK_color.num_colors      = BLOCKAGE_COLORS;

	/* Old: blk_define_colors (BLK_color.color);                        *
         *                                                                  *
         * 20120119 CCR NA12-00054                                          *
         *          blk_define_colors()  uses the old interpolated colors.  *
         *          blk_define_colors2() uses the PPS/QPE transitions.      *
         *                                                                  *
         *          Before setting the colors we need to get the trasitison *
         *          levels.                                                 */

        /* ---------- block_thresh: 50 % ---------- */

        if(RPGC_ade_get_values("alg.hydromet_prep.", "block_thresh", &block_thresh) != 0)
        {
           RPGC_log_msg(GL_INFO, ">> RPGC_ade_get_values() failed - block_thresh");
           block_thresh = 0.0;
        }

        /* ---------- Kdp_max_beam_blk: 70 % ---------- */

        if(RPGC_ade_get_values("alg.dp_precip.", "Kdp_max_beam_blk", &Kdp_max_beam_blk) != 0)
        {
           RPGC_log_msg(GL_INFO, ">> RPGC_ade_get_values() failed - Kdp_max_beam_blk");
           Kdp_max_beam_blk = 0.0;
        }

        /* ---------- Kdp_min_beam_blk: 20 % ---------- */

        if(RPGC_ade_get_values("alg.dp_precip.", "Kdp_min_beam_blk", &Kdp_min_beam_blk) != 0)
        {
           RPGC_log_msg(GL_INFO, ">> RPGC_ade_get_values() failed - Kdp_min_beam_blk");
           Kdp_min_beam_blk = 0.0;
        }

        /* ---------- Min_blockage: 5 % ---------- */

        if(RPGC_ade_get_values("alg.dp_precip.", "Min_blockage", &Min_blockage) != 0)
        {
           RPGC_log_msg(GL_INFO, ">> RPGC_ade_get_values() failed - Min_blockage");
           Min_blockage = 0.0;
        }

	blk_define_colors2 (BLK_color.color); /* 20120119 CCR NA12-00054 Use PPS/QPE transitions */

	BLK_color_scale  = (BLK_color.max_value-BLK_color.min_value)/(BLOCKAGE_COLORS-1);

	XtRealizeWidget (Top_widget);

	XtVaGetValues (BLK_xsn_widget,
		XmNwidth,	&BLK_xsn_width,
		XmNheight,	&BLK_xsn_height,
		NULL);

	BLK_xsn_pixmap  = XCreatePixmap (BLK_display,
				      BLK_xsn_window,
				      BLK_xsn_width,
				      BLK_xsn_height,
				      BLK_depth);

/*	Add an expose callback for the drawing_area in order to allow	*
 *	holes to be filled in the display when  other windows are moved	*
 *	across it.							*/

	XtAddCallback (BLK_xsn_widget,
		XmNexposeCallback, blk_xsn_expose_callback, NULL);

/*	Permit the user to resize the data display window.	*/

	XtAddCallback (BLK_xsn_widget,
		XmNresizeCallback, blk_xsn_resize_callback, NULL);

/*	Initialize data.					*/

	if( initialize_blockage_data() < 0 )
	{
	  hci_error_popup( Top_widget, "Error Accessing Blockage Data", ok_callback );
	}
	else
	{
	  /* Create drop-down list of elevations for user to pick from. */
	  /* Make the lowest elevation of current VCP the starting point. */

	  lowest_VCP_elev = hci_current_vcp_elevation_angle( 0 );

	  for (i=0;i<BLK_num_levels;i++)
	  {
	     blockage_elev = (i-10)/10.0;
             if( blockage_elev >= lowest_VCP_elev && selected_elev == SELECTED_ELEV_INIT )
	     {
	       selected_elev = blockage_elev;
	     }
	     sprintf (title,"%4.1f", blockage_elev);
	     str = XmStringCreateLocalized (title);
	     XmComboBoxAddItem(Blockage_combo,str,i+1,True);
	     XmStringFree (str);
	  }

	  if( selected_elev == SELECTED_ELEV_INIT )
	  {
	    selected_elev = blockage_elev;
	  }

	  sprintf( title, "%4.1f", selected_elev );
	  str = XmStringCreateLocalized( title );
	  XmComboBoxSetItem( Blockage_combo, str );
	  XmStringFree( str );

	  /* Call callback function to initialize data to lowest *
	   * elevation in the current VCP.                       */

	  XtVaGetValues( Blockage_combo, XmNselectedPosition,
                         &cbs.item_position, NULL );
	  blk_select_blockage_level_callback( Top_widget,
	                                      NULL, (XtPointer) &cbs);
	}

/*	Start HCI loop. */

	HCI_start( timer_proc, HCI_HALF_SECOND, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This function handles the data window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the data pixmap to the		*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_xsn_expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XCopyArea (BLK_display,
		   BLK_xsn_pixmap,
		   BLK_xsn_window,
		   BLK_gc,
		   0,
		   0,
		   BLK_xsn_width,
		   BLK_xsn_height,
		   0,
		   0);
}

/************************************************************************
 *	Description: This function handles the data window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the data pixmap to the		*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Set the clip region to the full window, update the window by	*
 *	copying the data pixmap to it to fill in any holes left	by any	*
 *	previously overlain window and the restoring the clip region	*
 *	to the data display region only.				*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BLK_width;
	clip_rectangle.height  = BLK_height;

	XSetClipRectangles (BLK_display,
			    BLK_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XCopyArea (BLK_display,
		   BLK_pixmap,
		   BLK_window,
		   BLK_gc,
		   0,
		   0,
		   BLK_width,
		   BLK_height,
		   0,
		   0);

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BLK_width-75;
	clip_rectangle.height  = BLK_height;

	XSetClipRectangles (BLK_display,
			    BLK_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	Highlight the radial that is selected for display in the XSN	*
 *	window.								*/

	blk_overlay_selected_radial ();
}

/************************************************************************
 *	Description: This function handles the window resize callback	*
 *		     by redrawing all data window components.		*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_xsn_resize_callback (
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
  unsigned char	*bdata = NULL;
  int	azimuth_index;
  float	scale_x;
  float	scale_y;
  float	color_scale;
  int	color_index;
  char	buf [16];
  int	character_height, character_offset;
  int	x_max, x_min, y_max, y_min, y_mid, max_value, min_value;
  float	y_axis_incr, mid_value;

  bdata = (unsigned char *) BLK_data;

  /* Determine offset (midway point) of characters. */

  character_height = Fontinfo->ascent + Fontinfo->descent;
  character_offset = character_height/2;

  /* Draw XSN area. */

  XSetForeground( BLK_display, BLK_gc,
                  hci_get_read_color( PRODUCT_BACKGROUND_COLOR ) );

  XFillRectangle( BLK_display, BLK_xsn_pixmap, BLK_gc,
                  0, 0, BLK_xsn_width, BLK_xsn_height );

  /* Calculate scales. */

  x_max = BLK_xsn_width;
  x_min = XSN_X_OFFSET;
  y_max = BLK_xsn_height - XSN_Y_OFFSET;
  y_min = XSN_Y_OFFSET;
  y_mid = y_min + (y_max-y_min)/2;
  min_value = (int) BLK_color.min_value;
  max_value = (int) BLK_color.max_value;
  y_axis_incr = (max_value-min_value)/2.0;
  mid_value = ((float)min_value + y_axis_incr);
  scale_x = (float) (x_max-x_min)/MAX_RADIAL_RANGE;
  scale_y = (float) (y_max-y_min)/(max_value-min_value);
  color_scale = (float) (max_value-min_value)/(BLK_color.num_colors-1);

  /* Draw blockage data in XSN for given radial. */

  azimuth_index = Azimuth*10;
  pixel     = x_min;
  old_pixel = x_min;

  for( i = 0; i < MAX_BINS_ALONG_RADIAL; i++ )
  {
    pixel = ( i * scale_x ) + x_min;
    scanl = y_max - ((value-min_value)*scale_y) - 1;
    if( scanl < y_min ){ scanl = y_min; }
    else if( scanl > y_max ){ scanl = y_max; }
    value = bdata[azimuth_index*MAX_BINS_ALONG_RADIAL + i];

    if( value < min_value || value > max_value)
    {
      color_index = -1;
    }
    else
    {
      color_index = (int) ( ( value - min_value ) / color_scale + 1 );

      if( color_index < 0 )
      {
        color_index = 0;
      }
      else if( color_index > BLK_color.num_colors-1 )
      {
  	color_index = BLK_color.num_colors-1;
      }
    }
    if( color_index < 0 )
    {
      XSetForeground( BLK_display, BLK_gc, hci_get_read_color( PRODUCT_BACKGROUND_COLOR ) );
    }
    else
    {
      XSetForeground( BLK_display, BLK_gc, BLK_color.color[color_index] );
    }

    for( j = old_pixel + 1; j <= pixel; j++ )
    {
      XDrawLine( BLK_display, BLK_xsn_pixmap, BLK_gc, j, scanl, j, y_max );
    }

    old_pixel = pixel;

  }

  /* Draw Azimuth number in XSN area. */

  XSetForeground( BLK_display, BLK_gc, hci_get_read_color( CYAN ) );

  sprintf( buf,"Azimuth %4.1f", Azimuth );

  XDrawString( BLK_display, BLK_xsn_pixmap, BLK_gc,
               x_min, y_min-(y_min-character_height)/2, buf, strlen( buf ) );

  /* Change color to draw X/Y axes in XSN area. */

  XSetForeground( BLK_display, BLK_gc, hci_get_read_color( WHITE ) );

  /* Draw Y axis. */

  XDrawLine( BLK_display, BLK_xsn_pixmap, BLK_gc, x_min, y_max, x_min, y_min );

  /* Draw x-axis distance intervals. */

  for( i = 0; i < 125; i = i+50 )
  {
    pixel = (int) (scale_x*(i/HCI_KM_TO_NM)+0.5) + x_min;

    XDrawLine( BLK_display, BLK_xsn_pixmap, BLK_gc,
               pixel, y_max, pixel, y_max+5 );

    sprintf( buf,"%d nm", i );

    XDrawString( BLK_display, BLK_xsn_pixmap, BLK_gc,
                 pixel-12, BLK_xsn_height-2, buf, strlen( buf ) );
  }

  /* Draw line that represents minimum value on Y-axis. */

  XDrawLine( BLK_display, BLK_xsn_pixmap, BLK_gc, x_min, y_max, x_max, y_max );

  sprintf( buf,"%3d", min_value );

  XDrawString( BLK_display, BLK_xsn_pixmap, BLK_gc,
               0, y_max+character_offset, buf, strlen( buf ) );

  /* Draw line that represents midpoint value on Y-axis. */

  XDrawLine( BLK_display, BLK_xsn_pixmap, BLK_gc, x_min, y_mid, x_max, y_mid );

  /* Middle value could have a fraction. Once the max
     value gets above 20, don't worry about it. */

  if( max_value%2 != 0 && mid_value < 10.0 )
  {
    sprintf( buf,"%3.1f", mid_value );
  }
  else
  {
    sprintf( buf,"%3d", (int)mid_value );
  }

  XDrawString( BLK_display, BLK_xsn_pixmap, BLK_gc,
               0, y_mid+character_offset, buf, strlen( buf ) );

  /* Draw line that represents maximum value on Y-axis. */

  XDrawLine( BLK_display, BLK_xsn_pixmap, BLK_gc, x_min, y_min, x_max, y_min );

  sprintf( buf,"%3d", max_value );

  XDrawString( BLK_display, BLK_xsn_pixmap, BLK_gc,
               0, y_min+character_offset, buf, strlen( buf ) );

  blk_xsn_expose_callback( (Widget) NULL, (XtPointer) NULL,
                           (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function handles the window resize callback	*
 *		     by redrawing all data window components.		*
 *									*
 *	Input:  w - top level widget ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_resize_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XRectangle	clip_rectangle;

/*	Get the new size of the data window.			*/

	if ((BLK_draw_widget == (Widget) NULL)    ||
	    (BLK_display     == (Display *) NULL) ||
	    (BLK_window      == (Window) NULL)    ||
	    (BLK_pixmap      == (Pixmap) NULL)    ||
	    (BLK_gc          == (GC) NULL)) {

	    return;

	}

	XtVaGetValues (BLK_draw_widget,
		XmNwidth,	&BLK_width,
		XmNheight,	&BLK_height,
		NULL);

/*	Destroy the old pixmap since the size has changed and create	*
 *	create a new one.						*/

	if (BLK_pixmap != (Window) NULL) {

	    XFreePixmap (BLK_display, BLK_pixmap);

	}

	BLK_pixmap = XCreatePixmap (BLK_display,
		BLK_window,
		BLK_width,
		BLK_height,
		BLK_depth);

/*	Recompute the scale factors (in pixels/km) and a new window	*
 *	center pixel/scanline coordinate.				*/

	BLK_radial.scale_x      = BLK_zoom_factor * (BLK_width-75)/
					       (2*BLK_display_range);
	BLK_radial.scale_y	= -BLK_radial.scale_x;
	BLK_radial.center_pixel = (BLK_width-75) / 2;
	BLK_radial.center_scanl = BLK_height / 2;

/*	Set the clip region to the entire window so we can create a	*
 *	color bar along the right size.					*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BLK_width;
	clip_rectangle.height  = BLK_height;

	XSetClipRectangles (BLK_display,
			    BLK_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XSetForeground (BLK_display, BLK_gc,
		hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
	XFillRectangle (BLK_display,
			BLK_pixmap,
			BLK_gc,
			0, 0,
			BLK_width,
			BLK_height);

	blk_color_bar ();

/*	Restore the clip window to the data display area only.		*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BLK_width-75;
	clip_rectangle.height  = BLK_height;

	XSetClipRectangles (BLK_display,
			    BLK_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	blk_display ();

/*	If we want a grid overlay then display it.			*/

	if (Grid_overlay_flag) {

	    blk_overlay_grid ();

	}

/*	Call the expose callback to write the data pixmap to	*
 *	the data window.					*/

	blk_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function draws a color bar along the right	*
 *		     side of the data window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_color_bar ()
{
	int	i;

	int	width;
	int	height;
	int	scanl;
	float	xheight;
	float	label;
	float	data_range;
	char	text [16];
	XRectangle	clip_rectangle;
	int	step;

/*	Define the clip region to include the entire display region.	*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BLK_width;
	clip_rectangle.height  = BLK_height;

	XSetClipRectangles (BLK_display,
			    BLK_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

/*	Clear an area to the right of the data display to place	*
 *	the color bar.						*/

	XSetForeground (BLK_display,
			BLK_gc,
			hci_get_read_color (BACKGROUND_COLOR1));

	XFillRectangle (BLK_display,
			BLK_window,
			BLK_gc,
			(int) (BLK_width-75),
			0,
			75,
			BLK_height);

	XFillRectangle (BLK_display,
			BLK_pixmap,
			BLK_gc,
			(int) (BLK_width-75),
			0,
			75,
			BLK_height);

/*	Define the width of each color bar box and the height scale	*/

	width = 10;

	xheight = BLK_height/(2.0*(BLOCKAGE_COLORS-1));
	step = 1;

	height  = xheight+1;

/*	For each color element, create a box and fill it with the color	*/

	for (i=BLOCKAGE_COLORS-1;i>= step;i=i-step) {

	    scanl = BLK_height/4 + xheight*(i/step-1);

	    XSetForeground (BLK_display,
			    BLK_gc,
			    BLK_color.color [BLOCKAGE_COLORS-i]);

	    XFillRectangle (BLK_display,
			    BLK_window,
			    BLK_gc,
			    (int) (BLK_width-15),
			    scanl,
			    width,
			    height);

	    XFillRectangle (BLK_display,
			    BLK_pixmap,
			    BLK_gc,
			    (int) (BLK_width-15),
			    scanl,
			    width,
			    height);

	}

	XSetForeground (BLK_display,
			BLK_gc,
			hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XDrawRectangle (BLK_display,
			BLK_window,
			BLK_gc,
			(int) (BLK_width-15),
			(int) (BLK_height/4),
			width,
			(int) (BLK_height/2));

	XDrawRectangle (BLK_display,
			BLK_pixmap,
			BLK_gc,
			(int) (BLK_width-15),
			(int) (BLK_height/4),
			width,
			(int) (BLK_height/2));

/* Draw text labels to the left of the color bar for the minimun,  *
 * middle, and maximum values.					   *
 *                                                                 *
 * 20120119 Ward CCR NA12-00054 Added 70%, 20%, %5 levels          */

        data_range = BLK_color.max_value - BLK_color.min_value;

        /* ---------- minimum: 0 % ---------- */

	scanl = BLK_height/4 + xheight*(BLOCKAGE_COLORS-1);

	sprintf (text," %3d",(int) BLK_color.min_value);

	XDrawString (BLK_display,
		     BLK_window,
		     BLK_gc,
		     (int) (BLK_width-75),
		     (int) (scanl+5),
		     text,
		     strlen (text));

	XDrawString (BLK_display,
		     BLK_pixmap,
		     BLK_gc,
		     (int) (BLK_width-75),
		     (int) (scanl+5),
		     text,
		     strlen (text));

	XDrawLine (BLK_display,
		   BLK_window,
		   BLK_gc,
		   (int) (BLK_width-25),
		   scanl,
		   (int) (BLK_width-15),
		   scanl);

	XDrawLine (BLK_display,
		   BLK_pixmap,
		   BLK_gc,
		   (int) (BLK_width-25),
		   scanl,
		   (int) (BLK_width-15),
		   scanl);

        /* ---------- maximum: 100 % ---------- */

	scanl = BLK_height/4;

	sprintf (text," %3d",(int) BLK_color.max_value);

	XDrawString (BLK_display,
		     BLK_window,
		     BLK_gc,
		     (int) (BLK_width-75),
		     (int) (scanl+5),
		     text,
		     strlen (text));

	XDrawString (BLK_display,
		     BLK_pixmap,
		     BLK_gc,
		     (int) (BLK_width-75),
		     (int) (scanl+5),
		     text,
		     strlen (text));

	XDrawLine (BLK_display,
		   BLK_window,
		   BLK_gc,
		   (int) (BLK_width-25),
		   scanl,
		   (int) (BLK_width-15),
		   scanl);

	XDrawLine (BLK_display,
		   BLK_pixmap,
		   BLK_gc,
		   (int) (BLK_width-25),
		   scanl,
		   (int) (BLK_width-15),
		   scanl);

        /* ---------- block_thresh: 50 % ---------- */

        label = ((block_thresh*data_range) + BLK_color.min_value)/100.0;
        if( (block_thresh > 0.0) 
                  &&
            (((int) label > (int) BLK_color.min_value) && ((int) label < (int) BLK_color.max_value)) )
        {
           sprintf (text, " %3d", (int) label);

           scanl  = BLK_height/4 + (xheight * (BLOCKAGE_COLORS-1) * (1.0 - (block_thresh / 100.0)));
           scanl += 2; /* fudge factor */

           XDrawString (BLK_display,
                        BLK_window,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawString (BLK_display,
                        BLK_pixmap,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawLine (BLK_display,
                      BLK_window,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);

           XDrawLine (BLK_display,
                      BLK_pixmap,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);
        }

        /* ---------- Kdp_max_beam_blk: 70 % ---------- */

        label = ((Kdp_max_beam_blk*data_range) + BLK_color.min_value)/100.0;
        if( (Kdp_max_beam_blk > 0.0) 
                  &&
            (data_range >= 10)
                  &&
            (((int) label > (int) BLK_color.min_value) && ((int) label < (int) BLK_color.max_value)) )
        {
           sprintf (text, " %3d", (int) label);

	   scanl  = BLK_height/4 + (xheight * (BLOCKAGE_COLORS-1) * (1.0 - (Kdp_max_beam_blk / 100.0)));
           scanl += 2; /* fudge factor */

           XDrawString (BLK_display,
                        BLK_window,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawString (BLK_display,
                        BLK_pixmap,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawLine (BLK_display,
                      BLK_window,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);

           XDrawLine (BLK_display,
                      BLK_pixmap,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);
        }

        /* ---------- Kdp_min_beam_blk: 20 % ---------- */

        label = ((Kdp_min_beam_blk*data_range) + BLK_color.min_value)/100.0;
        if( (Kdp_min_beam_blk > 0.0) 
                  &&
            (data_range >= 10)
                  &&
            (((int) label > (int) BLK_color.min_value) && ((int) label < (int) BLK_color.max_value)) )
        {
           sprintf (text, " %3d", (int) label);

           scanl  = BLK_height/4 + (xheight * (BLOCKAGE_COLORS-1) * (1.0 - (Kdp_min_beam_blk / 100.0)));
           scanl += 2; /* fudge factor */

           XDrawString (BLK_display,
                        BLK_window,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawString (BLK_display,
                        BLK_pixmap,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawLine (BLK_display,
                      BLK_window,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);

           XDrawLine (BLK_display,
                      BLK_pixmap,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);
        }

        /* ---------- Min_blockage: 5 % ---------- */

        label = ((Min_blockage*data_range) + BLK_color.min_value)/100.0;
        if( (Min_blockage > 0.0) 
                  &&
            (data_range >= 10)
                  &&
            (((int) label > (int) BLK_color.min_value) && ((int) label < (int) BLK_color.max_value)) )
        {
           sprintf (text, " %3d", (int) label);

           scanl  = BLK_height/4 + (xheight * (BLOCKAGE_COLORS-1) * (1.0 - (Min_blockage / 100.0)));
           scanl += 2; /* fudge factor */

           XDrawString (BLK_display,
                        BLK_window,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawString (BLK_display,
                        BLK_pixmap,
                        BLK_gc,
                        (int) (BLK_width-75),
                        (int) (scanl+5),
                        text,
                        strlen (text));

           XDrawLine (BLK_display,
                      BLK_window,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);

           XDrawLine (BLK_display,
                      BLK_pixmap,
                      BLK_gc,
                      (int) (BLK_width-25),
                      scanl,
                      (int) (BLK_width-15),
                      scanl);
        }

/*	Reset the clip rectangle so that drawing in the data	*
 *	display region will not write over the color bar.	*/

	clip_rectangle.x       = 0;
	clip_rectangle.y       = 0;
	clip_rectangle.width   = BLK_width-75;
	clip_rectangle.height  = BLK_height;

	XSetClipRectangles (BLK_display,
			    BLK_gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);
}

/************************************************************************
 *	Description: This function displays the data.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_display ()
{
	int	i;

/*	Clear the contents of the data window.			*/

	XSetForeground (BLK_display, BLK_gc,
		   hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

	XFillRectangle (BLK_display, BLK_pixmap, BLK_gc,
			0, 0, BLK_width, BLK_height);

	blk_expose_callback ((Widget) NULL,
				     (XtPointer) NULL,
				     (XtPointer) NULL);

	if (Grid_overlay_flag) {

	    blk_overlay_grid ();

	}

	if (BLK_data == NULL) {

	    return;

	}

	for (i=0;i<3600;i++) {

	    BLK_radial.azimuth = i/10.0;

	    Gui_display_radial_data (&BLK_data[i*BLK_radial.bins],
				     DATA_TYPE_BYTE,
				     BLK_display,
				     BLK_pixmap,
				     BLK_gc,
				     &BLK_radial,
				     &BLK_color);
	}

/*	display a polar grid if enabled.				*/

	if (Grid_overlay_flag) {

	    blk_overlay_grid ();

	}

/*	Invoke the expose callback to make changes visible.		*/

	blk_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function handles all mouse button events	*
 *		     inside the data display region.			*
 *									*
 *	Input:  w - drawing area widget ID				*
 *		event - X event data					*
 *		args - number of user argunments			*
 *		num_args - user arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_draw (
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
	unsigned char	*bdata;
	int	azimuth_index;
	XmString	string;

	value = 0.0;

	if (!strcmp (args[0], "down1")) {

	    HCI_LE_log("Left mouse button pushed");

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

		BLK_zoom_factor = BLK_zoom_factor*2;

		if (BLK_zoom_factor > ZOOM_FACTOR_MAX) {

		    BLK_zoom_factor = ZOOM_FACTOR_MAX;
		    return;

		}

/*		Adjust the azimuth width to reduce specking	*/

		if (BLK_zoom_factor == 1) {

		    BLK_radial.azimuth_width = 0.5;

		} else if (BLK_zoom_factor == 2) {

		    BLK_radial.azimuth_width = 0.4;

		} else if (BLK_zoom_factor == 4) {

		    BLK_radial.azimuth_width = 0.25;

		} else {

		    BLK_radial.azimuth_width = 0.15;

		}

		BLK_radial.x_offset = (BLK_radial.center_pixel-first_pixel)/
				BLK_radial.scale_x + BLK_radial.x_offset;
		BLK_radial.y_offset = (BLK_radial.center_scanl-first_scanl)/
				BLK_radial.scale_y + BLK_radial.y_offset;

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BLK_radial.center_pixel +
				         BLK_radial.x_offset*BLK_radial.scale_x),
				  (int) (BLK_radial.center_scanl +
				         BLK_radial.y_offset*BLK_radial.scale_y));

		x1 = BLK_radial.x_offset;
		y1 = BLK_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > BLK_display_range) {

		   BLK_radial.x_offset = (BLK_radial.x_offset/range)*BLK_display_range;
		   BLK_radial.y_offset = (BLK_radial.y_offset/range)*BLK_display_range;

		}

		BLK_radial.scale_x  = BLK_zoom_factor * (BLK_width-75)/
						   (2*BLK_display_range);
		BLK_radial.scale_y  = - BLK_zoom_factor * (BLK_width-75)/
						   (2*BLK_display_range);

	        blk_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    } else {

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BLK_radial.center_pixel +
				         BLK_radial.x_offset*BLK_radial.scale_x),
				  (int) (BLK_radial.center_scanl +
				         BLK_radial.y_offset*BLK_radial.scale_y));

		x1 = (BLK_radial.center_pixel-event->xbutton.x)/
		     BLK_radial.scale_x + BLK_radial.x_offset;
		y1 = (BLK_radial.center_scanl-event->xbutton.y)/
		     BLK_radial.scale_y + BLK_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		sprintf (Buf,"%3d deg, %3d nm ",
			(int) Azimuth, (int) (range*HCI_KM_TO_NM+0.5));

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Azran_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

		azimuth_index = (int) (Azimuth*10+0.5);

		bdata = (unsigned char *) BLK_data;

		value = bdata [azimuth_index*MAX_BINS_ALONG_RADIAL + (int) range];

		sprintf (Buf,"%d %%", (int) value);

		string = XmStringCreateLocalized (Buf);

		XtVaSetValues (Value_data,
			XmNlabelString,	string,
			NULL);

		XmStringFree (string);

	        blk_xsn_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

		blk_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    }

 	} else if (!strcmp (args[0], "up1")) {

/*	If the left mouse button is released, stop the rubber-banding	*
 *	code and unset the button down flag.				*/

	    HCI_LE_log("Left mouse button released");
	    button_down  = 0;

	} else if (!strcmp (args[0], "motion1")) {

/*	If the cursor is being dragged with the left mouse button	*
 *	down, repeatedly draw the sector defined by the point where	*
 *	the button was pressed and where it currently is.  Use the	*
 *	left hand rule for determining the direction in which to draw	*
 *	the sector.							*/

	} else if (!strcmp (args[0], "down2")) {

	    HCI_LE_log("Middle mouse button pushed");

	    if (Display_mode == ZOOM_MODE) {

	        first_pixel = event->xbutton.x;
	        first_scanl = event->xbutton.y;

	        BLK_radial.x_offset = (BLK_radial.center_pixel-first_pixel)/
				BLK_radial.scale_x + BLK_radial.x_offset;
	        BLK_radial.y_offset = (BLK_radial.center_scanl-first_scanl)/
				BLK_radial.scale_y + BLK_radial.y_offset;

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BLK_radial.center_pixel +
				         BLK_radial.x_offset*BLK_radial.scale_x),
				  (int) (BLK_radial.center_scanl +
				         BLK_radial.y_offset*BLK_radial.scale_y));

		x1 = BLK_radial.x_offset;
		y1 = BLK_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > BLK_display_range) {

		   BLK_radial.x_offset = (BLK_radial.x_offset/range)*BLK_display_range;
		   BLK_radial.y_offset = (BLK_radial.y_offset/range)*BLK_display_range;

		}

	        blk_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	    }

	} else if (!strcmp (args[0], "down3")) {

	    HCI_LE_log("Right mouse button pushed");

	    if (Display_mode == ZOOM_MODE) {

	        first_pixel = event->xbutton.x;
	        first_scanl = event->xbutton.y;

	        BLK_zoom_factor = BLK_zoom_factor/2;

	        if (BLK_zoom_factor < ZOOM_FACTOR_MIN) {

	            BLK_zoom_factor = ZOOM_FACTOR_MIN;
	            return;

	        }

/*		Adjust the azimuth width to reduce specking	*/

		if (BLK_zoom_factor == 1) {

		    BLK_radial.azimuth_width = 0.5;

		} else if (BLK_zoom_factor == 2) {

		    BLK_radial.azimuth_width = 0.4;

		} else if (BLK_zoom_factor == 4) {

		    BLK_radial.azimuth_width = 0.25;

		} else {

		    BLK_radial.azimuth_width = 0.15;

		}

	        BLK_radial.x_offset = (BLK_radial.center_pixel-first_pixel)/
				BLK_radial.scale_x + BLK_radial.x_offset;
	        BLK_radial.y_offset = (BLK_radial.center_scanl-first_scanl)/
				BLK_radial.scale_y + BLK_radial.y_offset;
	        BLK_radial.scale_x  = BLK_zoom_factor * (BLK_width-75)/
			        (2*BLK_display_range);
	        BLK_radial.scale_y  = - BLK_zoom_factor * (BLK_width-75)/
			        (2*BLK_display_range);

/*		Determine the azimuth/range for the new location.  If	*
 *		the range is beyond the max data range, then set the	*
 *		X and Y offsets to the max data range, preserving the	*
 *	 	azimuth.						*/

		Azimuth = hci_find_azimuth (event->xbutton.x,
				  event->xbutton.y,
				  (int) (BLK_radial.center_pixel +
				         BLK_radial.x_offset*BLK_radial.scale_x),
				  (int) (BLK_radial.center_scanl +
				         BLK_radial.y_offset*BLK_radial.scale_y));

		x1 = BLK_radial.x_offset;
		y1 = BLK_radial.y_offset;

		range = sqrt ((double) (x1*x1 + y1*y1));

		if (range > BLK_display_range) {

		   BLK_radial.x_offset = (BLK_radial.x_offset/range)*BLK_display_range;
		   BLK_radial.y_offset = (BLK_radial.y_offset/range)*BLK_display_range;

		}

	        blk_resize_callback ((Widget) NULL,
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
blk_reset_zoom_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	BLK_zoom_factor = 1;

	BLK_radial.azimuth_width = 0.5;

	BLK_radial.x_offset = 0.0;
	BLK_radial.y_offset = 0.0;

	BLK_radial.scale_x  = BLK_zoom_factor * (BLK_width-75)/
					   (2*BLK_display_range);
	BLK_radial.scale_y  = - BLK_zoom_factor * (BLK_width-75)/
					   (2*BLK_display_range);

	blk_resize_callback ((Widget) NULL,
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
blk_display_mode (
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
blk_data_range_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Widget	form;
	Widget	rowcol;
	Widget	button;
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

/*	Define a row of buttons to close the window and to apply the	*
 *	changes.							*/

	rowcol = XtVaCreateManagedWidget ("data_range_rowcol",
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
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback,
		hci_close_data_range_dialog_callback,
		NULL);

	Apply_button = XtVaCreateManagedWidget ("Apply",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
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

/*	Creatye a set of slider bars to control the blockage data	*
 *	range.								*/

	blockage_frame = XtVaCreateManagedWidget ("blockage_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	rowcol = XtVaCreateManagedWidget ("data_range_rowcol",
		xmRowColumnWidgetClass,	blockage_frame,
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	Blockage_min_scale = XtVaCreateManagedWidget ("blockage_minimum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Minimum Blockage (%)", 23,
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
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

        /* We do not want the min scale to be anything other than the default. */
	XtVaSetValues (Blockage_min_scale,
		XmNsensitive,	False,
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
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) BLOCKAGE_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Blockage_min_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	Blockage_max_scale = XtVaCreateManagedWidget ("blockage_maximum",
		xmScaleWidgetClass,	rowcol,
		XtVaTypedArg, XmNtitleString, XmRString, "Maximum Blockage (%)", 23,
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
		XmNfontList,		hci_get_fontlist (LIST),
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
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	sprintf (buf,"%d",(int) BLOCKAGE_MAX);
	str = XmStringCreateLocalized (buf);

	label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	Blockage_max_scale,
		XmNlabelString,		str,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XmStringFree (str);

	XtManageChild( form );

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
 *		client_data -  2 (blockage maximum)			*
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
	BLK_color.min_value = Blockage_min;
	BLK_color.max_value = Blockage_max;
	BLK_color_scale  = (BLK_color.max_value-BLK_color.min_value)/
			    (BLOCKAGE_COLORS-1);

	blk_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	blk_xsn_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	Update_flag = 0;

	XtVaSetValues (Apply_button,
		XmNsensitive,	False,
		NULL);
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
blk_grid_callback (
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

	    blk_resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);

	}
}

/************************************************************************
 *	Description: This function displays a polar grid over a		*
 *		     data display.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_overlay_grid (
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

	XSetForeground (BLK_display,
			BLK_gc,
			hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

/*	Based on the current zoom factor, determine the distancce	*
 *	between polar grid rings (in nautical miles).			*/

	switch (BLK_zoom_factor) {

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

	x = fabs ((double) BLK_radial.x_offset);
	y = fabs ((double) BLK_radial.y_offset);

	if (x > y) {

	    start_range = x - BLK_display_range/BLK_zoom_factor;
	    stop_range  = x + BLK_display_range/BLK_zoom_factor;

	} else {

	    start_range = y - BLK_display_range/BLK_zoom_factor;
	    stop_range  = y + BLK_display_range/BLK_zoom_factor;

	}

	start_range = (start_range/step)*step;
	stop_range  = (stop_range/step)*step + step;

	if (start_range < step) {

	    start_range = step;

	}

	for (i=step;i<=stop_range;i=i+step) {

	    size  = i*BLK_radial.scale_x/HCI_KM_TO_NM;

	    pixel = BLK_radial.center_pixel + BLK_radial.x_offset*BLK_radial.scale_x -
		    size;
	    scanl = BLK_radial.center_scanl + BLK_radial.y_offset*BLK_radial.scale_y -
		    size;

	    XDrawArc (BLK_display,
		      BLK_pixmap,
		      BLK_gc,
		      pixel,
		      scanl,
		      size*2,
		      size*2,
		      0,
		      -(360*64));

	    sprintf (buf,"%i nm",i);

	    XDrawString (BLK_display,
			 BLK_pixmap,
			 BLK_gc,
			 (int) (BLK_radial.center_pixel +
				BLK_radial.x_offset*BLK_radial.scale_x -
				4*strlen (buf)),
			 scanl + 4,
			 buf,
			 strlen (buf));
	}

/*	Next display the "spokes" outward from the center.		*/

	for (i=0;i<360;i=i+step) {

	    pixel = (int) (((1+1.0/BLK_zoom_factor)*BLK_display_range *
		     cos ((double) (i+90)*HCI_DEG_TO_RAD) +
		     BLK_radial.x_offset) * BLK_radial.scale_x +
		     BLK_radial.center_pixel);
	    scanl = (int) (((1+1.0/BLK_zoom_factor)*BLK_display_range *
		     sin ((double) (i-90)*HCI_DEG_TO_RAD) +
		     BLK_radial.y_offset) * BLK_radial.scale_y +
		     BLK_radial.center_scanl);

	    XDrawLine (BLK_display,
		       BLK_pixmap,
		       BLK_gc,
		       (int) (BLK_radial.center_pixel +
			      BLK_radial.x_offset*BLK_radial.scale_x),
		       (int) (BLK_radial.center_scanl +
			      BLK_radial.y_offset*BLK_radial.scale_y),
		       pixel,
		       scanl);

	}

	blk_expose_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function displays the highlighted radial	*
 *		     in the data display window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_overlay_selected_radial (
)
{
	int	pixel;
	int	scanl;
static	int	flag = 0;

	flag = 1 - flag;

	if (flag) {

	    return;

	}

	XSetForeground (BLK_display,
			BLK_gc,
			hci_get_read_color (BLACK));

	pixel = (int) (((1+1.0/BLK_zoom_factor)*BLK_display_range/2 *
		     cos ((double) (Azimuth-90)*HCI_DEG_TO_RAD) +
		     BLK_radial.x_offset) * BLK_radial.scale_x +
		     BLK_radial.center_pixel);
	scanl = (int) (((1+1.0/BLK_zoom_factor)*BLK_display_range/2 *
		     sin ((double) (Azimuth+90)*HCI_DEG_TO_RAD) +
		     BLK_radial.y_offset) * BLK_radial.scale_y +
		     BLK_radial.center_scanl);

	XDrawLine (BLK_display,
		   BLK_window,
		   BLK_gc,
		   (int) (BLK_radial.center_pixel +
			  BLK_radial.x_offset*BLK_radial.scale_x),
		   (int) (BLK_radial.center_scanl +
			  BLK_radial.y_offset*BLK_radial.scale_y),
		   pixel,
		   scanl);
}

/************************************************************************
 *	Description: This function is executed at regular intervals.	*
 *	Input:  w - parent widget of timer				*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
	blk_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the RPG data display window.	*
 *									*
 *	Input:  w - Close button ID					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Close button pushed");
	HCI_task_exit( HCI_EXIT_SUCCESS );
}

/************************************************************************
 *	Description: This function defines colors for the data bar.	*
 *									*
 *	Input:  color - Pixel						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_define_colors (
Pixel *color
)
{
	int	status;
	int	i;
	XColor	xcolor;
	int	red;

	for (i=0;i<BLOCKAGE_COLORS;i++) {

	    red   = (short) (255-i*(2*256/BLOCKAGE_COLORS));	

	    if (red < 0)
		red = -red;

	    xcolor.green = (short) (i*(2*256/BLOCKAGE_COLORS));

	    if (xcolor.green > 255)
		xcolor.green = (short) (255 - ((i-32)*512/BLOCKAGE_COLORS));

	    xcolor.red   = (short) red << 8;
	    xcolor.green = xcolor.green << 8;
	    xcolor.blue  = (short) (255-i*(256/BLOCKAGE_COLORS)) << 8;

	    status = hci_find_best_color (BLK_display, HCI_get_colormap(), &xcolor);

	    color[i] = xcolor.pixel;
	}
}

/* 64 color levels, 100 blockage levels. Color -> blockage converter:
 *
 *  0   1.56 %
 *  1   3.12 %
 *  2   4.69 %
 *  3   6.25 %
 *  4   7.81 %
 *  5   9.38 %
 *  6  10.94 %
 *  7  12.50 %
 *  8  14.06 %
 *  9  15.62 %
 * 10  17.19 %
 * 11  18.75 %
 * 12  20.31 %
 * 13  21.88 %
 * 14  23.44 %
 * 15  25.00 %
 * 16  26.56 %
 * 17  28.12 %
 * 18  29.69 %
 * 19  31.25 %
 * 20  32.81 %
 * 21  34.38 %
 * 22  35.94 %
 * 23  37.50 %
 * 24  39.06 %
 * 25  40.62 %
 * 26  42.19 %
 * 27  43.75 %
 * 28  45.31 %
 * 29  46.88 %
 * 30  48.44 %
 * 31  50.00 %
 * 32  51.56 %
 * 33  53.12 %
 * 34  54.69 %
 * 35  56.25 %
 * 36  57.81 %
 * 37  59.38 %
 * 38  60.94 %
 * 39  62.50 %
 * 40  64.06 %
 * 41  65.62 %
 * 42  67.19 %
 * 43  68.75 %
 * 44  70.31 %
 * 45  71.88 %
 * 46  73.44 %
 * 47  75.00 %
 * 48  76.56 %
 * 49  78.12 %
 * 50  79.69 %
 * 51  81.25 %
 * 52  82.81 %
 * 53  84.38 %
 * 54  85.94 %
 * 55  87.50 %
 * 56  89.06 %
 * 57  90.62 %
 * 58  92.19 %
 * 59  93.75 %
 * 60  95.31 %
 * 61  96.88 %
 * 62  98.44 %
 * 63 100.00 % */

/************************************************************************
 *      Description: This function defines colors for the data bar.     *
 *                                                                      *
 *                   It uses QPE transitions at 5%, 20%, 70% levels:    *
 *                                                                      *
 *                    5 % - blockage stars being applied                *
 *                   20 % - HR switches from R(Z, Zdr) to R(Kdp)        *
 *                   70 % - QPE goes to the next elevation              *
 *                                                                      *
 *      Input:  color - Pixel                                           *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 *                                                                      *
 * 20120119 Ward Initial Version. CCR NA12-00054 Code cribbed from      *
 *               blk_define_colors() There are 64 color levels,         *
 *               and 100 blockage levels. A conversion table is         *
 *               provided above.                                        *
 *                                                                      *
 *         See: http://en.wikipedia.org/wiki/Web_colors#X11_color_names *
 ************************************************************************/

void
blk_define_colors2 (
Pixel *color
)
{
    int    i;
    XColor xcolor;

    int    hex_start, red_start, green_start, blue_start;
    int    hex_end,   red_end,   green_end,   blue_end;

    unsigned char red_byte, green_byte, blue_byte;

    short  start_level,  end_level;
    float  start_factor, end_factor;

    int Min_blockage_start     = 0;
    int Kdp_min_beam_blk_start = 0;
    int block_thresh_start     = 0;
    int Kdp_max_beam_blk_start = 0;
    int blocked_start          = 0;

    /* The color assignments assume that:
     *
     * Min_blockage      (5) < Kdp_min_beam_blk (20)
     * Kdp_min_beam_blk (20) < block_thresh     (50)
     * block_thresh     (50) < Kdp_max_beam_blk (70)
     *
     * Default start indices are:
     *
     * Min_blockage_start       1   3.12 %
     * Kdp_min_beam_blk_start   4   7.81 %
     * block_thresh_start      13  21.88 %
     * Kdp_max_beam_blk_start  32  51.56 %
     * blocked_start           45  71.88 %
     *
     * They are slightly high to match with the radial display. */

    Min_blockage_start      =  1;
    Kdp_min_beam_blk_start  = (Min_blockage     / 100.0) * (BLOCKAGE_COLORS + 1);
    Kdp_min_beam_blk_start += 1; /* to match up with radial display */
    block_thresh_start      = (Kdp_min_beam_blk / 100.0) * (BLOCKAGE_COLORS + 1);
    Kdp_max_beam_blk_start  = (block_thresh     / 100.0) * (BLOCKAGE_COLORS + 1);
    blocked_start           = (Kdp_max_beam_blk / 100.0) * (BLOCKAGE_COLORS + 1);

    for (i=0;i<BLOCKAGE_COLORS;i++)
    {
      if (i < Min_blockage_start)
      {
         /* Black to Black */

         hex_start = 0x000000;
         hex_end   = 0x000000;

         start_level = 0;
         end_level   = Min_blockage_start - 1;
      }
      else if (i < Kdp_min_beam_blk_start)
      {
         /* Dark Grey to Light Grey */

         hex_start = 0xA9A9A9;
         hex_end   = 0xD3D3D3;

         start_level = Min_blockage_start;
         end_level   = Kdp_min_beam_blk_start - 1;
      }
      else if (i < block_thresh_start)
      {
         /* Light Blue to Dark Blue */

         hex_start = 0xADD8E6;
         hex_end   = 0x00008B;

         start_level = Kdp_min_beam_blk_start;
         end_level   = block_thresh_start - 1;
      }
      else if (i < Kdp_max_beam_blk_start)
      {
         /* Light Green to Dark Green */

         hex_start = 0x90EE90;
         hex_end   = 0x006400;

         start_level = block_thresh_start;
         end_level   = Kdp_max_beam_blk_start - 1;
      }
      else if (i < blocked_start)
      {
         /* Light Yellow to Dark Orange */

         hex_start = 0xFFFFE0;
         hex_end   = 0xFFA500;

         start_level = Kdp_max_beam_blk_start;
         end_level   = blocked_start - 1;
      }
      else /* is totally blocked */
      {
         /* Pink to Dark Red */

         hex_start = 0xFFC0CB;
         hex_end   = 0x8B0000;

         start_level = blocked_start;
         end_level   = BLOCKAGE_COLORS - 1;
      }

      if(end_level > start_level)
         start_factor = (end_level - i) / (float) (end_level - start_level);
      else /* end_level == start_level */
         start_factor = 0.0;

      end_factor = 1.0 - start_factor;

      red_start   = (hex_start & 0xFF0000) >> 16;
      green_start = (hex_start & 0x00FF00) >> 8;
      blue_start  = (hex_start & 0x0000FF);

      red_end     = (hex_end   & 0xFF0000) >> 16;
      green_end   = (hex_end   & 0x00FF00) >> 8;
      blue_end    = (hex_end   & 0x0000FF);

      red_byte   = (int) RPGC_NINT((red_start   * start_factor) + (red_end   * end_factor));
      green_byte = (int) RPGC_NINT((green_start * start_factor) + (green_end * end_factor));
      blue_byte  = (int) RPGC_NINT((blue_start  * start_factor) + (blue_end  * end_factor));

      /* XColor values are unsigned short, 2 byte, 0xXXXX, so double byte.
       * I think it only reads the high byte, but I am unsure. */

      xcolor.red   = (unsigned short) (red_byte   << 8) | red_byte;
      xcolor.green = (unsigned short) (green_byte << 8) | green_byte;
      xcolor.blue  = (unsigned short) (blue_byte  << 8) | blue_byte;

      /* Debug
       *
       * fprintf(stderr, "%2d) R %4.4x G %4.4x B %4.4x\n", i,
       *         xcolor.red, xcolor.green, xcolor.blue); */

      hci_find_best_color (BLK_display, HCI_get_colormap(), &xcolor);

      color[i] = xcolor.pixel;
   }
}

/************************************************************************
 *	Description: This function handles blockage level selections	*
 *		     by updating the BLK_blockage_level variable and	*
 *		     reading in/displaying the new data.		*
 *									*
 *	Input:  w - ID of calling widget				*
 *		client_data - unused					*
 *		call_data - combo box data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
blk_select_blockage_level_callback (
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

	BLK_blockage_level = (int) cbs->item_position;

	HCI_LE_log("New Blockage level %d selected", BLK_blockage_level);

/*	Free previously read data.				*/

	free (BLK_data);
        BLK_data = NULL;

/*	Get data for the selected blockage level.		*/

	len = Read_and_decompress_data( Blockage_LBfd, BLK_blockage_level, &BLK_data );

	if (len <= 0) {

	    HCI_LE_error("Read_and_decompress_data() Failed: %d", len);
	    HCI_task_exit( HCI_EXIT_FAIL );

	}

/*	Since the data are shorts cast the data buffer to a short	*
 *	pointer.							*/

	bdata = (unsigned char *) BLK_data;

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

	BLK_color.min_value = 0;
	BLK_color.max_value = 100;

	Blockage_min = min_value;
	Blockage_max = max_value;

	BLK_color_scale = (BLK_color.max_value -
				BLK_color.min_value)/
				(BLOCKAGE_COLORS-1);

	sprintf (buf,"Min: %d %% at %5.1f deg,%d nm",
		(int) Blockage_min, ((float) min_radial)/10.0, (int) min_range);

	str = XmStringCreateLocalized (buf);
	XtVaSetValues (BLK_min_label,
		XmNlabelString,	str,
		NULL);
	XmStringFree (str);

	sprintf (buf,"Max: %d %% at %5.1f deg,%d nm",
		(int) Blockage_max, ((float) max_radial)/10.0, (int) max_range);

	str = XmStringCreateLocalized (buf);
	XtVaSetValues (BLK_max_label,
		XmNlabelString,	str,
		NULL);
	XmStringFree (str);

	blk_display ();

/*	Redisplay the color bar for the new field.			*/

	blk_color_bar ();

/*	Display the polar grid if enabled.				*/

	if (Grid_overlay_flag) {

	    blk_overlay_grid ();

	}

	blk_xsn_resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

/*	Directly invoke the expose callback to make the changes	*
 *	visible.							*/

	blk_expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: Open the blockage data LB.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: -1 on Error, 0 on success				*
*************************************************************************/

static int Open_Blockage_Data_LB()
{
  LB_status status;
  LB_attr attr;

  Blockage_LBfd = -1;

  Blockage_LBfd = ORPGDA_lbfd( ORPGDAT_BLOCKAGE );

  if( Blockage_LBfd < 0 )
  {
    HCI_LE_error("ORPGDA_lbfd(ORPGDAT_BLOCKAGE) Failed (%d)", Blockage_LBfd);
    return -1;
  }

  /* Get info about the blockage LB so we can determine the number *
   * of levels defined.                                            */

  status.n_check = 0;
  status.attr = &attr;
  if( ORPGDA_stat( ORPGDAT_BLOCKAGE, &status ) == LB_SUCCESS )
  {
    BLK_num_levels = status.n_msgs;
  }
  else
  {
    HCI_LE_error("ORPGDA_stat() Failed (%d)", Blockage_LBfd);
    return -1;
  }

  return 0;
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

int Read_and_decompress_data( int LB_fd, LB_id_t msg_id, char **out )
{
   ORPGCMP_hdr_t *hdr;
   char *tmp = NULL, *tout = NULL;
   int dlen, len;

   len = LB_read( LB_fd, &tmp, LB_ALLOC_BUF, msg_id );
   if( len <= 0 ){

       HCI_LE_error("LB_read( %d ) Failed: %d", LB_fd, len);
       return -1;
   }

   hdr = (ORPGCMP_hdr_t *) tmp;
   dlen = ORPGCMP_decompress( (int) hdr->code, tmp, len, &tout );
   if( dlen < 0 ){

      /* Problem with data in library. */
      HCI_LE_error("ORPGCMP_decompress Failed: %d", dlen );
      return -1;
   }
   else if( (dlen == 0) && (tout == NULL) ){

      /* Data isn't compressed. */
      HCI_LE_log("Data read from %d is not compressed", LB_fd );
      *out = tmp;
      return( len );
   }
   else{

      /* Data sucessfully decompressed. */
      free( tmp );
      *out = tout;
      return( dlen );
   }
}

/************************************************************************
 *	Description: Callback for error popup.				*
 *									*
 *	Input:  unused							*
 *	Output: NONE							*
 *	Return: NONE							*
*************************************************************************/

void ok_callback(
Widget w,
XtPointer client_data,
XtPointer call_data )
{
	HCI_task_exit( HCI_EXIT_FAIL );
}

/************************************************************************
 *	Description: Open/read blockage data LB. Initialize data stores.*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: -1 on Error, 0 on success				*
*************************************************************************/

int initialize_blockage_data()
{
	int		len;
	short		min_range = 0;
	short		max_range = 0;
	short		min_radial = 0;
	short		max_radial = 0;
	int		i,j;
	char		*sdata;
	int		min_value = 9999;
	int		max_value = -9999;

        if( Open_Blockage_Data_LB() < 0 ){ return -1; }
        len = Read_and_decompress_data( Blockage_LBfd, 1, &BLK_data );
        if( len < 0 ){ return -1; }

/*	Initialize the data buffer by openning the blockage data file	*
 *	and determining the data ranges in the file.			*/

	sdata = (char *) BLK_data;

	for (i=0;i<3600;i++) {
	  for (j=0;j<MAX_BINS_ALONG_RADIAL;j++) {
	    if ((int)sdata [i*MAX_BINS_ALONG_RADIAL+j] > max_value) {

	      max_value = (int)sdata [i*MAX_BINS_ALONG_RADIAL+j];
	      max_range = j;
	      max_radial = i;
	    }
	    else if ((int)sdata [i*MAX_BINS_ALONG_RADIAL+j] < min_value) {

	      min_value = (int)sdata [i*MAX_BINS_ALONG_RADIAL+j];
	      min_range = j;
	      min_radial = i;
	    }
	  }
	}

	Blockage_min = min_value;
	Blockage_max = max_value;
	BLK_color.min_value = min_value;
	BLK_color.max_value = max_value;

	return 0;
}
