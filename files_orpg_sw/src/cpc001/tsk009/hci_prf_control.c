 /*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2013/03/13 12:31:00 $
 * $Id: hci_prf_control.c,v 1.165 2013/03/13 12:31:00 ccalvert Exp $
 * $Revision: 1.165 $
 * $State: Exp $
 */

/************************************************************************
  Module: hci_prf_selection.c

  Description: This module contains a collection of routines
  used by the HCI to help the user make an intelligent decision
  about which PRF to use.
 ************************************************************************/

/* Include files */

#include <hci.h>
#include <hci_basedata.h>
#include <hci_prf_product.h>
#include <hci_rda_adaptation_data.h>
#include <hci_vcp_data.h>
#include <itc.h>

/* Defines and Enumerations */

enum
{
  REFLECTIVITY_INDEX = 0,
  PRF1,
  PRF2,
  PRF3,
  PRF4,
  PRF5,
  PRF6,
  PRF7,
  PRF8
};
enum
{
  S1_INDX = 0,
  S2_INDX,
  S3_INDX,
  NUM_SECTORS
};
enum
{
  SECTOR1 = 1,
  SECTOR2,
  SECTOR3
};
enum
{
  HCI_PRF_PRODUCT_GOOD,
  HCI_PRF_PRODUCT_TIMED_OUT,
  HCI_PRF_PRODUCT_ERROR,
};

/*
  Define sizes of drawing area, background product area and legend area.
  Currently, the window is not resizable, so sizes are constant. There
  are also variables that use these constants in case the window is
  made resizable in the future. In that case, sizes can be made relative
  to each other as opposed to explicitly defined.

  Currently, the drawing area is:
  X-direction: X_margin + background product width + legend width + X_margin
  Y-direction: Y_margin + background product height + Y_margin
*/
#define	DRAW_AREA_WIDTH			670
#define	DRAW_AREA_HEIGHT		630
#define	DRAW_AREA_X_MARGIN		5
#define	DRAW_AREA_Y_MARGIN		10
#define	BG_PRODUCT_WIDTH		610
#define	BG_PRODUCT_HEIGHT		610
#define	LEGEND_WIDTH			50
#define	DRAW_AREA_LABEL_MARGIN		5
#define	DATA_LEVEL_BOX_WIDTH		15
#define	DATA_LEVEL_BOX_HEIGHT		2*DATA_LEVEL_BOX_WIDTH
#define	DATA_LEVEL_MARGIN		5

#define	RANGE_FOLDED			16
#define	PRF_BINS_ALONG_RADIAL		230 /* # gates in prf product radial */
#define	PRF_BINS_ALONG_DIAMETER		PRF_BINS_ALONG_RADIAL * 2
#define	MAX_RADIALS_IN_CUT		400 /* maximum # radials in a cut */
#define	MAX_PRF_RANGE_NM		PRF_BINS_ALONG_RADIAL*HCI_KM_TO_NM
#define	HCI_PRF_DATA_TIMEOUT		900 /* 15 minutes */
#define	MAX_OBSC_VALUE			100
#define	MIN_OBSC_VALUE			0
#define	BOUNDARY_TOLERANCE		2.0
#define	SECTOR_LABEL_POSITION_RATIO	0.80 * PRF_BINS_ALONG_RADIAL
#define	SCROLL_HEIGHT			140
#define	MAX_NUM_STORMS			MAX_STORMS
#define	MAX_NUM_STORMS_TRACKED		MAX_TRACKED
#define	ID_LABEL_WIDTH			4
#define	AZIMUTH_LABEL_WIDTH		7
#define	RANGE_LABEL_WIDTH		6
#define	VIL_LABEL_WIDTH			9
#define	DBZM_LABEL_WIDTH		6
#define	DBZM_HEIGHT_LABEL_WIDTH		9
#define	STORM_TOP_LABEL_WIDTH		9
#define	NUM_PROD_PARAMS			6
#define	MAX_NUM_PRFS			PRFMAX
#define	DEGREES_IN_A_CIRCLE		360
#define	DEGREES_IN_A_QUADRANT		DEGREES_IN_A_CIRCLE / 4
#define	XLIB_DEGREE_RESOLUTION		64
#define	UPPER_ELEVATION_LIMIT		7.0
#define	PCT_OBSCURED_MISSING_FLAG	-1.0
#define	X_ORIGIN			0
#define	Y_ORIGIN			0
#define	AZIMUTH_WIDTH_RATIO		1.5

/* Structures */

typedef struct {
  Widget rowcol;
  Widget select_button;
  Widget id_label;
  Widget az_label;
  Widget range_label;
  Widget vil_label;
  Widget dbzm_label;
  Widget dbzm_height_label;
  Widget storm_top_label;
  char   id[ID_LABEL_WIDTH+1];
  char   azimuth_buf[AZIMUTH_LABEL_WIDTH+1];
  char   range_buf[RANGE_LABEL_WIDTH+1];
  char   vil_buf[VIL_LABEL_WIDTH+1];
  char   dbzm_buf[DBZM_LABEL_WIDTH+1];
  char   dbzm_height_buf[DBZM_HEIGHT_LABEL_WIDTH+1];
  char   storm_top_buf[STORM_TOP_LABEL_WIDTH+1];
  float  azimuth;
  float  range;
  float  vil;
  float  dbzm;
  float  dbzm_height;
  float  storm_top;
  int    selected_flag;
  int    tracked_flag;
} Storm_struct_t;

typedef struct {
  int current_mode;
  int selected_mode;
  int radius;
  int num_storms;
  Storm_struct_t storms[MAX_NUM_STORMS];
} PRF_Mode_status_t;

/* Static/global variables */

static Display     *PRF_display = (Display *) NULL;;
static Pixmap      PRF_pixmap = (Pixmap) NULL;
static GC          PRF_gc = (GC) NULL;
static Widget      Top_widget = (Widget) NULL;
static Widget      Top_form = (Widget) NULL;
static Widget      PRF_draw_widget= (Widget) NULL;
static Window      PRF_window = (Window) NULL;
static Widget      Close_button = (Widget) NULL;
static Widget      Apply_button = (Widget) NULL;
static Widget      Elevation_PRF_button = (Widget) NULL;
static Widget      Storm_PRF_button = (Widget) NULL;
static Widget      Manual_PRF_button = (Widget) NULL;
static Widget      Auto_refresh_button = (Widget) NULL;
static Widget      Show_labels_button = (Widget) NULL;
static Widget      Control_rowcol = (Widget) NULL;
static Widget      PRF_Mode_rowcol = (Widget) NULL;
static Widget      Description_rowcol = (Widget) NULL;
static Widget      Description_label_auto_elevation = (Widget) NULL;
static Widget      Description_label_auto_storm = (Widget) NULL;
static Widget      Description_label_manual = (Widget) NULL;
static Widget      Data_scroll = (Widget) NULL;
static Dimension   PRF_width = DRAW_AREA_WIDTH;
static Dimension   PRF_height = DRAW_AREA_HEIGHT;
static Dimension   PRF_depth = HCI_DEPTH_3D;
static Pixel       Product_bg = (Pixel) NULL;
static Pixel       Product_fg = (Pixel) NULL;
static Pixel       Base_bg = (Pixel) NULL;
static Pixel       Button_bg = (Pixel) NULL;
static Pixel       Button_fg = (Pixel) NULL;
static Pixel       Text_bg = (Pixel) NULL;
static Pixel       Text_fg = (Pixel) NULL;
static Pixel       RF_color = (Pixel) NULL;
static Pixel       White_color = (Pixel) NULL;
static Pixel       Green_color = (Pixel) NULL;
static Pixel       Red_color = (Pixel) NULL;
static Pixel       Warning_color = (Pixel) NULL;
static XmFontList  List_font = (XmFontList) NULL;
static XFontStruct *Fontinfo_list = (XFontStruct *) NULL;
static String      PRF_translations =
  "<PtrMoved>: Mouse_callback(move) \n\
  <Btn1Motion>: Mouse_callback(motion1) ManagerGadgetButtonMotion() \n\
  <Btn1Up>: Mouse_callback(up1) ManagerGadgetActivate() \n\
  <Btn2Up>: Mouse_callback(up2) ManagerGadgetActivate() \n\
  <Btn3Up>: Mouse_callback(up3) ManagerGadgetActivate() \n\
  <Btn1Down>: Mouse_callback(down1) ManagerGadgetArm() \n\
  <Btn2Down>: Mouse_callback(down2) ManagerGadgetArm() \n\
  <Btn3Down>: Mouse_callback(down3) ManagerGadgetArm()";

static int   Low_elevation_index = 1;
static int   High_elevation_index = 1;
static int   Product_area_x = 0;
static int   Product_area_y = 0;
static int   Product_area_width = 0;
static int   Product_area_height = 0;
static int   Product_area_ctr_x = 0;
static int   Product_area_ctr_y = 0;
static float Product_area_scale_x = 0.0;        /* pixel/km in X direction */
static float Product_area_scale_y = 0.0;        /* pixel/km in Y direction */
static int   Legend_area_x = 0;
static int   Legend_area_y = 0;
static int   Legend_area_width = 0;
static int   Legend_area_height = 0;
static int   Legend_area_ctr_x = 0;
static int   Legend_area_ctr_y = 0;
static int   Mouse_display_x = 0;
static int   Mouse_display_y = 0;
static int   Background_product_flag = HCI_PRF_PRODUCT_ERROR;
static int   Obscuration_product_flag = HCI_PRF_PRODUCT_ERROR;
static int   PRF_product_flag[MAX_NUM_PRFS+1]; /* PRF availability table */
static float Sector_azimuth[NUM_SECTORS] = { 0.0 };
static short Sector_prf[NUM_SECTORS] = { PRF1 };
static int   Sector_id[NUM_SECTORS] = { SECTOR1, SECTOR2, SECTOR3 };
static int   Change_pending_flag = HCI_NO_FLAG;
static int   Auto_refresh_flag = HCI_YES_FLAG;
static int   Show_labels_flag = HCI_YES_FLAG;
static int   Already_warned_flag = HCI_NO_FLAG;
static char  Buf[256];
static int   Font_height = 0;
static int   Background_product_id = 0;
static int   VCP_index = 0;
static int   VCP_number = 0;
static float VCP_low_elevation = 0.0;
static int   Write_prf_cmd_flag = HCI_NO_FLAG;
static int   PRF_Mode_status_update_flag = HCI_NO_FLAG;
/* Reflectivity color table */
static int Product_colors[PRODUCT_COLORS+1]; /* reflectivity color table */
/* Obscuration product data buffer */
static unsigned char Radial_data[MAX_NUM_PRFS+1][MAX_RADIALS_IN_CUT][PRF_BINS_ALONG_RADIAL];
static PRF_Mode_status_t PRF_Mode_status;
static Prf_command_t PRF_Mode_cmd_msg;
static RPG_prod_rec_t DB_info;

/* Local prototypes */

static void Expose_callback( Widget, XtPointer, XtPointer );
static void Resize_callback( Widget, XtPointer, XtPointer );
static void Mouse_callback( Widget, XEvent *, String *, int * );
static void Close_callback( Widget, XtPointer, XtPointer );
static void Close_accept( Widget, XtPointer, XtPointer );
static void Close_cancel( Widget, XtPointer, XtPointer );
static void Apply_callback( Widget, XtPointer, XtPointer );
static void Apply_accept( Widget, XtPointer, XtPointer );
static void Apply_cancel( Widget, XtPointer, XtPointer );
static void Auto_refresh_callback( Widget, XtPointer, XtPointer );
static void Show_labels_callback( Widget, XtPointer, XtPointer );
static void PRF_Mode_selected_callback( Widget, XtPointer, XtPointer );
static void PRF_storm_selected_callback( Widget, XtPointer, XtPointer );
static void Initialize_graphic_context();
static void Initialize_drawing_area();
static void Initialize_product_area();
static void Initialize_display_variables();
static void Initialize_RPG_variables();
static void Initialize_RDA_adaptation_data();
static void Initialize_base_data();
static void Initialize_background_product_ID();
static void Initialize_VCP_info();
static void Build_control_buttons_widget();
static void Build_PRF_Mode_control_widget();
static void Build_PRF_Mode_description_widget();
static void Build_PRF_draw_area_widget();
static void Build_storm_scroll_widget();
static void PRF_Mode_register_for_status_msg_updates();
static void PRF_Mode_status_msg_callback( int, LB_id_t, int, void * );
static void PRF_Mode_get_updated_status_msg();
static void PRF_Mode_update_storm_scroll_widgets();
static void Set_change_pending( int );
static void Write_prf_cmd();
static void PRF_Mode_set_widgets( int );
static int  Is_cursor_near_boundary( float, float, float );
static int  Get_sector_of_cursor_position( float, float, float );
static int  Get_sector_of_azimuth( float );
static void Clear_storm_struct_entry( int );
static void Timer_proc();
static void Check_for_updated_background_product();
static void Find_latest_background_product();
static void PRF_load_obscuration_product();
static void PRF_load_background_product();
static void PRF_display_background_product();
static void PRF_display_background_product_manual_mode();
static void PRF_display_background_product_auto_mode();
static void PRF_display_background_product_radial( int, float, float, int );
static void PRF_display_background_product_legend();
static void PRF_display_overlays();
static void PRF_display_overlays_range_ring();
static void PRF_display_overlays_manual_mode();
static void PRF_display_overlays_product_attributes();
static void PRF_display_labels();
static void PRF_display_labels_manual_mode();
static void PRF_display_labels_auto_storm_mode();
static void PRF_Mode_print_status_msg( Prf_status_t * );
static char *Convert_PRF_mode_state_to_string( int );
       void hci_display_product_radial_data( Display *, Drawable,
                GC, float, float, float, float, int, int, float,
                float, int, unsigned char *, int * );
       float hci_find_azimuth( int, int, int, int );
       int hci_vcp_io_status();

/* Following are used for debugging */

static int  Debug_flag = HCI_NO_FLAG;
static int  Fake_data_flag = HCI_NO_FLAG;
static void Initstorms( Prf_status_t *, int, int );
static void Print_debug( const char *format, ... );

/************************************************************************
  Description: This is the main function for the PRF Control task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_PRF_TASK );
  Top_widget = HCI_get_top_widget();
  Top_form = XtVaCreateWidget( "t_form", xmFormWidgetClass, Top_widget, NULL );

  /* If low bandwidth user, display progress meter for I/O */

  HCI_PM( "Initialize Task Information" );

  /* Register/read PRF Mode status message */

  PRF_Mode_register_for_status_msg_updates();

  /* Initialize X/Motif related variables */

  Initialize_display_variables();

  /* Initialize RPG related variables */

  Initialize_RPG_variables();

  /* Build widgets that comprise the application */

  Build_control_buttons_widget();
  Build_PRF_Mode_control_widget();
  Build_PRF_Mode_description_widget();
  Build_PRF_draw_area_widget();
  Build_storm_scroll_widget();

  /* Finish initializing and popup window */

  PRF_Mode_set_widgets( PRF_Mode_status.current_mode );
  Check_for_updated_background_product();
  XtManageChild( Top_form );
  XtRealizeWidget( Top_widget );
  Initialize_graphic_context();
  Resize_callback( NULL, NULL, NULL );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_TWO_SECONDS, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
  Description: This function is activated when the PRF Selection window
     is resized. It is also manually invoked when the contents of the
     window needs to be refreshed.
 ************************************************************************/

static void Resize_callback( Widget w, XtPointer y, XtPointer z )
{
  Print_debug( "Resize_callback" );

  /* If any of the global X variables are undefined do nothing */

  if( ( PRF_draw_widget == (Widget) NULL ) ||
      ( PRF_window      == (Window) NULL ) ||
      ( PRF_pixmap      == (Pixmap) NULL ) ||
      ( PRF_gc          == (GC)     NULL ) )
  {
    if( PRF_draw_widget == (Widget) NULL )
    {
      HCI_LE_error( "Resize - draw widget == NULL" );
    }
    if( PRF_window == (Window) NULL )
    {
      HCI_LE_error( "Resize - window == NULL" );
    }
    if( PRF_pixmap == (Pixmap) NULL )
    {
      HCI_LE_error( "Resize - Pixmap == NULL" );
    }
    if( PRF_gc == (GC) NULL )
    {
      HCI_LE_error( "Resize - GC == NULL" );
    }
    return;
  }


  Initialize_drawing_area();
  Initialize_product_area();
  PRF_display_background_product();
  PRF_display_background_product_legend();
  PRF_display_overlays();
  Expose_callback( NULL, NULL, NULL );
}

