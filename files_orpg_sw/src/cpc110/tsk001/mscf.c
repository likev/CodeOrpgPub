/****************************************************************
 *								*
 *	mscf.c - The source code for MSCF GUI.			*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/25 16:00:56 $
 * $Id: mscf.c,v 1.7 2012/01/25 16:00:56 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/* Header files */

#include <hci.h>

/* Macros */

enum { NO_ACTION = 0, RPG_HCI_ACTION, RDA_HCI_ACTION, COMMS_STATUS_HCI_ACTION, POWER_CONTROL_HCI_ACTION, HUB_ROUTER_HCI_ACTION };

#define	MSCF_DEFAULT_CONF_NAME	"mscf1.conf"
#define	MSCF_CH2_CONF_NAME	"mscf2.conf"
#define	MSCF_NAME_LEN		128
#define	MSCF_CMD_LEN		128
#define	MAX_NET_NODES		20
#define	NODE_NAME_SIZE		64
#define	TEMP_BUF_SIZE		128
#define	COPROCESS_SIZE		512
#define	MSCF_TITLE		"Master System Control Functions"

typedef struct
{
  char *name;   /* node name */
  short status; /* connection status (MCS_UNKOWN, etc) */
  short old;    /* old connection status */
} Node_connect_t; /* Connection monitoring node */

enum { MCS_UNKOWN, MCS_CONNECTED, MCS_DISCONN }; /* Node connect status */

/* Global variables. */

static	Widget	Top_widget = (Widget) NULL;
static	Widget	Net_node_widgets[MAX_NET_NODES];
static	Widget	RPG_button = (Widget) NULL;
static	Widget	Pwr_button = (Widget) NULL;
static	Pixel	Bg_color = (Pixel) NULL;
static	Pixel	Text_color = (Pixel) NULL;
static	Pixel	Alarm_color = (Pixel) NULL;
static	Pixel	Warn_color = (Pixel) NULL;
static	Pixel	Normal_color = (Pixel) NULL;
static	Pixel	Button_bg_color = (Pixel) NULL;
static	Pixel	Button_fg_color = (Pixel) NULL;
static	Pixel	White_color = (Pixel) NULL;
static	XmFontList List_font = (XmFontList) NULL;
static	int	Action_flag = NO_ACTION;
static	int	System_flag = 0;
static	int	Node_flag = 0;
static	int	Mscf_has_hub_router = NO; /* MSCF has HUB router? */
static	int	Channel_number = 1; /* selected channel number */
static	char	Mscf_conf_name[MSCF_NAME_LEN]; /* configuration file name */
static	char	Rpg_hci_cmd[MSCF_CMD_LEN]; /* command starting RPG HCI */
static	char	Rda_hci_cmd[MSCF_CMD_LEN]; /* command starting RDA HCI */
static	char	Rda_hci_cmd2[MSCF_CMD_LEN]; /* command starting RDA HCI */
static	int	Num_network_nodes = 0;
static	void	*Cp = NULL;
static	int	Coprocess_flag = HCI_CP_NOT_STARTED;
static	Node_connect_t	Nodes[MAX_NET_NODES];
static	char	Title_buf[TEMP_BUF_SIZE] = "";
static	char	Cmd[MSCF_CMD_LEN] = "";

/*  Function prototypes  */

static	int	Destroy_callback( Widget, XtPointer, XtPointer );
static	void	Close_callback( Widget, XtPointer, XtPointer );
static	void	Channel_callback( Widget, XtPointer, XtPointer );
static	void	RPG_callback( Widget, XtPointer, XtPointer );
static	void	RDA_callback( Widget, XtPointer, XtPointer );
static	void	Pwr_callback( Widget, XtPointer, XtPointer );
static	void	Comms_status_callback( Widget, XtPointer, XtPointer );
static	void	HUB_rtr_callback( Widget, XtPointer, XtPointer );
static	void	Perform_action();
static	int	Read_mscf_conf();
static	int	Read_options( int, char **);
static	void	Print_usage( char ** );
static	void	Initialize_site_info();
static	void	Clear_data();
static	void	Set_connectivity_status( int, int );
static	void	Start_coprocess();
static	void	Manage_coprocess();
static	void	Close_coprocess();
static	void	Parse_connect_status( char * );
static	void	Print_cs_error( char * );
static	void	Timer_proc();

