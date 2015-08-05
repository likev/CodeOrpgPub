/************************************************************************
 *	Module: xpdt_main.c						*
 *	Description: This is the main module for the NEXRAD Product	*
 *		     Display Tool (xpdt).  It is used to display a	*
 *		     selected set of RPG generated products in an X	*
 *		     Window.  If the user does not specify an input	*
 *		     filename in the command-line, he/she is prompted	*
 *		     for one.						*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:31:11 $
 * $Id: xpdt_main.c,v 1.53 2009/02/27 22:31:11 ccalvert Exp $
 * $Revision: 1.53 $
 * $State: Exp $
 */

/*	System include file definitions.				*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>
#include <Xm/ComboBox.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ArrowB.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
#include <math.h>

/*	Local include file definitions.					*/

#include "rle_def.h"
#include "product_colors.h"
#include <infr.h>
#include <orpg.h>
#include <orpgda.h>
#include "lb.h"
#include "rss_replace.h"
#include "prod_gen_msg.h"
#include "xpdt_def.h"


static char Prod_dir[128] = "";
static char Prod_table[128] = "";
static int Called_from_timer = 0;

/*	Currently, only products with 16 colors or less can be		*
 *	displayed.							*/

#define		COLORS		16    /* Maximum number of colors in a non DHR
					 product display */
#define		DHR_COLORS	64    /* colors in digital hybrid reflectivity
					 product display */
#define	DEGRAD	(3.14159265/180.0)    /* degrees to radians conversion */
#define	MAX_PRODUCTS		32000 /* maximum number of products in LB */

enum { OPEN_FILE, SAVE_FILE };

/*	Define global variables to be used throughout other modules	*/

Widget		top_widget         = (Widget) NULL;
Widget		draw_widget        = (Widget) NULL;
Widget		rowcol             = (Widget) NULL;
Widget		file_button        = (Widget) NULL;
Widget		product_button     = (Widget) NULL;
Widget		grid_button        = (Widget) NULL;
Widget		map_button         = (Widget) NULL;
Widget		invert_button      = (Widget) NULL;
Widget		cancel_button      = (Widget) NULL;
Widget		left_button        = (Widget) NULL;
Widget		right_button       = (Widget) NULL;
Widget		file_dialog        = (Widget) NULL;
Widget		storm_track_dialog = (Widget) NULL;
Widget		hail_dialog        = (Widget) NULL;
Widget		ttab_dialog        = (Widget) NULL;
Widget		File_dialog        = (Widget) NULL;
Widget		Product_dialog     = (Widget) NULL;
Widget		Product_scroll     = (Widget) NULL;
Widget		product_list = (Widget) NULL;
Widget		product_text = (Widget) NULL;

/*	Various X window properties	*/

Colormap	cmap;
GC		gc;
Display		*display;
Window		window;
Pixmap		pixmap;
XtAppContext	control_app;
XColor		color[COLORS+8];	/* product color data */
XColor		dhr_color[DHR_COLORS+1];  /* DHR product colors */
Dimension	width   = 700;		/* width (pixels) of window */
Dimension	height  = 700;		/* height (scanlines) of window */
float		range = 460.0;		/* Maximum distance range (km) */
float		scale_x;	  /* pixel/km scale factor */
float		scale_y;	  /* scanline/km scale factor */
float		value_min = 0.0;  /* minimum data value */
float		value_max = 16.0; /* maximum data value */
int		center_pixel;     /* pixel coordinate at window center */
int		center_scanl;     /* scanline coordinate at window center */
int		Colors;		  /* number of product colors */
int		grid_overlay_flag = 1; /* grid on = 1; grid off = 0 */
int		map_overlay_flag  = 0; /* map on = 1; map off = 0 */
XmString	str;	/* common compound string buffer */
int		product_type; /* current open product type */
float		max_range;    /* max range distance for current product. */
int		lbfd    = -1; /* product file descriptor. */
int		db_lbfd = -1; /* database file descriptor */
int		num_products; /* number of products in database matching
				 search criteria */
LB_info		list [32000]; /* info of products in LB for product code */
short		*product_data = NULL; /* pointer to product data */
static short	*uncompress = NULL; /* pointer to uncompressed product data */
int		prod_len; /* length (bytes) of product */
int		product;

/*	Widget/label color data	*/

Pixel		black_color;
Pixel		white_color;
Pixel		steelblue_color;
Pixel		lightsteelblue_color;
Pixel		seagreen_color;
Pixel		green_color;
Pixel		red_color;
Pixel		yellow_color;
Pixel		cyan_color;

int		lb_product = 1;	/* 1 = product from LB; 0 = product from file */
int		current_row = 0; /* current graphics attribute row */
int		page_size = 5;   /* number of pages in table */
int		product_start_scanl = 75; /* starting scanline for product
					     data */
int             kilometer=0;  /*  Whether to display using km or nm */


/* Define storage for pointers to various blocks in product */
int symbology_block = 0;
int grafattr_block  = 0;
int tabular_block   = 0;


/* Product types */
unsigned short radial_type = 0xAF1F;
unsigned short raster_type = 0xBA0F;
unsigned short vwp_type    = 48;
unsigned short sti_type    = 58;
unsigned short hi_type     = 59;
unsigned short meso_type   = 60;
unsigned short tvs_type    = 61;
unsigned short swp_type    = 47;
unsigned short dhr_type    = 32;
unsigned short dbv_type    = 93;
unsigned short ss_type     = 62;
unsigned short ftm_type    = 75;
unsigned short pup_type    = 77;
unsigned short spd_type    = 82;
unsigned short dr_type     = 94;
unsigned short dv_type     = 99;
unsigned short clr_type    = 132;
unsigned short cld_type    = 133;
unsigned short dvl_type    = 134;
unsigned short stand_alone_type = 5;
unsigned short generic_radial_type       = 996;
unsigned short generic_hires_radial_type = 997;
unsigned short generic_raster_type       = 998;
unsigned short generic_hires_raster_type = 999;

/* Elevation Angle data */
float vcp21_elev[] = { 0.0, 0.5, 1.5, 2.4, 3.4, 4.3, 6.0, 9.9, 14.6, 19.5 };
float vcp11_elev[] = { 0.0, 0.5, 1.5, 2.4, 3.4, 4.3, 5.3, 6.2, 7.5, 8.7, \
                       10.0, 12.0, 14.0, 16.7, 19.5 };
float vcp31_elev[] = { 0.0, 0.5, 1.5, 2.5, 3.5, 4.5 };
float vcp32_elev[] = { 0.0, 0.5, 1.5, 2.5, 3.5, 4.5 };

#include "xpdt_tables.h"

/*	Various font information	*/

XFontStruct	*font_info;
Font		font;
XmFontListEntry	label_font;
XmFontListEntry	ttab_font;
XmFontList	fontlist;
char    fontname[] = "-*-courier-bold-r-*-*-*-100-100-100-*-*-iso8859-1";

int		pixel_offset; /* pixel offset for start of product data */
int		scanl_offset; /* scanline offset for start of product data */
int		color_step; /* increment between color levels */
char		current_file [128]; /* filename (if data from reg file) */
unsigned long	next_time = 8000; /* update timer (miliseconds) */
XtIntervalId	xpdt_timer;

float	radar_latitude;  /* radar latitude (degrees) */
float	radar_longitude;  /* radar longitude (degrees) */
char xpdt_map_filename[128] = ""; 		/* map filename */
static char database_filename[128] = ""; /* database filename */
int	auto_display_flag = 1; /* 1 - automatically update product when new one
				  is generated */
int	timer_flag        = 0; /* 1 - timer initialized */
int	first_storm = 1;       /* first storm in storm tracking product */
int	last_storm = 100;      /* last storm in storm tracking product */
int	hail_threshold1 = 30;  /* lower hail product threshold */
int	hail_threshold2 = 50;  /* upper hail product threshold */
int	svr_hail_threshold1 = 30;  /* lower severe hail product threshold */
int	svr_hail_threshold2 = 50;  /* upper severe hail product threshold */
int	debug = 0;	/* display debug data = 1 */
short	product_code = 0; /* current product code */

int	Access_database_first_time = 1; /* 1 - database needs initialization */
int	New_product_file           = 1; /* 1 = new product needs to be read. */

char	buf [128]; /* shared buffer for strings */

#define	MAX_PRODUCTS_IN_LIST	4000 /* maximum items in menus */

int	Msg_id_list  [MAX_PRODUCTS_IN_LIST]; /* Msg ID LUT */
int	Time_list    [MAX_PRODUCTS_IN_LIST]; /* Time menu items */
int	Product_list [MAX_PRODUCTS_IN_LIST]; /* Product menu items */

int	Num_products = 0; /* number of items in product menu */
int	Num_times    = 0; /* number of items in time menu */
int	Num_msg_ids  = 0; /* number of message IDs in LUT */


/*  Global data structures  */
/* Define Pointer to radial and raster data structures */
struct radial_rle *radial_image = NULL;
struct raster_rle *raster_image = NULL;
struct dhr_rle *dhr_image = NULL;

/* Define Pointer to VWP data structure */
struct vwp *vwp_image = NULL;

/* Define Pointer to STI data structure */
struct sti *sti_image = NULL;

/* Define Pointer to HI data structure */
struct hi *hi_image = NULL;

/* Define Pointer to MESO data structure */
struct meso *meso_image = NULL;

/* Define Pointer to TVS data structure */
struct tvs *tvs_image = NULL;

/* Define Pointer to SWP data structure */
struct swp *swp_image = NULL;

/* Define pointer to product_pertinent data structure */
struct product_pertinent *attribute = NULL;

/* Define pointer to graphic attribute data structure */
struct graphic_attr *gtab = NULL;

/* Define pointer to tabular alphanumeric data structure */
struct tabular_attr *ttab = NULL;

#define	READ_FROM_FILENAME	0
#define	READ_FROM_DATABASE	1

int	Current_product = 0;	/* current product code */
int	Current_time    = 0;    /* current product volume time */
int	Current_item    = 1;	/* current product in list */
int	Current_mode    = READ_FROM_DATABASE;

ORPGDBM_query_data_t	Query_data [16]; /* DB query data */
RPG_prod_rec_t		Db_info [MAX_PRODUCTS_IN_LIST]; /* DB query results */