/************************************************************************
  Description: This function is activated when an expose event is
               generated for the PRF selection window.
 ************************************************************************/

 void Expose_callback( Widget w, XtPointer y, XtPointer z )
{
  Print_debug( "Expose_callback" );

  if( ( PRF_pixmap == (Pixmap) NULL ) ||
      ( PRF_window == (Window) NULL ) ||
      ( PRF_gc     == (GC)     NULL) )
  {
    if( PRF_window == (Window) NULL )
    {
      HCI_LE_error( "Resize - window == NULL" );
    }
    if( PRF_pixmap == (Pixmap) NULL )
    {
      HCI_LE_error( "Resize - Pixmap == NULL" );
    }
    if( PRF_gc == (GC) NULL )
    {
      HCI_LE_error( "Resize - GC == NULL" );
    }
    return;
  }

  XCopyArea( PRF_display, PRF_pixmap, PRF_window, PRF_gc,
             X_ORIGIN, Y_ORIGIN, PRF_width, PRF_height, X_ORIGIN, Y_ORIGIN );

  PRF_display_labels();
}

/************************************************************************
  Description: Initialize drawing area on window.
 ************************************************************************/

static void Initialize_drawing_area()
{
  Print_debug( "Initialize_drawing_area" );

  /* Fill drawing area with the background color. */

  XSetForeground( PRF_display, PRF_gc, Base_bg );
  XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                  X_ORIGIN, Y_ORIGIN, PRF_width, PRF_height );
  XSetForeground( PRF_display, PRF_gc, Product_fg );
}

/************************************************************************
  Description: Initialize product drawing area on window.
 ************************************************************************/

static void Initialize_product_area()
{
  Print_debug( "Initialize_product_area" );

  /* Initialize the background product drawing area by filling it black. */

  XSetForeground( PRF_display, PRF_gc, Product_bg );
  XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                  Product_area_x, Product_area_y,
                  Product_area_width, Product_area_height );
  XSetForeground( PRF_display, PRF_gc, Product_fg );
}

/************************************************************************
  Description: This function is used to display the product generated
               by the Manual PRF Mode.
 ************************************************************************/

static void PRF_display_background_product_manual_mode()
{
  int i = 0;
  int j = 0;
  int indexA = 0;
  int indexB = 0;
  float azi1 = 0.0;
  float azi2 = 0.0;
  int prf = 0;
  float *azimuths = NULL;
  float *azimuth_widths = NULL;

  Print_debug( "PRF_display_background_product_manual_mode" );

  for( j = S1_INDX; j < NUM_SECTORS; j++ )
  {
    indexA = j;
    indexB = (indexA+1)%NUM_SECTORS;
    azi1 = Sector_azimuth[indexA];
    azi2 = Sector_azimuth[indexB];
    prf = Sector_prf[indexA];

    /* An assumption is made here that if the number of data elements
       are defined, then everything else should be defined. */

    azimuths = (float *) hci_product_azimuth_data();
    azimuth_widths = (float *) hci_product_azimuth_width();

    /* Display all radials which fall inside the specified sector. Do this
       whether or not data are available so that we clear out any
       previously displayed data. */

    for( i = 0; i < hci_product_radials(); i++ )
    {
      if( azi1 < azi2 )
      {
        /* Sector does not cross 0/360 degrees. */
        if( ( azimuths[i] >= azi1 ) && ( azimuths[i] <= azi2 ) )
        {
          PRF_display_background_product_radial( prf, azimuths[i], azimuth_widths[i], i );
        }
      }
      else if( azi1 > azi2 )
      {
        /* Sector does cross 0/360 degrees */
        if( ( azimuths[i] >= azi1 ) || ( azimuths[i] <= azi2 ) )
        {
          PRF_display_background_product_radial( prf, azimuths[i], azimuth_widths[i], i );
        }
      }
    }
  }
}

/************************************************************************
  Description: This function is used to display the product generated
               by a non-Manual PRF Mode.
 ************************************************************************/

static void PRF_display_background_product_auto_mode()
{
  int i = 0;
  float *azimuths = (float *) hci_product_azimuth_data();
  float *azimuth_widths = (float *) hci_product_azimuth_width();
  /* Index 1 is lowest elevation index. Use the PRF from
     the first doppler sector, since in AUTO PRF all
     sectors' PRF should be the same. */
  int prf = hci_current_vcp_get_sector_prf_num( 1, 1 );

  Print_debug( "PRF_display_background_product_auto_mode" );

  /* Display all radials. Do this whether data is available or
     not so previously drawn data is overwritten. */

  for( i = 0; i < hci_product_radials(); i++ )
  {
    PRF_display_background_product_radial( prf, azimuths[i], azimuth_widths[i], i );
  }
}

/************************************************************************
  Description: Draw a radial of product data.
 ************************************************************************/

static void PRF_display_background_product_radial( int prf, float azimuth, float azimuth_width, int radial )
{
  int j = 0;
  unsigned char radial_data[PRF_BINS_ALONG_RADIAL];

  for( j = 0; j < PRF_BINS_ALONG_RADIAL; j++ )
  {
    if( Radial_data[prf][radial][j] )
    {
      radial_data[j] = RANGE_FOLDED;
    }
    else
    {
      radial_data[j] = Radial_data[REFLECTIVITY_INDEX][radial][j];
    }
  }

  hci_display_product_radial_data( PRF_display,
             PRF_pixmap, PRF_gc, (float) X_ORIGIN, (float) Y_ORIGIN,
             Product_area_scale_x, Product_area_scale_y,
             Product_area_ctr_x, Product_area_ctr_y,
             azimuth, AZIMUTH_WIDTH_RATIO*azimuth_width, PRF_BINS_ALONG_RADIAL,
             (unsigned char *) radial_data, Product_colors );
}

/************************************************************************
  Description: This function handles mouse events inside the data
               display region.
 ************************************************************************/

