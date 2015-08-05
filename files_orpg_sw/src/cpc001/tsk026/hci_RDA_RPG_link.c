/************************************************************************
 
  Module: hci_RDA_RPG_link.c

  Description:  This module is used by the ORPG HCI to display and edit
     wideband adaptation data aong with controlling the connection.
 
************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/15 12:17:56 $
 * $Id: hci_RDA_RPG_link.c,v 1.35 2012/10/15 12:17:56 ccalvert Exp $
 * $Revision: 1.35 $
 * $State: Exp $
 */

/* Local includes */

#include <hci.h>

/* Enums/definitions */

#define LOOPBACK_FLAG               "rda_control_adapt.loopback_disabled"
#define LOOPBACK_RATE               "rda_control_adapt.loopback_rate"
enum {WIDEBAND_NO_ACTION, WIDEBAND_CONNECT, WIDEBAND_DISCONNECT};
enum {WIDEBAND_LOOPBACK_RATE};

/* Global/static variables */

static Widget Top_widget = (Widget) NULL;
static Widget Save_button = (Widget) NULL;
static Widget Undo_button = (Widget) NULL;
static Widget Connect_button = (Widget) NULL;
static Widget Disconnect_button = (Widget) NULL;
static Widget WB_state = (Widget) NULL;
static Widget Loopback_label = (Widget) NULL;
static Widget WB_loopback = (Widget) NULL;
static Widget Loopback_disable_button = (Widget) NULL;
static Pixel Bg_color = (Pixel) NULL;
static Pixel Button_bg = (Pixel) NULL;
static Pixel Button_fg = (Pixel) NULL;
static Pixel Edit_bg = (Pixel) NULL;
static Pixel Edit_fg = (Pixel) NULL;
static Pixel Text_fg = (Pixel) NULL;
static Pixel White_color = (Pixel) NULL;
static Pixel Black_color = (Pixel) NULL;
static Pixel Normal_color = (Pixel) NULL;
static Pixel Warning_color = (Pixel) NULL;
static Pixel Alarm_color = (Pixel) NULL;
static XmFontList Font_list = (XmFontList) NULL;
static int WB_change_flag = HCI_NOT_CHANGED_FLAG;
static int WB_cmd_flag = WIDEBAND_NO_ACTION;
static int WB_loopback_rate = 60;
static int WB_loopback_disabled_flag = HCI_NO_FLAG;
static  int Inactive_channel_flag = HCI_NO_FLAG;
static char Msg_buf [128];
static char WB_buf [256];

/* Function prototypes */

static void Close_callback( Widget, XtPointer, XtPointer );
static void Save_callback( Widget, XtPointer, XtPointer );
static void Undo_callback( Widget, XtPointer, XtPointer );
static void Accept_save_callback( Widget, XtPointer, XtPointer );
static void Cancel_save_callback( Widget, XtPointer, XtPointer );
static void Modify_callback( Widget, XtPointer, XtPointer );
static void Acknowledge_invalid_value( Widget, XtPointer, XtPointer );
static void Disable_loopback_callback( Widget, XtPointer, XtPointer );
static void Connection_callback( Widget, XtPointer, XtPointer );
static void Issue_command_callback( Widget, XtPointer, XtPointer );
static void Cancel_command_callback( Widget, XtPointer, XtPointer );
static void Display_data();
static void Timer_proc();
static int  Is_inactive_channel();