char	*Month_string[] = {
	"",
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

void	destroy_products_dialog_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	destroy_file_dialog_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_new_product (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_new_time (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_product_from_list (Widget w,
		XtPointer client_data, XtPointer call_data);
void	close_product_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	choose_file_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void new_LB_product (Widget w, 
		XtPointer client_data, XtPointer call_data);
void	update_select_product_list ();
int find_best_color (Display *display, Colormap cmap, XColor *color);
static int Read_options(int argc, char **argv);
static XtTimerCallbackProc xpdt_timer_proc (Widget w, XtIntervalId id);
static int open_product_file (char *filename, int mode);
static int Display_product (char *prod_with_hd, int prod_len);

/************************************************************************
 *	Description: This is the main function for the NEXRAD Product	*
 *		     Display Tool (xpdt).				*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int main (
int	argc,
char	*argv []
)
{
	int		i;
	XGCValues	gcv;
	ArgList		args = NULL;
	XColor		kolor;
	int		status;
	int		retval = -1;
	char		*variable;

	void	input_callback ();
	void	expose_callback ();
	void	resize_callback ();
	void	product_callback ();
	void	grid_callback ();
	void	map_callback ();
	void	invert_callback ();
	void	cancel_callback ();

	extern	int	polar_slice ();

/*	If no command-line arguments specified, the tool assumes	*
 *	the products database is used.					*/

        Read_options (argc, argv);
	Current_mode = READ_FROM_DATABASE;
	auto_display_flag = 1;

	if (XQ_init (Prod_dir, Prod_table) != 0)
		exit (1);

        ORPGPAT_clear_tbl();

	if (ORPGPAT_read_ASCII_PAT (Prod_table) <= 0){

            fprintf( stderr, "Error Reading ASCII PAT: %d\n", retval );
	    exit (1);

        }

/*	Check for a products database path-filename from the environment*
 *	variable ORPG_PRODUCTS_DATABASE.				*/

	variable = getenv ("ORPG_PRODUCTS_DATABASE");

	if (variable == NULL) {

/*	    fprintf (stderr, "WARNING: no database name specified in ORPG_PRODUCTS_DATABASE\n"); */
	    database_filename[0] = '\0';

	} else {

	    strcpy (database_filename, variable);

	}

/*	Check for a map filename from the environment variable		*
 *	XPDT_MAP_FILE.							*/

	variable = getenv ("XPDT_MAP_FILE");

	if (variable == NULL)
	    fprintf (stderr, "No overlay map specified in XPDT_MAP_FILE\n");
	else
	    strcpy (xpdt_map_filename, variable);

/*	Create an X display window to display the product.		*/

	top_widget = XtAppInitialize (&control_app, "Product Display Tool",
		NULL, 0, &argc, argv, NULL, args, 0);

/*	Create a DrawingAreaWidget to draw the data in.  Add the	*
 *	necessary callback procedures for resize, expose, and input.	*/

	draw_widget = XtVaCreateWidget ("drawing_area",
		xmDrawingAreaWidgetClass,	top_widget,
		XmNwidth,			width,
		XmNheight,			height,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		NULL);

/*	Add a callback to handle mouse events inside the draw window.	*
 *	The routine input_callback () displays the position of the	*
 *	cursor along with the value of any data displayed at that	*
 *	position.							*/

	XtAddCallback (draw_widget,
		XmNinputCallback, input_callback, NULL);

	XtAddCallback (draw_widget,
		XmNexposeCallback, expose_callback, NULL);

	XtAddCallback (draw_widget,
		XmNresizeCallback, resize_callback, NULL);

/*	Define a set of buttons to display in the lower left portion	*
 *	of the window.							*/

	XtVaGetValues (top_widget, XmNcolormap, &cmap, NULL);

  XParseColor (XtDisplay (draw_widget), cmap, "black", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  black_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "white", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  white_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "red", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  red_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "seagreen", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  seagreen_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "green", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  green_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "steelblue", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  steelblue_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "lightsteelblue", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  lightsteelblue_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "yellow", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  yellow_color = kolor.pixel;

  XParseColor (XtDisplay (draw_widget), cmap, "cyan", &kolor);
  find_best_color (XtDisplay (draw_widget), cmap, &kolor);
  cyan_color = kolor.pixel;

/*	The file button allows one to open a new file or linear buffer	*
 *	for display.							*/

	file_button = XtVaCreateManagedWidget ("File",
		xmPushButtonWidgetClass,	draw_widget,
		XmNx,				10,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		XmNforeground,			black_color,
		XmNbackground,			steelblue_color,
		NULL);

	XtAddCallback (file_button,
		XmNactivateCallback, choose_file_callback,
		(XtPointer) OPEN_FILE);

/*	The product button allows one to open a new product from the	*
 *	ORPG Products Database for display.				*/

	product_button = XtVaCreateManagedWidget ("Product",
		xmPushButtonWidgetClass,	draw_widget,
		XmNx,				90,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		XmNforeground,			black_color,
		XmNbackground,			steelblue_color,
		NULL);

	XtAddCallback (product_button,
		XmNactivateCallback, product_callback, NULL);

/*	The grid button toggles the display of a polar grid over the	*
 *	product data.							*/

	grid_button = XtVaCreateManagedWidget ("Grid Off",
		xmPushButtonWidgetClass,	draw_widget,
		XmNx,				170,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		XmNforeground,			black_color,
		XmNbackground,			steelblue_color,
		NULL);

	XtAddCallback (grid_button,
		XmNactivateCallback, grid_callback, NULL);

/*	The map button toggles the display of a USGS map over the	*
 *	product data.							*/

	map_button = XtVaCreateManagedWidget ("Map On",
		xmPushButtonWidgetClass,	draw_widget,
		XmNx,				250,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		XmNforeground,			black_color,
		XmNbackground,			steelblue_color,
		NULL);

	XtAddCallback (map_button,
		XmNactivateCallback, map_callback, NULL);

/*	The invert button toggles the overlay/background colors from	*
 *	white/black to black/white.					*/

	invert_button = XtVaCreateManagedWidget ("Invert",
		xmPushButtonWidgetClass,	draw_widget,
		XmNx,				330,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		XmNforeground,			black_color,
		XmNbackground,			steelblue_color,
		NULL);

	XtAddCallback (invert_button,
		XmNactivateCallback, invert_callback, NULL);

/*	The cancel button closes the X window and exits the program.	*/

	cancel_button = XtVaCreateManagedWidget ("Quit",
		xmPushButtonWidgetClass,	draw_widget,
		XmNx,				400,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		XmNforeground,			black_color,
		XmNbackground,			red_color,
		NULL);

	XtAddCallback (cancel_button,
		XmNactivateCallback, cancel_callback, NULL);

	left_button = XtVaCreateManagedWidget ("arrow1",
		xmArrowButtonWidgetClass,	draw_widget,
		XmNx,				width-100,
		XmNy,				height-80,
		XmNwidth,			25,
		XmNheight,			25,
		XmNarrowDirection,		XmARROW_LEFT,
		XmNforeground,			black_color,
		XmNbackground,			green_color,
		NULL);

	XtAddCallback (left_button,
		XmNarmCallback, new_LB_product, (XtPointer) -1);

	right_button = XtVaCreateManagedWidget ("arrow1",
		xmArrowButtonWidgetClass,	draw_widget,
		XmNx,				width-60,
		XmNy,				height-80,
		XmNwidth,			25,
		XmNheight,			25,
		XmNarrowDirection,		XmARROW_RIGHT,
		XmNforeground,			black_color,
		XmNbackground,			green_color,
		NULL);

	XtAddCallback (right_button,
		XmNarmCallback, new_LB_product, (XtPointer) 1);

	XtManageChild (draw_widget);

	XtRealizeWidget (top_widget);

/*	Define the various X properties of the draw window.		*/

	display = XtDisplay (draw_widget);
	window  = XtWindow (draw_widget);
	pixmap  = XCreatePixmap (display,
		window,
		width,
		height,
		XDefaultDepthOfScreen(XDefaultScreenOfDisplay(display)));

/*	Define a colormap to be used for drawing data in different	*
 *	colors.  Allow extra colors for overlays.			*/

	cmap = DefaultColormap (display, 0);

	for (i=0;i<COLORS;i++) {

	    color [i].flags = DoRed | DoGreen | DoBlue;
	    color [i].red   = ((unsigned short) Product_Colors [1].red   [i])<<8;
	    color [i].green = ((unsigned short) Product_Colors [1].green [i])<<8;
	    color [i].blue  = ((unsigned short) Product_Colors [1].blue  [i])<<8;

      find_best_color (display, cmap, &color[i]);
	}

	for (i=0;i<=DHR_COLORS;i++)
	{
	    dhr_color [i].flags = DoRed | DoGreen | DoBlue;
            dhr_color [i].red   = ((unsigned short) Dhr_Colors.red   [i])<<8;
            dhr_color [i].green = ((unsigned short) Dhr_Colors.green [i])<<8;
            dhr_color [i].blue  = ((unsigned short) Dhr_Colors.blue  [i])<<8;
      	    find_best_color (display, cmap, &dhr_color[i]);
	}

/*-------------------Base Data Background Color (WHITE)-----------------*/

	color [COLORS].flags = DoRed | DoGreen | DoBlue;
	color [COLORS].red   = (255)<<8;
	color [COLORS].green = (255)<<8;
	color [COLORS].blue  = (255)<<8;

  find_best_color (display, cmap, &color[COLORS]);

/*--------------------Base Data Overlay Color (BLACK)-------------------*/

	color [COLORS+1].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+1].red   = (0)<<8;
	color [COLORS+1].blue  = (0)<<8;
	color [COLORS+1].green = (0)<<8;

  find_best_color (display, cmap, &color[COLORS+1]);

/*----------------------Base Data Overlay Color (RED)-------------------*/

	color [COLORS+2].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+2].red   = (255)<<8;
	color [COLORS+2].blue  = (0)<<8;
	color [COLORS+2].green = (0)<<8;

  find_best_color (display, cmap, &color[COLORS+2]);

/*----------------------Base Data Overlay Color (DARK RED)--------------*/

	color [COLORS+3].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+3].red   = (192)<<8;
	color [COLORS+3].blue  = (0)<<8;
	color [COLORS+3].green = (0)<<8;

  find_best_color (display, cmap, &color[COLORS+3]);

/*---------------------Base Data Overlay Color (YELLOW)------------------*/

	color [COLORS+4].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+4].red   = (255)<<8;
	color [COLORS+4].blue  = (0)<<8;
	color [COLORS+4].green = (255)<<8;
 
  find_best_color (display, cmap, &color[COLORS+4]);