/**************************************************************************
  Description:  The main function.
**************************************************************************/

int main( int argc, char *argv[] )
{
  Widget form = (Widget) NULL;
  Widget control_frame = (Widget) NULL;
  Widget channel_frame = (Widget) NULL;
  Widget channel_radio = (Widget) NULL;
  Widget net_status_frame = (Widget) NULL;
  Widget rowcol = (Widget) NULL;
  Widget net_status_rc = (Widget) NULL;
  Widget button = (Widget) NULL;
  char   *channel_labels[] = { " 1", " 2" };
  int    i, n;
  Arg    args[16];

  /* Initialize HCI. */
  
  HCI_partial_init( argc, argv, -1 );
  
  Top_widget = HCI_get_top_widget();
  
  HCI_set_destroy_callback( Destroy_callback );
  
  System_flag = HCI_get_system();

  Node_flag = HCI_get_node();

  /* Initialize miscellaneous variables. */

  Bg_color = hci_get_read_color( BACKGROUND_COLOR1 );
  Text_color = hci_get_read_color( TEXT_FOREGROUND );
  Alarm_color = hci_get_read_color( ALARM_COLOR1 );
  Warn_color = hci_get_read_color( WARNING_COLOR );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Button_bg_color = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg_color = hci_get_read_color( BUTTON_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  List_font = hci_get_fontlist( LIST );
  
  /* Read command line options */

  if( Read_options( argc, argv ) != 0 ){ HCI_task_exit( HCI_EXIT_FAIL ); }

  /* Initialize site information. */

  Initialize_site_info();

  /* read configuration info */

  if( Read_mscf_conf() < 0 ){ HCI_task_exit( HCI_EXIT_FAIL ); }

  /* Define widgets to create GUI. */

  form = XtVaCreateWidget( "form",
             xmFormWidgetClass, Top_widget,
             XmNforeground, Bg_color,
             XmNbackground, Bg_color,
             NULL );

  control_frame = XtVaCreateManagedWidget( "control_frame",
                      xmFrameWidgetClass, form,
                      XmNbackground, Bg_color,
                      XmNtopAttachment, XmATTACH_FORM,
                      XmNleftAttachment, XmATTACH_FORM,
                      NULL );

  XtVaCreateManagedWidget( "Components",
                 xmLabelWidgetClass, control_frame,
                 XmNchildType, XmFRAME_TITLE_CHILD,
                 XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                 XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 NULL );

  rowcol = XtVaCreateWidget( "control_rc",
                  xmRowColumnWidgetClass, control_frame,
                  XmNbackground, Bg_color,
                  XmNorientation, XmHORIZONTAL,
                  XmNpacking, XmPACK_TIGHT,
                  NULL );

  button = XtVaCreateManagedWidget( "Close",
                      xmPushButtonWidgetClass, rowcol,
                      XmNforeground, Button_fg_color,
                      XmNbackground, Button_bg_color,
                      XmNfontList, List_font,
                      XmNalignment, XmALIGNMENT_CENTER,
                      XmNmarginHeight, 2,
                      NULL );

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  XtVaCreateManagedWidget( "    ",
                 xmLabelWidgetClass, rowcol,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 NULL );

  button = XtVaCreateManagedWidget( "Comms Status",
                      xmPushButtonWidgetClass, rowcol,
                      XmNforeground, Button_fg_color,
                      XmNbackground, Button_bg_color,
                      XmNfontList, List_font,
                      XmNalignment, XmALIGNMENT_CENTER,
                      XmNmarginHeight, 2,
                      NULL );

  XtAddCallback( button, XmNactivateCallback, Comms_status_callback, NULL );

  XtVaCreateManagedWidget( "   ",
                 xmLabelWidgetClass, rowcol,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 NULL );

  Pwr_button = XtVaCreateManagedWidget( "Power Control",
                      xmPushButtonWidgetClass, rowcol,
                      XmNforeground, Button_fg_color,
                      XmNbackground, Button_bg_color,
                      XmNfontList, List_font,
                      XmNalignment, XmALIGNMENT_CENTER,
                      XmNmarginHeight, 2,
                      NULL );

  XtAddCallback( Pwr_button, XmNactivateCallback, Pwr_callback, NULL );

  XtVaCreateManagedWidget( "   ",
                 xmLabelWidgetClass, rowcol,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 NULL );

  RPG_button = XtVaCreateManagedWidget( "RPG HCI",
                      xmPushButtonWidgetClass, rowcol,
                      XmNforeground, Button_fg_color,
                      XmNbackground, Button_bg_color,
                      XmNfontList, List_font,
                      XmNalignment, XmALIGNMENT_CENTER,
                      XmNmarginHeight, 2,
                      NULL );

  XtAddCallback( RPG_button, XmNactivateCallback, RPG_callback, NULL );

  XtVaCreateManagedWidget( "   ",
                 xmLabelWidgetClass, rowcol,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 NULL );

  button = XtVaCreateManagedWidget( "RDA HCI",
                      xmPushButtonWidgetClass, rowcol,
                      XmNforeground, Button_fg_color,
                      XmNbackground, Button_bg_color,
                      XmNfontList, List_font,
                      XmNalignment, XmALIGNMENT_CENTER,
                      XmNmarginHeight, 2,
                      NULL );

  XtAddCallback( button, XmNactivateCallback, RDA_callback, NULL );

  if( Node_flag == HCI_MSCF_NODE )
  {
    XtVaSetValues( button, XmNsensitive, True, NULL );
  }
  else
  {
    XtVaSetValues( button, XmNsensitive, False, NULL );
  }

  XtVaCreateManagedWidget( "   ",
                 xmLabelWidgetClass, rowcol,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 NULL );

    /* If this is a MSCF with a Frame Relay HUB router, create button
       to load/config router if running on the MSCF. */

  if( Mscf_has_hub_router == YES )
  {
    button = XtVaCreateManagedWidget( "Configure HUB Router",
                      xmPushButtonWidgetClass, rowcol,
                      XmNforeground, Button_fg_color,
                      XmNbackground, Button_bg_color,
                      XmNfontList, List_font,
                      XmNalignment, XmALIGNMENT_CENTER,
                      XmNmarginHeight, 2,
                      NULL );

    XtAddCallback( button, XmNactivateCallback, HUB_rtr_callback, NULL );

    XtVaCreateManagedWidget( "   ",
                 xmLabelWidgetClass, rowcol,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 NULL );

    /* Only allow RDA HCI to be selected on a MSCF system. */
    if( Node_flag == HCI_MSCF_NODE )
    {
      XtVaSetValues( button, XmNsensitive, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNsensitive, False, NULL );
    }
  }

  XtManageChild( rowcol );

  /* Create Channel widget */

  channel_frame = XtVaCreateWidget( "channel_frame",
                      xmFrameWidgetClass, form,
                      XmNbackground, Bg_color,
                      XmNleftAttachment, XmATTACH_WIDGET,
                      XmNleftWidget, control_frame,
                      XmNtopAttachment, XmATTACH_FORM,
                      XmNrightAttachment, XmATTACH_FORM,
                      NULL );

  XtVaCreateManagedWidget( "Channels",
                 xmLabelWidgetClass, channel_frame,
                 XmNchildType, XmFRAME_TITLE_CHILD,
                 XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                 XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 NULL );

  /* Create radio box with user defined number of buttons */

  n = 0;
  XtSetArg( args[n], XmNforeground, Text_color );
  n++;
  XtSetArg( args[n], XmNbackground, Bg_color );
  n++;
  XtSetArg( args[n], XmNorientation, XmHORIZONTAL );
  n++;
  XtSetArg( args[n], XmNfontList, List_font );
  n++;

  channel_radio = XmCreateRadioBox( channel_frame, "channel_radio", args, n );
  for( i = 0; i < 2; i++ )
  {
    button = XtVaCreateManagedWidget( channel_labels[i],
                 xmToggleButtonWidgetClass, channel_radio,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNselectColor, White_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_CENTER,
                 NULL );
    if( i == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNvalueChangedCallback, Channel_callback, (XtPointer) i );
  }
  XtManageChild( channel_radio );
  XtVaSetValues( channel_radio, XmNmarginHeight, 5, NULL );

  /* Channel widget is only for FAA or NWSR sites */

  if( System_flag == HCI_FAA_SYSTEM )
  {
    sprintf( Title_buf, "%s (FAA:%1d)", MSCF_TITLE, Channel_number );
    XtManageChild( channel_frame );
  }
  else if( System_flag == HCI_NWSR_SYSTEM )
  {
    sprintf( Title_buf, "%s (NWS:%1d)", MSCF_TITLE, Channel_number );
    XtManageChild( channel_frame );
  }
  else
  {
    strcpy( Title_buf, MSCF_TITLE );
    XtVaSetValues( control_frame, XmNrightAttachment, XmATTACH_FORM, NULL );
  }
  XtVaSetValues( Top_widget, XmNtitle, Title_buf, NULL );


  /* Must be at least one network node to monitor to draw labels */

  if( Num_network_nodes > 0 )
  {
    net_status_frame = XtVaCreateManagedWidget( "net_status_frame",
                        xmFrameWidgetClass, form,
                        XmNbackground, Bg_color,
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, control_frame,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        NULL );

    XtVaCreateManagedWidget( "Network Connectivity",
                 xmLabelWidgetClass, net_status_frame,
                 XmNchildType, XmFRAME_TITLE_CHILD,
                 XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                 XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 NULL );

    net_status_rc = XtVaCreateWidget( "network_status_rc",
                  xmRowColumnWidgetClass, net_status_frame,
                  XmNbackground, Bg_color,
                  XmNorientation, XmHORIZONTAL,
                  XmNpacking, XmPACK_TIGHT,
                  NULL );

    /* Loop through and set label according to its node properties */

    for( i = 0; i < Num_network_nodes; i++ )
    {
      Net_node_widgets[i] = XtVaCreateManagedWidget( Nodes[i].name,
                 xmLabelWidgetClass, net_status_rc,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNfontList, List_font,
                 XmNalignment, XmALIGNMENT_CENTER,
                 NULL );
      XtVaSetValues( Net_node_widgets[i],
                     XmNborderWidth, 1,
                     XmNborderColor, Text_color,
                     NULL);
      Set_connectivity_status( i, MCS_UNKOWN );
    }

    XtManageChild( net_status_rc );
    XtManageChild( net_status_frame );
  }

  XtManageChild( form );

  /* Display GUI to screen. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/**************************************************************************
  Description:  Updates the "ind"-th node connectivity label.
**************************************************************************/

static void Set_connectivity_status( int ind, int status )
{
  switch( status )
  {
    case MCS_UNKOWN:
      XtVaSetValues( Net_node_widgets[ind], XmNbackground, Warn_color, NULL );
    break;

    case MCS_DISCONN:
      XtVaSetValues( Net_node_widgets[ind], XmNbackground, Alarm_color, NULL );
    break;

    case MCS_CONNECTED:
      XtVaSetValues( Net_node_widgets[ind], XmNbackground, Normal_color, NULL );
    break;
  }
}

/**************************************************************************
  Description: Callback for event timer.
**************************************************************************/

static void Timer_proc()
{
  if( Action_flag != NO_ACTION )
  {
    Action_flag = NO_ACTION;
    Perform_action();
  }

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
}

/**************************************************************************
  Description: Main window destroy callback function.
**************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "In destroy callback" );

  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    Close_coprocess();
  }

  return HCI_OK_TO_EXIT;
}

/**************************************************************************
  Description: Close button callback function.
**************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "In close button callback" );
  XtDestroyWidget( Top_widget );
}

/**************************************************************************
  Description: RPG button callback function.
**************************************************************************/

static void RPG_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "RPG button callback" );
  Action_flag = RPG_HCI_ACTION;
  sprintf( Cmd, "%s", Rpg_hci_cmd );
}

