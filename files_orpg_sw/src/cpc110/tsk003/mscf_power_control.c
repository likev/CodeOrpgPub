/************************************************************************

      The main source file for MSCF Power Control HCI.

************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 21:40:52 $
 * $Id: mscf_power_control.c,v 1.15 2013/07/22 21:40:52 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/* RPG include files. */

#include <hci.h>
#include <power_control_icon.h>
#include <mscf_power_control.h>

/* Macros. */
enum { APC = 0, SENTRY }; 

#define	MPING_TIMER		2
#define NUM_LE_LOG_MESSAGES	500
#define	MSCF_NAME_LEN		128
#define	MSCF_DEFAULT_CONF_NAME	"mscf1.conf"
#define	MSCF_CH2_CONF_NAME	"mscf2.conf"
#define	MSCF_NPA_DEF_CONF_NAME	"mscf1_npa.conf"
#define	MSCF_CH2_NPA_CONF_NAME	"mscf2_npa.conf"
#define	N_PCW_STATES		2
#define	N_SEL_STATES		2
#define	DEFAULT_UPDATE_PERIOD	10
#define	SHORT_UPDATE_PERIOD	3
#define	UPDATE_CHANGE_INTERVAL	20
#define	KEY_LEN			32
#define	MAX_CMD_LEN		256
#define	MAX_ADDRESS_LEN		32
#define	MAX_STRING_LEN		64
#define	MISC_BUF_LEN		512
#define	TITLE_BUF_LEN		128
#define	DEFAULT_TIMER_EVENT	HCI_TWO_SECONDS
#define	SHORT_TIMER_EVENT	HCI_HALF_SECOND
#define	POWER_CONTROL_TITLE	"Power Control"
#define	COPROCESS_SIZE		512


/* Global variables. */

Power_status_t	Outlet_status[MAX_NUM_OUTLETS];
int	Num_outlets;
int	Channel_number;
int	System_flag;
int     Verbose;
int     Log_outlet_names;
int	Power_control_flag;
int     Pwr_adm;
char	Pv_cmd[MAX_CMD_LEN];
char	Pc_addr[MAX_ADDRESS_LEN];
char	*Current_text_pointer;
char	Pc_cmd[MAX_CMD_LEN];
char	Pc_ret_strs[NUM_PWRCTRL_OPTIONS][MAX_STRING_LEN];
char	Pc_key[MAX_CMD_LEN];
char	Pn_cmd[MAX_CMD_LEN];
char	Pn_key[MAX_CMD_LEN];
char	Ps_cmd[MAX_CMD_LEN];
char	Ps_ret_strs[NUM_PWRCTRL_STATES][MAX_STRING_LEN];
char	Ps_key[MAX_CMD_LEN];


/* Static variables. */
static Widget	Top_widget = ( Widget ) NULL;
static Widget	Form = ( Widget ) NULL;
static Widget	Turn_on_button = ( Widget ) NULL;
static Widget	Turn_off_button = ( Widget ) NULL;
static Widget	Reboot_button = ( Widget ) NULL;
static Widget	Close_button = ( Widget ) NULL;
static Widget	Status_dialog = ( Widget ) NULL;
static Pixel	Bg_color = (Pixel) NULL;
static Pixel	Text_color = (Pixel) NULL;
static Pixel	Alarm_color = (Pixel) NULL;
static Pixel	Warn_color = (Pixel) NULL;
static Pixel	Normal_color = (Pixel) NULL;
static Pixel	Button_bg_color = (Pixel) NULL;
static Pixel	Button_fg_color = (Pixel) NULL;
static Pixel	White_color = (Pixel) NULL;
static Pixel	Gray_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;

static void	*Cp = NULL;
static int	Coprocess_flag = HCI_CP_NOT_STARTED;
static int	Initialized_outlets = NO;
static int	Top_widget_minimized_flag = NO;
static char	Mscf_conf_name[ MSCF_NAME_LEN ];
static int	Update_flag = YES;
static Pixmap	Active_icon_pixmaps[NUM_PWRCTRL_STATES][NUM_PWRCTRL_SELECTIONS];
static Pixmap	Inactive_icon_pixmap;
static int	Wait_time = -1;
static int	Wait_for_PC_confirmation = NO;
static int	PC_confirmation = PC_NONE;
static int	Update_period = -1;
static int	No_close_allowed = NO;
static int	Power_control_network_status = PWRADM_CONNECTED;
static char	Popup_buf[MISC_BUF_LEN];

/*  Function prototypes. */
static  void    Notify_func( EN_id_t id, char *msg, int msg_size, void *arg );
static	int	Destroy_callback( Widget, XtPointer, XtPointer );
static	void	Close_callback( Widget, XtPointer, XtPointer );
static	void	Map_cb( Widget, XtPointer, XEvent *, Boolean * );
static	void	Turn_off_callback( Widget, XtPointer, XtPointer );
static	void	Turn_on_callback( Widget, XtPointer, XtPointer );
static	void	Reboot_callback( Widget, XtPointer, XtPointer );
static	void	Pc_icon_activate( Widget, XtPointer, XtPointer );
static	void	Power_control_yes( Widget, XtPointer, XtPointer );
static	void	Power_control_no( Widget, XtPointer, XtPointer );
static	int	Get_power_control_version( int *new );
static	void	Read_mscf_conf();
static	void	Initialize_outlet_pixmaps();
static	void	Update_outlet_labels();
static	void	Get_outlet_names();
static	void	Get_outlet_status();
static	Widget	Create_outlet_widgets();
static	void	PC_action();
static	int	PC_power_control( int, int );
static	void	Update_wait_text( char *, Widget );
static	void	Execute_power_control_yes();
static	void	Execute_power_control_no( );
static	void	Reset_to_initial_state();
static	void	Initialize_status_dialog();
static	void	Popup_status_dialog( char * );
static	void	Popdown_status_dialog();
static	void	Start_coprocess();
static	void	Manage_coprocess();
static	void	Close_coprocess();
static	void	Flush_X_events();
static	void	Timer_proc();
static	void	Parse_connect_status( char * );

