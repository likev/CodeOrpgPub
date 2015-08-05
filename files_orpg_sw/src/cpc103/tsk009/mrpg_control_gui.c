/************************************************************************
 *      Module:  mrpg_control_gui.c                                     *
 *                                                                      *
 *      Description:  This task allows a user to command a restart	*
 *		      of the RPG software. It is designed to work	*
 *		      regardless if mrpg is running.			*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/25 16:00:18 $
 * $Id: mrpg_control_gui.c,v 1.12 2012/01/25 16:00:18 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/* Include files. */

#include <hci.h>

/*      RPG Command macros      */

enum { MSCF, RPGA1, RPGA2, RPGB1, RPGB2, NUM_NODES };

#define	COPROCESS_BUF		512
#define	MAX_LINE_BUF		512
#define	BUF_SIZE		128
#define	POPUP_BUF_SIZE		COPROCESS_BUF + BUF_SIZE
#define	MAX_NUM_LABELS		30
#define	SCROLL_WIDTH		550
#define	SCROLL_HEIGHT		400
#define	LOG_RESTART_TIMER	5
#define	MAX_NODE_LENGTH		4
#define	MAX_IP_LENGTH		64
#define	GET_IP_ERROR_FLAG	-1
#define	MAX_SKIP_LOG		30

/*	Struct/typedefs		*/

typedef struct IP_info {
  int  channel;
  char node[MAX_NODE_LENGTH+1];
  int  active;
  char IP[MAX_IP_LENGTH+1];
} IP_info_t;

/*      Global widget definitions. */

static	Widget	Top_widget = (Widget) NULL;
static	Widget	Close_button = (Widget) NULL;
static	Widget	Restart_button = (Widget) NULL;
static	Widget	Channel_toggle = (Widget) NULL;
static	Widget	Info_scroll = (Widget) NULL;
static	Widget	Info_scroll_form = (Widget) NULL;
static	Widget	Msg_labels[MAX_NUM_LABELS];
static	Pixel	Bg_color = (Pixel) NULL;
static	Pixel	Button_bg_color = (Pixel) NULL;
static	Pixel	Button_fg_color = (Pixel) NULL;
static	Pixel	Text_fg_color = (Pixel) NULL;
static	Pixel	White_color = (Pixel) NULL;
static	Pixel	Warning_color = (Pixel) NULL;
static  XmFontList	Font_list = (XmFontList) NULL;

/* Global variables. */

static	void	*Cp = NULL;
static	int	Coprocess_flag = HCI_CP_NOT_STARTED;
static	char	Popup_buf[POPUP_BUF_SIZE+1];
static	char	Output_msgs[MAX_NUM_LABELS][MAX_LINE_BUF+1];
static	int	Current_msg_pos = -1;
static	int	Num_output_msgs = 0;
static	int	Local_node_flag = -1;
static	int	Local_node_index = -1;
static	int	Target_node_index = -1;
static	int	Local_channel_number = -1;
static	int	Local_system_flag = HCI_NWS_SYSTEM;
static	int	Busy_mode_flag = NO;
static	int	Selected_channel_number = 1;
static	int	RSSD_running_locally_flag = NO;
static	int	Channel_change_flag = NO;
static	int	Restart_flag = NO;
static	int	Verify_connectivity_flag = NO;
static	int	Confirm_restart_flag = NO;
static	int	Waiting_on_confirmation_flag = NO;
static	int	Restart_confirmed_flag = NO;
static	IP_info_t IP_info[NUM_NODES];
static	char	Cp_open_cmd[BUF_SIZE];
static	char	Cp_read_cmd[BUF_SIZE];
static	char	Cp_status_cmd[BUF_SIZE];
static	char	Cp_close_cmd[BUF_SIZE];

/* Function prototypes. */

static	int	Destroy_callback( Widget, XtPointer, XtPointer );
static	void	Close_callback( Widget, XtPointer, XtPointer );
static	void	Restart_button_cb( Widget, XtPointer, XtPointer );
static	void	Accept_restart_cb( Widget, XtPointer, XtPointer );
static	void	Cancel_restart_cb( Widget, XtPointer, XtPointer );
static	void	Channel_toggle_cb( Widget, XtPointer, XtPointer );
static	void	Update_scroll_display();
static	void	Reset_scroll_display();
static	void	Update_label( Widget, int );
static	void	Start_coprocess();
static	void	Manage_coprocess();
static	void	Close_coprocess();
static	void	Timer_proc();
static	void	Initialize_IP_info();
static	void	Verify_connectivity();
static	void	Confirm_restart_command();
static	int	No_action_pending();
static	void	Set_busy_mode();
static	void	Set_normal_mode();

