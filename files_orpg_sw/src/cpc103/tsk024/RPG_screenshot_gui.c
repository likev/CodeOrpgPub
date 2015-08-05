/************************************************************************

      The main source file for GUI wrapper to GNOME screenshot utility.

************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/10/03 21:49:38 $
 * $Id: RPG_screenshot_gui.c,v 1.4 2011/10/03 21:49:38 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* Include files. */

#include <hci.h>

/* Definitions. */

#define NUM_LE_LOG_MESSAGES	200
#define	TEMP_BUF		512
#define	MIN_DELAY_COUNT		1
#define	MAX_DELAY_COUNT		10

enum {NO_ACTION_FLAG, SNAP_SCREEN_FLAG, SNAP_WINDOW_FLAG};

/* Global variables. */

static Widget		Top_widget = ( Widget ) NULL;
static Widget		Screen_button = ( Widget ) NULL;
static Widget		Window_button = ( Widget ) NULL;
static Widget		Close_button = ( Widget ) NULL;
static int		Action_flag = NO_ACTION_FLAG;
static int		Delay_count = MIN_DELAY_COUNT;

/* Function prototypes. */

static int Destroy_callback( Widget, XtPointer, XtPointer );
static void Close_callback( Widget, XtPointer, XtPointer );
static void Set_action_flag( Widget, XtPointer, XtPointer );
static void Error_popup( char * );
static void Take_snapshot();
static void Reset_gui();
static void Delay_slider_callback( Widget, XtPointer, XtPointer );
static void Timer_proc();

/**************************************************************************

    The main function.

**************************************************************************/