int main( int argc, char **argv )
{
  Widget        form_rowcol = ( Widget ) NULL;
  Widget        rowcol = ( Widget ) NULL;
  Widget        label = ( Widget ) NULL;
  char		title_buf[TITLE_BUF_LEN];
  int		i = -1;
  int           new = 0;
  int		power_control_version = -1;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  /* Register for verbose mode event.  By default, 
     verbose mode is off. */
  
  Verbose = 0;
  Log_outlet_names = 0;
  EN_register( ORPGEVT_MSCF_PWR_CTRL_VERBOSE, Notify_func );
  EN_register( ORPGEVT_MSCF_PWR_CTRL_GETNAMES, Notify_func );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  System_flag = HCI_get_system();

  Channel_number = HCI_get_channel_number();

  /* Need to reset the GUI title. */

  if( System_flag == HCI_FAA_SYSTEM )
  {
    sprintf( title_buf, "%s (FAA:%1d)", POWER_CONTROL_TITLE, Channel_number );
  }
  else if( System_flag == HCI_NWSR_SYSTEM )
  {
    sprintf( title_buf, "%s (NWS:%1d)", POWER_CONTROL_TITLE, Channel_number );
  }
  else
  {
    strcpy( title_buf, POWER_CONTROL_TITLE );
  }
  XtVaSetValues( Top_widget, XmNtitle, title_buf, NULL );

  XtAddEventHandler( Top_widget, StructureNotifyMask, False, Map_cb, NULL );

  /* Initialize miscellaneous variables. */

  Bg_color = hci_get_read_color( BACKGROUND_COLOR1 );
  Text_color = hci_get_read_color( TEXT_FOREGROUND );
  Alarm_color = hci_get_read_color( ALARM_COLOR1 );
  Warn_color = hci_get_read_color( WARNING_COLOR );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Button_bg_color = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg_color = hci_get_read_color( BUTTON_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  White_color = hci_get_read_color( GRAY );
  List_font = hci_get_fontlist( LIST );

  /* Initialize outlet status information. */

  Initialize_outlet_pixmaps();
  for( i = 0; i < MAX_NUM_OUTLETS; i++ )
  {
    Outlet_status[i].active = PC_ACTIVE_NO;
    Outlet_status[i].on = PC_OFF_STATE;
    Outlet_status[i].selected = PC_SELECT_NO;
    strcpy( Outlet_status[i].label, " Unknown \n status " );
    Outlet_status[i].switch_index = -1;
    Outlet_status[i].outlet_index = -1;
    Outlet_status[i].enable_turn_on = YES;
    Outlet_status[i].enable_turn_off = YES;
    Outlet_status[i].enable_reboot = YES;
    Outlet_status[i].host_name[0] = '\0';
    Outlet_status[i].shutdown_cmd[0] = '\0';
    Outlet_status[i].shutdown_delay = 0;
  }

  Num_outlets = 0;
  Power_control_flag = PC_NONE;
  Pwr_adm = APC;

  /* Read the MSCF configuration file. */

  Read_mscf_conf();

  /* Start coprocess that will monitor network connectivity
     Power Control device. */

  Start_coprocess();

  /* Define top-level form and rowcol to hold all widgets. */

  Form = XtVaCreateWidget( "mode_list_form",
                 xmFormWidgetClass, Top_widget,
                 NULL);

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                        xmRowColumnWidgetClass, Form,
                        XmNbackground, Bg_color,
                        XmNorientation, XmVERTICAL,
                        XmNpacking, XmPACK_TIGHT,
                        XmNnumColumns, 1,
                        NULL);

  /* Get version of Power Control device. */

  power_control_version = Get_power_control_version( &new );

  /* If version could not be found, then no use continuing. Make
     the GUI into a warning popup. */

  if( power_control_version == PC_DEVICE_UNAVAILABLE )
  {
    /* Make sure background is warning color. */
    XtVaSetValues( form_rowcol, XmNbackground, Warn_color, NULL );

    /* Use blank label for vertical space at the top of the GUI. */
    label = XtVaCreateManagedWidget( " ",
                   xmLabelWidgetClass, form_rowcol,
                   XmNbackground, Warn_color,
                   XmNforeground, Text_color,
                   XmNfontList, List_font,
                   NULL );

    /* Warning message to user. */
    label = XtVaCreateManagedWidget( "An error occurred while trying to verify a network\nconnection to the power control device. If this\nproblem continues, call the WSR-88D Hotline.\n",
                   xmLabelWidgetClass, form_rowcol,
                   XmNbackground, Warn_color,
                   XmNforeground, Text_color,
                   XmNfontList, List_font,
                   NULL );

    rowcol = XtVaCreateManagedWidget( "close_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, Warn_color,
                         XmNorientation, XmHORIZONTAL,
                         XmNpacking, XmPACK_TIGHT,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, label,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

    /* Use blank label for horizontal space. */
    label = XtVaCreateManagedWidget( "                    ",
                   xmLabelWidgetClass, rowcol,
                   XmNbackground, Warn_color,
                   XmNforeground, Text_color,
                   XmNfontList, List_font,
                   NULL );

    Close_button = XtVaCreateManagedWidget( "Close",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList, List_font,
                   NULL);

    XtAddCallback( Close_button, XmNactivateCallback, Close_callback, NULL );
  }
  else
  {
    /* Add close and update buttons. */

    rowcol = XtVaCreateManagedWidget( "close_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, Bg_color,
                         XmNorientation, XmHORIZONTAL,
                         XmNpacking, XmPACK_TIGHT,
                         XmNnumColumns, 1,
                         XmNtopOffset, 20,
                         XmNleftOffset, 25,
                         XmNrightOffset, 20,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

    Close_button = XtVaCreateManagedWidget( "Close",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList, List_font,
                   NULL);

    XtAddCallback( Close_button, XmNactivateCallback, Close_callback, NULL );

    label = XtVaCreateManagedWidget( "            ",
                   xmLabelWidgetClass, rowcol,
                   XmNbackground, Bg_color,
                   NULL );

    /* Add outlets. */

    rowcol = Create_outlet_widgets( form_rowcol, rowcol );

    /* Add On/Off/Reboot buttons. */

    rowcol = XtVaCreateManagedWidget( "on_off_reboot_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, Bg_color,
                         XmNorientation, XmHORIZONTAL,
                         XmNpacking, XmPACK_TIGHT,
                         XmNnumColumns, 1,
                         XmNtopOffset, 20,
                         XmNleftOffset, 25,
                         XmNrightOffset, 20,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, rowcol,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

    Turn_off_button = XtVaCreateManagedWidget( "Turn Off",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList, List_font,
                   XmNsensitive, True,
                   NULL);

    XtAddCallback( Turn_off_button, XmNactivateCallback, Turn_off_callback, NULL );

    label = XtVaCreateManagedWidget( "   ",
                   xmLabelWidgetClass, rowcol,
                   XmNbackground, Bg_color,
                   NULL );

    Turn_on_button = XtVaCreateManagedWidget( "Turn On",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList, List_font,
                   XmNsensitive, True,
                   NULL);

    XtAddCallback( Turn_on_button, XmNactivateCallback, Turn_on_callback, NULL );

    label = XtVaCreateManagedWidget( "   ",
                   xmLabelWidgetClass, rowcol,
                   XmNbackground, Bg_color,
                   NULL );

    Reboot_button = XtVaCreateManagedWidget( "Reboot",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList, List_font,
                   XmNsensitive, True,
                   NULL);

    XtAddCallback( Reboot_button, XmNactivateCallback, Reboot_callback, NULL );
  } /* If power control version is known. */

  XtManageChild( Form );

  /* Display GUI to screen. */

  XtRealizeWidget( Top_widget );

  /* Initialize status dialog. */

  Initialize_status_dialog();

  /* Start HCI loop. */

  if( power_control_version == PC_DEVICE_AVAILABLE )
  {
    HCI_start( Timer_proc, SHORT_TIMER_EVENT, NO_RESIZE_HCI );
  }
  else
  {
    HCI_start( NULL, -1, NO_RESIZE_HCI );
  }

  return 0;
}

/***************************************************************************
 Description: Main window destroy callback function.
 ***************************************************************************/

static int Destroy_callback( Widget w, XtPointer y, XtPointer z )
{
  if( No_close_allowed == YES )
  {
    sprintf( Popup_buf, "You are not allowed to exit this task\n" );
    strcat( Popup_buf, "until the countdown timer is finished." );
    hci_warning_popup( Top_widget, Popup_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/***************************************************************************
 Description: Close button callback function.
 ***************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  XtDestroyWidget( Top_widget );
}

/***************************************************************************
 Description: Callback to determine if top widget is minimized.
 ***************************************************************************/

static void Map_cb( Widget w, XtPointer x, XEvent *y, Boolean *z )
{
  if( y->type == MapNotify ){ Top_widget_minimized_flag = NO; }
  else if( y->type == UnmapNotify ){ Top_widget_minimized_flag = YES; }
  else{ /* Ignore */ }
}

/***************************************************************************
 Description: Callback for event timer.
 ***************************************************************************/

static void Timer_proc()
{
  static time_t last_update = 0;
  static time_t update_change_time = 0;
  static int update_next_timer_event = NO;
  time_t current_time = time( NULL );

  /* Handle coprocess that monitor network connectivity. */

  if( Coprocess_flag == HCI_CP_NOT_STARTED )
  {
    Start_coprocess();
  }
  else if( Coprocess_flag == HCI_CP_STARTED )
  {
    Manage_coprocess();
  }
  else if( Coprocess_flag == HCI_CP_FINISHED )
  {
    Coprocess_flag = HCI_CP_NOT_STARTED;
  }

  if( Wait_for_PC_confirmation == YES )
  {
    /* Do nothing. Still waiting for user to acknowledge popup. */
  }
  else if( PC_confirmation != PC_NONE )
  {
    if( PC_confirmation == YES )
    {
      Execute_power_control_yes();
      HCI_set_timer_interval( SHORT_TIMER_EVENT );
      Update_period = SHORT_UPDATE_PERIOD;
      update_change_time = current_time;
    }
    else
    {
      Execute_power_control_no();
    }
    Reset_to_initial_state();
  }
  else if( Power_control_flag != PC_NONE )
  {
    PC_action();
  }
  else if( update_next_timer_event )
  {
    update_next_timer_event = NO;
    Update_flag = NO;
    last_update = current_time;
    Update_outlet_labels();
    Popdown_status_dialog();
    HCI_default_cursor();
    HCI_set_timer_interval( DEFAULT_TIMER_EVENT );
    if( current_time >= update_change_time + UPDATE_CHANGE_INTERVAL )
    {
      Update_period = DEFAULT_UPDATE_PERIOD;
    }
  }
  else if( Update_flag || current_time >= ( last_update + Update_period ) )
  {
    update_next_timer_event = YES;
    HCI_set_timer_interval( SHORT_TIMER_EVENT );
    HCI_busy_cursor();
    Popup_status_dialog( "Obtaining status of outlets" );
  }
}

/**************************************************************************
  Description: Starts coprocess to monitor connectivity.
**************************************************************************/

static void Start_coprocess()
{
  int ret;
  char buf[MISC_BUF_LEN];

  sprintf( buf, "mping -p %d", MPING_TIMER );

  if( ( ret = MISC_cp_open( buf, 0, &Cp ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "MISC_cp_open mping failed (%d)", ret );
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }

  sprintf( buf, "++%s\n", Pc_addr );

  if( Cp != NULL && ( ( ret = MISC_cp_write_to_cp( Cp, buf ) ) < 0 ) )
  {
    LE_send_msg( GL_ERROR, "Cp write failed (%d) for %s", ret, buf );
    Close_coprocess();
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }

  Coprocess_flag = HCI_CP_STARTED;
}

/**************************************************************************
  Description: Manages coprocess to monitor connectivity.
**************************************************************************/

static void Manage_coprocess()
{
  char cp_buf[COPROCESS_SIZE];
  int ret;

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, COPROCESS_SIZE ) ) != 0 )
  {
    /* Strip trailing newline. */

    if( cp_buf[strlen( cp_buf )-1] == '\n' )
    {
      cp_buf[strlen( cp_buf )-1] = '\0';
    }

    /* MISC function succeeded. Take appropriate action. */

    if( ret == HCI_CP_DOWN )
    {
      Close_coprocess();
    }
    else if( ret == HCI_CP_STDERR )
    {
      LE_send_msg( GL_ERROR, "ERROR mping: (%s)", cp_buf );
    }
    else if( ret == MISC_CP_STDOUT )
    {
      Parse_connect_status( cp_buf );
    }
    else
    {
      /* Do nothing. */
    }
  }
}

/**************************************************************************
  Description: Close coprocess.
**************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/**************************************************************************
  Description: Parses connectivity status output from mping
**************************************************************************/

static void Parse_connect_status( char *status )
{
  int qtime;
  char *p = NULL;

  /* Parse status string returned from mping. Format
     should be hostname--# where hostname is Pc_addr,
     and # is number of seconds since mping last
     made network contact.  */

  p = strstr( status, "--" );

  if( p == NULL || sscanf( p + 2, "%d", &qtime ) != 1 )
  {
    LE_send_msg( GL_ERROR, "Unexpected mping token (%s)", status );
  }
  else
  {
    if( qtime == 0 )
    {
      Power_control_network_status = PWRADM_CONNECTED;
    }
    else if( qtime > 0 )
    {
      Power_control_network_status = PWRADM_DISCONNECTED;
    }
  }
}

/***************************************************************************
 Description: Read mscf.conf file for user-defined information.
 ***************************************************************************/

static void Read_mscf_conf()
{
  int  i, j, new = 0, ret;
  char key[KEY_LEN];
  char cmd[MAX_CMD_LEN];
  char pc_cmd_disable_string[MAX_CMD_LEN];

  /* Initially it does not matter which conf file is read.  
     We need the hostname of the power administrator. */

  if( System_flag == HCI_FAA_SYSTEM && Channel_number == 2 )
  {
    strcpy( Mscf_conf_name, MSCF_CH2_CONF_NAME );
  }
  else
  {
    strcpy( Mscf_conf_name, MSCF_DEFAULT_CONF_NAME );
  }

  CS_cfg_name( Mscf_conf_name );
  CS_control( CS_COMMENT | '#' );

  sprintf( key, "Power_control_device_address:" );
  if( ( ret = CS_entry( key, 1, MAX_ADDRESS_LEN, Pc_addr ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  LE_send_msg( GL_INFO, "Read_mscf_conf-->Power_control_device_address: %s\n",
               Pc_addr );

  /* SNMP command to get version of device. The version is used to
     differentiate between the APC and Sentry units. */

  sprintf( cmd, "snmpwalk -m ALL -c npios -v 1 %s systemVersion", Pc_addr );
  Send_snmp_command( cmd );

  /* Check the return string.  New power administrator will have "Sentry"
     in its version. */

  new = 0;
  if( (Current_text_pointer != NULL) 
                  && 
      (strstr( Current_text_pointer, "Sentry" ) != NULL) )
     new = 1;

  if( new )
     Pwr_adm = SENTRY;
  else
     Pwr_adm = APC;

  if( new )
  {
     if( System_flag == HCI_FAA_SYSTEM && Channel_number == 2 )
     {
       strcpy( Mscf_conf_name, MSCF_CH2_NPA_CONF_NAME );
     }
     else
     {
       strcpy( Mscf_conf_name, MSCF_NPA_DEF_CONF_NAME );
     }
  }
  else
  {
     if( System_flag == HCI_FAA_SYSTEM && Channel_number == 2 )
     {
       strcpy( Mscf_conf_name, MSCF_CH2_CONF_NAME );
     }
     else
     {
       strcpy( Mscf_conf_name, MSCF_DEFAULT_CONF_NAME );
     }
  }
  LE_send_msg( GL_INFO, "MSCF conf file set to %s", Mscf_conf_name);

  CS_cfg_name( Mscf_conf_name );
  CS_control( CS_COMMENT | '#' );

  sprintf( key, "Power_control_device_address:" );
  if( ( ret = CS_entry( key, 1, MAX_ADDRESS_LEN, Pc_addr ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  LE_send_msg( GL_INFO, "Read_mscf_conf-->Power_control_device_address: %s\n",
               Pc_addr );

  sprintf( key, "Pv_cmd:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Pv_cmd ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Pv_cmd: %s\n", Pv_cmd );

  sprintf( key, "Pc_cmd:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Pc_cmd ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Pc_cmd: %s\n", Pc_cmd );

  for( i = PC_TURN_ON; i < NUM_PWRCTRL_OPTIONS; i++ )
  {
    sprintf( key, "Pc_ret_strs:" );
    if( ( ret = CS_entry( key, i+1, MAX_STRING_LEN, Pc_ret_strs[i] ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d) tok %d", key,ret,i );
      exit(1);
    }
    if( Verbose )
      LE_send_msg( GL_INFO, "Read_mscf_conf-->Pc_ret_strs[%d]: %s\n",
                   i, Pc_ret_strs[i] );
  }

  sprintf( key, "Pc_key:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Pc_key ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Pc_key: %s\n", Pc_key );

  sprintf( key, "Pn_cmd:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Pn_cmd ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Pn_cmd: %s\n", Pn_cmd );

  sprintf( key, "Pn_key:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Pn_key ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Pn_key: %s\n", Pn_key );

  sprintf( key, "Ps_cmd:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Ps_cmd ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Ps_cmd: %s\n", Ps_cmd );

  for( i = PC_ON_STATE; i < NUM_PWRCTRL_STATES; i++ )
  {
    sprintf( key, "Ps_ret_strs:" );
    if( ( ret = CS_entry( key, i+1, MAX_STRING_LEN, Ps_ret_strs[i] ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d) tok %d", key,ret,i );
      exit(1);
    }
    if( Verbose )
      LE_send_msg( GL_INFO, "Read_mscf_conf-->Ps_ret_strs[%d]: %s\n",
                   i, Ps_ret_strs[i] );
  }

  sprintf( key, "Ps_key:" );
  if( ( ret = CS_entry( key, 1, MAX_CMD_LEN, Ps_key ) ) <= 0 )
  {
    LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
    exit(1);
  }
  if( Verbose )
    LE_send_msg( GL_INFO, "Read_mscf_conf-->Ps_key: %s\n", Ps_key );

  for( i = 0; i < MAX_NUM_OUTLETS; i++ )
  {
    sprintf( key, "Pc_host_name_%1d:", i+1 );
    if( CS_entry( key, 1, MAX_CMD_LEN, (void*)&Outlet_status[i].host_name[0] ) <= 0 )
    {
      Outlet_status[i].host_name[0] = '\0';
    }
    if( Verbose )
      LE_send_msg( GL_INFO, "Read_mscf_conf-->Outlet_status[%d].host_name: %s\n", 
                   i, Outlet_status[i].host_name );

    sprintf( key, "Pc_host_shutdown_cmd_%1d:", i+1 );
    if( CS_entry( key, 1, MAX_CMD_LEN, (void*)&Outlet_status[i].shutdown_cmd[0] ) <= 0 )
    {
      Outlet_status[i].shutdown_cmd[0] = '\0';
    }
    if( Verbose )
      LE_send_msg( GL_INFO, "Read_mscf_conf-->Outlet_status[%d].shutdown_cmd: %s\n", 
                   i, Outlet_status[i].shutdown_cmd );

    sprintf( key, "Pc_cmd_disable_%1d:", i+1 );
    if( CS_entry( key, 1, MAX_CMD_LEN, (void*)&pc_cmd_disable_string ) <= 0 )
    {
      for( j = PC_TURN_ON; j < NUM_PWRCTRL_OPTIONS; j++ )
      {
        Outlet_status[i].enable_turn_on = YES;
        Outlet_status[i].enable_turn_off = YES;
        Outlet_status[i].enable_reboot = YES;
        if( Verbose ){
          LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].enable_turn_on: %d\n", 
                       i, Outlet_status[i].enable_turn_on );
          LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].enable_turn_off: %d\n", 
                       i, Outlet_status[i].enable_turn_off );
          LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].enable_reboot: %d\n", 
                       i, Outlet_status[i].enable_reboot );
        }
      }
    }
    else
    {
      if( strstr( pc_cmd_disable_string, "on" ) != NULL )
      {
        Outlet_status[i].enable_turn_on = NO;
      }
      else
      {
        Outlet_status[i].enable_turn_on = YES;
      }
      if( Verbose )
        LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].enable_turn_on: %d\n", 
                     i, Outlet_status[i].enable_turn_on );

      if( strstr( pc_cmd_disable_string, "off" ) != NULL )
      {
        Outlet_status[i].enable_turn_off = NO;
      }
      else
      {
        Outlet_status[i].enable_turn_off = YES;
      }
      if( Verbose )
        LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].enable_turn_off: %d\n", 
                     i, Outlet_status[i].enable_turn_off );

      if( strstr( pc_cmd_disable_string, "reboot" ) != NULL )
      {
        Outlet_status[i].enable_reboot = NO;
      }
      else
      {
        Outlet_status[i].enable_reboot = YES;
      }
      if( Verbose )
        LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].enable_reboot: %d\n", 
                     i, Outlet_status[i].enable_reboot );
    }

    /* Shutdown Delay */

    sprintf( key, "Pc_host_shutdown_delay_%1d:", i+1 );
    if( CS_entry( key, 1 | CS_INT, 0, (void*)&Outlet_status[i].shutdown_delay ) < 0 )
    {
      Outlet_status[i].shutdown_delay = 20;
    }
    else if( Outlet_status[i].shutdown_delay < 0 )
    {
      Outlet_status[i].shutdown_delay = 20;
    }
    else if( Outlet_status[i].shutdown_delay > 600 )
    {
      Outlet_status[i].shutdown_delay = 600;
    }
    if( Verbose )
      LE_send_msg( GL_INFO, "Read_mscf_conf---->Outlet_status[%d].shutdown_delay: %d\n", 
                   i, Outlet_status[i].shutdown_delay );
  }
}

