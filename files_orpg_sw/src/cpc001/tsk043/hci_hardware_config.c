/************************************************************************
 *									*
 *	Module:  hci_hardware_config.c					*
 *									*
 *	Description:  This module provides a gui wrapper for		*
 *		      configuring various hardware devices.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/10 14:01:21 $
 * $Id: hci_hardware_config.c,v 1.51 2014/04/10 14:01:21 steves Exp $
 * $Revision: 1.51 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <hci.h>

enum	{ ALL_DEV = 0,
          CONSERV_DEV,
	  LAN_DEV,
	  RPG_ROUTER_DEV,
	  PWR_ADMIN_DEV,
	  UPS_DEV,
	  PTIA_DEV,
	  PTIB_DEV,
	  DIO_DEV,
	  FR_ROUTER_RPG_DEV,
	  NUM_HW_DEVICES };

/*	Defines.			*/

#define	MAX_NUM_LABELS		20
#define	MAX_PASSWD_LENGTH	15
#define	NO_DEVICE		99
#define	NOERR_MSG		HCI_CP_STDOUT
#define	ERR_MSG			HCI_CP_STDERR
#define	RPG_SHUTDOWN_WAIT_TIME	150
#define	CMD_RPG_STARTUP		0
#define	CMD_RPG_RESTART		1
#define	MRPG_STARTUP_TIMER_LAG	300

/*	Global widget definitions.	*/

static	Widget	Top_widget = ( Widget ) NULL;
static	Widget	Config_date_labels[NUM_HW_DEVICES];
static	Widget	Device_buttons[NUM_HW_DEVICES];
static	Widget	Info_scroll = ( Widget ) NULL;
static	Widget	Info_scroll_form = ( Widget ) NULL;
static	Widget	Password_dialog = ( Widget ) NULL;

/*	Global variables.		*/

static	char	Output_msgs[MAX_NUM_LABELS][HCI_CP_MAX_BUF];
static	int	Output_msgs_code[MAX_NUM_LABELS];
static	int	Current_msg_pos = 0; /* Index of latest output msg */
static	int	Num_output_msgs = 0; /* Number of output msgs to display */
static	int	Coprocess_flag = HCI_CP_NOT_STARTED;
static	void*	Cp = NULL;
static	int	Device_flag = NO_DEVICE;
static	int	Popups_to_acknowledge = 0;
static	int	Mrpg_is_ready = HCI_YES_FLAG;
static	int	Shutdown_start_timer = 0;
static	int	Mrpg_was_commanded_to_shutdown = HCI_NO_FLAG;
static	int	Mrpg_was_commanded_to_startup = HCI_NO_FLAG;
static	int	Mrpg_startup_timer = 0;
static	int	Check_for_mrpg_shutdown = HCI_NO_FLAG;
static	int	Prompt_to_startup = HCI_NO_FLAG;
static	int	Wait_for_startup_decision = HCI_NO_FLAG;
static	int	User_cancelled_flag = HCI_NO_FLAG;
static	int	Frame_relay_present = HCI_NO_FLAG;
static	int	Close_popup_acknowledged = HCI_NO_FLAG;
static	int	Pwd_popup_destroyed = HCI_NO_FLAG;
static	char  	Msg_buf[HCI_CP_MAX_BUF+HCI_BUF_256];
static	char  	Password_input[MAX_PASSWD_LENGTH];
static	void*	Callback_fxs[NUM_HW_DEVICES];
static	char*	Device_labels[NUM_HW_DEVICES] = {
			"    All Devices     ",
			"   Console Server   ",
			"         LAN        ",
			"     RPG Router     ",
			"Power Administrator ",
			"         UPS        ",
			"        PTI-A       ",
			"        PTI-B       ",
			"      DIO Module    ",
			" Frame Relay Router " };
static	char*	Device_cmds[NUM_HW_DEVICES] = {
			"config_all",
			"config_device -N -f console",
			"config_device -N -f lan",
			"config_device -N -f rtr",
			"config_device -N -f msp",
			"config_device -N -f ups",
			"config_device -N -f pti-a",
			"config_device -N -f pti-b",
			"config_device -N -f dio",
			"config_device -N -f hub" };
static	char*	Device_tags[NUM_HW_DEVICES] = {
			ALL_DEVICES_TIME_TAG,
			CONSERV_DEVICE_TIME_TAG,
			LAN_DEVICE_TIME_TAG,
			RPG_ROUTER_DEVICE_TIME_TAG,
			PWR_ADMIN_DEVICE_TIME_TAG,
			UPS_DEVICE_TIME_TAG,
			PTIA_DEVICE_TIME_TAG,
			PTIB_DEVICE_TIME_TAG,
			DIO_MODULE_DEVICE_TIME_TAG,
			FR_ROUTER_RPG_DEVICE_TIME_TAG };
static	int	Critical_device[NUM_HW_DEVICES] = {
			HCI_NO_FLAG,
			HCI_YES_FLAG,
			HCI_YES_FLAG,
			HCI_YES_FLAG,
			HCI_YES_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_YES_FLAG,
			HCI_YES_FLAG };
static	int	Device_configured[NUM_HW_DEVICES] = {
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG,
			HCI_NO_FLAG };

/*	Function prototypes.		*/

static int	Destroy_callback ();
static void	Close_callback( Widget, XtPointer, XtPointer );
static void	All_devices_callback (Widget, XtPointer, XtPointer );
static void	Conserv_callback (Widget, XtPointer, XtPointer );
static void	LAN_callback (Widget, XtPointer, XtPointer );
static void	RPG_router_callback (Widget, XtPointer, XtPointer );
static void	Power_admin_callback (Widget, XtPointer, XtPointer );
static void	UPS_callback (Widget, XtPointer, XtPointer );
static void	PTIA_callback (Widget, XtPointer, XtPointer );
static void	PTIB_callback (Widget, XtPointer, XtPointer );
static void	DIO_callback (Widget, XtPointer, XtPointer );
static void	FR_router_rpg_callback (Widget, XtPointer, XtPointer );
static void	Hide_password_input (Widget, XtPointer, XtPointer );
static void	Submit_password (Widget, XtPointer, XtPointer );
static void	Cancel_password (Widget, XtPointer, XtPointer );
static void	Popup_close_callback (Widget, XtPointer, XtPointer );
static void	Ok_prompt_confirm (Widget, XtPointer, XtPointer );
static void	Command_to_shutdown( Widget, XtPointer, XtPointer );
static void	Cancel_shutdown( Widget, XtPointer, XtPointer );
static void	Command_to_startup( Widget, XtPointer, XtPointer );
static void	Cancel_startup( Widget, XtPointer, XtPointer );
static void	Ack_popup( Widget, XtPointer, XtPointer );
static void	Error_popup();
static void	Update_scroll_display( char *, int );
static void	Update_label( Widget, int );
static void	Update_button_display( int );
static void	Update_date_display( int );
static void	Configure_device();
static void	Manage_coprocess();
static void	Close_coprocess();
static void	Manage_config_all_coprocess();
static void	Update_configuration_date( int );
static void	Successful_configuration();
static void	Set_configured_flag_error();
static void	Password_prompt( char * );
static void	Reset_to_initial_state();
static time_t	Get_device_config_time( int );
static int	Get_device_flag( char * );
static void	Destroy_password_prompt();
static void	Close_popup();
static void	Critical_devices_not_configured();
static void	Noncritical_devices_not_configured();
static int	Num_critical_devices_not_configured();
static int	Num_noncritical_devices_not_configured();
static void	Find_configured_devices();
static void	Check_if_mrpg_is_ready();
static void	Prompt_to_command_shutdown();
static void	Prompt_to_command_startup( int );
static void	User_acknowledge_prompt( char * );
static void	Timer_proc();