/*---------------------Base Data Overlay Color (DARK YELLOW)-------------*/

	color [COLORS+5].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+5].red   = (128)<<8;
	color [COLORS+5].blue  = (0)<<8;
	color [COLORS+5].green = (128)<<8;

  find_best_color (display, cmap, &color[COLORS+5]);

/*---------------------Base Data Overlay Color (GREEN)-------------------*/

	color [COLORS+6].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+6].red   = (0)<<8;
	color [COLORS+6].blue  = (0)<<8;
	color [COLORS+6].green = (255)<<8;

  find_best_color (display, cmap, &color[COLORS+6]);

/*---------------------Base Data Overlay Color (DARK GREEN)--------------*/

	color [COLORS+7].flags = DoRed | DoGreen | DoBlue;
	color [COLORS+7].red   = (0)<<8;
	color [COLORS+7].blue  = (0)<<8;
	color [COLORS+7].green = (128)<<8;

  find_best_color (display, cmap, &color[COLORS+7]);

/*	Define the Graphics Context to be used in drawing data		*/

	gcv.foreground = color [COLORS].pixel;
	gcv.background = color [0].pixel;
	gcv.graphics_exposures = FALSE;

	gc = XCreateGC (display,
	      window,
	      GCBackground | GCForeground | GCGraphicsExposures, &gcv);

	XtVaSetValues (draw_widget,
		XmNbackground,	color [0].pixel,
		NULL);

/*	Clear the data display portion of the window by filling it	*
 *	with the background color.					*/

	XSetForeground (display, gc,
		color [0].pixel);
	XFillRectangle (display, pixmap,
		gc,
		0, 0, width, height);

/*	Define the data display window properties.			*/

	center_pixel = (width-width/4)/2;
	center_scanl = (width-width/4)/2 + product_start_scanl;
	scale_x      = (width-width/4)/(2.0*range);
	scale_y      = -(width-width/4)/(2.0*range);

	if ((font_info = XLoadQueryFont (display, fontname)) == NULL) {

	    fprintf (stderr,"Unable to load font: %s\n", fontname);

	} else {

	    font = XLoadFont (display, fontname);
	    ttab_font = XmFontListEntryCreate ("ttab_font",
		XmFONT_IS_FONT, (XtPointer) font_info);
	    fontlist = XmFontListAppendEntry (NULL, ttab_font);
	    XmFontListEntryFree (&ttab_font);

	}

/*	Open the product now and display something			*/

	 status = open_product_file (current_file, Current_mode);
         if (status != 0) 
         {
	    if (Current_mode == READ_FROM_DATABASE)
		fprintf (stderr,"ERROR: unable to access product database\n");
	    else {
		fprintf (stderr,"ERROR: unable to open product file \"%s\" [%d]\n", current_file, status);
	       exit (1);
	    }
         }

	if (Num_products <= 0)
       	     XtVaSetValues (top_widget,
		XmNtitle,	"No product in database",
		NULL);
	else
       	     XtVaSetValues (top_widget,
		XmNtitle,	attribute->product_name,
		NULL);

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	xpdt_timer = XtAppAddTimeOut (control_app, next_time, 
		(XtTimerCallbackProc) xpdt_timer_proc, (XtPointer) NULL);

	XtAppMainLoop (control_app);
	exit (0);
}

/************************************************************************
 *	Description: This function is activated when the main window	*
 *		     receives an expose event.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
expose_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
/*	Handle exposure events by copying the pixmap to the display	*
 *	window.								*/

	XCopyArea (display,
		   pixmap,
		   window,
		   gc,
		   0,
		   0,
		   width,
		   height,
		   0,
		   0);

}

