/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/03 23:31:25 $
 * $Id: hci_basedata_tool.c,v 1.14 2011/03/03 23:31:25 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_basedata_tool.c					*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      to open a window for displaying base data and	*
 *		      change elevation/volume scans.			*
 *									*
 *************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_basedata_tool.h>
#include <hci_scan_info.h>

/* Local Constants. */

#define BASE_DATA_WIDTH 520      /* default width of window */
#define BASE_DATA_HEIGHT 490     /* default height of window */
#define ZOOM_FACTOR_MAX 64       /* maximum magnification factor */
#define ZOOM_FACTOR_MIN 1        /* minimum magnification factor */
#define RAW_MODE 0               /* interrogation mode */
#define ZOOM_MODE 1              /* zoom mode */
#define MAX_AZIMUTHS 720         /* max azimuths allowed in cut */
#define MAX_ELEVATION_SCANS 25   /* max cuts allowed in volume */
#define COLOR_BAR_AREA_WIDTH 60  /* width of color bar */
#define COLOR_BOX_WIDTH 10       /* width of color box */
#define TICK_MARK_LENGTH 5       /* length of data tick mark */
#define COLOR_BAR_BUFFER 5       /* pixel buffer between color bar and edge */
#define REFLECTIVITY_PRODUCT 19      /* reflectivity product for colors */
#define VELOCITY_PRODUCT 25          /* velocity product for colors */
#define SPECTRUM_WIDTH_PRODUCT 28    /* spectrum width product for colors */
#define DIFF_REFLECTIVITY_PRODUCT 19 /* reflectivity product for colors */
#define DIFF_PHASE_PRODUCT 19        /* reflectivity product for colors */
#define DIFF_CORRELATION_PRODUCT 19  /* reflectivity product for colors */
#define MAX_NUM_FIELDS 6        /* Max number of moments/fields */
#define MAX_NUM_BASEDATA_LBS 6  /* Max number of basedata LBs */
#define	PBD_LB 54
#define	SR_VELDEAL_LB 76
#define	AZIMUTH_RECOMBINED_LB 276
#define	VELDEAL_LB 55
#define	RANGE_RECOMBINED_LB 53
#define	DPPREP_LB 305

enum { REF_MIN, REF_MAX, VEL_MIN, VEL_MAX, SWP_MIN, SWP_MAX,
       ZDR_MIN, ZDR_MAX, PHI_MIN, PHI_MAX, RHO_MIN, RHO_MAX };

/* Global widget variables. */

Widget Zoom_button = (Widget) NULL;
Widget Raw_button = (Widget) NULL;
Widget Filter_off_button = (Widget) NULL;
Widget Filter_on_button = (Widget) NULL;
Widget Grid_off_button = (Widget) NULL;
Widget Grid_on_button = (Widget) NULL;
Widget Vcp_label = (Widget) NULL;
Widget Apply_button = (Widget) NULL;
Widget Azran_label = (Widget) NULL;
Widget Height_label = (Widget) NULL;
Widget Value_label = (Widget) NULL;
Widget  Azran_data = (Widget) NULL;
Widget Height_data = (Widget) NULL;
Widget Value_data = (Widget) NULL;
Widget Left_button = (Widget) NULL;
Widget Middle_button = (Widget) NULL;
Widget Right_button = (Widget) NULL;
Widget Button[MAX_ELEVATION_SCANS];

Widget Data_range_dialog = (Widget) NULL;
Widget Reflectivity_min_scale = (Widget) NULL;
Widget Reflectivity_max_scale = (Widget) NULL;
Widget Velocity_min_scale = (Widget) NULL;
Widget Velocity_max_scale = (Widget) NULL;
Widget Spectrum_width_min_scale = (Widget) NULL;
Widget Spectrum_width_max_scale = (Widget) NULL;
Widget Diff_reflectivity_min_scale = (Widget) NULL;
Widget Diff_reflectivity_max_scale = (Widget) NULL;
Widget Diff_phase_min_scale = (Widget) NULL;
Widget Diff_phase_max_scale = (Widget) NULL;
Widget Diff_correlation_min_scale = (Widget) NULL;
Widget Diff_correlation_max_scale = (Widget) NULL;

int Elevation_number = 0;    /* Current elevation cut index number */
int Display_mode = RAW_MODE; /* cursor mode inside display area */
float Reflectivity_min = REFLECTIVITY_MIN;     /* Reflectivity min value */
float Reflectivity_max = REFLECTIVITY_MAX;     /* Reflectivity max value */
float Velocity_min = VELOCITY_MIN;             /* Velocity min value */
float Velocity_max = VELOCITY_MAX;             /* Velocity max value */
float Spectrum_width_min = SPECTRUM_WIDTH_MIN;       /* Spw min value */
float Spectrum_width_max = SPECTRUM_WIDTH_MAX;       /* Spw max value */
float Diff_reflectivity_min = DIFF_REFLECTIVITY_MIN  /* Zdr min value */; 
float Diff_reflectivity_max = DIFF_REFLECTIVITY_MAX; /* Zdr max value */
float Diff_phase_min = DIFF_PHASE_MIN;               /* Phi min value */
float Diff_phase_max = DIFF_PHASE_MAX;               /* Phi max value */
float Diff_correlation_min = DIFF_CORRELATION_MIN;   /* Rho min value */ 
float Diff_correlation_max = DIFF_CORRELATION_MAX;   /* Rho max value */
int Grid_overlay_flag = 1; /* Display polar grid flag */
int Field_changed = 0;     /* Moment changed by operator flag */
int Filter_mode = 0;       /* Data range filter is active flag */
int Basedata_LB_id = VELDEAL_LB; /* LB id of base data source */
int Basedata_static_index = 3; /* Table index for VELDEAL_LB */
float Velocity_scale = 0.5; /* Velocity scale (resolution) */
int Prev_vel_resolution = DOPPLER_RESOLUTION_LOW;

/* Translation table for base data window.  We need a mechanism
   to capture mouse events for interrogation and zooming. */

String Translations =
 "<Btn1Down>:   hci_basedata_draw(down1)   ManagerGadgetArm() \n\
  <Btn1Up>:     hci_basedata_draw(up1)     ManagerGadgetActivate() \n\
  <Btn1Motion>: hci_basedata_draw(motion1) ManagerGadgetButtonMotion() \n\
  <Btn2Down>:   hci_basedata_draw(down2)   ManagerGadgetArm() \n\
  <Btn3Down>:   hci_basedata_draw(down3)   ManagerGadgetArm()";

/* Global variables */

Widget Top_widget   ; /* Top level window */
Display *BD_display; /* Display info */
Widget BD_label;     /* Elevation/Time label at top of window */
Dimension BD_width;  /* Width of drawable region */
Dimension BD_height; /* Height of drawable region */
Dimension BD_depth;  /* Depth (bits) of display */
Widget BD_buttons_rowcol;    /* Elevation buttons manager */
Widget BD_draw_widget; /* Drawing area widget for base data */
Window BD_window;      /* Visible Drawable */
Pixmap BD_pixmap;      /* Invisible Drawable */
GC BD_gc;              /* Graphics context */
Pixel BD_background_color = 0; /* Color of drawable background */
Pixel BD_foreground_color = 0; /* Color of drawable foreground */

char BD_current_field[128]; /* Name of currently displayed moment */
int BD_center_pixel; /* Center pixel (X) of drawable region */
int BD_center_scanl; /* Center scanline (Y) of drawable region */
float BD_scale_x;    /* Pixel/km in X direction */
float BD_scale_y;    /* Pixel/km in Y direction */
float BD_value_max;  /* Maximum value to display for active moment */
float BD_value_min;  /* Minimum value to display for active moment */
float BD_display_range = MAX_RADIAL_RANGE; /* Maximum range */
int BD_display_scan = -1; /* Scan cut number */
float BD_x_offset; /* X offset of radar from region center */
float BD_y_offset; /* Y offset of radar from region center */
int BD_display_mode = DISPLAY_MODE_DYNAMIC; /* Display mode */
int BD_data_type = REFLECTIVITY;            /* Active data type */
int BD_doppler_flag = 0;    /* Is active data type Doppler? */
float BD_current_elevation; /* Active elevation angle */
int BD_zoom_factor = 1;     /* Current zoom factor (1 to 64) */
char *BD_data = (char *) NULL; /* Pointer to radial data */
int BD_data_len = 0;           /* Size of BD_data array */
int BD_color[PRODUCT_COLORS]; /* Color lookup table */
int BD_elevations = 0;         /* Number of elevation cuts in scan */
int BD_azi_ptr[MAX_AZIMUTHS]; /* Azimuth/LB_msg_id lookup table */

XtIntervalId Timer; /* Timer ID */
unsigned long Timer_interval = 500; /* Timer proc update interval (ms). */

int New_volume  = 0; /* New volume flag */
int Current_vcp = 0; /* Active VCP number */
int Update_flag = 0; /* Data update flag */
int Scan_info_update_flag = 0; /* Data update flag */

char *field_names[ MAX_NUM_FIELDS ] = { "REF", "VEL", "SPW",
                                        "ZDR", "PHI", "RHO" };
char *basedata_LB_names[ MAX_NUM_BASEDATA_LBS ] = { "PBD", "SR VELDEAL",
                              "AZ REC", "VELDEAL", "RECOMB", "DPPREP" };
int basedata_LB_ids[ MAX_NUM_BASEDATA_LBS ] = { PBD_LB, SR_VELDEAL_LB,
   AZIMUTH_RECOMBINED_LB, VELDEAL_LB, RANGE_RECOMBINED_LB, DPPREP_LB };
int basedata_static_allowed[ MAX_NUM_FIELDS ] = { 0, 0, 0, 0, 0, 0 };
typedef struct replay_info {

   int replay_LBID;
   int acct_LBID;

} replay_info_t;

replay_info_t basedata_replay_info[ MAX_NUM_BASEDATA_LBS ] = 
   { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };

void hci_display_selected_scan( Widget w, XtPointer cl, XtPointer ca );
void hci_basedata_expose_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_basedata_resize_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_basedata_close_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_data_range_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_basedata_filter_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_basedata_grid_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_basedata_draw( Widget w, XEvent *evt, String *args, int *num_args );
void hci_basedata_display_mode( Widget w, XtPointer cl, XtPointer ca );
void hci_change_basedata_LB_callback( Widget w, XtPointer cl, XtPointer ca );
void hci_change_field_callback( Widget w, XtPointer cl, XtPointer ca );

void hci_basedata( int argc, char **argv );
void hci_basedata_draw_text_message( char *msg );
float hci_find_azimuth( int pixel1, int scanl1, int pixel2, int scanl2 );

void hci_basedata_set_window_header_dynamic();
void hci_basedata_set_window_header_static( int );
void hci_basedata_clear_buttons();
void hci_basedata_set_button( Widget button_widget );
void hci_basedata_clear_pixmap();
void hci_basedata_clear_window();
void hci_basedata_clear_screen();

int hci_determine_azimuth_index( float azimuth );
int hci_initialize_basedata_scan_table();
void hci_basedata_elevation_buttons();
void hci_basedata_color_bar();
void hci_basedata_overlay_grid();
void hci_basedata_ref();
void hci_basedata_vel();
void hci_basedata_spw();
void hci_basedata_zdr();
void hci_basedata_phi();
void hci_basedata_rho();

void check_velocity_resolution();
void timer_proc();
void hci_scan_info_update( en_t evtcd, void *info, size_t msglen );