static void Mouse_callback( Widget w, XEvent *evt, String *args, int *n_args )
{
  float azimuth = 0.0;
  float range = 0.0;
  int cursor_near_boundary_flag = 0;
  int x = 0;
  int y = 0;
  int i = 0;
  int prf_sector_index = 0;
  int indx = 0;
  float temp = 0.0;
  float diff1 = 0.0;
  float diff2 = 0.0;
  float diff3 = 0.0;
  short pulse_cnt;
  static int button_down = HCI_NO_FLAG;
  static int cursor_sector_index = -1;
  static int display_mouse_pos = HCI_NO_FLAG;
  static int mouse_display_width = -1;
  static int mouse_display_height = -1;
  static int previous_cursor_near_boundary_flag = HCI_NO_FLAG;

  /* Find the coordinates of the cursor where the button is at.
     Determine the azimuth so we can determine if it is close
     is close enough to a sector boundary. */

  x = evt->xbutton.x;
  y = evt->xbutton.y;

  /* If mouse is outside drawing area, do nothing. */

  if( ( y < Product_area_y ) ||
      ( y > ( Product_area_y + Product_area_height ) ) ||
      ( x < Product_area_x ) ||
      ( x > ( Product_area_x + Product_area_width ) ) )
  {
    if( display_mouse_pos )
    {
      display_mouse_pos = HCI_NO_FLAG;

      XSetForeground( PRF_display, PRF_gc, Product_bg );

      XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                      Mouse_display_x,
                      Mouse_display_y - mouse_display_height,
                      mouse_display_width,
                      mouse_display_height );

      XSetForeground( PRF_display, PRF_gc, Product_fg );

      Expose_callback( NULL, NULL, NULL );
    }
    return;
  }

  /* Mouse is inside drawing area. */

  display_mouse_pos = HCI_YES_FLAG;

  azimuth = hci_find_azimuth( x, y, Product_area_ctr_x, Product_area_ctr_y );

  range = sqrt( (double)( (x-Product_area_ctr_x)*(x-Product_area_ctr_x) +
                (y-Product_area_ctr_y) * (y-Product_area_ctr_y)) / 
                (Product_area_scale_x * Product_area_scale_x) ) * HCI_KM_TO_NM;

  /* Display the azimuth position of the cursor in the
     upper left corner of the display window */

  sprintf( Buf,"(%3d Deg,%3d nm)", (int) azimuth, (int) range );

  mouse_display_width  = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
  mouse_display_height  = Font_height;

  XDrawImageString( PRF_display, PRF_window, PRF_gc,
                    Mouse_display_x, Mouse_display_y, Buf, strlen( Buf ) );

  /* Proceed only if in PRF Mode manual */

  if( PRF_Mode_status.selected_mode != HCI_PRF_MODE_MANUAL ){ return; }

  /* Calculate azimuth difference between sector boundaries
     and mouse event */
  diff1 = fabs( (double) (Sector_azimuth[S1_INDX] - azimuth) );
  diff2 = fabs( (double) (Sector_azimuth[S2_INDX] - azimuth) );
  diff3 = fabs( (double) (Sector_azimuth[S3_INDX] - azimuth) );

  /* Determine if mouse event was close to a sector boundary */
  if( range > MAX_PRF_RANGE_NM )
  {
    cursor_near_boundary_flag = HCI_NO_FLAG;
  }
  else
  {
    cursor_near_boundary_flag = Is_cursor_near_boundary( diff1, diff2, diff3 );
  }

  /* Set cursor appearance depending location of mouse event */
  if( cursor_near_boundary_flag )
  {
    /* Cursor is close to a boundary */
    if( !previous_cursor_near_boundary_flag )
    {
      /* Cursor previously was not close to a boundary */
      previous_cursor_near_boundary_flag = cursor_near_boundary_flag;
      HCI_selectable_cursor();
    }
  }
  else
  {
    /* Cursor is not close to a boundary */
    if( previous_cursor_near_boundary_flag && (!button_down) )
    {
      /* Cursor previously was close to a boundary */
      previous_cursor_near_boundary_flag = cursor_near_boundary_flag;
      HCI_default_cursor();
    }
    else
    {
      cursor_near_boundary_flag = previous_cursor_near_boundary_flag;
    }
  }

  /* Handle specific mouse events */
  if( !strcmp( args[0], "down1" ) && range <= MAX_PRF_RANGE_NM )
  {
    HCI_LE_log( "left button pushed" );

    /* Find the sector boundary that is closest to the selected
       azimuth, change it and update display. */

    button_down = HCI_YES_FLAG;

    if( cursor_near_boundary_flag )
    {
      cursor_sector_index = Get_sector_of_cursor_position( diff1, diff2, diff3 );
      Sector_azimuth[cursor_sector_index] = azimuth;
    }
    else
    {
      /* If the cursor is not near a boundary, increment the PRF
         number. If past the max allowed, change to the first. */

      prf_sector_index = Get_sector_of_azimuth( azimuth );
      Set_change_pending( HCI_YES_FLAG );

      Sector_prf[prf_sector_index]++;
      if( Sector_prf[prf_sector_index] > hci_rda_adapt_allowable_prf_num(
                VCP_index, hci_rda_adapt_allowable_prf_prfs( VCP_index ) - 1 ) )
      {
        Sector_prf[prf_sector_index] = hci_rda_adapt_allowable_prf_num( VCP_index, 0 );
      }
      for( i = Low_elevation_index; i <= High_elevation_index; i++ )
      {
        if( (hci_current_vcp_wave_type(i) != WAVEFORM_CONTIGUOUS_SURVEILLANCE) 
                                     &&
            (hci_current_vcp_wave_type(i) != WAVEFORM_STAGGERED_PULSE_PAIR) )
        {
          indx = ORPGVST_get_index(i) - 1;

          pulse_cnt = hci_rda_adapt_allowable_prf_pulse_count( VCP_index, indx, Sector_prf[prf_sector_index] );

          if( pulse_cnt > 0 )
          {
            hci_current_vcp_set_sector_prf_num( Sector_id[prf_sector_index], i, Sector_prf[prf_sector_index] );
            hci_current_vcp_set_sector_pulse_cnt( Sector_id[prf_sector_index], i, pulse_cnt );
            HCI_LE_log( "Changing sector 1 cut %d to prf# %d - pulse count: %d",
            i, Sector_prf[S1_INDX], hci_current_vcp_get_sector_pulse_cnt( Sector_id[prf_sector_index], i ) );
          }
        }
      }
      Resize_callback( NULL, NULL, NULL );
    }
  }
  else if( !strcmp( args[0], "down3" ) && range <= MAX_PRF_RANGE_NM )
  {
    HCI_LE_log( "right button pushed" );

    if( !cursor_near_boundary_flag )
    {
      /* If the cursor is not near a boundary, increment the PRF
         number. If past the max allowed, change to the first. */

      Set_change_pending( HCI_YES_FLAG );

      prf_sector_index = Get_sector_of_azimuth( azimuth );

      Sector_prf[prf_sector_index]--;
      if( Sector_prf[prf_sector_index] < hci_rda_adapt_allowable_prf_num( VCP_index, 0 ) )
      {
        Sector_prf[prf_sector_index] = hci_rda_adapt_allowable_prf_num(
          VCP_index, hci_rda_adapt_allowable_prf_prfs( VCP_index ) - 1 );
      }
      for( i = Low_elevation_index; i <= High_elevation_index; i++ )
      {
        if( (hci_current_vcp_wave_type(i) != WAVEFORM_CONTIGUOUS_SURVEILLANCE) 
                                                       &&
            (hci_current_vcp_wave_type(i) != WAVEFORM_STAGGERED_PULSE_PAIR) )
        {
          indx = ORPGVST_get_index(i)-1;

          pulse_cnt = hci_rda_adapt_allowable_prf_pulse_count( VCP_index, indx, Sector_prf[prf_sector_index] );

          if( pulse_cnt > 0 )
          {
            hci_current_vcp_set_sector_prf_num( Sector_id[prf_sector_index], i, Sector_prf[S1_INDX] );
            hci_current_vcp_set_sector_pulse_cnt( Sector_id[prf_sector_index], i, pulse_cnt );
            HCI_LE_log( "Changing sector 1 cut %d to prf# %d - pulse count: %d",
            i, Sector_prf[S1_INDX], hci_current_vcp_get_sector_pulse_cnt( Sector_id[prf_sector_index], i ) );
          }
        }
      }

      Resize_callback( NULL, NULL, NULL );
    }
  }
  else if( ( ( !strcmp( args[0], "up1" ) ) &&
           ( range <= MAX_PRF_RANGE_NM ) ) ||
           ( ( button_down == HCI_YES_FLAG ) &&
           ( range > MAX_PRF_RANGE_NM ) ) )
  {
    if( button_down && cursor_near_boundary_flag )
    {
      Set_change_pending( HCI_YES_FLAG );

      /* Update the sector azimuth properties of the elevation(s)
         in the currently selected vcp structure. */

      for( i = Low_elevation_index; i <= High_elevation_index; i++ )
      {
        if( (hci_current_vcp_wave_type(i) != WAVEFORM_CONTIGUOUS_SURVEILLANCE) 
                                                   &&
            (hci_current_vcp_wave_type(i) != WAVEFORM_STAGGERED_PULSE_PAIR) )
        {
          hci_current_vcp_set_sector_azimuth( Sector_id[S1_INDX], i, Sector_azimuth[S1_INDX] );
          hci_current_vcp_set_sector_azimuth( Sector_id[S2_INDX], i, Sector_azimuth[S2_INDX] );
          hci_current_vcp_set_sector_azimuth( Sector_id[S3_INDX], i, Sector_azimuth[S3_INDX] );
        }
      }
    }

    button_down = HCI_NO_FLAG;

    /* If the cursor position is within tolerance, the left button
       is pressed, and the mouse dragged, move the appropriate sector
       boundary, keeping in mind the clockwise ordering of the sectors. */

  }
  else if( ( ( !strcmp( args[0], "move" ) ) &&
             ( button_down )               &&
             ( cursor_near_boundary_flag ) ) &&
             ( range <= MAX_PRF_RANGE_NM ) )
  {
    Sector_azimuth[cursor_sector_index] = azimuth;

    /* We need to make a final check to make sure the azimuths
     for the sectors are ordered in a clockwise fashion. If not,
     then reorder appropriately. */

    if( Sector_azimuth[S1_INDX] > Sector_azimuth[S2_INDX] )
    {
      /* S1 > S2 */
      if( Sector_azimuth[S3_INDX] > Sector_azimuth[S1_INDX] )
      {
        /* S3 > S1 */
        /* S1 > S2, S3 > S1 so S3 > S1 > S2 */
        /* Need to reorder to be clockwise (R to L)*/ 
        if( cursor_sector_index == S1_INDX )
        {
          /*S1 > S2, S3 > S1 index S1 - switch S1 and S3 */
          /* Make it S1 > S3 > S2 */
          temp = Sector_azimuth[S3_INDX];
          Sector_azimuth[S1_INDX] = Sector_azimuth[S3_INDX];
          Sector_azimuth[S3_INDX] = temp;
          cursor_sector_index = S3_INDX;
        }
        else if( cursor_sector_index == S2_INDX )
        {
          /* S1 > S2, S3 > S1 index S2 - switch S1 and S2 */
          /* Make it S3 > S2 > S1 */
          temp = Sector_azimuth[S1_INDX];
          Sector_azimuth[S1_INDX] = Sector_azimuth[S2_INDX];
          Sector_azimuth[S2_INDX] = temp;
          cursor_sector_index = S1_INDX;
        }
        else if( cursor_sector_index == S3_INDX )
        {
          /* S1 > S2, S3 > S1index S3 - switch S1 and S3 */
          /* Make it S1 > S3 > S2 */
          temp = Sector_azimuth[S3_INDX];
          Sector_azimuth[S1_INDX] = Sector_azimuth[S3_INDX];
          Sector_azimuth[S3_INDX] = temp;
          cursor_sector_index = S1_INDX;
        }
      }
      else
      {
        /* S3 < S1 */
        /* S1 > S2, S1 > S3 - Inconclusive, need info of S2 and S3 */
        if( Sector_azimuth[S3_INDX] < Sector_azimuth[S2_INDX] )
        {
          /* S3 < S2 */
          /* S1 > S2, S1 > S3, S3 < S2 so S1 > S2 > S3 */
          /* Need to reorder to be clockwise (R to L ) */
          if( cursor_sector_index == S1_INDX )
          {
            /* S1 > S2, S3 < S1, S3 < S2 index S1 - switch S1 and S2 */
            /* Make it S2 > S1 > S3 */
            temp = Sector_azimuth[S1_INDX];
            Sector_azimuth[S1_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S2_INDX;
          }
          else if( cursor_sector_index == S2_INDX )
          {
            /*S1 > S2, S3 < S1, S3 < S2 index S2 - switch S3 and S2 */
            /* Make it S1 > S3 > S2 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S3_INDX;
          }
          else if( cursor_sector_index == S3_INDX )
          {
            /* S1 > S2, S3 < S1, S3 < S2 index S3 - switch S3 and S2 */
            /* Make it S1 > S3 > S2 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S2_INDX;
          }
        }
        else
        {
          /* S1 > S2, S3 < S1, S3 > S2 */
        }
      }
    }
    else
    {
      /* S1 < S2 */
      if( Sector_azimuth[S2_INDX] > Sector_azimuth[S3_INDX] )
      {
        /* S2 > S3 */
        /* S1 < S2, S2 > S3 - Inconclusive, need info of S1 and S3 */
        if( Sector_azimuth[S3_INDX] > Sector_azimuth[S1_INDX] )
        {
          /* S3 > S1 */
          /* S1 < S2, S2 > S3, S3 > S1 so S2 > S3 > S1 */
          /* Need to reorder to be clockwise (R to L ) */
          if( cursor_sector_index == S1_INDX )
          {
            /* S1 < S2, S2 > S3, S3 > S1 index S1 - switch S3 and S1 */
            /* Make it S2 > S1 > S3 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S1_INDX];
            Sector_azimuth[S1_INDX] = temp;
            cursor_sector_index = S3_INDX;
          }
          else if( cursor_sector_index == S2_INDX )
          {
            /* S1 < S2, S2 > S3, S3 > S1 index S2 - switch S3 and S2 */
            /* Make it S3 > S2 > S1 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S3_INDX;
          }
          else if( cursor_sector_index == S3_INDX )
          {
            /* S1 < S2, S2 > S3, S3 > S1 index S3 - switch S3 and S2 */
            /* Make it S3 > S2 > S1 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S2_INDX;
          }
        }
        else
        {
          /* S1 < S2, S2 > S3, S3 < S1 */
        }
      }
      else
      {
        if( Sector_azimuth[S3_INDX] < Sector_azimuth[S1_INDX] )
        {
          if( cursor_sector_index == S1_INDX )
          {
            /* S1 < S2, S2 < S3, S3 < S1 index S1 - switch S3 and S1 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S1_INDX];
            Sector_azimuth[S1_INDX] = temp;
            cursor_sector_index = S3_INDX;
          }
          else if( cursor_sector_index == S2_INDX )
          {
            /* S1 < S2, S2 < S3, S3 < S1 index S2 - switch S3 and S2 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S3_INDX;
          }
          else if( cursor_sector_index == S3_INDX )
          {
            /* S1 < S2, S2 < S3, S3 < S1 index S3 - switch S3 and S2 */
            temp = Sector_azimuth[S3_INDX];
            Sector_azimuth[S3_INDX] = Sector_azimuth[S2_INDX];
            Sector_azimuth[S2_INDX] = temp;
            cursor_sector_index = S2_INDX;
          }
        }
        else
        {
          /* S1 < S2, S2 < S3, S3 > S1 */
        }
      }
    }

    /* Resize instead of expose so boundary is erased and redrawn
       while being moved. */

    Resize_callback( NULL, NULL, NULL );
  }
}

/************************************************************************
  Description: This function is activated when the user selects the 
               "Close" button.
 ************************************************************************/

static void Close_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Close button pushed" );

  Already_warned_flag = HCI_NO_FLAG;

  if( Change_pending_flag == HCI_YES_FLAG )
  {
    sprintf( Buf, "Changes have not been applied.\nDo you want to continue?" );
    hci_confirm_popup( Top_widget, Buf, Close_accept, Close_cancel );
  }
  else
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}

/************************************************************************
  Description: This function is activated when the user selects the 
               "Yes" button in the close confirmation popup.
 ************************************************************************/

static void Close_accept( Widget w, XtPointer y, XtPointer z )
{
  Set_change_pending( HCI_NO_FLAG );
  Close_callback( NULL, NULL, NULL );
}

/************************************************************************
  Description: This function is activated when the user selects the 
               "No" button in the close confirmation popup.
 ************************************************************************/

static void Close_cancel( Widget w, XtPointer y, XtPointer z )
{
  /* Do nothing */
}

/************************************************************************
  Description: Display overlays over background product.
 ************************************************************************/

static void PRF_display_overlays()
{
  Print_debug( "PRF_display_overlays" );

  /* Display 230 km range ring */

  PRF_display_overlays_range_ring();

  /* Display mode-specific overlays */

  if( PRF_Mode_status.selected_mode == HCI_PRF_MODE_MANUAL )
  {
    PRF_display_overlays_manual_mode();
  }

  /* Display any product attribute overlays */

  PRF_display_overlays_product_attributes();
}

/************************************************************************
  Description: Display circle associated with 230 km range ring.
 ************************************************************************/

static void PRF_display_overlays_range_ring()
{
  Print_debug( "PRF_display_overlays_range_ring" );

  /* Subtract 1 to prevent graphic context from clipping range ring */
  XDrawArc( PRF_display, PRF_pixmap, PRF_gc,
            (int) ( Product_area_ctr_x - (PRF_BINS_ALONG_RADIAL-1) * Product_area_scale_x ),
            (int) ( Product_area_ctr_y + (PRF_BINS_ALONG_RADIAL-1) * Product_area_scale_y ),
            (int) ( (PRF_BINS_ALONG_DIAMETER-1) * Product_area_scale_x ),
            (int) ( (PRF_BINS_ALONG_DIAMETER-1) * Product_area_scale_x ),
            0, DEGREES_IN_A_CIRCLE*XLIB_DEGREE_RESOLUTION );
}

/************************************************************************
  Description: This function displays lines representing the sector
               boundaries for a given elevation cut. If enabled, labels
               are also displayed with the left edge of the label at the
               middle azimuth in the sector.
 ************************************************************************/

static void PRF_display_overlays_manual_mode()
{
  int x = 0;
  int y = 0;
  int i = 0;
  float rad_angle = 0.0;
  float ratio = 0.0;
  int indexA = 0;
  int indexB = 0;

  Print_debug( "PRF_display_overlays_manual_mode" );

  for( i = S1_INDX; i < NUM_SECTORS; i++ )
  {
    indexA = i;
    indexB = (indexA+1)%NUM_SECTORS;

    rad_angle = (Sector_azimuth[indexB]-DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD;
    ratio = cos( rad_angle );
    x = PRF_BINS_ALONG_RADIAL * ratio * Product_area_scale_x + Product_area_ctr_x;

    rad_angle = (Sector_azimuth[indexB]+DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD;
    ratio = sin( rad_angle );
    y = PRF_BINS_ALONG_RADIAL * ratio * Product_area_scale_y + Product_area_ctr_y;
    
    XDrawLine( PRF_display, PRF_pixmap, PRF_gc,
               Product_area_ctr_x, Product_area_ctr_y, x, y );
  }
}

/************************************************************************
  Description: Display any product attributes with the background product.
 ************************************************************************/

static void PRF_display_overlays_product_attributes()
{
  int ret_val = 0;
  int data_id = 0;
  char *buf = NULL;
  Pct_obs *obs_data = NULL;
  int pct_value = 0;
  int y = 0;
  int x = 0;
  int width = 0;
  static int pct_obs_width = 0;
  static int elev_attr_width = 0;
  static int date_attr_width = 0;
  static int time_attr_width = 0;
  static int vcp_attr_width = 0;
  int length = 0;
  int right_x = Product_area_x + Product_area_width - DRAW_AREA_LABEL_MARGIN;
  int left_x = Product_area_x + DRAW_AREA_LABEL_MARGIN;
  int upper_y = Product_area_y + DRAW_AREA_LABEL_MARGIN;
  int lower_y = Product_area_y + Product_area_height - DRAW_AREA_LABEL_MARGIN;

  Print_debug( "PRF_display_overlays_product_attributes" );

  /* Draw MPDA Pct Obscured string in lower left corner of product area */

  y = lower_y - Fontinfo_list->descent;
  x = left_x;

  if( Background_product_flag == HCI_PRF_PRODUCT_GOOD &&
      hci_product_vcp() >= VCP_MIN_MPDA &&
      hci_product_vcp() <= VCP_MAX_MPDA )
  {
    data_id = ( PCT_OBS / ITC_IDRANGE) * ITC_IDRANGE;

    ret_val = ORPGDA_read( data_id, &buf, LB_ALLOC_BUF, LBID_PCT_OBS );
    if( ret_val < 0 || ret_val != sizeof( Pct_obs ) )
    {
      HCI_LE_error( "ITC PCT_OBS Read Failed: (%d)", ret_val );
      pct_value = PCT_OBSCURED_MISSING_FLAG;
    }
    else
    {
      obs_data = (Pct_obs *) buf;
      if( hci_product_date() != obs_data->vol_date )
      {
        HCI_LE_error( "OBS data doesn't match product date" );
        pct_value = PCT_OBSCURED_MISSING_FLAG;
      }
      else if( hci_product_time() == obs_data->vol_time )
      {
        HCI_LE_error( "OBS data doesn't match product time" );
        pct_value = PCT_OBSCURED_MISSING_FLAG;
      }
      else if( obs_data->percent_obscured > MAX_OBSC_VALUE )
      {
        HCI_LE_status( "OBS max clip %f", obs_data->percent_obscured );
        pct_value = MAX_OBSC_VALUE;
      }
      else if( obs_data->percent_obscured < MIN_OBSC_VALUE )
      {
        HCI_LE_status( "OBS min clip %f", obs_data->percent_obscured );
        pct_value = MIN_OBSC_VALUE;
      }
      else
      {
        pct_value = (int) obs_data->percent_obscured;
      }
    }

    if( pct_value == PCT_OBSCURED_MISSING_FLAG )
    {
      sprintf( Buf, "MPDA %% Obscured: N/A" );
    }
    else
    {
      sprintf( Buf, "MPDA %% Obscured: %3d", pct_value );
    }
    length = strlen( Buf );
    pct_obs_width  = XTextWidth( Fontinfo_list, Buf, length );
    x = left_x;
    XSetForeground( PRF_display, PRF_gc, Product_fg );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      x, y, Buf, length );

    if( buf != NULL ){ free( buf ); }
  }
  else
  {
    XSetForeground( PRF_display, PRF_gc, Product_bg );
    XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                    x, y - Font_height, pct_obs_width, Font_height );
  }

  /* Draw cut elevation in upper-right corner of product area */

  y = upper_y + Fontinfo_list->ascent;

  if( Background_product_flag == HCI_PRF_PRODUCT_GOOD )
  {
    sprintf( Buf,"Elev %s Deg", hci_product_attribute_text( PROD_CUT_ANGLE_INDEX ) );
    length = strlen( Buf );
    elev_attr_width  = XTextWidth( Fontinfo_list, Buf, length );
    x = right_x - elev_attr_width;
    XSetForeground( PRF_display, PRF_gc, Product_fg );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      x, y, Buf, length );
  }
  else
  {
    XSetForeground( PRF_display, PRF_gc, Product_bg );
    XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                    x, y - Font_height, elev_attr_width, Font_height );
  }

  /* Draw date in lower-right corner of product area */

  y = lower_y - ( Fontinfo_list->descent + 2*Font_height );

  if( Background_product_flag == HCI_PRF_PRODUCT_GOOD )
  {
    sprintf( Buf,"%s", hci_product_attribute_text( PROD_DATE_INDEX ) );
    length = strlen( Buf );
    date_attr_width  = XTextWidth( Fontinfo_list, Buf, length );
    x = right_x - date_attr_width;
    XSetForeground( PRF_display, PRF_gc, Product_fg );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      x, y, Buf, length );
  }
  else
  {
    XSetForeground( PRF_display, PRF_gc, Product_bg );
    XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                    x, y - Font_height, date_attr_width, Font_height );
  }

  /* Draw time in lower-right corner of product area */

  y = lower_y - ( Fontinfo_list->descent + Font_height );

  if( Background_product_flag == HCI_PRF_PRODUCT_GOOD )
  {
    sprintf( Buf,"%s UTC", hci_product_attribute_text( PROD_TIME_INDEX ) );
    length = strlen( Buf );
    time_attr_width  = XTextWidth( Fontinfo_list, Buf, length );
    x = right_x - time_attr_width;
    XSetForeground( PRF_display, PRF_gc, Product_fg );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      x, y, Buf, length );
  }
  else
  {
    XSetForeground( PRF_display, PRF_gc, Product_bg );
    XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                    x, y - Font_height, time_attr_width, Font_height );
  }

  /* Draw VCP in lower-right corner of product area */

  y = lower_y - Fontinfo_list->descent;

  if( Background_product_flag == HCI_PRF_PRODUCT_GOOD )
  {
    sprintf( Buf,"VCP %s", hci_product_attribute_text( PROD_VCP_INDEX ) );
    length = strlen( Buf );
    vcp_attr_width  = XTextWidth( Fontinfo_list, Buf, length );
    x = right_x - vcp_attr_width;
    XSetForeground( PRF_display, PRF_gc, Product_fg );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      x, y, Buf, length );
  }
  else
  {
    XSetForeground( PRF_display, PRF_gc, Product_bg );
    XFillRectangle( PRF_display, PRF_pixmap, PRF_gc,
                    x, y - Font_height, vcp_attr_width, Font_height );
  }

  XSetForeground( PRF_display, PRF_gc, Product_fg );

  /* Draw any product-related messages in the center of the
     background product area */

  if( Background_product_flag == HCI_PRF_PRODUCT_ERROR )
  {
    strcpy( Buf, "Data Not Available" );
    width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      Product_area_ctr_x - width/2, Product_area_ctr_y,
                      Buf, strlen( Buf ) );
  }
  else if( Obscuration_product_flag == HCI_PRF_PRODUCT_ERROR )
  {
    strcpy( Buf, "Obscuration Data Not Available" );
    width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      Product_area_ctr_x - width/2, Product_area_ctr_y,
                      Buf, strlen( Buf ) );
  }
  else if( Background_product_flag == HCI_PRF_PRODUCT_TIMED_OUT )
  {
    strcpy( Buf, "Reflectivity Product Data Expired" );
    width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
    XDrawImageString( PRF_display, PRF_pixmap, PRF_gc,
                      Product_area_ctr_x - width/2, Product_area_ctr_y,
                      Buf, strlen( Buf ) );
  }
}