/************************************************************************
 *	Description: This function is activated when the main window	*
 *		     receives a resize event.  It is also called 	*
 *		     directly to refresh the product display.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
resize_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
/*	Handle resize events by destroying the old pixmap, redrawing	*
 *	the product data and overlays, and repositioning the buttons.	*/
{
	XRectangle	clip_rectangle;
	Dimension x, y, size;

	void	color_bar ();
	void	overlay_grid ();
	void	overlay_USGS_GRV_file ();
	extern	void	display_radial_data ();
	extern	void	display_dhr_data ();
	extern	void	display_raster_data ();
	extern	void	display_vad_data ();
	extern	void	display_meso_data ();
	extern	void	display_tvs_data ();
	extern	void	display_swp_data ();
	extern	void	display_storm_track_data ();
	extern	void	display_probability_of_hail_data ();
	extern	void	display_graphics_attributes_table ();
	extern	void	display_product_attributes ();
	extern	void	select_hail_thresholds ();
	extern	void	select_storm_track_cells ();

/*	Get the new dimensions of the display window.			*/

	if (Num_products <= 0)
	    return;

	XtVaGetValues (top_widget,
		XmNwidth,	&width,
		XmNheight,	&height,
		NULL);

	size = width;
	if (height < width)
	    size = height;
	if (size < 465)
	    size = 465;
	width = height = size;
	XtVaSetValues (top_widget,
		XmNwidth,	width,
		XmNheight,	height,
		NULL);
	XtVaSetValues (draw_widget,
		XmNwidth,	width,
		XmNheight,	height,
		NULL);

	clip_rectangle.x      = 0;
	clip_rectangle.y      = 0;
	clip_rectangle.width  = width;
	clip_rectangle.height = height;

	XSetClipRectangles (display,
			    gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	product_start_scanl = width/8;

/*	Make sure to get rid of the old pixmap since it is no longer	*
 *	of use.								*/

	XFreePixmap (display, pixmap);

/*	Create a new pixmap of the same size as the new display window.	*/

	pixmap = XCreatePixmap (display,
		window,
		width,
		height,
		XDefaultDepthOfScreen(XDefaultScreenOfDisplay(display)));

/*	Clear the contents of the new pixmap by writing filling in with	*
 *	the background color.						*/

	XSetForeground (display, gc,
		seagreen_color);
	XFillRectangle (display,
			pixmap,
			gc,
			0, 0,
			width,
			height);

	XSetForeground (display, gc,
		color [0].pixel);
	XFillRectangle (display,
			pixmap,
			gc,
			0,
			product_start_scanl,
			(width-width/4),
			(width-width/4));

/*	Check to see if previous product had tabular data and current	*
 *	doesn't.  If so, destroy it.					*/

	if (ttab == (struct tabular_attr *) NULL) {

	    if (ttab_dialog != (Widget) NULL) {

	        XtDestroyWidget (ttab_dialog);
	        ttab_dialog = (Widget) NULL;

	    }

	}

/*	Check to see if a hail product was previously displayed.	*
 *	If so, destroy the hail select popup.				*/

	if ((product_type != hi_type) &&
	    (hail_dialog != (Widget) NULL)) {

	    XtDestroyWidget (hail_dialog);
	    hail_dialog = (Widget) NULL;

	}

/*	Check to see if a storm track product was previously displayed.	*
 *	If so, destroy the storm select popup.				*/

	if ((product_type != sti_type) &&
	    (storm_track_dialog != (Widget) NULL)) {

	    XtDestroyWidget (storm_track_dialog);
	    storm_track_dialog = (Widget) NULL;

	}

/*	Recalculate the scale factors and the coordinates of the window	*
 *	center coordinate.						*/

	if (attribute->full_screen) {

	    if (product_type == radial_type) {

	        scale_x = (width-width/4) / (2*radial_image->data_elements *
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == raster_type) {

	        scale_x = (width-width/4) / (raster_image->number_of_columns *
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == dbv_type) {

		scale_x = (width-width/4) / (2*dhr_image->data_elements*
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == dhr_type) {

		scale_x = (width-width/4) / (2*dhr_image->data_elements*
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == dv_type) {

		scale_x = (width-width/4) / (2*dhr_image->data_elements*
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == dr_type) {

		scale_x = (width-width/4) / (2*dhr_image->data_elements*
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == dvl_type) {

		scale_x = (width-width/4) / (2*dhr_image->data_elements*
			   xy_azran_reso [attribute->product_code][0]);

	    } else if (product_type == generic_radial_type) {

		scale_x = (width-width/4) / (2*radial_image->data_elements*
			   radial_image->range_interval);

	    } else if (product_type == generic_hires_radial_type) {

		scale_x = (width-width/4) / (2*dhr_image->data_elements*
			   dhr_image->range_interval);
	    
	    } else if (product_type == generic_hires_raster_type) {

	        scale_x = (width-width/4) / (raster_image->number_of_columns *
			   xy_azran_reso [attribute->product_code][0]);

	    } else {

		scale_x = 1.0;

	    }

	    scale_y = -scale_x;
	    pixel_offset = 0;
	    scanl_offset = 0;

	} else {

	    scale_x = (width-width/4) / 50.0;
	    scale_y = -scale_x;
	    pixel_offset = -1.852 * attribute->center_range * scale_x *
		cos ((double) (attribute->center_azimuth-90)*DEGRAD);
	    scanl_offset = -1.852 * attribute->center_range * scale_y *
		sin ((double) (attribute->center_azimuth+90)*DEGRAD);

	}

	center_pixel = (width-width/4)/2;
	center_scanl = (width-width/4)/2 + product_start_scanl;

/*	Rescale the font to reflect the window size			*/

	sprintf (buf,"%3i", width/8);
	strncpy (fontname+24,buf,3);

	if ((font_info = XLoadQueryFont (display, fontname)) == NULL) {

	    fprintf (stderr,"Unable to load font: %s\n", fontname);

	} else {

	    font = XLoadFont (display, fontname);
	    label_font = XmFontListEntryCreate ("label_font",
		XmFONT_IS_FONT, (XtPointer) font_info);
	    fontlist = XmFontListAppendEntry (fontlist, label_font);
	    XSetFont (display, gc, font);
	    XmFontListEntryFree (&label_font);

	}


/*	Redisplay the data to the pixmap.				*/

	if (product_type == radial_type) {

	    max_range = radial_image->data_elements *
			xy_azran_reso [attribute->product_code][0];

	    display_radial_data ();

	} else if (product_type == generic_radial_type) {

	    max_range = radial_image->data_elements *
			radial_image->range_interval;

	    display_radial_data ();

	} else if (product_type == cld_type) {

	    max_range = radial_image->data_elements *
			xy_azran_reso [attribute->product_code][0];

	    display_radial_data ();

	} else if (product_type == clr_type) {

	    max_range = radial_image->data_elements *
			xy_azran_reso [attribute->product_code][0];

	    display_radial_data ();

	} else if (product_type == dbv_type) {

	    max_range = dhr_image->data_elements *
			 xy_azran_reso [attribute->product_code][0];

	    display_dhr_data ();

	} else if (product_type == dhr_type) {

	    max_range = dhr_image->data_elements *
			 xy_azran_reso [attribute->product_code][0];

	    display_dhr_data ();

	} else if (product_type == dv_type) {

	    max_range = dhr_image->data_elements *
			 xy_azran_reso [attribute->product_code][0];

	    display_dhr_data ();

	} else if (product_type == dvl_type) {

	    max_range = dhr_image->data_elements *
			 xy_azran_reso [attribute->product_code][0];

	    display_dhr_data ();

	} else if (product_type == dr_type) {

	    max_range = dhr_image->data_elements *
			 xy_azran_reso [attribute->product_code][0];

	    display_dhr_data ();

	} else if (product_type == generic_hires_radial_type) {

	    max_range = dhr_image->data_elements *
			dhr_image->range_interval;

	    display_dhr_data ();

	} else if (product_type == generic_hires_raster_type) {

	    max_range = raster_image->number_of_columns *
			xy_azran_reso [attribute->product_code][0];
	    printf ("Max range: %f\n", max_range);

	    display_hires_raster_data ();

	} else if (product_type == raster_type) {

	    max_range = raster_image->number_of_columns *
			xy_azran_reso [attribute->product_code][0];
/*
	    max_range = (raster_image->number_of_columns *
			   raster_image->x_scale);
*/
	    printf ("Max range: %f\n", max_range);

	    display_raster_data ();

	} else if (product_type == vwp_type) {

	    display_vad_data ();

	} else if (product_type == sti_type) {

	    max_range = 460.0;
	    scale_x = (width-width/4) / 460.0;
	    scale_y = -scale_x;
	    pixel_offset = 0;
	    scanl_offset = 0;

	    if (storm_track_dialog == (Widget) NULL) {

		select_storm_track_cells ();

	    }

	    display_storm_track_data ();

	} else if (product_type == hi_type) {

	    max_range = 460.0;
	    scale_x = (width-width/4) / 460.0;
	    scale_y = -scale_x;
	    pixel_offset = 0;
	    scanl_offset = 0;

	    if (hail_dialog == (Widget) NULL) {

		select_hail_thresholds ();

	    }

	    display_probability_of_hail_data ();

	} else if (product_type == meso_type) {

	    max_range = 460.0;
	    scale_x = (width-width/4) / 460.0;
	    scale_y = -scale_x;
	    pixel_offset = 0;
	    scanl_offset = 0;

	    display_meso_data ();

	} else if (product_type == tvs_type) {

	    max_range = 460.0;
	    scale_x = (width-width/4) / 460.0;
	    scale_y = -scale_x;
	    pixel_offset = 0;
	    scanl_offset = 0;

	    display_tvs_data ();

	} else if (product_type == swp_type) {

	    max_range = 460.0;
	    scale_x = (width-width/4) / 460.0;
	    scale_y = -scale_x;
	    pixel_offset = 0;
	    scanl_offset = 0;

	    display_swp_data ();

	} else if (product_type == ss_type) {

	} else if (product_type == spd_type) {

	} else if (product_type == stand_alone_type) {

	} else {

	    fprintf (stderr,"ERROR: Unsupported product type! %d\n",
			product_type);
	    return;

	}

	if ((product_type != dhr_type) &&
	    (product_type != dbv_type) &&
	    (product_type != dr_type ) &&
	    (product_type != dv_type ) &&
	    (product_type != dvl_type)) {

	    if (gtab != (struct graphic_attr *) NULL) {

	        if (gtab->number_of_lines >= 0) {

		    display_graphics_attributes_table ();

		}

	    }

	}

	if (attribute != (struct product_pertinent *) NULL) {

	    if (attribute->number_of_lines >= 0) {

		display_product_attributes ();

	    }

	}

	XSetForeground (display, gc,
		color [COLORS].pixel);
	XDrawRectangle (display,
			pixmap,
			gc,
			0,
			product_start_scanl,
			(width-width/4),
			(width-width/4));

/*	Display the grid (if appropriate) and a color bar to the left.	*/

	if ((product_type == raster_type) ||
	    (product_type == radial_type) ||
	    (product_type == meso_type)   ||
	    (product_type == tvs_type)    ||
	    (product_type == swp_type)    ||
	    (product_type == sti_type)    ||
	    (product_type == dbv_type)    ||
	    (product_type == dhr_type)    ||
	    (product_type == dv_type)     ||
	    (product_type == dr_type)     ||
	    (product_type == clr_type)    ||
	    (product_type == cld_type)    ||
	    (product_type == dvl_type)    ||
	    (product_type == generic_radial_type)       ||
	    (product_type == generic_hires_radial_type) ||
	    (product_type == generic_hires_raster_type) ||
	    (product_type == hi_type)) {

	    if (grid_overlay_flag)
		overlay_grid ();

	    if (map_overlay_flag)
		overlay_USGS_GRV_file ();

	}

	if ((product_type == raster_type) ||
	    (product_type == radial_type) || 
	    (product_type == swp_type)    ||
	    (product_type == dbv_type)    ||
	    (product_type == dhr_type)    ||
	    (product_type == dr_type)     ||
	    (product_type == dv_type)     ||
	    (product_type == clr_type)    ||
	    (product_type == cld_type)    ||
	    (product_type == dvl_type)    ||
	    (product_type == generic_radial_type)       ||
	    (product_type == generic_hires_radial_type) ||
            (product_type == generic_hires_raster_type) ||
	    (product_type == vwp_type)) {

    	    color_bar ();

	} 

	expose_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

/*	Reposition the buttons at the bottom of the display window.	*/

	XtVaSetValues (file_button,
		XmNx,				10,
		XmNy,				height-40,
		NULL);

	XtVaSetValues (product_button,
		XmNx,				85,
		XmNy,				height-40,
		NULL);

	XtVaSetValues (grid_button,
		XmNx,				160,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		NULL);

	XtVaSetValues (map_button,
		XmNx,				235,
		XmNy,				height-40,
		XmNwidth,			70,
		XmNheight,			30,
		NULL);

	XtVaSetValues (invert_button,
		XmNx,				310,
		XmNy,				height-40,
		NULL);

	XtVaSetValues (cancel_button,
		XmNx,				385,
		XmNy,				height-40,
		NULL);

	if (width < 600) {
	    x = width - 90;
	    y = height - 85;
	}
	else {
	    x = 470;
	    y = height - 40;
	}

	XtVaSetValues (left_button,
		XmNx,				x,
		XmNy,				y,
		NULL);

	XtVaSetValues (right_button,
		XmNx,				x + 40,
		XmNy,				y,
		NULL);

}

/************************************************************************
 *	Description: This function iis used to display a vertical color	*
 *		     bar to the right of the displayed product.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
color_bar ()
/*	This routine displays a vertical color bar along the right	*
 *	side of the display window.					*/
{
	int	i;
	int	ii;
	int	Width;
	int	Height;
	int	scanl = -1;
	float	xheight;
	char	text [64];
	int	start_index;
	int	stop_index;
	float	value;
	Pixel	kolor;

	extern	void	expose_callback ();

/*	Define standard color bar attributes.				*/

	Width = 10;
	xheight = height/(2.0*COLORS);
	Height  = xheight+1;

/*	Handle the special case products first followed by a more	*
 *	generic method.							*/

	if (product_type == swp_type) {		/* SWP Product */

	    for (i=0;i<4;i++) {

		scanl = height/4 + xheight*i;

		switch (i) {

		    case 0 :

			kolor = black_color;
			sprintf (text," 0");
			break;

		    case 1 :

			kolor = cyan_color;
			sprintf (text," 1");
			break;

		    case 2 :

			kolor = yellow_color;
			sprintf (text,"35");
			break;

		    case 3 :

			kolor = red_color;
			sprintf (text,"50");
			break;

		}

		XSetForeground (display,
				gc,
			        kolor);

		XFillRectangle (display,
			pixmap,
			gc,
			width-width/8,
			scanl,
			Width,
			Height);

		XSetForeground (display, gc,
			black_color);

		XDrawRectangle (display,
			pixmap,
		  	gc,
			width-width/8,
			scanl,
			Width,
			Height);

		XDrawString (display,
			pixmap,
			gc,
			width-width/8-30,
			scanl+5,
			text,
			2);

	    }

	    return;

	} else if ((product_type == dhr_type) ||	/* DHR Product */
		   (product_type == dbv_type) ||	/* DBV Product */
		   (product_type == dr_type)  ||	/* DR  Product */
		   (product_type == dv_type)  ||	/* DV  Product */
		   (product_type == generic_hires_radial_type) ||
		   (product_type == generic_hires_raster_type) ||
		   (product_type == dvl_type)) {	/* DVL Product */

	    xheight = height/(2.0*DHR_COLORS);
	    Height  = xheight+1;

	    ii = 8;

	    for (i=0;i<DHR_COLORS;i++) {

		scanl = height/4 + xheight*i;

		XSetForeground (display, gc,
			dhr_color [i].pixel);
		XFillRectangle (display,
				pixmap,
				gc,
				width-width/8,
				scanl,
				Width,
				Height);

		ii++;

		if (ii >= 8) {

		    ii = 0;

		    XSetForeground (display, gc,
		 		black_color);

		    if (product_code == dvl_type) {	/* DVL Product */

                       if (i*4 < attribute->log_start) {

		          value = (i*4 - attribute->linear_offset)/
				  attribute->linear_coeff;

                       } else {

		          value = (i*4 - attribute->log_offset)/
				  attribute->log_coeff;
                          value = exp ((double) value);

		       }

		       sprintf (text,"%4.1f",value);

		    } else {

		       value = attribute->min_value + i*4*attribute->inc_value;
		       sprintf (text,"%d",(int) value);
		    }
	  	    XDrawString (display,
				 pixmap,
				 gc,
				 width-width/8-40,
				 scanl+5,
				 text,
				 strlen (text));

		}
	    }

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
		    pixmap,
		    gc,
		    width-width/8,
		    height/4,
		    Width,
		    scanl+Height-height/4);

	    expose_callback ((Widget) NULL,
		    (XtPointer) NULL, (XtPointer) NULL);

	    return;

	} else if (product_code == 34) {	/* CFC Product */

	    XSetForeground (display, gc,
		    black_color);

	    Height = Height - 2;

	    scanl = height/4 + xheight;

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-70,
			 scanl+5,
			 "DISABLE FILTER",
			 14);

	    scanl = height/4 + 2*xheight;

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-70,
			 scanl+5,
			 "(OP SEL CODE 0)",
			 15);

	    scanl = height/4 + 3*xheight;

	    XSetForeground (display, gc,
			color [0].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    white_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "FILTER OFF",
			 10);

	    scanl = height/4 + 5*xheight;

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-70,
			 scanl+5,
			 "BYPASS MAP IN CTRL",
			 18);

	    scanl = height/4 + 6*xheight;

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-70,
			 scanl+5,
			 "(OP SEL CODE 1)",
			 15);

	    scanl = height/4 + 7*xheight;

	    XSetForeground (display, gc,
			color [2].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "NO CLUTTER",
			 10);

	    scanl = height/4 + 8*xheight;

	    XSetForeground (display, gc,
			color [4].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "LOW     (1)",
			 11);

	    scanl = height/4 + 9*xheight;

	    XSetForeground (display, gc,
			color [6].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "MEDIUM  (2)",
			 11);

	    scanl = height/4 + 10*xheight;

	    XSetForeground (display, gc,
			color [8].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "HIGH    (3)",
			 11);

	    scanl = height/4 + 12*xheight;

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-70,
			 scanl+5,
			 "FORCE FILTER",
			 12);

	    scanl = height/4 + 13*xheight;

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-70,
			 scanl+5,
			 "(OP SEL CODE 2)",
			 15);

	    scanl = height/4 + 14*xheight;

	    XSetForeground (display, gc,
			color [10].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "LOW     (1)",
			 11);

	    scanl = height/4 + 15*xheight;

	    XSetForeground (display, gc,
			color [12].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "MEDIUM  (2)",
			 11);

	    scanl = height/4 + 16*xheight;

	    XSetForeground (display, gc,
			color [14].pixel);
	    XFillRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XSetForeground (display, gc,
		    black_color);

	    XDrawRectangle (display,
			    pixmap,
			    gc,
			    width-width/8-70,
			    scanl-5,
			    Width*2,
			    Height);

	    XDrawString (display,
			 pixmap,
			 gc,
			 width-width/8-60+2*Width,
			 scanl+5,
			 "HIGH    (3)",
			 11);

	    return;

	}

	Height  = Height * COLORS / attribute->num_data_levels;
	xheight = (xheight * COLORS) / attribute->num_data_levels;
	color_step = COLORS / attribute->num_data_levels;

	if (color_step <= 0) {

	    fprintf (stderr,"WARNING: check number of data levels -> assume 16\n");
	    color_step = 1;

	}

	if (product_type == vwp_type) {

	    start_index = 1;

	} else {

	    start_index = 0;

	}

	stop_index  = COLORS;

	if (attribute->num_data_levels == COLORS) {

	    stop_index  = COLORS;
	    color_step = 1;

	} else if (attribute->num_data_levels == 8) {

	    stop_index  = COLORS;
	    color_step = 2;

	} else {

	    stop_index  = attribute->num_data_levels;
	    color_step = 1;
/*
	} else if (attribute->num_data_levels == 6) {

	    stop_index  = attribute->num_data_levels;
	    color_step = 1;
*/
	}

	ii = start_index;

	for (i=start_index;i<stop_index;i=i+color_step) {

		scanl = height/4 + xheight*(i-start_index)/color_step;

		XSetForeground (display, gc,
			color [i].pixel);
		XFillRectangle (display,
			pixmap,
			gc,
			width-width/8,
			scanl,
			Width,
			Height);

		XSetForeground (display, gc,
			black_color);

		if (attribute->data_levels [ii] != NULL) {

		    XDrawString (display,
			pixmap,
			gc,
			width-width/8-70,
			scanl+5,
			(char *) attribute->data_levels [ii],
			strlen ((char *) attribute->data_levels [ii]));

		}

		ii++;

	}

	XSetForeground (display, gc,
		black_color);

	XDrawRectangle (display,
		pixmap,
		gc,
		width-width/8,
		height/4,
		Width,
		scanl+Height-height/4);

	expose_callback ((Widget) NULL,
		(XtPointer) NULL, (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function displays a polar grid over an erath	*
 *		     locatable product.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
overlay_grid ()
/*	This routine displays a polar grid in the display window.	*/
{
	int	i;
	int	size;
	int	pixel;
	int	scanl;
	int	max;
	int	interval;
	XRectangle	clip_rectangle;

/*	Define a clip rectangle so that overlay stays inside product	*
 *	display region.							*/

	clip_rectangle.x      = 0;
	clip_rectangle.y      = product_start_scanl;
	clip_rectangle.width  = 3*width/4;
	clip_rectangle.height = 3*width/4;

	XSetClipRectangles (display,
			    gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

	XSetForeground (display, gc,
		color [COLORS].pixel);

	max = max_range * (60.0/111.12);

	interval = 50;

	if ((product_type != dhr_type) &&
	    (product_type != dbv_type) &&
	    (product_type != dr_type ) &&
	    (product_type != dv_type ) &&
	    (product_type != generic_hires_radial_type ) &&
	    (product_type != dvl_type)) {

	    if (attribute->full_screen) {

	        if (max_range > 150.0) {

	            interval = 50;

	        } else {

	            interval = 25;

	        }

	    } else {

	        interval = 10;

	    }

	}

        if (kilometer)
           interval = (int)((float)interval * (111.12/60.0));

/*	Display a range ring at each interval.				*/

	for (i=interval;i<=max;i=i+interval) {

/*	    Radar range is referenced in kilometers, range rings in	*
 *	    nautical miles.  Apply the conversion factor.		*/

            if (kilometer)
               size = i * scale_x;
            else
	       size  = i*scale_x * (111.12/60.0);
	    pixel = center_pixel + pixel_offset - size;
	    scanl = center_scanl + scanl_offset - size;

	    XDrawArc (display,
		      pixmap,
		      gc,
		      pixel,
		      scanl,
		      size*2,
		      size*2,
		      0,
		      -(360*64));

            if (kilometer)
	       sprintf (buf,"%d km",i);
            else
               sprintf (buf,"%d nm",i);

	    XDrawString (display,
			 pixmap,
			 gc,
			 center_pixel + pixel_offset - 4*strlen (buf),
			 scanl + 4,
			 buf,
			 strlen (buf));
	}

	for (i=0;i<360;i=i+45) {

	    pixel = max_range * cos ((double) (i+90)*DEGRAD) * scale_x +
		    center_pixel + pixel_offset;
	    scanl = max_range * sin ((double) (i-90)*DEGRAD) * scale_y +
		    center_scanl + scanl_offset;

	    XDrawLine (display,
		       pixmap,
		       gc,
		       center_pixel+pixel_offset,
		       center_scanl+scanl_offset,
		       pixel,
		       scanl);

	}

	clip_rectangle.x      = 0;
	clip_rectangle.y      = 0;
	clip_rectangle.width  = width;
	clip_rectangle.height = height;

	XSetClipRectangles (display,
			    gc,
			    0,
			    0,
			    &clip_rectangle,
			    1,
			    Unsorted);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Grid" button.  It toggles the grid overlay	*
 *		     on/off.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
grid_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
/*	This routine toggles the display of the grid in the display	*
 *	window.								*/
{
	void	resize_callback ();
	void	overlay_grid ();

	if (grid_overlay_flag) {

	    str = XmStringCreateLocalized ("Grid On");

	} else {

	    str = XmStringCreateLocalized ("Grid Off");
	    overlay_grid ();

	}

	XtVaSetValues (grid_button,
		XmNlabelString,			str,
		NULL);

	XmStringFree (str);

	grid_overlay_flag = 1-grid_overlay_flag;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Map" button.  Map overlays are toggled	*
 *		     on/off.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
map_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
/*	This routine toggles the display of the map in the display	*
 *	window.								*/
{
	void	resize_callback ();
	void	overlay_USGS_GRV_file ();

	if (map_overlay_flag) {

	    str = XmStringCreateLocalized ("Map On");

	} else {

	    str = XmStringCreateLocalized ("Map Off");
	    overlay_USGS_GRV_file ();

	}

	XtVaSetValues (map_button,
		XmNlabelString,			str,
		NULL);

	XmStringFree (str);

	map_overlay_flag = 1-map_overlay_flag;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Invert" button.  It toggles the background	*
 *		     color between black/white.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
invert_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
/*	This routine inverts the overlay/background colors from		*
 *	white/black to black/white.					*/
{
	XColor	Color;
	int	i;

	void	resize_callback (Widget w, XtPointer client_data,
			XtPointer call_data);

	Color.red   = color [0].red;
	Color.green = color [0].green;
	Color.blue  = color [0].blue;

	color [0].red   = color [COLORS].red;
	color [0].green = color [COLORS].green;
	color [0].blue  = color [COLORS].blue;

	color [COLORS].red   = Color.red;
	color [COLORS].green = Color.green;
	color [COLORS].blue  = Color.blue;

	Color.red   = color [COLORS+2].red;
	Color.green = color [COLORS+2].green;
	Color.blue  = color [COLORS+2].blue;

	color [COLORS+2].red   = color [COLORS+3].red;
	color [COLORS+2].green = color [COLORS+3].green;
	color [COLORS+2].blue  = color [COLORS+3].blue;

	color [COLORS+3].red   = Color.red;
	color [COLORS+3].green = Color.green;
	color [COLORS+3].blue  = Color.blue;

	Color.red   = color [COLORS+4].red;
	Color.green = color [COLORS+4].green;
	Color.blue  = color [COLORS+4].blue;

	color [COLORS+4].red   = color [COLORS+5].red;
	color [COLORS+4].green = color [COLORS+5].green;
	color [COLORS+4].blue  = color [COLORS+5].blue;

	color [COLORS+5].red   = Color.red;
	color [COLORS+5].green = Color.green;
	color [COLORS+5].blue  = Color.blue;

	Color.red   = color [COLORS+6].red;
	Color.green = color [COLORS+6].green;
	Color.blue  = color [COLORS+6].blue;

	color [COLORS+6].red   = color [COLORS+7].red;
	color [COLORS+6].green = color [COLORS+7].green;
	color [COLORS+6].blue  = color [COLORS+7].blue;

	color [COLORS+7].red   = Color.red;
	color [COLORS+7].green = Color.green;
	color [COLORS+7].blue  = Color.blue;

	for (i=0;i<COLORS+8;i++) {

   find_best_color (display, cmap, &color[i]);

	}

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Quit" button.  The application is closed.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
cancel_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtDestroyWidget (top_widget);
	exit (0);
}

/************************************************************************
 *	Description: This function is activated when the user moves	*
 *		     the cursor inside the display region and pressed	*
 *		     mouse buttons.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
input_callback (
Widget		w,
XtPointer	client_data,
XmDrawingAreaCallbackStruct	*cbs
)
/*	This routine handles mouse events inside the display window.	*
 *	If the left button is pressed, position information is 		*
 *	displayed along with any data value at that point.		*/
{
	float	x;
	float	y;
	float	angle;
	float	range, disp_range;
	int	value;
	int	element;
	int	i, j;
	float	pixel_int;
	float	scanl_int;
	int	pixel;
	int	scanl;

	extern	void	display_graphics_attributes_table ();

	XEvent	*event = cbs->event;

	if (event->xany.type == ButtonPress) {

	    pixel = event->xbutton.x;
	    scanl = event->xbutton.y;

	    if (scanl < product_start_scanl) {

		current_row = current_row + page_size;

		if (gtab == NULL) {

		    return;

		}

		if (current_row > gtab->number_of_lines) {

		    current_row = 0;

		}

		display_graphics_attributes_table ();
		resize_callback ((Widget) NULL,
			(XtPointer) NULL,
			(XtPointer) NULL);
		return;

	    }

	    x = (pixel-center_pixel-pixel_offset)/scale_x;
	    y = (scanl-center_scanl-scanl_offset)/scale_y;


		range = sqrt ((double) x*x + y*y);

		if (fabs ((double) y) > 0.001) {

		    angle = atan( fabs ((double) x/y));

		} else {

		    angle = 3.14159265/2.0;

		}

	        if (x >= 0.0) {

		    if (y < 0.0) {

			angle = 3.14159265 - angle;

		    }

		} else {

		    if (y >= 0.0) {

			angle = 2 * 3.14159265 - angle;

		    } else {

			angle = 3.14159265 + angle;

		    }

		}

	        angle = angle / DEGRAD;

		if (angle >= 360.0) {

		    angle = angle - 360.0;

		} else if (angle < 0.0) {

		    angle = angle + 360.0;

		}

               if (!kilometer)
               {
                  disp_range = range * (60.0/111.12);
               }
               else
               {
                  disp_range = range;
               }

            /*  Convert range to nm if we are viewing in nm  */
	    if (product_type == radial_type) {


		element = range / xy_azran_reso [attribute->product_code][0];

		if (element >= radial_image->data_elements) {

		    value = -1;

		} else {

		    value = -2;

		    for (i=0;i<radial_image->number_of_radials;i++) {

			if ((angle >= radial_image->azimuth_angle [i]) &&
			    (angle <= radial_image->azimuth_angle [i] + radial_image->azimuth_width [i])) {
			    angle = radial_image->azimuth_angle [i] +
				    radial_image->azimuth_width [i]/2.0;
			    range = element * xy_azran_reso [attribute->product_code][0];
			    value = radial_image->radial_data [i][element];
			    break;

			}

		    }

		}

	        sprintf (buf,"Azi: %7.1f: Ran: %7.1f Value: %2d\n", angle, disp_range,
		    value);

		XSetForeground (display, gc,
			white_color);

		XDrawImageString (display,
				  window,
				  gc,
				  (width-width/4)/4,
				  (width-width/4+20+width/8),
				  buf,
				  strlen (buf)-1);

	    } else if ((product_type == raster_type) ||
		       (product_type == generic_hires_raster_type))
	       {

		pixel_int = ((float) (width-width/4)) / raster_image->number_of_columns;
		scanl_int = pixel_int;

		i = pixel / pixel_int;
		j = (scanl-product_start_scanl) / scanl_int;

		if ((i < raster_image->number_of_rows) &&
		    (j < raster_image->number_of_columns)) {

		    value = raster_image->raster_data [j][i];

		    XSetForeground (display, gc, white_color);

		    if (product_type == generic_hires_raster_type)
		    {
			float data_value;
			if (value == 0)
			    data_value = 0;
			else
			if (value == 1)
			    data_value = 1;
			else
			    data_value = (value * attribute->inc_value) + attribute->min_value;


/* 			printf("data value = (%d * %f) + %f = %f, x_scale = %f, y_scale=%f\n",
				value, attribute->inc_value, attribute->min_value, data_value,
			        raster_image->x_scale, raster_image->y_scale); */
	                sprintf (buf,"Azi: %7.1f: Ran: %7.1f Value: %3.2f\n", angle, disp_range,
		                        data_value);

		    }
		    else
		    {
		       sprintf (buf,"Grid Coordinate (%3i,%3i) - Value: %d   ", j, i, value);
		    }

		     XDrawImageString (display,
				  window,
				  gc,
				  (width-width/4)/4,
				  (width-width/8+20),
				  buf,
				  strlen (buf)-1);

		}

	    }

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the left and right arrow buttons.  If the left	*
 *		     arrow is selected, the product at the previous	*
 *		     volume time is displayed (if available).  If the	*
 *		     right arrow is selected, the product at the next	*
 *		     volume time is displayed (if available).		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - -1 = left button, 1 = right button	*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

static void new_LB_product (Widget w, XtPointer client_data, 
						XtPointer call_data) {
    int status;

    Current_time = Current_time - (int) client_data;

    if (Current_time < 0) {
	Current_time = 0;
	XtVaSetValues (left_button, XmNbackground, green_color, NULL);
	XtVaSetValues (right_button, XmNbackground, white_color, NULL);
    }
    else if (Current_time >= Num_times) {
	Current_time = Num_times-1;
	XtVaSetValues (left_button, XmNbackground, white_color, NULL);
	XtVaSetValues (right_button, XmNbackground, green_color, NULL);
    } 
    else {
	update_select_product_list ();
	XtVaSetValues (right_button, XmNbackground, green_color, NULL);
	XtVaSetValues (left_button, XmNbackground, green_color, NULL);
    }

	    if (Current_item <= Num_msg_ids) {

	        status = open_product_file (current_file, Current_mode);
	        auto_display_flag = 1;

	    }
    return;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Product" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
product_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
static	Widget	form         = (Widget) NULL;
static	Widget	product_main = (Widget) NULL;
static	Widget	time_main    = (Widget) NULL;
static	Widget	time_list    = (Widget) NULL;
	Widget	button;
	Widget	label;
	Widget	time_frame;
	Widget	product_frame;
	Widget	product_list_frame;

	int	i, j, n;
	int	cnt;
	int	month, day, year;
	int	hour, minute, second;
	int	buf_num;
	Arg	args [16];

	long int	seconds;
	XmString	str;

/*	If the Products window is not yet defined, create and display	*
 *	it.								*/

	if (Product_dialog == (Widget) NULL) {

	    if (Called_from_timer)
		return;

	    Product_dialog = XtVaCreatePopupShell ("Products in Database",
		xmDialogShellWidgetClass,	w,
		XmNdeleteResponse,		XmDESTROY,
		NULL);

	    XtAddCallback (Product_dialog,
		XmNdestroyCallback, destroy_products_dialog_callback, NULL);

	    form = XtVaCreateWidget ("products_form",
		xmFormWidgetClass,	Product_dialog,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		XmNwidth,		500,
		NULL);

	    rowcol = XtVaCreateWidget ("products_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		XmNpacking,		XmPACK_TIGHT,
		XmNorientation,		XmHORIZONTAL,
		NULL);

	    button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		black_color,
		XmNbackground,		steelblue_color,
		NULL);

	    XtAddCallback (button,
		XmNactivateCallback, close_product_callback, NULL);

	    label = XtVaCreateManagedWidget ("Spacer",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		seagreen_color,
		XmNbackground,		seagreen_color,
		NULL);

	    time_frame = XtVaCreateManagedWidget ("time_frame",
		xmFrameWidgetClass,	rowcol,
		XmNforeground,		seagreen_color,
		XmNbackground,		seagreen_color,
		NULL);

	    label = XtVaCreateManagedWidget ("Volume Time List",
		xmLabelWidgetClass,	time_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		NULL);

	    time_main = XtVaCreateWidget ("time_list",
		xmComboBoxWidgetClass,	time_frame,
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNforeground,		black_color,
		XmNbackground,		steelblue_color,
		NULL);

	    XtVaGetValues (time_main,
		XmNlist,	&time_list,
		NULL);

/*
	    Num_times = 0;

	    Query_data [0].field = ORPGDBM_MODE;
	    Query_data [0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			       ORPGDBM_DISTINCT_FIELD_VALUES;
	    Query_data [1].field = RPGP_VOLT;
	    Query_data [1].value = 0;

	    Num_times = ORPGDBM_query (Db_info,
				   Query_data,
				   2,
				   MAX_PRODUCTS_IN_LIST);
*/
	    Num_times = XQ_get_all_vol_times (Db_info, MAX_PRODUCTS_IN_LIST);

	    for (i=0;i<Num_times;i++) {

	        seconds = Db_info [i].vol_t;

	        Time_list [i] = seconds;

	        unix_time ( &seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

	        sprintf (buf,"%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
	    	    Month_string [month], day, year,
		    hour, minute, second);

		if (i == 0) {

		    if (debug)
			fprintf (stderr,"Time [0]: %s\n", buf);

		}

	        str = XmStringCreateLocalized (buf);
	        XmListAddItemUnselected (time_list, str, 0);
	        XmStringFree (str);

	    }

	    XtManageChild (time_main);

	    XtVaSetValues (time_main,
		XmNselectedPosition,	0,
		NULL);

	    XtManageChild (rowcol);

	    product_frame = XtVaCreateManagedWidget ("product_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	    label = XtVaCreateManagedWidget ("Product Types",
		xmLabelWidgetClass,	product_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		NULL);

	    product_main = XtVaCreateWidget ("products_list",
		xmComboBoxWidgetClass,	product_frame,
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNforeground,		black_color,
		XmNbackground,		steelblue_color,
		NULL);

	    XtVaGetValues (product_main,
		XmNlist,	&product_list,
		XmNtextField,	&product_text,
		NULL);

/*
	    Num_products = 0;

	    Query_data [0].field = ORPGDBM_MODE;
	    Query_data [0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			       ORPGDBM_DISTINCT_FIELD_VALUES;
	    Query_data [1].field = RPGP_PCODE;
	    Query_data [1].value = 0;

	    Num_products = ORPGDBM_query (Db_info,
				      Query_data,
				      2,
				      MAX_PRODUCTS_IN_LIST);
*/
	    Num_products = XQ_get_all_product_code (Db_info, 
						MAX_PRODUCTS_IN_LIST);

	    cnt = 0;

	    for (i=16;i<300;i++) {

	        for (j=0;j<Num_products;j++) {

		    if (i == Db_info [j].prod_code) {

		        buf_num = ORPGPAT_get_prod_id_from_code (i);

		        if (buf_num < 0) {

			    break;

		        }

		        Product_list [cnt] = buf_num;
		        cnt++;

		        sprintf (buf,"%3.3s[%2d] - %s",
			    ORPGPAT_get_mnemonic (buf_num),
			    i,
			    ORPGPAT_get_description (buf_num, STRIP_MNEMONIC));
		        str = XmStringCreateLocalized (buf);
		        XmListAddItemUnselected (product_list, str, 0);
		        XmStringFree (str);

		    }
	        }
	    }

	    XtManageChild (product_main);

	    XtVaSetValues (product_main,
		XmNselectedPosition,	Current_product,
		NULL);

	    product_list_frame = XtVaCreateManagedWidget ("product_list_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		product_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	    label = XtVaCreateManagedWidget ("Products List",
		xmLabelWidgetClass,	product_list_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		black_color,
		XmNbackground,		seagreen_color,
		NULL);

	    n = 0;

	    XtSetArg (args[n], XmNvisibleItemCount, 4);			n++;
	    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);	n++;
	    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);	n++;
	    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	    XtSetArg (args[n], XmNtopWidget, product_list_frame);	n++;
	    XtSetArg (args[n], XmNforeground, black_color);		n++;
	    XtSetArg (args[n], XmNbackground, lightsteelblue_color);	n++;

	    Product_scroll = XmCreateScrolledList (form, "product_scroll",
				args, n);

	    XtAddCallback (Product_scroll,
		XmNbrowseSelectionCallback, select_product_from_list,
		NULL);

	    XtManageChild (Product_scroll);

	    XtManageChild (form);

	    update_select_product_list ();

	    XtAddCallback (time_main,
		XmNselectionCallback, select_new_time, NULL);

	    XtAddCallback (product_main,
		XmNselectionCallback, select_new_product, NULL);

	    XtPopup (Product_dialog, XtGrabNone);

	    XtVaSetValues (left_button,
		XmNbackground,	green_color,
		NULL);
	    XtVaSetValues (right_button,
		XmNbackground,	white_color,
		NULL);

	} else {

	    XmListDeleteItemsPos (product_list, Num_products, 1);
	    XmListDeleteItemsPos (time_list, Num_times, 1);

/*
	    Num_products = 0;

	    Query_data [0].field = ORPGDBM_MODE;
	    Query_data [0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			       ORPGDBM_DISTINCT_FIELD_VALUES;
	    Query_data [1].field = RPGP_PCODE;
	    Query_data [1].value = 0;

	    Num_products = ORPGDBM_query (Db_info,
				      Query_data,
				      2,
				      MAX_PRODUCTS_IN_LIST);
*/
	    Num_products = XQ_get_all_product_code (Db_info, 
						MAX_PRODUCTS_IN_LIST);

	    cnt = 0;

	    for (i=16;i<300;i++) {

	        for (j=0;j<Num_products;j++) {

		    if (i == Db_info [j].prod_code) {

		        buf_num = ORPGPAT_get_prod_id_from_code (i);

		        if (buf_num < 0) {

			    break;

		        }

		        Product_list [cnt] = buf_num;
		        cnt++;

		        sprintf (buf,"%3.3s[%2d] - %s",
			    ORPGPAT_get_mnemonic (buf_num),
			    i,
			    ORPGPAT_get_description (buf_num, STRIP_MNEMONIC));
		        str = XmStringCreateLocalized (buf);
		        XmListAddItemUnselected (product_list, str, 0);
		        XmStringFree (str);

		    }
	        }
	    }

	    XtManageChild (product_main);

	    XtVaSetValues (product_main,
		XmNselectedPosition,	Current_product,
		NULL);

/*
	    Num_times = 0;

	    Query_data [0].field = ORPGDBM_MODE;
	    Query_data [0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			       ORPGDBM_DISTINCT_FIELD_VALUES;
	    Query_data [1].field = RPGP_VOLT;
	    Query_data [1].value = 0;

	    Num_times = ORPGDBM_query (Db_info,
				   Query_data,
				   2,
				   MAX_PRODUCTS_IN_LIST);
*/
	    Num_times = XQ_get_all_vol_times (Db_info, MAX_PRODUCTS_IN_LIST);

	    for (i=0;i<Num_times;i++) {

	        seconds = Db_info [i].vol_t;

	        Time_list [i] = seconds;

	        unix_time ( &seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

	        sprintf (buf,"%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
		    Month_string [month], day, year,
		    hour, minute, second);
	        str = XmStringCreateLocalized (buf);
	        XmListAddItemUnselected (time_list, str, 0);
	        XmStringFree (str);

	    }

	    XtManageChild (time_main);

	    XtVaSetValues (time_main,
		XmNselectedPosition,	Current_time,
		NULL);

	    if (!Called_from_timer)
		update_select_product_list ();

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Open" or "Save" buttons in the File window.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - OPEN_FILE, SAVE_FILE			*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
choose_file_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	select_product_file ();
	void	do_nothing ();

/*	XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct *) call_data; */
	file_dialog = XmCreateFileSelectionDialog (w, "SelectProductFile", NULL, 0);

	XtVaSetValues (file_dialog,
		XmNforeground,	white_color,
		XmNbackground,	steelblue_color,
		NULL);

	if (strlen (Prod_dir) > 0) {
	    str = XmStringCreateLocalized (Prod_dir);
	    XtVaSetValues (file_dialog,
		XmNdirectory, str,
		NULL);
	    XmStringFree (str);
	}

	XtAddCallback (file_dialog, XmNcancelCallback, do_nothing, NULL);
	XtAddCallback (file_dialog, XmNokCallback, select_product_file,
			(XtPointer) client_data);

	XtManageChild (file_dialog);
	XtRealizeWidget (file_dialog);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Cancel" button from the file selection	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
do_nothing (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtDestroyWidget (w);
	file_dialog = (Widget) NULL;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a filename from the file selection window.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - OPEN_FILE, SAVE_FILE			*
 *		call_data - file selection data				*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
select_product_file (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
    char *filename, *data;
    int	status, len;
    XmFileSelectionBoxCallbackStruct *cbs =
	    (XmFileSelectionBoxCallbackStruct *) call_data;

    if (!XmStringGetLtoR (cbs->value, 
	    XmSTRING_DEFAULT_CHARSET, &filename)) {
	XtFree (filename);
	XtDestroyWidget (w);
	return;
    }

    if (!*filename) {
	fprintf (stderr, "No product file selected\n");
	XtFree (filename);
	XtDestroyWidget (w);
	return;
    }

/*
    strcpy (current_file, filename);
    New_product_file = 1;
    status = open_product_file (filename, Current_mode);
*/
    if ((len = XQ_read_product_by_name (filename, &data)) > 0)
	Display_product (data, len);

    XtFree (filename);
    if (status)
	return;
/*    XtDestroyWidget (w); */
}

/************************************************************************
 *	Description: This function is used to open a product file for	*
 *		     reading.						*
 *									*
 *	Input:  filename - product filename				*
 *		mode - READ_FROM_FILENAME, READ_FROM_DATABASE		*
 *	Output: NONE							*
 *	Return: read status (negative indicates error)			*
 ************************************************************************/

static int open_product_file (char *filename, int mode) {
    char *lb_data;
    static int first_time = 1;

    if (first_time) {
	product_callback (product_button, (XtPointer) NULL,
			  (XtPointer) NULL);
	first_time = 0;
    }
    if (Num_msg_ids <= 0)
	return (-1);

    prod_len = XQ_read_product (Msg_id_list[Current_item - 1], &lb_data);
    if (prod_len > 0)
	Display_product (lb_data, prod_len);
    if (lb_data != NULL)
	free (lb_data);
    if (prod_len <= 0)
	return (-1);
    return (0);
}

/**************************************************************************

    Displays product "prod_with_hd" (including 96 byte header) of length 
    "prod_len".

**************************************************************************/

static int Display_product (char *prod_with_hd, int prod_len) {

	int free_uncompress, pcode, i;
	int size = prod_len - sizeof(Prod_header);
	Graphic_product *phd = NULL;

	product_data = (short *)prod_with_hd + (sizeof (Prod_header)/2);
        phd = (Graphic_product *) product_data;

	if (!MISC_i_am_bigendian ())
	    UMC_msg_hdr_desc_blk_swap( product_data );

/*	Use decode product to open the product and read the product	*
 *	data and translate it.						*/

	if ((product_code >= 16) &&
	    (product_code <= 2999)) {

	    free_memory (product_code);

	}

/*	Get the product code for the current product. 			*/
	pcode = phd->msg_code;

/* Check to see if the product needs to be decompressed.  If so,        *
 * decompress it.                                                       */

        free_uncompress = 0;
	if (debug)
	    fprintf (stderr,"ORPGPAT_get_compression_type (%d) = %d\n",
	      pcode, ORPGPAT_get_compression_type ( ORPGPAT_get_prod_id_from_code (pcode)));
	if (ORPGPAT_get_compression_type( ORPGPAT_get_prod_id_from_code (pcode)) > 0) {

	    fprintf (stderr,"Product [%d] is compressed\n", pcode);
            
	    uncompress = Decompress_product ( product_data, &size );
	    product_data = uncompress;

            free_uncompress = 1;

	}

/*	Check to see if byte swapping needed.			*/

	if (!MISC_i_am_bigendian ()) {
	    fprintf(stdout, "Swapping product data\n ");

	    UMC_msg_hdr_desc_blk_swap( product_data );
	    UMC_icd_to_product( product_data, size );
	}

	product_type = decode_product (product_data);

	if (product_type <= 0) {

	    fprintf (stderr,
		"Bad product code %d returned from decode_product\n",
		product_type);

	    return (0);

	}

	current_row = 0;

	if ((product_type != dhr_type) &&
	    (product_type != dbv_type) &&
	    (product_type != dr_type ) &&
	    (product_type != dv_type ) &&
	    (product_type != generic_hires_radial_type ) &&
	    (product_type != dvl_type)) {

	    decode_grafattr_block (&product_data[grafattr_block]);

	    if (gtab != (struct graphic_attr *) NULL) {

	        if (gtab->number_of_lines >= 0) {

		    for (i=0;i<=gtab->number_of_lines;i++) {

		        fprintf (stderr,"%s\n",gtab->text [i]);

		    }
	        }
	    }

	    decode_tabular_block (&product_data[tabular_block],
				  (unsigned short) product_type);

	    if (ttab != (struct tabular_attr *) NULL) {

		display_tabular_data ();

	    }

	    if (attribute->product_code < 300) {

	        for (i=1;i<COLORS;i++) {

		    color [i].red   =
			    ((unsigned short) Product_Colors [attribute->product_code-1].red   [i])<<8;
		    color [i].green =
			((unsigned short) Product_Colors [attribute->product_code-1].green [i])<<8;
		    color [i].blue  =
			((unsigned short) Product_Colors [attribute->product_code-1].blue  [i])<<8;

       find_best_color (display, cmap, &color[i]);
		}
	    }
	}

        if( free_uncompress )
           free( uncompress ); 

/*	Reset the color table according to the product number	*/

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);

	XtVaSetValues (top_widget,
		XmNtitle,	attribute->product_name,
		NULL);

    return (0);
}

/************************************************************************
 *	Description: This function iis the timer procedure for the	*
 *		     task.						*
 *									*
 *	Input:  w - timer parent widget ID				*
 *		id - timer interval ID					*
 *	Output: NONE							*
 *	Return:	0							*
 ************************************************************************/

static XtTimerCallbackProc xpdt_timer_proc (Widget w, XtIntervalId id) {

	XQ_routine ();

	Called_from_timer = 1;
	product_callback (product_button,
				  (XtPointer) NULL,
				  (XtPointer) NULL);
	Called_from_timer = 0;

	xpdt_timer = XtAppAddTimeOut (control_app, next_time, 
		(XtTimerCallbackProc) xpdt_timer_proc, (XtPointer) NULL);

	return ((XtTimerCallbackProc) 0);
}

/************************************************************************
 *	Description: This function is activated when the Product window	*
 *		     is destroyed.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
destroy_products_dialog_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Product_dialog = (Widget) NULL;
	Product_scroll = (Widget) NULL;
	Num_msg_ids = 0;
}

/************************************************************************
 *	Description: This function is activated when the File window	*
 *		     is destroyed.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
destroy_file_dialog_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	File_dialog = (Widget) NULL;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a new product from the product select menu.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - combo box data				*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
select_new_product (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	XmComboBoxCallbackStruct	*cbs =
		(XmComboBoxCallbackStruct *) call_data;

	if (Called_from_timer)
	    return;

	Current_product = (int) cbs->item_position;

	update_select_product_list ();
	Current_item = 1;

	status = open_product_file (current_file, Current_mode);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a new volume time from the time select menu.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - combo box data				*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
select_new_time (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	XmComboBoxCallbackStruct	*cbs =
		(XmComboBoxCallbackStruct *) call_data;

	if (Called_from_timer)
	    return;

	Current_time = (int) cbs->item_position;

	update_select_product_list ();

	if (Current_time == 0) {

	    XtVaSetValues (left_button,
		XmNbackground,	white_color,
		NULL);
	    XtVaSetValues (right_button,
		XmNbackground,	green_color,
		NULL);

	} else if (Current_time == Num_times) {

	    XtVaSetValues (left_button,
		XmNbackground,	green_color,
		NULL);
	    XtVaSetValues (right_button,
		XmNbackground,	white_color,
		NULL);

	} else {

	    XtVaSetValues (left_button,
		XmNbackground,	green_color,
		NULL);
	    XtVaSetValues (right_button,
		XmNbackground,	green_color,
		NULL);

	}

	status = open_product_file (current_file, Current_mode);
}

/************************************************************************
 *	Description: This function updates the product list in the	*
 *		     Products window.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
update_select_product_list (
)
{
	int	i;
	int	cnt;
static	int	first_time = 1;


	if (Product_scroll == (Widget) NULL) {

	    return;

	}

	if (Num_msg_ids != 0) {

	    XmListDeleteAllItems (Product_scroll);

	}

/*
	fprintf (stderr,"Query_data[0].field = RPGP_PCODE;\n");
	Query_data[0].field = RPGP_PCODE;
	fprintf (stderr,"Query_data[0].value = %d\n",
		ORPGPAT_get_code (Product_list [Current_product]));
	Query_data[0].value = ORPGPAT_get_code (Product_list [Current_product]);
	Query_data[1].field = RPGP_VOLT;
	Query_data[1].value = Time_list [Current_time];

	Num_msg_ids = ORPGDBM_query (Db_info,
			   Query_data,
			   2,
			   MAX_PRODUCTS_IN_LIST);
*/
	Num_msg_ids = XQ_query_prod_by_code_and_vol_time (
			ORPGPAT_get_code (Product_list [Current_product]),
			Time_list [Current_time],
			Db_info, MAX_PRODUCTS_IN_LIST);
	if (debug)
	    fprintf (stderr,"Num_msg_ids = %d\n", Num_msg_ids);
	if (Num_msg_ids < 0)
	    return;
	cnt = 0;

	while ((Num_msg_ids == 0) &&
	       (cnt < Num_products) &&
	       first_time) {

	    Current_product++;
/*
	    Query_data[0].field = RPGP_PCODE;
	    fprintf (stderr,"%d = ORPGPAT_get_code (Product_list [%d])\n",
		ORPGPAT_get_code (Product_list [Current_product]), Current_product);
	    Query_data[0].value = ORPGPAT_get_code (Product_list [Current_product]);
	    Query_data[1].field = RPGP_VOLT;
	    Query_data[1].value = Time_list [Current_time];

	    Num_msg_ids = ORPGDBM_query (Db_info,
			   Query_data,
			   2,
			   MAX_PRODUCTS_IN_LIST);
*/
	    Num_msg_ids = XQ_query_prod_by_code_and_vol_time (
			ORPGPAT_get_code (Product_list [Current_product]),
			Time_list [Current_time], 
			Db_info, MAX_PRODUCTS_IN_LIST);
	    if (debug)
		fprintf (stderr,"Num_msg_ids = %d\n", Num_msg_ids);
	    if (Num_msg_ids < 0)
		return;

	    if (Num_msg_ids == 0)
	        cnt++;

	}
	if (Num_msg_ids <= 0)
	    return;

	Current_item = cnt+1;

	sprintf (buf,"%3.3s[%2d] - %s",
		    ORPGPAT_get_mnemonic (Product_list [Current_product]),
			    ORPGPAT_get_code (Product_list [Current_product]),
			    ORPGPAT_get_description (Product_list [Current_product], STRIP_MNEMONIC));
	XmTextSetString (product_text, buf);

	first_time = 0;

	if (debug)
	    fprintf (stderr,"for (i=Num_msg_ids-1;i>=0;i--) {\n");
/*	for (i=Num_msg_ids-1;i>=0;i--) {	*/
	for (i=0;i<Num_msg_ids;i++) {

	    int		j;
	    int		month, day, year;
	    int		hour, minute, second;
	    int		indx;
	    long int	seconds;
	    char	buf1 [64];

	    seconds = Db_info [i].vol_t;
	    Msg_id_list [i] = Db_info [i].msg_id;

	    unix_time ( &seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

	    sprintf (buf,"%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
	    	    Month_string [month], day, year,
		    hour, minute, second);

	    if (debug)
		fprintf (stderr,"%s\n",buf);
	    for (j=0;j<ORPGPAT_get_num_parameters (Product_list [Current_product]);j++) {

		indx = ORPGPAT_get_parameter_index (Product_list [Current_product], j);
		sprintf (buf1,"   %5d", Db_info[i].params[indx]);
		strcat (buf, buf1);

	    }

	    str = XmStringCreateLocalized (buf);
	    XmListAddItemUnselected (Product_scroll, str, 0);
	    XmStringFree (str);

	}

	if (Current_item > 0)
	    XmListSelectPos (Product_scroll, Current_item, False);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a product from the products list.  The selected	*
 *		     product is opened and displayed.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - list data					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
select_product_from_list (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;
	XmListCallbackStruct	*cbs =
		(XmListCallbackStruct *) call_data;

	Current_item = cbs->item_position;

	status = open_product_file (current_file, Current_mode);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button from the Products window.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void
close_product_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtDestroyWidget (Product_dialog);
}

void print_usage()
{
           printf ("Usage: %s [options]\n", "xpdt");
           printf ("    Display ORPG products.\n");
           printf ("Options:\n");
           printf ("    -h: Print usage information\n");
           printf ("    -k: Display distances in kilometers\n");
           printf ("    -D dir: Specifies a product dir (Default: RPG DB)\n");
           printf ("    -T prod_table: Specifies product table file name\n");
           printf ("    -v: Verbose mode\n");
}


static int Read_options(int argc, char **argv)
{
  int c;

/* */
/*This subroutine checks all the options passed to the program */
/* */
  if(argc < 1)
  {
    print_usage();
    exit(-1);
  }

  opterr = 0;
  while ((c = getopt (argc, argv, "D:T:vkh?")) != EOF)
  {
    switch (c)
    {
        case 'k':   kilometer = 1;
           break;

        case 'D':
		strncpy (Prod_dir, optarg, 128);
		Prod_dir[127] = '\0';
           break;

        case 'v':
		debug = 1;
           break;

        case 'T':
		strncpy (Prod_table, optarg, 128);
		Prod_table[127] = '\0';
           break;

        default:
        case 'h':
        case '?':
	   print_usage();
           exit (0);

    }
  }
     return(0);
}