/************************************************************************
 *	Description: This is the main function for the HCI Base Data	*
 *		     Display task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments			*
 *	Return: exit code						*
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget form;
  Widget frame;
  Widget options_frame;
  Widget options_form;
  Widget control_form;
  Widget button;
  Widget label;
  Widget close_rowcol;
  Widget filter_rowcol;
  Widget grid_rowcol;
  Widget mode_rowcol;
  Widget basedata_LB_rowcol;
  Widget basedata_LB_combo_box;
  Widget field_rowcol;
  Widget field_combo_box;
  Widget filter_list;
  Widget grid_list;
  Widget mode_list;
  Widget properties_form;
  Widget properties_rowcol;
  Widget properties_label_rowcol;
  Widget properties_value_rowcol;
  Widget button_label_rowcol;
  Widget button_value_rowcol;
  int i;
  XGCValues gcv;
  XtActionsRec actions;
  Arg arg[10];
  int n;
  int status;
  int replay_LBID;
  int acct_LBID;
  Pd_attr_entry *entry = NULL;
  XmString str;

  /*  Initialize HCI. */

  HCI_init( argc, argv, HCI_BASEDATA_TOOL );

  /* Set write permission so window doesn't close unexpectedly. */

  ORPGDA_write_permission( ORPGDAT_GSM_DATA );

  /* Initialize scan_info data. */

  status = EN_register( ORPGEVT_SCAN_INFO, (void *) hci_scan_info_update );
  HCI_LE_status("EN_register ORPGEVT_SCAN_INFO: %d", status );


  /* Check how many of the input LB data types are warehoused.  This will
     determine how may can use the static display. */

  for( i = 0; i < MAX_NUM_BASEDATA_LBS; i++ ){

     basedata_static_allowed[i] = 0;
     status = ORPGPAT_get_warehoused( basedata_LB_ids[i] );
     if( status > 0 ){

        entry = ORPGPAT_get_tbl_entry( basedata_LB_ids[i] );
        if( entry != NULL ){

           HCI_LE_status("Basedata LB ID: %d is warehoused.  Allow Static.\n",
                         basedata_LB_ids[i] );
           basedata_static_allowed[i] = 1;
           replay_LBID = entry->warehouse_id;
           acct_LBID = entry->warehouse_acct_id;

           HCI_LE_status("-->Warehouse ID: %d, Accounting ID: %d\n",
                         replay_LBID, acct_LBID );
           basedata_replay_info[i].replay_LBID = replay_LBID;
           basedata_replay_info[i].acct_LBID = acct_LBID;

        }
     }
  }

  /* Make "Reflectivity" the default field to display. */

  strcpy( BD_current_field,"  Reflectivity" );

  Top_widget = HCI_get_top_widget();
  BD_display = XtDisplay( Top_widget );
  BD_depth = XDefaultDepth( BD_display, DefaultScreen( BD_display ) );

  /* Initialize the product color database. */

  hci_basedata_tool_get_product_colors( REFLECTIVITY_PRODUCT, BD_color );

  /* Set background color. */

  BD_background_color = hci_get_read_color( PRODUCT_BACKGROUND_COLOR );
  BD_foreground_color = hci_get_read_color( PRODUCT_FOREGROUND_COLOR );

  /* Set various window properties. */

  XtVaSetValues( Top_widget,
                 XmNminWidth, 250,
                 XmNminHeight, 250,
                 XmNforeground, hci_get_read_color(BLACK),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  /* Make the initial size of the base data display window something
     reasonable. The user can resize it as needed. Define a minimum
     size to be able to hold all of the widgets.
     NOTE:  This is something which should eventually get put into
     a resource file so it can be configured at startup. */

  BD_width  = BASE_DATA_WIDTH; 
  BD_height = BASE_DATA_HEIGHT;

  /* Create the form widget which will manage the drawing_area and
     row_column widgets. */

  form = XtVaCreateWidget( "form", xmFormWidgetClass, Top_widget, NULL );

  BD_label = XtVaCreateManagedWidget( "BD Label",
                 xmLabelWidgetClass, form,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
/* XmNrecomputeSize to False prevents widget from resizing to original size.
   Otherwise, widget resizes to original size every new elevation scan. */
                 XmNrecomputeSize, False,
                 NULL );

  /* Use another form widget to manage all of the control buttons
     on the left side of the basedata display window. */

  control_form = XtVaCreateWidget( "control_form",
                        xmFormWidgetClass, form,
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, BD_label,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                        NULL );

  frame = XtVaCreateManagedWidget( "close_frame",
                        xmFrameWidgetClass, control_form,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        NULL );

  close_rowcol = XtVaCreateWidget( "close_rowcol",
                        xmRowColumnWidgetClass, frame,
                        XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                        XmNpacking, XmPACK_COLUMN,
                        XmNalignment, XmALIGNMENT_CENTER,
                        XmNorientation, XmHORIZONTAL,
                        XmNnumColumns, 1,
                        NULL );

  button = XtVaCreateManagedWidget( "Close",
                        xmPushButtonWidgetClass, close_rowcol,
                        XmNforeground, hci_get_read_color(BUTTON_FOREGROUND),
                        XmNbackground, hci_get_read_color(BUTTON_BACKGROUND),
                        XmNfontList, hci_get_fontlist(LIST),
                        NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_basedata_close_callback, NULL );

  XtManageChild( close_rowcol );

  /* Display a label at the top of the control form which contains
     the VCP number of the current volume scan. */

  Vcp_label = XtVaCreateManagedWidget( "VCP   ",
                        xmLabelWidgetClass, control_form,
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, frame,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                        XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                        XmNfontList, hci_get_fontlist(LARGE),
                        NULL );

  label = XtVaCreateManagedWidget( "space",
                 xmLabelWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, Vcp_label,
                 XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  /* Create a "Scan" button which will set the display mode to
     dynamic.  This displays data as it is received.  By default,
     the display mode is dynamic. */

  Button[0] = XtVaCreateManagedWidget( "Scan Mode",
                       xmPushButtonWidgetClass, control_form,
                       XmNforeground, hci_get_read_color(BUTTON_BACKGROUND),
                       XmNbackground, hci_get_read_color(BUTTON_FOREGROUND),
                       XmNtopAttachment, XmATTACH_WIDGET,
                       XmNtopWidget, label,
                       XmNleftAttachment, XmATTACH_FORM,
                       XmNrightAttachment, XmATTACH_FORM,
                       XmNfontList, hci_get_fontlist(LIST),
                       XmNuserData, (XtPointer) -1,
                       NULL );

  label = XtVaCreateManagedWidget( "space",
                 xmLabelWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, Button[0],
                 XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  XtAddCallback( Button[0],
                 XmNactivateCallback, hci_display_selected_scan,
                 (XtPointer) -1 );

  frame = XtVaCreateManagedWidget( "elevation_frame",
                  xmFrameWidgetClass, control_form,
                  XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                  XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, label,
                  NULL );

  label = XtVaCreateManagedWidget ("Static Mode",
                xmLabelWidgetClass,     frame,
                XmNchildType,           XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment,    XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment,      XmALIGNMENT_CENTER,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

  /* Create the row_column widget for the elevation scan buttons to 
     be placed along the left side of the form.  The buttons are
     displayed with elevation angle as labels in two columns. */

  BD_buttons_rowcol = XtVaCreateWidget("basedata_rowcol",
                        xmRowColumnWidgetClass, frame,
                        XmNorientation, XmVERTICAL,
                        XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                        XmNpacking, XmPACK_COLUMN,
                        XmNalignment, XmALIGNMENT_CENTER,
                        XmNnumColumns, 3,
                        NULL );

  /* Create the pushbuttons for the elevation scan selections.  When
     the VCP is changed, this list is updated accordingly. */

  for( i=1; i<MAX_ELEVATION_SCANS; i++ )
  {
    Button[i] = XtVaCreateManagedWidget("    ",
                   xmPushButtonWidgetClass, BD_buttons_rowcol,
                   XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
                   XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                   XmNfontList, hci_get_fontlist(LIST),
                   XmNuserData, (XtPointer) (i),
                   XmNalignment, XmALIGNMENT_CENTER,
                   NULL );

    XtAddCallback( Button[i],
                   XmNactivateCallback, hci_display_selected_scan,
                   (XtPointer) (i) );
  }

  /* Use the function hci_basedata_elevation_buttons () to define
     the VCP and elevation angles and create the elevation angle
     buttons.  This function will be called whenever the VCP is
     changed */

  hci_set_scan_info_flag(1);

  Current_vcp = hci_get_scan_vcp_number();

  hci_basedata_elevation_buttons();

  XtManageChild( BD_buttons_rowcol );

  label = XtVaCreateManagedWidget( "space",
                 xmLabelWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, BD_buttons_rowcol,
                 XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  button = XtVaCreateManagedWidget("Data Range",
                 xmPushButtonWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, label,
                 XmNforeground, hci_get_read_color(BUTTON_FOREGROUND),
                 XmNbackground, hci_get_read_color(BUTTON_BACKGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_data_range_callback,
                 (XtPointer) NULL );

  /* Define toggle button properties. */

  n = 0;
  XtSetArg(arg[n], XmNforeground, hci_get_read_color(TEXT_FOREGROUND)); n++;
  XtSetArg(arg[n], XmNbackground, hci_get_read_color(BACKGROUND_COLOR1)); n++;
  XtSetArg(arg[n], XmNmarginHeight, 1); n++;
  XtSetArg(arg[n], XmNmarginWidth, 1); n++;
  XtSetArg(arg[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg(arg[n], XmNpacking, XmPACK_TIGHT); n++;

  /* Create the row_column widget which will hold the data filter
     selection buttons. */ 

  filter_rowcol = XtVaCreateWidget( "filter_rowcol",
                 xmRowColumnWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, button,
                 XmNorientation, XmHORIZONTAL,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNspacing, 1,
                 NULL );

  /* Create a Label for each set of buttons and the buttons themselves. */

  XtVaCreateManagedWidget( "Filter: ",
                 xmLabelWidgetClass, filter_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  filter_list = XmCreateRadioBox( filter_rowcol, "filter_list", arg, n );

  Filter_on_button = XtVaCreateManagedWidget( "On",
                 xmToggleButtonWidgetClass, filter_list,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNselectColor, hci_get_read_color(WHITE),
                 XmNset, True,
                 NULL );

  XtAddCallback( Filter_on_button,
                 XmNvalueChangedCallback, hci_basedata_filter_callback,
                 (XtPointer) 0 );

  Filter_off_button = XtVaCreateManagedWidget( "Off",
                 xmToggleButtonWidgetClass, filter_list,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNselectColor, hci_get_read_color(WHITE),
                 NULL );

  XtAddCallback( Filter_off_button,
                 XmNvalueChangedCallback, hci_basedata_filter_callback,
                 (XtPointer) 1 );

  XtManageChild( filter_list );
  XtManageChild( filter_rowcol );

  label = XtVaCreateManagedWidget( "space",
                 xmLabelWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, filter_rowcol,
                 XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  options_frame = XtVaCreateManagedWidget( "options_frame",
                 xmFrameWidgetClass, control_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, label,
                 NULL );

  options_form = XtVaCreateWidget( "options_form",
                 xmFormWidgetClass, options_frame,
                 XmNbackground, hci_get_read_color(BLACK),
                 XmNverticalSpacing, 1,
                 NULL );

  /* Create the row_column widget which will hold the field
     selection buttons. */ 

  basedata_LB_rowcol = XtVaCreateWidget("basedata_LB_rowcol",
                 xmRowColumnWidgetClass, options_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, filter_rowcol,
                 XmNorientation, XmHORIZONTAL,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNspacing, 1,
                 NULL );

  /* Create a Label for each set of buttons and the buttons themselves. */

  XtVaCreateManagedWidget( "Data LB: ",
                 xmLabelWidgetClass, basedata_LB_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  basedata_LB_combo_box = XtVaCreateManagedWidget( "basedata_LB_combo_box",
                 xmComboBoxWidgetClass, basedata_LB_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNcomboBoxType, XmDROP_DOWN_LIST,
                 XmNcolumns, 10,
                 XmNvisibleItemCount, MAX_NUM_BASEDATA_LBS,
                 NULL );

  XtAddCallback( basedata_LB_combo_box,
           XmNselectionCallback, hci_change_basedata_LB_callback, NULL );

  for( i = MAX_NUM_BASEDATA_LBS - 1; i >= 0; i-- )
  {
    str = XmStringCreateLocalized( basedata_LB_names[ i ] );
    XmComboBoxAddItem( basedata_LB_combo_box, str, 1, True );
    if( Basedata_LB_id == basedata_LB_ids[i] )
    {
      XmComboBoxSetItem( basedata_LB_combo_box, str );
    }
    XmStringFree( str );
  }

  XtManageChild( basedata_LB_rowcol );

  /* Create the row_column widget which will hold the field
     selection buttons. */ 

  field_rowcol = XtVaCreateWidget( "field_rowcol",
                 xmRowColumnWidgetClass, options_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, basedata_LB_rowcol,
                 XmNorientation, XmHORIZONTAL,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNspacing, 1,
                 NULL );

  /* Create a Label for each set of buttons and the buttons themselves. */

  XtVaCreateManagedWidget( "Moment: ",
                 xmLabelWidgetClass, field_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  field_combo_box = XtVaCreateManagedWidget( "field_combo_box",
                 xmComboBoxWidgetClass, field_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNcomboBoxType, XmDROP_DOWN_LIST,
                 XmNcolumns, 3,
                 XmNvisibleItemCount, MAX_NUM_FIELDS,
                 NULL );

  XtAddCallback( field_combo_box,
                 XmNselectionCallback, hci_change_field_callback, NULL );

  for( i = MAX_NUM_FIELDS - 1; i >= 0; i-- )
  {
    str = XmStringCreateLocalized( field_names[ i ] );
    XmComboBoxAddItem( field_combo_box, str, 1, True );
    XmComboBoxSetItem( field_combo_box, str );
    XmStringFree( str );
  }

  XtManageChild( field_rowcol );

  /* Create the row_column widget which will hold the grid overlay
     selection buttons. */ 

  grid_rowcol = XtVaCreateWidget( "grid_rowcol",
                 xmRowColumnWidgetClass, options_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, field_rowcol,
                 XmNorientation, XmHORIZONTAL,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNspacing, 1,
                 NULL );

  /* Create a Label for each set of buttons and the buttons themselves. */

  XtVaCreateManagedWidget( "Grid:   ",
                 xmLabelWidgetClass, grid_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  grid_list = XmCreateRadioBox( grid_rowcol, "grid_list", arg, n );

  Grid_on_button = XtVaCreateManagedWidget( "On ",
                 xmToggleButtonWidgetClass, grid_list,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNselectColor, hci_get_read_color(WHITE),
                 XmNset, True,
                 NULL );

  XtAddCallback( Grid_on_button,
                 XmNvalueChangedCallback, hci_basedata_grid_callback,
                 (XtPointer) 1 );

  Grid_off_button = XtVaCreateManagedWidget( "Off",
                 xmToggleButtonWidgetClass, grid_list,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNselectColor, hci_get_read_color(WHITE),
                 NULL );

  XtAddCallback( Grid_off_button,
                 XmNvalueChangedCallback, hci_basedata_grid_callback,
                 (XtPointer) 0 );

  XtManageChild( grid_list );
  XtManageChild( grid_rowcol );

  /* Create the row_column widget which will hold the mode
     selection buttons. */ 

  mode_rowcol = XtVaCreateWidget( "mode_rowcol",
                 xmRowColumnWidgetClass, options_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, grid_rowcol,
                 XmNorientation, XmHORIZONTAL,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNspacing, 1,
                 NULL );

  XtVaCreateManagedWidget( "Mode:   ",
                 xmLabelWidgetClass, mode_rowcol,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  mode_list = XmCreateRadioBox( mode_rowcol, "mode_list", arg, n );

  Raw_button = XtVaCreateManagedWidget( "Raw",
                 xmToggleButtonWidgetClass, mode_list,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNselectColor, hci_get_read_color(WHITE),
                 XmNset, True,
                 NULL );

  XtAddCallback( Raw_button,
                 XmNvalueChangedCallback, hci_basedata_display_mode,
                 (XtPointer) RAW_MODE);

  Zoom_button = XtVaCreateManagedWidget( "Zoom",
                 xmToggleButtonWidgetClass, mode_list,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNfontList, hci_get_fontlist(LIST),
                 XmNselectColor, hci_get_read_color(WHITE),
                 NULL );

  XtAddCallback( Zoom_button,
                 XmNvalueChangedCallback, hci_basedata_display_mode,
                 (XtPointer) ZOOM_MODE);

  XtManageChild( mode_list );
  XtManageChild( mode_rowcol );

  XtManageChild( options_form );
  XtManageChild( control_form );

  /* Create a widget along the bottom of the window to left of the
     control buttons which will contain various data attributes. */

  properties_form = XtVaCreateWidget("properties_form",
                 xmFormWidgetClass, form,
                 XmNleftAttachment, XmATTACH_WIDGET,
                 XmNleftWidget, control_form,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_FORM,
                 NULL );

  properties_rowcol = XtVaCreateManagedWidget( "properties_rowcol",
                 xmRowColumnWidgetClass, properties_form,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNorientation, XmHORIZONTAL,
                 XmNpacking, XmPACK_TIGHT,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  button_label_rowcol = XtVaCreateManagedWidget ("button_label_rowcol",
                 xmRowColumnWidgetClass, properties_rowcol,
                 XmNorientation, XmVERTICAL,
                 XmNpacking, XmPACK_TIGHT,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  XtVaCreateManagedWidget( "Left Button: ",
                 xmLabelWidgetClass, button_label_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  XtVaCreateManagedWidget( "Middle Button: ",
                 xmLabelWidgetClass, button_label_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  XtVaCreateManagedWidget( "Right Button: ",
                 xmLabelWidgetClass, button_label_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  button_value_rowcol = XtVaCreateManagedWidget ("button_value_rowcol",
                 xmRowColumnWidgetClass, properties_rowcol,
                 XmNorientation, XmVERTICAL,
                 XmNpacking, XmPACK_TIGHT,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  Left_button = XtVaCreateManagedWidget( "Interrogate      ",
                 xmLabelWidgetClass, button_value_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  Middle_button = XtVaCreateManagedWidget( "N/A",
                 xmLabelWidgetClass, button_value_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  Right_button = XtVaCreateManagedWidget("N/A",
                 xmLabelWidgetClass, button_value_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );
 
  properties_label_rowcol = XtVaCreateManagedWidget ("properties_label_rowcol",
                 xmRowColumnWidgetClass, properties_rowcol,
                 XmNorientation, XmVERTICAL,
                 XmNpacking, XmPACK_TIGHT,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  Azran_label = XtVaCreateManagedWidget( "Azran: ",
                 xmLabelWidgetClass, properties_label_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  Height_label = XtVaCreateManagedWidget( "Height: ",
                 xmLabelWidgetClass, properties_label_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  Value_label = XtVaCreateManagedWidget( "Value: ",
                 xmLabelWidgetClass, properties_label_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  properties_value_rowcol = XtVaCreateManagedWidget ("properties_value_rowcol",
                 xmRowColumnWidgetClass, properties_rowcol,
                 XmNorientation, XmVERTICAL,
                 XmNpacking, XmPACK_TIGHT,
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 NULL );

  Azran_data = XtVaCreateManagedWidget( "               ",
                 xmLabelWidgetClass, properties_value_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  Height_data = XtVaCreateManagedWidget( "               ",
                 xmLabelWidgetClass, properties_value_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );

  Value_data = XtVaCreateManagedWidget( "       ",
                 xmLabelWidgetClass, properties_value_rowcol,
                 XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
                 XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
                 XmNfontList, hci_get_fontlist(LIST),
                 NULL );
 
  XtManageChild( properties_form );

  /* Create the drawing_area widget which will be used to display
     base level radar data.  It will occupy the upper right portion
     of the form. */

  actions.string = "hci_basedata_draw";
  actions.proc = (XtActionProc) hci_basedata_draw;
  XtAppAddActions( HCI_get_appcontext(), &actions, 1 );

  BD_draw_widget = XtVaCreateWidget( "basedata_drawing_area",
                 xmDrawingAreaWidgetClass, form,
                 XmNwidth, BD_width,
                 XmNheight, BD_height,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, BD_label,
                 XmNleftAttachment, XmATTACH_WIDGET,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_WIDGET,
                 XmNleftWidget, BD_buttons_rowcol,
                 XmNbottomWidget, properties_form,
                 XmNtranslations, XtParseTranslationTable(Translations),
                 NULL );

  /* Add an expose callback for the drawing_area in order to allow
     holes to be filled in the display when  other windows are moved
     across it. */

  XtAddCallback( BD_draw_widget,
                 XmNexposeCallback, hci_basedata_expose_callback, NULL );

  /* Permit the user to resize the base data display window. */

  XtAddCallback( BD_draw_widget,
                 XmNresizeCallback, hci_basedata_resize_callback, NULL );

  XtManageChild( BD_draw_widget );
  XtManageChild( form );

  XtRealizeWidget( Top_widget );

  /* Define the various window variables to be used as arguments in
     the various Xlib and Xt calls. */

  BD_window = XtWindow( BD_draw_widget );
  BD_pixmap = XCreatePixmap( BD_display, BD_window,
                             BD_width, BD_height, BD_depth );

  /* Define the Graphics Context to be used in drawing data */

  gcv.foreground = BD_foreground_color;
  gcv.background = BD_background_color;
  gcv.graphics_exposures = FALSE;

  BD_gc = XCreateGC( BD_display, BD_window,
      GCBackground | GCForeground | GCGraphicsExposures, &gcv );

  XSetFont( BD_display, BD_gc, hci_get_font(LIST) );

  XtVaSetValues( BD_draw_widget, XmNbackground, BD_background_color, NULL );

  /* Clear the data display portion of the window by filling it with
     the background color. */

  XSetForeground( BD_display, BD_gc, BD_background_color );

  XFillRectangle( BD_display, BD_pixmap, BD_gc, 0, 0, BD_width, BD_width );

  /* Define the data display window properties. */

  BD_center_pixel = (BD_width-COLOR_BAR_AREA_WIDTH)/2;
  BD_center_scanl = BD_height/2;
  BD_scale_x = BD_zoom_factor * (BD_width-COLOR_BAR_AREA_WIDTH)/
                                (2.0*BD_display_range);
  BD_scale_y = -BD_scale_x;
  BD_value_max = REFLECTIVITY_MAX;
  BD_value_min = REFLECTIVITY_MIN;

  /* Initialize the basedata pointer table. */

  if( hci_initialize_basedata_scan_table() < 0 )
  {
    hci_basedata_draw_text_message( "Unable To Initialize Basedata LB" );
  }

  XtRealizeWidget( Top_widget );

  hci_basedata_resize_callback( (Widget) NULL,
               (XtPointer) NULL, (XtPointer) NULL );

  HCI_start( timer_proc, HCI_HALF_SECOND, RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function handles the base window exposure	*
 *		     callback by setting the clip window to the full	*
 *		     window and copying the base data pixmap to the	*
 *		     window.						*
 *									*
 *	Input:  w - top level widget ID					*
 *		cl - unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_expose_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XRectangle clip_rectangle;

  /* Set the clip region to the full window, update the window by
     copying the base data pixmap to it to fill in any holes left
     by any previously overlain window and the restoring the clip
     region to the base data display region only. */

  clip_rectangle.x       = 0;
  clip_rectangle.y       = 0;
  clip_rectangle.width   = BD_width;
  clip_rectangle.height  = BD_height;

  XSetClipRectangles( BD_display, BD_gc, 0, 0,
                      &clip_rectangle, 1, Unsorted );

  XCopyArea( BD_display, BD_pixmap, BD_window, BD_gc,
             0, 0, BD_width, BD_height, 0, 0 );

  clip_rectangle.x       = 0;
  clip_rectangle.y       = 0;
  clip_rectangle.width   = BD_width-COLOR_BAR_AREA_WIDTH;
  clip_rectangle.height  = BD_height;

  XSetClipRectangles( BD_display, BD_gc, 0, 0,
                      &clip_rectangle, 1, Unsorted );
}

/************************************************************************
 *	Description: This function handles the base window resize	*
 *		     callback by redrawing all base data window		*
 *		     components.					*
 *									*
 *	Input:  w - top level widget ID					*
 *		cl - unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_resize_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XRectangle clip_rectangle;

  /* Get the new size of the base data window. */

  if( (BD_draw_widget == (Widget) NULL) ||
      (BD_display == (Display *) NULL)  ||
      (BD_window == (Window) NULL)      ||
      (BD_pixmap == (Pixmap) NULL)      ||
      (BD_gc == (GC) NULL) )
  {
    return;
  }

  XtVaGetValues( BD_draw_widget,
                 XmNwidth, &BD_width,
                 XmNheight, &BD_height,
                 NULL );

  /* Destroy the old pixmap since the size has changed and create
     create a new one. */

  if( BD_pixmap != (Window) NULL )
  {
    XFreePixmap( BD_display, BD_pixmap );
  }

  BD_pixmap = XCreatePixmap( BD_display, BD_window,
                             BD_width, BD_height, BD_depth );

  /* Recompute the scale factors (in pixels/km) and a new window
     center pixel/scanline coordinate. */

  BD_scale_x = BD_zoom_factor * (BD_width-COLOR_BAR_AREA_WIDTH)/
                                (2*BD_display_range);
  BD_scale_y = -BD_scale_x;
  BD_center_pixel = (BD_width-COLOR_BAR_AREA_WIDTH) / 2;
  BD_center_scanl = BD_height / 2;

  /* Set the clip region to the entire window so we can create a
     color bar along the right size. */

  clip_rectangle.x       = 0;
  clip_rectangle.y       = 0;
  clip_rectangle.width   = BD_width;
  clip_rectangle.height  = BD_height;

  XSetClipRectangles( BD_display, BD_gc, 0, 0,
                      &clip_rectangle, 1, Unsorted );

  XSetForeground( BD_display, BD_gc, BD_background_color );

  XFillRectangle( BD_display, BD_pixmap, BD_gc,
                  0, 0, BD_width, BD_height );

  hci_basedata_color_bar();

  /* Restore the clip window to the data display area only. */

  clip_rectangle.x       = 0;
  clip_rectangle.y       = 0;
  clip_rectangle.width   = BD_width-COLOR_BAR_AREA_WIDTH;
  clip_rectangle.height  = BD_height;

  XSetClipRectangles( BD_display, BD_gc, 0, 0,
                      &clip_rectangle, 1, Unsorted );

  /* If we are looking at a previously read scan then lets display it. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                         (XtPointer) BD_display_scan, (XtPointer) NULL );
    hci_basedata_set_window_header_static(-1);
  }
  else
  {
    hci_basedata_set_window_header_dynamic();
  }

  hci_basedata_overlay_grid();

  /* Call the expose callback to write the base data pixmap to the
     base data window. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function draws a color bar along the right	*
 *		     side of the base data window.			*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_color_bar()
{
  int i;
  int scanl;
  char text[16];
  float avg_value = 0.0; /* Average value (max-min)/2 */
  int num_colors = PRODUCT_COLORS-1; /* No "No Data" color in bar */
  int font_off = 0; /* Offset of pixels to center text over tick mark */
  int tick_x1 = 0; /* X pt of start of tick mark line */
  int tick_x2 = 0; /* X pt of end of tick mark line */
  int text_x1 = 0; /* X pt to draw string */
  int text_width = 0; /* Width of text to draw */
  int color_box_x1 = 0; /* X pt of left side of color box */
  int color_box_y1 = 0; /* Y pt of top side of color box */
  int color_box_height = 0; /* Height of each color box */
  int color_bar_area_x1 = 0; /* X pt of left side of color bar area */
  XFontStruct *fontinfo = NULL; /* Pointer to Font info */
  XRectangle clip_rectangle;

  /* Define the clip region to include the entire display region. */

  clip_rectangle.x = 0;
  clip_rectangle.y = 0;
  clip_rectangle.width = BD_width;
  clip_rectangle.height = BD_height;

  XSetClipRectangles( BD_display, BD_gc, 0, 0, &clip_rectangle, 1, Unsorted );

  /* Initialize variables. */

  color_bar_area_x1 = BD_width - COLOR_BAR_AREA_WIDTH;
  tick_x1 = BD_width - COLOR_BAR_AREA_WIDTH/3;
  tick_x1 = BD_width - COLOR_BAR_BUFFER - COLOR_BOX_WIDTH - TICK_MARK_LENGTH;
  tick_x2 = tick_x1 + TICK_MARK_LENGTH;
  color_box_x1 = tick_x2;
  color_box_y1 = BD_width/4;
  text_x1 = tick_x1 - 1;

  /* Define offset of font (to center over tick marks) as
     half of ascent, since numbers don't descend. */

  fontinfo = hci_get_fontinfo( LIST );
  font_off = fontinfo->ascent/2;

  /* Clear an area to the right of the base data display to place
     the color bar. */

  XSetForeground( BD_display, BD_gc, hci_get_read_color(BACKGROUND_COLOR1) );

  XFillRectangle( BD_display, BD_window, BD_gc, color_bar_area_x1, 0,
                  COLOR_BAR_AREA_WIDTH, BD_height );

  XFillRectangle( BD_display, BD_pixmap, BD_gc, color_bar_area_x1, 0,
                  COLOR_BAR_AREA_WIDTH, BD_height );

  /* Define the height scale for the color boxes. */

  color_box_height  = (BD_height/2)/num_colors;

  /* For each color element, create a box and fill it with the color. */

  for( i=1; i<PRODUCT_COLORS; i++ )
  {
    scanl = color_box_y1 + color_box_height*(i-1);

    XSetForeground( BD_display, BD_gc, BD_color[i] );

    XFillRectangle( BD_display, BD_window, BD_gc, color_box_x1, scanl,
                    COLOR_BOX_WIDTH, color_box_height );

    XFillRectangle( BD_display, BD_pixmap, BD_gc, color_box_x1, scanl,
                    COLOR_BOX_WIDTH, color_box_height );
  }

  XSetForeground( BD_display, BD_gc, BD_background_color );

  XDrawRectangle( BD_display, BD_window, BD_gc, color_box_x1, color_box_y1,
                  COLOR_BOX_WIDTH, num_colors*color_box_height );
  XDrawRectangle( BD_display, BD_pixmap, BD_gc, color_box_x1, color_box_y1,
                  COLOR_BOX_WIDTH, num_colors*color_box_height );

  /* Draw text labels and tick marks for minimum value. */

  scanl = color_box_y1;

  if( BD_data_type == DIFF_CORRELATION )
  {
    sprintf( text,"%3.1f", BD_value_min );
  }
  else if( BD_value_min < 0.0 )
  {
    sprintf( text,"%d",(int) (BD_value_min-0.5) );
  }
  else
  {
    sprintf( text,"%d",(int) (BD_value_min+0.5) );
  }

  text_width = XTextWidth( fontinfo, text, strlen( text ) );

  XDrawString( BD_display, BD_window, BD_gc, (text_x1-text_width),
               (scanl+font_off), text, strlen( text ) );

  XDrawString( BD_display, BD_pixmap, BD_gc, (text_x1-text_width),
               (scanl+font_off), text, strlen( text ) );

  XDrawLine( BD_display, BD_window, BD_gc, tick_x1, scanl, tick_x2, scanl );
  XDrawLine( BD_display, BD_pixmap, BD_gc, tick_x1, scanl, tick_x2, scanl );

  /* Draw text labels and tick marks for maximum value. */

  if( BD_data_type == DIFF_CORRELATION )
  {
    sprintf( text,"%3.1f", BD_value_max );
  }
  else if( BD_value_max < 0.0 )
  {
    sprintf( text,"%d",(int) (BD_value_max-0.5) );
  }
  else
  {
    sprintf( text,"%d",(int) (BD_value_max+0.5) );
  }

  text_width = XTextWidth( fontinfo, text, strlen( text ) );

  if( !BD_doppler_flag )
  {
    scanl = color_box_y1 + color_box_height*num_colors;

    XDrawString( BD_display, BD_window, BD_gc, (text_x1-text_width),
                 (scanl+font_off), text, strlen( text ) );

    XDrawString( BD_display, BD_pixmap, BD_gc, (text_x1-text_width),
                 (scanl+font_off), text, strlen( text ) );

    XDrawLine( BD_display, BD_window, BD_gc, tick_x1, scanl, tick_x2, scanl );
    XDrawLine( BD_display, BD_pixmap, BD_gc, tick_x1, scanl, tick_x2, scanl );
  }
  else
  {
    /* Doppler also needs text label and tick mark for range folding. */

    scanl = color_box_y1 + color_box_height*(num_colors-1);

    XDrawString( BD_display, BD_window, BD_gc, (text_x1-text_width),
                 (scanl+font_off), text, strlen( text ) );

    XDrawString( BD_display, BD_pixmap, BD_gc, (text_x1-text_width),
                 (scanl+font_off), text, strlen( text ) );

    XDrawLine( BD_display, BD_window, BD_gc, tick_x1, scanl, tick_x2, scanl );
    XDrawLine( BD_display, BD_pixmap, BD_gc, tick_x1, scanl, tick_x2, scanl );

    scanl = color_box_y1 + color_box_height*num_colors;

    text_width = XTextWidth( fontinfo, "RF", 2 );

    XDrawString( BD_display, BD_window, BD_gc, (text_x1-text_width),
                 (scanl+font_off), "RF", 2 );

    XDrawString( BD_display, BD_pixmap, BD_gc, (text_x1-text_width),
                 (scanl+font_off), "RF", 2 );

    XDrawLine( BD_display, BD_window, BD_gc, tick_x1, scanl, tick_x2, scanl );
    XDrawLine( BD_display, BD_pixmap, BD_gc, tick_x1, scanl, tick_x2, scanl );
  }

  /* Draw text labels and tick marks for center value. */

  avg_value = (BD_value_max+BD_value_min)/2.0;

  if( !BD_doppler_flag )
  {
    scanl = color_box_y1 + color_box_height*num_colors/2;
  }
  else
  {
    /* One less Doppler "value" color because of range folding. */
    scanl = color_box_y1 + color_box_height*(num_colors-1)/2;
  }

  if( BD_data_type == DIFF_CORRELATION )
  {
    sprintf( text,"%3.1f", avg_value );
  }
  else if( avg_value < 0.0 )
  {
    sprintf( text,"%d",(int) (avg_value-0.5) );
  }
  else
  {
    sprintf( text,"%d",(int) (avg_value+0.5) );
  }

  text_width = XTextWidth( fontinfo, text, strlen( text ) );

  XDrawString( BD_display, BD_window, BD_gc, (text_x1-text_width),
               (scanl+font_off), text, strlen( text ) );

  XDrawString( BD_display, BD_pixmap, BD_gc, (text_x1-text_width),
               (scanl+font_off), text, strlen( text ) );

  XDrawLine( BD_display, BD_window, BD_gc, tick_x1, scanl, tick_x2, scanl );
  XDrawLine( BD_display, BD_pixmap, BD_gc, tick_x1, scanl, tick_x2, scanl );

  /* Reset the clip rectangle so that drawing in the base data
     display region will not write over the color bar. */

  clip_rectangle.x = 0;
  clip_rectangle.y = 0;
  clip_rectangle.width = BD_width-COLOR_BAR_AREA_WIDTH;
  clip_rectangle.height = BD_height;

  XSetClipRectangles( BD_display, BD_gc, 0, 0, &clip_rectangle, 1, Unsorted );
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
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_elevation_buttons()
{
  char temp_buf[32];
  XmString text;
  int i;
  int number_cuts;
  float old_angle = -99;

  void hci_data_range_callback( Widget w, XtPointer cl, XtPointer ca );

  /* Get the number of elevation cuts in the current VCP */

  number_cuts = hci_get_scan_number_elevation_cuts();

  /* Update the VCP Label in the Base Data Display Window. */

  sprintf( temp_buf,"VCP %2d", hci_get_scan_vcp_number() );
  text = XmStringCreateLocalized( temp_buf );

  XtVaSetValues( Vcp_label, XmNlabelString, text, NULL );
  XmStringFree( text );

  /* Now update elevation angle labels for all cuts. */

  BD_elevations = 0;

  for( i=1; i<=number_cuts; i++ )
  {
    if( fabsf(old_angle - hci_get_scan_elevation_angle(i-1) ) >= ONE_BAM )
    {
      BD_elevations++;
      old_angle = hci_get_scan_elevation_angle(i-1);
      sprintf( temp_buf," %4.1f ",hci_get_scan_elevation_angle(i-1) );
      text = XmStringCreateLocalized( temp_buf );
      XtVaSetValues( Button[BD_elevations],
                     XmNlabelString, text,
                     XmNforeground, hci_get_read_color(BUTTON_FOREGROUND),
                     XmNbackground, hci_get_read_color(BUTTON_BACKGROUND),
                     NULL );
      XmStringFree( text );

      /* Manage all of those which are active. */

      XtManageChild( Button[BD_elevations] );
    }
  }

  /* Unmanage all inactive elevation cut buttons. */

  for( i=BD_elevations+1; i<MAX_ELEVATION_SCANS; i++ )
  {
    XtUnmanageChild( Button[i] );
  }
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to reflectivity.				*
 ************************************************************************/

void hci_basedata_ref()
{
  HCI_LE_log("Reflectivity selected");

  /* Reset the current field, range, min, and max fields. */

  Field_changed = 1;
  BD_data_type = REFLECTIVITY;
  BD_doppler_flag = 0;
  strcpy( BD_current_field,"  Reflectivity" );

  BD_value_min  = Reflectivity_min;
  BD_value_max  = Reflectivity_max;

  hci_basedata_tool_get_product_colors( REFLECTIVITY_PRODUCT, BD_color );

  /* If we are looking at a previously read scan then lets
     display it. Else, clear the display. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                         (XtPointer) BD_display_scan, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
  }

  hci_basedata_color_bar();
  hci_basedata_overlay_grid();

  /* Directly invoke the expose callback to make the changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to velocity.				*
 ************************************************************************/

void hci_basedata_vel()
{
  HCI_LE_log("Base Data Velocity selected" );

  /* Reset the current field, range, min, and max fields. */

  Field_changed = 1;
  BD_data_type = VELOCITY;
  BD_doppler_flag = 1;
  strcpy( BD_current_field,"      Velocity" );

  BD_value_min  = Velocity_min;
  BD_value_max  = Velocity_max;

  hci_basedata_tool_get_product_colors(VELOCITY_PRODUCT, BD_color);

  /* If we are looking at a previously read scan then lets
     display it. Else, clear the display. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                         (XtPointer) BD_display_scan, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
  }

  hci_basedata_color_bar();
  hci_basedata_overlay_grid();

  /* Directly invoke the expose callback to make the changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to spectrum width.				*
 ************************************************************************/

void hci_basedata_spw()
{
  HCI_LE_log("Base Data Spectrum Width selected" );

  /* Reset the current field, range, min, and max fields. */

  Field_changed = 1;
  BD_data_type = SPECTRUM_WIDTH;
  BD_doppler_flag = 1;
  strcpy( BD_current_field,"Spectrum Width" );

  BD_value_min  = Spectrum_width_min;
  BD_value_max  = Spectrum_width_max;

  hci_basedata_tool_get_product_colors(SPECTRUM_WIDTH_PRODUCT, BD_color);

  /* If we are looking at a previously read scan then lets
     display it. Else, clear the display. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                         (XtPointer) BD_display_scan, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
  }

  hci_basedata_color_bar();
  hci_basedata_overlay_grid();

  /* Directly invoke the expose callback to make the changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to Differential Reflectivity (ZDR).		*
 ************************************************************************/

void hci_basedata_zdr()
{
  HCI_LE_log("Differential Reflectivity selected" );

  /* Reset the current field, range, min, and max fields. */

  Field_changed = 1;
  BD_data_type = DIFF_REFLECTIVITY;
  BD_doppler_flag = 0;
  strcpy( BD_current_field,"  Diff Reflect" );

  BD_value_min  = Diff_reflectivity_min;
  BD_value_max  = Diff_reflectivity_max;

  hci_basedata_tool_get_product_colors(DIFF_REFLECTIVITY_PRODUCT, BD_color);

  /* If we are looking at a previously read scan then lets
     display it. Else, clear the display. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                        (XtPointer) BD_display_scan, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
  }

  hci_basedata_color_bar();
  hci_basedata_overlay_grid();

  /* Directly invoke the expose callback to make the changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to Differential Phase (PHI).		*
 ************************************************************************/

void hci_basedata_phi()
{
  HCI_LE_log("Differential Phase selected" );

  /* Reset the current field, range, min, and max fields. */

  Field_changed = 1;
  BD_data_type = DIFF_PHASE;
  BD_doppler_flag = 0;
  strcpy( BD_current_field,"    Diff Phase" );

  BD_value_min  = Diff_phase_min;
  BD_value_max  = Diff_phase_max;

  hci_basedata_tool_get_product_colors(DIFF_PHASE_PRODUCT, BD_color);

  /* If we are looking at a previously read scan then lets
     display it. Else, clear the display. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                         (XtPointer) BD_display_scan, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
  }

  hci_basedata_color_bar();
  hci_basedata_overlay_grid();

  /* Directly invoke the expose callback to make the changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function is used to change the data field	*
 *		     that is being dispayed in the basedata display	*
 *		     window to Differential Correlation (RHO).		*
 ************************************************************************/

void hci_basedata_rho()
{
  HCI_LE_log("Differential Correlation selected" );

  /* Reset the current field, range, min, and max fields. */

  Field_changed = 1;
  BD_data_type = DIFF_CORRELATION;
  BD_doppler_flag = 0;
  strcpy( BD_current_field,"    Corr Coeff" );

  BD_value_min  = Diff_correlation_min;
  BD_value_max  = Diff_correlation_max;

  hci_basedata_tool_get_product_colors(DIFF_CORRELATION_PRODUCT, BD_color);

  /* If we are looking at a previously read scan then lets
     display it. Else, clear the display. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_display_selected_scan( Button[BD_display_scan],
                        (XtPointer) BD_display_scan, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
  }

  hci_basedata_color_bar();
  hci_basedata_overlay_grid();

  /* Directly invoke the expose callback to make the changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function is used to display the selected	*
 *		     elevation scan from the list of elevation buttons.	*
 *		     If the "Scan" button is selected, dynamic display	*
 *		     mode is activated and data from all scans are	*
 *		     displays as they are ingested.			*
 *									*
 *	Input:  w - push button ID					*
 *		cl - scan index (-1 for dynamic mode)			*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_display_selected_scan( Widget w, XtPointer cl, XtPointer ca )
{
  int status;
  int i;
  int vol_num;
  int sub_type;
  int allow_static = 0;
  LB_id_t start_id;
  LB_id_t stop_id;

  HCI_LE_log("Base Data scan %d selected", (int) cl);

  /* Get the current elevation scan from the button client data. */

  BD_display_scan = (int) cl;

  /* Clear the contents of the base data window. */

  hci_basedata_clear_pixmap();

  /* If we are in dynamic display mode we read and display the
     data directly from the raw RPG input buffer. */

  if( BD_display_scan == -1 )
  {
    /* If we had previously displayed a static scan from the
       replay database, free memory associated with it. */

    if( BD_data != NULL )
    {
      free( BD_data );
      BD_data = NULL;
    }

    BD_display_mode = DISPLAY_MODE_DYNAMIC;

    hci_basedata_expose_callback( (Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL );

    hci_basedata_clear_buttons();
    hci_basedata_set_button( w );

    hci_basedata_set_window_header_dynamic();
    hci_basedata_overlay_grid();

  } /* End of BD_display_scan == -1 (dynamic mode) */
  else
  {
    /* Set the display mode to static so we don't get the display
       updated with new data when we are finished. */

    BD_display_mode = DISPLAY_MODE_STATIC;

    hci_basedata_clear_buttons();
    hci_basedata_set_button( w );

    /* Only reflectivity, velocity, and spectrum width can
       be displayed statically, and only from RANGE RECOMBINED LB (for now). */

    allow_static = 0;
    Basedata_static_index = -1;
    for( i = 0; i < MAX_NUM_BASEDATA_LBS; i++ ){

       if( basedata_LB_ids[i] == Basedata_LB_id ){

          if( basedata_static_allowed[i] ){

             allow_static = 1;
             Basedata_static_index = i;

          }
          
          break;

       } 

    }

    if( !allow_static )
    {
      hci_basedata_draw_text_message( "Data Not Available In Static Mode" );
      return;
    }

    /* For the rest of the cases we read the data from the replay
       database since we are guaranteed that at least one full
       volume of data will be available. */

    status = ORPGBDR_reg_radial_replay_type( Basedata_LB_id,
                      basedata_replay_info[Basedata_static_index].replay_LBID,
                      basedata_replay_info[Basedata_static_index].acct_LBID );

    if( status != 0 )
    {
      HCI_LE_error("ORPGBDR_reg_radial_replay_type (%d)", status);
      hci_basedata_draw_text_message( "Product Not Available" );
      return;
    }

    if( BD_data == NULL )
    {
      BD_data = calloc( SIZEOF_BASEDATA, 1 );

      if( BD_data == NULL )
      {
        HCI_LE_error("calloc (SIZEOF_BASEDATA, 1) failed" );
        return;
      }
    }

    /* Read the first radial in the elevation cut and extract the
       time and elevation information and display it in the Base
       Data Window header. */

    vol_num = ORPGVST_get_volume_number();

    if( !BD_doppler_flag )
    {
      sub_type = (BASEDATA_TYPE | REFLDATA_TYPE);
    }
    else
    {
      sub_type = (BASEDATA_TYPE | COMBBASE_TYPE);
    }
 
    start_id = ORPGBDR_get_start_of_elevation_msgid( Basedata_LB_id,
                 sub_type, vol_num, BD_display_scan );
 
    stop_id = ORPGBDR_get_end_of_elevation_msgid( Basedata_LB_id,
                 sub_type, vol_num, BD_display_scan );

    if( (start_id <= 0) || (stop_id <= 0) )
    {
      /* Try previous volume. */

      vol_num--;

      start_id = ORPGBDR_get_start_of_elevation_msgid( Basedata_LB_id,
                   sub_type, vol_num, BD_display_scan );
 
      stop_id  = ORPGBDR_get_end_of_elevation_msgid(Basedata_LB_id,
                   sub_type, vol_num, BD_display_scan );

      if( (start_id <= 0) || (stop_id <= 0) )
      {
        hci_basedata_draw_text_message( "Product Not Available" );
        return;
      }
    }

    hci_basedata_set_window_header_static(vol_num);

    for( i=start_id; i<=stop_id; i++ )
    {
      status = ORPGBDR_read_radial( Basedata_LB_id,
                   BD_data, SIZEOF_BASEDATA, (LB_id_t) i );

      if( status <= 0 )
      {
        HCI_LE_error("ORPGBDR_read_radial (%d) failed (%d)",
                     Basedata_LB_id, status );
        break;
      }

      BD_data_len = status;

      hci_basedata_tool_display( BD_data, BD_data_len,
                     BD_display, BD_pixmap, BD_gc,
                     BD_x_offset, BD_y_offset, BD_scale_x, BD_scale_y,
                     BD_center_pixel, BD_center_scanl,
                     BD_value_min, BD_value_max,
                     BD_color, BD_data_type );
    }
  }/* End of BD_display_scan != -1 */

  hci_basedata_overlay_grid();

  /* Invoke the expose callback to make changes visible. */

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function handles all mouse button events	*
 *		     inside the basedata display region.		*
 *									*
 *	Input:  w - drawing area widget ID				*
 *		evt - X event data					*
 *		args - number of user argunments			*
 *		num_args - user arguments				*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_draw( Widget w, XEvent *evt, String *args, int *num_args )
{
  static int button_down = 0;
  static int first_pixel;
  static int first_scanl;
  int iazi;
  float x1, y1;
  float azimuth, range;
  float height;
  float value;
  char temp_buf[32];
  XmString string;

  /* Ignore button clicks outside of drawing area. */

  if( evt->xbutton.x > ( BD_width - COLOR_BAR_AREA_WIDTH ) )
  {
    return;
  }

  value = 0.0;

  if( !strcmp( args[0], "down1" ) )
  {
    HCI_LE_log("Base Data left button pushed" );

    /* If the left button is pressed, save the value of this point for
       future reference.  For editing clutter suppression regions,
       this point marks the left side of the sector. */

    first_pixel = evt->xbutton.x;
    first_scanl = evt->xbutton.y;

    /* Set the button down flag to indicate that the left mouse button
       has been pressed.  This is necessary for letting the rubber-
       banding code to know when to start (for motion events when the
       left mouse button is pressed). */

    button_down = 1;

    if( Display_mode == ZOOM_MODE )
    {
      BD_zoom_factor = BD_zoom_factor*2;

      if( BD_zoom_factor > ZOOM_FACTOR_MAX )
      {
        BD_zoom_factor = ZOOM_FACTOR_MAX;
        return;
      }

      BD_x_offset = (BD_center_pixel-first_pixel)/
                     BD_scale_x + BD_x_offset;
      BD_y_offset = (BD_center_scanl-first_scanl)/
                     BD_scale_y + BD_y_offset;

      /* Determine the azimuth/range for the new location.  If the
         range is beyond the max data range, then set the X and Y
         offsets to the max data range, preserving the azimuth. */

      azimuth = hci_find_azimuth( evt->xbutton.x, evt->xbutton.y,
                   (int)(BD_center_pixel + BD_x_offset*BD_scale_x),
                   (int)(BD_center_scanl + BD_y_offset*BD_scale_y) );

      x1 = BD_x_offset;
      y1 = BD_y_offset;

      range = sqrt((double) (x1*x1 + y1*y1));

      if( range > BD_display_range )
      {
        BD_x_offset = (BD_x_offset/range)*BD_display_range;
        BD_y_offset = (BD_y_offset/range)*BD_display_range;
      }

      BD_scale_x  = BD_zoom_factor * (BD_width-COLOR_BAR_AREA_WIDTH)/
                                      (2*BD_display_range);
      BD_scale_y  = - BD_zoom_factor * (BD_width-COLOR_BAR_AREA_WIDTH)/
                                      (2*BD_display_range);

      hci_basedata_resize_callback( (Widget) NULL,
                                    (XtPointer) NULL, (XtPointer) NULL );

    } /* End of Display_mode == ZOOM_MODE */
    else
    {
      azimuth = hci_find_azimuth(evt->xbutton.x, evt->xbutton.y,
                   (int)(BD_center_pixel + BD_x_offset*BD_scale_x),
                   (int)(BD_center_scanl + BD_y_offset*BD_scale_y) );

      x1 = (BD_center_pixel-evt->xbutton.x)/BD_scale_x + BD_x_offset;
      y1 = (BD_center_scanl-evt->xbutton.y)/BD_scale_y + BD_y_offset;

      range = sqrt((double) (x1*x1 + y1*y1));

      sprintf( temp_buf,"%d deg, %d nm",
               (int) azimuth, (int) (range*HCI_KM_TO_NM+0.5));

      string = XmStringCreateLocalized( temp_buf );
      XtVaSetValues( Azran_data, XmNlabelString, string, NULL );
      XmStringFree(string);

      height = range*sin((double) BD_current_elevation*HCI_DEG_TO_RAD)
                     + range*range/(2.0*1.21*6371.0);

      sprintf( temp_buf,"%d ft (ARL)", (int) (3281*height+0.5) );

      string = XmStringCreateLocalized( temp_buf );
      XtVaSetValues( Height_data, XmNlabelString, string, NULL );
      XmStringFree( string );

      if( BD_display_scan == -1 )
      {
        iazi = hci_determine_azimuth_index( azimuth );
        value = hci_basedata_tool_interrogate( DISPLAY_MODE_DYNAMIC,
                            azimuth, range,
                            BD_data_type, Basedata_LB_id,
                            BD_azi_ptr[iazi] );
      }
      else
      {
        /* Only RANGE RECOMBINED LB can be read statically. */
        if( Basedata_LB_id != RANGE_RECOMBINED_LB )
        {
          value = LOWER_MOMENT_THRESHOLD;
        }
        else
        { 
          value = hci_basedata_tool_interrogate( DISPLAY_MODE_STATIC,
                              azimuth, range,
                              BD_data_type, Basedata_LB_id,
                              BD_display_scan );
        }
      }

      sprintf( temp_buf, "       " );

      if( BD_data_type == REFLECTIVITY )
      {
        if( value < REFLECTIVITY_MIN ){ sprintf( temp_buf, "No Data" ); }
        else{ sprintf( temp_buf,"%3d dBZ", (int) value ); }
      }
      else if( BD_data_type == VELOCITY )
      {
        if( value > 999 )
        {
          sprintf( temp_buf,"RF    " );
        }
        else if( (value < (VELOCITY_MIN*Velocity_scale) ) ||
                 (value > ( VELOCITY_MAX*Velocity_scale) ) )
        {
          sprintf( temp_buf,"No Data" );
        }
        else
        {
          sprintf( temp_buf,"%3d kts", (int) value );
        }
      }
      else if( BD_data_type == SPECTRUM_WIDTH )
      {
        if( value >999 )
        {
          sprintf( temp_buf,"RF    " );
        }
        else if( value < 0.0 )
        {
          sprintf( temp_buf,"No Data" );
        }
        else
        {
          if( value > SPECTRUM_WIDTH_MAX ){ value = SPECTRUM_WIDTH_MAX; }
          sprintf( temp_buf,"%3d kts", (int) value );
        }
      }
      else if( BD_data_type == DIFF_REFLECTIVITY )
      {
        if( value < DIFF_REFLECTIVITY_MIN ){ sprintf( temp_buf, "No Data" ); }
        else{ sprintf( temp_buf,"%3d dBZ", (int) value ); }
      }
      else if( BD_data_type == DIFF_PHASE )
      {
        if( value < DIFF_PHASE_MIN ){ sprintf( temp_buf, "No Data" ); }
        else{ sprintf( temp_buf,"%3d dBZ", (int) value ); }
      }
      else if( BD_data_type == DIFF_CORRELATION )
      {
        if( value < DIFF_CORRELATION_MIN ){ sprintf( temp_buf, "No Data" ); }
        else{ sprintf( temp_buf,"%3d dBZ", (int) value ); }
      }

      string = XmStringCreateLocalized( temp_buf );
      XtVaSetValues( Value_data, XmNlabelString, string, NULL );
      XmStringFree( string );

    } /* End of Display_mode != ZOOM_MODE */
  } /* End of left-button down. */
  else if( !strcmp( args[0], "up1" ) )
  {
    /* If the left mouse button is released, stop the rubber-banding
       code and unset the button down flag. */

    HCI_LE_log("Base Data left button released" );
    button_down  = 0;
  }
  else if( !strcmp( args[0], "motion1" ) )
  {
    /* If the cursor is being dragged with the left mouse button
       down, repeatedly draw the sector defined by the point where
       the button was pressed and where it currently is.  Use the
       left hand rule for determining the direction in which to draw 
       the sector. */
  }
  else if( !strcmp( args[0], "down2" ) )
  {
    HCI_LE_log("Base Data middle button pushed");

    if( Display_mode == ZOOM_MODE )
    {
      first_pixel = evt->xbutton.x;
      first_scanl = evt->xbutton.y;

      BD_x_offset = (BD_center_pixel-first_pixel)/BD_scale_x + BD_x_offset;
      BD_y_offset = (BD_center_scanl-first_scanl)/BD_scale_y + BD_y_offset;

      /* Determine the azimuth/range for the new location.  If
         the range is beyond the max data range, then set the
         X and Y offsets to the max data range, preserving the
        azimuth. */

      azimuth = hci_find_azimuth( evt->xbutton.x, evt->xbutton.y,
                     (int)(BD_center_pixel + BD_x_offset*BD_scale_x),
                     (int)(BD_center_scanl + BD_y_offset*BD_scale_y) );

      x1 = BD_x_offset;
      y1 = BD_y_offset;

      range = sqrt( (double) (x1*x1 + y1*y1) );

      if( range > BD_display_range )
      {
        BD_x_offset = (BD_x_offset/range)*BD_display_range;
        BD_y_offset = (BD_y_offset/range)*BD_display_range;
      }

      hci_basedata_resize_callback( (Widget) NULL,
                                    (XtPointer) NULL, (XtPointer) NULL );
    }
  }
  else if( !strcmp( args[0], "down3" ) )
  {
    HCI_LE_log("Base Data right button pushed" );

    if( Display_mode == ZOOM_MODE )
    {
      first_pixel = evt->xbutton.x;
      first_scanl = evt->xbutton.y;

      BD_zoom_factor = BD_zoom_factor/2;

      if( BD_zoom_factor < ZOOM_FACTOR_MIN )
      {
        BD_zoom_factor = ZOOM_FACTOR_MIN;
        return;
      }

      BD_x_offset = (BD_center_pixel-first_pixel)/BD_scale_x + BD_x_offset;
      BD_y_offset = (BD_center_scanl-first_scanl)/BD_scale_y + BD_y_offset;
      BD_scale_x  = BD_zoom_factor * (BD_width-COLOR_BAR_AREA_WIDTH)/
                                      (2*BD_display_range);
      BD_scale_y  = - BD_zoom_factor * (BD_width-COLOR_BAR_AREA_WIDTH)/
                                      (2*BD_display_range);

      /* Determine the azimuth/range for the new location. If
         the range is beyond the max data range, then set the
         X and Y offsets to the max data range, preserving the
         azimuth. */

      azimuth = hci_find_azimuth( evt->xbutton.x, evt->xbutton.y,
                     (int)(BD_center_pixel + BD_x_offset*BD_scale_x),
                     (int)(BD_center_scanl + BD_y_offset*BD_scale_y) );

      x1 = BD_x_offset;
      y1 = BD_y_offset;

      range = sqrt( (double) (x1*x1 + y1*y1) );

      if( range > BD_display_range )
      {
        BD_x_offset = (BD_x_offset/range)*BD_display_range;
        BD_y_offset = (BD_y_offset/range)*BD_display_range;
      }

      hci_basedata_resize_callback( (Widget) NULL,
                                    (XtPointer) NULL, (XtPointer) NULL );
    }
  }
}

/************************************************************************
 *	Description: This function is used to compute the azimuth	*
 *		     coordinate of a screen pixel element relative to	*
 *		     a reference screen coordinate (radar).		*
 *									*
 *	Input:  pt_x - pixel coordinate of point			*
 *		pt_y - scanline coordinate of point			*
 *		ref_x  - reference pixel coordinate			*
 *		ref_y  - reference scanline coordinate			*
 *	Return: azimuth angle (degrees)					*
 ************************************************************************/

float hci_find_azimuth( int pt_x, int pt_y, int ref_x, int ref_y )
{
  float x1, y1, xr, yr;
  float azimuth;

  xr = ref_x;
  yr = ref_y;
  x1 = pt_x;
  y1 = pt_y;

  if( x1 <= xr )
  {
    if( y1 >= yr )
    {
      /* ----------NW Quadrant---------- */
      if( x1 == xr ){ azimuth = 180.0; }
      else{  azimuth = 270.0 + atan( ((double) (yr-y1))/(xr-x1) )/HCI_DEG_TO_RAD; }
    }
    else
    {
      /* ----------SW Quadrant---------- */
      if( x1 == xr ){ azimuth = 0.0; }
      else{  azimuth = 270.0 - atan( ((double) (y1-yr))/(xr-x1) )/HCI_DEG_TO_RAD; }
    }
  }
  else
  {
    if( y1 <= yr )
    {
      /* ----------NE Quadrant---------- */
      if( y1 == yr ){ azimuth = 90.0; }
      else{ azimuth = 90.0 - atan( ((double) (yr-y1))/(x1-xr) )/HCI_DEG_TO_RAD; }
    }
    else
    {
      /* ----------SE Quadrant---------- */
      azimuth = 90.0 + atan( ((double) (y1-yr))/(x1-xr) )/HCI_DEG_TO_RAD;
    }
  }

  return azimuth;
}

/************************************************************************
 *	Description: This function is activated when one of the Mode	*
 *		     radio buttons is selected.				*
 *									*
 *	Input:  w - radio button ID					*
 *		cl - RAW_MODE, ZOOM_MODE				*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_display_mode( Widget w, XtPointer cl, XtPointer ca )
{
  char string[32];
  XmString str;

  XmToggleButtonCallbackStruct *state =
                   (XmToggleButtonCallbackStruct *) ca;

  /* Only process when button is set. */

  if( state->set )
  {
    switch( (int) cl )
    {
      case RAW_MODE:

        Display_mode = RAW_MODE;

        sprintf( string, "Interrogate      " );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Left_button, XmNlabelString, str, NULL );
        XmStringFree( str );

        sprintf( string, "N/A" );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Middle_button, XmNlabelString, str, NULL );
        XmStringFree( str );

        sprintf( string, "N/A" );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Right_button, XmNlabelString, str, NULL );
        XmStringFree( str );

        XtVaSetValues( Azran_label,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND), NULL );
        XtVaSetValues( Height_label,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND), NULL );
        XtVaSetValues( Value_label,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND), NULL );

        break;

      case ZOOM_MODE:

        Display_mode = ZOOM_MODE;

        sprintf( string, "Center and Zoom +" );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Left_button, XmNlabelString, str, NULL );
        XmStringFree( str );

        sprintf( string, "Center Only" );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Middle_button, XmNlabelString, str, NULL );
        XmStringFree( str );

        sprintf( string, "Center and Zoom -" );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Right_button, XmNlabelString, str, NULL );
        XmStringFree( str );

        XtVaSetValues( Azran_label,
               XmNforeground, hci_get_read_color(BACKGROUND_COLOR1), NULL );
        XtVaSetValues( Height_label,
               XmNforeground, hci_get_read_color(BACKGROUND_COLOR1), NULL );
        XtVaSetValues( Value_label,
               XmNforeground, hci_get_read_color(BACKGROUND_COLOR1), NULL );

        sprintf( string, "               " );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Azran_data, XmNlabelString, str, NULL );
        XmStringFree( str );

        sprintf( string, "               " );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Height_data, XmNlabelString, str, NULL );
        XmStringFree( str );

        sprintf( string, "      " );
        str = XmStringCreateLocalized( string );
        XtVaSetValues( Value_data, XmNlabelString, str, NULL );
        XmStringFree( str );

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
 *		cl - unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_data_range_callback( Widget w, XtPointer cl, XtPointer ca )
{
  Widget form;
  Widget h_rowcol, v_rowcol;
  Widget button;
  Widget reflectivity_frame;
  Widget velocity_frame;
  Widget spectrum_width_frame;
  Widget diff_reflectivity_frame;
  Widget diff_phase_frame;
  Widget diff_correlation_frame;
  Widget label;
  XmString str;
  float temp_min = 0.0;
  float temp_max = 0.0;
  char buf[16];

  void hci_change_data_range_callback( Widget w, XtPointer cl, XtPointer ca );
  void hci_new_data_range_callback( Widget w, XtPointer cl, XtPointer ca );
  void hci_close_data_range_dialog_callback( Widget w,
                                             XtPointer cl, XtPointer ca );

  HCI_LE_log("Change data range selected" );

  /* If the window already exists do nothing and return. */

  if( Data_range_dialog != NULL )
  {
    HCI_Shell_popup( Data_range_dialog );
    return;
  }

  HCI_Shell_init( &Data_range_dialog, "Set Data Range" );

  /* Use a form to manage the contents of the window. */

  form = XtVaCreateWidget( "data_range_form",
              xmFormWidgetClass, Data_range_dialog,
              XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              NULL );

  /* Define a row of buttons to close the window and to apply the changes. */

  h_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, form,
             XmNtopAttachment, XmATTACH_FORM,
             XmNleftAttachment, XmATTACH_FORM,
             XmNrightAttachment, XmATTACH_FORM,
             XmNorientation, XmHORIZONTAL,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  button = XtVaCreateManagedWidget( "Close",
               xmPushButtonWidgetClass, h_rowcol,
               XmNforeground, hci_get_read_color(BUTTON_FOREGROUND),
               XmNbackground, hci_get_read_color(BUTTON_BACKGROUND),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XtAddCallback( button,
       XmNactivateCallback, hci_close_data_range_dialog_callback, NULL );

  Apply_button = XtVaCreateManagedWidget( "Apply",
               xmPushButtonWidgetClass, h_rowcol,
               XmNforeground, hci_get_read_color(BUTTON_FOREGROUND),
               XmNbackground, hci_get_read_color(BUTTON_BACKGROUND),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  if( Update_flag )
  {
    XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
  }
  else
  {
    XtVaSetValues( Apply_button, XmNsensitive, False, NULL );
  }

  XtAddCallback( Apply_button,
       XmNactivateCallback, hci_change_data_range_callback, NULL );

  XtManageChild( h_rowcol );

  /* Define first row of slider bars. */

  h_rowcol = XtVaCreateManagedWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, form,
             XmNtopAttachment, XmATTACH_WIDGET,
             XmNtopWidget, h_rowcol,
             XmNleftAttachment, XmATTACH_FORM,
             XmNrightAttachment, XmATTACH_FORM,
             XmNorientation, XmHORIZONTAL,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  /* Create a set of slider bars to control the reflectivity data range. */

  reflectivity_frame = XtVaCreateWidget( "reflectivity_frame",
             xmFrameWidgetClass, h_rowcol,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  v_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, reflectivity_frame,
             XmNorientation, XmVERTICAL,
             XmNnumColumns, 1,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  temp_min = REFLECTIVITY_MIN;
  temp_max = REFLECTIVITY_MAX;

  Reflectivity_min_scale = XtVaCreateWidget( "reflectivity_minimum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Minimum Reflectivity (dBZ)", 27,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Reflectivity_min,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Reflectivity_min_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) REF_MIN );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Reflectivity_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Reflectivity_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Reflectivity_min_scale );

  Reflectivity_max_scale = XtVaCreateWidget( "reflectivity_maximum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Maximum Reflectivity (dBZ)", 27,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Reflectivity_max,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Reflectivity_max_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) REF_MAX );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Reflectivity_max_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Reflectivity_max_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Reflectivity_max_scale );
  XtManageChild( v_rowcol );
  XtManageChild( reflectivity_frame );

  /* Create a set of slider bars to control the velocity data range. */

  velocity_frame = XtVaCreateWidget( "velocity_frame",
             xmFrameWidgetClass, h_rowcol,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNleftAttachment, XmATTACH_WIDGET,
             XmNleftWidget, reflectivity_frame,
             XmNrightAttachment, XmATTACH_FORM,
             NULL );

  v_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, velocity_frame,
             XmNorientation, XmVERTICAL,
             XmNnumColumns, 1,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  temp_min = VELOCITY_MIN;
  temp_max = VELOCITY_MAX;

  Velocity_min_scale = XtVaCreateWidget( "velocity_minimum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Minimum Velocity (kts)", 27,
              XmNshowValue, True,
              XmNminimum, (int) (temp_min*Velocity_scale),
              XmNmaximum, (int) (temp_max*Velocity_scale),
              XmNvalue, (int) Velocity_min,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Velocity_min_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) VEL_MIN );

  sprintf( buf,"%d",(int) (temp_min*Velocity_scale) );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Velocity_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) (temp_max*Velocity_scale) );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Velocity_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Velocity_min_scale );

  Velocity_max_scale = XtVaCreateWidget( "velocity_maximum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Maximum Velocity (kts)", 27,
              XmNshowValue, True,
              XmNminimum, (int) (temp_min*Velocity_scale),
              XmNmaximum, (int) (temp_max*Velocity_scale),
              XmNvalue, (int) Velocity_max,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Velocity_max_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) VEL_MAX );

  sprintf( buf,"%d",(int) (temp_min*Velocity_scale) );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Velocity_max_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) (temp_max*Velocity_scale) );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Velocity_max_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Velocity_max_scale );
  XtManageChild( v_rowcol );
  XtManageChild( velocity_frame );

  /* Define second row of slider bars. */

  h_rowcol = XtVaCreateManagedWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, form,
             XmNtopAttachment, XmATTACH_WIDGET,
             XmNtopWidget, h_rowcol,
             XmNleftAttachment, XmATTACH_FORM,
             XmNrightAttachment, XmATTACH_FORM,
             XmNorientation, XmHORIZONTAL,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  /* Create a set of slider bars to control the spectrum width data range. */

  spectrum_width_frame = XtVaCreateWidget( "spectrum_width_frame",
             xmFrameWidgetClass, h_rowcol,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  v_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, spectrum_width_frame,
             XmNorientation, XmVERTICAL,
             XmNnumColumns, 1,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  temp_min = SPECTRUM_WIDTH_MIN;
  temp_max = SPECTRUM_WIDTH_MAX;

  Spectrum_width_min_scale = XtVaCreateWidget( "spectrum_width_minimum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Minimum Spectrum Width (kts)", 29,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Spectrum_width_min,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Spectrum_width_min_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) SWP_MIN );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Spectrum_width_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Spectrum_width_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Spectrum_width_min_scale );

  Spectrum_width_max_scale = XtVaCreateWidget( "spectrum_width_maximum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Maximum Spectrum Width (kts)", 29,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Spectrum_width_max,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Spectrum_width_max_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) SWP_MAX );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Spectrum_width_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Spectrum_width_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  XtManageChild( Spectrum_width_max_scale );
  XtManageChild( v_rowcol );
  XtManageChild( spectrum_width_frame );

  /* Create a set of slider bars to control the ZDR data range. */

  diff_reflectivity_frame = XtVaCreateWidget( "diff_reflectivity_frame",
             xmFrameWidgetClass, h_rowcol,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNleftAttachment, XmATTACH_WIDGET,
             XmNleftWidget, spectrum_width_frame,
             NULL );

  v_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, diff_reflectivity_frame,
             XmNorientation, XmVERTICAL,
             XmNnumColumns, 1,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  temp_min = DIFF_REFLECTIVITY_MIN;
  temp_max = DIFF_REFLECTIVITY_MAX;

  Diff_reflectivity_min_scale = XtVaCreateWidget( "diff_reflectivity_minimum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Minimum Diff Reflectivity (dB)", 30,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Diff_reflectivity_min,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Diff_reflectivity_min_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) ZDR_MIN );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Diff_reflectivity_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Diff_reflectivity_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Diff_reflectivity_min_scale );

  Diff_reflectivity_max_scale = XtVaCreateWidget( "diff_reflectivity_maximum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Maximum Diff Reflectivity (dB)", 30,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Diff_reflectivity_max,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Diff_reflectivity_max_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) ZDR_MAX );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Diff_reflectivity_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Diff_reflectivity_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  XtManageChild( Diff_reflectivity_max_scale );
  XtManageChild( v_rowcol );
  XtManageChild( diff_reflectivity_frame );

  /* Define third row of slider bars. */

  h_rowcol = XtVaCreateManagedWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, form,
             XmNtopAttachment, XmATTACH_WIDGET,
             XmNtopWidget, h_rowcol,
             XmNleftAttachment, XmATTACH_FORM,
             XmNrightAttachment, XmATTACH_FORM,
             XmNorientation, XmHORIZONTAL,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  /* Create a set of slider bars to control the PHI data range. */

  diff_phase_frame = XtVaCreateWidget( "diff_phase_frame",
             xmFrameWidgetClass, h_rowcol,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  v_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, diff_phase_frame,
             XmNorientation, XmVERTICAL,
             XmNnumColumns, 1,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  temp_min = DIFF_PHASE_MIN;
  temp_max = DIFF_PHASE_MAX;

  Diff_phase_min_scale = XtVaCreateWidget( "diff_phase_minimum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Minimum Diff Phase (deg)", 24,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Diff_phase_min,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Diff_phase_min_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) PHI_MIN );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Diff_phase_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Diff_phase_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Diff_phase_min_scale );

  Diff_phase_max_scale = XtVaCreateWidget( "diff_phase_maximum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Maximum Diff Phase (deg)", 24,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Diff_phase_max,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Diff_phase_max_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) PHI_MAX );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Diff_phase_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Diff_phase_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  XtManageChild( Diff_phase_max_scale );
  XtManageChild( v_rowcol );
  XtManageChild( diff_phase_frame );

  /* Create a set of slider bars to control the RHO data range. */

  diff_correlation_frame = XtVaCreateWidget( "diff_correlation_frame",
             xmFrameWidgetClass, h_rowcol,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNleftAttachment, XmATTACH_WIDGET,
             XmNleftWidget, diff_phase_frame,
             NULL );

  v_rowcol = XtVaCreateWidget( "data_range_rowcol",
             xmRowColumnWidgetClass, diff_correlation_frame,
             XmNorientation, XmVERTICAL,
             XmNnumColumns, 1,
             XmNforeground, hci_get_read_color(BACKGROUND_COLOR1),
             XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
             NULL );

  temp_min = DIFF_CORRELATION_MIN;
  temp_max = DIFF_CORRELATION_MAX;

  Diff_correlation_min_scale = XtVaCreateWidget( "diff_correlation_minimum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Minimum Corr Coefficient", 24,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Diff_correlation_min,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Diff_correlation_min_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) RHO_MIN );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Diff_correlation_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
              xmLabelWidgetClass, Diff_correlation_min_scale,
              XmNlabelString, str,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XmStringFree( str );

  XtManageChild( Diff_correlation_min_scale );

  Diff_correlation_max_scale = XtVaCreateWidget( "diff_correlation_maximum",
              xmScaleWidgetClass, v_rowcol,
     XtVaTypedArg, XmNtitleString, XmRString, "Maximum Corr Coefficient", 24,
              XmNshowValue, True,
              XmNminimum, (int) temp_min,
              XmNmaximum, (int) temp_max,
              XmNvalue, (int) Diff_correlation_max,
              XmNorientation, XmHORIZONTAL,
              XmNdecimalPoints, 0,
              XmNscaleMultiple, 1,
              XmNwidth, 300,
              XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
              XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
              XmNfontList, hci_get_fontlist(LIST),
              NULL );

  XtAddCallback( Diff_correlation_max_scale,
                 XmNvalueChangedCallback, hci_new_data_range_callback,
                 (XtPointer) RHO_MAX );

  sprintf( buf,"%d",(int) temp_min );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Diff_correlation_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  sprintf( buf,"%d",(int) temp_max );
  str = XmStringCreateLocalized( buf );

  label = XtVaCreateManagedWidget( "",
               xmLabelWidgetClass, Diff_correlation_max_scale,
               XmNlabelString, str,
               XmNforeground, hci_get_read_color(TEXT_FOREGROUND),
               XmNbackground, hci_get_read_color(BACKGROUND_COLOR1),
               XmNfontList, hci_get_fontlist(LIST),
               NULL );

  XmStringFree( str );

  XtManageChild( Diff_correlation_max_scale );
  XtManageChild( v_rowcol );
  XtManageChild( diff_correlation_frame );

  XtManageChild( form );
  HCI_Shell_start( Data_range_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is activated when the "Close"	*
 *		     button is selected in the Set Data Range window.	*
 *									*
 *	Input:  w - Close button ID					*
 *		cl - unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_close_data_range_dialog_callback( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_Shell_popdown( Data_range_dialog );
}

/************************************************************************
 *	Description: This function is activated when one of the		*
 *		     slider bars is moved	.			*
 *									*
 *	Input:  w - slider bar ID					*
 *		cl -  slider bar ID					*
 *		ca - slider widget data					*
 *	Return: NONE							*
 ************************************************************************/

void hci_new_data_range_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *) ca;

  switch( (int) cl )
  {
    /* Reflectivity Min. */
    case REF_MIN :
      Reflectivity_min = (float) cbs->value;
      if( Reflectivity_min >= Reflectivity_max )
      {
        Reflectivity_min = Reflectivity_max-1;
        XtVaSetValues( Reflectivity_min_scale,
                       XmNvalue, (int) Reflectivity_min, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Reflectivity Max. */
    case REF_MAX :
      Reflectivity_max = (float) cbs->value;
      if( Reflectivity_max <= Reflectivity_min )
      {
        Reflectivity_max = Reflectivity_min+1;
        XtVaSetValues( Reflectivity_max_scale,
                       XmNvalue, (int) Reflectivity_max, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Velocity Min. */
    case VEL_MIN :
     Velocity_min = (float) cbs->value;
      if( Velocity_min >= Velocity_max )
      {
        Velocity_min = Velocity_max-1;
        XtVaSetValues( Velocity_min_scale,
                       XmNvalue, (int) Velocity_min, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Velocity Max. */
    case VEL_MAX :
      Velocity_max = (float) cbs->value;
      if( Velocity_max <= Velocity_min )
      {
        Velocity_max = Velocity_min+1;
        XtVaSetValues( Velocity_max_scale,
                       XmNvalue, (int) Velocity_max, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Spectrum Width Min. */
    case SWP_MIN :
      Spectrum_width_min = (float) cbs->value;
      if( Spectrum_width_min >= Spectrum_width_max )
      {
        Spectrum_width_min = Spectrum_width_max-1;
        XtVaSetValues( Spectrum_width_min_scale,
                       XmNvalue, (int) Spectrum_width_min, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Spectrum Width Max. */
    case SWP_MAX :
      Spectrum_width_max = (float) cbs->value;
      if( Spectrum_width_max <= Spectrum_width_min )
      {
        Spectrum_width_max = Spectrum_width_min+1;
        XtVaSetValues( Spectrum_width_max_scale,
                       XmNvalue, (int) Spectrum_width_max, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Diff Reflectivity Min. */
    case ZDR_MIN :
      Diff_reflectivity_min = (float) cbs->value;
      if( Diff_reflectivity_min >= Diff_reflectivity_max )
      {
        Diff_reflectivity_min = Diff_reflectivity_max-1;
        XtVaSetValues( Diff_reflectivity_min_scale,
                       XmNvalue, (int) Diff_reflectivity_min, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Diff Reflectivity Max. */
    case ZDR_MAX :
      Diff_reflectivity_max = (float) cbs->value;
      if( Diff_reflectivity_max <= Diff_reflectivity_min )
      {
        Diff_reflectivity_max = Diff_reflectivity_min+1;
        XtVaSetValues( Diff_reflectivity_max_scale,
                       XmNvalue, (int) Diff_reflectivity_max, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Diff Phase Min. */
    case PHI_MIN :
      Diff_phase_min = (float) cbs->value;
      if( Diff_phase_min >= Diff_phase_max )
      {
        Diff_phase_min = Diff_phase_max-1;
        XtVaSetValues( Diff_phase_min_scale,
                       XmNvalue, (int) Diff_phase_min, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Diff Phase Max. */
    case PHI_MAX :
      Diff_phase_max = (float) cbs->value;
      if( Diff_phase_max <= Diff_phase_min )
      {
        Diff_phase_max = Diff_phase_min+1;
        XtVaSetValues( Diff_phase_max_scale,
                       XmNvalue, (int) Diff_phase_max, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Diff Correlation Min. */
    case RHO_MIN :
      Diff_correlation_min = (float) cbs->value;
      if( Diff_correlation_min >= Diff_correlation_max )
      {
        Diff_correlation_min = Diff_correlation_max-1;
        XtVaSetValues( Diff_correlation_min_scale,
                       XmNvalue, (int) Diff_correlation_min, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
      }
      break;

    /* Diff Correlation Max. */
    case RHO_MAX :
      Diff_correlation_max = (float) cbs->value;
      if( Diff_correlation_max <= Diff_correlation_min )
      {
        Diff_correlation_max = Diff_correlation_min+1;
        XtVaSetValues( Diff_correlation_max_scale,
                       XmNvalue, (int) Diff_correlation_max, NULL );
      }
      if( !Update_flag )
      {
        Update_flag = 1;
        XtVaSetValues( Apply_button, XmNsensitive, True, NULL );
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
 *		cl -  unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_change_data_range_callback( Widget w, XtPointer cl, XtPointer ca )
{
  switch( BD_data_type )
  {
    case REFLECTIVITY :
      BD_value_min = Reflectivity_min;
      BD_value_max = Reflectivity_max;
      break;

    case VELOCITY :
      BD_value_min = Velocity_min;
      BD_value_max = Velocity_max;
      break;

    case SPECTRUM_WIDTH :
      BD_value_min = Spectrum_width_min;
      BD_value_max = Spectrum_width_max;
      break;

    case DIFF_REFLECTIVITY :
      BD_value_min = Diff_reflectivity_min;
      BD_value_max = Diff_reflectivity_max;
      break;

    case DIFF_PHASE :
      BD_value_min = Diff_phase_min;
      BD_value_max = Diff_phase_max;
      break;

    case DIFF_CORRELATION :
      BD_value_min = Diff_correlation_min;
      BD_value_max = Diff_correlation_max;
      break;
  }

  /* If we are looking at a previously read scan then lets display it. */

  if( BD_display_mode == DISPLAY_MODE_STATIC )
  {
    hci_basedata_resize_callback( (Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL );
  }
  else
  {
    hci_basedata_clear_screen();
    hci_basedata_color_bar();
    hci_basedata_overlay_grid();
  }

  Update_flag = 0;

  XtVaSetValues( Apply_button, XmNsensitive, False, NULL );
}

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     "Filter" radio buttons are selected.		*
 *									*
 *	Input:  w - radio button ID					*
 *		cl -  filter mode					*
 *		ca - toggle button data					*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_filter_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XmToggleButtonCallbackStruct *state =
                   (XmToggleButtonCallbackStruct *) ca;

  /* Only do this if the button is set. */

  if( state->set )
  {
    Filter_mode = (int) cl;

    /* Update the data filter flag (0 = off, 1 = on) */

    hci_basedata_resize_callback( (Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL );
  }
}

/************************************************************************
 *	Description: This function is activated when one of the 	*
 *		     "Grid" radio buttons are selected.			*
 *									*
 *	Input:  w - radio button ID					*
 *		cl -  grid overlay mode					*
 *		ca - toggle button data					*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_grid_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XmToggleButtonCallbackStruct *state =
                   (XmToggleButtonCallbackStruct *) ca;

  /* Only do this if the button is set. */

  if( state->set )
  {
    Grid_overlay_flag = (int) cl;

    /* Update the grid overlay toggle flag (0 = off, 1 = on) */

    hci_basedata_resize_callback( (Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL );
  }
}

/************************************************************************
 *	Description: This function displays a polar grid over a		*
 *		     base data display (PPI).				*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_overlay_grid()
{
  int i;
  int size;
  int pixel;
  int scanl;
  int step;
  char buf[32];
  float x, y;
  int start_range;
  int stop_range;

  /* If grid over flag is not set, return. */

  if( !Grid_overlay_flag )
  {
    return;
  }

  XSetForeground( BD_display, BD_gc, BD_foreground_color );

  /* Based on the current zoom factor, determine the distancce
     between polar grid rings (in nautical miles). */

  switch( BD_zoom_factor )
  {
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

  /* First display the circles outward from the radar center. */

  x = fabs( (double) BD_x_offset );
  y = fabs( (double) BD_y_offset );

  if( x > y )
  {
    start_range = x - BD_display_range/BD_zoom_factor;
    stop_range  = x + BD_display_range/BD_zoom_factor;
  }
  else
  {
    start_range = y - BD_display_range/BD_zoom_factor;
    stop_range  = y + BD_display_range/BD_zoom_factor;
  }

  start_range = (start_range/step)*step;
  stop_range  = (stop_range/step)*step + step;

  if( start_range < step )
  {
    start_range = step;
  }

  for( i=step; i<=stop_range; i=i+step )
  {
    size  = i*BD_scale_x/HCI_KM_TO_NM;
    pixel = BD_center_pixel + BD_x_offset*BD_scale_x - size;
    scanl = BD_center_scanl + BD_y_offset*BD_scale_y - size;

    XDrawArc( BD_display, BD_pixmap, BD_gc,
              pixel, scanl, size*2, size*2, 0, -(360*64) );

    sprintf( buf,"%i nm",i );

    XDrawString( BD_display, BD_pixmap, BD_gc,
                 (int)(BD_center_pixel +
                       BD_x_offset*BD_scale_x -
                       4*strlen(buf) ),
                 scanl + 4,
                 buf,
                 strlen( buf ) );
  }

  /* Next display the "spokes" outward from the center. */

  for( i=0; i<360; i=i+step )
  {
    pixel = (int) (((1+1.0/BD_zoom_factor)*BD_display_range *
                    cos( (double) (i+90)*HCI_DEG_TO_RAD ) +
                    BD_x_offset) * BD_scale_x +
                    BD_center_pixel);

    scanl = (int) (((1+1.0/BD_zoom_factor)*BD_display_range *
                    sin( (double) (i-90)*HCI_DEG_TO_RAD ) +
                    BD_y_offset) * BD_scale_y +
                    BD_center_scanl);

    XDrawLine( BD_display, BD_pixmap, BD_gc,
               (int)(BD_center_pixel + BD_x_offset*BD_scale_x),
               (int)(BD_center_scanl + BD_y_offset*BD_scale_y),
               pixel, scanl );
  }

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function initilializes the base data scan	*
 *		     lookup table.					*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

int hci_initialize_basedata_scan_table()
{
  int i = 0;
  int status = 0;

  status = hci_basedata_tool_id();

  if( status <= 0 )
  {
    HCI_LE_error("Unable to get LB file descriptor: %d", status );
    return -1;
  }

  /* We start by moving the message pointer to the beginning of the
     basedata LB. */

  status = hci_basedata_tool_seek(LB_FIRST);

  if( status != LB_SUCCESS )
  {
    HCI_LE_error("LB seek failed during initialization: %d", status);
    return -1;
  }

  /* Initialize array. */

  for( i=0; i<MAX_AZIMUTHS; i++ ){ BD_azi_ptr[i] = -1; }

  /* Now lets read the entire LB and save the message ID of each
     radial in the current virtual volume scan. */

  status = 1;

  while( (status > 0) ||
         (status == LB_EXPIRED) ||
         (status == LB_BUF_TOO_SMALL) )
  {
    /*  Only partially read base data when in low bandwidth mode */
    if( HCI_is_low_bandwidth() )
    {
      status = hci_basedata_tool_read_radial( LB_NEXT, HCI_BASEDATA_PARTIAL_READ );
      if( status == LB_EXPIRED )
      {
        status = hci_basedata_tool_read_radial( LB_LATEST,
                                           HCI_BASEDATA_PARTIAL_READ );
      }
    }
    else
    {
      status = hci_basedata_tool_read_radial( LB_NEXT, HCI_BASEDATA_COMPLETE_READ );
      if( status == LB_EXPIRED )
      {
        status = hci_basedata_tool_read_radial( LB_LATEST,
                                           HCI_BASEDATA_COMPLETE_READ );
      }
    }
  }

  return 0;
}

/************************************************************************
 *	Description: This function is used as a timer to update the	*
 *		     basedata display at a defined interval.  This is	*
 *		     especially needed for dynamic display mode.	*
 *									*
 *	Input:  w - parent widget of timer				*
 *		id - timer ID						*
 *	Return: NONE							*
 ************************************************************************/

void timer_proc()
{
  int LB_msg_id;
  int azimuth_index;
  int status;
  int update_flag;
  static int old_vcp = 0;
  int vcp;
  int i;

  /* Check to see if Scan Info has been updated. */

  if( Scan_info_update_flag )
  {
    Scan_info_update_flag = 0;
    hci_set_scan_info_flag(1);

    if( hci_get_scan_vcp_number() != Current_vcp )
    {
      New_volume  = 1;
      Current_vcp = hci_get_scan_vcp_number();
    }
  }

  /* Now lets read the new LB msgs and save the message ID of each
     radial in the current virtual volume scan. */

  status      = 1;
  update_flag = 0;

  /* The following check is needed to ensure that the basedata LB
     isn't read for new messages while the LB pointer is set to
     an earlier scan for a static display or for interrogation. */

  if( hci_basedata_tool_get_lock_state() )
  {
    return;
  }

  hci_basedata_tool_set_lock_state(1);

  /* Check to see if the VCP changed.  If so, change the buttons
     to reflect the new elevation cuts available. */

  if( New_volume )
  {
    vcp = hci_get_scan_vcp_number();

    if( vcp != old_vcp )
    {
      old_vcp = vcp;
      hci_basedata_elevation_buttons();
      HCI_LE_status("New VCP %d detected", vcp);
    }
    New_volume = 0;
  }

  /* If user changed field, redraw with new data. */

  if( Field_changed )
  {
    Field_changed = 0;
    if( BD_display_mode == DISPLAY_MODE_DYNAMIC )
    {
      update_flag = 1;

      /* If displaying velocity, check resolution. */
      if( BD_data_type == VELOCITY ){ check_velocity_resolution(); }

      /* Clear previous pixmap and redraw grid and window header. */
      hci_basedata_clear_pixmap();
      hci_basedata_overlay_grid();
      hci_basedata_set_window_header_dynamic();

      /* Loop through current list of LB_msg_ids and draw accordingly. */
      for( i = 0; i < MAX_AZIMUTHS; i++ )
      {
        if( BD_azi_ptr[i] > 0 )
        {
          if( !HCI_is_low_bandwidth() )
          {
            hci_basedata_tool_read_radial( BD_azi_ptr[i], HCI_BASEDATA_COMPLETE_READ );
          }
          else
          {
            hci_basedata_tool_read_radial( BD_azi_ptr[i], HCI_BASEDATA_PARTIAL_READ );
          }
          if( hci_basedata_tool_data_available( BD_data_type ) )
          {
            hci_basedata_tool_display( NULL, 0, BD_display, BD_pixmap, BD_gc,
                    BD_x_offset, BD_y_offset, BD_scale_x, BD_scale_y,
                    BD_center_pixel, BD_center_scanl, BD_value_min,
                    BD_value_max, BD_color, BD_data_type );
          }
        }
      }
    }
  }

  /* Check for any new basedata messages to read. */

  while( (status > 0) || (status == LB_BUF_TOO_SMALL) )
  {
    /* Only partially read base data when in low bandwidth mode */

    if( HCI_is_low_bandwidth() )
    {
      status = hci_basedata_tool_read_radial( LB_NEXT, HCI_BASEDATA_PARTIAL_READ );
      if( status == LB_EXPIRED )
      {
        status = hci_basedata_tool_read_radial( LB_LATEST, HCI_BASEDATA_PARTIAL_READ );
      }
    }
    else
    {
      status = hci_basedata_tool_read_radial( LB_NEXT, HCI_BASEDATA_COMPLETE_READ );
      if( status == LB_EXPIRED )
      {
        status = hci_basedata_tool_read_radial( LB_LATEST, HCI_BASEDATA_COMPLETE_READ );
      }
    }

    /* If a new message was read then process it. */

    if( (status > 0) || (status == LB_BUF_TOO_SMALL) )
    {
      /* Get the message id and add it to the azran pointer
         so we can interrogate the displayed data and locate
         the corresponding data. */

      LB_msg_id = hci_basedata_tool_msgid();
      azimuth_index = hci_determine_azimuth_index( hci_basedata_tool_azimuth() );
      BD_azi_ptr[azimuth_index] = LB_msg_id;

      /* If we are dynamically displaying radial data then check
         to see if the elevation changed before displaying it.
         If it did change, then clear the display and update the
         window scan/date/time title information. */

      if( BD_display_mode == DISPLAY_MODE_DYNAMIC )
      {

        if( hci_basedata_tool_radial_status() == BEG_ELEV ||
            hci_basedata_tool_radial_status() == BEG_VOL )
        {
          if( BD_data_type == VELOCITY )
          {
            check_velocity_resolution();
          }

          /* Clear azimuth/LB_msg_id array except for the current azimuth. */

          for( i = 0; i < MAX_AZIMUTHS; i++ ){ BD_azi_ptr[i] = -1; }
          BD_azi_ptr[azimuth_index] = LB_msg_id;

          hci_basedata_set_window_header_dynamic();
          hci_basedata_clear_pixmap();
          hci_basedata_overlay_grid();

        } /* End if new volume or new elevation. */

        hci_basedata_tool_display( NULL, 0, BD_display, BD_pixmap, BD_gc,
                BD_x_offset, BD_y_offset, BD_scale_x, BD_scale_y,
                BD_center_pixel, BD_center_scanl, BD_value_min,
                BD_value_max, BD_color, BD_data_type );

        update_flag = 1;

      } /* End if in dynamic mode */
    } /* End if read radial was successful */
  } /* End while */

  hci_basedata_tool_set_lock_state(0);

  if( update_flag )
  {
    hci_basedata_expose_callback( (Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL );
  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the RPG Base Data Display	*
 *		     window.						*
 *									*
 *	Input:  w - Close button ID					*
 *		cl - unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_close_callback( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log("Base Data window Close pushed" );
  HCI_task_exit( GL_EXIT_SUCCESS );
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
 *	Return: NONE							*
 ************************************************************************/

void hci_scan_info_update( en_t evtcd, void *info, size_t msglen )
{
  orpgevt_scan_info_t *scan_info = (orpgevt_scan_info_t *) info;

  if( (scan_info->key == ORPGEVT_BEGIN_VOL) ||
      (scan_info->key == ORPGEVT_BEGIN_ELEV) )
  {
    Scan_info_update_flag = 1;
  }
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   from the Basedata LBs drop-down list.		*
 *                                                                      *
 *      Input:  w - Close button ID                                     *
 *              cl - unused						*
 *              ca - unused						*
 *      Return: NONE                                                    *
 ************************************************************************/

void hci_change_basedata_LB_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XmComboBoxCallbackStruct *cbs;
  char *temp_buf;
  char new_LB[ 32 ];
  int i;

  cbs = ( XmComboBoxCallbackStruct * ) ca;
  XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );
  strcpy( new_LB, temp_buf );
  XtFree( temp_buf );

  for( i=0; i<MAX_NUM_BASEDATA_LBS; i++ )
  {
    if( strcmp( new_LB, basedata_LB_names[i] ) == 0 )
    {
      Basedata_LB_id = basedata_LB_ids[i];
      hci_basedata_tool_set_basedata_LB_id( Basedata_LB_id );
      if( hci_initialize_basedata_scan_table() < 0 )
      {
        hci_basedata_draw_text_message( "Unable To Initialize Basedata LB" );
      }
      if( BD_display_mode == DISPLAY_MODE_STATIC )
      {
        hci_display_selected_scan( Button[BD_display_scan],
                          (XtPointer) BD_display_scan, (XtPointer) NULL );
      }
      else
      {
        hci_basedata_clear_screen();
        hci_basedata_overlay_grid();
        hci_basedata_set_window_header_dynamic();
      }
    }
  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     from the Moments drop-down list.			*
 *									*
 *	Input:  w - Close button ID					*
 *		cl - unused						*
 *		ca - unused						*
 *	Return: NONE							*
 ************************************************************************/

void hci_change_field_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XmComboBoxCallbackStruct *cbs;
  char *temp_buf;
  char new_field[ 5 ];

  cbs = ( XmComboBoxCallbackStruct * ) ca;
  XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &temp_buf );
  strcpy( new_field, temp_buf );
  XtFree( temp_buf );

  if( strcmp( new_field, "REF" ) == 0 ){ hci_basedata_ref(); }
  else if( strcmp( new_field, "VEL" ) == 0 ){ hci_basedata_vel(); }
  else if( strcmp( new_field, "SPW" ) == 0 ){ hci_basedata_spw(); }
  else if( strcmp( new_field, "ZDR" ) == 0 ){ hci_basedata_zdr(); }
  else if( strcmp( new_field, "PHI" ) == 0 ){ hci_basedata_phi(); }
  else if( strcmp( new_field, "RHO" ) == 0 ){ hci_basedata_rho(); }
}

/************************************************************************
 *	Description: This function writes a message to the display	*
 *		     area.						*
 *									*
 *	Input:  msg - message to write					*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_draw_text_message( char *msg )
{
  XSetFont( BD_display, BD_gc, hci_get_font(LARGE) );

  XSetForeground( BD_display, BD_gc, BD_foreground_color );

  XDrawString( BD_display, BD_pixmap, BD_gc,
               (BD_center_pixel - 4*strlen(msg)),
               BD_center_scanl + 4,
               msg,
               strlen(msg) );

  XSetFont( BD_display, BD_gc, hci_get_font(LIST) );

  hci_basedata_expose_callback( (Widget) NULL,
                                (XtPointer) NULL, (XtPointer) NULL );
}

/************************************************************************
 *	Description: This function clears the data display pixmap.	*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_clear_pixmap()
{
  hci_basedata_tool_display_clear( BD_display, BD_pixmap, BD_gc,
                              BD_width, BD_height, BD_background_color );
}

/************************************************************************
 *	Description: This function clears the data display window.	*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_clear_window()
{
  hci_basedata_tool_display_clear( BD_display, BD_window, BD_gc,
                              BD_width, BD_height, BD_background_color );
}

/************************************************************************
 *	Description: This function clears the data display screen.	*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_clear_screen()
{
  hci_basedata_clear_window();
  hci_basedata_clear_pixmap();
}

/************************************************************************
 *	Description: This function resets the color of all elevation	*
 *		buttons.						*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_clear_buttons()
{
  int i=0;

  /* Set the foreground/background colors of the static
     elevation cut buttons to white/steelblue to indicate
     they are not active. */

  for( i=0; i<=hci_get_scan_number_elevation_cuts(); i++ )
  {
    XtVaSetValues( Button[i],
                   XmNforeground, hci_get_read_color(BUTTON_FOREGROUND),
                   XmNbackground, hci_get_read_color(BUTTON_BACKGROUND),
                   NULL );
  }
}

/************************************************************************
 *	Description: This function sets the color of the selected	*
 *		elevation button.					*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_set_button( Widget button_widget )
{
  XtVaSetValues( button_widget,
                 XmNbackground, hci_get_read_color(BUTTON_FOREGROUND),
                 XmNforeground, hci_get_read_color(BUTTON_BACKGROUND),
                 NULL );
}

/************************************************************************
 *	Description: This function writes elevation/date/time info	*
 *		at top of display in dynamic mode.			*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_set_window_header_dynamic()
{
  int           sr_flag = 0;
  int		elev_date = 0;
  int		yr = 0, mo = 0, dy = 0, hr = 0, min = 0, sec = 0;
  time_t        start_time = 0;
  char		buf[128];
  XmString      str;

  Elevation_number = hci_basedata_tool_elevation_number();

  /* Is this a super resolution cut? */
  if( Basedata_LB_id == PBD_LB || Basedata_LB_id == SR_VELDEAL_LB )
  {
    if( ORPGVST_is_rpg_elev_superres( ORPGVST_get_index(Elevation_number-1) ) )
    {
      sr_flag = 1;
    }
  }

  elev_date = ORPGVST_get_volume_date();
  start_time = ORPGVST_get_volume_time() / 1000;
  start_time += ((elev_date-1)*86400);

  if( start_time > 0 )
  {
    unix_time( &start_time, &yr, &mo, &dy, &hr, &min, &sec );

    if( yr >= 2000 ){ yr -= 2000; }
    else{ yr -= 1900; }
  }

  BD_current_elevation = hci_basedata_tool_target_elevation();

  if( sr_flag )
  {
    sprintf( buf,"%s: %s %5.1f %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d",
             BD_current_field, "Elevation",
             BD_current_elevation, "SR  Date/Time:",
             mo, dy, yr, hr, min, sec ); 
  }
  else
  {
    sprintf( buf,"%s: %s %5.1f     %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d",
             BD_current_field, "Elevation",
             BD_current_elevation, "Date/Time:",
             mo, dy, yr, hr, min, sec ); 
  }

  str = XmStringCreateLocalized( buf );
  XtVaSetValues( BD_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

/************************************************************************
 *	Description: This function writes elevation/date/time info	*
 *		at top of display in static mode.			*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_set_window_header_static( int vol_num )
{
  float         angs[MAX_ELEVATION_SCANS];
  int           num_angs = MAX_ELEVATION_SCANS;
  int		elev_date = 0;
  int		yr = 0, mo = 0, dy = 0, hr = 0, min = 0, sec = 0;
  time_t        start_time = 0;
  char		buf[128];
  XmString      str;

  if( vol_num > -1 )
  {
    /* call ORPGVCP function to get all elev angs for this vcp */
    ORPGVCP_get_all_elevation_angles( hci_get_scan_vcp_number(),
                                               num_angs, angs );
    ORPGBDR_get_start_date_and_time( Basedata_LB_id, 
                                     vol_num, &elev_date, &start_time );

    start_time += (elev_date-1)*86400;

    if( start_time > 0 )
    {
      unix_time( &start_time, &yr, &mo, &dy, &hr, &min, &sec );

      if( yr >= 2000 ){ yr -= 2000; }
      else{ yr -= 1900; }
    }

    /* Set the scan elev to the proper angle chosen by the user */
    BD_current_elevation = angs[BD_display_scan - 1];
  }

  sprintf( buf,"%s: %s %5.1f     %s %02d/%02d/%02d  %2.2d:%2.2d:%2.2d",
           BD_current_field, "Elevation",
           BD_current_elevation, "Date/Time:",
           mo, dy, yr, hr, min, sec ); 

  str = XmStringCreateLocalized( buf );
  XtVaSetValues( BD_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

/************************************************************************
 *	Description: Return array index of given azimuth.		*
 *									*
 *	Input:  Azimuth to find index of				*
 *	Return: Integer of array index of azimuth			*
 ************************************************************************/

int hci_determine_azimuth_index( float azimuth )
{
  if( hci_basedata_tool_azimuth_resolution() == BASEDATA_HALF_DEGREE )
  {
    return (int)(azimuth*2+0.5)%720;
  }

  return (int)(azimuth+0.5)%360;
}

/************************************************************************
 *                                                                      *
 *      Description: Find velocity resolution and display accordingly.  *
 *                                                                      *
 ************************************************************************/

void check_velocity_resolution()
{
  int new_vel_resolution = hci_basedata_tool_velocity_resolution();

  if( new_vel_resolution != Prev_vel_resolution )
  {
    Prev_vel_resolution = new_vel_resolution;
    if( new_vel_resolution == DOPPLER_RESOLUTION_HIGH )
    {
      Velocity_scale = 0.5;
    }
    else
    {
      Velocity_scale = 1.0;
    }
    Velocity_min = VELOCITY_MIN*Velocity_scale;
    Velocity_max = VELOCITY_MAX*Velocity_scale;
    BD_value_min = Velocity_min;
    BD_value_max = Velocity_max;
    hci_basedata_color_bar();
  }
}