/************************************************************************
  Description: This function reads the base reflectivity product.
 ************************************************************************/

static void PRF_load_background_product()
{
  int i = 0;
  int indx = 0;
  int step = 0;
  int status = 0;
  int num_radials = 0;
  int num_gates = 0;
  int radial = 0;
  int gate = 0;
  int julian_seconds = 0;
  int low_elevation = (int) ( VCP_low_elevation * 10 );
  float interval = 0.0;
  unsigned short **data;
  static int init_colors = HCI_NO_FLAG;

  /* Set param3 to get lowest elevation */

  short product_params[NUM_PROD_PARAMS] = { 0, 0, low_elevation, 0, 0, 0 };

  Print_debug( "PRF_load_background_product" );

  /* Check to see if the time of the latest product is more than
     HCI_PRF_DATA_TIMEOUT old. If so, we do not want to display any data. */

  julian_seconds = ( ORPGVST_get_volume_date() - 1 ) *
                   HCI_SECONDS_PER_DAY + ORPGVST_get_volume_time()/1000;

  HCI_LE_log( "current-product time (s): %d", julian_seconds - DB_info.vol_t );

  if( ( julian_seconds - DB_info.vol_t ) > HCI_PRF_DATA_TIMEOUT )
  {
    Background_product_flag = HCI_PRF_PRODUCT_TIMED_OUT;
    return;
  }

  status = hci_load_product_data( Background_product_id,
                                  DB_info.vol_t, product_params );

  /* Get data from the latest time available for the product in the
     data base. */

  if( status <= 0 ) 
  {
    if( hci_load_product_io_status() == RMT_CANCELLED )
    {
      HCI_LE_error( "User cancelled loading background product" );
      HCI_task_exit( HCI_EXIT_SUCCESS );
    }
    HCI_LE_error( "Error openning reflectivity product...RMT cancelled" );
    Background_product_flag = HCI_PRF_PRODUCT_ERROR;
    return;
  }

  /* Define product color scale. It only has to be done once. */

  if( !init_colors )
  {
    if( hci_product_code() > 0 )
    {
      init_colors = HCI_YES_FLAG;
    }
    hci_get_product_colors( hci_product_code(), Product_colors );
    Product_colors[PRODUCT_COLORS] = RF_color;
  }

  /* Get the product radial data from the product and 
     write it into the 0th element of working storage */

  data = hci_product_radial_data();
  num_radials  = hci_product_radials();
  num_gates = hci_product_data_elements();
  interval = hci_product_resolution();
  step = (int) ( interval + 0.1 );

  /* The PRF product resolution is 1 km. So fill in the
     data array from the reflectivity product taking
     into account the resolution difference. In other
     words, the reflectivity product must be 1 km or
     coarser. If coarser, it would be 2 km, 4 km, etc.
     As an example, if the reflectivity product was 4 km
     resolution, there are 4-1km PRF product bins per
     1-4km reflectivity product bin. */

  for( radial = 0; radial < num_radials; radial++ )
  {
    indx = 0;
    for( gate = 0; gate < num_gates; gate++ )
    {
      for( i = 0; i < step; i++ )
      {
        Radial_data[REFLECTIVITY_INDEX][radial][indx] = data[radial][gate];
        indx++;
      }
      if( indx >= PRF_BINS_ALONG_RADIAL )
      {
        break;
      }
    }
  }

  Background_product_flag = HCI_PRF_PRODUCT_GOOD;
}

/************************************************************************
  Description: This function reads the obscuration product.
 ************************************************************************/

static void PRF_load_obscuration_product()
{
  int status = -1;
  int i = 0;
  int num_radials = hci_product_radials();
  int radial = 0;
  int gate = 0;

  Print_debug( "PRF_load_obscuration_product" );

  if( Background_product_flag != HCI_PRF_PRODUCT_GOOD )
  {
    Obscuration_product_flag = HCI_PRF_PRODUCT_ERROR;
    return;
  }

  /* Open the obscuration product. It consists of a set of bitmaps
     for each appropriate PRF indicating areas of range folding. */

  status = hci_load_prf_product( hci_product_date(),
                                 hci_product_time(),
                                 hci_product_elevation() );

  if( status <= 0 )
  {
    if( hci_prf_io_status() == RMT_CANCELLED )
    {
      HCI_LE_error( "User cancelled loading obscuration product" );
      HCI_task_exit( HCI_EXIT_SUCCESS );
    }
    HCI_LE_error( "Error openning obscuration product (%d)", status );
    Obscuration_product_flag = HCI_PRF_PRODUCT_ERROR;
    return;
  }

  /* Check the allowable PRF table and check to see if obscuration
     bitmap exists for each PRF. */

  for( i = PRF1; i < PRF1 + MAX_NUM_PRFS; i++ )
  {
    if( hci_prf_get_index( i ) < 0 )
    {
      PRF_product_flag[i] = HCI_NO_FLAG;
      for( radial = 0; radial < MAX_RADIALS_IN_CUT; radial++ )
      {
        for( gate = 0; gate < PRF_BINS_ALONG_RADIAL; gate++ )
        {
          Radial_data[i][radial][gate] = HCI_NO_FLAG;
        }
      }
    }
    else
    {
      PRF_product_flag[i] = HCI_YES_FLAG;

      /* Get the product radial data from the product and
         write it into working storage */

      for( radial = 0; radial < num_radials; radial++ )
      {
        for( gate = 0; gate < PRF_BINS_ALONG_RADIAL; gate++ )
        {
          Radial_data[i][radial][gate] = hci_prf_get_data( i, radial, gate );
        }
      }
    }
  }

  if( hci_prf_io_status() == RMT_CANCELLED )
  {
    HCI_LE_error( "User cancelled checking for obscuration bitmaps" );
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }

  Obscuration_product_flag = HCI_PRF_PRODUCT_GOOD;
}

/************************************************************************
  Description: Find latest background product.
 ************************************************************************/

static void Find_latest_background_product()
{
  int low_elevation = (int) ( VCP_low_elevation * 10 );
  int status = 0;
  ORPGDBM_query_data_t query_data[3];

  Print_debug( "Find_latest_background_product with low elevation of %d", low_elevation );

  /* Query the RPG products database for the latest background product */

  query_data[2].field = RPGP_ELEV;
  query_data[2].value = low_elevation;
  query_data[1].field = RPGP_PCODE;
  query_data[1].value = ORPGPAT_get_code( Background_product_id );
  query_data[0].field = ORPGDBM_MODE;
  query_data[0].value = ORPGDBM_ALL_MATCH |
                        ORPGDBM_HIGHEND_SEARCH;

  if( ( status = ORPGDBM_query( &DB_info, query_data, 3, 1 ) ) <= 0 )
  {
    HCI_LE_error( "ORPGDBM_query failed (%d)", status );
  }
}

/************************************************************************
  Description: Check for updated background product.
 ************************************************************************/

static void Check_for_updated_background_product()
{
  static int previous_vol_t = -1;

  Print_debug( "Check_for_updated_background_product" );

  Find_latest_background_product();

  if( previous_vol_t != DB_info.vol_t )
  {
    previous_vol_t = DB_info.vol_t;
    PRF_load_background_product();
    PRF_load_obscuration_product();
  }
}

/************************************************************************
  Description: Timer procedure for this application
 ************************************************************************/

static void Timer_proc()
{
  int vcp_number = ORPGVST_get_vcp();
  int i = 0;
  int prf = 0;

  Print_debug( "Timer_proc" );

  /* If the VCP has changed, tell the user to re-launch */

  if( vcp_number != VCP_number && Already_warned_flag == HCI_NO_FLAG )
  {
    Set_change_pending( HCI_NO_FLAG );
    Already_warned_flag = HCI_YES_FLAG;

    sprintf( Buf, "The VCP number has changed at the RPG. You\nmust acknowledge this warning and reopen\nthe PRF Control window." );

    hci_warning_popup( Top_widget, Buf, Close_callback );
  }

  /* If new volume, refresh the data */

  if( !hci_current_vcp_update_flag() )
  {
    /* Get the sector information from the current VCP/volume. */

    for( i = S1_INDX; i < NUM_SECTORS; i++ )
    {
      Sector_azimuth[i] = hci_current_vcp_get_sector_azimuth( Sector_id[i], 1 );
      Sector_prf[i] = hci_current_vcp_get_sector_prf_num( Sector_id[i], 1 );
    }

    if( PRF_Mode_status.selected_mode == HCI_PRF_MODE_MANUAL )
    {
      /* Index 0 is lowest elevation index */
      prf = hci_rda_adapt_allowable_prf_num( VCP_index, 0 );
      if( PRF_product_flag[prf] != 0 )
      {
        Resize_callback( NULL, NULL, NULL );
      }
    }
  }

  /* If "Auto Refresh" is yes, check for new background product */

  if( Auto_refresh_flag )
  {
    if( PRF_Mode_status_update_flag )
    {
      PRF_Mode_status_update_flag = HCI_NO_FLAG;
      PRF_Mode_get_updated_status_msg();
      Check_for_updated_background_product();
      PRF_Mode_update_storm_scroll_widgets();
      Resize_callback( NULL, NULL, NULL );
    }
  }

  /* User command pending so write new command */

  if( Write_prf_cmd_flag == HCI_YES_FLAG )
  {
    Write_prf_cmd_flag = HCI_NO_FLAG;
    Write_prf_cmd();
    Set_change_pending( HCI_NO_FLAG );
  }
}

/************************************************************************
  Description: Callback for "Apply" button.
 ************************************************************************/

static void Apply_callback( Widget w, XtPointer y, XtPointer z )
{
  int current = PRF_Mode_status.current_mode;
  int selected = PRF_Mode_status.selected_mode;
  int i = 0;

  /* For manual mode, certain VCPs require all sectors have the same PRF */
  if( selected == HCI_PRF_MODE_MANUAL )
  {
    if( VCP_number == 31 || VCP_number == 121 )
    {
      sprintf( Buf, "PRF cannot be modified for this VCP" );
      hci_warning_popup( Top_widget, Buf, NULL );
      HCI_LE_log( "PRF for VCP %d cannot be modified", VCP_number );
      return;
    }
    else if( VCP_number == 211 || VCP_number == 212 || VCP_number == 221 )
    {
      for( i = S1_INDX; i < NUM_SECTORS - 1; i++ )
      {
        if( Sector_prf[i] != Sector_prf[i+1] )
        {
          sprintf( Buf, "For this VCP all sectors must have the same PRF" );
          hci_warning_popup( Top_widget, Buf, NULL );
          HCI_LE_log( "PRF for sectors %d (%d) and %d (%d) don't match in VCP %d", i, Sector_prf[i], i+1, Sector_prf[i+1], VCP_number );
          return;
        }
      }
    }
  }

  /* All other cases */
  if( current == HCI_PRF_MODE_AUTO_STORM &&
      selected == HCI_PRF_MODE_AUTO_CELL )
  {
    sprintf( Buf, "You are about to enable single storm\ntracking under Auto PRF-Storm Mode.\nChanges take effect next volume scan.\nDo you want to continue?" );
  }
  else if( current == HCI_PRF_MODE_AUTO_CELL &&
      selected == HCI_PRF_MODE_AUTO_STORM )
  {
    sprintf( Buf, "You are about to disable single storm\ntracking under Auto PRF-Storm Mode.\nChanges take effect next volume scan.\nDo you want to continue?" );
  }
  else if( current == HCI_PRF_MODE_AUTO_CELL &&
      selected == HCI_PRF_MODE_AUTO_CELL )
  {
    sprintf( Buf, "You are about to select a new single-storm for\ntracking under Auto PRF-Storm Mode. Changes\ntake effect next volume scan.\nDo you want to continue?" );
  }
  else if( current != selected )
  {
    sprintf( Buf, "You are about to change PRF Mode.\nChanges take effect next volume scan.\nDo you want to continue?" );
  }
  else if( current == HCI_PRF_MODE_MANUAL )
  {
    sprintf( Buf, "You are about to modify the Manual PRF sectors.\nChanges take effect next volume scan.\nDo you want to continue?" );
  }
  else
  {
    sprintf( Buf, "Unknown changes being applied.\nDo you want to continue?" );
    HCI_LE_log( "Unknown changes being applied - current (%d) selected (%d)", current, selected );
  }
  hci_confirm_popup( Top_widget, Buf, Apply_accept, Apply_cancel );
}

/************************************************************************
  Description: Callback for "Auto Refresh" buttons.
 ************************************************************************/

static void Auto_refresh_callback( Widget w, XtPointer y, XtPointer z )
{
  XmString str;

  if( Auto_refresh_flag )
  {
    Auto_refresh_flag = HCI_NO_FLAG;
    str = XmStringCreateLocalized( "Auto Refresh Off" );
    Print_debug( "Auto refresh callback - OFF" );
  }
  else
  {
    Auto_refresh_flag = HCI_YES_FLAG;
    str = XmStringCreateLocalized( "Auto Refresh  On" );
    Print_debug( "Auto refresh callback - ON" );
  }

  XtVaSetValues( Auto_refresh_button, XmNlabelString, str,NULL ); 
  XmStringFree( str );
}

/************************************************************************
  Description: Callback for "Show Labels" buttons.
 ************************************************************************/

static void Show_labels_callback( Widget w, XtPointer y, XtPointer z )
{
  XmString str;

  if( Show_labels_flag )
  {
    Show_labels_flag = HCI_NO_FLAG;
    str = XmStringCreateLocalized( "Show Labels Off" );
    Print_debug( "Show labels callback - OFF" );
  }
  else
  {
    Show_labels_flag = HCI_YES_FLAG;
    str = XmStringCreateLocalized( "Show Labels  On" );
    Print_debug( "Show labels callback - ON" );
  }

  XtVaSetValues( Show_labels_button, XmNlabelString, str,NULL ); 
  XmStringFree( str );

  Resize_callback( NULL, NULL, NULL );
}

/************************************************************************
  Description: Retrieve ID of background product to display.
 ************************************************************************/

static void Initialize_background_product_ID()
{
  char *buf = NULL;
  Hci_prf_data_t *config_data = NULL;
  int status = 0;

  /* From site configuration data, get the product code for the base
     reflectivity product to be used as a background image. */

  buf = (char *) calloc( 1, ALIGNED_SIZE( sizeof( Hci_prf_data_t ) ) );

  status = ORPGDA_read( ORPGDAT_HCI_DATA, buf,
                        ALIGNED_SIZE( sizeof( Hci_prf_data_t ) ),
                        HCI_PRF_TASK_DATA_MSG_ID );

  if( status <= 0 )
  {
    HCI_LE_error( "Unable to read configuration data: %d", status );
  }
  else
  {
    config_data = (Hci_prf_data_t *) buf;

    if( HCI_is_low_bandwidth() )
    {
      Background_product_id =
          ORPGPAT_get_prod_id_from_code( (int) config_data->low_product );
    }
    else
    {
      Background_product_id =
          ORPGPAT_get_prod_id_from_code((int) config_data->high_product);
    }

    HCI_LE_log( "Background product ID set to: %d", Background_product_id );
  }

  if( buf != NULL ){ free( buf ); }

  Print_debug( "Background product ID: %d", Background_product_id );
}

/************************************************************************
  Description: This function displays a color bar for the data
               displayed in the PRF Selection display region.
 ************************************************************************/

