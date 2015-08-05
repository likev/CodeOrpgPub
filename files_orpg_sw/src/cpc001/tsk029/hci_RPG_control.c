/************************************************************************
 *      Module:  hci_RPG_control.c                                      *
 *                                                                      *
 *      Description:  This module is used by the ORPG HCI to define     *
 *                    and display the RPG Control menu.                 *
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/08 17:58:11 $
 * $Id: hci_RPG_control.c,v 1.62 2014/07/08 17:58:11 steves Exp $
 * $Revision: 1.62 $
 * $State: Exp $
 */

/* Local include files */

#include <hci.h>
#include <rms_util.h>

/* Enumerations and Definitions */

enum{ RPG_DO_NOTHING,
      RPG_RESTART_ALL,
      RPG_RESTART_CLEAN,
      RPG_RESTART_TASK,
      RPG_SHUTDOWN_OFF,
      RPG_SHUTDOWN_STANDBY };

/* Static/global variables */

static Widget Top_widget = (Widget) NULL;
static Widget Rpg_state = (Widget) NULL;
static Widget ArchiveII_button = (Widget) NULL;
static Widget Startup_button = (Widget) NULL;
static Widget Clean_startup_button = (Widget) NULL;
static Widget Activate_button = (Widget) NULL;
static Widget Shutdown_button = (Widget) NULL;
static Widget Standby_button = (Widget) NULL;
static Widget Lock_button = (Widget) NULL;
static Widget Lock_widget = (Widget) NULL;
static Widget Option_button[HCI_MAX_INIT_OPTIONS] = { (Widget) NULL };
static Pixel  Base_bg = (Pixel) NULL;
static Pixel  Button_fg = (Pixel) NULL;
static Pixel  Button_bg = (Pixel) NULL;
static Pixel  Text_fg = (Pixel) NULL;
static Pixel  White_color = (Pixel) NULL;
static Pixel  Normal_color = (Pixel) NULL;
static Pixel  Alarm_color = (Pixel) NULL;
static Pixel  Warning_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;

static int Command_lock = HCI_NO_FLAG; /* RMS locked from sending commands */
static int Change_rms_lock = HCI_NO_FLAG; /* RMS lock state change flag */
static int Unlocked_urc = HCI_NO_FLAG; /* lock state for urc users */
static int Unlocked_roc = HCI_NO_FLAG; /* lock state for roc users */
static int Channel_number = 1;
static int Rpg_command_to_run = RPG_DO_NOTHING; /* command to run */
static int Command_to_run_is_verified = HCI_NO_FLAG;
static int Options_activate_flag = HCI_NO_FLAG;
static unsigned int Options = 0; /* Options bit mask */
static char Cmd[HCI_BUF_128]; /* shared buffer for feedback message */

/* Prototypes */

static void Timer_proc();
static void Rpg_control_button_close( Widget, XtPointer, XtPointer );
static void Rpg_options_button_activate( Widget, XtPointer, XtPointer );
static void Cancel_rpg_options_button_activate( Widget, XtPointer, XtPointer );
static void Verify_rpg_options_button_activate( Widget, XtPointer, XtPointer );
static void Rpg_init_options_toggle( Widget, XtPointer, XtPointer );
static void ArchiveII_button_callback( Widget, XtPointer, XtPointer );
static void Rpg_command_callback( Widget, XtPointer, XtPointer );
static void Rpg_lock_button_callback( Widget, XtPointer, XtPointer );
static void Rpg_lock_button_verify_callback( Widget, XtPointer, XtPointer );
static int  Rpg_startup_options_lock();
static void Verify_rpg_command_callback( Widget, XtPointer, XtPointer );
static void Cancel_rpg_command_callback( Widget, XtPointer, XtPointer );
static void Run_rpg_command();
static void Set_rpg_button();
static void Update_rpg_control_menu_properties();
static void Update_rpg_init_options_window();
static void Rpg_options_activate();