/**************************************************************************
  Description: RDA button callback function.
**************************************************************************/

static void RDA_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "RDA button callback" );
  Action_flag = RDA_HCI_ACTION;
  if( Channel_number == 1 ){ sprintf( Cmd, "%s", Rda_hci_cmd ); }
  else{ sprintf( Cmd, "%s", Rda_hci_cmd2 ); }
}

/**************************************************************************
  Description: Power Control button callback function.
**************************************************************************/

static void Pwr_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "Power control button callback" );
  Action_flag = POWER_CONTROL_HCI_ACTION;
  sprintf( Cmd, "mscf_power_control -A %d &", Channel_number );
}

/**************************************************************************
  Description: Comms Status button callback function.
**************************************************************************/

static void Comms_status_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "Comms_status button callback" );
  Action_flag = COMMS_STATUS_HCI_ACTION;
  sprintf( Cmd, "mscf_comms_status -A %d &", Channel_number );
}

/**************************************************************************
  Description: HUB router button callback function.
**************************************************************************/

static void HUB_rtr_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( 0, "HUB router button callback" );
  Action_flag = HUB_ROUTER_HCI_ACTION;
  sprintf( Cmd, "hci_configure_HUB_rtr -A %d &", Channel_number );
}

/**************************************************************************
  Description: Control buttons callback function.
**************************************************************************/