static void PRF_display_background_product_legend()
{
  int i = 0;
  int j = 0;
  int ii = 0;
  int y = 0;
  int start_index = 0;
  int stop_index = 0;
  int color_step = 0;
  int x_offset_text = 0;
  int x_offset_box = 0;
  int y_offset = 0;
  int num_data_levels = 0;
  int text_width = -1;
  int max_text_width = -1;
  int max_width = -1;
  int half_text_height = -1;
  int product_colors[PRODUCT_COLORS];
  char *s_buf = NULL;

  Print_debug( "PRF_display_background_product_legend" );

  /* If background product isn't available, do nothing */

  if( Background_product_flag != HCI_PRF_PRODUCT_GOOD ){ return; }

  /* Set product colors and get number of data levels. */

  hci_get_product_colors( hci_product_code(), product_colors );
  num_data_levels = hci_product_attribute_num_data_levels();

  /* Determine starting y (Y-coord) of color bar so it's centered. */

  y_offset = Legend_area_y;
  y_offset += (Product_area_height-(num_data_levels*DATA_LEVEL_BOX_HEIGHT))/2;

  /* Determine starting pixel (X-coord) of color bar so it's centered.
     Assume "RF" is the longest data level string. */

  max_text_width = XTextWidth( Fontinfo_list, "RF", strlen( "RF" ) );
  max_width = max_text_width + DATA_LEVEL_MARGIN + DATA_LEVEL_BOX_WIDTH;
  x_offset_text = Legend_area_ctr_x - max_width/2;
  x_offset_box = x_offset_text + max_text_width + DATA_LEVEL_MARGIN;

  /* Determine color step. */

  color_step = PRODUCT_COLORS / num_data_levels;
  if( color_step <= 0 ){ color_step = 1; }

  Print_debug( "Num data levels: %d y offset: %d max text width: %d max width: %d x offset text: %d x offset box: %d color step: %d", num_data_levels, y_offset, max_text_width, max_width, x_offset_text, x_offset_box, color_step );

  /* Loop through color levels and draw color bars. */

  start_index = 0;
  stop_index  = PRODUCT_COLORS;
  ii = start_index;
  half_text_height = Font_height/2;

  for( i = start_index; i <= stop_index; i = i+color_step )
  {
    y = y_offset + (i*DATA_LEVEL_BOX_HEIGHT)/color_step;

    if( i == stop_index )
    {
      XSetForeground( PRF_display, PRF_gc, RF_color );
      XFillRectangle( PRF_display, PRF_pixmap, PRF_gc, x_offset_box,
                      y, DATA_LEVEL_BOX_WIDTH, DATA_LEVEL_BOX_HEIGHT );
      XSetForeground( PRF_display, PRF_gc, Text_fg );

      if( hci_product_attribute_data_level(ii) != NULL )
      {
        XDrawString( PRF_display, PRF_pixmap, PRF_gc,
                     x_offset_text, y+half_text_height, "RF", 2 );
      }
    }
    else
    {
      XSetForeground( PRF_display, PRF_gc, product_colors[i] );
      XFillRectangle( PRF_display, PRF_pixmap, PRF_gc, x_offset_box,
                      y, DATA_LEVEL_BOX_WIDTH, DATA_LEVEL_BOX_HEIGHT );
      XSetForeground( PRF_display, PRF_gc, Text_fg );

      if( ( s_buf = hci_product_attribute_data_level(ii) ) != NULL )
      {
        /* Manual hack. By default, the data levels are pre-pended
           with blank spaces. To remove them, loop over each character
           until a non-blank is found. Since I assume data levels have
           a max length of 2 characters, if a data level has a length
           of 1 keep one of the blank characters. */
        j=0;
        while( s_buf[j] == ' ' ){ j++; }
        if( strlen( &s_buf[j] ) == 1 ){ j--; }
        XDrawString( PRF_display, PRF_pixmap, PRF_gc,
                     x_offset_text, y+half_text_height,
                     (char *) &s_buf[j], strlen( (char *) &s_buf[j] ) );
      }
      ii++;
    }
  }

  XSetForeground( PRF_display, PRF_gc, Text_fg );

  XDrawRectangle( PRF_display, PRF_pixmap, PRF_gc, x_offset_box, y_offset,
                  DATA_LEVEL_BOX_WIDTH,
                  DATA_LEVEL_BOX_HEIGHT * ( num_data_levels + 1 ) );

  if( hci_product_attribute_units() != NULL )
  {
    s_buf = hci_product_attribute_units();
    text_width = XTextWidth( Fontinfo_list, s_buf, strlen( s_buf ) );
    XDrawString( PRF_display, PRF_pixmap, PRF_gc,
                   Legend_area_ctr_x - text_width/2,
                   y_offset - DRAW_AREA_Y_MARGIN,
                   hci_product_attribute_units(),
                   strlen( hci_product_attribute_units() ) );
  }

  XSetForeground( PRF_display, PRF_gc, Product_fg );
}

/************************************************************************
  Description: This function is activated when a new PRF mode is selected.
 ************************************************************************/

static void PRF_Mode_selected_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state =
                (XmToggleButtonCallbackStruct *) z;

  Print_debug( "PRF_Mode_selected_callback" );

  if( state->set )
  {
    PRF_Mode_status.selected_mode = (int) y;
    Print_debug( "Selected mode: %d", PRF_Mode_status.selected_mode );
    PRF_Mode_cmd_msg.command = PRF_Mode_status.selected_mode;
    PRF_Mode_cmd_msg.storm_id[0] = '\0';
    if( PRF_Mode_status.selected_mode != PRF_Mode_status.current_mode )
    {
      Print_debug( "Set change pending to YES" );
      Set_change_pending( HCI_YES_FLAG );
    }
    else
    {
      Print_debug( "Set change pending to NO" );
      Set_change_pending( HCI_NO_FLAG );
    }
    PRF_Mode_set_widgets( PRF_Mode_status.selected_mode );
    Resize_callback( NULL, NULL, NULL );
  }
}

/************************************************************************
  Description: This function is activated when changes are made.
 ************************************************************************/

static void Set_change_pending( int flag )
{
  Print_debug( "Set_change_pending with flag %d", flag );

  if( Change_pending_flag == HCI_NO_FLAG && flag == HCI_YES_FLAG )
  {
    /* Set the change flag if it isn't already. Make sure to
       lock the local copy of the current VCP so it isn't
       updated while being editing. */
    Change_pending_flag = HCI_YES_FLAG;
    XtSetSensitive( Apply_button, True );
    hci_lock_current_vcp_data();
  }
  else if( Change_pending_flag == HCI_YES_FLAG && flag == HCI_NO_FLAG )
  {
    Change_pending_flag = HCI_NO_FLAG;
    XtSetSensitive( Apply_button, False );
    hci_unlock_current_vcp_data();
  }
}

/************************************************************************
  Description: This function is activated when the user selects the 
               "Yes" button in the Apply confirmation popup.
 ************************************************************************/

static void Apply_accept( Widget w, XtPointer y, XtPointer z )
{
  Print_debug( "Apply accept callback" );
  Write_prf_cmd_flag = HCI_YES_FLAG;
}

/************************************************************************
  Description: This function is activated when the user selects the 
               "No" button in the Apply confirmation popup.
 ************************************************************************/

static void Apply_cancel( Widget w, XtPointer y, XtPointer z )
{
  Print_debug( "Apply cancel callback" );
  Write_prf_cmd_flag = HCI_NO_FLAG;
}

/************************************************************************
  Description: Write PRF mode to LB so it will take effect.
 ************************************************************************/

static void Write_prf_cmd()
{
  Print_debug( "Write_prf_cmd" );

  /* Set feedback line */
  if( PRF_Mode_cmd_msg.command == HCI_PRF_MODE_MANUAL )
  {
    hci_write_current_vcp_data();
    HCI_LE_status( "Manual PRF selected" );
    HCI_display_feedback( "Manual PRF selected" );
  }
  else if( PRF_Mode_cmd_msg.command == HCI_PRF_MODE_AUTO_ELEVATION )
  {
    HCI_LE_status( "Auto PRF - Elevation selected" );
    HCI_display_feedback( "Auto PRF - Elevation selected" );
  }
  else if( PRF_Mode_cmd_msg.command == HCI_PRF_MODE_AUTO_STORM )
  {
    HCI_LE_status( "Auto PRF - Multi-Storm selected" );
    HCI_display_feedback( "Auto PRF - Multi-Storm selected" );
  }
  else if( PRF_Mode_cmd_msg.command == HCI_PRF_MODE_AUTO_CELL )
  {
    HCI_LE_status( "Auto PRF - Single-Storm selected" );
    HCI_LE_status( "CELL ID: %s", PRF_Mode_cmd_msg.storm_id );
    HCI_display_feedback( "Auto PRF - Single-Storm selected" );
  }
  else
  {
    HCI_LE_status( "Unknown PRF Mode selected" );
    HCI_display_feedback( "Unknown PRF Mode selected" );
  }

  HCI_LE_status( "Write cmd: %d", hci_write_PRF_command( PRF_Mode_cmd_msg ) );
}

/************************************************************************
  Description: Determine if cursor is close to a sector boundary.
 ************************************************************************/

static int Is_cursor_near_boundary( float d1, float d2, float d3 )
{
  /* If cursor position is within the defined tolerance, return YES */

  if( ( d1 <= BOUNDARY_TOLERANCE ) ||
      ( d2 <= BOUNDARY_TOLERANCE ) ||
      ( d3 <= BOUNDARY_TOLERANCE ) )
  {
    return HCI_YES_FLAG;
  }

  return HCI_NO_FLAG;
}

/************************************************************************
  Description: Get sector containing cursor.
 ************************************************************************/

static int Get_sector_of_cursor_position( float d1, float d2, float d3 )
{
  if( d1 <= d2 )
  {
    if( d1 <= d3 )
    {
      /* -----Sector 1 boundary closest----- */
      return S1_INDX;
    }
    else
    {
      /* -----Sector 3 boundary closest----- */
      return S3_INDX;
    }
  }
  else
  {
    if( d2 <= d3 )
    {
      /* -----Sector 2 Boundary closest----- */
      return S2_INDX;
    }
  }

  /* -----Sector 3 boundary closest----- */
  return S3_INDX;
}

/************************************************************************
  Description: Get sector containing azimuth.
 ************************************************************************/

static int Get_sector_of_azimuth( float azimuth )
{
  /* Check to see if azimuth is in sector 1 */
  if( azimuth > Sector_azimuth[S1_INDX] )
  {
    if( Sector_azimuth[S2_INDX] > Sector_azimuth[S1_INDX] )
    {
      if( Sector_azimuth[S3_INDX] > Sector_azimuth[S2_INDX] )
      {
        if( azimuth < Sector_azimuth[S2_INDX] )
        {
          return S1_INDX;
        }
        else if( azimuth < Sector_azimuth[S3_INDX] )
        {
          return S2_INDX;
        }
        else
        {
          return S3_INDX;
        }
      }
      else
      {
        if( azimuth > Sector_azimuth[S2_INDX] )
        {
          return S2_INDX;
        }
        else
        {
          return S1_INDX;
        }
      }
    }
    else
    {
      return S1_INDX;
    }
  }
  else
  {
    /* azimuth < Sector_azimuth[S1_INDX] */
    if( Sector_azimuth[S2_INDX] > Sector_azimuth[S1_INDX] )
    {
      if( Sector_azimuth[S3_INDX] > Sector_azimuth[S2_INDX] )
      {
        return S3_INDX;
      }
      else
      {
        if( azimuth > Sector_azimuth[S3_INDX] )
        {
          return S3_INDX;
        }
        else
        {
          return S2_INDX;
        }
      }
    }
    else
    {
      if( azimuth > Sector_azimuth[S3_INDX] )
      {
        return S3_INDX;
      }
      else if( azimuth > Sector_azimuth[S2_INDX] )
      {
        return S2_INDX;
      }
    }
  }

  return S1_INDX;
}

/************************************************************************
  Description: Get information of current VCP.
 ************************************************************************/