/************************************************************************
     Description: This is the main function for the RPG Control task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget form;
  Widget form_rowcol;
  Widget control_rowcol;
  Widget labels_rowcol;
  Widget system_frame;
  Widget system_rowcol;
  Widget archII_frame;
  Widget archII_rowcol;
  Widget init_frame;
  Widget init_rowcol;
  Widget button;
  Widget frame;
  Widget rowcol;
  Widget label;
  XmString str;
  char temp_group[HCI_INIT_OPTIONS_GROUP_LEN] = "No Group";
  int i = 0;

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_RPC_TASK );

  /* Define the widgets for the RPG menu and display them. */

  Top_widget = HCI_get_top_widget();

  /* Add redundancy information if site FAA redundant */

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    Channel_number = ORPGRED_channel_num( ORPGRED_MY_CHANNEL );
  }

  /* Define miscellaneous varaibles */

  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Alarm_color = hci_get_read_color( ALARM_COLOR1 );
  Warning_color = hci_get_read_color( WARNING_COLOR );
  List_font = hci_get_fontlist( LIST );

  /* Use a form widget and rowcol to organize the various menu widgets. */

  form = XtVaCreateManagedWidget( "form",
                xmFormWidgetClass, Top_widget,
                XmNautoUnmanage, False,
                XmNbackground, Base_bg,
                NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  /* Define rowcol for Close button, lock widget, and RMS (if needed). */

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNisAligned, False,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, control_rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Rpg_control_button_close, NULL );

  if( HCI_has_rms() )
  {
    /* Blank label is for spacing purposes. */
    label = XtVaCreateManagedWidget( "                ",
                xmLabelWidgetClass, control_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

    Lock_button = XtVaCreateManagedWidget( "Lock RMS",
                xmToggleButtonWidgetClass, control_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                XmNselectColor, White_color,
                NULL );
                
    XtAddCallback( Lock_button,
                XmNvalueChangedCallback, Rpg_lock_button_verify_callback, NULL );
           
    /* Highlight the lock button based on lock stauts */
    Set_rpg_button();
           
    label = XtVaCreateManagedWidget( "              ",
                xmLabelWidgetClass, control_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );
  }
  else
  {
    /* Blank label is for spacing purposes. */
    label = XtVaCreateManagedWidget ("                                                ",
                xmLabelWidgetClass, control_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );
  }

  Lock_widget = hci_lock_widget( control_rowcol, Rpg_startup_options_lock, HCI_LOCA_URC | HCI_LOCA_ROC );

  /* Define rowcol for State label. */

  labels_rowcol = XtVaCreateManagedWidget( "labels_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNisAligned, False,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, control_rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  XtVaCreateManagedWidget( " State: ",
                xmLabelWidgetClass, labels_rowcol,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );  

  Rpg_state = XtVaCreateManagedWidget( "TRANSITION",
                xmLabelWidgetClass, labels_rowcol,
                XmNbackground, Normal_color,
                XmNforeground, Text_fg,
                XmNfontList, List_font,
                NULL );  

  /* Create a container to hold all system control selections. */

  system_frame = XtVaCreateManagedWidget( "top_frame",
                xmFrameWidgetClass, form_rowcol,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, labels_rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "RPG System Control",
                xmLabelWidgetClass, system_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNfontList, List_font,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

  system_rowcol = XtVaCreateManagedWidget( "system_rowcol",
                xmRowColumnWidgetClass, system_frame,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 2,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  Standby_button = XtVaCreateManagedWidget( "       Standby       ",
                xmPushButtonWidgetClass, system_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Standby_button,
                XmNactivateCallback, Rpg_command_callback, NULL );

  Shutdown_button = XtVaCreateManagedWidget( "      Shutdown       ",
                xmPushButtonWidgetClass, system_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Shutdown_button,
                XmNactivateCallback, Rpg_command_callback, NULL );

  Startup_button = XtVaCreateManagedWidget( "       Startup       ",
                xmPushButtonWidgetClass, system_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Startup_button,
                XmNactivateCallback, Rpg_command_callback, NULL );

  Clean_startup_button = XtVaCreateManagedWidget( "    Clean Startup    ",
                xmPushButtonWidgetClass, system_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Clean_startup_button,
                XmNactivateCallback, Rpg_command_callback, NULL );

  /* Blank label for vertical spacing. */

  label = XtVaCreateManagedWidget( " ",
                xmLabelWidgetClass, form_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  /* Create a container and selections for Archive-II. */

  archII_frame = XtVaCreateManagedWidget( "archII_frame",
                xmFrameWidgetClass, form_rowcol,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNrightAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Archive II Control",
                xmLabelWidgetClass, archII_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNfontList, List_font,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

  archII_rowcol = XtVaCreateManagedWidget( "archII_rowcol",
                xmRowColumnWidgetClass, archII_frame,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  /* Blank label is for spacing purposes. */

  label = XtVaCreateManagedWidget( "                      ",
                xmLabelWidgetClass, archII_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  ArchiveII_button = XtVaCreateManagedWidget( "  Archive II ",
                xmPushButtonWidgetClass, archII_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( ArchiveII_button,
                XmNactivateCallback, ArchiveII_button_callback,
                NULL );

  /* Blank label for vertical spacing. */

  label = XtVaCreateManagedWidget( " ",
                xmLabelWidgetClass, form_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  /* Create a container to hold all initialization selections. */

  init_frame = XtVaCreateManagedWidget( "top_frame",
                xmFrameWidgetClass, form_rowcol,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, system_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Initialization Control",
                xmLabelWidgetClass, init_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNfontList, List_font,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

  init_rowcol = XtVaCreateManagedWidget( "init_form",
                xmRowColumnWidgetClass, init_frame,
                XmNorientation, XmVERTICAL,
                XmNbackground, Base_bg,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  frame = XtVaCreateManagedWidget( "frame",
                xmFrameWidgetClass, init_rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, control_rowcol,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "label",
                xmLabelWidgetClass, frame,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_BEGINNING,
                NULL );

  str = XmStringCreateLtoR( "The following items denote various functions.\nThe selectivity state of each item depends\non the RPG state.  After items are selected\nthe Activate button invokes the function(s).", 
                XmFONTLIST_DEFAULT_TAG );

  XtVaSetValues( label, XmNlabelString, str, NULL );

  XmStringFree( str );

  rowcol = XtVaCreateManagedWidget( "rowcol",
                xmRowColumnWidgetClass, init_rowcol,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  /* Blank label is for spacing purposes. */
  label = XtVaCreateManagedWidget( "                        ",
                xmLabelWidgetClass, rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  Activate_button = XtVaCreateManagedWidget( "Activate",
                xmPushButtonWidgetClass, rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, label,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNsensitive, False,
                NULL );

  XtAddCallback( Activate_button,
                XmNactivateCallback, Rpg_options_button_activate, NULL );

  for( i = 0; i < HCI_num_init_options(); i++ )
  {
    if( strcmp( HCI_get_init_options_group(i), temp_group ) != 0 )
    {
      strcpy( temp_group, HCI_get_init_options_group(i) );

      /* Create a frame for new groups and create new option buttons. */

      frame = XtVaCreateManagedWidget( "action_frame",
                        xmFrameWidgetClass, init_rowcol,
                        XmNbackground, Base_bg,
                        NULL );

      label = XtVaCreateManagedWidget( temp_group,
                        xmLabelWidgetClass, frame,
                        XmNchildType, XmFRAME_TITLE_CHILD,
                        XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                        XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                        XmNfontList, List_font,
                        XmNforeground, Text_fg,
                        XmNbackground, Base_bg,
                        NULL );

      rowcol = XtVaCreateManagedWidget( "rowcol",
                        xmRowColumnWidgetClass, frame,
                        XmNorientation, XmVERTICAL,
                        XmNnumColumns, 1+HCI_num_init_options()/16,
                        XmNpacking, XmPACK_COLUMN,
                        XmNbackground, Base_bg,
                        NULL );
    }

    Option_button[i] = XtVaCreateManagedWidget( HCI_get_init_options_name(i),
                xmToggleButtonWidgetClass, rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNselectColor, White_color,
                XmNsensitive, False,
                XmNuserData, (XtPointer) i,
                NULL );

    XtAddCallback( Option_button[i],
                XmNvalueChangedCallback, Rpg_init_options_toggle,
                (XtPointer) i );
  }

  /* Update info. */

  HCI_PM( "Reading RPG Status information" );

  Update_rpg_control_menu_properties();

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
     Description: This is the timer function for the RPG Control task.
                  Its main purpose is to force a window refresh when
                  RPG status changes.
 ************************************************************************/

static void Timer_proc()
{
  static  int old_rpg_state = -1;
  static  int old_vcp =  0;
  static  int old_rda_control = -1;
          int i = 0;
          int current_vcp = 0;
          int rda_control = 0;
          Mrpg_state_t    mrpg;

  if( Command_to_run_is_verified == HCI_YES_FLAG )
  {
    Run_rpg_command();
    Command_to_run_is_verified = HCI_NO_FLAG;
    Rpg_command_to_run = RPG_DO_NOTHING;
  }

  current_vcp = abs( ORPGVST_get_vcp() );
  rda_control = ORPGRDA_get_status( RS_CONTROL_STATUS );

  if( ORPGMGR_get_RPG_states( &mrpg ) == 0 )
  {
    if( ( mrpg.state != old_rpg_state ) ||
        ( rda_control != old_rda_control ) ||
        ( current_vcp != old_vcp ) )
    {
      /* If the RPG state has changed, then we want to
         unset all toggle buttons. */

      if( mrpg.state != old_rpg_state )
      {
        Options = 0;
        for( i = 0; i < HCI_num_init_options(); i++ )
        {
          XmToggleButtonSetState( Option_button[i], False, False );
        }
      }

      old_rpg_state = mrpg.state;
      old_vcp = current_vcp;
      old_rda_control = rda_control;

      Update_rpg_control_menu_properties();
    }
  }

  if( Options_activate_flag == HCI_YES_FLAG )
  {
    Options_activate_flag = HCI_NO_FLAG;
    Rpg_options_activate();
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Close" button in the RPG Control window.
 ************************************************************************/

static void Rpg_control_button_close( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "RPG Control Close pushed" );
  XtDestroyWidget( Top_widget );
}

/************************************************************************
     Description: This function launches the Archive II GUI.
 ************************************************************************/

static void ArchiveII_button_callback( Widget w, XtPointer y, XtPointer z )
{
  char window_name[256];
  char task_name[256];
  char process_name[50];
  char channel[32];
  char buf[HCI_BUF_128];
  int is_inactive = HCI_NO_FLAG; /* Assume channel is inactive */
  Mrpg_state_t rpg_state;

  int  hci_activate_child( Display *, Window, char *, char *, char *, int );

  /* Don't let user display Archive II GUI if RPG is inactive. */

  if( ORPGMGR_get_RPG_states( &rpg_state ) >= 0 )
  {
    if( rpg_state.active == MRPG_ST_INACTIVE ){ is_inactive = HCI_YES_FLAG; }
  }

  if( is_inactive )
  {
    HCI_LE_log( "RPG is inactive. Cannot launch Archive II GUI." );
    sprintf( buf, "The Archive II GUI will not\nlaunch when the RPG is inactive." );
    hci_warning_popup( Top_widget, buf, NULL );
  }
  else
  {
    sprintf( window_name, "Archive II" );
    sprintf( channel,"-A 0" );
    if( HCI_get_system() == HCI_FAA_SYSTEM )
    {
      sprintf( channel, "-A %1d",
               ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) );
    }

    sprintf( process_name, "hci_archII %s", channel );
    sprintf( task_name, "%s -name \"Archive II\"", process_name);

    strcat( task_name, HCI_child_options_string() );

    HCI_LE_log( "Spawning %s", task_name );

    hci_activate_child( HCI_get_display(),
                        RootWindowOfScreen( HCI_get_screen() ),
                        task_name, process_name, window_name, -1 );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the RPG command buttons in the RPG Control window.
 ************************************************************************/

static void Rpg_command_callback( Widget w, XtPointer y, XtPointer z )
{
  Mrpg_state_t mrpg;
  int status = 0;
  char buf[256];

  if( w == Startup_button )
  {
    /* The function of the Restart All Tasks selection is 
       dependent on the RPG state. */

    if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) )
    {
      HCI_LE_error( "ORPGMGR_get_RPG_states() failed" );
      return;
    }

    if( mrpg.state == MRPG_ST_STANDBY )
    {
      HCI_LE_log( "RPG Startup selected" );
      sprintf( buf, "You are about to restart all of the RPG processes.\nContents of datastores will be preserved.\nDo you want to continue?" );
    }
    else if( mrpg.state == MRPG_ST_SHUTDOWN )
    {
      HCI_LE_log( "RPG Startup selected" );
      sprintf( buf, "You are about to start all of the RPG processes.\nContents of some datastores will be reinitialized.\nDo you want to continue?" );
    }
    else if( mrpg.state == MRPG_ST_OPERATING )
    {
      HCI_LE_log( "RPG Restart selected" );
      sprintf( buf, "You are about to restart all of the RPG processes.\nContents of datastores will be preserved.\nDo you want to continue?" );
    }
    else
    {
      /* If the RPG is not in any one of the above operating
         states then do nothing and return. */
      HCI_LE_log( "RPG Startup only valid in Standby, Shutdown, and Operate states" );
      return;
    }

    Rpg_command_to_run = RPG_RESTART_ALL;
  }
  else if( w == Clean_startup_button )
  {
    /* The function of the Clean Startup selection is dependent
       on the RPG state. */

    if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) )
    {
      HCI_LE_log( "ORPGMGR_get_RPG_states() failed" );
      return;
    }

    if( mrpg.state == MRPG_ST_STANDBY ||
        mrpg.state == MRPG_ST_SHUTDOWN ||
        mrpg.state == MRPG_ST_OPERATING )
    {
      HCI_LE_log( "RPG Clean Startup selected" );
      sprintf( buf, "Using this Clean Startup option will empty\nthe contents of ALL datastores and then\nrestart all of the RPG processes.\nDo you want to continue?" );
    }
    else
    {
      /* If the RPG is not in any one of the above operating
         states then do nothing and return. */

      HCI_LE_log( "RPG Clean Startup only valid in Standby, Shutdown, and Operate states" );
      return;
    }

    Rpg_command_to_run = RPG_RESTART_CLEAN;
  }
  else if( w == Shutdown_button )
  {
    HCI_LE_log( "RPG Shutdown to OFF selected" );
    sprintf( buf, "You are about to stop all of the RPG processes.\nDo you want to continue?" );

    Rpg_command_to_run = RPG_SHUTDOWN_OFF;
  }
  else if( w == Standby_button )
  {
    HCI_LE_log( "RPG Shutdown to STANDBY selected" );
    sprintf( buf, "You are about to stop all of the RPG processes.\nDo you want to continue?" );

    Rpg_command_to_run = RPG_SHUTDOWN_STANDBY;
  }
  else
  {
    HCI_LE_log( "Unknown RPG command" );
    return;
  }

  /* Display a confirmation popup window. */

  hci_confirm_popup( Top_widget, buf, Verify_rpg_command_callback, Cancel_rpg_command_callback );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Yes" button from the RPG Command confirmation
                  popup window.
 ************************************************************************/

static void Verify_rpg_command_callback( Widget w, XtPointer y, XtPointer z )
{
  Command_to_run_is_verified = HCI_YES_FLAG;
}

/************************************************************************
     Description: This function executes the RPG command to run.
 ************************************************************************/

static void Run_rpg_command()
{
  int status = 0;
  Mrpg_state_t mrpg;
  char cmd[HCI_BUF_128];
  char func[HCI_BUF_128];
  char rpg_ip_address[HCI_BUF_64];
  char output_buffer[HCI_SYSTEM_MAX_BUF];
  int sys_ret = -1;
  int n_bytes = -1;

  if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) )
  {
    HCI_LE_error( "ORPGMGR_get_RPG_states() failed" );
    return;
  }

  switch( Rpg_command_to_run )
  {
    case RPG_RESTART_ALL :

      if( mrpg.state == MRPG_ST_SHUTDOWN )
      {
        HCI_PM( "Sending the RPG Startup command" );          
        HCI_LE_log( "RPG Startup accepted" );

        if( ( status = ORPGMGR_send_command( MRPG_STARTUP ) ) != 0 )
        {
          HCI_LE_error( "Error sending RPG (STARTUP) command: %d", status );
          /* Display feedback message */
          sprintf( Cmd, "RPG Startup Command Failed" );
          HCI_display_feedback( Cmd );
          return;
        }

        sprintf( Cmd, "RPG Startup Command Accepted" );
      }
      else if( mrpg.state == MRPG_ST_STANDBY )
      {
        HCI_PM( "Sending the RPG Restart command" );          
        HCI_LE_log( "RPG Restart accepted" );

        if( ( status = ORPGMGR_send_command( MRPG_RESTART ) ) != 0 )
        {
          HCI_LE_error( "Error sending RPG (RESTART) command: %d", status );
          /* Display feedback message */
          sprintf( Cmd, "RPG Restart Command Failed" );
          HCI_display_feedback( Cmd );
          return;
        }

        sprintf( Cmd, "RPG Restart Command Accepted" );
      }
      else
      {
        HCI_PM( "Sending the RPG Restart command" );          
        HCI_LE_log( "RPG Restart accepted" );

        if( ( status = ORPGMGR_send_command( MRPG_STANDBY_RESTART ) ) != 0 )
        {
          HCI_LE_error( "Error sending RPG (RESTART) command: %d", status );
          /* Display feedback message */
          sprintf( Cmd, "RPG Restart Command Failed" );
          HCI_display_feedback( Cmd );
          return;
        }

        sprintf( Cmd, "RPG Restart Command Accepted" );
      }

      HCI_display_feedback( Cmd );
      break;

    case RPG_RESTART_CLEAN :

      /* Unblock EN. */
      (void) EN_cntl_unblock();

      /* Find IP address of rpg. */
      status = ORPGMGR_discover_host_ip( "rpga", Channel_number,
                    rpg_ip_address, HCI_BUF_64 );

      if( status < 0 )
      {
        HCI_LE_error( "ORPGMGR_discover_host_ip_failed: %d", status );
      }
      else if( status == 0 )
      {
        HCI_LE_error( "Node:rpg channel:%d not found", Channel_number );
      }
      else
      {
        /* Build function to call remotely. */
        sprintf( cmd, "mrpg -p startup > /dev/null" );
        sprintf( func, "%s:MISC_system_to_buffer", rpg_ip_address );
        status = RSS_rpc( func, "i-r s-i ba-%d-io i ia-io",
                          HCI_SYSTEM_MAX_BUF, &sys_ret, cmd,
                          output_buffer, HCI_SYSTEM_MAX_BUF, &n_bytes );

        (void) EN_cntl_block();

        sys_ret = sys_ret >> 8;

         if( status != 0 )
         {
           HCI_LE_error( "RSS_rpc failed: %d", status );
           /* Display feedback message */
           sprintf( Cmd, "RPG Clean Startup Command Failed" );
           HCI_display_feedback( Cmd );
         }
         else if( sys_ret != 0 )
         {
           HCI_LE_error( "mrpg failed (%d) on RPG node", sys_ret );
           /* Display feedback message */
           sprintf( Cmd, "RPG Clean Startup Command Failed" );
           HCI_display_feedback( Cmd );
         }
       }
       break;

    case RPG_SHUTDOWN_OFF :

      HCI_PM( "Sending the RPG Shutdown command" );     
      HCI_LE_log( "RPG Shutdown accepted" );

      if( ( status = ORPGMGR_send_command( MRPG_SHUTDOWN ) ) != 0 )
      {
        HCI_LE_error( "Error sending RPG (SHUTDOWN) command: %d", status );
        /* Display feedback message */
        sprintf( Cmd, "RPG Shutdown Command Failed" );
        HCI_display_feedback( Cmd );
        return;
      }

      /* Display feedback message */
      sprintf( Cmd, "RPG Shutdown Command Accepted" );
      HCI_display_feedback( Cmd );
      break;

    case RPG_SHUTDOWN_STANDBY :

      HCI_PM( "Sending the RPG Standby command" );      
      HCI_LE_log( "RPG Standby accepted" );

      if( ( status = ORPGMGR_send_command( MRPG_STANDBY ) ) != 0 )
      {
        HCI_LE_error( "Error sending RPG (STANDBY) command: %d", status );
        /* Display feedback message */
        sprintf( Cmd, "RPG Standby Command Failed" );
        HCI_display_feedback( Cmd );
        return;
      }

      /* Display feedback message */
      sprintf( Cmd, "RPG Standby Command Accepted" );
      HCI_display_feedback( Cmd );
      break;

    default :
      return;
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "No" button from the RPG Command confirmation
                  popup window.
 ************************************************************************/

static void Cancel_rpg_command_callback( Widget w, XtPointer y, XtPointer z )
{
  switch( Rpg_command_to_run )
  {
    case RPG_RESTART_ALL :
      HCI_LE_log( "RPG Restart command cancelled" );
      break;
    case RPG_RESTART_CLEAN :
      HCI_LE_log( "RPG Clean Start command cancelled" );
      break;
    case RPG_SHUTDOWN_OFF :
      HCI_LE_log( "RPG Shutdown command cancelled" );
      break;
    case RPG_SHUTDOWN_STANDBY :
      HCI_LE_log( "RPG Standby command cancelled" );
      break;
  }

  Rpg_command_to_run = RPG_DO_NOTHING;
}

/************************************************************************
     Description: This function is used to update the items in the
                  RPG Control window. 
 ************************************************************************/

static void Update_rpg_control_menu_properties()
{
  XmString        str;
  int             status = 0;
  int             fg_color = 0;
  int             bg_color = 0;
  char            buf[32];
  Mrpg_state_t    mrpg;

  /* If the control/status dialog does not exist, return */

  if( Top_widget == (Widget) NULL ){ return; }

  /* Check the current state and display. */

  if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) == 0 )
  {
    switch( mrpg.state )
    {
      case MRPG_ST_SHUTDOWN :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, " SHUTDOWN " );
        XtSetSensitive( Startup_button, True );
        XtSetSensitive( Standby_button, False );
        XtSetSensitive( Shutdown_button, False );
        if( ( Unlocked_urc == HCI_YES_FLAG )
                         ||
            ( Unlocked_roc == HCI_YES_FLAG ) )
        {
          XtSetSensitive( Clean_startup_button, True );
        }
        else
        {
          XtSetSensitive( Clean_startup_button, False );
        }
        break;

      case MRPG_ST_STANDBY :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, " STANDBY  " );
        XtSetSensitive( Startup_button, True );
        XtSetSensitive( Standby_button, False );
        XtSetSensitive( Shutdown_button, True );
        if( ( Unlocked_urc == HCI_YES_FLAG )
                         ||
            ( Unlocked_roc == HCI_YES_FLAG ) )
        {
          XtSetSensitive( Clean_startup_button, True );
        }
        else
        {
          XtSetSensitive( Clean_startup_button, False );
        }
        break;

      case MRPG_ST_OPERATING :

        fg_color = Text_fg;
        bg_color = Normal_color;
        sprintf( buf, " OPERATE  " );
        XtSetSensitive( Startup_button, True );
        XtSetSensitive( Standby_button, True );
        XtSetSensitive( Shutdown_button, True );
        if( ( Unlocked_urc == HCI_YES_FLAG )
                         ||
            ( Unlocked_roc == HCI_YES_FLAG ) )
        {
          XtSetSensitive( Clean_startup_button, True );
        }
        else
        {
          XtSetSensitive( Clean_startup_button, False );
        }
        break;

      case MRPG_ST_TRANSITION :

        fg_color = Text_fg;
        bg_color = Warning_color;
        sprintf( buf, "TRANSITION" );
        XtSetSensitive( Startup_button, False );
        XtSetSensitive( Standby_button, False );
        XtSetSensitive( Shutdown_button, False );
        XtSetSensitive( Clean_startup_button, False );
        break;

      case MRPG_ST_FAILED :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, "  FAILED  " );
        XtSetSensitive( Startup_button, True );
        XtSetSensitive( Standby_button, True );
        XtSetSensitive( Shutdown_button, True );
        if( ( Unlocked_urc == HCI_YES_FLAG )
                         ||
            ( Unlocked_roc == HCI_YES_FLAG ) )
        {
          XtSetSensitive( Clean_startup_button, True );
        }
        else
        {
          XtSetSensitive( Clean_startup_button, False );
        }
        break;

      case MRPG_ST_POWERFAIL :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, " POWERFAIL" );
        XtSetSensitive( Startup_button, True );
        XtSetSensitive( Standby_button, True );
        XtSetSensitive( Shutdown_button, True );
        if( ( Unlocked_urc == HCI_YES_FLAG )
                         ||
            ( Unlocked_roc == HCI_YES_FLAG ) )
        {
          XtSetSensitive( Clean_startup_button, True );
        }
        else
        {
          XtSetSensitive( Clean_startup_button, False );
        }
        break;

      default :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, " UNKNOWN  " );
        XtSetSensitive( Startup_button, True );
        XtSetSensitive( Standby_button, True );
        XtSetSensitive( Shutdown_button, True );
        if( ( Unlocked_urc == HCI_YES_FLAG )
                         ||
            ( Unlocked_roc == HCI_YES_FLAG ) )
        {
          XtSetSensitive( Clean_startup_button, True );
        }
        else
        {
          XtSetSensitive( Clean_startup_button, False );
        }
        break;
    }

    str = XmStringCreateLocalized( buf );

    XtVaSetValues( Rpg_state,
                XmNlabelString, str,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    XmStringFree( str );
  }
  else
  {
    HCI_LE_error( "Unable to get RPG state information" );
  }

  Update_rpg_init_options_window();
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Yes" button in the RMS lock confirmation
                  popup window.
 ************************************************************************/

static void Rpg_lock_button_callback( Widget w, XtPointer y, XtPointer z )
{
  int ret = 0;
                
  if( Change_rms_lock )
  {
    /* Set the command lock */
    if( ( ret = rms_rda_rpg_lock( RMS_RPG_LOCK, RMS_SET_LOCK ) ) < 0 )
    {
      HCI_LE_error( "Unable to set RPG lock" );
    }
    else
    {
      Command_lock = HCI_YES_FLAG;
    }
  }
  else
  {
    /* Set the command lock */
    if( ( ret = rms_rda_rpg_lock( RMS_RPG_LOCK, RMS_CLEAR_LOCK ) ) < 0 )
    { 
      HCI_LE_error( "Unable to clear RPG lock" );
    }
    else
    {
      Command_lock = HCI_NO_FLAG;
    }
  }

  /* reset button */
  Set_rpg_button();
}

/************************************************************************
     Description: This function is activated when the user selects
                  the RMS lock check box.
 ************************************************************************/

static void Rpg_lock_button_verify_callback( Widget w, XtPointer y, XtPointer z )
{
  char buf[128];
  XmString yes;
  XmString no;
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;
        
  Change_rms_lock = state->set;
  yes = XmStringCreateLocalized( "Yes" );
  no = XmStringCreateLocalized( "No" );
        
  Change_rms_lock = state->set;
        
  if( rms_get_lock_status( RMS_RPG_LOCK ) )
  {
    sprintf( buf, "You are about to enable\nthe RMS to RPG commands.\nDo you want to continue?" );
  }
  else
  {
    sprintf ( buf, "You are about to disable\nthe RMS to RPG commands.\nDo you want to continue?" );
  }
                
  hci_confirm_popup( Top_widget, buf, Rpg_lock_button_callback, Set_rpg_button );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "No" button in the RMS lock confirmation
                  popup window.
 ************************************************************************/

static void Set_rpg_button()
{
  int ret = 0;
        
  if( ( ret = rms_get_lock_status( RMS_RPG_LOCK ) ) < 0 )
  {
    HCI_LE_error( "Unable to get RPG lock status" );
  }
  else
  {                 
    if( ret == RMS_COMMAND_LOCKED )
    {
      XtVaSetValues( Lock_button, XmNset, True, NULL );
    }
    else if( ret == RMS_COMMAND_UNLOCKED )
    {
      XtVaSetValues( Lock_button, XmNset, False, NULL );
    }
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Activate" button in the RPG Options window.
 ************************************************************************/

static void Rpg_options_button_activate( Widget w, XtPointer y, XtPointer z )
{
  char buf[HCI_BUF_128];

  HCI_LE_log( "Startup Options Activate pushed" );

  /* If any options were selected, then popup a confirmation window. */

  if( Options )
  {
    sprintf( buf, "You are about to activate all of the functions\nselected under Initialization Control.\nDo you want to continue?" );
    hci_confirm_popup( Top_widget, buf, Verify_rpg_options_button_activate, Cancel_rpg_options_button_activate );

    /* Else, if no options were selected, pop up an information window
       indicating that no options were selected. */
  }
  else
  {
    hci_warning_popup( Top_widget, "No initialization options were selected", NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
 *                   the "Yes" button from the Activate RPG Options
 *                   confirmation popup window.
 ************************************************************************/

static void Verify_rpg_options_button_activate( Widget w, XtPointer y, XtPointer z )
{
  Options_activate_flag = HCI_YES_FLAG;
}

/************************************************************************
     Description: Actual options activation called in timer proc.
 ************************************************************************/

static void Rpg_options_activate()
{
  int i = 0;
  int options_ret[HCI_MAX_INIT_OPTIONS] = {0};
  int num_failed_options = 0;
  char popup1[HCI_BUF_128*(HCI_MAX_INIT_OPTIONS+1)];
  char popup2[HCI_BUF_128];

  for( i = 0; i < HCI_MAX_INIT_OPTIONS; i++ )
  {
    if( ( ( Options >> i )&0x00000001 ) )
    {
      HCI_LE_log( "Running option: %s", HCI_get_init_options_name(i) );
      if( ( options_ret[i] = HCI_init_options_exec(i) ) != 0 )
      {
        num_failed_options++;
      }
    }
  }

  if( num_failed_options > 0 )
  {
    if( num_failed_options == 1 )
    {
      strcpy( popup1, "The following option failed:\n" );
    }
    else
    {
      strcpy( popup1, "The following options failed:\n" );
    }
    for( i = 0; i < HCI_MAX_INIT_OPTIONS; i++ )
    {
      if( options_ret[i] != 0 )
      {
        sprintf( popup2, "%s (%d)\n", HCI_get_init_options_name(i), options_ret[i] );
        strcat( popup1, popup2 );
      }
    }
    HCI_LE_log( popup1 );
    hci_error_popup( Top_widget, popup1, NULL );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "No" button from the Activate RPG Options
                  confirmation popup window.
 ************************************************************************/

static void Cancel_rpg_options_button_activate( Widget w, XtPointer y, XtPointer z )
{
}

/************************************************************************
     Description: This function is activated when the user selects
                  the lock button or selects a LOCA radio button
                  or enters a password in the Password window.
 ************************************************************************/

static int Rpg_startup_options_lock()
{
  Pixel   fg_color;
  int     i = 0;

  fg_color = Text_fg;

  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_selected() )
  {
    /* When user selects LOCA, show which selections are available. */
    fg_color = hci_get_read_color( LOCA_FOREGROUND );
  }
  else if( hci_lock_loca_unlocked() )
  {
    /* If the URC or ROC password was entered, set the unlocked flag. */

    if( hci_lock_ROC_unlocked() )
    {
      Unlocked_roc = HCI_YES_FLAG;
    }
    else if( hci_lock_URC_unlocked() )
    {
      Unlocked_urc = HCI_YES_FLAG;
    }
  }
  else if( hci_lock_close() &&
           ( Unlocked_roc == HCI_YES_FLAG || Unlocked_urc == HCI_YES_FLAG ) )
  {
    Unlocked_roc = HCI_NO_FLAG;
    Unlocked_urc = HCI_NO_FLAG;
    for( i = 0; i < HCI_num_init_options(); i++ )
    {
      XtVaSetValues( Option_button[i], XmNsensitive, False, NULL );
    }
  }

  /* Update the foreground color for the selectable objects. */

  for( i = 0; i < HCI_num_init_options(); i++ )
  {
    XtVaSetValues( Option_button[i], XmNforeground, fg_color, NULL );
  }

  /* Update the window objects. */

  Update_rpg_control_menu_properties();

  return HCI_LOCK_PROCEED;
}

/************************************************************************
     Description: This function is activated when the user selects
                  any of the RPG Options check boxes.
 ************************************************************************/

static void Rpg_init_options_toggle( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state =
                (XmToggleButtonCallbackStruct *) z;

  if( state->set )
  {
    /* The checkbox is set so we need to set the bit
       corresponding to the category. */

    Options = Options | ( 1 << ( (int) y ) );
  }
  else
  {
    /* The checkbox is unset so we need to unset the bit
       corresponding to the category. */

    Options = Options & ~( 1 << ( (int) y ) );
  }

  Update_rpg_init_options_window();
}

/************************************************************************
     Description: This function is used to update the properties of
                  objects in the RPG Options window.
 ************************************************************************/

static void Update_rpg_init_options_window()
{
  int i = 0;
  int status = 0;
  Mrpg_state_t mrpg;
  char permission_buf[HCI_INIT_OPTIONS_PERM_LEN];
  char state_buf[HCI_INIT_OPTIONS_STATE_LEN];
  int roc_flag = HCI_NO_FLAG;
  int urc_flag = HCI_NO_FLAG;
  int none_flag = HCI_NO_FLAG;

  /* Get the current RPG state.  The sensitivity is based on what
     is defined for the option. */

  if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) == 0 )
  {
    for( i = 0; i < HCI_num_init_options(); i++ )
    {
      /* Reset permission flags. */
      roc_flag = HCI_NO_FLAG;
      urc_flag = HCI_NO_FLAG;
      none_flag = HCI_NO_FLAG;

      /* Get permission/state info from configuration file. */
      strcpy( permission_buf, HCI_get_init_options_permission(i) );
      strcpy( state_buf, HCI_get_init_options_state(i) );

      /* Set appropriate permission flag. */
      if( strstr( permission_buf, "ROC" ) != NULL ){ roc_flag = HCI_YES_FLAG; }
      if( strstr( permission_buf, "URC" ) != NULL ){ urc_flag = HCI_YES_FLAG; }
      if( strcmp( permission_buf, "NONE" ) == 0 ){ none_flag = HCI_YES_FLAG; }

      if (  ( Unlocked_roc == HCI_YES_FLAG && roc_flag ) ||
            ( Unlocked_urc == HCI_YES_FLAG && urc_flag ) ||
            ( none_flag ) )
      {
        if( ( strstr( state_buf, "MRPG_ST_SHUTDOWN" ) != NULL ) &&
            ( mrpg.state == MRPG_ST_SHUTDOWN ) )
        {
          XtSetSensitive( Option_button[i], True );
        }
        else if( ( strstr( state_buf, "MRPG_ST_STANDBY" ) != NULL ) &&
                 ( mrpg.state == MRPG_ST_STANDBY ) )
        {
          XtSetSensitive( Option_button[i], True );

        }
        else if( ( strstr( state_buf, "MRPG_ST_OPERATING" ) != NULL ) &&
                 ( mrpg.state == MRPG_ST_OPERATING ) )
        {
          XtSetSensitive( Option_button[i], True );
        }
        else if( ( strstr( state_buf, "MRPG_ST_FAILED" ) != NULL ) &&
                 ( mrpg.state == MRPG_ST_FAILED ) )
        {
          XtSetSensitive( Option_button[i], True );
        }
        else if( ( strstr( state_buf, "MRPG_ST_TRANSITION" ) != NULL ) &&
                 ( mrpg.state == MRPG_ST_TRANSITION ) )
        {
          XtSetSensitive( Option_button[i], True );
        }
        else if( ( strstr( state_buf, "MRPG_ST_POWERFAIL" ) != NULL ) &&
                 ( mrpg.state == MRPG_ST_POWERFAIL ) )
        {
          XtSetSensitive( Option_button[i], True );
        }
        else
        {
          XtSetSensitive( Option_button[i], False );
        }
      }
    }
  }

  if( Options )
  {
    XtSetSensitive( Activate_button, True );
  }
  else
  {
    XtSetSensitive( Activate_button, False );
  }
}