/************************************************************************
  Description: This is the main function for the RDA/RPG
     Interface Control/Status task.
  Input:  argc - number of commandline arguments
          argv - pointer to commandline argument data
  Output: NONE
  Return: exit code
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget control_rowcol = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget form = (Widget) NULL;
  Widget wb_control = (Widget) NULL;
  Widget wb_control_frame = (Widget) NULL;
  Widget wb_control_rowcol = (Widget) NULL;
  Widget wb_data_form = (Widget) NULL;
  Widget wb_data_frame = (Widget) NULL;
  Widget wb_data_rowcol = (Widget) NULL;
  Arg    arg [10];
  int    n = 0;
  double value = 0.0;

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_RDA_LINK_TASK );

  /* Initialize colors and fonts */

  Bg_color = hci_get_read_color( BACKGROUND_COLOR1 );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Edit_bg = hci_get_read_color( EDIT_BACKGROUND );
  Edit_fg = hci_get_read_color( EDIT_FOREGROUND );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  Black_color = hci_get_read_color( BLACK );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Warning_color = hci_get_read_color( WARNING_COLOR );
  Alarm_color = hci_get_read_color( ALARM_COLOR2 );
  Font_list = hci_get_fontlist( LIST );

  /* Read Redundant information. */

  Inactive_channel_flag = Is_inactive_channel();

  /* Define the widgets for the menu and display them. */

  Top_widget = HCI_get_top_widget();

  /* Use a form widget to organize the various menu widgets. */

  form   = XtVaCreateWidget( "form",
                xmFormWidgetClass, Top_widget,
                XmNautoUnmanage, False,
                XmNbackground, Bg_color,
                NULL );

  control_rowcol = XtVaCreateWidget( "control_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground, Bg_color,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNisAligned, False,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                NULL );

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, control_rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, Font_list,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  Save_button = XtVaCreateManagedWidget( "Save",
                xmPushButtonWidgetClass, control_rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, Font_list,
                XmNsensitive, False,
                NULL );

  XtAddCallback( Save_button, XmNactivateCallback, Save_callback, NULL );

  Undo_button = XtVaCreateManagedWidget( "Undo",
                xmPushButtonWidgetClass, control_rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, Font_list,
                XmNsensitive, False,
                NULL );

  XtAddCallback( Undo_button, XmNactivateCallback, Undo_callback, NULL );

  label = XtVaCreateManagedWidget( "      State: ",
                xmLabelWidgetClass, control_rowcol,
                XmNbackground, Bg_color,
                XmNforeground, Text_fg,
                XmNfontList, Font_list,
                NULL );

  /* Use longest label value to set initial width. */

  WB_state = XtVaCreateManagedWidget( " Disconnected (SHUTDOWN) ",
                xmLabelWidgetClass, control_rowcol,
                XmNbackground, Bg_color,
                XmNforeground, Text_fg,
                XmNfontList, Font_list,
                NULL );

  XtManageChild( control_rowcol );

  wb_control_frame   = XtVaCreateManagedWidget( "wb_control",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, control_rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbackground, Bg_color,
NULL );

  label = XtVaCreateManagedWidget( "Wideband Control",
                xmLabelWidgetClass, wb_control_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNfontList, Font_list,
                XmNforeground, Text_fg,
                XmNbackground, Bg_color,
                NULL );

  wb_control_rowcol = XtVaCreateWidget( "wb_control_rowcol",
                xmRowColumnWidgetClass, wb_control_frame,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Bg_color,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNspacing, 0,
                XmNmarginHeight, 0,
                XmNmarginWidth, 0,
                NULL );

  n = 0;

  XtSetArg( arg[n], XmNforeground, Text_fg );
  n++;
  XtSetArg( arg[n], XmNbackground, Bg_color );
  n++;
  XtSetArg( arg[n], XmNfontList, Font_list );
  n++;
  XtSetArg( arg[n], XmNorientation, XmHORIZONTAL );
  n++;

  wb_control = XmCreateRadioBox( wb_control_rowcol,
                "wb_control", arg, n );

  Connect_button = XtVaCreateManagedWidget( "Connect        ",
                xmToggleButtonWidgetClass, wb_control,
                XmNtraversalOn, False,
                XmNfontList, Font_list,
                XmNforeground, Text_fg,
                XmNbackground, Bg_color,
                XmNselectColor, White_color,
                NULL );

  XtAddCallback( Connect_button,
                XmNvalueChangedCallback, Connection_callback,
                (XtPointer) WIDEBAND_CONNECT );

  Disconnect_button = XtVaCreateManagedWidget( "Disconnect",
                xmToggleButtonWidgetClass, wb_control,
                XmNtraversalOn, False,
                XmNfontList, Font_list,
                XmNforeground, Text_fg,
                XmNbackground, Bg_color,
                XmNselectColor, White_color,
                NULL );

  XtAddCallback( Disconnect_button,
                XmNvalueChangedCallback, Connection_callback,
                (XtPointer) WIDEBAND_DISCONNECT );

  XtManageChild( wb_control );
  XtManageChild( wb_control_rowcol );

  wb_data_frame = XtVaCreateManagedWidget( "wb_data_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, wb_control_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbackground, Bg_color,
                NULL );

  label = XtVaCreateManagedWidget( "Wideband Interface Parameters",
                xmLabelWidgetClass, wb_data_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNfontList, Font_list,
                XmNforeground, Text_fg,
                XmNbackground, Bg_color,
                NULL );

  wb_data_form = XtVaCreateWidget( "wb_data_form",
                xmFormWidgetClass, wb_data_frame,
                XmNbackground, Bg_color,
                XmNverticalSpacing, 1,
                NULL );

  wb_data_rowcol = XtVaCreateWidget( "wb_data_rowcol",
                xmRowColumnWidgetClass, wb_data_form,
                XmNorientation, XmVERTICAL,
                XmNbackground, Bg_color,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                XmNspacing, 2,
                XmNmarginHeight, 0,
                XmNmarginWidth, 0,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  Loopback_label = XtVaCreateManagedWidget( "Loopback Rate (sec):",
                xmLabelWidgetClass, wb_data_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Bg_color,
                XmNfontList, Font_list,
                XmNmarginHeight, 2,
                NULL );

  /* If channel is inactive, desensitize Loopback_label. */
  if( Inactive_channel_flag )
  {
    XtSetSensitive( Loopback_label, False );
  }

  XtManageChild( wb_data_rowcol );

  wb_data_rowcol = XtVaCreateWidget( "wb_data_rowcol",
                xmRowColumnWidgetClass, wb_data_form,
                XmNorientation, XmVERTICAL,
                XmNbackground, Black_color,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNspacing, 0,
                XmNmarginHeight, 0,
                XmNmarginWidth, 0,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, wb_data_rowcol,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  WB_loopback = XtVaCreateManagedWidget( "Loopback_rate",
                xmTextWidgetClass, wb_data_rowcol,
                XmNforeground, Edit_fg,
                XmNbackground, Edit_bg,
                XmNfontList, Font_list,
                XmNcolumns, 3,
                XmNmarginHeight, 2,
                XmNshadowThickness, 0,
                XmNmarginWidth, 0,
                XmNborderWidth, 0,
                XmNeditable, True,
                NULL );

  /* If channel is inactive, desensitize loopback. */
  if( Inactive_channel_flag )
  {
    XtSetSensitive( WB_loopback, False );
  }

  if( DEAU_get_values( LOOPBACK_RATE, &value, 1 ) < 0 )
  {
    HCI_LE_error( "Error Getting Initial Loopback Rate" );
    HCI_LE_error( "Keep previous value %d", WB_loopback_rate );
  }
  else
  {
    WB_loopback_rate = (int) value;
  }

  sscanf( WB_buf, "%d", &WB_loopback_rate );
  XmTextSetString( WB_loopback, WB_buf );

  XtManageChild( wb_data_rowcol );

  label = XtVaCreateManagedWidget( "xxx",
                xmLabelWidgetClass, wb_data_form,
                XmNforeground, Bg_color,
                XmNbackground, Bg_color,
                XmNfontList, Font_list,
                XmNmarginHeight, 3,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, wb_data_rowcol,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  Loopback_disable_button = XtVaCreateManagedWidget( "Disabled",
                xmToggleButtonWidgetClass, wb_data_form,
                XmNbackground, Bg_color,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, Font_list,
                XmNmarginHeight, 1,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  /* If channel is inactive, desensitize Loopback_disable_button. */
  if( Inactive_channel_flag )
  {
    XtSetSensitive( Loopback_disable_button, False );
  }

  if( DEAU_get_values( LOOPBACK_FLAG, &value, 1 ) < 0 )
  {
    HCI_LE_error( "Error Getting Initial Loopback Flag" );
    HCI_LE_error( "Keep previous value %d", WB_loopback_disabled_flag );
  }
  else
  {
    WB_loopback_disabled_flag = (int) value;
  }


  XtAddCallback( Loopback_disable_button,
                XmNvalueChangedCallback, Disable_loopback_callback,
                (XtPointer) NULL );

  XtAddCallback( WB_loopback,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3 );

  XtAddCallback( WB_loopback,
                XmNfocusCallback, hci_gain_focus_callback,
                (XtPointer) NULL );

  XtAddCallback( WB_loopback,
                XmNlosingFocusCallback, Modify_callback,
                (XtPointer) WIDEBAND_LOOPBACK_RATE );

  XtAddCallback( WB_loopback,
                XmNactivateCallback, Modify_callback,
                (XtPointer) WIDEBAND_LOOPBACK_RATE );

  XtManageChild( wb_data_form );

  XtManageChild( form );
  XtPopup( Top_widget, XtGrabNone );

  /* If low bandwidth, display a progress meter */

  HCI_PM( "Reading RDA/RPG Interface Status" );

  Display_data();

  if( ORPGRDA_status_io_status() != RMT_CANCELLED )
  {
    XtRealizeWidget( Top_widget );
    /* Start HCI loop. */
    HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );
  }

  return 0;
}

/************************************************************************
  Description: This is the timer procedure for the RDA/RPG
     Interface Control/Status task.
  Input:  w - timer widget ID
         id - timer interval ID
  Output: NONE
  Return: 0 (unused)
 ************************************************************************/

static void Timer_proc ()
{
  static time_t previous_RDA_Status_time = 0;
         time_t current_RDA_Status_time = 0;

  /* Check to see if channel has become active/inactive since
     last timer. If status has changed, set sensitivity of various
     widgets accordingly. */

  if( Inactive_channel_flag != Is_inactive_channel() )
  {
    Inactive_channel_flag = Is_inactive_channel();
    if( Inactive_channel_flag )
    {
      /* If channel is inactive, desensitize various widgets. */
      XtSetSensitive( Loopback_label, False );
      XtSetSensitive( WB_loopback, False );
      XtSetSensitive( Loopback_disable_button, False );
    }
    else
    {
      /* If channel is not inactive, sensitize various widgets. */
      XtSetSensitive( Loopback_label, True );
      XtSetSensitive( WB_loopback, True );
      XtSetSensitive( Loopback_disable_button, True );
    }
  }

  /* If the RDA status message was updated then we need to reread
     it and refresh the display. */

  current_RDA_Status_time = ORPGRDA_status_update_time();
  if( current_RDA_Status_time != previous_RDA_Status_time )
  {
    previous_RDA_Status_time = current_RDA_Status_time;
    HCI_PM( "Reading RDA/RPG Interface Status" );
    Display_data ();
  }
}

/************************************************************************
  Description: This function is activated when the user selects
     the "Close" button.
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Close_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Wideband Control/Status Close selected" );

  if( WB_change_flag == HCI_CHANGED_FLAG )
  {
    Save_callback( w, (XtPointer) -1, (XtPointer) NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
  Description: This function is activated when the user selects
     the "Save" button
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Save_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Wideband Control/Status Save selected" );
  hci_confirm_popup( Top_widget, "Do you want to save your changes?", Accept_save_callback, Cancel_save_callback );
}

/************************************************************************
  Description: This function is activated when the user selects
     the "Yes" button from the Save confirmation popup
     window.
  Input:  w - widget id
          y - negative means close window
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Accept_save_callback( Widget w, XtPointer y, XtPointer z )
{

  double value1 = 0.0;
  double value2 = 0.0;

  HCI_LE_log( "Wideband Control/Status Save selected" );

  /* Update the wideband parameters adaptation data. */

  value1 = (double) WB_loopback_rate;
  value2 = (double) WB_loopback_disabled_flag;

  if( (DEAU_set_values( LOOPBACK_RATE, 0, &value1, 1, 0 ) < 0) ||
      (DEAU_set_values( LOOPBACK_FLAG, 0, &value2, 1, 0 ) < 0) )
  { 
    HCI_LE_error( "Unable to save Wideband Parameters adaptation data" );
    sprintf( Msg_buf, "Unable to Update Wideband Interface Parameters" );
  }
  else
  {
    sprintf( Msg_buf, "Wideband Interface Parameters Updated" );
  }

  /* Generate feedback message to display in RPG Control/Status window */

  HCI_display_feedback( Msg_buf );

  WB_change_flag = HCI_NOT_CHANGED_FLAG;

  XtSetSensitive( Save_button, False );
  XtSetSensitive( Undo_button, False );
}

/************************************************************************
  Description: This function is activated when the user selects
     the "No" button from the Save confirmation popup
     window.
  Input:  w - widget id
          y - negative means close window
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Cancel_save_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Wideband Control/Status Save cancelled" );
}

/************************************************************************
  Description: This function is used to display the current
     wideband state and adaptable parameters in the
     RDA/RPG Interface Control/Status window
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Display_data()
{
  int wbstat = 0;
  Pixel color_bg = -1;
  Pixel color_fg = -1;
  XmString str;

  /* If the top level widget isn't defined, do nothing. */

  if( Top_widget == (Widget) NULL ){ return; }

  /* Get the latest wideband status and update the state and
     properties of wideband Control buttons. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  switch( wbstat )
  {
    case RS_NOT_IMPLEMENTED :
      color_bg = White_color,
      color_fg = Text_fg;
      sprintf( WB_buf, "     Not Implemented     " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_CONNECT_PENDING :
      color_bg = Warning_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "     Connect Pending     " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_DISCONNECT_PENDING :
      color_bg = Warning_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "    Disconnect Pending   " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_DISCONNECTED_HCI :
      color_bg = Warning_color;
      color_fg = Text_fg;
      sprintf( WB_buf, " Disconnected (From RPG) " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, False );
      break;

    case RS_DISCONNECTED_CM :
      color_bg = Warning_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "    Disconnected (CM)    " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_DISCONNECTED_SHUTDOWN :
      color_bg = Warning_color;
      color_fg = Text_fg;
      sprintf( WB_buf, " Disconnected (SHUTDOWN) " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_DISCONNECTED_RMS :
      color_bg = Warning_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "    Disconnected (RMS)   " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_CONNECTED :
      color_bg = Normal_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "        Connected        " );
      XtSetSensitive( Connect_button, False );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_DOWN :
      color_bg = Alarm_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "           Down          " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    case RS_WBFAILURE :
      color_bg = Alarm_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "         Failure         " );
      XtSetSensitive( Connect_button, True );
      XtSetSensitive( Disconnect_button, True );
      break;

    default :
      color_bg = Alarm_color;
      color_fg = Text_fg;
      sprintf( WB_buf, "         Unknown         " );
      break;
  }

  str = XmStringCreateLocalized( WB_buf );

  XtVaSetValues( WB_state,
                XmNlabelString, str,
                XmNbackground, color_bg,
                XmNforeground, color_fg,
                NULL );
  XmStringFree( str );

  /* Display the values of the wideband adaptable parameters. */

  sprintf( WB_buf, "%3d ", WB_loopback_rate );
  XmTextSetString( WB_loopback, WB_buf );

  if( WB_loopback_disabled_flag )
  {
    XmToggleButtonSetState( Loopback_disable_button, True, False );
    XtVaSetValues( WB_loopback,
                XmNeditable, False,
                XmNbackground, Bg_color,
                NULL );
  }
  else
  {
    XmToggleButtonSetState( Loopback_disable_button, False, False );
    XtVaSetValues( WB_loopback,
                XmNeditable, True,
                XmNbackground, Edit_bg,
                NULL );
  }
}

/************************************************************************
  Description: This function is activated when the user changes a
     wideband adaptable parameter value or the edit box loses focus
  Input:  w - widget id
          y - WIDEBAND_LOOPBACK_RATE
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Modify_callback( Widget w, XtPointer y, XtPointer z )
{
  XtPointer data = (XtPointer) NULL;
  int val = 0;
  char *str = NULL;
  char tbuf [16];
  DEAU_attr_t *at = NULL;
  char *p = NULL;

  XtVaGetValues( w, XmNuserData, &data, NULL );

  str = XmTextGetString( w );
  strcpy( tbuf, str );
  sscanf( tbuf, "%d", &val );
  XtFree (str);

  switch( (int) y )
  {
    case WIDEBAND_LOOPBACK_RATE :
      if( val != WB_loopback_rate )
      {
        if( DEAU_get_attr_by_id( LOOPBACK_RATE, &at ) < 0 )
        {
          at = NULL;
        }

        if( DEAU_check_data_range( LOOPBACK_RATE, DEAU_T_INT, 1, (char *) &val ) < 0 )
        {
          if( ( at != NULL )  && ( DEAU_get_min_max_values( at, &p ) == 7 ) )
          {
            sprintf( WB_buf, "%d is out of range, must be in the range %s <= x <= ", val, p );
            p += strlen (p) + 1;
            sprintf( WB_buf + strlen( WB_buf ), "%s.\n", p );
          }
          else
          {
            sprintf( WB_buf, "%d is not valid for %s\n", val, LOOPBACK_RATE );
          }

          hci_warning_popup( Top_widget, WB_buf, Acknowledge_invalid_value );
        }
        else
        {
          WB_loopback_rate = val;

          if( WB_change_flag == HCI_NOT_CHANGED_FLAG )
          {
            XtSetSensitive( Save_button, True );
            XtSetSensitive( Undo_button, True );
            WB_change_flag = HCI_CHANGED_FLAG;
          }
        }
      }
      break;
  }

  Display_data();

  return;
}

/************************************************************************
  Description: This function is activated when the user selects
     the Disabled check box
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Disable_loopback_callback( Widget w, XtPointer y, XtPointer z )
{
  double value = 0.0;

  XmToggleButtonCallbackStruct *state =
                (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    value = 0.0;
    if( DEAU_set_values( LOOPBACK_FLAG, 0, &value, 1, 0 ) < 0 )
    {
      HCI_LE_error( "Unable to disable Wideband loopback rate" );
    }
    else
    {
      WB_loopback_disabled_flag = HCI_YES_FLAG;

      if( WB_change_flag == HCI_NOT_CHANGED_FLAG )
      {
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
        WB_change_flag = HCI_CHANGED_FLAG;
      }
    }

    XtVaSetValues( WB_loopback,
                XmNeditable, False,
                XmNbackground, Bg_color,
                NULL );
  }
  else
  {
    value = 1.0;
    if( DEAU_set_values( LOOPBACK_FLAG, 0, &value, 1, 0 ) < 0 )
    {
      HCI_LE_error( "Unable to enable Wideband loopback rate" );
    }
    else
    {
      WB_loopback_disabled_flag = HCI_NO_FLAG;

      if( WB_change_flag == HCI_NOT_CHANGED_FLAG )
      {
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
        WB_change_flag = HCI_CHANGED_FLAG;
      }
    }

    XtVaSetValues( WB_loopback,
                XmNeditable, True,
                XmNforeground, Text_fg,
                XmNbackground, Edit_bg,
                NULL );
  }

  Display_data ();
}

/************************************************************************
  Description: This function is activated when the user selects
     the "Undo" button.
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Undo_callback( Widget w, XtPointer y, XtPointer z )
{
  double value = 0.0;

  if( DEAU_get_values( LOOPBACK_RATE, &value, 1 ) < 0 )
  {
    HCI_LE_error( "Error Get Loopback Rate" );
  }
  else
  {
    WB_loopback_rate = (int) value;
  }

  if( DEAU_get_values( LOOPBACK_FLAG, &value, 1 ) < 0 )
  {
    HCI_LE_error( "Error Get Loopback Disabled" );
  }
  else
  {
    WB_loopback_disabled_flag = (int) value;
  }

  Display_data ();

  WB_change_flag = HCI_NOT_CHANGED_FLAG;

  XtSetSensitive( Save_button, False );
  XtSetSensitive( Undo_button, False );
}

/************************************************************************
  Description: This function is activated when the user selects
     the "Continue" button from a warning popup window
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Acknowledge_invalid_value( Widget w, XtPointer y, XtPointer z )
{
}

/************************************************************************
  Description: This function is activated when the user selects
     one of the Wideband Control radio buttons
  Input:  w - widget id
          y - WIDEBAND_CONNECT
              WIDEBAND_DISCONNECT
          z - toggle data
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Connection_callback( Widget w, XtPointer y, XtPointer z )
{
  char buf[HCI_BUF_256];

  XmToggleButtonCallbackStruct *state =
                (XmToggleButtonCallbackStruct *) z;

  /* If the button is set, determine the command from the client
     data.  Prompt the user before actually sending the command. */

  if( state->set )
  {
    WB_cmd_flag = (int) y;

    if( WB_cmd_flag == WIDEBAND_CONNECT )
    {
      HCI_LE_log( "Wideband Connect selected" );
      sprintf( buf, "You are about to issue a request to enable\nthe connection between the RDA and RPG.\nDo you want to continue?" );
      hci_confirm_popup( Top_widget, buf, Issue_command_callback, Cancel_command_callback );
    }
    else if( WB_cmd_flag == WIDEBAND_DISCONNECT )
    {
      HCI_LE_log( "Disconnect Wideband Link" );
      sprintf( buf, "You are about to issue a request to disable\nthe connection between the RDA and RPG.\nDo you want to continue?" );
      hci_confirm_popup( Top_widget, buf, Issue_command_callback, Cancel_command_callback );
    }

    XtVaSetValues( w, XmNset, False, NULL );
  }
}