static void Initialize_VCP_info()
{
  float elevation = 99.9;
  int i = 0;

  Print_debug( "Initialize_VCP_info" );

  /* Match the current active VCP with a file in the VCP adaptation
     data. If no match is found, exit. */
 
  HCI_PM( "Matching current VCP with adaptation data" );

  VCP_number = ORPGVST_get_vcp();

  VCP_index = hci_rda_adapt_vcp_table_index( abs( VCP_number ) );

  if( ORPGVST_io_status() == RMT_CANCELLED )
  {
    HCI_task_exit(HCI_EXIT_FAIL);
  }

  if( VCP_index < 0 )
  {
    HCI_LE_error( "VCP: %d not in adaptation data", VCP_number );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Get elevation of lowest cut */

  VCP_low_elevation = hci_rda_adapt_vcp_elevation_angle( VCP_number, 0 );

  Print_debug( "VCP: %d VCP index: %d VCP lowest elevation: %f", VCP_number, VCP_index, VCP_low_elevation );

  /* Get the sector information from the current VCP data. */

  for( i = S1_INDX; i < NUM_SECTORS; i++ )
  {
    Sector_azimuth[i] = hci_current_vcp_get_sector_azimuth( Sector_id[i], 1 );
    Sector_prf[i] = hci_current_vcp_get_sector_prf_num( Sector_id[i], 1 );
    Print_debug( "Index: %d Sector az: %d Sector_prf: %d", i, Sector_azimuth[i], Sector_prf[i] );
  }

  /* Calculate index of highest elevation at or below 7.0 degrees. */

  High_elevation_index = hci_current_vcp_num_elevations();

  while( elevation > UPPER_ELEVATION_LIMIT )
  {
    elevation = hci_current_vcp_elevation_angle( High_elevation_index - 1 );
    High_elevation_index--;

    if( High_elevation_index == 1 ){ break; }
  }

  Print_debug( "High elevaton index: %d", High_elevation_index );

  if( hci_vcp_io_status() == RMT_CANCELLED )
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}

/************************************************************************
  Description: Initialize RDA adaptation data.
 ************************************************************************/

static void Initialize_RDA_adaptation_data()
{
  int status = 0;

  Print_debug( "Initialize_RDA_adaptation_data" );

  /* Read RDA adaptation data. If read fails, log a message and exit. */
 
  HCI_PM( "Reading RDA Adaptation Data" );

  if( ( status = hci_read_rda_adaptation_data() ) < 0 )
  {
    HCI_LE_error( "Initialize RDA Adaptation failed: %d", status );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else if( status == RMT_CANCELLED )
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}
    
/************************************************************************
  Description: Initialize base data at RPG.
 ************************************************************************/

static void Initialize_base_data()
{
  int status = 0;

  Print_debug( "Initialize_base_data" );

  /* Initialize the basedata structure so we can get the unambiguous
     range later so we can determine the delta PRI value. */

  HCI_PM( "Reading Partial Base Data" );
  status = hci_basedata_seek( hci_basedata_msgid() - 1 ); 

  /* Only partially read base data when in low bandwidth mode */
  if( HCI_is_low_bandwidth() )
  {
    status = hci_basedata_read_radial( LB_NEXT, HCI_BASEDATA_PARTIAL_READ );
  }
  else
  {
    status = hci_basedata_read_radial( LB_NEXT, HCI_BASEDATA_COMPLETE_READ );
  }

  if( status <= 0 )
  {
    HCI_LE_error( "Unable to initialize Basedata: %d", status );
  }
  else if( status == RMT_CANCELLED )
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}

/************************************************************************
  Description: Initialize X/Motif related variables.
 ************************************************************************/

static void Initialize_display_variables()
{
  XtActionsRec actions;

  Print_debug( "Initialize_display_variables" );

  /* Define colors */

  Product_bg = hci_get_read_color( PRODUCT_BACKGROUND_COLOR );
  Product_fg = hci_get_read_color( PRODUCT_FOREGROUND_COLOR );
  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Text_bg = hci_get_read_color( TEXT_BACKGROUND );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  RF_color = hci_get_read_color( PURPLE );
  White_color = hci_get_read_color( WHITE );
  Green_color = hci_get_read_color( GREEN );
  Red_color = hci_get_read_color( RED );
  Warning_color = hci_get_read_color( WARNING_COLOR );
  List_font = hci_get_fontlist( LIST );
  Fontinfo_list = hci_get_fontinfo( LIST );
  Font_height = Fontinfo_list->ascent + Fontinfo_list->descent;

  /* Define mouse event translation table. */

  actions.string = "Mouse_callback";
  actions.proc = (XtActionProc) Mouse_callback;
  XtAppAddActions( HCI_get_appcontext(), &actions, 1 );

  /* Get various X properties for reference. */

  PRF_display = HCI_get_display();

  PRF_depth = XDefaultDepth( PRF_display, DefaultScreen( PRF_display ) );

  /* Initialize the read only color database for drawing */

  hci_initialize_product_colors( PRF_display, HCI_get_colormap() );

  /* Initialize reference points for the background product. It 
     could be handled with macros instead of variables, but this
     allows for the window to be resizable in the future if needed. */

  Product_area_x = DRAW_AREA_X_MARGIN;
  Product_area_y = DRAW_AREA_Y_MARGIN;
  Product_area_width = BG_PRODUCT_WIDTH;
  Product_area_height = BG_PRODUCT_HEIGHT;
  Product_area_ctr_x = Product_area_x + Product_area_width/2;
  Product_area_ctr_y = Product_area_y + Product_area_height/2;
  Product_area_scale_x = Product_area_width / (float)(PRF_BINS_ALONG_DIAMETER);
  Product_area_scale_y = -Product_area_scale_x;
  Legend_area_x = Product_area_x + Product_area_width;
  Legend_area_y = Product_area_y;
  Legend_area_width = LEGEND_WIDTH;
  Legend_area_height = Product_area_height;
  Legend_area_ctr_x = Legend_area_x + Legend_area_width/2;
  Legend_area_ctr_y = Product_area_ctr_y;
  Mouse_display_x  = Product_area_x + DRAW_AREA_LABEL_MARGIN;
  Mouse_display_y  = Product_area_y + DRAW_AREA_LABEL_MARGIN + Font_height;

  Print_debug( "Product area" );
  Print_debug( "     x: %d y: %d width: %d height: %d ctr x: %d ctr y: %d scale x: %d scale y: %d", Product_area_x, Product_area_y, Product_area_width, Product_area_height, Product_area_ctr_x, Product_area_ctr_y, Product_area_scale_x, Product_area_scale_y );
  Print_debug( "Legend area" );
  Print_debug( "     x: %d y: %d width: %d height: %d ctr x: %d ctr y: %d", Legend_area_x, Legend_area_y, Legend_area_width, Legend_area_height, Legend_area_ctr_x, Legend_area_ctr_y );
  Print_debug( "Mouse display" );
  Print_debug( "     x: %d  y: %d", Mouse_display_x, Mouse_display_y );
}

/************************************************************************
  Description: Initialize graphic context for drawing area.
 ************************************************************************/

static void Initialize_graphic_context()
{
  XGCValues gcv;
  XRectangle clip_area[2];

  Print_debug( "Initialize_graphic_context" );

  /* Define the base X display variables */

  PRF_window = XtWindow( PRF_draw_widget );
  PRF_pixmap = XCreatePixmap( PRF_display, PRF_window, 
                              PRF_width, PRF_height, PRF_depth );

  if( PRF_gc == (GC) NULL )
  {
    gcv.foreground = Product_fg;
    gcv.background = Product_bg;
    gcv.graphics_exposures = FALSE;

    PRF_gc = XCreateGC( PRF_display, PRF_window,
                    GCBackground | GCForeground | GCGraphicsExposures, &gcv );

    XSetFont( PRF_display, PRF_gc, hci_get_font( LIST ) );
  }

  clip_area[0].x = Product_area_x;
  clip_area[0].y = Product_area_y;
  clip_area[0].width = Product_area_width;
  clip_area[0].height = Product_area_height;
  clip_area[1].x = Legend_area_x;
  clip_area[1].y = Legend_area_y;
  clip_area[1].width = Legend_area_width;
  clip_area[1].height = Legend_area_height;
  XSetClipRectangles( PRF_display, PRF_gc, 0, 0, &clip_area[0], 2, Unsorted );
}

/************************************************************************
  Description: Initialize RPG related variables.
 ************************************************************************/

static void Initialize_RPG_variables()
{
  int i = 0;

  Print_debug( "Initialize_RPG_variables" );

  /* Get current PRF Mode status message */

  PRF_Mode_get_updated_status_msg();
  PRF_Mode_status.selected_mode = PRF_Mode_status.current_mode;
  PRF_Mode_cmd_msg.command = PRF_Mode_status.current_mode;

  Print_debug( "Current Mode: %d", PRF_Mode_status.current_mode );

  /* If mode is PRF Auto Cell, get ID of storm being tracked */
  if( PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_CELL )
  {
    for( i =0; i < PRF_Mode_status.num_storms; i++ )
    {
      if( PRF_Mode_status.storms[i].tracked_flag )
      {
        Print_debug( "Tracking ID %d", PRF_Mode_status.storms[i].id );
        strcpy( PRF_Mode_cmd_msg.storm_id, PRF_Mode_status.storms[i].id );
        break;
      }
    }
  }

  /* Get information of current VCP */

  Initialize_VCP_info();

  /* Define ID of background product */

  Initialize_background_product_ID();

  /* Initialize RDA adaptation data */

  Initialize_RDA_adaptation_data();

  /* Initialize RPG base data */

  Initialize_base_data();
}

/************************************************************************
  Description: Build widget for control buttons.
 ************************************************************************/

static void Build_control_buttons_widget()
{
  Print_debug( "Build_control_button_widgets" );

  /* Use a row_column widget to manage action selections. */

  Control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
        xmRowColumnWidgetClass, Top_form,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNorientation, XmHORIZONTAL,
        XmNbackground, Base_bg,
        NULL );

  /* Close button */

  Close_button = XtVaCreateManagedWidget( "Close",
        xmPushButtonWidgetClass, Control_rowcol,
        XmNfontList, List_font,
        XmNbackground, Button_bg,
        XmNforeground, Button_fg,
        NULL );

  XtAddCallback( Close_button, XmNactivateCallback, Close_callback, NULL );

  /* Apply button */

  Apply_button = XtVaCreateManagedWidget( "Apply",
        xmPushButtonWidgetClass, Control_rowcol,
        XmNfontList, List_font,
        XmNbackground, Button_bg,
        XmNforeground, Button_fg,
        XmNsensitive, False,
        NULL );

  XtAddCallback( Apply_button, XmNactivateCallback, Apply_callback, NULL );

  /* Auto-refresh button */

  Auto_refresh_button = XtVaCreateManagedWidget( "Auto Refresh  On",
        xmPushButtonWidgetClass, Control_rowcol,
        XmNfontList, List_font,
        XmNbackground, Button_bg,
        XmNforeground, Button_fg,
        NULL );

  XtAddCallback( Auto_refresh_button, XmNactivateCallback, Auto_refresh_callback, NULL );

  /* Show labels button */

  Show_labels_button = XtVaCreateManagedWidget( "Show Labels  On",
        xmPushButtonWidgetClass, Control_rowcol,
        XmNfontList, List_font,
        XmNbackground, Button_bg,
        XmNforeground, Button_fg,
        NULL );

  XtAddCallback( Show_labels_button, XmNactivateCallback, Show_labels_callback, NULL );
}

/************************************************************************
  Description: Build widget for PRF Mode radio buttons.
 ************************************************************************/

static void Build_PRF_Mode_control_widget()
{
  Widget separator_widget = (Widget) NULL;
  Widget prf_mode_filter = (Widget) NULL;
  int n = 0;
  Arg args[8];

  Print_debug( "Build_PRF_Mode_control_widget" );

  separator_widget = XtVaCreateManagedWidget( "prf_separator",
        xmSeparatorWidgetClass, Top_form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, Control_rowcol,
        NULL );

  PRF_Mode_rowcol = XtVaCreateManagedWidget( "prf_mode_rowcol",
        xmRowColumnWidgetClass, Top_form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_widget,
        XmNorientation, XmHORIZONTAL,
        XmNpacking, XmPACK_TIGHT,
        XmNbackground, Base_bg,
        NULL );

  XtVaCreateManagedWidget( "Mode: ",
        xmLabelWidgetClass, PRF_Mode_rowcol,
        XmNbackground, Text_bg,
        XmNforeground, Text_fg,
        XmNfontList, List_font,
        NULL );

  n = 0;
  XtSetArg( args[n], XmNforeground, Text_fg ); n++;
  XtSetArg( args[n], XmNbackground, Base_bg ); n++;
  XtSetArg( args[n], XmNfontList, List_font ); n++;
  XtSetArg( args[n], XmNorientation, XmHORIZONTAL ); n++;

  prf_mode_filter = XmCreateRadioBox( PRF_Mode_rowcol,
                      "prf_mode_filter", args, n );

  Elevation_PRF_button = XtVaCreateManagedWidget( "Auto PRF-Elevation",
        xmToggleButtonWidgetClass, prf_mode_filter,
        XmNbackground, Base_bg,
        XmNforeground, Text_fg,
        XmNselectColor, White_color,
        XmNfontList, List_font,
        NULL );

  XtAddCallback( Elevation_PRF_button,
                 XmNvalueChangedCallback, PRF_Mode_selected_callback,
                 (XtPointer) HCI_PRF_MODE_AUTO_ELEVATION );

  Storm_PRF_button = XtVaCreateManagedWidget( "Auto PRF-Storm",
        xmToggleButtonWidgetClass, prf_mode_filter,
        XmNbackground, Base_bg,
        XmNforeground, Text_fg,
        XmNselectColor, White_color,
        XmNfontList, List_font,
        NULL );

  XtAddCallback( Storm_PRF_button,
                 XmNvalueChangedCallback, PRF_Mode_selected_callback,
                 (XtPointer) HCI_PRF_MODE_AUTO_STORM );

  Manual_PRF_button = XtVaCreateManagedWidget( "Manual PRF",
        xmToggleButtonWidgetClass, prf_mode_filter,
        XmNbackground, Base_bg,
        XmNforeground, Text_fg,
        XmNselectColor, White_color,
        XmNfontList, List_font,
        NULL );

  XtAddCallback( Manual_PRF_button,
                 XmNvalueChangedCallback, PRF_Mode_selected_callback,
                 (XtPointer) HCI_PRF_MODE_MANUAL );

  XtManageChild( prf_mode_filter );
}

/************************************************************************
  Description: Build widget for PRF Mode description labels.
 ************************************************************************/

static void Build_PRF_Mode_description_widget()
{
  Widget separator_widget = (Widget) NULL;

  Print_debug( "Build_PRF_Mode_description_widget" );

  separator_widget = XtVaCreateManagedWidget( "prf_separator",
        xmSeparatorWidgetClass, Top_form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, PRF_Mode_rowcol,
        NULL );

  /* Descriptions for each PRF Mode. */

  Description_rowcol = XtVaCreateManagedWidget( "desc_rowcol",
        xmRowColumnWidgetClass, Top_form,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_widget,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNorientation, XmHORIZONTAL,
        XmNpacking, XmPACK_TIGHT,
        XmNbackground, Base_bg,
        NULL );

  Description_label_auto_elevation = XtVaCreateWidget( "Auto PRF-Elevation mode selects the PRF that results in the fewest\nnumber of range folded gates regardless of the location of storms.\n\n",
        xmLabelWidgetClass, Description_rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        NULL );

  Description_label_auto_storm = XtVaCreateWidget( "Auto PRF-Storm mode uses up to three storm cells to determine an\noptimum PRF.\n\n",
        xmLabelWidgetClass, Description_rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        NULL );

  Description_label_manual = XtVaCreateWidget( "Manual PRF mode allows you to set boundaries and PRF for three sectors.\nMove sector boundaries by selecting a sector edge and dragging the mouse.\nChange sector PRFs by positioning the cursor in a sector and clicking\nthe Left (+) and Right (-) mouse buttons.",
        xmLabelWidgetClass, Description_rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        NULL );

  if( PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_ELEVATION )
  {
    XtManageChild( Description_label_auto_elevation );
  }
  else if( PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_STORM ||
           PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_CELL  )
  {
    XtManageChild( Description_label_auto_storm );
  }
  else if( PRF_Mode_status.current_mode == HCI_PRF_MODE_MANUAL )
  {
    XtManageChild( Description_label_manual );
  }
}

/************************************************************************
  Description: Build widget for drawing area.
 ************************************************************************/

static void Build_PRF_draw_area_widget()
{
  Widget separator_widget = (Widget) NULL;

  Print_debug( "Build_PRF_draw_area_widget" );

  separator_widget = XtVaCreateManagedWidget( "prf_separator",
        xmSeparatorWidgetClass, Top_form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, Description_rowcol,
        NULL );

  PRF_draw_widget = XtVaCreateWidget( "prf_drawing_area",
        xmDrawingAreaWidgetClass, Top_form,
        XmNwidth, PRF_width,
        XmNheight, PRF_height,
        XmNbackground, Base_bg,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_widget,
        XmNtranslations, XtParseTranslationTable( PRF_translations ),
        NULL );

  XtAddCallback( PRF_draw_widget, XmNexposeCallback, Expose_callback, NULL );
  XtAddCallback( PRF_draw_widget, XmNresizeCallback, Resize_callback, NULL );
  XtManageChild( PRF_draw_widget );
}

/************************************************************************
  Description: Build widget for scrolling list of storms.
 ************************************************************************/

static void Build_storm_scroll_widget()
{
  Widget separator_widget = (Widget) NULL;
  Widget label_rowcol = (Widget) NULL;
  Widget label_widget = (Widget) NULL;
  Widget form = (Widget) NULL;
  Widget clip = (Widget) NULL;
  int i = 0;

  Print_debug( "Build_storm_scroll_widget" );

  separator_widget = XtVaCreateManagedWidget( "prf_separator",
        xmSeparatorWidgetClass, Top_form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, PRF_draw_widget,
        NULL );

  label_rowcol = XtVaCreateManagedWidget( "label_rowcol",
        xmRowColumnWidgetClass, Top_form,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_widget,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNorientation, XmHORIZONTAL,
        XmNpacking, XmPACK_TIGHT,
        XmNbackground, Base_bg,
        NULL );

  label_widget = XtVaCreateManagedWidget( "       ID   Azimuth   Range     VIL       Rmax    Rmax Hgt  Storm Top",
        xmLabelWidgetClass, label_rowcol,
        XmNbackground, Text_bg,
        XmNforeground, Text_fg,
        XmNfontList, List_font,
        NULL );

  Data_scroll = XtVaCreateManagedWidget( "data_scroll",
        xmScrolledWindowWidgetClass, Top_form,
        XmNheight, SCROLL_HEIGHT,
        XmNwidth, DRAW_AREA_WIDTH,
        XmNscrollingPolicy, XmAUTOMATIC,
        XmNforeground, Base_bg,
        XmNbackground, Base_bg,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_widget,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL );

  XtVaGetValues( Data_scroll, XmNclipWindow, &clip, NULL );
  XtVaSetValues( clip, XmNbackground, Base_bg, NULL );

  form = XtVaCreateWidget( "scroll_form",
        xmFormWidgetClass, Data_scroll,
        XmNbackground, Base_bg,
        NULL );

  for( i = 0; i < MAX_NUM_STORMS; i++ )
  {
    PRF_Mode_status.storms[i].rowcol = XtVaCreateWidget( "data_rowcol",
        xmRowColumnWidgetClass, form,
        XmNorientation, XmHORIZONTAL,
        XmNbackground, Base_bg,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL );

    if( i == 0 )
    {
      XtVaSetValues( PRF_Mode_status.storms[i].rowcol, XmNtopAttachment, XmATTACH_FORM, NULL );
    }
    else
    {
      XtVaSetValues( PRF_Mode_status.storms[i].rowcol, XmNtopAttachment, XmATTACH_WIDGET, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].rowcol, XmNtopWidget, PRF_Mode_status.storms[i-1].rowcol, NULL );
    }

    PRF_Mode_status.storms[i].select_button = XtVaCreateManagedWidget( " ",
        xmToggleButtonWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNselectColor, White_color,
        XmNborderColor, Base_bg,
        XmNset, False,
        NULL);

    XtAddCallback( PRF_Mode_status.storms[i].select_button,
        XmNvalueChangedCallback, PRF_storm_selected_callback,
        (XtPointer) i );

    PRF_Mode_status.storms[i].id_label = XtVaCreateManagedWidget( "id_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, ID_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    PRF_Mode_status.storms[i].az_label = XtVaCreateManagedWidget( "az_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, AZIMUTH_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    PRF_Mode_status.storms[i].range_label = XtVaCreateManagedWidget( "range_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, RANGE_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    PRF_Mode_status.storms[i].vil_label = XtVaCreateManagedWidget( "vil_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, VIL_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    PRF_Mode_status.storms[i].dbzm_label = XtVaCreateManagedWidget( "dbzm_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, DBZM_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    PRF_Mode_status.storms[i].dbzm_height_label = XtVaCreateManagedWidget( "dbzm_height_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, DBZM_HEIGHT_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    PRF_Mode_status.storms[i].storm_top_label = XtVaCreateManagedWidget( "storm_top_label",
        xmTextFieldWidgetClass, PRF_Mode_status.storms[i].rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNhighlightColor, White_color,
        XmNborderColor, Base_bg,
        XmNcolumns, STORM_TOP_LABEL_WIDTH,
        XmNeditable, False,
        NULL );

    XtManageChild( PRF_Mode_status.storms[i].rowcol );
  }

  PRF_Mode_update_storm_scroll_widgets();

  XtManageChild( form );
}

/************************************************************************
  Description: Function activated when a storm is selected.
 ************************************************************************/

static void PRF_storm_selected_callback( Widget w, XtPointer y, XtPointer z )
{
  static int previous_index = -1;
  int index = (int) y;
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) z;
  int i = 0;
  int do_something_flag = 1;

  Print_debug( "PRF_storm_selected_callback with index %d", index );

  /* If auto cell mode, initialize previous_index to index of
     storm being tracked. This allows the toggle button for
     the tracked storm to be toggled off */
  if( PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_CELL &&
      previous_index < 0 )
  {
    for( i = 0; i < PRF_Mode_status.num_storms; i++ )
    {
      if( PRF_Mode_status.storms[i].tracked_flag )
      {
        previous_index = i;
        Print_debug( "Previous index %d", previous_index );
        break;
      }
    }
  }

  if( cbs->set )
  {
    /* Toggle button is being selected from an unselected state */
    Print_debug( "Select" );
    if( PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_CELL )
    {
      if( PRF_Mode_status.storms[index].tracked_flag == YES )
      {
        /* If auto cell mode and user selects the storm
           already being tracked, don't do anything. This
           prevents the Apply button from being sensitized. */
        Print_debug( "Same single storm, do nothing" );
        do_something_flag = 0;
      }
    }

    PRF_Mode_status.storms[index].selected_flag = HCI_YES_FLAG;
    strcpy( PRF_Mode_cmd_msg.storm_id, PRF_Mode_status.storms[index].id );
    if( previous_index > -1 && previous_index != index )
    {
      /* Call callback for button of previous_index so it
         will de-select. This is the only way to ensure only
         only button is selected at a time. */
      XmToggleButtonSetState( PRF_Mode_status.storms[previous_index].select_button, False, True );
    }
    previous_index = index;
  }
  else
  {
    /* Toggle button is being de-selected from a selected state */
    Print_debug( "De-select" );
    PRF_Mode_status.storms[index].selected_flag = HCI_NO_FLAG;
  }

  PRF_Mode_status.selected_mode = HCI_PRF_MODE_AUTO_STORM;
  PRF_Mode_cmd_msg.command = HCI_PRF_MODE_AUTO_STORM;
  PRF_Mode_cmd_msg.storm_id[0] = '\0';

  for( i = 0; i < PRF_Mode_status.num_storms; i++ )
  {
    if( PRF_Mode_status.storms[i].selected_flag )
    {
      PRF_Mode_status.selected_mode = HCI_PRF_MODE_AUTO_CELL;
      PRF_Mode_cmd_msg.command = HCI_PRF_MODE_AUTO_CELL;
      strcpy( PRF_Mode_cmd_msg.storm_id, PRF_Mode_status.storms[i].id );
      break;
    }
  }

  if( do_something_flag ){ Set_change_pending( HCI_YES_FLAG ); }
  else{ Set_change_pending( HCI_NO_FLAG ); }
}

/************************************************************************
  Description: Display background product.
 ************************************************************************/

static void PRF_display_background_product()
{
  Print_debug( "PRF_display_background_product" );

  if( PRF_Mode_status.selected_mode == HCI_PRF_MODE_MANUAL )
  {
    PRF_display_background_product_manual_mode();
  }
  else
  {
    PRF_display_background_product_auto_mode();
  }
}

/************************************************************************
  Description: Display labels over background product.
 ************************************************************************/

static void PRF_display_labels()
{
  Print_debug( "PRF_display_labels" );

  /* Display mode-specific labels */

  if( PRF_Mode_status.selected_mode == HCI_PRF_MODE_MANUAL )
  {
    PRF_display_labels_manual_mode();
  }
  else if( PRF_Mode_status.selected_mode == HCI_PRF_MODE_AUTO_STORM ||
           PRF_Mode_status.selected_mode == HCI_PRF_MODE_AUTO_CELL )
  {
    PRF_display_labels_auto_storm_mode();
  }
}

/************************************************************************
  Description: This function displays lables for PRF Manual Mode.
 ************************************************************************/

static void PRF_display_labels_manual_mode()
{
  int x = 0;
  int y = 0;
  float azimuth = 0.0;
  int string_width = 0;
  int i = 0;
  float rad_angle = 0.0;
  float ratio = 0.0;
  int indexA = 0;
  int indexB = 0;

  Print_debug( "PRF_display_labels_manual_mode" );

  if( !Show_labels_flag )
  {
    Print_debug( "Show labels flag is %d, do nothing", Show_labels_flag );
    return;
  }

  for( i = S1_INDX; i < NUM_SECTORS; i++ )
  {
    indexA = i;
    indexB = (indexA+1)%NUM_SECTORS;

    rad_angle = (Sector_azimuth[indexB]-DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD;
    ratio = cos( rad_angle );
    x = PRF_BINS_ALONG_RADIAL * ratio * Product_area_scale_x + Product_area_ctr_x;

    rad_angle = (Sector_azimuth[indexB]+DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD;
    ratio = sin( rad_angle );
    y = PRF_BINS_ALONG_RADIAL * ratio * Product_area_scale_y + Product_area_ctr_y;

    Print_debug( "index A: %d index B: %d x B: %d y B: %d", indexA, indexB, x, y );

    azimuth = Sector_azimuth[indexA] + Sector_azimuth[indexB];
    if( Sector_azimuth[indexA] > Sector_azimuth[indexB] )
    {
      azimuth += DEGREES_IN_A_CIRCLE;
    }

    azimuth /= 2.0;

    if( azimuth >= DEGREES_IN_A_CIRCLE )
    {
      azimuth -= DEGREES_IN_A_CIRCLE;
    }

    Print_debug( "Sector az A: %f Sector az B: %f azimuth: %f", Sector_azimuth[indexA], Sector_azimuth[indexB], azimuth );

    rad_angle = (azimuth-DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD;
    ratio = cos( rad_angle );
    x = SECTOR_LABEL_POSITION_RATIO * ratio * Product_area_scale_x + Product_area_ctr_x;
  
    Print_debug( "X ratio: %f x: %d", ratio, x );

    rad_angle = (azimuth+DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD;
    ratio = sin( rad_angle );
    y = SECTOR_LABEL_POSITION_RATIO * ratio * Product_area_scale_y + Product_area_ctr_y;
 
    Print_debug( "Y ratio: %f y: %d", ratio, y );
 
    sprintf( Buf, "Sector %d", Sector_id[i] );
    string_width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
    XDrawImageString( PRF_display, PRF_window, PRF_gc,
                      x - string_width/2, y,
                      Buf, strlen( Buf ) );

    y = y + Font_height;

    if( !PRF_product_flag[Sector_prf[indexA]] )
    {
      sprintf( Buf, "Data Not" );
      string_width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
      XDrawImageString( PRF_display, PRF_window, PRF_gc,
                        x - string_width/2, y,
                        Buf, strlen( Buf ) );
      y = y + Font_height;
      sprintf( Buf, "Available" );
      string_width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
      XDrawImageString( PRF_display, PRF_window, PRF_gc,
                        x - string_width/2, y,
                        Buf, strlen( Buf ) );
    }
    else
    {
      sprintf( Buf,"  PRF#%d ", Sector_prf[indexA] );
      string_width = XTextWidth( Fontinfo_list, Buf, strlen( Buf ) );
      XDrawImageString( PRF_display, PRF_window, PRF_gc,
                        x - string_width/2, y,
                        Buf, strlen( Buf ) );
    }
  }
}

/************************************************************************
  Description: This function displays labels for Auto-Storm PRF Mode.
 ************************************************************************/

static void PRF_display_labels_auto_storm_mode()
{
  int circle_x = 0;
  int circle_y = 0;
  int id_x = 0;
  int id_y = 0;
  int id_width = 0;
  int i = 0;
  int xdiff = 0;
  int influence_diameter = PRF_Mode_status.radius * Product_area_scale_x * 2;
  int influence_radius = influence_diameter/2;
  char id[ID_LABEL_WIDTH+1];
  int start_angle = 0;
  int end_angle = 0;
  int right_edge = Product_area_x + Product_area_width;

  Print_debug( "PRF_display_labels_auto_storm_mode" );

  if( !Show_labels_flag )
  {
    Print_debug( "Show labels flag is %d, do nothing", Show_labels_flag );
    return;
  }

  for( i = 0; i < PRF_Mode_status.num_storms; i++ )
  {
    XSetForeground( PRF_display, PRF_gc, Product_fg );
    XSetBackground( PRF_display, PRF_gc, Product_bg );

    strcpy( id, PRF_Mode_status.storms[i].id );

    Print_debug( "Index: %d ID: %s", i, id );

    /* Determine x,y coordinates of upper left corner of rectangle
       to contain the storm circle */

    circle_x = (int)(PRF_Mode_status.storms[i].range *
          cos( (double)(PRF_Mode_status.storms[i].azimuth-DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD )) *
          Product_area_scale_x + Product_area_ctr_x - influence_radius;

    circle_y = (int)(PRF_Mode_status.storms[i].range *
          sin( (double)(PRF_Mode_status.storms[i].azimuth+DEGREES_IN_A_QUADRANT)*HCI_DEG_TO_RAD )) *
          Product_area_scale_y + Product_area_ctr_y - influence_radius;

    /* For the upper, lower and left edges of the product display area,
       Storm ID circles are clipped at the edge. The clipping is done
       automatically after ClipRectangles were defined for the Graphics
       Context of the display. For the right edge, the legend abuts the
       product display area. Using a ClipRectangle on the right edge would
       cause the legend to be hidden. The solution (i.e. hack) is to ensure
       XDrawArc only draws in the product display area by finding the
       circle's start/end azimuth contained within the area to be visible.
       To be efficient, only use the hack if the circle is close to the
       right edge. */

    start_angle = 0;
    end_angle = DEGREES_IN_A_CIRCLE;
    if( ( circle_x + influence_diameter ) > right_edge )
    {
      xdiff = right_edge - ( circle_x + influence_radius );
      start_angle = (int) (acos( (float) xdiff/influence_radius )*HCI_RAD_TO_DEG);
      end_angle = DEGREES_IN_A_CIRCLE - ( start_angle * 2 );
    }

    /* If the Storm ID is tracked, make the circle bold */

    if( PRF_Mode_status.storms[i].tracked_flag )
    {
      XSetLineAttributes(PRF_display, PRF_gc, 3, LineSolid, CapButt, JoinMiter);
    }
    else
    {
      XSetLineAttributes(PRF_display, PRF_gc, 0, LineSolid, CapButt, JoinMiter);
    }

    Print_debug( "Tracked: %d Circle x: %d Circle y: %d start angle: %d end_angle: %d", PRF_Mode_status.storms[i].tracked_flag, circle_x, circle_y, start_angle, end_angle );

    XDrawArc( PRF_display, PRF_window, PRF_gc,
             circle_x, circle_y, influence_diameter, influence_diameter,
             start_angle * XLIB_DEGREE_RESOLUTION,
             end_angle * XLIB_DEGREE_RESOLUTION );

    /* Determine x,y coordinates of origin of Storm ID string */

    id_x = circle_x + influence_diameter;
    id_y = circle_y + influence_radius + Fontinfo_list->descent;
    id_width  = XTextWidth( Fontinfo_list, id, strlen( id ) );

    /* Make sure the Storm ID string isn't beyond the edge of the
       product display area. Since the Storm ID string is on the
       right side of the circle by default, the left edge of the
       product display area can be ignored. */

    if( ( id_y - Fontinfo_list->ascent ) < Product_area_y )
    {
      /* Beyond upper edge */
      id_x -= (influence_radius + id_width/2 );
      id_y = circle_y + influence_diameter + Fontinfo_list->ascent;
    }
    else if( ( id_y + Fontinfo_list->descent ) > Product_area_y + Product_area_height )
    {
      /* Beyond lower edge */
      id_x -= (influence_radius + id_width/2 );
      id_y = circle_y - Fontinfo_list->descent;
    }
    else if( id_x + id_width > right_edge )
    {
      /* Beyond right edge */
      id_x = circle_x - id_width;
    }

    /* If the Storm ID is being tracked, invert the colors */

    if( PRF_Mode_status.storms[i].tracked_flag )
    {
      XSetForeground( PRF_display, PRF_gc, Product_bg );
      XSetBackground( PRF_display, PRF_gc, Product_fg );
    }

    Print_debug( "Tracked: %d id_x: %d id_y: %d id_width: %d", PRF_Mode_status.storms[i].tracked_flag, id_x, id_y, id_width );
 
    XDrawImageString( PRF_display, PRF_window, PRF_gc,
                      id_x, id_y, id, strlen( id ) );
  }
  XSetLineAttributes(PRF_display, PRF_gc, 0, LineSolid, CapButt, JoinMiter);
  XSetForeground( PRF_display, PRF_gc, Product_fg );
}

/************************************************************************
  Description: Register to be updated when PRF Mode status is updated.
 ************************************************************************/

static void PRF_Mode_register_for_status_msg_updates()
{
  int status = -1;

  Print_debug( "PRF_Mode_register_for_status_msg_updates" );

  /* Register for PRF Mode updates. */

  ORPGDA_write_permission( ORPGDAT_PRF_COMMAND_INFO );
  status = ORPGDA_UN_register( ORPGDAT_PRF_COMMAND_INFO,
                               ORPGDAT_PRF_STATUS_MSGID,
                               PRF_Mode_status_msg_callback );

  if( status != 0 )
  {
    HCI_LE_log( "UN_register ORPGDAT_PRF_COMMAND_INFO failed: %d", status );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
}

/************************************************************************
  Description: This function is the callback activated when the PRF
               Mode is updated.
 ************************************************************************/

static void PRF_Mode_status_msg_callback( int fd, LB_id_t msgid, int msginfo, void *arg )
{
  Print_debug( "PRF_Mode_status_msg_callback executed" );
  PRF_Mode_status_update_flag = HCI_YES_FLAG;
}

/************************************************************************
  Description: Get updated PRF Mode status information.
 ************************************************************************/

static void PRF_Mode_get_updated_status_msg()
{
  int i = 0;
  int j = 0;
  Prf_status_t status_msg;

  Print_debug( "PRF_Mode_get_updated_status_msg" );

  status_msg = hci_get_PRF_Mode_status_msg();

  /* Create fake data for debugging */
  if( Fake_data_flag == HCI_YES_FLAG )
  {
    Initstorms( &status_msg, 8, 3 );
  }

  /* Print PRF Mode status msg */
  PRF_Mode_print_status_msg( &status_msg );

  PRF_Mode_status.current_mode = status_msg.state;
  PRF_Mode_status.radius = status_msg.radius;
  PRF_Mode_status.num_storms = status_msg.num_storms;

  for( i = 0; i < status_msg.num_storms; i++ )
  {
    sprintf( PRF_Mode_status.storms[i].id, "%s", status_msg.storm_data[i].storm_id );
    PRF_Mode_status.storms[i].azimuth = status_msg.storm_data[i].storm_azm;
    sprintf( PRF_Mode_status.storms[i].azimuth_buf, "%3d deg", (int) PRF_Mode_status.storms[i].azimuth );
    PRF_Mode_status.storms[i].range = status_msg.storm_data[i].storm_rng;
    sprintf( PRF_Mode_status.storms[i].range_buf, "%3d nm", (int) (PRF_Mode_status.storms[i].range * HCI_KM_TO_NM) );
    PRF_Mode_status.storms[i].vil = status_msg.storm_data[i].storm_vil;
    sprintf( PRF_Mode_status.storms[i].vil_buf, "%2d kg/m^2", (int) PRF_Mode_status.storms[i].vil );
    PRF_Mode_status.storms[i].dbzm = status_msg.storm_data[i].storm_mx_refl;
    sprintf( PRF_Mode_status.storms[i].dbzm_buf, "%2d dBZ", (int) PRF_Mode_status.storms[i].dbzm );
    PRF_Mode_status.storms[i].dbzm_height = status_msg.storm_data[i].storm_ht_mx_refl;
    sprintf( PRF_Mode_status.storms[i].dbzm_height_buf, " %4.1f kft", PRF_Mode_status.storms[i].dbzm_height * HCI_KM_TO_KFT );
    PRF_Mode_status.storms[i].storm_top = status_msg.storm_data[i].storm_ht_top ;
    sprintf( PRF_Mode_status.storms[i].storm_top_buf, " %4.1f kft", PRF_Mode_status.storms[i].storm_top * HCI_KM_TO_KFT );
    PRF_Mode_status.storms[i].selected_flag = HCI_NO_FLAG;
    PRF_Mode_status.storms[i].tracked_flag = HCI_NO_FLAG;
    for( j = 0; j < status_msg.num_storms_tracked; j++ )
    {
      if( status_msg.ids_storms_tracked[j] == i )
      {
        PRF_Mode_status.storms[i].tracked_flag = HCI_YES_FLAG;
        break;
      }
    }
  }

  for( ; i < MAX_NUM_STORMS; i++ )
  {
    Clear_storm_struct_entry(i);
  }
}

/************************************************************************
  Description: Update widgets with the new PRF Mode status information.
 ************************************************************************/

static void PRF_Mode_update_storm_scroll_widgets()
{
  int i = 0;
  int j = 0;
  char buf[ID_LABEL_WIDTH];
  int id_len = 0;
  int num_lead_blanks = 0;

  Print_debug( "PRF_Mode_update_storm_scroll_widgets" );

  for( i = 0; i < MAX_NUM_STORMS; i++ )
  {
    if( i < PRF_Mode_status.num_storms )
    {
      /* Center id within label */
      id_len = strlen( PRF_Mode_status.storms[i].id );
      num_lead_blanks = (ID_LABEL_WIDTH - id_len) / 2;
      for( j = 0; j < num_lead_blanks; j++ ){ buf[j] = ' '; }
      sprintf( &buf[j], "%s", PRF_Mode_status.storms[i].id );
      XmTextSetString( PRF_Mode_status.storms[i].id_label, buf );
      XtSetSensitive( PRF_Mode_status.storms[i].select_button, True );
    }
    else
    {
      XmTextSetString( PRF_Mode_status.storms[i].id_label, PRF_Mode_status.storms[i].id );
      XtSetSensitive( PRF_Mode_status.storms[i].select_button, False );
    }
    XmTextSetString( PRF_Mode_status.storms[i].az_label, PRF_Mode_status.storms[i].azimuth_buf );
    XmTextSetString( PRF_Mode_status.storms[i].range_label, PRF_Mode_status.storms[i].range_buf );
    XmTextSetString( PRF_Mode_status.storms[i].vil_label, PRF_Mode_status.storms[i].vil_buf );
    XmTextSetString( PRF_Mode_status.storms[i].dbzm_label, PRF_Mode_status.storms[i].dbzm_buf );
    XmTextSetString( PRF_Mode_status.storms[i].dbzm_height_label, PRF_Mode_status.storms[i].dbzm_height_buf );
    XmTextSetString( PRF_Mode_status.storms[i].storm_top_label, PRF_Mode_status.storms[i].storm_top_buf );
    if( PRF_Mode_status.storms[i].selected_flag )
    {
      XmToggleButtonSetState( PRF_Mode_status.storms[i].select_button, True, False );
    }
    else
    {
      XmToggleButtonSetState( PRF_Mode_status.storms[i].select_button, False, False );
    }
    if( PRF_Mode_status.storms[i].tracked_flag )
    {
      if( PRF_Mode_status.current_mode == HCI_PRF_MODE_AUTO_CELL )
      {
        XmToggleButtonSetState( PRF_Mode_status.storms[i].select_button, True, False );
      }
      XtVaSetValues( PRF_Mode_status.storms[i].id_label, XmNbackground, White_color, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].az_label, XmNbackground, White_color, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].range_label, XmNbackground, White_color, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].vil_label, XmNbackground, White_color, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].dbzm_label, XmNbackground, White_color, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].dbzm_height_label, XmNbackground, White_color, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].storm_top_label, XmNbackground, White_color, NULL );
    }
    else
    {
      XtVaSetValues( PRF_Mode_status.storms[i].id_label, XmNbackground, Base_bg, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].az_label, XmNbackground, Base_bg, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].range_label, XmNbackground, Base_bg, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].vil_label, XmNbackground, Base_bg, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].dbzm_label, XmNbackground, Base_bg, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].dbzm_height_label, XmNbackground, Base_bg, NULL );
      XtVaSetValues( PRF_Mode_status.storms[i].storm_top_label, XmNbackground, Base_bg, NULL );
    }
  }
}

