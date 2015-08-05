/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/31 20:50:44 $
 * $Id: hci_RDA_control_orda.c,v 1.22 2013/07/31 20:50:44 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

/************************************************************************
     Module: hci_RDA_control.c
     Description: This module is used by the ORPG HCI to define
                  and display the RDA functions menu.
 ************************************************************************/

/* Local includes */

#include <hci.h>
#include <rms_util.h>
#include <hci_orda_pmd.h>

/* Local definitions */

#define	SPOT_BLANKING_NOT_INSTALLED	0

/* Wideband control macros */

enum { WIDEBAND_CONNECT, WIDEBAND_DISCONNECT };

/* ID of top level dialog shell widget. */

static Widget Top_widget = (Widget) NULL;

/* ID's of RDA State radio button widgets */

static Widget Rda_operate_button = (Widget) NULL;
static Widget Rda_offline_operate_button = (Widget) NULL;
static Widget Rda_restart_button = (Widget) NULL;
static Widget Rda_standby_button = (Widget) NULL;

/* ID's of RDA Control radio button widgets */

static Widget Control_rda_button = (Widget) NULL;
static Widget Control_rpg_button = (Widget) NULL;

/* ID's of FAA Redundant Channel Control radio button widgets */

static Widget Red_control_button = (Widget) NULL;
static Widget Red_non_control_button = (Widget) NULL;

/* ID's of RDA Power Source radio button widgets */

static Widget Power_utility_button = (Widget) NULL;
static Widget Power_auxiliary_button = (Widget) NULL;

/* ID's of RDA Performance Check radio button widgets */

static Widget Perfcheck_button = (Widget) NULL;

/* ID's of Spot Blanking radio button widgets */

static Widget Spot_blanking_frame = (Widget) NULL;
static Widget Spot_blanking_enable_button = (Widget) NULL;
static Widget Spot_blanking_disable_button = (Widget) NULL;

/* ID's of various status label widgets */

static Widget Control_authorization_label = (Widget) NULL;
static Widget Ave_transmitter_power_label = (Widget) NULL;
static Widget Horiz_calibration_correction_label = (Widget) NULL;
static Widget Vert_calibration_correction_label = (Widget) NULL;
static Widget Moments_enabled_label = (Widget) NULL;
static Widget Current_state_label = (Widget) NULL;
static Widget Current_control_label = (Widget) NULL;
static Widget Current_red_local_status = (Widget) NULL;
static Widget Current_red_red_status = (Widget) NULL;
static Widget Current_red_local_time = (Widget) NULL;
static Widget Current_red_red_time = (Widget) NULL;
static Widget Current_power_label = (Widget) NULL;
static Widget Current_perfcheck_label = (Widget) NULL;
static Widget Current_spot_blanking_label = (Widget) NULL;

/* Colors and Font */
static Pixel Base_bg = (Pixel) NULL;
static Pixel Base_bg2 = (Pixel) NULL;
static Pixel Button_bg = (Pixel) NULL;
static Pixel Button_fg = (Pixel) NULL;
static Pixel Loca_fg = (Pixel) NULL;
static Pixel Text_fg = (Pixel) NULL;
static Pixel White_color = (Pixel) NULL;
static Pixel Alarm_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;

/* ID of pushbutton widget to send command to request RDA status. */

static Widget Status_button = (Widget) NULL;

/* ID of checkbox widget to lock/unlock RDA control from RMS */

static Widget Lock_rms = (Widget) NULL;

/* ID of lock button widget for spot blanking and channel control. */

static Widget Lock_button = (Widget) NULL;

static int Command_lock = HCI_NO_FLAG; /* Lock to prevent RMS from control */
static int Unlocked_loca = HCI_NO_FLAG; /* URC/ROC LOCA flag */
static int Change_rms_lock = HCI_NO_FLAG; /* change rms lock flag */
static int Config_change_popup = HCI_NO_FLAG; /* RDA change popup flag. */
static int Redundant_type = -1;
static int User_selected_RDA_state_flag = -1;
static int User_selected_RDA_red_control_flag = -1;
static int User_selected_spot_blank_flag = -1;
static int Redundant_status_change_flag = HCI_NO_FLAG;
static char Cmd[128]; /* common buffer for string operations */

static int Hci_rda_control_security();
static void Set_rda_button();
static void Rda_state_callback( Widget, XtPointer, XtPointer );
static void Accept_red_control_callback( Widget, XtPointer, XtPointer );
static void Cancel_red_control_callback( Widget, XtPointer, XtPointer );
static void Spot_blanking_callback( Widget, XtPointer, XtPointer );
static void Redundant_status_change_cb( int, LB_id_t, int, void * );
static void Timer_proc();
static void Update_rda_control_menu_properties();
static void Rda_config_change();
static void Verify_spot_blanking( Widget, XtPointer );
static void RDA_control_close( Widget, XtPointer, XtPointer );
static void Toggle_spot_blanking_callback( Widget,  XtPointer, XtPointer );
static void Switch_power_callback( Widget, XtPointer, XtPointer );
static void Perfcheck_callback( Widget, XtPointer, XtPointer );
static void Accept_perfcheck_callback( Widget, XtPointer, XtPointer );
static void Toggle_RDA_control_callback( Widget, XtPointer, XtPointer );
static void Toggle_red_control_callback( Widget, XtPointer, XtPointer );
static void Change_rda_state_callback( Widget, XtPointer, XtPointer );
static void Request_RDA_status_callback( Widget, XtPointer, XtPointer );
static void Hci_select_rda_alarms_callback( Widget, XtPointer, XtPointer );
static void Hci_select_vcp_callback( Widget, XtPointer, XtPointer );
static void Hci_lock_button_callback( Widget, XtPointer, XtPointer );
static void Hci_lock_button_verify_callback( Widget, XtPointer, XtPointer );

int hci_activate_child (Display *, Window, char *, char *, char *, int );