/***************************************************************************
 Description: Initialze pixmaps for outlets.
 ***************************************************************************/

static void Initialize_outlet_pixmaps()
{
  XImage ximage;
  ximage.width            = power_control_icon_width;
  ximage.height           = power_control_icon_height;
  ximage.data             = (char *)power_control_icon_bits;
  ximage.xoffset          = 0;
  ximage.format           = XYBitmap;
  ximage.byte_order       = MSBFirst;
  ximage.bitmap_pad       = 8;
  ximage.bitmap_bit_order = LSBFirst;
  ximage.bitmap_unit      = 8;
  ximage.depth            = 1;
  ximage.bytes_per_line   = power_control_icon_width/8;
  ximage.obdata           = NULL;

  XmInstallImage( &ximage, "power_control_icon" );

  Active_icon_pixmaps[PC_OFF_STATE][PC_SELECT_NO] =
      XmGetPixmap( XtScreen( Top_widget ),
                             "power_control_icon",
                             White_color,
                             Alarm_color );

  Active_icon_pixmaps[PC_ON_STATE][PC_SELECT_NO] =
      XmGetPixmap( XtScreen( Top_widget ),
                             "power_control_icon",
                             White_color,
                             Normal_color );

  Active_icon_pixmaps[PC_OFF_STATE][PC_SELECT_YES] =
      XmGetPixmap( XtScreen( Top_widget ),
                             "power_control_icon",
                             Warn_color,
                             Alarm_color );

  Active_icon_pixmaps[PC_ON_STATE][PC_SELECT_YES] =
      XmGetPixmap( XtScreen( Top_widget ),
                             "power_control_icon",
                             Warn_color,
                             Normal_color );

  Inactive_icon_pixmap =
      XmGetPixmap( XtScreen( Top_widget ),
                             "power_control_icon",
                             White_color,
                             Gray_color );
}