/************************************************************************
  Description: This function is activated when the user selects the "No"
     button from the Wideband Control confirmation popup window
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Cancel_command_callback( Widget w, XtPointer y, XtPointer z )
{
  WB_cmd_flag = WIDEBAND_NO_ACTION;
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
     button from the Wideband Control confirmation popup window
  Input:  w - widget id
          y - unused
          z - unused
  Output: NONE
  Return: NONE
 ************************************************************************/

static void Issue_command_callback( Widget w, XtPointer y, XtPointer z )
{
  if( WB_cmd_flag == WIDEBAND_CONNECT )
  {
    /* Generate feedback message to display in RPG Control/Status window */

    sprintf( Msg_buf, "Connect Wideband Link" );

    HCI_display_feedback( Msg_buf );

    ORPGRDA_send_cmd( COM4_WBENABLE, HCI_INITIATED_RDA_CTRL_CMD,
                      0, 0, 0, 0, 0, NULL );
  }
  else if( WB_cmd_flag == WIDEBAND_DISCONNECT )
  {
    /* Generate feedback message to display in RPG Control/Status window */

    sprintf( Msg_buf, "Disconnect wideband link" );

    HCI_display_feedback( Msg_buf );


    ORPGRDA_send_cmd( COM4_WBDISABLE, HCI_INITIATED_RDA_CTRL_CMD,
                      0, 0, 0, 0, 0, NULL );
  }

  WB_cmd_flag = WIDEBAND_NO_ACTION;
}

/************************************************************************
  Description: Determines if system is inactive/non-controlling channel
  Input:  NONE
  Output: NONE
  Return: 1 is YES, 0 if NO
 ************************************************************************/

int Is_inactive_channel()
{
  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    if( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) != ORPGRED_CHANNEL_ACTIVE )
    {
      return HCI_YES_FLAG;
    }
  }

  return HCI_NO_FLAG;
}