/************************************************************************
  Description: Clear storm entry.
 ************************************************************************/

static void Clear_storm_struct_entry( int index )
{
  Print_debug( "Clear_storm_struct_entry of index %d", index );

  PRF_Mode_status.storms[index].id[0] = '\0';
  PRF_Mode_status.storms[index].azimuth_buf[0] = '\0';
  PRF_Mode_status.storms[index].range_buf[0] = '\0';
  PRF_Mode_status.storms[index].vil_buf[0] = '\0';
  PRF_Mode_status.storms[index].dbzm_buf[0] = '\0';
  PRF_Mode_status.storms[index].dbzm_height_buf[0] = '\0';
  PRF_Mode_status.storms[index].storm_top_buf[0] = '\0';
  PRF_Mode_status.storms[index].azimuth = 0.0;
  PRF_Mode_status.storms[index].range = 0.0;
  PRF_Mode_status.storms[index].vil = 0.0;
  PRF_Mode_status.storms[index].dbzm = 0.0;
  PRF_Mode_status.storms[index].dbzm_height = 0.0;
  PRF_Mode_status.storms[index].storm_top = 0.0;
  PRF_Mode_status.storms[index].selected_flag = HCI_NO_FLAG;
  PRF_Mode_status.storms[index].tracked_flag = HCI_NO_FLAG;
}