/***************************************************************************
 Description: Get names of outlets.
 ***************************************************************************/

static void Get_outlet_names()
{
   if( Pwr_adm == SENTRY )
      Sentry_get_outlet_names();

   else
      APC_get_outlet_names();
}

/***************************************************************************
 Description: Get status of outlets.
 ***************************************************************************/

static void Get_outlet_status()
{
   if( Pwr_adm == SENTRY )
      Sentry_get_outlet_status();

   else
      APC_get_outlet_status();
}

/***************************************************************************
 Description: Update each outlet's label.
 ***************************************************************************/

static void Update_outlet_labels()
{
  static int Initialize_outlet_names = NO;
  int i;

  if( Power_control_network_status == PWRADM_DISCONNECTED )
  {
    XtSetSensitive( Form, False );
    return;
  }
  else
  {
    XtSetSensitive( Form, True );
  }

  if( (!Initialize_outlet_names)
                 ||
      (Log_outlet_names) )
  {
    Initialize_outlet_names = YES;
    Get_outlet_names();
    if( Log_outlet_names )
       Log_outlet_names = 0;
  }

  Get_outlet_status();

  if( Num_outlets == 0 )
  {
    LE_send_msg( GL_ERROR, "No outlet found on power control device" );
    XtSetSensitive( Turn_off_button, False );
    XtSetSensitive( Turn_on_button, False );
    XtSetSensitive( Reboot_button, False );
    return;
  }

  for( i = 0; i < Num_outlets; i++ )
  {
    XtVaSetValues( Outlet_status[i].icon, XmNlabelPixmap,
         Active_icon_pixmaps[Outlet_status[i].on][Outlet_status[i].selected], NULL );

    XmString label = XmStringCreateLocalized( Outlet_status[i].label );

    if( label != NULL )
    {
      XtVaSetValues( Outlet_status[i].label_widget, XmNlabelString, label,
                     XmNalignment, XmALIGNMENT_CENTER,
                     XmNfontList, List_font, NULL );

      XmStringFree( label );
    }
  }

  for( i = Num_outlets; i < MAX_NUM_OUTLETS; i++ )
  {
    Outlet_status[Num_outlets].active = PC_ACTIVE_NO;
    Outlet_status[i].on = PC_OFF_STATE;
    Outlet_status[i].selected = PC_SELECT_NO;
    strcpy( Outlet_status[i].label, " Unknown \n status " );
    Outlet_status[Num_outlets].switch_index = -1;
    Outlet_status[Num_outlets].outlet_index = -1;
    Outlet_status[Num_outlets].enable_turn_on = YES;
    Outlet_status[Num_outlets].enable_turn_off = YES;
    Outlet_status[Num_outlets].enable_reboot = YES;
    Outlet_status[i].host_name[0] = '\0';
    Outlet_status[i].shutdown_cmd[0] = '\0';
    Outlet_status[i].shutdown_delay = 0;
    XtVaSetValues( Outlet_status[i].icon, XmNlabelPixmap,
                   Inactive_icon_pixmap, NULL );
    XtSetSensitive( Outlet_status[i].icon, False );
  } 
}