static void Perform_action()
{
  char buf[TEMP_BUF_SIZE];
  int ret;

  LE_send_msg( 0, "Execute command: %s", Cmd  );

  if( strlen( Cmd ) == 0 )
  {
    sprintf( buf, "No command specified" );
    hci_error_popup( Top_widget, buf, NULL );
    return;
  }
  else if( ( ret = MISC_system_to_buffer( Cmd, NULL, 0, NULL ) ) < 0 )
  {
    sprintf( buf, "MISC_system_to_buffer failed (%d)", ret );
    hci_error_popup( Top_widget, buf, NULL );
  }
  else if( strstr( Cmd, "&" ) == NULL && ret != 0 )
  {
    /* If not launched in background, then ret != 0
       means the command exited with error. If launched
       in background, ret is PID of process. */
    ret = ret >> 8;
    sprintf( buf, "Command %s failed (%d)", Cmd, ret );
    hci_error_popup( Top_widget, buf, NULL );
  }
}

/**************************************************************************
  Description:  Channel callback (FAA/NWSR only)
**************************************************************************/

static void Channel_callback( Widget w, XtPointer x, XtPointer y )
{
  XmToggleButtonCallbackStruct *state =
          (XmToggleButtonCallbackStruct*) y;

  /* Make sure user is setting new channel number */
  if( state->set )
  {
    /* Set new channel number. */
    Channel_number = ( (int)x + 1 );

    /* Clear data from previous channel. */
    Clear_data();

    /* Read configuration info for new channel. */
    if( Read_mscf_conf() < 0 ){ HCI_task_exit( HCI_EXIT_FAIL ); }

    /* Set various properties depending on new channel number */
    if( System_flag == HCI_FAA_SYSTEM )
    {
      sprintf( Title_buf, "%s (FAA:%1d)", MSCF_TITLE, Channel_number );
    }
    else
    {
      sprintf( Title_buf, "%s (NWS:%1d)", MSCF_TITLE, Channel_number );
      /* Assume NWS-R. Desensitize RPG and Power Control
         buttons on channel 2 */
      if( (int)x + 1 == 2 )
      {
        XtVaSetValues( RPG_button, XmNsensitive, False, NULL );
        XtVaSetValues( Pwr_button, XmNsensitive, False, NULL );
      }
      else
      {
        XtVaSetValues( RPG_button, XmNsensitive, True, NULL );
        XtVaSetValues( Pwr_button, XmNsensitive, True, NULL );
      }
    }
    XtVaSetValues( Top_widget, XmNtitle, Title_buf, NULL );
  }
}