/**************************************************************************
  Description: The main function for the MRPG Control HCI.
**************************************************************************/

int main( int argc, char *argv[] )
{
  Widget	form;
  Widget	form_rowcol;
  Widget	control_rowcol;
  Widget	system_rowcol;
  Widget	button;
  Widget	rowcol;
  Widget	clip;
  int		loop = 0;
  int		n = 0;
  Arg		args[10];
  
  /* Initialize GUI */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, "RPG Software Restart", NULL );

  Local_node_flag = HCI_get_node();

  Local_system_flag = HCI_get_system();

  if( ( Local_channel_number = HCI_get_channel_number() ) != 2 )
  {
    Local_channel_number = 1;
  }

  Bg_color = hci_get_read_color( BACKGROUND_COLOR1 );
  Button_bg_color = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg_color = hci_get_read_color( BUTTON_FOREGROUND );
  Text_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  Warning_color = hci_get_read_color( WARNING_COLOR );
  Font_list = hci_get_fontlist( LIST );
  Initialize_IP_info();

  /* Use a form widget and rowcol to organize the various menu widgets. */

  form = XtVaCreateManagedWidget( "form",
                xmFormWidgetClass, Top_widget,
                XmNautoUnmanage,   False,
                XmNbackground,     Bg_color,
                NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground,          Bg_color,
                XmNorientation,         XmVERTICAL,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                NULL);
  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNbackground,          Bg_color,
                XmNorientation,         XmHORIZONTAL,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                XmNisAligned,           False,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL );

  Close_button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, control_rowcol,
                XmNbackground,           Button_bg_color,
                XmNforeground,           Button_fg_color,
                XmNfontList,             Font_list,
                NULL );

  XtAddCallback( Close_button, XmNactivateCallback, Close_callback, NULL );

  if( Local_system_flag == HCI_FAA_SYSTEM )
  {
    XtVaCreateManagedWidget ("                    Channel:",
                xmLabelWidgetClass, control_rowcol,
                XmNbackground,      Bg_color,
                XmNforeground,      Text_fg_color,
                XmNfontList,        Font_list,
                NULL);  

    /* Set toggle button parameters. */

    n = 0;
    XtSetArg( args [n], XmNforeground, Text_fg_color );
    n++;
    XtSetArg( args [n], XmNbackground, Bg_color );
    n++;
    XtSetArg( args [n], XmNfontList, Font_list );
    n++;
    XtSetArg( args [n], XmNpacking, XmPACK_TIGHT);
    n++;
    XtSetArg( args [n], XmNorientation, XmHORIZONTAL );
    n++;

    Channel_toggle = XmCreateRadioBox( control_rowcol, "toggle", args, n );

    button = XtVaCreateManagedWidget( "1",
                 xmToggleButtonWidgetClass, Channel_toggle,
                 XmNselectColor, White_color,
                 XmNforeground,  Text_fg_color,
                 XmNbackground,  Bg_color,
                 XmNfontList,    Font_list,
                 NULL );

    XtAddCallback( button, XmNarmCallback, Channel_toggle_cb, (XtPointer) 1 );

    if( Local_channel_number == 1 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    button = XtVaCreateManagedWidget( "2",
                 xmToggleButtonWidgetClass, Channel_toggle,
                 XmNselectColor, White_color,
                 XmNforeground,  Text_fg_color,
                 XmNbackground,  Bg_color,
                 XmNfontList,    Font_list,
                 NULL );

    XtAddCallback( button, XmNarmCallback, Channel_toggle_cb, (XtPointer) 2 );

    if( Local_channel_number == 2 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtManageChild( Channel_toggle );
  
    XtVaCreateManagedWidget ("                  ",
                xmLabelWidgetClass, control_rowcol,
                XmNbackground,      Bg_color,
                XmNforeground,      Text_fg_color,
                XmNfontList,        Font_list,
                NULL);  
  }
  else
  {
    XtVaCreateManagedWidget ("                                                         ",
                xmLabelWidgetClass, control_rowcol,
                XmNbackground,      Bg_color,
                XmNforeground,      Text_fg_color,
                XmNfontList,        Font_list,
                NULL);  
  }

  /* Create a container to hold the restart button and output display. */

  system_rowcol = XtVaCreateManagedWidget ("system_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNorientation,         XmVERTICAL,
                XmNbackground,          Bg_color,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                NULL);

  rowcol = XtVaCreateManagedWidget ("rowcol",
                xmRowColumnWidgetClass, system_rowcol,
                XmNorientation,         XmHORIZONTAL,
                XmNbackground,          Bg_color,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                NULL);

  XtVaCreateManagedWidget( "                           ",
                xmLabelWidgetClass, rowcol,
                XmNbackground,      Bg_color,
                XmNbackground,      Bg_color,
                NULL );

  Restart_button = XtVaCreateManagedWidget (" RPG Software Restart ",
                xmPushButtonWidgetClass, rowcol,
                XmNforeground,           Button_fg_color,
                XmNbackground,           Button_bg_color,
                 XmNfontList,            Font_list,
                NULL);

  XtAddCallback( Restart_button,
                XmNactivateCallback, Restart_button_cb, NULL );

  XtVaCreateManagedWidget( "      ",
                xmLabelWidgetClass, rowcol,
                XmNforeground,      Bg_color,
                XmNbackground,      Bg_color,
                NULL );

  XtVaCreateManagedWidget( "                  Log Output Of RPG Restart",
                xmLabelWidgetClass, system_rowcol,
                XmNforeground,      Text_fg_color,
                XmNbackground,      Bg_color,
                 XmNfontList,       Font_list,
                NULL );

  /* Create scroll area to display MRPG output. */

  Info_scroll = XtVaCreateManagedWidget ("data_scroll",
                xmScrolledWindowWidgetClass,    system_rowcol,
                XmNheight,              SCROLL_HEIGHT,
                XmNwidth,               SCROLL_WIDTH,
                XmNscrollingPolicy,     XmAUTOMATIC,
                XmNforeground,          Bg_color,
                XmNbackground,          Bg_color,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           rowcol,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

  XtVaGetValues( Info_scroll, XmNclipWindow, &clip, NULL );

  XtVaSetValues( clip, XmNbackground, Bg_color, NULL );

  Info_scroll_form = XtVaCreateManagedWidget( "scroll_form",
       xmFormWidgetClass,  Info_scroll,
       XmNforeground,      Bg_color,
       XmNbackground,      Bg_color,
       XmNverticalSpacing, 1,
       NULL );

  rowcol = XtVaCreateManagedWidget( "rowcol",
          xmRowColumnWidgetClass, Info_scroll_form,
          XmNforeground,          Bg_color,
          XmNbackground,          Bg_color,
          XmNorientation,         XmVERTICAL,
          XmNpacking,             XmPACK_COLUMN,
          XmNnumColumns,          1,
          XmNleftAttachment,      XmATTACH_FORM,
          XmNrightAttachment,     XmATTACH_FORM,
          NULL );

  Msg_labels[0] = XtVaCreateManagedWidget ("Waiting for output from RPG restart",
                        xmLabelWidgetClass, rowcol,
                        XmNfontList,        Font_list,
                        XmNleftAttachment,  XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNtopAttachment,   XmATTACH_FORM,
                        XmNmarginHeight,    0,
                        XmNborderWidth,     0,
                        XmNshadowThickness, 0,
                        XmNforeground,      Text_fg_color,
                        XmNbackground,      Bg_color,
                        NULL);

  for( loop = 1; loop < MAX_NUM_LABELS; loop++ )
  {
        Msg_labels[loop] = XtVaCreateManagedWidget (" ",
                xmLabelWidgetClass, rowcol,
                 XmNfontList,       Font_list,
                XmNleftAttachment,  XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNtopAttachment,   XmATTACH_WIDGET,
                XmNtopWidget,       Msg_labels [loop-1],
                XmNmarginHeight,    0,
                XmNborderWidth,     0,
                XmNshadowThickness, 0,
                XmNforeground,      Text_fg_color,
                XmNbackground,      Bg_color,
                NULL);
  }

  /* Display gui to screen. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_LE_log( "Entering main loop" );

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/**************************************************************************
  Description: The timer proc callback.
**************************************************************************/

static void Timer_proc()
{
  if( Channel_change_flag == YES )
  {
    Channel_change_flag = NO;
    Reset_scroll_display();
  }
  else if( Restart_flag == YES )
  {
    Restart_flag = NO;
    Reset_scroll_display();
    Verify_connectivity_flag = YES;
  }
  else if( Verify_connectivity_flag == YES )
  {
    Verify_connectivity_flag = NO;
    Verify_connectivity();
    Confirm_restart_flag = YES;
  }
  else if( Confirm_restart_flag == YES )
  {
    Confirm_restart_flag = NO;
    Confirm_restart_command();
  }
  else if( Waiting_on_confirmation_flag == YES )
  {
    /* Waiting on user to confirm restart. Do nothing. */
  }
  else if( Restart_confirmed_flag == YES &&
           Coprocess_flag == HCI_CP_NOT_STARTED )
  {
    Restart_confirmed_flag = NO;
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

  /* Reset cursor if no pending actions. */

  if( No_action_pending() && Busy_mode_flag == YES )
  {
    Set_normal_mode();
  }
}

/**************************************************************************
  Description: The destroy callback.
**************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Destroy HCI" );
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    /* Don't exit until the coprocess is terminated. */
    strcpy( Popup_buf, "You are not allowed to exit this task\n" );
    strcat( Popup_buf, "until the restart process is finished." );
    hci_warning_popup( Top_widget, Popup_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/**************************************************************************
  Description: The "Close" button callback.
**************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Close HCI" );
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    /* Don't exit until the coprocess is terminated. */
    strcpy( Popup_buf, "You are not allowed to exit this task\n" );
    strcat( Popup_buf, "until the restart process is finished." );
    hci_warning_popup( Top_widget, Popup_buf, NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/**************************************************************************
  Description: This function is activated when the user selects the
               RPG Software Restart button.
**************************************************************************/

static void Restart_button_cb( Widget w, XtPointer x, XtPointer y )
{
  Restart_flag = YES;
  Set_busy_mode();
}

/**************************************************************************
  Description: This function initializes IP information for local and
               remote nodes.
**************************************************************************/

static void Initialize_IP_info()
{
  int i = 0;

  for( i = 0; i < NUM_NODES; i++ )
  {
    if( i == MSCF )
    {
      IP_info[i].channel = 1;
      strcpy( IP_info[i].node, "MSCF" );
      if( Local_node_flag == HCI_MSCF_NODE )
      {
        Local_node_index = i;
      }
    }
    else if( i == RPGA1 )
    {
      IP_info[i].channel = 1;
      strcpy( IP_info[i].node, "RPGA" );
      if( Local_node_flag == HCI_RPGA_NODE && Local_channel_number == 1 )
      {
        Local_node_index = i;
      }
    }
    else if( i == RPGA2 )
    {
      IP_info[i].channel = 2;
      strcpy( IP_info[i].node, "RPGA" );
      if( Local_node_flag == HCI_RPGA_NODE && Local_channel_number == 2 )
      {
        Local_node_index = i;
      }
    }
    else if( i == RPGB1 )
    {
      IP_info[i].channel = 1;
      strcpy( IP_info[i].node, "RPGB" );
      if( Local_node_flag == HCI_RPGB_NODE && Local_channel_number == 1 )
      {
        Local_node_index = i;
      }
    }
    else if( i == RPGB2 )
    {
      IP_info[i].channel = 2;
      strcpy( IP_info[i].node, "RPGB" );
      if( Local_node_flag == HCI_RPGB_NODE && Local_channel_number == 2 )
      {
        Local_node_index = i;
      }
    }
  }
}

/**************************************************************************
  Description: This function verifies connectivity of IPs.
**************************************************************************/

static void Verify_connectivity()
{
  int i = 0;
  char ip_buf[MAX_IP_LENGTH];
  int ret = 0;
  int error_count = 0;

  EN_cntl_unblock();
  for( i = 0; i < NUM_NODES; i++ )
  {
    ret = ORPGMGR_discover_host_ip( IP_info[i].node, IP_info[i].channel, ip_buf, MAX_IP_LENGTH );

    if( ret == 1 )
    {
      /* Node found. */
      IP_info[i].active = YES;
      strncpy( IP_info[i].IP, ip_buf, MAX_IP_LENGTH );
      if( strlen( ip_buf ) >= MAX_IP_LENGTH )
      {
        IP_info[i].IP[MAX_IP_LENGTH] = '\0';
      }
    }
    else if( ret == 0 )
    {
      /* Node not found. */
      IP_info[i].active = NO;
      strcpy( IP_info[i].IP, "" );
    }
    else if( ret < 0 )
    {
      /* Error. */
      IP_info[i].active = GET_IP_ERROR_FLAG;
      strcpy( IP_info[i].IP, "" );
      HCI_LE_error( "discover host ip error (%d) for %s%d", ret, IP_info[i].node, IP_info[i].channel );
      error_count++;
    }
  }
  EN_cntl_block();

  if( error_count == NUM_NODES )
  {
    /* If ORPGMGR_discover_host_ip fails for
       each node, can only conclude that RSSD
       is not running on the local node. */
    RSSD_running_locally_flag = NO;
  }
  else
  {
    RSSD_running_locally_flag = YES;
  }
}

/**************************************************************************
  Description: This function confirms the user wants to restart software.
**************************************************************************/

static void Confirm_restart_command()
{
  /* RSSD needs to be running on the local machine to command
     a RPG restart on a remote system. If the RPG restart is
     on the local system, then RSSD running is not a requirement. */

  if( Selected_channel_number == 1 ){ Target_node_index = RPGA1; }
  else{ Target_node_index = RPGA2; }

  if( RSSD_running_locally_flag == NO &&
      Local_node_index != Target_node_index )
  {
    /* RSSD not running on the local machine, and the local
       machine is not the main RPG node for the selected channel */
    sprintf( Popup_buf, "The RPG internal communication software is not\nrunning on the local machine. Unable to command a\nRPG Software Restart. A reboot of the local machine\nmay fix the problem. If the problem continues, call\nthe WSR-88D Hotline." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    sprintf( Popup_buf, "Unable to verify RSSD running locally." );
    HCI_LE_error( Popup_buf );
  }
  else
  {
    Waiting_on_confirmation_flag = YES;
    sprintf (Popup_buf,"You are about to restart all of the RPG processes.\nThis process could take a few minutes to complete.\n\nDo you want to continue?");
    hci_confirm_popup( Top_widget, Popup_buf, Accept_restart_cb, Cancel_restart_cb );
  }
}

/**************************************************************************
  Description: This function is activated when the user selects the "Yes"
               button from the restart confirmation popup window.
**************************************************************************/

static void Accept_restart_cb( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "User confirmed RPG Software Restart" );
  Waiting_on_confirmation_flag = NO;
  Restart_confirmed_flag = YES;
}

/**************************************************************************
  Description: This function is activated when the user selects the "No"
               button from the restart confirmation popup window.
**************************************************************************/

static void Cancel_restart_cb( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "User cancelled RPG Software Restart" );
  Waiting_on_confirmation_flag = NO;
}

/**************************************************************************
  Description: This function starts the coprocess.
**************************************************************************/

static void Start_coprocess()
{
  char rpg_cmd[BUF_SIZE];
  int rpc_ret = -1;
  int cmd_ret = -1;

  if( IP_info[Target_node_index].active == NO )
  {
    sprintf( Popup_buf, "Unable to establish connectivity to target\nRPG node. Unable to command restart." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }

  /* Determine RPG command to run. */

  if( ORPGMISC_is_operational() &&
      Local_node_index == Target_node_index &&
      RSSD_running_locally_flag == NO )
  {
    sprintf( rpg_cmd, "/bin/sh -c \"start_rssd; mrpg startup\"" );
  }
  else
  {
    sprintf( rpg_cmd, "/bin/sh -c \"mrpg startup\"" );
  }

  /* Determine MISC_cp command to run. */

  if( ORPGMISC_is_operational() &&
      Local_node_index != Target_node_index )
  {
    sprintf( Cp_open_cmd, "%s:MISC_cp_open", IP_info[Target_node_index].IP );
    sprintf( Cp_read_cmd, "%s:MISC_cp_read_from_cp", IP_info[Target_node_index].IP );
    sprintf( Cp_status_cmd, "%s:MISC_cp_get_status", IP_info[Target_node_index].IP );
    sprintf( Cp_close_cmd, "%s:MISC_cp_close", IP_info[Target_node_index].IP );
  }
  else
  {
    sprintf( Cp_open_cmd, "MISC_cp_open" );
    sprintf( Cp_read_cmd, "MISC_cp_read_from_cp" );
    sprintf( Cp_status_cmd, "MISC_cp_get_status" );
    sprintf( Cp_close_cmd, "MISC_cp_close" );
  }

  /* Start co-process via RSS_rpc. */

  HCI_LE_log( "Starting coprocess %s %s", rpg_cmd, Cp_open_cmd );

  rpc_ret = RSS_rpc( Cp_open_cmd, "i-r s-i i-i ba-4-o",
                 &cmd_ret, rpg_cmd, MISC_CP_MANAGE, &Cp );

  if( rpc_ret != 0 )
  {
    sprintf( Popup_buf, "RSS_rpc failed (%d).\nUnable to restart RPG.", rpc_ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }
  else if( cmd_ret != 0 )
  {
    sprintf( Popup_buf, "%s failed (%d).\nUnable to restart RPG.",
             Cp_open_cmd, cmd_ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }

  Coprocess_flag = HCI_CP_STARTED;
}

/**************************************************************************
  Description: This function manages the coprocess.
**************************************************************************/

static void Manage_coprocess()
{
  int rpc_ret = 0;
  int cmd_ret = -1;
  char cp_buf[COPROCESS_BUF];
  int new_msgs = NO;

  while( Cp != NULL && rpc_ret == 0 && cmd_ret != 0 )
  {
    rpc_ret = RSS_rpc( Cp_read_cmd, "i-r p-i ba-%d-o i-i",
                   COPROCESS_BUF, &cmd_ret, Cp, cp_buf, COPROCESS_BUF );

    if( rpc_ret != 0 )
    {
      sprintf( Popup_buf, "RSS_rpc failed (%d).\nRPG restart failed.", rpc_ret );
      hci_error_popup( Top_widget, Popup_buf, NULL );
      Close_coprocess();
    }
    else if( cmd_ret != 0 )
    {
      /* Strip trailing newline. */

      if( cp_buf[strlen( cp_buf )-1] == '\n' )
      {
        cp_buf[strlen( cp_buf )-1] = '\0';
      }

      /* MISC function succeeded. Take appropriate action. */

      if( cmd_ret == HCI_CP_DOWN )
      {
        /* Coprocess finished, check exit status. */
        rpc_ret = RSS_rpc( Cp_status_cmd, "i-r p-i", &cmd_ret, Cp );
        if( rpc_ret != 0 )
        {
          sprintf( Popup_buf, "RSS_rpc failed (%d).\nRPG restart finished, but unable to retrieve status", rpc_ret );
          hci_error_popup( Top_widget, Popup_buf, NULL );
        }
        else if( cmd_ret != 0 )
        {
          /* Exit status indicates an error. */
          sprintf( Popup_buf, "RPG restart failed (%d)", cmd_ret );
          hci_error_popup( Top_widget, Popup_buf, NULL );
          HCI_LE_error( "%s", Popup_buf );
        }
        else
        {
          /* Exit status indicates success. */
          sprintf( Popup_buf, "RPG restart succeeded" );
          hci_info_popup( Top_widget, Popup_buf, NULL );
          HCI_LE_log( "%s", Popup_buf );
        }
        Close_coprocess();
        cmd_ret = 0;
      }
      else if( cmd_ret == HCI_CP_STDOUT || cmd_ret == HCI_CP_STDERR )
      {
        HCI_LE_log( "cp_buf: %s", cp_buf );
        /* Set flag to indicate new messages to display. */
        new_msgs = YES;
        /* Update starting label position (circular array). */
        Current_msg_pos++;
        Current_msg_pos%=MAX_NUM_LABELS;
        /* Save coprocess output for display. */
        sprintf( Output_msgs[Current_msg_pos], "%s", cp_buf );
        /* Update number of labels (not to exceed maximum number allowed). */
        Num_output_msgs++;
        if( Num_output_msgs > MAX_NUM_LABELS ){ Num_output_msgs--; }
      }
      else
      {
        /* Do nothing. */
      }
    }
  }

  /* If new messages available, update scroll with latest info. */

  if( new_msgs )
  {
    Update_scroll_display();
  }
}

/**************************************************************************
  Description: Close coprocess.
**************************************************************************/

static void Close_coprocess()
{
  HCI_LE_log( "Close coprocess" );
  RSS_rpc( Cp_close_cmd, "p-i", Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/**************************************************************************
  Description: This function updates the scroll with new output.
**************************************************************************/

static void Update_scroll_display()
{
  int loop = -1;
  int temp_int = -1;

  /* The latest message is located at position Current_msg_pos.
     Start there and work down to index 0. */

  temp_int = 0;
  for( loop = Current_msg_pos; loop >= 0; loop-- )
  {
    Update_label( Msg_labels[loop], temp_int );
    temp_int++;
  }

  /* If the number of output messages has reached the maximum
     allowed amount, then start at the last array position and
     work down to the index just above the Current_msg_pos index.
     This is done to allow the array to be treated as circular. */

  if( Num_output_msgs == MAX_NUM_LABELS )
  {
    for( loop = MAX_NUM_LABELS - 1; loop > Current_msg_pos; loop-- )
    {
      Update_label( Msg_labels[loop], temp_int );
      temp_int++;
    }
  }
}

/**************************************************************************
  Description: This function resets the scroll display.
**************************************************************************/

static void Reset_scroll_display()
{
  int i = -1;
  XmString msg;

  /* Reset positional indices. */

  Current_msg_pos = -1;
  Num_output_msgs = 0;

  /* Change first label to default message. */

  sprintf( Output_msgs[0], "Waiting for output from RPG restart." );
  msg = XmStringCreateLtoR( Output_msgs[0], XmFONTLIST_DEFAULT_TAG );
  XtVaSetValues( Msg_labels[0],
                 XmNlabelString, msg,
                 XmNbackground,  Bg_color,
                 NULL );
  XmStringFree( msg );

  /* Clear rest of labels. */

  for( i = 1; i < MAX_NUM_LABELS; i++ )
  {
    strcpy( Output_msgs[i], "" );
    msg = XmStringCreateLtoR( Output_msgs[i], XmFONTLIST_DEFAULT_TAG );
    XtVaSetValues( Msg_labels[i],
                   XmNlabelString, msg,
                   XmNbackground,  Bg_color,
                   NULL );
    XmStringFree( msg );
  }
}

/**************************************************************************
  Description: This function updates a label with appropriate value/color.
**************************************************************************/

static void Update_label( Widget w, int index )
{
  /* Convert c-buf to XmString variable. */

  XmString msg = XmStringCreateLtoR( Output_msgs[index], XmFONTLIST_DEFAULT_TAG );

  /* Modify appropriate output label. */

  XtVaSetValues( w, XmNlabelString, msg, XmNbackground, Bg_color, NULL );

  /* Free allocated space. */

  XmStringFree( msg );
}

/**************************************************************************
  Description:  Set info according to channel selected.
**************************************************************************/

static void Channel_toggle_cb( Widget w, XtPointer x, XtPointer y )
{
  if( (int) x != Selected_channel_number )
  {
    Selected_channel_number = (int) x;
    HCI_LE_log( "User changed to channel %d", Selected_channel_number );
    Channel_change_flag = YES;
  }
}

/**************************************************************************
  Description:  Set cursor and widgets to indicate busy mode and no user
                interaction is allowed.
**************************************************************************/

static void Set_busy_mode()
{
  Busy_mode_flag = YES;
  HCI_busy_cursor();
  XtSetSensitive( Restart_button, False );
  XtSetSensitive( Close_button, False );
  if( Local_system_flag == HCI_FAA_SYSTEM )
  {
    XtSetSensitive( Channel_toggle, False );
  }
}

/**************************************************************************
  Description:  Set cursor and widgets to indicate non-busy mode and user
                interaction is allowed.
**************************************************************************/

static void Set_normal_mode()
{
  Busy_mode_flag = NO;
  HCI_default_cursor();
  XtSetSensitive( Restart_button, True );
  XtSetSensitive( Close_button, True );
  if( Local_system_flag == HCI_FAA_SYSTEM )
  {
    XtSetSensitive( Channel_toggle, True );
  }
}

/**************************************************************************
  Description:  Determines if any actions are left to process by checking
                action flag variables.
**************************************************************************/

static int No_action_pending()
{
  if( Channel_change_flag == YES ||
      Restart_flag == YES  ||
      Coprocess_flag != HCI_CP_NOT_STARTED ||
      Verify_connectivity_flag == YES ||
      Waiting_on_confirmation_flag == YES ||
      Confirm_restart_flag == YES ||
      Restart_confirmed_flag == YES )
  {
    return NO;
  }

  return YES;
}