/***************************************************************************
 Description: Callback for "Turn Off" button.
 ***************************************************************************/

static void Turn_off_callback( Widget w, XtPointer x, XtPointer y )
{
  Power_control_flag = PC_TURN_OFF;
  if( Verbose )
    LE_send_msg( GL_INFO, "Turn_off_callback-->Power_control_flag: %d\n",
                 Power_control_flag );
  XtSetSensitive( Turn_off_button, False );
  XtSetSensitive( Turn_on_button, False );
  XtSetSensitive( Reboot_button, False );
  HCI_busy_cursor();
}

/***************************************************************************
 Description: Callback for "Turn On" button.
 ***************************************************************************/

static void Turn_on_callback( Widget w, XtPointer x, XtPointer y )
{
  Power_control_flag = PC_TURN_ON;
  if( Verbose )
    LE_send_msg( GL_INFO, "Turn_on_callback-->Power_control_flag: %d\n",
                 Power_control_flag );
  XtSetSensitive( Turn_off_button, False );
  XtSetSensitive( Turn_on_button, False );
  XtSetSensitive( Reboot_button, False );
  HCI_busy_cursor();
}

/***************************************************************************
 Description: Callback for Reboot button.
 ***************************************************************************/

static void Reboot_callback( Widget w, XtPointer x, XtPointer y )
{
  Power_control_flag = PC_REBOOT;
  if( Verbose )
    LE_send_msg( GL_INFO, "Reboot_callback-->Power_control_flag: %d\n",
                 Power_control_flag );
  XtSetSensitive( Turn_off_button, False );
  XtSetSensitive( Turn_on_button, False );
  XtSetSensitive( Reboot_button, False );
  HCI_busy_cursor();
}

/***************************************************************************
 Description: Get version of power control device.
 ***************************************************************************/

static int Get_power_control_version( int *new )
{
  char cmd[MAX_CMD_LEN];

  /* SNMP command to get version of device. The version was previously
     used to differentiate between the MS and MSP in Build 8.0. It is
     kept here as a check to make sure the power administrator is
     available on the network. */

  sprintf( cmd, Pv_cmd, Pc_addr );
  if( Verbose )
    LE_send_msg( GL_INFO, "Get_power_control_version-->Send_snmp_command( %s )\n", cmd );
  Send_snmp_command( cmd );

  if( Current_text_pointer == NULL )
  {
    LE_send_msg(GL_ERROR, "Error getting Power Control version");
    return PC_DEVICE_UNAVAILABLE;
  }

  /* Check the return string.  New power administrator will have "Sentry"
     in its version. */
  *new = 0;
  if( strstr( Current_text_pointer, "Sentry" ) != NULL )
     *new = 1;

  return PC_DEVICE_AVAILABLE;
}