/**************************************************************************
  Description: Reads the MSCF configuration file.
**************************************************************************/

static int Read_mscf_conf()
{
  char buf[NODE_NAME_SIZE];
  int cnt, n_local_ips, token_ind, i;
  unsigned int *ips, ip;

  /* Determine name of MSCF configuration file given channel number */
  strcpy( Mscf_conf_name, MSCF_DEFAULT_CONF_NAME );
  if( System_flag == HCI_FAA_SYSTEM && Channel_number == 2 )
  {
    strcpy( Mscf_conf_name, MSCF_CH2_CONF_NAME );
  }
  LE_send_msg( 0, "MSCF conf file set to %s", Mscf_conf_name );

  /* Set CS parameters and turn on CS error reporting */
  CS_cfg_name( Mscf_conf_name );
  CS_control( CS_COMMENT | '#' );
  CS_error( Print_cs_error );

  /* Read in variables from the MSCF configuration file */
  if( ( CS_entry( "Start_rpg_command:", 1,
                 MSCF_CMD_LEN, (void *)Rpg_hci_cmd ) <= 0 ) ||
      ( CS_entry( "Start_rda_command:", 1,
                 MSCF_CMD_LEN, (void *)Rda_hci_cmd ) <= 0 ) ||
      ( System_flag != HCI_NWS_SYSTEM &&
        CS_entry( "Start_rda_command2:", 1,
                 MSCF_CMD_LEN, (void *)Rda_hci_cmd2 ) <= 0 ) )
  {
    LE_send_msg( 0, "Reading MSCF config info failed for file %s", 
                 Mscf_conf_name );
    return -1;
  }

  /* Get list of known IP addresses and machine names */
  n_local_ips = NET_find_local_ip_address( &ips );

  token_ind = 1;
  cnt = 0;
  CS_control( CS_KEY_OPTIONAL );

  /* Read in remote nodes to monitor for connectivity */
  while( CS_entry( "Remote_nodes:", token_ind, NODE_NAME_SIZE, buf ) > 0 )
  {
    token_ind++;
    /* Ignore "network" label for now */
    if( strcmp( buf, "network" ) != 0 )
    {
      /* Make sure remote node has known associated IP address */
      ip = NET_get_ip_by_name( buf );
      for( i = 0; i < n_local_ips; i++ )
      {
        if( ips[i] == ip ){ break; } /* Node has known IP address */
      }
      if( i < n_local_ips ){ continue; } /* Ignore node if local machine */
    }
    if( cnt >= MAX_NET_NODES )
    {
      LE_send_msg( 0, "Too many nodes to be monitored" );
      CS_control( CS_KEY_REQUIRED );
      return -1;
    }
    Nodes[cnt].name = MISC_malloc( strlen( buf ) + 1 );
    strcpy( Nodes[cnt].name, buf );
    Nodes[cnt].status = MCS_UNKOWN;
    cnt++; /* Increment number of remote nodes to monitor */
  }

  CS_control( CS_KEY_REQUIRED );
  Num_network_nodes = cnt;

  /* Turn off CS error reporting. */
  CS_error( NULL );
  CS_cfg_name( "" );

  return 0;
}

