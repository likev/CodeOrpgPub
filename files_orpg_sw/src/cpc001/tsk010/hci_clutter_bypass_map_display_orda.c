
/************************************************************************
 *									*
 *	Module:  hci_clutter_bypass_map_display.c			*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      for displaying the Clutter Bypass Map.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:51 $
 * $Id: hci_clutter_bypass_map_display_orda.c,v 1.22 2010/03/10 18:46:51 ccalvert Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_clutter_bypass_map.h>

/*	Macros.								*/

#define	BASE_DATA_WIDTH		500     /* Default window width (pixels) */
#define	BASE_DATA_HEIGHT	500     /* Default window height (scanlines) */
#define	ZOOM_FACTOR_MAX		 32     /* Maximum zoom factor */
#define BYPASS_MAP_RANGE_MAX	512     /* Maximum radial range (km) */

char Buf[HCI_BUF_256];                /* common buffer for string operations */

Widget Top_widget = (Widget) NULL;
Widget Request_button = (Widget) NULL;
Widget Segment_button = (Widget) NULL;
Widget Draw_widget = (Widget) NULL;
Widget Generation_label = (Widget) NULL;

Dimension Width = BASE_DATA_WIDTH;      /* current window width */
Dimension Height = BASE_DATA_HEIGHT;    /* current window height */
int Bypass_map_depth = 8;       /* window depth (bits) */
int Zoom_factor = 1;            /* current magnification */
int Center_pixel = BASE_DATA_WIDTH / 2; /* window center pixel */
int Center_scanl = BASE_DATA_HEIGHT / 2;        /* window center scanline */
float Scale_x = 1.0;            /* pixels/km in X direction */
float Scale_y = 1.0;            /* scanlines/km in Y direction */
float X_offset = 0.0;           /* X offset of radar to window center */
float Y_offset = 0.0;           /* Y offset of radar to window center */
int Grid_flag = HCI_ON_FLAG;             /* current grid overlay mode */
int Segment = 0;                /* current displayed segment */
int Init_status = 0;            /* initialization flag */
int Bypass_map_update_flag = 0; /* bypass map data update flag */
static int config_change_popup = 0; /* RDA config change popup flag. */
static int Request_flag = HCI_NO_FLAG;

/*	X properties	*/

Display *Bypass_map_display = (Display *) NULL;
GC Bypass_map_gc = (GC) NULL;
Pixmap Bypass_map_pixmap = (Pixmap) NULL;
Window Bypass_map_window = (Window) NULL;

char Cmd[128];                  /* common buffer for feedback messages */

ORDA_bypass_map_msg_t Bypass_map;        /* local bypass map data */

/*	Bypass map region translation table (mouse events)	*/

String Bypass_map_translations = "<PtrMoved>:	bypass_map_input(move) \n\
	<Btn1Down>:	bypass_map_input(down1) ManagerGadgetArm() \n\
	<Btn1Up>:	bypass_map_input(up1)   ManagerGadgetActivate() \n\
	<Btn2Down>:	bypass_map_input(down2) ManagerGadgetArm() \n\
	<Btn2Up>:	bypass_map_input(up2)   ManagerGadgetActivate() \n\
	<Btn3Down>:	bypass_map_input(down3) ManagerGadgetArm() \n\
	<Btn3Up>:	bypass_map_input(up3)   ManagerGadgetActivate()";

void hci_display_bypass_map (
  );
void hci_bypass_map_overlay_grid (
  );
void hci_bypass_map_resize_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void hci_bypass_map_expose_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void bypass_map_input (
  Widget w,
  XEvent * event,
  String * string,
  int *num_args);
void hci_bypass_map_request_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void accept_bypass_map_request_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void bypass_map_request ();
void cancel_bypass_map_request_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void hci_bypass_map_grid_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void hci_bypass_map_close_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void hci_bypass_map_load_baseline_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void hci_bypass_map_segment_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data);
void hci_display_bypass_map_data (
  float azimuth,
  float range,
  int value);
void timer_proc ();
void rda_config_change();

void hci_bypass_map_updated (
  int fd,
  LB_id_t msg_id,
  int msg_info,
  void *arg);
void update_sensitivities();

/************************************************************************
 *	Description: This is the main function for the Clutter Bypass	*
 *		     Map Display task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
  int argc,
  char *argv[])
{
  Widget form;
  Widget frame;
  Widget button;
  Widget segment_rowcol;
  Widget rowcol1;
  Widget rowcol2;
  Widget grid_mode;
  int n;
  Arg arg[10];
  int status;
  XGCValues gcv;
  XtActionsRec actions;

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_CBM_TASK );

  Top_widget = HCI_get_top_widget();
  Bypass_map_display = HCI_get_display();

  Bypass_map_depth = XDefaultDepth (Bypass_map_display,
                                    DefaultScreen (Bypass_map_display));

/*	Make the initial size of the bypass map window something	*
 *	reasonable. The user can resize it as needed. Define a minimum  *
 *	size to be able to hold all of the widgets.			*/

  Width = BASE_DATA_WIDTH;
  Height = BASE_DATA_HEIGHT;

/*	Create the form widget which will manage the drawing_area and	*
 *	row_column widgets.						*/

  form = XtVaCreateWidget ("bypass_map_form",
                           xmFormWidgetClass, Top_widget,
                           XmNbackground,
                           hci_get_read_color (BACKGROUND_COLOR1), NULL);