/***************************************************************************
 Description: Sends SNMP command to power control device.
 ***************************************************************************/

void Send_snmp_command( char *cmd )
{
  int ret, n_bytes;
  char output_buf[ MAX_OUT_TEXT_LEN ];

  LE_send_msg( GL_INFO, "MISC_system_to_buffer( %s )\n", cmd );
  ret = MISC_system_to_buffer( cmd, output_buf, MAX_OUT_TEXT_LEN, &n_bytes );
  if( ret != 0 )
  {
    Current_text_pointer = NULL;
    LE_send_msg( GL_ERROR, "Failed (%d) in executing \"%s\"", ret, cmd );
  }
  else
  {
    if( n_bytes == 0 ){ Current_text_pointer = NULL; }
    else{ Current_text_pointer = &output_buf[0]; }
  }
}

/***************************************************************************
 Description: Create outlet widgets to show status.
 ***************************************************************************/

static Widget Create_outlet_widgets( Widget parent, Widget top_attachment )
{
  int i;
  Widget horiz_rowcol, vertical_rowcol;

  horiz_rowcol = XtVaCreateWidget( "button_holder",
                xmRowColumnWidgetClass, parent,
                XmNtopOffset,           20,
                XmNleftOffset,          20,
                XmNrightOffset,         20,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           top_attachment,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNforeground,          Bg_color,
                XmNbackground,          Bg_color,
                XmNorientation,         XmHORIZONTAL,
                XmNpacking,             XmPACK_COLUMN,
                XmNnumColumns,          MAX_NUM_SWITCHES,
                XmNisAligned,           False,
                NULL );

  for( i = 0; i < MAX_NUM_OUTLETS; i++ )
  {
    /* Use temporary widget to allow for vertical orientation       *
     * of outlet pixmap and labels.                                 */

    vertical_rowcol = XtVaCreateWidget( "vertical_rowcol",
                xmRowColumnWidgetClass, horiz_rowcol,
                XmNforeground,          Bg_color,
                XmNbackground,          Bg_color,
                XmNorientation,         XmVERTICAL,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                XmNisAligned,           False,
                NULL );

    /* Create outlet pixmap/button. */

    Outlet_status[i].icon = XtVaCreateManagedWidget( "",
            xmDrawnButtonWidgetClass,   vertical_rowcol,
            XmNlabelType,               XmPIXMAP,
            XmNlabelPixmap,             Inactive_icon_pixmap,
            XmNpushButtonEnabled,       True,
            XmNsensitive,               False,
            NULL );

    /* Add callback function to outlet pixmap/button. */

    XtAddCallback( Outlet_status[i].icon,
                XmNactivateCallback, Pc_icon_activate, (XtPointer) i );

    /* Create label to identify device associated   *
     * with outlet pixmap/button.                   */

    Outlet_status[i].label_widget = XtVaCreateManagedWidget( Outlet_status[i].label,
            xmLabelWidgetClass,         vertical_rowcol,
            XmNforeground,              Text_color,
            XmNbackground,              Bg_color,
            XmNfontList,                List_font,
            XmNalignment,               XmALIGNMENT_CENTER,
            NULL );

    XtManageChild( vertical_rowcol );
  }

  XtManageChild( horiz_rowcol );
  return horiz_rowcol;
}

/***************************************************************************
 Description: Callback when outlet widget is selected.
 ***************************************************************************/

static void Pc_icon_activate( Widget w, XtPointer x, XtPointer y )
{
  int i = (int)x;
  int enable_on = 1;
  int enable_off = 1;
  int enable_reboot = 1;
  int j;

  /* Toggle selected state */

  if( Outlet_status[i].active == PC_ACTIVE_NO )
  {
    return;
  }
  else if( Outlet_status[i].selected == PC_SELECT_YES )
  {
    Outlet_status[i].selected = PC_SELECT_NO;
  }
  else
  {
    Outlet_status[i].selected = PC_SELECT_YES;
  }

  /* Determine if devices are configured to be turned on/off  *
   * or rebooted and set button sensitivities accordingly.    */

  for( j = 0; j < MAX_NUM_OUTLETS; j++ )
  {
    if( enable_on &&
        Outlet_status[j].selected == PC_SELECT_YES &&
        Outlet_status[j].enable_turn_on == NO )
    {
       enable_on = 0;
    }

    if( enable_off &&
        Outlet_status[j].selected == PC_SELECT_YES &&
        Outlet_status[j].enable_turn_off == NO )
    {
       enable_off = 0;
    }

    if( enable_reboot &&
        Outlet_status[j].selected == PC_SELECT_YES &&
        Outlet_status[j].enable_reboot == NO )
    {
         enable_reboot = 0;
    }
  }

  if( enable_on )
  {
    XtVaSetValues( Turn_on_button, XmNsensitive, True, NULL );
  }
  else
  {
    XtVaSetValues( Turn_on_button, XmNsensitive, False, NULL );
  }

  if( enable_off )
  {
    XtVaSetValues( Turn_off_button, XmNsensitive, True, NULL );
  }
  else
  {
    XtVaSetValues( Turn_off_button, XmNsensitive, False, NULL );
  }

  if( enable_reboot )
  {
    XtVaSetValues( Reboot_button, XmNsensitive, True, NULL );
  }
  else
  {
    XtVaSetValues( Reboot_button, XmNsensitive, False, NULL );
  }

  XtVaSetValues( Outlet_status[i].icon, XmNlabelPixmap,
      Active_icon_pixmaps[Outlet_status[i].on][Outlet_status[i].selected], NULL );
}

/***************************************************************************
 Description: Perform action defined by Power_control_flag.
 ***************************************************************************/