int main( int argc, char **argv )
{
  Widget	form = ( Widget ) NULL;
  Widget	form_rowcol = ( Widget ) NULL;
  Widget	label = ( Widget ) NULL;
  Widget	slider_frame = ( Widget ) NULL;
  Widget	slider_rowcol = ( Widget ) NULL;
  Widget	delay_count_slider = ( Widget ) NULL;
  Widget	buttons_rowcol = ( Widget ) NULL;

  /* Initialize HCI. */
  
  HCI_partial_init( argc, argv, -1 );
  
  Top_widget = HCI_get_top_widget();
  
  HCI_set_destroy_callback( Destroy_callback );
  
  XtVaSetValues( Top_widget, XmNtitle, "Screenshot Utility", NULL );

  /* Ensure write permission for ORPGDAT_HCI_DATA. */

  ORPGDA_write_permission( ORPGDAT_HCI_DATA );

  /* Define form to hold the widgets. */

  form = XtVaCreateManagedWidget( "mode_list_form",
           xmFormWidgetClass, Top_widget,
           NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                  xmRowColumnWidgetClass, form,
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNorientation, XmVERTICAL,
                  XmNpacking, XmPACK_TIGHT,
                  XmNnumColumns, 1,
                  XmNspacing, 20,
                  XmNmarginHeight, 10,
                  NULL );

  /* Top-level label describing what to do. */

  label = XtVaCreateManagedWidget( "For screenshot of entire screen, click \"Snap Screen\".\nFor screenshot of highlighted window, click \"Snap Window\".\nTo increase the delay before a screen shot is created,\nadjust the slider below.",
            xmLabelWidgetClass, form_rowcol,
            XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
            XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
            XmNfontList, hci_get_fontlist( LIST ),
            NULL );

  /* Slider for user to pick snap shot delay. */

  slider_frame = XtVaCreateManagedWidget( "slider_frame",
                   xmFrameWidgetClass, form_rowcol,
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNtopAttachment, XmATTACH_WIDGET,
                   XmNtopWidget, label,
                   XmNleftAttachment, XmATTACH_FORM,
                   XmNrightAttachment, XmATTACH_FORM,
                   NULL );

  label = XtVaCreateManagedWidget( "Snap Delay (seconds)",
            xmLabelWidgetClass, slider_frame,
            XmNchildType, XmFRAME_TITLE_CHILD,
            XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
            XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
            XmNfontList, hci_get_fontlist( LIST ),
            XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
            XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
            NULL );

  slider_rowcol = XtVaCreateManagedWidget( "slider_rowcol",
                    xmRowColumnWidgetClass, slider_frame,
                    XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                    XmNorientation, XmHORIZONTAL,
                    XmNpacking, XmPACK_TIGHT,
                    XmNnumColumns, 1,
                    XmNmarginWidth, 10,
                    NULL );

  delay_count_slider = XtVaCreateManagedWidget ("delay count slider",
      xmScaleWidgetClass, slider_rowcol,
      XtVaTypedArg, XmNtitleString, XmRString, "Delay (seconds)", 16,
      XmNshowValue, True,
      XmNminimum, (int) MIN_DELAY_COUNT,
      XmNmaximum, (int) MAX_DELAY_COUNT,
      XmNvalue, (int) Delay_count,
      XmNorientation, XmHORIZONTAL,
      XmNdecimalPoints, 0,
      XmNscaleMultiple, 1,
      XmNwidth, 500,
      XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList, hci_get_fontlist( LIST ),
      NULL );

  XtAddCallback( delay_count_slider,
                 XmNvalueChangedCallback, Delay_slider_callback,
                 NULL );

  /* Control/action buttons. */

  buttons_rowcol = XtVaCreateManagedWidget( "buttons_rowcol",
                     xmRowColumnWidgetClass, form_rowcol,
                     XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                     XmNorientation, XmHORIZONTAL,
                     XmNnumColumns, 1,
                     XmNpacking, XmPACK_COLUMN,
                     XmNtopAttachment, XmATTACH_WIDGET,
                     XmNtopWidget, slider_frame,
                     XmNleftAttachment, XmATTACH_FORM,
                     XmNrightAttachment, XmATTACH_FORM,
                     XmNmarginWidth, 10,
                     XmNspacing, 57,
                     NULL);

  Screen_button = XtVaCreateManagedWidget( " Snap Screen ",
                    xmPushButtonWidgetClass, buttons_rowcol,
                    XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                    XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                    XmNfontList, hci_get_fontlist( LIST ),
                    NULL );

  XtAddCallback( Screen_button,
                 XmNactivateCallback, Set_action_flag,
                 (XtPointer) SNAP_SCREEN_FLAG );

  Window_button = XtVaCreateManagedWidget( " Snap Window ",
                    xmPushButtonWidgetClass, buttons_rowcol,
                    XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                    XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                    XmNfontList, hci_get_fontlist( LIST ),
                    NULL );

  XtAddCallback( Window_button,
                 XmNactivateCallback, Set_action_flag,
                 (XtPointer) SNAP_WINDOW_FLAG );

  Close_button = XtVaCreateManagedWidget( "    Close    ",
                   xmPushButtonWidgetClass, buttons_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL );

  XtAddCallback( Close_button,
                 XmNactivateCallback, Close_callback,
                 NULL );

  /* Display gui to screen. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */
  
  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/***************************************************************************/
/* SCREENSHOT_DESTROY(): Callback for main widget when it is destroyed.    */
/***************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In destroy callback" );
  return HCI_OK_TO_EXIT;
}

/***************************************************************************/
/* SCREENSHOT_CLOSE(): Callback for "Close" button. Destroys main widget.  */
/***************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In close callback" );
  XtDestroyWidget( Top_widget );
}

/***************************************************************************/
/* SET_ACTION_FLAG(): Callback for "Snap Screen" and "Snap Window" buttons.*/
/***************************************************************************/

static void Set_action_flag( Widget w, XtPointer x, XtPointer y )
{
  /* Set action flag. */

  Action_flag = (int) x;
  HCI_LE_log( "Action flag set to %d", Action_flag );

  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* De-sensitize action buttons. */

  XtVaSetValues( Screen_button, XmNsensitive, False, NULL );
  XtVaSetValues( Window_button, XmNsensitive, False, NULL );
  XtVaSetValues( Close_button, XmNsensitive, False, NULL );
}

/***************************************************************************/
/* TIMER_PROC(): Callback for event timer.                                 */
/***************************************************************************/

static void Timer_proc()
{
  if( Action_flag != NO_ACTION_FLAG )
  {
    HCI_LE_log( "Calling Take_snapshot()" );
    Take_snapshot();
    HCI_LE_log( "Calling Reset_gui()" );
    Reset_gui();
  }
}

/***************************************************************************/
/* DELAY_SLIDER_CALLBACK(): Callback for delay count slider.               */
/***************************************************************************/

static void Delay_slider_callback( Widget w, XtPointer x, XtPointer y )
{
  XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *) y;
  Delay_count = (int) cbs->value;
  HCI_LE_log( "Delay set to %d seconds", Delay_count);
}