/************************************************************************
 *	Description: This is the main function for the Hardware		*
 *		     Configuration task.				*
 ************************************************************************/

int main( int argc, char *argv [] )
{
  Widget	form;
  Widget	frame;
  Widget	body_frame;
  Widget	label;
  Widget	rowcol;
  Widget	item_rowcol;
  Widget	row_rowcol;
  Widget	control_rowcol;
  Widget	button;
  Widget	clip;
  int		fr_rpg_status;
  int		i;
  int		status;
  int		devices_configured_flag = HCI_NO_FLAG;
  XmString	initial_label;
  XmString	label_string;

  /* Set IP of RPG to query. */
   
  status = hci_install_info_set_rpg_host( argc, argv );

  if( status < 0 )
  {
    HCI_LE_error( "Unable to set rpg host." );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Set write permission for MRPG CMD LB. It's needed if
     ORPGMGR_send_command is called. */

  ORPGDA_write_permission( ORPGDAT_MRPG_CMDS );

  /* Find out if hardware devices have been configured. */

  devices_configured_flag = hci_get_install_info_dev_configured();

  if( devices_configured_flag < 0 )
  {
    HCI_LE_error( "Unable to determine hardware configuration flag." );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Initialize HCI. */

  if( devices_configured_flag == HCI_NO_FLAG || ORPGMGR_is_mrpg_up() != 1 )
  {
    HCI_partial_init( argc, argv, HCI_HARDWARE_CONFIG_TASK );
    XtVaSetValues( HCI_get_top_widget(), XmNtitle, HCI_get_task_name( HCI_HARDWARE_CONFIG_TASK ), NULL );
  }
  else
  {
    HCI_init( argc, argv, HCI_HARDWARE_CONFIG_TASK );
  }

  Top_widget = HCI_get_top_widget();
  HCI_set_destroy_callback( Destroy_callback );

  /* Determine if frame relay is present. */

  fr_rpg_status = MISC_system_to_buffer( "find_adapt -F", NULL, 0, NULL );
  fr_rpg_status = fr_rpg_status >> 8;

  if( fr_rpg_status == 0 )
  {
    Frame_relay_present = HCI_YES_FLAG;
  }
  else
  {
    Frame_relay_present = HCI_NO_FLAG;
    if( fr_rpg_status < 0 )
    {
      HCI_LE_error( "Error with find_adapt -F." );
    }
  }

  /* Initialize callback functions array. */

  Callback_fxs[ALL_DEV] = All_devices_callback;
  Callback_fxs[CONSERV_DEV] = Conserv_callback;
  Callback_fxs[LAN_DEV] = LAN_callback;
  Callback_fxs[RPG_ROUTER_DEV] = RPG_router_callback;
  Callback_fxs[PWR_ADMIN_DEV] = Power_admin_callback;
  Callback_fxs[UPS_DEV] = UPS_callback;
  Callback_fxs[PTIA_DEV] = PTIA_callback;
  Callback_fxs[PTIB_DEV] = PTIB_callback;
  Callback_fxs[DIO_DEV] = DIO_callback;
  Callback_fxs[FR_ROUTER_RPG_DEV] = FR_router_rpg_callback;

  /* Initialize date labels so width is correct. */

  initial_label = XmStringCreateLocalized( "                         " );

  /* Define a form widget to be used as the manager for widgets in *
   * the top level window.                                         */

  form = XtVaCreateManagedWidget( "form",
         xmFormWidgetClass, Top_widget,
         XmNbackground,     hci_get_read_color( BACKGROUND_COLOR1 ),
         NULL );

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
           xmRowColumnWidgetClass, form,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNtopAttachment,       XmATTACH_FORM,
           XmNleftAttachment,      XmATTACH_FORM,
           XmNrightAttachment,     XmATTACH_FORM,
           NULL );

  button = XtVaCreateManagedWidget( "Close",
           xmPushButtonWidgetClass, control_rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  /* End of control rowcol, start of main body of GUI. */

  body_frame = XtVaCreateManagedWidget( "body_frame",
          xmFrameWidgetClass,  form,
          XmNtopAttachment,    XmATTACH_WIDGET,
          XmNtopWidget,        control_rowcol,
          XmNleftAttachment,   XmATTACH_FORM,
          XmNrightAttachment,  XmATTACH_FORM,
          NULL );

  item_rowcol = XtVaCreateManagedWidget( "item_rowcol",
           xmRowColumnWidgetClass, body_frame,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmVERTICAL,
           XmNnumColumns,          1,
           NULL );

  row_rowcol = XtVaCreateManagedWidget( "row_rowcol",
           xmRowColumnWidgetClass, item_rowcol,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNnumColumns,          1,
           NULL );

  label = XtVaCreateManagedWidget( "                        ",
          xmLabelWidgetClass,     row_rowcol,
          XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNfontList,            hci_get_fontlist (LIST),
          NULL);

  Device_buttons[ALL_DEV] = XtVaCreateManagedWidget( "Configure All Devices",
           xmPushButtonWidgetClass, row_rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( Device_buttons[ALL_DEV],
                 XmNactivateCallback, Callback_fxs[ALL_DEV], NULL );

  for( i = ALL_DEV+1; i < NUM_HW_DEVICES; i++ )
  {
    row_rowcol = XtVaCreateManagedWidget( "row_rowcol",
           xmRowColumnWidgetClass, item_rowcol,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNnumColumns,          1,
           NULL );

    frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,  row_rowcol,
          XmNleftAttachment,   XmATTACH_FORM,
          NULL );

    label_string = XmStringCreateLocalized( Device_labels[i] );

    label = XtVaCreateManagedWidget( "",
          xmLabelWidgetClass,     frame,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNfontList,            hci_get_fontlist (LIST),
          XmNlabelString,         label_string,
          NULL);

    XmStringFree( label_string );

    frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,  row_rowcol,
          NULL );

    rowcol = XtVaCreateManagedWidget( "row_rowcol",
           xmRowColumnWidgetClass, frame,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNnumColumns,          1,
           NULL );

    label = XtVaCreateManagedWidget( " Last Configured ",
          xmLabelWidgetClass,     rowcol,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNfontList,            hci_get_fontlist (LIST),
          NULL);

    Config_date_labels[i] = XtVaCreateManagedWidget( "",
          xmLabelWidgetClass,     rowcol,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR2),
          XmNfontList,            hci_get_fontlist (LIST),
          XmNlabelString,         initial_label,
          NULL);

    Device_buttons[i] = XtVaCreateManagedWidget( "Configure",
           xmPushButtonWidgetClass, row_rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

    XtAddCallback( Device_buttons[i],
                   XmNactivateCallback,
                   Callback_fxs[i], NULL );

    /* Initialize configuration date. */

    Update_date_display( i );

    /* If not FAA, Assume PTI-A and PTI-B are configured. */

    if( ((i == PTIA_DEV) || (i == PTIB_DEV)) && HCI_get_system() != HCI_FAA_SYSTEM )
    {
      Device_configured[i] = HCI_YES_FLAG;
    }

    /* If not FAA, no need to show DIO button. Assume it's configured. */

    if( i == DIO_DEV && HCI_get_system() != HCI_FAA_SYSTEM )
    {
      Device_configured[i] = HCI_YES_FLAG;
      XtUnmanageChild( row_rowcol );
    }

    /* If Frame relay is not present, no need to show the button. Assume
       it's configured. */

    if( i == FR_ROUTER_RPG_DEV && Frame_relay_present == HCI_NO_FLAG )
    {
      Device_configured[i] = HCI_YES_FLAG;
      XtUnmanageChild( row_rowcol );
    }
  }

  XmStringFree( initial_label );

  /* Create scroll area to display configuration output. */

  Info_scroll = XtVaCreateManagedWidget ("data_scroll",
                xmScrolledWindowWidgetClass,    form,
                XmNheight,              200,
                XmNscrollingPolicy,     XmAUTOMATIC,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           body_frame,
                XmNbottomAttachment,      XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

  XtVaGetValues( Info_scroll, XmNclipWindow, &clip, NULL );

  XtVaSetValues( clip,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_AND_HALF_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the the HCI	*
 *		     Hardware Configuration window is destroyed.	*
 ************************************************************************/

static int Destroy_callback()
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to exit this task\n" );
    strcat( Msg_buf, "until the configuration process has been\n" );
    strcat( Msg_buf, "completed.\n" );
    Error_popup();
    return HCI_NOT_OK_TO_EXIT;
  }

  /* If this is the initial hardware configuration,
     call a function that will take that into account.
     Otherwise, just exit. */

  if( Close_popup_acknowledged == HCI_NO_FLAG &&
      ( Num_critical_devices_not_configured() > 0 ||
        Num_noncritical_devices_not_configured() > 0 ) )
  {
    Close_popup();
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 ************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to close this task\n" );
    strcat( Msg_buf, "until the configuration process has been\n" );
    strcat( Msg_buf, "completed.\n" );
    Error_popup();
  }
  else
  {
    /* If this is the initial hardware configuration,
       call a function that will take that into account.
       Otherwise, just exit. */
       
    if( Close_popup_acknowledged == HCI_NO_FLAG &&
        ( Num_critical_devices_not_configured() > 0 ||
          Num_noncritical_devices_not_configured() > 0 ) )
    {
      Close_popup();
    }
    else
    {
      XtDestroyWidget( Top_widget );
    }
  }
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "All" button.					*
 ************************************************************************/

static void All_devices_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = ALL_DEV;
  Prompt_to_startup = HCI_YES_FLAG;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
  Check_for_mrpg_shutdown = HCI_YES_FLAG;
  Update_scroll_display( "Selection requires check of RPG state", NOERR_MSG );
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Console Server" button.                       *
 ************************************************************************/

static void Conserv_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = CONSERV_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "RPG Router" button.				*
 ************************************************************************/

static void RPG_router_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = RPG_ROUTER_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Lan" button.					*
 ************************************************************************/

static void LAN_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = LAN_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Power Administrator" button.			*
 ************************************************************************/

static void Power_admin_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = PWR_ADMIN_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "UPS" button.					*
 ************************************************************************/

static void UPS_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = UPS_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "PTI-A" button.				*
 ************************************************************************/

static void PTIA_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = PTIA_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
  Check_for_mrpg_shutdown = HCI_YES_FLAG;
  Update_scroll_display( "Selection requires check of RPG state", NOERR_MSG );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "PTI-B" button.				*
 ************************************************************************/

static void PTIB_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = PTIB_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
  Check_for_mrpg_shutdown = HCI_YES_FLAG;
  Update_scroll_display( "Selection requires check of RPG state", NOERR_MSG );
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "DIO Module" button.                           *
 ************************************************************************/

static void DIO_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = DIO_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Frame Relay Router" button.			*
 ************************************************************************/

static void FR_router_rpg_callback( Widget w, XtPointer x, XtPointer y )
{
  Device_flag = FR_ROUTER_RPG_DEV;
  Update_button_display( HCI_OFF_FLAG );
  HCI_busy_cursor();
}

/************************************************************************
 * Timer_proc: Callback for event timer.
 ************************************************************************/

static void Timer_proc()
{
  if( Device_flag != NO_DEVICE )
  {
    if( Check_for_mrpg_shutdown == HCI_YES_FLAG )
    {
      Check_for_mrpg_shutdown = HCI_NO_FLAG;
      Update_scroll_display( "Checking RPG state", NOERR_MSG );
      Check_if_mrpg_is_ready();
      if( Mrpg_is_ready == HCI_NO_FLAG )
      {
        Update_scroll_display( "Prompt user to shutdown RPG", NOERR_MSG );
        Prompt_to_command_shutdown();
      }
    }
    else if( Mrpg_is_ready == HCI_NO_FLAG )
    {
      if( Mrpg_was_commanded_to_shutdown == HCI_YES_FLAG )
      {
        if( ( time( NULL ) - Shutdown_start_timer ) < RPG_SHUTDOWN_WAIT_TIME )
        {
          Check_if_mrpg_is_ready();
        }
        else
        {
          sprintf( Msg_buf, "Shutdown timer (%d seconds) expired.\n", RPG_SHUTDOWN_WAIT_TIME );
          strcat( Msg_buf, "Stopping configuration" );
          Update_scroll_display( Msg_buf, ERR_MSG );
          HCI_LE_error( "%s", Msg_buf );
          Error_popup();
          Reset_to_initial_state();
        }
      }
    }
    else if( Coprocess_flag == HCI_CP_NOT_STARTED )
    {
      /* Call function to start co-process to configure device(s). */
      Update_scroll_display( "Begin configuration process", NOERR_MSG );
      Configure_device();
    }
    else if( Coprocess_flag == HCI_CP_STARTED && Device_flag == ALL_DEV )
    {
      /* Call function to manage "config all" co-process. */
      Manage_config_all_coprocess();
    }
    else if( Coprocess_flag == HCI_CP_STARTED )
    {
      /* Call function to manage non-"config all" co-process. */
      Manage_coprocess();
    }
    else if( Coprocess_flag == HCI_CP_FINISHED && Pwd_popup_destroyed == HCI_NO_FLAG )
    {
      Destroy_password_prompt();
    }
    else if( Popups_to_acknowledge )
    {
      /* Waiting for user to acknowledge all popups. This will
         prevent additional popups from covering up previously
         existing popups the user may not have seen. */
    }
    else if( hci_get_install_info_dev_configured() != HCI_YES_FLAG )
    {
      if( Num_critical_devices_not_configured() == 0 )
      {
        /* Attempt to set the flag indicating devices have been
           configured. If the flag is unable to be set, the system
           is in a non-recoverable state (since every time the user
           tries to launch the HCI the hardware configuration GUI 
           will launch instead). Tell the user they're screwed
           and to call the WSR-88D Hotline. */

        if( hci_set_install_info_dev_configured() < 0 )
        {
          Set_configured_flag_error();
          Coprocess_flag = HCI_CP_NOT_STARTED;
          Reset_to_initial_state();
        }
      }
      else
      {
        /* Critical devices not configured, no use continuing. */
        Coprocess_flag = HCI_CP_NOT_STARTED;
        Reset_to_initial_state();
      }
    }
    else if( Mrpg_was_commanded_to_shutdown == HCI_YES_FLAG ||
             Prompt_to_startup == HCI_YES_FLAG )
    {
      Wait_for_startup_decision = HCI_YES_FLAG;
      if( Mrpg_was_commanded_to_shutdown )
      {
        Update_scroll_display( "Prompt user to restart RPG", NOERR_MSG );
        Prompt_to_command_startup( CMD_RPG_RESTART );
      }
      else
      {
        Update_scroll_display( "Prompt user to start RPG", NOERR_MSG );
        Prompt_to_command_startup( CMD_RPG_STARTUP );
      }
      Mrpg_was_commanded_to_shutdown = HCI_NO_FLAG;
      Prompt_to_startup = HCI_NO_FLAG;
    }
    else if( Wait_for_startup_decision == HCI_YES_FLAG )
    {
      /* Waiting for user to decide whether to re-start the RPG.
         Without this wait, the scroll display will clear prematurely. */
    }
    else if( Mrpg_was_commanded_to_startup == HCI_YES_FLAG )
    {
      if( ORPGMGR_is_mrpg_up() == 1 )
      {
        Mrpg_was_commanded_to_startup = HCI_NO_FLAG;
        Update_scroll_display( "RPG startup successful", NOERR_MSG );
      }
      else if( ( time(NULL) - Mrpg_startup_timer ) > MRPG_STARTUP_TIMER_LAG )
      {
        Mrpg_was_commanded_to_startup = HCI_NO_FLAG;
        strcpy( Msg_buf, "Unable to verify RPG startup" );
        Update_scroll_display( Msg_buf, ERR_MSG );
        HCI_LE_error( "%s", Msg_buf );
        Error_popup();
      }
    }
    else if( Coprocess_flag == HCI_CP_FINISHED )
    {
      Coprocess_flag = HCI_CP_NOT_STARTED;
      /* Finished, but didn't have to restart the RPG software. */
      Reset_to_initial_state();
    }
  }
}

/************************************************************************
 *	Description: This function updates a date widget to its		*
 *		     latest value.					*
 ************************************************************************/

static void Update_date_display( int dev_flag )
{
  time_t config_time = -1;
  char label_buf[HCI_BUF_32];
  XmString label_string;

  config_time = Get_device_config_time( dev_flag );

  if( config_time <= 0 )
  {
    sprintf( label_buf, "            NA            " );
    XtVaSetValues( Config_date_labels[dev_flag],
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR2 ),
                   NULL );
  }
  else
  {
    strftime( label_buf, HCI_BUF_32, " %c ",
              gmtime( &config_time ) );
    XtVaSetValues( Config_date_labels[dev_flag],
                   XmNbackground, hci_get_read_color( WHITE ),
                   NULL );
  }
  label_string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( Config_date_labels[dev_flag], XmNlabelString,
                 label_string, NULL );
  XmStringFree( label_string );
}

/************************************************************************
 *	Description: This function displays a popup with an error	*
 *		     message for the user.				*
 ************************************************************************/

static void Error_popup()
{
  /* Increment number of popups to acknowledge. */
  Popups_to_acknowledge++;
  hci_error_popup( Top_widget, Msg_buf, Ack_popup );
}

/************************************************************************
 * Ack_popup: Callback for OK button on popup.
 ************************************************************************/

static void Ack_popup( Widget w, XtPointer x, XtPointer y )
{
  /* Decrement number of popups to acknowledge. */
  Popups_to_acknowledge--;
}

/************************************************************************
 *	Description: This function displays a popup with a message	*
 *		     indicating success.				*
 ************************************************************************/

static void Successful_configuration()
{
  /* Increment number of popups to acknowledge. */
  Popups_to_acknowledge++;
  hci_info_popup( Top_widget, "Successful configuration", Ack_popup );
}

/************************************************************************
 *	Description: This function is called when the user wants to close
 *		     the GUI. Do bookkeepping before closing it.
 ************************************************************************/

static void Close_popup()
{
  /* If any critical or non-critical devices are not configured,
     let user know. */

  if( Num_critical_devices_not_configured() > 0 )
  {
    Critical_devices_not_configured();
  }
  else if( Num_noncritical_devices_not_configured > 0 )
  {
    Noncritical_devices_not_configured();
  }
}

/************************************************************************
 *	Description: This function displays a popup with a message	*
 *		     regarding critical devices not configured.		*
 ************************************************************************/

static void Critical_devices_not_configured()
{
  int i;

  sprintf( Msg_buf, "The following critical device(s) are not configured:\n" );

  for( i = ALL_DEV+1; i < NUM_HW_DEVICES; i++ )
  {
    if( Critical_device[i] == HCI_YES_FLAG && Device_configured[i] == HCI_NO_FLAG )
    {
      strcat( Msg_buf, "\n" );
      strcat( Msg_buf, Device_labels[i] );
      strcat( Msg_buf, "\n" );
    }
  }

  strcat( Msg_buf, "\nThe system is not functional. If the device(s)\n" );
  strcat( Msg_buf, "are unable to be configured, call the WSR-88D Hotline.\n" );

  hci_custom_confirm_popup( Top_widget, Msg_buf, "Close", Popup_close_callback, "Cancel", NULL );
}

/************************************************************************
 *	Description: This function displays a popup with a message	*
 *		     regarding noncritical devices not configured.	*
 ************************************************************************/

static void Noncritical_devices_not_configured()
{
  int i;

  sprintf( Msg_buf, "The following non-critical device(s) are not configured:\n" );

  for( i = ALL_DEV+1; i < NUM_HW_DEVICES; i++ )
  {
    if( Critical_device[i] == HCI_NO_FLAG && Device_configured[i] == HCI_NO_FLAG )
    {
      strcat( Msg_buf, "\n" );
      strcat( Msg_buf, Device_labels[i] );
      strcat( Msg_buf, "\n" );
    }
  }

  strcat( Msg_buf, "\nThe system is functional, however, the user\n" );
  strcat( Msg_buf, "should keep trying to configure the device(s).\n" );
  strcat( Msg_buf, "If the configuration continues to fail, call\n" );
  strcat( Msg_buf, "the WSR-88D Hotline.\n");

  hci_custom_confirm_popup( Top_widget, Msg_buf, "Close", Popup_close_callback, "Cancel", NULL );
}

/************************************************************************
 * Popup_close_callback: Callback for "Close" button.
 ************************************************************************/

static void Popup_close_callback( Widget w, XtPointer x, XtPointer y )
{
  Close_popup_acknowledged = HCI_YES_FLAG;
  Close_callback( w, x, y );
}

/************************************************************************
 *	Description: This function executes the script to configure the	*
 *		     device.						*
 ************************************************************************/

static void Configure_device()
{
  int ret = -1;

  /* Build function to call remotely. */

  ret = MISC_cp_open( Device_cmds[Device_flag], HCI_CP_MANAGE, &Cp );

  if( ret != 0 )
  {
    sprintf( Msg_buf, "MISC_cp_open failed (%d)\n\n", ret );
    strcat( Msg_buf, "Unable to configure device." );
    Update_scroll_display( Msg_buf, ERR_MSG );
    HCI_LE_error( "%s", Msg_buf );
    Error_popup();
    Coprocess_flag = HCI_CP_FINISHED;
  }
  else
  {
    Coprocess_flag = HCI_CP_STARTED;
  }
}

/************************************************************************
 *	Description: This function manages the coprocess that is
 *                   configuring the selected device.
 ************************************************************************/

static void Manage_coprocess()
{
  int ret = -1;
  char cp_buf[HCI_CP_MAX_BUF];

  /* Read coprocess to see if any new data is available. */

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, HCI_CP_MAX_BUF ) ) != 0 )
  {
    /* Strip trailing newline. */
    if( cp_buf[strlen( cp_buf )-1] == '\n' )
    {
      cp_buf[strlen( cp_buf )-1] = '\0';
    }

    /* MISC function succeeded. Take action depending on return value. */
    if( ret == HCI_CP_DOWN )
    {
      /* Co-process has been terminated. If the user cancelled
         the coprocess (for example, cancelling at the password
         prompt), then start over. */
      if( User_cancelled_flag == HCI_YES_FLAG )
      {
        User_cancelled_flag = HCI_NO_FLAG;
        return; 
      }
      /*Retrieve exit status. */
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      {
        /* Exit status indicates error. */
        sprintf( Msg_buf, "Configuration error (%d)", ret );
        Update_scroll_display( Msg_buf, ERR_MSG );
        HCI_LE_error( "%s", Msg_buf );
        Error_popup();
      }
      else
      {
        /* Exit status indicates success. Update date label. */
        Update_configuration_date( Device_flag );
        Successful_configuration();
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR )
    {
      /* Display message in scroll box. */
      Update_scroll_display( cp_buf, HCI_CP_STDERR );
    }
    else if( ret == HCI_CP_STDOUT )
    {
      /* If coprocess buffer contains the token "Password:", then
         the user needs to be prompted for a password to enter. */
      if( strstr( cp_buf, "Password:" ) != NULL )
      {
        /* This line has the "Password:" token. Create
           password prompt popup. */
        Password_prompt( cp_buf );
      }
      else
      {
        /* This line is just a message that needs to
           be displayed in the message scroll box. */
        Update_scroll_display( cp_buf, HCI_CP_STDOUT );
      }
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }
}

/************************************************************************
 *	Description: This function manages the coprocess that is
 *                   configuring all devices.
 ************************************************************************/

static void Manage_config_all_coprocess()
{
  int ret = -1;
  int dev_flag = -1;
  char cp_buf[HCI_CP_MAX_BUF];
  char *dev_string = NULL;

  /* Read coprocess to see if any new data is available. */

  while ( Cp != NULL &&
          ( ret = MISC_cp_read_from_cp( Cp, cp_buf, HCI_CP_MAX_BUF ) ) != 0 )
  {
    /* Strip trailing newline. */
    if( cp_buf[strlen( cp_buf )-1] == '\n' )
    {
      cp_buf[strlen( cp_buf )-1] = '\0';
    }

    /* MISC function succeeded. Take action depending on return value. */
    if( ret == HCI_CP_DOWN )
    {
      /* Co-process has been terminated. If the user cancelled
         the coprocess (for example, cancelling at the password
         prompt), then return here so user can start over. */
      if( User_cancelled_flag == HCI_YES_FLAG )
      {
        User_cancelled_flag = HCI_NO_FLAG;
        return; 
      }
      /*Retrieve exit status. */
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      {
        sprintf( Msg_buf, "Configuration error (%d)", ret );
        Update_scroll_display( Msg_buf, ERR_MSG );
        HCI_LE_error( "%s", Msg_buf );
        Error_popup();
      }
      else
      {
        /* Exit status indicates success. */
        Successful_configuration();
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR )
    {
      /* Display message in scroll box. */
      Update_scroll_display( cp_buf, HCI_CP_STDERR );
    }
    else if( ret == HCI_CP_STDOUT )
    {
      if( strstr( cp_buf, "Password:" ) != NULL )
      {
        /* If this line contains the token "Password:", then
         the user needs to be prompted for a password to enter.
         Do not display this line in the scroll box. */
        Password_prompt( cp_buf );
      }
      else if( strstr( cp_buf, "Note:" ) != NULL )
      {
        /* If this line contains the token "NOTE:", then
           create a confirmation popup. */
        User_acknowledge_prompt( &cp_buf[6] );
      }
      else
      {
        /* Display the line in the scroll box. */
        Update_scroll_display( cp_buf, HCI_CP_STDOUT );

        /* If this line contains the token "succeeded", then
           a device was successfully configured and we need
           to parse the line to determine which device. The
           line will take the form "Configuring XXX succeeded",
           where XXX is the device name. */
        if( strstr(  cp_buf, "succeeded" ) != NULL )
        {
          /* Skip token "Configuring". */
          strtok( cp_buf, " " );
          /* Next token is name of device that was successfully configured. */
          dev_string = strtok( NULL, " " );
          /* Convert device string to device flag number. */
          dev_flag = Get_device_flag( dev_string );
          if( dev_flag > 0 )
          {
            Update_configuration_date( dev_flag );
          }
          else
          {
            HCI_LE_error( "Bad device string %s.", dev_string  );
          }
        }
      }
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }
}

/************************************************************************
 *	Description: This function updates the date information after	*
 *		     a successful configuration.			*
 ************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 *	Description: This function updates the date information after	*
 *		     a successful configuration.			*
 ************************************************************************/

static void Update_configuration_date( int dev_flag )
{
  int ret = -1;
  char buf[HCI_BUF_32];

  sprintf( buf, "%li", time( NULL ) );
  if ( ( ret = hci_set_install_info( Device_tags[dev_flag], buf ) ) < 0 )
  {
    HCI_LE_error( "Unable to update config time for header %s",
                 Device_tags[dev_flag] );
    sprintf( Msg_buf, "Unable to update config time for header %s",
             Device_tags[dev_flag] );
    Update_scroll_display( Msg_buf, ERR_MSG );
  }
  Update_date_display( dev_flag );
}

/************************************************************************
 *	Description: This function updates the sensitivity of the	*
 *		     button widgets.					*
 ************************************************************************/

static void Update_button_display( int sensitivity_flag )
{
  int i = 0;

  for( i = ALL_DEV; i < NUM_HW_DEVICES; i++ )
  {
    if( sensitivity_flag )
    {
      XtSetSensitive( Device_buttons[i], True );
    }
    else
    {
      XtSetSensitive( Device_buttons[i], False );
    }
  }
}

/************************************************************************
 *	Description: This function updates the scroll with the latest	*
 *		     output from the configuration task.		*
 ************************************************************************/

static void Update_scroll_display( char *Msg_buf, int msg_code )
{
  Widget label = ( Widget ) NULL;
  Widget rowcol = ( Widget ) NULL;
  int loop = -1;

  /* Modify appropriate msg in array. */

  sprintf( Output_msgs[Current_msg_pos], "%s", Msg_buf );
  Output_msgs_code[Current_msg_pos] = msg_code;

  /* Create widgets to display labels. */

  if( Info_scroll_form != ( Widget ) NULL )
  {
    XtDestroyWidget( Info_scroll_form );
  }

  Info_scroll_form = XtVaCreateWidget( "scroll_form",
       xmFormWidgetClass, Info_scroll,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNverticalSpacing, 1,
       NULL );

  /* This rowcol ensures all labels are of equal width. */

  rowcol = XtVaCreateManagedWidget( "rc",
       xmRowColumnWidgetClass, Info_scroll_form,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNorientation, XmVERTICAL,
       XmNpacking, XmPACK_COLUMN,
       XmNnumColumns,1,
       XmNleftAttachment, XmATTACH_FORM,
       XmNrightAttachment, XmATTACH_FORM,
       NULL );

  /* This long label sets the initial width. This ensures any
     colored labels stretch across the entire scrolling area. */

  label = XtVaCreateManagedWidget( "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
       xmLabelWidgetClass, rowcol,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNfontList, hci_get_fontlist( LIST ),
       XmNleftAttachment, XmATTACH_FORM,
       XmNrightAttachment, XmATTACH_FORM,
       XmNalignment, XmALIGNMENT_BEGINNING,
       NULL );

  /* The latest message is located at position Current_msg_pos.
     Start there and work down to index 0. */

  for( loop = Current_msg_pos; loop >= 0; loop-- )
  {
    label = XtVaCreateManagedWidget( "label",
              xmLabelWidgetClass, Info_scroll_form,
              XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
              XmNfontList, hci_get_fontlist( LIST ),
              XmNtopAttachment, XmATTACH_WIDGET,
              XmNtopWidget, label,
              XmNleftAttachment, XmATTACH_FORM,
              XmNrightAttachment, XmATTACH_FORM,
              XmNalignment, XmALIGNMENT_BEGINNING,
              NULL );
    Update_label( label, loop );
  }

  /* If the number of output messages has reached the maximum
     allowed amount, then start at the last array position and
     work down to the index just above the Current_msg_pos index.
     This is done to allow the array to be treated as circular. */

  if( Num_output_msgs == MAX_NUM_LABELS )
  {
    for( loop = MAX_NUM_LABELS - 1; loop > Current_msg_pos; loop-- )
    {
      label = XtVaCreateManagedWidget( "label",
              xmLabelWidgetClass, Info_scroll_form,
              XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
              XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
              XmNfontList, hci_get_fontlist( LIST ),
              XmNtopAttachment, XmATTACH_WIDGET,
              XmNtopWidget, label,
              XmNleftAttachment, XmATTACH_FORM,
              XmNrightAttachment, XmATTACH_FORM,
              XmNalignment, XmALIGNMENT_BEGINNING,
              NULL );
      Update_label( label, loop );
    }
  }

  XtManageChild( Info_scroll_form );
  XmUpdateDisplay( Info_scroll_form ); /* Force update. */

  /* Update starting label position (circular array). */

  Current_msg_pos++;
  Current_msg_pos%=MAX_NUM_LABELS;

  /* Update number of labels (not to exceed maximum number allowed). */

  Num_output_msgs++;
  if( Num_output_msgs > MAX_NUM_LABELS ){ Num_output_msgs--; }
}

/************************************************************************
 *	Description: This function updates a label with the appropriate	*
 *		     value and color.					*
 ************************************************************************/

static void Update_label( Widget w, int index )
{
  XmString msg;

  /* Convert c-buf to XmString variable. */

  msg = XmStringCreateLtoR( Output_msgs[index], XmFONTLIST_DEFAULT_TAG );

  /* Modify appropriate output label. */

  if( Output_msgs_code[index] == HCI_CP_STDERR )
  {
    XtVaSetValues( w,
                   XmNlabelString, msg,
                   XmNbackground, hci_get_read_color( WARNING_COLOR ),
                   NULL );
  }
  else
  {
    XtVaSetValues( w,
                   XmNlabelString, msg,
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL );
  }

  /* Free allocated space. */

  XmStringFree( msg );
}

/************************************************************************
 * Set_configured_flag_error: Popup if can't set flag to indicate devices
 *                            configured.
 ************************************************************************/

static void Set_configured_flag_error()
{
  sprintf( Msg_buf, "Unable to set flag indicating hardware devices have\n" );
  strcat( Msg_buf, "been configured. The system is in an unrecoverable\n" );
  strcat( Msg_buf, "state. Call the WSR-88D Hotline for assistance.\n" );

  hci_error_popup( Top_widget, Msg_buf, NULL );
}

/**************************************************************************
 * Get_device_config_time: Print information about custom options.
**************************************************************************/

static time_t Get_device_config_time( int dev_flag )
{
  char value_buf[HCI_BUF_32];
  long time_value;

  if( hci_get_install_info( Device_tags[dev_flag], value_buf, HCI_BUF_32 ) < 0 )
  {
    return 0;
  }

  sscanf( value_buf, "%li", &time_value );
  return ( time_t ) time_value;
}

/**************************************************************************
 * Password_prompt: Allow user to enter password for configuration.
**************************************************************************/

static void Password_prompt( char *cp_buf )
{
  Widget temp_widget;
  Widget ok_button;
  Widget cancel_button;
  Widget text_field;
  XmString ok_button_string;
  XmString cancel_button_string;
  XmString msg;

  Destroy_password_prompt();
  Pwd_popup_destroyed = HCI_NO_FLAG;

  /* Create popup for entering password. */

  Password_dialog = XmCreatePromptDialog( Top_widget, "password", NULL, 0 );

  /* Set various string labels. */

  ok_button_string = XmStringCreateLocalized( "OK" );
  msg = XmStringCreateLtoR( cp_buf, XmFONTLIST_DEFAULT_TAG );

  cancel_button_string = XmStringCreateLocalized( "Cancel" );
  msg = XmStringCreateLtoR( cp_buf, XmFONTLIST_DEFAULT_TAG );

  /* Get rid of Help button on popup. */

  temp_widget = XmSelectionBoxGetChild( Password_dialog,
                                        XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of message label. */

  temp_widget = XmSelectionBoxGetChild( Password_dialog, XmDIALOG_SELECTION_LABEL );
  XtVaSetValues( temp_widget,
          XmNforeground,  hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,  hci_get_read_color( WARNING_COLOR ),
          XmNfontList,    hci_get_fontlist( LIST ),
          NULL );

  /* Set properties of OK button. */

  ok_button = XmSelectionBoxGetChild( Password_dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( ok_button, XmNactivateCallback, Submit_password, NULL );

  /* Set properties of CANCEL button. */

  cancel_button = XmSelectionBoxGetChild( Password_dialog,
                                          XmDIALOG_CANCEL_BUTTON );

  XtVaSetValues( cancel_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( cancel_button, XmNactivateCallback, Cancel_password, NULL );

  /* Set properties of text field. */

  text_field = XmSelectionBoxGetChild( Password_dialog, XmDIALOG_TEXT );

  XtVaSetValues( text_field,
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          XmNcolumns, MAX_PASSWD_LENGTH,
          XmNmaxLength, MAX_PASSWD_LENGTH,
          NULL );

  XtAddCallback( text_field, XmNmodifyVerifyCallback,
                 Hide_password_input, ( XtPointer ) Password_input );

  /* Set properties of popup. */

  XtVaSetValues (Password_dialog,
          XmNselectionLabelString, msg,
          XmNokLabelString, ok_button_string,
          XmNcancelLabelString, cancel_button_string,
          XmNbackground, hci_get_read_color( WARNING_COLOR ),
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          XmNdialogType, XmDIALOG_PROMPT,
          XmNdeleteResponse, XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok_button_string );
  XmStringFree( cancel_button_string );
  XmStringFree( msg );

  /* Remove close/destroy button on menu. */

  XtVaSetValues( XtParent(Password_dialog),
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_CLOSE, NULL );

  /* Do this to make popup appear. */

  XtManageChild( Password_dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/***************************************************************************
 * Destroy_password_prompt: Destroy password prompt if it isn't NULL.
 ***************************************************************************/

static void Destroy_password_prompt()
{
  if( Password_dialog != ( Widget ) NULL )
  {
    XtDestroyWidget( Password_dialog );
    Password_dialog = ( Widget ) NULL;
  }
  Pwd_popup_destroyed = HCI_YES_FLAG;
}

/***************************************************************************
 * Hide_password_input: Hide password by replacing letters with "*".
 ***************************************************************************/

static void Hide_password_input( Widget w, XtPointer x, XtPointer y )
{
  char *passwd_array;
  XmTextVerifyCallbackStruct *cbs;

  cbs = ( XmTextVerifyCallbackStruct * ) y;
  passwd_array = ( char * ) x;

  /* If text field is empty, don't worry about it. */
  if( cbs->text->length == 0 && cbs->currInsert == 0 ){ return; }

  /* Don't worry about backspaces. */
  if( cbs->startPos < cbs->currInsert )
  {
    passwd_array[cbs->startPos] = '\0';
    return;
  }

  /* Don't allow cutting and pasting. */
  if( cbs->text->length > 1 )
  {
    cbs->doit = False;
    return;
  }

  passwd_array[cbs->currInsert] = cbs->text->ptr[0];
  passwd_array[cbs->currInsert+1] = '\0';

  cbs->text->ptr[0] = '*';
}

/***************************************************************************
 * Submit_password: Submit password to configuration task.
 ***************************************************************************/

static void Submit_password( Widget w, XtPointer x, XtPointer y )
{
  char temp_password[MAX_PASSWD_LENGTH];
  /* Add newline to end of password. */
  sprintf( temp_password, "%s\n", Password_input );
  if( Cp != NULL ){ MISC_cp_write_to_cp( Cp, temp_password ); }
  else
  {
    sprintf( Msg_buf, "Coprocess terminated. Unable to submit password." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
  }
}

/***************************************************************************
 * Cancel_password: Cancel password submission.
 ***************************************************************************/

static void Cancel_password( Widget w, XtPointer x, XtPointer y )
{
  Close_coprocess();
  User_cancelled_flag = HCI_YES_FLAG;
}

/**************************************************************************
 * User_acknowledge_prompt: Popup that user must acknowledge.
**************************************************************************/

static void User_acknowledge_prompt( char *cp_buf )
{
  char *temp_buf = "Click Continue to proceed.";
  char *temp_ptr = NULL;

  /* Some messages may end with "hit return to continue". To make sense
     for the GUI, convert phrase to "Click Continue to proceed". */

  if( ( temp_ptr = strstr( cp_buf, "hit return" ) ) != NULL )
  {
    strcpy( temp_ptr, temp_buf );
  }
  
  hci_info_popup( Top_widget, cp_buf, Ok_prompt_confirm );
}

/***************************************************************************
 * Ok_prompt_confirm: User clicked "Continue" on confirmation popup.
 ***************************************************************************/

static void Ok_prompt_confirm( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag == HCI_CP_STARTED )
  {
    /* Send newline to Coprocess. */
    if( Cp != NULL ){ MISC_cp_write_to_cp( Cp, "\n" ); }
    else
    {
      sprintf( Msg_buf, "Coprocess terminated. Unable to continue." );
      hci_error_popup( Top_widget, Msg_buf, NULL );
    }
  }
}

/***************************************************************************
 * Get_device_flag: Return device flag for given device string.
 ***************************************************************************/

static int Get_device_flag( char *dev_string )
{
  if( strcmp( dev_string, "RPG" ) == 0 ){ return RPG_ROUTER_DEV; }
  else if( strcmp( dev_string, "LAN" ) == 0 ){ return LAN_DEV; }
  else if( strcmp( dev_string, "Console" ) == 0 ){ return CONSERV_DEV; }
  else if( strcmp( dev_string, "UPS" ) == 0 ){ return UPS_DEV; }
  else if( strcmp( dev_string, "Power" ) == 0 ){ return PWR_ADMIN_DEV; }
  else if( strcmp( dev_string, "PTI-A" ) == 0 ){ return PTIA_DEV; }
  else if( strcmp( dev_string, "PTI-B" ) == 0 ){ return PTIB_DEV; }
  else if( strcmp( dev_string, "DIO" ) == 0 ){ return DIO_DEV; }
  else if( strcmp( dev_string, "Frame" ) == 0 )
  {
    if( Frame_relay_present == HCI_YES_FLAG ){ return FR_ROUTER_RPG_DEV; }
  }

  /* dev_string doesn't match any known device, return error code. */
  return -1;
}

/***************************************************************************
 * Reset_to_initial_state: Reset flags, buttons, cursors, etc. to initial
 *    states.
 ***************************************************************************/

static void Reset_to_initial_state()
{
  Device_flag = NO_DEVICE;
  Mrpg_is_ready = HCI_YES_FLAG;
  Mrpg_was_commanded_to_startup = HCI_NO_FLAG;
  Mrpg_startup_timer = 0;
  Update_button_display( HCI_ON_FLAG );
  HCI_default_cursor();
  Wait_for_startup_decision = HCI_NO_FLAG;
  Close_popup_acknowledged = HCI_NO_FLAG;
  Check_for_mrpg_shutdown = HCI_NO_FLAG;
  User_cancelled_flag = HCI_NO_FLAG;
  Num_output_msgs = 0;
  Current_msg_pos = 0;
}

/***************************************************************************
 * Num_critical_devices_not_configured: Determines if any critical devices
 *    have not been configured.
 ***************************************************************************/

static int Num_critical_devices_not_configured()
{
  int i;
  int count = 0;

  Find_configured_devices();

  for( i = ALL_DEV+1; i < NUM_HW_DEVICES; i++ )
  {
    if( Critical_device[i] == HCI_YES_FLAG && Device_configured[i] == HCI_NO_FLAG )
    {
      count++;
    }
  }

  return count;
}

/***************************************************************************
 * Num_noncritical_devices_not_configured: Determines if any noncritical devices
 *    have not been configured.
 ***************************************************************************/

static int Num_noncritical_devices_not_configured()
{
  int i;
  int count = 0;

  Find_configured_devices();

  for( i = ALL_DEV+1; i < NUM_HW_DEVICES; i++ )
  {
    if( Critical_device[i] == HCI_NO_FLAG && Device_configured[i] == HCI_NO_FLAG )
    {
      count++;
    }
  }

  return count;
}

/***************************************************************************
 * Find_configured_devices: Determines which devices have been configured.
 ***************************************************************************/

static void Find_configured_devices()
{
  int i;
  char buf[HCI_BUF_32];

  for( i = ALL_DEV+1; i < NUM_HW_DEVICES; i++ )
  {
    if( ORPGMISC_get_install_info(Device_tags[i],buf,HCI_BUF_32) == 0 )
    {
      Device_configured[i] = HCI_YES_FLAG;
    }
  }
}

/***************************************************************************
 * Prompt_to_command_shutdown: Prompt user to shutdown MRPG.
 ***************************************************************************/

static void Prompt_to_command_shutdown()
{
  sprintf( Msg_buf, "This option requires the RPG to be in the SHUTDOWN state.\nTo put in the SHUTDOWN state now and continue with the\nconfiguration, click \"Continue\"." );
  hci_custom_confirm_popup( Top_widget, Msg_buf, "Continue", Command_to_shutdown, "Cancel", Cancel_shutdown );
}

/***************************************************************************
 * Command_to_shutdown: Command MRPG to shutdown.
 ***************************************************************************/

static void Command_to_shutdown( Widget w, XtPointer x, XtPointer y )
{
  int status = 0;

  HCI_LE_log( "Commanding RPG to SHUTDOWN" );
  Update_scroll_display( "Commanding RPG to SHUTDOWN (may take several minutes)", NOERR_MSG );

  if( ( status = ORPGMGR_send_command( MRPG_SHUTDOWN ) ) != 0 )
  {
    sprintf( Msg_buf, "Error (%d) commanding RPG to SHUTDOWN state", status );
    Update_scroll_display( Msg_buf, ERR_MSG );
    HCI_LE_error( "%s", Msg_buf );
    Error_popup();
    Reset_to_initial_state();
  }
  else
  {
    Mrpg_was_commanded_to_shutdown = HCI_YES_FLAG;
    Shutdown_start_timer = time( NULL );
  }
}

/***************************************************************************
 * Cancel_shutdown: Cancel MRPG shutdown.
 ***************************************************************************/

static void Cancel_shutdown( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "User declinced shutdown and cancelled configuration" );
  Update_scroll_display( "User declined shutdown", NOERR_MSG );
  Update_scroll_display( "Configuration cancelled", NOERR_MSG );
  Reset_to_initial_state();
}

/***************************************************************************
 * Check_if_mrpg_is_ready: Check if MRPG is in SHUTDOWN state.
 ***************************************************************************/

static void Check_if_mrpg_is_ready()
{
  int status = 0;
  Mrpg_state_t mrpg;

  HCI_LE_log( "Check if MRPG is ready for configuration" );

  if( ( status = ORPGMGR_is_mrpg_up() ) != 1 )
  {
    /* Mrpg software isn't running (or we don't know for sure).
       Assume no shutdown is needed and proceed with configuration. */
    Mrpg_is_ready = HCI_YES_FLAG;
    HCI_LE_log( "MRPG isn't running (%d)", status );
  }
  else
  {
    if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) == 0 &&
        mrpg.state == MRPG_ST_SHUTDOWN )
    {
      /* Mrpg software is running and already in the SHUTDOWN state. */
      Mrpg_is_ready = HCI_YES_FLAG;
      HCI_LE_log( "MRPG is running, state == SHUTDOWN" );
    }
    else
    {
      /* Mrpg software is running, but not in the SHUTDOWN state. */
      Mrpg_is_ready = HCI_NO_FLAG;
      HCI_LE_log( "MRPG is running, but state != SHUTDOWN" );
    }
  }
}

/***************************************************************************
 * Prompt_to_command_startup: Prompt user to startup MRPG.
 ***************************************************************************/

static void Prompt_to_command_startup( int startup_flag )
{
  if( startup_flag == CMD_RPG_RESTART )
  {
    sprintf( Msg_buf, "The RPG was put in the SHUTDOWN state before\nconfiguration began. Do you want to restart\nthe RPG application software?" );
  }
  else
  {
    sprintf( Msg_buf, "Do you want to restart the RPG application software?" );
  }

  hci_confirm_popup( Top_widget, Msg_buf, Command_to_startup, Cancel_startup );
}

/***************************************************************************
 * Command_to_startup: Command MRPG to startup.
 ***************************************************************************/

static void Command_to_startup( Widget w, XtPointer x, XtPointer y )
{
  int status = 0;

  Wait_for_startup_decision = HCI_NO_FLAG;

  HCI_LE_log( "Executing MRPG startup" );
  Update_scroll_display( "Executing RPG startup", NOERR_MSG );

  status =  MISC_system_to_buffer( "mrpg startup &", NULL, 0, NULL );

  if( status < 0 )
  {
    sprintf( Msg_buf, "Error (%d) executing RPG startup", status );
    Update_scroll_display( Msg_buf, ERR_MSG );
    HCI_LE_error( "%s", Msg_buf );
    Error_popup();
  }
  else
  {
    Mrpg_startup_timer = time(NULL);
    Mrpg_was_commanded_to_startup = HCI_YES_FLAG;
    Update_scroll_display( "Verifying RPG startup (may take several minutes)", NOERR_MSG );
  }
}

/***************************************************************************
 * Cancel_startup: Cancel MRPG startup.
 ***************************************************************************/

static void Cancel_startup( Widget w, XtPointer x, XtPointer y )
{
  Wait_for_startup_decision = HCI_NO_FLAG;
  HCI_LE_log( "User declinced MRPG startup" );
  Update_scroll_display( "User declined RPG startup", NOERR_MSG );
}