static void PC_action()
{
  int num_selected, num_valid_selected, i, j;
  static int warning_already_popped_up = NO;

  if( Power_control_network_status == PWRADM_DISCONNECTED )
  {
    if( warning_already_popped_up == NO )
    {
      warning_already_popped_up = YES;
      XtSetSensitive( Form, False );
      sprintf( Popup_buf, "An error occurred while trying to verify a network\nconnection to the power control device. If this\nproblem continues, call the WSR-88D Hotline." );
      hci_warning_popup( Top_widget, Popup_buf, NULL );
    }
    return;
  }
  warning_already_popped_up = NO;

  /* Set popup message depending on button selected. */

  if( Power_control_flag == PC_TURN_ON )
  {
    strcpy( Popup_buf, "You are about to turn ON devices:\n" );
    LE_send_msg( GL_INFO, "ON button selected" );
  }
  else if( Power_control_flag == PC_TURN_OFF )
  {
    strcpy( Popup_buf, "You are about to turn OFF devices:\n" );
    LE_send_msg( GL_INFO, "OFF button selected" );
  }
  else if( Power_control_flag == PC_REBOOT )
  {
    for( i = 0; i < Num_outlets; i++ )
    {
      if( strstr( Outlet_status[i].label, "RPGA" ) != NULL )
      {
        if( Outlet_status[i].selected == PC_SELECT_YES )
        {
          for( j = 0; j < Num_outlets; j++ )
          {
            if( strstr( Outlet_status[j].label, "RPGB" ) != NULL )
            {
              if( Outlet_status[j].selected == PC_SELECT_YES )
              {
                  sprintf( Popup_buf, "Rebooting RPGA and RPGB at the same time is not allowed.\nIf both need to be rebooted, it is recommended that each\nbe done separately, with RPGB rebooted first." );
                  hci_warning_popup( Top_widget, Popup_buf, NULL );
                  Power_control_no( NULL, NULL, NULL );
                  return;
              }
            }
          }
        }
      }
    }

    strcpy( Popup_buf, "You are about to REBOOT devices:\n" );
    LE_send_msg( GL_INFO, "REBOOT button selected" );
  }

  /* Count number of device buttons selected. Add each selected       *
   * device to the popup message.                                     */

  num_selected = 0;
  num_valid_selected = 0;
  for( i = 0; i < Num_outlets; i++ )
  {
    if( Outlet_status[i].selected )
    {
      num_selected++;
      if( ( Power_control_flag == PC_TURN_ON &&
            Outlet_status[i].on == PC_OFF_STATE ) ||
          ( Power_control_flag == PC_REBOOT ) ||
          ( Power_control_flag == PC_TURN_OFF &&
            Outlet_status[i].on == PC_ON_STATE ) )
      {
        sprintf( Popup_buf+strlen( Popup_buf ), "%s, ", Outlet_status[i].label );
        num_valid_selected++;
      }
    }
  }

  strcat( Popup_buf, "\n\nThe RPG software may need to\nbe restarted.\n" );
  strcat( Popup_buf, "\nDo you want to continue?" );

  /* If button devices are selected, let user confirm actions.        *
   * If no button devices are selected, let popup inform user.        */

  if( num_valid_selected > 0 )
  {
    Wait_for_PC_confirmation = YES;
    hci_confirm_popup( Top_widget, Popup_buf, Power_control_yes, Power_control_no );
  }
  else if( Num_outlets < 1 )
  {
    hci_warning_popup( Top_widget, "Connection to Power Administrator lost.", NULL );
    Reset_to_initial_state();
  }
  else if( num_selected > 0 )
  {
    sprintf( Popup_buf, "The desired action is not valid for the selected outlets." );
    hci_warning_popup( Top_widget, Popup_buf, NULL );
    Reset_to_initial_state();
  }
  else
  {
    sprintf( Popup_buf, "You need to select the appropriate device(s)\nby clicking on the connector icon." );
    hci_warning_popup( Top_widget, Popup_buf, NULL );
    Reset_to_initial_state();
  }
}

/***************************************************************************
 Description: YES confirmation callback.
 ***************************************************************************/

static void Power_control_yes( Widget w, XtPointer x, XtPointer y )
{
  PC_confirmation = YES;
  Wait_for_PC_confirmation = NO;
}

/***************************************************************************
 Description: NO confirmation callback.
 ***************************************************************************/

static void Power_control_no( Widget w, XtPointer x, XtPointer y )
{
  PC_confirmation = NO;
  Wait_for_PC_confirmation = NO;
}

/***************************************************************************
 Description: User confirmed power control action.
 ***************************************************************************/

static void Execute_power_control_yes()
{
  int i;
  int cnt = 0;
  int ret = -1;
  char action_buf[MAX_STRING_LEN];

  LE_send_msg( GL_INFO, "Execute Power Control YES" );

  for( i = 0; i < Num_outlets; i++ )
  {
    if( Outlet_status[i].selected == PC_SELECT_YES )
    {
      if( Power_control_flag == PC_TURN_ON &&
          Outlet_status[i].on  == PC_OFF_STATE )
      {
        sprintf( action_buf, "power on" );
      }
      else if( Power_control_flag == PC_TURN_OFF &&
          Outlet_status[i].on == PC_ON_STATE )
      {
        sprintf( action_buf, "power off" );
      }
      else if( Power_control_flag == PC_REBOOT )
      {
        sprintf( action_buf, "reboot" );
      }

      LE_send_msg( GL_INFO, "Attempt to %s outlet: %s",
                   action_buf, Outlet_status[i].label );
      
      ret = PC_power_control( Outlet_status[i].switch_index,
                              Outlet_status[i].outlet_index );
      if( ret > 0 )
      {
         LE_send_msg( GL_INFO, "Successful %s of %s outlet",
                      action_buf, Outlet_status[i].label );
         cnt++;
      }
      else if( ret == 0 )
      {
        LE_send_msg( GL_ERROR, "Cannot change local outlet" );
        sprintf( Popup_buf, "You are not allowed to control\npower of the local outlet." );
        hci_error_popup( Top_widget, Popup_buf, NULL );
      }
      else
      {
        LE_send_msg( GL_ERROR, "Unable to %s outlet %s",
                     action_buf, Outlet_status[i].label );
        sprintf( Popup_buf, "Failed (%d) to %s outlet %s\n",
                 ret, action_buf, Outlet_status[i].label );
        strcat( Popup_buf, "Try again. If it continues to fail,\n" );
        strcat( Popup_buf, "call the WSR-88D Hotline." );
        hci_error_popup( Top_widget, Popup_buf, NULL );
      }
    }
  }

  /* Reset outlets to not be selected. */
  Execute_power_control_no();

  if( cnt > 0 )
  {
    Update_flag = YES;
  }
}

/***************************************************************************
 Description: User decided to not execute power control.
 ***************************************************************************/

static void Execute_power_control_no()
{
  int i;

  LE_send_msg( GL_INFO, "Execute Power Control NO" );

  /* reset all selected icons */
  for( i = 0; i < Num_outlets; i++ )
  {
    if( Outlet_status[i].selected == PC_SELECT_YES )
    {
      Outlet_status[i].selected = PC_SELECT_NO;
    }
    XtVaSetValues( Outlet_status[i].icon, XmNlabelPixmap,
    Active_icon_pixmaps[Outlet_status[i].on][Outlet_status[i].selected], NULL );
  }
}

/***************************************************************************
 Description: Sends SNMP power control commands.
 ***************************************************************************/

static int PC_power_control( int sw_ind, int o_ind )
{
   int ret;

   if( Pwr_adm == SENTRY )
      ret = Sentry_PC_power_control( sw_ind, o_ind );

   else
      ret = APC_PC_power_control( sw_ind, o_ind );

  return ret;
}

/***************************************************************************
 Description: Launch countdown popup.
 ***************************************************************************/