/***************************************************************************/
/* ERROR_POPUP(): Build/display pop-up with error message.                 */
/***************************************************************************/

static void Error_popup( char *msg_buf )
{
  Widget err_dialog;
  Widget temp_widget;
  Widget ok_button;
  XmString ok;
  XmString msg;

  ok = XmStringCreateLocalized( "OK" );

  msg = XmStringCreateLtoR( msg_buf, XmFONTLIST_DEFAULT_TAG );

  err_dialog = XmCreateInformationDialog( Top_widget, "Error", NULL, 0 );

  /* Get rid of Cancel and Help buttons on popup. */

  temp_widget = XmMessageBoxGetChild( err_dialog, XmDIALOG_CANCEL_BUTTON );
  XtUnmanageChild( temp_widget );

  temp_widget = XmMessageBoxGetChild( err_dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of OK button. */

  ok_button = XmMessageBoxGetChild( err_dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          NULL );

  /* Set properties of popup. */

  XtVaSetValues (err_dialog,
          XmNmessageString, msg,
          XmNokLabelString, ok,
          XmNbackground, hci_get_read_color( WARNING_COLOR ),
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
          XmNdeleteResponse, XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok );
  XmStringFree( msg );

  /* Do this to make error popup appear. */

  XtManageChild( err_dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/***************************************************************************/
/* TAKE_SNAPSHOT: Take snapshot using GNOME snapshot utility.              */
/***************************************************************************/

static void Take_snapshot()
{
  char cmd[ TEMP_BUF ] = "";
  char output_buf[ TEMP_BUF ] = "";
  char error_buf[ 2*TEMP_BUF ] = "";
  char *username = NULL;
  int ret_code = 0;
  int n_bytes = 0;

  username = getenv( "USERNAME" );
  if( username == NULL )
  {
    sprintf( error_buf, "Cannot determine USERNAME" );
    HCI_LE_error( "Cannot determine USERNAME" );
    Error_popup( error_buf );
  }

  sprintf( cmd, "sudo -u %s gnome-screenshot --sm-disable --delay=%d ", username, Delay_count );

  if( Action_flag == SNAP_WINDOW_FLAG )
  {
    strcat( cmd, " --window " );
  }

  HCI_LE_log( "CMD: %s", cmd);
  ret_code = MISC_system_to_buffer( cmd, output_buf, TEMP_BUF, &n_bytes ) >> 8;
  HCI_LE_log( "CMD return code: %d", ret_code );
  if( ret_code != 0 )
  {
    sprintf( error_buf, "Snapshot failed (%d)\n\n%s", ret_code, output_buf );
    HCI_LE_error( "CMD ERROR MSG:" );
    HCI_LE_error( "%ms", output_buf );
    Error_popup( error_buf );
  }
}

/***************************************************************************/
/* RESET_GUI: Reset GUI to initial state.                                  */
/***************************************************************************/

static void Reset_gui()
{
  Action_flag = NO_ACTION_FLAG;
  HCI_default_cursor();
  XtVaSetValues( Screen_button, XmNsensitive, True, NULL );
  XtVaSetValues( Window_button, XmNsensitive, True, NULL );
  XtVaSetValues( Close_button, XmNsensitive, True, NULL );
}