/*	If low bandwidth, display a progress meter.			*/

  HCI_PM ("Read bypass map data");

  ORPGDA_write_permission( ORPGDAT_CLUTTERMAP );
  Init_status = hci_clutter_bypass_map_initialize (ORPGRDA_ORDA_CONFIG);

  /*  Exit if the user cancels I/O operations */
  if (Init_status == RMT_CANCELLED)
    HCI_task_exit (HCI_EXIT_SUCCESS);

  /* Register for bypass map updates */
  status = ORPGDA_UN_register (ORPGDAT_CLUTTERMAP,
                           LBID_BYPASSMAP_ORDA, hci_bypass_map_updated);

  if (status != LB_SUCCESS)
  {
    HCI_LE_error("Unable to register for bypass map updates (%d)", status);
  }

  if (Init_status > 0)
  {
/*	We need to check the date of the message.  If it is 0, the display*
 *	buffer has never been initialized with a valid bypass map.  We	*
 *	need to set the init flag back to 0 so we know we still need to	*
 *	initalize it when one is received from the RDA.			*/

    if (!hci_get_bypass_map_date ())
    {

      Init_status = 0;

    }
  }

/*	Create the row_column widget for the control buttons to 	*
 *	be placed along the left side of the form.			*/

  rowcol1 = XtVaCreateWidget ("bypass_map_rowcol",
                              xmRowColumnWidgetClass, form,
                              XmNtopAttachment, XmATTACH_FORM,
                              XmNleftAttachment, XmATTACH_FORM,
                              XmNorientation, XmHORIZONTAL,
                              XmNbackground,
                              hci_get_read_color (BACKGROUND_COLOR1),
                              XmNpacking, XmPACK_TIGHT, XmNisAligned, False,
                              XmNspacing, 1, NULL);

/*	Create the pushbuttons for the control selections.		*/

  button = XtVaCreateManagedWidget ("Close",
                                    xmPushButtonWidgetClass, rowcol1,
                                    XmNforeground,
                                    hci_get_read_color (BUTTON_FOREGROUND),
                                    XmNbackground,
                                    hci_get_read_color (BUTTON_BACKGROUND),
                                    XmNfontList, hci_get_fontlist (LIST),
                                    NULL);

  XtAddCallback (button,
                 XmNactivateCallback, hci_bypass_map_close_callback, NULL);

  Request_button = XtVaCreateManagedWidget ("Request",
                                            xmPushButtonWidgetClass, rowcol1,
                                            XmNforeground,
                                            hci_get_read_color
                                            (BUTTON_FOREGROUND),
                                            XmNbackground,
                                            hci_get_read_color
                                            (BUTTON_BACKGROUND), XmNfontList,
                                            hci_get_fontlist (LIST),
                                            XmNsensitive, True, NULL);

  XtAddCallback (Request_button,
                 XmNactivateCallback, hci_bypass_map_request_callback, NULL);

  XtManageChild (rowcol1);

  rowcol2 = XtVaCreateWidget ("bypass_map_rowcol",
                              xmRowColumnWidgetClass, form,
                              XmNtopAttachment, XmATTACH_WIDGET,
                              XmNtopWidget, rowcol1,
                              XmNleftAttachment, XmATTACH_FORM,
                              XmNrightAttachment, XmATTACH_FORM,
                              XmNorientation, XmHORIZONTAL,
                              XmNbackground,
                              hci_get_read_color (BACKGROUND_COLOR1),
                              XmNpacking, XmPACK_TIGHT, XmNisAligned, False,
                              XmNspacing, 1, NULL);


  XtVaCreateManagedWidget ("     Grid: ",
                           xmLabelWidgetClass, rowcol2,
                           XmNforeground,
                           hci_get_read_color (TEXT_FOREGROUND),
                           XmNbackground,
                           hci_get_read_color (BACKGROUND_COLOR1),
                           XmNfontList, hci_get_fontlist (LIST), XmNalignment,
                           XmALIGNMENT_CENTER, NULL);

  n = 0;

  XtSetArg (arg[n], XmNforeground, hci_get_read_color (TEXT_FOREGROUND));
  n++;
  XtSetArg (arg[n], XmNbackground, hci_get_read_color (BACKGROUND_COLOR1));
  n++;
  XtSetArg (arg[n], XmNorientation, XmHORIZONTAL);
  n++;

  grid_mode = XmCreateRadioBox (rowcol2, "grid_mode", arg, n);

  button = XtVaCreateManagedWidget ("On",
                                    xmToggleButtonWidgetClass, grid_mode,
                                    XmNselectColor,
                                    hci_get_read_color (WHITE), XmNforeground,
                                    hci_get_read_color (TEXT_FOREGROUND),
                                    XmNbackground,
                                    hci_get_read_color (BACKGROUND_COLOR1),
                                    XmNfontList, hci_get_fontlist (LIST),
                                    XmNset, True, NULL);

  XtAddCallback (button,
                 XmNvalueChangedCallback, hci_bypass_map_grid_callback,
                 (XtPointer) HCI_ON_FLAG);

  button = XtVaCreateManagedWidget ("Off",
                                    xmToggleButtonWidgetClass, grid_mode,
                                    XmNselectColor,
                                    hci_get_read_color (WHITE), XmNforeground,
                                    hci_get_read_color (TEXT_FOREGROUND),
                                    XmNbackground,
                                    hci_get_read_color (BACKGROUND_COLOR1),
                                    XmNfontList, hci_get_fontlist (LIST),
                                    XmNset, False, NULL);

  XtAddCallback (button,
                 XmNvalueChangedCallback, hci_bypass_map_grid_callback,
                 (XtPointer) HCI_OFF_FLAG);

  XtManageChild (grid_mode);

  XtManageChild (rowcol2);