void Popup_wait_for_command( char *title_text, char *command_text, int s )
{
  static Widget wait_dialog = NULL;

  LE_send_msg( GL_INFO, "Wait Dialog - TITLE: %s CMD: %s WAIT: %d",
               title_text, command_text, s );

  /* Desensitize the GUI during the countdown and prevent user from
     closing GUI. */

  XtSetSensitive( Form, False );
  No_close_allowed = YES;
 
  if( wait_dialog == NULL )
  {
    LE_send_msg( GL_INFO, "wait_dialog == NULL...create for %d secs", s );
    wait_dialog = XmCreateInformationDialog( Top_widget, "wait", NULL, 0 );
    /* Set various attributes of popup. */
    XtUnmanageChild( XmMessageBoxGetChild(wait_dialog, XmDIALOG_OK_BUTTON));
    XtUnmanageChild( XmMessageBoxGetChild(wait_dialog, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild( XmMessageBoxGetChild(wait_dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild( XmMessageBoxGetChild(wait_dialog, XmDIALOG_SEPARATOR));
    XtUnmanageChild( XmMessageBoxGetChild(wait_dialog, XmDIALOG_SYMBOL_LABEL));
    XtVaSetValues( XmMessageBoxGetChild( wait_dialog, XmDIALOG_MESSAGE_LABEL ),
                   XmNforeground, Text_color,
                   XmNbackground, Bg_color,
                   XmNfontList, List_font, NULL );
    XtVaSetValues( wait_dialog,
                   XmNforeground, Text_color,
                   XmNbackground, Bg_color,
                   XmNdeleteResponse, XmDO_NOTHING, NULL );
    /* Set various attributes of popup parent. */
    XtVaSetValues( XtParent( wait_dialog ),
                   XmNtitle, "Wait Dialog",
                   XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_CLOSE, NULL );
  }

  Wait_time = s;

  Update_wait_text( command_text, wait_dialog );

  /* Make popup appear. */

  XtManageChild( wait_dialog );

  /*  Simulate main processing loop. */

  while( Wait_time > 0 )
  {
    Flush_X_events();
    sleep( 1 );
    Wait_time--;
    Update_wait_text( command_text, wait_dialog );
  }

  /* Make popup disappear. */

  XtUnmanageChild( wait_dialog );

  /* Restore GUI sensitivity */

  XtSetSensitive(Form,True);
  No_close_allowed = NO;
}

/***************************************************************************
 Description: Update text of countdown popup.
 ***************************************************************************/

static void Update_wait_text( char *command_text, Widget wait_dialog )
{
  XmString str;
  char display_text[MISC_BUF_LEN];

  sprintf( display_text, "%s\n%d seconds remaining", command_text, Wait_time );
  LE_send_msg( GL_INFO, "DISPLAY: %s", display_text );
  str = XmStringCreateLocalized(display_text );
  XtVaSetValues( wait_dialog, XmNmessageString, str, NULL );
  XmStringFree( str );
}

/***************************************************************************
 Description: Reset variables to initial defaults.
 ***************************************************************************/

static void Reset_to_initial_state()
{
  Power_control_flag = PC_NONE;
  Wait_for_PC_confirmation = NO;
  PC_confirmation = PC_NONE;
  HCI_default_cursor();
  XtSetSensitive( Turn_on_button, True );
  XtSetSensitive( Turn_off_button, True );
  XtSetSensitive( Reboot_button, True );
}

/***************************************************************************
 Description: Popup dialog when getting status of outlets.
 ***************************************************************************/

static void Popup_status_dialog( char *msg )
{
  /* If initial status of outlets has already been obtained or
     popup is already managed or top-level widget is minimized,
     then do nothing. Managing the popup when the top-level widget
     is minimized will cause the top-level widget to "un"-minimize.
     To make popup appear every time status is being obtained (and
     not just the first time), then remove all references to variable
     Initialized_outlets. */

  if( Initialized_outlets || XtIsManaged( Status_dialog ) || Top_widget_minimized_flag )
  {
    return;
  }

  /* Manage popup to make it appear. */
  XtManageChild( Status_dialog );
  /* Convert C-style buf to X-style buf. */
  XmString msg_string = XmStringCreateLtoR( msg, XmFONTLIST_DEFAULT_TAG );
  /* Set message in dialog. */
  XtVaSetValues( Status_dialog, XmNmessageString, msg_string, NULL );
  /* Force update of widget. */
  XmUpdateDisplay( Status_dialog );
  /* Cleanup allocated memory. */
  XmStringFree( msg_string );
}

/***************************************************************************
 Description: Popdown dialog when getting status of outlets.
 ***************************************************************************/

static void Popdown_status_dialog()
{ 
  if( !Initialized_outlets )
  {
    /* Reset flag. */
    Initialized_outlets = YES;
    /* If popup is managed, unmanage it to disappear. */
    if( XtIsManaged( Status_dialog ) )
    {
      XtUnmanageChild( Status_dialog );
    }
  }
}

/***************************************************************************
 Description: Initialize dialog to show status info.
 ***************************************************************************/

static void Initialize_status_dialog()
{ 
  /* Initialize status dialog. */
  Status_dialog = XmCreateInformationDialog( Top_widget, "Progress", NULL, 0 );
  
  /* Remove unwanted widgets. */
  XtUnmanageChild( XmMessageBoxGetChild( Status_dialog, XmDIALOG_OK_BUTTON ) ); 
  XtUnmanageChild( XmMessageBoxGetChild( Status_dialog, XmDIALOG_CANCEL_BUTTON ) );
  XtUnmanageChild( XmMessageBoxGetChild( Status_dialog, XmDIALOG_HELP_BUTTON ) );
  XtUnmanageChild( XmMessageBoxGetChild( Status_dialog, XmDIALOG_SEPARATOR ) ); 
  XtUnmanageChild( XmMessageBoxGetChild( Status_dialog, XmDIALOG_SYMBOL_LABEL ) );
  
  /* Set various attributes of popup. */
  XtVaSetValues( XmMessageBoxGetChild( Status_dialog, XmDIALOG_MESSAGE_LABEL ),
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font, NULL );

  XtVaSetValues( Status_dialog,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNdeleteResponse, XmDO_NOTHING, NULL );

  /* Set various attributes of popup parent. */
  XtVaSetValues( XtParent( Status_dialog ),
                 XmNtitle, "Read Status Progress",
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_CLOSE, NULL );
}

/***************************************************************************
 Description: Process all X events and flush event buffer.
 ***************************************************************************/

static void Flush_X_events()
{
  while( ( XtAppPending( HCI_get_appcontext() ) & XtIMAll ) != 0 )
  {
    XtAppProcessEvent( HCI_get_appcontext(), XtIMAll );
  }
}

/*************************************************************************

   Description:
      Event handler for ORPGEVT_MSCF_PWR_CTRL_VERBOSE event.

   Notes:
      See en man page for description of event notify handler format.

**************************************************************************/
void Notify_func( EN_id_t id, char *msg, int msg_size, void *arg ){

   /* Catch the ORPGEVT_MSCF_PWR_CTRL_VERBOSE event .... turn on/off
      Verbose mode. */
   if( id == ORPGEVT_MSCF_PWR_CTRL_VERBOSE )
   {
      if( Verbose > 0 )
      {
         LE_send_msg( GL_INFO, "Turn OFF Verbose Mode.\n" );
         Verbose = 0;
      }
      else{
         LE_send_msg( GL_INFO, "Turn ON Verbose Mode.\n" );
         Verbose++;
      }
   }

   if( id == ORPGEVT_MSCF_PWR_CTRL_GETNAMES )
   {
     Log_outlet_names++;
   }

}