/**************************************************************************
  Description:  Clear out all data structures
**************************************************************************/

static void Clear_data()
{
  strcpy( Mscf_conf_name, MSCF_DEFAULT_CONF_NAME );
  strcpy( Rpg_hci_cmd, "" );
  strcpy( Rda_hci_cmd, "" );
}

/**************************************************************************
  Description: This function reads command line arguments.
**************************************************************************/

static int Read_options( int argc, char **argv )
{
  extern char *optarg;
  extern int optind;
  int c;
  int verbose, err;

  verbose = 0;

  /* Set mscf configuration file to default value (channel 1) */
  strcpy( Mscf_conf_name, MSCF_DEFAULT_CONF_NAME );

  err = 0;
  while( ( c = getopt( argc, argv, "hf:v?" ) ) != EOF )
  {
    switch( c )
    {
      case 'f':
        /* Set mscf configuration file to user defined value */
        strncpy( Mscf_conf_name, optarg, MSCF_NAME_LEN );
        Mscf_conf_name[MSCF_NAME_LEN-1] = '\0';
      break;

      case 'v':
        verbose = 3;
      break;

      case 'h':
      case '?':
        Print_usage( argv );
        break;
    }
  }

  if( strlen( Mscf_conf_name ) == 0 )
  {
    LE_send_msg( 0, "Server LB name not defined" ) ;
    err = -1;
  }
  LE_local_vl( verbose );

  return err;
}