/*	Create a row_column widget to control the segment number 	*
 *	below the top set of buttons.					*/

  frame = XtVaCreateManagedWidget ("bypass_map_frame",
                                   xmFrameWidgetClass, form,
                                   XmNforeground,
                                   hci_get_read_color (BACKGROUND_COLOR1),
                                   XmNbackground,
                                   hci_get_read_color (BACKGROUND_COLOR1),
                                   XmNtopAttachment, XmATTACH_WIDGET,
                                   XmNtopWidget, rowcol2, XmNleftAttachment,
                                   XmATTACH_FORM, XmNrightAttachment,
                                   XmATTACH_FORM, NULL);

  segment_rowcol = XtVaCreateWidget ("segment_rowcol",
                                     xmRowColumnWidgetClass, frame,
                                     XmNorientation, XmHORIZONTAL,
                                     XmNbackground,
                                     hci_get_read_color (BACKGROUND_COLOR1),
                                     XmNpacking, XmPACK_TIGHT, XmNisAligned,
                                     False, XmNspacing, 1, NULL);

/*	Create the pushbuttons for the control selections.		*/

  XtVaCreateManagedWidget ("Segment Number:",
                           xmLabelWidgetClass, segment_rowcol,
                           XmNforeground,
                           hci_get_read_color (TEXT_FOREGROUND),
                           XmNbackground,
                           hci_get_read_color (BACKGROUND_COLOR1),
                           XmNfontList, hci_get_fontlist (LIST), NULL);

  Segment_button = XtVaCreateManagedWidget ("1",
                                    xmPushButtonWidgetClass, segment_rowcol,
                                    XmNforeground,
                                    hci_get_read_color (BUTTON_FOREGROUND),
                                    XmNbackground,
                                    hci_get_read_color (BUTTON_BACKGROUND),
                                    XmNfontList, hci_get_fontlist (LIST),
                                    NULL);

  XtAddCallback (Segment_button,
                 XmNactivateCallback, hci_bypass_map_segment_callback, NULL);

  Generation_label = XtVaCreateManagedWidget ("Generated",
                                              xmLabelWidgetClass,
                                              segment_rowcol, XmNforeground,
                                              hci_get_read_color
                                              (TEXT_FOREGROUND),
                                              XmNbackground,
                                              hci_get_read_color
                                              (BACKGROUND_COLOR1),
                                              XmNfontList,
                                              hci_get_fontlist (LIST), NULL);

  XtManageChild (segment_rowcol);

/*	Create the drawing_area widget which will be used to display	*
 *	bypass map data.  It will occupy the right portion of the form.	*/

  actions.string = "bypass_map_input";
  actions.proc = (XtActionProc) bypass_map_input;
  XtAppAddActions (HCI_get_appcontext(), &actions, 1);

  Draw_widget = XtVaCreateWidget ("bypass_map_drawing_area",
                                  xmDrawingAreaWidgetClass, form,
                                  XmNwidth, Width,
                                  XmNheight, Height,
                                  XmNtopAttachment, XmATTACH_WIDGET,
                                  XmNtopWidget, frame,
                                  XmNleftAttachment, XmATTACH_FORM,
                                  XmNrightAttachment, XmATTACH_FORM,
                                  XmNtranslations,
                                  XtParseTranslationTable
                                  (Bypass_map_translations), NULL);

/*	Add an expose callback for the drawing_area in order to allow	*
 *	holes to be filled in the display when  other windows are moved	*
 *	across it.							*/

  XtAddCallback (Draw_widget,
                 XmNexposeCallback, hci_bypass_map_expose_callback, NULL);

/*	Permit the user to resize the bypass map window.		*/

  XtAddCallback (Draw_widget,
                 XmNresizeCallback, hci_bypass_map_resize_callback, NULL);

  XtManageChild (Draw_widget);

  frame = XtVaCreateManagedWidget ("bypass_map_frame",
                                   xmFrameWidgetClass, form,
                                   XmNforeground,
                                   hci_get_read_color (BACKGROUND_COLOR1),
                                   XmNbackground,
                                   hci_get_read_color (BACKGROUND_COLOR1),
                                   XmNtopAttachment, XmATTACH_WIDGET,
                                   XmNtopWidget, Draw_widget,
                                   XmNleftAttachment, XmATTACH_FORM,
                                   XmNrightAttachment, XmATTACH_FORM,
                                   XmNbottomAttachment, XmATTACH_FORM, NULL);

  XtVaCreateManagedWidget
    ("Zoom: Left + Right - Middle Center",
     xmLabelWidgetClass, frame, XmNforeground,
     hci_get_read_color (TEXT_FOREGROUND), XmNbackground,
     hci_get_read_color (TEXT_BACKGROUND), XmNfontList,
     hci_get_fontlist (LIST), NULL);

  XtManageChild (form);

  XtRealizeWidget (Top_widget);