/************************************************************************
     Description: This is the main function for the RDA Control task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget form = (Widget) NULL;
  Widget state_form = (Widget) NULL;
  Widget status_form = (Widget) NULL;
  Widget state_frame = (Widget) NULL;
  Widget status_frame = (Widget) NULL;
  Widget control_frame = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget space = (Widget) NULL;
  Widget rda_control_form = (Widget) NULL;
  Widget red_control_list = (Widget) NULL;
  Widget red_control_frame = (Widget) NULL;
  Widget red_control_form = (Widget) NULL;
  Widget red_control_frame_l = (Widget) NULL;
  Widget red_control_form_l = (Widget) NULL;
  Widget red_control_frame_r = (Widget) NULL;
  Widget red_control_form_r = (Widget) NULL;
  Widget power_form = (Widget) NULL;
  Widget perfcheck_form = (Widget) NULL;
  Widget spot_blanking_form = (Widget) NULL;
  Widget rda_control_frame = (Widget) NULL;
  Widget power_frame = (Widget) NULL;
  Widget perfcheck_frame = (Widget) NULL;
  Widget status1_rowcol = (Widget) NULL;
  Widget status2_rowcol = (Widget) NULL;
  Widget status3_rowcol = (Widget) NULL;
  Widget status4_rowcol = (Widget) NULL;
  Widget control_rowcol = (Widget) NULL;
  Widget state_list = (Widget) NULL;
  Widget control_list = (Widget) NULL;
  Widget power_list = (Widget) NULL;
  Widget perfcheck_list = (Widget) NULL;
  Widget spot_blanking_list = (Widget) NULL;
  Widget lock_form = (Widget) NULL;
  int n = 0;
  int status = 0;
  int spot_blank_flag = 0;
  Arg arg[10];

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_RDC_TASK );

  /* Define the widgets for the RDA menu and display them. */

  Top_widget = HCI_get_top_widget();

  Redundant_type = HCI_get_system();

  /* Define colors and font */

  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Base_bg2 = hci_get_read_color( BACKGROUND_COLOR2 );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Loca_fg = hci_get_read_color( LOCA_FOREGROUND );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  Alarm_color = hci_get_read_color( ALARM_COLOR1 );
  List_font = hci_get_fontlist( LIST );

  /* Set write permission for RDA Command LB */

  ORPGDA_write_permission( ORPGDAT_RDA_COMMAND );

  /* Get Spot Blank flag */

  spot_blank_flag = ORPGRDA_get_status( RS_SPOT_BLANKING_STATUS );

  /* Use a form widget to organize the various menu widgets. */

  form = XtVaCreateWidget( "RDA_form",
                           xmFormWidgetClass, Top_widget,
                           XmNautoUnmanage, False,
                           XmNbackground, Base_bg,
                           NULL );

  /* Create the buttons for requesting RDA status and performance data */

  control_frame = XtVaCreateManagedWidget( "control_frame",
              xmFrameWidgetClass, form,
              XmNbackground, Base_bg,
              XmNtopAttachment, XmATTACH_FORM,
              XmNleftAttachment, XmATTACH_FORM,
              NULL );

  control_rowcol = XtVaCreateWidget( "control_rowcol",
              xmRowColumnWidgetClass, control_frame,
              XmNbackground, Base_bg,
              XmNorientation, XmHORIZONTAL,
              XmNpacking, XmPACK_TIGHT,
              XmNnumColumns, 1,
              XmNisAligned, False,
              NULL );

  button = XtVaCreateManagedWidget( "Close",
              xmPushButtonWidgetClass, control_rowcol,
              XmNalignment, XmALIGNMENT_CENTER,
              XmNbackground, Button_bg,
              XmNforeground, White_color,
              XmNfontList, List_font,
              NULL );

  XtAddCallback( button, XmNactivateCallback, RDA_control_close, NULL );

  Status_button = XtVaCreateManagedWidget( "Get Status",
              xmPushButtonWidgetClass, control_rowcol,
              XmNalignment, XmALIGNMENT_CENTER,
              XmNbackground, Button_bg,
              XmNforeground, White_color,
              XmNfontList, List_font,
              NULL );

  XtAddCallback( Status_button, XmNactivateCallback, Request_RDA_status_callback, NULL );

  button = XtVaCreateManagedWidget( "RDA Alarms",
              xmPushButtonWidgetClass, control_rowcol,
              XmNalignment, XmALIGNMENT_CENTER,
              XmNbackground, Button_bg,
              XmNforeground, White_color,
              XmNfontList, List_font,
              NULL );

  XtAddCallback( button, XmNactivateCallback, Hci_select_rda_alarms_callback, NULL );

              button = XtVaCreateManagedWidget( "VCP and Mode Control",
              xmPushButtonWidgetClass, control_rowcol,
              XmNalignment, XmALIGNMENT_CENTER,
              XmNbackground, Button_bg,
              XmNforeground, White_color,
              XmNfontList, List_font,
              NULL );

  XtAddCallback( button, XmNactivateCallback, Hci_select_vcp_callback, NULL );

  /* If this is an RMS site, then we need to add a check box so
     we can prevent the RMS from sending RDA control commands while
     the HCI is also sending them. */

  if( HCI_has_rms() )
  {
    Lock_rms = XtVaCreateManagedWidget( "Lock RMS",
              xmToggleButtonWidgetClass, control_rowcol,
              XmNalignment, XmALIGNMENT_CENTER,
              XmNbackground, Base_bg,
              XmNforeground, Text_fg,
              XmNselectColor, White_color,
              XmNfontList, List_font,
              NULL );

    XtAddCallback( Lock_rms, XmNvalueChangedCallback, Hci_lock_button_verify_callback, NULL );
    Set_rda_button();
  }

  XtManageChild( control_rowcol );

  lock_form = XtVaCreateWidget( "lock_form",
              xmFormWidgetClass, form,
              XmNtopAttachment, XmATTACH_FORM,
              XmNrightAttachment, XmATTACH_FORM,
              XmNbackground, Base_bg,
              NULL );

  /* We need to define a lock button since the spot blanking and
     FAA redundant channel control require passwords. */

  Lock_button = hci_lock_widget( lock_form, Hci_rda_control_security, HCI_LOCA_URC | HCI_LOCA_ROC | HCI_LOCA_FAA_OVERRIDE );

  XtManageChild( lock_form );

  /* Define a set of labels and buttons to display the RDA state
     and change it. */

  state_frame = XtVaCreateManagedWidget( "top_frame",
              xmFrameWidgetClass, form,
              XmNtopAttachment, XmATTACH_WIDGET,
              XmNtopWidget, control_frame,
              XmNleftAttachment, XmATTACH_FORM,
              XmNbackground, Base_bg,
              NULL );

  label = XtVaCreateManagedWidget( "RDA State",
              xmLabelWidgetClass, state_frame,
              XmNchildType, XmFRAME_TITLE_CHILD,
              XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
              XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
              XmNbackground, Base_bg,
              XmNforeground, Text_fg,
              XmNfontList, List_font,
              NULL );

  state_form = XtVaCreateWidget( "control_form",
              xmFormWidgetClass, state_frame,
              XmNbackground, Base_bg,
              NULL );

  label = XtVaCreateManagedWidget( "State: ",
              xmLabelWidgetClass, state_form,
              XmNbackground, Base_bg,
              XmNforeground, Text_fg,
              XmNfontList, List_font,
              XmNleftAttachment, XmATTACH_FORM,
              XmNtopAttachment, XmATTACH_FORM,
              NULL );

  Current_state_label = XtVaCreateManagedWidget( "    UNKNOWN    ",
              xmLabelWidgetClass, state_form,
              XmNbackground, Base_bg2,
              XmNforeground, Text_fg,
              XmNfontList, List_font,
              XmNleftAttachment, XmATTACH_WIDGET,
              XmNleftWidget, label,
              XmNtopAttachment, XmATTACH_FORM,
              NULL );

  n = 0;
  XtSetArg( arg[n], XmNforeground, White_color );
  n++;
  XtSetArg( arg[n], XmNbackground, Base_bg );
  n++;
  XtSetArg( arg[n], XmNorientation, XmVERTICAL );
  n++;
  XtSetArg( arg[n], XmNspacing, 0 );
  n++;
  XtSetArg( arg[n], XmNmarginHeight, 1 );
  n++;
  XtSetArg( arg[n], XmNmarginWidth, 1 );
  n++;
  XtSetArg( arg[n], XmNnumColumns, 2 );
  n++;
  XtSetArg( arg[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( arg[n], XmNtopWidget, label );
  n++;
  XtSetArg( arg[n], XmNleftAttachment, XmATTACH_FORM );
  n++;


  state_list = XmCreateRadioBox( state_form, "state_list", arg, n );

  Rda_standby_button = XtVaCreateManagedWidget( "Standby",
              xmToggleButtonWidgetClass, state_list,
              XmNbackground, Base_bg,
              XmNforeground, Text_fg,
              XmNselectColor, White_color,
              XmNfontList, List_font,
              NULL );

  XtAddCallback( Rda_standby_button,
                XmNvalueChangedCallback, Change_rda_state_callback, 
                (XtPointer) RS_STANDBY );

  Rda_operate_button = XtVaCreateManagedWidget( "Operate",
                xmToggleButtonWidgetClass, state_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Rda_operate_button,
                XmNvalueChangedCallback, Change_rda_state_callback, 
                (XtPointer) RS_OPERATE );

  Rda_restart_button = XtVaCreateManagedWidget( "Restart",
                xmToggleButtonWidgetClass, state_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Rda_restart_button,
                XmNvalueChangedCallback, Change_rda_state_callback, 
                (XtPointer) RS_RESTART );

  Rda_offline_operate_button = XtVaCreateManagedWidget( "Offline Operate",
                xmToggleButtonWidgetClass, state_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Rda_offline_operate_button,
                XmNvalueChangedCallback, Change_rda_state_callback, 
                (XtPointer) RS_OFFOPER );

  XtManageChild( state_list );
  XtManageChild( state_form );

  /* Put a space between the state and RDA Control frames. */

  label = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, control_frame,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, state_frame,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  /* Create the widgets defining the buttons to change the RDA
     control source.  Highlight the currently active source. */

  rda_control_frame = XtVaCreateManagedWidget( "rda_control_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, control_frame,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "RDA Control",
                xmLabelWidgetClass, rda_control_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  rda_control_form = XtVaCreateWidget( "rda_control_form",
                xmFormWidgetClass, rda_control_frame,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Control: ",
                xmLabelWidgetClass, rda_control_form,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  Current_control_label = XtVaCreateManagedWidget( "  UNKNOWN   ",
                xmLabelWidgetClass, rda_control_form,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  n = 0;
  XtSetArg( arg[n], XmNforeground, Button_fg );
  n++;
  XtSetArg( arg[n], XmNbackground, Base_bg );
  n++;
  XtSetArg( arg[n], XmNmarginHeight, 1 );
  n++;
  XtSetArg( arg[n], XmNmarginWidth, 1 );
  n++;
  XtSetArg( arg[n], XmNorientation, XmVERTICAL );
  n++;
  XtSetArg( arg[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( arg[n], XmNtopWidget, label );
  n++;
  XtSetArg( arg[n], XmNleftAttachment, XmATTACH_FORM );
  n++;

  control_list = XmCreateRadioBox( rda_control_form, "control_list", arg, n );

  Control_rda_button = XtVaCreateManagedWidget( "Enable Local (RDA)",
                xmToggleButtonWidgetClass, control_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Control_rda_button,
                XmNvalueChangedCallback, Toggle_RDA_control_callback, 
                (XtPointer) CS_LOCAL_ONLY );

  Control_rpg_button = XtVaCreateManagedWidget( "Select Remote (RPG)",
                xmToggleButtonWidgetClass, control_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Control_rpg_button,
                XmNvalueChangedCallback, Toggle_RDA_control_callback,
                (XtPointer) CS_RPG_REMOTE );

  XtManageChild( control_list );
  XtManageChild( rda_control_form );

  /* Put a space between the state and RDA state and power frames. */

  space = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, state_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  /* Create the widgets defining the buttons to change the RDA
     power source.  Highlight the currently active source. */

  power_frame = XtVaCreateManagedWidget( "power_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "RDA Power Source",
                xmLabelWidgetClass, power_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  power_form = XtVaCreateWidget( "power_form",
                xmFormWidgetClass, power_frame,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Source: ",
                xmLabelWidgetClass, power_form,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  Current_power_label = XtVaCreateManagedWidget( " UNKNOWN ",
                xmLabelWidgetClass, power_form,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  n = 0;
  XtSetArg( arg[n], XmNforeground, White_color );
  n++;
  XtSetArg( arg[n], XmNbackground, Base_bg );
  n++;
  XtSetArg( arg[n], XmNmarginHeight, 1 );
  n++;
  XtSetArg( arg[n], XmNmarginWidth, 1 ); 
  n++;
  XtSetArg( arg[n], XmNorientation, XmVERTICAL );
  n++;
  XtSetArg( arg[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( arg[n], XmNtopWidget, label );
  n++;
  XtSetArg( arg[n], XmNleftAttachment, XmATTACH_FORM);
  n++;

  power_list = XmCreateRadioBox( power_form, "power_list", arg, n );

  Power_utility_button = XtVaCreateManagedWidget( "Utility",
                xmToggleButtonWidgetClass, power_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Power_utility_button,
                XmNvalueChangedCallback, Switch_power_callback,
                (XtPointer) UTILITY_POWER );

  Power_auxiliary_button = XtVaCreateManagedWidget( "Auxiliary",
                xmToggleButtonWidgetClass, power_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Power_auxiliary_button,
                XmNvalueChangedCallback, Switch_power_callback,
                (XtPointer) AUXILLIARY_POWER );

  XtManageChild( power_list );
  XtManageChild( power_form );

  /* Put a space between the power and performance check frames. */

  label = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, power_frame,
                XmNbackground, Base_bg,
                XmNforeground, Base_bg,
                XmNfontList, List_font,
                NULL );

  /* Create the widgets defining the button to initiate a
     RDA Performance Check */

  perfcheck_frame = XtVaCreateManagedWidget( "perfcheck_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Performance Check",
                xmLabelWidgetClass, perfcheck_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  perfcheck_form = XtVaCreateWidget( "perfcheck_form",
                xmFormWidgetClass, perfcheck_frame,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Status: ",
                xmLabelWidgetClass, perfcheck_form,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  Current_perfcheck_label = XtVaCreateManagedWidget( "  UNKNOWN  ",
                xmLabelWidgetClass, perfcheck_form,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  n = 0;
  XtSetArg( arg[n], XmNforeground, White_color );
  n++;
  XtSetArg( arg[n], XmNbackground, Base_bg );
  n++;
  XtSetArg( arg[n], XmNmarginHeight, 1 );
  n++;
  XtSetArg( arg[n], XmNmarginWidth, 1 ); 
  n++;
  XtSetArg( arg[n], XmNorientation, XmVERTICAL );
  n++;
  XtSetArg( arg[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( arg[n], XmNtopWidget, label );
  n++;
  XtSetArg( arg[n], XmNleftAttachment, XmATTACH_FORM);
  n++;

  perfcheck_list = XmCreateRadioBox( perfcheck_form, "perfcheck_list", arg, n );

  Perfcheck_button = XtVaCreateManagedWidget( "Initiate Performance Check",
                xmToggleButtonWidgetClass, perfcheck_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Perfcheck_button,
                XmNvalueChangedCallback, Perfcheck_callback,
                (XtPointer) NULL );

  XtManageChild( perfcheck_list );
  XtManageChild( perfcheck_form );

  /* Put a space between the performance check and spot blanking frames. */

  label = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, perfcheck_frame,
                XmNbackground, Base_bg,
                XmNforeground, Base_bg,
                XmNfontList, List_font,
                NULL );

  /* Create the buttons for the Spot Blanking Status selections. */

  Spot_blanking_frame = XtVaCreateManagedWidget( "spot_blanking_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Spot Blanking Status",
                xmLabelWidgetClass, Spot_blanking_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  spot_blanking_form = XtVaCreateWidget( "spot_blanking_form",
                xmFormWidgetClass, Spot_blanking_frame,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Status: ",
                xmLabelWidgetClass, spot_blanking_form,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  Current_spot_blanking_label = XtVaCreateManagedWidget( "UNKNOWN ",
                xmLabelWidgetClass, spot_blanking_form,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  n = 0;
  XtSetArg( arg[n], XmNforeground, White_color );
  n++;
  XtSetArg( arg[n], XmNbackground, Base_bg );
  n++;
  XtSetArg( arg[n], XmNmarginHeight, 1 );
  n++;
  XtSetArg( arg[n], XmNmarginWidth, 1 );
  n++;
  XtSetArg( arg[n], XmNorientation, XmVERTICAL );
  n++;
  XtSetArg( arg[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( arg[n], XmNtopWidget, label );
  n++;
  XtSetArg( arg[n], XmNleftAttachment, XmATTACH_FORM );
  n++;

  spot_blanking_list = XmCreateRadioBox( spot_blanking_form, "spot_blanking_list", arg, n );

  Spot_blanking_enable_button = XtVaCreateManagedWidget( "Enabled",
                xmToggleButtonWidgetClass, spot_blanking_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Spot_blanking_enable_button,
                XmNvalueChangedCallback, Toggle_spot_blanking_callback,
                (XtPointer) SB_ENABLED );

  Spot_blanking_disable_button = XtVaCreateManagedWidget( "Disabled",
                xmToggleButtonWidgetClass, spot_blanking_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Spot_blanking_disable_button,
                XmNvalueChangedCallback, Toggle_spot_blanking_callback,
                (XtPointer) SB_DISABLED );

  XtManageChild( spot_blanking_list );
  XtManageChild( spot_blanking_form );

  /* Put a space below the RDA power source frame. */

  space = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, power_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  /* Create the widgets defining the buttons to change the RDA
     redundant channel state.  Highlight the currently active source.*/

  if( Redundant_type == HCI_FAA_SYSTEM )
  {
    status = ORPGDA_UN_register( ORPGDAT_REDMGR_CHAN_MSGS, ORPGRED_CHANNEL_STATUS_MSG, Redundant_status_change_cb );

    if( status != LB_SUCCESS )
    {
      HCI_LE_error( "Unable to register for local ch updates (%d)", status);
      HCI_task_exit( HCI_EXIT_FAIL );
    }

    status = ORPGDA_UN_register( ORPGDAT_REDMGR_CHAN_MSGS, ORPGRED_REDUN_CHANL_STATUS_MSG, Redundant_status_change_cb );

    if( status != LB_SUCCESS )
    {
      HCI_LE_error( "Unable to register for redundant ch updates (%d)", status);
      HCI_task_exit( HCI_EXIT_FAIL );
    }

    red_control_frame = XtVaCreateManagedWidget( "red_control_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

    label = XtVaCreateManagedWidget( "Redundant Control",
                xmLabelWidgetClass, red_control_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

    red_control_form = XtVaCreateWidget( "red_control_form",
                xmFormWidgetClass, red_control_frame,
                XmNbackground, Base_bg,
                NULL);

    red_control_frame_l = XtVaCreateManagedWidget( "red_control_frame_l",
                xmFrameWidgetClass, red_control_form,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

    label = XtVaCreateManagedWidget( "Local Channel",
                xmLabelWidgetClass, red_control_frame_l,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

    red_control_frame_r = XtVaCreateManagedWidget( "red_control_frame_r",
                xmFrameWidgetClass, red_control_form,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, red_control_frame_l,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

    label = XtVaCreateManagedWidget( "Redundant Channel",
                xmLabelWidgetClass, red_control_frame_r,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

    red_control_form_l = XtVaCreateWidget( "red_control_form_l",
                xmFormWidgetClass, red_control_frame_l,
                XmNbackground, Base_bg,
                NULL );

    label = XtVaCreateManagedWidget( "Status: ",
                xmLabelWidgetClass, red_control_form_l,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

    Current_red_local_status = XtVaCreateManagedWidget( "      UNKNOWN       ",
                xmLabelWidgetClass, red_control_form_l,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

    label = XtVaCreateManagedWidget( "Adapt:  ",
                xmLabelWidgetClass, red_control_form_l,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, label,
                NULL );

    Current_red_local_time = XtVaCreateManagedWidget( "00/00/00 00:00:00 UT",
                xmLabelWidgetClass, red_control_form_l,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Current_red_local_status,
                NULL );

    n = 0;
    XtSetArg( arg[n], XmNforeground, Button_fg );
    n++;
    XtSetArg( arg[n], XmNbackground, Base_bg );
    n++;
    XtSetArg( arg[n], XmNmarginHeight, 1 );
    n++;
    XtSetArg( arg[n], XmNmarginWidth, 1 );
    n++;
    XtSetArg( arg[n], XmNorientation, XmVERTICAL );
    n++;
    XtSetArg( arg[n], XmNtopAttachment, XmATTACH_WIDGET );
    n++;
    XtSetArg( arg[n], XmNtopWidget, label );
    n++;
    XtSetArg( arg[n], XmNleftAttachment, XmATTACH_FORM );
    n++;

    red_control_list = XmCreateRadioBox( red_control_form_l, "red_control_list", arg, n );

    Red_control_button = XtVaCreateManagedWidget( "Controlling",
                xmToggleButtonWidgetClass, red_control_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

    XtAddCallback( Red_control_button,
                XmNvalueChangedCallback, Toggle_red_control_callback, 
                (XtPointer) RDA_IS_CONTROLLING );

    if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) != 1 )
    {
      Red_non_control_button = XtVaCreateManagedWidget( "Non-controlling",
                xmToggleButtonWidgetClass, red_control_list,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                NULL );

      XtAddCallback( Red_non_control_button,
                XmNvalueChangedCallback, Toggle_red_control_callback,
                (XtPointer) RDA_IS_NON_CONTROLLING );
    }

    XtManageChild( red_control_list );
    XtManageChild( red_control_form_l );

    red_control_form_r = XtVaCreateWidget( "red_control_form_r",
                xmFormWidgetClass, red_control_frame_r,
                XmNbackground, Base_bg,
                NULL );

    label = XtVaCreateManagedWidget( "Status: ",
                xmLabelWidgetClass, red_control_form_r,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

    Current_red_red_status = XtVaCreateManagedWidget( "      UNKNOWN       ",
                xmLabelWidgetClass, red_control_form_r,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

    label = XtVaCreateManagedWidget( "Adapt:  ",
                xmLabelWidgetClass, red_control_form_r,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, label,
                NULL );

    Current_red_red_time = XtVaCreateManagedWidget( "00/00/00 00:00:00 UT",
                xmLabelWidgetClass, red_control_form_r,
                XmNbackground, Base_bg2,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Current_red_red_status,
                NULL );

    XtManageChild( red_control_form_r );
    XtManageChild( red_control_form );

    /* Put a space between the Redundant and status frames. */

    space = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, red_control_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );
  }

  /* Create a container and set of labels to display various RDA
     status information. */

  status_frame = XtVaCreateManagedWidget( "status_frame",
                xmFrameWidgetClass, form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, space,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

  status_form = XtVaCreateWidget( "status_form",
                xmFormWidgetClass, status_frame,
                XmNbackground, Base_bg,
                NULL );

  status1_rowcol = XtVaCreateWidget( "status1_rowcol",
                xmRowColumnWidgetClass, status_form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNcolumns, 1,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  status2_rowcol = XtVaCreateWidget( "status2_rowcol",
                xmRowColumnWidgetClass, status_form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNcolumns, 1,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, status1_rowcol,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "S",
                xmLabelWidgetClass, status_form,
                XmNbackground, Base_bg,
                XmNforeground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, status2_rowcol,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  status3_rowcol = XtVaCreateWidget( "status3_rowcol",
                xmRowColumnWidgetClass, status_form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNcolumns, 1,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  status4_rowcol = XtVaCreateWidget( "status4_rowcol",
                xmRowColumnWidgetClass, status_form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNcolumns, 1,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, status3_rowcol,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  XtVaCreateManagedWidget( "Control Authority:  ",
                xmLabelWidgetClass, status1_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  Control_authorization_label = XtVaCreateManagedWidget( "                ",
                xmLabelWidgetClass, status2_rowcol,
                XmNbackground, White_color,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  XtVaCreateManagedWidget( "Transmitter Power:  ",
                xmLabelWidgetClass, status1_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  Ave_transmitter_power_label = XtVaCreateManagedWidget( "                ",
                xmLabelWidgetClass, status2_rowcol,
                XmNbackground, White_color,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  XtVaCreateManagedWidget( "H Delta dBZ0: ",
                xmLabelWidgetClass, status3_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  Horiz_calibration_correction_label = XtVaCreateManagedWidget( "            ",
                xmLabelWidgetClass, status4_rowcol,
                XmNbackground, White_color,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  XtVaCreateManagedWidget( "V Delta dBZ0: ",
                xmLabelWidgetClass, status3_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  Vert_calibration_correction_label = XtVaCreateManagedWidget( "            ",
                xmLabelWidgetClass, status4_rowcol,
                XmNbackground, White_color,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  XtVaCreateManagedWidget( "Moments Enabled:  ",
                xmLabelWidgetClass, status3_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  Moments_enabled_label = XtVaCreateManagedWidget( "            ",
                xmLabelWidgetClass, status4_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );

  XtManageChild( status1_rowcol );
  XtManageChild( status2_rowcol );
  XtManageChild( status3_rowcol );
  XtManageChild( status4_rowcol );
  XtManageChild( status_form );

  /* If the site is not FAA redundant and spot blanking is not
     installed, then do not manage the lock button.  Otherwise, we
     need to manage it. */

  if( Redundant_type != HCI_FAA_SYSTEM &&
      spot_blank_flag == SPOT_BLANKING_NOT_INSTALLED )
  {
    XtUnmanageChild( Lock_button );
  }

  if( spot_blank_flag == SPOT_BLANKING_NOT_INSTALLED )
  {
    XtUnmanageChild( Spot_blanking_frame );
  }

  /* If low bandwidth, display a progress meter. */

  HCI_PM( "Reading RDA Status Information" );

  /* Use information in the latest RDA status message to update the
     properties (colors/sensitivity) of the buttons and labels. */

  Update_rda_control_menu_properties();

  XtManageChild( form );

  /* If I/O was cancelled, do not pop up shell. */

  if( ORPGRDA_status_io_status() != RMT_CANCELLED )
  {
    XtRealizeWidget( Top_widget );
    /* Start HCI loop. */
    HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );
  }

  return 0;
}

/************************************************************************
     Description: This function is the timer procedure for the RDA
                  Control task.  Its main purpose is to update the
                  window properties when the wideband state changes
                  or when new RDA status data are received.
 ************************************************************************/

static void Timer_proc()
{
  static int old_wbstat = -1;
  static time_t old_RDA_status_update_time = -1;

  /* If the RDA configuration changes, call function to display
     popup window and exit. */

  if( ORPGRDA_get_rda_config( NULL ) != ORPGRDA_ORDA_CONFIG )
  {
    Rda_config_change();
  }

  if( ( ORPGRDA_status_update_flag() == 0 ) ||
      ( ORPGRDA_status_update_time() != old_RDA_status_update_time ) ||
      ( ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT ) != old_wbstat ) ||
      ( Redundant_status_change_flag == HCI_YES_FLAG ) )
  {
    Redundant_status_change_flag = HCI_NO_FLAG;
    old_wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
    old_RDA_status_update_time = ORPGRDA_status_update_time();

    Update_rda_control_menu_properties();
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Close" button in the RDA Control/Status
                  window.
 ************************************************************************/

static void RDA_control_close( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "RDA Close pushed" );
  XtDestroyWidget (Top_widget);
}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the RDA State radio buttons in the RDA
                  Control/Status window.
 ************************************************************************/

static void Change_rda_state_callback( Widget w, XtPointer y, XtPointer z )
{
  char buf[HCI_BUF_128];
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    User_selected_RDA_state_flag = (int) y;

    sprintf( buf, "You are about to change the RDA state.\nDo you want to continue?" );
    hci_confirm_popup( Top_widget, buf, Rda_state_callback, NULL );
    XtVaSetValues( w, XmNset, False, NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Yes" button from the RDA State confirmation
                  popup window.
 ************************************************************************/

static void Rda_state_callback( Widget w, XtPointer y, XtPointer z )
{
  /* If "RS_OPERATE" selected then check to see if it is different
     than the current state and RPG control is allowed.  If so,
     generate the command. */

  if( User_selected_RDA_state_flag == RS_OPERATE )
  {
    HCI_LE_log( "RDA Operate selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Change RDA to Operate state" );
      HCI_display_feedback( Cmd );

      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_OPERATE,
                        0, 0, 0, 0, NULL );
    }
  }
  else if( User_selected_RDA_state_flag == RS_OFFOPER )
  {
    /* If "RS_OFFOPER" selected then check to see if it is different
       than the current state and RPG control is allowed. If so,
       generate the command. */

    HCI_LE_log( "RDA Offline Operate selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Change RDA to Offline Operate state" );
      HCI_display_feedback( Cmd );
      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_OFFOPER,
                        0, 0, 0, 0, NULL );
    }
  }
  else if( User_selected_RDA_state_flag == RS_RESTART )
  {
    /* If "RS_RESTART" selected then check to see if it is
       different than the current state and RPG control is allowed.
       If so, generate the command. */
    HCI_LE_log( "RDA Restart selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Change RDA to Restart state" );
      HCI_display_feedback( Cmd );
      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_RESTART,
                        0, 0, 0, 0, NULL );
    }
  }
  else if( User_selected_RDA_state_flag == RS_STANDBY )
  {
    /* If "RS_STANDBY" selected then check to see if it is
       different than the current state and RPG control is allowed.
       If so, generate the command. */
    HCI_LE_log( "RDA Standby selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Change RDA to Standby state" );
      HCI_display_feedback( Cmd );
      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_STANDBY,
                        0, 0, 0, 0, NULL );
    }
  }

}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the Redundant Control radio buttons.
 ************************************************************************/

static void Toggle_red_control_callback( Widget w, XtPointer y, XtPointer z )
{
  char buf[HCI_BUF_256];
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    User_selected_RDA_red_control_flag = (int) y;

    if ( User_selected_RDA_red_control_flag == RDA_IS_CONTROLLING )
    {
      sprintf( buf, "You are about to change the control\nstate of this channel.\n\nThe active channel RDA must be in\neither Standby or Offline Operate.\n\nDo you want to continue?" );
    }
    else
    {
      sprintf( buf, "You are about to change the control\nstate of this channel.\n\nDo you want to continue?" );
    }

    hci_confirm_popup( Top_widget, buf, Accept_red_control_callback, Cancel_red_control_callback );
  }

  XtVaSetValues (w, XmNset, False, NULL );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "No" button from the Redundant Control
                  confirmation popup window.
 ************************************************************************/

static void Cancel_red_control_callback( Widget w, XtPointer y, XtPointer z )
{
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "No" button from the Redundant Control
                  confirmation popup window. 
 ************************************************************************/

static void Accept_red_control_callback( Widget w, XtPointer y, XtPointer z )
{
  int channel = ORPGRED_channel_num( ORPGRED_MY_CHANNEL );

  if( User_selected_RDA_red_control_flag == RDA_IS_CONTROLLING )
  {
    sprintf( Cmd, "Command RDA channel %d to Controlling", channel );
    HCI_LE_log( Cmd );

    /* Generate a feedback message which can be displayed
       in the RPG Control/Status window. */
    HCI_display_feedback( Cmd );
    ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_CHAN_CTL,
                      0, 0, 0, 0, NULL );
  }
  else
  {
    sprintf( Cmd,"Command RDA channel %d to Non-controlling", channel );
    HCI_LE_log( Cmd );

    /* Generate a feedback message which can be displayed
       in the RPG Control/Status window. */
    HCI_display_feedback( Cmd );
    ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_CHAN_NONCTL,
                      0, 0, 0, 0, NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the RDA control radio buttons.
 ************************************************************************/

static void Toggle_RDA_control_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    if( (int) y == CS_LOCAL_ONLY )
    {
      HCI_LE_log( "Enable RDA control of RDA selected" );

      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
       sprintf( Cmd, "Enable RDA control of RDA" );
       HCI_display_feedback( Cmd );
       ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_ENALOCAL,
                         0, 0, 0, 0, NULL );
    }
    else
    {
      HCI_LE_log( "Request RPG to control RDA selected" );

      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Request RPG to control RDA" );
      HCI_display_feedback( Cmd );
      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_REQREMOTE,
                         0, 0, 0, 0, NULL );
    }

    XtVaSetValues( w, XmNset, False, NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the RDA Power Source radio buttons.
 ************************************************************************/

static void Switch_power_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    Verify_power_source_change( w, y );
    XtVaSetValues( w, XmNset, False, NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  to Initiate a Performance Check.
 ************************************************************************/

static void Perfcheck_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state;
  char buf[HCI_BUF_256];

  state = (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    if( ORPGRDA_get_status( RS_RDA_STATUS ) != RS_OPERATE ||
        ORPGRDA_get_status( RS_CONTROL_STATUS ) != CS_RPG_REMOTE )
    {
      sprintf( buf, "A Performance Check can only be initated when\nthe RDA is in Operate and the RPG has control\nof the RDA." );
      hci_error_popup( Top_widget, buf, NULL );
    }
    else
    {
      sprintf( buf, "You have selected to force a Performance Check\nafter completion of the current volume scan. The\nPerformance Check will take approximately two\nminutes to complete.\n\nDo you wish to continue?" );
      hci_confirm_popup( Top_widget, buf, Accept_perfcheck_callback, NULL );
    }
    XtVaSetValues( w, XmNset, False, NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user accepts to
                  initiate a performance check.
 ************************************************************************/

static void Accept_perfcheck_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Performance Check selected" );

  /* Generate a feedback message which can be displayed
     in the RPG Control/Status window. */
  sprintf( Cmd, "Initiating Performance Check after current volume" );
  HCI_display_feedback( Cmd );
  ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_PERF_CHECK,
                    0, 0, 0, 0, NULL );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Get Status" button in the RDA Control/Status
                  window.
 ************************************************************************/

static void Request_RDA_status_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Request RDA status data selected" );

  /* Generate a feedback message which can be displayed
     in the RPG Control/Status window. */
  sprintf( Cmd, "Requesting RDA Status data" );
  HCI_display_feedback( Cmd );
  ORPGRDA_send_cmd( COM4_REQRDADATA, HCI_INITIATED_RDA_CTRL_CMD, DREQ_STATUS,
                    0, 0, 0, 0, NULL );
}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the Spot Blanking radio buttons.
                  NOTE: This is a restricted command and requires
                  a password to invoke.
 ************************************************************************/

static void Toggle_spot_blanking_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    Verify_spot_blanking( w, y );
    XtVaSetValues( w, XmNset, False, NULL );
  }
}

/************************************************************************
     Description: This function is activated after the user selects
                  one of the Spot Blanking radio buttons.  It
                  generates a confrmation popup window.
 ************************************************************************/

static void Verify_spot_blanking( Widget w, XtPointer y )
{
  char buf[HCI_BUF_128];

  User_selected_spot_blank_flag = (int) y;

  sprintf( buf, "You are about to change the spot blanking mode.\nDo you want to continue?" );
  hci_confirm_popup( Top_widget, buf, Spot_blanking_callback, NULL );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Yes" button from the Spot Blanking
                  confirmation popup window.
 ************************************************************************/

static void Spot_blanking_callback( Widget w, XtPointer y, XtPointer z )
{
  if( User_selected_spot_blank_flag == SB_DISABLED )
  {
    /* If "SPOT_BLANKING_DISABLED" selected then check to see if 
       it is different than the current state and RPG control is
       allowed.  If so, generate the command. */
    HCI_LE_log( "Spot Blanking Status Disable selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Disable Spot Blanking Status" );
      HCI_display_feedback( Cmd );
      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_SB_DIS,
                        0, 0, 0, 0, NULL );
    }
  }
  else if( User_selected_spot_blank_flag == SB_ENABLED )
  {
    /* If "SPOT_BLANKING_ENABLED" selected then check to see if 
       it is different than the current state and RPG control is
       allowed.  If so, generate the command. */
    HCI_LE_log( "Spot Blanking Status Enable selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      strcpy( Cmd, "Enable Spot Blanking Status" );
      HCI_display_feedback( Cmd );
      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD, CRDA_SB_ENAB,
                        0, 0, 0, 0, NULL );
    }
  }
}

/************************************************************************
     Description: This function is used to update the objects in the
                  RDA Control/Status window.
 ************************************************************************/

#define MIN_WIDTH		673

static void Update_rda_control_menu_properties()
{
  XmString string;
  Pixel highlight_color;
  Pixel foreground_color;
  Boolean sensitivity;
  Dimension min_width = (Dimension) MIN_WIDTH;
  Dimension width;
  short moments = 0;
  int wbstat = 0;
  int blanking = 0;
  int status = 0;
  int calib = 0;
  int other_ch_state = 0;
  int my_ch_state = 0;
  int rs_cntrl_stat = 0;
  int spot_blank_flag = 0;
  char buf[80];
  time_t rtime = 0;
  time_t ltime = 0;
  int fg = 0;
  int bg = 0;
  int month = 0;
  int day = 0;
  int year = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  hci_perfcheck_info_t *pc_info = (hci_perfcheck_info_t *) NULL;

  foreground_color = Button_fg;

  /* Check the display blanking flag.  If it is non-zero, then
     display blanking is enabled and only the RDA status element
     which corresponds to the half word index defined by the flag
     is displayed. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
  blanking = ORPGRDA_get_wb_status( ORPGRDA_DISPLAY_BLANKING );
  other_ch_state = ORPGRED_rda_control_status( ORPGRED_OTHER_CHANNEL );
  my_ch_state = ORPGRED_rda_control_status( ORPGRED_MY_CHANNEL );
  rs_cntrl_stat = ORPGRDA_get_status( RS_CONTROL_STATUS );
  spot_blank_flag = ORPGRDA_get_status( RS_SPOT_BLANKING_STATUS );

  if( blanking )
  {
    highlight_color = Button_fg;
  }
  else
  {
    highlight_color = White_color;
  }

  /* If the RDA Control window exists, then update the child widget
     properties based on the RDA control and wideband states. */

  if( Top_widget != (Widget) NULL )
  {
    if( spot_blank_flag != SPOT_BLANKING_NOT_INSTALLED )
    {
      XtVaSetValues( Top_widget,
                     XmNmaxWidth, (Dimension) 1024,
                     XmNallowShellResize, True,
                     NULL );

      XtManageChild( Lock_button );
      XtManageChild( Spot_blanking_frame );
      XtManageChild( XtParent( Spot_blanking_frame ) );

      XtVaGetValues( Top_widget, XmNwidth, &width, NULL );

      if( width < min_width )
      {
         width = min_width;
      }
         
      /* This fixes the size of this widget. */

      XtVaSetValues( Top_widget,
                     XmNminWidth, width,
                     XmNmaxWidth, width,
                     NULL );

      XtVaSetValues( Top_widget, XmNallowShellResize, False, NULL );
    }

    /* If the wideband is not connected, then we want to
       desensitize the "Get Status" button. */

    if( wbstat != RS_CONNECTED )
    {
      XtSetSensitive( Status_button, False );
    }
    else
    {
      XtSetSensitive( Status_button, True );
    }

    /* If the RDA is in control, desensitize the commands which cannot
       be invoked. */

    if( ( rs_cntrl_stat == CS_RPG_REMOTE || rs_cntrl_stat == CS_EITHER ) &&
        wbstat == RS_CONNECTED )
    {
      sensitivity = True;
    }
    else
    {
      sensitivity = False;
    }

    if( Redundant_type == HCI_FAA_SYSTEM &&  
        other_ch_state == ORPGRED_RDA_CONTROLLING )
    {
      XtVaSetValues( Rda_operate_button,
                     XmNsensitive, False,
                     XmNset, False,
                     NULL );
    }
    else
    {
      XtVaSetValues( Rda_operate_button,
                     XmNsensitive, sensitivity,
                     XmNset, False,
                     NULL );
    }

    XtVaSetValues( Rda_offline_operate_button,
                   XmNsensitive, sensitivity,
                   XmNset, False,
                   NULL );

    XtVaSetValues( Rda_restart_button,
                   XmNsensitive, sensitivity,
                   XmNset, False,
                   NULL );

    XtVaSetValues( Rda_standby_button,
                   XmNsensitive, sensitivity,
                   XmNset, False,
                   NULL );


    XtVaSetValues( Power_utility_button,
                   XmNsensitive, sensitivity,
                   XmNset, False,
                   NULL );

    XtVaSetValues( Power_auxiliary_button,
                   XmNsensitive, sensitivity,
                   XmNset, False,
                   NULL );

    XtVaSetValues( Perfcheck_button,
                   XmNsensitive, sensitivity,
                   XmNset, False,
                   NULL );

    if( hci_lock_URC_selected() || hci_lock_ROC_selected() )
    {
      XtVaSetValues( Spot_blanking_disable_button,
                     XmNsensitive, False,
                     XmNforeground, Loca_fg,
                     XmNset, False,
                     NULL );

      XtVaSetValues( Spot_blanking_enable_button,
                     XmNsensitive, False,
                     XmNforeground, Loca_fg,
                     XmNset, False,
                     NULL );
    }
    else
    {
      XtVaSetValues( Spot_blanking_disable_button,
                     XmNsensitive, False,
                     XmNforeground, Text_fg,
                     XmNset, False,
                     NULL );

      XtVaSetValues( Spot_blanking_enable_button,
                     XmNsensitive, False,
                     XmNforeground, Text_fg,
                     XmNset, False,
                     NULL );
    }

    if( Unlocked_loca == HCI_YES_FLAG )
    {
      XtVaSetValues( Spot_blanking_disable_button,
                     XmNsensitive, sensitivity,
                     XmNforeground, Text_fg,
                     XmNset, False,
                     NULL );

      XtVaSetValues( Spot_blanking_enable_button,
                     XmNsensitive, sensitivity,
                     XmNforeground, Text_fg,
                     XmNset, False,
                     NULL );
    }
    else
    {
      XtVaSetValues( Spot_blanking_disable_button,
                     XmNsensitive, False,
                     XmNforeground, Text_fg,
                     XmNset, False,
                     NULL );

      XtVaSetValues( Spot_blanking_enable_button,
                     XmNsensitive, False,
                     XmNforeground, Text_fg,
                     XmNset, False,
                     NULL );
    }

    if( Redundant_type == HCI_FAA_SYSTEM )
    {
      if( Unlocked_loca == HCI_YES_FLAG )
      {
        XtVaSetValues( Red_control_button,
                       XmNsensitive, False,
                       XmNforeground, Loca_fg,
                       XmNset, False,
                       NULL );

        if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) != 1 )
        {
          XtVaSetValues( Red_non_control_button,
                         XmNsensitive, False,
                         XmNforeground, Loca_fg,
                         XmNset, False,
                         NULL );
        }
      }
      else
      {
        XtVaSetValues( Red_control_button,
                       XmNsensitive, False,
                       XmNforeground, Text_fg,
                       XmNset, False,
                       NULL );

        if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) != 1 )
        {
          XtVaSetValues( Red_non_control_button,
                         XmNsensitive, False,
                         XmNforeground, Text_fg,
                         XmNset, False,
                         NULL );
        }
      }

      if( Unlocked_loca == HCI_YES_FLAG )
      {
        XtVaSetValues( Red_control_button,
                       XmNsensitive, sensitivity,
                       XmNforeground, Text_fg,
                       XmNset, False,
                       NULL );

        if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) != 1 )
        {
          XtVaSetValues( Red_non_control_button,
                         XmNsensitive, sensitivity,
                         XmNforeground, Text_fg,
                         XmNset, False,
                         NULL );
        }
      }
      else
      {
        XtVaSetValues( Red_control_button,
                       XmNsensitive, False,
                       XmNforeground, Text_fg,
                       XmNset, False,
                       NULL );

        if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) != 1 )
        {
          XtVaSetValues( Red_non_control_button,
                         XmNsensitive, False,
                         XmNforeground, Text_fg,
                         XmNset, False,
                         NULL );
        }
      }
    }

    /* Check to see if display blanking disabled for next item */

    if( ( blanking && rs_cntrl_stat == CS_LOCAL_ONLY ) ||
        !( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      XtSetSensitive( Rda_operate_button, False );
      XtSetSensitive( Rda_offline_operate_button, False );
      XtSetSensitive( Rda_restart_button, False );
      XtSetSensitive( Rda_standby_button, False );

      if( blanking != RS_CONTROL_STATUS )
      {
        sprintf( buf, "    UNKNOWN    " );
      }
      else
      {
        switch( ORPGRDA_get_status( RS_RDA_STATUS ) )
        {
          case RS_OPERATE :

            sprintf( buf, "    OPERATE    " );
            break;

          case RS_OFFOPER:

            sprintf( buf, "OFFLINE OPERATE" );
            break;

          case RS_RESTART:

            sprintf( buf, "    RESTART    " );
            break;

          case RS_STARTUP:

            sprintf( buf, "   START-UP    " );
            break;

          case RS_STANDBY:

            sprintf( buf, "    STANDBY    " );
            break;

          default:

            sprintf( buf, "    UNKNOWN    " );
            break;
        }
      }
    }
    else
    {
      switch( ORPGRDA_get_status( RS_RDA_STATUS ) )
      {
        case RS_OPERATE :

          sprintf( buf, "    OPERATE    " );
          XtSetSensitive( Rda_operate_button, False );
          break;

        case RS_OFFOPER:

          sprintf( buf, "OFFLINE OPERATE" );
          XtSetSensitive( Rda_offline_operate_button, False );
          break;

        case RS_RESTART:

          sprintf( buf, "    RESTART    " );
          XtSetSensitive( Rda_restart_button, False );
          break;

        case RS_STARTUP:
          sprintf( buf, "   START-UP    " );
          break;

        case RS_STANDBY:
          sprintf( buf, "    STANDBY    " );
          XtSetSensitive( Rda_standby_button, False );
          break;

        default:

          sprintf( buf, "    UNKNOWN    " );
          break;
      }
    }

    string = XmStringCreateLocalized( buf );
    XtVaSetValues( Current_state_label, XmNlabelString, string, NULL );
    XmStringFree( string );

    if( Redundant_type == HCI_FAA_SYSTEM )
    {
      if( wbstat != RS_CONNECTED )
      {
        sprintf( buf, "      UNKNOWN       " );
      }
      else
      {
        if( my_ch_state == ORPGRED_RDA_CONTROLLING )
        {
          XtSetSensitive( Red_control_button, False );
          sprintf( buf, "    CONTROLLING     " );
        }
        else if( my_ch_state == ORPGRED_RDA_NON_CONTROLLING )
        {
          if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) != 1 )
          {
            XtSetSensitive( Red_non_control_button, False );
          }
          sprintf( buf, "  NON-CONTROLLING   " );
        }
        else
        {
          sprintf( buf, "      UNKNOWN       " );
        }
      }

      string = XmStringCreateLocalized( buf );
      XtVaSetValues( Current_red_local_status, XmNlabelString, string, NULL );
      XmStringFree( string );

      {
        ltime = ORPGRED_adapt_dat_time( ORPGRED_MY_CHANNEL );
        rtime = ORPGRED_adapt_dat_time( ORPGRED_OTHER_CHANNEL );

        if( rtime == ltime )
        {
          fg = Text_fg;
          bg = Base_bg2;
        }
        else
        {
          fg = White_color;
          bg = Alarm_color;
        }

        if( ltime > 0 )
        {
          unix_time( &ltime, &year, &month, &day, &hour, &minute, &second );
        }
        else
        {
          month = 0;
          day = 0;
          year = 0;
          hour = 0;
          minute = 0;
          second = 0;
        }

        sprintf( buf, "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d UT",
                 month, day, year%100, hour, minute, second );
        string = XmStringCreateLocalized( buf );
        XtVaSetValues( Current_red_local_time,
                       XmNlabelString, string,
                       XmNforeground, fg,
                       XmNbackground, bg,
                       NULL );
        XmStringFree( string );

        if( rtime > 0 )
        {
          unix_time( &ltime, &year, &month, &day, &hour, &minute, &second );
        }
        else
        {
          month = 0;
          day = 0;
          year = 0;
          hour = 0;
          minute = 0;
          second = 0;
        }

        sprintf( buf, "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d UT",
                 month, day, year%100, hour, minute, second );
        string = XmStringCreateLocalized( buf );
        XtVaSetValues( Current_red_red_time,
                       XmNlabelString, string,
                       XmNforeground, fg,
                       XmNbackground, bg,
                       NULL );
        XmStringFree( string );
      }

      if( other_ch_state == ORPGRED_RDA_CONTROLLING )
      {
        sprintf( buf, "    CONTROLLING     " );
      }
      else if( other_ch_state == ORPGRED_RDA_NON_CONTROLLING )
      { 
        sprintf( buf, "  NON-CONTROLLING   " );
      }
      else
      {
        sprintf( buf, "      UNKNOWN       " );
      }

      string = XmStringCreateLocalized( buf );
      XtVaSetValues( Current_red_red_status,
                     XmNlabelString, string,
                     NULL );
      XmStringFree( string );
    }

    /* Check to see if display blanking disabled for next item */

    if( ( blanking && rs_cntrl_stat == CS_LOCAL_ONLY ) ||
        !( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      XtSetSensitive( Control_rda_button, False );
      XtSetSensitive( Control_rpg_button, False );

      if( blanking != RS_CONTROL_STATUS )
      {
        sprintf( buf, "  UNKNOWN    " );
      }
      else
      {
        switch( rs_cntrl_stat )
        {
          case CS_LOCAL_ONLY:

            sprintf( buf, "LOCAL (RDA) " );
            break;

          case CS_RPG_REMOTE:

            sprintf( buf, "REMOTE (RPG)" );
            break;

          case CS_EITHER:

            sprintf( buf, "   EITHER   " );
            break;

          default:

            sprintf( buf, "  UNKNOWN   " );
            break;
        }
      }
    }
    else
    {
      switch( rs_cntrl_stat )
      {
        case CS_LOCAL_ONLY:

          XtSetSensitive( Control_rda_button, False );
          XtSetSensitive( Control_rpg_button, True );
          sprintf( buf, "LOCAL (RDA) " );
          break;

        case CS_RPG_REMOTE:

          XtSetSensitive( Control_rda_button, True );
          XtSetSensitive( Control_rpg_button, False );
          sprintf( buf, "REMOTE (RPG)" );
          break;

        case CS_EITHER:

          XtSetSensitive( Control_rda_button, False );
          XtSetSensitive( Control_rpg_button, False );
          sprintf( buf, "   EITHER   " );
          break;

        default:

          sprintf( buf, "  UNKNOWN   " );
          break;
      }
    }

    string = XmStringCreateLocalized( buf );
    XtVaSetValues( Current_control_label, XmNlabelString, string, NULL );
    XmStringFree( string );

    /* Check to see if display blanking disabled for next item */

    if( ( blanking && rs_cntrl_stat == CS_LOCAL_ONLY ) ||
        !( wbstat == RS_CONNECTED ||
           wbstat == RS_DISCONNECT_PENDING ) )
    {
      XtSetSensitive( Power_utility_button, False );
      XtSetSensitive( Power_auxiliary_button, False );

      if( blanking != RS_AUX_POWER_GEN_STATE )
      {
        sprintf( buf, " UNKNOWN " );
      }
      else
      {
        if( ( ORPGRDA_get_status( RS_AUX_POWER_GEN_STATE ) & 1 ) )
        {
          sprintf( buf, "AUXILIARY" );
        }
        else
        {
          sprintf( buf, " UTILITY " );
        }
      }
    }
    else
    {
      if( ( ORPGRDA_get_status( RS_AUX_POWER_GEN_STATE ) & 1 ) )
      {
        sprintf( buf, "AUXILIARY" );
        XtSetSensitive( Power_auxiliary_button, False );
      }
      else
      {
        sprintf( buf, " UTILITY " );
        XtSetSensitive( Power_utility_button, False );
      }
    }

    string = XmStringCreateLocalized( buf );
    XtVaSetValues( Current_power_label, XmNlabelString, string, NULL );
    XmStringFree( string );

    /* Display blanking not valid for next item */

    if( ( rs_cntrl_stat == CS_LOCAL_ONLY ) ||
        !( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      XtSetSensitive( Perfcheck_button, False );

      pc_info = hci_get_perfcheck_info();
      if( pc_info->state == HCI_PC_STATE_PENDING )
      {
        sprintf( buf, "  PENDING  " );
      }
      else if( pc_info->state == HCI_PC_STATE_AUTO )
      {
        sprintf( buf, " AUTOMATIC " );
      }
      else
      {
        sprintf( buf, "  UNKNOWN  " );
      }
    }
    else
    {
      pc_info = hci_get_perfcheck_info();
      if( pc_info->state == HCI_PC_STATE_PENDING )
      {
        sprintf( buf, "  PENDING  " );
        XtSetSensitive( Perfcheck_button, False );
      }
      else if( pc_info->state == HCI_PC_STATE_AUTO )
      {
        sprintf( buf, " AUTOMATIC " );
        XtSetSensitive( Perfcheck_button, True );
      }
      else
      {
        sprintf( buf, "  UNKNOWN  " );
        XtSetSensitive( Perfcheck_button, True );
      }
    }

    string = XmStringCreateLocalized( buf );
    XtVaSetValues( Current_perfcheck_label, XmNlabelString, string, NULL );
    XmStringFree( string );

    /* Update the properties of the spot blanking items. */

    if( ( blanking && rs_cntrl_stat == CS_LOCAL_ONLY ) ||
        !( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      XtSetSensitive( Spot_blanking_disable_button, False );
      XtSetSensitive( Spot_blanking_enable_button, False );

      if( blanking != spot_blank_flag )
      {
        sprintf( buf, "UNKNOWN " );
      }
      else
      {
        if( spot_blank_flag == SB_DISABLED )
        {
          sprintf( buf, "DISABLED" );
        }
        else
        {
          sprintf( buf, "ENABLED " );
        }
      }
    }
    else
    {
      if( spot_blank_flag == SB_DISABLED )
      {
        XtSetSensitive( Spot_blanking_disable_button, False );
        sprintf( buf, "DISABLED" );
      }
      else
      {
        XtSetSensitive( Spot_blanking_enable_button, False );
        sprintf( buf, "ENABLED " );
      }
    }

    string = XmStringCreateLocalized( buf );
    XtVaSetValues( Current_spot_blanking_label, XmNlabelString, string, NULL );
    XmStringFree( string );

    if( ( blanking == 0 || blanking == RS_RDA_CONTROL_AUTH ) &&
        ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      status = ORPGRDA_get_status( RS_RDA_CONTROL_AUTH );

      if( status == CA_LOCAL_CONTROL_REQUESTED )
      {
        sprintf( buf, "LOCAL REQUESTED " );
      }
      else if( status == CA_REMOTE_CONTROL_ENABLED )
      {
        sprintf( buf, " REMOTE ENABLED " );
      }
      else
      {
        sprintf( buf, "   NO ACTION    " );
      }

      highlight_color = White_color;
    }
    else
    {
      sprintf( buf, "    UNKNOWN     " );
      highlight_color = Base_bg2;
    }

    string = XmStringCreateSimple( buf );
    XtVaSetValues( Control_authorization_label,
                   XmNlabelString, string,
                   XmNbackground, highlight_color,
                   NULL );
    XmStringFree( string );

    if( ( blanking == 0 || blanking == RS_AVE_TRANS_POWER ) &&
        ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      sprintf( buf, "   %4d Watts   ",
      ORPGRDA_get_status( RS_AVE_TRANS_POWER ) );
      highlight_color = White_color;
    }
    else
    {
      sprintf( buf, "    UNKNOWN     " );
      highlight_color = Base_bg2;
    }

    string = XmStringCreateSimple( buf );
    XtVaSetValues( Ave_transmitter_power_label,
                   XmNlabelString, string,
                   XmNbackground, highlight_color,
                   NULL );
    XmStringFree( string );

    if( ( blanking == 0 || blanking == RS_REFL_CALIB_CORRECTION ) &&
        ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      calib = ORPGRDA_get_status( RS_REFL_CALIB_CORRECTION );
      sprintf( buf," %7.2f dB ", ((float) calib)/100.0 );
      highlight_color = White_color;
    }
    else
    {
      sprintf( buf, "  UNKNOWN   " );
      highlight_color = Base_bg2;
    }

    string = XmStringCreateSimple( buf );
    XtVaSetValues( Horiz_calibration_correction_label,
                   XmNlabelString, string,
                   XmNbackground, highlight_color,
                   NULL );
    XmStringFree( string );

    if( ( blanking == 0 || blanking == RS_REFL_CALIB_CORRECTION ) &&
        ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      calib = ORPGRDA_get_status( RS_VC_REFL_CALIB_CORRECTION );
      sprintf( buf, " %7.2f dB ", ((float) calib)/100.0 );
      highlight_color = White_color;
    }
    else
    {
      sprintf( buf, "  UNKNOWN   " );
      highlight_color = Base_bg2;
    }

    string = XmStringCreateSimple( buf );
    XtVaSetValues( Vert_calibration_correction_label,
                   XmNlabelString, string,
                   XmNbackground, highlight_color,
                   NULL );
    XmStringFree( string );

    /* Highlight the currently transmitted data types. */

    if( ( blanking == 0 || blanking == RS_DATA_TRANS_ENABLED ) &&
        ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) )
    {
      moments = ( ORPGRDA_get_status( RS_DATA_TRANS_ENABLED ) >> 2 ) & 7;

      switch( moments )
      {
        case 0 : /*  No moments enabled */

          sprintf( buf, "    NONE    " );
          break;

        case 1 : /*  Reflectivity enabled */

          sprintf( buf, "     R      " );
          break;

        case 2 : /*  Velocity enabled */

          sprintf( buf, "     V      " );
          break;

        case 3 : /*  Reflectivity, Velocity enabled */

          sprintf( buf, "   R   V    " );
          break;

        case 4 : /*  Spectrum Width enabled */

          sprintf( buf, "     W      " );
          break;

        case 5 : /*  Reflectivity, Spectrum Width enabled */

          sprintf( buf, "   R   W    " );
          break;

        case 6 : /*  Velocty, Spectrum Width enabled */

          sprintf( buf, "   V   W    " );
          break;

        case 7 : /*  Reflectivity, Velocity, Spectrum Width enabled */

          sprintf( buf, "  R  V  W   " );
          break;
      }

      highlight_color = White_color;
    }
    else
    {
      highlight_color = Base_bg2;
      sprintf( buf, "  UNKNOWN   " );
    }

    string = XmStringCreateLocalized( buf );
    XtVaSetValues( Moments_enabled_label,
                   XmNlabelString, string,
                   XmNbackground, highlight_color,
                   NULL );
    XmStringFree( string );
  }
}

/************************************************************************
     Description: This function is activated when the user selects the
                  "RDA Alarms" button from the RDA Control/Status* window.
 ************************************************************************/

static void Hci_select_rda_alarms_callback( Widget w, XtPointer y, XtPointer z )
{
  char task_name[100] = "";

  sprintf( task_name, "hci_rda_orda -A %d -name \"RDA Alarms\"", HCI_get_channel_number() );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );
  hci_activate_child( HCI_get_display(), RootWindowOfScreen( HCI_get_screen() ),
                      task_name, "hci_rda_orda", "RDA Alarms",  -1 );
}

/************************************************************************
     Description: This function is activated when the user selects the
                  "VCP" button from the RDA Control/Status window.
 ************************************************************************/

static void Hci_select_vcp_callback( Widget w, XtPointer y, XtPointer z )
{
  char task_name[100] = "";

  sprintf( task_name, "hci_vcp -A %d -name \"VCP and Mode Control\"", HCI_get_channel_number() );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );
  hci_activate_child( HCI_get_display(), RootWindowOfScreen( HCI_get_screen() ),
                      task_name, "hci_vcp", "VCP and Mode Control", -1 );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Yes" button in the Lock RMS confirmation
                  popup window.
 ************************************************************************/

static void Hci_lock_button_callback( Widget w, XtPointer y, XtPointer z )
{
  int ret = 0;

  if( Change_rms_lock )
  {
    if( ( ret = rms_rda_rpg_lock( RMS_RDA_LOCK, RMS_SET_LOCK ) ) < 0 )
    {
      HCI_LE_error( "Unable to set RDA lock" );
    }
    else
    {
      Command_lock = HCI_YES_FLAG;
    }
  }
  else
  {
    if( ( ret = rms_rda_rpg_lock( RMS_RDA_LOCK, RMS_CLEAR_LOCK ) ) < 0 )
    {
      HCI_LE_error( "Unable to clear RDA lock" );
    }
    else
    {
      Command_lock = HCI_NO_FLAG;
    }
  }

  Set_rda_button();
}

/************************************************************************
     Description: This function is activated when the user selects
                  the Lock RMS check box.
 ************************************************************************/

static void Hci_lock_button_verify_callback( Widget w, XtPointer y, XtPointer z )
{
  char buf[HCI_BUF_128];
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  Change_rms_lock = state->set;

  if( rms_get_lock_status( RMS_RDA_LOCK ) )
  {
    sprintf( buf,"You are about to enable\nthe RMMS to RDA commands.\nDo you want to continue?" );
  }
  else
  {
    sprintf ( buf,"You are about to disable\nthe RMMS to RDA commands.\nDo you want to continue?" );
  }

  hci_confirm_popup( Top_widget, buf, Hci_lock_button_callback, Set_rda_button );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "No" button from the Lock RMS confirmation
                  popup window.
 ************************************************************************/

static void Set_rda_button()

{
  int ret = rms_get_lock_status( RMS_RDA_LOCK );

  if( ret < 0 )
  {
    HCI_LE_error( "Unable to get RDA lock status" );
  }
  else
  { 
    if( ret == RMS_COMMAND_LOCKED )
    {
      XtVaSetValues( Lock_rms, XmNset, True, NULL );
    }
    else if( ret == RMS_COMMAND_UNLOCKED )
    {
      XtVaSetValues( Lock_rms, XmNset, False, NULL );
    }
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the lock button or selects a LOCA radio button or
                  enters a password in the Password window.
 ************************************************************************/

static int Hci_rda_control_security()
{
  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_URC_unlocked() || hci_lock_ROC_unlocked() )
    {
      Unlocked_loca = HCI_YES_FLAG;
    }
  }
  else if( hci_lock_close() && Unlocked_loca == HCI_YES_FLAG )
  {
      Unlocked_loca = HCI_NO_FLAG;
  }

  Update_rda_control_menu_properties();

  return HCI_LOCK_PROCEED;
}

/************************************************************************
     Description: This function is called when RDA configuration changes.
 ************************************************************************/

void Rda_config_change()
{
   if( Top_widget != (Widget) NULL && !Config_change_popup )
   {
     Config_change_popup = HCI_YES_FLAG; /* Indicate popup has been launched. */
     hci_rda_config_change_popup();
   }
   else
   {
     return;
   }
}

/************************************************************************
     Description: This function is called when the Redundant Manager
                  posts a change.
 ************************************************************************/

void Redundant_status_change_cb( int fd, LB_id_t mid, int minfo, void *arg )
{
  Redundant_status_change_flag = HCI_YES_FLAG;
}