/**************************************************************************
  Description: This function prints the usage info.
**************************************************************************/

static void Print_usage( char **argv )
{
  printf( "Usage: %s (options)\n", argv[0] );
  printf( "       Options:\n" );
  printf( "       -f config_file_name (default: mscf1.conf)\n" );
  printf( "       -v (sets initial verbose level to 3. default: 0)\n" );
  HCI_task_exit( HCI_EXIT_SUCCESS );
}

/**************************************************************************
  Description: This function sends error messages while reading the link 
**************************************************************************/

static void Print_cs_error( char *msg )
{
  LE_send_msg( 0, "CS ERROR: %s", msg );
}

/**************************************************************************
  Description:  Initialize various site-specific variables.
**************************************************************************/

static void Initialize_site_info()
{
  int status = -1;

  /* Does this site have a HUB router at the MSCF? */

  status = MISC_system_to_buffer( "find_adapt -L", NULL, 0, NULL );

  status = status >> 8;

  if( status == 0 )
  {
    Mscf_has_hub_router = YES;
  }
  else
  {
    Mscf_has_hub_router = NO;
    if( status < 0 )
    {
      LE_send_msg( 0, "find_adapt -L failed (%d)", status );
    }
  }
}

/**************************************************************************
  Description: Starts coprocess to monitor connectivity.
**************************************************************************/

static void Start_coprocess()
{
  int ret, i;
  char buf[TEMP_BUF_SIZE];

  if( ( ret = MISC_cp_open( "mping -p 3", 0, &Cp ) ) < 0 )
  {
    LE_send_msg( 0, "MISC_cp_open mping failed (%d)", ret );
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }

  for( i = 0; i < Num_network_nodes; i++ )
  {
    /* Skip local node. */
    if( strcmp( Nodes[i].name, "network" ) == 0 ){ continue; }
    /* Add nodes to monitor. */
    sprintf( buf, "++%s\n", Nodes[i].name );
    if( Cp != NULL && ( ( ret = MISC_cp_write_to_cp( Cp, buf ) ) < 0 ) )
    {
      LE_send_msg( 0, "Cp write failed (%d) for %s", ret, Nodes[i].name );
      Close_coprocess();
      Coprocess_flag = HCI_CP_FINISHED;
      return;
    }
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
      LE_send_msg( 0, "ERROR mping: (%s)", cp_buf );
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
  int i, qtime, ccnt, dcnt;
  char *t = NULL;
  char *p = NULL;

  /* Initialize to unknown */
  for( i = 0; i < Num_network_nodes; i++ )
  {
    Nodes[i].old = Nodes[i].status;
    Nodes[i].status = MCS_UNKOWN;
  }

  /* Parse status string returned from mping */
  ccnt = dcnt = 0;
  t = strtok (status, " \t\n");
  while( t != NULL )
  {
    p = strstr( t, "--" );
    if( p == NULL || sscanf( p + 2, "%d", &qtime ) != 1 )
    {
      LE_send_msg( 0, "Unexpected mping token (%s)", t );
    }
    else
    {
      *p = '\0';
      for( i = 0; i < Num_network_nodes; i++ )
      {
        if( strcmp( Nodes[i].name, t ) == 0 )
        {
          if( qtime == 0 )
          {
            Nodes[i].status = MCS_CONNECTED;
            ccnt++; /* Increment number of connected nodes */
          }
          else if( qtime > 0 )
          {
            Nodes[i].status = MCS_DISCONN;
            dcnt++; /* Increment number of disconnected nodes */
          }
          break;
        }
      }
    }
    t = strtok( NULL, " \t\n" );
  }

  /* Determine and set value of "network" label */
  for( i = 0; i < Num_network_nodes; i++ )
  {
    if( strcmp( Nodes[i].name, "network" ) == 0 )
    {
      if( ccnt > 0 ){ Nodes[i].status = MCS_CONNECTED; }
      else if( dcnt > 0 ){ Nodes[i].status = MCS_DISCONN; }
    }
    /* Only change label if value has changed. */
    if( Nodes[i].status != Nodes[i].old )
    {
      Set_connectivity_status( i, Nodes[i].status );
    }
  }
}