/*	Define the various window variables to be used as arguments in 	*
 *	the various Xlib and Xt calls.					*/

  Bypass_map_window = XtWindow (Draw_widget);
  Bypass_map_pixmap = XCreatePixmap (Bypass_map_display,
                                     Bypass_map_window,
                                     Width, Height, Bypass_map_depth);

  gcv.foreground = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
  gcv.background = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
  gcv.fill_rule = EvenOddRule;
  gcv.graphics_exposures = FALSE;

  Bypass_map_gc = XCreateGC (Bypass_map_display,
                             Bypass_map_window,
                             GCBackground | GCForeground | GCGraphicsExposures
                             | GCFillRule, &gcv);

  XtVaSetValues (Draw_widget,
                 XmNbackground, hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
                 NULL);

/*	Clear the data display portion of the window by filling it with	*
 *	the background color.						*/

  XSetForeground (Bypass_map_display,
                  Bypass_map_gc,
                  hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
  XFillRectangle (Bypass_map_display,
                  Bypass_map_pixmap, Bypass_map_gc, 0, 0, Width, Height);

/*	Define the data display window properties.			*/

  Center_pixel = Width / 2;
  Center_scanl = Height / 2;
  Scale_x = Zoom_factor * Width / (2.0 * BYPASS_MAP_RANGE_MAX);
  Scale_y = -Scale_x;
  XSetFont (Bypass_map_display, Bypass_map_gc, hci_get_font (LIST));

  XtPopup (Top_widget, XtGrabNone);

  hci_bypass_map_resize_callback ((Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL);

  update_sensitivities();

  /* Start HCI loop. */

  HCI_start( timer_proc, HCI_TWO_SECONDS, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the Clutter Bypass	*
 *		     Map Display window receives an expose event.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_expose_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data)
{
  XCopyArea (Bypass_map_display,
             Bypass_map_pixmap,
             Bypass_map_window, Bypass_map_gc, 0, 0, Width, Height, 0, 0);

}

/************************************************************************
 *	Description: This function is activated when the Clutter Bypass	*
 *		     Map Display window is resized.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_resize_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data)
{
  time_t j_seconds;
  int month, day, year;
  int hour, minute, second;
  XmString str;

/*	Get the new size of the base data window.			*/

  if ((Draw_widget == (Widget) NULL) ||
      (Bypass_map_display == (Display *) NULL) ||
      (Bypass_map_window == (Window) NULL) ||
      (Bypass_map_pixmap == (Pixmap) NULL) || (Bypass_map_gc == (GC) NULL))
  {

    return;

  }

  XtVaGetValues (Draw_widget, XmNwidth, &Width, XmNheight, &Height, NULL);

/*	Destroy the old pixmap since the size has changed and create	*
 *	create a new one.						*/

  if (Bypass_map_pixmap != (Window) NULL)
  {

    XFreePixmap (Bypass_map_display, Bypass_map_pixmap);

  }

  Bypass_map_pixmap = XCreatePixmap (Bypass_map_display,
                                     Bypass_map_window,
                                     Width, Height, Bypass_map_depth);

/*	Recompute the scale factors (in pixels/km) and a new window	*
 *	center pixel/scanline coordinate.				*/

  Scale_x = Zoom_factor * Width / (2.0 * BYPASS_MAP_RANGE_MAX);
  Scale_y = -Scale_x;
  Center_pixel = Width / 2;
  Center_scanl = Height / 2;

  XSetForeground (Bypass_map_display, Bypass_map_gc,
                  hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
  XFillRectangle (Bypass_map_display,
                  Bypass_map_pixmap, Bypass_map_gc, 0, 0, Width, Height);

/*	Now display the bypass map data.				*/

  hci_display_bypass_map ();

/*	If the grid overlay flag is set display the polar grid.		*/

  if (Grid_flag == HCI_ON_FLAG)
  {

    hci_bypass_map_overlay_grid ();

  }

/*	Display the Bypass Map Generation Data/Time in the upper	*
 *	right corner of the display window.				*/

  if ( hci_get_bypass_map_date() == 0)
  {

    sprintf (Buf, "  Generation Time: N/A");

  }
  else
  {

    /*
     Get date/time and convert to seconds. Date is julian
     since 1/1/70. Time is minutes past midnight.
    */

    j_seconds = ( ( hci_get_bypass_map_date() - 1 ) * HCI_SECONDS_PER_DAY ) + ( hci_get_bypass_map_time() * 60 );

    unix_time (&j_seconds, &year, &month, &day, &hour, &minute, &second);

    sprintf (Buf, "     Generated: %2.2d/%2.2d/%4.4d at %2.2d:%2.2d",
             month, day, year, hour, minute);

  }

  str = XmStringCreateLocalized (Buf);

  XtVaSetValues (Generation_label, XmNlabelString, str, NULL);

  XmStringFree (str);

/*	Call the expose callback to write the base data pixmap to the	*
 *	base data window.						*/

  hci_bypass_map_expose_callback ((Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL);

}

/************************************************************************
 *	Description: This function is activated when a mouse event is	*
 *		     generated inside the clutter bypass map display	*
 *		     region.						*
 *									*
 *	Input:  w - widget ID						*
 *		event - X event data					*
 *		args - user arguments					*
 *		num_args - number of user arguments			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
bypass_map_input (
  Widget w,
  XEvent * event,
  String * args,
  int *num_args)
{
  int pixel, scanl;
  double x1, y1;
  float azimuth, range;
  static int value;
  XFontStruct *fontinfo;
  int height;
  int width;
  static int button_down = 0;

  float hci_find_azimuth (
  int pixel1,
  int scanl1,
  int pixel2,
  int scanl2);

/*	If we do not have a valid bypass map to display, then do nothing*
 *	and return;							*/

  if (Init_status <= 0)
  {

    return;

  }

/*	We have a valid bypass map to display so lets start by converting*
 *	cursor coordiante to polar coordinate.				*/

  azimuth = hci_find_azimuth (event->xbutton.x,
                              event->xbutton.y,
                              (int) (Center_pixel + X_offset * Scale_x),
                              (int) (Center_scanl + Y_offset * Scale_y));

  x1 = (Center_pixel - event->xbutton.x) / Scale_x + X_offset;
  y1 = (Center_scanl - event->xbutton.y) / Scale_y + Y_offset;

  range = sqrt ((double) (x1 * x1 + y1 * y1));

/*	Display the azran position of the cursor in the upper left	*
 *	corner of the display window.					*/

  XSetForeground (Bypass_map_display,
                  Bypass_map_gc, hci_get_read_color (TEXT_FOREGROUND));
  XSetBackground (Bypass_map_display,
                  Bypass_map_gc, hci_get_read_color (TEXT_BACKGROUND));

  sprintf (Buf, "(%3d Deg,%3d NM)", (int) azimuth, (int) (range * HCI_KM_TO_NM));

  fontinfo = hci_get_fontinfo (LIST);

  width = XTextWidth (fontinfo, Buf, strlen (Buf));
  height = fontinfo->ascent + fontinfo->descent;

  scanl = height + 2;

  XDrawImageString (Bypass_map_display,
                    Bypass_map_pixmap,
                    Bypass_map_gc, (int) 10, (int) scanl, Buf, strlen (Buf));

  XDrawRectangle (Bypass_map_display,
                  Bypass_map_pixmap,
                  Bypass_map_gc,
                  (int) 9,
                  (int) (scanl - height + 2),
                  (int) (width + 1), (int) (height + 1));

  XDrawImageString (Bypass_map_display,
                    Bypass_map_window,
                    Bypass_map_gc, (int) 10, (int) scanl, Buf, strlen (Buf));

  XDrawRectangle (Bypass_map_display,
                  Bypass_map_window,
                  Bypass_map_gc,
                  (int) 9,
                  (int) (scanl - height + 2),
                  (int) (width + 1), (int) (height + 1));

  pixel = event->xbutton.x;
  scanl = event->xbutton.y;

/*	If a button is down the last time this function was invoked,	*
 *	we need to see if it is still down.  If not, then we need to	*
 *	terminate a set/clear operation.				*/

  if (button_down)
  {

    hci_set_bypass_map_data (azimuth, range, Segment, value);
    hci_display_bypass_map_data (azimuth, range, value);

    if ((!strcmp (args[0], "up1")) ||
        (!strcmp (args[0], "up2")) || (!strcmp (args[0], "up3")))
    {

      button_down = 0;

    }
  }

  if ((!strcmp (args[0], "down1")) ||
      (!strcmp (args[0], "down2")) || (!strcmp (args[0], "down3")))
  {

    if (!strcmp (args[0], "down1"))
    {

      Zoom_factor = Zoom_factor * 2;

      if (Zoom_factor > ZOOM_FACTOR_MAX)
      {

        Zoom_factor = ZOOM_FACTOR_MAX;

      }

    }
    else if (!strcmp (args[0], "down3"))
    {

      Zoom_factor = Zoom_factor / 2;

      if (Zoom_factor < 1)
      {

        Zoom_factor = 1;

      }
    }

    X_offset = (Center_pixel - pixel) / Scale_x + X_offset;
    Y_offset = (Center_scanl - scanl) / Scale_y + Y_offset;
    Scale_x = Zoom_factor * Width / (2.0 * BYPASS_MAP_RANGE_MAX);
    Scale_y = -Scale_x;

    hci_bypass_map_resize_callback ((Widget) NULL,
                                    (XtPointer) NULL, (XtPointer) NULL);

  }
  else if ((!strcmp (args[0], "up1")) ||
           (!strcmp (args[0], "up2")) || (!strcmp (args[0], "up3")))
  {

    button_down = 0;

  }
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_close_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data)
{
  HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This function is used to display a clutter bypass	*
 *		     map element in the clytter bypass map display	*
 *		     region.						*
 *									*
 *	Input:  azimuth - azimuth angle (from radar) of data element	*
 *		range - range (km from radar) of data element		*
 *		value - (0 or 1)					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
hci_display_bypass_map_data (
  float azimuth,
  float range,
  int value)
{
  float azimuth1 = -1.0;
  double sin1, sin2, cos1, cos2;
  XPoint X[5];

  range = (float) ((int) range);

  azimuth1 = (int) ((azimuth + ORDA_AZINT)) - ORDA_AZINT;
  sin1 = sin ((double) (azimuth1 + 90) * HCI_DEG_TO_RAD);
  cos1 = cos ((double) (azimuth1 - 90) * HCI_DEG_TO_RAD);

  azimuth1 = ((int) (azimuth + ORDA_AZINT)) + ORDA_AZINT;
  sin2 = sin ((double) (azimuth1 + 90) * HCI_DEG_TO_RAD);
  cos2 = cos ((double) (azimuth1 - 90) * HCI_DEG_TO_RAD);

  X[0].x = (range * cos1 + X_offset) * Scale_x + Center_pixel - sin1;
  X[0].y = (range * sin1 + Y_offset) * Scale_y + Center_scanl - cos1;
  X[1].x = (range * cos2 + X_offset) * Scale_x + Center_pixel + sin1;
  X[1].y = (range * sin2 + Y_offset) * Scale_y + Center_scanl + cos1;
  range++;

  X[2].x = (range * cos2 + X_offset) * Scale_x + Center_pixel + sin1;
  X[2].y = (range * sin2 + Y_offset) * Scale_y + Center_scanl + cos1;
  X[3].x = (range * cos1 + X_offset) * Scale_x + Center_pixel - sin1;
  X[3].y = (range * sin1 + Y_offset) * Scale_y + Center_scanl - cos1;
  X[4].x = X[0].x;
  X[4].y = X[0].y;

  if (value)
  {

    XSetForeground (Bypass_map_display,
                    Bypass_map_gc,
                    hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

  }
  else
  {

    XSetForeground (Bypass_map_display,
                    Bypass_map_gc, hci_get_read_color (BACKGROUND_COLOR2));

  }

  XFillPolygon (Bypass_map_display,
                Bypass_map_window,
                Bypass_map_gc, X, 5, Convex, CoordModeOrigin);

  XFillPolygon (Bypass_map_display,
                Bypass_map_pixmap,
                Bypass_map_gc, X, 5, Convex, CoordModeOrigin);
}

/************************************************************************
 *	Description: This function displays the clutter bypass map in	*
 *		     the clutter bypass map display region.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_display_bypass_map (
  )
{
  float azimuth = -1.0;
  float range;
  int i, j, k;
  double sin1, sin2, cos1, cos2;
  XPoint X[5];
  int new_radial;
  int value = -1;
  int old_value = -1;
  int num_of_radials = -1;

/*	If the display buffer has not been initialized, display a message*
 *	that data are not available and return.				*/

  if (Init_status <= 0)
  {

    XSetForeground (Bypass_map_display,
                    Bypass_map_gc, hci_get_read_color (WHITE));

    XDrawString (Bypass_map_display,
                 Bypass_map_pixmap,
                 Bypass_map_gc,
                 (int) (Center_pixel - 100),
                 (int) (Center_scanl + 4),
                 "Bypass Map Data Not Available", 29);

    hci_bypass_map_expose_callback ((Widget) NULL,
                                    (XtPointer) NULL, (XtPointer) NULL);

    return;

  }

/*	This routine is much more efficient than the other routine	*
 *	(hci_display_bypass_map_data ()) as it only paints a polygon	*
 *	when the data value along the beam changes.  For instance, lets	*
 *	say that if you could group bins along a beam by like value and	*
 *	that there are 20 groups.  Only 20 XPolygon fill operations	*
 *	would be needed instead of 512 (32*16); a considerable increase	*
 *	in performance.  NOTE:  Each segment end point has been		*
 *	adjusted by either the sine or cosine value for that radial.	*
 *	If this isn't done, artifats appear in the display due to	*
 *	the conersion from real to screen (integer) coordinates.  This	*
 *	problem isn't noticable when the data for each gate if displayed*
 *	individually.  However, performance would be sacrificed.	*/

  azimuth = 360.0 - ORDA_AZINT;
  num_of_radials = ORDA_BYPASS_MAP_RADIALS;
  sin2 = sin ((double) (azimuth + 90) * HCI_DEG_TO_RAD);
  cos2 = cos ((double) (azimuth - 90) * HCI_DEG_TO_RAD);

/*	For each radial in the bypass map, display the foreground color	*
 *	check each bin to see if bypass map filtering is on (0) or	*
 *	off (1).							*/

  for (i = 0; i < num_of_radials; i++)
  {

    new_radial = 1;
    azimuth = i + ORDA_AZINT;

    sin1 = sin2;
    cos1 = cos2;
    sin2 = sin ((double) (azimuth + 90) * HCI_DEG_TO_RAD);
    cos2 = cos ((double) (azimuth - 90) * HCI_DEG_TO_RAD);

    for (j = 0; j < 32; j++)
    {

      for (k = 0; k < 16; k++)
      {

        range = j * 16 + k + 1;

        value = hci_get_bypass_map_data((azimuth - ORDA_AZINT), range, Segment);

        if (new_radial)
        {

          X[0].x = X_offset * Scale_x + Center_pixel;
          X[0].y = Y_offset * Scale_y + Center_scanl;
          X[1].x = X_offset * Scale_x + Center_pixel;
          X[1].y = Y_offset * Scale_y + Center_scanl;
          X[4].x = X_offset * Scale_x + Center_pixel;
          X[4].y = Y_offset * Scale_y + Center_scanl;
          new_radial = 0;
          old_value = value;

        }

        X[2].x = (range * cos2 + X_offset) * Scale_x + Center_pixel + sin1;
        X[2].y = (range * sin2 + Y_offset) * Scale_y + Center_scanl + cos1;
        X[3].x = (range * cos1 + X_offset) * Scale_x + Center_pixel - sin1;
        X[3].y = (range * sin1 + Y_offset) * Scale_y + Center_scanl - cos1;

        if (value != old_value)
        {

          if (old_value)
          {

            XSetForeground (Bypass_map_display,
                            Bypass_map_gc,
                            hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

          }
          else
          {

            XSetForeground (Bypass_map_display,
                            Bypass_map_gc,
                            hci_get_read_color (BACKGROUND_COLOR2));

          }

          old_value = value;

          XFillPolygon (Bypass_map_display,
                        Bypass_map_pixmap,
                        Bypass_map_gc, X, 5, Complex, CoordModeOrigin);

          X[0].x = X[3].x;
          X[0].y = X[3].y;
          X[1].x = X[2].x;
          X[1].y = X[2].y;
          X[4].x = X[0].x;
          X[4].y = X[0].y;

        }
      }
    }

    if (old_value)
    {

      XSetForeground (Bypass_map_display,
                      Bypass_map_gc,
                      hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

    }
    else
    {

      XSetForeground (Bypass_map_display,
                      Bypass_map_gc, hci_get_read_color (BACKGROUND_COLOR2));

    }

    old_value = value;

    XFillPolygon (Bypass_map_display,
                  Bypass_map_pixmap,
                  Bypass_map_gc, X, 5, Complex, CoordModeOrigin);

  }

  hci_bypass_map_expose_callback ((Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the "Grid" radio buttons.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - ON or OFF					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_grid_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data)
{
  XmToggleButtonCallbackStruct *state =
    (XmToggleButtonCallbackStruct *) call_data;

  if (state->set)
  {

    Grid_flag = (int) client_data;

  }

  hci_bypass_map_resize_callback ((Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL);

}

/************************************************************************
 *	Description: This function is used to display a polar grid	*
 *		     in the clutter bypass map display region.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_overlay_grid (
  )
{
  int i;
  int size;
  int pixel;
  int scanl;
  int step;
  int spoke_interval;
  char buf[32];
  float x, y;
  int start_range;
  int stop_range;

  XSetForeground (Bypass_map_display, Bypass_map_gc,
                  hci_get_read_color (YELLOW));

/*	The ring interval is dependent on zoom factor.			*/

  if (Zoom_factor > 8)
  {

    step = 5;
    spoke_interval = 10;

  }
  else if (Zoom_factor > 4)
  {

    step = 10;
    spoke_interval = 10;

  }
  else if (Zoom_factor > 2)
  {

    step = 25;
    spoke_interval = 30;

  }
  else
  {

    step = 50;
    spoke_interval = 45;

  }

/*	First display the circles outward from the radar center.	*/

  x = fabs ((double) X_offset);
  y = fabs ((double) Y_offset);

  if (x > y)
  {

    start_range = x - BYPASS_MAP_RANGE_MAX / Zoom_factor;
    stop_range = x + BYPASS_MAP_RANGE_MAX / Zoom_factor;

  }
  else
  {

    start_range = y - BYPASS_MAP_RANGE_MAX / Zoom_factor;
    stop_range = y + BYPASS_MAP_RANGE_MAX / Zoom_factor;

  }

  start_range = (start_range / step) * step;
  stop_range = (stop_range / step) * step + step;

  if (start_range < step)
  {

    start_range = step;

  }

  stop_range = BYPASS_MAP_RANGE_MAX * HCI_KM_TO_NM;

  for (i = step; i <= stop_range; i = i + step)
  {

    size = i * Scale_x / HCI_KM_TO_NM;

    pixel = Center_pixel + X_offset * Scale_x - size;
    scanl = Center_scanl + Y_offset * Scale_y - size;

    XDrawArc (Bypass_map_display,
              Bypass_map_window,
              Bypass_map_gc,
              pixel, scanl, size * 2, size * 2, 0, -(360 * 64));

    XDrawArc (Bypass_map_display,
              Bypass_map_pixmap,
              Bypass_map_gc,
              pixel, scanl, size * 2, size * 2, 0, -(360 * 64));

    sprintf (buf, "%i nm", i);

    XDrawString (Bypass_map_display,
                 Bypass_map_pixmap,
                 Bypass_map_gc,
                 (int) (Center_pixel +
                        X_offset * Scale_x -
                        4 * strlen (buf)), scanl + 4, buf, strlen (buf));

  }

/*	Next display the "spokes" outward from the center.		*/

  for (i = 0; i < 360; i = i + spoke_interval)
  {

    pixel = (int) ((BYPASS_MAP_RANGE_MAX *
                    cos ((double) (i + 90) * HCI_DEG_TO_RAD) +
                    X_offset) * Scale_x + Center_pixel);
    scanl = (int) ((BYPASS_MAP_RANGE_MAX *
                    sin ((double) (i - 90) * HCI_DEG_TO_RAD) +
                    Y_offset) * Scale_y + Center_scanl);

    XDrawLine (Bypass_map_display,
               Bypass_map_pixmap,
               Bypass_map_gc,
               (int) (Center_pixel +
                      X_offset * Scale_x),
               (int) (Center_scanl + Y_offset * Scale_y), pixel, scanl);

  }

  hci_bypass_map_expose_callback ((Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Request" button. A confirmation popup is	*
 *		     displayed for the user to either accept or cancel	*
 *		     the request.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_request_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data)
{
  char buf[ HCI_BUF_128 ];

  HCI_LE_log("Request new Clutter Bypass Map");

 /* Display a confirmation popup window. */
  sprintf( buf, "You are about to request a new Clutter Bypass Map from the RDA.\nDo you want to continue?" );
  hci_confirm_popup( Top_widget, buf, accept_bypass_map_request_callback, cancel_bypass_map_request_callback );
}

/************************************************************************
 *      Description: This function is activated when the user selects	*
 *		     "Yes" on the request confirmation popup.		*
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
accept_bypass_map_request_callback(
  Widget w,
  XtPointer client_data,
  XtPointer call_data )
{
  Request_flag = HCI_YES_FLAG;
}

void bypass_map_request()
{
  HCI_LE_log("Request new Clutter Bypass Map Accepted");

  sprintf (Cmd, "Request new Clutter Bypass Map");

  HCI_display_feedback( Cmd );

  HCI_PM ("Request RDA bypass map");

  ORPGRDA_send_cmd (COM4_REQRDADATA,
                    (int) HCI_INITIATED_RDA_CTRL_CMD,
                    (int) DREQ_CLUTMAP,
                    (int) 0, (int) 0, (int) 0, (int) 0, NULL);
}

/************************************************************************
 *      Description: This function is activated when the user selects	*
 *		     "No" on the request confirmation popup.		*
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
cancel_bypass_map_request_callback(
  Widget w,
  XtPointer client_data,
  XtPointer call_data )
{
  HCI_LE_log("Request new Clutter Bypass Map Canceled");
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the segment number button.  The segment number	*
 *		     is incremented until it is greater than the	*
 *		     number of segments in the clutter bypass map	*
 *		     where it is set to the first segment.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_segment_callback (
  Widget w,
  XtPointer client_data,
  XtPointer call_data)
{
  XmString str;

  Segment++;

  if (Segment >= hci_clutter_bypass_map_segments ())
  {

    Segment = 0;

  }

  sprintf (Buf, "%d", Segment + 1);

  str = XmStringCreateLocalized (Buf);

  XtVaSetValues (w, XmNlabelString, str, NULL);

  XmStringFree (str);

  hci_bypass_map_resize_callback ((Widget) NULL,
                                  (XtPointer) NULL, (XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is the timer procedure for the	*
 *		     Clutter Bypass Map Display window.			*
 *									*
 *	Input:  w - widget ID						*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
  int flag = 0;
  int rc = 0;

/* If the RDA configuration changes, call function to display
   popup window and exit. */

   if( ORPGRDA_get_rda_config( NULL ) != ORPGRDA_ORDA_CONFIG )
   {
     rda_config_change();
   }

   if( Request_flag == HCI_YES_FLAG )
   {
     Request_flag = HCI_NO_FLAG;
     bypass_map_request();
   }

/*	Update the local copy of the clutter bypass map if it hasn't	*
 *	been initialized or has been updated by another user.		*/

  flag = 0;

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {

    if( ( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) ==
         ORPGRED_CHANNEL_INACTIVE )
        && ( ORPGRED_channel_state( ORPGRED_OTHER_CHANNEL ) ==
            ORPGRED_CHANNEL_ACTIVE )
        && ( ORPGRED_rpg_rpg_link_state () == ORPGRED_CHANNEL_LINK_UP ) )
    {
      flag = 1;
    }
  }

  if( ( Init_status <= 0 ) && ( flag == 0 ) )
  {
    if( Bypass_map_update_flag )
    {
      Init_status = hci_clutter_bypass_map_read( LBID_BYPASSMAP_ORDA );

      if( Init_status <= 0 )
      {
        HCI_LE_error("bypass map read failed (%d)", Init_status);
      }
      else
      {
        hci_bypass_map_resize_callback( (Widget) NULL,
                                        (XtPointer) NULL, (XtPointer) NULL );
        Bypass_map_update_flag = 0;
      }
    }
  }
  else if( Bypass_map_update_flag && ( flag == 0 ) )
  {
    rc = hci_clutter_bypass_map_read( LBID_BYPASSMAP_ORDA );

    if( rc <= 0 )
    {
      HCI_LE_error("bypass map read failed (%d)", rc);
    }
    else
    {
      hci_bypass_map_resize_callback( (Widget) NULL,
                                      (XtPointer) NULL, (XtPointer) NULL );
      Bypass_map_update_flag = 0;
    }
  }

  update_sensitivities();
}

/************************************************************************
 *	Description: This function is activated when a message in the	*
 *		     clutter bypass map LB is updated.			*
 *									*
 *	Input:  fd - LB file descriptor					*
 *		msg_id - ID of message that was updated			*
 *		msg_info - lenght (bytes) of updated message		*
 *		arg - user registered argument (unused)			* 
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_bypass_map_updated (
  int fd,
  LB_id_t msg_id,
  int msg_info,
  void *arg)
{
  Bypass_map_update_flag = 1;
}

/************************************************************************
 *      Description: This function updates button sensitivities.        *
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
update_sensitivities()
{
  if( ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT ) != RS_CONNECTED )
  {
    XtVaSetValues( Request_button, XmNsensitive, False, NULL );
  }
  else
  {
    XtVaSetValues( Request_button, XmNsensitive, True, NULL );
  }

  if( Init_status <= 0 )
  {
    XtVaSetValues( Segment_button, XmNsensitive, False, NULL );
  }
  else
  {
    XtVaSetValues( Segment_button, XmNsensitive, True, NULL );
  }
}

/************************************************************************
 *  Description: This function is called when the RDA configuration     *
 *         changes.                                                     *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

void rda_config_change()
{
   if( Top_widget != (Widget) NULL && !config_change_popup )
   {
     config_change_popup = 1; /* Indicate popup has been launched. */
     hci_rda_config_change_popup();
   }
   else
   {
     return;
   }
}