/************************************************************************
  Description: Set appearance according to mode.
 ************************************************************************/

static void PRF_Mode_set_widgets( int mode_flag )
{
  Print_debug( "PRF_Mode_set_widgets with flag %d", mode_flag );

  if( mode_flag == HCI_PRF_MODE_MANUAL )
  {
    XtVaSetValues( Manual_PRF_button, XmNset, True, NULL );
    XtVaSetValues( Elevation_PRF_button, XmNset, False, NULL );
    XtVaSetValues( Storm_PRF_button, XmNset, False, NULL );
    XtManageChild( Description_label_manual );
    if( XtIsManaged( Description_label_auto_storm ) )
    {
      XtUnmanageChild( Description_label_auto_storm );
    }
    else if( XtIsManaged( Description_label_auto_elevation ) )
    {
      XtUnmanageChild( Description_label_auto_elevation );
    }
    XtSetSensitive( Data_scroll, False );
  }
  else if( mode_flag == HCI_PRF_MODE_AUTO_ELEVATION )
  {
    XtVaSetValues( Manual_PRF_button, XmNset, False, NULL );
    XtVaSetValues( Elevation_PRF_button, XmNset, True, NULL );
    XtVaSetValues( Storm_PRF_button, XmNset, False, NULL );
    XtManageChild( Description_label_auto_elevation );
    if( XtIsManaged( Description_label_manual ) )
    {
      XtUnmanageChild( Description_label_manual );
    }
    else if( XtIsManaged( Description_label_auto_storm ) )
    {
      XtUnmanageChild( Description_label_auto_storm );
    }
    XtSetSensitive( Data_scroll, False );
  }
  else if( mode_flag == HCI_PRF_MODE_AUTO_STORM ||
           mode_flag == HCI_PRF_MODE_AUTO_CELL )
  {
    XtVaSetValues( Manual_PRF_button, XmNset, False, NULL );
    XtVaSetValues( Elevation_PRF_button, XmNset, False, NULL );
    XtVaSetValues( Storm_PRF_button, XmNset, True, NULL );
    XtManageChild( Description_label_auto_storm );
    if( XtIsManaged( Description_label_manual ) )
    {
      XtUnmanageChild( Description_label_manual );
    }
    else if( XtIsManaged( Description_label_auto_elevation ) )
    {
      XtUnmanageChild( Description_label_auto_elevation );
    }
    XtSetSensitive( Data_scroll, True );
  }
  else
  {
    HCI_LE_error( "Unknown mode (%d)", PRF_Mode_status.selected_mode );
  }
}

/************************************************************************
  Description: Print PRF Mode status msg.
 ************************************************************************/

static void PRF_Mode_print_status_msg( Prf_status_t *p )
{
  int i = 0;

  HCI_LE_status( "PRF Mode Status Message:" );
  HCI_LE_status( "STATE: %1d (%s) ERR: %1d RADIUS: %d #STORMS: %d\n", p->state, Convert_PRF_mode_state_to_string( p->state ), p->error_code, p->radius, p->num_storms );
  for( i = 0; i < p->num_storms; i++ )
  {
    HCI_LE_status( "  STORM #%d  INDEX: %d:\n", i+1, i );
    HCI_LE_status( "  ID: %4s RNG: %6.2f AZ: %6.2f RNG PROJ: %6.2f AZ_PROJ: %6.2f\n", p->storm_data[i].storm_id, p->storm_data[i].storm_rng, p->storm_data[i].storm_azm, p->storm_data[i].storm_rng_proj, p->storm_data[i].storm_azm_proj );
    HCI_LE_status( "  VIL: %6.2f  Rmax: %6.2f Rmax HGT: %6.2f Storm HGT: %6.2f\n", p->storm_data[i].storm_vil, p->storm_data[i].storm_mx_refl, p->storm_data[i].storm_ht_mx_refl, p->storm_data[i].storm_ht_top );
  }
  HCI_LE_status("# TRACKED: %d\n",p->num_storms_tracked);
  for( i = 0; i < p->num_storms_tracked; i++ )
  {
    HCI_LE_status("  TRACKED INDEX #%d: %d\n", i+1, p->ids_storms_tracked[i] );
  }
}

/************************************************************************
  Description: Convert PRF Mode state to string equivalent.
 ************************************************************************/

static char *Convert_PRF_mode_state_to_string( int state )
{
  static char buf[32];

  if( state == HCI_PRF_MODE_MANUAL )
  {
    strcpy( buf, "Manual" );
  }
  else if( state == HCI_PRF_MODE_AUTO_ELEVATION )
  {
    strcpy( buf, "Auto-Elevation" );
  }
  else if( state == HCI_PRF_MODE_AUTO_STORM )
  {
    strcpy( buf, "Auto-Storm" );
  }
  else if( state == HCI_PRF_MODE_AUTO_CELL )
  {
    strcpy( buf, "Auto-Cell" );
  }
  else
  {
    strcpy( buf, "???" );
  }

  Print_debug( "Converted %d to %s", state, buf );

  return buf;
}

/************************************************************************
  Description: Create fake data for debugging.
 ************************************************************************/

static void Initstorms( Prf_status_t *p, int numstorms, int numtracked )
{
  int i = 0;

  Print_debug( "Creating fake data" );

  p->state = HCI_PRF_MODE_AUTO_STORM;
  p->error_code = 0;
  p->radius = 20;
  p->num_storms = numstorms;
  p->num_storms_tracked = numtracked;
  for( i = 0; i < numtracked; i++ )
  {
    p->ids_storms_tracked[i] = (i*2);
  }

  strcpy( p->storm_data[0].storm_id, "A0" );
  p->storm_data[0].storm_rng = 230.011948;
  p->storm_data[0].storm_azm = 180.881119;
  p->storm_data[0].storm_rng_proj = 181.53;
  p->storm_data[0].storm_azm_proj = 243.89;
  p->storm_data[0].storm_vil = 75.848831;
  p->storm_data[0].storm_mx_refl = 64.5;
  p->storm_data[0].storm_ht_mx_refl = 7.118118;
  p->storm_data[0].storm_ht_top = 13.721404;

  strcpy( p->storm_data[1].storm_id, "B0" );
  p->storm_data[1].storm_rng = 230.799530;
  p->storm_data[1].storm_azm = 135.221130;
  p->storm_data[1].storm_rng_proj = 181.665115;
  p->storm_data[1].storm_azm_proj = 201.639099;
  p->storm_data[1].storm_vil = 64.973129;
  p->storm_data[1].storm_mx_refl = 70.500000;
  p->storm_data[1].storm_ht_mx_refl = 6.910916;
  p->storm_data[1].storm_ht_top = 13.209550;

  strcpy( p->storm_data[2].storm_id, "B1" );
  p->storm_data[2].storm_rng = 230.324455;
  p->storm_data[2].storm_azm = 0.853119;
  p->storm_data[2].storm_rng_proj = 104.086700;
  p->storm_data[2].storm_azm_proj = 235.441727;
  p->storm_data[2].storm_vil = 33.680836;
  p->storm_data[2].storm_mx_refl = 56.000000;
  p->storm_data[2].storm_ht_mx_refl = 1.696639;
  p->storm_data[2].storm_ht_top = 7.029257;

  strcpy( p->storm_data[3].storm_id, "F0" );
  p->storm_data[3].storm_rng = 220.328201;
  p->storm_data[3].storm_azm = 100.901794;
  p->storm_data[3].storm_rng_proj = 128.135452;
  p->storm_data[3].storm_azm_proj = 250.534531;
  p->storm_data[3].storm_vil = 26.122915;
  p->storm_data[3].storm_mx_refl = 58.000000;
  p->storm_data[3].storm_ht_mx_refl = 2.278111;
  p->storm_data[3].storm_ht_top = 7.981780;

  strcpy( p->storm_data[4].storm_id, "M0" );
  p->storm_data[4].storm_rng = 218.467484;
  p->storm_data[4].storm_azm = 261.198242;
  p->storm_data[4].storm_rng_proj = 214.656174;
  p->storm_data[4].storm_azm_proj = 260.687439;
  p->storm_data[4].storm_vil = 24.409449;
  p->storm_data[4].storm_mx_refl = 51.500000;
  p->storm_data[4].storm_ht_mx_refl = 5.014254;
  p->storm_data[4].storm_ht_top = 8.837302;

  strcpy( p->storm_data[5].storm_id, "E0" );
  p->storm_data[5].storm_rng = 230.809448;
  p->storm_data[5].storm_azm = 90.858841;
  p->storm_data[5].storm_rng_proj = 235.618126;
  p->storm_data[5].storm_azm_proj = 91.834488;
  p->storm_data[5].storm_vil = 23.121548;
  p->storm_data[5].storm_mx_refl = 57.000000;
  p->storm_data[5].storm_ht_mx_refl = 1.595777;
  p->storm_data[5].storm_ht_top = 6.246698;

  strcpy( p->storm_data[6].storm_id, "L0" );
  p->storm_data[6].storm_rng = 230.941650;
  p->storm_data[6].storm_azm = 110.643799;
  p->storm_data[6].storm_rng_proj = 199.157501;
  p->storm_data[6].storm_azm_proj = 257.820129;
  p->storm_data[6].storm_vil = 19.529854;
  p->storm_data[6].storm_mx_refl = 51.000000;
  p->storm_data[6].storm_ht_mx_refl = 4.383981;
  p->storm_data[6].storm_ht_top = 7.947412;

  strcpy( p->storm_data[7].storm_id, "G0" );
  p->storm_data[7].storm_rng = 230.428726;
  p->storm_data[7].storm_azm = 35.021835;
  p->storm_data[7].storm_rng_proj = 143.602493;
  p->storm_data[7].storm_azm_proj = 195.844772;
  p->storm_data[7].storm_vil = 19.283661;
  p->storm_data[7].storm_mx_refl = 52.500000;
  p->storm_data[7].storm_ht_mx_refl = 2.583689;
  p->storm_data[7].storm_ht_top = 7.220561;

  strcpy( p->storm_data[8].storm_id, "A2" );
  p->storm_data[8].storm_rng = 214.006851;
  p->storm_data[8].storm_azm = 240.957977;
  p->storm_data[8].storm_rng_proj = 212.476242;
  p->storm_data[8].storm_azm_proj = 240.071350;
  p->storm_data[8].storm_vil = 15.206994;
  p->storm_data[8].storm_mx_refl = 48.500000;
  p->storm_data[8].storm_ht_mx_refl = 4.850846;
  p->storm_data[8].storm_ht_top = 8.586102;

  strcpy( p->storm_data[9].storm_id, "K0" );
  p->storm_data[9].storm_rng = 225.575958;
  p->storm_data[9].storm_azm = 254.363785;
  p->storm_data[9].storm_rng_proj = 224.602875;
  p->storm_data[9].storm_azm_proj = 253.155746;
  p->storm_data[9].storm_vil = 13.625057;
  p->storm_data[9].storm_mx_refl = 48.000000;
  p->storm_data[9].storm_ht_mx_refl = 3.961506;
  p->storm_data[9].storm_ht_top = 6.887368;
}

/************************************************************************
  Description: Print message specifically for debugging.
 ************************************************************************/

static void Print_debug( const char *format, ... )
{
  char buf[256];
  va_list arg_ptr;

  if( Debug_flag )
  {
    /* Extract print format */
    va_start( arg_ptr, format );
    vsprintf( buf, format, arg_ptr );
    va_end( arg_ptr );
    printf( "DEBUG: %s\n", buf );
  }
}

